import json
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
from payments_evidence import analyze, to_ocel  # noqa: E402

_t = [0]


def sp(phase, effect="E1", **extra):
    _t[0] += 1
    a = {"payments.effect_id": effect, "payments.phase": phase,
         "payments.obligation_id": "O1", "payments.idempotency_key": "K1",
         "payments.rail_correlation_id": "R1"}
    a.update({"payments." + k: v for k, v in extra.items()})
    return {"span_id": f"s{_t[0]}", "trace_id": "t", "start_time_unix_nano": _t[0], "attributes": a}


class Vectors(unittest.TestCase):
    def test_accept_then_drop_ack_is_unknown_reconcile(self):
        v, f = analyze([sp("prepare"), sp("submit"), sp("timeout")])
        self.assertEqual(v["E1"].state, "UNKNOWN")
        self.assertEqual(v["E1"].action, "RECONCILE")
        self.assertFalse(v["E1"].retry_eligible)
        self.assertEqual(f, [])

    def test_f3_blind_retry_after_timeout(self):
        _, f = analyze([sp("prepare"), sp("submit"), sp("timeout"), sp("submit")])
        self.assertEqual([x.code for x in f], ["BLIND_RETRY"])
        self.assertEqual(f[0].falsifier, "F3")

    def test_retry_after_reconcile_not_accepted_is_lawful(self):
        v, f = analyze([sp("prepare"), sp("submit"), sp("timeout"),
                        sp("reconcile", reconciliation_result="not_accepted"), sp("submit")])
        self.assertEqual(f, [])
        self.assertEqual(v["E1"].state, "SUBMITTED")

    def test_reconcile_accepted_no_retry(self):
        v, _ = analyze([sp("submit"), sp("timeout"), sp("reconcile", reconciliation_result="accepted")])
        self.assertEqual(v["E1"].state, "ACCEPTED")
        self.assertFalse(v["E1"].retry_eligible)

    def test_f5_settled_without_finality_not_settled(self):
        v, f = analyze([sp("submit"), sp("status_accepted"), sp("settled")])
        self.assertEqual(v["E1"].state, "ACCEPTED")
        self.assertEqual(f[0].code, "SETTLED_WITHOUT_FINALITY")

    def test_settled_with_finality(self):
        v, f = analyze([sp("submit"), sp("status_accepted"), sp("settled", finality_observed=True)])
        self.assertEqual(v["E1"].state, "SETTLED")
        self.assertEqual(f, [])

    def test_duplicate_status_and_reorder_stable(self):
        spans = [sp("submit"), sp("status_accepted"), sp("status_accepted")]
        v1, _ = analyze(spans)
        v2, _ = analyze(list(reversed(spans)))
        self.assertEqual(v1["E1"].state, v2["E1"].state)

    def test_f4_chain_key_mismatch(self):
        a = sp("submit")
        b = sp("status_accepted", rail_correlation_id="R2")
        _, f = analyze([a, b])
        self.assertEqual(f[0].code, "CHAIN_NOT_JOINABLE")

    def test_ocel_shape_and_joinability(self):
        o = to_ocel([sp("prepare"), sp("submit"), sp("timeout")])
        json.dumps(o)
        self.assertEqual([e["type"] for e in o["events"]], ["prepare", "submit", "timeout"])
        ids = {x["id"] for x in o["objects"]}
        self.assertEqual(ids, {"effect:E1", "obligation:O1", "idempotency_key:K1", "rail_correlation:R1"})
        for e in o["events"]:
            self.assertEqual(len(e["relationships"]), 4)

    def test_f3_retry_eligibility_consumed_by_retry_submit(self):
        # audit sequence: submit,timeout,reconcile(not_accepted),submit,timeout,submit
        v, f = analyze([sp("submit"), sp("timeout"),
                        sp("reconcile", reconciliation_result="not_accepted"),
                        sp("submit"), sp("timeout"), sp("submit")])
        self.assertEqual([x.code for x in f], ["BLIND_RETRY"])
        self.assertFalse(v["E1"].retry_eligible)
        self.assertEqual(v["E1"].state, "SUBMITTED")

    def test_f3_second_cycle_rearms_only_after_fresh_reconcile(self):
        na = dict(reconciliation_result="not_accepted")
        v, f = analyze([sp("submit"), sp("timeout"), sp("reconcile", **na),
                        sp("submit"), sp("timeout"), sp("reconcile", **na), sp("submit")])
        self.assertEqual(f, [])
        self.assertFalse(v["E1"].retry_eligible)

    def test_f3_retry_eligible_cleared_when_new_timeout_makes_unknown(self):
        v, _ = analyze([sp("submit"), sp("timeout"),
                        sp("reconcile", reconciliation_result="not_accepted"),
                        sp("submit"), sp("timeout")])
        self.assertEqual(v["E1"].state, "UNKNOWN")
        self.assertFalse(v["E1"].retry_eligible)

    def test_f3_duplicate_submit_in_submitted_state(self):
        _, f = analyze([sp("submit"), sp("submit")])
        self.assertEqual([x.code for x in f], ["BLIND_RETRY"])
        self.assertIn("duplicate submit", f[0].detail)

    def test_f3_duplicate_submit_in_accepted_state(self):
        _, f = analyze([sp("submit"), sp("status_accepted"), sp("submit")])
        self.assertEqual([x.code for x in f], ["BLIND_RETRY"])

    def test_f4_effect_with_no_chain_keys_is_not_joinable(self):
        bare = {"span_id": "b1", "start_time_unix_nano": 1,
                "attributes": {"payments.effect_id": "E9", "payments.phase": "submit"}}
        _, f = analyze([bare])
        self.assertEqual({x.detail for x in f},
                         {"missing obligation_id on span b1", "missing idempotency_key on span b1",
                          "missing rail_correlation_id on span b1"})
        self.assertTrue(all(x.code == "CHAIN_NOT_JOINABLE" and x.falsifier == "F4" for x in f))

    def test_f4_key_present_on_some_spans_absent_on_others(self):
        a = sp("submit")
        b = sp("status_accepted")
        del b["attributes"]["payments.rail_correlation_id"]
        _, f = analyze([a, b])
        self.assertEqual([x.code for x in f], ["CHAIN_NOT_JOINABLE"])
        self.assertIn("rail_correlation_id", f[0].detail)

    def test_unknown_phase_is_not_labelled_f4(self):
        _, f = analyze([sp("submit"), sp("bogus")])
        self.assertEqual(f[0].code, "UNKNOWN_PHASE")
        self.assertNotEqual(f[0].falsifier, "F4")


if __name__ == "__main__":
    unittest.main()
