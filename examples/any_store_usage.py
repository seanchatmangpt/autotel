"""
AutoTel AnyStore Usage Example

Demonstrates the universal store pattern for loading/saving any supported file format.
"""

import tempfile
import os
from pathlib import Path

from autotel.stores.any_store import AnyStore
from autotel.stores.base import StoreConfig


def demonstrate_any_store():
    """Demonstrate AnyStore capabilities with various file formats."""
    
    print("🚀 AutoTel AnyStore Demo")
    print("=" * 50)
    
    # Test data for different formats
    test_data = {
        "users": [
            {"id": 1, "name": "Alice", "email": "alice@example.com"},
            {"id": 2, "name": "Bob", "email": "bob@example.com"},
            {"id": 3, "name": "Charlie", "email": "charlie@example.com"}
        ],
        "settings": {
            "debug": True,
            "timeout": 30,
            "features": ["auth", "logging", "telemetry"]
        }
    }
    
    # Test different file formats
    formats = [
        (".json", "JSON format"),
        (".yaml", "YAML format"),
        (".yml", "YAML format (short extension)"),
        (".xml", "XML format"),
    ]
    
    for extension, description in formats:
        print(f"\n📁 Testing {description} ({extension})")
        print("-" * 30)
        
        with tempfile.NamedTemporaryFile(suffix=extension, delete=False) as f:
            temp_path = f.name
        
        try:
            # Create store configuration
            config = StoreConfig(
                name=f"demo_{extension[1:]}",  # Remove dot
                path=temp_path,
                settings={
                    "indent": 2,
                    "encoding": "utf-8"
                }
            )
            
            # Create AnyStore instance
            store = AnyStore(config)
            
            # Show which store will be delegated to
            info = store.get_delegated_store_info()
            print(f"  Delegated to: {info['store_class']}")
            
            # Save data
            save_result = store.save(test_data)
            if save_result.success:
                print(f"  ✅ Saved successfully")
                print(f"  📊 Metadata: {save_result.metadata}")
            else:
                print(f"  ❌ Save failed: {save_result.error}")
                continue
            
            # Load data back
            load_result = store.load()
            if load_result.success:
                print(f"  ✅ Loaded successfully")
                print(f"  📊 Metadata: {load_result.metadata}")
                
                # Verify data integrity
                if load_result.data == test_data:
                    print(f"  ✅ Data integrity verified")
                else:
                    print(f"  ⚠️  Data integrity check failed")
            else:
                print(f"  ❌ Load failed: {load_result.error}")
                
        finally:
            # Cleanup
            if os.path.exists(temp_path):
                os.unlink(temp_path)
    
    print(f"\n🎯 AnyStore Capabilities")
    print("-" * 30)
    
    # Show supported extensions
    config = StoreConfig(name="demo", path="demo.json")
    store = AnyStore(config)
    
    extensions = store.get_supported_extensions()
    print(f"  Supported extensions: {', '.join(sorted(extensions))}")
    
    # Show metadata
    metadata = store.get_metadata()
    print(f"  Store name: {metadata['name']}")
    print(f"  Version: {metadata['version']}")
    print(f"  Capabilities: {', '.join(metadata['capabilities'])}")
    print(f"  Enterprise ready: {metadata['enterprise_ready']}")


def demonstrate_error_handling():
    """Demonstrate error handling with AnyStore."""
    
    print(f"\n🛡️  Error Handling Demo")
    print("=" * 50)
    
    # Test unsupported extension
    config = StoreConfig(name="test", path="test.unsupported")
    store = AnyStore(config)
    
    result = store.load()
    print(f"  Unsupported extension: {result.error}")
    
    # Test missing file
    config = StoreConfig(name="test", path="nonexistent.json")
    store = AnyStore(config)
    
    result = store.load()
    print(f"  Missing file: {result.error}")
    
    # Test no extension
    config = StoreConfig(name="test", path="testfile")
    store = AnyStore(config)
    
    result = store.load()
    print(f"  No extension: {result.error}")


def demonstrate_custom_registration():
    """Demonstrate custom store registration."""
    
    print(f"\n🔧 Custom Registration Demo")
    print("=" * 50)
    
    # Create a custom store class
    class CustomStore:
        def __init__(self, config):
            self.config = config
        
        def load(self):
            from autotel.stores.base import StoreResult
            return StoreResult.success_result({"custom": "data", "source": "CustomStore"})
        
        def save(self, data):
            from autotel.stores.base import StoreResult
            return StoreResult.success_result(data, {"custom_store": True})
    
    # Register custom extension
    from autotel.stores.any_store import register_store_extension
    register_store_extension(".custom", CustomStore)
    
    # Test custom store
    config = StoreConfig(name="test", path="test.custom")
    store = AnyStore(config)
    
    result = store.load()
    if result.success:
        print(f"  ✅ Custom store loaded: {result.data}")
        print(f"  📊 Metadata: {result.metadata}")


def demonstrate_settings_passthrough():
    """Demonstrate settings passthrough to delegated stores."""
    
    print(f"\n⚙️  Settings Passthrough Demo")
    print("=" * 50)
    
    with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as f:
        temp_path = f.name
    
    try:
        # Test with custom settings
        config = StoreConfig(
            name="test",
            path=temp_path,
            settings={
                "indent": 4,  # 4-space indentation
                "encoding": "utf-8",
                "sort_keys": True
            }
        )
        
        store = AnyStore(config)
        test_data = {"z": 3, "a": 1, "m": 2}  # Unsorted keys
        
        result = store.save(test_data)
        if result.success:
            print(f"  ✅ Saved with custom settings")
            
            # Verify settings were applied
            with open(temp_path, 'r') as f:
                content = f.read()
                print(f"  📄 File content preview:")
                print(f"     {content[:100]}...")
                
                # Check for 4-space indentation
                if '    "a"' in content:
                    print(f"  ✅ 4-space indentation applied")
                else:
                    print(f"  ⚠️  Indentation not as expected")
                    
    finally:
        if os.path.exists(temp_path):
            os.unlink(temp_path)


if __name__ == "__main__":
    # Run all demonstrations
    demonstrate_any_store()
    demonstrate_error_handling()
    demonstrate_custom_registration()
    demonstrate_settings_passthrough()
    
    print(f"\n🎉 AnyStore Demo Complete!")
    print("=" * 50)
    print("The AnyStore provides a unified interface for all supported file formats.")
    print("It automatically delegates to the appropriate format-specific store based on file extension.")
    print("This enables seamless data loading/saving without worrying about format-specific APIs.") 