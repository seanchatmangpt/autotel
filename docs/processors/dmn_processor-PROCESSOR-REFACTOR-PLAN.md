# DMN Processor Refactor Plan

**Current Role:**
- Parses DMN 1.3 XML into SpiffWorkflow decision objects.
- Ignores non-DMN elements.
- Handles XML parsing, decision extraction, error handling, telemetry.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string or file path.
  - Returns `ProcessorResult` with decision dict or error.
- Use `ProcessorConfig` for parser/telemetry options.
- Integrate telemetry (span for parse, metrics for decisions found/errors).
- Use contract decorators for input validation (non-empty XML, valid file).
- Centralize error handling (XML syntax, missing file, parse errors).
- Expose metadata (capabilities: "dmn_parse", "decision_extraction").
- Add happy path tests for valid/invalid DMN, telemetry checks. 