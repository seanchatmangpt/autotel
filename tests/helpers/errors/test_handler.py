"""
Tests for AutoTel Error Handler Helpers - Happy Path Validation

Tests the successful execution paths for all error handler functions,
focusing on proper telemetry integration and metric recording.
"""

import pytest
from unittest.mock import Mock, patch
from autotel.helpers.errors.handler import (
    handle_processor_error,
    handle_contract_violation,
    handle_validation_error,
    handle_xml_parsing_error,
    handle_telemetry_error
)

class TestErrorHandlerHelpers:
    """Test suite for error handler functions - happy path validation."""
    
    def test_handle_processor_error_success(self):
        mock_telemetry = Mock()
        error = ValueError("Test processor error")
        context = {"input_size": 1024, "attempt": 2}
        
        with patch('autotel.helpers.errors.handler.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            handle_processor_error(error, "bpmn", "parse", context)
            # Main error metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_operation_error", 1, {
                    "processor_type": "bpmn",
                    "operation": "parse",
                    "error_type": "ValueError",
                    "status": "failure"
                }
            )
            # Context metrics
            mock_telemetry.record_metric.assert_any_call(
                "processor_error_context_input_size", 1024, {
                    "processor_type": "bpmn",
                    "operation": "parse",
                    "error_type": "ValueError",
                    "context_key": "input_size"
                }
            )
            mock_telemetry.record_metric.assert_any_call(
                "processor_error_context_attempt", 2, {
                    "processor_type": "bpmn",
                    "operation": "parse",
                    "error_type": "ValueError",
                    "context_key": "attempt"
                }
            )
    
    def test_handle_contract_violation_success(self):
        mock_telemetry = Mock()
        context = {"method": 1, "param": 2}
        
        with patch('autotel.helpers.errors.handler.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            handle_contract_violation("violation", "owl", "precondition", context)
            # Main contract violation metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_contract_violation", 1, {
                    "processor_type": "owl",
                    "contract_type": "precondition",
                    "violation": "true"
                }
            )
            # Context metrics
            mock_telemetry.record_metric.assert_any_call(
                "processor_contract_violation_method", 1, {
                    "processor_type": "owl",
                    "contract_type": "precondition",
                    "context_key": "method"
                }
            )
            mock_telemetry.record_metric.assert_any_call(
                "processor_contract_violation_param", 2, {
                    "processor_type": "owl",
                    "contract_type": "precondition",
                    "context_key": "param"
                }
            )
    
    def test_handle_validation_error_success(self):
        mock_telemetry = Mock()
        error = TypeError("Validation error")
        context = {"constraint_type": 3}
        
        with patch('autotel.helpers.errors.handler.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            handle_validation_error(error, "shacl", "validate", context)
            # Main validation error metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_validation_error", 1, {
                    "processor_type": "shacl",
                    "operation": "validate",
                    "error_type": "TypeError",
                    "status": "failure"
                }
            )
            # Context metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_validation_error_constraint_type", 3, {
                    "processor_type": "shacl",
                    "operation": "validate",
                    "error_type": "TypeError",
                    "context_key": "constraint_type"
                }
            )
    
    def test_handle_xml_parsing_error_success(self):
        mock_telemetry = Mock()
        error = SyntaxError("XML parsing error")
        content_preview = "<bpmn:process ...>"
        
        with patch('autotel.helpers.errors.handler.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            handle_xml_parsing_error(error, "bpmn", "parse", content_preview)
            # Main XML parsing error metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_xml_parsing_error", 1, {
                    "processor_type": "bpmn",
                    "operation": "parse",
                    "error_type": "SyntaxError",
                    "status": "failure"
                }
            )
            # Content preview length metric
            mock_telemetry.record_metric.assert_any_call(
                "processor_xml_content_preview_length", len(content_preview), {
                    "processor_type": "bpmn",
                    "operation": "parse"
                }
            )
    
    def test_handle_telemetry_error_success(self):
        # This is a stub, just ensure it runs without error
        handle_telemetry_error(Exception("telemetry error")) 