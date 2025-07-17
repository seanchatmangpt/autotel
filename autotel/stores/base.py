"""
AutoTel Base Store

Unified base class for all stores using helper infrastructure.
Integrates contracts, error handling, and configuration.
"""

from abc import ABC, abstractmethod
from typing import Any, Dict, Optional, List, Union
from pathlib import Path

from pydantic import BaseModel, Field, field_validator, ConfigDict
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.helpers import safe_get
from autotel.stores.meta import StoreMeta


class StoreConfig(BaseModel):
    """Configuration for a store instance."""
    model_config = ConfigDict(
        json_schema_extra={
            "examples": [
                {
                    "name": "json_store",
                    "enabled": True,
                    "path": "/path/to/data.json",
                    "settings": {"indent": 2, "encoding": "utf-8"}
                }
            ]
        }
    )
    
    name: str = Field(..., description="Store name")
    enabled: bool = Field(default=True, description="Whether store is enabled")
    path: str = Field(..., description="Store file path or connection string")
    settings: Dict[str, Any] = Field(default_factory=dict, description="Store settings")
    
    @field_validator('name')
    @classmethod
    def validate_name(cls, v: str) -> str:
        """Validate store name is not empty."""
        if not v.strip():
            raise ValueError("Store name cannot be empty")
        return v.strip()
    
    @field_validator('path')
    @classmethod
    def validate_path(cls, v: str) -> str:
        """Validate store path is not empty."""
        if not v.strip():
            raise ValueError("Store path cannot be empty")
        return v.strip()


class StoreResult(BaseModel):
    """Result from store operations."""
    model_config = ConfigDict(
        json_schema_extra={
            "examples": [
                {
                    "success": True,
                    "data": {"loaded": "content"},
                    "metadata": {"size_bytes": 1024, "duration_ms": 45.2}
                },
                {
                    "success": False,
                    "error": "File not found",
                    "metadata": {"error_type": "FileNotFoundError"}
                }
            ]
        }
    )
    
    success: bool = Field(..., description="Whether operation was successful")
    data: Optional[Any] = Field(default=None, description="Operation result data")
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
    def success_result(cls, data: Any, metadata: Optional[Dict[str, Any]] = None) -> 'StoreResult':
        """Create a successful result."""
        return cls(success=True, data=data, metadata=metadata or {})
    
    @classmethod
    def error_result(cls, error: str, metadata: Optional[Dict[str, Any]] = None) -> 'StoreResult':
        """Create an error result."""
        return cls(success=False, error=error, metadata=metadata or {})


class BaseStore(ABC, metaclass=StoreMeta):
    """
    Unified base class for all AutoTel stores.
    
    Integrates:
    - Contract validation (pre/post conditions)
    - Error handling
    - Configuration management
    - Helper utilities
    - Automatic registration via metaclass
    """
    
    def __init__(self, config: StoreConfig):
        self.config = config
        self._validate_config()
    
    def _validate_config(self) -> None:
        """Validate store configuration."""
        # Pydantic handles most validation, but we can add custom logic here
        pass
    
    @contract_precondition(lambda self: self.config.enabled)
    @contract_postcondition(lambda result: isinstance(result, StoreResult))
    def load(self) -> StoreResult:
        """
        Load data from store with full infrastructure integration.
        
        Returns:
            StoreResult with success/error status and data
        """
        try:
            # Execute actual loading
            result = self._load_impl()
            return result
            
        except Exception as e:
            # Return error result
            return StoreResult.error_result(str(e), {"error_type": type(e).__name__})
    
    @contract_precondition(lambda self, data: data is not None)
    @contract_precondition(lambda self: self.config.enabled)
    @contract_postcondition(lambda result: isinstance(result, StoreResult))
    def save(self, data: Any) -> StoreResult:
        """
        Save data to store with full infrastructure integration.
        
        Args:
            data: Data to save
            
        Returns:
            StoreResult with success/error status
        """
        try:
            # Execute actual saving
            result = self._save_impl(data)
            return result
            
        except Exception as e:
            # Return error result
            return StoreResult.error_result(str(e), {"error_type": type(e).__name__})
    
    @abstractmethod
    def _load_impl(self) -> StoreResult:
        """
        Implementation-specific loading logic.
        Override this method in concrete store classes.
        
        Returns:
            StoreResult with loading results
        """
        pass
    
    @abstractmethod
    def _save_impl(self, data: Any) -> StoreResult:
        """
        Implementation-specific saving logic.
        Override this method in concrete store classes.
        
        Args:
            data: Data to save
            
        Returns:
            StoreResult with saving results
        """
        pass
    
    def exists(self) -> bool:
        """Check if store exists."""
        try:
            return Path(self.config.path).exists()
        except Exception:
            return False
    
    def get_name(self) -> str:
        """Get store name."""
        return self.config.name
    
    def get_config(self) -> StoreConfig:
        """Get store configuration."""
        return self.config
    
    def is_enabled(self) -> bool:
        """Check if store is enabled."""
        return self.config.enabled
    
    def get_setting(self, key: str, default: Any = None) -> Any:
        """Get a configuration setting."""
        value = safe_get(self.config.settings, key)
        return value if value is not None else default
    
    def set_setting(self, key: str, value: Any) -> None:
        """Set a configuration setting."""
        self.config.settings[key] = value
    
    def get_metadata(self) -> Dict[str, Any]:
        """Get store metadata."""
        return getattr(self, '_store_metadata', {})
    
    def get_capabilities(self) -> List[str]:
        """Get store capabilities."""
        metadata = self.get_metadata()
        return metadata.get('capabilities', [])
    
    def get_supported_formats(self) -> List[str]:
        """Get supported input/output formats."""
        metadata = self.get_metadata()
        return metadata.get('supported_formats', []) 