# AUTOTEL-002: SHACL Processor Implementation

## 🎯 **TELEMETRY IMPLEMENTATION REQUIREMENTS**

### **Required Spans and Events**
The following telemetry must be implemented to validate real functionality:

#### **Main Processing Span**
```yaml
span_name: "shacl.processor.parse"
required_attributes:
  - component: "shacl_processor"
  - operation: "parse"
  - input_format: "shacl_xml"
  - output_format: "Graph"
  - success: boolean
  - node_shapes_extracted: integer
  - property_shapes_extracted: integer
  - constraints_extracted: integer
  - target_classes_found: integer
  - validation_rules_generated: integer
  - xml_size_bytes: integer
  - parsing_duration_ms: integer

required_events:
  - "xml_parsing_started"
  - "node_shapes_extracted"
  - "property_shapes_extracted"
  - "constraints_extracted"
  - "validation_rules_generated"
```

#### **Shape Extraction Spans**
```yaml
span_name: "shacl.processor.extract_node_shapes"
required_attributes:
  - node_shapes_found: integer
  - target_classes: list
  - shape_types: list
  - deactivated_shapes: integer

span_name: "shacl.processor.extract_property_shapes"
required_attributes:
  - property_shapes_found: integer
  - property_paths: list
  - cardinality_constraints: integer
  - data_type_constraints: integer
```

### **Dynamic Data Validation**
The following dynamic data must be generated from real SHACL XML parsing:

#### **Expected Dynamic Data from Sample SHACL XML**
```xml
<!-- Input XML -->
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:shacl="http://www.w3.org/ns/shacl#">
    <shacl:NodeShape rdf:about="#UserInputShape">
        <shacl:targetClass rdf:resource="#UserInput"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasText"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
    <shacl:NodeShape rdf:about="#RecommendationShape">
        <shacl:targetClass rdf:resource="#Recommendation"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasConfidence"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
                <shacl:minInclusive>0.0</shacl:minInclusive>
                <shacl:maxInclusive>1.0</shacl:maxInclusive>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
</rdf:RDF>
```

#### **Expected Telemetry Data**
```yaml
# Span: shacl.processor.parse
attributes:
  success: true
  node_shapes_extracted: 2
  property_shapes_extracted: 2
  constraints_extracted: 6
  target_classes_found: 2
  validation_rules_generated: 2
  xml_size_bytes: 1024
  parsing_duration_ms: 150

# Span: shacl.processor.extract_node_shapes
attributes:
  node_shapes_found: 2
  target_classes: ["#UserInput", "#Recommendation"]
  shape_types: ["NodeShape", "NodeShape"]
  deactivated_shapes: 0

# Span: shacl.processor.extract_property_shapes
attributes:
  property_shapes_found: 2
  property_paths: ["#hasText", "#hasConfidence"]
  cardinality_constraints: 2
  data_type_constraints: 2

# Expected Graph output structure
output:
  node_shapes:
    UserInputShape:
      target_class: "#UserInput"
      properties:
        hasText:
          path: "#hasText"
          min_count: 1
          max_count: 1
          datatype: "http://www.w3.org/2001/XMLSchema#string"
    RecommendationShape:
      target_class: "#Recommendation"
      properties:
        hasConfidence:
          path: "#hasConfidence"
          min_count: 1
          max_count: 1
          datatype: "http://www.w3.org/2001/XMLSchema#float"
          min_inclusive: 0.0
          max_inclusive: 1.0
```

### **Validation Criteria**
- **NO HARDCODED VALUES**: All telemetry data must be generated from actual SHACL XML parsing
- **REAL PARSING**: XML must be parsed using `xml.etree.ElementTree` or `rdflib`
- **DYNAMIC EXTRACTION**: All counts, paths, and constraints must come from XML content
- **CONSTRAINT MAPPING**: Validation rules must be extracted from SHACL constraints
- **ERROR HANDLING**: Failed parsing must generate error spans with context
- **PERFORMANCE TRACKING**: Parsing duration must be measured and reported

---

## Summary
Implement the SHACL processor component of the AutoTel semantic execution pipeline to extract validation constraints from XML/RDF for DSPy signature validation.

## Description
The SHACL Processor extracts Shape Constraint Language (SHACL) validation rules from XML/RDF content. These constraints will be used by the DSPy compiler to generate validation rules for DSPy signatures, ensuring data integrity and semantic correctness.

## Acceptance Criteria

### Functional Requirements
- [ ] Parse SHACL shapes from XML/RDF fragments into rdflib.Graph objects
- [ ] Extract NodeShape definitions with target classes
- [ ] Extract PropertyShape constraints (minCount, maxCount, datatype, etc.)
- [ ] Extract validation rules for cardinality, data types, and value constraints
- [ ] Support SHACL namespaces and standard constraint types
- [ ] Parse from XML fragments and file paths
- [ ] Handle nested SHACL shapes and complex constraint hierarchies

### Technical Requirements
- [ ] Use rdflib for standards-compliant SHACL/RDF parsing
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Return rdflib.Graph objects for constraint extraction
- [ ] Support SHACL Core constraint types
- [ ] Handle property paths and target class references
- [ ] Extract constraint severity levels (Violation, Warning, Info)

### Constraint Types Supported
- **Cardinality**: minCount, maxCount, qualifiedMinCount, qualifiedMaxCount
- **Data Types**: datatype, nodeKind, class
- **Value Constraints**: minLength, maxLength, pattern, uniqueLang
- **Logical**: and, or, not, xone
- **Comparison**: equals, disjoint, lessThan, lessThanOrEquals

## Implementation Notes

### SHACL Shape Extraction
The processor should extract:
- **NodeShapes**: Target classes and their constraint sets
- **PropertyShapes**: Property-specific constraints with paths
- **Constraint Components**: Individual validation rules
- **Severity Levels**: Violation, Warning, Info for constraint enforcement

### Integration Points
- **Input**: SHACL/RDF XML fragments or files
- **Output**: rdflib.Graph for DSPy compiler constraint extraction
- **Dependencies**: rdflib, lxml.etree
- **No Dependencies**: BPMN, DMN (prototype phase only)

## Test Cases

### Basic SHACL Parsing
```xml
<shacl:shapes>
    <rdf:RDF xmlns:shacl="http://www.w3.org/ns/shacl#">
        <shacl:NodeShape rdf:about="#UserInputShape">
            <shacl:targetClass rdf:resource="#UserInput"/>
            <shacl:property>
                <shacl:PropertyShape>
                    <shacl:path rdf:resource="#hasText"/>
                    <shacl:minCount>1</shacl:minCount>
                    <shacl:maxCount>1</shacl:maxCount>
                    <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                </shacl:PropertyShape>
            </shacl:property>
        </shacl:NodeShape>
    </rdf:RDF>
</shacl:shapes>
```

### Constraint Extraction Tests
- Test cardinality constraints (minCount, maxCount)
- Test data type constraints (datatype, nodeKind)
- Test value constraints (minLength, maxLength, pattern)
- Test logical constraints (and, or, not)
- Test complex nested constraint hierarchies

### Integration Tests
- Test with DSPy compiler constraint extraction
- Test validation rule generation
- Test constraint severity handling

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with DSPy compiler pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No BPMN/DMN dependencies in prototype phase

## Story Points
**3** - Medium complexity due to SHACL constraint parsing

## Priority
**High** - Validation component for the processor > compiler > linker > executor pipeline

## Labels
- `processor`
- `validation`
- `shacl`
- `prototype`
- `constraints`

## Implementation Files
- `autotel/factory/processors/shacl_processor.py` - Main SHACL processor implementation
- `autotel/schemas/shacl_types.py` - SHACLShape and related dataclasses
- `tests/test_factory_shacl_processor.py` - Unit tests for SHACL processing