#!/usr/bin/env python3
"""
Example demonstrating AutoTel configuration using Confz v2
Shows hierarchical configuration with clear precedence rules.
"""

import os
import tempfile
from pathlib import Path
from autotel.config import create_config, get_signature_config, show_config, validate_config


def example_1_basic_config():
    """Example 1: Basic configuration with defaults"""
    print("=" * 60)
    print("Example 1: Basic Configuration with Defaults")
    print("=" * 60)
    
    # Load configuration with just defaults
    config = create_config()
    
    # Show configuration
    print(show_config(config))
    
    # Get signature configuration
    sig_config = get_signature_config(config, "CustomerFeedbackAnalyzer")
    print(f"\nSignature config: {sig_config}")


def example_2_config_file():
    """Example 2: Configuration with config file"""
    print("\n" + "=" * 60)
    print("Example 2: Configuration with Config File")
    print("=" * 60)
    
    # Create a temporary config file
    config_content = """
dspy:
  default_model: "ollama/qwen2.5:latest"
  default_temperature: 0.7
  default_max_tokens: 1000
  
  models:
    qwen2.5:
      provider: "ollama"
      model: "qwen2.5:latest"
      temperature: 0.7
      max_tokens: 1000
      timeout: 30
    
    gpt4:
      provider: "openai"
      model: "gpt-4"
      temperature: 0.3
      max_tokens: 2000
      timeout: 60

signatures:
  CustomerFeedbackAnalyzer:
    model: "qwen2.5"
    temperature: 0.8
    max_tokens: 1500
  
  SentimentAnalyzer:
    model: "gpt4"
    temperature: 0.3
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(config_content)
        config_file = f.name
    
    try:
        # Load configuration with config file
        config = create_config(config_file=config_file)
        
        # Show configuration
        print(show_config(config, "CustomerFeedbackAnalyzer"))
        
        # Get signature configurations
        feedback_config = get_signature_config(config, "CustomerFeedbackAnalyzer")
        sentiment_config = get_signature_config(config, "SentimentAnalyzer")
        
        print(f"\nCustomerFeedbackAnalyzer config: {feedback_config}")
        print(f"SentimentAnalyzer config: {sentiment_config}")
        
    finally:
        # Clean up
        os.unlink(config_file)


def example_3_xml_settings():
    """Example 3: Configuration with XML embedded settings"""
    print("\n" + "=" * 60)
    print("Example 3: Configuration with XML Embedded Settings")
    print("=" * 60)
    
    # XML settings (extracted from BPMN)
    xml_settings = {
        "CustomerFeedbackAnalyzer": {
            "model": "ollama/qwen2.5:latest",
            "temperature": 0.7,
            "max_tokens": 1000
        }
    }
    
    # Load configuration with XML settings
    config = create_config(xml_settings=xml_settings)
    
    # Show configuration
    print(show_config(config, "CustomerFeedbackAnalyzer"))


def example_4_cli_overrides():
    """Example 4: Configuration with CLI overrides"""
    print("\n" + "=" * 60)
    print("Example 4: Configuration with CLI Overrides")
    print("=" * 60)
    
    # CLI overrides (highest priority)
    cli_overrides = {
        "model": "gpt4",
        "temperature": 0.3,
        "max_tokens": 2000
    }
    
    # Load configuration with CLI overrides
    config = create_config(cli_overrides=cli_overrides)
    
    # Show configuration
    print(show_config(config, "CustomerFeedbackAnalyzer"))


def example_5_environment_variables():
    """Example 5: Configuration with environment variables"""
    print("\n" + "=" * 60)
    print("Example 5: Configuration with Environment Variables")
    print("=" * 60)
    
    # Set environment variables
    os.environ["AUTOTEL_DSPY__DEFAULT_MODEL"] = "ollama/llama2:latest"
    os.environ["AUTOTEL_DSPY__DEFAULT_TEMPERATURE"] = "0.5"
    
    try:
        # Load configuration (will pick up environment variables)
        config = create_config()
        
        # Show configuration
        print(show_config(config, "CustomerFeedbackAnalyzer"))
        
    finally:
        # Clean up environment variables
        if "AUTOTEL_DSPY__DEFAULT_MODEL" in os.environ:
            del os.environ["AUTOTEL_DSPY__DEFAULT_MODEL"]
        if "AUTOTEL_DSPY__DEFAULT_TEMPERATURE" in os.environ:
            del os.environ["AUTOTEL_DSPY__DEFAULT_TEMPERATURE"]


def example_6_hierarchical_precedence():
    """Example 6: Full hierarchical precedence demonstration"""
    print("\n" + "=" * 60)
    print("Example 6: Full Hierarchical Precedence")
    print("=" * 60)
    
    # Create config file
    config_content = """
dspy:
  default_model: "ollama/qwen2.5:latest"
  default_temperature: 0.7
  
signatures:
  CustomerFeedbackAnalyzer:
    model: "qwen2.5"
    temperature: 0.8
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(config_content)
        config_file = f.name
    
    try:
        # XML settings (medium priority)
        xml_settings = {
            "CustomerFeedbackAnalyzer": {
                "model": "ollama/qwen2.5:latest",
                "temperature": 0.7,  # This will be overridden by config file
                "max_tokens": 1000
            }
        }
        
        # CLI overrides (highest priority)
        cli_overrides = {
            "model": "gpt4",  # This will override everything
            "max_tokens": 2000
        }
        
        # Load configuration with all sources
        config = create_config(
            config_file=config_file,
            xml_settings=xml_settings,
            cli_overrides=cli_overrides
        )
        
        # Show final configuration
        print("Final Configuration for CustomerFeedbackAnalyzer:")
        print(show_config(config, "CustomerFeedbackAnalyzer"))
        
        # Explain the precedence
        print("\nPrecedence Explanation:")
        print("1. CLI override: model=gpt4, max_tokens=2000")
        print("2. Config file: temperature=0.8 (overrides XML)")
        print("3. XML: max_tokens=1000 (overridden by CLI)")
        print("4. Defaults: timeout=30")
        
    finally:
        # Clean up
        os.unlink(config_file)


def example_7_validation():
    """Example 7: Configuration validation"""
    print("\n" + "=" * 60)
    print("Example 7: Configuration Validation")
    print("=" * 60)
    
    # Invalid configuration
    cli_overrides = {
        "temperature": 1.5,  # Invalid: > 1.0
        "max_tokens": -100   # Invalid: negative
    }
    
    try:
        # Load configuration with invalid settings
        config = create_config(cli_overrides=cli_overrides)
        
        # Validate configuration
        errors = validate_config(config)
        
        if errors:
            print("Configuration validation errors:")
            for error in errors:
                print(f"  ❌ {error}")
        else:
            print("✅ Configuration is valid")
            
    except Exception as e:
        print(f"❌ Configuration error: {e}")


def example_8_environment_specific():
    """Example 8: Environment-specific configuration"""
    print("\n" + "=" * 60)
    print("Example 8: Environment-Specific Configuration")
    print("=" * 60)
    
    # Config file with environment-specific settings
    config_content = """
dspy:
  default_model: "ollama/qwen2.5:latest"
  default_temperature: 0.7

environments:
  production:
    dspy:
      default_model: "openai/gpt-4"
      default_temperature: 0.3
  
  development:
    dspy:
      default_model: "ollama/qwen2.5:latest"
      default_temperature: 0.7
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(config_content)
        config_file = f.name
    
    try:
        # Load production configuration
        print("Production Configuration:")
        config = create_config(config_file=config_file, environment="production")
        print(show_config(config, "CustomerFeedbackAnalyzer"))
        
        # Load development configuration
        print("\nDevelopment Configuration:")
        config = create_config(config_file=config_file, environment="development")
        print(show_config(config, "CustomerFeedbackAnalyzer"))
        
    finally:
        # Clean up
        os.unlink(config_file)


if __name__ == "__main__":
    print("AutoTel Configuration Examples using Confz v2")
    print("=" * 60)
    
    # Run all examples
    example_1_basic_config()
    example_2_config_file()
    example_3_xml_settings()
    example_4_cli_overrides()
    example_5_environment_variables()
    example_6_hierarchical_precedence()
    example_7_validation()
    example_8_environment_specific()
    
    print("\n" + "=" * 60)
    print("Configuration Examples Complete!")
    print("=" * 60) 