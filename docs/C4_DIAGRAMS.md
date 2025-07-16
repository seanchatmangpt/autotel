# AutoTel C4 Architecture Diagrams

## Overview
This document contains C4 model diagrams for the AutoTel semantic execution pipeline, focusing on the multi-compiler architecture during the prototype phase.

## C4 Level 1: System Context Diagram

```mermaid
graph TB
    User[User/Developer] --> AutoTel[AutoTel System]
    AutoTel --> OWL[OWL/RDF XML Files]
    AutoTel --> SHACL[SHACL/RDF XML Files]
    AutoTel --> DSPy[DSPy XML Files]
    AutoTel --> Telemetry[OpenTelemetry System]
    AutoTel --> Models[LLM Model Providers]
    
    subgraph "AutoTel System"
        Pipeline[Semantic Execution Pipeline]
    end
    
    subgraph "External Systems"
        OWL
        SHACL
        DSPy
        Telemetry
        Models
    end
    
    subgraph "Users"
        User
    end
```

**Description**: The AutoTel system processes ontology and validation XML files to execute DSPy signatures with semantic context and comprehensive telemetry.

## C4 Level 2: Container Diagram

```mermaid
graph TB
    CLI[CLI Interface] --> Pipeline[Pipeline Orchestrator]
    Pipeline --> OWLProc[OWL Processor]
    Pipeline --> SHACLProc[SHACL Processor]
    Pipeline --> DSPyProc[DSPy Processor]
    
    OWLProc --> OntologyComp[Ontology Compiler]
    SHACLProc --> ValidationComp[Validation Compiler]
    DSPyProc --> DSPyComp[DSPy Compiler]
    
    OntologyComp --> DSPyComp
    ValidationComp --> DSPyComp
    
    DSPyComp --> Linker[Semantic Linker]
    Linker --> Executor[Ontology Executor]
    
    Executor --> Telemetry[Telemetry System]
    Executor --> Models[Model Providers]
    
    subgraph "AutoTel Containers"
        CLI
        Pipeline
        OWLProc
        SHACLProc
        DSPyProc
        OntologyComp
        ValidationComp
        DSPyComp
        Linker
        Executor
    end
    
    subgraph "External Systems"
        Telemetry
        Models
    end
```

**Description**: The container diagram shows the main architectural components and their relationships in the AutoTel pipeline.

## C4 Level 3: Component Diagram - Pipeline Orchestrator

```mermaid
graph TB
    Pipeline[Pipeline Orchestrator] --> OWLProc[OWL Processor]
    Pipeline --> SHACLProc[SHACL Processor]
    Pipeline --> DSPyProc[DSPy Processor]
    Pipeline --> OntologyComp[Ontology Compiler]
    Pipeline --> ValidationComp[Validation Compiler]
    Pipeline --> DSPyComp[DSPy Compiler]
    Pipeline --> Linker[Semantic Linker]
    Pipeline --> Executor[Ontology Executor]
    
    subgraph "Pipeline Orchestrator Components"
        Pipeline
        OWLProc
        SHACLProc
        DSPyProc
        OntologyComp
        ValidationComp
        DSPyComp
        Linker
        Executor
    end
```

**Description**: The Pipeline Orchestrator coordinates the execution flow through all pipeline stages.

## C4 Level 3: Component Diagram - Compiler Architecture

```mermaid
graph TB
    OWLProc[OWL Processor] --> OntologyComp[Ontology Compiler]
    SHACLProc[SHACL Processor] --> ValidationComp[Validation Compiler]
    DSPyProc[DSPy Processor] --> DSPyComp[DSPy Compiler]
    
    OntologyComp --> DSPyComp
    ValidationComp --> DSPyComp
    
    subgraph "Data Flow"
        OWLData[OWLOntologyDefinition]
        SHACLData[Graph]
        DSPyData[DSPySignatureDefinition]
        OntologySchema[OntologySchema]
        ValidationRules[ValidationRules]
        DSPySignature[DSPySignature]
    end
    
    OWLProc --> OWLData
    SHACLProc --> SHACLData
    DSPyProc --> DSPyData
    
    OntologyComp --> OntologySchema
    ValidationComp --> ValidationRules
    DSPyComp --> DSPySignature
    
    OWLData --> OntologyComp
    SHACLData --> ValidationComp
    DSPyData --> DSPyComp
    OntologySchema --> DSPyComp
    ValidationRules --> DSPyComp
```

**Description**: The compiler architecture shows the data flow between processors and compilers, with the DSPy compiler integrating outputs from all other compilers.

## C4 Level 4: Code Diagram - Ontology Compiler

```mermaid
graph TB
    OntologyCompiler[OntologyCompiler] --> SemanticClassifier[SemanticClassifier]
    OntologyCompiler --> SchemaGenerator[SchemaGenerator]
    OntologyCompiler --> ContextGenerator[ContextGenerator]
    
    SemanticClassifier --> ClassSchema[ClassSchema]
    SchemaGenerator --> PropertySchema[PropertySchema]
    ContextGenerator --> OntologySchema[OntologySchema]
    
    subgraph "Input"
        OWLOntologyDefinition
    end
    
    subgraph "Output"
        OntologySchema
        ClassSchema
        PropertySchema
    end
    
    OWLOntologyDefinition --> OntologyCompiler
    OntologyCompiler --> OntologySchema
```

**Description**: The Ontology Compiler transforms OWL definitions into structured schemas with semantic classifications.

## C4 Level 4: Code Diagram - Validation Compiler

```mermaid
graph TB
    ValidationCompiler[ValidationCompiler] --> ConstraintExtractor[ConstraintExtractor]
    ValidationCompiler --> RuleGenerator[RuleGenerator]
    ValidationCompiler --> SeverityHandler[SeverityHandler]
    
    ConstraintExtractor --> ValidationRule[ValidationRule]
    RuleGenerator --> ValidationRules[ValidationRules]
    SeverityHandler --> ConstraintMetadata[ConstraintMetadata]
    
    subgraph "Input"
        SHACLGraph
    end
    
    subgraph "Output"
        ValidationRules
        ValidationRule
        ConstraintMetadata
    end
    
    SHACLGraph --> ValidationCompiler
    ValidationCompiler --> ValidationRules
```

**Description**: The Validation Compiler extracts SHACL constraints and generates structured validation rules.

## C4 Level 4: Code Diagram - DSPy Compiler

```mermaid
graph TB
    DSPyCompiler[DSPyCompiler] --> SchemaIntegrator[SchemaIntegrator]
    DSPyCompiler --> ValidationIntegrator[ValidationIntegrator]
    DSPyCompiler --> ConfigIntegrator[ConfigIntegrator]
    
    SchemaIntegrator --> DSPySignature[DSPySignature]
    ValidationIntegrator --> ValidationRules
    ConfigIntegrator --> ModelConfig[DSPyModelConfiguration]
    ConfigIntegrator --> ModuleConfig[DSPyModuleDefinition]
    
    subgraph "Inputs"
        OntologySchema
        ValidationRules
        DSPySignatureDefinition
        DSPyModuleDefinition
        DSPyModelConfiguration
    end
    
    subgraph "Output"
        DSPySignature
    end
    
    OntologySchema --> SchemaIntegrator
    ValidationRules --> ValidationIntegrator
    DSPySignatureDefinition --> DSPyCompiler
    DSPyModuleDefinition --> ConfigIntegrator
    DSPyModelConfiguration --> ConfigIntegrator
    
    DSPyCompiler --> DSPySignature
```

**Description**: The DSPy Compiler integrates all inputs to generate execution-ready DSPy signatures.

## Data Flow Sequence Diagram

```mermaid
sequenceDiagram
    participant CLI as CLI Interface
    participant Pipeline as Pipeline Orchestrator
    participant OWLProc as OWL Processor
    participant SHACLProc as SHACL Processor
    participant DSPyProc as DSPy Processor
    participant OntologyComp as Ontology Compiler
    participant ValidationComp as Validation Compiler
    participant DSPyComp as DSPy Compiler
    participant Linker as Semantic Linker
    participant Executor as Ontology Executor
    
    CLI->>Pipeline: Execute pipeline
    Pipeline->>OWLProc: Parse OWL XML
    OWLProc-->>Pipeline: OWLOntologyDefinition
    Pipeline->>SHACLProc: Parse SHACL XML
    SHACLProc-->>Pipeline: Graph
    Pipeline->>DSPyProc: Parse DSPy XML
    DSPyProc-->>Pipeline: DSPySignatureDefinition
    
    Pipeline->>OntologyComp: Compile ontology
    OntologyComp-->>Pipeline: OntologySchema
    Pipeline->>ValidationComp: Compile validation
    ValidationComp-->>Pipeline: ValidationRules
    
    Pipeline->>DSPyComp: Compile DSPy signature
    DSPyComp-->>Pipeline: DSPySignature
    
    Pipeline->>Linker: Link system
    Linker-->>Pipeline: ExecutableSystem
    Pipeline->>Executor: Execute system
    Executor-->>Pipeline: Results + Telemetry
    Pipeline-->>CLI: Execution results
```

**Description**: The sequence diagram shows the complete flow through the multi-compiler pipeline.

## Implementation Guidelines

### File Structure
```
autotel/
├── factory/
│   ├── pipeline.py              # Pipeline orchestration
│   ├── ontology_compiler.py     # Ontology compiler
│   ├── validation_compiler.py   # Validation compiler
│   ├── dspy_compiler.py         # DSPy compiler
│   ├── linker.py                # Semantic linker
│   ├── executor.py              # Ontology executor
│   └── processors/
│       ├── owl_processor.py     # OWL processor
│       ├── shacl_processor.py   # SHACL processor
│       └── dspy_processor.py    # DSPy processor
├── schemas/
│   ├── ontology_types.py        # Ontology data structures
│   ├── validation_types.py      # Validation data structures
│   ├── dspy_types.py           # DSPy data structures
│   └── linker_types.py         # Linker data structures
└── core/
    └── telemetry.py            # Telemetry integration
```

### Key Design Principles

1. **Single Responsibility**: Each compiler has one clear purpose
2. **Fail Fast**: No try-catch blocks, let errors crash naturally
3. **Structured Data**: Use dataclasses for all data structures
4. **Clean Dependencies**: Clear separation between components
5. **Telemetry First**: Comprehensive observability throughout

### Integration Points

- **Processors**: Parse XML/RDF into structured data objects
- **Compilers**: Transform data objects into execution-ready objects
- **Linker**: Create executable systems with semantic context
- **Executor**: Run systems with telemetry and validation

This architecture provides a clean, modular design that's easy to implement, test, and maintain while preserving all semantic execution capabilities. 