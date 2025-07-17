# AutoTel Pipeline Sequence Diagrams

## Complete Pipeline Execution Sequence

```mermaid
sequenceDiagram
    participant Client
    participant PipelineOrchestrator
    participant OWLProcessor
    participant SHACLProcessor
    participant DSPyProcessor
    participant OntologyCompiler
    participant ValidationCompiler
    participant DSPyCompiler
    participant SemanticLinker
    participant OntologyExecutor
    participant TelemetryManager
    participant ModelProvider

    Client->>PipelineOrchestrator: execute_pipeline(owl_xml, shacl_xml, dspy_xml, inputs)
    
    Note over PipelineOrchestrator: Stage 1: Processors with OpenTelemetry Tracing
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.processors", "PROCESSORS")
    
    PipelineOrchestrator->>OWLProcessor: parse_ontology_definition(owl_xml, prefix)
    OWLProcessor->>OWLProcessor: _extract_ontology_uri(root)
    OWLProcessor->>OWLProcessor: _extract_namespace(root)
    OWLProcessor->>OWLProcessor: _extract_classes(root)
    OWLProcessor->>OWLProcessor: _extract_object_properties(root)
    OWLProcessor->>OWLProcessor: _extract_data_properties(root)
    OWLProcessor->>OWLProcessor: _extract_individuals(root)
    OWLProcessor->>OWLProcessor: _extract_axioms(root)
    OWLProcessor-->>PipelineOrchestrator: OWLOntologyDefinition
    
    PipelineOrchestrator->>SHACLProcessor: parse(shacl_xml)
    SHACLProcessor-->>PipelineOrchestrator: SHACL Graph
    
    PipelineOrchestrator->>DSPyProcessor: parse(dspy_xml)
    DSPyProcessor-->>PipelineOrchestrator: DSPySignatureDefinition[]
    PipelineOrchestrator->>DSPyProcessor: parse_modules(dspy_xml)
    DSPyProcessor-->>PipelineOrchestrator: DSPyModuleDefinition[]
    PipelineOrchestrator->>DSPyProcessor: parse_model_configuration(dspy_xml)
    DSPyProcessor-->>PipelineOrchestrator: DSPyModelConfiguration
    
    TelemetryManager-->>PipelineOrchestrator: span complete
    
    Note over PipelineOrchestrator: Stage 2: Compilers with OpenTelemetry Tracing
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.compilers", "COMPILERS")
    
    PipelineOrchestrator->>OntologyCompiler: compile(ontology_def)
    OntologyCompiler->>OntologyCompiler: _extract_classes(ontology_def)
    OntologyCompiler->>OntologyCompiler: _extract_properties(ontology_def)
    OntologyCompiler->>OntologyCompiler: _extract_individuals(ontology_def)
    OntologyCompiler->>OntologyCompiler: _extract_axioms(ontology_def)
    OntologyCompiler->>OntologyCompiler: _create_ontology_class(class_data)
    OntologyCompiler->>OntologyCompiler: _create_ontology_property(property_data)
    OntologyCompiler-->>PipelineOrchestrator: OntologySchema
    
    PipelineOrchestrator->>ValidationCompiler: compile(shacl_graph)
    ValidationCompiler->>ValidationCompiler: _extract_node_shapes(shacl_graph)
    ValidationCompiler->>ValidationCompiler: _extract_property_shapes(shacl_graph)
    ValidationCompiler->>ValidationCompiler: _extract_constraints(shape)
    ValidationCompiler->>ValidationCompiler: _create_validation_rule(shape_type, shape_data)
    ValidationCompiler-->>PipelineOrchestrator: ValidationRules
    
    PipelineOrchestrator->>DSPyCompiler: compile(ontology_schema, validation_rules, dspy_signatures, dspy_modules, model_config)
    DSPyCompiler->>DSPyCompiler: _merge_signatures(signatures)
    DSPyCompiler->>DSPyCompiler: _integrate_ontology_context(signature, ontology)
    DSPyCompiler->>DSPyCompiler: _integrate_validation_rules(signature, validation)
    DSPyCompiler->>DSPyCompiler: _create_dspy_signature(signature_def, module_def, model_config)
    DSPyCompiler-->>PipelineOrchestrator: DSPySignature
    
    TelemetryManager-->>PipelineOrchestrator: span complete
    
    Note over PipelineOrchestrator: Stage 3: Linker with OpenTelemetry Tracing
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.linker", "LINKER")
    
    PipelineOrchestrator->>SemanticLinker: link(dspy_signature)
    SemanticLinker->>SemanticLinker: _validate_signature(signature)
    SemanticLinker->>SemanticLinker: _generate_semantic_context(signature)
    SemanticLinker->>SemanticLinker: _prepare_validation_rules(signature)
    SemanticLinker->>SemanticLinker: _generate_metadata(signature)
    SemanticLinker-->>PipelineOrchestrator: ExecutableSystem
    
    TelemetryManager-->>PipelineOrchestrator: span complete
    
    Note over PipelineOrchestrator: Stage 4: Executor with OpenTelemetry Tracing
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.executor", "EXECUTOR")
    
    PipelineOrchestrator->>OntologyExecutor: execute(executable_system, inputs)
    OntologyExecutor->>OntologyExecutor: _validate_inputs(executable_system, inputs)
    OntologyExecutor->>OntologyExecutor: _apply_validation_rules(executable_system, inputs)
    OntologyExecutor->>OntologyExecutor: _execute_signature(executable_system, inputs)
    OntologyExecutor->>ModelProvider: execute_dspy_signature(signature, inputs)
    ModelProvider-->>OntologyExecutor: model_outputs
    OntologyExecutor->>OntologyExecutor: _generate_telemetry(executable_system, inputs, outputs)
    OntologyExecutor->>OntologyExecutor: _generate_execution_metadata(executable_system, inputs, outputs)
    OntologyExecutor-->>PipelineOrchestrator: ExecutionResult
    
    TelemetryManager-->>PipelineOrchestrator: span complete
    
    Note over PipelineOrchestrator: Pipeline Complete
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.complete", "COMPLETE")
    TelemetryManager-->>PipelineOrchestrator: span complete
    
    PipelineOrchestrator-->>Client: ExecutionResult
```

## OWL Processor Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant OWLProcessor
    participant XMLParser
    participant OWLOntologyDefinition

    PipelineOrchestrator->>OWLProcessor: parse_ontology_definition(xml_content, prefix)
    
    OWLProcessor->>XMLParser: ET.fromstring(xml_content)
    XMLParser-->>OWLProcessor: root_element
    
    OWLProcessor->>OWLProcessor: _extract_ontology_uri(root)
    Note over OWLProcessor: Find owl:Ontology element<br/>Extract rdf:about attribute<br/>Handle namespace declarations<br/>Return fully qualified URI
    
    OWLProcessor->>OWLProcessor: _extract_namespace(root)
    Note over OWLProcessor: Extract xmlns attributes<br/>Handle multiple namespaces<br/>Return primary namespace URI
    
    OWLProcessor->>OWLProcessor: _extract_classes(root)
    Note over OWLProcessor: Find all owl:Class elements<br/>Extract URI, label, comment<br/>Determine semantic type<br/>Handle hierarchies and restrictions
    
    OWLProcessor->>OWLProcessor: _extract_object_properties(root)
    Note over OWLProcessor: Find all owl:ObjectProperty elements<br/>Extract domain and range<br/>Handle property characteristics<br/>Process inheritance
    
    OWLProcessor->>OWLProcessor: _extract_data_properties(root)
    Note over OWLProcessor: Find all owl:DatatypeProperty elements<br/>Extract domain and range<br/>Handle XSD type mappings<br/>Process constraints
    
    OWLProcessor->>OWLProcessor: _extract_individuals(root)
    Note over OWLProcessor: Find all owl:NamedIndividual elements<br/>Extract types and property values<br/>Create example data<br/>Generate structured examples
    
    OWLProcessor->>OWLProcessor: _extract_axioms(root)
    Note over OWLProcessor: Extract OWL axioms<br/>Handle logical expressions<br/>Convert to validation rules<br/>Process complex constructs
    
    OWLProcessor->>OWLOntologyDefinition: create(ontology_uri, prefix, namespace, classes, object_properties, data_properties, individuals, axioms)
    OWLOntologyDefinition-->>OWLProcessor: ontology_definition
    
    OWLProcessor-->>PipelineOrchestrator: OWLOntologyDefinition
```

## Ontology Compiler Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant OntologyCompiler
    participant OWLOntologyDefinition
    participant OntologySchema
    participant ClassSchema
    participant PropertySchema

    PipelineOrchestrator->>OntologyCompiler: compile(ontology_def)
    
    OntologyCompiler->>OntologyCompiler: _extract_classes(ontology_def)
    Note over OntologyCompiler: Convert raw class data<br/>Apply semantic type classification<br/>Extract property relationships<br/>Handle hierarchies and restrictions
    
    loop For each class
        OntologyCompiler->>OntologyCompiler: _create_ontology_class(class_data)
        Note over OntologyCompiler: Create ClassSchema object<br/>Set name, URI, semantic_type<br/>Extract properties and superclasses<br/>Handle inheritance and restrictions
        OntologyCompiler->>ClassSchema: create(name, uri, semantic_type, properties, superclasses, description)
        ClassSchema-->>OntologyCompiler: class_schema
    end
    
    OntologyCompiler->>OntologyCompiler: _extract_properties(ontology_def)
    Note over OntologyCompiler: Convert object and data properties<br/>Extract domain, range, constraints<br/>Handle inheritance and restrictions<br/>Map to appropriate data types
    
    loop For each property
        OntologyCompiler->>OntologyCompiler: _create_ontology_property(property_data)
        Note over OntologyCompiler: Create PropertySchema object<br/>Set name, URI, data_type<br/>Extract domain, range, cardinality<br/>Handle property characteristics
        OntologyCompiler->>PropertySchema: create(name, uri, data_type, domain, range, cardinality)
        PropertySchema-->>OntologyCompiler: property_schema
    end
    
    OntologyCompiler->>OntologyCompiler: _extract_individuals(ontology_def)
    Note over OntologyCompiler: Extract OWL individuals<br/>Convert to DSPy examples<br/>Handle property values<br/>Generate structured examples
    
    OntologyCompiler->>OntologyCompiler: _extract_axioms(ontology_def)
    Note over OntologyCompiler: Extract OWL axioms<br/>Convert to validation rules<br/>Handle logical constructs<br/>Generate constraint metadata
    
    OntologyCompiler->>OntologySchema: create(ontology_uri, namespace, prefix, classes, semantic_context, examples)
    OntologySchema-->>OntologyCompiler: ontology_schema
    
    OntologyCompiler-->>PipelineOrchestrator: OntologySchema
```

## Validation Compiler Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant ValidationCompiler
    participant SHACLGraph
    participant ValidationRules
    participant ValidationRule

    PipelineOrchestrator->>ValidationCompiler: compile(shacl_graph)
    
    ValidationCompiler->>ValidationCompiler: _extract_node_shapes(shacl_graph)
    Note over ValidationCompiler: Find all SHACL node shapes<br/>Extract target classes<br/>Handle complex shape definitions<br/>Generate validation rules
    
    ValidationCompiler->>ValidationCompiler: _extract_property_shapes(shacl_graph)
    Note over ValidationCompiler: Find all SHACL property shapes<br/>Extract property paths<br/>Handle nested shapes<br/>Process constraints
    
    loop For each shape
        ValidationCompiler->>ValidationCompiler: _extract_constraints(shape)
        Note over ValidationCompiler: Extract SHACL constraints<br/>Handle cardinality, datatype, value<br/>Convert to rule format<br/>Process logical constraints
    end
    
    loop For each shape
        ValidationCompiler->>ValidationCompiler: _create_validation_rule(shape_type, shape_data)
        Note over ValidationCompiler: Create ValidationRule object<br/>Set rule ID, target class<br/>Configure property path, constraints<br/>Handle severity and messages
        ValidationCompiler->>ValidationRule: create(rule_id, target_class, property_path, constraint_type, constraint_value, severity, message, metadata)
        ValidationRule-->>ValidationCompiler: validation_rule
    end
    
    ValidationCompiler->>ValidationRules: create(target_classes, constraint_count, severity_levels, metadata)
    ValidationRules-->>ValidationCompiler: validation_rules
    
    ValidationCompiler-->>PipelineOrchestrator: ValidationRules
```

## DSPy Compiler Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant DSPyCompiler
    participant OntologySchema
    participant ValidationRules
    participant DSPySignatureDefinition
    participant DSPyModuleDefinition
    participant DSPyModelConfiguration
    participant DSPySignature

    PipelineOrchestrator->>DSPyCompiler: compile(ontology_schema, validation_rules, dspy_signatures, dspy_modules, model_config)
    
    DSPyCompiler->>DSPyCompiler: _merge_signatures(dspy_signatures)
    Note over DSPyCompiler: Combine multiple signatures<br/>Resolve conflicts<br/>Merge inputs, outputs, examples<br/>Return unified signature
    
    DSPyCompiler->>DSPyCompiler: _integrate_ontology_context(signature, ontology_schema)
    Note over DSPyCompiler: Map ontology classes to inputs/outputs<br/>Apply semantic type classification<br/>Generate semantic context<br/>Enhance with ontological info
    
    DSPyCompiler->>DSPyCompiler: _integrate_validation_rules(signature, validation_rules)
    Note over DSPyCompiler: Convert validation rules to DSPy format<br/>Apply constraints to inputs/outputs<br/>Generate validation metadata<br/>Integrate with signature validation
    
    DSPyCompiler->>DSPyCompiler: _create_dspy_signature(signature_def, module_def, model_config)
    Note over DSPyCompiler: Create DSPySignature object<br/>Set name, description, metadata<br/>Configure model and module settings<br/>Return executable signature
    
    DSPyCompiler->>DSPySignature: create(name, description, inputs, outputs, validation_rules, examples, model_config, module_config, semantic_context, ontology_metadata)
    DSPySignature-->>DSPyCompiler: dspy_signature
    
    DSPyCompiler-->>PipelineOrchestrator: DSPySignature
```

## Semantic Linker Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant SemanticLinker
    participant DSPySignature
    participant ExecutableSystem

    PipelineOrchestrator->>SemanticLinker: link(dspy_signature)
    
    SemanticLinker->>SemanticLinker: _validate_signature(signature)
    Note over SemanticLinker: Validate signature completeness<br/>Check required fields<br/>Verify input/output definitions<br/>Ensure executability
    
    SemanticLinker->>SemanticLinker: _generate_semantic_context(signature)
    Note over SemanticLinker: Extract semantic context<br/>Generate execution metadata<br/>Create semantic relationships<br/>Return structured context
    
    SemanticLinker->>SemanticLinker: _prepare_validation_rules(signature)
    Note over SemanticLinker: Prepare rules for execution<br/>Convert to executable format<br/>Generate validation metadata<br/>Return prepared rules
    
    SemanticLinker->>SemanticLinker: _generate_metadata(signature)
    Note over SemanticLinker: Generate system metadata<br/>Include ontology, model, module info<br/>Create execution tracking<br/>Return structured metadata
    
    SemanticLinker->>ExecutableSystem: create(signature, semantic_context, validation_rules, examples, model_config, module_config, shacl_validation, shacl_triples, metadata)
    ExecutableSystem-->>SemanticLinker: executable_system
    
    SemanticLinker-->>PipelineOrchestrator: ExecutableSystem
```

## Ontology Executor Detailed Sequence

```mermaid
sequenceDiagram
    participant PipelineOrchestrator
    participant OntologyExecutor
    participant ExecutableSystem
    participant ModelProvider
    participant TelemetryManager
    participant ExecutionResult

    PipelineOrchestrator->>OntologyExecutor: execute(executable_system, inputs)
    
    OntologyExecutor->>OntologyExecutor: _validate_inputs(executable_system, inputs)
    Note over OntologyExecutor: Validate against signature<br/>Check required fields<br/>Apply semantic validation<br/>Ensure completeness
    
    OntologyExecutor->>OntologyExecutor: _apply_validation_rules(executable_system, inputs)
    Note over OntologyExecutor: Apply SHACL validation<br/>Check constraint violations<br/>Generate validation results<br/>Return structured data
    
    OntologyExecutor->>OntologyExecutor: _execute_signature(executable_system, inputs)
    Note over OntologyExecutor: Execute DSPy signature<br/>Handle model provider integration<br/>Process inputs and outputs<br/>Return structured results
    
    OntologyExecutor->>ModelProvider: execute_dspy_signature(signature, inputs)
    ModelProvider-->>OntologyExecutor: model_outputs
    
    OntologyExecutor->>OntologyExecutor: _generate_telemetry(executable_system, inputs, outputs)
    Note over OntologyExecutor: Generate OpenTelemetry spans<br/>Track performance metrics<br/>Create semantic telemetry<br/>Return comprehensive data
    
    OntologyExecutor->>TelemetryManager: record_metric("execution_duration", duration)
    OntologyExecutor->>TelemetryManager: record_metric("input_count", len(inputs))
    OntologyExecutor->>TelemetryManager: record_metric("output_count", len(outputs))
    
    OntologyExecutor->>OntologyExecutor: _generate_execution_metadata(executable_system, inputs, outputs)
    Note over OntologyExecutor: Generate execution metadata<br/>Include timing and context<br/>Create tracking information<br/>Return structured metadata
    
    OntologyExecutor->>ExecutionResult: create(success, outputs, telemetry, validation_results, execution_time, metadata)
    ExecutionResult-->>OntologyExecutor: execution_result
    
    OntologyExecutor-->>PipelineOrchestrator: ExecutionResult
```

## Test Pipeline Sequence

```mermaid
sequenceDiagram
    participant TestRunner
    participant TelemetryManager
    participant PipelineOrchestrator
    participant TestPipeline
    participant FileBasedPipeline

    TestRunner->>TelemetryManager: start_span("test.pipeline", "TEST")
    
    TestRunner->>TestPipeline: test_pipeline()
    TestPipeline->>TelemetryManager: start_span("test.in_memory", "IN_MEMORY")
    TestPipeline->>PipelineOrchestrator: execute_pipeline(owl_xml, shacl_xml, dspy_xml, inputs)
    PipelineOrchestrator-->>TestPipeline: ExecutionResult
    TestPipeline->>TelemetryManager: record_metric("test_success", 1)
    TelemetryManager-->>TestPipeline: span complete
    
    TestRunner->>FileBasedPipeline: test_file_based_pipeline()
    FileBasedPipeline->>TelemetryManager: start_span("test.file_based", "FILE_BASED")
    FileBasedPipeline->>PipelineOrchestrator: execute_from_files(owl_file, shacl_file, dspy_file, inputs)
    PipelineOrchestrator-->>FileBasedPipeline: ExecutionResult
    FileBasedPipeline->>TelemetryManager: record_metric("file_test_success", 1)
    TelemetryManager-->>FileBasedPipeline: span complete
    
    TestRunner->>TelemetryManager: record_metric("total_tests", 2)
    TestRunner->>TelemetryManager: record_metric("tests_passed", 2)
    TelemetryManager-->>TestRunner: span complete
    
    TestRunner-->>TestRunner: Generate test report with telemetry
```

## Error Handling Sequence

```mermaid
sequenceDiagram
    participant Client
    participant PipelineOrchestrator
    participant TelemetryManager
    participant ErrorHandler

    Client->>PipelineOrchestrator: execute_pipeline(invalid_inputs)
    
    PipelineOrchestrator->>TelemetryManager: start_span("pipeline.error", "ERROR")
    
    Note over PipelineOrchestrator: Error occurs in any stage
    
    PipelineOrchestrator->>ErrorHandler: handle_error(error)
    ErrorHandler->>TelemetryManager: record_metric("pipeline_errors", 1)
    ErrorHandler->>TelemetryManager: record_event("error", error_details)
    
    TelemetryManager-->>PipelineOrchestrator: span complete with error status
    
    PipelineOrchestrator-->>Client: NotImplementedError with detailed message
    
    Note over Client: Fail-fast behavior<br/>No try-catch blocks<br/>Immediate error propagation<br/>Comprehensive error telemetry
```

## Data Flow Summary

### Input Data Flow
1. **OWL XML** → OWLProcessor → OWLOntologyDefinition → OntologyCompiler → OntologySchema
2. **SHACL XML** → SHACLProcessor → SHACL Graph → ValidationCompiler → ValidationRules
3. **DSPy XML** → DSPyProcessor → DSPySignatureDefinition[] → DSPyCompiler → DSPySignature

### Integration Flow
1. **OntologySchema + ValidationRules + DSPySignature** → DSPyCompiler → Integrated DSPySignature
2. **Integrated DSPySignature** → SemanticLinker → ExecutableSystem
3. **ExecutableSystem + Inputs** → OntologyExecutor → ExecutionResult

### Telemetry Flow
1. **Pipeline Stages** → TelemetryManager → OpenTelemetry Spans
2. **Validation Results** → TelemetryManager → Validation Events
3. **Execution Metrics** → TelemetryManager → Performance Metrics
4. **Error Events** → TelemetryManager → Error Tracking

### Output Data Flow
1. **ExecutionResult** → Client (success/failure, outputs, telemetry, validation, metadata)
2. **Telemetry Data** → OpenTelemetry Backend (spans, events, metrics)
3. **Validation Results** → Client (constraint violations, warnings, info)
4. **Metadata** → Client (execution context, performance data, semantic information) 