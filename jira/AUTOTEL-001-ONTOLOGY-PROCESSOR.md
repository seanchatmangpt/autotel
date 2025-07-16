# AUTOTEL-001: Ontology Processor Implementation

## Summary
Implement the OWL processor component of the AutoTel semantic execution pipeline to extract ontological structures from XML/RDF for DSPy signature generation.

## Description
The Ontology Processor is the first stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It parses OWL/RDF XML content and extracts structured ontological definitions that will be used by the DSPy compiler to generate context-aware signatures.

## Acceptance Criteria

### Functional Requirements
- [ ] Parse OWL/RDF XML fragments into structured `OWLOntologyDefinition` objects
- [ ] Extract OWL classes with semantic type classification (user_input, recommendation, decision, analysis, reasoning, general)
- [ ] Extract OWL object and data properties with domain/range information
- [ ] Extract OWL individuals as examples for DSPy signature generation
- [ ] Extract OWL axioms and restrictions for validation rules
- [ ] Support standard OWL/RDF namespaces (owl, rdf, rdfs, xsd)
- [ ] Handle ontology URIs and namespace declarations
- [ ] Parse from XML fragments and file paths

### Technical Requirements
- [ ] Use rdflib for standards-compliant OWL/RDF parsing
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Return structured dataclasses, not raw rdflib.Graph objects
- [ ] Support semantic type inference from class names, labels, and comments
- [ ] Handle property inheritance and class hierarchies
- [ ] Extract cardinality constraints and data type restrictions

### Data Structures
```python
@dataclass
class OWLClassDefinition:
    uri: str
    name: str
    label: str
    comment: str
    semantic_type: str  # user_input, recommendation, decision, analysis, reasoning, general
    properties: List[str]
    superclasses: List[str]
    restrictions: List[Dict[str, Any]]

@dataclass
class OWLOntologyDefinition:
    ontology_uri: str
    prefix: str
    namespace: str
    classes: Dict[str, OWLClassDefinition]
    object_properties: Dict[str, OWLPropertyDefinition]
    data_properties: Dict[str, OWLPropertyDefinition]
    individuals: Dict[str, OWLIndividualDefinition]
    axioms: List[Dict[str, Any]]
```

## Implementation Notes

### Semantic Type Classification
The processor should automatically classify OWL classes based on:
- **user_input**: Classes with input-related keywords (input, user, request, etc.)
- **recommendation**: Classes with recommendation keywords (recommend, suggest, advice, etc.)
- **decision**: Classes with decision keywords (decision, choice, select, etc.)
- **analysis**: Classes with analysis keywords (analyze, evaluate, assess, etc.)
- **reasoning**: Classes with reasoning keywords (reason, logic, inference, etc.)
- **general**: Default for classes without specific semantic indicators

### Integration Points
- **Input**: OWL/RDF XML fragments or files
- **Output**: `OWLOntologyDefinition` for DSPy compiler consumption
- **Dependencies**: rdflib, lxml.etree
- **No Dependencies**: BPMN, DMN (prototype phase only)

## Test Cases

### Basic Ontology Parsing
```xml
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Class rdf:about="#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input data from user</rdfs:comment>
    </owl:Class>
</rdf:RDF>
```

### Semantic Type Classification
- Test classes with different semantic keywords
- Verify automatic classification logic
- Test edge cases and default behavior

### Property Extraction
- Test object properties with domain/range
- Test data properties with XSD types
- Test property inheritance

### Individual Examples
- Test OWL individuals as DSPy signature examples
- Test property value extraction
- Test class type associations

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with DSPy compiler pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No BPMN/DMN dependencies in prototype phase

## Story Points
**5** - Medium complexity due to OWL/RDF parsing and semantic classification

## Priority
**High** - Foundation component for the processor > compiler > linker > executor pipeline

## Labels
- `processor`
- `ontology`
- `owl`
- `prototype`
- `foundation`

## Implementation Files
- `autotel/factory/processors/owl_processor.py` - Main OWL processor implementation
- `autotel/schemas/owl_types.py` - OWLClassDefinition and related dataclasses
- `tests/test_factory_owl_processor.py` - Unit tests for OWL processing
- `tests/test_owl_integration.py` - Integration tests with telemetry

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/processors/owl_processor.py`:

### Core Parsing Methods
1. **`parse_ontology_definition()`** - "OWL ontology parsing must be implemented with real parsing logic"
   - Parse XML content using `xml.etree.ElementTree`
   - Extract ontology URI, namespace, and prefix
   - Call all extraction methods and return `OWLOntologyDefinition`

2. **`_extract_ontology_uri()`** - "Ontology URI extraction must be implemented with real extraction logic"
   - Extract `rdf:about` from `owl:Ontology` element
   - Handle namespace declarations and relative URIs
   - Return fully qualified ontology URI

3. **`_extract_namespace()`** - "Namespace extraction must be implemented with real extraction logic"
   - Extract namespace from root element attributes
   - Handle multiple namespace declarations
   - Return primary namespace URI

### Class and Property Extraction
4. **`_extract_classes()`** - "Class extraction must be implemented with real extraction logic"
   - Find all `owl:Class` elements using XPath
   - Extract class URI, label, comment, and semantic type
   - Handle class hierarchies and restrictions
   - Return structured class definitions

5. **`_extract_object_properties()`** - "Object property extraction must be implemented with real extraction logic"
   - Find all `owl:ObjectProperty` elements
   - Extract domain, range, and property characteristics
   - Handle property hierarchies and restrictions

6. **`_extract_data_properties()`** - "Data property extraction must be implemented with real extraction logic"
   - Find all `owl:DatatypeProperty` elements
   - Extract domain, range, and data type constraints
   - Handle XSD type mappings

### Individual and Axiom Extraction
7. **`_extract_individuals()`** - "Individual extraction must be implemented with real extraction logic"
   - Find all `owl:NamedIndividual` elements
   - Extract individual types and property values
   - Create example data for DSPy signatures

8. **`_extract_axioms()`** - "Axiom extraction must be implemented with real extraction logic"
   - Extract OWL axioms (subClassOf, equivalentClass, etc.)
   - Handle complex logical expressions
   - Convert to validation rules

### Element Property Extraction
9. **`_extract_label()`** - "Label extraction must be implemented with real extraction logic"
   - Find `rdfs:label` elements within given element
   - Handle multiple labels and language tags
   - Return primary label text

10. **`_extract_comment()`** - "Comment extraction must be implemented with real extraction logic"
    - Find `rdfs:comment` elements within given element
    - Handle multiple comments and language tags
    - Return primary comment text

11. **`_extract_domain()`** - "Domain extraction must be implemented with real extraction logic"
    - Find `rdfs:domain` elements within property element
    - Handle multiple domains and class references
    - Return domain class URI

12. **`_extract_range()`** - "Range extraction must be implemented with real extraction logic"
    - Find `rdfs:range` elements within property element
    - Handle multiple ranges and type references
    - Return range class or datatype URI

13. **`_extract_type()`** - "Type extraction must be implemented with real extraction logic"
    - Find `rdf:type` elements within individual element
    - Handle multiple types and class references
    - Return primary type URI

14. **`_extract_individual_properties()`** - "Individual property extraction must be implemented with real extraction logic"
    - Extract all property assertions for individual
    - Handle object and data property values
    - Return structured property-value pairs

### Implementation Requirements
- Use `xml.etree.ElementTree` for XML parsing
- Handle OWL/RDF namespaces correctly
- Implement semantic type classification logic
- Support both XML fragments and file paths
- Return structured dataclasses, not raw XML
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for malformed XML 