# AUTOTEL-002: SHACL Processor Implementation

## Summary
Implement the SHACL processor component to extract validation constraints from XML/RDF for integration with DSPy signature validation.

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
- `tests/test_shacl_integration.py` - Integration tests with validation 