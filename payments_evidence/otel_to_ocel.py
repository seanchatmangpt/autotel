"""Map OpenTelemetry-style span dicts about payment effects to OCEL 2.0 and
classify settlement state from telemetry alone.

Telemetry is evidence, not authority: this module never marks anything SETTLED
unless a span carries an observed-finality attribute, and never treats a
timeout as permission to retry.

Span attribute vocabulary (non-authoritative, project-local):
  payments.effect_id, payments.obligation_id, payments.idempotency_key,
  payments.rail_correlation_id, payments.phase, payments.finality_observed,
  payments.reconciliation_result
Phases: prepare, submit, ack, status_accepted, status_rejected, timeout,
        reconcile, settled, returned, reversed.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Iterable

A = "payments."
PHASES = {
    "prepare", "submit", "ack", "status_accepted", "status_rejected",
    "timeout", "reconcile", "settled", "returned", "reversed",
}
CHAIN_KEYS = ("obligation_id", "idempotency_key", "rail_correlation_id")


@dataclass
class Finding:
    code: str       # BLIND_RETRY | SETTLED_WITHOUT_FINALITY | CHAIN_NOT_JOINABLE | UNKNOWN_PHASE
    falsifier: str  # F3 | F5 | F4
    effect_id: str
    detail: str


@dataclass
class EffectView:
    effect_id: str
    state: str = "PREPARED"
    action: str = "NONE"  # RECONCILE when outcome unknown
    retry_eligible: bool = False
    span_ids: list[str] = field(default_factory=list)


def _attr(span: dict, key: str) -> Any:
    return (span.get("attributes") or {}).get(A + key)


def _ordered(spans: Iterable[dict]) -> list[dict]:
    return sorted(spans, key=lambda s: (s.get("start_time_unix_nano", 0), s.get("span_id", "")))


def analyze(spans: Iterable[dict]) -> tuple[dict[str, EffectView], list[Finding]]:
    views: dict[str, EffectView] = {}
    findings: list[Finding] = []
    seen_chain: dict[str, dict[str, set]] = {}
    submits: dict[str, int] = {}
    for sp in _ordered(spans):
        eid = _attr(sp, "effect_id")
        if not eid:
            continue
        v = views.setdefault(eid, EffectView(eid))
        v.span_ids.append(sp.get("span_id", ""))
        for k in CHAIN_KEYS:
            val = _attr(sp, k)
            if val is not None:
                seen_chain.setdefault(eid, {}).setdefault(k, set()).add(val)
        ph = _attr(sp, "phase")
        if ph not in PHASES:
            findings.append(Finding("UNKNOWN_PHASE", "F4", eid, f"phase={ph!r}"))
            continue
        if ph == "prepare":
            v.state = "PREPARED"
        elif ph == "submit":
            submits[eid] = submits.get(eid, 0) + 1
            if v.state in ("UNKNOWN",) and not v.retry_eligible:
                findings.append(Finding("BLIND_RETRY", "F3", eid,
                                        "submit after UNKNOWN without reconciliation proving non-acceptance"))
            if submits[eid] > 1 and v.state in ("SUBMITTED", "ACCEPTED", "SETTLED"):
                findings.append(Finding("BLIND_RETRY", "F3", eid, f"duplicate submit in state {v.state}"))
            v.state = "SUBMITTED"
        elif ph == "ack":
            v.state = "ACCEPTED" if v.state == "SUBMITTED" else v.state
        elif ph == "status_accepted":
            v.state = "ACCEPTED"
            v.action = "NONE"
        elif ph == "status_rejected":
            v.state = "REJECTED"
            v.action = "NONE"
        elif ph == "timeout":
            if v.state in ("SUBMITTED", "ACCEPTED"):
                v.state = "UNKNOWN"
                v.action = "RECONCILE"
        elif ph == "reconcile":
            res = _attr(sp, "reconciliation_result")
            if res == "accepted":
                v.state, v.action = "ACCEPTED", "NONE"
            elif res == "rejected":
                v.state, v.action = "REJECTED", "NONE"
            elif res == "not_accepted":
                v.state, v.action, v.retry_eligible = "REJECTED", "NONE", True
        elif ph in ("settled", "returned", "reversed"):
            if ph == "settled" and _attr(sp, "finality_observed") is not True:
                findings.append(Finding("SETTLED_WITHOUT_FINALITY", "F5", eid,
                                        "settled span lacks payments.finality_observed=true"))
            else:
                v.state = ph.upper()
    for eid, chain in seen_chain.items():
        for k in CHAIN_KEYS:
            if k not in chain:
                findings.append(Finding("CHAIN_NOT_JOINABLE", "F4", eid, f"missing {k}"))
            elif len(chain[k]) > 1:
                findings.append(Finding("CHAIN_NOT_JOINABLE", "F4", eid, f"{k} differs across spans"))
    return views, findings


def to_ocel(spans: Iterable[dict]) -> dict:
    """OCEL 2.0 JSON: one event per payment span; objects = effect, obligation,
    idempotency key, rail correlation."""
    objects: dict[str, dict] = {}
    events: list[dict] = []
    etypes: set[str] = set()
    otypes = {"effect": "effect", "obligation": "obligation_id",
              "idempotency_key": "idempotency_key", "rail_correlation": "rail_correlation_id"}
    for sp in _ordered(spans):
        eid = _attr(sp, "effect_id")
        ph = _attr(sp, "phase")
        if not eid or ph is None:
            continue
        rels = []
        for typ, key in otypes.items():
            val = eid if typ == "effect" else _attr(sp, key)
            if val is None:
                continue
            oid = f"{typ}:{val}"
            objects.setdefault(oid, {"id": oid, "type": typ, "attributes": []})
            rels.append({"objectId": oid, "qualifier": typ})
        etypes.add(ph)
        events.append({
            "id": sp.get("span_id", f"ev{len(events)}"),
            "type": ph,
            "time": sp.get("start_time_unix_nano", 0),
            "attributes": [{"name": "trace_id", "value": sp.get("trace_id", "")}],
            "relationships": rels,
        })
    return {
        "objectTypes": [{"name": t, "attributes": []} for t in sorted({o["type"] for o in objects.values()})],
        "eventTypes": [{"name": t, "attributes": [{"name": "trace_id", "type": "string"}]} for t in sorted(etypes)],
        "objects": sorted(objects.values(), key=lambda o: o["id"]),
        "events": events,
    }
