"""
AutoTel Persistence/State Helper

Provides utilities for saving/loading state in JSON, YAML, and pickle formats.
Happy path only, with type hints and docstrings.
"""
from typing import Any, Dict, Optional
from pathlib import Path
import json
import pickle

try:
    import yaml
except ImportError:
    yaml = None

def save_json(data: Any, file_path: str) -> None:
    """
    Save data as JSON to a file.
    Args:
        data: Data to save
        file_path: Path to file
    """
    with open(file_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)

def load_json(file_path: str) -> Any:
    """
    Load data from a JSON file.
    Args:
        file_path: Path to file
    Returns:
        Loaded data
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def save_yaml(data: Any, file_path: str) -> None:
    """
    Save data as YAML to a file.
    Args:
        data: Data to save
        file_path: Path to file
    """
    if yaml is None:
        raise ImportError("PyYAML is required for YAML support.")
    with open(file_path, 'w', encoding='utf-8') as f:
        yaml.safe_dump(data, f)

def load_yaml(file_path: str) -> Any:
    """
    Load data from a YAML file.
    Args:
        file_path: Path to file
    Returns:
        Loaded data
    """
    if yaml is None:
        raise ImportError("PyYAML is required for YAML support.")
    with open(file_path, 'r', encoding='utf-8') as f:
        return yaml.safe_load(f)

def save_pickle(data: Any, file_path: str) -> None:
    """
    Save data as a pickle file.
    Args:
        data: Data to save
        file_path: Path to file
    """
    with open(file_path, 'wb') as f:
        pickle.dump(data, f)

def load_pickle(file_path: str) -> Any:
    """
    Load data from a pickle file.
    Args:
        file_path: Path to file
    Returns:
        Loaded data
    """
    with open(file_path, 'rb') as f:
        return pickle.load(f)

def atomic_write(data: str, file_path: str) -> None:
    """
    Atomically write string data to a file (write to temp, then move).
    Args:
        data: String data to write
        file_path: Path to file
    """
    tmp_path = f"{file_path}.tmp"
    with open(tmp_path, 'w', encoding='utf-8') as f:
        f.write(data)
    Path(tmp_path).replace(file_path) 