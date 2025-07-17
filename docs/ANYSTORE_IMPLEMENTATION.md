# AutoTel AnyStore Implementation

## Overview

The `AnyStore` is a universal store implementation that provides a unified interface for loading and saving data in any supported file format. It automatically delegates to the appropriate format-specific store based on file extension, following the `BaseStore` + `StoreMeta` pattern.

## Architecture

### Core Components

1. **AnyStore** - Universal delegator that routes to format-specific stores
2. **Format-Specific Stores** - Concrete implementations for each file format
3. **Extension Registry** - Dynamic mapping of file extensions to store classes
4. **StoreMeta Integration** - Automatic registration and metadata collection

### Supported Formats

| Format | Extension | Store Class | Capabilities |
|--------|-----------|-------------|--------------|
| JSON | `.json` | `JSONStore` | Serialization, validation |
| YAML | `.yaml`, `.yml` | `YAMLStore` | Human-readable, comments |
| XML | `.xml` | `XMLStore` | Tree structure, validation |
| Pickle | `.pickle`, `.pkl` | `PickleStore` | Python objects, compression |
| Parquet | `.parquet` | `ParquetStore` | Columnar storage, compression |
| SQLite | `.db`, `.sqlite` | `SQLiteStore` | Relational, SQL queries |

## Implementation Details

### AnyStore Class

```python
@store_metadata(
    name="any_store",
    version="1.0.0",
    capabilities=["universal_loading", "format_detection", "delegation"],
    supported_formats=["json", "yaml", "yml", "pickle", "pkl", "xml", "parquet", "db", "sqlite"],
    author="AutoTel",
    enterprise_ready=True,
    cloud_supported=True
)
class AnyStore(BaseStore):
    """Universal store that delegates to specific format-based stores."""
```

### Key Features

1. **Automatic Format Detection** - Based on file extension
2. **Dynamic Delegation** - Routes to appropriate concrete store
3. **Settings Passthrough** - Configurations passed to delegated stores
4. **Error Handling** - Graceful handling of unsupported formats
5. **Metadata Enrichment** - Adds delegation information to results
6. **Extensible Registry** - Easy addition of new format support

### Extension Registration

```python
# Auto-registration of existing stores
EXTENSION_TO_STORE = {}

def register_store_extension(extension: str, store_class: Type[BaseStore]) -> None:
    """Register a store class for a specific file extension."""
    EXTENSION_TO_STORE[extension.lower()] = store_class

# Manual registration example
register_store_extension(".custom", CustomStore)
```

## Usage Examples

### Basic Usage

```python
from autotel.stores.any_store import AnyStore
from autotel.stores.base import StoreConfig

# JSON file
config = StoreConfig(name="data", path="data.json")
store = AnyStore(config)

# Save data
result = store.save({"key": "value"})
print(result.success)  # True

# Load data
result = store.load()
print(result.data)  # {"key": "value"}
```

### Format-Specific Settings

```python
# YAML with custom settings
config = StoreConfig(
    name="config",
    path="config.yaml",
    settings={
        "default_flow_style": False,
        "indent": 4
    }
)
store = AnyStore(config)
```

### Error Handling

```python
# Unsupported format
config = StoreConfig(name="test", path="data.unsupported")
store = AnyStore(config)

result = store.load()
if not result.success:
    print(f"Error: {result.error}")
    # "Unsupported file extension '.unsupported'"
```

### Custom Store Registration

```python
class CustomStore(BaseStore):
    def _load_impl(self) -> StoreResult:
        return StoreResult.success_result({"custom": "data"})
    
    def _save_impl(self, data: Any) -> StoreResult:
        return StoreResult.success_result(data)

# Register custom format
register_store_extension(".custom", CustomStore)

# Use custom store
config = StoreConfig(name="test", path="data.custom")
store = AnyStore(config)
result = store.load()  # Delegates to CustomStore
```

## Store Implementations

### JSONStore

```python
@store_metadata(
    name="json_store",
    version="1.0.0",
    capabilities=["serialization", "validation"],
    supported_formats=["json"]
)
class JSONStore(BaseStore):
    """JSON file operations with configurable formatting."""
```

**Features:**
- Configurable indentation
- Encoding support
- Sort keys option
- Error handling for malformed JSON

### YAMLStore

```python
@store_metadata(
    name="yaml_store",
    version="1.0.0",
    capabilities=["human_readable", "comments", "anchors"],
    supported_formats=["yaml", "yml"]
)
class YAMLStore(BaseStore):
    """YAML file operations with safe loading."""
```

**Features:**
- Safe loading (prevents code execution)
- Configurable flow style
- Comment preservation
- Anchor and alias support

### XMLStore

```python
@store_metadata(
    name="xml_store",
    version="1.0.0",
    capabilities=["xml_parsing", "xml_serialization", "tree_structure"],
    supported_formats=["xml"]
)
class XMLStore(BaseStore):
    """XML file operations using ElementTree."""
```

**Features:**
- ElementTree integration
- Pretty printing with indentation
- Dict/list to XML conversion
- XML validation

### ParquetStore

```python
@store_metadata(
    name="parquet_store",
    version="1.0.0",
    capabilities=["columnar_storage", "compression", "schema_preservation"],
    supported_formats=["parquet"]
)
class ParquetStore(BaseStore):
    """Parquet file operations using pandas."""
```

**Features:**
- Pandas DataFrame support
- Columnar compression
- Schema preservation
- Configurable engines (pyarrow, fastparquet)

### SQLiteStore

```python
@store_metadata(
    name="sqlite_store",
    version="1.0.0",
    capabilities=["relational_storage", "sql_queries", "transactions"],
    supported_formats=["db", "sqlite"]
)
class SQLiteStore(BaseStore):
    """SQLite database operations."""
```

**Features:**
- SQL query execution
- Table operations
- Transaction support
- Pandas integration

## Testing

### Test Coverage

The AnyStore implementation includes comprehensive tests:

1. **Basic Functionality** - Save/load operations
2. **Format Delegation** - Correct store routing
3. **Error Handling** - Unsupported formats, missing files
4. **Settings Passthrough** - Configuration propagation
5. **Custom Registration** - Extension registry functionality
6. **Metadata Validation** - Store metadata and capabilities

### Running Tests

```bash
# Run simple tests
uv run python tests/test_any_store_simple.py

# Run comprehensive tests (requires full test environment)
uv run pytest tests/test_any_store.py -v
```

## Integration

### With AutoTel Pipeline

```python
from autotel.stores.any_store import AnyStore
from autotel.stores.base import StoreConfig

# Use in data loading pipeline
config = StoreConfig(name="input", path="input.yaml")
input_store = AnyStore(config)
data = input_store.load().data

# Process data...

# Save results
output_config = StoreConfig(name="output", path="results.json")
output_store = AnyStore(config)
output_store.save(processed_data)
```

### With CLI Commands

```python
# CLI integration example
@cli.command()
def load_data(file_path: str):
    """Load data from any supported format."""
    config = StoreConfig(name="cli_load", path=file_path)
    store = AnyStore(config)
    result = store.load()
    
    if result.success:
        print(f"Loaded {len(result.data)} items")
    else:
        print(f"Error: {result.error}")
```

## Performance Characteristics

### Delegation Overhead

- **Extension Detection**: O(1) - Constant time lookup
- **Store Resolution**: O(1) - Direct class instantiation
- **Memory Usage**: Minimal - No data caching in AnyStore
- **Settings Passthrough**: O(1) - Direct configuration forwarding

### Format-Specific Performance

| Format | Load Time | Save Time | Memory Usage |
|--------|-----------|-----------|--------------|
| JSON | Fast | Fast | Low |
| YAML | Medium | Medium | Low |
| XML | Medium | Medium | Medium |
| Pickle | Fast | Fast | Low |
| Parquet | Fast | Fast | Efficient |
| SQLite | Fast | Fast | Efficient |

## Extensibility

### Adding New Formats

1. **Create Store Class**
```python
@store_metadata(name="csv_store", supported_formats=["csv"])
class CSVStore(BaseStore):
    def _load_impl(self) -> StoreResult:
        # Implementation
        pass
    
    def _save_impl(self, data: Any) -> StoreResult:
        # Implementation
        pass
```

2. **Register Extension**
```python
register_store_extension(".csv", CSVStore)
```

3. **Update AnyStore Metadata**
```python
# In AnyStore class
supported_formats=["json", "yaml", "yml", "pickle", "pkl", "xml", "parquet", "db", "sqlite", "csv"]
```

### Plugin System

The AnyStore supports a plugin system for dynamic format registration:

```python
# Plugin registration
def register_format_plugin(extension: str, store_class: Type[BaseStore], metadata: dict):
    register_store_extension(extension, store_class)
    # Update AnyStore capabilities
    AnyStore._store_metadata["supported_formats"].append(extension)
```

## Best Practices

### Configuration

1. **Use Descriptive Names** - Store names should indicate purpose
2. **Set Appropriate Settings** - Format-specific configurations
3. **Handle Errors Gracefully** - Check success status
4. **Validate Data** - Ensure data integrity

### Performance

1. **Choose Appropriate Format** - Consider use case requirements
2. **Use Settings Wisely** - Balance readability vs performance
3. **Handle Large Files** - Consider streaming for large datasets
4. **Cache When Appropriate** - Store frequently accessed data

### Security

1. **Validate Input** - Check file extensions and content
2. **Use Safe Parsers** - Prefer safe loading options
3. **Handle Paths Securely** - Validate file paths
4. **Limit Permissions** - Use appropriate file permissions

## Future Enhancements

### Planned Features

1. **Content-Based Detection** - Fallback when extension is missing
2. **Compression Support** - Automatic compression/decompression
3. **Streaming Operations** - Large file handling
4. **Cloud Storage** - S3, GCS, Azure integration
5. **Schema Validation** - Format-specific validation
6. **Caching Layer** - Performance optimization

### API Improvements

1. **Async Support** - Non-blocking operations
2. **Batch Operations** - Multiple file handling
3. **Progress Callbacks** - Operation progress tracking
4. **Retry Logic** - Automatic retry on failures

## Conclusion

The AnyStore implementation provides a robust, extensible, and user-friendly interface for data persistence across multiple formats. It follows AutoTel's architectural patterns and integrates seamlessly with the existing store infrastructure.

Key benefits:
- **Unified Interface** - Single API for all formats
- **Automatic Detection** - No manual format specification
- **Extensible Design** - Easy addition of new formats
- **Enterprise Ready** - Production-grade implementation
- **Performance Optimized** - Minimal overhead
- **Well Tested** - Comprehensive test coverage

The implementation successfully demonstrates the power of the `BaseStore` + `StoreMeta` pattern and provides a solid foundation for AutoTel's data persistence needs. 