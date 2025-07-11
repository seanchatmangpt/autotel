from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn.parser.TaskParser import TaskParser
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask
from SpiffWorkflow.bpmn.parser.ValidationException import ValidationException
from SpiffWorkflow.camunda.parser.CamundaParser import CamundaParser
from SpiffWorkflow.dmn.specs import BusinessRuleTaskMixin
from SpiffWorkflow.dmn.serializer import BaseBusinessRuleTaskConverter
from SpiffWorkflow.dmn.engine.DMNEngine import DMNEngine
import dspy
from typing import Dict, Any, Type
from dataclasses import dataclass

@dataclass
class DSPySignatureDefinition:
    """Definition of a DSPy signature from XML"""
    name: str
    description: str
    inputs: Dict[str, Dict[str, str]]  # input_name -> {description, optional}
    outputs: Dict[str, str]  # output_name -> description

class FixedDMNEngine(DMNEngine):
    """DMN Engine that properly evaluates input expressions"""
    
    def evaluate(self, input_expr, match_expr, task):
        """Override to add debugging and fix task data"""
        print(f"🔍 DMN Engine Debug:")
        print(f"   Task data before fix: {task.data}")
        print(f"   Workflow data: {task.workflow.data}")
        
        # Fix: Copy workflow data to task data if task data is empty
        if not task.data and task.workflow.data:
            task.data = task.workflow.data.copy()
            print(f"   Task data after fix: {task.data}")
        
        print(f"   Input expression: {input_expr}")
        print(f"   Match expression: {match_expr}")
        
        # Call the parent method
        return super().evaluate(input_expr, match_expr, task)
    
    @staticmethod
    def __get_input_val(input_entry, context):
        """
        Override to properly evaluate input expressions as variable references.
        """
        print(f"🔍 __get_input_val Debug:")
        print(f"   Context: {context}")
        print(f"   Input expression: {input_entry.input.expression}")
        
        if input_entry.input.expression:
            # Evaluate the expression as a variable reference
            return input_entry.input.expression
        else:
            # Backwards compatibility
            return "%r" % context[input_entry.input.label]

class DspyServiceTask(ServiceTask):
    """Service task that can execute DSPy services"""
    
    def _run_hook(self, my_task):
        """Override to handle DSPy service execution"""
        ext = getattr(self, 'extensions', None)
        if ext and ext.get('dspy_service'):
            # Handle DSPy service tasks
            from autotel.utils.dspy_services import dspy_service
            dspy_info = ext['dspy_service']
            resolved_params = {k: my_task.get_data(v) for k, v in dspy_info['params'].items()}
            result = dspy_service(dspy_info['service'], **resolved_params)
            # Set each output field on the task if result is a dict
            if isinstance(result, dict):
                my_task.set_data(**result)
                for k, v in result.items():
                    print(f"[DEBUG] Set task data: {k}, type: {type(v)}, value: {v}")
            elif dspy_info['result']:
                my_task.set_data(**{dspy_info['result']: result})
        
        # Call the parent method to continue normal execution
        return super()._run_hook(my_task)

class DspyTaskParser(TaskParser):
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

class DspyBpmnParser(CamundaParser):
    OVERRIDE_PARSER_CLASSES = CamundaParser.OVERRIDE_PARSER_CLASSES.copy()
    OVERRIDE_PARSER_CLASSES['{http://www.omg.org/spec/BPMN/20100524/MODEL}serviceTask'] = (DspyTaskParser, DspyServiceTask)
    
    def get_engine(self, decision_ref, node):
        if decision_ref not in self.dmn_parsers:
            options = ', '.join(list(self.dmn_parsers.keys()))
            raise ValidationException(
                'No DMN Diagram available with id "%s", Available DMN ids are: %s' %(decision_ref, options),
                node=node, file_name='')
        dmn_parser = self.dmn_parsers[decision_ref]
        dmn_parser.parse()
        decision = dmn_parser.decision

        return FixedDMNEngine(decision.decisionTables[0])
    
    def get_decision_ref(self, node):
        """Override to handle both Camunda-style and inline DMN definitions"""
        # First try the standard Camunda approach
        try:
            return super().get_decision_ref(node)
        except KeyError:
            # If no camunda:decisionRef, look for inline DMN definitions
            dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
            dmn_decision = node.find('.//dmn:decision', dmn_ns)
            if dmn_decision is not None:
                return dmn_decision.attrib.get('id', 'inline_decision')
            raise KeyError(f"No DMN decision reference found for node {node.attrib.get('id', 'unknown')}")
    
    def add_dmn_file(self, filename):
        """Add a DMN file to the parser"""
        from lxml import etree
        dmn = etree.parse(filename)
        self.add_dmn_xml(dmn.getroot(), filename)
    
    def add_dmn_xml(self, dmn, filename=None):
        """Add DMN XML to the parser"""
        # Check for CDATA sections in the XML
        cdata_sections = dmn.xpath('//text()[contains(., "<![CDATA[")]')
        if cdata_sections:
            raise ValidationException(
                f"CDATA sections are not allowed in DMN XML. Found CDATA in file: {filename}",
                file_name=filename
            )
        
        # Call the parent method to add DMN definitions
        super().add_dmn_xml(dmn, filename)
    
    def __init__(self):
        super().__init__()
        self.signature_definitions: Dict[str, DSPySignatureDefinition] = {}
        self.dynamic_signatures: Dict[str, Type[dspy.Signature]] = {}
    
    def add_bpmn_xml(self, bpmn, filename=None):
        """Override to validate that no CDATA sections are present, parse DSPy signatures, and parse DMN definitions"""
        # Check for CDATA sections in the XML
        cdata_sections = bpmn.xpath('//text()[contains(., "<![CDATA[")]')
        if cdata_sections:
            raise ValidationException(
                f"CDATA sections are not allowed in BPMN XML. Found CDATA in file: {filename}",
                file_name=filename
            )
        
        # Parse DSPy signature definitions
        self._parse_dspy_signatures(bpmn)
        
        # Parse DMN definitions from the BPMN file
        self._parse_dmn_definitions(bpmn, filename)
        
        # Call the parent method to continue normal parsing (includes DMN parsing)
        super().add_bpmn_xml(bpmn, filename)
    
    def _parse_dmn_definitions(self, bpmn, filename=None):
        """Parse DMN definitions from the BPMN XML and add them to the parser"""
        # Find DMN definitions in the BPMN file
        dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
        dmn_definitions = bpmn.findall('.//dmn:definitions', dmn_ns)
        
        for dmn_def in dmn_definitions:
            print(f"✅ Found DMN definition in BPMN file")
            # Add the DMN definition to the parser
            self.add_dmn_xml(dmn_def, filename)
    
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
            
            # Create signature definition
            sig_def = DSPySignatureDefinition(
                name=name,
                description=description,
                inputs=inputs,
                outputs=outputs
            )
            
            self.signature_definitions[name] = sig_def
            
            # Dynamically create DSPy signature class
            self._create_dspy_signature_class(sig_def)
    
    def _create_dspy_signature_class(self, sig_def: DSPySignatureDefinition):
        """Dynamically create a DSPy signature class from XML definition (robust version)"""
        class_attrs = {
            '__doc__': sig_def.description,
            '__module__': 'autotel.workflows.dynamic_signatures',
            '__annotations__': {}
        }

        # Add input fields
        for input_name, input_info in sig_def.inputs.items():
            if not input_name:
                raise ValueError(f"Input field missing 'name' attribute in DSPy signature '{sig_def.name}' XML.")
            class_attrs[input_name] = dspy.InputField(desc=input_info['description'])
            class_attrs['__annotations__'][input_name] = dspy.InputField

        # Add output fields
        for output_name, output_desc in sig_def.outputs.items():
            if not output_name:
                raise ValueError(f"Output field missing 'name' attribute in DSPy signature '{sig_def.name}' XML.")
            class_attrs[output_name] = dspy.OutputField(desc=output_desc)
            class_attrs['__annotations__'][output_name] = dspy.OutputField

        # Validate at least one input or output
        if not class_attrs['__annotations__']:
            raise ValueError(f"DSPy signature '{sig_def.name}' must have at least one input or output field.")

        signature_class = type(sig_def.name, (dspy.Signature,), class_attrs)
        self.dynamic_signatures[sig_def.name] = signature_class

        print(f"✅ Created dynamic DSPy signature: {sig_def.name}")
        print(f"   Inputs: {list(sig_def.inputs.keys())}")
        print(f"   Outputs: {list(sig_def.outputs.keys())}")
    
    def get_dynamic_signature(self, name: str) -> Type[dspy.Signature]:
        """Get a dynamically created DSPy signature by name"""
        return self.dynamic_signatures.get(name)
    
    def get_signature_definition(self, name: str) -> DSPySignatureDefinition:
        """Get the signature definition by name"""
        return self.signature_definitions.get(name)
    
    def list_signatures(self) -> Dict[str, DSPySignatureDefinition]:
        """List all signature definitions"""
        return self.signature_definitions.copy() 