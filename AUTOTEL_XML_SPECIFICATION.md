# AutoTel XML Specification
## Five Pillars XML Schema Definition

### Overview

This document defines the XML schemas for AutoTel's five pillars. All system definitions must conform to these schemas. **No embedded values, CDATA, or Python code is allowed.**

### 1. BPMN 2.0 (Workflow)

#### 1.1 Core BPMN Structure
```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  id="Definitions_1"
                  targetNamespace="http://autotel.ai/workflows">
  
  <bpmn:process id="CustomerProcess" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Customer">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="CustomerAnalysis" result="analysis_result">
          <dspy:param name="customer_data" value="input_data"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Risk Decision" 
                           camunda:decisionRef="RiskDecision">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
    </bpmn:businessRuleTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_3</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1"/>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="BusinessRuleTask_1"/>
    <bpmn:sequenceFlow id="Flow_3" sourceRef="BusinessRuleTask_1" targetRef="EndEvent_1"/>
  </bpmn:process>
</bpmn:definitions>
```

#### 1.2 BPMN Elements
- **Process:** `bpmn:process` with executable workflows
- **Tasks:** `bpmn:serviceTask`, `bpmn:businessRuleTask`, `bpmn:userTask`
- **Events:** `bpmn:startEvent`, `bpmn:endEvent`, `bpmn:intermediateEvent`
- **Gateways:** `bpmn:exclusiveGateway`, `bpmn:parallelGateway`
- **Flows:** `bpmn:sequenceFlow` with conditions
- **Extensions:** DSPy services and DMN decisions

### 2. DMN (Decisions)

#### 2.1 Decision Table Structure
```xml
<?xml version="1.0" encoding="UTF-8"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                 xmlns:dmndi="http://www.omg.org/spec/DMN/20191111/DMNDI/"
                 id="RiskDecision" name="Risk Assessment Decision">
  
  <dmn:decision id="RiskDecision" name="Risk Assessment">
    <dmn:decisionTable>
      <dmn:input id="risk_score" label="Risk Score" typeRef="double"/>
      <dmn:input id="customer_type" label="Customer Type" typeRef="string"/>
      <dmn:output id="approval_status" label="Approval Status" typeRef="string"/>
      <dmn:output id="risk_level" label="Risk Level" typeRef="string"/>
      
      <dmn:rule id="rule1">
        <dmn:inputEntry>&lt;= 0.3</dmn:inputEntry>
        <dmn:inputEntry>"premium"</dmn:inputEntry>
        <dmn:outputEntry>"approved"</dmn:outputEntry>
        <dmn:outputEntry>"low"</dmn:outputEntry>
      </dmn:rule>
      
      <dmn:rule id="rule2">
        <dmn:inputEntry>[0.31..0.7]</dmn:inputEntry>
        <dmn:inputEntry>"standard"</dmn:inputEntry>
        <dmn:outputEntry>"review"</dmn:outputEntry>
        <dmn:outputEntry>"medium"</dmn:outputEntry>
      </dmn:rule>
      
      <dmn:rule id="rule3">
        <dmn:inputEntry>&gt; 0.7</dmn:inputEntry>
        <dmn:inputEntry>-</dmn:inputEntry>
        <dmn:outputEntry>"rejected"</dmn:outputEntry>
        <dmn:outputEntry>"high"</dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>
```

#### 2.2 DMN Elements
- **Decision:** `dmn:decision` with decision tables
- **Inputs:** `dmn:input` with data types and labels
- **Outputs:** `dmn:output` with data types and labels
- **Rules:** `dmn:rule` with input/output entries
- **Expressions:** `dmn:inputEntry`, `dmn:outputEntry`

### 3. DSPy (AI Services)

#### 3.1 Signature Definitions
```xml
<dspy:signatures>
  <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
    <dspy:input name="customer_data" 
                description="Customer information" 
                type="object"
                shaclShape="http://autotel.ai/shapes#CustomerShape"/>
    <dspy:output name="risk_assessment" 
                 description="Risk analysis result"
                 type="object"
                 shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
  </dspy:signature>
</dspy:signatures>
```

#### 3.2 Signature Elements
- **Inputs:** `dspy:input` with name, description, type, and SHACL shape
- **Outputs:** `dspy:output` with name, description, type, and SHACL shape
- **Modules:** `dspy:module` for DSPy module configuration
- **Models:** `dspy:model` for AI model configuration

### 4. SHACL (Data Validation)

#### 4.1 Shape Definitions
```xml
<rdf:RDF xmlns="http://autotel.ai/shapes#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
  
  <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
    <sh:targetClass rdf:resource="http://autotel.ai/ontology#Customer"/>
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
```

#### 4.2 Constraint Types
- **Cardinality:** minCount, maxCount, qualifiedMinCount, qualifiedMaxCount
- **Data Types:** datatype, nodeKind, class
- **Value Constraints:** minLength, maxLength, pattern, uniqueLang
- **Range Constraints:** minInclusive, maxInclusive, minExclusive, maxExclusive
- **Logical:** and, or, not, xone
- **Comparison:** equals, disjoint, lessThan, lessThanOrEquals

### 5. OWL (Ontology)

#### 5.1 Core OWL Structure
```xml
<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
  
  <owl:Ontology rdf:about="http://autotel.ai/ontology">
    <rdfs:label>Domain Ontology</rdfs:label>
    <rdfs:comment>Domain-specific ontology for AI system</rdfs:comment>
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
```

#### 5.2 Ontology Elements
- **Classes:** `owl:Class` with labels, comments, and properties
- **Object Properties:** `owl:ObjectProperty` for relationships
- **Data Properties:** `owl:DataProperty` for attributes
- **Individuals:** `owl:NamedIndividual` for instances
- **Axioms:** `owl:SubClassOf`, `owl:EquivalentClass`, etc.

### 6. Complete Example

#### 6.1 Five Pillars Integration
```xml
<?xml version="1.0" encoding="UTF-8"?>
<autotel:system xmlns:autotel="http://autotel.ai/system"
                xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                xmlns:owl="http://www.w3.org/2002/07/owl#"
                xmlns:shacl="http://www.w3.org/ns/shacl#"
                xmlns:dspy="http://autotel.ai/dspy"
                xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">

  <!-- BPMN Workflow -->
  <bpmn:workflow>
    <bpmn:definitions>
      <bpmn:process id="CustomerProcess" isExecutable="true">
        <bpmn:startEvent id="StartEvent_1" name="Start"/>
        <bpmn:serviceTask id="ServiceTask_1" name="Analyze Customer">
          <bpmn:extensionElements>
            <dspy:service name="CustomerAnalysis" result="analysis_result"/>
          </bpmn:extensionElements>
        </bpmn:serviceTask>
        <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Risk Decision" 
                               camunda:decisionRef="RiskDecision"/>
        <bpmn:endEvent id="EndEvent_1" name="End"/>
      </bpmn:process>
    </bpmn:definitions>
  </bpmn:workflow>

  <!-- DMN Decision -->
  <dmn:decision>
    <dmn:definitions>
      <dmn:decision id="RiskDecision" name="Risk Assessment">
        <dmn:decisionTable>
          <dmn:input id="risk_score" label="Risk Score" typeRef="double"/>
          <dmn:output id="approval_status" label="Approval Status" typeRef="string"/>
          <dmn:rule id="rule1">
            <dmn:inputEntry>&lt;= 0.3</dmn:inputEntry>
            <dmn:outputEntry>"approved"</dmn:outputEntry>
          </dmn:rule>
        </dmn:decisionTable>
      </dmn:decision>
    </dmn:definitions>
  </dmn:decision>

  <!-- OWL Ontology -->
  <owl:ontology>
    <rdf:RDF>
      <owl:Ontology rdf:about="http://autotel.ai/ontology">
        <rdfs:label>Customer Analysis Ontology</rdfs:label>
      </owl:Ontology>
      
      <owl:Class rdf:about="http://autotel.ai/ontology#Customer">
        <rdfs:label>Customer</rdfs:label>
      </owl:Class>
      
      <owl:Class rdf:about="http://autotel.ai/ontology#RiskAssessment">
        <rdfs:label>Risk Assessment</rdfs:label>
      </owl:Class>
    </rdf:RDF>
  </owl:ontology>

  <!-- SHACL Shapes -->
  <shacl:shapes>
    <rdf:RDF>
      <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
        <sh:targetClass rdf:resource="http://autotel.ai/ontology#Customer"/>
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

  <!-- DSPy Signatures -->
  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
      <dspy:input name="customer_data" 
                  description="Customer information" 
                  type="object"
                  shaclShape="http://autotel.ai/shapes#CustomerShape"/>
      <dspy:output name="risk_assessment" 
                   description="Risk analysis result"
                   type="object"
                   shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
    </dspy:signature>
  </dspy:signatures>

</autotel:system>
```

#### 2.2 Constraint Types
- **Cardinality:** minCount, maxCount, qualifiedMinCount, qualifiedMaxCount
- **Data Types:** datatype, nodeKind, class
- **Value Constraints:** minLength, maxLength, pattern, uniqueLang
- **Range Constraints:** minInclusive, maxInclusive, minExclusive, maxExclusive
- **Logical:** and, or, not, xone
- **Comparison:** equals, disjoint, lessThan, lessThanOrEquals

### 3. DSPy (AI Reasoning)

#### 3.1 Signature Definitions
```xml
<dspy:config xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
    <dspy:input name="customer_data" 
                description="Customer information" 
                type="object"
                shaclShape="http://autotel.ai/shapes#CustomerShape"/>
    <dspy:output name="risk_assessment" 
                 description="Risk analysis result"
                 type="object"
                 shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
  </dspy:signature>
  
  <dspy:module type="predict">
    <dspy:config key="temperature" value="0.7"/>
  </dspy:module>
  
  <dspy:model provider="openai" name="gpt-4">
    <dspy:parameter key="temperature" value="0.7"/>
    <dspy:parameter key="max_tokens" value="1000"/>
  </dspy:model>
</dspy:config>
```

#### 3.2 Signature Elements
- **Inputs:** `dspy:input` with name, description, type, and SHACL shape
- **Outputs:** `dspy:output` with name, description, type, and SHACL shape
- **Modules:** `dspy:module` for DSPy module configuration
- **Models:** `dspy:model` for AI model configuration

### 4. Complete Example

#### 4.1 Three Pillars Integration
```xml
<?xml version="1.0" encoding="UTF-8"?>
<autotel:system xmlns:autotel="http://autotel.ai/system"
                xmlns:owl="http://www.w3.org/2002/07/owl#"
                xmlns:shacl="http://www.w3.org/ns/shacl#"
                xmlns:dspy="http://autotel.ai/dspy"
                xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">

  <!-- OWL Ontology -->
  <owl:ontology>
    <rdf:RDF>
      <owl:Ontology rdf:about="http://autotel.ai/ontology">
        <rdfs:label>Customer Analysis Ontology</rdfs:label>
      </owl:Ontology>
      
      <owl:Class rdf:about="http://autotel.ai/ontology#Customer">
        <rdfs:label>Customer</rdfs:label>
      </owl:Class>
      
      <owl:Class rdf:about="http://autotel.ai/ontology#RiskAssessment">
        <rdfs:label>Risk Assessment</rdfs:label>
      </owl:Class>
    </rdf:RDF>
  </owl:ontology>

  <!-- SHACL Shapes -->
  <shacl:shapes>
    <rdf:RDF>
      <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
        <sh:targetClass rdf:resource="http://autotel.ai/ontology#Customer"/>
        <sh:property>
          <sh:PropertyShape>
            <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
            <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
            <sh:minCount>1</sh:minCount>
          </sh:PropertyShape>
        </sh:property>
      </sh:NodeShape>
      
      <sh:NodeShape rdf:about="http://autotel.ai/shapes#RiskAssessmentShape">
        <sh:targetClass rdf:resource="http://autotel.ai/ontology#RiskAssessment"/>
        <sh:property>
          <sh:PropertyShape>
            <sh:path rdf:resource="http://autotel.ai/shapes#riskScore"/>
            <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
            <sh:minInclusive>0.0</sh:minInclusive>
            <sh:maxInclusive>1.0</sh:maxInclusive>
          </sh:PropertyShape>
        </sh:property>
      </sh:NodeShape>
    </rdf:RDF>
  </shacl:shapes>

  <!-- DSPy Signatures -->
  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
      <dspy:input name="customer_data" 
                  description="Customer information" 
                  type="object"
                  shaclShape="http://autotel.ai/shapes#CustomerShape"/>
      <dspy:output name="risk_assessment" 
                   description="Risk analysis result"
                   type="object"
                   shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
    </dspy:signature>
  </dspy:signatures>

  <!-- DSPy Configuration -->
  <dspy:config>
    <dspy:module type="predict">
      <dspy:config key="temperature" value="0.7"/>
    </dspy:module>
    
    <dspy:model provider="openai" name="gpt-4">
      <dspy:parameter key="temperature" value="0.7"/>
      <dspy:parameter key="max_tokens" value="1000"/>
    </dspy:model>
  </dspy:config>

</autotel:system>
```

### 7. Validation Rules

#### 7.1 XML Schema Validation
- All XML must validate against the defined schemas
- No CDATA sections allowed
- No embedded Python code allowed
- All namespaces must be properly declared

#### 5.2 Cross-Reference Validation
- DSPy signatures must reference valid SHACL shapes
- OWL classes must be referenced by SHACL target classes
- All URIs must be valid and consistent
- All IDs must be unique within the document

#### 5.3 Semantic Validation
- SHACL shapes must be valid RDF/XML
- OWL ontologies must be valid RDF/XML
- DSPy signatures must have valid input/output definitions
- All references between pillars must be valid

### 6. Best Practices

#### 6.1 Structure
- Use descriptive names for all elements
- Group related elements together
- Use consistent naming conventions
- Document complex logic with comments

#### 6.2 Validation
- Define SHACL shapes for all data structures
- Use OWL ontologies for domain modeling
- Create comprehensive DSPy signatures
- Test all validation rules

#### 6.3 Integration
- Ensure all three pillars work together
- Validate cross-references between pillars
- Test complete pipeline execution
- Monitor telemetry generation

### 7. Runtime Data Flow

#### 7.1 Input Data Provision
Input data is provided at runtime, not in XML:

```bash
# CLI command with input data
autotel pipeline execute --owl ontology.owl --shacl shapes.shacl --dspy signatures.dspy --data '{"customer_id": "CUST_12345", "name": "John Doe"}'

# Or via JSON file
autotel pipeline execute --owl ontology.owl --shacl shapes.shacl --dspy signatures.dspy --input input_data.json
```

#### 7.2 Pipeline Context
The pipeline context contains all runtime data:
- Input parameters from CLI
- Intermediate results from processors
- Output from DSPy services
- Validation results from SHACL
- Ontology context from OWL

#### 7.3 Data Flow Example
1. **Input:** `{"customer_id": "CUST_12345", "name": "John Doe"}`
2. **SHACL Validation:** Validates customer_id against CustomerShape
3. **OWL Processing:** Provides semantic context for customer data
4. **DSPy Execution:** Processes customer data, returns risk assessment
5. **Output:** `{"risk_score": 0.3, "recommendation": "Low risk customer"}`

---

*This specification defines the complete XML structure for AutoTel's three pillars. All system definitions must conform to these schemas to ensure proper execution and telemetry generation. Input data is provided at runtime, not embedded in XML.* 