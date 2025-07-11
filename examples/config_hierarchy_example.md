# AutoTel Configuration Hierarchy

AutoTel supports multiple configuration sources that work together with clear precedence rules.

## Configuration Hierarchy (Priority Order)

### 1. CLI Override (Highest Priority)
```bash
# Override model for this run only
autotel run --workflow workflow.bpmn --input data.json --model gpt4 --temperature 0.3

# Override multiple settings
autotel run --workflow workflow.bpmn --input data.json \
  --model gpt4 \
  --temperature 0.3 \
  --max-tokens 2000 \
  --timeout 60
```

### 2. XML Embedded (Medium Priority)
```xml
<dspy:signature name="CustomerFeedbackAnalyzer">
  <dspy:model>ollama/qwen2.5:latest</dspy:model>
  <dspy:temperature>0.7</dspy:temperature>
  <dspy:max_tokens>1000</dspy:max_tokens>
  <!-- inputs and outputs -->
</dspy:signature>
```

### 3. Config File (Lower Priority)
```yaml
# autotel_config.yaml
dspy:
  default_model: "ollama/qwen2.5:latest"
  default_temperature: 0.7
  
signatures:
  CustomerFeedbackAnalyzer:
    model: "qwen2.5"
    temperature: 0.8
```

### 4. Environment Variables (Lowest Priority)
```bash
export AUTOTEL_DSPY_MODEL="ollama/qwen2.5:latest"
export AUTOTEL_DSPY_TEMPERATURE="0.7"
export AUTOTEL_DSPY_MAX_TOKENS="1000"
```

### 5. Defaults (Fallback)
```python
# Built-in defaults
DEFAULT_MODEL = "ollama/qwen2.5:latest"
DEFAULT_TEMPERATURE = 0.7
DEFAULT_MAX_TOKENS = 1000
```

## Example: How It All Works Together

### Scenario 1: XML + Config File
```bash
# XML has: model="ollama/qwen2.5:latest", temperature="0.7"
# Config has: CustomerFeedbackAnalyzer.temperature="0.8"
# Result: Uses XML model, config temperature (0.8 wins)
autotel run --workflow workflow.bpmn --input data.json --config config.yaml
```

### Scenario 2: CLI Override Everything
```bash
# XML has: model="ollama/qwen2.5:latest", temperature="0.7"
# Config has: CustomerFeedbackAnalyzer.temperature="0.8"
# CLI has: --model gpt4 --temperature 0.3
# Result: Uses CLI settings (gpt4, 0.3)
autotel run --workflow workflow.bpmn --input data.json --config config.yaml --model gpt4 --temperature 0.3
```

### Scenario 3: Partial Override
```bash
# XML has: model="ollama/qwen2.5:latest", temperature="0.7"
# CLI has: --temperature 0.5
# Result: Uses XML model (qwen2.5), CLI temperature (0.5)
autotel run --workflow workflow.bpmn --input data.json --temperature 0.5
```

## Configuration Resolution Examples

### Example 1: Complete Configuration
```bash
# Environment: AUTOTEL_DSPY_MODEL="ollama/qwen2.5:latest"
# Config file: CustomerFeedbackAnalyzer.model="gpt4"
# XML: <dspy:model>claude</dspy:model>
# CLI: --model llama2

# Final result: llama2 (CLI wins)
autotel run --workflow workflow.bpmn --input data.json --config config.yaml --model llama2
```

### Example 2: Mixed Configuration
```bash
# Environment: AUTOTEL_DSPY_TEMPERATURE="0.5"
# Config file: CustomerFeedbackAnalyzer.temperature="0.8"
# XML: <dspy:temperature>0.7</dspy:temperature>
# CLI: --max-tokens 2000

# Final result: 
# - model: from XML (no CLI override)
# - temperature: 0.8 (config wins over XML)
# - max_tokens: 2000 (CLI wins)
autotel run --workflow workflow.bpmn --input data.json --config config.yaml --max-tokens 2000
```

## CLI Commands for Configuration Management

### Show Configuration Resolution
```bash
# Show what configuration will be used
autotel config --show --workflow workflow.bpmn --config config.yaml

# Show with CLI overrides
autotel config --show --workflow workflow.bpmn --config config.yaml --model gpt4
```

### Validate Configuration
```bash
# Validate config file
autotel config --validate config.yaml

# Validate workflow + config combination
autotel config --validate --workflow workflow.bpmn --config config.yaml
```

### Generate Configuration
```bash
# Generate config from workflow XML
autotel config --generate --workflow workflow.bpmn --output config.yaml

# Generate config template
autotel config --template --output config_template.yaml
```

## Best Practices

### 1. Use XML for Workflow-Specific Settings
```xml
<!-- Good: Model that makes sense for this specific workflow -->
<dspy:signature name="CustomerFeedbackAnalyzer">
  <dspy:model>ollama/qwen2.5:latest</dspy:model>
  <dspy:temperature>0.7</dspy:temperature>
</dspy:signature>
```

### 2. Use Config File for Environment Settings
```yaml
# Good: Environment-specific overrides
dspy:
  models:
    production:
      provider: "openai"
      model: "gpt-4"
      temperature: 0.3
    
    development:
      provider: "ollama"
      model: "qwen2.5:latest"
      temperature: 0.7
```

### 3. Use CLI for Testing and Overrides
```bash
# Good: Quick testing with different models
autotel run --workflow workflow.bpmn --input data.json --model gpt4 --temperature 0.3

# Good: Override for specific use case
autotel run --workflow workflow.bpmn --input data.json --max-tokens 5000
```

### 4. Use Environment Variables for Secrets
```bash
# Good: API keys and sensitive config
export OPENAI_API_KEY="sk-..."
export ANTHROPIC_API_KEY="sk-ant-..."
```

## Configuration File Structure

```yaml
# autotel_config.yaml
dspy:
  # Global defaults
  default_model: "ollama/qwen2.5:latest"
  default_temperature: 0.7
  default_max_tokens: 1000
  
  # Model definitions
  models:
    qwen2.5:
      provider: "ollama"
      model: "qwen2.5:latest"
      temperature: 0.7
      max_tokens: 1000
    
    gpt4:
      provider: "openai"
      model: "gpt-4"
      temperature: 0.3
      max_tokens: 2000
    
    claude:
      provider: "anthropic"
      model: "claude-3-sonnet-20240229"
      temperature: 0.5
      max_tokens: 1500

# Signature-specific overrides
signatures:
  CustomerFeedbackAnalyzer:
    model: "qwen2.5"
    temperature: 0.8
  
  SentimentAnalyzer:
    model: "gpt4"
    temperature: 0.3

# Environment-specific settings
environments:
  production:
    dspy:
      default_model: "gpt4"
      default_temperature: 0.3
  
  development:
    dspy:
      default_model: "qwen2.5"
      default_temperature: 0.7
```

## CLI Help Output

```bash
$ autotel run --help

Usage: autotel run [OPTIONS] --workflow FILE --input FILE

Options:
  --workflow FILE           BPMN workflow file [required]
  --input FILE              Input data file [required]
  --config FILE             Configuration file
  --model TEXT              Override model for all signatures
  --temperature FLOAT       Override temperature for all signatures
  --max-tokens INTEGER      Override max tokens for all signatures
  --timeout INTEGER         Override timeout in seconds
  --output FILE             Output file for results
  --verbose                 Verbose output
  --debug                   Debug mode
  --help                    Show this message and exit

Configuration Hierarchy:
  1. CLI overrides (highest priority)
  2. XML embedded settings
  3. Config file settings
  4. Environment variables
  5. Defaults (lowest priority)
```

This approach gives users maximum flexibility while maintaining clear precedence rules! 