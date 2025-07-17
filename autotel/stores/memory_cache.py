"""
Memory Cache Store Implementation

Provides in-memory cache persistence for fast, ephemeral storage.
"""

import time
from typing import Any, Dict

from .base import BaseStore, StoreConfig, StoreResult
from .meta import store_metadata


@store_metadata(
    name="memory_cache_store",
    version="2.1.0",
    capabilities=["cache", "memory", "ephemeral"],
    supported_formats=["dict", "json"],
    author="AutoTel Team",
    cli_enabled=True,
    api_enabled=True,
    enterprise_ready=False,
    cloud_supported=False,
    performance_characteristics={
        "read_speed": "fast",
        "write_speed": "fast", 
        "memory_usage": "variable",
        "file_size": "n/a"
    },
    security_requirements=[],
    compliance_tags=[]
)
class MemoryCacheStore(BaseStore):
    """
    In-memory cache store implementation.
    """
    CAPABILITIES = ["cache", "memory", "ephemeral"]
    SUPPORTED_FORMATS = ["dict", "json"]
    _cache: Dict[str, Any] = {}

    def _load_impl(self) -> StoreResult:
        start_time = time.time()
        key = self.get_setting("key")
        if key is None:
            return StoreResult.error_result(
                "No key specified for memory cache load",
                {"error_type": "KeyError"}
            )
        value = self._cache.get(key)
        duration = (time.time() - start_time) * 1000
        if value is None:
            return StoreResult.error_result(
                f"Key not found in memory cache: {key}",
                {"error_type": "KeyError"}
            )
        return StoreResult.success_result(
            value,
            {
                "duration_ms": duration,
                "key": key
            }
        )

    def _save_impl(self, data: Any) -> StoreResult:
        start_time = time.time()
        key = self.get_setting("key")
        if key is None:
            return StoreResult.error_result(
                "No key specified for memory cache save",
                {"error_type": "KeyError"}
            )
        self._cache[key] = data
        duration = (time.time() - start_time) * 1000
        return StoreResult.success_result(
            None,
            {
                "duration_ms": duration,
                "key": key
            }
        ) 