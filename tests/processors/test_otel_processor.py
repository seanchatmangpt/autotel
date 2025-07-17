import pytest
import json
from autotel.processors.otel_processor import OTELProcessor
from autotel.processors.base import ProcessorConfig

@pytest.fixture
def processor():
    config = ProcessorConfig(name="otel_processor")
    return OTELProcessor(config=config)

@pytest.fixture
def otel_json():
    # Minimal OTEL-like JSON structure using factory_boy style
    return json.dumps({
        "spans": [
            {"name": "span1", "context": {"trace_id": "abc", "span_id": "123"}, "start_time": "2024-01-01T00:00:00Z", "end_time": "2024-01-01T00:00:01Z", "status": {"status_code": "OK"}, "attributes": {}, "events": [], "links": [], "kind": "INTERNAL", "resource": {}}
        ],
        "metrics": [
            {"name": "metric1", "value": 42, "unit": "ms", "description": "desc", "attributes": {}, "timestamp": "2024-01-01T00:00:00Z"}
        ],
        "logs": [
            {"timestamp": "2024-01-01T00:00:00Z", "severity": "INFO", "message": "log1", "attributes": {}, "resource": {}, "trace_id": "abc", "span_id": "123"}
        ]
    })

def test_otel_processor_happy_path(processor, otel_json):
    result = processor.process(otel_json)
    assert result.success
    assert "spans" in result.data
    assert "metrics" in result.data
    assert "logs" in result.data
    assert len(result.data["spans"]) == 1
    assert len(result.data["metrics"]) == 1
    assert len(result.data["logs"]) == 1
    assert result.metadata["spans_count"] == 1
    assert result.metadata["metrics_count"] == 1
    assert result.metadata["logs_count"] == 1 
 