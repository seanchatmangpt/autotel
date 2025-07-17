#!/usr/bin/env python3
"""
Test Generated Unified Processors - 80/20 Focus

Tests that processors crash properly with NotImplementedError when processing.
Uses factory_boy to avoid hardcoded values.
"""

import pytest
from factory import Factory, Faker
from pathlib import Path
from typing import Any

from autotel.processors import ProcessorConfig, ProcessorResult
from autotel.processors.unified import (
    BPMNProcessor, DMNProcessor, DSPyProcessor, SHACLProcessor,
    OWLProcessor, JinjaProcessor, OTELProcessor
)


class ProcessorConfigFactory(Factory):
    """Factory for creating ProcessorConfig instances."""
    class Meta:
        model = ProcessorConfig
    
    name = Faker('word')
    enabled = True
    timeout = Faker('random_int', min=10, max=60)
    settings = Faker('pydict', nb_elements=3)


class TestGeneratedProcessors:
    """Test the generated unified processors - 80/20 focus on crash behavior."""
    
    def test_processor_creation_and_metadata(self):
        """Test that all processors can be created and have basic metadata."""
        processors = [
            BPMNProcessor, DMNProcessor, DSPyProcessor, SHACLProcessor,
            OWLProcessor, JinjaProcessor, OTELProcessor
        ]
        
        for processor_class in processors:
            config = ProcessorConfigFactory()
            processor = processor_class(config)
            
            # Test basic metadata exists
            metadata = processor.get_metadata()
            assert metadata is not None
            assert 'name' in metadata
            assert 'capabilities' in metadata
            assert 'supported_formats' in metadata
            
            # Test capabilities exist
            capabilities = processor.get_capabilities()
            assert isinstance(capabilities, list)
            assert len(capabilities) > 0
            
            # Test supported formats exist
            formats = processor.get_supported_formats()
            assert isinstance(formats, list)
            assert len(formats) > 0
    
    def test_processor_processing_crashes_with_notimplemented(self):
        """Test that processors crash with NotImplementedError when _process_impl is called."""
        processors = [
            (BPMNProcessor, "<xml>test</xml>"),
            (DMNProcessor, "<xml>test</xml>"),
            (DSPyProcessor, "<xml>test</xml>"),
            (SHACLProcessor, "<xml>test</xml>"),
            (OWLProcessor, "<xml>test</xml>"),
            (JinjaProcessor, "<xml>test</xml>"),
            (OTELProcessor, {"spans": []})
        ]
        
        for processor_class, test_data in processors:
            config = ProcessorConfigFactory()
            processor = processor_class(config)
            
            # Test that _process_impl crashes with NotImplementedError
            with pytest.raises(NotImplementedError):
                processor._process_impl(test_data)

    def test_processor_initialization_crashes(self):
        """Test that processor _initialize_processor does NOT crash (should be a no-op)."""
        processors = [
            BPMNProcessor, DMNProcessor, DSPyProcessor, SHACLProcessor,
            OWLProcessor, JinjaProcessor, OTELProcessor
        ]
        
        for processor_class in processors:
            config = ProcessorConfigFactory()
            processor = processor_class(config)
            # Should not raise
            processor._initialize_processor()
    
    def test_processor_config_creation(self):
        """Test that processors can create default configurations."""
        processors = [
            BPMNProcessor, DMNProcessor, DSPyProcessor, SHACLProcessor,
            OWLProcessor, JinjaProcessor, OTELProcessor
        ]
        
        for processor_class in processors:
            config = processor_class.create_default_config()
            assert isinstance(config, ProcessorConfig)
            assert config.name is not None
            assert config.enabled is True


if __name__ == "__main__":
    # Run tests
    pytest.main([__file__, "-v"]) 