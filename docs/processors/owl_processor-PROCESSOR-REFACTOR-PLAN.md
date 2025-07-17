# OWL Processor Refactor Plan

**Current Role:**
- Parses OWL/RDF XML into ontology definitions.
- Extracts classes, properties, individuals, axioms.
- Integrates telemetry, supports fallback/no-op telemetry.

**Refactor Plan:**
- Inherit from `BaseProcessor`.
- Implement `_process_impl(self, data: Any) -> ProcessorResult`:
  - Accepts XML string.
  - Returns `ProcessorResult` with ontology definition or error.
- Use `ProcessorConfig` for namespace, telemetry, fallback options.
- Integrate telemetry (span for parse, metrics for ontology components).
- Use contract decorators for XML/ontology validation.
- Centralize error handling (XML, ontology extraction errors).
- Expose metadata (capabilities: "owl_parse", "ontology_extraction").
- Add happy path tests for ontology parsing, telemetry. 