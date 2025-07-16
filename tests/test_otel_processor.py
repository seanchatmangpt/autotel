"""
Tests for OTEL processor functionality.

This module provides comprehensive tests for the OTEL processor including:
- Data parsing and validation
- Analysis and metrics calculation
- Data conversion and transformation
- File processing capabilities
- CLI integration
- Edge cases and error handling
"""

import json
import tempfile
import pytest
from pathlib import Path
from unittest.mock import patch, MagicMock

from autotel.factory.processors.otel_processor import OTELProcessor
from autotel.schemas.otel_types import (
    OTELSpanDefinition, OTELMetricDefinition, OTELLogDefinition, OTELTraceDefinition,
    OTELAnalysisResult, OTELProcessingConfig
)


class TestOTELProcessor:
    """Test suite for OTEL processor functionality."""

    def setup_method(self):
        """Set up test fixtures."""
        self.processor = OTELProcessor()
        
        # Sample OTEL data for testing
        self.sample_spans = [
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "abcdef1234567890",
                "name": "test-span-1",
                "kind": 1,
                "startTimeUnixNano": "1640995200000000000",
                "endTimeUnixNano": "1640995201000000000",
                "attributes": [
                    {"key": "service.name", "value": {"stringValue": "test-service"}},
                    {"key": "http.method", "value": {"stringValue": "GET"}}
                ],
                "events": [
                    {
                        "timeUnixNano": "1640995200500000000",
                        "name": "test-event",
                        "attributes": [
                            {"key": "event.type", "value": {"stringValue": "info"}}
                        ]
                    }
                ]
            },
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "fedcba0987654321",
                "name": "test-span-2",
                "kind": 2,
                "startTimeUnixNano": "1640995201000000000",
                "endTimeUnixNano": "1640995202000000000",
                "attributes": [
                    {"key": "service.name", "value": {"stringValue": "test-service"}},
                    {"key": "http.status_code", "value": {"intValue": 200}}
                ]
            }
        ]
        
        self.sample_metrics = [
            {
                "name": "http_requests_total",
                "description": "Total HTTP requests",
                "unit": "requests",
                "gauge": {
                    "dataPoints": [
                        {
                            "timeUnixNano": "1640995200000000000",
                            "value": 100.0,
                            "attributes": [
                                {"key": "method", "value": {"stringValue": "GET"}}
                            ]
                        }
                    ]
                }
            }
        ]
        
        self.sample_logs = [
            {
                "timeUnixNano": "1640995200000000000",
                "severityText": "INFO",
                "severityNumber": 9,
                "body": {"stringValue": "Test log message"},
                "attributes": [
                    {"key": "logger.name", "value": {"stringValue": "test-logger"}}
                ]
            }
        ]

    def test_otel_processor_initialization(self):
        """Test OTEL processor initialization."""
        processor = OTELProcessor()
        assert processor is not None
        assert hasattr(processor, 'parse_spans')
        assert hasattr(processor, 'analyze_spans')
        assert hasattr(processor, 'convert_spans')

    def test_parse_spans_valid_data(self):
        """Test parsing valid span data."""
        spans = self.processor.parse_spans(self.sample_spans)
        
        assert len(spans) == 2
        assert isinstance(spans[0], dict)  # Processor returns dict format
        assert spans[0]['trace_id'] == "1234567890abcdef1234567890abcdef"
        assert spans[0]['span_id'] == "abcdef1234567890"
        assert spans[0]['name'] == "test-span-1"
        assert spans[0]['kind'] == 1
        assert len(spans[0]['attributes']) == 2
        assert len(spans[0]['events']) == 1

    def test_parse_spans_empty_data(self):
        """Test parsing empty span data."""
        spans = self.processor.parse_spans([])
        assert spans == []

    def test_parse_spans_invalid_data(self):
        """Test parsing invalid span data."""
        invalid_spans = [{"invalid": "data"}]
        spans = self.processor.parse_spans(invalid_spans)
        # Should handle gracefully and return empty list or partial data
        assert isinstance(spans, list)

    def test_parse_metrics_valid_data(self):
        """Test parsing valid metric data."""
        metrics = self.processor.parse_metrics(self.sample_metrics)
        
        assert len(metrics) == 1
        assert isinstance(metrics[0], dict)  # Processor returns dict format
        assert metrics[0]['name'] == "http_requests_total"
        assert metrics[0]['description'] == "Total HTTP requests"
        assert metrics[0]['unit'] == "requests"

    def test_parse_logs_valid_data(self):
        """Test parsing valid log data."""
        logs = self.processor.parse_logs(self.sample_logs)
        
        assert len(logs) == 1
        assert isinstance(logs[0], dict)  # Processor returns dict format
        assert logs[0]['severity_text'] == "INFO"
        assert logs[0]['severity_number'] == 9
        assert logs[0]['body'] == "Test log message"

    def test_analyze_spans_basic_metrics(self):
        """Test basic span analysis metrics."""
        spans = self.processor.parse_spans(self.sample_spans)
        analysis = self.processor.analyze_spans(spans)
        
        assert analysis['total_spans'] == 2
        assert analysis['unique_traces'] == 1
        assert analysis['avg_duration_ns'] > 0
        assert analysis['min_duration_ns'] > 0
        assert analysis['max_duration_ns'] > 0

    def test_analyze_spans_empty_spans(self):
        """Test analysis with empty spans."""
        analysis = self.processor.analyze_spans([])
        
        assert analysis['total_spans'] == 0
        assert analysis['unique_traces'] == 0
        assert analysis['avg_duration_ns'] == 0
        assert analysis['min_duration_ns'] == 0
        assert analysis['max_duration_ns'] == 0

    def test_analyze_spans_performance_metrics(self):
        """Test performance analysis metrics."""
        spans = self.processor.parse_spans(self.sample_spans)
        analysis = self.processor.analyze_spans(spans)
        
        # Check that performance metrics are calculated
        assert 'performance_analysis' in analysis
        perf = analysis['performance_analysis']
        assert 'p50_duration_ns' in perf
        assert 'p95_duration_ns' in perf
        assert 'p99_duration_ns' in perf
        assert 'throughput_spans_per_second' in perf

    def test_analyze_spans_error_analysis(self):
        """Test error analysis in spans."""
        # Add error spans to test data
        error_spans = self.sample_spans + [
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "error1234567890",
                "name": "error-span",
                "kind": 1,
                "startTimeUnixNano": "1640995202000000000",
                "endTimeUnixNano": "1640995203000000000",
                "attributes": [
                    {"key": "error", "value": {"boolValue": True}},
                    {"key": "error.message", "value": {"stringValue": "Test error"}}
                ]
            }
        ]
        
        spans = self.processor.parse_spans(error_spans)
        analysis = self.processor.analyze_spans(spans)
        
        assert analysis['error_spans'] == 1
        assert analysis['error_rate'] > 0

    def test_convert_spans_to_dict(self):
        """Test converting spans to dictionary format."""
        spans = self.processor.parse_spans(self.sample_spans)
        converted = self.processor.convert_spans(spans, output_format='dict')
        
        assert isinstance(converted, list)
        assert len(converted) == 2
        assert isinstance(converted[0], dict)
        assert 'trace_id' in converted[0]
        assert 'span_id' in converted[0]
        assert 'name' in converted[0]

    def test_convert_spans_to_json(self):
        """Test converting spans to JSON format."""
        spans = self.processor.parse_spans(self.sample_spans)
        converted = self.processor.convert_spans(spans, output_format='json')
        
        assert isinstance(converted, str)
        # Verify it's valid JSON
        parsed = json.loads(converted)
        assert isinstance(parsed, list)
        assert len(parsed) == 2

    def test_convert_spans_invalid_format(self):
        """Test converting spans with invalid format."""
        spans = self.processor.parse_spans(self.sample_spans)
        converted = self.processor.convert_spans(spans, output_format='invalid')
        
        # Should default to dict format
        assert isinstance(converted, list)

    def test_process_otel_file_valid_json(self):
        """Test processing valid OTEL JSON file."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump({
                "resourceSpans": [{
                    "resource": {"attributes": []},
                    "scopeSpans": [{
                        "spans": self.sample_spans
                    }]
                }],
                "resourceMetrics": [{
                    "resource": {"attributes": []},
                    "scopeMetrics": [{
                        "metrics": self.sample_metrics
                    }]
                }],
                "resourceLogs": [{
                    "resource": {"attributes": []},
                    "scopeLogs": [{
                        "logs": self.sample_logs
                    }]
                }]
            }, f)
            file_path = f.name
        
        try:
            result = self.processor.process_otel_file(file_path)
            
            assert result is not None
            assert 'spans' in result
            assert 'metrics' in result
            assert 'logs' in result
            assert len(result['spans']) == 2
            assert len(result['metrics']) == 1
            assert len(result['logs']) == 1
        finally:
            Path(file_path).unlink()

    def test_process_otel_file_invalid_json(self):
        """Test processing invalid JSON file."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            f.write("invalid json content")
            file_path = f.name
        
        try:
            result = self.processor.process_otel_file(file_path)
            # Should handle gracefully
            assert result is not None
        finally:
            Path(file_path).unlink()

    def test_process_otel_file_nonexistent(self):
        """Test processing nonexistent file."""
        result = self.processor.process_otel_file("nonexistent_file.json")
        # Should handle gracefully
        assert result is not None

    def test_analyze_otel_data_comprehensive(self):
        """Test comprehensive OTEL data analysis."""
        otel_data = {
            'spans': self.processor.parse_spans(self.sample_spans),
            'metrics': self.processor.parse_metrics(self.sample_metrics),
            'logs': self.processor.parse_logs(self.sample_logs)
        }
        
        analysis = self.processor.analyze_otel_data(otel_data)
        
        assert 'spans_analysis' in analysis
        assert 'metrics_analysis' in analysis
        assert 'logs_analysis' in analysis
        assert 'overall_summary' in analysis
        
        # Check spans analysis
        spans_analysis = analysis['spans_analysis']
        assert spans_analysis['total_spans'] == 2
        assert spans_analysis['unique_traces'] == 1
        
        # Check metrics analysis
        metrics_analysis = analysis['metrics_analysis']
        assert metrics_analysis['total_metrics'] == 1
        
        # Check logs analysis
        logs_analysis = analysis['logs_analysis']
        assert logs_analysis['total_logs'] == 1

    def test_convert_otel_data_to_dict(self):
        """Test converting complete OTEL data to dictionary."""
        otel_data = {
            'spans': self.processor.parse_spans(self.sample_spans),
            'metrics': self.processor.parse_metrics(self.sample_metrics),
            'logs': self.processor.parse_logs(self.sample_logs)
        }
        
        converted = self.processor.convert_otel_data(otel_data, output_format='dict')
        
        assert isinstance(converted, dict)
        assert 'spans' in converted
        assert 'metrics' in converted
        assert 'logs' in converted
        assert isinstance(converted['spans'], list)
        assert isinstance(converted['metrics'], list)
        assert isinstance(converted['logs'], list)

    def test_convert_otel_data_to_json(self):
        """Test converting complete OTEL data to JSON."""
        otel_data = {
            'spans': self.processor.parse_spans(self.sample_spans),
            'metrics': self.processor.parse_metrics(self.sample_metrics),
            'logs': self.processor.parse_logs(self.sample_logs)
        }
        
        converted = self.processor.convert_otel_data(otel_data, output_format='json')
        
        assert isinstance(converted, str)
        parsed = json.loads(converted)
        assert isinstance(parsed, dict)
        assert 'spans' in parsed
        assert 'metrics' in parsed
        assert 'logs' in parsed

    def test_span_duration_calculation(self):
        """Test span duration calculation accuracy."""
        # Create spans with known durations
        test_spans = [
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span1",
                "name": "span-1",
                "kind": 1,
                "startTimeUnixNano": "1640995200000000000",  # 1000000000 ns
                "endTimeUnixNano": "1640995201000000000",    # 1010000000 ns
                "attributes": []
            },
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span2",
                "name": "span-2",
                "kind": 1,
                "startTimeUnixNano": "1640995201000000000",  # 1010000000 ns
                "endTimeUnixNano": "1640995203000000000",    # 1030000000 ns
                "attributes": []
            }
        ]
        
        spans = self.processor.parse_spans(test_spans)
        analysis = self.processor.analyze_spans(spans)
        
        # First span: 1010000000 - 1000000000 = 10000000 ns
        # Second span: 1030000000 - 1010000000 = 20000000 ns
        # Average: (10000000 + 20000000) / 2 = 15000000 ns
        expected_avg = 15000000
        assert abs(analysis['avg_duration_ns'] - expected_avg) < 1

    def test_trace_span_relationships(self):
        """Test trace and span relationship analysis."""
        # Create spans with different trace IDs
        multi_trace_spans = [
            {
                "traceId": "trace1",
                "spanId": "span1",
                "name": "span-1",
                "kind": 1,
                "startTimeUnixNano": "1640995200000000000",
                "endTimeUnixNano": "1640995201000000000",
                "attributes": []
            },
            {
                "traceId": "trace1",
                "spanId": "span2",
                "name": "span-2",
                "kind": 1,
                "startTimeUnixNano": "1640995201000000000",
                "endTimeUnixNano": "1640995202000000000",
                "attributes": []
            },
            {
                "traceId": "trace2",
                "spanId": "span3",
                "name": "span-3",
                "kind": 1,
                "startTimeUnixNano": "1640995202000000000",
                "endTimeUnixNano": "1640995203000000000",
                "attributes": []
            }
        ]
        
        spans = self.processor.parse_spans(multi_trace_spans)
        analysis = self.processor.analyze_spans(spans)
        
        assert analysis['total_spans'] == 3
        assert analysis['unique_traces'] == 2
        assert analysis['avg_spans_per_trace'] == 1.5

    def test_attribute_analysis(self):
        """Test attribute analysis in spans."""
        spans_with_attrs = [
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span1",
                "name": "span-1",
                "kind": 1,
                "startTimeUnixNano": "1640995200000000000",
                "endTimeUnixNano": "1640995201000000000",
                "attributes": [
                    {"key": "service.name", "value": {"stringValue": "service-a"}},
                    {"key": "http.method", "value": {"stringValue": "GET"}},
                    {"key": "http.status_code", "value": {"intValue": 200}}
                ]
            }
        ]
        
        spans = self.processor.parse_spans(spans_with_attrs)
        analysis = self.processor.analyze_spans(spans)
        
        assert 'attribute_analysis' in analysis
        attr_analysis = analysis['attribute_analysis']
        assert 'service.name' in attr_analysis
        assert 'http.method' in attr_analysis
        assert 'http.status_code' in attr_analysis

    def test_span_kind_analysis(self):
        """Test span kind analysis."""
        spans_with_kinds = [
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span1",
                "name": "span-1",
                "kind": 1,  # INTERNAL
                "startTimeUnixNano": "1640995200000000000",
                "endTimeUnixNano": "1640995201000000000",
                "attributes": []
            },
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span2",
                "name": "span-2",
                "kind": 2,  # SERVER
                "startTimeUnixNano": "1640995201000000000",
                "endTimeUnixNano": "1640995202000000000",
                "attributes": []
            },
            {
                "traceId": "1234567890abcdef1234567890abcdef",
                "spanId": "span3",
                "name": "span-3",
                "kind": 3,  # CLIENT
                "startTimeUnixNano": "1640995202000000000",
                "endTimeUnixNano": "1640995203000000000",
                "attributes": []
            }
        ]
        
        spans = self.processor.parse_spans(spans_with_kinds)
        analysis = self.processor.analyze_spans(spans)
        
        assert 'span_kind_analysis' in analysis
        kind_analysis = analysis['span_kind_analysis']
        assert kind_analysis['INTERNAL'] == 1
        assert kind_analysis['SERVER'] == 1
        assert kind_analysis['CLIENT'] == 1

    def test_metric_data_point_analysis(self):
        """Test metric data point analysis."""
        metrics_with_points = [
            {
                "name": "http_requests_total",
                "description": "Total HTTP requests",
                "unit": "requests",
                "gauge": {
                    "dataPoints": [
                        {
                            "timeUnixNano": "1640995200000000000",
                            "value": 100.0,
                            "attributes": [{"key": "method", "value": {"stringValue": "GET"}}]
                        },
                        {
                            "timeUnixNano": "1640995201000000000",
                            "value": 150.0,
                            "attributes": [{"key": "method", "value": {"stringValue": "POST"}}]
                        }
                    ]
                }
            }
        ]
        
        metrics = self.processor.parse_metrics(metrics_with_points)
        analysis = self.processor.analyze_metrics(metrics)
        
        assert analysis['total_metrics'] == 1
        assert analysis['total_data_points'] == 2
        assert analysis['avg_data_points_per_metric'] == 2.0

    def test_log_severity_analysis(self):
        """Test log severity analysis."""
        logs_with_severities = [
            {
                "timeUnixNano": "1640995200000000000",
                "severityText": "INFO",
                "severityNumber": 9,
                "body": {"stringValue": "Info message"},
                "attributes": []
            },
            {
                "timeUnixNano": "1640995201000000000",
                "severityText": "ERROR",
                "severityNumber": 17,
                "body": {"stringValue": "Error message"},
                "attributes": []
            },
            {
                "timeUnixNano": "1640995202000000000",
                "severityText": "WARN",
                "severityNumber": 13,
                "body": {"stringValue": "Warning message"},
                "attributes": []
            }
        ]
        
        logs = self.processor.parse_logs(logs_with_severities)
        analysis = self.processor.analyze_logs(logs)
        
        assert 'severity_analysis' in analysis
        severity_analysis = analysis['severity_analysis']
        assert severity_analysis['INFO'] == 1
        assert severity_analysis['ERROR'] == 1
        assert severity_analysis['WARN'] == 1

    def test_otel_data_validation(self):
        """Test OTEL data structure validation."""
        # Test with valid data
        valid_data = {
            'spans': self.processor.parse_spans(self.sample_spans),
            'metrics': self.processor.parse_metrics(self.sample_metrics),
            'logs': self.processor.parse_logs(self.sample_logs)
        }
        
        # Should not raise any exceptions
        self.processor.validate_otel_data(valid_data)
        
        # Test with empty data
        empty_data = {'spans': [], 'metrics': [], 'logs': []}
        self.processor.validate_otel_data(empty_data)

    def test_processor_integration_with_pipeline(self):
        """Test OTEL processor integration with pipeline orchestrator."""
        from autotel.factory.pipeline import PipelineOrchestrator
        
        orchestrator = PipelineOrchestrator()
        
        # Test that OTEL processor is available
        assert hasattr(orchestrator, 'process_otel_data')
        assert hasattr(orchestrator, 'process_otel_file')
        assert hasattr(orchestrator, 'analyze_otel_data')
        assert hasattr(orchestrator, 'convert_otel_data')

    @patch('autotel.factory.processors.otel_processor.OTELProcessor.parse_spans')
    def test_telemetry_integration(self, mock_parse_spans):
        """Test telemetry integration in OTEL processor."""
        # Mock the parse_spans method to return test data
        mock_parse_spans.return_value = self.processor.parse_spans(self.sample_spans)
        
        # Test that telemetry is properly integrated
        spans = self.processor.parse_spans(self.sample_spans)
        
        # Verify the method was called (telemetry should be triggered)
        mock_parse_spans.assert_called_once_with(self.sample_spans)
        
        # Verify we get the expected result
        assert len(spans) == 2


if __name__ == "__main__":
    pytest.main([__file__, "-v"]) 