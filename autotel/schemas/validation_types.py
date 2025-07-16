"""Validation data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any


@dataclass
class ValidationRule:
    """Individual validation rule from SHACL constraints."""
    rule_id: str
    target_class: str
    property_path: str
    constraint_type: str  # cardinality, datatype, value, logical
    constraint_value: Any
    severity: str  # Violation, Warning, Info
    message: str
    metadata: Dict[str, Any]


@dataclass
class ValidationRules:
    """Compiled validation rules from SHACL constraints."""
    target_classes: Dict[str, List[ValidationRule]]
    constraint_count: int
    severity_levels: Dict[str, int]
    metadata: Dict[str, Any]


@dataclass
class ConstraintMetadata:
    """Metadata about SHACL constraints."""
    shacl_triples: int
    node_shapes: int
    property_shapes: int
    constraint_types: List[str] 