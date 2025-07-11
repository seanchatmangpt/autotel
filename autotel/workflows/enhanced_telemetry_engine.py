#!/usr/bin/env python3
"""
Enhanced Telemetry Engine for SpiffWorkflow
Provides proper task-level telemetry integration with consistent workflow IDs
"""

import uuid
import threading
import logging
from typing import Dict, Any, Optional, List
from dataclasses import dataclass, field
from datetime import datetime

from opentelemetry import trace
from opentelemetry.trace import SpanContext, Status, StatusCode
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor

from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec

from ..core.telemetry import TelemetryManager, TelemetryConfig

logger = logging.getLogger(__name__)

@dataclass
class WorkflowExecutionContext:
    """Context for a single workflow execution with telemetry"""
    workflow_id: str
    process_id: str
    start_time: datetime
    telemetry_context: 'WorkflowTelemetryContext'
    workflow_instance: BpmnWorkflow
    execution_data: Dict[str, Any] = field(default_factory=dict)
    completed_tasks: List[str] = field(default_factory=list)
    failed_tasks: List[str] = field(default_factory=list)

class WorkflowTelemetryContext:
    """Enhanced context manager for workflow telemetry with consistent ID tracking"""
    
    def __init__(self, telemetry_manager: TelemetryManager):
        self.telemetry_manager = telemetry_manager
        self.active_workflows: Dict[str, WorkflowExecutionContext] = {}
        self._lock = threading.Lock()
    
    def start_workflow_execution(self, process_id: str, workflow_instance: BpmnWorkflow) -> WorkflowExecutionContext:
        """Start tracking a new workflow execution"""
        with self._lock:
            workflow_id = str(uuid.uuid4())
            start_time = datetime.now()
            
            # Create workflow span with attributes
            span_context = self.telemetry_manager.start_span(
                f"workflow.{process_id}", 
                "workflow_execution",
                workflow_id=workflow_id,
                workflow_process_id=process_id,
                workflow_type="bpmn",
                workflow_start_time=start_time.isoformat()
            )
            
            # Create execution context
            context = WorkflowExecutionContext(
                workflow_id=workflow_id,
                process_id=process_id,
                start_time=start_time,
                telemetry_context=self,
                workflow_instance=workflow_instance
            )
            
            self.active_workflows[workflow_id] = context
            return context
    
    def end_workflow_execution(self, workflow_id: str, success: bool = True, error: Optional[str] = None):
        """End tracking a workflow execution"""
        with self._lock:
            if workflow_id not in self.active_workflows:
                return
            
            context = self.active_workflows[workflow_id]
            end_time = datetime.now()
            duration = (end_time - context.start_time).total_seconds()
            
            # Record workflow completion metrics
            self.telemetry_manager.record_metric(
                "operation_duration_seconds", 
                duration,
                workflow_id=workflow_id,
                workflow_success=success,
                completed_tasks=len(context.completed_tasks),
                failed_tasks=len(context.failed_tasks)
            )
            
            # Remove from active workflows
            del self.active_workflows[workflow_id]
    
    def start_task_execution(self, workflow_id: str, task: BpmnTaskSpec, task_instance) -> trace.Span:
        """Start tracking a task execution"""
        with self._lock:
            if workflow_id not in self.active_workflows:
                # Create a standalone span if workflow context not found
                return self.telemetry_manager.start_span("task.unknown", "task_execution")
            
            context = self.active_workflows[workflow_id]
            
            # Create task span with attributes
            task_name = getattr(task, 'bpmn_name', task.name)
            task_type = self._determine_task_type(task)
            task_id = f"{workflow_id}-{task_name}-{len(context.completed_tasks) + len(context.failed_tasks) + 1}"
            
            # Prepare task attributes
            task_attributes = {
                "workflow_id": workflow_id,
                "task_id": task_id,
                "task_name": task_name,
                "task_type": task_type,
                "task_bpmn_id": getattr(task, 'bpmn_id', 'unknown'),
                "task_sequence": len(context.completed_tasks) + len(context.failed_tasks) + 1
            }
            
            # Add task input data
            if hasattr(task_instance, 'data') and task_instance.data:
                for key, value in task_instance.data.items():
                    task_attributes[f"task_input_{key}"] = str(value)
            
            span_context = self.telemetry_manager.start_span(
                f"{task_type}.{task_name}",
                "task_execution",
                **task_attributes
            )
            
            return span_context
    
    def end_task_execution(self, span_context, workflow_id: str, task_name: str, success: bool = True, 
                          error: Optional[str] = None, result: Optional[Any] = None):
        """End tracking a task execution"""
        with self._lock:
            if workflow_id in self.active_workflows:
                context = self.active_workflows[workflow_id]
                
                if success:
                    context.completed_tasks.append(task_name)
                else:
                    context.failed_tasks.append(task_name)
            
            # Record task completion metrics
            task_attributes = {
                "workflow_id": workflow_id,
                "task_name": task_name,
                "task_success": success
            }
            
            if error:
                task_attributes["task_error"] = error
            
            if result is not None:
                task_attributes["task_result"] = str(result)
            
            self.telemetry_manager.record_metric("linkml_operations_total", 1, **task_attributes)
    
    def _determine_task_type(self, task: BpmnTaskSpec) -> str:
        """Determine the type of task for telemetry categorization"""
        task_class = task.__class__.__name__.lower()
        
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

class EnhancedTelemetryBpmnEngine:
    """Enhanced BPMN engine with proper telemetry integration"""
    
    def __init__(self, telemetry_manager: Optional[TelemetryManager] = None):
        self.telemetry_manager = telemetry_manager or TelemetryManager(
            TelemetryConfig(require_linkml_validation=False)
        )
        self.telemetry_context = WorkflowTelemetryContext(self.telemetry_manager)
        self.parser = BpmnParser()
        self.active_workflows: Dict[str, WorkflowExecutionContext] = {}
    
    def add_bpmn_file(self, bpmn_path: str) -> None:
        """Add a BPMN file to the parser"""
        self.parser.add_bpmn_file(bpmn_path)
        logger.info(f"Added BPMN file: {bpmn_path}")
    
    def add_dmn_file(self, dmn_path: str) -> None:
        """Add a DMN file to the parser"""
        self.parser.add_dmn_file(dmn_path)
        logger.info(f"Added DMN file: {dmn_path}")
    
    def create_workflow(self, process_id: str, initial_data: Optional[Dict[str, Any]] = None) -> BpmnWorkflow:
        """Create a workflow instance with telemetry tracking"""
        specs = self.parser.find_all_specs()
        if process_id not in specs:
            raise ValueError(f"Process ID '{process_id}' not found")
        
        spec = specs[process_id]
        workflow = BpmnWorkflow(spec)
        
        if initial_data:
            workflow.set_data(**initial_data)
        
        # Start telemetry tracking
        execution_context = self.telemetry_context.start_workflow_execution(process_id, workflow)
        self.active_workflows[execution_context.workflow_id] = execution_context
        
        logger.info(f"Created workflow for process: {process_id} with ID: {execution_context.workflow_id}")
        return workflow
    
    def execute_workflow(self, workflow: BpmnWorkflow, run_until_user_input: bool = True) -> Dict[str, Any]:
        """Execute a workflow with enhanced telemetry"""
        # Find the execution context for this workflow
        execution_context = None
        for context in self.telemetry_context.active_workflows.values():
            if context.workflow_instance == workflow:
                execution_context = context
                break
        
        if not execution_context:
            logger.warning("No execution context found for workflow, creating standalone execution")
            return self._execute_workflow_standalone(workflow, run_until_user_input)
        
        try:
            return self._execute_workflow_with_telemetry(workflow, execution_context, run_until_user_input)
        except Exception as e:
            self.telemetry_context.end_workflow_execution(
                execution_context.workflow_id, 
                success=False, 
                error=str(e)
            )
            raise
    
    def _execute_workflow_with_telemetry(self, workflow: BpmnWorkflow, 
                                       execution_context: WorkflowExecutionContext,
                                       run_until_user_input: bool) -> Dict[str, Any]:
        """Execute workflow with full telemetry tracking"""
        workflow_id = execution_context.workflow_id
        
        try:
            if run_until_user_input:
                workflow.run_until_user_input_required()
            else:
                workflow.run_all()
            
            # Mark workflow as successful
            self.telemetry_context.end_workflow_execution(workflow_id, success=True)
            
            return dict(workflow.data)
            
        except Exception as e:
            self.telemetry_context.end_workflow_execution(workflow_id, success=False, error=str(e))
            raise
    
    def _execute_workflow_standalone(self, workflow: BpmnWorkflow, run_until_user_input: bool) -> Dict[str, Any]:
        """Execute workflow without telemetry context (fallback)"""
        if run_until_user_input:
            workflow.run_until_user_input_required()
        else:
            workflow.run_all()
        
        return dict(workflow.data)
    
    def execute_task_with_telemetry(self, task_instance, workflow_id: str) -> None:
        """Execute a single task with telemetry tracking"""
        task_spec = task_instance.task_spec
        
        # Start task span
        span_context = self.telemetry_context.start_task_execution(workflow_id, task_spec, task_instance)
        
        try:
            # Execute the task
            task_instance.run()
            
            # Get task result
            result = None
            if hasattr(task_instance, 'data') and task_instance.data:
                result = dict(task_instance.data)
            
            # End task span successfully
            self.telemetry_context.end_task_execution(
                span_context, workflow_id, task_spec.name, 
                success=True, result=result
            )
            
        except Exception as e:
            # End task span with error
            self.telemetry_context.end_task_execution(
                span_context, workflow_id, task_spec.name, 
                success=False, error=str(e)
            )
            raise
    
    def list_processes(self) -> Dict[str, str]:
        """List all available processes"""
        specs = self.parser.find_all_specs()
        return {process_id: spec.name for process_id, spec in specs.items()}
    
    def get_workflow_status(self, workflow_id: str) -> Optional[Dict[str, Any]]:
        """Get status of a workflow execution"""
        if workflow_id in self.telemetry_context.active_workflows:
            context = self.telemetry_context.active_workflows[workflow_id]
            return {
                "workflow_id": workflow_id,
                "process_id": context.process_id,
                "start_time": context.start_time.isoformat(),
                "completed_tasks": context.completed_tasks,
                "failed_tasks": context.failed_tasks,
                "is_completed": context.workflow_instance.is_completed()
            }
        return None

def create_enhanced_telemetry_engine(telemetry_manager: Optional[TelemetryManager] = None) -> EnhancedTelemetryBpmnEngine:
    """Factory function to create an enhanced telemetry BPMN engine"""
    return EnhancedTelemetryBpmnEngine(telemetry_manager) 