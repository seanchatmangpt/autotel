"""Tests for contract/ABC helper utilities."""

import pytest
import icontract
from autotel.helpers import contract

# --- ABC Compliance ---

def test_base_processor_abc_enforces_methods():
    class IncompleteProcessor(contract.BaseProcessorABC):
        def process(self, data):
            return data
        # Missing get_name()
    with pytest.raises(TypeError):
        IncompleteProcessor()

    class CompleteProcessor(contract.BaseProcessorABC):
        def process(self, data):
            return data
        def get_name(self):
            return "complete"
    p = CompleteProcessor()
    assert p.get_name() == "complete"
    assert p.process(123) == 123

# --- Contract Decorators ---

def test_contract_pre_post_conditions():
    @contract.contract_precondition(lambda x: x > 0)
    @contract.contract_postcondition(lambda result: result < 10)
    def add_one(x):
        return x + 1
    assert add_one(5) == 6
    with pytest.raises(icontract.ViolationError):
        add_one(-1)
    with pytest.raises(icontract.ViolationError):
        add_one(10)

# --- Dynamic Contract Application ---

def test_apply_contracts_dynamically():
    def pre(x): return x > 0
    def post(result): return result < 10
    def add_one(x): return x + 1
    wrapped = contract.apply_contracts_dynamically(add_one, pre=pre, post=post)
    assert wrapped(3) == 4
    with pytest.raises(AssertionError):
        wrapped(-1)
    with pytest.raises(AssertionError):
        wrapped(10)

# --- Contract Compliance Utility ---

def test_check_contract_compliance():
    class NotAProcessor:
        pass
    class Incomplete(contract.BaseProcessorABC):
        def process(self, data): return data
    class Complete(contract.BaseProcessorABC):
        def process(self, data): return data
        def get_name(self): return "ok"
    assert not contract.check_contract_compliance(NotAProcessor)
    assert not contract.check_contract_compliance(Incomplete)
    assert contract.check_contract_compliance(Complete) 