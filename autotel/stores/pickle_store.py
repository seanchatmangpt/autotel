"""
Pickle Store Implementation

Provides Pickle file persistence with validation and enterprise features.
"""

import pickle
import time
from pathlib import Path
from typing import Any

from .base import BaseStore, StoreConfig, StoreResult
from .meta import store_metadata


@store_metadata(
    name="pickle_store",
    version="2.1.0",
    capabilities=["serialization"],
    supported_formats=["pickle", "pkl"],
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
class PickleStore(BaseStore):
    """
    Pickle file store implementation.
    """
    CAPABILITIES = ["serialization"]
    SUPPORTED_FORMATS = ["pickle", "pkl"]

    def _load_impl(self) -> StoreResult:
        start_time = time.time()
        try:
            path = Path(self.config.path)
            if not path.exists():
                return StoreResult.error_result(
                    f"File not found: {self.config.path}",
                    {"error_type": "FileNotFoundError"}
                )
            with open(path, 'rb') as f:
                data = pickle.load(f)
            duration = (time.time() - start_time) * 1000
            return StoreResult.success_result(
                data,
                {
                    "size_bytes": path.stat().st_size,
                    "duration_ms": duration,
                    "file_path": str(path)
                }
            )
        except pickle.UnpicklingError as e:
            return StoreResult.error_result(
                f"Invalid Pickle format: {e}",
                {"error_type": "UnpicklingError"}
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
            with open(path, 'wb') as f:
                pickle.dump(data, f)
            duration = (time.time() - start_time) * 1000
            return StoreResult.success_result(
                None,
                {
                    "size_bytes": path.stat().st_size,
                    "duration_ms": duration,
                    "file_path": str(path)
                }
            )
        except Exception as e:
            return StoreResult.error_result(
                str(e),
                {"error_type": type(e).__name__}
            ) 