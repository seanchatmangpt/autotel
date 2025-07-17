"""
Pytest configuration and fixtures for AutoTel tests
"""

import pytest
import sys
import os
from pathlib import Path
import logging 

logging.getLogger('litellm.llms.custom_httpx.async_client_cleanup').setLevel(logging.INFO)

# Add the project root to the path
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

# Import factories for dynamic test data generation
try:
    from .factories import (
        BPMNXMLFactory, DMNXMLFactory, OWLXMLFactory, SHACLXMLFactory, DSPyXMLFactory,
        DSPySignatureDefinitionFactory, TelemetryDataFactory, WorkflowContextFactory,
        TestFileFactory, create_comprehensive_test_suite
    )
except ImportError:
    # Fallback for when running tests directly
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from factories import (
        BPMNXMLFactory, DMNXMLFactory, OWLXMLFactory, SHACLXMLFactory, DSPyXMLFactory,
        DSPySignatureDefinitionFactory, TelemetryDataFactory, WorkflowContextFactory,
        TestFileFactory, create_comprehensive_test_suite
    )

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
    """Dynamic BPMN XML for testing using Factory Boy"""
    return BPMNXMLFactory()['xml_content']

@pytest.fixture
def sample_dmn_xml():
    """Dynamic DMN XML for testing using Factory Boy"""
    return DMNXMLFactory()['xml_content']

@pytest.fixture
def sample_owl_xml():
    """Dynamic OWL XML for testing using Factory Boy"""
    return OWLXMLFactory()['xml_content']

@pytest.fixture
def sample_shacl_xml():
    """Dynamic SHACL XML for testing using Factory Boy"""
    return SHACLXMLFactory()['xml_content']

@pytest.fixture
def sample_dspy_signature():
    """Dynamic DSPy signature for testing using Factory Boy"""
    return DSPySignatureDefinitionFactory().__dict__

@pytest.fixture
def sample_telemetry_data():
    """Dynamic telemetry data for testing using Factory Boy"""
    return TelemetryDataFactory()

@pytest.fixture
def sample_workflow_data():
    """Dynamic workflow data for testing using Factory Boy"""
    return WorkflowContextFactory()

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

@pytest.fixture
def dynamic_test_suite():
    """Comprehensive dynamic test suite with all data types"""
    return create_comprehensive_test_suite()

@pytest.fixture
def dynamic_bpmn_files():
    """Multiple dynamic BPMN files for testing"""
    return create_bpmn_test_data(3)

@pytest.fixture
def dynamic_dmn_files():
    """Multiple dynamic DMN files for testing"""
    return create_dmn_test_data(2)

@pytest.fixture
def dynamic_owl_files():
    """Multiple dynamic OWL files for testing"""
    return create_owl_test_data(2)

@pytest.fixture
def dynamic_shacl_files():
    """Multiple dynamic SHACL files for testing"""
    return create_shacl_test_data(2)

@pytest.fixture
def dynamic_dspy_files():
    """Multiple dynamic DSPy files for testing"""
    return create_dspy_test_data(3)

@pytest.fixture
def dynamic_test_files():
    """Dynamic test file objects for testing"""
    return [TestFileFactory() for _ in range(5)]

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
    """Generate random test data using Factory Boy"""
    return WorkflowContextFactory(context_size=size)

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

@pytest.fixture(autouse=True, scope="session")
def suppress_litellm_logging():
    # Suppress all LiteLLM and related logs below WARNING
    for logger_name in [
        "LiteLLM", "litellm", "httpx", "httpcore"
    ]:
        logging.getLogger(logger_name).setLevel(logging.WARNING) 