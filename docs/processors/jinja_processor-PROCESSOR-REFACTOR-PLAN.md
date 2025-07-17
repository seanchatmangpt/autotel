# Jinja Processor Refactor Plan

**Current Role:**
- Parses Jinja2 templates from XML, renders with variables.
- Validates templates, variables, supports custom filters.
- Handles errors, returns processing/validation results.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string and variables dict.
  - Returns `ProcessorResult` with rendered/validated templates or error.
- Use `ProcessorConfig` for environment, filters, telemetry.
- Integrate telemetry (span for parse/render, metrics for templates/validation).
- Use contract decorators for input validation (XML, variable types).
- Centralize error handling (XML, Jinja errors, validation).
- Expose metadata (capabilities: "jinja_parse", "template_render", "template_validation").
- Add happy path tests for template parsing/rendering, telemetry. 