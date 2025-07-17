"""
AutoTel Processor Registry

Manages processor registration, discovery, and instantiation.
Provides factory pattern for processor creation with metadata support.
"""

from typing import Dict, Type, Optional, List, Any
from autotel.processors.base import BaseProcessor, ProcessorConfig


class ProcessorRegistry:
    """
    Registry for managing processor classes and instances with metadata support.
    
    Provides:
    - Registration of processor classes
    - Factory methods for instantiation
    - Discovery and listing capabilities
    - Metadata-based filtering and search
    """
    
    def __init__(self):
        self._processors: Dict[str, Type[BaseProcessor]] = {}
        self._instances: Dict[str, BaseProcessor] = {}
        self._metadata: Dict[str, Dict[str, Any]] = {}
    
    def register(self, name: str, processor_class: Type[BaseProcessor]) -> None:
        """
        Register a processor class with metadata.
        
        Args:
            name: Unique name for the processor
            processor_class: Processor class to register
        """
        if not issubclass(processor_class, BaseProcessor):
            raise ValueError(f"Processor class must inherit from BaseProcessor")
        
        self._processors[name] = processor_class
        
        # Store metadata if available
        if hasattr(processor_class, '_processor_metadata'):
            self._metadata[name] = processor_class._processor_metadata
    
    def get_class(self, name: str) -> Optional[Type[BaseProcessor]]:
        """
        Get a registered processor class.
        
        Args:
            name: Processor name
            
        Returns:
            Processor class or None if not found
        """
        return self._processors.get(name)
    
    def get_metadata(self, name: str) -> Optional[Dict[str, Any]]:
        """
        Get metadata for a registered processor.
        
        Args:
            name: Processor name
            
        Returns:
            Processor metadata or None if not found
        """
        return self._metadata.get(name)
    
    def create(self, name: str, config: ProcessorConfig) -> BaseProcessor:
        """
        Create a processor instance.
        
        Args:
            name: Processor name
            config: Processor configuration
            
        Returns:
            Processor instance
            
        Raises:
            ValueError: If processor not registered
        """
        processor_class = self.get_class(name)
        if not processor_class:
            raise ValueError(f"Processor '{name}' not registered")
        
        instance = processor_class(config)
        self._instances[name] = instance
        return instance
    
    def get_instance(self, name: str) -> Optional[BaseProcessor]:
        """
        Get a processor instance.
        
        Args:
            name: Processor name
            
        Returns:
            Processor instance or None if not found
        """
        return self._instances.get(name)
    
    def list_registered(self) -> List[str]:
        """
        List all registered processor names.
        
        Returns:
            List of processor names
        """
        return list(self._processors.keys())
    
    def list_instances(self) -> List[str]:
        """
        List all created processor instance names.
        
        Returns:
            List of instance names
        """
        return list(self._instances.keys())
    
    def find_by_capability(self, capability: str) -> List[str]:
        """
        Find processors by capability.
        
        Args:
            capability: Capability to search for
            
        Returns:
            List of processor names with the capability
        """
        matching_processors = []
        for name, metadata in self._metadata.items():
            if capability in metadata.get('capabilities', []):
                matching_processors.append(name)
        return matching_processors
    
    def find_by_format(self, format_name: str) -> List[str]:
        """
        Find processors that support a specific format.
        
        Args:
            format_name: Format to search for
            
        Returns:
            List of processor names supporting the format
        """
        matching_processors = []
        for name, metadata in self._metadata.items():
            if format_name in metadata.get('supported_formats', []):
                matching_processors.append(name)
        return matching_processors
    
    def get_all_metadata(self) -> Dict[str, Dict[str, Any]]:
        """
        Get metadata for all registered processors.
        
        Returns:
            Dictionary mapping processor names to their metadata
        """
        return self._metadata.copy()
    
    def clear(self) -> None:
        """Clear all registrations and instances."""
        self._processors.clear()
        self._instances.clear()
        self._metadata.clear()


# Global registry instance
registry = ProcessorRegistry() 