"""DSPy data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional


@dataclass
class DSPyModelConfiguration:
    """DSPy model configuration."""
    provider: str
    model_name: str
    parameters: Dict[str, Any]
    telemetry_config: Dict[str, Any]


@dataclass
class DSPyModuleDefinition:
    """DSPy module definition."""
    module_type: str
    configuration: Dict[str, Any]
    dependencies: List[str]


@dataclass
class DSPySignature:
    """Execution-ready DSPy signature with semantic context."""
    name: str
    description: str
    inputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to input schemas
    outputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to output schemas
    validation_rules: List[Dict[str, Any]]  # SHACL constraints
    examples: List[Dict[str, Any]]  # OWL individuals
    model_config: Optional[DSPyModelConfiguration]
    module_config: Optional[DSPyModuleDefinition]
    semantic_context: Dict[str, Any]
    ontology_metadata: Dict[str, Any]


@dataclass
class DSPySignatureDefinition:
    """Raw DSPy signature definition from processor."""
    name: str
    description: str
    inputs: Dict[str, Any]
    outputs: Dict[str, Any]
    examples: List[Dict[str, Any]] 