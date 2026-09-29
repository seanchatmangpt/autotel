# Payments Role: autotel

autotel is the evidence/telemetry source: OpenTelemetry spans become OCEL 2.0 events
for settlement-observation and reconciliation. Code: `payments_evidence/`. Tests:
`tests/payments_evidence/`. Attribute names are project-local and non-authoritative;
no ISO 20022 conformance is claimed.

## Falsifier coverage

| Falsifier | Coverage |
|---|---|
| F3 timeout permits blind retry | `BLIND_RETRY` finding for a submit while UNKNOWN without a reconciliation proving non-acceptance, and for a duplicate submit in SUBMITTED/ACCEPTED/SETTLED. Retry eligibility (set only by `reconcile` = `not_accepted`) is consumed by the retry submit and cleared on a new timeout, so each later UNKNOWN needs a fresh reconcile. Regression tests cover the multi-cycle sequence. Telemetry-only classification; it does not prevent a retry, it reports one. |
| F4 ids not joinable | `CHAIN_NOT_JOINABLE` when any span of an effect lacks obligation/idempotency/rail-correlation keys, or a key takes more than one value across an effect's spans. Only the span-level presence and consistency of these three keys is checked; ledger, settlement observation and receipt ids are not part of the chain here. |
| F5 SETTLED without finality | settled span without `finality_observed=true` is a finding, state not advanced |
| F1, F2, F6-F10 | not covered here |

`UNKNOWN_PHASE` findings map to no falsifier.

## OCEL 2.0 scope

`to_ocel` emits an OCEL 2.0-shaped JSON dict (object/event types, objects, events with
relationships). Tests check the shape, JSON serializability and object joinability only;
the output is not validated against the OCEL 2.0 schema, so schema conformance is not claimed.

## CI

The repo has no CI workflow; the only gate is the local command below.

Gate: `python3 -m unittest discover -s tests/payments_evidence -v`
