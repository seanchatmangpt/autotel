"""OpenTelemetry data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional, Union
from datetime import datetime


@dataclass
class OTELSpanDefinition:
    """OpenTelemetry span definition."""
    name: str
    trace_id: str
    span_id: str
    parent_id: Optional[str]
    start_time: str  # ISO format string
    end_time: str    # ISO format string
    status: str
    attributes: Dict[str, Any]
    events: List[Dict[str, Any]]
    links: List[Dict[str, Any]]
    kind: str
    resource: Dict[str, Any]


@dataclass
class OTELMetricDefinition:
    """OpenTelemetry metric definition."""
    name: str
    value: Union[int, float]
    unit: str
    description: str
    attributes: Dict[str, Any]
    timestamp: str  # ISO format string


@dataclass
class OTELLogDefinition:
    """OpenTelemetry log definition."""
    timestamp: str  # ISO format string
    severity: str
    message: str
    attributes: Dict[str, Any]
    resource: Dict[str, Any]
    trace_id: Optional[str]
    span_id: Optional[str]


@dataclass
class OTELTraceDefinition:
    """Complete OpenTelemetry trace definition."""
    trace_id: str
    spans: List[OTELSpanDefinition]
    metrics: List[OTELMetricDefinition]
    logs: List[OTELLogDefinition]
    metadata: Dict[str, Any]


@dataclass
class OTELAnalysisResult:
    """Result of OpenTelemetry trace analysis."""
    trace_id: str
    span_analysis: Dict[str, Any]
    metric_analysis: Dict[str, Any]
    log_analysis: Dict[str, Any]
    performance_analysis: Dict[str, Any]
    error_analysis: Dict[str, Any]
    dependency_analysis: Dict[str, Any]
    metadata: Dict[str, Any]


@dataclass
class OTELProcessingConfig:
    """Configuration for OTEL processing."""
    enable_span_analysis: bool = True
    enable_metric_analysis: bool = True
    enable_log_analysis: bool = True
    enable_performance_analysis: bool = True
    enable_error_analysis: bool = True
    enable_dependency_analysis: bool = True
    include_attributes: bool = True
    include_events: bool = True
    include_links: bool = True
    max_spans_per_trace: int = 10000
    max_metrics_per_trace: int = 1000
    max_logs_per_trace: int = 10000
    error_threshold: float = 0.1  # 10% error rate threshold
    performance_threshold_ms: float = 1000.0  # 1 second threshold 