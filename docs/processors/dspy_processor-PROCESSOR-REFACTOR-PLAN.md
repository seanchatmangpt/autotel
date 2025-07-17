# DSPy Processor Refactor Plan

**Current Role:**
- Parses DSPy XML into signature/module/model definitions.
- Extracts validation rules, semantic context, SHACL constraints.
- Integrates telemetry for all parsing steps.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string.
  - Returns `ProcessorResult` with parsed definitions or error.
- Use `ProcessorConfig` for namespace, telemetry, validation options.
- Integrate telemetry (span for each parse step, metrics for counts/errors).
- Use contract decorators for XML validation, schema compliance.
- Centralize error handling (XML, missing elements, parse errors).
- Expose metadata (capabilities: "dspy_parse", "signature_extraction", "module_extraction", "validation_rule_extraction").
- Add happy path tests for signature/module/model parsing, telemetry checks. 