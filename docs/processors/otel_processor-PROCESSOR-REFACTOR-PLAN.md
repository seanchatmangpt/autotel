# OTEL Processor Refactor Plan

**Current Role:**
- Parses OpenTelemetry data (spans, metrics, logs, traces) from JSON.
- Analyzes, converts, and validates telemetry data.
- Integrates telemetry for all processing steps.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts JSON dict or file path.
  - Returns `ProcessorResult` with parsed/analyzed telemetry or error.
- Use `ProcessorConfig` for telemetry, analysis, conversion options.
- Integrate telemetry (span for parse/analyze/convert, metrics for counts/errors).
- Use contract decorators for input validation (JSON schema, required fields).
- Centralize error handling (JSON, telemetry, analysis errors).
- Expose metadata (capabilities: "otel_parse", "telemetry_analysis", "telemetry_conversion").
- Add happy path tests for span/metric/log/trace parsing, analysis, telemetry. 