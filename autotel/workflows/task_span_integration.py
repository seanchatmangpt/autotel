#!/usr/bin/env python3
"""
Task-Span Integration - Complete integration of tasks into spans with consistent workflow IDs
"""

import uuid
import threading
import logging
from typing import Dict, Any, Optional, List
from dataclasses import dataclass, field
from datetime import datetime
from contextlib import contextmanager

from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode, SpanContext
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor

from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
from SpiffWorkflow.util.task import TaskState

from ..core.telemetry import TelemetryManager, TelemetryConfig

logger = logging.getLogger(__name__)

@dataclass
class TaskSpanContext:
    """Context for a single task execution with telemetry"""
    task_id: str
    workflow_id: str
    task_name: str
    task_type: str
    start_time: datetime
    span_context: Optional[SpanContext] = None
    parent_span_context: Optional[SpanContext] = None
    input_data: Dict[str, Any] = field(default_factory=dict)
    output_data: Dict[str, Any] = field(default_factory=dict)
    error_message: Optional[str] = None
    duration_seconds: Optional[float] = None

class TaskSpanManager:
    """Manages task-span integration with consistent workflow IDs"""
    
    def __init__(self, telemetry_manager: TelemetryManager):
        self.telemetry_manager = telemetry_manager
        self.active_tasks: Dict[str, TaskSpanContext] = {}
        self.workflow_tasks: Dict[str, List[str]] = {}  # workflow_id -> [task_ids]
        self._lock = threading.Lock()
    
    def start_task_span(self, 
                       workflow_id: str, 
                       task_spec: BpmnTaskSpec, 
                       task_instance,
                       parent_span_context: Optional[SpanContext] = None) -> TaskSpanContext:
        """Start a task span with proper parent-child relationship"""
        with self._lock:
            # Generate consistent task ID
            task_id = f"{workflow_id}-{task_spec.name}-{uuid.uuid4().hex[:8]}"
            
            # Determine task type
            task_type = self._determine_task_type(task_spec)
            
            # Create task context
            task_context = TaskSpanContext(
                task_id=task_id,
                workflow_id=workflow_id,
                task_name=task_spec.name,
                task_type=task_type,
                start_time=datetime.now(),
                parent_span_context=parent_span_context
            )
            
            # Create span with proper attributes
            span_attributes = {
                "workflow_id": workflow_id,
                "task_id": task_id,
                "task_name": task_spec.name,
                "task_type": task_type,
                "task_bpmn_id": getattr(task_spec, 'bpmn_id', 'unknown'),
                "task_class": task_spec.__class__.__name__
            }
            
            # Add task input data
            if hasattr(task_instance, 'data') and task_instance.data:
                for key, value in task_instance.data.items():
                    span_attributes[f"task_input_{key}"] = str(value)
                    task_context.input_data[key] = value
            
            # Create span
            span_context = self.telemetry_manager.start_span(
                f"{task_type}.{task_spec.name}",
                "task_execution",
                **span_attributes
            )
            
            task_context.span_context = span_context
            
            # Track active task
            self.active_tasks[task_id] = task_context
            
            # Track workflow-task relationship
            if workflow_id not in self.workflow_tasks:
                self.workflow_tasks[workflow_id] = []
            self.workflow_tasks[workflow_id].append(task_id)
            
            logger.info(f"Started task span: {task_id} ({task_spec.name})")
            return task_context
    
    def end_task_span(self, 
                     task_context: TaskSpanContext, 
                     success: bool = True, 
                     output_data: Optional[Dict[str, Any]] = None,
                     error_message: Optional[str] = None) -> None:
        """End a task span with results"""
        with self._lock:
            end_time = datetime.now()
            duration = (end_time - task_context.start_time).total_seconds()
            
            # Update task context
            task_context.duration_seconds = duration
            task_context.output_data = output_data or {}
            task_context.error_message = error_message
            
            # Get current span and update it
            current_span = trace.get_current_span()
            if current_span:
                # Add output data attributes
                for key, value in task_context.output_data.items():
                    current_span.set_attribute(f"task_output_{key}", str(value))
                
                # Add duration
                current_span.set_attribute("task_duration_seconds", duration)
                
                # Set status
                if success:
                    current_span.set_status(Status(StatusCode.OK))
                    current_span.add_event("Task execution completed successfully")
                else:
                    current_span.set_status(Status(StatusCode.ERROR, error_message or "Task failed"))
                    current_span.add_event("Task execution failed", {"error": error_message})
            
            # Remove from active tasks
            if task_context.task_id in self.active_tasks:
                del self.active_tasks[task_context.task_id]
            
            # Remove from workflow tracking
            if task_context.workflow_id in self.workflow_tasks:
                if task_context.task_id in self.workflow_tasks[task_context.workflow_id]:
                    self.workflow_tasks[task_context.workflow_id].remove(task_context.task_id)
            
            logger.info(f"Ended task span: {task_context.task_id} (duration: {duration:.3f}s)")
    
    def get_workflow_task_summary(self, workflow_id: str) -> Dict[str, Any]:
        """Get summary of all tasks for a workflow"""
        with self._lock:
            task_ids = self.workflow_tasks.get(workflow_id, [])
            active_tasks = [self.active_tasks.get(tid) for tid in task_ids if tid in self.active_tasks]
            
            return {
                "workflow_id": workflow_id,
                "total_tasks": len(task_ids),
                "active_tasks": len(active_tasks),
                "task_details": [
                    {
                        "task_id": task.task_id,
                        "task_name": task.task_name,
                        "task_type": task.task_type,
                        "duration": task.duration_seconds,
                        "status": "completed" if task.duration_seconds else "active"
                    }
                    for task in active_tasks
                ]
            }
    
    def _determine_task_type(self, task_spec: BpmnTaskSpec) -> str:
        """Determine the type of task for telemetry categorization"""
        task_class = task_spec.__class__.__name__.lower()
        
        if 'service' in task_class:
            return 'service_task'
        elif 'decision' in task_class or 'businessrule' in task_class:
            return 'decision_task'
        elif 'user' in task_class:
            return 'user_task'
        elif 'manual' in task_class:
            return 'manual_task'
        elif 'script' in task_class:
            return 'script_task'
        else:
            return 'bpmn_task'

class TaskSpanMetaclass(type):
    """Metaclass that adds telemetry to task execution"""
    
    def __new__(mcs, name, bases, namespace, **kwargs):
        cls = super().__new__(mcs, name, bases, namespace, **kwargs)
        
        # Add telemetry to task execution methods
        if hasattr(cls, '_run_hook'):
            original_run_hook = cls._run_hook
            
            def telemetry_run_hook(self, my_task):
                # Get task span manager from workflow
                task_span_manager = getattr(my_task.workflow, 'task_span_manager', None)
                
                if task_span_manager:
                    # Start task span
                    task_context = task_span_manager.start_task_span(
                        workflow_id=getattr(my_task.workflow, 'workflow_id', 'unknown'),
                        task_spec=my_task.task_spec,
                        task_instance=my_task,
                        parent_span_context=trace.get_current_span().get_span_context() if trace.get_current_span() else None
                    )
                    
                    try:
                        # Execute original method
                        result = original_run_hook(self, my_task)
                        
                        # Get output data
                        output_data = {}
                        if hasattr(my_task, 'data') and my_task.data:
                            output_data = dict(my_task.data)
                        
                        # End task span successfully
                        task_span_manager.end_task_span(
                            task_context=task_context,
                            success=True,
                            output_data=output_data
                        )
                        
                        return result
                        
                    except Exception as e:
                        # End task span with error
                        task_span_manager.end_task_span(
                            task_context=task_context,
                            success=False,
                            error_message=str(e)
                        )
                        raise
                else:
                    # Fallback to original method if no telemetry manager
                    return original_run_hook(self, my_task)
            
            cls._run_hook = telemetry_run_hook
        
        return cls

def create_task_span_manager(telemetry_manager: TelemetryManager) -> TaskSpanManager:
    """Factory function to create a task span manager"""
    return TaskSpanManager(telemetry_manager)

# Global task span manager instance
task_span_manager = None

def initialize_task_span_manager(telemetry_manager: TelemetryManager) -> None:
    """Initialize the global task span manager"""
    global task_span_manager
    task_span_manager = create_task_span_manager(telemetry_manager)
    logger.info("Task span manager initialized")

def get_task_span_manager() -> Optional[TaskSpanManager]:
    """Get the global task span manager"""
    return task_span_manager 