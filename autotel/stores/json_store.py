"""
JSON Store Implementation

Provides JSON file persistence with validation and enterprise features.
"""

import json
import time
from pathlib import Path
from typing import Any, Dict

from .base import BaseStore, StoreConfig, StoreResult
from .meta import store_metadata


@store_metadata(
    name="json_store",
    version="2.1.0",
    capabilities=["serialization", "validation", "schema_validation"],
    supported_formats=["json"],
    author="AutoTel Team",
    cli_enabled=True,
    api_enabled=True,
    enterprise_ready=True,
    cloud_supported=True,
    performance_characteristics={
        "read_speed": "fast",
        "write_speed": "fast", 
        "memory_usage": "low",
        "file_size": "compact"
    },
    security_requirements=["encryption", "access_control"],
    compliance_tags=["GDPR", "SOX", "HIPAA"]
)
class JSONStore(BaseStore):
    """
    JSON file store implementation.
    
    Features:
    - JSON serialization/deserialization
    - Schema validation (optional)
    - Pretty printing
    - Encoding support
    - Error handling
    """
    
    CAPABILITIES = ["serialization", "validation", "schema_validation"]
    SUPPORTED_FORMATS = ["json"]
    
    def _load_impl(self) -> StoreResult:
        """
        Load data from JSON file.
        
        Returns:
            StoreResult with loaded data or error
        """
        start_time = time.time()
        
        try:
            path = Path(self.config.path)
            
            if not path.exists():
                return StoreResult.error_result(
                    f"File not found: {self.config.path}",
                    {"error_type": "FileNotFoundError"}
                )
            
            # Get encoding setting
            encoding = self.get_setting("encoding", "utf-8")
            
            # Load JSON data
            with open(path, 'r', encoding=encoding) as f:
                data = json.load(f)
            
            duration = (time.time() - start_time) * 1000
            
            return StoreResult.success_result(
                data,
                {
                    "size_bytes": path.stat().st_size,
                    "duration_ms": duration,
                    "encoding": encoding,
                    "file_path": str(path)
                }
            )
            
        except json.JSONDecodeError as e:
            return StoreResult.error_result(
                f"Invalid JSON format: {e}",
                {"error_type": "JSONDecodeError", "line": e.lineno, "column": e.colno}
            )
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            )
    
    def _save_impl(self, data: Any) -> StoreResult:
        """
        Save data to JSON file.
        
        Args:
            data: Data to save
            
        Returns:
            StoreResult with save status
        """
        start_time = time.time()
        
        try:
            path = Path(self.config.path)
            
            # Create directory if it doesn't exist
            path.parent.mkdir(parents=True, exist_ok=True)
            
            # Get settings
            encoding = self.get_setting("encoding", "utf-8")
            indent = self.get_setting("indent", 2)
            ensure_ascii = self.get_setting("ensure_ascii", False)
            sort_keys = self.get_setting("sort_keys", False)
            
            # Save JSON data
            with open(path, 'w', encoding=encoding) as f:
                json.dump(
                    data, 
                    f, 
                    indent=indent,
                    ensure_ascii=ensure_ascii,
                    sort_keys=sort_keys
                )
            
            duration = (time.time() - start_time) * 1000
            
            return StoreResult.success_result(
                None,
                {
                    "size_bytes": path.stat().st_size,
                    "duration_ms": duration,
                    "encoding": encoding,
                    "file_path": str(path),
                    "indent": indent,
                    "ensure_ascii": ensure_ascii,
                    "sort_keys": sort_keys
                }
            )
            
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            )
    
    def validate_schema(self, schema: Dict[str, Any]) -> StoreResult:
        """
        Validate data against JSON schema.
        
        Args:
            schema: JSON schema to validate against
            
        Returns:
            StoreResult with validation results
        """
        try:
            from jsonschema import validate, ValidationError
            
            # Load current data
            load_result = self.load()
            if not load_result.success:
                return load_result
            
            # Validate against schema
            validate(instance=load_result.data, schema=schema)
            
            return StoreResult.success_result(
                {"valid": True},
                {"schema_validated": True}
            )
            
        except ImportError:
            return StoreResult.error_result(
                "jsonschema library not installed",
                {"error_type": "ImportError"}
            )
        except ValidationError as e:
            return StoreResult.error_result(
                f"Schema validation failed: {e.message}",
                {
                    "error_type": "ValidationError",
                    "path": list(e.path),
                    "schema_path": list(e.schema_path)
                }
            )
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            ) 