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
logging.basicConfig(level=logging.DEBUG)
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
from autotel.utils.dspy_services import dspy_registry

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
                 process_definitions: Optional[Dict[str, BpmnWorkflow]] = None,
                 telemetry_manager: Optional[Any] = None,
                 enable_persistence: bool = True,
                 dspy_config: Optional[Any] = None):
        """
        Initialize the orchestrator with process definitions and telemetry
        
        Args:
            process_definitions: Linked process definitions/specs
            telemetry_manager: Telemetry manager instance
            enable_persistence: Whether to enable process persistence
            dspy_config: DSPy model configuration
        """
        self.telemetry_manager = telemetry_manager
        self.enable_persistence = enable_persistence
        self.dspy_config = dspy_config
        
        # Initialize telemetry
        if telemetry_manager:
            self.tracer = telemetry_manager.tracer
        else:
            self.tracer = None
        
        # Use provided process definitions
        self.process_definitions: Dict[str, BpmnWorkflow] = process_definitions or {}
        self.active_instances: Dict[str, ProcessInstance] = {}
        # No file loading here; pipeline handles parsing and linking
        # logger.info(f"Orchestrator initialized with {len(self.process_definitions)} process definitions")
    
    def _load_persisted_instances(self) -> None:
        """Load persisted process instances from storage"""
        try:
            persisted_instances = persistence.load_instances()
            for instance_data in persisted_instances:
                # Reconstruct process instance from persisted data
                # This is a simplified version - in practice, you'd need more robust deserialization
                pass
        except Exception as e:
            logger.warning(f"Failed to load persisted instances: {e}")
    
    def _persist_instance(self, instance: ProcessInstance):
        """Persist a process instance to storage"""
        if self.enable_persistence:
            try:
                persistence.save_instance(instance.to_dict())
            except Exception as e:
                logger.error(f"Failed to persist instance {instance.instance_id}: {e}")
    
    def _delete_instance(self, instance_id: str):
        """Delete a persisted process instance"""
        if self.enable_persistence:
            try:
                persistence.delete_instance(instance_id)
            except Exception as e:
                logger.error(f"Failed to delete instance {instance_id}: {e}")
    
    def start_process(self, 
                     process_id: str, 
                     variables: Optional[Dict[str, Any]] = None,
                     instance_id: Optional[str] = None) -> ProcessInstance:
        """
        Start a new process instance
        
        Args:
            process_id: ID of the process to start
            variables: Initial process variables
            instance_id: Optional instance ID (auto-generated if not provided)
            
        Returns:
            ProcessInstance: The started process instance
            
        Raises:
            ValueError: If process_id is not found
        """
        with self.tracer.start_as_current_span("start_process", 
                                             attributes={"process_id": process_id}) as span:
            try:
                if process_id not in self.process_definitions:
                    raise ValueError(f"Process '{process_id}' not found")
                
                # Generate instance ID if not provided
                if not instance_id:
                    instance_id = str(uuid.uuid4())
                
                # Create workflow instance
                workflow = BpmnWorkflow(self.process_definitions[process_id])
                
                # Set initial variables
                if variables:
                    workflow.data.update(variables)
                
                # Create process instance
                instance = ProcessInstance(
                    instance_id=instance_id,
                    process_definition_id=process_id,
                    workflow=workflow,
                    start_time=datetime.now(),
                    status=ProcessStatus.RUNNING,
                    variables=variables or {}
                )
                
                # Store instance
                self.active_instances[instance_id] = instance
                
                # Persist instance
                self._persist_instance(instance)
                
                # Update metrics
                process_counter.add(1, {"process_id": process_id})
                
                span.set_attributes({
                    "instance_id": instance_id,
                    "variables_count": len(variables or {})
                })
                
                logger.info(f"Started process '{process_id}' with instance '{instance_id}'")
                return instance
                
            except Exception as e:
                span.record_exception(e)
                error_counter.add(1, {"process_id": process_id, "error_type": "start_failed"})
                raise
    
    def execute_process(self, instance_id: str, max_steps: int = 100) -> ProcessInstance:
        """
        Execute a process instance until completion or max steps
        
        Args:
            instance_id: ID of the process instance to execute
            max_steps: Maximum number of execution steps
            
        Returns:
            ProcessInstance: The updated process instance
            
        Raises:
            ValueError: If instance_id is not found
        """
        with self.tracer.start_as_current_span("execute_process", 
                                             attributes={"instance_id": instance_id}) as span:
            try:
                if instance_id not in self.active_instances:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                instance = self.active_instances[instance_id]
                workflow = instance.workflow
                
                # Execute workflow
                steps_executed = 0
                while not workflow.is_completed() and steps_executed < max_steps:
                    # Get ready tasks
                    ready_tasks = workflow.get_tasks(state=TaskState.READY)
                    
                    if not ready_tasks:
                        # No ready tasks - workflow may be stuck
                        instance.status = ProcessStatus.SUSPENDED
                        break
                    
                    # Execute each ready task
                    for task in ready_tasks:
                        print(f"[DEBUG] Executing task: {task.task_spec.name} (type: {getattr(task.task_spec, 'type_', type(task.task_spec).__name__)})")  # TEMP DEBUG
                        try:
                            self._execute_task(task, instance)
                            steps_executed += 1
                        except Exception as e:
                            logger.error(f"Error executing task {task.id}: {e}")
                            instance.status = ProcessStatus.FAILED
                            instance.error_message = str(e)
                            span.record_exception(e)
                            raise
                
                # Update instance status
                if workflow.is_completed():
                    instance.status = ProcessStatus.COMPLETED
                    instance.end_time = datetime.now()
                elif instance.status == ProcessStatus.RUNNING:
                    instance.status = ProcessStatus.SUSPENDED
                
                # Update execution path
                instance.execution_path = self._get_execution_path(workflow)
                
                # Persist updated instance
                self._persist_instance(instance)
                
                span.set_attributes({
                    "steps_executed": steps_executed,
                    "final_status": instance.status.value
                })
                
                logger.info(f"Executed process instance '{instance_id}': {steps_executed} steps, status: {instance.status.value}")
                return instance
                
            except Exception as e:
                span.record_exception(e)
                error_counter.add(1, {"instance_id": instance_id, "error_type": "execution_failed"})
                raise
    
    def _execute_task(self, task: BpmnTaskSpec, process_instance: ProcessInstance) -> None:
        """
        Execute a single task with telemetry
        
        Args:
            task: The task to execute
            process_instance: The process instance containing the task
        """
        task_start_time = datetime.now()
        
        with self.tracer.start_as_current_span(f"execute_task.{task.id}", 
                                             attributes={
                                                 "task_id": task.id,
                                                 "task_name": task.task_spec.bpmn_name or task.task_spec.name,
                                                 "task_type": task.__class__.__name__
                                             }) as span:
            try:
                # Debug: print extensions for service tasks
                if getattr(task.task_spec, 'type_', None) == 'ServiceTask':
                    print(f"[DEBUG] ServiceTask extensions: {getattr(task.task_spec, 'extensions', None)}")
                
                # Check if this is a DSPy service task
                if hasattr(task, 'extensions') and task.extensions:
                    dspy_info = task.extensions.get('dspy_service')
                    if dspy_info:
                        self._execute_dspy_service(task, process_instance, dspy_info)
                        return
                
                # Execute standard task
                task.run()
                
                # Record task completion
                task_end_time = datetime.now()
                duration = (task_end_time - task_start_time).total_seconds()
                
                task_duration_histogram.record(duration, {
                    "task_id": task.id,
                    "task_type": task.__class__.__name__
                })
                
                span.set_attributes({
                    "duration_seconds": duration,
                    "task_status": "completed"
                })
                
            except Exception as e:
                span.record_exception(e)
                span.set_attributes({"task_status": "failed"})
                raise
    
    def _execute_dspy_service(self, task: BpmnTaskSpec, process_instance: ProcessInstance, dspy_info: dict) -> None:
        """
        Execute a DSPy service task with telemetry
        
        Args:
            task: The DSPy service task
            process_instance: The process instance
            dspy_info: DSPy configuration from task extensions
        """
        with self.tracer.start_as_current_span(f"execute_dspy_service.{task.id}",
                                             attributes={
                                                 "dspy_signature": dspy_info.get('signature'),
                                                 "dspy_module_type": dspy_info.get('module_type', 'predict')
                                             }) as span:
            try:
                import dspy
                # Get signature definition from injected info
                sig_def = dspy_info.get('executable_dspy_signature')
                if not sig_def:
                    raise ValueError("No executable_dspy_signature injected into task spec.")
                # Use unified registry to create signature class
                signature_class = dspy_registry.create_dynamic_signature(
                    sig_def['name'],
                    sig_def['inputs'],
                    sig_def['outputs'],
                    sig_def.get('description', '')
                )
                # Instantiate DSPy module
                module_type = dspy_info.get('module_type', 'predict')
                if module_type == 'chain-of-thought':
                    dspy_module = dspy.ChainOfThought(signature_class)
                else:
                    dspy_module = dspy.Predict(signature_class)
                
                # Configure DSPy module with LLM model if provided
                if self.dspy_config and 'llm_model' in self.dspy_config:
                    dspy_module.llm_model = self.dspy_config['llm_model']
                
                # Prepare inputs
                inputs = {}
                for param in sig_def['inputs']:
                    if param in process_instance.variables:
                        inputs[param] = process_instance.variables[param]
                # Run module
                result = dspy_module(**inputs)
                print(f"[DEBUG] DSPy module raw result: {result}")  # TEMP DEBUG
                # Unpack outputs: always write all fields to workflow variables
                if hasattr(result, '_store') and isinstance(result._store, dict):
                    for k, v in result._store.items():
                        process_instance.variables[k] = v
                elif isinstance(result, dict):
                    for k, v in result.items():
                        process_instance.variables[k] = v
                else:
                    for k in getattr(result, '__dict__', {}):
                        if not k.startswith('_'):
                            process_instance.variables[k] = getattr(result, k)
                # Always write main result under result_var if specified
                result_var = dspy_info.get('result_variable')
                if result_var:
                    process_instance.variables[result_var] = result
            except Exception as e:
                span.record_exception(e)
                raise
    
    def _get_execution_path(self, workflow: BpmnWorkflow) -> List[str]:
        """Get the execution path of a workflow"""
        path = []
        for task in workflow.get_tasks():
            if task.state == TaskState.COMPLETED:
                task_name = getattr(task.task_spec, 'bpmn_name', None) or getattr(task.task_spec, 'name', '')
                path.append(f"{task.id}:{task_name}")
        return path
    
    def get_process_instance(self, instance_id: str) -> Optional[ProcessInstance]:
        """Get a process instance by ID"""
        return self.active_instances.get(instance_id)
    
    def list_process_instances(self, 
                             status: Optional[ProcessStatus] = None,
                             process_id: Optional[str] = None) -> List[ProcessInstance]:
        """
        List process instances with optional filtering
        
        Args:
            status: Filter by process status
            process_id: Filter by process definition ID
            
        Returns:
            List[ProcessInstance]: Filtered list of process instances
        """
        instances = list(self.active_instances.values())
        
        if status:
            instances = [i for i in instances if i.status == status]
        
        if process_id:
            instances = [i for i in instances if i.process_definition_id == process_id]
        
        return instances
    
    def cancel_process(self, instance_id: str) -> ProcessInstance:
        """
        Cancel a running process instance
        
        Args:
            instance_id: ID of the process instance to cancel
            
        Returns:
            ProcessInstance: The cancelled process instance
        """
        with self.tracer.start_as_current_span("cancel_process", 
                                             attributes={"instance_id": instance_id}) as span:
            try:
                if instance_id not in self.active_instances:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                instance = self.active_instances[instance_id]
                
                if instance.status not in [ProcessStatus.RUNNING, ProcessStatus.SUSPENDED]:
                    raise ValueError(f"Cannot cancel process in status: {instance.status}")
                
                # Cancel the workflow
                instance.workflow.cancel()
                
                # Update instance status
                instance.status = ProcessStatus.CANCELLED
                instance.end_time = datetime.now()
                
                # Persist updated instance
                self._persist_instance(instance)
                
                span.set_attributes({"final_status": instance.status.value})
                
                logger.info(f"Cancelled process instance '{instance_id}'")
                return instance
                
            except Exception as e:
                span.record_exception(e)
                raise
    
    def get_process_variables(self, instance_id: str) -> Dict[str, Any]:
        """Get process variables for an instance"""
        instance = self.get_process_instance(instance_id)
        if not instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        return instance.variables.copy()
    
    def set_process_variables(self, instance_id: str, variables: Dict[str, Any]) -> None:
        """Set process variables for an instance"""
        instance = self.get_process_instance(instance_id)
        if not instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        instance.variables.update(variables)
        instance.workflow.data.update(variables)
        self._persist_instance(instance)
    
    def get_ready_tasks(self, instance_id: str) -> List[Dict[str, Any]]:
        """
        Get ready tasks for a process instance
        
        Args:
            instance_id: ID of the process instance
            
        Returns:
            List[Dict[str, Any]]: List of ready tasks with metadata
        """
        instance = self.get_process_instance(instance_id)
        if not instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        ready_tasks = instance.workflow.get_tasks(state=TaskState.READY)
        
        return [{
            "task_id": task.id,
            "task_name": task.task_spec.bpmn_name or task.task_spec.name,
            "task_type": task.__class__.__name__,
            "process_id": instance.process_definition_id,
            "instance_id": instance_id
        } for task in ready_tasks]
    
    def complete_task(self, instance_id: str, task_id: str, data: Optional[Dict[str, Any]] = None) -> None:
        """
        Complete a user task with data
        
        Args:
            instance_id: ID of the process instance
            task_id: ID of the task to complete
            data: Optional data to pass to the task
        """
        with self.tracer.start_as_current_span("complete_task", 
                                             attributes={"instance_id": instance_id, "task_id": task_id}) as span:
            try:
                instance = self.get_process_instance(instance_id)
                if not instance:
                    raise ValueError(f"Process instance '{instance_id}' not found")
                
                # Find the task
                task = None
                for t in instance.workflow.get_tasks():
                    if t.id == task_id:
                        task = t
                        break
                
                if not task:
                    raise ValueError(f"Task '{task_id}' not found in instance '{instance_id}'")
                
                # Complete the task
                if data:
                    task.data.update(data)
                    instance.variables.update(data)
                
                task.complete()
                
                # Persist updated instance
                self._persist_instance(instance)
                
                span.add_event("task_completed", {"data": str(data) if data else "none"})
                
                logger.info(f"Completed task '{task_id}' in instance '{instance_id}'")
                
            except Exception as e:
                span.record_exception(e)
                raise
    
    def serialize_workflow(self, instance_id: str) -> str:
        """Serialize a workflow instance to JSON"""
        instance = self.get_process_instance(instance_id)
        if not instance:
            raise ValueError(f"Process instance '{instance_id}' not found")
        
        serializer = BpmnWorkflowSerializer(DEFAULT_CONFIG)
        return serializer.serialize_workflow(instance.workflow)
    
    def deserialize_workflow(self, instance_id: str, serialized_data: str) -> ProcessInstance:
        """Deserialize a workflow instance from JSON"""
        serializer = BpmnWorkflowSerializer(DEFAULT_CONFIG)
        workflow = serializer.deserialize_workflow(serialized_data)
        
        instance = ProcessInstance(
            instance_id=instance_id,
            process_definition_id=workflow.spec.name,
            workflow=workflow,
            start_time=datetime.now(),
            status=ProcessStatus.RUNNING
        )
        
        self.active_instances[instance_id] = instance
        return instance
    
    def get_process_statistics(self) -> Dict[str, Any]:
        """Get process execution statistics"""
        total_instances = len(self.active_instances)
        status_counts = {}
        
        for status in ProcessStatus:
            status_counts[status.value] = 0
        
        for instance in self.active_instances.values():
            status_counts[instance.status.value] += 1
        
        return {
            "total_instances": total_instances,
            "status_counts": status_counts,
            "process_definitions": len(self.process_definitions)
        }
    
    def cleanup_completed_processes(self, max_age_hours: int = 24) -> int:
        """
        Clean up completed process instances older than max_age_hours
        
        Args:
            max_age_hours: Maximum age in hours for completed processes
            
        Returns:
            int: Number of processes cleaned up
        """
        cutoff_time = datetime.now() - timedelta(hours=max_age_hours)
        cleaned_count = 0
        
        for instance_id, instance in list(self.active_instances.items()):
            if (instance.status in [ProcessStatus.COMPLETED, ProcessStatus.CANCELLED] and 
                instance.end_time and instance.end_time < cutoff_time):
                
                # Remove from active instances
                del self.active_instances[instance_id]
                
                # Delete from persistence
                self._delete_instance(instance_id)
                
                cleaned_count += 1
        
        logger.info(f"Cleaned up {cleaned_count} completed process instances")
        return cleaned_count
    
    def export_telemetry(self) -> Dict[str, Any]:
        """Export telemetry data for analysis"""
        return {
            "process_instances": [instance.to_dict() for instance in self.active_instances.values()],
            "process_definitions": list(self.process_definitions.keys()),
            "statistics": self.get_process_statistics()
        } 