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
from .telemetry_bpmn_parser import TelemetryBpmnParser

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

class EnhancedTelemetryBpmnEngine:
    """Enhanced BPMN engine with proper telemetry integration"""
    
    def __init__(self, telemetry_manager: Optional[TelemetryManager] = None):
        self.telemetry_manager = telemetry_manager or TelemetryManager(
            TelemetryConfig(require_linkml_validation=False)
        )
        self.telemetry_context = WorkflowTelemetryContext(self.telemetry_manager)
        self.parser = TelemetryBpmnParser(self.telemetry_context)
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
        
        # Start telemetry tracking
        execution_context = self.telemetry_context.start_workflow_execution(process_id, spec)
        self.active_workflows[execution_context.workflow_id] = execution_context
        
        # Inject the workflow_id into each task spec
        for task_spec in spec.get_tasks():
            task_spec.workflow_id = execution_context.workflow_id

        workflow = BpmnWorkflow(spec)
        
        if initial_data:
            workflow.set_data(**initial_data)
        
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