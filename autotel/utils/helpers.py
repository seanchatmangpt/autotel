"""
AutoTel Helper Functions
"""

import json
import yaml
from typing import Dict, Any, List, Optional, Union, Callable, TypeVar, cast
from pathlib import Path
from datetime import datetime
from autotel.core.telemetry import TelemetryManager
from functools import wraps

T = TypeVar('T', bound=Callable[..., Any])

def load_yaml_file(file_path: str) -> Dict[str, Any]:
    """Load a YAML file safely"""
    try:
        with open(file_path, 'r') as f:
            return yaml.safe_load(f)
    except Exception as e:
        raise ValueError(f"Failed to load YAML file {file_path}: {e}")

def save_yaml_file(data: Dict[str, Any], file_path: str) -> None:
    """Save data to a YAML file"""
    try:
        with open(file_path, 'w') as f:
            yaml.dump(data, f, default_flow_style=False, indent=2)
    except Exception as e:
        raise ValueError(f"Failed to save YAML file {file_path}: {e}")

def load_json_file(file_path: str) -> Dict[str, Any]:
    """Load a JSON file safely"""
    try:
        with open(file_path, 'r') as f:
            return json.load(f)
    except Exception as e:
        raise ValueError(f"Failed to load JSON file {file_path}: {e}")

def save_json_file(data: Dict[str, Any], file_path: str, indent: int = 2) -> None:
    """Save data to a JSON file"""
    try:
        with open(file_path, 'w') as f:
            json.dump(data, f, indent=indent, default=str)
    except Exception as e:
        raise ValueError(f"Failed to save JSON file {file_path}: {e}")

def format_duration(seconds: float) -> str:
    """Format duration in seconds to human readable string"""
    if seconds < 60:
        return f"{seconds:.2f}s"
    elif seconds < 3600:
        minutes = seconds / 60
        return f"{minutes:.1f}m"
    else:
        hours = seconds / 3600
        return f"{hours:.1f}h"

def generate_id(prefix: str = "id") -> str:
    """Generate a unique ID with prefix"""
    import uuid
    return f"{prefix}_{uuid.uuid4().hex[:8]}"

def validate_required_fields(data: Dict[str, Any], required_fields: List[str]) -> List[str]:
    """Validate that required fields are present in data"""
    missing_fields = []
    for field in required_fields:
        if field not in data or data[field] is None:
            missing_fields.append(field)
    return missing_fields

def deep_merge(dict1: Dict[str, Any], dict2: Dict[str, Any]) -> Dict[str, Any]:
    """Deep merge two dictionaries"""
    result = dict1.copy()
    for key, value in dict2.items():
        if key in result and isinstance(result[key], dict) and isinstance(value, dict):
            result[key] = deep_merge(result[key], value)
        else:
            result[key] = value
    return result

def sanitize_filename(filename: str) -> str:
    """Sanitize a filename for safe file system usage"""
    import re
    # Remove or replace unsafe characters
    sanitized = re.sub(r'[<>:"/\\|?*]', '_', filename)
    # Remove leading/trailing spaces and dots
    sanitized = sanitized.strip(' .')
    # Ensure it's not empty
    if not sanitized:
        sanitized = "unnamed_file"
    return sanitized

def ensure_directory(path: str) -> None:
    """Ensure a directory exists, create if it doesn't"""
    Path(path).mkdir(parents=True, exist_ok=True)

def get_file_extension(file_path: str) -> str:
    """Get file extension from path"""
    return Path(file_path).suffix.lower()

def is_valid_json(data: str) -> bool:
    """Check if a string is valid JSON"""
    try:
        json.loads(data)
        return True
    except (json.JSONDecodeError, TypeError):
        return False

def is_valid_yaml(data: str) -> bool:
    """Check if a string is valid YAML"""
    try:
        yaml.safe_load(data)
        return True
    except (yaml.YAMLError, TypeError):
        return False

def timestamp_to_datetime(timestamp: Union[str, float, int]) -> datetime:
    """Convert timestamp to datetime object"""
    if isinstance(timestamp, str):
        return datetime.fromisoformat(timestamp.replace('Z', '+00:00'))
    elif isinstance(timestamp, (int, float)):
        return datetime.fromtimestamp(timestamp)
    else:
        raise ValueError(f"Unsupported timestamp type: {type(timestamp)}")

def datetime_to_timestamp(dt: datetime) -> float:
    """Convert datetime object to timestamp"""
    return dt.timestamp()

def otel_command(func: T) -> T:
    """
    Decorator to wrap a Typer command in an OpenTelemetry span with LinkML schema validation.
    - All arguments are added as span attributes.
    - Adds module, function, and file_path for file system routing metadata.
    - Exceptions are recorded as span events and re-raised.
    - TelemetryManager is always instantiated with LinkML validation enabled.
    """
    from autotel.core.telemetry import TelemetryConfig
    import inspect
    @wraps(func)
    def wrapper(*args, **kwargs):
        telemetry_manager = TelemetryManager(TelemetryConfig(require_linkml_validation=True))
        sig = inspect.signature(func)
        bound = sig.bind(*args, **kwargs)
        bound.apply_defaults()
        span_attrs = dict(bound.arguments)
        # Add file system routing metadata
        module = func.__module__
        function = func.__name__
        file_path = inspect.getfile(func)
        span_attrs.update({
            'module': module,
            'function': function,
            'file_path': file_path
        })
        try:
            with telemetry_manager.start_span(
                name=function,
                operation_type="class_analysis",  # Use a valid enum value or look up dynamically
                **span_attrs
            ):
                return func(*args, **kwargs)
        except Exception as e:
            raise
    return cast(T, wrapper) 