# DSPy + DMN Integration Guide for AutoTel

This guide explains how to integrate DSPy (LLM decision logic) with DMN (Decision Model and Notation) within BPMN workflows in the AutoTel framework.

## Overview

The integration allows you to:
1. **Define DSPy signatures in XML** - Create LLM-powered decision logic directly in BPMN files
2. **Execute DSPy services** - Use LLMs to analyze data and generate insights
3. **Feed DSPy results to DMN** - Use LLM outputs as inputs to traditional decision tables
4. **Create hybrid decision systems** - Combine AI-powered analysis with rule-based decisions

## Architecture

```
BPMN Workflow
├── DSPy Service Tasks (LLM Analysis)
│   ├── XML-defined signatures
│   ├── Dynamic signature creation
│   └── LLM-powered data processing
├── DMN Business Rule Tasks (Rule-based Decisions)
│   ├── Decision tables
│   ├── DSPy results as inputs
│   └── Structured decision logic
└── Workflow Orchestration
    ├── Data flow between DSPy and DMN
    ├── Telemetry and observability
    └── Schema validation
```

## Key Components

### 1. DSPy XML Integration
- **Signature Definitions**: Define DSPy signatures directly in BPMN XML
- **Dynamic Creation**: Auto-generate DSPy signature classes from XML
- **Service Tasks**: Execute DSPy signatures as BPMN service tasks

### 2. DMN Integration
- **Decision Tables**: Traditional rule-based decision logic
- **DSPy Inputs**: Use DSPy results as inputs to DMN decisions
- **Hybrid Decisions**: Combine AI insights with business rules

### 3. AutoTel Framework Integration
- **Telemetry**: Full OTEL instrumentation for DSPy and DMN execution
- **Validation**: LinkML schema validation for data integrity
- **CLI Support**: Command-line tools for workflow management

## XML Structure

### DSPy Signature Definitions
```xml
<dspy:signatures>
  <dspy:signature name="AnalyzeData" description="Analyze input data and provide insights">
    <dspy:input name="data" description="The data to analyze"/>
    <dspy:output name="analysis" description="Analysis results"/>
    <dspy:output name="confidence" description="Confidence score (0-100)"/>
    <dspy:output name="recommendation" description="Recommended action"/>
  </dspy:signature>
</dspy:signatures>
```

### DSPy Service Tasks
```xml
<bpmn:serviceTask id="ServiceTask_1" name="Analyze Data">
  <bpmn:extensionElements>
    <dspy:service name="AnalyzeData" result="analysis_result">
      <dspy:param name="data" value="input_data"/>
    </dspy:service>
  </bpmn:extensionElements>
</bpmn:serviceTask>
```

### DMN Decision Tables
```xml
<dmn:definitions id="Definitions_2" name="DRD" namespace="http://camunda.org/schema/1.0/dmn">
  <dmn:decision id="Decision_1" name="Approval Decision">
    <dmn:decisionTable id="DecisionTable_1" hitPolicy="FIRST">
      <!-- Input: Uses DSPy results -->
      <dmn:input id="Input_1" label="Confidence">
        <dmn:inputExpression id="InputExpression_1" typeRef="number">
          <dmn:text>analysis_result.confidence</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:input id="Input_2" label="Recommendation">
        <dmn:inputExpression id="InputExpression_2" typeRef="string">
          <dmn:text>analysis_result.recommendation</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output id="Output_1" label="Action" typeRef="string"/>
      
      <!-- Decision Rules -->
      <dmn:rule id="Rule_1">
        <dmn:inputEntry id="InputEntry_1">
          <dmn:text>&gt;= 80</dmn:text>
        </dmn:inputEntry>
        <dmn:inputEntry id="InputEntry_2">
          <dmn:text>"proceed"</dmn:text>
        </dmn:inputEntry>
        <dmn:outputEntry id="OutputEntry_1">
          <dmn:text>"APPROVE"</dmn:text>
        </dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>
```

### Business Rule Tasks
```xml
<bpmn:businessRuleTask id="BusinessRuleTask_1" name="Make Decision" 
                       camunda:decisionRef="Decision_1"/>
```

## Data Flow

### 1. DSPy Execution
```python
# DSPy signature is executed with input data
result = dspy_registry.call_signature("AnalyzeData", data=input_data)

# Result contains:
{
    "analysis": "Detailed analysis of the data...",
    "confidence": 85,
    "recommendation": "proceed"
}
```

### 2. Data Binding to DMN
```python
# DSPy results are automatically bound to workflow variables
workflow.data["analysis_result"] = {
    "analysis": "Detailed analysis of the data...",
    "confidence": 85,
    "recommendation": "proceed"
}
```

### 3. DMN Decision Execution
```python
# DMN decision table evaluates using DSPy results
dmn_inputs = {
    "confidence": 85,  # From analysis_result.confidence
    "recommendation": "proceed"  # From analysis_result.recommendation
}

# DMN rules are evaluated
decision_result = dmn_engine.execute(dmn_inputs)
# Result: {"Action": "APPROVE"}
```

## Implementation Examples

### Example 1: Sentiment Analysis + Routing
```xml
<!-- DSPy Signature for Sentiment Analysis -->
<dspy:signature name="SentimentAnalyzer" description="Analyze text sentiment">
  <dspy:input name="text" description="Text to analyze"/>
  <dspy:output name="sentiment" description="Sentiment (positive/negative/neutral)"/>
  <dspy:output name="confidence" description="Confidence score (0.0-1.0)"/>
  <dspy:output name="keywords" description="Key words found in text"/>
</dspy:signature>

<!-- DMN Decision for Routing -->
<dmn:decision id="sentiment_routing" name="Sentiment Routing">
  <dmn:decisionTable>
    <dmn:input id="sentiment" label="Sentiment" typeRef="string"/>
    <dmn:input id="confidence" label="Confidence" typeRef="double"/>
    <dmn:output id="route" label="Route" typeRef="string"/>
    
    <dmn:rule id="rule1">
      <dmn:inputEntry>"positive"</dmn:inputEntry>
      <dmn:inputEntry>&gt;= 0.8</dmn:inputEntry>
      <dmn:outputEntry>"approval"</dmn:outputEntry>
    </dmn:rule>
    
    <dmn:rule id="rule2">
      <dmn:inputEntry>"negative"</dmn:inputEntry>
      <dmn:inputEntry>&gt;= 0.7</dmn:inputEntry>
      <dmn:outputEntry>"rejection"</dmn:outputEntry>
    </dmn:rule>
  </dmn:decisionTable>
</dmn:decision>
```

### Example 2: Document Analysis + Approval
```xml
<!-- DSPy Signature for Document Analysis -->
<dspy:signature name="DocumentAnalyzer" description="Analyze document content">
  <dspy:input name="document_text" description="Document text to analyze"/>
  <dspy:output name="document_type" description="Type of document"/>
  <dspy:output name="risk_score" description="Risk assessment (0-100)"/>
  <dspy:output name="compliance_issues" description="List of compliance issues"/>
</dspy:signature>

<!-- DMN Decision for Approval -->
<dmn:decision id="document_approval" name="Document Approval">
  <dmn:decisionTable>
    <dmn:input id="document_type" label="Document Type" typeRef="string"/>
    <dmn:input id="risk_score" label="Risk Score" typeRef="integer"/>
    <dmn:output id="approval_level" label="Approval Level" typeRef="string"/>
    
    <dmn:rule id="rule1">
      <dmn:inputEntry>"contract"</dmn:inputEntry>
      <dmn:inputEntry>&lt;= 30</dmn:inputEntry>
      <dmn:outputEntry>"auto_approve"</dmn:outputEntry>
    </dmn:rule>
    
    <dmn:rule id="rule2">
      <dmn:inputEntry>"contract"</dmn:inputEntry>
      <dmn:inputEntry>[31..70]</dmn:inputEntry>
      <dmn:outputEntry>"manager_approval"</dmn:outputEntry>
    </dmn:rule>
    
    <dmn:rule id="rule3">
      <dmn:inputEntry>"contract"</dmn:inputEntry>
      <dmn:inputEntry>&gt; 70</dmn:inputEntry>
      <dmn:outputEntry>"legal_review"</dmn:outputEntry>
    </dmn:rule>
  </dmn:decisionTable>
</dmn:decision>
```

## Usage Patterns

### Pattern 1: Analysis → Decision → Action
1. **DSPy Service Task**: Analyze data using LLM
2. **DMN Business Rule Task**: Make structured decision based on analysis
3. **User/Service Tasks**: Execute actions based on decision

### Pattern 2: Multi-Stage Analysis
1. **DSPy Service Task 1**: Initial analysis
2. **DMN Business Rule Task 1**: Route to appropriate analysis
3. **DSPy Service Task 2**: Detailed analysis based on route
4. **DMN Business Rule Task 2**: Final decision

### Pattern 3: Validation + Approval
1. **DSPy Service Task**: Validate and assess risk
2. **DMN Business Rule Task**: Determine approval requirements
3. **User Tasks**: Manual review if needed
4. **DMN Business Rule Task**: Final approval decision

## CLI Usage

### Run DSPy + DMN Workflow
```bash
# Run a workflow with DSPy and DMN integration
autotel workflow run --bpmn workflow.bpmn --process-id "my_process" --data input.json

# Example with sample data
autotel workflow run \
  --bpmn bpmn/simple_dspy_dmn_example.bpmn \
  --process-id "SimpleDspyDmnExample" \
  --data '{"input_data": "Sample text for analysis"}'
```

### List Available Signatures
```bash
# List all DSPy signatures (including dynamic ones)
autotel dspy list-signatures

# List specific signature details
autotel dspy describe-signature "AnalyzeData"
```

### Test DSPy Signatures
```bash
# Test a DSPy signature directly
autotel dspy test-signature "AnalyzeData" --input '{"data": "Test data"}'
```

## Advanced Features

### 1. Dynamic Signature Creation
```python
# Signatures are automatically created from XML definitions
parser = DspyBpmnParser()
parser.add_bpmn_file("workflow.bpmn")

# Access dynamic signatures
signature_class = parser.get_dynamic_signature("AnalyzeData")
signature_def = parser.get_signature_definition("AnalyzeData")
```

### 2. Telemetry Integration
```python
# Full OTEL instrumentation for DSPy and DMN execution
with telemetry_manager.start_span("dspy.execute_signature") as span:
    span.set_attribute("dspy.signature_name", "AnalyzeData")
    span.set_attribute("dspy.input_data", str(input_data))
    
    result = dspy_registry.call_signature("AnalyzeData", **input_data)
    
    span.set_attribute("dspy.output_data", str(result))
```

### 3. Schema Validation
```python
# Validate DSPy inputs/outputs against LinkML schemas
validated_input = schema_validator.validate_dspy_input("AnalyzeData", input_data)
validated_output = schema_validator.validate_dspy_output("AnalyzeData", result)
```

### 4. Error Handling
```python
# Robust error handling for DSPy and DMN execution
try:
    dspy_result = dspy_registry.call_signature("AnalyzeData", **input_data)
    dmn_result = dmn_integration.execute_dmn_decision("Decision_1", dspy_result)
except Exception as e:
    # Handle errors gracefully with telemetry
    telemetry_manager.record_exception(e)
    # Fallback to default decision logic
```

## Best Practices

### 1. DSPy Signature Design
- **Clear Inputs/Outputs**: Define precise input and output fields
- **Descriptive Names**: Use meaningful names for signatures and fields
- **Type Consistency**: Ensure DSPy output types match DMN input types
- **Error Handling**: Design signatures to handle edge cases

### 2. DMN Decision Table Design
- **DSPy Input Mapping**: Clearly map DSPy outputs to DMN inputs
- **Comprehensive Rules**: Cover all possible scenarios
- **Default Rules**: Include catch-all rules for unexpected cases
- **Rule Ordering**: Order rules from most specific to most general

### 3. Workflow Design
- **Data Flow**: Ensure clear data flow from DSPy to DMN
- **Error Recovery**: Include error handling and recovery paths
- **Monitoring**: Add appropriate telemetry and logging
- **Testing**: Test both DSPy and DMN components independently

### 4. Performance Considerations
- **LLM Optimization**: Use appropriate LLM models and parameters
- **Caching**: Cache DSPy results when appropriate
- **Parallel Execution**: Execute independent DSPy tasks in parallel
- **Resource Management**: Monitor and manage LLM API usage

## Integration with AutoTel Framework

The DSPy + DMN integration is fully integrated with the AutoTel framework:

1. **Core Integration**: Uses AutoTel's telemetry, validation, and orchestration
2. **CLI Support**: Full command-line interface for workflow management
3. **Schema Validation**: LinkML-based validation for data integrity
4. **Observability**: Complete OTEL instrumentation for monitoring
5. **Extensibility**: Easy to extend with new DSPy signatures and DMN decisions

This integration enables AutoTel to support sophisticated AI-powered decision-making workflows while maintaining the reliability and observability of traditional business process management. 