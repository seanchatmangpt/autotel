"""
AutoTel Telemetry Metric Helpers

Metric recording utilities for performance monitoring and success tracking
across all processors, providing consistent metrics for operations, durations,
success/failure rates, and contract violations.
"""

import time
from typing import Dict, Any, Optional, Union, List
from pathlib import Path

from autotel.core.telemetry import get_telemetry_manager_or_noop, TelemetryManager


def record_processor_metric(
    metric_name: str, 
    value: Union[int, float], 
    tags: Dict[str, str]
) -> None:
    """
    Record a custom metric for processor operations.
    
    Args:
        metric_name: Name of the metric to record
        value: Numeric value of the metric
        tags: Dictionary of tags for categorizing the metric
        
    Example:
        >>> record_processor_metric("elements_parsed", 15, {
        ...     "processor_type": "bpmn",
        ...     "operation": "parse"
        ... })
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Add standard tags
        enhanced_tags = {
            "helper_version": "1.0.0",
            "metric_type": "processor_custom"
        }
        enhanced_tags.update(tags)
        
        telemetry.record_metric(metric_name, value, enhanced_tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass


def record_processing_duration(
    operation: str, 
    duration_ms: float, 
    processor_type: str,
    additional_tags: Optional[Dict[str, str]] = None
) -> None:
    """
    Record processing duration for performance monitoring.
    
    Args:
        operation: Name of the operation (parse, validate, etc.)
        duration_ms: Duration in milliseconds
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        additional_tags: Optional additional tags for the metric
        
    Example:
        >>> record_processing_duration("parse", 45.2, "bpmn", {
        ...     "input_size": "large",
        ...     "cache_used": "true"
        ... })
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Standard tags for duration metrics
        tags = {
            "processor_type": processor_type,
            "operation": operation,
            "metric_type": "duration",
            "unit": "milliseconds"
        }
        
        if additional_tags:
            tags.update(additional_tags)
        
        # Record the duration metric
        telemetry.record_metric("processor_duration_ms", duration_ms, tags)
        
        # Also record as a histogram for percentile analysis
        telemetry.record_metric(f"{processor_type}_{operation}_duration_ms", duration_ms, tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass


def record_processing_success(
    processor_type: str, 
    operation: str,
    additional_metadata: Optional[Dict[str, Any]] = None
) -> None:
    """
    Record successful processing operations.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        additional_metadata: Optional additional metadata for the metric
        
    Example:
        >>> record_processing_success("dspy", "parse", {
        ...     "signatures_count": 5,
        ...     "modules_count": 3
        ... })
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Standard tags for success metrics
        tags = {
            "processor_type": processor_type,
            "operation": operation,
            "status": "success",
            "metric_type": "counter"
        }
        
        # Record success counter
        telemetry.record_metric("processor_operation_success", 1, tags)
        
        # Record processor-specific success metric
        telemetry.record_metric(f"{processor_type}_operation_success", 1, tags)
        
        # Record additional metadata as separate metrics if provided
        if additional_metadata:
            for key, value in additional_metadata.items():
                if isinstance(value, (int, float)):
                    metadata_tags = tags.copy()
                    metadata_tags["metadata_key"] = key
                    telemetry.record_metric(f"processor_metadata_{key}", value, metadata_tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass


def record_processing_failure(
    processor_type: str, 
    operation: str, 
    error_type: str,
    additional_context: Optional[Dict[str, Any]] = None
) -> None:
    """
    Record failed processing operations with error details.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        error_type: Type of error that occurred
        additional_context: Optional additional context for the error
        
    Example:
        >>> record_processing_failure("shacl", "validate", "ValidationError", {
        ...     "constraint_type": "cardinality",
        ...     "input_size": 1024
        ... })
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Standard tags for failure metrics
        tags = {
            "processor_type": processor_type,
            "operation": operation,
            "status": "failure",
            "error_type": error_type,
            "metric_type": "counter"
        }
        
        # Record failure counter
        telemetry.record_metric("processor_operation_error", 1, tags)
        
        # Record processor-specific failure metric
        telemetry.record_metric(f"{processor_type}_operation_error", 1, tags)
        
        # Record error type specific metric
        telemetry.record_metric(f"processor_error_{error_type}", 1, tags)
        
        # Record additional context as separate metrics if provided
        if additional_context:
            for key, value in additional_context.items():
                if isinstance(value, (int, float)):
                    context_tags = tags.copy()
                    context_tags["context_key"] = key
                    telemetry.record_metric(f"processor_error_context_{key}", value, context_tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass


def record_contract_violation(
    processor_type: str, 
    contract_type: str,
    violation_details: Optional[Dict[str, Any]] = None
) -> None:
    """
    Record contract violations for monitoring and debugging.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        contract_type: Type of contract violated (precondition, postcondition, invariant)
        violation_details: Optional details about the violation
        
    Example:
        >>> record_contract_violation("owl", "precondition", {
        ...     "method": "parse_ontology_definition",
        ...     "parameter": "xml_content",
        ...     "expected_type": "str"
        ... })
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Standard tags for contract violation metrics
        tags = {
            "processor_type": processor_type,
            "contract_type": contract_type,
            "metric_type": "counter",
            "violation": "true"
        }
        
        # Record contract violation counter
        telemetry.record_metric("processor_contract_violation", 1, tags)
        
        # Record processor-specific contract violation
        telemetry.record_metric(f"{processor_type}_contract_violation", 1, tags)
        
        # Record contract type specific violation
        telemetry.record_metric(f"processor_contract_{contract_type}_violation", 1, tags)
        
        # Record violation details as separate metrics if provided
        if violation_details:
            for key, value in violation_details.items():
                if isinstance(value, (int, float)):
                    detail_tags = tags.copy()
                    detail_tags["detail_key"] = key
                    telemetry.record_metric(f"processor_contract_violation_{key}", value, detail_tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass


def record_performance_metrics(
    processor_type: str,
    operation: str,
    start_time: float,
    end_time: Optional[float] = None,
    additional_metrics: Optional[Dict[str, Union[int, float]]] = None
) -> None:
    """
    Record comprehensive performance metrics for processor operations.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        start_time: Start time as timestamp
        end_time: End time as timestamp (if None, uses current time)
        additional_metrics: Optional additional metrics to record
        
    Example:
        >>> start_time = time.time()
        >>> # ... processing logic ...
        >>> record_performance_metrics("dspy", "parse", start_time, {
        ...     "memory_usage_mb": 45.2,
        ...     "cpu_usage_percent": 12.5
        ... })
    """
    if end_time is None:
        end_time = time.time()
    
    # Calculate duration
    duration_ms = (end_time - start_time) * 1000
    
    # Record duration metric
    record_processing_duration(operation, duration_ms, processor_type)
    
    # Record additional metrics if provided
    if additional_metrics:
        for metric_name, value in additional_metrics.items():
            if isinstance(value, (int, float)):
                record_processor_metric(
                    f"{processor_type}_{operation}_{metric_name}",
                    value,
                    {
                        "processor_type": processor_type,
                        "operation": operation,
                        "metric_type": "performance"
                    }
                )


def record_batch_metrics(
    processor_type: str,
    operation: str,
    batch_size: int,
    successful_count: int,
    failed_count: int,
    total_duration_ms: float
) -> None:
    """
    Record metrics for batch processing operations.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        batch_size: Total number of items in the batch
        successful_count: Number of successfully processed items
        failed_count: Number of failed items
        total_duration_ms: Total processing duration in milliseconds
        
    Example:
        >>> record_batch_metrics("shacl", "validate", 100, 95, 5, 1250.5)
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name="autotel-helpers",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    try:
        # Standard tags for batch metrics
        tags = {
            "processor_type": processor_type,
            "operation": operation,
            "metric_type": "batch"
        }
        
        # Record batch metrics
        telemetry.record_metric(f"{processor_type}_{operation}_batch_size", batch_size, tags)
        telemetry.record_metric(f"{processor_type}_{operation}_batch_success", successful_count, tags)
        telemetry.record_metric(f"{processor_type}_{operation}_batch_failed", failed_count, tags)
        telemetry.record_metric(f"{processor_type}_{operation}_batch_duration_ms", total_duration_ms, tags)
        
        # Calculate and record success rate
        if batch_size > 0:
            success_rate = (successful_count / batch_size) * 100
            telemetry.record_metric(f"{processor_type}_{operation}_batch_success_rate", success_rate, tags)
        
        # Calculate and record throughput
        if total_duration_ms > 0:
            throughput = batch_size / (total_duration_ms / 1000)  # items per second
            telemetry.record_metric(f"{processor_type}_{operation}_batch_throughput", throughput, tags)
        
    except Exception:
        # Silently ignore metric recording failures
        pass 