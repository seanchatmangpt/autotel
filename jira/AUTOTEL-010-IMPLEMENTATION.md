# AUTOTEL-010 Implementation Guide: Runtime Workflow Execution

## Context

The runtime workflow execution engine is the core component that actually runs BPMN workflows with all five pillars (BPMN, DMN, DSPy, SHACL, OWL) integrated.

## Working

### Existing Components
- `autotel/workflows/dspy_bpmn_parser.py` - Can parse BPMN XML and extract five pillars
- `autotel/workflows/execution_engine.py` - Comprehensive execution framework with `AutoTelWorkflowManager`
- `autotel/core/telemetry.py` - Telemetry manager with OpenTelemetry integration
- `autotel/utils/owl_integration.py` - OWL ontology processing
- `SpiffWorkflow` - BPMN execution engine
- `pyshacl` - SHACL validation library
- `rdflib` - OWL/RDF processing
- Clean separation between XML parsing and execution logic

### Available Infrastructure
- DSPy signature definitions from XML
- SHACL shapes parsing
- OWL ontology parsing
- DMN decision table parsing
- Basic workflow context management

## Not Working

### Missing Runtime Execution
- Basic workflow execution exists but needs CLI integration (AUTOTEL-002)
- Limited integration between five pillars at runtime
- Basic workflow context management exists
- Basic error handling exists
- Basic performance monitoring exists
- Basic telemetry generation exists but needs enhancement

### Missing Integration
- DSPy services not called during execution
- SHACL validation not performed on runtime data
- OWL validation not performed during execution
- DMN decisions not evaluated with real data
- No data flow between pillars

## Implementation Steps

### 1. Complete Execution Engine
**File:** `autotel/workflows/execution_engine.py`

**Tasks:**
- Enhance `AutoTelExecutionEngine.execute_specification()` (already exists)
- Enhance workflow context management (already exists)
- Enhance error handling and recovery (already exists)
- Enhance performance monitoring (already exists)
- Enhance telemetry spans for execution (already exists)

### 2. Implement Workflow Context
**File:** `autotel/workflows/context.py`

**Tasks:**
- Create `WorkflowContext` class
- Manage input data throughout execution
- Track intermediate results
- Handle data flow between tasks
- Maintain execution state

### 3. Integrate DSPy Services
**File:** `autotel/workflows/dspy_integration.py`

**Tasks:**
- Implement DSPy service calling during execution
- Handle dynamic signature resolution
- Manage DSPy service parameters
- Handle DSPy service errors
- Add telemetry for DSPy calls

### 4. Integrate SHACL Validation
**File:** `autotel/workflows/shacl_integration.py`

**Tasks:**
- Implement runtime SHACL validation
- Validate input data against shapes
- Validate intermediate results
- Validate output data
- Handle validation errors

### 5. Integrate OWL Validation
**File:** `autotel/workflows/owl_integration.py`

**Tasks:**
- Implement runtime OWL validation
- Validate data against ontology
- Perform ontology reasoning
- Handle ontology validation errors
- Add telemetry for OWL operations

### 6. Integrate DMN Decisions
**File:** `autotel/workflows/dmn_integration.py`

**Tasks:**
- Implement DMN decision evaluation
- Handle decision table execution
- Manage decision input/output
- Handle decision errors
- Add telemetry for decisions

### 7. Create Task Executors
**File:** `autotel/workflows/task_executors.py`

**Tasks:**
- Implement `ServiceTaskExecutor`
- Implement `BusinessRuleTaskExecutor`
- Implement `UserTaskExecutor`
- Handle task-specific logic
- Add telemetry for each task type

## Testing Strategy

### Unit Tests
**File:** `tests/test_workflow_execution.py`

**Test Cases:**
- Simple BPMN workflow execution
- DSPy service integration
- SHACL validation during execution
- OWL validation during execution
- DMN decision evaluation
- Error handling scenarios

### Integration Tests
**File:** `tests/test_five_pillars_execution.py`

**Test Cases:**
- Complete five pillars workflow
- Data flow between pillars
- Error recovery scenarios
- Performance testing
- Telemetry generation

## Dependencies

### External Libraries
- `SpiffWorkflow` - BPMN execution
- `pyshacl` - SHACL validation
- `rdflib` - OWL/RDF processing
- `dspy-ai` - DSPy services
- `opentelemetry` - Telemetry generation

### Internal Dependencies
- `autotel/workflows/dspy_bpmn_parser.py` - XML parsing
- `autotel/core/telemetry.py` - Telemetry management
- `autotel/utils/dspy_services.py` - DSPy service registry

## Success Criteria

### Functional
- [ ] BPMN process executes from start to end
- [ ] DMN business rules are evaluated
- [ ] DSPy services are called with proper signatures
- [ ] SHACL validation is performed on data
- [ ] OWL ontology validation is performed
- [ ] Workflow context is maintained throughout execution

### Technical
- [ ] Error handling and recovery is implemented
- [ ] Execution results are returned
- [ ] Performance meets requirements (< 5 seconds for simple workflows)
- [ ] All five pillars are integrated
- [ ] Telemetry is generated for all activities

### User Experience
- [ ] Workflows execute reliably
- [ ] Error messages are clear
- [ ] Performance is acceptable
- [ ] Results are consistent

## Common Pitfalls

### Avoid
- Not handling workflow context properly
- Not integrating all five pillars
- Poor error handling
- Not generating telemetry
- Not managing performance

### Best Practices
- Maintain workflow context throughout execution
- Handle all error cases gracefully
- Generate telemetry for all activities
- Monitor performance continuously
- Test all pillar integrations thoroughly 