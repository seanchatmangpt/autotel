import pytest
from factory import Factory, Faker
from typing import Any, Dict
from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import ProcessorMeta, processor_metadata

# Factory for ProcessorConfig
class ProcessorConfigFactory(Factory):
    class Meta:
        model = ProcessorConfig
    name = Faker('word')
    enabled = True
    timeout = Faker('random_int', min=10, max=60)
    settings = Faker('pydict', nb_elements=3)

# Minimal valid processor for testing
class DummyProcessor(BaseProcessor):
    def _process_impl(self, data: Any) -> ProcessorResult:
        return ProcessorResult.success_result(data={"echo": data})

# Invalid processor (missing _process_impl)
def make_invalid_processor():
    class InvalidProcessor(BaseProcessor):
        pass
    return InvalidProcessor

def test_baseprocessor_instantiation_and_process():
    config = ProcessorConfigFactory()
    proc = DummyProcessor(config)
    result = proc.process("foo")
    assert isinstance(result, ProcessorResult)
    assert result.success
    assert result.data == {"echo": "foo"}

def test_baseprocessor_contracts():
    config = ProcessorConfigFactory()
    proc = DummyProcessor(config)
    # Should raise ViolationError if data is None due to contract precondition
    with pytest.raises(Exception) as exc_info:  # icontract.ViolationError
        proc.process(None)
    assert "data is not None" in str(exc_info.value)

def test_baseprocessor_config_and_metadata():
    config = ProcessorConfigFactory()
    proc = DummyProcessor(config)
    assert proc.get_config() == config
    assert proc.is_enabled() is True
    assert isinstance(proc.get_metadata(), dict)
    assert isinstance(proc.get_capabilities(), list)
    assert isinstance(proc.get_supported_formats(), list)

def test_baseprocessor_abstract_enforcement():
    # Should raise TypeError if _process_impl is not implemented
    with pytest.raises(TypeError):
        make_invalid_processor()()

def test_processormeta_metadata_collection_and_decorator():
    @processor_metadata(
        name="custom_proc",
        version="9.9.9",
        capabilities=["foo", "bar"],
        supported_formats=["baz"],
        author="Test Author"
    )
    class MetaTestProcessor(BaseProcessor):
        def _process_impl(self, data: Any) -> ProcessorResult:
            return ProcessorResult.success_result(data=data)
    config = ProcessorConfigFactory()
    proc = MetaTestProcessor(config)
    meta = proc.get_metadata()
    assert meta["name"] == "custom_proc"
    assert meta["version"] == "9.9.9"
    assert "foo" in meta["capabilities"]
    assert "baz" in meta["supported_formats"]
    assert meta["author"] == "Test Author"

def test_processormeta_interface_validation():
    # Should raise TypeError if required method missing
    with pytest.raises(TypeError):
        class NoImpl(BaseProcessor):
            pass 
 