"""
AutoTel Base Processor

Unified base class for all processors using helper infrastructure.
Integrates contracts, error handling, and configuration.
"""

from abc import ABC, abstractmethod
from typing import Any, Dict, Optional, List
from pathlib import Path

from pydantic import BaseModel, Field, field_validator, ConfigDict
from autotel.helpers.contract import BaseProcessorABC, contract_precondition, contract_postcondition
from autotel.helpers.config.loader import load_processor_config, get_config_value
from autotel.helpers.helpers import deep_merge_dicts, safe_get
from autotel.processors.meta import ProcessorMeta


class ProcessorConfig(BaseModel):
    """Configuration for a processor instance."""
    model_config = ConfigDict(
        json_schema_extra={
            "examples": [
                {
                    "name": "bpmn_processor",
                    "enabled": True,
                    "timeout": 30,
                    "settings": {"validate_schema": True}
                }
            ]
        }
    )
    
    name: str = Field(..., description="Processor name")
    enabled: bool = Field(default=True, description="Whether processor is enabled")
    timeout: int = Field(default=30, gt=0, description="Processor timeout in seconds")
    config_path: Optional[str] = Field(default=None, description="Path to config file")
    settings: Dict[str, Any] = Field(default_factory=dict, description="Processor settings")
    
    @field_validator('name')
    @classmethod
    def validate_name(cls, v: str) -> str:
        """Validate processor name is not empty."""
        if not v.strip():
            raise ValueError("Processor name cannot be empty")
        return v.strip()
    
    @classmethod
    def from_file(cls, config_path: str, processor_type: str) -> 'ProcessorConfig':
        """Load configuration from file."""
        config_data = load_processor_config(processor_type, Path(config_path).parent)
        processor_config = config_data.get('processors', {}).get(processor_type, {})
        
        return cls(
            name=processor_type,
            enabled=safe_get(processor_config, 'enabled', True),
            timeout=safe_get(processor_config, 'timeout', 30),
            config_path=config_path,
            settings=safe_get(processor_config, 'settings', {})
        )


class ProcessorResult(BaseModel):
    """Result from processor execution."""
    model_config = ConfigDict(
        json_schema_extra={
            "examples": [
                {
                    "success": True,
                    "data": {"processed": "result"},
                    "metadata": {"duration_ms": 45.2}
                },
                {
                    "success": False,
                    "error": "Invalid input format",
                    "metadata": {"error_type": "ValidationError"}
                }
            ]
        }
    )
    
    success: bool = Field(..., description="Whether processing was successful")
    data: Optional[Any] = Field(default=None, description="Processing result data")
    error: Optional[str] = Field(default=None, description="Error message if failed")
    metadata: Dict[str, Any] = Field(default_factory=dict, description="Additional metadata")
    
    @field_validator('error')
    @classmethod
    def validate_error_consistency(cls, v: Optional[str], info) -> Optional[str]:
        """Validate error is present when success is False."""
        if info.data.get('success') is False and not v:
            raise ValueError("Error message required when success is False")
        return v
    
    @classmethod
    def success_result(cls, data: Any, metadata: Optional[Dict[str, Any]] = None) -> 'ProcessorResult':
        """Create a successful result."""
        return cls(success=True, data=data, metadata=metadata or {})
    
    @classmethod
    def error_result(cls, error: str, metadata: Optional[Dict[str, Any]] = None) -> 'ProcessorResult':
        """Create an error result."""
        return cls(success=False, error=error, metadata=metadata or {})


class BaseProcessor(BaseProcessorABC, metaclass=ProcessorMeta):
    """
    Unified base class for all AutoTel processors.
    
    Integrates:
    - Contract validation (pre/post conditions)
    - Error handling
    - Configuration management
    - Helper utilities
    - Automatic registration via metaclass
    """
    
    def __init__(self, config: ProcessorConfig):
        self.config = config
        self._validate_config()
    
    def _validate_config(self) -> None:
        """Validate processor configuration."""
        # Pydantic handles most validation, but we can add custom logic here
        pass
    
    @contract_precondition(lambda self, data: data is not None)
    @contract_postcondition(lambda result: isinstance(result, ProcessorResult))
    def process(self, data: Any) -> ProcessorResult:
        """
        Process input data with full infrastructure integration.
        
        Args:
            data: Input data to process
            
        Returns:
            ProcessorResult with success/error status and data
        """
        try:
            # Execute actual processing
            result = self._process_impl(data)
            return result
            
        except Exception as e:
            # Return error result
            return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})
    
    @abstractmethod
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Implementation-specific processing logic.
        Override this method in concrete processor classes.
        
        Args:
            data: Input data to process
            
        Returns:
            ProcessorResult with processing results
        """
        pass
    
    def get_name(self) -> str:
        """Get processor name."""
        return self.config.name
    
    def get_config(self) -> ProcessorConfig:
        """Get processor configuration."""
        return self.config
    
    def is_enabled(self) -> bool:
        """Check if processor is enabled."""
        return self.config.enabled
    
    def get_setting(self, key: str, default: Any = None) -> Any:
        """Get a configuration setting."""
        value = safe_get(self.config.settings, key)
        return value if value is not None else default
    
    def set_setting(self, key: str, value: Any) -> None:
        """Set a configuration setting."""
        self.config.settings[key] = value
    
    def get_metadata(self) -> Dict[str, Any]:
        """Get processor metadata."""
        return getattr(self, '_processor_metadata', {})
    
    def get_capabilities(self) -> List[str]:
        """Get processor capabilities."""
        metadata = self.get_metadata()
        return metadata.get('capabilities', [])
    
    def get_supported_formats(self) -> List[str]:
        """Get supported input/output formats."""
        metadata = self.get_metadata()
        return metadata.get('supported_formats', []) 