"""
AutoTel Error Handler Helpers

Centralized error handling utilities for all processors, providing
consistent error reporting, contract violation handling, validation
error management, and telemetry integration.
"""

from typing import Any, Dict, Optional
from autotel.core.telemetry import get_telemetry_manager_or_noop


def handle_processor_error(
    error: Exception,
    processor_type: str,
    operation: str,
    context: Optional[Dict[str, Any]] = None
) -> None:
    """
    Handle processor errors with telemetry integration.
    
    Args:
        error: Exception that occurred
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        context: Optional additional context for the error
        
    Example:
        >>> try:
        ...     # Processor logic
        ...     pass
        ... except Exception as e:
        ...     handle_processor_error(e, "bpmn", "parse", {"input_size": 1024})
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name=f"autotel-{processor_type}-processor",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    # Record error metric
    telemetry.record_metric("processor_operation_error", 1, {
        "processor_type": processor_type,
        "operation": operation,
        "error_type": type(error).__name__,
        "status": "failure"
    })
    
    # Optionally add context as separate metrics
    if context:
        for key, value in context.items():
            if isinstance(value, (int, float)):
                telemetry.record_metric(f"processor_error_context_{key}", value, {
                    "processor_type": processor_type,
                    "operation": operation,
                    "error_type": type(error).__name__,
                    "context_key": key
                })


def handle_contract_violation(
    violation: Any,
    processor_type: str,
    contract_type: str = "precondition",
    context: Optional[Dict[str, Any]] = None
) -> None:
    """
    Handle contract violations with telemetry integration.
    
    Args:
        violation: Contract violation object or message
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        contract_type: Type of contract violated (precondition, postcondition, invariant)
        context: Optional additional context for the violation
        
    Example:
        >>> handle_contract_violation("input must be str", "owl", "precondition", {"method": "parse_ontology_definition"})
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name=f"autotel-{processor_type}-processor",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    # Record contract violation metric
    telemetry.record_metric("processor_contract_violation", 1, {
        "processor_type": processor_type,
        "contract_type": contract_type,
        "violation": "true"
    })
    
    # Optionally add context as separate metrics
    if context:
        for key, value in context.items():
            if isinstance(value, (int, float)):
                telemetry.record_metric(f"processor_contract_violation_{key}", value, {
                    "processor_type": processor_type,
                    "contract_type": contract_type,
                    "context_key": key
                })


def handle_validation_error(
    error: Exception,
    processor_type: str,
    operation: str,
    context: Optional[Dict[str, Any]] = None
) -> None:
    """
    Handle validation errors with telemetry integration.
    
    Args:
        error: Exception that occurred
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        context: Optional additional context for the error
        
    Example:
        >>> try:
        ...     # Validation logic
        ...     pass
        ... except Exception as e:
        ...     handle_validation_error(e, "shacl", "validate", {"constraint_type": "cardinality"})
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name=f"autotel-{processor_type}-processor",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    # Record validation error metric
    telemetry.record_metric("processor_validation_error", 1, {
        "processor_type": processor_type,
        "operation": operation,
        "error_type": type(error).__name__,
        "status": "failure"
    })
    
    # Optionally add context as separate metrics
    if context:
        for key, value in context.items():
            if isinstance(value, (int, float)):
                telemetry.record_metric(f"processor_validation_error_{key}", value, {
                    "processor_type": processor_type,
                    "operation": operation,
                    "error_type": type(error).__name__,
                    "context_key": key
                })


def handle_xml_parsing_error(
    error: Exception,
    processor_type: str,
    operation: str,
    content_preview: Optional[str] = None
) -> None:
    """
    Handle XML parsing errors with telemetry integration.
    
    Args:
        error: Exception that occurred
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        operation: Name of the operation (parse, validate, etc.)
        content_preview: Optional preview of the XML content
        
    Example:
        >>> try:
        ...     # XML parsing logic
        ...     pass
        ... except Exception as e:
        ...     handle_xml_parsing_error(e, "bpmn", "parse", content_preview="<bpmn:process ...>")
    """
    telemetry = get_telemetry_manager_or_noop(
        service_name=f"autotel-{processor_type}-processor",
        require_linkml_validation=False,
        fallback_to_noop=True
    )
    
    # Record XML parsing error metric
    telemetry.record_metric("processor_xml_parsing_error", 1, {
        "processor_type": processor_type,
        "operation": operation,
        "error_type": type(error).__name__,
        "status": "failure"
    })
    
    # Optionally add content preview as a metric (length only)
    if content_preview:
        telemetry.record_metric("processor_xml_content_preview_length", len(content_preview), {
            "processor_type": processor_type,
            "operation": operation
        })


def handle_telemetry_error(
    error: Exception,
    fallback_to_noop: bool = True
) -> None:
    """
    Handle telemetry errors, optionally falling back to no-op telemetry.
    
    Args:
        error: Exception that occurred
        fallback_to_noop: Whether to fallback to no-op telemetry
        
    Example:
        >>> try:
        ...     # Telemetry logic
        ...     pass
        ... except Exception as e:
        ...     handle_telemetry_error(e)
    """
    # This is a stub for future expansion; currently just logs or ignores
    pass 