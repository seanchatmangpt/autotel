# Product Requirements Document (PRD)

## Product: AutoTel - XML-Driven AI Workflow Orchestration

---

### 1. Problem Statement
Modern organizations need to automate complex business processes that combine human tasks, AI-driven decisions, and business rules. However, most workflow automation tools require users to write or modify code, which limits accessibility and increases maintenance overhead. There is a need for a system where all workflow, AI, and decision logic can be defined declaratively in XML, with a simple CLI for execution—eliminating the need for users to write or modify Python or any other code.

---

### 2. Goals & Objectives
- **No-Code User Experience:** Users define all workflows, AI signatures, and decision rules in XML only.
- **CLI-Only Interaction:** Users interact with the system exclusively via a command-line interface.
- **Dynamic AI & Rules:** The system dynamically interprets and executes AI signatures and DMN decision tables defined in XML.
- **Separation of Concerns:** All engine logic is generic and not user-editable; users only provide XML and data files.
- **Enterprise-Grade Standards:** Robust error handling, validation, and clear output using modern CLI UX (Rich, Typer).

---

### 3. 80/20 Focus & Best Practices

#### 80/20 Priorities
- **CLI Simplicity:** Focus on making the CLI intuitive, with clear commands, help, and error messages. 80% of user friction comes from 20% of confusing CLI interactions.
- **XML Schema & Examples:** Provide robust, well-documented XML templates and examples. Most user success will come from a few clear, copy-pasteable examples.
- **Declarative, Not Programmatic:** Ensure all logic (AI, rules, routing) is in XML, not code. This is the core differentiator and unlocks most of the value for non-programmer users.
- **Validation & Feedback:** Invest in strong XML validation and actionable error messages. Most user frustration is caused by unclear errors or silent failures.
- **Quickstart & FAQ:** A single, well-crafted end-to-end example and a troubleshooting FAQ will solve most onboarding issues.
- **Pluggable Expressions:** Support a small, safe set of built-in expressions (e.g., `len()`, basic math) in XML for data transformation, covering 80% of real-world needs.

#### Best Practices
- **CLI:**
  - Use Typer for modern CLI UX.
  - Use Rich for beautiful, readable output and error messages.
  - Group commands logically (e.g., `run`, `validate`, `examples`).
  - Always provide `--help` and usage examples.
- **XML Workflows:**
  - Use clear, versioned XML schemas (XSD) for BPMN, DMN, and DSPy signatures.
  - Validate all XML before execution; fail fast and informatively.
  - Encourage modular XML: separate files for workflows, signatures, and decision tables.
  - Document variable names and data flow in each example.
- **Engine:**
  - Keep the engine generic and stateless; all state is in the workflow context.
  - Parse and register all logic (signatures, rules) at runtime from XML.
  - Never allow user-supplied code execution—only declarative XML and safe expressions.
- **Documentation:**
  - Provide a “Hello World” workflow and a real-world business process example.
  - Include a troubleshooting section for common XML and CLI errors.
  - Use diagrams (e.g., Mermaid) to illustrate workflow and data flow.

---

### 4. User Stories
- **As a business analyst**, I want to define a workflow in BPMN XML, so that I can automate a business process without writing code.
- **As a data scientist**, I want to define AI signature inputs/outputs in XML, so that I can integrate AI tasks declaratively.
- **As a process owner**, I want to define decision rules in DMN XML, so that I can change business logic without developer support.
- **As a user**, I want to run workflows and see results using a simple CLI, so that I can operate the system without programming knowledge.
- **As a compliance officer**, I want all logic to be auditable in XML, so that I can review and approve process changes easily.

---

### 5. Requirements
#### Functional
- **XML-Only Logic:**
  - All workflow steps, AI signatures, and decision rules are defined in XML (BPMN, DMN, custom signature XML).
  - No user-facing Python or other programming required or allowed.
- **CLI-Only Interface:**
  - Users interact with the system exclusively via a CLI (e.g., `autotel run --workflow my.bpmn --input data.json`).
  - CLI provides commands for running workflows, validating XML, and displaying help/examples.
- **Dynamic Signature & Rule Parsing:**
  - The engine parses and registers AI signatures and DMN tables from XML at runtime.
  - All data flow and variable mapping is defined in XML.
- **Declarative Data Transformation:**
  - Any required data transformation (e.g., `keyword_count = len(keywords)`) must be expressible in XML, either via built-in expressions or declarative constructs.
- **Rich Output & Error Handling:**
  - CLI uses Rich for beautiful, clear output and error messages.
  - All errors (validation, execution) are reported with actionable feedback.
- **Validation:**
  - CLI validates XML files before execution and provides clear error messages for invalid files.
- **Documentation:**
  - System includes clear documentation and examples for defining workflows, AI signatures, and DMN tables in XML.

#### Non-Functional
- **Security:** No code execution from user input; only declarative XML is allowed.
- **Performance:** Workflows should execute efficiently for typical business process sizes.
- **Extensibility:** Engine can be updated to support new XML constructs without breaking user workflows.
- **Auditability:** All process logic is visible and reviewable in XML files.

---

### 6. Acceptance Criteria (80/20 Optimized)
- [ ] **CLI is intuitive and provides clear, actionable feedback.**
- [ ] **Users can define and run a complete workflow using only XML and the CLI.**
- [ ] **XML validation catches 80%+ of common user errors before execution.**
- [ ] **Documentation includes at least one end-to-end example and a troubleshooting FAQ.**
- [ ] **Engine supports basic expressions in XML for 80% of transformation needs.**
- [ ] **All user-facing output is clear, beautiful, and actionable.**

---

### 7. Quickstart Example

#### 1. Define a Workflow in XML (BPMN)
```xml
<!-- my_workflow.bpmn -->
<bpmn:definitions ...>
  <bpmn:process id="my_process" ...>
    <!-- Define tasks, service tasks, and business rule tasks here -->
  </bpmn:process>
</bpmn:definitions>
```

#### 2. Define AI Signatures in XML
```xml
<spiffext:signature name="SentimentAnalyzer">
  <spiffext:input name="text" type="str" description="Text to analyze"/>
  <spiffext:output name="sentiment" type="str" description="Sentiment result"/>
</spiffext:signature>
```

#### 3. Define DMN Decision Table in XML
```xml
<spiffext:decision name="sentiment_routing" id="sentiment_routing">
  <spiffext:decisionTable>
    <spiffext:input id="sentiment" label="Sentiment" typeRef="string"/>
    <spiffext:output id="route" label="Route" typeRef="string"/>
    <!-- ... rules ... -->
  </spiffext:decisionTable>
</spiffext:decision>
```

#### 4. Run the Workflow from the CLI
```sh
autotel run --workflow my_workflow.bpmn --input data.json
```

---

### 8. FAQ & Troubleshooting

**Q: My workflow fails to run. What should I check first?**
- Validate your XML using `autotel validate --workflow my_workflow.bpmn`.
- Check for typos in variable names and ensure all required fields are present.

**Q: How do I perform data transformations (e.g., count keywords) in XML?**
- Use built-in expressions in XML, e.g., `<spiffext:input id="keyword_count" expr="len(keywords)"/>` if supported.
- If not supported, request this feature or use a supported workaround.

**Q: Can I use Python or scripts in my workflow?**
- No. All logic must be in XML. The engine does not execute user code for security and maintainability.

**Q: How do I get help or see examples?**
- Run `autotel --help` or `autotel examples` for usage and templates.

---

### 9. Out of Scope
- No web UI or GUI.
- No direct Python API or SDK for users.
- No support for user-written scripts or plugins.

---

### 10. Open Questions
- What expression language (if any) should be supported in XML for data transformation?
- Should the engine provide a set of built-in functions (e.g., `len()`, `sum()`) for use in XML?
- How should versioning and migration of XML schemas be handled? 