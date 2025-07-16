"""Linker data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional

from .dspy_types import DSPySignature, DSPyModelConfiguration, DSPyModuleDefinition


@dataclass
class ExecutableSystem:
    """Executable system configuration for runtime execution."""
    signature: DSPySignature
    semantic_context: Dict[str, Any]
    validation_rules: List[Dict[str, Any]]
    examples: List[Dict[str, Any]]
    model_config: Optional[DSPyModelConfiguration]
    module_config: Optional[DSPyModuleDefinition]
    shacl_validation: bool
    shacl_triples: int
    metadata: Dict[str, Any] 