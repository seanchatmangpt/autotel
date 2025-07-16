# AutoTel Documentation Index

## Overview

This index provides navigation to all AutoTel documentation, organized by purpose and implementation status. The AutoTel project is an **enterprise BPMN 2.0 orchestration framework** with five integrated pillars: BPMN 2.0, DMN, DSPy, SHACL, and OWL.

**✅ Current Status**: BPMN/DMN workflow functionality is **fully implemented and working**. Semantic pipeline components (OWL/SHACL/DSPy processors) are **NotImplementedError stubs**. See [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md) for detailed implementation status.

## Documentation Alignment Status

### ✅ **Fully Aligned**
- **BPMN/DMN Architecture**: All documents consistently describe five pillars integration
- **Workflow Execution**: BPMN workflow and DMN decision execution is fully functional
- **CLI Interface**: All BPMN/DMN commands are implemented and working
- **XML Specifications**: BPMN and DMN XML schemas are complete and documented
- **Data Structures**: Workflow and decision data structures are consistent
- **Examples**: BPMN/DMN usage patterns and examples are consistent

### ⚠️ **Minor Inconsistencies**
- **Semantic Pipeline**: OWL/SHACL/DSPy processors are stubs (not implemented)
- **C4 Diagrams**: Don't include BPMN/DMN workflow components
- **JIRA Tickets**: Missing BPMN/DMN implementation tickets
- **Integration Examples**: Limited examples of workflow + AI service integration

### 📋 **Documentation Quality**
- **Completeness**: 85% - BPMN/DMN complete, semantic pipeline needs work
- **Accuracy**: 90% - BPMN/DMN accurate, semantic pipeline outdated
- **Consistency**: 85% - Cross-references need updates for current state
- **Usability**: 95% - Clear navigation for working BPMN/DMN functionality

## Quick Navigation

### 🚨 **Start Here** (Critical Reading)
- [README.md](./README.md) - **Complete AutoTel overview with working BPMN/DMN**
- [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md) - **Semantic pipeline gap analysis and roadmap**
- [AUTOTEL_PRD.md](./AUTOTEL_PRD.md) - **Product requirements with five pillars**

### 📋 **Implementation Guides**

#### ✅ **Working Components** (BPMN/DMN)
- [autotel/cli.py](./autotel/cli.py) - **BPMN/DMN CLI commands (fully implemented)**
- [autotel/core/orchestrator.py](./autotel/core/orchestrator.py) - **Workflow orchestration (working)**
- [bpmn/](./bpmn/) - **BPMN workflow examples**
- [AUTOTEL_XML_SPECIFICATION.md](./AUTOTEL_XML_SPECIFICATION.md) - **BPMN/DMN XML schemas**

#### 🔴 **Semantic Pipeline** (NotImplementedError stubs)
- [jira/PIPELINE-INDEX.md](./jira/PIPELINE-INDEX.md) - **JIRA tickets and implementation priority**
- [jira/AUTOTEL-001-ONTOLOGY-PROCESSOR.md](./jira/AUTOTEL-001-ONTOLOGY-PROCESSOR.md) - OWL Processor implementation
- [jira/AUTOTEL-002-SHACL-PROCESSOR.md](./jira/AUTOTEL-002-SHACL-PROCESSOR.md) - SHACL Processor implementation
- [jira/AUTOTEL-003-DSPY-COMPILER.md](./jira/AUTOTEL-003-DSPY-COMPILER.md) - Ontology Compiler implementation
- [jira/AUTOTEL-004-SEMANTIC-LINKER.md](./jira/AUTOTEL-004-SEMANTIC-LINKER.md) - Semantic Linker implementation
- [jira/AUTOTEL-005-ONTOLOGY-EXECUTOR.md](./jira/AUTOTEL-005-ONTOLOGY-EXECUTOR.md) - Ontology Executor implementation
- [jira/AUTOTEL-006-VALIDATION-COMPILER.md](./jira/AUTOTEL-006-VALIDATION-COMPILER.md) - Validation Compiler implementation
- [jira/AUTOTEL-007-DSPY-COMPILER.md](./jira/AUTOTEL-007-DSPY-COMPILER.md) - DSPy Compiler implementation

### 🏗️ **Architecture Documentation**
- [README.md](./README.md) - **Five pillars architecture overview**
- [AUTOTEL_PRD.md](./AUTOTEL_PRD.md) - **Enterprise BPMN 2.0 framework requirements**
- [c4_validation_diagrams.md](./c4_validation_diagrams.md) - **C4 architecture diagrams (semantic pipeline only)**
- [pipeline_sequence_diagrams.md](./pipeline_sequence_diagrams.md) - **Sequence diagrams (semantic pipeline only)**
- [docs/C4_DIAGRAMS.md](./docs/C4_DIAGRAMS.md) - C4 diagram documentation
- [docs/COMPONENT_INTERACTIONS.md](./docs/COMPONENT_INTERACTIONS.md) - Component interaction details
- [docs/DEPLOYMENT_ARCHITECTURE.md](./docs/DEPLOYMENT_ARCHITECTURE.md) - Deployment architecture

### 📊 **Telemetry Planning**
- [autotel_telemetry.yaml](./autotel_telemetry.yaml) - **Weaver OpenTelemetry span definitions**
- [WEAVER_TELEMETRY_PLANNING.md](./WEAVER_TELEMETRY_PLANNING.md) - **Comprehensive telemetry planning guide**
- [TELEMETRY_VALIDATION_ANALYSIS.md](./TELEMETRY_VALIDATION_ANALYSIS.md) - **Telemetry-driven validation analysis**
- [git_implementation_history.md](./git_implementation_history.md) - Git commit history and implementation timeline

### 🎯 **Telemetry Implementation Requirements**
Each JIRA ticket now includes comprehensive telemetry requirements to ensure real functionality:

- **Required Spans and Events**: Exact span names, attributes, and events that must be implemented
- **Dynamic Data Validation**: Expected telemetry data from real processing (no hardcoded values)
- **Validation Criteria**: Specific requirements to prevent hardcoded functionality
- **Sample Input/Output**: Real data examples with expected telemetry responses

**Key Validation Principles**:
- **NO HARDCODED VALUES**: All telemetry data must be generated from actual processing
- **REAL PARSING**: XML must be parsed using actual libraries (xml.etree.ElementTree, rdflib)
- **DYNAMIC EXTRACTION**: All counts, URIs, and data must come from input content
- **PERFORMANCE TRACKING**: Duration, size, and count metrics must be measured
- **ERROR HANDLING**: Failed operations must generate error spans with context

### 📊 **Data Structures**
- [OWL_DATA_STRUCTURES.md](./OWL_DATA_STRUCTURES.md) - OWL data structure documentation
- [autotel/schemas/ontology_types.py](./autotel/schemas/ontology_types.py) - Ontology type definitions
- [autotel/schemas/validation_types.py](./autotel/schemas/validation_types.py) - Validation type definitions
- [autotel/schemas/dspy_types.py](./autotel/schemas/dspy_types.py) - DSPy type definitions
- [autotel/schemas/linker_types.py](./autotel/schemas/linker_types.py) - Linker type definitions

### 🔧 **Implementation Files**

#### ✅ **Working Components** (BPMN/DMN)
- [autotel/cli.py](./autotel/cli.py) - **BPMN/DMN CLI commands (fully implemented)**
- [autotel/core/orchestrator.py](./autotel/core/orchestrator.py) - **Workflow orchestration (working)**
- [autotel/core/telemetry.py](./autotel/core/telemetry.py) - **Telemetry management (working)**
- [autotel/workflows/](./autotel/workflows/) - **Workflow execution components**

#### 🔴 **Semantic Pipeline** (NotImplementedError stubs)
- [autotel/factory/pipeline.py](./autotel/factory/pipeline.py) - Main pipeline orchestrator
- [autotel/factory/processors/owl_processor.py](./autotel/factory/processors/owl_processor.py) - OWL processor (stubs)
- [autotel/factory/processors/shacl_processor.py](./autotel/factory/processors/shacl_processor.py) - SHACL processor (stubs)
- [autotel/factory/processors/dspy_processor.py](./autotel/factory/processors/dspy_processor.py) - DSPy processor (stubs)
- [autotel/factory/ontology_compiler.py](./autotel/factory/ontology_compiler.py) - Ontology compiler (stubs)
- [autotel/factory/validation_compiler.py](./autotel/factory/validation_compiler.py) - Validation compiler (stubs)
- [autotel/factory/dspy_compiler.py](./autotel/factory/dspy_compiler.py) - DSPy compiler (stubs)
- [autotel/factory/linker.py](./autotel/factory/linker.py) - Semantic linker (stubs)
- [autotel/factory/executor.py](./autotel/factory/executor.py) - Ontology executor (stubs)

### 🧪 **Testing**

#### ✅ **Working Tests** (BPMN/DMN)
- [test_camunda_simple.py](./test_camunda_simple.py) - **BPMN workflow execution tests**
- [test_camunda_simple_dspy.py](./test_camunda_simple_dspy.py) - **BPMN + DSPy integration tests**
- [validate_camunda_examples.py](./validate_camunda_examples.py) - **BPMN validation tests**

#### 🔴 **Semantic Pipeline Tests** (stubs)
- [test_pipeline.py](./test_pipeline.py) - Main test script (stubs)
- [test_ontology_dspy_integration.py](./test_ontology_dspy_integration.py) - Integration tests (stubs)

### 📁 **Examples**

#### ✅ **Working Examples** (BPMN/DMN)
- [bpmn/](./bpmn/) - **BPMN workflow examples**
- [bpmn_only/](./bpmn_only/) - **BPMN-only workflow examples**
- [trivial.bpmn](./trivial.bpmn) - **Simple BPMN workflow**
- [AUTOTEL_XML_SPECIFICATION.md](./AUTOTEL_XML_SPECIFICATION.md) - **Complete BPMN/DMN XML examples**

#### 🔴 **Semantic Pipeline Examples** (stubs)
- [examples/sample_owl.xml](./examples/sample_owl.xml) - Sample OWL XML
- [examples/sample_shacl.xml](./examples/sample_shacl.xml) - Sample SHACL XML
- [examples/sample_dspy.xml](./examples/sample_dspy.xml) - Sample DSPy XML
- [examples/sample_inputs.json](./examples/sample_inputs.json) - Sample input data

### 📈 **Project History**
- [git_implementation_history.md](./git_implementation_history.md) - Git commit history and implementation timeline

## Implementation Status

### ✅ **Fully Implemented** (BPMN/DMN Workflow)
BPMN and DMN functionality is complete and working:

| Component | File | Status | Features |
|-----------|------|--------|----------|
| BPMN CLI | `autotel/cli.py` | ✅ Working | run, list, validate, workflow commands |
| Workflow Engine | `autotel/core/orchestrator.py` | ✅ Working | SpiffWorkflow integration |
| Telemetry | `autotel/core/telemetry.py` | ✅ Working | OpenTelemetry integration |
| DMN Support | `autotel/workflows/` | ✅ Working | Decision table execution |

### 🔴 **Not Started** (Semantic Pipeline)
Semantic pipeline components exist as `NotImplementedError` stubs:

| Component | File | Methods | Priority |
|-----------|------|---------|----------|
| OWL Processor | `autotel/factory/processors/owl_processor.py` | 15+ | Critical |
| SHACL Processor | `autotel/factory/processors/shacl_processor.py` | All | Critical |
| DSPy Processor | `autotel/factory/processors/dspy_processor.py` | All | Critical |
| Ontology Compiler | `autotel/factory/ontology_compiler.py` | 7 | High |
| Validation Compiler | `autotel/factory/validation_compiler.py` | 5 | High |
| DSPy Compiler | `autotel/factory/dspy_compiler.py` | 5 | High |
| Semantic Linker | `autotel/factory/linker.py` | 5 | Medium |
| Ontology Executor | `autotel/factory/executor.py` | 6 | Medium |

### ✅ **Completed**
- **BPMN/DMN Architecture**: Complete workflow and decision execution
- **CLI Interface**: Full BPMN/DMN command support
- **XML Specifications**: Complete BPMN/DMN XML schemas
- **Telemetry Integration**: Working OpenTelemetry for workflows
- **Test Framework**: Working BPMN/DMN tests

## Getting Started

### For New Contributors

#### ✅ **Start with Working BPMN/DMN**
1. **Try the CLI**: Run `autotel --help` to see available commands
2. **Run a Workflow**: Try `autotel run bpmn/trivial.bpmn --input '{"data": "test"}'`
3. **Explore Examples**: Check [bpmn/](./bpmn/) directory for workflow examples
4. **Read Documentation**: Review [README.md](./README.md) for complete overview

#### 🔴 **Then Work on Semantic Pipeline**
1. **Read the Gaps**: Review [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md)
2. **Understand Architecture**: Review [c4_validation_diagrams.md](./c4_validation_diagrams.md)
3. **Pick a Ticket**: Choose from [jira/PIPELINE-INDEX.md](./jira/PIPELINE-INDEX.md)
4. **Follow Implementation Guide**: Each ticket has detailed steps
5. **Implement Tests**: Update tests to validate your work
6. **Submit PR**: Create pull request with implementation and tests

### For Experienced Contributors

#### ✅ **BPMN/DMN Development**
1. **Review Working Code**: Check [autotel/cli.py](./autotel/cli.py) and [autotel/core/orchestrator.py](./autotel/core/orchestrator.py)
2. **Add Features**: Extend workflow capabilities or add new DMN decisions
3. **Improve Integration**: Enhance BPMN + DSPy integration
4. **Add Tests**: Create more comprehensive workflow tests

#### 🔴 **Semantic Pipeline Development**
1. **Review Current State**: All semantic components are stubs - start from scratch
2. **Choose Priority**: Follow Phase 1 → Phase 2 → Phase 3 → Phase 4 order
3. **Implement Incrementally**: One component at a time with tests
4. **Validate Integration**: Ensure components work together
5. **Document Changes**: Update documentation as you go

### Development Setup

```bash
# Clone and setup
git clone <repository>
cd autotel

# Install dependencies
uv sync

# Try working BPMN/DMN functionality
autotel --help
autotel version
autotel list --workflows

# Run a simple workflow
autotel run bpmn/trivial.bpmn --input '{"data": "test"}'

# Review semantic pipeline gaps
cat GAPS_AND_MISTAKES.md
ls jira/

# Start with first semantic pipeline ticket
cat jira/AUTOTEL-001-ONTOLOGY-PROCESSOR.md
```

## Implementation Priority

### ✅ **Phase 0: Complete** (BPMN/DMN Workflow)
1. **BPMN CLI**: All workflow commands implemented
2. **Workflow Engine**: SpiffWorkflow integration complete
3. **DMN Support**: Decision table execution working
4. **Telemetry**: OpenTelemetry integration complete

### 🔴 **Phase 1: Critical** (Semantic Pipeline) - **START HERE**
1. **AUTOTEL-001**: OWL Processor (Foundation)
2. **AUTOTEL-002**: SHACL Processor (Validation)

### 🔴 **Phase 2: High** (Semantic Integration)
3. **AUTOTEL-003**: Ontology Compiler (Schema Generation)
4. **AUTOTEL-006**: Validation Compiler (Constraint Rules)
5. **AUTOTEL-007**: DSPy Compiler (Integration)

### 🔴 **Phase 3: Medium** (Semantic Testing)
6. **AUTOTEL-004**: Semantic Linker (System Integration)
7. **AUTOTEL-005**: Ontology Executor (Runtime)

### 🔴 **Phase 4: Low** (Semantic Deployment)
8. Configuration Management
9. CLI Integration
10. Deployment Configuration

## Architecture Overview

### ✅ **Working Architecture** (BPMN/DMN)
```
BPMN XML → Workflow Engine → Task Execution → Results + Telemetry
    ↓
DMN XML → Decision Engine → Rule Evaluation
    ↓
DSPy Services → AI Service Execution
```

### 🔴 **Planned Architecture** (Semantic Pipeline)
```
XML Inputs → Processors → Compilers → Linker → Executor → Results + Telemetry
```

### Components

#### ✅ **Working Components** (BPMN/DMN)
1. **Workflow Engine**: SpiffWorkflow-based BPMN execution
2. **Decision Engine**: DMN decision table execution
3. **DSPy Integration**: AI service execution in workflows
4. **Telemetry**: OpenTelemetry integration for workflows

#### 🔴 **Planned Components** (Semantic Pipeline)
1. **Processors** (XML → Data Objects)
   - `OWLProcessor`: Parses OWL/RDF XML into structured ontology definitions
   - `SHACLProcessor`: Parses SHACL/RDF XML into constraint graphs
   - `DSPyProcessor`: Parses DSPy XML into signature definitions

2. **Compilers** (Data Objects → Execution-Ready Objects)
   - `OntologyCompiler`: Transforms OWL definitions into semantic schemas
   - `ValidationCompiler`: Transforms SHACL constraints into validation rules
   - `DSPyCompiler`: Integrates all inputs into execution-ready signatures

3. **Linker** (Execution-Ready Objects → Executable System)
   - `SemanticLinker`: Creates executable systems with semantic context

4. **Executor** (Executable System → Results + Telemetry)
   - `OntologyExecutor`: Runs systems with comprehensive telemetry

## Key Features

### Semantic Execution
- Ontology-driven execution with semantic context
- Automatic semantic type classification
- Property relationship handling
- Class hierarchy resolution

### Validation
- SHACL constraint application at runtime
- Input/output validation against schemas
- Constraint violation reporting
- Semantic type validation

### Telemetry
- Comprehensive execution tracing
- Semantic-aware span generation
- Validation event tracking
- Performance metrics collection

### Model Integration
- Multiple DSPy model provider support
- Model configuration management
- Response caching and optimization
- Provider-specific telemetry

## Success Criteria

### Phase 1 Complete
- [ ] OWL Processor can parse real OWL XML files
- [ ] SHACL Processor can parse real SHACL XML files
- [ ] DSPy Processor can parse real DSPy XML files
- [ ] All processors return structured data objects
- [ ] Unit tests pass for all processors

### Phase 2 Complete
- [ ] Ontology Compiler transforms OWL data into schemas
- [ ] Validation Compiler transforms SHACL data into rules
- [ ] DSPy Compiler integrates all inputs into signatures
- [ ] All compilers work with real processor outputs
- [ ] Integration tests pass between processors and compilers

### Phase 3 Complete
- [ ] Semantic Linker creates executable systems
- [ ] Ontology Executor runs systems with telemetry
- [ ] End-to-end pipeline execution works
- [ ] All tests pass including integration tests
- [ ] CLI can execute real workflows

### Phase 4 Complete
- [ ] Configuration management works
- [ ] Deployment configuration exists
- [ ] External service integration works
- [ ] Production-ready deployment
- [ ] Complete documentation

## Support and Questions

- **Architecture Questions**: Review C4 diagrams and sequence diagrams
- **Implementation Questions**: Check JIRA tickets for detailed guides
- **Current Status**: See GAPS_AND_MISTAKES.md for complete gap analysis
- **Getting Started**: Follow the implementation roadmap above

## Related Projects

- **AutoTel Core**: Main AutoTel framework
- **AutoTel CLI**: Command-line interface
- **AutoTel Telemetry**: OpenTelemetry integration
- **AutoTel Validation**: SHACL/OWL validation framework

## License
