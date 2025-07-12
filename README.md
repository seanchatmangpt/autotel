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

3. **Validate an integrated workflow**:
   ```bash
   autotel validate examples/ideal_workflow.bpmn
   ```

4. **Execute an integrated workflow**:
   ```bash
   autotel workflow examples/ideal_workflow.bpmn
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

# Validate an integrated workflow with strict validation
autotel validate examples/ideal_workflow.bpmn --strict

# Show telemetry statistics
autotel telemetry --stats

# Export telemetry to JSON
autotel telemetry --export traces.json --format json

# List available DSPy signatures
autotel dspy --list

# Execute an integrated workflow with telemetry export
autotel workflow examples/ideal_workflow.bpmn --export-telemetry workflow_traces.json

# Show current configuration
autotel config --show
```

## 🔄 Integrated BPMN + DMN + DSPy Workflows

AutoTel's power lies in combining all three technologies in a single file. Here are the integrated examples:

### Example 1: Customer Feedback Processing (Complete Integration)

**File**: `examples/ideal_workflow.bpmn`

This workflow demonstrates the complete integration of BPMN, DMN, and DSPy in a single file:

```xml
<?xml version="1.0"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL" 
                   xmlns:dspy="http://autotel.ai/dspy"
                   xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                   id="Definitions_1">
  
  <bpmn:process id="customer_feedback_workflow" name="Customer Feedback Processing" isExecutable="true">
    
    <!-- DSPy Service Task - Analyze customer feedback -->
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Customer Feedback">
      <bpmn:extensionElements>
        <dspy:signature name="CustomerFeedbackAnalyzer">
          <dspy:model>ollama/qwen2.5:latest</dspy:model>
          <dspy:input name="feedback_text" type="str" description="Customer feedback text to analyze"/>
          <dspy:input name="customer_id" type="str" description="Customer identifier"/>
          <dspy:output name="sentiment" type="str" description="Sentiment analysis result"/>
          <dspy:output name="confidence" type="float" description="Confidence score (0.0-1.0)"/>
          <dspy:output name="urgency_score" type="int" description="Urgency score (1-10)"/>
          <dspy:output name="category" type="str" description="Feedback category"/>
        </dspy:signature>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- DMN Business Rule Task - Route based on analysis -->
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Route Feedback">
      <bpmn:extensionElements>
        <spiffext:spiffExtension>
          <spiffext:decision name="feedback_routing" id="feedback_routing">
            <spiffext:decisionTable>
              <spiffext:input id="sentiment" label="Sentiment" typeRef="string"/>
              <spiffext:input id="urgency_score" label="Urgency Score" typeRef="integer"/>
              <spiffext:input id="confidence" label="Confidence" typeRef="double"/>
              <spiffext:input id="category" label="Category" typeRef="string"/>
              
              <spiffext:output id="route" label="Route" typeRef="string"/>
              <spiffext:output id="priority" label="Priority" typeRef="string"/>
              <spiffext:output id="escalation_level" label="Escalation Level" typeRef="string"/>
              
              <!-- High urgency negative feedback -->
              <spiffext:rule id="rule1">
                <spiffext:inputEntry id="input1">"negative"</spiffext:inputEntry>
                <spiffext:inputEntry id="input2">[8..10]</spiffext:inputEntry>
                <spiffext:inputEntry id="input3">[0.7..1.0]</spiffext:inputEntry>
                <spiffext:inputEntry id="input4">-</spiffext:inputEntry>
                <spiffext:outputEntry id="output1">"immediate_escalation"</spiffext:outputEntry>
                <spiffext:outputEntry id="output2">"critical"</spiffext:outputEntry>
                <spiffext:outputEntry id="output3">"executive"</spiffext:outputEntry>
              </spiffext:rule>
              
              <!-- Bug reports -->
              <spiffext:rule id="rule2">
                <spiffext:inputEntry id="input6">-</spiffext:inputEntry>
                <spiffext:inputEntry id="input7">[5..10]</spiffext:inputEntry>
                <spiffext:inputEntry id="input8">[0.6..1.0]</spiffext:inputEntry>
                <spiffext:inputEntry id="input9">"bug"</spiffext:inputEntry>
                <spiffext:outputEntry id="output4">"bug_triage"</spiffext:outputEntry>
                <spiffext:outputEntry id="output5">"high"</spiffext:outputEntry>
                <spiffext:outputEntry id="output6">"technical"</spiffext:outputEntry>
              </spiffext:rule>
              
              <!-- Feature requests -->
              <spiffext:rule id="rule3">
                <spiffext:inputEntry id="input11">-</spiffext:inputEntry>
                <spiffext:inputEntry id="input12">[1..7]</spiffext:inputEntry>
                <spiffext:inputEntry id="input13">[0.5..1.0]</spiffext:inputEntry>
                <spiffext:inputEntry id="input14">"feature"</spiffext:inputEntry>
                <spiffext:outputEntry id="output7">"product_planning"</spiffext:outputEntry>
                <spiffext:outputEntry id="output8">"medium"</spiffext:outputEntry>
                <spiffext:outputEntry id="output9">"product"</spiffext:outputEntry>
              </spiffext:rule>
              
              <!-- Positive feedback -->
              <spiffext:rule id="rule4">
                <spiffext:inputEntry id="input16">"positive"</spiffext:inputEntry>
                <spiffext:inputEntry id="input17">[1..5]</spiffext:inputEntry>
                <spiffext:inputEntry id="input18">[0.6..1.0]</spiffext:inputEntry>
                <spiffext:inputEntry id="input19">-</spiffext:inputEntry>
                <spiffext:outputEntry id="output10">"customer_success"</spiffext:outputEntry>
                <spiffext:outputEntry id="output11">"low"</spiffext:outputEntry>
                <spiffext:outputEntry id="output12">"none"</spiffext:outputEntry>
              </spiffext:rule>
              
              <!-- Default case -->
              <spiffext:rule id="rule5">
                <spiffext:inputEntry id="input21">-</spiffext:inputEntry>
                <spiffext:inputEntry id="input22">-</spiffext:inputEntry>
                <spiffext:inputEntry id="input23">-</spiffext:inputEntry>
                <spiffext:inputEntry id="input24">-</spiffext:inputEntry>
                <spiffext:outputEntry id="output13">"manual_review"</spiffext:outputEntry>
                <spiffext:outputEntry id="output14">"medium"</spiffext:outputEntry>
                <spiffext:outputEntry id="output15">"supervisor"</spiffext:outputEntry>
              </spiffext:rule>
            </spiffext:decisionTable>
          </spiffext:decision>
        </spiffext:spiffExtension>
      </bpmn:extensionElements>
    </bpmn:businessRuleTask>
    
    <!-- Gateway to route based on DMN decision -->
    <bpmn:exclusiveGateway id="Gateway_1" name="Route Decision">
      <bpmn:incoming>Flow_4</bpmn:incoming>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
      <bpmn:outgoing>Flow_6</bpmn:outgoing>
      <bpmn:outgoing>Flow_7</bpmn:outgoing>
      <bpmn:outgoing>Flow_8</bpmn:outgoing>
      <bpmn:outgoing>Flow_9</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- User Tasks for different routes -->
    <bpmn:userTask id="UserTask_1" name="Executive Escalation">
      <bpmn:incoming>Flow_5</bpmn:incoming>
      <bpmn:outgoing>Flow_10</bpmn:outgoing>
    </bpmn:userTask>
    
    <bpmn:userTask id="UserTask_2" name="Bug Triage">
      <bpmn:incoming>Flow_6</bpmn:incoming>
      <bpmn:outgoing>Flow_11</bpmn:outgoing>
    </bpmn:userTask>
    
    <bpmn:userTask id="UserTask_3" name="Product Planning Review">
      <bpmn:incoming>Flow_7</bpmn:incoming>
      <bpmn:outgoing>Flow_12</bpmn:outgoing>
    </bpmn:userTask>
    
    <bpmn:userTask id="UserTask_4" name="Customer Success Follow-up">
      <bpmn:incoming>Flow_8</bpmn:incoming>
      <bpmn:outgoing>Flow_13</bpmn:outgoing>
    </bpmn:userTask>
    
    <bpmn:userTask id="UserTask_5" name="Manual Review">
      <bpmn:incoming>Flow_9</bpmn:incoming>
      <bpmn:outgoing>Flow_14</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- End Event -->
    <bpmn:endEvent id="EndEvent_1" name="Feedback Processed">
      <bpmn:incoming>Flow_10</bpmn:incoming>
      <bpmn:incoming>Flow_11</bpmn:incoming>
      <bpmn:incoming>Flow_12</bpmn:incoming>
      <bpmn:incoming>Flow_13</bpmn:incoming>
      <bpmn:incoming>Flow_14</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows with conditional routing -->
    <bpmn:sequenceFlow id="Flow_5" sourceRef="Gateway_1" targetRef="UserTask_1">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">${route == "immediate_escalation"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="Flow_6" sourceRef="Gateway_1" targetRef="UserTask_2">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">${route == "bug_triage"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="Flow_7" sourceRef="Gateway_1" targetRef="UserTask_3">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">${route == "product_planning"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="Flow_8" sourceRef="Gateway_1" targetRef="UserTask_4">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">${route == "customer_success"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="Flow_9" sourceRef="Gateway_1" targetRef="UserTask_5">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">${route == "manual_review"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
  </bpmn:process>
  
</bpmn:definitions>
```

### Example 2: Data Quality Analysis (Simplified Integration)

**File**: `bpmn/dspy_dmn_workflow.bpmn`

This workflow shows a simpler integration pattern:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  id="Definitions_1">
  
  <!-- DSPy Signature Definition -->
  <dspy:signatures>
    <dspy:signature name="analyze_data" description="Analyze input data">
      <dspy:input name="data" description="Input data to analyze"/>
      <dspy:output name="result" description="Analysis result"/>
      <dspy:output name="quality" description="Data quality score"/>
    </dspy:signature>
  </dspy:signatures>
  
  <bpmn:process id="DspyDmnWorkflow" isExecutable="true">
    
    <!-- DSPy Analysis Task -->
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Data">
      <bpmn:extensionElements>
        <dspy:service name="analyze_data" result="analysis_result">
          <dspy:param name="data" value="input_data"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- DMN Decision Task -->
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Quality Decision" camunda:decisionRef="quality_decision">
    </bpmn:businessRuleTask>
    
    <!-- Gateway for conditional routing -->
    <bpmn:exclusiveGateway id="Gateway_1" name="Route Decision">
      <bpmn:incoming>Flow_3</bpmn:incoming>
      <bpmn:outgoing>Flow_4</bpmn:outgoing>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Conditional flows based on DMN decision -->
    <bpmn:sequenceFlow id="Flow_4" sourceRef="Gateway_1" targetRef="EndEvent_1">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">
        ${action == "proceed"}
      </bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="Flow_5" sourceRef="Gateway_1" targetRef="EndEvent_2">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">
        ${action == "stop"}
      </bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
  </bpmn:process>
  
</bpmn:definitions>
```

## 🎯 How the Integration Works

### 1. **DSPy in BPMN**
- **Signature Definition**: DSPy signatures are defined in the BPMN XML using `<dspy:signature>` elements
- **Service Tasks**: BPMN service tasks can call DSPy services using `<dspy:service>` extensions
- **AI-Powered Analysis**: DSPy provides LLM-powered analysis, classification, and decision support

### 2. **DMN in BPMN**
- **Business Rule Tasks**: BPMN business rule tasks execute DMN decision tables
- **Decision Tables**: DMN decision tables are embedded in the BPMN XML
- **Structured Decisions**: DMN provides rule-based decision making with clear input/output mappings

### 3. **BPMN Orchestration**
- **Workflow Control**: BPMN orchestrates the entire process flow
- **Conditional Routing**: BPMN gateways route based on DSPy analysis and DMN decisions
- **Task Management**: BPMN manages user tasks, service tasks, and business rule tasks

### 4. **Data Flow**
1. **Input** → BPMN Start Event
2. **DSPy Analysis** → AI-powered processing and analysis
3. **DMN Decision** → Rule-based decision making
4. **BPMN Routing** → Conditional workflow routing
5. **Output** → BPMN End Event

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
  "name": "workflow",
  "attributes": {
    "operation_type": "class_analysis",
    "module": "autotel.cli",
    "function": "workflow",
    "file_path": "/Users/sac/autotel/autotel/cli.py",
    "file_path": "examples/ideal_workflow.bpmn",
    "export_telemetry": "workflow_traces.json"
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

### Use Case 1: Customer Feedback Processing

**Scenario**: Automated customer feedback processing with AI-powered analysis and intelligent routing.

**Components** (all in one file):
- **BPMN**: Customer feedback workflow orchestration
- **DSPy**: AI-powered sentiment analysis and classification
- **DMN**: Business rules for routing and escalation

**Workflow**:
1. Customer submits feedback
2. DSPy AI analyzes sentiment, urgency, and category
3. DMN decision table routes based on analysis results
4. BPMN routes to appropriate team (executive escalation, bug triage, product planning, etc.)

```bash
# Execute the integrated customer feedback workflow
autotel workflow examples/ideal_workflow.bpmn --export-telemetry feedback_traces.json
```

### Use Case 2: Data Quality Pipeline

**Scenario**: Automated data quality assessment with ML-powered analysis and rule-based decisions.

**Components** (all in one file):
- **BPMN**: Data processing workflow
- **DSPy**: ML-powered data quality analysis
- **DMN**: Quality thresholds and routing decisions

**Workflow**:
1. Data enters the pipeline
2. DSPy AI analyzes data quality and patterns
3. DMN evaluates quality scores against thresholds
4. BPMN routes data for processing or rejection

```bash
# Execute the integrated data quality workflow
autotel workflow bpmn/dspy_dmn_workflow.bpmn --export-telemetry quality_traces.json
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
autotel --verbose validate examples/ideal_workflow.bpmn
```

## 📈 Monitoring and Observability

### Telemetry Analysis

```bash
# Export telemetry for analysis
autotel telemetry --export traces.json

# Analyze with external tools
jq '.spans[] | select(.attributes.function == "workflow")' traces.json
```

### Performance Monitoring

```bash
# Check DSPy performance
autotel dspy --stats

# Monitor workflow execution
autotel workflow examples/ideal_workflow.bpmn --export-telemetry perf_traces.json
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

**AutoTel**: Enterprise BPMN 2.0 orchestration with integrated AI-powered decision making and zero-touch telemetry integration. 🚀
