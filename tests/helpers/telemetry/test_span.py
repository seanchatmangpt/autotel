"""
Tests for AutoTel Telemetry Span Helpers - Happy Path Validation

Tests the successful execution paths for all span helper functions,
focusing on proper telemetry span creation, attribute management,
and success/error recording.
"""

import pytest
from unittest.mock import Mock, patch, MagicMock
from autotel.helpers.telemetry.span import (
    create_processor_span,
    add_span_attributes,
    record_span_success,
    record_span_error,
    create_span_context
)


class TestSpanHelpers:
    """Test suite for span helper functions - happy path validation."""
    
    def test_create_processor_span_success(self):
        """Test successful creation of processor span with basic attributes."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        mock_span = Mock()
        mock_telemetry.start_span.return_value = mock_span
        
        with patch('autotel.helpers.telemetry.span.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test span creation
            span = create_processor_span("parse", "bpmn", input_size=1024)
            
            # Verify telemetry manager was called correctly
            mock_telemetry.start_span.assert_called_once_with(
                name="bpmn.parse",
                operation_type="BPMN_PARSE",
                stage="bpmn_processing"
            )
            
            # Verify span attributes were set
            assert mock_span.set_attribute.call_count >= 4  # processor_type, operation, timestamp, input_size
            
            # Verify specific attributes
            mock_span.set_attribute.assert_any_call("processor_type", "bpmn")
            mock_span.set_attribute.assert_any_call("operation", "parse")
            mock_span.set_attribute.assert_any_call("input_size", 1024)
    
    def test_add_span_attributes_success(self):
        """Test successful addition of attributes to span."""
        mock_span = Mock()
        attributes = {
            "elements_count": 15,
            "processing_time_ms": 45.2,
            "status": "completed"
        }
        
        # Test attribute addition
        add_span_attributes(mock_span, attributes)
        
        # Verify all attributes were set
        assert mock_span.set_attribute.call_count == 3
        mock_span.set_attribute.assert_any_call("elements_count", 15)
        mock_span.set_attribute.assert_any_call("processing_time_ms", 45.2)
        mock_span.set_attribute.assert_any_call("status", "completed")
    
    def test_record_span_success_success(self):
        """Test successful recording of span success with metadata."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        mock_span = Mock()
        
        with patch('autotel.helpers.telemetry.span.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            metadata = {
                "elements_parsed": 10,
                "processing_time_ms": 45.2,
                "cache_hits": 3
            }
            
            # Test success recording
            record_span_success(mock_span, metadata)
            
            # Verify success attributes were set
            mock_span.set_attribute.assert_any_call("success", True)
            mock_span.set_attribute.assert_any_call("status", "success")
            
            # Verify metadata attributes were set
            mock_span.set_attribute.assert_any_call("elements_parsed", 10)
            mock_span.set_attribute.assert_any_call("processing_time_ms", 45.2)
            mock_span.set_attribute.assert_any_call("cache_hits", 3)
            
            # Verify metric was recorded
            mock_telemetry.record_metric.assert_called_once_with("processor_operation_success", 1)
    
    def test_record_span_error_success(self):
        """Test successful recording of span error with context."""
        # Mock telemetry manager and trace
        mock_telemetry = Mock()
        mock_trace = Mock()
        mock_status = Mock()
        mock_telemetry.trace = mock_trace
        mock_trace.Status = mock_status
        mock_trace.StatusCode = Mock()
        mock_trace.StatusCode.ERROR = "ERROR"
        
        mock_span = Mock()
        test_error = ValueError("Test error message")
        context = {
            "input_size": 1024,
            "error_location": "xml_parsing",
            "processor_version": "1.0.0"
        }
        
        with patch('autotel.helpers.telemetry.span.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test error recording
            record_span_error(mock_span, test_error, context)
            
            # Verify error attributes were set
            mock_span.set_attribute.assert_any_call("success", False)
            mock_span.set_attribute.assert_any_call("status", "error")
            mock_span.set_attribute.assert_any_call("error_type", "ValueError")
            mock_span.set_attribute.assert_any_call("error_message", "Test error message")
            
            # Verify context attributes were set
            mock_span.set_attribute.assert_any_call("input_size", 1024)
            mock_span.set_attribute.assert_any_call("error_location", "xml_parsing")
            mock_span.set_attribute.assert_any_call("processor_version", "1.0.0")
            
            # Verify span status was set
            mock_span.set_status.assert_called_once()
            mock_status.assert_called_once_with("ERROR", "Test error message")
            
            # Verify metric was recorded
            mock_telemetry.record_metric.assert_called_once_with("processor_operation_error", 1)
    
    def test_create_span_context_success(self):
        """Test successful creation of span context dictionary."""
        # Test context creation
        context = create_span_context("parse", "bpmn")
        
        # Verify context structure
        assert isinstance(context, dict)
        assert context["operation"] == "parse"
        assert context["processor_type"] == "bpmn"
        assert "timestamp" in context
        assert context["helper_version"] == "1.0.0"
        assert isinstance(context["timestamp"], float)
    
    def test_span_helpers_integration_success(self):
        """Test successful integration of all span helpers in a typical workflow."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        mock_span = Mock()
        mock_telemetry.start_span.return_value = mock_span
        
        with patch('autotel.helpers.telemetry.span.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Simulate a typical processor workflow
            span = create_processor_span("parse", "dspy", input_size=2048)
            
            # Add processing attributes
            add_span_attributes(span, {
                "signatures_found": 5,
                "modules_parsed": 3
            })
            
            # Record success
            record_span_success(span, {
                "processing_time_ms": 67.8,
                "validation_rules": 12
            })
            
            # Verify the complete workflow
            assert mock_span.set_attribute.call_count >= 8  # All attributes from the workflow
            
            # Verify telemetry manager was used correctly
            mock_telemetry.start_span.assert_called_once()
            mock_telemetry.record_metric.assert_called_once_with("processor_operation_success", 1)
    
    def test_multiple_processor_types_success(self):
        """Test successful span creation for different processor types."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        mock_span = Mock()
        mock_telemetry.start_span.return_value = mock_span
        
        processor_types = ["bpmn", "dmn", "dspy", "shacl", "owl", "jinja", "otel"]
        
        with patch('autotel.helpers.telemetry.span.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            for processor_type in processor_types:
                # Reset mock for each iteration
                mock_span.reset_mock()
                mock_telemetry.reset_mock()
                
                # Test span creation for each processor type
                span = create_processor_span("validate", processor_type)
                
                # Verify processor-specific attributes
                mock_span.set_attribute.assert_any_call("processor_type", processor_type)
                mock_span.set_attribute.assert_any_call("operation", "validate")
                
                # Verify span name format
                mock_telemetry.start_span.assert_called_with(
                    name=f"{processor_type}.validate",
                    operation_type=f"{processor_type.upper()}_VALIDATE",
                    stage=f"{processor_type}_processing"
                ) 