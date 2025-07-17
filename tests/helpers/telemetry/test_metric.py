"""
Tests for AutoTel Telemetry Metric Helpers - Happy Path Validation

Tests the successful execution paths for all metric helper functions,
focusing on proper metric recording, performance monitoring, and
success/failure tracking.
"""

import pytest
import time
from unittest.mock import Mock, patch, MagicMock
from autotel.helpers.telemetry.metric import (
    record_processor_metric,
    record_processing_duration,
    record_processing_success,
    record_processing_failure,
    record_contract_violation,
    record_performance_metrics,
    record_batch_metrics
)


class TestMetricHelpers:
    """Test suite for metric helper functions - happy path validation."""
    
    def test_record_processor_metric_success(self):
        """Test successful recording of custom processor metric."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test metric recording
            record_processor_metric("elements_parsed", 15, {
                "processor_type": "bpmn",
                "operation": "parse"
            })
            
            # Verify telemetry manager was called correctly
            mock_telemetry.record_metric.assert_called_once()
            
            # Verify metric name and value
            call_args = mock_telemetry.record_metric.call_args
            assert call_args[0][0] == "elements_parsed"  # metric_name
            assert call_args[0][1] == 15  # value
            
            # Verify enhanced tags
            tags = call_args[0][2]  # tags
            assert tags["processor_type"] == "bpmn"
            assert tags["operation"] == "parse"
            assert tags["helper_version"] == "1.0.0"
            assert tags["metric_type"] == "processor_custom"
    
    def test_record_processing_duration_success(self):
        """Test successful recording of processing duration."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test duration recording
            record_processing_duration("parse", 45.2, "bpmn", {
                "input_size": "large",
                "cache_used": "true"
            })
            
            # Verify two metrics were recorded (general and processor-specific)
            assert mock_telemetry.record_metric.call_count == 2
            
            # Verify general duration metric
            general_call = mock_telemetry.record_metric.call_args_list[0]
            assert general_call[0][0] == "processor_duration_ms"
            assert general_call[0][1] == 45.2
            
            general_tags = general_call[0][2]
            assert general_tags["processor_type"] == "bpmn"
            assert general_tags["operation"] == "parse"
            assert general_tags["metric_type"] == "duration"
            assert general_tags["unit"] == "milliseconds"
            assert general_tags["input_size"] == "large"
            assert general_tags["cache_used"] == "true"
            
            # Verify processor-specific duration metric
            specific_call = mock_telemetry.record_metric.call_args_list[1]
            assert specific_call[0][0] == "bpmn_parse_duration_ms"
            assert specific_call[0][1] == 45.2
    
    def test_record_processing_success_success(self):
        """Test successful recording of processing success."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test success recording with metadata
            record_processing_success("dspy", "parse", {
                "signatures_count": 5,
                "modules_count": 3,
                "validation_rules": 12
            })
            
            # Verify multiple metrics were recorded
            assert mock_telemetry.record_metric.call_count >= 3
            
            # Verify success counter
            success_call = mock_telemetry.record_metric.call_args_list[0]
            assert success_call[0][0] == "processor_operation_success"
            assert success_call[0][1] == 1
            
            success_tags = success_call[0][2]
            assert success_tags["processor_type"] == "dspy"
            assert success_tags["operation"] == "parse"
            assert success_tags["status"] == "success"
            assert success_tags["metric_type"] == "counter"
            
            # Verify processor-specific success metric
            processor_call = mock_telemetry.record_metric.call_args_list[1]
            assert processor_call[0][0] == "dspy_operation_success"
            assert processor_call[0][1] == 1
            
            # Verify metadata metrics
            metadata_calls = mock_telemetry.record_metric.call_args_list[2:]
            metadata_metric_names = [call[0][0] for call in metadata_calls]
            assert "processor_metadata_signatures_count" in metadata_metric_names
            assert "processor_metadata_modules_count" in metadata_metric_names
            assert "processor_metadata_validation_rules" in metadata_metric_names
    
    def test_record_processing_failure_success(self):
        """Test successful recording of processing failure."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test failure recording with context
            record_processing_failure("shacl", "validate", "ValidationError", {
                "constraint_type": "cardinality",
                "input_size": 1024,
                "severity": "high"
            })
            
            # Verify multiple metrics were recorded
            assert mock_telemetry.record_metric.call_count >= 3
            
            # Verify failure counter
            failure_call = mock_telemetry.record_metric.call_args_list[0]
            assert failure_call[0][0] == "processor_operation_error"
            assert failure_call[0][1] == 1
            
            failure_tags = failure_call[0][2]
            assert failure_tags["processor_type"] == "shacl"
            assert failure_tags["operation"] == "validate"
            assert failure_tags["status"] == "failure"
            assert failure_tags["error_type"] == "ValidationError"
            assert failure_tags["metric_type"] == "counter"
            
            # Verify processor-specific failure metric
            processor_call = mock_telemetry.record_metric.call_args_list[1]
            assert processor_call[0][0] == "shacl_operation_error"
            assert processor_call[0][1] == 1
            
            # Verify error type specific metric
            error_call = mock_telemetry.record_metric.call_args_list[2]
            assert error_call[0][0] == "processor_error_ValidationError"
            assert error_call[0][1] == 1
            
            # Verify context metrics
            context_calls = mock_telemetry.record_metric.call_args_list[3:]
            context_metric_names = [call[0][0] for call in context_calls]
            assert "processor_error_context_input_size" in context_metric_names
    
    def test_record_contract_violation_success(self):
        """Test successful recording of contract violation."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test contract violation recording
            record_contract_violation("owl", "precondition", {
                "method": "parse_ontology_definition",
                "parameter": "xml_content",
                "expected_type": "str",
                "actual_type": "int"
            })
            
            # Verify multiple metrics were recorded
            assert mock_telemetry.record_metric.call_count >= 3
            
            # Verify contract violation counter
            violation_call = mock_telemetry.record_metric.call_args_list[0]
            assert violation_call[0][0] == "processor_contract_violation"
            assert violation_call[0][1] == 1
            
            violation_tags = violation_call[0][2]
            assert violation_tags["processor_type"] == "owl"
            assert violation_tags["contract_type"] == "precondition"
            assert violation_tags["metric_type"] == "counter"
            assert violation_tags["violation"] == "true"
            
            # Verify processor-specific violation metric
            processor_call = mock_telemetry.record_metric.call_args_list[1]
            assert processor_call[0][0] == "owl_contract_violation"
            assert processor_call[0][1] == 1
            
            # Verify contract type specific violation metric
            contract_call = mock_telemetry.record_metric.call_args_list[2]
            assert contract_call[0][0] == "processor_contract_precondition_violation"
            assert contract_call[0][1] == 1
    
    def test_record_performance_metrics_success(self):
        """Test successful recording of comprehensive performance metrics."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test performance metrics recording
            start_time = time.time() - 0.1  # 100ms ago
            additional_metrics = {
                "memory_usage_mb": 45.2,
                "cpu_usage_percent": 12.5,
                "cache_hits": 15
            }
            
            record_performance_metrics("dspy", "parse", start_time, additional_metrics=additional_metrics)
            
            # Verify multiple metrics were recorded (duration + additional metrics)
            assert mock_telemetry.record_metric.call_count >= 4
            
            # Verify duration metrics were called
            duration_calls = [call for call in mock_telemetry.record_metric.call_args_list 
                            if "duration_ms" in call[0][0]]
            assert len(duration_calls) >= 2  # General and processor-specific
            
            # Verify additional metrics were recorded
            additional_calls = [call for call in mock_telemetry.record_metric.call_args_list 
                              if "memory_usage_mb" in call[0][0] or 
                                 "cpu_usage_percent" in call[0][0] or
                                 "cache_hits" in call[0][0]]
            assert len(additional_calls) == 3
    
    def test_record_batch_metrics_success(self):
        """Test successful recording of batch processing metrics."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Test batch metrics recording
            record_batch_metrics("shacl", "validate", 100, 95, 5, 1250.5)
            
            # Verify multiple metrics were recorded
            assert mock_telemetry.record_metric.call_count >= 6
            
            # Verify batch size metric
            batch_size_call = mock_telemetry.record_metric.call_args_list[0]
            assert batch_size_call[0][0] == "shacl_validate_batch_size"
            assert batch_size_call[0][1] == 100
            
            # Verify success count metric
            success_call = mock_telemetry.record_metric.call_args_list[1]
            assert success_call[0][0] == "shacl_validate_batch_success"
            assert success_call[0][1] == 95
            
            # Verify failed count metric
            failed_call = mock_telemetry.record_metric.call_args_list[2]
            assert failed_call[0][0] == "shacl_validate_batch_failed"
            assert failed_call[0][1] == 5
            
            # Verify duration metric
            duration_call = mock_telemetry.record_metric.call_args_list[3]
            assert duration_call[0][0] == "shacl_validate_batch_duration_ms"
            assert duration_call[0][1] == 1250.5
            
            # Verify success rate metric (95%)
            success_rate_call = mock_telemetry.record_metric.call_args_list[4]
            assert success_rate_call[0][0] == "shacl_validate_batch_success_rate"
            assert success_rate_call[0][1] == 95.0
            
            # Verify throughput metric (use pytest.approx for floating point comparison)
            throughput_call = mock_telemetry.record_metric.call_args_list[5]
            assert throughput_call[0][0] == "shacl_validate_batch_throughput"
            assert throughput_call[0][1] == pytest.approx(80.0, rel=1e-3)  # 100 items / 1.25 seconds
    
    def test_metric_helpers_integration_success(self):
        """Test successful integration of all metric helpers in a typical workflow."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            # Simulate a typical processor workflow
            start_time = time.time()
            
            # Record custom metric
            record_processor_metric("elements_found", 25, {
                "processor_type": "bpmn",
                "operation": "parse"
            })
            
            # Record processing duration
            record_processing_duration("parse", 67.8, "bpmn", {
                "input_size": "medium"
            })
            
            # Record success with metadata
            record_processing_success("bpmn", "parse", {
                "workflows_parsed": 3,
                "tasks_found": 15
            })
            
            # Record performance metrics (fix parameter order)
            record_performance_metrics("bpmn", "parse", start_time, additional_metrics={
                "memory_usage_mb": 32.1
            })
            
            # Verify multiple metrics were recorded
            assert mock_telemetry.record_metric.call_count >= 8
            
            # Verify different metric types were recorded
            metric_names = [call[0][0] for call in mock_telemetry.record_metric.call_args_list]
            assert "elements_found" in metric_names
            assert "processor_duration_ms" in metric_names
            assert "bpmn_parse_duration_ms" in metric_names
            assert "processor_operation_success" in metric_names
            assert "bpmn_operation_success" in metric_names
            assert "bpmn_parse_memory_usage_mb" in metric_names
    
    def test_multiple_processor_types_metrics_success(self):
        """Test successful metric recording for different processor types."""
        # Mock telemetry manager
        mock_telemetry = Mock()
        
        processor_types = ["bpmn", "dmn", "dspy", "shacl", "owl", "jinja", "otel"]
        
        with patch('autotel.helpers.telemetry.metric.get_telemetry_manager_or_noop', return_value=mock_telemetry):
            for processor_type in processor_types:
                # Reset mock for each iteration
                mock_telemetry.reset_mock()
                
                # Test success recording for each processor type
                record_processing_success(processor_type, "validate")
                
                # Verify processor-specific metric was recorded
                mock_telemetry.record_metric.assert_any_call(
                    f"{processor_type}_operation_success", 
                    1, 
                    pytest.approx({
                        "processor_type": processor_type,
                        "operation": "validate",
                        "status": "success",
                        "metric_type": "counter"
                    })
                ) 