"""Tests for configuration loader module."""

import pytest
import tempfile
import os
from pathlib import Path
from unittest.mock import patch, MagicMock
from autotel.helpers.config.loader import (
    load_processor_config,
    load_telemetry_config,
    load_contract_config,
    load_config_file,
    validate_config_schema,
    merge_configs,
    get_config_value,
    set_config_value,
    reload_config
)


class TestConfigLoader:
    """Test configuration loader functionality."""

    def test_load_processor_config_success(self):
        """Test successful processor config loading."""
        config_data = {
            "processors": {
                "bpmn": {"enabled": True, "timeout": 30},
                "dmn": {"enabled": False, "timeout": 15}
            }
        }
        
        with patch('autotel.helpers.config.loader.load_config_file', return_value=config_data):
            with patch('autotel.helpers.config.loader.validate_config_schema') as mock_validate:
                result = load_processor_config("test_config.yaml")
                
                assert result == config_data
                mock_validate.assert_called_once_with(config_data, "processor")

    def test_load_telemetry_config_success(self):
        """Test successful telemetry config loading."""
        config_data = {
            "telemetry": {
                "enabled": True,
                "endpoint": "http://localhost:4317",
                "sampling_rate": 0.1
            }
        }
        
        with patch('autotel.helpers.config.loader.load_config_file', return_value=config_data):
            with patch('autotel.helpers.config.loader.validate_config_schema') as mock_validate:
                result = load_telemetry_config("test_config.yaml")
                
                assert result == config_data
                mock_validate.assert_called_once_with(config_data, "telemetry")

    def test_load_contract_config_success(self):
        """Test successful contract config loading."""
        config_data = {
            "contracts": {
                "enabled": True,
                "strict_mode": False,
                "validation_level": "warn"
            }
        }
        
        with patch('autotel.helpers.config.loader.load_config_file', return_value=config_data):
            with patch('autotel.helpers.config.loader.validate_config_schema') as mock_validate:
                result = load_contract_config("test_config.yaml")
                
                assert result == config_data
                mock_validate.assert_called_once_with(config_data, "contract")

    def test_load_config_file_yaml(self):
        """Test loading YAML config file."""
        yaml_content = """
        processors:
          bpmn:
            enabled: true
            timeout: 30
        """
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            f.write(yaml_content)
            temp_file = f.name
        
        try:
            result = load_config_file(temp_file)
            assert result["processors"]["bpmn"]["enabled"] is True
            assert result["processors"]["bpmn"]["timeout"] == 30
        finally:
            os.unlink(temp_file)

    def test_load_config_file_json(self):
        """Test loading JSON config file."""
        json_content = '{"processors": {"bpmn": {"enabled": true, "timeout": 30}}}'
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            f.write(json_content)
            temp_file = f.name
        
        try:
            result = load_config_file(temp_file)
            assert result["processors"]["bpmn"]["enabled"] is True
            assert result["processors"]["bpmn"]["timeout"] == 30
        finally:
            os.unlink(temp_file)

    def test_validate_config_schema_success(self):
        """Test successful config schema validation."""
        config_data = {
            "processors": {
                "bpmn": {"enabled": True, "timeout": 30}
            }
        }
        # No patching needed, just call the stub
        assert validate_config_schema(config_data, "processor") is None

    def test_merge_configs_success(self):
        """Test successful config merging."""
        base_config = {
            "processors": {
                "bpmn": {"enabled": True, "timeout": 30}
            }
        }
        
        override_config = {
            "processors": {
                "bpmn": {"timeout": 60},
                "dmn": {"enabled": False}
            }
        }
        
        result = merge_configs(base_config, override_config)
        
        assert result["processors"]["bpmn"]["enabled"] is True
        assert result["processors"]["bpmn"]["timeout"] == 60
        assert result["processors"]["dmn"]["enabled"] is False

    def test_get_config_value_success(self):
        """Test successful config value retrieval."""
        config = {
            "processors": {
                "bpmn": {
                    "settings": {
                        "timeout": 30
                    }
                }
            }
        }
        
        result = get_config_value(config, "processors.bpmn.settings.timeout")
        assert result == 30

    def test_get_config_value_with_default(self):
        """Test config value retrieval with default."""
        config = {"processors": {"bpmn": {}}}
        
        result = get_config_value(config, "processors.bpmn.timeout", default=60)
        assert result == 60

    def test_set_config_value_success(self):
        """Test successful config value setting."""
        config = {
            "processors": {
                "bpmn": {
                    "settings": {}
                }
            }
        }
        
        set_config_value(config, "processors.bpmn.settings.timeout", 45)
        assert config["processors"]["bpmn"]["settings"]["timeout"] == 45

    def test_reload_config_success(self):
        """Test successful config reloading."""
        original_config = {"processors": {"bpmn": {"enabled": True}}}
        new_config = {"processors": {"bpmn": {"enabled": False}}}
        
        with patch('autotel.helpers.config.loader.load_config_file', return_value=new_config):
            with patch('autotel.helpers.config.loader.validate_config_schema'):
                result = reload_config("test_config.yaml", original_config)
                assert result == new_config

    def test_load_config_file_unsupported_format(self):
        """Test loading unsupported config format."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write("some content")
            temp_file = f.name
        
        try:
            with pytest.raises(ValueError, match="Unsupported config file format"):
                load_config_file(temp_file)
        finally:
            os.unlink(temp_file)

    def test_get_config_value_nested_path(self):
        """Test getting config value with deeply nested path."""
        config = {
            "level1": {
                "level2": {
                    "level3": {
                        "value": "test_value"
                    }
                }
            }
        }
        
        result = get_config_value(config, "level1.level2.level3.value")
        assert result == "test_value"

    def test_set_config_value_nested_path(self):
        """Test setting config value with deeply nested path."""
        config = {
            "level1": {
                "level2": {}
            }
        }
        
        set_config_value(config, "level1.level2.level3.value", "new_value")
        assert config["level1"]["level2"]["level3"]["value"] == "new_value"

    def test_merge_configs_deep_nesting(self):
        """Test merging configs with deep nesting."""
        base_config = {
            "processors": {
                "bpmn": {
                    "settings": {
                        "timeout": 30,
                        "retries": 3
                    }
                }
            }
        }
        
        override_config = {
            "processors": {
                "bpmn": {
                    "settings": {
                        "timeout": 60
                    }
                }
            }
        }
        
        result = merge_configs(base_config, override_config)
        
        assert result["processors"]["bpmn"]["settings"]["timeout"] == 60
        assert result["processors"]["bpmn"]["settings"]["retries"] == 3 