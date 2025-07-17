"""
AutoTel Store Metaclass

Provides automatic registration, interface validation, and metadata collection
for all store classes using metaclass pattern.
"""

from typing import Dict, Any, Optional, Type, ClassVar
from abc import ABCMeta


class StoreMeta(ABCMeta):
    """
    Metaclass for AutoTel stores providing automatic registration and validation.
    
    Features:
    - Automatic registration with StoreRegistry
    - Interface validation (ensures required methods exist)
    - Metadata collection (capabilities, version, etc.)
    - Plugin system support
    """
    
    def __new__(mcs, name: str, bases: tuple, namespace: Dict[str, Any]) -> Type:
        """Create a new store class with automatic registration."""
        # Create the class
        cls = super().__new__(mcs, name, bases, namespace)
        
        # Skip registration for abstract base classes
        if name in ['BaseStore', 'StoreMeta'] or 'ABC' in str(bases):
            return cls
        
        # Validate interface
        mcs._validate_store_interface(cls)
        
        # Collect metadata
        metadata = mcs._collect_store_metadata(cls, namespace)
        setattr(cls, '_store_metadata', metadata)
        
        # Auto-register with registry if not abstract
        if not getattr(cls, '__abstractmethods__', None):
            # Lazy import to avoid circular dependency
            from autotel.stores.registry import registry
            # Always use metadata name for consistency
            store_name = metadata.get('name', name.lower())
            registry.register(store_name, cls)
        
        return cls
    
    @classmethod
    def _validate_store_interface(mcs, cls: Type) -> None:
        """Validate that the store implements required interface."""
        required_methods = ['_load_impl', '_save_impl']
        
        for method_name in required_methods:
            if not hasattr(cls, method_name):
                raise TypeError(
                    f"Store class '{cls.__name__}' must implement '{method_name}' method"
                )
    
    @classmethod
    def _collect_store_metadata(mcs, cls: Type, namespace: Dict[str, Any]) -> Dict[str, Any]:
        """Collect metadata about the store class."""
        # Start with decorator metadata - check both namespace and class attributes
        metadata = {
            'name': (namespace.get('__store_name__') or 
                    getattr(cls, '__store_name__', None) or 
                    cls.__name__.lower()),
            'version': (namespace.get('__store_version__') or 
                       getattr(cls, '__store_version__', None) or 
                       '1.0.0'),
            'description': namespace.get('__doc__', ''),
            'capabilities': (namespace.get('__store_capabilities__') or 
                           getattr(cls, '__store_capabilities__', None) or 
                           []),
            'supported_formats': (namespace.get('__supported_formats__') or 
                                getattr(cls, '__supported_formats__', None) or 
                                []),
            'author': (namespace.get('__store_author__') or 
                      getattr(cls, '__store_author__', None) or 
                      'Unknown'),
            'class_name': cls.__name__,
            'module': cls.__module__,
            'cli_enabled': namespace.get('__cli_enabled__', True),
            'api_enabled': namespace.get('__api_enabled__', True),
            'enterprise_ready': namespace.get('__enterprise_ready__', False),
            'cloud_supported': namespace.get('__cloud_supported__', False),
            'performance_characteristics': namespace.get('__performance_characteristics__', {}),
            'security_requirements': namespace.get('__security_requirements__', []),
            'compliance_tags': namespace.get('__compliance_tags__', []),
        }
        
        # Merge with class attributes (class attributes take precedence)
        if hasattr(cls, 'CAPABILITIES'):
            class_capabilities = getattr(cls, 'CAPABILITIES')
            if isinstance(class_capabilities, list):
                metadata['capabilities'] = class_capabilities
            else:
                metadata['capabilities'].extend(class_capabilities)
        
        if hasattr(cls, 'SUPPORTED_FORMATS'):
            class_formats = getattr(cls, 'SUPPORTED_FORMATS')
            if isinstance(class_formats, list):
                metadata['supported_formats'] = class_formats
            else:
                metadata['supported_formats'].extend(class_formats)
        
        return metadata
    
    @classmethod
    def _update_metadata_from_decorator(mcs, cls: Type) -> None:
        """Update metadata from decorator attributes after class creation."""
        if hasattr(cls, '_store_metadata'):
            metadata = cls._store_metadata
            
            # Check for decorator attributes and update metadata
            if hasattr(cls, '__store_name__'):
                metadata['name'] = cls.__store_name__
                # Update registry with correct name
                from autotel.stores.registry import registry
                old_name = cls.__name__.lower()
                if old_name in registry._stores:
                    del registry._stores[old_name]
                registry.register(cls.__store_name__, cls)
            
            if hasattr(cls, '__store_version__'):
                metadata['version'] = cls.__store_version__
            
            if hasattr(cls, '__store_capabilities__'):
                metadata['capabilities'] = cls.__store_capabilities__
            
            if hasattr(cls, '__supported_formats__'):
                metadata['supported_formats'] = cls.__supported_formats__
            
            if hasattr(cls, '__store_author__'):
                metadata['author'] = cls.__store_author__
            
            if hasattr(cls, '__cli_enabled__'):
                metadata['cli_enabled'] = cls.__cli_enabled__
            
            if hasattr(cls, '__api_enabled__'):
                metadata['api_enabled'] = cls.__api_enabled__
            
            if hasattr(cls, '__enterprise_ready__'):
                metadata['enterprise_ready'] = cls.__enterprise_ready__
            
            if hasattr(cls, '__cloud_supported__'):
                metadata['cloud_supported'] = cls.__cloud_supported__
            
            if hasattr(cls, '__performance_characteristics__'):
                metadata['performance_characteristics'] = cls.__performance_characteristics__
            
            if hasattr(cls, '__security_requirements__'):
                metadata['security_requirements'] = cls.__security_requirements__
            
            if hasattr(cls, '__compliance_tags__'):
                metadata['compliance_tags'] = cls.__compliance_tags__
    
    def __call__(cls, *args, **kwargs):
        """Enhanced instantiation with metadata validation."""
        # Update metadata from decorator if needed
        cls._update_metadata_from_decorator(cls)
        
        # Create instance
        instance = super().__call__(*args, **kwargs)
        
        # Validate metadata if present
        if hasattr(instance, '_store_metadata'):
            cls._validate_instance_metadata(instance)
        
        return instance
    
    @classmethod
    def _validate_instance_metadata(cls, instance: Any) -> None:
        """Validate instance metadata after creation."""
        metadata = getattr(instance, '_store_metadata', {})
        
        # Ensure required metadata fields
        required_fields = ['name', 'version']
        for field in required_fields:
            if field not in metadata:
                raise ValueError(f"Store metadata missing required field: {field}")


def store_metadata(
    name: Optional[str] = None,
    version: str = "1.0.0",
    capabilities: Optional[list] = None,
    supported_formats: Optional[list] = None,
    author: str = "Unknown",
    cli_enabled: bool = True,
    api_enabled: bool = True,
    enterprise_ready: bool = False,
    cloud_supported: bool = False,
    performance_characteristics: Optional[dict] = None,
    security_requirements: Optional[list] = None,
    compliance_tags: Optional[list] = None,
) -> Dict[str, Any]:
    """
    Decorator to add metadata to store classes.
    
    Args:
        name: Store name (defaults to class name)
        version: Store version
        capabilities: List of store capabilities
        supported_formats: List of supported input/output formats
        author: Store author
        cli_enabled: Whether CLI commands should be auto-generated
        api_enabled: Whether API endpoints should be auto-generated
        enterprise_ready: Whether store is enterprise-ready
        cloud_supported: Whether store supports cloud storage
        performance_characteristics: Performance metadata
        security_requirements: Security requirements
        compliance_tags: Compliance tags
    
    Example:
        @store_metadata(
            name="json_store",
            version="2.1.0",
            capabilities=["serialization", "validation"],
            supported_formats=["json"],
            enterprise_ready=True
        )
        class JSONStore(BaseStore):
            pass
    """
    def decorator(cls: Type) -> Type:
        # Add metadata to class namespace
        cls.__store_name__ = name or cls.__name__.lower()
        cls.__store_version__ = version
        cls.__store_capabilities__ = capabilities or []
        cls.__supported_formats__ = supported_formats or []
        cls.__store_author__ = author
        cls.__cli_enabled__ = cli_enabled
        cls.__api_enabled__ = api_enabled
        cls.__enterprise_ready__ = enterprise_ready
        cls.__cloud_supported__ = cloud_supported
        cls.__performance_characteristics__ = performance_characteristics or {}
        cls.__security_requirements__ = security_requirements or []
        cls.__compliance_tags__ = compliance_tags or []
        
        return cls
    
    return decorator 