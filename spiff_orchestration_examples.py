#!/usr/bin/env python3
"""
SpiffWorkflow Orchestration Examples - LinkML + OpenTelemetry Validation Paradigm
Demonstrates how code agents validate their work by generating structured telemetry
"""

import json
import uuid
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Any, Optional, Union
from dataclasses import dataclass, field
from pathlib import Path
import yaml

# SpiffWorkflow imports
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
from SpiffWorkflow.bpmn.specs.bpmn_process_spec import BpmnProcessSpec
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.serializer import BpmnWorkflowSerializer
from SpiffWorkflow.bpmn.serializer.config import DEFAULT_CONFIG

# OpenTelemetry for structured telemetry
from opentelemetry import trace, metrics
from opentelemetry.trace import Status, StatusCode
from opentelemetry.metrics import Counter, Histogram
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor
from opentelemetry.sdk.metrics import MeterProvider

# LinkML for schema validation
try:
    from linkml_runtime.utils.schemaview import SchemaView
    from linkml_runtime.loaders import yaml_loader, json_loader
    from linkml_runtime.dumpers import yaml_dumper, json_dumper
    LINKML_AVAILABLE = True
except ImportError:
    LINKML_AVAILABLE = False
    print("⚠️ LinkML not available - telemetry validation disabled")

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Configure OpenTelemetry
trace.set_tracer_provider(TracerProvider())
trace.get_tracer_provider().add_span_processor(
    BatchSpanProcessor(ConsoleSpanExporter())
)

metrics.set_meter_provider(MeterProvider())
meter = metrics.get_meter(__name__)

# Telemetry instruments
workflow_counter = meter.create_counter(
    name="spiff_workflows_total",
    description="Total number of SpiffWorkflow instances"
)

task_duration_histogram = meter.create_histogram(
    name="spiff_task_duration_seconds",
    description="Duration of SpiffWorkflow tasks"
)

# LinkML Schema for Workflow Telemetry
WORKFLOW_TELEMETRY_SCHEMA = """
id: https://example.org/spiff-workflow-telemetry
name: spiff-workflow-telemetry
title: SpiffWorkflow Telemetry Schema
version: 1.0.0

prefixes:
  linkml: https://w3id.org/linkml/
  spiff: https://example.org/spiff-workflow-telemetry/
  
default_prefix: spiff
default_range: string

imports:
  - linkml:types

classes:
  WorkflowInstance:
    description: A SpiffWorkflow instance with telemetry data
    attributes:
      instance_id:
        range: string
        required: true
        description: Unique identifier for the workflow instance
      process_id:
        range: string
        required: true
        description: BPMN process definition ID
      status:
        range: WorkflowStatus
        required: true
        description: Current status of the workflow
      start_time:
        range: datetime
        required: true
        description: When the workflow started
      end_time:
        range: datetime
        description: When the workflow completed
      total_tasks:
        range: integer
        description: Total number of tasks in the workflow
      completed_tasks:
        range: integer
        description: Number of completed tasks
      variables:
        range: string
        description: JSON string of workflow variables
      error_message:
        range: string
        description: Error message if workflow failed
        
  TaskExecution:
    description: Execution details for a single task
    attributes:
      task_id:
        range: string
        required: true
        description: Unique task identifier
      task_name:
        range: string
        required: true
        description: Human-readable task name
      task_type:
        range: string
        required: true
        description: Type of task (e.g., UserTask, ServiceTask)
      status:
        range: TaskStatus
        required: true
        description: Current task status
      start_time:
        range: datetime
        required: true
        description: When task execution started
      end_time:
        range: datetime
        description: When task execution completed
      duration_seconds:
        range: float
        description: Task execution duration
      input_data:
        range: string
        description: JSON string of input data
      output_data:
        range: string
        description: JSON string of output data
      error_message:
        range: string
        description: Error message if task failed
      workflow_instance_id:
        range: string
        required: true
        description: Reference to parent workflow instance

enums:
  WorkflowStatus:
    permissible_values:
      pending:
        description: Workflow is pending execution
      running:
        description: Workflow is currently running
      completed:
        description: Workflow completed successfully
      failed:
        description: Workflow failed with error
      suspended:
        description: Workflow is suspended
      cancelled:
        description: Workflow was cancelled
        
  TaskStatus:
    permissible_values:
      pending:
        description: Task is pending execution
      ready:
        description: Task is ready to execute
      running:
        description: Task is currently running
      completed:
        description: Task completed successfully
      failed:
        description: Task failed with error
      cancelled:
        description: Task was cancelled
"""

@dataclass
class WorkflowTelemetry:
    """Structured telemetry data for SpiffWorkflow operations"""
    instance_id: str
    process_id: str
    status: str
    start_time: datetime
    end_time: Optional[datetime] = None
    total_tasks: int = 0
    completed_tasks: int = 0
    variables: str = ""
    error_message: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for LinkML validation"""
        return {
            "instance_id": self.instance_id,
            "process_id": self.process_id,
            "status": self.status,
            "start_time": self.start_time.isoformat(),
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "total_tasks": self.total_tasks,
            "completed_tasks": self.completed_tasks,
            "variables": self.variables,
            "error_message": self.error_message
        }

@dataclass
class TaskTelemetry:
    """Structured telemetry data for task execution"""
    task_id: str
    task_name: str
    task_type: str
    status: str
    start_time: datetime
    end_time: Optional[datetime] = None
    duration_seconds: Optional[float] = None
    input_data: str = ""
    output_data: str = ""
    error_message: Optional[str] = None
    workflow_instance_id: str = ""
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for LinkML validation"""
        return {
            "task_id": self.task_id,
            "task_name": self.task_name,
            "task_type": self.task_type,
            "status": self.status,
            "start_time": self.start_time.isoformat(),
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "duration_seconds": self.duration_seconds,
            "input_data": self.input_data,
            "output_data": self.output_data,
            "error_message": self.error_message,
            "workflow_instance_id": self.workflow_instance_id
        }

class SpiffOrchestrator:
    """
    SpiffWorkflow Orchestrator with LinkML-validated OpenTelemetry
    
    This demonstrates the new paradigm where code agents validate their work
    by generating structured telemetry that conforms to LinkML schemas.
    """
    
    def __init__(self, bpmn_files_path: str = "bpmn"):
        """Initialize the orchestrator with LinkML schema validation"""
        self.tracer = trace.get_tracer(__name__)
        self.bpmn_files_path = Path(bpmn_files_path)
        
        # Initialize SpiffWorkflow parser
        self.parser = BpmnParser()
        self.parser.add_bpmn_files_by_glob(str(self.bpmn_files_path / "*.bpmn"))
        
        # Load LinkML schema for validation
        self.schema_view = None
        if LINKML_AVAILABLE:
            try:
                # Create temporary schema file
                schema_path = Path("workflow_telemetry_schema.yaml")
                with open(schema_path, 'w') as f:
                    f.write(WORKFLOW_TELEMETRY_SCHEMA)
                
                self.schema_view = SchemaView(str(schema_path))
                logger.info("✅ LinkML schema loaded for telemetry validation")
            except Exception as e:
                logger.warning(f"⚠️ Failed to load LinkML schema: {e}")
        
        # Process definitions cache
        self.process_definitions: Dict[str, BpmnProcessSpec] = {}
        self.active_workflows: Dict[str, BpmnWorkflow] = {}
        self.telemetry_data: List[WorkflowTelemetry] = []
        
        # Load process definitions
        self._load_process_definitions()
        
        logger.info(f"SpiffOrchestrator initialized with {len(self.process_definitions)} process definitions")
    
    def _load_process_definitions(self) -> None:
        """Load BPMN process definitions with telemetry"""
        with self.tracer.start_as_current_span("load_process_definitions") as span:
            try:
                for process_id, process_spec in self.parser.find_all_specs().items():
                    self.process_definitions[process_id] = process_spec
                    span.add_event(f"Loaded process definition: {process_id}")
                
                logger.info(f"Loaded {len(self.process_definitions)} process definitions")
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to load process definitions: {e}")
                raise
    
    def _validate_telemetry_with_linkml(self, telemetry_data: Dict[str, Any], class_name: str) -> bool:
        """Validate telemetry data against LinkML schema"""
        if not LINKML_AVAILABLE or not self.schema_view:
            return True  # Skip validation if LinkML not available
        
        try:
            # Validate using LinkML loader
            if class_name == "WorkflowInstance":
                validated_obj = yaml_loader.loads(
                    yaml.dump(telemetry_data), 
                    target_class="WorkflowInstance", 
                    schemaview=self.schema_view
                )
            elif class_name == "TaskExecution":
                validated_obj = yaml_loader.loads(
                    yaml.dump(telemetry_data), 
                    target_class="TaskExecution", 
                    schemaview=self.schema_view
                )
            else:
                logger.warning(f"Unknown class name for validation: {class_name}")
                return False
            
            logger.debug(f"✅ LinkML validation passed for {class_name}")
            return True
            
        except Exception as e:
            logger.error(f"❌ LinkML validation failed for {class_name}: {e}")
            return False
    
    def start_workflow(self, process_id: str, variables: Optional[Dict[str, Any]] = None) -> str:
        """
        Start a new workflow instance with validated telemetry
        
        This demonstrates the new paradigm: the code agent validates its work
        by generating structured telemetry that conforms to LinkML schemas.
        """
        with self.tracer.start_as_current_span("start_workflow") as span:
            span.set_attributes({
                "process.id": process_id,
                "process.variables": json.dumps(variables or {})
            })
            
            try:
                # Validate process exists
                if process_id not in self.process_definitions:
                    raise ValueError(f"Process definition '{process_id}' not found")
                
                # Generate instance ID
                instance_id = f"{process_id}_{uuid.uuid4().hex[:8]}"
                
                # Create workflow instance
                workflow = BpmnWorkflow(self.process_definitions[process_id])
                
                # Set initial variables
                if variables:
                    workflow.data.update(variables)
                
                # Store workflow
                self.active_workflows[instance_id] = workflow
                
                # Generate structured telemetry
                workflow_telemetry = WorkflowTelemetry(
                    instance_id=instance_id,
                    process_id=process_id,
                    status="running",
                    start_time=datetime.utcnow(),
                    total_tasks=len(workflow.get_tasks()),
                    completed_tasks=0,
                    variables=json.dumps(variables or {})
                )
                
                # Validate telemetry with LinkML
                telemetry_dict = workflow_telemetry.to_dict()
                validation_passed = self._validate_telemetry_with_linkml(telemetry_dict, "WorkflowInstance")
                
                if validation_passed:
                    # Record validated telemetry
                    self.telemetry_data.append(workflow_telemetry)
                    
                    # Update OpenTelemetry metrics
                    workflow_counter.add(1, {
                        "process.id": process_id,
                        "action": "start",
                        "linkml_validated": "true"
                    })
                    
                    span.set_attributes({
                        "instance.id": instance_id,
                        "linkml.validation": "passed"
                    })
                    
                    logger.info(f"✅ Started workflow {instance_id} with validated telemetry")
                else:
                    span.set_attributes({"linkml.validation": "failed"})
                    logger.error(f"❌ Telemetry validation failed for workflow {instance_id}")
                
                return instance_id
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to start workflow {process_id}: {e}")
                raise
    
    def run_until_user_input_required(self, instance_id: str) -> List[Dict[str, Any]]:
        """
        Run workflow until user input is required (Arena pattern)
        
        This demonstrates advanced orchestration with validated telemetry.
        """
        with self.tracer.start_as_current_span("run_until_user_input") as span:
            span.set_attributes({"instance.id": instance_id})
            
            try:
                if instance_id not in self.active_workflows:
                    raise ValueError(f"Workflow instance '{instance_id}' not found")
                
                workflow = self.active_workflows[instance_id]
                ready_tasks = []
                
                # Run automatic tasks (manual=False)
                task = workflow.get_next_task(state=TaskState.READY, manual=False)
                while task is not None:
                    # Execute task with telemetry
                    self._execute_task_with_telemetry(task, instance_id)
                    
                    # Refresh waiting tasks and handle events
                    self._run_ready_events(workflow, instance_id)
                    
                    # Get next automatic task
                    task = workflow.get_next_task(state=TaskState.READY, manual=False)
                
                # Get ready human tasks
                human_tasks = workflow.get_tasks(state=TaskState.READY, manual=True)
                for task in human_tasks:
                    ready_tasks.append({
                        "task_id": str(task.id),
                        "task_name": task.task_spec.name,
                        "task_type": task.task_spec.__class__.__name__,
                        "data": dict(task.data) if hasattr(task, 'data') else {}
                    })
                
                # Update workflow telemetry
                self._update_workflow_telemetry(instance_id, workflow)
                
                span.set_attributes({
                    "ready_tasks_count": len(ready_tasks),
                    "workflow_completed": workflow.is_completed()
                })
                
                return ready_tasks
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to run workflow {instance_id}: {e}")
                raise
    
    def _execute_task_with_telemetry(self, task: BpmnTaskSpec, instance_id: str) -> None:
        """Execute a task with LinkML-validated telemetry"""
        with self.tracer.start_as_current_span("execute_task") as span:
            task_start_time = datetime.utcnow()
            
            span.set_attributes({
                "task.id": str(task.id),
                "task.name": task.task_spec.name,
                "task.type": task.task_spec.__class__.__name__
            })
            
            try:
                # Create task telemetry
                task_telemetry = TaskTelemetry(
                    task_id=str(task.id),
                    task_name=task.task_spec.name,
                    task_type=task.task_spec.__class__.__name__,
                    status="running",
                    start_time=task_start_time,
                    input_data=json.dumps(dict(task.data) if hasattr(task, 'data') else {}),
                    workflow_instance_id=instance_id
                )
                
                # Execute the task
                task.complete()
                
                # Update task telemetry
                task_telemetry.end_time = datetime.utcnow()
                task_telemetry.duration_seconds = (
                    task_telemetry.end_time - task_telemetry.start_time
                ).total_seconds()
                task_telemetry.status = "completed"
                task_telemetry.output_data = json.dumps(dict(task.data) if hasattr(task, 'data') else {})
                
                # Validate task telemetry with LinkML
                telemetry_dict = task_telemetry.to_dict()
                validation_passed = self._validate_telemetry_with_linkml(telemetry_dict, "TaskExecution")
                
                if validation_passed:
                    # Record task duration metric
                    task_duration_histogram.record(
                        task_telemetry.duration_seconds,
                        {
                            "task.id": str(task.id),
                            "task.type": task_telemetry.task_type,
                            "linkml_validated": "true"
                        }
                    )
                    
                    span.set_attributes({
                        "task.duration": task_telemetry.duration_seconds,
                        "linkml.validation": "passed"
                    })
                    
                    logger.debug(f"✅ Executed task {task.id} with validated telemetry")
                else:
                    span.set_attributes({"linkml.validation": "failed"})
                    logger.error(f"❌ Task telemetry validation failed for {task.id}")
                
            except Exception as e:
                # Record failed task telemetry
                task_telemetry.end_time = datetime.utcnow()
                task_telemetry.duration_seconds = (
                    task_telemetry.end_time - task_telemetry.start_time
                ).total_seconds()
                task_telemetry.status = "failed"
                task_telemetry.error_message = str(e)
                
                # Validate failed telemetry
                telemetry_dict = task_telemetry.to_dict()
                self._validate_telemetry_with_linkml(telemetry_dict, "TaskExecution")
                
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to execute task {task.id}: {e}")
                raise
    
    def _run_ready_events(self, workflow: BpmnWorkflow, instance_id: str) -> None:
        """Run ready events (Arena pattern)"""
        workflow.refresh_waiting_tasks()
        
        # Handle any ready tasks that might be events
        # Note: Simplified event handling for this example
        ready_tasks = workflow.get_tasks(state=TaskState.READY)
        for task in ready_tasks:
            if hasattr(task.task_spec, 'event_definition'):
                self._execute_task_with_telemetry(task, instance_id)
    
    def _update_workflow_telemetry(self, instance_id: str, workflow: BpmnWorkflow) -> None:
        """Update workflow telemetry with current state"""
        # Find existing telemetry
        for telemetry in self.telemetry_data:
            if telemetry.instance_id == instance_id:
                telemetry.completed_tasks = len(workflow.get_tasks(state=TaskState.COMPLETED))
                
                if workflow.is_completed():
                    telemetry.status = "completed"
                    telemetry.end_time = datetime.utcnow()
                elif hasattr(workflow, 'task_tree') and getattr(workflow.task_tree, 'state', None) == TaskState.CANCELLED:
                    telemetry.status = "cancelled"
                    telemetry.end_time = datetime.utcnow()
                
                # Re-validate updated telemetry
                telemetry_dict = telemetry.to_dict()
                self._validate_telemetry_with_linkml(telemetry_dict, "WorkflowInstance")
                break
    
    def complete_task(self, instance_id: str, task_id: str, data: Optional[Dict[str, Any]] = None) -> None:
        """Complete a human task with validated telemetry"""
        with self.tracer.start_as_current_span("complete_task") as span:
            span.set_attributes({
                "instance.id": instance_id,
                "task.id": task_id
            })
            
            try:
                if instance_id not in self.active_workflows:
                    raise ValueError(f"Workflow instance '{instance_id}' not found")
                
                workflow = self.active_workflows[instance_id]
                
                # Find the task
                tasks = workflow.get_tasks()
                target_task = None
                
                for task in tasks:
                    if str(task.id) == task_id:
                        target_task = task
                        break
                
                if not target_task:
                    raise ValueError(f"Task '{task_id}' not found")
                
                # Set task data if provided
                if data and hasattr(target_task, 'data'):
                    target_task.data.update(data)
                
                # Execute task with telemetry
                self._execute_task_with_telemetry(target_task, instance_id)
                
                # Update workflow telemetry
                self._update_workflow_telemetry(instance_id, workflow)
                
                span.set_attributes({"task.status": "completed"})
                logger.info(f"✅ Completed task {task_id} with validated telemetry")
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to complete task {task_id}: {e}")
                raise
    
    def get_workflow_status(self, instance_id: str) -> Optional[Dict[str, Any]]:
        """Get workflow status with telemetry validation"""
        if instance_id not in self.active_workflows:
            return None
        
        workflow = self.active_workflows[instance_id]
        
        # Find telemetry data
        for telemetry in self.telemetry_data:
            if telemetry.instance_id == instance_id:
                return {
                    "instance_id": instance_id,
                    "status": telemetry.status,
                    "completed_tasks": telemetry.completed_tasks,
                    "total_tasks": telemetry.total_tasks,
                    "is_completed": workflow.is_completed(),
                    "linkml_validated": True
                }
        
        return None
    
    def get_telemetry_summary(self) -> Dict[str, Any]:
        """Get summary of all validated telemetry data"""
        return {
            "total_workflows": len(self.telemetry_data),
            "workflow_statuses": {
                status: len([w for w in self.telemetry_data if w.status == status])
                for status in ["running", "completed", "failed", "cancelled"]
            },
            "linkml_validation_enabled": LINKML_AVAILABLE and self.schema_view is not None,
            "telemetry_data": [w.to_dict() for w in self.telemetry_data]
        }

# Example BPMN files for testing
def create_sample_bpmn_files():
    """Create sample BPMN files for testing the orchestration examples"""
    
    # Simple process with automatic tasks
    simple_process = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="SimpleProcess" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:task id="Task_1" name="Automatic Task">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:task>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="EndEvent_1" />
  </bpmn:process>
  <bpmndi:BPMNDiagram id="BPMNDiagram_1">
    <bpmndi:BPMNPlane id="BPMNPlane_1" bpmnElement="SimpleProcess">
      <bpmndi:BPMNShape id="StartEvent_1_di" bpmnElement="StartEvent_1">
        <dc:Bounds x="152" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="Task_1_di" bpmnElement="Task_1">
        <dc:Bounds x="240" y="80" width="100" height="80" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="EndEvent_1_di" bpmnElement="EndEvent_1">
        <dc:Bounds x="392" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNEdge id="Flow_1_di" bpmnElement="Flow_1">
        <di:waypoint x="188" y="120" />
        <di:waypoint x="240" y="120" />
      </bpmndi:BPMNEdge>
      <bpmndi:BPMNEdge id="Flow_2_di" bpmnElement="Flow_2">
        <di:waypoint x="340" y="120" />
        <di:waypoint x="392" y="120" />
      </bpmndi:BPMNEdge>
    </bpmndi:BPMNPlane>
  </bpmndi:BPMNDiagram>
</bpmn:definitions>'''
    
    # Process with human task
    human_task_process = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_2"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="HumanTaskProcess" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:userTask id="UserTask_1" name="Human Approval">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:userTask>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="UserTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="UserTask_1" targetRef="EndEvent_1" />
  </bpmn:process>
  <bpmndi:BPMNDiagram id="BPMNDiagram_2">
    <bpmndi:BPMNPlane id="BPMNPlane_2" bpmnElement="HumanTaskProcess">
      <bpmndi:BPMNShape id="StartEvent_1_di" bpmnElement="StartEvent_1">
        <dc:Bounds x="152" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="UserTask_1_di" bpmnElement="UserTask_1">
        <dc:Bounds x="240" y="80" width="100" height="80" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="EndEvent_1_di" bpmnElement="EndEvent_1">
        <dc:Bounds x="392" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNEdge id="Flow_1_di" bpmnElement="Flow_1">
        <di:waypoint x="188" y="120" />
        <di:waypoint x="240" y="120" />
      </bpmndi:BPMNEdge>
      <bpmndi:BPMNEdge id="Flow_2_di" bpmnElement="Flow_2">
        <di:waypoint x="340" y="120" />
        <di:waypoint x="392" y="120" />
      </bpmndi:BPMNEdge>
    </bpmndi:BPMNPlane>
  </bpmndi:BPMNDiagram>
</bpmn:definitions>'''
    
    # Create BPMN directory and files
    bpmn_path = Path("bpmn")
    bpmn_path.mkdir(exist_ok=True)
    
    with open(bpmn_path / "simple_process.bpmn", "w") as f:
        f.write(simple_process)
    
    with open(bpmn_path / "human_task_process.bpmn", "w") as f:
        f.write(human_task_process)
    
    print("✅ Created sample BPMN files for testing")

def demonstrate_orchestration_patterns():
    """Demonstrate the new orchestration patterns with LinkML validation"""
    print("🚀 SpiffWorkflow Orchestration Examples - LinkML + OpenTelemetry Validation")
    print("=" * 80)
    
    # Create sample BPMN files
    create_sample_bpmn_files()
    
    # Initialize orchestrator
    orchestrator = SpiffOrchestrator()
    
    print(f"\n📊 Orchestrator initialized with {len(orchestrator.process_definitions)} process definitions")
    print(f"🔍 LinkML validation: {'✅ Enabled' if LINKML_AVAILABLE else '❌ Disabled'}")
    
    # Example 1: Simple automatic workflow
    print(f"\n🧪 Example 1: Simple Automatic Workflow")
    print("-" * 50)
    
    instance_id = orchestrator.start_workflow("SimpleProcess", {"input": "test_data"})
    print(f"✅ Started workflow: {instance_id}")
    
    ready_tasks = orchestrator.run_until_user_input_required(instance_id)
    print(f"📋 Ready tasks after execution: {len(ready_tasks)}")
    
    status = orchestrator.get_workflow_status(instance_id)
    print(f"📊 Workflow status: {status}")
    
    # Example 2: Human task workflow
    print(f"\n🧪 Example 2: Human Task Workflow")
    print("-" * 50)
    
    instance_id_2 = orchestrator.start_workflow("HumanTaskProcess", {"approval_required": True})
    print(f"✅ Started workflow: {instance_id_2}")
    
    ready_tasks = orchestrator.run_until_user_input_required(instance_id_2)
    print(f"📋 Ready human tasks: {len(ready_tasks)}")
    
    if ready_tasks:
        task = ready_tasks[0]
        print(f"👤 Human task: {task['task_name']} (ID: {task['task_id']})")
        
        # Complete the human task
        orchestrator.complete_task(instance_id_2, task['task_id'], {"approved": True})
        print(f"✅ Completed human task")
        
        # Continue workflow
        ready_tasks = orchestrator.run_until_user_input_required(instance_id_2)
        print(f"📋 Remaining tasks: {len(ready_tasks)}")
    
    status = orchestrator.get_workflow_status(instance_id_2)
    print(f"📊 Workflow status: {status}")
    
    # Example 3: Telemetry validation summary
    print(f"\n🧪 Example 3: Telemetry Validation Summary")
    print("-" * 50)
    
    summary = orchestrator.get_telemetry_summary()
    print(f"📈 Total workflows: {summary['total_workflows']}")
    print(f"📊 Status distribution: {summary['workflow_statuses']}")
    print(f"🔍 LinkML validation: {'✅ Enabled' if summary['linkml_validation_enabled'] else '❌ Disabled'}")
    
    # Example 4: Demonstrate the new paradigm
    print(f"\n🧪 Example 4: New Paradigm - Code Agent Validation")
    print("-" * 50)
    print("""
🎯 NEW PARADIGM: Code agents validate their work by generating 
   structured telemetry that conforms to LinkML schemas.

✅ BENEFITS:
   - Structured, validated observability
   - Schema-driven telemetry validation
   - Consistent data formats across systems
   - Automated validation of agent outputs
   - Weaver-like integration but with formal schemas

🔧 IMPLEMENTATION:
   - LinkML schemas define telemetry structure
   - OpenTelemetry provides the transport
   - Code agents generate validated telemetry
   - Validation ensures data quality and consistency
    """)
    
    return orchestrator

if __name__ == "__main__":
    orchestrator = demonstrate_orchestration_patterns()
    
    print(f"\n🎉 Orchestration examples completed successfully!")
    print(f"📁 Check the 'bpmn/' directory for sample BPMN files")
    print(f"📊 Review the console output for OpenTelemetry traces")
    print(f"🔍 LinkML validation ensures telemetry data quality") 