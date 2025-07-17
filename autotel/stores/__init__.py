"""
AutoTel Stores Module

Provides unified data persistence layer with enterprise capabilities.
Supports multiple formats (JSON, YAML, XML, RDF, etc.) with automatic discovery.
"""

from .base import BaseStore, StoreConfig, StoreResult
from .registry import StoreRegistry, registry
from .meta import StoreMeta, store_metadata
from .any_store import AnyStore, register_store_extension

__all__ = [
    'BaseStore',
    'StoreConfig', 
    'StoreResult',
    'StoreRegistry',
    'registry',
    'StoreMeta',
    'store_metadata',
    'AnyStore',
    'register_store_extension'
]

# Auto-discover stores on import
registry.discover_stores() 