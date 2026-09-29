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


class SurvivorKillers(unittest.TestCase):
    """Added after mutation review: each test pins one behaviour a mutant broke."""

    # M3: finality must be exactly True
    def test_f5_explicit_false_finality_is_finding_not_settled(self):
        v, f = analyze([sp("submit"), sp("status_accepted"), sp("settled", finality_observed=False)])
        self.assertEqual(v["E1"].state, "ACCEPTED")
        self.assertEqual([x.code for x in f], ["SETTLED_WITHOUT_FINALITY"])
        self.assertEqual(f[0].falsifier, "F5")

    def test_f5_non_bool_finality_values_are_findings_not_settled(self):
        for bad in ("true", 1, "yes", [True], 0, ""):
            with self.subTest(finality=bad):
                v, f = analyze([sp("submit"), sp("status_accepted"), sp("settled", finality_observed=bad)])
                self.assertEqual(v["E1"].state, "ACCEPTED")
                self.assertEqual([x.code for x in f], ["SETTLED_WITHOUT_FINALITY"])

    def test_f5_returned_and_reversed_need_no_finality(self):
        for ph in ("returned", "reversed"):
            v, f = analyze([sp("submit"), sp("status_accepted"), sp(ph)])
            self.assertEqual(v["E1"].state, ph.upper())
            self.assertEqual(f, [])

    # M10: reconcile(not_accepted) arms retry; nothing else does
    def test_reconcile_not_accepted_arms_retry_and_proves_non_acceptance(self):
        v, f = analyze([sp("submit"), sp("timeout"),
                        sp("reconcile", reconciliation_result="not_accepted")])
        self.assertTrue(v["E1"].retry_eligible)
        self.assertEqual(v["E1"].state, "REJECTED")
        self.assertEqual(v["E1"].action, "NONE")
        self.assertEqual(f, [])

    def test_resubmit_allowed_only_when_reconcile_proved_non_acceptance(self):
        _, ok = analyze([sp("submit"), sp("timeout"),
                         sp("reconcile", reconciliation_result="not_accepted"), sp("submit")])
        self.assertEqual(ok, [])
        for res in ("accepted", "rejected", None, "bogus"):
            with self.subTest(result=res):
                extra = {} if res is None else {"reconciliation_result": res}
                v, f = analyze([sp("submit"), sp("timeout"), sp("reconcile", **extra), sp("submit")])
                self.assertFalse(v["E1"].retry_eligible)
                if res in (None, "bogus"):
                    # unrecognised result leaves the outcome UNKNOWN: the resubmit is blind
                    self.assertEqual([x.code for x in f], ["BLIND_RETRY"])

    def test_unrecognised_reconcile_result_never_arms_retry(self):
        for extra in ({}, {"reconciliation_result": "bogus"}, {"reconciliation_result": "NOT_ACCEPTED"},
                      {"reconciliation_result": True}):
            with self.subTest(extra=extra):
                v, f = analyze([sp("submit"), sp("timeout"), sp("reconcile", **extra)])
                self.assertFalse(v["E1"].retry_eligible)
                self.assertEqual(v["E1"].state, "UNKNOWN")
                self.assertEqual(v["E1"].action, "RECONCILE")
                self.assertEqual(f, [])

    def test_reconcile_rejected_state_and_no_retry(self):
        v, _ = analyze([sp("submit"), sp("timeout"), sp("reconcile", reconciliation_result="rejected")])
        self.assertEqual((v["E1"].state, v["E1"].action, v["E1"].retry_eligible), ("REJECTED", "NONE", False))

    def test_reconcile_accepted_clears_action(self):
        v, _ = analyze([sp("submit"), sp("timeout"), sp("reconcile", reconciliation_result="accepted")])
        self.assertEqual((v["E1"].state, v["E1"].action), ("ACCEPTED", "NONE"))

    # remaining state-machine transitions
    def test_ack_moves_submitted_to_accepted_only(self):
        v, _ = analyze([sp("submit"), sp("ack")])
        self.assertEqual(v["E1"].state, "ACCEPTED")
        v, _ = analyze([sp("prepare"), sp("ack")])
        self.assertEqual(v["E1"].state, "PREPARED")
        v, _ = analyze([sp("submit"), sp("timeout"), sp("ack")])
        self.assertEqual(v["E1"].state, "UNKNOWN")
        self.assertEqual(v["E1"].action, "RECONCILE")

    def test_timeout_only_makes_submitted_or_accepted_unknown(self):
        v, _ = analyze([sp("prepare"), sp("timeout")])
        self.assertEqual((v["E1"].state, v["E1"].action), ("PREPARED", "NONE"))
        v, _ = analyze([sp("submit"), sp("status_accepted"), sp("timeout")])
        self.assertEqual((v["E1"].state, v["E1"].action), ("UNKNOWN", "RECONCILE"))
        v, _ = analyze([sp("submit"), sp("status_rejected"), sp("timeout")])
        self.assertEqual(v["E1"].state, "REJECTED")

    def test_status_rejected_sets_rejected_and_clears_action(self):
        v, _ = analyze([sp("submit"), sp("timeout"), sp("status_rejected")])
        self.assertEqual((v["E1"].state, v["E1"].action), ("REJECTED", "NONE"))

    def test_status_accepted_clears_reconcile_action(self):
        v, _ = analyze([sp("submit"), sp("timeout"), sp("status_accepted")])
        self.assertEqual((v["E1"].state, v["E1"].action), ("ACCEPTED", "NONE"))

    def test_prepare_resets_state(self):
        v, _ = analyze([sp("submit"), sp("prepare")])
        self.assertEqual(v["E1"].state, "PREPARED")

    def test_duplicate_submit_in_settled_state(self):
        _, f = analyze([sp("submit"), sp("status_accepted"), sp("settled", finality_observed=True), sp("submit")])
        self.assertEqual([x.code for x in f], ["BLIND_RETRY"])
        self.assertIn("SETTLED", f[0].detail)

    def test_unknown_phase_does_not_change_state(self):
        v, f = analyze([sp("submit"), sp("bogus")])
        self.assertEqual(v["E1"].state, "SUBMITTED")
        self.assertEqual([x.code for x in f], ["UNKNOWN_PHASE"])

    def test_span_without_effect_id_is_ignored(self):
        orphan = {"span_id": "x", "start_time_unix_nano": 1, "attributes": {"payments.phase": "submit"}}
        v, f = analyze([orphan])
        self.assertEqual((v, f), ({}, []))

    def test_same_timestamp_ordering_is_by_span_id(self):
        a = {"span_id": "a", "start_time_unix_nano": 5, "attributes": sp("submit")["attributes"]}
        b = {"span_id": "b", "start_time_unix_nano": 5, "attributes": sp("status_accepted")["attributes"]}
        v1, _ = analyze([a, b])
        v2, _ = analyze([b, a])
        self.assertEqual(v1["E1"].span_ids, ["a", "b"])
        self.assertEqual(v2["E1"].span_ids, ["a", "b"])
        self.assertEqual(v1["E1"].state, v2["E1"].state)

    def test_f4_each_chain_key_mismatch_detected(self):
        for k in ("obligation_id", "idempotency_key", "rail_correlation_id"):
            with self.subTest(key=k):
                _, f = analyze([sp("submit"), sp("status_accepted", **{k: "OTHER"})])
                self.assertEqual([(x.code, x.detail) for x in f],
                                 [("CHAIN_NOT_JOINABLE", f"{k} differs across spans")])

    # to_ocel
    def test_ocel_orders_events_by_time_and_carries_time_trace_qualifiers(self):
        a = sp("prepare"); b = sp("submit")
        a["trace_id"] = "TA"; b["trace_id"] = "TB"
        o = to_ocel([b, a])
        self.assertEqual([e["type"] for e in o["events"]], ["prepare", "submit"])
        self.assertEqual([e["time"] for e in o["events"]], [a["start_time_unix_nano"], b["start_time_unix_nano"]])
        self.assertEqual([e["attributes"][0]["value"] for e in o["events"]], ["TA", "TB"])
        self.assertEqual({r["qualifier"] for r in o["events"][0]["relationships"]},
                         {"effect", "obligation", "idempotency_key", "rail_correlation"})
        for r in o["events"][0]["relationships"]:
            self.assertTrue(r["objectId"].startswith(r["qualifier"] + ":"))

    def test_ocel_type_declarations_and_object_order(self):
        o = to_ocel([sp("prepare"), sp("submit"), sp("submit")])
        self.assertEqual([t["name"] for t in o["eventTypes"]], ["prepare", "submit"])
        self.assertEqual([t["name"] for t in o["objectTypes"]],
                         ["effect", "idempotency_key", "obligation", "rail_correlation"])
        ids = [x["id"] for x in o["objects"]]
        self.assertEqual(ids, sorted(ids))

    def test_ocel_skips_spans_without_effect_or_phase(self):
        no_eff = {"span_id": "n1", "start_time_unix_nano": 1, "attributes": {"payments.phase": "submit"}}
        no_ph = {"span_id": "n2", "start_time_unix_nano": 2, "attributes": {"payments.effect_id": "E1"}}
        o = to_ocel([no_eff, no_ph, sp("submit")])
        self.assertEqual(len(o["events"]), 1)
        self.assertEqual([t["name"] for t in o["eventTypes"]], ["submit"])

    def test_ocel_omits_relationship_for_missing_chain_key(self):
        s = sp("submit")
        del s["attributes"]["payments.rail_correlation_id"]
        o = to_ocel([s])
        self.assertEqual(len(o["events"][0]["relationships"]), 3)
        self.assertNotIn("rail_correlation:R1", {x["id"] for x in o["objects"]})


if __name__ == "__main__":
    unittest.main()
