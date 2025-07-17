"""
AutoTel SQLite Store

Store implementation for SQLite database operations.
"""

import sqlite3
from typing import Any, Dict, List, Optional
from pathlib import Path

from autotel.stores.base import BaseStore, StoreResult
from autotel.stores.meta import store_metadata


@store_metadata(
    name="sqlite_store",
    version="1.0.0",
    capabilities=["relational_storage", "sql_queries", "transactions"],
    supported_formats=["db", "sqlite"],
    author="AutoTel",
    enterprise_ready=True,
    performance_characteristics={
        "load_time": "Fast - indexed queries",
        "memory_usage": "Efficient - streaming support",
        "concurrency": "Limited - single writer"
    }
)
class SQLiteStore(BaseStore):
    """
    Store implementation for SQLite database operations.
    
    Supports both loading data from SQLite databases and saving data
    to SQLite tables. Can execute custom SQL queries and handle
    table operations.
    
    Example:
        config = StoreConfig(
            name="data", 
            path="data.db",
            settings={"table": "users", "query": "SELECT * FROM users"}
        )
        store = SQLiteStore(config)
        result = store.load()  # Returns query results
    """

    def _load_impl(self) -> StoreResult:
        """
        Load data from SQLite database.
        
        Returns:
            StoreResult with query results as data
        """
        try:
            # Get settings
            table = self.get_setting("table", None)
            query = self.get_setting("query", None)
            
            if not query and not table:
                return StoreResult.error_result(
                    "Either 'table' or 'query' setting must be specified",
                    {"error_type": "ConfigurationError"}
                )
            
            # Build query if only table specified
            if not query:
                query = f"SELECT * FROM {table}"
            
            # Connect and execute
            with sqlite3.connect(self.config.path) as conn:
                conn.row_factory = sqlite3.Row  # Enable dict-like access
                cursor = conn.cursor()
                cursor.execute(query)
                
                # Fetch results
                rows = cursor.fetchall()
                columns = [description[0] for description in cursor.description] if cursor.description else []
                
                # Convert to list of dicts
                data = [dict(row) for row in rows]
                
                # Add metadata
                metadata = {
                    "row_count": len(data),
                    "column_count": len(columns),
                    "columns": columns,
                    "query": query,
                    "file_size": Path(self.config.path).stat().st_size if Path(self.config.path).exists() else 0
                }
                
                return StoreResult.success_result(data, metadata)
                
        except sqlite3.OperationalError as e:
            return StoreResult.error_result(
                f"SQLite operational error: {str(e)}",
                {"error_type": "OperationalError", "sql_error": str(e)}
            )
        except FileNotFoundError:
            return StoreResult.error_result(
                f"SQLite database not found: {self.config.path}",
                {"error_type": "FileNotFoundError"}
            )
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to load from SQLite: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def _save_impl(self, data: Any) -> StoreResult:
        """
        Save data to SQLite database.
        
        Args:
            data: dict, list of dicts, or pandas.DataFrame to save
            
        Returns:
            StoreResult with save operation result
        """
        try:
            # Get settings
            table = self.get_setting("table", "data")
            if_exists = self.get_setting("if_exists", "replace")  # replace, append, fail
            
            # Convert data to list of dicts
            if isinstance(data, dict):
                rows = [data]
            elif isinstance(data, list):
                rows = data
            else:
                # Try to convert pandas DataFrame
                try:
                    import pandas as pd
                    if isinstance(data, pd.DataFrame):
                        rows = data.to_dict('records')
                    else:
                        return StoreResult.error_result(
                            f"Unsupported data type for SQLite: {type(data).__name__}",
                            {"error_type": "TypeError"}
                        )
                except ImportError:
                    return StoreResult.error_result(
                        f"Unsupported data type for SQLite: {type(data).__name__}. "
                        "For DataFrames, install pandas",
                        {"error_type": "TypeError"}
                    )
            
            if not rows:
                return StoreResult.error_result(
                    "No data to save",
                    {"error_type": "ValueError"}
                )
            
            # Ensure directory exists
            Path(self.config.path).parent.mkdir(parents=True, exist_ok=True)
            
            # Connect and save
            with sqlite3.connect(self.config.path) as conn:
                # Convert to DataFrame for easy saving
                try:
                    import pandas as pd
                    df = pd.DataFrame(rows)
                    df.to_sql(
                        table,
                        conn,
                        if_exists=if_exists,
                        index=False
                    )
                    
                    # Get metadata
                    cursor = conn.cursor()
                    cursor.execute(f"SELECT COUNT(*) FROM {table}")
                    row_count = cursor.fetchone()[0]
                    
                    metadata = {
                        "table": table,
                        "row_count": row_count,
                        "column_count": len(df.columns),
                        "columns": list(df.columns),
                        "if_exists": if_exists,
                        "file_size": Path(self.config.path).stat().st_size
                    }
                    
                    return StoreResult.success_result(data, metadata)
                    
                except ImportError:
                    # Fallback to manual SQL if pandas not available
                    return self._save_manual_sql(conn, table, rows, if_exists)
                
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to save to SQLite: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def _save_manual_sql(self, conn: sqlite3.Connection, table: str, rows: List[Dict], if_exists: str) -> StoreResult:
        """Manual SQL-based saving when pandas is not available."""
        cursor = conn.cursor()
        
        if not rows:
            return StoreResult.error_result("No data to save", {"error_type": "ValueError"})
        
        # Get columns from first row
        columns = list(rows[0].keys())
        
        # Create table if needed
        if if_exists == "replace":
            cursor.execute(f"DROP TABLE IF EXISTS {table}")
        
        if if_exists in ["replace", "fail"]:
            # Create table
            column_defs = ", ".join([f"{col} TEXT" for col in columns])
            cursor.execute(f"CREATE TABLE IF NOT EXISTS {table} ({column_defs})")
        
        # Insert data
        placeholders = ", ".join(["?" for _ in columns])
        insert_sql = f"INSERT INTO {table} ({', '.join(columns)}) VALUES ({placeholders})"
        
        for row in rows:
            values = [row.get(col, None) for col in columns]
            cursor.execute(insert_sql, values)
        
        conn.commit()
        
        metadata = {
            "table": table,
            "row_count": len(rows),
            "column_count": len(columns),
            "columns": columns,
            "if_exists": if_exists,
            "file_size": Path(self.config.path).stat().st_size
        }
        
        return StoreResult.success_result(rows, metadata)

    def execute_query(self, query: str, params: Optional[tuple] = None) -> StoreResult:
        """Execute a custom SQL query."""
        try:
            with sqlite3.connect(self.config.path) as conn:
                conn.row_factory = sqlite3.Row
                cursor = conn.cursor()
                
                if params:
                    cursor.execute(query, params)
                else:
                    cursor.execute(query)
                
                # Handle different query types
                if query.strip().upper().startswith(('SELECT', 'PRAGMA')):
                    rows = cursor.fetchall()
                    columns = [description[0] for description in cursor.description] if cursor.description else []
                    data = [dict(row) for row in rows]
                    
                    metadata = {
                        "row_count": len(data),
                        "column_count": len(columns),
                        "columns": columns,
                        "query": query
                    }
                    
                    return StoreResult.success_result(data, metadata)
                else:
                    # INSERT, UPDATE, DELETE, etc.
                    conn.commit()
                    metadata = {
                        "rows_affected": cursor.rowcount,
                        "query": query
                    }
                    
                    return StoreResult.success_result(None, metadata)
                    
        except Exception as e:
            return StoreResult.error_result(
                f"Query execution failed: {str(e)}",
                {"error_type": type(e).__name__, "query": query}
            )

    def get_tables(self) -> List[str]:
        """Get list of tables in the database."""
        try:
            with sqlite3.connect(self.config.path) as conn:
                cursor = conn.cursor()
                cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
                return [row[0] for row in cursor.fetchall()]
        except Exception:
            return [] 