# AutoTel XML Specification
## Five Pillars XML Schema Definition

### Overview

This document defines the XML schemas for AutoTel's five pillars. All workflow definitions must conform to these schemas. **No embedded values, CDATA, or Python code is allowed.**

### 1. BPMN (Process Orchestration)

#### 1.1 Core BPMN Structure
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  xmlns:shacl="http://autotel.ai/shacl"
                  xmlns:owl="http://autotel.ai/owl"
                  targetNamespace="http://autotel.ai/workflows">
  
  <!-- Five Pillars Definitions -->
  <dspy:signatures>...</dspy:signatures>
  <shacl:shapes>...</shacl:shapes>
  <owl:ontology>...</owl:ontology>
  <dmn:definitions>...</dmn:definitions>
  
  <!-- BPMN Process -->
  <bpmn:process id="process_id" isExecutable="true">
    <!-- Process elements -->
  </bpmn:process>
</bpmn:definitions>
```

#### 1.2 Process Elements
- **Start Events:** `bpmn:startEvent`
- **End Events:** `bpmn:endEvent`
- **Tasks:** `bpmn:serviceTask`, `bpmn:userTask`, `bpmn:businessRuleTask`
- **Gateways:** `bpmn:exclusiveGateway`, `bpmn:parallelGateway`
- **Sequence Flows:** `bpmn:sequenceFlow`

### 2. DSPy (AI Reasoning)

#### 2.1 Signature Definitions
```xml
<dspy:signatures>
  <dspy:signature name="SignatureName" description="Signature description">
    <dspy:input name="input_name" 
                description="Input description" 
                optional="false"
                shaclShape="http://autotel.ai/shapes#InputShape"/>
    <dspy:output name="output_name" 
                 description="Output description"
                 shaclShape="http://autotel.ai/shapes#OutputShape"/>
  </dspy:signature>
</dspy:signatures>
```

#### 2.2 Service Task Integration
```xml
<bpmn:serviceTask id="task_id" name="Task Name">
  <bpmn:extensionElements>
    <dspy:service name="service_name" 
                  signature="SignatureName" 
                  result="result_variable">
      <dspy:param name="param_name" value="param_value"/>
    </dspy:service>
  </bpmn:extensionElements>
</bpmn:serviceTask>
```

### 3. SHACL (Data Validation)

#### 3.1 Shape Definitions
```xml
<shacl:shapes>
  <!-- RDF/XML format SHACL shapes -->
  <rdf:RDF xmlns="http://autotel.ai/shapes#"
           xmlns:sh="http://www.w3.org/ns/shacl#"
           xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
           xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    
    <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
      <sh:property>
        <sh:PropertyShape>
          <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
          <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
          <sh:minCount>1</sh:minCount>
          <sh:pattern>^CUST_[0-9]+$</sh:pattern>
          <sh:message>Customer ID must follow pattern CUST_XXXXX</sh:message>
        </sh:PropertyShape>
      </sh:property>
    </sh:NodeShape>
  </rdf:RDF>
</shacl:shapes>
```

#### 3.2 Validation Task Integration
```xml
<bpmn:serviceTask id="validation_task" name="Validate Data">
  <bpmn:extensionElements>
    <shacl:shaclValidator dataSource="task_data" 
                         dataVariable="input_data"
                         shapesSource="inline"
                         resultVariable="validation_result"
                         failOnViolation="true"/>
  </bpmn:extensionElements>
</bpmn:serviceTask>
```

### 4. OWL (Ontology)

#### 4.1 Ontology Definitions
```xml
<owl:ontology>
  <rdf:RDF xmlns="http://autotel.ai/ontology#"
           xmlns:owl="http://www.w3.org/2002/07/owl#"
           xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
           xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology">
      <rdfs:label>Domain Ontology</rdfs:label>
      <rdfs:comment>Domain-specific ontology</rdfs:comment>
    </owl:Ontology>
    
    <owl:Class rdf:about="http://autotel.ai/ontology#Customer">
      <rdfs:label>Customer</rdfs:label>
      <rdfs:comment>A customer entity</rdfs:comment>
    </owl:Class>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology#hasCustomer">
      <rdfs:label>hasCustomer</rdfs:label>
      <rdfs:domain rdf:resource="http://autotel.ai/ontology#LoanApplication"/>
      <rdfs:range rdf:resource="http://autotel.ai/ontology#Customer"/>
    </owl:ObjectProperty>
  </rdf:RDF>
</owl:ontology>
```

#### 4.2 Ontology Validation Task
```xml
<bpmn:serviceTask id="ontology_task" name="Validate Ontology">
  <bpmn:extensionElements>
    <owl:ontologyValidator dataSource="task_data"
                          dataVariable="input_data"
                          ontologySource="inline"
                          resultVariable="ontology_result"
                          failOnViolation="false"/>
  </bpmn:extensionElements>
</bpmn:serviceTask>
```

### 5. DMN (Business Rules)

#### 5.1 Decision Table Definitions
```xml
<dmn:definitions>
  <dmn:decision id="DecisionId" name="Decision Name">
    <dmn:decisionTable id="DecisionTableId">
      <dmn:input id="input_id" label="Input Label">
        <dmn:inputExpression id="input_expr_id" typeRef="string">
          <dmn:text>input_variable</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output id="output_id" label="Output Label" typeRef="boolean"/>
      
      <dmn:rule id="rule_id">
        <dmn:inputEntry id="input_entry_id">
          <dmn:text>"LOW"</dmn:text>
        </dmn:inputEntry>
        <dmn:outputEntry id="output_entry_id">
          <dmn:text>true</dmn:text>
        </dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>
```

**Note:** The DMN decision table contains business rule logic that defines the decision criteria. The input values in rules (like `"LOW"`) represent the business logic conditions, not hardcoded input data. The actual input data comes from the workflow context at runtime.

#### 5.2 Business Rule Task Integration
```xml
<bpmn:businessRuleTask id="decision_task" 
                      name="Apply Business Rules"
                      camunda:decisionRef="DecisionId"/>
```

### 6. Complete Example

#### 6.1 Five Pillars Integration
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://autotel.ai/shacl"
                  xmlns:owl="http://autotel.ai/owl"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  targetNamespace="http://autotel.ai/workflows">

  <!-- DSPy Signatures -->
  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
      <dspy:input name="customer_data" description="Customer information" 
                  shaclShape="http://autotel.ai/shapes#CustomerShape"/>
      <dspy:output name="risk_assessment" description="Risk analysis result"
                   shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
    </dspy:signature>
  </dspy:signatures>

  <!-- SHACL Shapes -->
  <shacl:shapes>
    <rdf:RDF xmlns="http://autotel.ai/shapes#"
             xmlns:sh="http://www.w3.org/ns/shacl#"
             xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
        <sh:property>
          <sh:PropertyShape>
            <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
            <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
            <sh:minCount>1</sh:minCount>
          </sh:PropertyShape>
        </sh:property>
      </sh:NodeShape>
    </rdf:RDF>
  </shacl:shapes>

  <!-- OWL Ontology -->
  <owl:ontology>
    <rdf:RDF xmlns="http://autotel.ai/ontology#"
             xmlns:owl="http://www.w3.org/2002/07/owl#"
             xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <owl:Class rdf:about="http://autotel.ai/ontology#Customer">
        <rdfs:label>Customer</rdfs:label>
      </owl:Class>
    </rdf:RDF>
  </owl:ontology>

  <!-- DMN Decision -->
  <dmn:definitions>
    <dmn:decision id="ApprovalDecision" name="Approval Decision">
      <dmn:decisionTable id="ApprovalTable">
        <dmn:input id="riskInput" label="Risk Level">
          <dmn:inputExpression id="riskExpr" typeRef="string">
            <dmn:text>risk_level</dmn:text>
          </dmn:inputExpression>
        </dmn:input>
        <dmn:output id="approvedOutput" label="Approved" typeRef="boolean"/>
        <dmn:rule id="rule1">
          <dmn:inputEntry id="input1"><dmn:text>"LOW"</dmn:text></dmn:inputEntry>
          <dmn:outputEntry id="output1"><dmn:text>true</dmn:text></dmn:outputEntry>
        </dmn:rule>
        <dmn:rule id="rule2">
          <dmn:inputEntry id="input2"><dmn:text>"MEDIUM"</dmn:text></dmn:inputEntry>
          <dmn:outputEntry id="output2"><dmn:text>true</dmn:text></dmn:outputEntry>
        </dmn:rule>
        <dmn:rule id="rule3">
          <dmn:inputEntry id="input3"><dmn:text>"HIGH"</dmn:text></dmn:inputEntry>
          <dmn:outputEntry id="output3"><dmn:text>false</dmn:text></dmn:outputEntry>
        </dmn:rule>
      </dmn:decisionTable>
    </dmn:decision>
  </dmn:definitions>

  <!-- BPMN Process -->
  <bpmn:process id="five_pillars_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    
    <!-- SHACL Validation -->
    <bpmn:serviceTask id="validate" name="Validate Input">
      <bpmn:extensionElements>
        <shacl:shaclValidator dataSource="task_data" 
                             dataVariable="input_data"
                             shapesSource="inline"
                             resultVariable="validation_result"/>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- DSPy AI Analysis -->
    <bpmn:serviceTask id="analyze" name="AI Analysis">
      <bpmn:extensionElements>
        <dspy:service name="customer_analyzer" 
                      signature="CustomerAnalysis" 
                      result="analysis_result">
          <dspy:param name="customer_data" value="input_data"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- DMN Decision -->
    <bpmn:businessRuleTask id="decide" 
                          name="Apply Rules"
                          camunda:decisionRef="ApprovalDecision"/>
    
    <bpmn:endEvent id="end" name="End"/>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="validate"/>
    <bpmn:sequenceFlow id="flow2" sourceRef="validate" targetRef="analyze"/>
    <bpmn:sequenceFlow id="flow3" sourceRef="analyze" targetRef="decide"/>
    <bpmn:sequenceFlow id="flow4" sourceRef="decide" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>
```

### 7. Validation Rules

#### 7.1 XML Schema Validation
- All XML must validate against the defined schemas
- No CDATA sections allowed
- No embedded Python code allowed
- All namespaces must be properly declared

#### 7.2 Business Logic vs. Input Data
- **Business Logic Values:** DMN rules contain business logic conditions (e.g., `"LOW"`, `"HIGH"`) that define decision criteria
- **Input Data:** Actual workflow data comes from runtime context, not from XML
- **No Hardcoded Inputs:** XML should not contain actual input data, only business logic definitions
- **Dynamic Execution:** All input data is provided at runtime through workflow context

#### 7.3 Cross-Reference Validation
- DSPy signatures must reference valid SHACL shapes
- DMN decisions must be referenced by business rule tasks
- OWL classes must be referenced by validation tasks
- All IDs must be unique within the document

#### 7.4 Semantic Validation
- SHACL shapes must be valid RDF/XML
- OWL ontologies must be valid RDF/XML
- DMN decision tables must have valid expressions
- BPMN processes must be executable

### 8. Best Practices

#### 8.1 Structure
- Use descriptive names for all elements
- Group related elements together
- Use consistent naming conventions
- Document complex logic with comments

#### 8.2 Validation
- Define SHACL shapes for all data structures
- Use OWL ontologies for domain modeling
- Create comprehensive DMN decision tables
- Test all validation rules

#### 8.3 Integration
- Ensure all five pillars work together
- Validate cross-references between pillars
- Test complete workflow execution
- Monitor telemetry generation

### 9. Runtime Data Flow

#### 9.1 Input Data Provision
Input data is provided at runtime, not in XML:

```bash
# CLI command with input data
autotel run workflow.bpmn --input '{"risk_level": "LOW", "customer_id": "CUST_12345"}'

# Or via JSON file
autotel run workflow.bpmn --input-file input_data.json
```

#### 9.2 Workflow Context
The workflow context contains all runtime data:
- Input parameters from CLI
- Intermediate results from tasks
- Output from DSPy services
- Decision results from DMN
- Validation results from SHACL/OWL

#### 9.3 Data Flow Example
1. **Input:** `{"risk_level": "LOW", "customer_id": "CUST_12345"}`
2. **SHACL Validation:** Validates customer_id against CustomerShape
3. **DSPy Analysis:** Processes customer data, returns risk assessment
4. **DMN Decision:** Applies business rules to risk assessment
5. **Output:** `{"approved": true, "interest_rate": 3.5}`

---

*This specification defines the complete XML structure for AutoTel's five pillars. All workflow definitions must conform to these schemas to ensure proper execution and telemetry generation. Input data is provided at runtime, not embedded in XML.* 