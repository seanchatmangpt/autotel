"""
Simple tests for AutoTel AnyStore

Basic validation without complex dependencies.
"""

import json
import tempfile
import os
import pytest

from autotel.stores.any_store import AnyStore, register_store_extension
from autotel.stores.base import StoreConfig, StoreResult


def test_any_store_basic_functionality():
    """Test basic AnyStore functionality."""
    test_data = {"key": "value", "number": 42}
    
    with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as f:
        temp_path = f.name
    
    try:
        config = StoreConfig(name="test", path=temp_path)
        store = AnyStore(config)
        
        # Test save
        save_result = store.save(test_data)
        assert save_result.success
        assert save_result.metadata["delegated_store"] == "JSONStore"
        
        # Test load
        load_result = store.load()
        assert load_result.success
        assert load_result.data == test_data
        assert load_result.metadata["delegated_store"] == "JSONStore"
        
    finally:
        if os.path.exists(temp_path):
            os.unlink(temp_path)


def test_any_store_supported_extensions():
    """Test AnyStore reports supported extensions."""
    config = StoreConfig(name="test", path="test.json")
    store = AnyStore(config)
    
    extensions = store.get_supported_extensions()
    assert ".json" in extensions
    assert ".yaml" in extensions
    assert ".yml" in extensions
    assert ".xml" in extensions


def test_any_store_delegation_info():
    """Test AnyStore can report delegation info."""
    config = StoreConfig(name="test", path="test.json")
    store = AnyStore(config)
    
    info = store.get_delegated_store_info()
    assert info["extension"] == ".json"
    assert info["store_class"] == "JSONStore"


def test_any_store_unsupported_extension():
    """Test AnyStore handles unsupported extensions."""
    config = StoreConfig(name="test", path="test.unsupported")
    store = AnyStore(config)
    
    result = store.load()
    assert not result.success
    assert "Unsupported file extension" in result.error


def test_any_store_custom_registration():
    """Test custom store registration."""
    class MockStore:
        def __init__(self, config):
            self.config = config
        
        def load(self):
            return StoreResult.success_result({"mock": "data"})
        
        def save(self, data):
            return StoreResult.success_result(data)
    
    # Register custom extension
    register_store_extension(".mock", MockStore)
    
    config = StoreConfig(name="test", path="test.mock")
    store = AnyStore(config)
    
    result = store.load()
    assert result.success
    assert result.data == {"mock": "data"}


if __name__ == "__main__":
    # Run tests directly
    test_any_store_basic_functionality()
    test_any_store_supported_extensions()
    test_any_store_delegation_info()
    test_any_store_unsupported_extension()
    test_any_store_custom_registration()
    print("✅ All AnyStore tests passed!") 