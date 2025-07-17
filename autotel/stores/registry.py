"""
AutoTel Store Registry

Provides automatic discovery, registration, and factory methods for all store implementations.
Supports enterprise features like capability filtering and performance analysis.
"""

from typing import Dict, Type, List, Optional, Any, Callable
from pathlib import Path
import importlib
import inspect
from collections import defaultdict

from .base import BaseStore, StoreConfig
from .meta import StoreMeta


class StoreRegistry:
    """
    Registry for all AutoTel store implementations.
    
    Features:
    - Automatic discovery and registration
    - Factory methods for store creation
    - Capability-based filtering
    - Performance analysis
    - Enterprise integration support
    """
    
    def __init__(self):
        self._stores: Dict[str, Type[BaseStore]] = {}
        self._metadata_cache: Dict[str, Dict[str, Any]] = {}
        self._capability_index: Dict[str, List[str]] = defaultdict(list)
        self._format_index: Dict[str, List[str]] = defaultdict(list)
        self._enterprise_stores: List[str] = []
        self._cloud_stores: List[str] = []
    
    def register(self, name: str, store_class: Type[BaseStore]) -> None:
        """
        Register a store class.
        
        Args:
            name: Store name
            store_class: Store class to register
        """
        if not issubclass(store_class, BaseStore):
            raise TypeError(f"Store class must inherit from BaseStore: {store_class}")
        
        self._stores[name] = store_class
        
        # Index by capabilities
        metadata = getattr(store_class, '_store_metadata', {})
        capabilities = metadata.get('capabilities', [])
        for capability in capabilities:
            self._capability_index[capability].append(name)
        
        # Index by formats
        formats = metadata.get('supported_formats', [])
        for format_name in formats:
            self._format_index[format_name].append(name)
        
        # Index enterprise features
        if metadata.get('enterprise_ready', False):
            self._enterprise_stores.append(name)
        
        if metadata.get('cloud_supported', False):
            self._cloud_stores.append(name)
        
        # Cache metadata
        self._metadata_cache[name] = metadata
    
    def unregister(self, name: str) -> None:
        """
        Unregister a store class.
        
        Args:
            name: Store name to unregister
        """
        if name in self._stores:
            store_class = self._stores[name]
            metadata = getattr(store_class, '_store_metadata', {})
            
            # Remove from capability index
            capabilities = metadata.get('capabilities', [])
            for capability in capabilities:
                if name in self._capability_index[capability]:
                    self._capability_index[capability].remove(name)
            
            # Remove from format index
            formats = metadata.get('supported_formats', [])
            for format_name in formats:
                if name in self._format_index[format_name]:
                    self._format_index[format_name].remove(name)
            
            # Remove from enterprise/cloud lists
            if name in self._enterprise_stores:
                self._enterprise_stores.remove(name)
            
            if name in self._cloud_stores:
                self._cloud_stores.remove(name)
            
            # Remove from main registry and cache
            del self._stores[name]
            if name in self._metadata_cache:
                del self._metadata_cache[name]
    
    def get(self, name: str) -> Optional[Type[BaseStore]]:
        """
        Get a store class by name.
        
        Args:
            name: Store name
            
        Returns:
            Store class or None if not found
        """
        return self._stores.get(name)
    
    def create(self, name: str, config: StoreConfig) -> Optional[BaseStore]:
        """
        Create a store instance.
        
        Args:
            name: Store name
            config: Store configuration
            
        Returns:
            Store instance or None if not found
        """
        store_class = self.get(name)
        if store_class:
            return store_class(config)
        return None
    
    def list_stores(self) -> List[str]:
        """
        List all registered store names.
        
        Returns:
            List of store names
        """
        return list(self._stores.keys())
    
    def list_by_capability(self, capability: str) -> List[str]:
        """
        List stores that support a specific capability.
        
        Args:
            capability: Capability to filter by
            
        Returns:
            List of store names with the capability
        """
        return self._capability_index.get(capability, [])
    
    def list_by_format(self, format_name: str) -> List[str]:
        """
        List stores that support a specific format.
        
        Args:
            format_name: Format to filter by
            
        Returns:
            List of store names supporting the format
        """
        return self._format_index.get(format_name, [])
    
    def list_enterprise_stores(self) -> List[str]:
        """
        List enterprise-ready stores.
        
        Returns:
            List of enterprise-ready store names
        """
        return self._enterprise_stores.copy()
    
    def list_cloud_stores(self) -> List[str]:
        """
        List cloud-supported stores.
        
        Returns:
            List of cloud-supported store names
        """
        return self._cloud_stores.copy()
    
    def get_metadata(self, name: str) -> Optional[Dict[str, Any]]:
        """
        Get metadata for a store.
        
        Args:
            name: Store name
            
        Returns:
            Store metadata or None if not found
        """
        return self._metadata_cache.get(name)
    
    def discover_stores(self, search_paths: Optional[List[str]] = None) -> None:
        """
        Automatically discover and register store classes.
        
        Args:
            search_paths: Optional list of paths to search for store modules
        """
        if search_paths is None:
            search_paths = ['autotel.stores']
        
        for search_path in search_paths:
            try:
                module = importlib.import_module(search_path)
                self._discover_in_module(module)
            except ImportError:
                continue
    
    def _discover_in_module(self, module: Any) -> None:
        """Discover store classes in a module."""
        for name, obj in inspect.getmembers(module):
            if (inspect.isclass(obj) and 
                issubclass(obj, BaseStore) and 
                obj != BaseStore and
                hasattr(obj, '_store_metadata')):
                
                metadata = getattr(obj, '_store_metadata', {})
                store_name = metadata.get('name', name.lower())
                self.register(store_name, obj)
    
    def get_cli_commands(self) -> Dict[str, Callable]:
        """
        Get CLI commands for all stores.
        
        Returns:
            Dictionary of CLI command functions
        """
        commands = {}
        
        for name, store_class in self._stores.items():
            metadata = self._metadata_cache.get(name, {})
            if metadata.get('cli_enabled', True):
                # Generate CLI command for this store
                commands[f"store_{name}"] = self._create_cli_command(name, store_class)
        
        return commands
    
    def _create_cli_command(self, name: str, store_class: Type[BaseStore]) -> Callable:
        """Create a CLI command for a store."""
        def cli_command(config_path: str, operation: str = "load", data: Optional[str] = None):
            """CLI command for store operations."""
            config = StoreConfig(name=name, path=config_path)
            store = store_class(config)
            
            if operation == "load":
                result = store.load()
            elif operation == "save" and data:
                result = store.save(data)
            else:
                raise ValueError(f"Invalid operation: {operation}")
            
            return result
        
        return cli_command
    
    def get_api_endpoints(self) -> Dict[str, Dict[str, Any]]:
        """
        Get API endpoints for all stores.
        
        Returns:
            Dictionary of API endpoint configurations
        """
        endpoints = {}
        
        for name, store_class in self._stores.items():
            metadata = self._metadata_cache.get(name, {})
            if metadata.get('api_enabled', True):
                endpoints[f"/api/stores/{name}"] = {
                    "methods": ["GET", "POST", "PUT", "DELETE"],
                    "store_class": store_class,
                    "metadata": metadata
                }
        
        return endpoints
    
    def analyze_performance(self) -> Dict[str, Dict[str, Any]]:
        """
        Analyze performance characteristics of all stores.
        
        Returns:
            Dictionary of performance analysis results
        """
        analysis = {}
        
        for name, metadata in self._metadata_cache.items():
            performance = metadata.get('performance_characteristics', {})
            analysis[name] = {
                "capabilities": metadata.get('capabilities', []),
                "formats": metadata.get('supported_formats', []),
                "enterprise_ready": metadata.get('enterprise_ready', False),
                "cloud_supported": metadata.get('cloud_supported', False),
                "performance": performance,
                "security_requirements": metadata.get('security_requirements', []),
                "compliance_tags": metadata.get('compliance_tags', [])
            }
        
        return analysis


# Global registry instance
registry = StoreRegistry() 