"""
Pytest configuration and fixtures for AutoTel tests
"""

import pytest
import sys
import os
from pathlib import Path

# Add the project root to the path
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

# Test categories and markers
pytest_plugins = []

def pytest_configure(config):
    """Configure pytest with custom markers"""
    config.addinivalue_line(
        "markers", "unit: marks tests as unit tests (fast, no external dependencies)"
    )
    config.addinivalue_line(
        "markers", "integration: marks tests as integration tests (medium, some external dependencies)"
    )
    config.addinivalue_line(
        "markers", "dspy: marks tests that use DSPy/LLM calls (slow)"
    )
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (long running)"
    )
    config.addinivalue_line(
        "markers", "e2e: marks tests as end-to-end tests (full workflows)"
    )
    config.addinivalue_line(
        "markers", "performance: marks tests as performance tests"
    )
    config.addinivalue_line(
        "markers", "manual: marks tests that require manual intervention"
    )
    config.addinivalue_line(
        "markers", "skip: marks tests to be skipped by default"
    )

def pytest_collection_modifyitems(config, items):
    """Modify test collection to add default markers"""
    for item in items:
        # Add unit marker to tests without any marker
        if not any(item.iter_markers()):
            item.add_marker(pytest.mark.unit)

# ============================================================================
# FIXTURES
# ============================================================================

@pytest.fixture(scope="session")
def project_root_path():
    """Get the project root path"""
    return Path(__file__).parent.parent

@pytest.fixture(scope="session")
def test_data_dir():
    """Get the test data directory"""
    return Path(__file__).parent / "test_data"

@pytest.fixture(scope="session")
def bpmn_files_dir():
    """Get the BPMN files directory"""
    return project_root_path() / "bpmn"

@pytest.fixture
def sample_bpmn_xml():
    """Sample BPMN XML for testing"""
    return """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="Process_1" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:task id="Task_1" name="Process Task">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:task>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="EndEvent_1" />
  </bpmn:process>
</bpmn:definitions>"""

@pytest.fixture
def sample_dmn_xml():
    """Sample DMN XML for testing"""
    return """<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/"
             xmlns:dmndi="https://www.omg.org/spec/DMN/20191111/DMNDI/"
             xmlns:dc="http://www.omg.org/spec/DD/20100524/DC/"
             xmlns:di="http://www.omg.org/spec/DD/20100524/DI/"
             id="Definitions_1"
             name="DRD_1"
             targetNamespace="http://camunda.org/schema/1.0/dmn">
  <decision id="Decision_1" name="Sample Decision">
    <decisionTable id="DecisionTable_1" hitPolicy="UNIQUE">
      <input id="Input_1" label="Input">
        <inputExpression id="InputExpression_1" typeRef="string">
          <text>input</text>
        </inputExpression>
      </input>
      <output id="Output_1" label="Output" typeRef="string" />
      <rule id="Rule_1">
        <inputEntry id="InputEntry_1">
          <text>"test"</text>
        </inputEntry>
        <outputEntry id="OutputEntry_1">
          <text>"result"</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>"""

@pytest.fixture
def sample_owl_xml():
    """Sample OWL XML for testing"""
    return """<?xml version="1.0"?>
<rdf:RDF xmlns="http://example.com/ontology#"
         xml:base="http://example.com/ontology"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:xml="http://www.w3.org/XML/1998/namespace"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Ontology rdf:about="http://example.com/ontology"/>
    <owl:Class rdf:about="#Person">
        <rdfs:label>Person</rdfs:label>
        <rdfs:comment>A human being</rdfs:comment>
    </owl:Class>
    <owl:Class rdf:about="#Organization">
        <rdfs:label>Organization</rdfs:label>
        <rdfs:comment>An organization</rdfs:comment>
    </owl:Class>
</rdf:RDF>"""

@pytest.fixture
def sample_shacl_xml():
    """Sample SHACL XML for testing"""
    return """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    <sh:NodeShape rdf:about="#PersonShape">
        <sh:targetClass rdf:resource="http://example.com/ontology#Person"/>
        <sh:property>
            <sh:PropertyShape>
                <sh:path rdf:resource="http://example.com/ontology#name"/>
                <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
            </sh:PropertyShape>
        </sh:property>
    </sh:NodeShape>
</rdf:RDF>"""

@pytest.fixture
def sample_dspy_signature():
    """Sample DSPy signature for testing"""
    return {
        "name": "test_signature",
        "inputs": {
            "question": "A test question",
            "context": "Test context"
        },
        "outputs": {
            "answer": "The answer",
            "confidence": "Confidence score"
        },
        "description": "Test signature for unit testing"
    }

@pytest.fixture
def sample_telemetry_data():
    """Sample telemetry data for testing"""
    import uuid
    import time
    
    return {
        "trace_id": str(uuid.uuid4()),
        "span_id": str(uuid.uuid4()),
        "operation": "test_operation",
        "timestamp": int(time.time()),
        "attributes": {
            "test": "value",
            "component": "test_component"
        }
    }

@pytest.fixture
def sample_workflow_data():
    """Sample workflow data for testing"""
    import uuid
    import time
    
    return {
        "workflow_id": f"test_workflow_{uuid.uuid4().hex[:8]}",
        "process_id": "test_process",
        "input_data": {
            "customer_name": "Test Customer",
            "inquiry": "Test inquiry",
            "random_seed": f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
        },
        "metadata": {
            "created_at": int(time.time()),
            "version": "1.0.0"
        }
    }

@pytest.fixture
def mock_telemetry_manager():
    """Mock telemetry manager for testing"""
    class MockTelemetryManager:
        def __init__(self):
            self.spans = []
            self.attributes = {}
        
        def start_span(self, name):
            class MockSpan:
                def __init__(self, name):
                    self.name = name
                    self.attributes = {}
                
                def set_attribute(self, key, value):
                    self.attributes[key] = value
                
                def get_attribute(self, key):
                    return self.attributes.get(key)
                
                def __enter__(self):
                    return self
                
                def __exit__(self, exc_type, exc_val, exc_tb):
                    pass
            
            span = MockSpan(name)
            self.spans.append(span)
            return span
        
        def get_spans(self):
            return self.spans
    
    return MockTelemetryManager()

@pytest.fixture
def mock_dspy_registry():
    """Mock DSPy registry for testing"""
    class MockDSPyRegistry:
        def __init__(self):
            self.signatures = {}
            self.calls = []
        
        def register_dynamic_signature(self, name, inputs, outputs, description, disable_cache=False):
            self.signatures[name] = {
                "inputs": inputs,
                "outputs": outputs,
                "description": description,
                "disable_cache": disable_cache
            }
        
        def call_signature(self, name, **kwargs):
            if name not in self.signatures:
                raise ValueError(f"Signature {name} not found")
            
            self.calls.append({"name": name, "kwargs": kwargs})
            
            # Return mock output
            signature = self.signatures[name]
            result = {}
            for output_name in signature["outputs"]:
                result[output_name] = f"mock_{output_name}_value"
            
            return result
        
        def get_stats(self):
            return {
                "cache_stats": {
                    "hits": 0,
                    "misses": len(self.calls),
                    "hit_rate": 0.0,
                    "disabled": True
                },
                "signatures": len(self.signatures),
                "calls": len(self.calls)
            }
    
    return MockDSPyRegistry()

# ============================================================================
# TEST HELPERS
# ============================================================================

def create_test_file(content: str, filename: str, directory: Path = None):
    """Create a temporary test file"""
    if directory is None:
        directory = Path(__file__).parent / "temp"
    
    directory.mkdir(exist_ok=True)
    file_path = directory / filename
    
    with open(file_path, 'w') as f:
        f.write(content)
    
    return file_path

def cleanup_test_files(directory: Path = None):
    """Clean up temporary test files"""
    if directory is None:
        directory = Path(__file__).parent / "temp"
    
    if directory.exists():
        import shutil
        shutil.rmtree(directory)

def assert_telemetry_spans_created(telemetry_manager, expected_span_names):
    """Assert that expected telemetry spans were created"""
    spans = telemetry_manager.get_spans()
    span_names = [span.name for span in spans]
    
    for expected_name in expected_span_names:
        assert expected_name in span_names, f"Expected span '{expected_name}' not found in {span_names}"

def assert_response_time(operation_name: str, start_time: float, max_time: float = 5.0):
    """Assert that an operation completed within expected time"""
    import time
    end_time = time.time()
    duration = end_time - start_time
    
    assert duration < max_time, f"{operation_name} took too long: {duration:.2f}s"

def generate_random_data(size: int = 100):
    """Generate random test data"""
    import uuid
    import time
    
    return {
        "id": str(uuid.uuid4()),
        "timestamp": int(time.time()),
        "data": "x" * size,
        "random_seed": f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
    }

# ============================================================================
# TEST CATEGORIES
# ============================================================================

def pytest_generate_tests(metafunc):
    """Generate test parameters for parametrized tests"""
    if "test_category" in metafunc.fixturenames:
        categories = ["unit", "integration", "dspy", "slow", "e2e"]
        metafunc.parametrize("test_category", categories)

# ============================================================================
# ENVIRONMENT SETUP
# ============================================================================

def pytest_sessionstart(session):
    """Setup test session"""
    # Set test environment
    os.environ["AUTOTEL_ENV"] = "test"
    os.environ["PYTHONPATH"] = str(project_root)
    
    # Create test directories if they don't exist
    test_dirs = [
        project_root / "tests" / "temp",
        project_root / "tests" / "test_data"
    ]
    
    for test_dir in test_dirs:
        test_dir.mkdir(exist_ok=True)

def pytest_sessionfinish(session, exitstatus):
    """Cleanup test session"""
    # Clean up temporary files
    cleanup_test_files()
    
    # Print test summary
    print(f"\nTest session completed with status: {exitstatus}")
    print(f"Total tests run: {session.testscollected}") 