"""
AutoTel Telemetry Span Helpers

Core span management utilities for all processors, providing consistent
telemetry integration with automatic span creation, attribute management,
and error reporting.
"""

import time
from typing import Dict, Any, Optional, Union
from pathlib import Path
from contextlib import contextmanager

from autotel.core.telemetry import get_telemetry_manager_or_noop, TelemetryManager


@contextmanager
def create_processor_span(
    operation_name: str, 
    processor_type: str, 
    **kwargs: Any
):
    """
    Create a telemetry span for processor operations.
    
    Args:
        operation_name: Name of the operation being performed
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        **kwargs: Additional span attributes
        
    Yields:
        Telemetry span object for the operation
        
    Example:
        >>> with create_processor_span("parse", "bpmn", input_size=1024) as span:
        ...     # Processor logic here
        ...     add_span_attributes(span, {"elements_parsed": 10})
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name=f"autotel-{processor_type}-processor",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    # Always use a 'with' block to get the span object, regardless of manager type
    with telemetry.start_span(
        name=f"{processor_type}.{operation_name}",
        operation_type=f"{processor_type.upper()}_{operation_name.upper()}",
        **kwargs
    ) as span:
        # Add standard processor attributes
        if hasattr(span, "set_attribute"):
            span.set_attribute("processor_type", processor_type)
            span.set_attribute("operation", operation_name)
            span.set_attribute("timestamp", time.time())
            # Add any additional attributes
            for key, value in kwargs.items():
                span.set_attribute(key, value)
        yield span


def add_span_attributes(span: Any, attributes: Dict[str, Any]) -> None:
    """
    Add attributes to an existing telemetry span.
    
    Args:
        span: Telemetry span object
        attributes: Dictionary of attributes to add
        
    Example:
        >>> with create_processor_span("parse", "bpmn") as span:
        ...     add_span_attributes(span, {
        ...         "elements_count": 15,
        ...         "processing_time_ms": 45.2
        ...     })
    """
    if span is None:
        return
        
    for key, value in attributes.items():
        try:
            span.set_attribute(key, value)
        except Exception:
            # Silently ignore attribute setting failures
            pass


def record_span_success(span: Any, metadata: Dict[str, Any]) -> None:
    """
    Record successful completion of a span with metadata.
    
    Args:
        span: Telemetry span object
        metadata: Additional metadata about the successful operation
        
    Example:
        >>> with create_processor_span("parse", "bpmn") as span:
        ...     # Processing logic
        ...     record_span_success(span, {
        ...         "elements_parsed": 10,
        ...         "processing_time_ms": 45.2
        ...     })
    """
    if span is None:
        return
        
    try:
        # Add success metadata
        span.set_attribute("success", True)
        span.set_attribute("status", "success")
        
        # Add provided metadata
        add_span_attributes(span, metadata)
        
        # Record success metric
        telemetry = get_telemetry_manager_or_noop(
            service_name="autotel-helpers",
            require_linkml_validation=False,
            fallback_to_noop=True
        )
        telemetry.record_metric("processor_operation_success", 1)
        
    except Exception:
        # Silently ignore telemetry failures
        pass


def record_span_error(
    span: Any, 
    error: Exception, 
    context: Dict[str, Any]
) -> None:
    """
    Record error in a span with error details and context.
    
    Args:
        span: Telemetry span object
        error: Exception that occurred
        context: Additional context about the error
        
    Example:
        >>> with create_processor_span("parse", "bpmn") as span:
        ...     try:
        ...         # Processing logic
        ...         pass
        ...     except Exception as e:
        ...         record_span_error(span, e, {
        ...             "input_size": 1024,
        ...             "error_location": "xml_parsing"
        ...         })
    """
    if span is None:
        return
        
    try:
        # Add error attributes
        span.set_attribute("success", False)
        span.set_attribute("status", "error")
        span.set_attribute("error_type", type(error).__name__)
        span.set_attribute("error_message", str(error))
        
        # Add context
        add_span_attributes(span, context)
        
        # Set span status to error
        telemetry = get_telemetry_manager_or_noop(
            service_name="autotel-helpers",
            require_linkml_validation=False,
            fallback_to_noop=True
        )
        
        if hasattr(telemetry, 'trace') and hasattr(telemetry.trace, 'Status'):
            span.set_status(telemetry.trace.Status(
                telemetry.trace.StatusCode.ERROR, 
                str(error)
            ))
        
        # Record error metric
        telemetry.record_metric("processor_operation_error", 1)
        
    except Exception:
        # Silently ignore telemetry failures
        pass


def create_span_context(
    operation: str, 
    processor_type: str
) -> Dict[str, Any]:
    """
    Create a context dictionary for span operations.
    
    Args:
        operation: Name of the operation
        processor_type: Type of processor
        
    Returns:
        Dictionary with span context information
        
    Example:
        >>> context = create_span_context("parse", "bpmn")
        >>> # Use context in span creation or error handling
    """
    return {
        "operation": operation,
        "processor_type": processor_type,
        "timestamp": time.time(),
        "helper_version": "1.0.0"
    } 