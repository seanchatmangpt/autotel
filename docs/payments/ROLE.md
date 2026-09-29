# Payments Role: autotel

autotel is the evidence/telemetry source: OpenTelemetry spans become OCEL 2.0 events
for settlement-observation and reconciliation. Code: `payments_evidence/`. Tests:
`tests/payments_evidence/`. Attribute names are project-local and non-authoritative;
no ISO 20022 conformance is claimed.

## Falsifier coverage

| Falsifier | Coverage |
|---|---|
| F3 timeout permits blind retry | `BLIND_RETRY` finding for a submit while UNKNOWN unless a prior `reconcile` returned `not_accepted`, and for a duplicate submit in SUBMITTED/ACCEPTED/SETTLED. Only `reconciliation_result` = `not_accepted` sets `retry_eligible` (accepted, rejected, missing or unrecognised results never do; the latter two leave the effect UNKNOWN). `not_accepted` moves the effect to REJECTED, and the flag is consumed (cleared) by the next submit, so each later UNKNOWN needs a fresh reconcile. Tests cover the flag, the multi-cycle sequence and the negative results. Telemetry-only classification; it does not prevent a retry, it reports one. |
| F4 ids not joinable | `CHAIN_NOT_JOINABLE` when any span of an effect lacks obligation/idempotency/rail-correlation keys, or a key takes more than one value across an effect's spans. Only the span-level presence and consistency of these three keys is checked; ledger, settlement observation and receipt ids are not part of the chain here. |
| F5 SETTLED without finality | a settled span whose `finality_observed` is not exactly boolean `true` (absent, `false`, or any non-bool such as `"true"` or `1`) is a finding and state is not advanced; `returned`/`reversed` spans need no finality attribute |
| F1, F2, F6-F10 | not covered here |

`UNKNOWN_PHASE` findings map to no falsifier.

## OCEL 2.0 scope

`to_ocel` emits an OCEL 2.0-shaped JSON dict (object/event types, objects, events with
relationships). Tests check the shape, JSON serializability and object joinability only;
the output is not validated against the OCEL 2.0 schema, so schema conformance is not claimed.

## CI

The repo has no CI workflow; the only gate is the local command below.

Gate: `python3 -m unittest discover -s tests/payments_evidence -v`
