# Standard Operating Procedure (SOP) - Autotel CLI
**System:** Autotel Command Line Interface  
**Purpose:** Define operational rules, naming conventions, architecture constraints, and execution pipelines for the Autotel Command Line Interface.  
**Version:** v8.0 (Operation Dogfood)  
**Effective Date:** 2025-07-16  
**Owner:** AutoTel Core Intelligence Team  

## 🎯 Purpose

The Autotel CLI is the execution entry point for all ontological workflows. It does not run code — it instantiates, validates, executes, renders, simulates, or monitors ontological constructs that define the system. This SOP defines:

- Command structure
- Ontological mapping  
- Output behaviors
- Execution context
- Testing and simulation flows

## 🧱 1. Command Structure

### Format
```bash
autotel <domain> <tool> <verb> [--args]
```

### Domains (Subject Areas)
| Domain | Description |
|--------|-------------|
| `ontology` | OWL + SHACL + SPARQL operations |
| `bpmn` | Workflow orchestration and simulation |
| `dmn` | Decision logic |
| `jinja` | Rendering and template emission |
| `dspy` | LLM-augmented inference (fallback only) |
| `otel` | Observability, metrics, telemetry |
| `mermaid` | Diagram orchestration and rendering |
| `nuxt` | Frontend code orchestration |

## 🧠 2. Ontological Mapping

All CLI verbs are driven by an underlying BPMN process.

| Verb | Backed by Ontology | Example BPMN Task |
|------|-------------------|-------------------|
| `init` | Yes (Jinja template) | InitializeSystem |
| `generate` | Yes (SHACL + Jinja) | EmitArtifacts |
| `validate` | Yes (SHACL/OWL/DMN) | ValidateModel |
| `simulate` | Yes (Monte Carlo BPMN) | RunSimulation |
| `run` | Yes (BPMN + OTEL) | ExecuteWorkflow |
| `monitor` | Yes (OTEL + SPARQL) | MonitorFlow |
| `query` | Yes (SPARQL only) | QueryOntology |

## 🛠 3. Output and Behavior

| Behavior | Description |
|----------|-------------|
| **Deterministic** | Output must be 100% reproducible from ontologies |
| **Telemetry-first** | Every command emits OTEL spans with full tracing |
| **Human-optional** | All outputs are for systems, not for users |
| **Declarative** | No imperative CLI code; each verb maps to ontological intent |

## 🔄 4. Execution Flow

Every command follows this pipeline:

```
[OWL/SHACL/SPARQL] → [BPMN Sequence] → [Execution/Rendering] → [Telemetry]
```

### Example: `autotel ontology validate --input ontology.owl`

1. Load OWL file
2. Apply SHACL constraints  
3. Emit validation report (JSON, SPARQL query)
4. Telemetry logged to OTEL processor

## 🧪 5. Testing / Dogfood Loop

Every CLI command should be used as part of the system that builds Autotel itself.

**Dogfood-Driven Development Loop:**
```
Ontology updated → CLI regenerates code → CLI validates ontology → Telemetry confirms state
```

- No manual steps
- Output auto-versioned via ontology hash

## 📚 6. Vocabulary + Naming Rules

All CLI commands map directly to ontology class or BPMN task names.

No procedural verbs unless derived from ontological process.

Commands are not namespaced by user preference — only by domain logic.

### Examples:
```bash
autotel dmn generate --model decision-table.dmn.xml
autotel jinja render --template ui.xml --context sprint-ontology.ttl  
autotel nuxt emit --page product-backlog --ontology scrum.owl
```

## 🚨 7. Constraints

- No Docker, no runtime containers
- XML-only configuration (DSPy, Jinja, OWL, SHACL)
- CLI may not emit shell commands — only render instructions
- Everything typed, everything traced

## 🧭 8. CLI Extension Protocol

New verbs or domains must:

- Map to a BPMN task
- Be ontologically defined
- Pass OTEL telemetry coverage threshold
- Use Typer + Pydantic CLI standards
- Be testable via SHACL + FactoryBoy

## 🔒 9. Security & Governance

- All CLI executions are stateless, immutable
- Sensitive transformations (e.g., ontology mutations) require SHA256 signed provenance
- Governance rules defined in autotel governance ontology must be validated per command

## 📋 10. Command Reference

### Ontology Commands
```bash
autotel ontology load --input file.owl
autotel ontology validate --shape shapes.shacl
autotel ontology query --sparql query.rq
autotel ontology export --format json
```

### BPMN Commands  
```bash
autotel bpmn simulate --process workflow.bpmn
autotel bpmn validate --input process.bpmn
autotel bpmn generate --from ontology.owl
autotel bpmn run --process-id Process_1
```

### Jinja Commands
```bash
autotel jinja render --template template.xml --context data.json
autotel jinja validate --template template.xml
autotel jinja generate --from ontology.owl
```

### DSPy Commands (Fallback Only)
```bash
autotel dspy call --signature signature.xml --input data.json
autotel dspy generate --signature signature.xml
autotel dspy validate --signature signature.xml
```

### Telemetry Commands
```bash
autotel otel trace --span-id abc123
autotel otel metrics --service autotel-cli
autotel otel export --format rdf
```

## 🔄 11. Error Handling

All errors must:
- Emit structured JSON with error codes
- Include OTEL span context
- Reference SHACL validation failures
- Provide actionable next steps

## 📊 12. Performance Requirements

- Command execution: < 100ms for cached operations
- Ontology loading: < 50ms for standard ontologies
- Telemetry emission: < 10ms overhead
- Memory usage: < 100MB for standard operations

## 🧪 13. Testing Requirements

Every command must have:
- Unit tests using FactoryBoy
- Integration tests with real ontologies
- Telemetry validation tests
- SHACL constraint tests

## 📝 14. Documentation Standards

All commands must:
- Include ontology mapping documentation
- Provide SPARQL query examples
- Show BPMN process diagrams
- Include telemetry span examples

---

**This SOP ensures that the Autotel CLI remains a pure ontological interface, never devolving into imperative programming or human-centric workflows.** 