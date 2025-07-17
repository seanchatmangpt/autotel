"""
Tests for AutoTel Stores

Tests the store implementations using the BaseStore architecture.
"""

import pytest
import json
import tempfile
from pathlib import Path
from autotel.stores.base import BaseStore, StoreConfig, StoreResult
from autotel.stores.json_store import JSONStore
from autotel.stores.registry import registry
from tests.factories import StoreConfigFactory


class TestStoreConfig:
    """Test suite for StoreConfig functionality."""
    
    def test_store_config_creation(self):
        """Test creating a store config."""
        config = StoreConfig(
            name="test_store",
            path="/path/to/data.json"
        )
        
        assert config.name == "test_store"
        assert config.path == "/path/to/data.json"
        assert config.enabled is True
        assert config.settings == {}
    
    def test_store_config_with_settings(self):
        """Test creating a store config with settings."""
        config = StoreConfig(
            name="test_store",
            path="/path/to/data.json",
            settings={"indent": 4, "encoding": "utf-8"}
        )
        
        assert config.settings["indent"] == 4
        assert config.settings["encoding"] == "utf-8"
    
    def test_store_config_validation(self):
        """Test store config validation."""
        with pytest.raises(ValueError, match="Store name cannot be empty"):
            StoreConfig(name="", path="/path/to/data.json")
        
        with pytest.raises(ValueError, match="Store path cannot be empty"):
            StoreConfig(name="test_store", path="")


class TestStoreResult:
    """Test suite for StoreResult functionality."""
    
    def test_success_result(self):
        """Test creating a successful result."""
        data = {"key": "value"}
        result = StoreResult.success_result(data, {"duration_ms": 45.2})
        
        assert result.success is True
        assert result.data == data
        assert result.error is None
        assert result.metadata["duration_ms"] == 45.2
    
    def test_error_result(self):
        """Test creating an error result."""
        error = "File not found"
        result = StoreResult.error_result(error, {"error_type": "FileNotFoundError"})
        
        assert result.success is False
        assert result.data is None
        assert result.error == error
        assert result.metadata["error_type"] == "FileNotFoundError"
    
    def test_error_validation(self):
        """Test error validation."""
        with pytest.raises(ValueError, match="Error message required when success is False"):
            StoreResult(success=False, error=None)


class TestBaseStore:
    """Test suite for BaseStore functionality."""
    
    @pytest.fixture
    def store_config(self):
        """Create a store config for testing."""
        return StoreConfigFactory()
    
    def test_base_store_creation(self, store_config):
        """Test creating a base store."""
        # This should raise TypeError since BaseStore is abstract
        with pytest.raises(TypeError):
            BaseStore(store_config)
    
    def test_base_store_interface(self):
        """Test that BaseStore requires abstract methods."""
        # Create a concrete subclass without implementing abstract methods
        class InvalidStore(BaseStore):
            pass
        
        with pytest.raises(TypeError):
            InvalidStore(StoreConfig(name="test", path="/test"))


class TestJSONStore:
    """Test suite for JSON store functionality."""
    
    @pytest.fixture
    def json_store(self):
        """Create a JSON store for testing."""
        import tempfile
        temp_file = tempfile.NamedTemporaryFile(suffix='.json', delete=False)
        temp_file.close()
        config = StoreConfig(
            name="test_json_store",
            path=temp_file.name
        )
        store = JSONStore(config)
        yield store
        # Cleanup
        Path(temp_file.name).unlink(missing_ok=True)
    
    @pytest.fixture
    def temp_json_file(self):
        """Create a temporary JSON file for testing."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump({"test": "data", "number": 42}, f)
            temp_path = f.name
        
        yield temp_path
        
        # Cleanup
        Path(temp_path).unlink(missing_ok=True)
    
    def test_json_store_metadata(self, json_store):
        """Test JSON store metadata."""
        metadata = json_store.get_metadata()
        
        assert metadata["name"] == "json_store"
        assert metadata["version"] == "2.1.0"
        assert "serialization" in metadata["capabilities"]
        assert "json" in metadata["supported_formats"]
        assert metadata["enterprise_ready"] is True
        assert metadata["cloud_supported"] is True
    
    def test_json_store_load_success(self, temp_json_file):
        """Test successful JSON loading."""
        config = StoreConfig(name="test", path=temp_json_file)
        store = JSONStore(config)
        
        result = store.load()
        
        assert result.success is True
        assert result.data == {"test": "data", "number": 42}
        assert "size_bytes" in result.metadata
        assert "duration_ms" in result.metadata
    
    def test_json_store_load_file_not_found(self):
        """Test JSON loading with non-existent file."""
        import tempfile
        # Create a config with a path that definitely doesn't exist
        config = StoreConfig(
            name="test_json_store",
            path="/nonexistent/path/test_data.json"
        )
        store = JSONStore(config)
        
        result = store.load()
        
        assert result.success is False
        assert "File not found" in result.error
        assert result.metadata["error_type"] == "FileNotFoundError"
    
    def test_json_store_save_success(self, json_store):
        """Test successful JSON saving."""
        data = {"new": "data", "list": [1, 2, 3]}
        
        result = json_store.save(data)
        
        assert result.success is True
        assert result.data is None
        assert "size_bytes" in result.metadata
        assert "duration_ms" in result.metadata
        
        # Verify file was created and contains correct data
        assert Path(json_store.config.path).exists()
        with open(json_store.config.path, 'r') as f:
            saved_data = json.load(f)
        assert saved_data == data
    
    def test_json_store_save_with_settings(self):
        """Test JSON saving with custom settings."""
        import tempfile
        temp_file = tempfile.NamedTemporaryFile(suffix='.json', delete=False)
        temp_file.close()
        
        config = StoreConfig(
            name="test",
            path=temp_file.name,
            settings={"indent": 4, "sort_keys": True}
        )
        store = JSONStore(config)
        
        data = {"c": 3, "a": 1, "b": 2}
        result = store.save(data)
        
        assert result.success is True
        assert result.metadata["indent"] == 4
        assert result.metadata["sort_keys"] is True
        
        # Verify file was created with correct formatting
        assert Path(store.config.path).exists()
        with open(store.config.path, 'r') as f:
            content = f.read()
            # Should be sorted and indented
            assert '"a"' in content
            assert '"b"' in content
            assert '"c"' in content
        
        # Cleanup
        Path(temp_file.name).unlink(missing_ok=True)
    
    def test_json_store_exists(self, temp_json_file):
        """Test store exists method."""
        config = StoreConfig(name="test", path=temp_json_file)
        store = JSONStore(config)
        
        assert store.exists() is True
        
        config = StoreConfig(name="test", path="/nonexistent/file.json")
        store = JSONStore(config)
        
        assert store.exists() is False
    
    def test_json_store_get_setting(self, json_store):
        """Test getting store settings."""
        # Test default value
        encoding = json_store.get_setting("encoding", "utf-8")
        assert encoding == "utf-8"
        
        # Test custom setting
        json_store.set_setting("custom_setting", "custom_value")
        value = json_store.get_setting("custom_setting")
        assert value == "custom_value"


class TestStoreRegistry:
    """Test suite for store registry functionality."""
    
    @pytest.fixture(autouse=True)
    def setup_registry(self):
        """Ensure registry is populated for all tests."""
        # Clear and repopulate registry
        registry._stores.clear()
        registry._metadata_cache.clear()
        registry._capability_index.clear()
        registry._format_index.clear()
        registry._enterprise_stores.clear()
        registry._cloud_stores.clear()
        
        # Force re-registration by creating instances
        from autotel.stores.json_store import JSONStore
        from autotel.stores.yaml_store import YAMLStore
        from autotel.stores.pickle_store import PickleStore
        from autotel.stores.rdf_graph import RDFGraphStore
        from autotel.stores.memory_cache import MemoryCacheStore
        config = StoreConfig(name="test", path="/tmp/test.json")
        JSONStore(config)
        YAMLStore(config)
        PickleStore(config)
        RDFGraphStore(config)
        MemoryCacheStore(config)
    
    def test_registry_registration(self):
        """Test store registration."""
        # Check if registered (fixture handles setup)
        assert "json_store" in registry.list_stores()
        assert registry.get("json_store") == JSONStore
    
    def test_registry_capability_filtering(self):
        """Test capability-based filtering."""
        stores = registry.list_by_capability("serialization")
        assert "json_store" in stores
    
    def test_registry_format_filtering(self):
        """Test format-based filtering."""
        stores = registry.list_by_format("json")
        assert "json_store" in stores
    
    def test_registry_enterprise_stores(self):
        """Test enterprise store filtering."""
        stores = registry.list_enterprise_stores()
        assert "json_store" in stores
    
    def test_registry_metadata(self):
        """Test metadata retrieval."""
        metadata = registry.get_metadata("json_store")
        assert metadata is not None
        assert metadata["name"] == "json_store"
        assert metadata["enterprise_ready"] is True
    
    def test_registry_performance_analysis(self):
        """Test performance analysis."""
        analysis = registry.analyze_performance()
        assert "json_store" in analysis
        assert analysis["json_store"]["enterprise_ready"] is True
        assert "performance" in analysis["json_store"]


class TestStoreCLI:
    """Test suite for store CLI functionality."""
    
    @pytest.fixture(autouse=True)
    def setup_registry(self):
        """Ensure registry is populated for all tests."""
        # Clear and repopulate registry
        registry._stores.clear()
        registry._metadata_cache.clear()
        registry._capability_index.clear()
        registry._format_index.clear()
        registry._enterprise_stores.clear()
        registry._cloud_stores.clear()
        
        # Force re-registration by creating instances
        from autotel.stores.json_store import JSONStore
        from autotel.stores.yaml_store import YAMLStore
        from autotel.stores.pickle_store import PickleStore
        from autotel.stores.rdf_graph import RDFGraphStore
        from autotel.stores.memory_cache import MemoryCacheStore
        config = StoreConfig(name="test", path="/tmp/test.json")
        JSONStore(config)
        YAMLStore(config)
        PickleStore(config)
        RDFGraphStore(config)
        MemoryCacheStore(config)
    
    def test_cli_commands_generation(self):
        """Test CLI command generation."""
        commands = registry.get_cli_commands()
        assert "store_json_store" in commands
    
    def test_api_endpoints_generation(self):
        """Test API endpoint generation."""
        endpoints = registry.get_api_endpoints()
        assert "/api/stores/json_store" in endpoints
        assert "GET" in endpoints["/api/stores/json_store"]["methods"]
        assert "POST" in endpoints["/api/stores/json_store"]["methods"] 