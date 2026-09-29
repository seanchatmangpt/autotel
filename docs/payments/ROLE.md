# Payments Role: autotel

autotel is the evidence/telemetry source: OpenTelemetry spans become OCEL 2.0 events
for settlement-observation and reconciliation. Code: `payments_evidence/`. Tests:
`tests/payments_evidence/`. Attribute names are project-local and non-authoritative;
no ISO 20022 conformance is claimed.

## Falsifier coverage

| Falsifier | Coverage |
|---|---|
| F3 timeout permits blind retry | `BLIND_RETRY` finding; timeout yields UNKNOWN/RECONCILE |
| F4 ids not joinable | `CHAIN_NOT_JOINABLE`; OCEL objects join effect/obligation/idempotency/rail ids |
| F5 SETTLED without finality | settled span without `finality_observed=true` is a finding, state not advanced |
| F1, F2, F6-F10 | not covered here |

Gate: `python3 -m unittest discover -s tests/payments_evidence -v`
