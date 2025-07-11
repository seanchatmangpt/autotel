#!/usr/bin/env python3
"""
AutoTel BPMN Orchestrator - Enterprise BPMN 2.0 Execution Engine
Powered by SpiffWorkflow with zero-touch telemetry integration
"""

import json
import uuid
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Any, Optional, Callable, Union
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import xml.etree.ElementTree as ET

# SpiffWorkflow imports
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
from SpiffWorkflow.bpmn.specs.bpmn_process_spec import BpmnProcessSpec
from SpiffWorkflow.bpmn.util.task import BpmnTaskFilter
from SpiffWorkflow.bpmn.util.subworkflow import BpmnSubWorkflow
from SpiffWorkflow.bpmn.serializer import BpmnWorkflowSerializer
from SpiffWorkflow.bpmn.serializer.config import DEFAULT_CONFIG
from SpiffWorkflow.util.task import TaskState  # <-- Add this import

# OpenTelemetry for observability
from opentelemetry import trace, metrics
from opentelemetry.trace import Status, StatusCode
from opentelemetry.metrics import Counter, Histogram
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import ConsoleMetricExporter, PeriodicExportingMetricReader

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
process_counter = meter.create_counter(
    name="bpmn_processes_total",
    description="Total number of BPMN processes executed"
)

task_duration_histogram = meter.create_histogram(
    name="bpmn_task_duration_seconds",
    description="Duration of BPMN tasks in seconds"
)

error_counter = meter.create_counter(
    name="bpmn_errors_total",
    description="Total number of BPMN execution errors"
)

class ProcessStatus(Enum):
    """BPMN Process execution status"""
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    SUSPENDED = "suspended"
    CANCELLED = "cancelled"

class TaskStatus(Enum):
    """BPMN Task execution status"""
    PENDING = "pending"
    READY = "ready"
    RUNNING = "running" 
    COMPLETED = "completed"
    FAILED = "failed"
    SKIPPED = "skipped"
    CANCELLED = "cancelled"

@dataclass
class ProcessInstance:
    """BPMN Process instance with execution state and telemetry"""
    instance_id: str
    process_definition_id: str
    workflow: BpmnWorkflow
    start_time: datetime
    end_time: Optional[datetime] = None
    status: ProcessStatus = ProcessStatus.PENDING
    variables: Dict[str, Any] = field(default_factory=dict)
    execution_path: List[str] = field(default_factory=list)
    error_message: Optional[str] = None
    telemetry_data: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert process instance to dictionary for serialization"""
        return {
            "instance_id": self.instance_id,
            "process_definition_id": self.process_definition_id,
            "start_time": self.start_time.isoformat(),
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "status": self.status.value,
            "variables": self.variables,
            "execution_path": self.execution_path,
            "error_message": self.error_message,
            "telemetry_data": self.telemetry_data
        }

@dataclass
class TaskExecution:
    """BPMN Task execution context with telemetry"""
    task_id: str
    task_name: str
    task_type: str
    start_time: datetime
    end_time: Optional[datetime] = None
    status: TaskStatus = TaskStatus.PENDING
    duration: Optional[float] = None
    error_message: Optional[str] = None
    input_data: Dict[str, Any] = field(default_factory=dict)
    output_data: Dict[str, Any] = field(default_factory=dict)
    telemetry_data: Dict[str, Any] = field(default_factory=dict)

class BPMNOrchestrator:
    """
    Enterprise-grade BPMN 2.0 Orchestrator powered by SpiffWorkflow
    
    Features:
    - Full BPMN 2.0 specification compliance
    - Zero-touch telemetry integration
    - Process persistence and recovery
    - Advanced task execution monitoring
    - Multi-instance process support
    - Event-driven execution
    - Error handling and recovery
    """
    
    def __init__(self, 
                 bpmn_files_path: Optional[str] = None,
                 enable_telemetry: bool = True,
                 enable_persistence: bool = True):
        """
        Initialize the BPMN Orchestrator
        
        Args:
            bpmn_files_path: Path to BPMN XML files
            enable_telemetry: Enable OpenTelemetry integration
            enable_persistence: Enable process state persistence
        """
        self.tracer = trace.get_tracer(__name__)
        self.bpmn_files_path = Path(bpmn_files_path) if bpmn_files_path else Path("bpmn")
        self.enable_telemetry = enable_telemetry
        self.enable_persistence = enable_persistence
        
        # Initialize SpiffWorkflow parser
        self.parser = BpmnParser()
        self.parser.add_bpmn_files_by_glob(str(self.bpmn_files_path / "*.bpmn"))
        
        # Process definitions cache
        self.process_definitions: Dict[str, BpmnProcessSpec] = {}
        self.active_instances: Dict[str, ProcessInstance] = {}
        
        # Load process definitions
        self._load_process_definitions()
        
        logger.info(f"BPMN Orchestrator initialized with {len(self.process_definitions)} process definitions")
    
    def _load_process_definitions(self) -> None:
        """Load all BPMN process definitions from the configured path"""
        with self.tracer.start_as_current_span("load_process_definitions") as span:
            try:
                # Get all process specs from the parser
                for process_id, process_spec in self.parser.find_all_specs().items():
                    self.process_definitions[process_id] = process_spec
                    span.add_event(f"Loaded process definition: {process_id}")
                
                logger.info(f"Loaded {len(self.process_definitions)} process definitions")
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to load process definitions: {e}")
                raise
    
    def start_process(self, 
                     process_id: str, 
                     variables: Optional[Dict[str, Any]] = None,
                     instance_id: Optional[str] = None) -> ProcessInstance:
        """
        Start a new BPMN process instance
        
        Args:
            process_id: ID of the process to start
            variables: Initial process variables
            instance_id: Optional custom instance ID
            
        Returns:
            ProcessInstance: The started process instance
        """
        with self.tracer.start_as_current_span("start_process") as span:
            span.set_attributes({
                "process.id": process_id,
                "process.variables": json.dumps(variables or {})
            })
            
            try:
                # Validate process exists
                if process_id not in self.process_definitions:
                    raise ValueError(f"Process definition '{process_id}' not found")
                
                # Generate instance ID if not provided
                if not instance_id:
                    instance_id = f"{process_id}_{uuid.uuid4().hex[:8]}"
                
                # Create workflow instance
                workflow = BpmnWorkflow(self.process_definitions[process_id])
                
                # Set initial variables
                if variables:
                    workflow.data.update(variables)
                
                # Create process instance
                process_instance = ProcessInstance(
                    instance_id=instance_id,
                    process_definition_id=process_id,
                    workflow=workflow,
                    start_time=datetime.utcnow(),
                    variables=variables or {},
                    status=ProcessStatus.RUNNING
                )
                
                # Store instance
                self.active_instances[instance_id] = process_instance
                
                # Update telemetry
                process_counter.add(1, {"process.id": process_id, "action": "start"})
                
                span.set_attributes({
                    "instance.id": instance_id,
                    "instance.status": ProcessStatus.RUNNING.value
                })
                
                logger.info(f"Started process instance {instance_id} for process {process_id}")
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {"process.id": process_id, "error.type": "start_failed"})
                logger.error(f"Failed to start process {process_id}: {e}")
                raise
    
    def execute_process(self, instance_id: str, max_steps: int = 100) -> ProcessInstance:
        """
        Execute a BPMN process instance
        
        Args:
            instance_id: ID of the process instance to execute
            max_steps: Maximum number of execution steps
            
        Returns:
            ProcessInstance: Updated process instance
        """
        with self.tracer.start_as_current_span("execute_process") as span:
            span.set_attributes({
                "instance.id": instance_id,
                "execution.max_steps": max_steps
            })
            
            try:
                # Get process instance
                if instance_id not in self.active_instances:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                process_instance = self.active_instances[instance_id]
                workflow = process_instance.workflow
                
                # Execute workflow
                step_count = 0
                while not workflow.is_completed() and step_count < max_steps:
                    # Get ready tasks
                    ready_tasks = workflow.get_tasks(state=TaskState.READY)
                    
                    if not ready_tasks:
                        # No ready tasks, workflow might be waiting for external events
                        break
                    
                    # Execute each ready task
                    for task in ready_tasks:
                        self._execute_task(task, process_instance)
                        step_count += 1
                        
                        if workflow.is_completed():
                            break
                
                # Update process status
                if workflow.is_completed():
                    process_instance.status = ProcessStatus.COMPLETED
                    process_instance.end_time = datetime.utcnow()
                elif step_count >= max_steps:
                    process_instance.status = ProcessStatus.SUSPENDED
                
                # Update execution path
                process_instance.execution_path = self._get_execution_path(workflow)
                
                span.set_attributes({
                    "execution.steps": step_count,
                    "instance.status": process_instance.status.value
                })
                
                logger.info(f"Executed process instance {instance_id} ({step_count} steps)")
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {"instance.id": instance_id, "error.type": "execution_failed"})
                logger.error(f"Failed to execute process instance {instance_id}: {e}")
                raise
    
    def _execute_task(self, task: BpmnTaskSpec, process_instance: ProcessInstance) -> None:
        """Execute a single BPMN task with telemetry"""
        with self.tracer.start_as_current_span("execute_task") as span:
            task_start_time = datetime.utcnow()
            
            span.set_attributes({
                "task.id": task.id,
                "task.name": task.task_spec.name,
                "task.type": task.task_spec.__class__.__name__
            })
            
            try:
                # Create task execution context
                task_execution = TaskExecution(
                    task_id=task.id,
                    task_name=task.task_spec.name,
                    task_type=task.task_spec.__class__.__name__,
                    start_time=task_start_time,
                    status=TaskStatus.RUNNING,
                    input_data=dict(task.data) if hasattr(task, 'data') else {}
                )
                
                # Execute the task
                task.complete()
                
                # Update task execution
                task_execution.end_time = datetime.utcnow()
                task_execution.duration = (task_execution.end_time - task_execution.start_time).total_seconds()
                task_execution.status = TaskStatus.COMPLETED
                task_execution.output_data = dict(task.data) if hasattr(task, 'data') else {}
                
                # Record telemetry
                if self.enable_telemetry:
                    task_duration_histogram.record(
                        task_execution.duration,
                        {
                            "task.id": task.id,
                            "task.type": task_execution.task_type,
                            "process.id": process_instance.process_definition_id
                        }
                    )
                
                span.set_attributes({
                    "task.duration": task_execution.duration,
                    "task.status": TaskStatus.COMPLETED.value
                })
                
                logger.debug(f"Executed task {task.id} ({task_execution.duration:.3f}s)")
                
            except Exception as e:
                task_execution.end_time = datetime.utcnow()
                task_execution.duration = (task_execution.end_time - task_execution.start_time).total_seconds()
                task_execution.status = TaskStatus.FAILED
                task_execution.error_message = str(e)
                
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {
                    "task.id": task.id,
                    "task.type": task.task_spec.__class__.__name__,
                    "error.type": "task_execution_failed"
                })
                
                logger.error(f"Failed to execute task {task.id}: {e}")
                raise
    
    def _get_execution_path(self, workflow: BpmnWorkflow) -> List[str]:
        """Get the execution path of completed tasks"""
        try:
            completed_tasks = workflow.get_tasks(state=TaskState.COMPLETED)
            return [task.id for task in completed_tasks]
        except Exception as e:
            logger.warning(f"Failed to get execution path: {e}")
            return []
    
    def get_process_instance(self, instance_id: str) -> Optional[ProcessInstance]:
        """Get a process instance by ID"""
        return self.active_instances.get(instance_id)
    
    def list_process_instances(self, 
                             status: Optional[ProcessStatus] = None,
                             process_id: Optional[str] = None) -> List[ProcessInstance]:
        """List process instances with optional filtering"""
        instances = list(self.active_instances.values())
        
        if status:
            instances = [i for i in instances if i.status == status]
        
        if process_id:
            instances = [i for i in instances if i.process_definition_id == process_id]
        
        return instances
    
    def cancel_process(self, instance_id: str) -> ProcessInstance:
        """Cancel a running process instance"""
        with self.tracer.start_as_current_span("cancel_process") as span:
            span.set_attributes({"instance.id": instance_id})
            
            try:
                if instance_id not in self.active_instances:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                process_instance = self.active_instances[instance_id]
                
                if process_instance.status not in [ProcessStatus.RUNNING, ProcessStatus.SUSPENDED]:
                    raise ValueError(f"Cannot cancel process in status: {process_instance.status}")
                
                # Cancel the workflow
                process_instance.workflow.cancel()
                process_instance.status = ProcessStatus.CANCELLED
                process_instance.end_time = datetime.utcnow()
                
                span.set_attributes({"instance.status": ProcessStatus.CANCELLED.value})
                
                logger.info(f"Cancelled process instance {instance_id}")
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {"instance.id": instance_id, "error.type": "cancel_failed"})
                logger.error(f"Failed to cancel process instance {instance_id}: {e}")
                raise
    
    def get_process_variables(self, instance_id: str) -> Dict[str, Any]:
        """Get process variables for an instance"""
        if instance_id not in self.active_instances:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        process_instance = self.active_instances[instance_id]
        return dict(process_instance.workflow.data)
    
    def set_process_variables(self, instance_id: str, variables: Dict[str, Any]) -> None:
        """Set process variables for an instance"""
        if instance_id not in self.active_instances:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        process_instance = self.active_instances[instance_id]
        process_instance.workflow.data.update(variables)
        process_instance.variables.update(variables)
    
    def get_ready_tasks(self, instance_id: str) -> List[Dict[str, Any]]:
        """Get ready tasks for a process instance"""
        if instance_id not in self.active_instances:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        process_instance = self.active_instances[instance_id]
        ready_tasks = process_instance.workflow.get_tasks(state=TaskState.READY)
        
        return [
            {
                "task_id": task.id,
                "task_name": task.task_spec.name,
                "task_type": task.task_spec.__class__.__name__,
                "data": dict(task.data) if hasattr(task, 'data') else {}
            }
            for task in ready_tasks
        ]
    
    def complete_task(self, instance_id: str, task_id: str, data: Optional[Dict[str, Any]] = None) -> None:
        """Complete a specific task in a process instance"""
        with self.tracer.start_as_current_span("complete_task") as span:
            span.set_attributes({
                "instance.id": instance_id,
                "task.id": task_id
            })
            
            try:
                if instance_id not in self.active_instances:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                process_instance = self.active_instances[instance_id]
                workflow = process_instance.workflow
                
                # Find the task
                tasks = workflow.get_tasks()
                target_task = None
                
                for task in tasks:
                    if task.id == task_id:
                        target_task = task
                        break
                
                if not target_task:
                    raise ValueError(f"Task '{task_id}' not found in process instance '{instance_id}'")
                
                # Set task data if provided
                if data and hasattr(target_task, 'data'):
                    target_task.data.update(data)
                
                # Complete the task
                target_task.complete()
                
                span.set_attributes({"task.status": "completed"})
                
                logger.info(f"Completed task {task_id} in process instance {instance_id}")
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {
                    "instance.id": instance_id,
                    "task.id": task_id,
                    "error.type": "task_completion_failed"
                })
                logger.error(f"Failed to complete task {task_id} in process instance {instance_id}: {e}")
                raise
    
    def serialize_workflow(self, instance_id: str) -> str:
        """Serialize a workflow instance to JSON"""
        if instance_id not in self.active_instances:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        process_instance = self.active_instances[instance_id]
        serializer = BpmnWorkflowSerializer(DEFAULT_CONFIG)
        return serializer.serialize_workflow(process_instance.workflow)
    
    def deserialize_workflow(self, instance_id: str, serialized_data: str) -> ProcessInstance:
        """Deserialize a workflow instance from JSON"""
        serializer = BpmnWorkflowSerializer(DEFAULT_CONFIG)
        workflow = serializer.deserialize_workflow(serialized_data)
        
        process_instance = ProcessInstance(
            instance_id=instance_id,
            process_definition_id=workflow.spec.name,
            workflow=workflow,
            start_time=datetime.utcnow(),
            status=ProcessStatus.RUNNING
        )
        
        self.active_instances[instance_id] = process_instance
        return process_instance
    
    def get_process_statistics(self) -> Dict[str, Any]:
        """Get orchestrator statistics"""
        total_instances = len(self.active_instances)
        status_counts = {}
        
        for status in ProcessStatus:
            status_counts[status.value] = len([
                i for i in self.active_instances.values() 
                if i.status == status
            ])
        
        return {
            "total_process_definitions": len(self.process_definitions),
            "total_instances": total_instances,
            "status_distribution": status_counts,
            "process_definitions": list(self.process_definitions.keys())
        }
    
    def cleanup_completed_processes(self, max_age_hours: int = 24) -> int:
        """Clean up completed process instances older than specified age"""
        cutoff_time = datetime.utcnow() - timedelta(hours=max_age_hours)
        removed_count = 0
        
        instances_to_remove = []
        
        for instance_id, process_instance in self.active_instances.items():
            if (process_instance.status in [ProcessStatus.COMPLETED, ProcessStatus.FAILED, ProcessStatus.CANCELLED] and
                process_instance.end_time and process_instance.end_time < cutoff_time):
                instances_to_remove.append(instance_id)
        
        for instance_id in instances_to_remove:
            del self.active_instances[instance_id]
            removed_count += 1
        
        logger.info(f"Cleaned up {removed_count} completed process instances")
        return removed_count

# Example usage and utility functions
def create_sample_bpmn_file() -> str:
    """Create a sample BPMN file for testing"""
    sample_bpmn = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="Process_1" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:task id="Task_1" name="Sample Task">
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
    <bpmndi:BPMNPlane id="BPMNPlane_1" bpmnElement="Process_1">
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
    
    return sample_bpmn

if __name__ == "__main__":
    # Example usage
    print("AutoTel BPMN Orchestrator - SpiffWorkflow Integration")
    print("=" * 60)
    
    # Create sample BPMN file
    sample_bpmn = create_sample_bpmn_file()
    bpmn_path = Path("bpmn")
    bpmn_path.mkdir(exist_ok=True)
    
    with open(bpmn_path / "sample_process.bpmn", "w") as f:
        f.write(sample_bpmn)
    
    # Initialize orchestrator
    orchestrator = BPMNOrchestrator(bpmn_files_path="bpmn")
    
    # Start a process
    instance = orchestrator.start_process("Process_1", {"input": "test"})
    print(f"Started process instance: {instance.instance_id}")
    
    # Execute the process
    result = orchestrator.execute_process(instance.instance_id)
    print(f"Process execution completed. Status: {result.status.value}")
    
    # Get statistics
    stats = orchestrator.get_process_statistics()
    print(f"Orchestrator statistics: {stats}")