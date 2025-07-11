# AutoTel - Enterprise BPMN 2.0 Orchestration with AI-Powered Decision Making

AutoTel is an enterprise-grade BPMN 2.0 orchestration framework that combines **workflow automation**, **decision management (DMN)**, and **AI-powered decision making (DSPy)** with **zero-touch telemetry integration**. Every operation is automatically instrumented with OpenTelemetry and validated against LinkML schemas.

## 🎯 Key Features

- **BPMN 2.0 Workflow Engine** - Full SpiffWorkflow integration
- **DMN Decision Tables** - Business rule automation
- **DSPy AI Integration** - LLM-powered decision making
- **Telemetry-First Architecture** - Automatic OpenTelemetry instrumentation
- **Schema-Driven Validation** - LinkML schema validation for all telemetry
- **File System Routing** - Automatic metadata capture for observability
- **Enterprise CLI** - Comprehensive command-line interface

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone <repository-url>
cd autotel

# Install dependencies using uv (recommended)
uv sync

# Install the package in development mode
uv pip install -e .
```

### First Steps

1. **Initialize AutoTel**:
   ```bash
   autotel init
   ```

2. **Check system status**:
   ```bash
   autotel version
   ```

3. **Validate a BPMN workflow**:
   ```bash
   autotel validate bpmn/sample_process.bpmn
   ```

4. **Execute a workflow**:
   ```bash
   autotel workflow bpmn/sample_process.bpmn
   ```

## 📋 CLI Commands

### Core Commands

| Command | Description |
|---------|-------------|
| `autotel version` | Show system information and versions |
| `autotel init` | Initialize AutoTel with configuration |
| `autotel validate <file>` | Validate BPMN, DMN, or YAML files |
| `autotel telemetry` | Manage telemetry data and export traces |
| `autotel dspy` | Manage DSPy services and signatures |
| `autotel workflow <file>` | Execute or validate BPMN workflows |
| `autotel dmn <file>` | Execute or validate DMN decisions |
| `autotel config` | Manage AutoTel configuration |

### Command Examples

```bash
# Initialize with custom telemetry schema
autotel init --telemetry-schema ./custom_schema.yaml --validation-level strict

# Validate a BPMN file with strict validation
autotel validate bpmn/sample_process.bpmn --strict

# Show telemetry statistics
autotel telemetry --stats

# Export telemetry to JSON
autotel telemetry --export traces.json --format json

# List available DSPy signatures
autotel dspy --list

# Execute a workflow with telemetry export
autotel workflow bpmn/sample_process.bpmn --export-telemetry workflow_traces.json

# Execute a DMN decision with input data
autotel dmn bpmn/quality_decision.dmn --input '{"score": 85, "priority": "high"}'

# Show current configuration
autotel config --show
```

## 🔄 BPMN Workflow Integration

### Example BPMN Workflow

```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <bpmn:process id="order_processing" name="Order Processing">
    <bpmn:startEvent id="start" name="Order Received"/>
    <bpmn:task id="validate_order" name="Validate Order"/>
    <bpmn:businessRuleTask id="check_credit" name="Check Credit Score"/>
    <bpmn:serviceTask id="process_payment" name="Process Payment"/>
    <bpmn:endEvent id="end" name="Order Completed"/>
    
    <bpmn:sequenceFlow sourceRef="start" targetRef="validate_order"/>
    <bpmn:sequenceFlow sourceRef="validate_order" targetRef="check_credit"/>
    <bpmn:sequenceFlow sourceRef="check_credit" targetRef="process_payment"/>
    <bpmn:sequenceFlow sourceRef="process_payment" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>
```

### Workflow Execution

```bash
# Validate the workflow
autotel validate bpmn/order_processing.bpmn

# Execute the workflow
autotel workflow bpmn/order_processing.bpmn

# Execute with telemetry export
autotel workflow bpmn/order_processing.bpmn --export-telemetry order_traces.json
```

## 🎯 DMN Decision Tables

### Example DMN Decision Table

```xml
<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/">
  <decision id="credit_decision" name="Credit Decision">
    <decisionTable hitPolicy="FIRST">
      <input id="credit_score" label="Credit Score"/>
      <input id="income" label="Annual Income"/>
      <output id="approval" label="Approval Decision"/>
      <output id="limit" label="Credit Limit"/>
      
      <rule id="rule1">
        <inputEntry id="input1">
          <text>>= 750</text>
        </inputEntry>
        <inputEntry id="input2">
          <text>>= 50000</text>
        </inputEntry>
        <outputEntry id="output1">
          <text>"APPROVED"</text>
        </outputEntry>
        <outputEntry id="output2">
          <text>10000</text>
        </outputEntry>
      </rule>
      
      <rule id="rule2">
        <inputEntry id="input3">
          <text>< 750</text>
        </inputEntry>
        <inputEntry id="input4">
          <text>< 50000</text>
        </inputEntry>
        <outputEntry id="output3">
          <text>"DENIED"</text>
        </outputEntry>
        <outputEntry id="output4">
          <text>0</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>
```

### DMN Execution

```bash
# Validate the DMN decision table
autotel dmn bpmn/credit_decision.dmn --validate-only

# Execute the decision with input data
autotel dmn bpmn/credit_decision.dmn --input '{"credit_score": 780, "income": 75000}'
```

## 🤖 DSPy AI Integration

### Example DSPy Signature

```python
# DSPy signature for credit analysis
@dspy_signature
def analyze_credit_risk(customer_data: dict) -> dict:
    """
    Analyze customer credit risk using AI.
    
    Args:
        customer_data: Customer information including credit history
        
    Returns:
        dict: Risk assessment with score and recommendations
    """
    return {
        "risk_score": 0.85,
        "risk_level": "MEDIUM",
        "recommendations": ["Request additional documentation", "Consider co-signer"]
    }
```

### DSPy Usage

```bash
# List available DSPy signatures
autotel dspy --list

# Call a DSPy signature with input data
autotel dspy --call analyze_credit_risk --input '{"credit_history": "good", "income": 60000}'

# Show DSPy statistics
autotel dspy --stats

# Clear DSPy cache
autotel dspy --clear-cache
```

## 📊 Telemetry-First Architecture

### Automatic Instrumentation

Every CLI command is automatically wrapped in OpenTelemetry spans with:

- **File system routing metadata** (`module`, `function`, `file_path`)
- **Schema-driven validation** against LinkML schemas
- **Automatic error recording** as span events
- **Rich attribute capture** from command arguments

### Example Telemetry Output

```json
{
  "name": "validate",
  "attributes": {
    "operation_type": "class_analysis",
    "module": "autotel.cli",
    "function": "validate",
    "file_path": "/Users/sac/autotel/autotel/cli.py",
    "file_path": "bpmn/sample_process.bpmn",
    "strict": false
  },
  "events": [],
  "resource": {
    "attributes": {
      "service.name": "autotel-service",
      "service.version": "1.0.0"
    }
  }
}
```

### Telemetry Management

```bash
# Show telemetry statistics
autotel telemetry --stats

# Export telemetry to JSON
autotel telemetry --export traces.json --format json

# Export telemetry to YAML
autotel telemetry --export traces.yaml --format yaml
```

## 🔧 Configuration

### Configuration File

Create `autotel-config.yaml`:

```yaml
telemetry:
  enabled: true
  linkml_validation: true
  export_format: json

dspy:
  cache_enabled: true
  models:
    - "openai:gpt-4o-mini"
    - "ollama:qwen2.5:7b"

validation:
  level: normal
  strict_mode: false

workflow:
  persistence_enabled: true
  max_instances: 100
```

### Environment Variables

```bash
export AUTOTEL_CONFIG_FILE="./autotel-config.yaml"
export AUTOTEL_LOG_LEVEL="INFO"
export AUTOTEL_TELEMETRY_ENABLED="true"
```

## 📚 Use Case Examples

### Use Case 1: Loan Application Processing

**Scenario**: Automated loan application processing with AI-powered risk assessment.

**Components**:
- **BPMN**: Loan application workflow
- **DMN**: Credit scoring decision table
- **DSPy**: AI-powered risk analysis

**Workflow**:
1. Customer submits loan application
2. BPMN workflow validates application data
3. DMN decision table calculates initial credit score
4. DSPy AI analyzes additional risk factors
5. Final approval decision made

```bash
# Execute the loan processing workflow
autotel workflow bpmn/loan_application.bpmn --export-telemetry loan_traces.json

# Check the DMN decision
autotel dmn bpmn/credit_scoring.dmn --input '{"income": 75000, "credit_score": 720}'

# Use DSPy for risk analysis
autotel dspy --call analyze_loan_risk --input '{"application_data": {...}}'
```

### Use Case 2: Quality Assurance Pipeline

**Scenario**: Automated quality control with ML-powered defect detection.

**Components**:
- **BPMN**: Quality control workflow
- **DMN**: Quality thresholds decision table
- **DSPy**: ML model for defect classification

**Workflow**:
1. Product enters quality control
2. Automated tests run
3. DMN evaluates test results against thresholds
4. DSPy AI classifies defects
5. Decision made on product disposition

```bash
# Execute quality control workflow
autotel workflow bpmn/quality_control.bpmn

# Evaluate quality thresholds
autotel dmn bpmn/quality_thresholds.dmn --input '{"test_score": 85, "defect_count": 2}'

# Classify defects with AI
autotel dspy --call classify_defects --input '{"defect_images": [...], "test_results": {...}}'
```

### Use Case 3: Customer Service Automation

**Scenario**: Intelligent customer service with automated routing and resolution.

**Components**:
- **BPMN**: Customer service workflow
- **DMN**: Service level agreement rules
- **DSPy**: Customer intent classification and response generation

**Workflow**:
1. Customer submits support ticket
2. DSPy AI classifies intent and priority
3. DMN determines SLA requirements
4. BPMN routes to appropriate team
5. DSPy generates initial response

```bash
# Process customer service ticket
autotel workflow bpmn/customer_service.bpmn

# Classify customer intent
autotel dspy --call classify_intent --input '{"ticket_text": "My order is delayed..."}'

# Determine SLA requirements
autotel dmn bpmn/sla_rules.dmn --input '{"priority": "high", "customer_tier": "premium"}'
```

## 🛠️ Development

### Adding New Commands

The CLI uses a custom `@otel_command` decorator for automatic telemetry:

```python
from autotel.utils.helpers import otel_command

@app.command()
@otel_command
def my_command(
    input_file: Path = typer.Argument(..., help="Input file"),
    verbose: bool = typer.Option(False, "--verbose", help="Verbose output")
):
    """My custom command with automatic telemetry."""
    # Command logic here
    pass
```

### Custom DSPy Signatures

```python
from autotel.utils.advanced_dspy_services import dspy_signature

@dspy_signature
def my_ai_function(input_data: dict) -> dict:
    """My AI-powered function."""
    # AI logic here
    return {"result": "success"}
```

## 🔍 Troubleshooting

### Common Issues

1. **"LinkML schema validation failed"**
   - Ensure `otel_traces_schema.yaml` exists
   - Check schema syntax and required classes

2. **"DSPy signature not found"**
   - Verify signature is properly registered
   - Check signature name and parameters

3. **"BPMN validation failed"**
   - Ensure BPMN file follows 2.0 specification
   - Check for CDATA sections (not allowed)

4. **"Telemetry export failed"**
   - Check file permissions
   - Verify export format (json/yaml)

### Debug Mode

```bash
# Enable verbose logging
export AUTOTEL_LOG_LEVEL="DEBUG"

# Run with debug output
autotel --verbose validate bpmn/sample_process.bpmn
```

## 📈 Monitoring and Observability

### Telemetry Analysis

```bash
# Export telemetry for analysis
autotel telemetry --export traces.json

# Analyze with external tools
jq '.spans[] | select(.attributes.function == "validate")' traces.json
```

### Performance Monitoring

```bash
# Check DSPy performance
autotel dspy --stats

# Monitor workflow execution
autotel workflow bpmn/sample_process.bpmn --export-telemetry perf_traces.json
```

## 🤝 Contributing

1. Follow the telemetry-first approach
2. Use the `@otel_command` decorator for new CLI commands
3. Add LinkML schema definitions for new attributes
4. Include comprehensive tests
5. Update documentation

## 📄 License

MIT License - see LICENSE file for details.

## 🆘 Support

- **Documentation**: Check this README and inline help
- **CLI Help**: `autotel --help` or `autotel <command> --help`
- **Issues**: Report on GitHub repository
- **Telemetry**: All operations are automatically logged for debugging

---

**AutoTel**: Enterprise BPMN 2.0 orchestration with AI-powered decision making and zero-touch telemetry integration. 🚀
