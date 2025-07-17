"""
Tests for AutoTel AnyStore

Validates universal store delegation to format-specific stores.
"""

import json
import tempfile
import os
from pathlib import Path
from typing import Dict, Any

import pytest
import yaml

from autotel.stores.any_store import AnyStore, register_store_extension
from autotel.stores.base import StoreConfig, StoreResult
from autotel.stores.json_store import JSONStore
from autotel.stores.yaml_store import YAMLStore
from autotel.stores.pickle_store import PickleStore
from autotel.stores.xml_store import XMLStore


class TestAnyStore:
    """Test AnyStore delegation and format detection."""

    def test_any_store_metadata(self):
        """Test AnyStore has correct metadata."""
        config = StoreConfig(name="test", path="test.json")
        store = AnyStore(config)
        
        metadata = store.get_metadata()
        assert metadata["name"] == "any_store"
        assert "universal_loading" in metadata["capabilities"]
        assert "format_detection" in metadata["capabilities"]

    def test_supported_extensions(self):
        """Test AnyStore reports supported extensions."""
        config = StoreConfig(name="test", path="test.json")
        store = AnyStore(config)
        
        extensions = store.get_supported_extensions()
        assert ".json" in extensions
        assert ".yaml" in extensions
        assert ".yml" in extensions
        assert ".pickle" in extensions
        assert ".pkl" in extensions
        assert ".xml" in extensions

    def test_delegated_store_info(self):
        """Test AnyStore can report which store would be delegated to."""
        config = StoreConfig(name="test", path="test.json")
        store = AnyStore(config)
        
        info = store.get_delegated_store_info()
        assert info["extension"] == ".json"
        assert info["store_class"] == "JSONStore"

    def test_json_delegation(self):
        """Test AnyStore delegates to JSONStore for .json files."""
        test_data = {"key": "value", "number": 42, "list": [1, 2, 3]}
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(test_data, f)
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.load()
            assert result.success
            assert result.data == test_data
            assert result.metadata["delegated_store"] == "JSONStore"
            assert result.metadata["file_extension"] == ".json"
            
        finally:
            os.unlink(temp_path)

    def test_yaml_delegation(self):
        """Test AnyStore delegates to YAMLStore for .yaml files."""
        test_data = {"key": "value", "number": 42, "list": [1, 2, 3]}
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            yaml.dump(test_data, f)
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.load()
            assert result.success
            assert result.data == test_data
            assert result.metadata["delegated_store"] == "YAMLStore"
            assert result.metadata["file_extension"] == ".yaml"
            
        finally:
            os.unlink(temp_path)

    def test_yml_delegation(self):
        """Test AnyStore delegates to YAMLStore for .yml files."""
        test_data = {"key": "value", "number": 42}
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yml', delete=False) as f:
            yaml.dump(test_data, f)
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.load()
            assert result.success
            assert result.data == test_data
            assert result.metadata["delegated_store"] == "YAMLStore"
            assert result.metadata["file_extension"] == ".yml"
            
        finally:
            os.unlink(temp_path)

    def test_xml_delegation(self):
        """Test AnyStore delegates to XMLStore for .xml files."""
        test_data = {"root": {"item": "value", "number": 42}}
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write('<?xml version="1.0" encoding="utf-8"?>\n')
            f.write('<root><item>value</item><number>42</number></root>')
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.load()
            assert result.success
            assert result.data.tag == "root"
            assert result.metadata["delegated_store"] == "XMLStore"
            assert result.metadata["file_extension"] == ".xml"
            
        finally:
            os.unlink(temp_path)

    def test_json_save_delegation(self):
        """Test AnyStore delegates save operations to JSONStore."""
        test_data = {"key": "value", "number": 42}
        
        with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.save(test_data)
            assert result.success
            assert result.metadata["delegated_store"] == "JSONStore"
            
            # Verify data was actually saved
            with open(temp_path, 'r') as f:
                saved_data = json.load(f)
            assert saved_data == test_data
            
        finally:
            os.unlink(temp_path)

    def test_yaml_save_delegation(self):
        """Test AnyStore delegates save operations to YAMLStore."""
        test_data = {"key": "value", "number": 42}
        
        with tempfile.NamedTemporaryFile(suffix='.yaml', delete=False) as f:
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.save(test_data)
            assert result.success
            assert result.metadata["delegated_store"] == "YAMLStore"
            
            # Verify data was actually saved
            with open(temp_path, 'r') as f:
                saved_data = yaml.safe_load(f)
            assert saved_data == test_data
            
        finally:
            os.unlink(temp_path)

    def test_unsupported_extension(self):
        """Test AnyStore handles unsupported extensions gracefully."""
        config = StoreConfig(name="test", path="test.unsupported")
        store = AnyStore(config)
        
        result = store.load()
        assert not result.success
        assert "Unsupported file extension" in result.error
        assert ".unsupported" in result.error

    def test_no_extension(self):
        """Test AnyStore handles files without extensions."""
        config = StoreConfig(name="test", path="testfile")
        store = AnyStore(config)
        
        result = store.load()
        assert not result.success
        assert "No file extension found" in result.error

    def test_extension_registration(self):
        """Test custom extension registration."""
        # Create a mock store class
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

    def test_case_insensitive_extension(self):
        """Test AnyStore handles case-insensitive extensions."""
        test_data = {"key": "value"}
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.JSON', delete=False) as f:
            json.dump(test_data, f)
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            result = store.load()
            assert result.success
            assert result.data == test_data
            assert result.metadata["delegated_store"] == "JSONStore"
            
        finally:
            os.unlink(temp_path)

    def test_settings_passthrough(self):
        """Test AnyStore passes settings to delegated stores."""
        test_data = {"key": "value"}
        
        with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            config = StoreConfig(
                name="test", 
                path=temp_path,
                settings={"indent": 4, "encoding": "utf-8"}
            )
            store = AnyStore(config)
            
            result = store.save(test_data)
            assert result.success
            
            # Verify settings were applied (indent=4)
            with open(temp_path, 'r') as f:
                content = f.read()
                # Should have 4-space indentation
                assert '    "key"' in content
            
        finally:
            os.unlink(temp_path)

    def test_error_delegation(self):
        """Test AnyStore properly delegates and handles errors."""
        config = StoreConfig(name="test", path="nonexistent.json")
        store = AnyStore(config)
        
        result = store.load()
        assert not result.success
        assert "FileNotFoundError" in result.metadata["error_type"]
        assert result.metadata["delegation_failed"] is True

    def test_telemetry_span_validation(self):
        """Test that AnyStore operations trigger telemetry spans."""
        from autotel.helpers.telemetry.span import get_current_span
        
        test_data = {"key": "value"}
        
        with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            config = StoreConfig(name="test", path=temp_path)
            store = AnyStore(config)
            
            # Test save operation
            result = store.save(test_data)
            assert result.success
            
            # Verify telemetry span was created
            span = get_current_span()
            assert span is not None
            assert "AnyStore.save" in span.name or "store.save" in span.name
            
        finally:
            os.unlink(temp_path) 