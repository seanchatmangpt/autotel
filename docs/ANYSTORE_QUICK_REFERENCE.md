# AnyStore Quick Reference

## Overview

AnyStore provides a universal interface for loading and saving data in any supported file format. It automatically delegates to the appropriate format-specific store based on file extension.

## Quick Start

```python
from autotel.stores.any_store import AnyStore
from autotel.stores.base import StoreConfig

# Create store configuration
config = StoreConfig(name="data", path="data.json")
store = AnyStore(config)

# Save data
result = store.save({"key": "value"})

# Load data
result = store.load()
print(result.data)  # {"key": "value"}
```

## Supported Formats

| Format | Extension | Store Class | Use Case |
|--------|-----------|-------------|----------|
| JSON | `.json` | `JSONStore` | Configuration, APIs, data exchange |
| YAML | `.yaml`, `.yml` | `YAMLStore` | Configuration, human-readable data |
| XML | `.xml` | `XMLStore` | Structured data, documents |
| Pickle | `.pickle`, `.pkl` | `PickleStore` | Python objects, serialization |
| Parquet | `.parquet` | `ParquetStore` | Analytics, large datasets |
| SQLite | `.db`, `.sqlite` | `SQLiteStore` | Relational data, queries |

## Configuration

### Basic Configuration

```python
config = StoreConfig(
    name="my_store",
    path="data.json",
    settings={
        "indent": 2,
        "encoding": "utf-8"
    }
)
```

### Format-Specific Settings

#### JSON
```python
settings = {
    "indent": 4,
    "sort_keys": True,
    "ensure_ascii": False
}
```

#### YAML
```python
settings = {
    "default_flow_style": False,
    "indent": 4
}
```

#### XML
```python
settings = {
    "indent": 2,
    "encoding": "utf-8"
}
```

#### Parquet
```python
settings = {
    "engine": "pyarrow",
    "compression": "snappy",
    "index": False
}
```

#### SQLite
```python
settings = {
    "table": "my_table",
    "if_exists": "replace"
}
```

## API Reference

### AnyStore Methods

#### `load() -> StoreResult`
Load data from the store.

```python
result = store.load()
if result.success:
    data = result.data
    metadata = result.metadata
else:
    error = result.error
```

#### `save(data: Any) -> StoreResult`
Save data to the store.

```python
result = store.save({"key": "value"})
if result.success:
    metadata = result.metadata
else:
    error = result.error
```

#### `get_supported_extensions() -> List[str]`
Get list of supported file extensions.

```python
extensions = store.get_supported_extensions()
# ['.json', '.yaml', '.yml', '.xml', '.pickle', '.pkl', '.parquet', '.db', '.sqlite']
```

#### `get_delegated_store_info() -> Dict[str, str]`
Get information about which store will be delegated to.

```python
info = store.get_delegated_store_info()
# {'extension': '.json', 'store_class': 'JSONStore', 'store_module': 'autotel.stores.json_store'}
```

### StoreResult Properties

```python
result = store.load()

# Success status
result.success  # bool

# Data (if successful)
result.data  # Any

# Error message (if failed)
result.error  # str

# Metadata
result.metadata  # Dict[str, Any]
```

## Error Handling

### Unsupported Format
```python
config = StoreConfig(name="test", path="data.unsupported")
store = AnyStore(config)
result = store.load()
# result.success = False
# result.error = "Unsupported file extension '.unsupported'"
```

### Missing File
```python
config = StoreConfig(name="test", path="nonexistent.json")
store = AnyStore(config)
result = store.load()
# result.success = False
# result.error = "File not found: nonexistent.json"
```

### No Extension
```python
config = StoreConfig(name="test", path="datafile")
store = AnyStore(config)
result = store.load()
# result.success = False
# result.error = "No file extension found in path: datafile"
```

## Custom Store Registration

### Register Custom Format
```python
from autotel.stores.any_store import register_store_extension

class CustomStore(BaseStore):
    def _load_impl(self) -> StoreResult:
        return StoreResult.success_result({"custom": "data"})
    
    def _save_impl(self, data: Any) -> StoreResult:
        return StoreResult.success_result(data)

# Register custom extension
register_store_extension(".custom", CustomStore)

# Use custom store
config = StoreConfig(name="test", path="data.custom")
store = AnyStore(config)
result = store.load()  # Delegates to CustomStore
```

## Integration Examples

### With AutoTel Pipeline
```python
# Load input data
input_config = StoreConfig(name="input", path="input.yaml")
input_store = AnyStore(input_config)
data = input_store.load().data

# Process data...

# Save results
output_config = StoreConfig(name="output", path="results.json")
output_store = AnyStore(output_config)
output_store.save(processed_data)
```

### With CLI Commands
```python
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

## Troubleshooting

### Common Issues

#### Import Errors
```python
# Ensure stores are imported
from autotel.stores import AnyStore, StoreConfig
```

#### Missing Dependencies
```python
# For Parquet support
pip install pandas pyarrow

# For YAML support
pip install pyyaml
```

#### Permission Errors
```python
# Check file permissions
import os
os.access("data.json", os.R_OK)  # Read permission
os.access("data.json", os.W_OK)  # Write permission
```

### Debug Information
```python
# Get store information
info = store.get_delegated_store_info()
print(f"Extension: {info['extension']}")
print(f"Store Class: {info['store_class']}")

# Get supported extensions
extensions = store.get_supported_extensions()
print(f"Supported: {extensions}")

# Check metadata
metadata = store.get_metadata()
print(f"Store metadata: {metadata}")
```

## Related Documentation

- [ANYSTORE_IMPLEMENTATION.md](./ANYSTORE_IMPLEMENTATION.md) - Complete implementation guide
- [DATA-PERSISTENCE-PLAN.md](./DATA-PERSISTENCE-PLAN.md) - Data persistence architecture
- [examples/any_store_usage.py](../examples/any_store_usage.py) - Usage examples
- [tests/test_any_store.py](../tests/test_any_store.py) - Test suite 