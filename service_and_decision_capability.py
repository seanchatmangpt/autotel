#!/usr/bin/env python3
"""
Enhanced capability test for BPMN tasks in SpiffWorkflow with proper telemetry integration.
Tests what actually works in the current implementation with advanced span management.
"""

from pathlib import Path
from autotel.core.telemetry import TelemetryManager, TelemetryConfig
from functools import wraps
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor
from opentelemetry.trace import SpanContext, TraceFlags
import xml.etree.ElementTree as ET
import uuid
import threading
from typing import Dict, Any, Optional

# Force OTEL tracer provider setup at script start
trace.set_tracer_provider(TracerProvider())
trace.get_tracer_provider().add_span_processor(BatchSpanProcessor(ConsoleSpanExporter()))

class WorkflowTelemetryContext:
    """Context manager for workflow telemetry with consistent ID tracking"""
    
    def __init__(self, telemetry_manager: TelemetryManager):
        self.telemetry_manager = telemetry_manager
        self.workflow_id = str(uuid.uuid4())
        self.workflow_span_context: Optional[SpanContext] = None
        self.task_counter = 0
        self._lock = threading.Lock()
    
    def start_workflow_span(self, workflow_name: str) -> trace.Span:
        """Start the main workflow span"""
        with self._lock:
            span_context = self.telemetry_manager.start_span(
                f"workflow.{workflow_name}", 
                "workflow_execution",
                workflow_id=self.workflow_id,
                workflow_name=workflow_name,
                workflow_type="bpmn"
            )
            return span_context
    
    def start_task_span(self, task_name: str, task_type: str = "task") -> trace.Span:
        """Start a task span as a child of the workflow span"""
        with self._lock:
            self.task_counter += 1
            task_id = f"{self.workflow_id}-task-{self.task_counter}"
            
            span_context = self.telemetry_manager.start_span(
                f"{task_type}.{task_name}", 
                "task_execution",
                workflow_id=self.workflow_id,
                task_id=task_id,
                task_name=task_name,
                task_type=task_type,
                task_sequence=self.task_counter
            )
            
            return span_context

def enhanced_otel_wrapper_factory(attr_value, name, telemetry_context: WorkflowTelemetryContext):
    """Enhanced wrapper factory that creates proper parent-child span relationships"""
    @wraps(attr_value)
    def otel_wrapped(self, *args, **kwargs):
        if not hasattr(self, "telemetry_context"):
            self.telemetry_context = telemetry_context
        
        # Create parent workflow span for run_all
        if attr_value.__name__ == "run_all":
            print(f"[OTEL] Starting workflow span: {name}.{attr_value.__name__}")
            with self.telemetry_context.start_workflow_span(name) as workflow_span:
                workflow_span.add_event("Workflow execution started")
                result = attr_value(self, *args, **kwargs)
                workflow_span.add_event("Workflow execution completed")
                workflow_span.set_attribute("workflow.completed", True)
                print(f"[OTEL] Finished workflow span: {name}.{attr_value.__name__}")
                return result
        else:
            # For other methods, create child spans under the workflow span
            task_name = attr_value.__name__
            task_type = "task"
            
            # Determine task type based on method name
            if "task" in task_name.lower():
                task_type = "bpmn_task"
            elif "service" in task_name.lower():
                task_type = "service_task"
            elif "decision" in task_name.lower():
                task_type = "decision_task"
            
            print(f"[OTEL] Starting task span: {name}.{task_name}")
            with self.telemetry_context.start_task_span(task_name, task_type) as task_span:
                task_span.add_event("Task execution started")
                result = attr_value(self, *args, **kwargs)
                task_span.add_event("Task execution completed")
                task_span.set_attribute("task.success", True)
                print(f"[OTEL] Finished task span: {name}.{task_name}")
                return result
    return otel_wrapped

# --- Enhanced OtelAutoMeta metaclass ---
class EnhancedOtelAutoMeta(type):
    """Enhanced metaclass that creates proper workflow-task span hierarchy"""
    
    def __new__(mcs, name, bases, namespace):
        # Create telemetry context for this workflow class
        telemetry_manager = TelemetryManager(TelemetryConfig(require_linkml_validation=False))
        telemetry_context = WorkflowTelemetryContext(telemetry_manager)
        
        # Wrap all callable methods
        for attr_name, attr_value in list(namespace.items()):
            if callable(attr_value) and not attr_name.startswith("_"):
                namespace[attr_name] = enhanced_otel_wrapper_factory(attr_value, name, telemetry_context)
        
        cls = super().__new__(mcs, name, bases, namespace)
        
        # Ensure run_all is properly wrapped
        if hasattr(cls, "run_all"):
            orig = getattr(cls, "run_all")
            if not getattr(orig, "__wrapped__", False):
                setattr(cls, "run_all", enhanced_otel_wrapper_factory(orig, name, telemetry_context))
        
        return cls

def get_first_process_id(bpmn_file):
    tree = ET.parse(bpmn_file)
    root = tree.getroot()
    ns = {'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'}
    process = root.find('bpmn:process', ns)
    if process is not None:
        return process.attrib['id']
    return None

def test_enhanced_spiff_capability():
    """Test enhanced telemetry integration with SpiffWorkflow"""
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print("[ERROR] BPMN file not found.")
        return False

    from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
    from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser

    class EnhancedOtelBpmnWorkflow(BpmnWorkflow, metaclass=EnhancedOtelAutoMeta):
        """Enhanced BPMN workflow with proper telemetry integration"""
        pass

    parser = BpmnParser()
    parser.add_bpmn_file(bpmn_file)
    specs = parser.find_all_specs()
    process_id = get_first_process_id(bpmn_file)
    if not process_id or process_id not in specs:
        print(f"[ERROR] Process id '{process_id}' not found in specs.")
        return False
    
    workflow_spec = specs[process_id]
    workflow = EnhancedOtelBpmnWorkflow(workflow_spec)
    
    print("[TEST] About to run enhanced workflow.run_all()...")
    workflow.run_all()
    print("[TEST] Enhanced run_all() complete.")
    return workflow.is_completed()

def test_task_level_telemetry():
    """Test individual task execution with telemetry"""
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print("[ERROR] BPMN file not found.")
        return False

    from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
    from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
    from SpiffWorkflow.util.task import TaskState

    class TaskLevelOtelBpmnWorkflow(BpmnWorkflow, metaclass=EnhancedOtelAutoMeta):
        """BPMN workflow with task-level telemetry"""
        pass

    parser = BpmnParser()
    parser.add_bpmn_file(bpmn_file)
    specs = parser.find_all_specs()
    process_id = get_first_process_id(bpmn_file)
    if not process_id or process_id not in specs:
        print(f"[ERROR] Process id '{process_id}' not found in specs.")
        return False
    
    workflow_spec = specs[process_id]
    workflow = TaskLevelOtelBpmnWorkflow(workflow_spec)
    
    print("[TEST] Testing task-level telemetry...")
    
    # Execute workflow step by step to see individual task spans
    while not workflow.is_completed():
        ready_tasks = workflow.get_tasks(state=TaskState.READY)
        if not ready_tasks:
            break
        
        for task in ready_tasks:
            print(f"[TEST] Executing task: {task.task_spec.name}")
            task.run()
            print(f"[TEST] Task completed: {task.task_spec.name}")
    
    print("[TEST] Task-level telemetry test complete.")
    return workflow.is_completed()

if __name__ == "__main__":
    print("🚀 Testing Enhanced SpiffWorkflow Telemetry Integration")
    print("=" * 60)
    
    # Test 1: Enhanced workflow execution
    print("\n📊 Test 1: Enhanced Workflow Execution")
    print("-" * 40)
    success1 = test_enhanced_spiff_capability()
    print(f"[RESULT] Enhanced workflow completed: {success1}")
    
    # Test 2: Task-level telemetry
    print("\n📊 Test 2: Task-Level Telemetry")
    print("-" * 40)
    success2 = test_task_level_telemetry()
    print(f"[RESULT] Task-level telemetry completed: {success2}")
    
    overall_success = success1 and success2
    print(f"\n[FINAL RESULT] All tests passed: {overall_success}")
    exit(0 if overall_success else 1) 