"""
YAML Store Implementation

Provides YAML file persistence with validation and enterprise features.
"""

import yaml
import time
from pathlib import Path
from typing import Any, Dict

from .base import BaseStore, StoreConfig, StoreResult
from .meta import store_metadata


@store_metadata(
    name="yaml_store",
    version="2.1.0",
    capabilities=["serialization", "validation"],
    supported_formats=["yaml", "yml"],
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
class YAMLStore(BaseStore):
    """
    YAML file store implementation.
    """
    CAPABILITIES = ["serialization", "validation"]
    SUPPORTED_FORMATS = ["yaml", "yml"]

    def _load_impl(self) -> StoreResult:
        start_time = time.time()
        try:
            path = Path(self.config.path)
            if not path.exists():
                return StoreResult.error_result(
                    f"File not found: {self.config.path}",
                    {"error_type": "FileNotFoundError"}
                )
            encoding = self.get_setting("encoding", "utf-8")
            with open(path, 'r', encoding=encoding) as f:
                data = yaml.safe_load(f)
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
        except yaml.YAMLError as e:
            return StoreResult.error_result(
                f"Invalid YAML format: {e}",
                {"error_type": "YAMLError"}
            )
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            )

    def _save_impl(self, data: Any) -> StoreResult:
        start_time = time.time()
        try:
            path = Path(self.config.path)
            path.parent.mkdir(parents=True, exist_ok=True)
            encoding = self.get_setting("encoding", "utf-8")
            default_flow_style = self.get_setting("default_flow_style", False)
            with open(path, 'w', encoding=encoding) as f:
                yaml.safe_dump(data, f, default_flow_style=default_flow_style, allow_unicode=True)
            duration = (time.time() - start_time) * 1000
            return StoreResult.success_result(
                None,
                {
                    "size_bytes": path.stat().st_size,
                    "duration_ms": duration,
                    "encoding": encoding,
                    "file_path": str(path),
                    "default_flow_style": default_flow_style
                }
            )
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            ) 