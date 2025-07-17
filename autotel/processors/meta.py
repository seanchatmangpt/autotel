"""
AutoTel Processor Metaclass

Provides automatic registration, interface validation, and metadata collection
for all processor classes using metaclass pattern.
"""

from typing import Dict, Any, Optional, Type, ClassVar
from abc import ABCMeta


class ProcessorMeta(ABCMeta):
    """
    Metaclass for AutoTel processors providing automatic registration and validation.
    
    Features:
    - Automatic registration with ProcessorRegistry
    - Interface validation (ensures required methods exist)
    - Metadata collection (capabilities, version, etc.)
    - Plugin system support
    """
    
    def __new__(mcs, name: str, bases: tuple, namespace: Dict[str, Any]) -> Type:
        """Create a new processor class with automatic registration."""
        # Create the class
        cls = super().__new__(mcs, name, bases, namespace)
        
        # Skip registration for abstract base classes
        if name in ['BaseProcessor', 'ProcessorMeta'] or 'ABC' in str(bases):
            return cls
        
        # Validate interface
        mcs._validate_processor_interface(cls)
        
        # Collect metadata
        metadata = mcs._collect_processor_metadata(cls, namespace)
        setattr(cls, '_processor_metadata', metadata)
        
        # Auto-register with registry if not abstract
        if not getattr(cls, '__abstractmethods__', None):
            # Lazy import to avoid circular dependency
            from autotel.processors.registry import registry
            processor_name = metadata.get('name', name.lower())
            registry.register(processor_name, cls)
        
        return cls
    
    @classmethod
    def _validate_processor_interface(mcs, cls: Type) -> None:
        """Validate that the processor implements required interface."""
        required_methods = ['_process_impl']
        
        for method_name in required_methods:
            if not hasattr(cls, method_name):
                raise TypeError(
                    f"Processor class '{cls.__name__}' must implement '{method_name}' method"
                )
    
    @classmethod
    def _collect_processor_metadata(mcs, cls: Type, namespace: Dict[str, Any]) -> Dict[str, Any]:
        """Collect metadata about the processor class."""
        # Start with decorator metadata
        metadata = {
            'name': namespace.get('__processor_name__', cls.__name__.lower()),
            'version': namespace.get('__processor_version__', '1.0.0'),
            'description': namespace.get('__doc__', ''),
            'capabilities': namespace.get('__processor_capabilities__', []),
            'supported_formats': namespace.get('__supported_formats__', []),
            'author': namespace.get('__processor_author__', 'Unknown'),
            'class_name': cls.__name__,
            'module': cls.__module__,
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
    
    def __call__(cls, *args, **kwargs):
        """Enhanced instantiation with metadata validation."""
        # Create instance
        instance = super().__call__(*args, **kwargs)
        
        # Validate metadata if present
        if hasattr(instance, '_processor_metadata'):
            cls._validate_instance_metadata(instance)
        
        return instance
    
    @classmethod
    def _validate_instance_metadata(cls, instance: Any) -> None:
        """Validate instance metadata after creation."""
        metadata = getattr(instance, '_processor_metadata', {})
        
        # Ensure required metadata fields
        required_fields = ['name', 'version']
        for field in required_fields:
            if field not in metadata:
                raise ValueError(f"Processor metadata missing required field: {field}")


def processor_metadata(
    name: Optional[str] = None,
    version: str = "1.0.0",
    capabilities: Optional[list] = None,
    supported_formats: Optional[list] = None,
    author: str = "Unknown"
) -> Dict[str, Any]:
    """
    Decorator to add metadata to processor classes.
    
    Args:
        name: Processor name (defaults to class name)
        version: Processor version
        capabilities: List of processor capabilities
        supported_formats: List of supported input/output formats
        author: Processor author
    
    Example:
        @processor_metadata(
            name="bpmn_processor",
            version="2.1.0",
            capabilities=["parsing", "validation"],
            supported_formats=["bpmn", "xml"]
        )
        class BPMNProcessor(BaseProcessor):
            pass
    """
    def decorator(cls: Type) -> Type:
        # Add metadata to class namespace
        cls.__processor_name__ = name or cls.__name__.lower()
        cls.__processor_version__ = version
        cls.__processor_capabilities__ = capabilities or []
        cls.__supported_formats__ = supported_formats or []
        cls.__processor_author__ = author
        
        return cls
    
    return decorator 