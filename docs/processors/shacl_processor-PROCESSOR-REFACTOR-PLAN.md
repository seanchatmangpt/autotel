# SHACL Processor Refactor Plan

**Current Role:**
- Parses SHACL XML into constraint graphs and validation rules.
- Supports pluggable validators, advanced constraint types, telemetry.
- Handles file/string input, caching, performance metrics.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string or file path.
  - Returns `ProcessorResult` with constraint/validation data or error.
- Use `ProcessorConfig` for validator config, cache options, telemetry.
- Integrate telemetry (span for parse/validate, metrics for rules, cache, performance).
- Use contract decorators for input validation, rule compliance.
- Centralize error handling (XML, SHACL, validation errors).
- Expose metadata (capabilities: "shacl_parse", "constraint_validation", "pluggable_validation").
- Add happy path tests for constraint extraction, validation, telemetry. 