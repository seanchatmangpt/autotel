# CLI/BPMN/Task Generation Pipeline: End-to-End Example

## Overview

This document demonstrates a complete end-to-end pipeline that follows the AutoTel SOP principles: **CLI as input validation + BPMN orchestration + Task generation**. The example shows how to generate a complete task management system from an ontology definition.

## Use Case: Scrum Task Generator

**Scenario**: Generate a complete task management system for a Scrum team based on a Sprint Backlog ontology.

**Input**: Sprint Backlog OWL ontology + Sprint data
**Output**: Generated tasks, assignments, and workflow definitions
**Process**: CLI → BPMN → Task Generation → Results

---

## 1. CLI Command (Input Validation + BPMN Trigger)

### Command Structure
```bash
autotel scrum generate-tasks \
  --ontology ontologies/scrum_sprint.owl \
  --sprint-data examples/sprint_2024_01.json \
  --output-dir generated/tasks \
  --team-size 5
```

### CLI Implementation
```python
@app.command()
@otel_command
def scrum():
    """Scrum domain operations"""
    pass

@scrum.command()
@otel_command
def generate_tasks(
    ontology: Path = typer.Argument(..., help="Sprint Backlog OWL ontology"),
    sprint_data: Path = typer.Option(None, "--sprint-data", help="Sprint data JSON file"),
    output_dir: Path = typer.Option(Path("generated/tasks"), "--output-dir", help="Output directory"),
    team_size: int = typer.Option(5, "--team-size", help="Team size for task distribution"),
    verbose: bool = typer.Option(False, "--verbose", help="Verbose output")
):
    """Generate Scrum tasks from ontology and sprint data"""
    
    # 1. INPUT VALIDATION ONLY
    if not ontology.exists():
        console.print(f"[red]❌ Ontology file not found: {ontology}[/red]")
        raise typer.Exit(1)
    
    if not ontology.suffix in ['.owl', '.rdf', '.ttl']:
        console.print(f"[red]❌ Invalid ontology file type: {ontology.suffix}[/red]")
        raise typer.Exit(1)
    
    if sprint_data and not sprint_data.exists():
        console.print(f"[red]❌ Sprint data file not found: {sprint_data}[/red]")
        raise typer.Exit(1)
    
    # Validate team size
    if team_size < 1 or team_size > 20:
        console.print(f"[red]❌ Invalid team size: {team_size} (must be 1-20)[/red]")
        raise typer.Exit(1)
    
    # 2. PREPARE CONTEXT FOR BPMN
    context = {
        "ontology_file": str(ontology),
        "sprint_data_file": str(sprint_data) if sprint_data else None,
        "output_directory": str(output_dir),
        "team_size": team_size,
        "command": "scrum_generate_tasks",
        "timestamp": datetime.now().isoformat()
    }
    
    # 3. TRIGGER BPMN WORKFLOW
    workflow_path = "bpmn/scrum_task_generation_workflow.bpmn"
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console
    ) as progress:
        task = progress.add_task("Generating Scrum tasks...", total=1)
        
        result = execute_bpmn_workflow(
            workflow_path=workflow_path,
            process_id="ScrumTaskGeneration",
            context=context
        )
        
        progress.advance(task)
    
    # 4. RETURN DETERMINISTIC OUTPUT
    if verbose:
        console.print(f"[green]✅ Task generation completed![/green]")
        console.print(f"📊 Generated {result.get('tasks_generated', 0)} tasks")
        console.print(f"📁 Output directory: {result.get('output_directory')}")
        console.print(f"⏱️  Generation time: {result.get('generation_time', 0):.2f}s")
    else:
        console.print(f"[green]✅ Generated {result.get('tasks_generated', 0)} tasks[/green]")
    
    return result
```

---

## 2. BPMN Workflow (Business Logic Orchestration)

### Workflow Definition: `bpmn/scrum_task_generation_workflow.bpmn`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  id="ScrumTaskGeneration"
                  targetNamespace="http://autotel.ai/scrum">

  <!-- DSPy Signatures for Task Generation -->
  <dspy:signatures>
    <dspy:signature name="parse_sprint_ontology" description="Parse Sprint Backlog ontology">
      <dspy:input name="ontology_file" description="OWL ontology file path"/>
      <dspy:output name="ontology_classes" description="Extracted ontology classes"/>
      <dspy:output name="ontology_properties" description="Extracted ontology properties"/>
      <dspy:output name="ontology_constraints" description="Extracted ontology constraints"/>
    </dspy:signature>
    
    <dspy:signature name="analyze_sprint_data" description="Analyze sprint data and requirements">
      <dspy:input name="sprint_data_file" description="Sprint data JSON file"/>
      <dspy:input name="ontology_classes" description="Available ontology classes"/>
      <dspy:output name="sprint_requirements" description="Extracted sprint requirements"/>
      <dspy:output name="story_points" description="Total story points"/>
      <dspy:output name="priority_items" description="High priority items"/>
    </dspy:signature>
    
    <dspy:signature name="generate_task_structure" description="Generate task structure from requirements">
      <dspy:input name="sprint_requirements" description="Sprint requirements"/>
      <dspy:input name="ontology_classes" description="Ontology classes"/>
      <dspy:input name="team_size" description="Team size"/>
      <dspy:output name="task_structure" description="Generated task structure"/>
      <dspy:output name="task_dependencies" description="Task dependencies"/>
      <dspy:output name="estimated_effort" description="Estimated effort per task"/>
    </dspy:signature>
    
    <dspy:signature name="assign_team_members" description="Assign team members to tasks">
      <dspy:input name="task_structure" description="Task structure"/>
      <dspy:input name="team_size" description="Team size"/>
      <dspy:input name="task_dependencies" description="Task dependencies"/>
      <dspy:output name="team_assignments" description="Team member assignments"/>
      <dspy:output name="workload_distribution" description="Workload distribution"/>
    </dspy:signature>
    
    <dspy:signature name="generate_task_artifacts" description="Generate task artifacts and files">
      <dspy:input name="task_structure" description="Task structure"/>
      <dspy:input name="team_assignments" description="Team assignments"/>
      <dspy:input name="output_directory" description="Output directory"/>
      <dspy:output name="generated_files" description="Generated file paths"/>
      <dspy:output name="tasks_generated" description="Number of tasks generated"/>
    </dspy:signature>
  </dspy:signatures>

  <bpmn:process id="ScrumTaskGeneration" isExecutable="true">
    
    <!-- Start Event -->
    <bpmn:startEvent id="StartEvent_1" name="Start Task Generation">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <!-- Parse Ontology Task -->
    <bpmn:serviceTask id="ServiceTask_ParseOntology" name="Parse Sprint Ontology">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="parse_sprint_ontology" result="ontology_analysis">
          <dspy:param name="ontology_file" value="ontology_file"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- Analyze Sprint Data Task -->
    <bpmn:serviceTask id="ServiceTask_AnalyzeSprint" name="Analyze Sprint Data">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="analyze_sprint_data" result="sprint_analysis">
          <dspy:param name="sprint_data_file" value="sprint_data_file"/>
          <dspy:param name="ontology_classes" value="ontology_analysis.ontology_classes"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- Generate Task Structure Task -->
    <bpmn:serviceTask id="ServiceTask_GenerateStructure" name="Generate Task Structure">
      <bpmn:incoming>Flow_3</bpmn:incoming>
      <bpmn:outgoing>Flow_4</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="generate_task_structure" result="task_planning">
          <dspy:param name="sprint_requirements" value="sprint_analysis.sprint_requirements"/>
          <dspy:param name="ontology_classes" value="ontology_analysis.ontology_classes"/>
          <dspy:param name="team_size" value="team_size"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- Assign Team Members Task -->
    <bpmn:serviceTask id="ServiceTask_AssignTeam" name="Assign Team Members">
      <bpmn:incoming>Flow_4</bpmn:incoming>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="assign_team_members" result="team_planning">
          <dspy:param name="task_structure" value="task_planning.task_structure"/>
          <dspy:param name="team_size" value="team_size"/>
          <dspy:param name="task_dependencies" value="task_planning.task_dependencies"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- Generate Artifacts Task -->
    <bpmn:serviceTask id="ServiceTask_GenerateArtifacts" name="Generate Task Artifacts">
      <bpmn:incoming>Flow_5</bpmn:incoming>
      <bpmn:outgoing>Flow_6</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="generate_task_artifacts" result="generation_result">
          <dspy:param name="task_structure" value="task_planning.task_structure"/>
          <dspy:param name="team_assignments" value="team_planning.team_assignments"/>
          <dspy:param name="output_directory" value="output_directory"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <!-- End Event -->
    <bpmn:endEvent id="EndEvent_1" name="Task Generation Complete">
      <bpmn:incoming>Flow_6</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_ParseOntology"/>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_ParseOntology" targetRef="ServiceTask_AnalyzeSprint"/>
    <bpmn:sequenceFlow id="Flow_3" sourceRef="ServiceTask_AnalyzeSprint" targetRef="ServiceTask_GenerateStructure"/>
    <bpmn:sequenceFlow id="Flow_4" sourceRef="ServiceTask_GenerateStructure" targetRef="ServiceTask_AssignTeam"/>
    <bpmn:sequenceFlow id="Flow_5" sourceRef="ServiceTask_AssignTeam" targetRef="ServiceTask_GenerateArtifacts"/>
    <bpmn:sequenceFlow id="Flow_6" sourceRef="ServiceTask_GenerateArtifacts" targetRef="EndEvent_1"/>
    
  </bpmn:process>
</bpmn:definitions>
```

---

## 3. Input Data

### Sprint Backlog Ontology: `ontologies/scrum_sprint.owl`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns="http://autotel.ai/scrum/sprint#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">

  <owl:Ontology rdf:about="http://autotel.ai/scrum/sprint"/>
  
  <!-- Sprint Backlog Item -->
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#SprintBacklogItem">
    <rdfs:label>Sprint Backlog Item</rdfs:label>
    <rdfs:comment>A user story or feature in the sprint backlog</rdfs:comment>
  </owl:Class>
  
  <!-- User Story -->
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#UserStory">
    <rdfs:subClassOf rdf:resource="http://autotel.ai/scrum/sprint#SprintBacklogItem"/>
    <rdfs:label>User Story</rdfs:label>
    <rdfs:comment>A user story with acceptance criteria</rdfs:comment>
  </owl:Class>
  
  <!-- Task -->
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#Task">
    <rdfs:label>Task</rdfs:label>
    <rdfs:comment>A specific task to implement a user story</rdfs:comment>
  </owl:Class>
  
  <!-- Task Types -->
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#DevelopmentTask">
    <rdfs:subClassOf rdf:resource="http://autotel.ai/scrum/sprint#Task"/>
    <rdfs:label>Development Task</rdfs:label>
  </owl:Class>
  
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#TestingTask">
    <rdfs:subClassOf rdf:resource="http://autotel.ai/scrum/sprint#Task"/>
    <rdfs:label>Testing Task</rdfs:label>
  </owl:Class>
  
  <owl:Class rdf:about="http://autotel.ai/scrum/sprint#DocumentationTask">
    <rdfs:subClassOf rdf:resource="http://autotel.ai/scrum/sprint#Task"/>
    <rdfs:label>Documentation Task</rdfs:label>
  </owl:Class>
  
  <!-- Properties -->
  <owl:ObjectProperty rdf:about="http://autotel.ai/scrum/sprint#hasTask">
    <rdfs:domain rdf:resource="http://autotel.ai/scrum/sprint#UserStory"/>
    <rdfs:range rdf:resource="http://autotel.ai/scrum/sprint#Task"/>
    <rdfs:label>has task</rdfs:label>
  </owl:ObjectProperty>
  
  <owl:DatatypeProperty rdf:about="http://autotel.ai/scrum/sprint#storyPoints">
    <rdfs:domain rdf:resource="http://autotel.ai/scrum/sprint#UserStory"/>
    <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
    <rdfs:label>story points</rdfs:label>
  </owl:DatatypeProperty>
  
  <owl:DatatypeProperty rdf:about="http://autotel.ai/scrum/sprint#priority">
    <rdfs:domain rdf:resource="http://autotel.ai/scrum/sprint#SprintBacklogItem"/>
    <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    <rdfs:label>priority</rdfs:label>
  </owl:DatatypeProperty>
  
  <owl:DatatypeProperty rdf:about="http://autotel.ai/scrum/sprint#estimatedHours">
    <rdfs:domain rdf:resource="http://autotel.ai/scrum/sprint#Task"/>
    <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
    <rdfs:label>estimated hours</rdfs:label>
  </owl:DatatypeProperty>
  
</rdf:RDF>
```

### Sprint Data: `examples/sprint_2024_01.json`

```json
{
  "sprint_name": "Sprint 2024-01",
  "sprint_duration": 14,
  "team_members": [
    {"name": "Alice", "role": "Developer", "capacity": 40},
    {"name": "Bob", "role": "Developer", "capacity": 40},
    {"name": "Carol", "role": "QA Engineer", "capacity": 40},
    {"name": "David", "role": "Developer", "capacity": 40},
    {"name": "Eve", "role": "Technical Writer", "capacity": 20}
  ],
  "user_stories": [
    {
      "id": "US-001",
      "title": "User Authentication System",
      "description": "Implement secure user authentication with OAuth2",
      "story_points": 8,
      "priority": "high",
      "acceptance_criteria": [
        "Users can register with email and password",
        "Users can login with OAuth2 providers",
        "Password reset functionality works",
        "Session management is secure"
      ]
    },
    {
      "id": "US-002", 
      "title": "Dashboard Analytics",
      "description": "Create analytics dashboard with charts and metrics",
      "story_points": 5,
      "priority": "medium",
      "acceptance_criteria": [
        "Dashboard displays key metrics",
        "Charts are interactive and responsive",
        "Data refreshes automatically",
        "Export functionality works"
      ]
    },
    {
      "id": "US-003",
      "title": "API Documentation",
      "description": "Generate comprehensive API documentation",
      "story_points": 3,
      "priority": "low",
      "acceptance_criteria": [
        "All endpoints are documented",
        "Examples are provided",
        "Documentation is searchable",
        "Swagger UI integration works"
      ]
    }
  ]
}
```

---

## 4. Generated Output

### Task Structure: `generated/tasks/task_structure.json`

```json
{
  "sprint_name": "Sprint 2024-01",
  "total_story_points": 16,
  "tasks_generated": 12,
  "generation_time": 2.34,
  "tasks": [
    {
      "id": "TASK-001",
      "title": "Design Authentication Database Schema",
      "type": "DevelopmentTask",
      "story_id": "US-001",
      "assigned_to": "Alice",
      "estimated_hours": 4,
      "priority": "high",
      "dependencies": [],
      "acceptance_criteria": "Database schema supports OAuth2 user data"
    },
    {
      "id": "TASK-002",
      "title": "Implement User Registration API",
      "type": "DevelopmentTask", 
      "story_id": "US-001",
      "assigned_to": "Bob",
      "estimated_hours": 6,
      "dependencies": ["TASK-001"],
      "acceptance_criteria": "POST /api/auth/register endpoint works"
    },
    {
      "id": "TASK-003",
      "title": "Implement OAuth2 Integration",
      "type": "DevelopmentTask",
      "story_id": "US-001", 
      "assigned_to": "Alice",
      "estimated_hours": 8,
      "dependencies": ["TASK-002"],
      "acceptance_criteria": "OAuth2 login with Google and GitHub works"
    },
    {
      "id": "TASK-004",
      "title": "Implement Password Reset",
      "type": "DevelopmentTask",
      "story_id": "US-001",
      "assigned_to": "David",
      "estimated_hours": 4,
      "dependencies": ["TASK-002"],
      "acceptance_criteria": "Password reset email and token validation works"
    },
    {
      "id": "TASK-005",
      "title": "Test Authentication Flow",
      "type": "TestingTask",
      "story_id": "US-001",
      "assigned_to": "Carol",
      "estimated_hours": 6,
      "dependencies": ["TASK-003", "TASK-004"],
      "acceptance_criteria": "All authentication scenarios pass tests"
    },
    {
      "id": "TASK-006",
      "title": "Design Analytics Dashboard Layout",
      "type": "DevelopmentTask",
      "story_id": "US-002",
      "assigned_to": "Bob",
      "estimated_hours": 3,
      "dependencies": [],
      "acceptance_criteria": "Dashboard layout is responsive and accessible"
    },
    {
      "id": "TASK-007",
      "title": "Implement Chart Components",
      "type": "DevelopmentTask",
      "story_id": "US-002",
      "assigned_to": "David",
      "estimated_hours": 8,
      "dependencies": ["TASK-006"],
      "acceptance_criteria": "Charts display real data and are interactive"
    },
    {
      "id": "TASK-008",
      "title": "Implement Data Refresh Logic",
      "type": "DevelopmentTask",
      "story_id": "US-002",
      "assigned_to": "Alice",
      "estimated_hours": 4,
      "dependencies": ["TASK-007"],
      "acceptance_criteria": "Data refreshes every 5 minutes automatically"
    },
    {
      "id": "TASK-009",
      "title": "Test Dashboard Functionality",
      "type": "TestingTask",
      "story_id": "US-002",
      "assigned_to": "Carol",
      "estimated_hours": 4,
      "dependencies": ["TASK-008"],
      "acceptance_criteria": "All dashboard features work correctly"
    },
    {
      "id": "TASK-010",
      "title": "Generate API Documentation",
      "type": "DocumentationTask",
      "story_id": "US-003",
      "assigned_to": "Eve",
      "estimated_hours": 6,
      "dependencies": [],
      "acceptance_criteria": "All endpoints documented with examples"
    },
    {
      "id": "TASK-011",
      "title": "Integrate Swagger UI",
      "type": "DevelopmentTask",
      "story_id": "US-003",
      "assigned_to": "Bob",
      "estimated_hours": 2,
      "dependencies": ["TASK-010"],
      "acceptance_criteria": "Swagger UI accessible at /api/docs"
    },
    {
      "id": "TASK-012",
      "title": "Review and Finalize Documentation",
      "type": "DocumentationTask",
      "story_id": "US-003",
      "assigned_to": "Eve",
      "estimated_hours": 2,
      "dependencies": ["TASK-011"],
      "acceptance_criteria": "Documentation is complete and accurate"
    }
  ],
  "team_assignments": {
    "Alice": ["TASK-001", "TASK-003", "TASK-008"],
    "Bob": ["TASK-002", "TASK-006", "TASK-011"],
    "Carol": ["TASK-005", "TASK-009"],
    "David": ["TASK-004", "TASK-007"],
    "Eve": ["TASK-010", "TASK-012"]
  },
  "workload_distribution": {
    "Alice": 16,
    "Bob": 11,
    "Carol": 10,
    "David": 12,
    "Eve": 8
  }
}
```

### Generated Files

#### 1. Jira Import: `generated/tasks/jira_import.csv`
```csv
Summary,Description,Issue Type,Story Points,Priority,Assignee,Epic Link
Design Authentication Database Schema,Database schema supports OAuth2 user data,Task,4,High,Alice,US-001
Implement User Registration API,POST /api/auth/register endpoint works,Task,6,High,Bob,US-001
Implement OAuth2 Integration,OAuth2 login with Google and GitHub works,Task,8,High,Alice,US-001
Implement Password Reset,Password reset email and token validation works,Task,4,High,David,US-001
Test Authentication Flow,All authentication scenarios pass tests,Task,6,High,Carol,US-001
Design Analytics Dashboard Layout,Dashboard layout is responsive and accessible,Task,3,Medium,Bob,US-002
Implement Chart Components,Charts display real data and are interactive,Task,8,Medium,David,US-002
Implement Data Refresh Logic,Data refreshes every 5 minutes automatically,Task,4,Medium,Alice,US-002
Test Dashboard Functionality,All dashboard features work correctly,Task,4,Medium,Carol,US-002
Generate API Documentation,All endpoints documented with examples,Task,6,Low,Eve,US-003
Integrate Swagger UI,Swagger UI accessible at /api/docs,Task,2,Low,Bob,US-003
Review and Finalize Documentation,Documentation is complete and accurate,Task,2,Low,Eve,US-003
```

#### 2. GitHub Issues: `generated/tasks/github_issues.md`
```markdown
# Sprint 2024-01 Tasks

## User Story: US-001 - User Authentication System

### Task: TASK-001 - Design Authentication Database Schema
- **Assignee**: @alice
- **Story Points**: 4
- **Priority**: High
- **Acceptance Criteria**: Database schema supports OAuth2 user data

### Task: TASK-002 - Implement User Registration API  
- **Assignee**: @bob
- **Story Points**: 6
- **Priority**: High
- **Dependencies**: TASK-001
- **Acceptance Criteria**: POST /api/auth/register endpoint works

[... additional tasks ...]
```

#### 3. Sprint Burndown: `generated/tasks/sprint_burndown.json`
```json
{
  "sprint_name": "Sprint 2024-01",
  "total_story_points": 16,
  "daily_burndown": [
    {"day": 1, "remaining_points": 16, "completed_points": 0},
    {"day": 2, "remaining_points": 14, "completed_points": 2},
    {"day": 3, "remaining_points": 12, "completed_points": 4},
    {"day": 4, "remaining_points": 10, "completed_points": 6},
    {"day": 5, "remaining_points": 8, "completed_points": 8},
    {"day": 6, "remaining_points": 6, "completed_points": 10},
    {"day": 7, "remaining_points": 4, "completed_points": 12},
    {"day": 8, "remaining_points": 2, "completed_points": 14},
    {"day": 9, "remaining_points": 0, "completed_points": 16}
  ]
}
```

---

## 5. Telemetry Output

### OpenTelemetry Spans
```json
{
  "spans": [
    {
      "name": "scrum_generate_tasks",
      "attributes": {
        "command": "scrum_generate_tasks",
        "ontology_file": "ontologies/scrum_sprint.owl",
        "team_size": 5,
        "input_validation": "success"
      }
    },
    {
      "name": "parse_sprint_ontology",
      "attributes": {
        "ontology_classes": 6,
        "ontology_properties": 4,
        "processing_time_ms": 45
      }
    },
    {
      "name": "analyze_sprint_data", 
      "attributes": {
        "user_stories": 3,
        "total_story_points": 16,
        "processing_time_ms": 23
      }
    },
    {
      "name": "generate_task_structure",
      "attributes": {
        "tasks_generated": 12,
        "task_types": ["DevelopmentTask", "TestingTask", "DocumentationTask"],
        "processing_time_ms": 156
      }
    },
    {
      "name": "assign_team_members",
      "attributes": {
        "team_size": 5,
        "workload_balanced": true,
        "processing_time_ms": 89
      }
    },
    {
      "name": "generate_task_artifacts",
      "attributes": {
        "files_generated": 4,
        "output_directory": "generated/tasks",
        "processing_time_ms": 234
      }
    }
  ]
}
```

---

## 6. Key Benefits Demonstrated

### ✅ **SOP Compliance**
- **CLI as Input Validation**: Only validates inputs, triggers BPMN
- **BPMN as Business Logic**: All generation logic in XML workflow
- **Deterministic Output**: Same inputs → Same BPMN → Same results
- **Telemetry-First**: Every step emits OTEL spans

### ✅ **Ontological Mapping**
- **CLI Verb → BPMN Process**: `scrum generate-tasks` → `ScrumTaskGeneration`
- **DSPy Signatures**: All AI logic defined in XML
- **No Procedural Code**: Pure declarative workflow

### ✅ **Dogfood-Driven**
- **Self-Referential**: Could generate AutoTel tasks from AutoTel ontology
- **Auto-Versioning**: Output includes generation metadata
- **Reproducible**: Complete trace from input to output

### ✅ **Enterprise Patterns**
- **Task Dependencies**: Proper dependency management
- **Team Assignment**: Workload balancing
- **Multiple Formats**: Jira, GitHub, JSON outputs
- **Sprint Planning**: Burndown charts and metrics

---

## 7. Usage Example

```bash
# Generate tasks for a 5-person team
autotel scrum generate-tasks \
  --ontology ontologies/scrum_sprint.owl \
  --sprint-data examples/sprint_2024_01.json \
  --output-dir generated/tasks \
  --team-size 5 \
  --verbose

# Output:
# ✅ Generated 12 tasks
# 📊 Generated 12 tasks
# 📁 Output directory: generated/tasks
# ⏱️  Generation time: 2.34s
```

This example demonstrates a complete end-to-end pipeline that follows AutoTel SOP principles: **CLI validation → BPMN orchestration → Task generation → Deterministic output**. 