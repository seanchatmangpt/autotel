from dataclasses import dataclass, field
from typing import List, Any

@dataclass
class DMNInputEntry:
    id: str
    description: str
    lhs: List[str]
    text: str

@dataclass
class DMNOutputEntry:
    id: str
    description: str
    text: str

@dataclass
class DMNInput:
    id: str
    label: str
    name: str
    expression: str
    typeRef: str

@dataclass
class DMNOutput:
    id: str
    label: str
    name: str
    typeRef: str

@dataclass
class DMNRule:
    id: str
    row_number: int
    description: str
    inputEntries: List[DMNInputEntry]
    outputEntries: List[DMNOutputEntry]

@dataclass
class DMNDecisionTable:
    id: str
    name: str
    hit_policy: str
    inputs: List[DMNInput]
    outputs: List[DMNOutput]
    rules: List[DMNRule]

@dataclass
class DMNDecision:
    id: str
    name: str
    decisionTables: List[DMNDecisionTable]


def validate_dmn_decision(obj: Any) -> bool:
    """
    Validate that the object matches the DMNDecision dataclass structure.
    Returns True if valid, raises AssertionError if not.
    """
    assert isinstance(obj, DMNDecision), f"Not a DMNDecision: {type(obj)}"
    assert isinstance(obj.id, str)
    assert isinstance(obj.name, str)
    assert isinstance(obj.decisionTables, list)
    for dt in obj.decisionTables:
        assert isinstance(dt, DMNDecisionTable)
        assert isinstance(dt.id, str)
        assert isinstance(dt.hit_policy, str)
        assert isinstance(dt.inputs, list)
        for i in dt.inputs:
            assert isinstance(i, DMNInput)
            assert isinstance(i.id, str)
            assert isinstance(i.expression, str)
        assert isinstance(dt.outputs, list)
        for o in dt.outputs:
            assert isinstance(o, DMNOutput)
            assert isinstance(o.id, str)
            assert isinstance(o.typeRef, str)
        assert isinstance(dt.rules, list)
        for r in dt.rules:
            assert isinstance(r, DMNRule)
            assert isinstance(r.id, str)
            assert isinstance(r.inputEntries, list)
            for ie in r.inputEntries:
                assert isinstance(ie, DMNInputEntry)
                assert isinstance(ie.id, str)
                assert isinstance(ie.text, str)
            assert isinstance(r.outputEntries, list)
            for oe in r.outputEntries:
                assert isinstance(oe, DMNOutputEntry)
                assert isinstance(oe.id, str)
                assert isinstance(oe.text, str)
    return True 