"""
AutoTel Camunda-style Integration
Follows the spiff-example-cli Camunda implementation patterns
for DSPy + DMN integration with BPMN workflows.
"""

import logging
import datetime
from typing import Dict, Any, Type

from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.camunda.specs import UserTask
from SpiffWorkflow.bpmn.specs.defaults import ManualTask, ServiceTask
from SpiffWorkflow.bpmn.specs.mixins.none_task import NoneTask
from SpiffWorkflow.bpmn.script_engine import TaskDataEnvironment
from SpiffWorkflow.bpmn.parser.TaskParser import TaskParser
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask as BaseServiceTask

from ..core.telemetry import TelemetryManager
from ..schemas.validation import SchemaValidator
from ..utils.dspy_services import dspy_registry
from .dspy_bpmn_parser import DspyBpmnParser

logger = logging.getLogger('autotel_engine')
logger.setLevel(logging.INFO)

spiff_logger = logging.getLogger('spiff')
spiff_logger.setLevel(logging.INFO)


class AutoTelServiceTask(BaseServiceTask):
    """Service task that can execute DSPy services with telemetry"""
    
    def __init__(self, wf_spec, name, **kwargs):
        super().__init__(wf_spec, name, **kwargs)
        self.telemetry_manager = None
        self.schema_validator = None
    
    def set_telemetry(self, telemetry_manager: TelemetryManager, schema_validator: SchemaValidator):
        """Set telemetry and validation components"""
        self.telemetry_manager = telemetry_manager
        self.schema_validator = schema_validator
    
    def _run_hook(self, my_task):
        """Override to handle DSPy service execution with telemetry"""
        ext = getattr(self, 'extensions', None)
        if ext and ext.get('dspy_service'):
            # Handle DSPy service tasks with telemetry
            dspy_info = ext['dspy_service']
            
            with self.telemetry_manager.start_span("dspy.service_task") as span:
                span.set_attribute("dspy.service_name", dspy_info['service'])
                span.set_attribute("dspy.result_var", dspy_info.get('result', ''))
                
                try:
                    # Resolve parameters from task data
                    resolved_params = {k: my_task.get_data(v) for k, v in dspy_info['params'].items()}
                    span.set_attribute("dspy.input_params", str(resolved_params))
                    
                    # Validate input if schema validator is available
                    if self.schema_validator and hasattr(self.schema_validator, 'validate_dspy_input'):
                        validated_params = self.schema_validator.validate_dspy_input(
                            dspy_info['service'], resolved_params
                        )
                        resolved_params = validated_params
                    
                    # Call DSPy service
                    from ..utils.dspy_services import dspy_service
                    result = dspy_service(dspy_info['service'], **resolved_params)
                    
                    # Validate output if schema validator is available
                    if self.schema_validator and hasattr(self.schema_validator, 'validate_dspy_output'):
                        validated_result = self.schema_validator.validate_dspy_output(
                            dspy_info['service'], result
                        )
                        result = validated_result
                    
                    # Store result in task data
                    if dspy_info['result']:
                        my_task.set_data(**{dspy_info['result']: result})
                    
                    span.set_attribute("dspy.output_result", str(result))
                    span.add_event("DSPy service executed successfully")
                    
                    logger.info(f"Executed DSPy service {dspy_info['service']} with result: {result}")
                    
                except Exception as e:
                    span.record_exception(e)
                    logger.error(f"Failed to execute DSPy service {dspy_info['service']}: {e}")
                    raise
        
        # Call the parent method to continue normal execution
        return super()._run_hook(my_task)


class AutoTelTaskParser(TaskParser):
    """Task parser that handles DSPy service extensions"""
    
    def parse_extensions(self):
        extensions = super().parse_extensions()
        dspy_ns = 'http://autotel.ai/dspy'
        dspy_services = self.node.xpath('.//dspy:service', namespaces={'dspy': dspy_ns})
        
        if dspy_services:
            service = dspy_services[0]
            service_name = service.attrib['name']
            result_var = service.attrib.get('result')
            params = {}
            
            for param in service.xpath('./dspy:param', namespaces={'dspy': dspy_ns}):
                param_name = param.attrib['name']
                param_value = param.attrib['value']
                params[param_name] = param_value
            
            extensions['dspy_service'] = {
                'service': service_name,
                'result': result_var,
                'params': params,
            }
        
        return extensions


class AutoTelCamundaParser(CamundaParser):
    """AutoTel Camunda parser that extends CamundaParser with DSPy support"""
    
    OVERRIDE_PARSER_CLASSES = CamundaParser.OVERRIDE_PARSER_CLASSES.copy()
    OVERRIDE_PARSER_CLASSES['{http://www.omg.org/spec/BPMN/20100524/MODEL}serviceTask'] = (AutoTelTaskParser, AutoTelServiceTask)
    
    def __init__(self, telemetry_manager: TelemetryManager = None, schema_validator: SchemaValidator = None):
        super().__init__()
        self.telemetry_manager = telemetry_manager
        self.schema_validator = schema_validator
        self.signature_definitions = {}
        self.dynamic_signatures = {}
    
    def add_bpmn_xml(self, bpmn, filename=None):
        """Override to parse DSPy signatures and DMN definitions"""
        # Check for CDATA sections
        cdata_sections = bpmn.xpath('//text()[contains(., "<![CDATA[")]')
        if cdata_sections:
            raise ValueError(f"CDATA sections are not allowed in BPMN XML. Found CDATA in file: {filename}")
        
        # Parse DSPy signature definitions
        self._parse_dspy_signatures(bpmn)
        
        # Parse DMN definitions from the BPMN file
        self._parse_dmn_definitions(bpmn, filename)
        
        # Call the parent method to continue normal parsing
        super().add_bpmn_xml(bpmn, filename)
    
    def _parse_dspy_signatures(self, bpmn):
        """Parse DSPy signature definitions from the BPMN XML"""
        dspy_ns = 'http://autotel.ai/dspy'
        
        # Find signature definitions
        signatures = bpmn.xpath('.//dspy:signatures/dspy:signature', namespaces={'dspy': dspy_ns})
        
        for signature in signatures:
            name = signature.attrib['name']
            description = signature.attrib.get('description', '')
            
            # Parse inputs
            inputs = {}
            for input_elem in signature.xpath('./dspy:input', namespaces={'dspy': dspy_ns}):
                input_name = input_elem.attrib['name']
                input_desc = input_elem.attrib.get('description', '')
                optional = input_elem.attrib.get('optional', 'false').lower() == 'true'
                inputs[input_name] = {
                    'description': input_desc,
                    'optional': optional
                }
            
            # Parse outputs
            outputs = {}
            for output_elem in signature.xpath('./dspy:output', namespaces={'dspy': dspy_ns}):
                output_name = output_elem.attrib['name']
                output_desc = output_elem.attrib.get('description', '')
                outputs[output_name] = output_desc
            
            # Store signature definition
            self.signature_definitions[name] = {
                'name': name,
                'description': description,
                'inputs': inputs,
                'outputs': outputs
            }
            
            logger.info(f"Parsed DSPy signature: {name}")
    
    def _parse_dmn_definitions(self, bpmn, filename=None):
        """Parse DMN definitions from the BPMN XML and add them to the parser"""
        # Find DMN definitions in the BPMN file
        dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
        dmn_definitions = bpmn.findall('.//dmn:definitions', dmn_ns)
        
        for dmn_def in dmn_definitions:
            logger.info(f"Found DMN definition in BPMN file: {filename}")
            # Add the DMN definition to the parser
            self.add_dmn_xml(dmn_def, filename)


class AutoTelCamundaEngine:
    """AutoTel Camunda-style engine that follows spiff-example-cli patterns"""
    
    def __init__(self, telemetry_manager: TelemetryManager = None, schema_validator: SchemaValidator = None):
        self.telemetry_manager = telemetry_manager or TelemetryManager()
        self.schema_validator = schema_validator or SchemaValidator()
        
        # Create parser with telemetry and validation
        self.parser = AutoTelCamundaParser(self.telemetry_manager, self.schema_validator)
        
        # Set up script environment
        self.script_env = TaskDataEnvironment({'datetime': datetime})
        
        # Set up handlers (following spiff-example-cli pattern)
        self.handlers = {
            UserTask: self._create_user_task_handler(),
            ManualTask: self._create_manual_task_handler(),
            NoneTask: self._create_manual_task_handler(),
        }
        
        logger.info("AutoTel Camunda engine initialized")
    
    def _create_user_task_handler(self):
        """Create user task handler with telemetry"""
        # This would be implemented based on your UI requirements
        return None
    
    def _create_manual_task_handler(self):
        """Create manual task handler with telemetry"""
        # This would be implemented based on your UI requirements
        return None
    
    def add_bpmn_file(self, bpmn_path: str) -> None:
        """Add BPMN file to the parser"""
        with self.telemetry_manager.start_span("autotel.add_bpmn_file") as span:
            span.set_attribute("bpmn.file_path", bpmn_path)
            
            try:
                self.parser.add_bpmn_file(bpmn_path)
                
                # Register dynamic signatures
                if hasattr(self.parser, 'dynamic_signatures') and self.parser.dynamic_signatures:
                    dspy_registry.register_parser_signatures(self.parser.dynamic_signatures)
                    span.set_attribute("dspy.signatures_registered", len(self.parser.dynamic_signatures))
                
                span.add_event("BPMN file added successfully")
                logger.info(f"Added BPMN file: {bpmn_path}")
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to add BPMN file {bpmn_path}: {e}")
                raise
    
    def add_dmn_file(self, dmn_path: str) -> None:
        """Add DMN file to the parser"""
        with self.telemetry_manager.start_span("autotel.add_dmn_file") as span:
            span.set_attribute("dmn.file_path", dmn_path)
            
            try:
                self.parser.add_dmn_files([dmn_path])
                span.add_event("DMN file added successfully")
                logger.info(f"Added DMN file: {dmn_path}")
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to add DMN file {dmn_path}: {e}")
                raise
    
    def create_workflow(self, process_id: str, initial_data: Dict[str, Any] = None) -> 'BpmnWorkflow':
        """Create a workflow instance"""
        with self.telemetry_manager.start_span("autotel.create_workflow") as span:
            span.set_attribute("bpmn.process_id", process_id)
            
            try:
                # Get workflow specification
                specs = self.parser.find_all_specs()
                if process_id not in specs:
                    raise ValueError(f"Process ID '{process_id}' not found in BPMN files")
                
                spec = specs[process_id]
                
                # Create workflow
                from SpiffWorkflow.bpmn import BpmnWorkflow
                workflow = BpmnWorkflow(spec, script_engine=self.script_env)
                
                # Set initial data
                if initial_data:
                    workflow.set_data(**initial_data)
                
                # Set telemetry on service tasks
                self._set_telemetry_on_tasks(workflow)
                
                span.add_event("Workflow created successfully")
                logger.info(f"Created workflow for process: {process_id}")
                
                return workflow
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to create workflow for process {process_id}: {e}")
                raise
    
    def _set_telemetry_on_tasks(self, workflow):
        """Set telemetry on all service tasks in the workflow"""
        for task_spec in workflow.spec.task_specs.values():
            if isinstance(task_spec, AutoTelServiceTask):
                task_spec.set_telemetry(self.telemetry_manager, self.schema_validator)
    
    def execute_workflow(self, workflow, run_until_user_input: bool = True) -> Dict[str, Any]:
        """Execute a workflow"""
        with self.telemetry_manager.start_span("autotel.execute_workflow") as span:
            span.set_attribute("workflow.initial_data", str(workflow.data))
            
            try:
                if run_until_user_input:
                    workflow.run_until_user_input_required()
                else:
                    workflow.run_all()
                
                final_data = workflow.data
                span.set_attribute("workflow.final_data", str(final_data))
                span.add_event("Workflow executed successfully")
                
                logger.info("Workflow executed successfully")
                return final_data
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to execute workflow: {e}")
                raise
    
    def list_processes(self) -> Dict[str, str]:
        """List all available processes"""
        try:
            specs = self.parser.find_all_specs()
            return {process_id: spec.name for process_id, spec in specs.items()}
        except Exception as e:
            logger.error(f"Failed to list processes: {e}")
            return {}
    
    def list_dmn_decisions(self) -> Dict[str, str]:
        """List all available DMN decisions"""
        try:
            decisions = {}
            for decision_id, parser in self.parser.dmn_parsers.items():
                decisions[decision_id] = parser.decision.name if hasattr(parser, 'decision') else decision_id
            return decisions
        except Exception as e:
            logger.error(f"Failed to list DMN decisions: {e}")
            return {}


def create_autotel_camunda_engine(telemetry_manager: TelemetryManager = None, 
                                 schema_validator: SchemaValidator = None) -> AutoTelCamundaEngine:
    """Factory function to create AutoTel Camunda engine"""
    return AutoTelCamundaEngine(telemetry_manager, schema_validator) 