"""
AutoTel Parquet Store

Store implementation for Parquet file operations using pandas.
"""

from typing import Any, Optional
from pathlib import Path

from autotel.stores.base import BaseStore, StoreResult
from autotel.stores.meta import store_metadata


@store_metadata(
    name="parquet_store",
    version="1.0.0",
    capabilities=["columnar_storage", "compression", "schema_preservation"],
    supported_formats=["parquet"],
    author="AutoTel",
    enterprise_ready=True,
    cloud_supported=True,
    performance_characteristics={
        "load_time": "Fast - columnar format",
        "memory_usage": "Efficient - columnar compression",
        "compression": "High - built-in compression"
    }
)
class ParquetStore(BaseStore):
    """
    Store implementation for Parquet file operations.
    
    Uses pandas for Parquet reading and writing. Supports both
    loading Parquet files into DataFrames and saving data as Parquet.
    
    Example:
        config = StoreConfig(name="data", path="data.parquet")
        store = ParquetStore(config)
        result = store.load()  # Returns pandas.DataFrame
    """

    def _load_impl(self) -> StoreResult:
        """
        Load Parquet data from file.
        
        Returns:
            StoreResult with pandas.DataFrame as data
        """
        try:
            import pandas as pd
            
            # Get settings
            engine = self.get_setting("engine", "pyarrow")
            columns = self.get_setting("columns", None)
            
            # Load data
            df = pd.read_parquet(
                self.config.path,
                engine=engine,
                columns=columns
            )
            
            # Add metadata about the DataFrame
            metadata = {
                "shape": df.shape,
                "columns": list(df.columns),
                "dtypes": df.dtypes.to_dict(),
                "file_size": Path(self.config.path).stat().st_size if Path(self.config.path).exists() else 0,
                "engine": engine
            }
            
            return StoreResult.success_result(df, metadata)
            
        except ImportError:
            return StoreResult.error_result(
                "pandas is required for Parquet operations. Install with: pip install pandas pyarrow",
                {"error_type": "ImportError", "missing_dependency": "pandas"}
            )
        except FileNotFoundError:
            return StoreResult.error_result(
                f"Parquet file not found: {self.config.path}",
                {"error_type": "FileNotFoundError"}
            )
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to load Parquet: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def _save_impl(self, data: Any) -> StoreResult:
        """
        Save data as Parquet file.
        
        Args:
            data: pandas.DataFrame or dict/list to save as Parquet
            
        Returns:
            StoreResult with save operation result
        """
        try:
            import pandas as pd
            
            # Convert data to DataFrame if needed
            if isinstance(data, pd.DataFrame):
                df = data
            elif isinstance(data, dict):
                df = pd.DataFrame([data])
            elif isinstance(data, list):
                df = pd.DataFrame(data)
            else:
                return StoreResult.error_result(
                    f"Unsupported data type for Parquet: {type(data).__name__}. "
                    "Supported: pandas.DataFrame, dict, list",
                    {"error_type": "TypeError"}
                )
            
            # Get settings
            engine = self.get_setting("engine", "pyarrow")
            compression = self.get_setting("compression", "snappy")
            index = self.get_setting("index", False)
            
            # Ensure directory exists
            Path(self.config.path).parent.mkdir(parents=True, exist_ok=True)
            
            # Save data
            df.to_parquet(
                self.config.path,
                engine=engine,
                compression=compression,
                index=index
            )
            
            metadata = {
                "shape": df.shape,
                "columns": list(df.columns),
                "dtypes": df.dtypes.to_dict(),
                "file_size": Path(self.config.path).stat().st_size,
                "engine": engine,
                "compression": compression
            }
            
            return StoreResult.success_result(data, metadata)
            
        except ImportError:
            return StoreResult.error_result(
                "pandas is required for Parquet operations. Install with: pip install pandas pyarrow",
                {"error_type": "ImportError", "missing_dependency": "pandas"}
            )
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to save Parquet: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def get_schema(self) -> Optional[dict]:
        """Get Parquet file schema information."""
        try:
            import pandas as pd
            
            if not Path(self.config.path).exists():
                return None
            
            # Read schema without loading data
            df = pd.read_parquet(self.config.path, engine="pyarrow")
            
            return {
                "columns": list(df.columns),
                "dtypes": df.dtypes.to_dict(),
                "shape": df.shape
            }
            
        except ImportError:
            return None
        except Exception:
            return None 