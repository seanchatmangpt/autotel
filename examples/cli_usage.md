# AutoTel CLI Usage Guide

## Overview
AutoTel is a CLI-only workflow orchestration system where all logic is defined in XML. Users interact exclusively through the command line interface.

## Basic Commands

### Run a Workflow
```bash
# Run a workflow with input data
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json

# Run with configuration file
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --config examples/autotel_config.yaml

# Run with specific model override
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --model gpt4

# Run with verbose output
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --verbose

# Run and save results to file
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --output results.json
```

### Validate XML Files
```bash
# Validate a BPMN workflow file
autotel validate --workflow examples/ideal_workflow.bpmn

# Validate multiple files
autotel validate --workflow examples/ideal_workflow.bpmn --dmn examples/decisions.dmn

# Show validation details
autotel validate --workflow examples/ideal_workflow.bpmn --detailed
```

### Get Help and Examples
```bash
# Show general help
autotel --help

# Show command-specific help
autotel run --help
autotel validate --help

# Show examples
autotel examples

# Show workflow examples
autotel examples --workflow

# Show DMN examples
autotel examples --dmn
```

## Example Workflow Execution

### 1. Customer Feedback Processing
```bash
# Run the customer feedback workflow
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json
```

**Expected Output:**
```
🚀 Starting Customer Feedback Processing workflow...

📊 DSPy Analysis Results:
   • Sentiment: negative
   • Confidence: 0.89
   • Key Topics: [dashboard, data, service, subscription]
   • Urgency Score: 9
   • Category: bug

📋 DMN Decision Results:
   • Route: immediate_escalation
   • Priority: critical
   • Escalation Level: executive

✅ Workflow completed successfully
   • Final Task: Executive Escalation
   • Duration: 2.3s
   • Status: completed
```

### 2. Workflow with Different Input
```bash
# Create a positive feedback input
echo '{
  "feedback_text": "The new dashboard is amazing! I love the new features and it loads so much faster now. Great work team!",
  "customer_id": "CUST-2024-002",
  "product_id": "PROD-DASH-001"
}' > positive_feedback.json

# Run workflow with positive feedback
autotel run --workflow examples/ideal_workflow.bpmn --input positive_feedback.json
```

**Expected Output:**
```
🚀 Starting Customer Feedback Processing workflow...

📊 DSPy Analysis Results:
   • Sentiment: positive
   • Confidence: 0.85
   • Key Topics: [dashboard, features, team]
   • Urgency Score: 2
   • Category: praise

📋 DMN Decision Results:
   • Route: customer_success
   • Priority: low
   • Escalation Level: none

✅ Workflow completed successfully
   • Final Task: Customer Success Follow-up
   • Duration: 1.8s
   • Status: completed
```

## Error Handling

### Invalid XML
```bash
autotel run --workflow invalid_workflow.bpmn --input data.json
```
**Output:**
```
❌ Validation Error: Invalid BPMN XML
   • Line 15: Missing required attribute 'id' in element 'bpmn:process'
   • Line 23: Invalid namespace prefix 'unknown'

💡 Tip: Run 'autotel validate --workflow invalid_workflow.bpmn' for detailed validation
```

### Missing Input Data
```bash
autotel run --workflow examples/ideal_workflow.bpmn --input missing.json
```
**Output:**
```
❌ Input Error: Could not read input file 'missing.json'
   • File does not exist
   • Check file path and permissions

💡 Tip: Ensure input JSON file exists and is readable
```

### Workflow Execution Error
```bash
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json
```
**Output:**
```
❌ Execution Error: DSPy signature 'CustomerFeedbackAnalyzer' not found
   • Available signatures: [interpret_otel_spans, analyze_process, diagnose_error]
   • Check signature definition in XML

💡 Tip: Ensure all DSPy signatures are properly defined in the BPMN file
```

## Advanced Usage

### Debug Mode
```bash
# Run with debug information
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --debug

# Show step-by-step execution
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --step-by-step
```

### Batch Processing
```bash
# Process multiple input files
autotel run --workflow examples/ideal_workflow.bpmn --input-dir feedback_data/ --output-dir results/

# Process with different configurations
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --config production.json
```

### Workflow Templates
```bash
# Generate a new workflow template
autotel template --type customer-feedback --output my_workflow.bpmn

# List available templates
autotel template --list

# Show template details
autotel template --type customer-feedback --show
```

## Configuration

### Environment Variables
```bash
# Set DSPy model
export AUTOTEL_DSPY_MODEL="ollama/qwen2.5:latest"

# Set log level
export AUTOTEL_LOG_LEVEL="INFO"

# Set output format
export AUTOTEL_OUTPUT_FORMAT="json"
```

### Configuration File
```bash
# Use custom configuration
autotel run --workflow examples/ideal_workflow.bpmn --input examples/sample_input.json --config autotel.yaml
```

**Example `autotel.yaml`:**
```yaml
dspy:
  model: "ollama/qwen2.5:latest"
  temperature: 0.7
  max_tokens: 1000

logging:
  level: "INFO"
  format: "json"

output:
  format: "json"
  include_debug: false
  save_intermediate: true
```

## Best Practices

1. **Always validate XML files before running workflows**
2. **Use descriptive names for workflows and tasks**
3. **Test with small input data first**
4. **Use version control for your XML files**
5. **Document your workflow logic in XML comments**
6. **Use consistent naming conventions for variables** 