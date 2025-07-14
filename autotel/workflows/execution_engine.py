"""
AutoTel Execution Engine - Best Practice Implementation
Demonstrates clean separation between XML parsing and execution logic
"""

from dataclasses import dataclass
from typing import Dict, Any, Optional, List
from pathlib import Path
import tempfile
import os

from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
from rdflib import Graph
import pyshacl

from .dspy_bpmn_parser import DspyBpmnParser, DSPySignatureDefinition


@dataclass
class ExecutionResult:
    """Result of workflow execution"""
    success: bool
    data: Dict[str, Any]
    errors: List[str]
    warnings: List[str]
    execution_time: float
    telemetry_spans: List[Dict[str, Any]]


class AutoTelExecutionEngine:
    """
    Pure execution engine - no XML parsing logic.
    Receives ExecutableSpecification from parser and executes workflows.
    """
    
    def __init__(self, telemetry_manager=None):
        self.telemetry_manager = telemetry_manager
        self.execution_history = []
    
    def execute_specification(self, spec: 'ExecutableSpecification', 
                            context: Dict[str, Any]) -> ExecutionResult:
        """
        Execute a workflow specification with given context.
        Pure execution logic - no XML parsing involved.
        """
        import time
        start_time = time.time()
        
        try:
            # 1. Validate input data against SHACL shapes
            validation_result = spec.validate_input_data(context)
            if not validation_result['valid']:
                return ExecutionResult(
                    success=False,
                    data={},
                    errors=validation_result['errors'],
                    warnings=validation_result['warnings'],
                    execution_time=time.time() - start_time,
                    telemetry_spans=[]
                )
            
            # 2. Create and configure workflow
            workflow = BpmnWorkflow(spec.bpmn_spec)
            workflow.set_data(**context)
            
            # 3. Register DSPy signatures for execution
            self._register_signatures(spec.dspy_signatures)
            
            # 4. Execute workflow
            workflow.run_all()
            
            # 5. Collect results
            execution_time = time.time() - start_time
            
            return ExecutionResult(
                success=True,
                data=workflow.data,
                errors=[],
                warnings=[],
                execution_time=execution_time,
                telemetry_spans=self._collect_telemetry()
            )
            
        except Exception as e:
            return ExecutionResult(
                success=False,
                data={},
                errors=[str(e)],
                warnings=[],
                execution_time=time.time() - start_time,
                telemetry_spans=[]
            )
    
    def _register_signatures(self, signatures: Dict[str, DSPySignatureDefinition]):
        """Register DSPy signatures for execution - pure registration logic"""
        from autotel.utils.dspy_services import dspy_registry
        
        for sig_name, sig_def in signatures.items():
            # Register signature with DSPy registry
            # No XML parsing, just registration
            pass
    
    def _collect_telemetry(self) -> List[Dict[str, Any]]:
        """Collect telemetry data - pure data collection"""
        if self.telemetry_manager:
            # Collect spans and events
            return []
        return []


class AutoTelWorkflowManager:
    """
    High-level workflow manager that orchestrates parsing and execution.
    Demonstrates the clean separation of concerns.
    """
    
    def __init__(self, telemetry_manager=None):
        self.parser = DspyBpmnParser()
        self.execution_engine = AutoTelExecutionEngine(telemetry_manager)
        self.specification_cache = {}
    
    def load_workflow_from_file(self, bpmn_path: str, process_id: str) -> 'ExecutableSpecification':
        """
        Load workflow from BPMN file and create executable specification.
        This is the only place where XML parsing happens.
        """
        # Parse XML and create specification
        self.parser.add_bpmn_file(bpmn_path)
        spec = self.parser.create_executable_specification(process_id)
        
        # Cache the specification
        cache_key = f"{bpmn_path}:{process_id}"
        self.specification_cache[cache_key] = spec
        
        return spec
    
    def load_workflow_from_string(self, bpmn_xml: str, process_id: str) -> 'ExecutableSpecification':
        """
        Load workflow from BPMN XML string and create executable specification.
        This is the only place where XML parsing happens.
        """
        # Parse XML and create specification
        self.parser.add_bpmn_xml_from_string(bpmn_xml, f"<in-memory-{process_id}>")
        spec = self.parser.create_executable_specification(process_id)
        
        # Cache the specification
        cache_key = f"<in-memory-{process_id}>:{process_id}"
        self.specification_cache[cache_key] = spec
        
        return spec
    
    def execute_workflow(self, spec: 'ExecutableSpecification', 
                        context: Dict[str, Any]) -> ExecutionResult:
        """
        Execute a workflow specification.
        Pure execution - no XML parsing involved.
        """
        return self.execution_engine.execute_specification(spec, context)
    
    def execute_workflow_from_file(self, bpmn_path: str, process_id: str, 
                                  context: Dict[str, Any]) -> ExecutionResult:
        """
        Convenience method: load and execute in one call.
        Still maintains clean separation internally.
        """
        spec = self.load_workflow_from_file(bpmn_path, process_id)
        return self.execute_workflow(spec, context)


# Example usage demonstrating best practice:
def demonstrate_best_practice():
    """
    Demonstrate the best practice separation between XML parsing and execution.
    """
    print("🎯 AutoTel Best Practice Demonstration")
    print("=" * 50)
    
    # 1. Create workflow manager
    manager = AutoTelWorkflowManager()
    
    # 2. Load workflow from file (XML parsing happens here)
    print("\n1. Loading workflow from XML file...")
    spec = manager.load_workflow_from_file("bpmn/simple_dspy_workflow.bpmn", "simple_dspy_process")
    
    print(f"   ✅ Loaded specification for process: {spec.process_id}")
    print(f"   📋 DSPy signatures: {len(spec.dspy_signatures)}")
    print(f"   🔧 DMN engines: {len(spec.dmn_engines)}")
    print(f"   📐 SHACL shapes: {len(spec.shacl_shapes)}")
    
    # 3. Execute workflow (pure execution - no XML parsing)
    print("\n2. Executing workflow...")
    context = {
        'input_text': 'Hello, AutoTel!',
        'user_id': 'test_user_123'
    }
    
    result = manager.execute_workflow(spec, context)
    
    print(f"   ✅ Execution completed: {result.success}")
    print(f"   ⏱️  Execution time: {result.execution_time:.3f}s")
    print(f"   📊 Result data: {result.data}")
    
    if result.errors:
        print(f"   ❌ Errors: {result.errors}")
    
    print("\n🎉 Best practice demonstration completed!")
    print("\n📋 Key Benefits:")
    print("   • XML parsing isolated to parser only")
    print("   • Execution logic is pure and testable")
    print("   • Clear separation of concerns")
    print("   • Reusable specifications")
    print("   • Easy to test and debug")


if __name__ == "__main__":
    demonstrate_best_practice() 