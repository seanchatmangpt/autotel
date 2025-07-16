# AUTOTEL-003: Ontology Compiler Implementation

## Summary
Implement the Ontology compiler component that transforms OWL ontology definitions into structured schemas with semantic type classifications for DSPy signature generation.

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

## Implementation Files
- `autotel/factory/ontology_compiler.py` - Main ontology compiler implementation
- `autotel/schemas/ontology_types.py` - OntologySchema and related dataclasses
- `tests/test_factory_ontology_compiler.py` - Unit tests for ontology compilation
- `tests/test_ontology_integration.py` - Integration tests with DSPy compiler

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/ontology_compiler.py`:

### Core Compilation Methods
1. **`compile()`** - "Ontology compilation must be implemented with real compilation logic"
   - Transform `OWLOntologyDefinition` into `OntologySchema`
   - Extract classes, properties, individuals, and axioms
   - Generate semantic context and examples
   - Return structured ontology schema

### Data Extraction Methods
2. **`_extract_classes()`** - "Class extraction must be implemented with real extraction logic"
   - Convert raw class data to `ClassSchema` objects
   - Apply semantic type classification
   - Extract property relationships
   - Handle class hierarchies and restrictions

3. **`_extract_properties()`** - "Property extraction must be implemented with real extraction logic"
   - Convert object and data properties to `PropertySchema` objects
   - Extract domain, range, and cardinality constraints
   - Handle property inheritance and restrictions
   - Map to appropriate data types

4. **`_extract_individuals()`** - "Individual extraction must be implemented with real extraction logic"
   - Extract OWL individuals as example data
   - Convert to DSPy signature examples
   - Handle property values and type associations
   - Generate structured example objects

5. **`_extract_axioms()`** - "Axiom extraction must be implemented with real extraction logic"
   - Extract OWL axioms and logical expressions
   - Convert to validation rules
   - Handle complex logical constructs
   - Generate constraint metadata

### Object Creation Methods
6. **`_create_ontology_class()`** - "Ontology class creation must be implemented with real creation logic"
   - Create `ClassSchema` from raw class data
   - Apply semantic type classification
   - Extract property relationships
   - Handle inheritance and restrictions

7. **`_create_ontology_property()`** - "Ontology property creation must be implemented with real creation logic"
   - Create `PropertySchema` from raw property data
   - Extract domain, range, and constraints
   - Map to appropriate data types
   - Handle property characteristics

### Implementation Requirements
- Transform raw OWL data into structured schemas
- Implement semantic type classification logic
- Generate semantic context for DSPy integration
- Create example data from OWL individuals
- Handle complex OWL constructs and axioms
- Support validation rule generation
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for malformed data 