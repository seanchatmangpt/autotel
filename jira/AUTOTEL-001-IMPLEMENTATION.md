# AUTOTEL-001 Implementation Guide: Core CLI Interface

## Context

The AutoTel CLI is the primary user interface. End users must be able to interact with AutoTel using only command-line commands without any Python knowledge.

## Working

### Current CLI Structure
- `autotel/cli.py` - Main CLI entry point with 7 commands
- `pyproject.toml` - CLI command registration (`autotel = "autotel.cli:app"`)
- Typer framework with Rich integration
- OpenTelemetry integration with `@otel_command` decorator
- SHACL and OWL validation integration

### Existing Components
- `autotel/workflows/dspy_bpmn_parser.py` - XML parser with SHACL/OWL integration
- `autotel/workflows/execution_engine.py` - Execution engine
- `autotel/core/telemetry.py` - Telemetry manager
- `autotel/utils/owl_integration.py` - OWL ontology processing

## Not Working

### Missing CLI Commands
- No `autotel run` command (AUTOTEL-002)
- `autotel validate` command exists but needs enhancement
- `autotel telemetry` command exists but needs enhancement
- No `autotel list` command
- Limited input data handling
- Progress indicators exist but need enhancement
- Basic error handling exists

### Missing Integration
- CLI doesn't connect to execution engine (AUTOTEL-002)
- Basic telemetry integration exists in CLI
- Basic validation integration exists in CLI
- Help text exists for commands

## Implementation Steps

### 1. Enhance CLI Structure
**File:** `autotel/cli.py`

**Tasks:**
- Add missing `run` command (AUTOTEL-002)
- Enhance existing commands with better error handling
- Improve progress indicators using Rich
- Add input data parsing (JSON and file)
- Add `list` command for workflows

### 2. Verify Package Configuration
**File:** `pyproject.toml`

**Tasks:**
- Verify CLI entry point is correctly configured (`autotel = "autotel.cli:app"`)
- Verify Rich and Typer dependencies are present
- Verify CLI command name is "autotel"

### 3. Enhance CLI Commands
**File:** `autotel/cli.py`

**Tasks:**
- Add missing `run` command (AUTOTEL-002)
- Enhance existing `validate` command
- Enhance existing `telemetry` command
- Add `list` command for workflows
- Improve error handling in all commands

### 4. Enhance CLI Utilities
**File:** `autotel/cli.py`

**Tasks:**
- Enhance JSON input parsing in existing commands
- Enhance file input handling
- Improve progress bar utilities using Rich
- Enhance error formatting utilities
- Improve telemetry display utilities

### 5. Integration Points
**Files to Connect:**
- `autotel/workflows/execution_engine.py` - For workflow execution (AUTOTEL-002)
- `autotel/workflows/dspy_bpmn_parser.py` - For XML parsing (already connected)
- `autotel/core/telemetry.py` - For telemetry display (already connected)
- `autotel/schemas/validation.py` - For XML validation (already connected)

## Testing Strategy

### Unit Tests
**File:** `tests/test_cli.py`

**Test Cases:**
- CLI command availability
- Help text display
- Input data parsing
- Error handling
- Progress indicators

### Integration Tests
**File:** `tests/test_cli_integration.py`

**Test Cases:**
- End-to-end workflow execution
- XML validation workflow
- Telemetry display workflow
- Error scenarios

## Dependencies

### External Libraries
- `typer` - CLI framework
- `rich` - Terminal output
- `click` - File handling

### Internal Dependencies
- `autotel/workflows/execution_engine.py`
- `autotel/workflows/dspy_bpmn_parser.py`
- `autotel/core/telemetry.py`

## Success Criteria

### Functional
- [x] `autotel --help` shows all commands
- [ ] `autotel run workflow.bpmn` executes workflows (AUTOTEL-002)
- [x] `autotel validate workflow.bpmn` validates XML
- [x] `autotel telemetry workflow.bpmn` shows telemetry
- [ ] `autotel list` lists available workflows

### Technical
- [ ] CLI commands are properly structured
- [ ] Error handling is comprehensive
- [ ] Progress indicators work
- [ ] Input data parsing is robust
- [ ] Integration tests pass

### User Experience
- [ ] Help text is clear and comprehensive
- [ ] Error messages are actionable
- [ ] Progress indicators are informative
- [ ] Commands are intuitive to use

## Common Pitfalls

### Avoid
- Hardcoding file paths in CLI
- Not handling file not found errors
- Missing input validation
- Poor error messages
- No progress feedback

### Best Practices
- Use descriptive help text
- Handle all error cases
- Show progress for long operations
- Validate input data
- Provide clear error messages 