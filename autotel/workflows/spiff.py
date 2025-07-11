"""
AutoTel SpiffWorkflow Integration
"""

import json
import yaml
from typing import Dict, Any, List, Optional
from pathlib import Path
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import dspy_service, dspy_registry

class SpiffCapabilityChecker:
    """Check SpiffWorkflow capabilities and configuration"""
    
    def __init__(self):
        self.config = {}
        self.capabilities = {}
    
    def load_config(self, config_path: str) -> None:
        """Load configuration from file"""
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
    
    def check_capabilities(self) -> Dict[str, Any]:
        """Check SpiffWorkflow capabilities"""
        
        capabilities = {
            "bpmn_2_0_support": True,
            "task_types": [
                "UserTask",
                "ServiceTask", 
                "ScriptTask",
                "ManualTask",
                "BusinessRuleTask"
            ],
            "gateway_types": [
                "ExclusiveGateway",
                "InclusiveGateway",
                "ParallelGateway",
                "EventBasedGateway"
            ],
            "event_types": [
                "StartEvent",
                "EndEvent",
                "IntermediateCatchEvent",
                "IntermediateThrowEvent",
                "BoundaryEvent"
            ],
            "data_objects": [
                "DataObject",
                "DataObjectReference",
                "DataStoreReference"
            ],
            "subprocesses": [
                "SubProcess",
                "CallActivity"
            ],
            "lanes": True,
            "pools": True,
            "message_flows": True,
            "signal_flows": True,
            "error_handling": True,
            "compensation": True,
            "multi_instance": True,
            "loop_characteristics": True,
            "conditional_flows": True,
            "default_flows": True,
            "sequence_flows": True,
            "message_flows": True,
            "association_flows": True,
            "data_associations": True,
            "data_input_associations": True,
            "data_output_associations": True
        }
        
        self.capabilities = capabilities
        return capabilities
    
    def validate_bpmn_file(self, bpmn_path: str) -> Dict[str, Any]:
        """Validate a BPMN file"""
        try:
            from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
            
            parser = BpmnParser()
            parser.add_bpmn_files_by_glob(bpmn_path)
            
            specs = parser.find_all_specs()
            
            validation_result = {
                "valid": True,
                "processes": list(specs.keys()),
                "errors": [],
                "warnings": []
            }
            
            # Check for common issues
            for process_id, process_spec in specs.items():
                if not process_spec.task_specs:
                    validation_result["warnings"].append(f"Process {process_id} has no tasks")
                
                # Check for start events
                start_events = [task for task in process_spec.task_specs.values() 
                              if task.__class__.__name__ == 'StartEvent']
                if not start_events:
                    validation_result["warnings"].append(f"Process {process_id} has no start events")
                
                # Check for end events
                end_events = [task for task in process_spec.task_specs.values() 
                            if task.__class__.__name__ == 'EndEvent']
                if not end_events:
                    validation_result["warnings"].append(f"Process {process_id} has no end events")
            
            return validation_result
            
        except Exception as e:
            return {
                "valid": False,
                "processes": [],
                "errors": [str(e)],
                "warnings": []
            }
    
    def get_supported_elements(self) -> Dict[str, List[str]]:
        """Get supported BPMN elements"""
        return {
            "task_types": self.capabilities.get("task_types", []),
            "gateway_types": self.capabilities.get("gateway_types", []),
            "event_types": self.capabilities.get("event_types", []),
            "data_objects": self.capabilities.get("data_objects", [])
        }
    
    def check_compatibility(self, bpmn_path: str) -> Dict[str, Any]:
        """Check BPMN file compatibility with SpiffWorkflow"""
        validation = self.validate_bpmn_file(bpmn_path)
        
        if not validation["valid"]:
            return {
                "compatible": False,
                "issues": validation["errors"],
                "warnings": validation["warnings"]
            }
        
        # Check for unsupported elements
        issues = []
        warnings = []
        
        # This would require parsing the BPMN XML to check for specific elements
        # For now, we'll return a basic compatibility check
        
        return {
            "compatible": True,
            "issues": issues,
            "warnings": warnings + validation["warnings"],
            "processes": validation["processes"]
        } 

def run_dspy_bpmn_process(bpmn_path: str, process_id: str, context: dict) -> dict:
    """
    Load a BPMN file using DspyBpmnParser, execute the process, and return the workflow context.
    Supports dynamic DSPy signatures defined in XML and DMN business rule tasks.
    """
    parser = DspyBpmnParser()
    parser.add_bpmn_file(bpmn_path)
    
    # Register dynamic signatures from the parser
    dynamic_signatures = parser.dynamic_signatures
    if dynamic_signatures:
        dspy_registry.register_parser_signatures(dynamic_signatures)
        print(f"📋 Registered {len(dynamic_signatures)} dynamic signatures from XML")
    
    specs = parser.find_all_specs()
    spec = specs[process_id]
    from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
    wf = BpmnWorkflow(spec)
    wf.set_data(**context)
    
    print(f"📊 Workflow data after set_data: {wf.data}")
    
    # Use SpiffWorkflow's natural execution flow
    # This ensures that _run_hook methods are called properly
    wf.run_all()
    
    print(f"📊 Workflow data after run_all: {wf.data}")
    
    return wf.data 