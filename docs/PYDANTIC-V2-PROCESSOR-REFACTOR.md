# Pydantic v2 Processor Refactor Plan

## Overview

This document outlines the plan to refactor AutoTel processors to use Pydantic v2, leveraging its improved performance, better type validation, and enhanced features for better data handling and validation across the framework.

## Current State Analysis

### Existing Data Structures
- **OWL Processor**: Uses dataclasses for ontology classes, properties, and individuals
- **SHACL Processor**: Uses dictionaries and RDF graphs for validation rules
- **DSPy Processor**: Uses custom dataclasses for signature definitions
- **DMN Processor**: Uses SpiffWorkflow objects with minimal custom data structures
- **Jinja Processor**: Uses dataclasses for template definitions and rendering results
- **BPMN Processor**: Uses SpiffWorkflow objects with custom extensions
- **OTEL Processor**: Uses dataclasses for telemetry configuration

### Current Issues
1. **Inconsistent validation**: Different processors use different validation approaches
2. **Type safety**: Limited type checking and validation
3. **Performance**: Dataclasses don't provide the same performance benefits as Pydantic
4. **Serialization**: Manual serialization/deserialization logic
5. **Error handling**: Inconsistent error messages and validation failures

## Pydantic v2 Benefits

### Performance Improvements
- **Rust-based validation**: Up to 10x faster validation
- **Lazy evaluation**: Only validates when needed
- **Optimized serialization**: Faster JSON/YAML conversion
- **Memory efficiency**: Better memory usage patterns

### Enhanced Features
- **Strict type checking**: Better type safety and IDE support
- **Rich validation**: Built-in validators for common patterns
- **Error handling**: Detailed, structured error messages
- **Serialization**: Automatic JSON/YAML/XML conversion
- **Documentation**: Auto-generated schema documentation
- **OpenAPI integration**: Native OpenAPI schema generation

### Developer Experience
- **Better IDE support**: Enhanced autocomplete and type hints
- **Validation at development time**: Catch errors early
- **Self-documenting**: Models serve as documentation
- **Consistent patterns**: Unified approach across all processors

## Migration Strategy

### Phase 1: Core Data Models (Week 1-2)

#### 1.1 OWL Processor Models
```python
from pydantic import BaseModel, Field, validator
from typing import List, Optional, Dict, Any
from rdflib import URIRef

class OWLClass(BaseModel):
    uri: URIRef = Field(..., description="Class URI")
    label: Optional[str] = Field(None, description="Human-readable label")
    comment: Optional[str] = Field(None, description="Class description")
    properties: List[OWLProperty] = Field(default_factory=list)
    
    @validator('uri')
    def validate_uri(cls, v):
        if not isinstance(v, URIRef):
            raise ValueError("URI must be a valid URIRef")
        return v

class OWLProperty(BaseModel):
    uri: URIRef = Field(..., description="Property URI")
    label: Optional[str] = Field(None, description="Property label")
    domain: Optional[URIRef] = Field(None, description="Property domain")
    range: Optional[URIRef] = Field(None, description="Property range")
    property_type: str = Field(..., description="Object or Data property")

class OWLOntology(BaseModel):
    uri: URIRef = Field(..., description="Ontology URI")
    classes: List[OWLClass] = Field(default_factory=list)
    object_properties: List[OWLProperty] = Field(default_factory=list)
    data_properties: List[OWLProperty] = Field(default_factory=list)
    individuals: List[OWLIndividual] = Field(default_factory=list)
    
    class Config:
        arbitrary_types_allowed = True  # For URIRef support
```

#### 1.2 SHACL Processor Models
```python
class SHACLNodeShape(BaseModel):
    uri: str = Field(..., description="Node shape URI")
    target_class: Optional[str] = Field(None, description="Target class")
    properties: List[SHACLPropertyShape] = Field(default_factory=list)
    deactivated: bool = Field(False, description="Whether shape is deactivated")

class SHACLPropertyShape(BaseModel):
    path: str = Field(..., description="Property path")
    constraints: Dict[str, Any] = Field(default_factory=dict)
    validators: List[str] = Field(default_factory=list)
    
    @validator('constraints')
    def validate_constraints(cls, v):
        # Validate constraint structure
        return v

class SHACLValidationResult(BaseModel):
    valid: bool = Field(..., description="Whether validation passed")
    violations: List[SHACLViolation] = Field(default_factory=list)
    warnings: List[SHACLViolation] = Field(default_factory=list)
    info: List[SHACLViolation] = Field(default_factory=list)
    
    @property
    def total_violations(self) -> int:
        return len(self.violations)

class SHACLViolation(BaseModel):
    severity: str = Field(..., description="Violation severity")
    message: str = Field(..., description="Violation message")
    path: Optional[str] = Field(None, description="Property path")
    value: Optional[Any] = Field(None, description="Invalid value")
```

#### 1.3 DSPy Processor Models
```python
class DSPySignature(BaseModel):
    name: str = Field(..., description="Signature name")
    description: str = Field(..., description="Signature description")
    inputs: Dict[str, DSPyParameter] = Field(..., description="Input parameters")
    outputs: Dict[str, DSPyParameter] = Field(..., description="Output parameters")
    shacl_input_shapes: Optional[Dict[str, str]] = Field(None, description="SHACL input shapes")
    shacl_output_shapes: Optional[Dict[str, str]] = Field(None, description="SHACL output shapes")
    
    @validator('name')
    def validate_name(cls, v):
        if not v.isidentifier():
            raise ValueError("Name must be a valid Python identifier")
        return v

class DSPyParameter(BaseModel):
    name: str = Field(..., description="Parameter name")
    description: str = Field(..., description="Parameter description")
    type: str = Field(..., description="Parameter type")
    optional: bool = Field(False, description="Whether parameter is optional")
    default: Optional[Any] = Field(None, description="Default value")
    
    @validator('type')
    def validate_type(cls, v):
        valid_types = ['string', 'integer', 'float', 'boolean', 'object', 'array']
        if v not in valid_types:
            raise ValueError(f"Type must be one of {valid_types}")
        return v
```

### Phase 2: Processor Integration (Week 3-4)

#### 2.1 Update Processor Classes
```python
class OWLProcessor:
    def __init__(self):
        self.telemetry = create_telemetry_manager("autotel-owl-processor")
    
    def parse_ontology_definition(self, xml_content: str) -> OWLOntology:
        """Parse OWL XML and return validated ontology model"""
        with self.telemetry.start_span("owl_parse_ontology") as span:
            # Parse XML and create OWLOntology instance
            ontology_data = self._extract_ontology_data(xml_content)
            
            # Pydantic validation happens automatically
            ontology = OWLOntology(**ontology_data)
            
            span.set_attribute("classes_count", len(ontology.classes))
            span.set_attribute("properties_count", len(ontology.object_properties) + len(ontology.data_properties))
            
            return ontology
    
    def validate_data_against_ontology(self, data: Dict[str, Any], ontology: OWLOntology) -> bool:
        """Validate data against ontology using Pydantic models"""
        # Create dynamic Pydantic model from ontology
        model_class = self._create_dynamic_model(ontology)
        
        try:
            model_class(**data)
            return True
        except ValidationError as e:
            # Log validation errors
            return False
```

#### 2.2 Enhanced Error Handling
```python
from pydantic import ValidationError

class ProcessorValidationError(Exception):
    def __init__(self, message: str, errors: List[Dict[str, Any]]):
        self.message = message
        self.errors = errors
        super().__init__(self.message)

def handle_validation_error(error: ValidationError, context: str) -> ProcessorValidationError:
    """Convert Pydantic validation errors to processor-specific errors"""
    formatted_errors = []
    for err in error.errors():
        formatted_errors.append({
            'field': '.'.join(str(loc) for loc in err['loc']),
            'message': err['msg'],
            'type': err['type'],
            'input': err['input']
        })
    
    return ProcessorValidationError(
        f"{context} validation failed",
        formatted_errors
    )
```

### Phase 3: Advanced Features (Week 5-6)

#### 3.1 Custom Validators
```python
from pydantic import validator, root_validator

class DMNDecision(BaseModel):
    decision_id: str = Field(..., description="Decision ID")
    name: str = Field(..., description="Decision name")
    inputs: List[DMNInput] = Field(..., description="Decision inputs")
    outputs: List[DMNOutput] = Field(..., description="Decision outputs")
    rules: List[DMNRule] = Field(..., description="Decision rules")
    
    @validator('decision_id')
    def validate_decision_id(cls, v):
        if not v.isidentifier():
            raise ValueError("Decision ID must be a valid identifier")
        return v
    
    @root_validator
    def validate_rule_consistency(cls, values):
        """Validate that rules are consistent with inputs/outputs"""
        inputs = values.get('inputs', [])
        outputs = values.get('outputs', [])
        rules = values.get('rules', [])
        
        # Validate rule consistency
        for rule in rules:
            if len(rule.input_entries) != len(inputs):
                raise ValueError(f"Rule {rule.rule_id} has {len(rule.input_entries)} input entries but {len(inputs)} inputs defined")
            
            if len(rule.output_entries) != len(outputs):
                raise ValueError(f"Rule {rule.rule_id} has {len(rule.output_entries)} output entries but {len(outputs)} outputs defined")
        
        return values

class DMNInput(BaseModel):
    input_id: str = Field(..., description="Input ID")
    label: str = Field(..., description="Input label")
    expression: str = Field(..., description="Input expression")
    type_ref: str = Field(..., description="Input type reference")
    
    @validator('expression')
    def validate_expression(cls, v):
        # Validate DMN expression syntax
        if not v.strip():
            raise ValueError("Expression cannot be empty")
        return v
```

#### 3.2 Serialization/Deserialization
```python
class JinjaTemplate(BaseModel):
    name: str = Field(..., description="Template name")
    content: str = Field(..., description="Template content")
    variables: Dict[str, Any] = Field(default_factory=dict, description="Template variables")
    
    def to_json(self) -> str:
        """Serialize to JSON"""
        return self.model_dump_json(indent=2)
    
    def to_yaml(self) -> str:
        """Serialize to YAML"""
        return self.model_dump_yaml()
    
    @classmethod
    def from_xml(cls, xml_content: str) -> 'JinjaTemplate':
        """Create from XML content"""
        # Parse XML and create instance
        template_data = parse_jinja_xml(xml_content)
        return cls(**template_data)
    
    def render(self, context: Dict[str, Any]) -> str:
        """Render template with context"""
        # Validate context against variables
        validated_context = self._validate_context(context)
        return render_template(self.content, validated_context)
    
    def _validate_context(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Validate context against expected variables"""
        # Create dynamic model for validation
        context_model = self._create_context_model()
        return context_model(**context).model_dump()
```

### Phase 4: Integration Testing (Week 7-8)

#### 4.1 Updated Integration Test
```python
def test_pydantic_processor_integration():
    """Test all processors with Pydantic v2 models"""
    
    # Generate test data using Factory Boy
    test_customer = CustomerFactory()
    
    # OWL Processor with Pydantic models
    owl_processor = OWLProcessor()
    owl_xml = create_test_owl_ontology()
    ontology: OWLOntology = owl_processor.parse_ontology_definition(owl_xml)
    
    # Validate ontology structure
    assert len(ontology.classes) == 1
    assert len(ontology.data_properties) == 3
    
    # SHACL Processor with Pydantic models
    shacl_processor = SHACLProcessor()
    shacl_xml = create_test_shacl_shapes()
    validation_rules: List[SHACLNodeShape] = shacl_processor.parse(shacl_xml)
    
    # Validate data against SHACL shapes
    validation_result: SHACLValidationResult = shacl_processor.validate_data(
        test_customer, validation_rules
    )
    assert validation_result.valid
    
    # DSPy Processor with Pydantic models
    dspy_processor = DSPyProcessor()
    dspy_xml = create_test_dspy_signature()
    signatures: List[DSPySignature] = dspy_processor.parse(dspy_xml)
    
    assert len(signatures) == 1
    signature = signatures[0]
    assert signature.name == "CreditAnalysis"
    assert len(signature.inputs) == 3
    
    # Test serialization
    signature_json = signature.model_dump_json(indent=2)
    signature_from_json = DSPySignature.model_validate_json(signature_json)
    assert signature_from_json == signature
```

## Implementation Benefits

### Performance Improvements
- **Faster validation**: Rust-based validation engine
- **Reduced memory usage**: More efficient data structures
- **Better caching**: Pydantic's internal caching mechanisms
- **Optimized serialization**: Faster JSON/YAML conversion

### Code Quality
- **Type safety**: Compile-time type checking
- **Self-documenting**: Models serve as documentation
- **Consistent patterns**: Unified approach across processors
- **Better error messages**: Detailed validation errors

### Developer Experience
- **IDE support**: Enhanced autocomplete and type hints
- **Validation at development time**: Catch errors early
- **OpenAPI integration**: Auto-generated API documentation
- **Testing**: Easier to test with validated models

## Migration Checklist

### Phase 1: Core Models
- [ ] Create Pydantic models for OWL data structures
- [ ] Create Pydantic models for SHACL validation rules
- [ ] Create Pydantic models for DSPy signatures
- [ ] Create Pydantic models for DMN decisions
- [ ] Create Pydantic models for Jinja templates
- [ ] Create Pydantic models for BPMN extensions
- [ ] Create Pydantic models for OTEL configuration

### Phase 2: Processor Updates
- [ ] Update OWLProcessor to use Pydantic models
- [ ] Update SHACLProcessor to use Pydantic models
- [ ] Update DSPyProcessor to use Pydantic models
- [ ] Update DMNProcessor to use Pydantic models
- [ ] Update JinjaProcessor to use Pydantic models
- [ ] Update BPMNProcessor to use Pydantic models
- [ ] Update OTELProcessor to use Pydantic models

### Phase 3: Advanced Features
- [ ] Implement custom validators for domain-specific rules
- [ ] Add serialization/deserialization methods
- [ ] Implement error handling and validation error conversion
- [ ] Add OpenAPI schema generation
- [ ] Implement caching for frequently used models

### Phase 4: Testing and Validation
- [ ] Update integration tests to use Pydantic models
- [ ] Add performance benchmarks
- [ ] Validate error handling and edge cases
- [ ] Test serialization/deserialization
- [ ] Update documentation

## Dependencies

### Required Packages
```toml
[tool.poetry.dependencies]
pydantic = "^2.0.0"
pydantic-settings = "^2.0.0"
```

### Optional Packages
```toml
[tool.poetry.dependencies]
pydantic-extra-types = "^2.0.0"  # For additional field types
pydantic-json-schema = "^2.0.0"  # For JSON schema generation
```

## Timeline

- **Week 1-2**: Core data models and basic validation
- **Week 3-4**: Processor integration and error handling
- **Week 5-6**: Advanced features and custom validators
- **Week 7-8**: Testing, validation, and documentation

## Risk Mitigation

### Backward Compatibility
- Maintain existing processor interfaces during migration
- Use feature flags to enable/disable Pydantic validation
- Provide migration guides for existing code

### Performance Monitoring
- Benchmark performance before and after migration
- Monitor memory usage and validation times
- Set up alerts for performance regressions

### Testing Strategy
- Comprehensive unit tests for all models
- Integration tests for processor workflows
- Performance tests for validation and serialization
- Error handling tests for edge cases

## Success Metrics

### Performance
- 50% reduction in validation time
- 30% reduction in memory usage
- 2x faster serialization/deserialization

### Code Quality
- 100% type coverage for processor models
- Zero runtime type errors
- Consistent error messages across processors

### Developer Experience
- Improved IDE autocomplete and type hints
- Self-documenting models
- Auto-generated API documentation
- Reduced debugging time

## Conclusion

The migration to Pydantic v2 will significantly improve the AutoTel framework's performance, type safety, and developer experience. The phased approach ensures minimal disruption while maximizing the benefits of the new validation system.

The refactor will result in:
- More robust and maintainable code
- Better performance and scalability
- Enhanced developer productivity
- Improved error handling and debugging
- Future-proof architecture for additional features 