"""
AutoTel Configuration Management using Confz v2
Handles hierarchical configuration with clear precedence rules.
"""

from typing import Optional, Dict, Any, List
from pathlib import Path
import os
from confz import BaseConfig, FileSource, EnvSource, DataSource
from pydantic import Field, validator


class DSPyModelConfig(BaseConfig):
    """Configuration for a specific DSPy model"""
    provider: str = Field(..., description="Model provider (ollama, openai, anthropic, etc.)")
    model: str = Field(..., description="Model identifier")
    temperature: float = Field(0.7, description="Model temperature (0.0-1.0)")
    max_tokens: int = Field(1000, description="Maximum tokens for responses")
    timeout: int = Field(30, description="Request timeout in seconds")
    
    @validator('temperature')
    def validate_temperature(cls, v):
        if not 0.0 <= v <= 1.0:
            raise ValueError('Temperature must be between 0.0 and 1.0')
        return v
    
    @validator('max_tokens')
    def validate_max_tokens(cls, v):
        if v <= 0:
            raise ValueError('Max tokens must be positive')
        return v


class DSPyConfig(BaseConfig):
    """DSPy-specific configuration"""
    default_model: str = Field("ollama/qwen2.5:latest", description="Default model for all signatures")
    default_temperature: float = Field(0.7, description="Default temperature")
    default_max_tokens: int = Field(1000, description="Default max tokens")
    default_timeout: int = Field(30, description="Default timeout")
    
    # Model definitions
    models: Dict[str, DSPyModelConfig] = Field(default_factory=dict, description="Available models")
    
    # Signature-specific overrides
    signatures: Dict[str, Dict[str, Any]] = Field(default_factory=dict, description="Signature-specific settings")


class WorkflowConfig(BaseConfig):
    """Workflow execution configuration"""
    validation: Dict[str, bool] = Field(
        default_factory=lambda: {
            "strict": True,
            "check_signatures": True,
            "check_dmn": True
        },
        description="Validation settings"
    )
    
    execution: Dict[str, Any] = Field(
        default_factory=lambda: {
            "timeout": 300,
            "retry_attempts": 3,
            "save_intermediate_results": True
        },
        description="Execution settings"
    )


class LoggingConfig(BaseConfig):
    """Logging configuration"""
    level: str = Field("INFO", description="Log level")
    format: str = Field("json", description="Log format")
    include_timestamps: bool = Field(True, description="Include timestamps in logs")
    include_workflow_id: bool = Field(True, description="Include workflow ID in logs")


class OutputConfig(BaseConfig):
    """Output configuration"""
    format: str = Field("json", description="Output format")
    include_debug: bool = Field(False, description="Include debug information")
    include_execution_time: bool = Field(True, description="Include execution time")
    include_model_usage: bool = Field(True, description="Include model usage statistics")


class EnvironmentConfig(BaseConfig):
    """Environment-specific configuration"""
    production: Dict[str, Any] = Field(default_factory=dict, description="Production settings")
    development: Dict[str, Any] = Field(default_factory=dict, description="Development settings")
    staging: Dict[str, Any] = Field(default_factory=dict, description="Staging settings")


class AutoTelConfig(BaseConfig):
    """Main AutoTel configuration class with ConfZ v2"""
    
    # Core configuration sections
    dspy: DSPyConfig = Field(default_factory=DSPyConfig, description="DSPy configuration")
    workflow: WorkflowConfig = Field(default_factory=WorkflowConfig, description="Workflow configuration")
    logging: LoggingConfig = Field(default_factory=LoggingConfig, description="Logging configuration")
    output: OutputConfig = Field(default_factory=OutputConfig, description="Output configuration")
    environments: EnvironmentConfig = Field(default_factory=EnvironmentConfig, description="Environment-specific settings")
    
    # CLI overrides (highest priority)
    cli_model: Optional[str] = Field(None, description="CLI model override")
    cli_temperature: Optional[float] = Field(None, description="CLI temperature override")
    cli_max_tokens: Optional[int] = Field(None, description="CLI max tokens override")
    cli_timeout: Optional[int] = Field(None, description="CLI timeout override")
    
    # XML embedded settings (medium priority)
    xml_settings: Dict[str, Any] = Field(default_factory=dict, description="Settings from XML")


# Main AutoTel configuration with ConfZ v2


def create_config(
    config_file: Optional[str] = None,
    cli_overrides: Optional[Dict[str, Any]] = None,
    xml_settings: Optional[Dict[str, Any]] = None,
    environment: Optional[str] = None
) -> AutoTelConfig:
    """
    Create AutoTel configuration with hierarchical precedence:
    1. CLI overrides (highest)
    2. XML embedded settings
    3. Config file settings
    4. Environment variables
    5. Defaults (lowest)
    """
    
    sources = []
    
    # 1. Environment variables (lowest priority)
    sources.append(EnvSource(prefix="AUTOTEL_"))
    
    # 2. Config file (if provided)
    if config_file and Path(config_file).exists():
        sources.append(FileSource(file=config_file))
    
    # 3. XML embedded settings (medium priority)
    if xml_settings:
        sources.append(DataSource(data=xml_settings))
    
    # 4. CLI overrides (highest priority)
    if cli_overrides:
        sources.append(DataSource(data=cli_overrides))
    
    # Create configuration with ConfZ v2
    config = AutoTelConfig(config_sources=sources)
    
    # Apply environment-specific settings
    if environment and hasattr(config.environments, environment):
        env_config = getattr(config.environments, environment)
        if env_config:
            _apply_environment_config(config, env_config)
    
    return config


def _apply_environment_config(config: AutoTelConfig, env_config: Dict[str, Any]):
    """Apply environment-specific configuration overrides"""
    if 'dspy' in env_config:
        dspy_config = env_config['dspy']
        if 'default_model' in dspy_config:
            config.dspy.default_model = dspy_config['default_model']
        if 'default_temperature' in dspy_config:
            config.dspy.default_temperature = dspy_config['default_temperature']


def get_signature_config(config: AutoTelConfig, signature_name: str) -> Dict[str, Any]:
    """Get configuration for a specific signature with precedence rules"""
    
    # Start with defaults
    sig_config = {
        'model': config.dspy.default_model,
        'temperature': config.dspy.default_temperature,
        'max_tokens': config.dspy.default_max_tokens,
        'timeout': config.dspy.default_timeout
    }
    
    # Apply signature-specific settings from config file
    if signature_name in config.dspy.signatures:
        sig_config.update(config.dspy.signatures[signature_name])
    
    # Apply XML embedded settings (if available)
    if signature_name in config.xml_settings:
        sig_config.update(config.xml_settings[signature_name])
    
    # Apply CLI overrides (highest priority)
    if config.cli_model:
        sig_config['model'] = config.cli_model
    if config.cli_temperature:
        sig_config['temperature'] = config.cli_temperature
    if config.cli_max_tokens:
        sig_config['max_tokens'] = config.cli_max_tokens
    if config.cli_timeout:
        sig_config['timeout'] = config.cli_timeout
    
    return sig_config


def get_model_config(config: AutoTelConfig, model_name: str) -> Optional[DSPyModelConfig]:
    """Get configuration for a specific model"""
    return config.dspy.models.get(model_name)


def show_config(config: AutoTelConfig, signature_name: Optional[str] = None) -> str:
    """Show current configuration with precedence information"""
    
    lines = ["AutoTel Configuration:"]
    lines.append("=" * 50)
    
    if signature_name:
        sig_config = get_signature_config(config, signature_name)
        lines.append(f"Signature: {signature_name}")
        lines.append(f"  Model: {sig_config['model']}")
        lines.append(f"  Temperature: {sig_config['temperature']}")
        lines.append(f"  Max Tokens: {sig_config['max_tokens']}")
        lines.append(f"  Timeout: {sig_config['timeout']}")
    else:
        lines.append(f"Default Model: {config.dspy.default_model}")
        lines.append(f"Default Temperature: {config.dspy.default_temperature}")
        lines.append(f"Default Max Tokens: {config.dspy.default_max_tokens}")
        lines.append(f"Default Timeout: {config.dspy.default_timeout}")
    
    if config.cli_model or config.cli_temperature or config.cli_max_tokens or config.cli_timeout:
        lines.append("\nCLI Overrides:")
        if config.cli_model:
            lines.append(f"  model: {config.cli_model}")
        if config.cli_temperature:
            lines.append(f"  temperature: {config.cli_temperature}")
        if config.cli_max_tokens:
            lines.append(f"  max_tokens: {config.cli_max_tokens}")
        if config.cli_timeout:
            lines.append(f"  timeout: {config.cli_timeout}")
    
    if config.xml_settings:
        lines.append("\nXML Settings:")
        for key, value in config.xml_settings.items():
            lines.append(f"  {key}: {value}")
    
    return "\n".join(lines)


def validate_config(config: AutoTelConfig) -> List[str]:
    """Validate current configuration and return any errors"""
    errors = []
    
    # Validate DSPy configuration
    try:
        # Check if default model exists in models
        if (config.dspy.default_model not in config.dspy.models and 
            not config.dspy.default_model.startswith(('ollama/', 'openai/', 'anthropic/'))):
            errors.append(f"Default model '{config.dspy.default_model}' not found in models")
        
        # Validate signature configurations
        for sig_name, sig_config in config.dspy.signatures.items():
            if 'model' in sig_config and sig_config['model'] not in config.dspy.models:
                errors.append(f"Model '{sig_config['model']}' for signature '{sig_name}' not found")
            
            if 'temperature' in sig_config and not 0.0 <= sig_config['temperature'] <= 1.0:
                errors.append(f"Invalid temperature for signature '{sig_name}': {sig_config['temperature']}")
    
    except Exception as e:
        errors.append(f"Configuration validation error: {str(e)}")
    
    return errors


# Convenience functions for backward compatibility
def load_config(
    config_file: Optional[str] = None,
    cli_overrides: Optional[Dict[str, Any]] = None,
    xml_settings: Optional[Dict[str, Any]] = None,
    environment: Optional[str] = None
) -> AutoTelConfig:
    """Load configuration with hierarchical precedence (alias for create_config)"""
    return create_config(
        config_file=config_file,
        cli_overrides=cli_overrides,
        xml_settings=xml_settings,
        environment=environment
    )


# Global configuration instance for singleton pattern
_global_config: Optional[AutoTelConfig] = None


def get_config() -> AutoTelConfig:
    """Get the current configuration (singleton pattern)"""
    global _global_config
    
    if _global_config is None:
        # Create default configuration if none exists
        _global_config = create_config()
    
    return _global_config


def set_global_config(config: AutoTelConfig) -> None:
    """Set the global configuration instance"""
    global _global_config
    _global_config = config


def reset_global_config() -> None:
    """Reset the global configuration instance (useful for testing)"""
    global _global_config
    _global_config = None 