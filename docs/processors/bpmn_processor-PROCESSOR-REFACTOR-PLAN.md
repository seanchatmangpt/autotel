# BPMN Processor Refactor Plan

**Current Role:**
- Parses BPMN 2.0 XML into SpiffWorkflow `WorkflowSpec` objects.
- Ignores custom DSLs.
- Handles XML parsing, process extraction, error handling.

**Refactor Plan:**
- Inherit from `BaseProcessor` (ABC/metaclass).
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string and process_id.
  - Returns `ProcessorResult` with `WorkflowSpec` or error.
- Use `ProcessorConfig` for configuration (e.g., parser options).
- Integrate telemetry via helper (span for parse, metrics for success/failure).
- Use contract decorators for pre/post conditions (e.g., valid XML, process_id).
- Centralize error handling (XML, validation, missing process).
- Expose metadata via metaclass/decorator (capabilities: "bpmn_parse", "workflow_spec").
- Support file-based and string-based parsing via config/overloads.
- Add happy path tests for valid/invalid BPMN, telemetry span/metric checks. 