#!/usr/bin/env python3
"""
AutoTel Orchestrator - Enterprise BPMN 2.0 Execution Engine
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
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.serializer.json import JSONSerializer

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

from autotel.utils import persistence

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

class Orchestrator:
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
        
        # Initialize SpiffWorkflow parser with DSPy support
        from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
        self.parser = DspyBpmnParser()
        self.parser.add_bpmn_files_by_glob(str(self.bpmn_files_path / "*.bpmn"))
        
        # Process definitions cache
        self.process_definitions: Dict[str, BpmnProcessSpec] = {}
        self.active_instances: Dict[str, ProcessInstance] = {}
        
        # Load process definitions
        self._load_process_definitions()
        
        logger.info(f"Orchestrator initialized with {len(self.process_definitions)} process definitions")
        # Load persisted instances
        if self.enable_persistence:
            self._load_persisted_instances()
    
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
    
    def _load_persisted_instances(self):
        """Load all persisted process instances from SQLite"""
        for row in persistence.list_instances():
            try:
                process_spec = self.process_definitions.get(row["process_definition_id"])
                if not process_spec:
                    logger.error(f"Process spec for {row['process_definition_id']} not found")
                    continue
                workflow = BpmnWorkflow(process_spec)
                workflow.data.update(row["variables"])
                process_instance = ProcessInstance(
                    instance_id=row["instance_id"],
                    process_definition_id=row["process_definition_id"],
                    workflow=workflow,
                    start_time=datetime.fromisoformat(row["start_time"]),
                    end_time=datetime.fromisoformat(row["end_time"]) if row["end_time"] else None,
                    status=ProcessStatus(row["status"]),
                    variables=row["variables"]
                )
                self.active_instances[process_instance.instance_id] = process_instance
            except Exception as e:
                logger.error(f"Failed to load persisted instance {row['instance_id']}: {e}")

    def _persist_instance(self, instance: ProcessInstance):
        if not self.enable_persistence:
            return
        persistence.save_instance(
            instance_id=instance.instance_id,
            process_definition_id=instance.process_definition_id,
            status=instance.status.value,
            variables=instance.variables,
            start_time=instance.start_time.isoformat(),
            end_time=instance.end_time.isoformat() if instance.end_time else None
        )

    def _delete_instance(self, instance_id: str):
        if not self.enable_persistence:
            return
        persistence.delete_instance(instance_id)

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
            ProcessInstance: The created process instance
        """
        with self.tracer.start_as_current_span("start_process") as span:
            try:
                span.set_attribute("process.id", process_id)
                span.set_attribute("instance.id", instance_id or "auto-generated")
                
                if process_id not in self.process_definitions:
                    raise ValueError(f"Process definition '{process_id}' not found")
                
                # Generate instance ID if not provided
                if not instance_id:
                    instance_id = f"{process_id}_{uuid.uuid4().hex[:8]}"
                
                # Create workflow instance
                process_spec = self.process_definitions[process_id]
                workflow = BpmnWorkflow(process_spec)
                
                # Set initial variables
                if variables:
                    workflow.data.update(variables)
                
                # Create process instance
                process_instance = ProcessInstance(
                    instance_id=instance_id,
                    process_definition_id=process_id,
                    workflow=workflow,
                    start_time=datetime.now(),
                    variables=variables or {},
                    status=ProcessStatus.RUNNING
                )
                
                # Store instance
                self.active_instances[instance_id] = process_instance
                self._persist_instance(process_instance)
                
                # Update telemetry
                if self.enable_telemetry:
                    process_counter.add(1, {"process_id": process_id})
                
                span.set_attribute("instance.status", process_instance.status.value)
                logger.info(f"Started process instance: {instance_id}")
                
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {"process_id": process_id, "error": str(e)})
                logger.error(f"Failed to start process {process_id}: {e}")
                raise
    
    def execute_process(self, instance_id: str, max_steps: int = 100) -> ProcessInstance:
        """
        Execute a BPMN process instance
        
        Args:
            instance_id: ID of the process instance to execute
            max_steps: Maximum number of execution steps
            
        Returns:
            ProcessInstance: The updated process instance
        """
        with self.tracer.start_as_current_span("execute_process") as span:
            try:
                span.set_attribute("instance.id", instance_id)
                span.set_attribute("max.steps", max_steps)
                
                process_instance = self.get_process_instance(instance_id)
                if not process_instance:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                if process_instance.status not in [ProcessStatus.RUNNING, ProcessStatus.PENDING]:
                    logger.warning(f"Process instance {instance_id} is not in executable state: {process_instance.status}")
                    return process_instance
                
                workflow = process_instance.workflow
                steps_executed = 0
                
                # Execute workflow steps
                while workflow.is_completed() is False and steps_executed < max_steps:
                    ready_tasks = workflow.get_tasks(TaskState.READY)
                    
                    if not ready_tasks:
                        # No ready tasks, try to complete the workflow
                        if workflow.is_completed():
                            break
                        else:
                            # Workflow is waiting for external input
                            process_instance.status = ProcessStatus.SUSPENDED
                            break
                    
                    # Execute ready tasks
                    for task in ready_tasks:
                        if steps_executed >= max_steps:
                            break
                        
                        try:
                            self._execute_task(task, process_instance)
                            steps_executed += 1
                            
                        except Exception as e:
                            process_instance.status = ProcessStatus.FAILED
                            process_instance.error_message = str(e)
                            span.set_status(Status(StatusCode.ERROR, str(e)))
                            error_counter.add(1, {"instance_id": instance_id, "task_id": task.id, "error": str(e)})
                            logger.error(f"Task execution failed: {e}")
                            return process_instance
                
                # Update instance status
                if workflow.is_completed():
                    process_instance.status = ProcessStatus.COMPLETED
                    process_instance.end_time = datetime.now()
                elif process_instance.status == ProcessStatus.SUSPENDED:
                    pass  # Keep suspended status
                else:
                    process_instance.status = ProcessStatus.RUNNING
                
                # Update execution path
                process_instance.execution_path = self._get_execution_path(workflow)
                
                span.set_attribute("steps.executed", steps_executed)
                span.set_attribute("instance.status", process_instance.status.value)
                
                logger.info(f"Process execution completed: {instance_id} - Status: {process_instance.status.value}")
                self._persist_instance(process_instance)
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                error_counter.add(1, {"instance_id": instance_id, "error": str(e)})
                logger.error(f"Failed to execute process {instance_id}: {e}")
                raise
    
    def _execute_task(self, task: BpmnTaskSpec, process_instance: ProcessInstance) -> None:
        """Execute a single BPMN task with telemetry and DSPy support"""
        with self.tracer.start_as_current_span("execute_task") as span:
            try:
                span.set_attribute("task.id", task.id)
                span.set_attribute("task.name", task.name)
                span.set_attribute("task.type", task.task_spec.__class__.__name__)
                
                start_time = datetime.now()
                
                # Check for DSPy service extensions
                ext = getattr(task.task_spec, 'extensions', None)
                if ext and ext.get('dspy_service'):
                    # Execute DSPy service
                    self._execute_dspy_service(task, process_instance, ext['dspy_service'])
                else:
                    # Execute the task normally
                    task.run()
                
                end_time = datetime.now()
                duration = (end_time - start_time).total_seconds()
                
                # Update telemetry
                if self.enable_telemetry:
                    task_duration_histogram.record(duration, {
                        "task_id": task.id,
                        "task_name": task.name,
                        "task_type": task.task_spec.__class__.__name__
                    })
                
                span.set_attribute("task.duration", duration)
                span.set_attribute("task.success", True)
                
                logger.debug(f"Task executed: {task.name} ({task.id}) - Duration: {duration:.3f}s")
                
            except Exception as e:
                span.set_attribute("task.success", False)
                span.set_attribute("task.error", str(e))
                logger.error(f"Task execution failed: {task.name} ({task.id}) - Error: {e}")
                raise
    
    def _execute_dspy_service(self, task: BpmnTaskSpec, process_instance: ProcessInstance, dspy_info: dict) -> None:
        """Execute a DSPy service and store the result"""
        from autotel.utils.dspy_services import dspy_service
        
        # Resolve parameters from workflow data
        resolved_params = {}
        for param_name, param_value in dspy_info['params'].items():
            # Get the value from workflow data
            resolved_params[param_name] = process_instance.workflow.get_data(param_value)
        
        # Call the DSPy service
        result = dspy_service(dspy_info['service'], **resolved_params)
        
        # Store the result if specified
        if dspy_info['result']:
            process_instance.workflow.set_data(**{dspy_info['result']: result})
            process_instance.variables[dspy_info['result']] = result
        
        # Complete the task
        task.complete()
        
        logger.info(f"DSPy service executed: {dspy_info['service']} -> {dspy_info['result']}")
    
    def _get_execution_path(self, workflow: BpmnWorkflow) -> List[str]:
        """Get the execution path of completed tasks"""
        completed_tasks = workflow.get_tasks(TaskState.COMPLETED)
        return [task.name for task in completed_tasks]
    
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
            try:
                span.set_attribute("instance.id", instance_id)
                
                process_instance = self.get_process_instance(instance_id)
                if not process_instance:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                if process_instance.status not in [ProcessStatus.RUNNING, ProcessStatus.PENDING, ProcessStatus.SUSPENDED]:
                    raise ValueError(f"Cannot cancel process in status: {process_instance.status}")
                
                process_instance.status = ProcessStatus.CANCELLED
                process_instance.end_time = datetime.now()
                
                span.set_attribute("instance.status", process_instance.status.value)
                logger.info(f"Process cancelled: {instance_id}")
                
                self._persist_instance(process_instance)
                return process_instance
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to cancel process {instance_id}: {e}")
                raise
    
    def get_process_variables(self, instance_id: str) -> Dict[str, Any]:
        """Get process instance variables"""
        process_instance = self.get_process_instance(instance_id)
        if not process_instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        return process_instance.variables.copy()
    
    def set_process_variables(self, instance_id: str, variables: Dict[str, Any]) -> None:
        """Set process instance variables"""
        process_instance = self.get_process_instance(instance_id)
        if not process_instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        process_instance.variables.update(variables)
        process_instance.workflow.data.update(variables)
        self._persist_instance(process_instance)
    
    def get_ready_tasks(self, instance_id: str) -> List[Dict[str, Any]]:
        """Get ready tasks for a process instance"""
        process_instance = self.get_process_instance(instance_id)
        if not process_instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        ready_tasks = process_instance.workflow.get_tasks(TaskState.READY)
        
        return [
            {
                "task_id": task.id,
                "task_name": task.name,
                "task_type": task.task_spec.__class__.__name__,
                "status": "ready"
            }
            for task in ready_tasks
        ]
    
    def complete_task(self, instance_id: str, task_id: str, data: Optional[Dict[str, Any]] = None) -> None:
        """Complete a task in a process instance"""
        with self.tracer.start_as_current_span("complete_task") as span:
            try:
                span.set_attribute("instance.id", instance_id)
                span.set_attribute("task.id", task_id)
                
                process_instance = self.get_process_instance(instance_id)
                if not process_instance:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                # Find the task
                ready_tasks = process_instance.workflow.get_tasks(TaskState.READY)
                task = next((t for t in ready_tasks if t.id == task_id), None)
                
                if not task:
                    raise ValueError(f"Task '{task_id}' not found or not ready")
                
                # Set task data if provided
                if data:
                    task.data.update(data)
                    process_instance.variables.update(data)
                
                # Complete the task
                task.complete()
                
                span.set_attribute("task.completed", True)
                logger.info(f"Task completed: {task_id}")
                
                self._persist_instance(process_instance)
                
            except Exception as e:
                span.set_status(Status(StatusCode.ERROR, str(e)))
                logger.error(f"Failed to complete task {task_id}: {e}")
                raise
    
    def serialize_workflow(self, instance_id: str) -> str:
        """Serialize a workflow instance to JSON"""
        process_instance = self.get_process_instance(instance_id)
        if not process_instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        serializer = JSONSerializer()
        return serializer.serialize_workflow(process_instance.workflow)
    
    def deserialize_workflow(self, instance_id: str, serialized_data: str) -> ProcessInstance:
        raise NotImplementedError("deserialize_workflow is not used in best-practice persistence mode.")
    
    def get_process_statistics(self) -> Dict[str, Any]:
        """Get process execution statistics"""
        instances = list(self.active_instances.values())
        
        total_instances = len(instances)
        running_instances = len([i for i in instances if i.status == ProcessStatus.RUNNING])
        completed_instances = len([i for i in instances if i.status == ProcessStatus.COMPLETED])
        failed_instances = len([i for i in instances if i.status == ProcessStatus.FAILED])
        
        # Calculate average execution time
        completed_with_time = [i for i in instances if i.status == ProcessStatus.COMPLETED and i.end_time]
        avg_execution_time = None
        if completed_with_time:
            total_time = sum((i.end_time - i.start_time).total_seconds() for i in completed_with_time)
            avg_execution_time = f"{total_time / len(completed_with_time):.2f}s"
        
        return {
            "total_instances": total_instances,
            "running_instances": running_instances,
            "completed_instances": completed_instances,
            "failed_instances": failed_instances,
            "avg_execution_time": avg_execution_time,
            "process_definitions": len(self.process_definitions)
        }
    
    def cleanup_completed_processes(self, max_age_hours: int = 24) -> int:
        """Clean up completed process instances older than specified hours"""
        cutoff_time = datetime.now() - timedelta(hours=max_age_hours)
        instances_to_remove = []
        
        for instance_id, instance in self.active_instances.items():
            if (instance.status in [ProcessStatus.COMPLETED, ProcessStatus.FAILED, ProcessStatus.CANCELLED] and
                instance.end_time and instance.end_time < cutoff_time):
                instances_to_remove.append(instance_id)
        
        to_delete = []
        for instance_id in instances_to_remove:
            del self.active_instances[instance_id]
            self._delete_instance(instance_id)
            to_delete.append(instance_id)
        
        logger.info(f"Cleaned up {len(to_delete)} completed process instances")
        return len(to_delete) 