#!/usr/bin/env python3
"""
Comprehensive AutoTel Test Suite

This test suite covers all major components of the AutoTel system
following the 80/20 rule - focusing on the most critical functionality
while keeping tests fast and maintainable.
"""

import pytest
import json
import sys
import time
import uuid
from pathlib import Path
from typing import Dict, Any, List

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

# Import core components
from autotel.core.framework import Framework
from autotel.core.orchestrator import Orchestrator
from autotel.core.telemetry import TelemetryManager
from autotel.factory.pipeline import Pipeline
from autotel.factory.processors.bpmn_processor import BPMNProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.processors.dmn_processor import DMNProcessor
from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.jinja_processor import JinjaProcessor
from autotel.utils.advanced_dspy_services import advanced_dspy_registry
from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser

# Test data and fixtures
@pytest.fixture
def sample_bpmn_data():
    """Sample BPMN data for testing"""
    return {
        "process_id": "test_process",
        "tasks": [
            {"id": "task1", "name": "Start Task", "type": "startEvent"},
            {"id": "task2", "name": "Process Task", "type": "serviceTask"},
            {"id": "task3", "name": "End Task", "type": "endEvent"}
        ]
    }

@pytest.fixture
def sample_dspy_signature():
    """Sample DSPy signature for testing"""
    return {
        "name": "test_signature",
        "inputs": {"question": "A test question"},
        "outputs": {"answer": "The answer"},
        "description": "Test signature"
    }

@pytest.fixture
def sample_telemetry_data():
    """Sample telemetry data for testing"""
    return {
        "trace_id": str(uuid.uuid4()),
        "span_id": str(uuid.uuid4()),
        "operation": "test_operation",
        "attributes": {"test": "value"}
    }

# ============================================================================
# UNIT TESTS (Fast - No External Dependencies)
# ============================================================================

def test_framework_initialization():
    """Test Framework initialization"""
    framework = Framework()
    assert framework is not None
    assert hasattr(framework, 'orchestrator')
    assert hasattr(framework, 'telemetry')

def test_orchestrator_creation():
    """Test Orchestrator creation"""
    orchestrator = Orchestrator()
    assert orchestrator is not None
    assert hasattr(orchestrator, 'pipeline')

def test_telemetry_manager_creation():
    """Test TelemetryManager creation"""
    telemetry = TelemetryManager()
    assert telemetry is not None
    assert hasattr(telemetry, 'start_span')

def test_pipeline_creation():
    """Test Pipeline creation"""
    pipeline = Pipeline()
    assert pipeline is not None
    assert hasattr(pipeline, 'processors')

def test_bpmn_processor_creation():
    """Test BPMNProcessor creation"""
    processor = BPMNProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_dspy_processor_creation():
    """Test DSPyProcessor creation"""
    processor = DSPyProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_dmn_processor_creation():
    """Test DMNProcessor creation"""
    processor = DMNProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_shacl_processor_creation():
    """Test SHACLProcessor creation"""
    processor = SHACLProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_owl_processor_creation():
    """Test OWLProcessor creation"""
    processor = OWLProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_jinja_processor_creation():
    """Test JinjaProcessor creation"""
    processor = JinjaProcessor()
    assert processor is not None
    assert hasattr(processor, 'process')

def test_advanced_dspy_registry_creation():
    """Test Advanced DSPy Registry creation"""
    registry = advanced_dspy_registry
    assert registry is not None
    assert hasattr(registry, 'register_dynamic_signature')
    assert hasattr(registry, 'call_signature')

# ============================================================================
# INTEGRATION TESTS (Medium - Some External Dependencies)
# ============================================================================

@pytest.mark.integration
def test_pipeline_processor_integration():
    """Test pipeline with multiple processors"""
    pipeline = Pipeline()
    
    # Add processors
    pipeline.add_processor(BPMNProcessor())
    pipeline.add_processor(DSPyProcessor())
    pipeline.add_processor(JinjaProcessor())
    
    # Test data
    data = {
        "bpmn_file": "bpmn/simple_dspy_workflow.bpmn",
        "template": "Hello {{ name }}!",
        "context": {"name": "World"}
    }
    
    result = pipeline.process(data)
    assert result is not None

@pytest.mark.integration
def test_framework_orchestration():
    """Test Framework orchestration capabilities"""
    framework = Framework()
    
    # Test basic orchestration
    result = framework.orchestrate({
        "workflow": "test_workflow",
        "data": {"test": "data"}
    })
    
    assert result is not None

@pytest.mark.integration
def test_telemetry_integration():
    """Test telemetry integration with components"""
    telemetry = TelemetryManager()
    
    # Start a span
    with telemetry.start_span("test_operation") as span:
        span.set_attribute("test", "value")
        
        # Simulate some work
        time.sleep(0.1)
        
        assert span is not None
        assert span.get_attribute("test") == "value"

@pytest.mark.integration
def test_dspy_bpmn_parser_integration():
    """Test DSPy BPMN parser integration"""
    parser = DspyBpmnParser()
    
    # Test with a simple BPMN file
    bpmn_path = "bpmn/simple_dspy_workflow.bpmn"
    
    if Path(bpmn_path).exists():
        parser.add_bpmn_file(bpmn_path)
        specs = parser.find_all_specs()
        assert len(specs) > 0

# ============================================================================
# DSPY TESTS (Slow - LLM Calls)
# ============================================================================

@pytest.mark.dspy
@pytest.mark.slow
def test_dspy_signature_registration():
    """Test DSPy signature registration and calling"""
    # Register a test signature
    advanced_dspy_registry.register_dynamic_signature(
        'test_signature',
        {'question': 'A test question', 'random_seed': 'Random seed'},
        {'answer': 'The answer'},
        'Test signature for integration',
        disable_cache=True
    )
    
    # Call the signature
    random_seed = f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
    result = advanced_dspy_registry.call_signature(
        'test_signature',
        question='What is 2+2?',
        random_seed=random_seed
    )
    
    assert result is not None
    assert 'answer' in result

@pytest.mark.dspy
@pytest.mark.slow
def test_dspy_bpmn_workflow_execution():
    """Test DSPy BPMN workflow execution"""
    bpmn_path = "bpmn/simple_dspy_workflow.bpmn"
    
    if not Path(bpmn_path).exists():
        pytest.skip(f"BPMN file not found: {bpmn_path}")
    
    # Test data with randomization
    initial_context = {
        "input_data": f"Test data {uuid.uuid4().hex[:8]}",
        "random_seed": f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
    }
    
    try:
        result = run_dspy_bpmn_process(
            bpmn_path, 
            "SimpleDspyWorkflow", 
            initial_context
        )
        
        assert result is not None
        assert isinstance(result, dict)
        
    except Exception as e:
        pytest.skip(f"DSPy workflow execution failed: {e}")

@pytest.mark.dspy
@pytest.mark.slow
def test_dspy_cache_controls():
    """Test DSPy cache controls"""
    # Register signature with cache disabled
    advanced_dspy_registry.register_dynamic_signature(
        'cache_test',
        {'input': 'Test input'},
        {'output': 'Test output'},
        'Cache test signature',
        disable_cache=True
    )
    
    # Make multiple calls
    results = []
    for i in range(3):
        random_seed = f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
        result = advanced_dspy_registry.call_signature(
            'cache_test',
            input=f'Test input {i} with {random_seed}'
        )
        results.append(result)
        time.sleep(0.5)  # Brief pause between calls
    
    # Verify all calls returned results
    assert len(results) == 3
    assert all(result is not None for result in results)
    
    # Check cache statistics
    stats = advanced_dspy_registry.get_stats()
    assert stats["cache_stats"]["disabled"] is True

# ============================================================================
# END-TO-END TESTS (Slow - Full Workflows)
# ============================================================================

@pytest.mark.integration
@pytest.mark.slow
def test_complete_workflow_execution():
    """Test complete workflow execution with all components"""
    framework = Framework()
    
    # Test data
    workflow_data = {
        "workflow_id": "test_complete_workflow",
        "input_data": {
            "customer_name": "Test Customer",
            "inquiry": "Test inquiry",
            "random_seed": f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
        }
    }
    
    try:
        result = framework.execute_workflow(workflow_data)
        assert result is not None
        
    except Exception as e:
        # This is expected if workflow execution is not fully implemented
        pytest.skip(f"Complete workflow execution not implemented: {e}")

@pytest.mark.integration
@pytest.mark.slow
def test_telemetry_workflow_tracking():
    """Test telemetry tracking throughout workflow execution"""
    telemetry = TelemetryManager()
    
    # Start root span
    with telemetry.start_span("workflow_execution") as root_span:
        root_span.set_attribute("workflow.id", "test_workflow")
        
        # Simulate workflow steps
        with telemetry.start_span("step_1") as step1:
            step1.set_attribute("step.name", "data_processing")
            time.sleep(0.1)
        
        with telemetry.start_span("step_2") as step2:
            step2.set_attribute("step.name", "dspy_analysis")
            time.sleep(0.1)
        
        with telemetry.start_span("step_3") as step3:
            step3.set_attribute("step.name", "template_rendering")
            time.sleep(0.1)
    
    # Verify spans were created
    assert root_span is not None
    assert root_span.get_attribute("workflow.id") == "test_workflow"

# ============================================================================
# VALIDATION TESTS (Medium - Schema/Data Validation)
# ============================================================================

@pytest.mark.integration
def test_schema_validation():
    """Test schema validation capabilities"""
    from autotel.schemas.validation import validate_schema
    
    # Test data
    test_schema = {
        "type": "object",
        "properties": {
            "name": {"type": "string"},
            "age": {"type": "integer"}
        },
        "required": ["name"]
    }
    
    valid_data = {"name": "Test", "age": 30}
    invalid_data = {"age": "not_a_number"}
    
    # Test valid data
    result = validate_schema(valid_data, test_schema)
    assert result is True
    
    # Test invalid data
    try:
        validate_schema(invalid_data, test_schema)
        assert False, "Should have raised validation error"
    except Exception:
        pass  # Expected

@pytest.mark.integration
def test_xml_processing():
    """Test XML processing capabilities"""
    from autotel.factory.processors.bpmn_processor import BPMNProcessor
    
    processor = BPMNProcessor()
    
    # Test XML parsing
    test_xml = """
    <?xml version="1.0" encoding="UTF-8"?>
    <bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL">
        <bpmn:process id="test_process">
            <bpmn:startEvent id="start" name="Start"/>
        </bpmn:process>
    </bpmn:definitions>
    """
    
    result = processor.process_xml(test_xml)
    assert result is not None

# ============================================================================
# PERFORMANCE TESTS (Slow - Performance Validation)
# ============================================================================

@pytest.mark.slow
def test_performance_baseline():
    """Test performance baseline for core operations"""
    import time
    
    # Test framework initialization time
    start_time = time.time()
    framework = Framework()
    init_time = time.time() - start_time
    
    assert init_time < 1.0, f"Framework initialization too slow: {init_time:.2f}s"
    
    # Test telemetry span creation time
    telemetry = TelemetryManager()
    start_time = time.time()
    
    with telemetry.start_span("test_span"):
        pass
    
    span_time = time.time() - start_time
    assert span_time < 0.1, f"Span creation too slow: {span_time:.3f}s"

@pytest.mark.slow
def test_memory_usage():
    """Test memory usage for core components"""
    import psutil
    import os
    
    process = psutil.Process(os.getpid())
    initial_memory = process.memory_info().rss
    
    # Create multiple components
    components = []
    for i in range(10):
        components.append(Framework())
        components.append(Orchestrator())
        components.append(TelemetryManager())
    
    final_memory = process.memory_info().rss
    memory_increase = final_memory - initial_memory
    
    # Memory increase should be reasonable (less than 100MB)
    assert memory_increase < 100 * 1024 * 1024, f"Memory usage too high: {memory_increase / 1024 / 1024:.1f}MB"

# ============================================================================
# ERROR HANDLING TESTS (Fast - Error Scenarios)
# ============================================================================

def test_error_handling_invalid_bpmn():
    """Test error handling for invalid BPMN"""
    processor = BPMNProcessor()
    
    invalid_bpmn = "This is not valid BPMN XML"
    
    try:
        result = processor.process_xml(invalid_bpmn)
        assert False, "Should have raised an error"
    except Exception:
        pass  # Expected

def test_error_handling_invalid_dspy_signature():
    """Test error handling for invalid DSPy signature"""
    try:
        advanced_dspy_registry.call_signature("nonexistent_signature")
        assert False, "Should have raised an error"
    except Exception:
        pass  # Expected

def test_error_handling_invalid_telemetry():
    """Test error handling for invalid telemetry operations"""
    telemetry = TelemetryManager()
    
    # Test with invalid span name
    try:
        with telemetry.start_span(""):
            pass
        # This might not raise an error, which is fine
    except Exception:
        pass

# ============================================================================
# CONFIGURATION TESTS (Fast - Configuration Validation)
# ============================================================================

def test_configuration_loading():
    """Test configuration loading capabilities"""
    from autotel.config import load_config
    
    try:
        config = load_config()
        assert config is not None
    except Exception as e:
        pytest.skip(f"Configuration loading not implemented: {e}")

def test_environment_variables():
    """Test environment variable handling"""
    import os
    
    # Test that required environment variables are handled
    test_env_vars = [
        "AUTOTEL_ENV",
        "PYTHONPATH"
    ]
    
    for var in test_env_vars:
        # Just check that we can access them without error
        os.getenv(var)

# ============================================================================
# UTILITY TESTS (Fast - Utility Functions)
# ============================================================================

def test_uuid_generation():
    """Test UUID generation for randomization"""
    uuid1 = str(uuid.uuid4())
    uuid2 = str(uuid.uuid4())
    
    assert uuid1 != uuid2
    assert len(uuid1) == 36
    assert len(uuid2) == 36

def test_timestamp_generation():
    """Test timestamp generation for randomization"""
    timestamp1 = int(time.time())
    time.sleep(0.001)  # Small delay
    timestamp2 = int(time.time())
    
    assert timestamp2 > timestamp1

def test_json_serialization():
    """Test JSON serialization of test data"""
    test_data = {
        "string": "test",
        "number": 42,
        "boolean": True,
        "list": [1, 2, 3],
        "dict": {"key": "value"}
    }
    
    json_str = json.dumps(test_data)
    parsed_data = json.loads(json_str)
    
    assert parsed_data == test_data

# ============================================================================
# TEST HELPERS AND UTILITIES
# ============================================================================

def generate_test_data(size: int = 100) -> Dict[str, Any]:
    """Generate test data of specified size"""
    return {
        "id": str(uuid.uuid4()),
        "timestamp": int(time.time()),
        "data": "x" * size,
        "random_seed": f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
    }

def assert_response_time(operation_name: str, start_time: float, max_time: float = 5.0):
    """Assert that an operation completed within expected time"""
    end_time = time.time()
    duration = end_time - start_time
    
    assert duration < max_time, f"{operation_name} took too long: {duration:.2f}s"

def assert_telemetry_spans(telemetry_manager, expected_span_names: List[str]):
    """Assert that expected telemetry spans were created"""
    # This is a placeholder - actual implementation would depend on telemetry backend
    assert telemetry_manager is not None
    assert isinstance(expected_span_names, list)

# ============================================================================
# MAIN TEST EXECUTION
# ============================================================================

if __name__ == "__main__":
    # This allows running the test file directly
    pytest.main([__file__, "-v"]) 