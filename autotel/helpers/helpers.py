"""
AutoTel General Utility Helper

Provides DRY utility functions for processors and helpers.
Happy path only, with type hints and docstrings.
"""
from typing import Any, Dict, List, Optional
import inflection

def deep_merge_dicts(a: Dict[Any, Any], b: Dict[Any, Any]) -> Dict[Any, Any]:
    """
    Recursively merge two dictionaries. b takes precedence.
    Args:
        a: Base dictionary
        b: Dictionary to merge in
    Returns:
        Merged dictionary
    """
    result = a.copy()
    for k, v in b.items():
        if k in result and isinstance(result[k], dict) and isinstance(v, dict):
            result[k] = deep_merge_dicts(result[k], v)
        else:
            result[k] = v
    return result


def flatten_dict(d: Dict[Any, Any], parent_key: str = '', sep: str = '.') -> Dict[str, Any]:
    """
    Flatten a nested dictionary using dot notation.
    Args:
        d: Dictionary to flatten
        parent_key: Prefix for keys
        sep: Separator
    Returns:
        Flattened dictionary
    """
    items = {}
    for k, v in d.items():
        new_key = f"{parent_key}{sep}{k}" if parent_key else k
        if isinstance(v, dict):
            items.update(flatten_dict(v, new_key, sep=sep))
        else:
            items[new_key] = v
    return items


def unflatten_dict(d: Dict[str, Any], sep: str = '.') -> Dict[str, Any]:
    """
    Unflatten a dot-notation dictionary into a nested dictionary.
    Args:
        d: Flattened dictionary
        sep: Separator
    Returns:
        Nested dictionary
    """
    result: Dict[str, Any] = {}
    for k, v in d.items():
        keys = k.split(sep)
        current = result
        for part in keys[:-1]:
            if part not in current or not isinstance(current[part], dict):
                current[part] = {}
            current = current[part]
        current[keys[-1]] = v
    return result


def safe_get(d: Dict[Any, Any], path: str, sep: str = '.') -> Optional[Any]:
    """
    Safely get a value from a nested dictionary using dot notation.
    Args:
        d: Dictionary
        path: Dot-separated path
        sep: Separator
    Returns:
        Value or None
    """
    keys = path.split(sep)
    current = d
    for key in keys:
        if isinstance(current, dict) and key in current:
            current = current[key]
        else:
            return None
    return current


def safe_set(d: Dict[Any, Any], path: str, value: Any, sep: str = '.') -> None:
    """
    Safely set a value in a nested dictionary using dot notation.
    Args:
        d: Dictionary
        path: Dot-separated path
        value: Value to set
        sep: Separator
    """
    keys = path.split(sep)
    current = d
    for key in keys[:-1]:
        if key not in current or not isinstance(current[key], dict):
            current[key] = {}
        current = current[key]
    current[keys[-1]] = value


def to_snake_case(s: str) -> str:
    """
    Convert a string to snake_case using inflection.
    Args:
        s: Input string
    Returns:
        snake_case string
    """
    return inflection.underscore(s)


def to_camel_case(s: str) -> str:
    """
    Convert a string to camelCase using inflection.
    Args:
        s: Input string
    Returns:
        camelCase string
    """
    return inflection.camelize(s, uppercase_first_letter=False) 