"""
AutoTel Config Loader Helpers

Configuration loading utilities for processor, telemetry, and contract
configuration files. Supports YAML and JSON formats, with type hints
and docstrings. Focuses on happy path and integration.
"""

import json
from typing import Dict, Any, Optional, Union
from pathlib import Path

try:
    import yaml
except ImportError:
    yaml = None


def load_config_file(config_path: Union[str, Path]) -> Dict[str, Any]:
    """
    Load a configuration file (YAML or JSON) and return as a dictionary.
    
    Args:
        config_path: Path to the configuration file
        
    Returns:
        Dictionary with configuration data
        
    Raises:
        ValueError: If file extension is not supported or file cannot be parsed
    """
    config_path = Path(config_path)
    if not config_path.exists():
        raise ValueError(f"Config file not found: {config_path}")
    
    ext = config_path.suffix.lower()
    if ext in {'.yaml', '.yml'}:
        if yaml is None:
            raise ImportError("PyYAML is required to load YAML config files.")
        with open(config_path, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)
    elif ext == '.json':
        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    else:
        raise ValueError("Unsupported config file format")


def load_processor_config(processor_type: str, config_dir: Optional[Union[str, Path]] = None) -> Dict[str, Any]:
    """
    Load processor-specific configuration from config directory.
    
    Args:
        processor_type: Type of processor (bpmn, dmn, dspy, etc.)
        config_dir: Optional directory containing config files (defaults to ./config)
        
    Returns:
        Dictionary with processor configuration
        
    Example:
        >>> config = load_processor_config("bpmn")
    """
    config_dir = Path(config_dir) if config_dir else Path("config")
    config_file = config_dir / f"{processor_type}_config.yaml"
    if not config_file.exists():
        config_file = config_dir / f"{processor_type}_config.json"
    config = load_config_file(config_file)
    validate_config_schema(config, "processor")
    return config


def load_telemetry_config(config_dir: Optional[Union[str, Path]] = None) -> Dict[str, Any]:
    """
    Load telemetry configuration from config directory.
    
    Args:
        config_dir: Optional directory containing config files (defaults to ./config)
        
    Returns:
        Dictionary with telemetry configuration
        
    Example:
        >>> telemetry_config = load_telemetry_config()
    """
    config_dir = Path(config_dir) if config_dir else Path("config")
    config_file = config_dir / "telemetry_config.yaml"
    if not config_file.exists():
        config_file = config_dir / "telemetry_config.json"
    config = load_config_file(config_file)
    validate_config_schema(config, "telemetry")
    return config


def load_contract_config(config_dir: Optional[Union[str, Path]] = None) -> Dict[str, Any]:
    """
    Load contract configuration from config directory.
    
    Args:
        config_dir: Optional directory containing config files (defaults to ./config)
        
    Returns:
        Dictionary with contract configuration
        
    Example:
        >>> contract_config = load_contract_config()
    """
    config_dir = Path(config_dir) if config_dir else Path("config")
    config_file = config_dir / "contract_config.yaml"
    if not config_file.exists():
        config_file = config_dir / "contract_config.json"
    config = load_config_file(config_file)
    validate_config_schema(config, "contract")
    return config


def validate_config_schema(config: Dict[str, Any], schema_type: str) -> None:
    """
    Validate a config dictionary against a schema type (stub for happy path).
    Args:
        config: The configuration dictionary to validate
        schema_type: The type of schema (e.g., 'processor', 'telemetry', 'contract')
    """
    # Happy path: assume always valid (stub for integration with jsonschema or similar)
    pass


def merge_configs(base: Dict[str, Any], override: Dict[str, Any]) -> Dict[str, Any]:
    """
    Merge two config dictionaries, with override taking precedence.
    Args:
        base: The base config
        override: The override config
    Returns:
        Merged config dictionary
    """
    result = base.copy()
    for k, v in override.items():
        if (
            k in result and isinstance(result[k], dict) and isinstance(v, dict)
        ):
            result[k] = merge_configs(result[k], v)
        else:
            result[k] = v
    return result


def get_config_value(config: Dict[str, Any], path: str, default: Any = None) -> Any:
    """
    Retrieve a value from a config dict using dot notation path.
    Args:
        config: The config dictionary
        path: Dot-separated path (e.g., 'processors.bpmn.timeout')
        default: Default value if not found
    Returns:
        The value or default
    """
    keys = path.split('.')
    current = config
    for key in keys:
        if isinstance(current, dict) and key in current:
            current = current[key]
        else:
            return default
    return current


def set_config_value(config: Dict[str, Any], path: str, value: Any) -> None:
    """
    Set a value in a config dict using dot notation path.
    Args:
        config: The config dictionary
        path: Dot-separated path
        value: Value to set
    """
    keys = path.split('.')
    current = config
    for key in keys[:-1]:
        if key not in current or not isinstance(current[key], dict):
            current[key] = {}
        current = current[key]
    current[keys[-1]] = value


def reload_config(config_path: Union[str, Path], old_config: Dict[str, Any]) -> Dict[str, Any]:
    """
    Reload config from file, replacing old config (happy path).
    Args:
        config_path: Path to config file
        old_config: Previous config (unused in happy path)
    Returns:
        New config dictionary
    """
    new_config = load_config_file(config_path)
    # Assume validation is handled elsewhere
    return new_config 