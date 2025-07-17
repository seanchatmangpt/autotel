# DMN Usage Guide for SpiffWorkflow

This guide explains how to use DMN (Decision Model and Notation) with the SpiffWorkflow engine through the spiff-example-cli.

## Overview

DMN allows you to define business rules in decision tables that can be executed during BPMN workflow execution. The spiff-example-cli provides a complete example of how to integrate DMN decision tables with BPMN workflows.

## Key Components

### 1. DMN Files
DMN files contain decision tables that define business rules. The spiff-example-cli includes two example DMN files:

- `bpmn/tutorial/product_prices.dmn` - Defines product pricing rules
- `bpmn/tutorial/shipping_costs.dmn` - Defines shipping cost rules

### 2. BPMN Integration
DMN decisions are integrated into BPMN workflows using `businessRuleTask` elements with `camunda:decisionRef` attributes.

### 3. Engine Integration
The SpiffWorkflow engine automatically loads and executes DMN decision tables when referenced in BPMN workflows.

## Setup and Usage

### Step 1: Install Dependencies
```bash
cd spiff-example-cli
pip install -r requirements.txt
```

### Step 2: Add Workflow with DMN
```bash
# Add a workflow that includes both BPMN and DMN files
python runner.py -e spiff_example.spiff.file add \
  -p customize_product \
  -b bpmn/camunda/call_activity.bpmn \
  -d bpmn/tutorial/product_prices.dmn
```

### Step 3: Run the Workflow
```bash
# Start the interactive workflow runner
python runner.py -e spiff_example.spiff.file
```

## DMN File Structure

### Example: product_prices.dmn
```xml
<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" 
             xmlns:dmndi="https://www.omg.org/spec/DMN/20191111/DMNDI/" 
             xmlns:dc="http://www.omg.org/spec/DMN/20180521/DC/" 
             id="product_price" name="DRD" 
             namespace="http://camunda.org/schema/1.0/dmn" 
             exporter="Camunda Modeler" exporterVersion="4.11.1">
  
  <decision id="product_prices" name="Product Prices">
    <decisionTable id="DecisionTable_0irsx4u">
      <!-- Input: product_name (string) -->
      <input id="Input_1">
        <inputExpression id="InputExpression_1" typeRef="string" expressionLanguage="python">
          <text>product_name</text>
        </inputExpression>
      </input>
      
      <!-- Output: product_price (long) -->
      <output id="Output_1" label="product_price" typeRef="long" />
      
      <!-- Decision Rules -->
      <rule id="DecisionRule_0yzfvwg">
        <description>Product A</description>
        <inputEntry id="UnaryTests_09p3f1m">
          <text>"product_a"</text>
        </inputEntry>
        <outputEntry id="LiteralExpression_04z7pbc">
          <text>15.00</text>
        </outputEntry>
      </rule>
      
      <rule id="DecisionRule_06r9cs6">
        <description>Product B</description>
        <inputEntry id="UnaryTests_081dqoi">
          <text>"product_b"</text>
        </inputEntry>
        <outputEntry id="LiteralExpression_09pcew9">
          <text>15.00</text>
        </outputEntry>
      </rule>
      
      <!-- Additional rules for other products... -->
    </decisionTable>
  </decision>
  
  <!-- Diagram information -->
  <dmndi:DMNDI>
    <dmndi:DMNDiagram>
      <dmndi:DMNShape dmnElementRef="product_prices">
        <dc:Bounds height="80" width="180" x="160" y="100" />
      </dmndi:DMNShape>
    </dmndi:DMNDiagram>
  </dmndi:DMNDI>
</definitions>
```

## BPMN Integration Pattern

### Business Rule Task Definition
```xml
<bpmn:businessRuleTask id="Activity_1x1kw47" 
                       name="Look Up Product Price" 
                       camunda:decisionRef="product_prices">
  <bpmn:incoming>Flow_1r5bppm</bpmn:incoming>
  <bpmn:outgoing>Flow_1gj4orb</bpmn:outgoing>
</bpmn:businessRuleTask>
```

### Key Integration Points

1. **Decision Reference**: The `camunda:decisionRef` attribute references the DMN decision ID
2. **Input Data**: The workflow provides input data (e.g., `product_name`) to the DMN decision
3. **Output Data**: The DMN decision returns output data (e.g., `product_price`) to the workflow

## Data Flow

### Input to DMN
- Workflow variables are automatically passed to DMN decisions
- Input expressions in DMN reference workflow variables
- Example: `product_name` from workflow → DMN input

### Output from DMN
- DMN decision results are automatically added to workflow data
- Output labels become workflow variables
- Example: DMN output `product_price` → workflow variable

## Advanced Usage

### Multiple DMN Files
```bash
# Load multiple DMN files for a single workflow
python runner.py -e spiff_example.spiff.file add \
  -p order_product \
  -b bpmn/tutorial/top_level.bpmn \
  -d bpmn/tutorial/product_prices.dmn bpmn/tutorial/shipping_costs.dmn
```

### Complex Decision Tables
DMN supports complex decision logic including:
- Multiple inputs and outputs
- Different hit policies (FIRST, UNIQUE, etc.)
- Complex expressions and conditions
- Business knowledge models

### Integration with AutoTel
The spiff-example-cli demonstrates the core DMN integration patterns that can be adapted for AutoTel:

1. **Parser Integration**: Load DMN files alongside BPMN files
2. **Decision Execution**: Execute DMN decisions during workflow execution
3. **Data Binding**: Automatically bind workflow data to DMN inputs/outputs
4. **Telemetry**: Track DMN decision execution for observability

## Testing DMN Decisions

### List Available Specs
```bash
python runner.py -e spiff_example.spiff.file list_specs
```

### Run Specific Workflow
```bash
python runner.py -e spiff_example.spiff.file run -s SPEC_ID
```

### Compare Workflow Specs
```bash
python runner.py -e spiff_example.spiff.file diff_spec -o ORIGINAL_SPEC -n NEW_SPEC
```

## Best Practices

1. **Naming Conventions**: Use descriptive names for DMN decisions and outputs
2. **Data Types**: Ensure input/output types match between BPMN and DMN
3. **Testing**: Test DMN decisions independently before workflow integration
4. **Documentation**: Document decision logic and business rules
5. **Versioning**: Version DMN files alongside BPMN workflows

## Troubleshooting

### Common Issues

1. **Decision Not Found**: Ensure DMN file is loaded and decision ID matches
2. **Type Mismatches**: Check input/output data types in DMN vs workflow
3. **Missing Data**: Verify workflow provides required input variables
4. **Parser Errors**: Validate DMN XML syntax and structure

### Debug Commands
```bash
# Enable detailed logging
export PYTHONPATH=.
python runner.py -e spiff_example.spiff.file add -p test_process -b test.bpmn -d test.dmn
```

## Integration with AutoTel Framework

The spiff-example-cli provides a foundation for integrating DMN into the AutoTel framework:

1. **Core Integration**: Use SpiffWorkflow's DMN parser and executor
2. **Telemetry**: Add OTEL instrumentation for DMN decision execution
3. **Validation**: Integrate with LinkML schema validation
4. **CLI Commands**: Add DMN-specific commands to AutoTel CLI
5. **Workflow Orchestration**: Coordinate DMN decisions with BPMN workflows

This integration enables AutoTel to support complex business rule execution while maintaining the observability and validation capabilities of the framework. 