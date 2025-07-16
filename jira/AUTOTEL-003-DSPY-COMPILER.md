# AUTOTEL-003: Ontology Compiler Implementation

## 🎯 **TELEMETRY IMPLEMENTATION REQUIREMENTS**

### **Required Spans and Events**
The following telemetry must be implemented to validate real functionality:

#### **Main Compilation Span**
```yaml
span_name: "ontology.compiler.compile"
required_attributes:
  - component: "ontology_compiler"
  - operation: "compile"
  - input_format: "OWLOntologyDefinition"
  - output_format: "OntologySchema"
  - success: boolean
  - classes_compiled: integer
  - properties_compiled: integer
  - semantic_context_generated: boolean
  - examples_generated: integer
  - compilation_duration_ms: integer
  - input_classes_count: integer
  - output_schemas_count: integer

required_events:
  - "compilation_started"
  - "classes_compiled"
  - "properties_compiled"
  - "semantic_context_generated"
  - "examples_generated"
```

#### **Compilation Method Spans**
```yaml
span_name: "ontology.compiler.extract_classes"
required_attributes:
  - classes_processed: integer
  - semantic_types_mapped: dict
  - hierarchies_detected: integer
  - restrictions_processed: integer
  - class_schemas_created: integer

span_name: "ontology.compiler.extract_properties"
required_attributes:
  - object_properties_processed: integer
  - data_properties_processed: integer
  - domains_mapped: integer
  - ranges_mapped: integer
  - property_schemas_created: integer

span_name: "ontology.compiler.create_ontology_class"
required_attributes:
  - class_name: string
  - semantic_type: string
  - properties_count: integer
  - superclasses_count: integer
  - restrictions_count: integer
```

### **Dynamic Data Validation**
The following dynamic data must be generated from real OWL compilation:

#### **Expected Dynamic Data from Sample OWL Input**
```yaml
# Input: OWLOntologyDefinition
input:
  ontology_uri: "http://example.org/ontology"
  prefix: "test"
  namespace: "http://example.org/"
  classes:
    UserInput:
      uri: "#UserInput"
      name: "UserInput"
      label: "User Input"
      comment: "Input data from user"
      semantic_type: "user_input"
      properties: []
      superclasses: []
      restrictions: []
    Recommendation:
      uri: "#Recommendation"
      name: "Recommendation"
      label: "Recommendation"
      comment: "AI-generated recommendation"
      semantic_type: "recommendation"
      properties: []
      superclasses: []
      restrictions: []
```

#### **Expected Telemetry Data**
```yaml
# Span: ontology.compiler.compile
attributes:
  success: true
  classes_compiled: 2
  properties_compiled: 0
  semantic_context_generated: true
  examples_generated: 0
  compilation_duration_ms: 45
  input_classes_count: 2
  output_schemas_count: 2

# Span: ontology.compiler.extract_classes
attributes:
  classes_processed: 2
  semantic_types_mapped:
    UserInput: "user_input"
    Recommendation: "recommendation"
  hierarchies_detected: 0
  restrictions_processed: 0
  class_schemas_created: 2

# Span: ontology.compiler.create_ontology_class (UserInput)
attributes:
  class_name: "UserInput"
  semantic_type: "user_input"
  properties_count: 0
  superclasses_count: 0
  restrictions_count: 0

# Span: ontology.compiler.create_ontology_class (Recommendation)
attributes:
  class_name: "Recommendation"
  semantic_type: "recommendation"
  properties_count: 0
  superclasses_count: 0
  restrictions_count: 0

# Expected OntologySchema output
output:
  ontology_uri: "http://example.org/ontology"
  namespace: "http://example.org/"
  prefix: "test"
  classes:
    UserInput:
      name: "UserInput"
      uri: "#UserInput"
      semantic_type: "user_input"
      properties: {}
      superclasses: []
      description: "Input data from user"
    Recommendation:
      name: "Recommendation"
      uri: "#Recommendation"
      semantic_type: "recommendation"
      properties: {}
      superclasses: []
      description: "AI-generated recommendation"
  semantic_context:
    user_input_classes: ["UserInput"]
    recommendation_classes: ["Recommendation"]
    decision_classes: []
    analysis_classes: []
    reasoning_classes: []
    general_classes: []
  examples: []
```

### **Validation Criteria**
- **NO HARDCODED VALUES**: All telemetry data must be generated from actual OWL compilation
- **REAL COMPILATION**: OWL data must be transformed into semantic schemas
- **DYNAMIC MAPPING**: All counts, types, and schemas must come from input data
- **SEMANTIC CLASSIFICATION**: Semantic types must be preserved from OWL input
- **ERROR HANDLING**: Failed compilation must generate error spans with context
- **PERFORMANCE TRACKING**: Compilation duration must be measured and reported
- **DATA INTEGRITY**: Input/output counts must match and be accurate

---

## Summary
Implement the Ontology Compiler component of the AutoTel semantic execution pipeline to transform OWL ontology definitions into semantic schemas for DSPy signature generation.

## Description
The Ontology Compiler is the first compiler stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It takes structured OWL ontology definitions and compiles them into semantic schemas that will be used by the DSPy compiler to generate context-aware signatures.

## Acceptance Criteria

### Functional Requirements
- [ ] Compile OWL ontology definitions into structured schemas
- [ ] Classify OWL classes by semantic types (user_input, recommendation, decision, analysis, reasoning, general)
- [ ] Extract class properties and map to schema fields
- [ ] Generate semantic context from ontology relationships
- [ ] Extract OWL individuals as example data
- [ ] Preserve ontology metadata (URI, namespace, prefix)
- [ ] Support property inheritance and class hierarchies
- [ ] Handle data type mappings from OWL to Python types

### Technical Requirements
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Use structured dataclasses for all outputs
- [ ] Support semantic type-based classification
- [ ] Handle property inheritance and class hierarchies
- [ ] Generate semantic context from ontology relationships
- [ ] Maintain ontology metadata for downstream use

### Data Structures
```python
@dataclass
class OntologySchema:
    ontology_uri: str
    namespace: str
    prefix: str
    classes: Dict[str, ClassSchema]
    semantic_context: Dict[str, Any]
    examples: List[Dict[str, Any]]

@dataclass
class ClassSchema:
    name: str
    uri: str
    semantic_type: str  # user_input, recommendation, decision, analysis, reasoning, general
    properties: Dict[str, PropertySchema]
    superclasses: List[str]
    description: str

@dataclass
class PropertySchema:
    name: str
    uri: str
    data_type: str
    domain: str
    range: str
    cardinality: Optional[str]
```

## Implementation Notes

### Semantic Type Classification
The compiler should automatically classify OWL classes based on:
- **user_input**: Classes with input-related keywords (input, user, request, etc.)
- **recommendation**: Classes with recommendation keywords (recommend, suggest, advice, etc.)
- **decision**: Classes with decision keywords (decision, choice, select, etc.)
- **analysis**: Classes with analysis keywords (analyze, evaluate, assess, etc.)
- **reasoning**: Classes with reasoning keywords (reason, logic, inference, etc.)
- **general**: Default for classes without specific semantic indicators

### Schema Generation
- Extract class properties as field schemas
- Map OWL data types to Python types
- Include semantic context and descriptions
- Preserve property relationships and hierarchies

### Semantic Context Generation
- Extract ontology URI and namespace information
- List available classes and properties
- Preserve semantic type classifications
- Include property relationships and hierarchies

### Integration Points
- **Input**: OWLOntologyDefinition from OWL processor
- **Output**: OntologySchema for DSPy compiler consumption
- **Dependencies**: OWL processor
- **No Dependencies**: SHACL, DSPy, BPMN, DMN (prototype phase only)

## Test Cases

### Basic Compilation
```python
# Test compilation from OWL processor output
ontology = owl_processor.parse_ontology_definition(xml_fragment, "test")

schema = ontology_compiler.compile(ontology=ontology)
```

### Semantic Type Classification
- Test user_input classes classified correctly
- Test recommendation classes classified correctly
- Test decision classes classified correctly
- Test analysis classes classified correctly
- Test reasoning classes classified correctly
- Test general classes as default

### Schema Generation
- Test property extraction and mapping
- Test data type conversion
- Test semantic context preservation
- Test example extraction from individuals
- Test class hierarchy handling

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with DSPy compiler pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No SHACL/DSPy/BPMN/DMN dependencies in prototype phase

## Story Points
**5** - Medium complexity due to ontology schema generation and semantic classification

## Priority
**High** - Foundation compiler component for the processor > compiler > linker > executor pipeline

## Labels
- `compiler`
- `ontology`
- `semantic`
- `prototype`
- `schema`