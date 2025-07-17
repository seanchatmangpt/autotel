"""
AutoTel AnyStore

Universal store that delegates to specific format-based stores based on file extension.
Provides a unified interface for loading/saving any supported file format.
"""

import os
from typing import Any, Dict, Type

from autotel.stores.base import BaseStore, StoreResult
from autotel.stores.meta import store_metadata


# Extension to store class mapping
EXTENSION_TO_STORE: Dict[str, Type[BaseStore]] = {}


def register_store_extension(extension: str, store_class: Type[BaseStore]) -> None:
    """Register a store class for a specific file extension."""
    EXTENSION_TO_STORE[extension.lower()] = store_class


@store_metadata(
    name="any_store",
    version="1.0.0",
    capabilities=["universal_loading", "format_detection", "delegation"],
    supported_formats=["json", "yaml", "yml", "pickle", "pkl", "xml", "parquet", "db", "sqlite"],
    author="AutoTel",
    enterprise_ready=True,
    cloud_supported=True,
    performance_characteristics={
        "load_time": "O(1) - constant time delegation",
        "memory_usage": "Minimal - no data caching",
        "format_detection": "O(1) - extension-based"
    }
)
class AnyStore(BaseStore):
    """
    Universal store that delegates to specific format-based stores based on file extension.
    
    Automatically detects the appropriate store implementation based on file extension
    and delegates all operations to the concrete store. Supports all registered formats
    through a unified interface.
    
    Example:
        config = StoreConfig(name="data", path="data/sample.json")
        store = AnyStore(config)
        result = store.load()  # Delegates to JSONStore
    """

    def _resolve_store(self) -> BaseStore:
        """
        Resolve the appropriate store implementation based on file extension.
        
        Returns:
            Concrete store instance for the detected format
            
        Raises:
            ValueError: If file extension is not supported
        """
        _, ext = os.path.splitext(self.config.path.lower())
        
        if not ext:
            raise ValueError(f"No file extension found in path: {self.config.path}")
        
        store_class = EXTENSION_TO_STORE.get(ext)
        if not store_class:
            supported = ", ".join(sorted(EXTENSION_TO_STORE.keys()))
            raise ValueError(
                f"Unsupported file extension '{ext}' for path {self.config.path}. "
                f"Supported extensions: {supported}"
            )
        
        return store_class(self.config)

    def _load_impl(self) -> StoreResult:
        """
        Load data by delegating to the appropriate format-specific store.
        
        Returns:
            StoreResult with loaded data or error information
        """
        try:
            concrete_store = self._resolve_store()
            result = concrete_store.load()
            
            # Add metadata about the delegated store
            if result.success:
                result.metadata.update({
                    "delegated_store": concrete_store.__class__.__name__,
                    "file_extension": os.path.splitext(self.config.path.lower())[1]
                })
            
            return result
            
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to load data: {str(e)}",
                {"error_type": type(e).__name__, "delegation_failed": True}
            )

    def _save_impl(self, data: Any) -> StoreResult:
        """
        Save data by delegating to the appropriate format-specific store.
        
        Args:
            data: Data to save
            
        Returns:
            StoreResult with save operation result
        """
        try:
            concrete_store = self._resolve_store()
            result = concrete_store.save(data)
            
            # Add metadata about the delegated store
            if result.success:
                result.metadata.update({
                    "delegated_store": concrete_store.__class__.__name__,
                    "file_extension": os.path.splitext(self.config.path.lower())[1]
                })
            
            return result
            
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to save data: {str(e)}",
                {"error_type": type(e).__name__, "delegation_failed": True}
            )

    def get_supported_extensions(self) -> list[str]:
        """Get list of supported file extensions."""
        return list(EXTENSION_TO_STORE.keys())

    def get_delegated_store_info(self) -> Dict[str, str]:
        """Get information about which store would be delegated to for this path."""
        try:
            _, ext = os.path.splitext(self.config.path.lower())
            store_class = EXTENSION_TO_STORE.get(ext)
            if store_class:
                return {
                    "extension": ext,
                    "store_class": store_class.__name__,
                    "store_module": store_class.__module__
                }
            else:
                return {
                    "extension": ext,
                    "store_class": "None",
                    "store_module": "None"
                }
        except Exception:
            return {"error": "Could not determine store info"}


# Auto-register existing stores when this module is imported
def _register_existing_stores():
    """Register existing store implementations with AnyStore."""
    try:
        from autotel.stores.json_store import JSONStore
        register_store_extension(".json", JSONStore)
    except ImportError:
        pass
    
    try:
        from autotel.stores.yaml_store import YAMLStore
        register_store_extension(".yaml", YAMLStore)
        register_store_extension(".yml", YAMLStore)
    except ImportError:
        pass
    
    try:
        from autotel.stores.pickle_store import PickleStore
        register_store_extension(".pickle", PickleStore)
        register_store_extension(".pkl", PickleStore)
    except ImportError:
        pass
    
    try:
        from autotel.stores.xml_store import XMLStore
        register_store_extension(".xml", XMLStore)
    except ImportError:
        pass
    
    try:
        from autotel.stores.parquet_store import ParquetStore
        register_store_extension(".parquet", ParquetStore)
    except ImportError:
        pass
    
    try:
        from autotel.stores.sqlite_store import SQLiteStore
        register_store_extension(".db", SQLiteStore)
        register_store_extension(".sqlite", SQLiteStore)
    except ImportError:
        pass


# Register stores when module is imported
_register_existing_stores() 