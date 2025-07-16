"""Executor data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional


@dataclass
class ValidationResult:
    """Result of input validation."""
    valid: bool
    violations: List[Dict[str, Any]]
    warnings: List[Dict[str, Any]]
    metadata: Dict[str, Any]


@dataclass
class TelemetryData:
    """Telemetry data from execution."""
    spans: List[Dict[str, Any]]
    events: List[Dict[str, Any]]
    metrics: Dict[str, Any]
    logs: List[Dict[str, Any]]


@dataclass
class ExecutionResult:
    """Result of ontology execution."""
    success: bool
    outputs: Dict[str, Any]
    execution_time: float
    validation_result: ValidationResult
    telemetry_data: TelemetryData
    metadata: Dict[str, Any] 