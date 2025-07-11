#!/usr/bin/env python3
"""
Configurable Spiff Verification
Tests actual functionality based on external config
"""

import yaml
import importlib
import sys
from pathlib import Path

def load_config():
    config_path = Path(__file__).parent / "spiff_check_config.yaml"
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

def test_import(module_path):
    """Test if a module can be imported"""
    try:
        return importlib.import_module(module_path)
    except ImportError:
        return None

def test_instantiation(class_path):
    """Test if a class can be instantiated"""
    try:
        module_path, class_name = class_path.rsplit('.', 1)
        module = importlib.import_module(module_path)
        cls = getattr(module, class_name)
        instance = cls()
        return True
    except Exception:
        return False

def main():
    config = load_config()
    
    # Print header
    output = config['output']
    separator = output['separator'] * output['separator_length']
    print(output['header'])
    print(separator)
    
    messages = config['messages']
    failed_modules = []
    
    for module_config in config['modules_to_check']:
        name = module_config['name']
        
        # Test main import
        main_module = test_import(module_config['import_path'])
        if not main_module:
            print(messages['not_found'].format(name=name))
            if 'SpiffWorkflow' in module_config['import_path']:
                print(messages['install_hint'])
            failed_modules.append(name)
            continue
        
        # Show version if available
        version_info = ""
        if 'version_attr' in module_config and hasattr(main_module, module_config['version_attr']):
            version = getattr(main_module, module_config['version_attr'])
            version_info = f"\n   Version: {version}"
        
        # Test additional imports
        if 'test_imports' in module_config:
            for import_path in module_config['test_imports']:
                if not test_import(import_path):
                    print(messages['error'].format(name=name, error=f"Import {import_path} failed"))
                    failed_modules.append(name)
                    continue
        
        # Test class instantiation
        if 'test_instantiation' in module_config:
            if not test_instantiation(module_config['test_instantiation']):
                print(messages['error'].format(name=name, error="Instantiation failed"))
                failed_modules.append(name)
                continue
        
        # Test class availability
        if 'test_classes' in module_config:
            for class_path in module_config['test_classes']:
                try:
                    module_path, class_name = class_path.rsplit('.', 1)
                    module = importlib.import_module(module_path)
                    if not hasattr(module, class_name):
                        raise AttributeError(f"Class {class_name} not found")
                except Exception as e:
                    print(messages['error'].format(name=name, error=f"Class test failed: {e}"))
                    failed_modules.append(name)
                    continue
        
        # If we get here, all tests passed
        print(messages['success'].format(name=name) + version_info)
    
    print(f"\nVerification complete: {len(failed_modules)} failures")
    
    if failed_modules:
        print(f"Failed modules: {', '.join(failed_modules)}")
        sys.exit(1)
    else:
        print("All capabilities verified working.")
        sys.exit(0)

if __name__ == "__main__":
    main()
