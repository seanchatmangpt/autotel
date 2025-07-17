"""
AutoTel Helpers - Centralized utilities for processor architecture

This module provides reusable, composable helpers for XML processing,
telemetry integration, contract validation, error handling, configuration
management, and general utilities.
"""

__version__ = "1.0.0"

# Main exports for easy access
from .telemetry.span import (
    create_processor_span,
    add_span_attributes,
    record_span_success,
    record_span_error,
    create_span_context
)

from .telemetry.metric import (
    record_processor_metric,
    record_processing_duration,
    record_processing_success,
    record_processing_failure,
    record_contract_violation,
    record_performance_metrics,
    record_batch_metrics
)

__all__ = [
    # Telemetry span helpers
    'create_processor_span',
    'add_span_attributes', 
    'record_span_success',
    'record_span_error',
    'create_span_context',
    
    # Telemetry metric helpers
    'record_processor_metric',
    'record_processing_duration',
    'record_processing_success',
    'record_processing_failure',
    'record_contract_violation',
    'record_performance_metrics',
    'record_batch_metrics',
] 