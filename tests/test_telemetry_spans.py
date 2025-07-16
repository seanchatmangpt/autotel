import pytest
import tempfile
from unittest.mock import patch
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, SimpleSpanProcessor
from opentelemetry import trace

from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.dmn_processor import DMNProcessor
from autotel.core.telemetry import create_telemetry_manager

# Simple test to verify telemetry manager works
def test_telemetry_manager_basic():
    """Test that telemetry manager can create spans and write them to a file."""
    tmpfile = tempfile.NamedTemporaryFile(mode="w+", delete=False)
    try:
        # Create a tracer provider that writes to our file
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider and disable LinkML validation
        telemetry = create_telemetry_manager(
            service_name="test-service",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        # Create a span with a valid operation type
        with telemetry.start_span("test_span", "schema_loading") as span:
            span.set_attribute("test_attr", "test_value")
        
        # Force flush and shutdown while file is still open
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        output = tmpfile.read()
        
        # Verify span was written
        assert "test_span" in output
        assert "schema_loading" in output
    finally:
        tmpfile.close()

# Test SHACL processor with monkeypatching
def test_shaclprocessor_span():
    """Test SHACL processor creates expected spans."""
    tmpfile = tempfile.NamedTemporaryFile(mode="w+", delete=False)
    try:
        # Create tracer provider for this test
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        test_telemetry = create_telemetry_manager(
            service_name="autotel-shacl-processor",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        # Patch the telemetry manager creation at the module level
        import autotel.factory.processors.shacl_processor
        original_create = autotel.factory.processors.shacl_processor.create_telemetry_manager
        autotel.factory.processors.shacl_processor.create_telemetry_manager = lambda *args, **kwargs: test_telemetry
        
        try:
            processor = SHACLProcessor()
            dummy_xml = '<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"></rdf:RDF>'
            try:
                result = processor.parse(dummy_xml)
            except Exception as e:
                pass
        finally:
            # Restore original function
            autotel.factory.processors.shacl_processor.create_telemetry_manager = original_create
        
        # Force flush and shutdown while file is still open
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        output = tmpfile.read()
        
        assert "shacl_parse" in output
    finally:
        tmpfile.close()

# Test DSPy processor with monkeypatching
def test_dspyprocessor_span():
    """Test DSPy processor creates expected spans."""
    tmpfile = tempfile.NamedTemporaryFile(mode="w+", delete=False)
    try:
        # Create tracer provider for this test
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        test_telemetry = create_telemetry_manager(
            service_name="autotel-dspy-processor",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        # Patch the telemetry manager creation at the module level
        import autotel.factory.processors.dspy_processor
        original_create = autotel.factory.processors.dspy_processor.create_telemetry_manager
        autotel.factory.processors.dspy_processor.create_telemetry_manager = lambda *args, **kwargs: test_telemetry
        
        try:
            processor = DSPyProcessor()
            dummy_xml = '<dspy:root xmlns:dspy="http://autotel.ai/dspy"></dspy:root>'
            try:
                processor.parse(dummy_xml)
            except Exception:
                pass
        finally:
            # Restore original function
            autotel.factory.processors.dspy_processor.create_telemetry_manager = original_create
        
        # Force flush and shutdown while file is still open
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        output = tmpfile.read()
        
        assert "dspy_parse_signatures" in output
    finally:
        tmpfile.close()

# Test OWL processor with monkeypatching
def test_owlprocessor_span():
    """Test OWL processor creates expected spans."""
    tmpfile = tempfile.NamedTemporaryFile(mode="w+", delete=False)
    try:
        # Create tracer provider for this test
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        test_telemetry = create_telemetry_manager(
            service_name="autotel-owl-processor",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        # Patch the telemetry manager creation at the module level
        import autotel.factory.processors.owl_processor
        original_create = autotel.factory.processors.owl_processor.create_telemetry_manager
        autotel.factory.processors.owl_processor.create_telemetry_manager = lambda *args, **kwargs: test_telemetry
        
        try:
            processor = OWLProcessor()
            dummy_xml = '<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"></rdf:RDF>'
            try:
                processor.parse_ontology_definition(dummy_xml)
            except Exception:
                pass
        finally:
            # Restore original function
            autotel.factory.processors.owl_processor.create_telemetry_manager = original_create
        
        # Force flush and shutdown while file is still open
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        output = tmpfile.read()
        
        assert "owl_parse_ontology" in output
    finally:
        tmpfile.close()

# Test DMN processor with monkeypatching
def test_dmnprocessor_span():
    """Test DMN processor creates expected spans."""
    tmpfile = tempfile.NamedTemporaryFile(mode="w+", delete=False)
    try:
        # Create tracer provider for this test
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        test_telemetry = create_telemetry_manager(
            service_name="autotel-dmn-processor",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        # Patch the telemetry manager creation at the module level
        import autotel.factory.processors.dmn_processor
        original_create = autotel.factory.processors.dmn_processor.create_telemetry_manager
        autotel.factory.processors.dmn_processor.create_telemetry_manager = lambda *args, **kwargs: test_telemetry
        
        try:
            processor = DMNProcessor()
            dummy_xml = '<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/"></definitions>'
            try:
                processor.parse(dummy_xml)
            except Exception:
                pass
        finally:
            # Restore original function
            autotel.factory.processors.dmn_processor.create_telemetry_manager = original_create
        
        # Force flush and shutdown while file is still open
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        output = tmpfile.read()
        
        assert "dmn_parse" in output
    finally:
        tmpfile.close() 