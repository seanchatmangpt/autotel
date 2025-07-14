# AUTOTEL-002: Implement CLI Run Command

**Type:** Story  
**Priority:** Critical  
**Sprint:** Phase 1  
**Category:** CLI  

## Description

Implement the `autotel run <workflow.bpmn>` command that executes a BPMN workflow with provided input data.

## User Story

As a business analyst, I want to run a workflow by providing a BPMN file and input data so that I can execute business processes.

## Acceptance Criteria

- [ ] `autotel run workflow.bpmn` executes the workflow
- [ ] `autotel run workflow.bpmn --input '{"key": "value"}'` accepts JSON input
- [ ] `autotel run workflow.bpmn --input-file data.json` accepts JSON file
- [ ] Command shows progress during execution
- [ ] Command displays final results
- [ ] Command handles missing files gracefully
- [ ] Command validates input data format
- [ ] Command shows execution time
- [ ] Command generates telemetry for execution

## Technical Requirements

- Integrate with DspyBpmnParser
- Use AutoTelWorkflowManager for execution
- Parse JSON input data
- Handle file I/O for input files
- Implement progress indicators
- Add telemetry spans for CLI operations
- Validate BPMN file exists and is readable

## Definition of Done

### For All Tickets:
- [ ] Code follows AutoTel coding standards
- [ ] No hardcoded values in XML
- [ ] No CDATA sections in XML
- [ ] Telemetry generated for all operations
- [ ] Tests written and passing
- [ ] Documentation updated

### For CLI Tickets:
- [ ] Command works with `autotel` CLI
- [ ] Help text provided
- [ ] Error handling implemented
- [ ] Integration tests passing

## Dependencies

- AUTOTEL-001: Implement Core CLI Interface
- AUTOTEL-010: Runtime Workflow Execution

## Estimation

**Story Points:** 8  
**Effort:** 2 days

## Test Cases

- [ ] Run simple workflow with inline JSON
- [ ] Run workflow with JSON file
- [ ] Handle missing BPMN file
- [ ] Handle invalid JSON input
- [ ] Handle workflow execution errors
- [ ] Verify telemetry generation 