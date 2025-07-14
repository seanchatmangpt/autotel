# AUTOTEL-002 Implementation Guide: CLI Run Command

## Context

The `autotel run` command is the primary way users execute workflows. It must accept BPMN files and input data, then execute the workflow and display results.

## Working

### Existing Components
- `autotel/workflows/dspy_bpmn_parser.py` - Can parse BPMN XML
- `autotel/workflows/execution_engine.py` - Has execution framework with `AutoTelWorkflowManager`
- `autotel/cli.py` - CLI structure exists with 7 commands (missing `run`)
- `autotel/core/telemetry.py` - Telemetry manager with OpenTelemetry integration

### Available Infrastructure
- Typer CLI framework
- Rich for terminal output
- JSON parsing capabilities
- File I/O utilities

## Not Working

### Missing Run Command
- No `autotel run` command implementation
- No input data parsing (JSON or file)
- No workflow execution integration
- No progress indicators during execution
- No result display
- No error handling for execution failures

### Missing Integration
- CLI doesn't connect to execution engine
- No telemetry integration during execution
- No validation of input data format
- No handling of missing files

## Implementation Steps

### 1. Add Run Command to CLI
**File:** `autotel/cli.py`

**Tasks:**
- Add `@app.command()` for run command
- Add parameters for BPMN file path
- Add options for `--input` and `--input-file`
- Add help text for the command
- Add error handling decorator

### 2. Create Run Command Logic
**File:** `autotel/cli.py`

**Tasks:**
- Implement `run_workflow()` function in CLI
- Parse input data (JSON string or file)
- Validate BPMN file exists
- Call `AutoTelWorkflowManager` from execution engine
- Display progress during execution
- Format and display results

### 3. Input Data Handling
**File:** `autotel/cli.py`

**Tasks:**
- Implement `parse_input_data()` function in CLI
- Handle JSON string input
- Handle JSON file input
- Validate JSON format
- Handle file not found errors

### 4. Progress Indicators
**File:** `autotel/cli.py`

**Tasks:**
- Implement `show_execution_progress()` function in CLI
- Use Rich progress bars (already imported)
- Show current task being executed
- Show execution time
- Handle cancellation gracefully

### 5. Result Display
**File:** `autotel/cli.py`

**Tasks:**
- Implement `display_execution_results()` function in CLI
- Format workflow output data
- Show execution statistics
- Display any errors or warnings
- Use Rich for beautiful output (already imported)

### 6. Integration with Execution Engine
**File:** `autotel/workflows/execution_engine.py`

**Tasks:**
- Ensure `AutoTelWorkflowManager` is properly integrated
- Add telemetry spans for CLI operations
- Handle execution errors gracefully
- Return structured execution results

## Testing Strategy

### Unit Tests
**File:** `tests/test_cli_run.py`

**Test Cases:**
- Run command with inline JSON
- Run command with JSON file
- Handle missing BPMN file
- Handle invalid JSON input
- Handle execution errors
- Progress indicator functionality

### Integration Tests
**File:** `tests/test_cli_run_integration.py`

**Test Cases:**
- End-to-end workflow execution
- Integration with execution engine
- Telemetry generation during execution
- Error handling scenarios

## Dependencies

### External Libraries
- `typer` - CLI command definition
- `rich` - Progress bars and output formatting
- `json` - JSON parsing
- `pathlib` - File path handling

### Internal Dependencies
- `autotel/workflows/execution_engine.py` - Workflow execution
- `autotel/workflows/dspy_bpmn_parser.py` - XML parsing
- `autotel/core/telemetry.py` - Telemetry generation

## Success Criteria

### Functional
- [ ] `autotel run workflow.bpmn` executes workflow
- [ ] `autotel run workflow.bpmn --input '{"key": "value"}'` works
- [ ] `autotel run workflow.bpmn --input-file data.json` works
- [ ] Progress is shown during execution
- [ ] Results are displayed clearly
- [ ] Errors are handled gracefully

### Technical
- [ ] Input data parsing is robust
- [ ] File handling is secure
- [ ] Progress indicators work correctly
- [ ] Error messages are clear
- [ ] Telemetry is generated

### User Experience
- [ ] Command is intuitive to use
- [ ] Progress feedback is informative
- [ ] Results are easy to read
- [ ] Error messages are actionable

## Common Pitfalls

### Avoid
- Not validating BPMN file existence
- Not handling JSON parsing errors
- Not showing progress for long operations
- Poor error messages
- Not handling file permissions

### Best Practices
- Always validate input files exist
- Provide clear error messages
- Show progress for user feedback
- Handle all error cases gracefully
- Use consistent output formatting 