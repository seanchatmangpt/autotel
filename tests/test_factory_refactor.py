"""
Test factory refactor to use unified processors.

This test verifies that the factory module correctly uses the unified processors
from the main processors directory instead of the non-existent .processors subdirectory.
"""

import pytest
from autotel.factory import PipelineOrchestrator
from autotel.processors.base import ProcessorConfig
from autotel.processors.owl_processor import OWLProcessor
from autotel.processors.shacl_processor import SHACLProcessor
from autotel.processors.dspy_processor import DSPyProcessor
from autotel.processors.otel_processor import OTELProcessor


class TestFactoryRefactor:
    """Test that factory correctly uses unified processors."""
    
    def test_factory_imports(self):
        """Test that factory can import all processors."""
        # This should not raise any import errors
        from autotel.factory import (
            PipelineOrchestrator,
            BPMNProcessor,
            DMNProcessor,
            DSPyProcessor,
            SHACLProcessor,
            OWLProcessor,
            OTELProcessor
        )
        
        assert PipelineOrchestrator is not None
        assert BPMNProcessor is not None
        assert DMNProcessor is not None
        assert DSPyProcessor is not None
        assert SHACLProcessor is not None
        assert OWLProcessor is not None
        assert OTELProcessor is not None
    
    def test_pipeline_orchestrator_initialization(self):
        """Test that PipelineOrchestrator can be initialized with unified processors."""
        orchestrator = PipelineOrchestrator()
        
        # Verify processors are initialized
        assert orchestrator.owl_processor is not None
        assert orchestrator.shacl_processor is not None
        assert orchestrator.dspy_processor is not None
        assert orchestrator.otel_processor is not None
        
        # Verify they are the correct types
        assert isinstance(orchestrator.owl_processor, OWLProcessor)
        assert isinstance(orchestrator.shacl_processor, SHACLProcessor)
        assert isinstance(orchestrator.dspy_processor, DSPyProcessor)
        assert isinstance(orchestrator.otel_processor, OTELProcessor)
    
    def test_processor_configuration(self):
        """Test that processors are configured correctly."""
        orchestrator = PipelineOrchestrator()
        
        # Verify processor configurations
        assert orchestrator.owl_processor.get_name() == "owl_processor"
        assert orchestrator.shacl_processor.get_name() == "shacl_processor"
        assert orchestrator.dspy_processor.get_name() == "dspy_processor"
        assert orchestrator.otel_processor.get_name() == "otel_processor"
        
        # Verify processors are enabled
        assert orchestrator.owl_processor.is_enabled()
        assert orchestrator.shacl_processor.is_enabled()
        assert orchestrator.dspy_processor.is_enabled()
        assert orchestrator.otel_processor.is_enabled()
    
    def test_processor_capabilities(self):
        """Test that processors have expected capabilities."""
        orchestrator = PipelineOrchestrator()
        
        # Verify OWL processor capabilities
        owl_capabilities = orchestrator.owl_processor.get_capabilities()
        assert "owl_parse" in owl_capabilities
        assert "ontology_extraction" in owl_capabilities
        
        # Verify SHACL processor capabilities
        shacl_capabilities = orchestrator.shacl_processor.get_capabilities()
        assert "shacl_parse" in shacl_capabilities
        assert "constraint_extraction" in shacl_capabilities
        
        # Verify DSPy processor capabilities
        dspy_capabilities = orchestrator.dspy_processor.get_capabilities()
        assert "dspy_parse" in dspy_capabilities
        assert "signature_extraction" in dspy_capabilities
        
        # Verify OTEL processor capabilities
        otel_capabilities = orchestrator.otel_processor.get_capabilities()
        assert "otel_parse" in otel_capabilities
        assert "telemetry_extraction" in otel_capabilities
    
    def test_processor_supported_formats(self):
        """Test that processors support expected formats."""
        orchestrator = PipelineOrchestrator()
        
        # Verify OWL processor formats
        owl_formats = orchestrator.owl_processor.get_supported_formats()
        assert "xml" in owl_formats
        assert "owl" in owl_formats
        assert "rdf" in owl_formats
        
        # Verify SHACL processor formats
        shacl_formats = orchestrator.shacl_processor.get_supported_formats()
        assert "xml" in shacl_formats
        assert "shacl" in shacl_formats
        assert "rdf" in shacl_formats
        
        # Verify DSPy processor formats
        dspy_formats = orchestrator.dspy_processor.get_supported_formats()
        assert "xml" in dspy_formats
        assert "dspy" in dspy_formats
        
        # Verify OTEL processor formats
        otel_formats = orchestrator.otel_processor.get_supported_formats()
        assert "json" in otel_formats
        assert "otel" in otel_formats
    
    def test_processor_metadata(self):
        """Test that processors have metadata."""
        orchestrator = PipelineOrchestrator()
        
        # Verify all processors have metadata
        assert orchestrator.owl_processor.get_metadata() is not None
        assert orchestrator.shacl_processor.get_metadata() is not None
        assert orchestrator.dspy_processor.get_metadata() is not None
        assert orchestrator.otel_processor.get_metadata() is not None
        
        # Verify metadata contains expected fields
        owl_metadata = orchestrator.owl_processor.get_metadata()
        assert "name" in owl_metadata
        assert "version" in owl_metadata
        assert "capabilities" in owl_metadata
        assert "supported_formats" in owl_metadata
        assert "author" in owl_metadata


if __name__ == "__main__":
    pytest.main([__file__]) 