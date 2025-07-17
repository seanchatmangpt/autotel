"""
AutoTel Telemetry Helpers

Centralized telemetry utilities for span creation, metric recording,
context management, and error reporting across all processors.
"""

from .span import (
    create_processor_span,
    add_span_attributes,
    record_span_success,
    record_span_error,
    create_span_context
)

from .metric import (
    record_processor_metric,
    record_processing_duration,
    record_processing_success,
    record_processing_failure,
    record_contract_violation,
    record_performance_metrics,
    record_batch_metrics
)

__all__ = [
    # Span helpers
    'create_processor_span',
    'add_span_attributes',
    'record_span_success', 
    'record_span_error',
    'create_span_context',
    
    # Metric helpers
    'record_processor_metric',
    'record_processing_duration',
    'record_processing_success',
    'record_processing_failure',
    'record_contract_violation',
    'record_performance_metrics',
    'record_batch_metrics',
] 