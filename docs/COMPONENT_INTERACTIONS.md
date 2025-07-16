# AutoTel Component Interactions

## Data Structure Transformations

### Pipeline Data Flow

```mermaid
graph LR
    subgraph "Input XML"
        OWLXML[OWL/RDF XML]
        SHACLXML[SHACL/RDF XML]
        DSPyXML[DSPy XML]
    end
    
    subgraph "Processors"
        OWLProc[OWL Processor]
        SHACLProc[SHACL Processor]
        DSPyProc[DSPy Processor]
    end
    
    subgraph "Processor Outputs"
        OWLDef[OWLOntologyDefinition]
        SHACLGraph[Graph]
        DSPyDef[DSPySignatureDefinition]
    end
    
    subgraph "Compilers"
        OntologyComp[Ontology Compiler]
        ValidationComp[Validation Compiler]
        DSPyComp[DSPy Compiler]
    end
    
    subgraph "Compiler Outputs"
        OntologySchema[OntologySchema]
        ValidationRules[ValidationRules]
        DSPySignature[DSPySignature]
    end
    
    subgraph "Final Stages"
        Linker[Semantic Linker]
        Executor[Ontology Executor]
    end
    
    subgraph "Results"
        ExecutableSystem[ExecutableSystem]
        Results[Results + Telemetry]
    end
    
    OWLXML --> OWLProc
    SHACLXML --> SHACLProc
    DSPyXML --> DSPyProc
    
    OWLProc --> OWLDef
    SHACLProc --> SHACLGraph
    DSPyProc --> DSPyDef
    
    OWLDef --> OntologyComp
    SHACLGraph --> ValidationComp
    DSPyDef --> DSPyComp
    
    OntologyComp --> OntologySchema
    ValidationComp --> ValidationRules
    
    OntologySchema --> DSPyComp
    ValidationRules --> DSPyComp
    
    DSPyComp --> DSPySignature
    DSPySignature --> Linker
    Linker --> ExecutableSystem
    ExecutableSystem --> Executor
    Executor --> Results
```

## Detailed Data Structure Definitions

### OWLOntologyDefinition (Processor Output)
```python
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

### OntologySchema (Ontology Compiler Output)
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
```

### ValidationRules (Validation Compiler Output)
```python
@dataclass
class ValidationRules:
    target_classes: Dict[str, List[ValidationRule]]
    constraint_count: int
    severity_levels: Dict[str, int]
    metadata: Dict[str, Any]

@dataclass
class ValidationRule:
    rule_id: str
    target_class: str
    property_path: str
    constraint_type: str  # cardinality, datatype, value, logical
    constraint_value: Any
    severity: str  # Violation, Warning, Info
    message: str
    metadata: Dict[str, Any]
```

### DSPySignature (DSPy Compiler Output)
```python
@dataclass
class DSPySignature:
    name: str
    description: str
    inputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to input schemas
    outputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to output schemas
    validation_rules: List[Dict[str, Any]]  # SHACL constraints
    examples: List[Dict[str, Any]]  # OWL individuals
    model_config: Optional[DSPyModelConfiguration]
    module_config: Optional[DSPyModuleDefinition]
    semantic_context: Dict[str, Any]
    ontology_metadata: Dict[str, Any]
```

## Component Interface Specifications

### Ontology Compiler Interface
```python
class OntologyCompiler:
    def compile(self, ontology: OWLOntologyDefinition) -> OntologySchema:
        """
        Transform OWL ontology definition into structured schema with semantic types.
        
        Args:
            ontology: Parsed OWL ontology definition
            
        Returns:
            OntologySchema with semantic classifications and structured schemas
        """
        pass
```

### Validation Compiler Interface
```python
class ValidationCompiler:
    def compile(self, shacl_graph: Graph) -> ValidationRules:
        """
        Transform SHACL constraint graph into structured validation rules.
        
        Args:
            shacl_graph: Parsed SHACL constraint graph
            
        Returns:
            ValidationRules with constraint mappings and severity levels
        """
        pass
```

### DSPy Compiler Interface
```python
class DSPyCompiler:
    def compile(
        self,
        ontology_schema: OntologySchema,
        validation_rules: ValidationRules,
        dspy_signatures: List[DSPySignatureDefinition],
        dspy_modules: List[DSPyModuleDefinition],
        model_config: DSPyModelConfiguration
    ) -> DSPySignature:
        """
        Integrate all compiler outputs into execution-ready DSPy signature.
        
        Args:
            ontology_schema: Compiled ontology schema
            validation_rules: Compiled validation rules
            dspy_signatures: Parsed DSPy signature definitions
            dspy_modules: Parsed DSPy module definitions
            model_config: Parsed model configuration
            
        Returns:
            DSPySignature ready for execution
        """
        pass
```

## Error Handling Strategy

### Fail-Fast Principle
```python
# NO try-catch blocks - let errors crash naturally
def process_ontology(ontology_def: OWLOntologyDefinition) -> OntologySchema:
    # If any step fails, the entire pipeline fails
    # This ensures data integrity and makes debugging easier
    semantic_types = classify_semantic_types(ontology_def.classes)
    schemas = generate_schemas(ontology_def, semantic_types)
    context = generate_semantic_context(ontology_def)
    return OntologySchema(...)
```

### Validation Points
```python
def validate_ontology_schema(schema: OntologySchema) -> None:
    """Validate ontology schema before passing to next stage."""
    if not schema.ontology_uri:
        raise ValueError("Ontology URI is required")
    if not schema.classes:
        raise ValueError("At least one class is required")
    # Additional validation rules...

def validate_validation_rules(rules: ValidationRules) -> None:
    """Validate validation rules before passing to next stage."""
    if rules.constraint_count == 0:
        raise ValueError("At least one validation rule is required")
    # Additional validation rules...
```

## Integration Testing Strategy

### Unit Test Structure
```python
class TestOntologyCompiler:
    def test_semantic_classification(self):
        """Test automatic semantic type classification."""
        pass
    
    def test_schema_generation(self):
        """Test schema generation from OWL classes."""
        pass
    
    def test_context_generation(self):
        """Test semantic context generation."""
        pass

class TestValidationCompiler:
    def test_constraint_extraction(self):
        """Test SHACL constraint extraction."""
        pass
    
    def test_rule_generation(self):
        """Test validation rule generation."""
        pass
    
    def test_severity_handling(self):
        """Test constraint severity level handling."""
        pass

class TestDSPyCompiler:
    def test_schema_integration(self):
        """Test ontology schema integration."""
        pass
    
    def test_validation_integration(self):
        """Test validation rules integration."""
        pass
    
    def test_config_integration(self):
        """Test model and module configuration integration."""
        pass
```

### Integration Test Structure
```python
class TestPipelineIntegration:
    def test_end_to_end_compilation(self):
        """Test complete pipeline from XML to DSPy signature."""
        # 1. Parse XML files
        # 2. Run all compilers
        # 3. Verify final DSPy signature
        pass
    
    def test_compiler_chain(self):
        """Test compiler dependencies and data flow."""
        # Test that each compiler output feeds correctly into next stage
        pass
```

## Performance Considerations

### Compiler Optimization
- **Ontology Compiler**: Cache semantic type classifications
- **Validation Compiler**: Optimize constraint graph traversal
- **DSPy Compiler**: Minimize data copying between stages

### Memory Management
- Use dataclasses for efficient memory usage
- Avoid deep copying of large data structures
- Pass references where possible

### Parallel Processing
- Processors can run in parallel (OWL, SHACL, DSPy)
- Compilers must run sequentially due to dependencies
- Consider parallel validation rule processing within Validation Compiler

This detailed interaction specification provides clear guidance for implementing each component with proper interfaces, error handling, and testing strategies. 