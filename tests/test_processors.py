"""Tests for AutoTel Processors Module."""

import pytest
from pydantic import ValidationError
from autotel.processors import (
    BaseProcessor, ProcessorConfig, ProcessorResult, ProcessorRegistry,
    processor_metadata
)
from typing import Any


class TestProcessor(BaseProcessor):
    """Concrete test processor implementation."""
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """Simple test implementation that echoes input."""
        return ProcessorResult.success_result(
            data=f"processed: {data}",
            metadata={"processor": "test"}
        )


@processor_metadata(
    name="advanced_test_processor",
    version="2.0.0",
    capabilities=["parsing", "validation", "transformation"],
    supported_formats=["xml", "json", "yaml"],
    author="Test Author"
)
class AdvancedTestProcessor(BaseProcessor):
    """Advanced test processor with metadata."""
    
    CAPABILITIES = ["advanced_parsing", "schema_validation"]
    SUPPORTED_FORMATS = ["bpmn", "dmn", "owl"]
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """Advanced test implementation."""
        return ProcessorResult.success_result(
            data=f"advanced_processed: {data}",
            metadata={"processor": "advanced_test", "version": "2.0.0"}
        )


class TestProcessors:
    """Test processor base classes and registry."""
    
    def test_processor_config_creation(self):
        """Test ProcessorConfig creation and validation."""
        config = ProcessorConfig(
            name="test_processor",
            enabled=True,
            timeout=60,
            settings={"key": "value"}
        )
        assert config.name == "test_processor"
        assert config.enabled is True
        assert config.timeout == 60
        assert config.settings["key"] == "value"
    
    def test_processor_result_creation(self):
        """Test ProcessorResult creation."""
        success_result = ProcessorResult.success_result(
            data="test_data",
            metadata={"key": "value"}
        )
        assert success_result.success is True
        assert success_result.data == "test_data"
        assert success_result.metadata["key"] == "value"
        assert success_result.error is None
        
        error_result = ProcessorResult.error_result(
            error="test_error",
            metadata={"key": "value"}
        )
        assert error_result.success is False
        assert error_result.error == "test_error"
        assert error_result.metadata["key"] == "value"
        assert error_result.data is None
    
    def test_base_processor_creation(self):
        """Test BaseProcessor instantiation and basic functionality."""
        config = ProcessorConfig(name="test_processor")
        processor = TestProcessor(config)
        
        assert processor.get_name() == "test_processor"
        assert processor.is_enabled() is True
        assert processor.get_config() == config
    
    def test_base_processor_processing(self):
        """Test BaseProcessor processing with telemetry integration."""
        config = ProcessorConfig(name="test_processor")
        processor = TestProcessor(config)
        
        result = processor.process("test_input")
        
        assert isinstance(result, ProcessorResult)
        assert result.success is True
        assert result.data == "processed: test_input"
        assert result.metadata["processor"] == "test"
    
    def test_processor_registry_registration(self):
        """Test processor registration and discovery."""
        registry = ProcessorRegistry()
        
        # Register processor
        registry.register("test", TestProcessor)
        
        # Check registration
        assert "test" in registry.list_registered()
        assert registry.get_class("test") == TestProcessor
    
    def test_processor_registry_creation(self):
        """Test processor instance creation through registry."""
        registry = ProcessorRegistry()
        registry.register("test", TestProcessor)
        
        config = ProcessorConfig(name="test_processor")
        processor = registry.create("test", config)
        
        assert isinstance(processor, TestProcessor)
        assert processor.get_name() == "test_processor"
        assert "test" in registry.list_instances()
    
    def test_processor_registry_get_instance(self):
        """Test getting processor instances from registry."""
        registry = ProcessorRegistry()
        registry.register("test", TestProcessor)
        
        config = ProcessorConfig(name="test_processor")
        created_processor = registry.create("test", config)
        retrieved_processor = registry.get_instance("test")
        
        assert retrieved_processor is created_processor
    
    def test_processor_config_validation(self):
        """Test processor configuration validation."""
        # Test invalid name
        with pytest.raises(ValidationError, match="Processor name cannot be empty"):
            ProcessorConfig(name="")
        
        # Test invalid timeout
        with pytest.raises(ValidationError, match="greater than 0"):
            ProcessorConfig(name="test", timeout=0)
    
    def test_processor_settings(self):
        """Test processor settings management."""
        config = ProcessorConfig(
            name="test_processor",
            settings={"setting1": "value1", "setting2": "value2"}
        )
        processor = TestProcessor(config)
        
        assert processor.get_setting("setting1") == "value1"
        assert processor.get_setting("setting2") == "value2"
        assert processor.get_setting("missing", "default") == "default"
        
        processor.set_setting("new_setting", "new_value")
        assert processor.get_setting("new_setting") == "new_value"
    
    def test_processor_registry_clear(self):
        """Test registry clearing functionality."""
        registry = ProcessorRegistry()
        registry.register("test", TestProcessor)
        
        assert len(registry.list_registered()) == 1
        
        registry.clear()
        assert len(registry.list_registered()) == 0
        assert len(registry.list_instances()) == 0
    
    def test_metaclass_automatic_registration(self):
        """Test that processors are automatically registered via metaclass."""
        # Clear the global registry
        from autotel.processors.registry import registry
        registry.clear()
        
        # Create a new processor class (should auto-register)
        class AutoRegisteredProcessor(BaseProcessor):
            def _process_impl(self, data: Any) -> ProcessorResult:
                return ProcessorResult.success_result(data)
        
        # Check if it was automatically registered
        assert "autoregisteredprocessor" in registry.list_registered()
        assert registry.get_class("autoregisteredprocessor") == AutoRegisteredProcessor
    
    def test_processor_metadata_collection(self):
        """Test metadata collection via metaclass."""
        config = ProcessorConfig(name="advanced_test_processor")
        processor = AdvancedTestProcessor(config)
        
        metadata = processor.get_metadata()
        assert metadata['name'] == "advanced_test_processor"
        assert metadata['version'] == "2.0.0"
        assert metadata['author'] == "Test Author"
        # Class attributes take precedence over decorator metadata
        assert "advanced_parsing" in metadata['capabilities']
        assert "schema_validation" in metadata['capabilities']
        assert "bpmn" in metadata['supported_formats']
        assert "dmn" in metadata['supported_formats']
        assert "owl" in metadata['supported_formats']
    
    def test_processor_capabilities(self):
        """Test processor capabilities access."""
        config = ProcessorConfig(name="advanced_test_processor")
        processor = AdvancedTestProcessor(config)
        
        capabilities = processor.get_capabilities()
        # Class attributes take precedence
        assert "advanced_parsing" in capabilities
        assert "schema_validation" in capabilities
    
    def test_processor_supported_formats(self):
        """Test processor supported formats access."""
        config = ProcessorConfig(name="advanced_test_processor")
        processor = AdvancedTestProcessor(config)
        
        formats = processor.get_supported_formats()
        # Class attributes take precedence
        assert "bpmn" in formats
        assert "dmn" in formats
        assert "owl" in formats
    
    def test_registry_metadata_support(self):
        """Test registry metadata functionality."""
        registry = ProcessorRegistry()
        registry.register("advanced", AdvancedTestProcessor)
        
        # Test metadata retrieval
        metadata = registry.get_metadata("advanced")
        assert metadata is not None
        assert metadata['name'] == "advanced_test_processor"
        assert metadata['version'] == "2.0.0"
        
        # Test capability search (class attributes take precedence)
        advanced_parsing_processors = registry.find_by_capability("advanced_parsing")
        assert "advanced" in advanced_parsing_processors
        
        # Test format search (class attributes take precedence)
        bpmn_processors = registry.find_by_format("bpmn")
        assert "advanced" in bpmn_processors
        
        # Test all metadata
        all_metadata = registry.get_all_metadata()
        assert "advanced" in all_metadata
        assert all_metadata["advanced"]["name"] == "advanced_test_processor"
    
    def test_processor_metadata_decorator(self):
        """Test the processor_metadata decorator."""
        @processor_metadata(
            name="decorated_processor",
            version="1.5.0",
            capabilities=["decorated_capability"],
            supported_formats=["decorated_format"],
            author="Decorated Author"
        )
        class DecoratedProcessor(BaseProcessor):
            def _process_impl(self, data: Any) -> ProcessorResult:
                return ProcessorResult.success_result(data)
        
        config = ProcessorConfig(name="decorated_processor")
        processor = DecoratedProcessor(config)
        
        metadata = processor.get_metadata()
        assert metadata['name'] == "decorated_processor"
        assert metadata['version'] == "1.5.0"
        assert metadata['author'] == "Decorated Author"
        assert "decorated_capability" in metadata['capabilities']
        assert "decorated_format" in metadata['supported_formats']
    
    def test_interface_validation(self):
        """Test that metaclass validates required interface."""
        # Clear registry to avoid conflicts
        from autotel.processors.registry import registry
        registry.clear()
        
        with pytest.raises(TypeError, match="must implement '_process_impl' method"):
            class InvalidProcessor(BaseProcessor):
                # Missing _process_impl method - should raise TypeError
                pass 