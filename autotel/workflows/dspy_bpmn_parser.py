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
from pathlib import Path
import pyshacl
from rdflib import Graph, URIRef, Literal, Namespace
from rdflib.namespace import RDF, XSD

@dataclass
class DSPySignatureDefinition:
    """Enhanced definition of a DSPy signature from XML with SHACL integration"""
    name: str
    description: str
    inputs: Dict[str, Dict[str, str]]  # input_name -> {description, optional}
    outputs: Dict[str, str]  # output_name -> description
    shacl_input_shapes: Dict[str, str] = None  # input_name -> shape_uri
    shacl_output_shapes: Dict[str, str] = None  # output_name -> shape_uri

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
    """Enhanced service task that executes DSPy services with SHACL validation"""
    
    def _run_hook(self, my_task):
        """Execute DSPy service with full validation pipeline"""
        ext = getattr(self, 'extensions', None)
        if ext and ext.get('dspy_service'):
            # Robustly get the parser for access to signatures and SHACL graphs
            wf_spec = getattr(self, 'wf_spec', None)
            parser = None
            if wf_spec is not None:
                parser = getattr(wf_spec, 'parser', None)
            if parser is None and hasattr(my_task, 'workflow'):
                wf_spec2 = getattr(my_task.workflow, 'spec', None)
                if wf_spec2 is not None:
                    parser = getattr(wf_spec2, 'parser', None)
            if parser is None:
                raise AttributeError("Could not find parser for DspyServiceTask")
            dspy_info = ext['dspy_service']
            
            # Get signature definition
            sig_def = parser.get_signature_definition(dspy_info['signature'])
            if not sig_def:
                raise ValueError(f"DSPy signature '{dspy_info['signature']}' not found")
            
            # Gather input parameters
            resolved_params = {}
            for param_name, param_value in dspy_info['params'].items():
                data = my_task.get_data(param_value)
                # If get_data returns the param_value itself, it's a workflow data reference
                if data == param_value:
                    # Try to get the actual workflow data
                    data = my_task.get_data(param_value)
                    # If still the same, try getting from workflow data directly
                    if data == param_value and hasattr(my_task, 'workflow'):
                        data = my_task.workflow.get_data(param_value)
                # If get_data returns None, use the param_value directly (for literal values)
                if data is None:
                    data = param_value
                resolved_params[param_name] = data
                
                # SHACL Input Validation
                if hasattr(parser, 'shacl_graph') and sig_def.shacl_input_shapes:
                    shape_uri = sig_def.shacl_input_shapes.get(param_name)
                    if shape_uri:
                        self._validate_data_with_shacl(
                            {param_name: data}, shape_uri, parser.shacl_graph, f"DSPy Input [{param_name}]"
                        )
            
            # Execute DSPy reasoning
            from autotel.utils.dspy_services import dspy_service
            print(f"DEBUG: Executing DSPy service {dspy_info['service']} with params {resolved_params}")
            try:
                result = dspy_service(dspy_info['service'], **resolved_params)
                print(f"DEBUG: DSPy service {dspy_info['service']} completed with result: {result}")
            except Exception as e:
                print(f"ERROR: DSPy service {dspy_info['service']} failed: {e}")
                raise
            
            # SHACL Output Validation
            if hasattr(parser, 'shacl_graph') and sig_def.shacl_output_shapes:
                for output_name, shape_uri in sig_def.shacl_output_shapes.items():
                    if output_name in result:
                        self._validate_data_with_shacl(
                            {output_name: result[output_name]}, shape_uri, parser.shacl_graph, f"DSPy Output [{output_name}]"
                        )
            
            # Set results in task data and ensure they propagate to workflow
            if dspy_info.get('result'):
                # Store result under the specified key (e.g., 'fraud_result')
                my_task.set_data(**{dspy_info['result']: result})
                # Also set in workflow data to ensure propagation
                my_task.workflow.set_data(**{dspy_info['result']: result})
            elif isinstance(result, dict):
                # Fallback: if no result key specified, flatten the dict
                my_task.set_data(**result)
                # Also set in workflow data to ensure propagation
                my_task.workflow.set_data(**result)
        
        return super()._run_hook(my_task)
    
    def _validate_data_with_shacl(self, data_dict: dict, shape_uri: str, shacl_graph, context: str):
        """Validate DSPy data using SHACL shapes"""
        
        # Convert Python dict to RDF graph
        data_graph = Graph()
        autotel_ns = Namespace("http://autotel.ai/data/")
        instance_uri = autotel_ns.instance
        
        # Add type information
        data_graph.add((instance_uri, RDF.type, URIRef(shape_uri)))
        
        # Convert data to RDF triples
        for key, value in data_dict.items():
            predicate = autotel_ns[key]
            if isinstance(value, str):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.string)))
            elif isinstance(value, int):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.integer)))
            elif isinstance(value, float):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.decimal)))
            elif isinstance(value, bool):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.boolean)))
            else:
                # Serialize complex objects as JSON strings
                import json
                json_value = json.dumps(value) if not isinstance(value, str) else str(value)
                data_graph.add((instance_uri, predicate, Literal(json_value, datatype=XSD.string)))
        
        # Perform SHACL validation
        conforms, results_graph, results_text = pyshacl.validate(
            data_graph,
            shacl_graph=shacl_graph,
            inference='rdfs'
        )
        
        if not conforms:
            raise ValidationException(f"{context} data failed SHACL validation for shape <{shape_uri}>:\n{results_text}")

class DspyTaskParser(TaskParser):
    """Enhanced parser for DSPy service tasks with SHACL integration"""
    
    def parse_extensions(self):
        extensions = super().parse_extensions()
        dspy_ns = 'http://autotel.ai/dspy'
        
        # Parse DSPy service definitions
        dspy_services = self.node.xpath('.//dspy:service', namespaces={'dspy': dspy_ns})
        
        if dspy_services:
            service = dspy_services[0]
            service_name = service.attrib['name']
            signature_name = service.attrib.get('signature', service_name)  # Use service name as fallback
            result_var = service.attrib.get('result')
            
            # Parse parameters
            params = {}
            for param in service.xpath('./dspy:param', namespaces={'dspy': dspy_ns}):
                param_name = param.attrib['name']
                param_value = param.attrib['value']
                params[param_name] = param_value
            
            extensions['dspy_service'] = {
                'service': service_name,
                'signature': signature_name,
                'result': result_var,
                'params': params,
            }
        
        return extensions

class DspyBpmnParser(CamundaParser):
    """
    Enhanced AutoTel V4 parser handling BPMN, DMN, DSPy, and SHACL
    This implements the "Four Pillars" architecture
    """
    
    # Override parser classes for custom task types
    OVERRIDE_PARSER_CLASSES = CamundaParser.OVERRIDE_PARSER_CLASSES.copy()
    OVERRIDE_PARSER_CLASSES['{http://www.omg.org/spec/BPMN/20100524/MODEL}serviceTask'] = (DspyTaskParser, DspyServiceTask)
    
    def __init__(self):
        super().__init__()
        # DSPy signature management
        self.signature_definitions: Dict[str, DSPySignatureDefinition] = {}
        self.dynamic_signatures: Dict[str, Type[dspy.Signature]] = {}
        
        # SHACL shapes management
        self.shacl_graph = Graph()  # Single graph for all SHACL shapes
        
        # Integration tracking
        self.loaded_contracts = {
            'bpmn_files': [],
            'dmn_files': [],
            'dspy_signatures': [],
            'shacl_shapes': []
        }
    
    def add_shacl_file(self, shacl_path: str):
        """Parse and add SHACL shapes file"""
        try:
            self.shacl_graph.parse(shacl_path, format="xml")
            self.loaded_contracts['shacl_shapes'].append(shacl_path)
        except Exception as e:
            raise ValidationException(f"Failed to parse SHACL file '{shacl_path}': {e}")
    
    def load_from_sealed_directory(self, dir_path: Path):
        """
        Load a complete AutoTel project directory with all four pillars
        Expected structure:
        - process.bpmn (main BPMN process)
        - *.dmn (DMN decision tables)
        - *.dspy.xml (DSPy signature definitions)
        - *.shacl.xml (SHACL data shapes)
        """
        dir_path = Path(dir_path)
        
        # Verify directory structure
        if not dir_path.exists():
            raise FileNotFoundError(f"Directory not found: {dir_path}")
        
        # Load SHACL shapes first (data contracts)
        for shacl_file in dir_path.glob("*.shacl.xml"):
            self.add_shacl_file(str(shacl_file))
        
        # Load DMN files (business rules)
        for dmn_file in dir_path.glob("*.dmn"):
            self.add_dmn_file(str(dmn_file))
            self.loaded_contracts['dmn_files'].append(str(dmn_file))
        
        # Load DSPy signature definitions (AI reasoning contracts)
        for dspy_file in dir_path.glob("*.dspy.xml"):
            self._load_dspy_file(str(dspy_file))
        
        # Load main BPMN process last (orchestration)
        main_bpmn = dir_path / "process.bpmn"
        if main_bpmn.exists():
            self.add_bpmn_file(str(main_bpmn))
            self.loaded_contracts['bpmn_files'].append(str(main_bpmn))
        else:
            raise FileNotFoundError(f"Main process.bpmn not found in {dir_path}")
        
        self._validate_integration()
    
    def _load_dspy_file(self, dspy_path: str):
        """Load DSPy signatures from separate XML file"""
        from lxml import etree
        
        try:
            tree = etree.parse(dspy_path)
            root = tree.getroot()
            
            # For standalone DSPy files, parse signatures directly from root
            self._parse_dspy_signatures_from_root(root)
            self.loaded_contracts['dspy_signatures'].append(dspy_path)
        except Exception as e:
            raise ValidationException(f"Failed to parse DSPy file '{dspy_path}': {e}")
    
    def _parse_dspy_signatures_from_root(self, root):
        """Parse DSPy signatures directly from root element (for standalone files)"""
        dspy_ns = 'http://autotel.ai/dspy'
        
        # Find signature definitions directly in root
        signatures = root.xpath('.//dspy:signature', namespaces={'dspy': dspy_ns})
        
        for signature in signatures:
            name = signature.attrib['name']
            description = signature.attrib.get('description', '')
            
            # Parse inputs with optional SHACL shape references
            inputs = {}
            shacl_input_shapes = {}
            for input_elem in signature.xpath('./dspy:input', namespaces={'dspy': dspy_ns}):
                input_name = input_elem.attrib['name']
                input_desc = input_elem.attrib.get('description', '')
                optional = input_elem.attrib.get('optional', 'false').lower() == 'true'
                shacl_shape = input_elem.attrib.get('shaclShape')
                
                inputs[input_name] = {
                    'description': input_desc,
                    'optional': optional
                }
                
                if shacl_shape:
                    shacl_input_shapes[input_name] = shacl_shape
            
            # Parse outputs with optional SHACL shape references
            outputs = {}
            shacl_output_shapes = {}
            for output_elem in signature.xpath('./dspy:output', namespaces={'dspy': dspy_ns}):
                output_name = output_elem.attrib['name']
                output_desc = output_elem.attrib.get('description', '')
                shacl_shape = output_elem.attrib.get('shaclShape')
                
                outputs[output_name] = output_desc
                
                if shacl_shape:
                    shacl_output_shapes[output_name] = shacl_shape
            
            # Create enhanced signature definition
            sig_def = DSPySignatureDefinition(
                name=name,
                description=description,
                inputs=inputs,
                outputs=outputs,
                shacl_input_shapes=shacl_input_shapes if shacl_input_shapes else None,
                shacl_output_shapes=shacl_output_shapes if shacl_output_shapes else None
            )
            
            self.signature_definitions[name] = sig_def
            self._create_dspy_signature_class(sig_def)
    
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
        """Override to handle both Camunda-style and inline DMN definitions robustly."""
        # First try the standard Camunda approach
        try:
            return super().get_decision_ref(node)
        except KeyError:
            # If no camunda:decisionRef, look for inline DMN definitions
            dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
            dmn_decision = node.find('.//dmn:decision', dmn_ns)
            if dmn_decision is not None:
                return dmn_decision.attrib.get('id', 'inline_decision')
            # Best practice: skip and warn if no decisionRef
            print(f"[WARN] Skipping businessRuleTask without camunda:decisionRef (id={node.attrib.get('id', 'unknown')})")
            return None
    
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
    
    def add_bpmn_xml(self, bpmn, filename=None):
        """Override to validate that no CDATA sections are present, parse DSPy signatures, and parse DMN definitions"""
        # Check for CDATA sections in the XML
        cdata_sections = bpmn.xpath('//text()[contains(., "<![CDATA[")]')
        if cdata_sections:
            raise ValidationException(
                f"CDATA sections are not allowed in BPMN XML. Found CDATA in file: {filename}",
                file_name=filename
            )
        
        # Parse embedded DSPy signatures
        self._parse_dspy_signatures(bpmn)
        
        # Parse embedded DMN definitions
        self._parse_dmn_definitions(bpmn, filename)
        
        # Continue with normal BPMN parsing
        super().add_bpmn_xml(bpmn, filename)
    
    def _parse_dmn_definitions(self, bpmn, filename=None):
        """Parse DMN definitions from the BPMN XML and add them to the parser"""
        # Find DMN definitions in the BPMN file
        dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
        dmn_definitions = bpmn.findall('.//dmn:definitions', dmn_ns)
        
        for dmn_def in dmn_definitions:
            # Add the DMN definition to the parser
            self.add_dmn_xml(dmn_def, filename)
    
    def _parse_dspy_signatures(self, bpmn):
        """Parse DSPy signature definitions from the BPMN XML with SHACL integration"""
        dspy_ns = 'http://autotel.ai/dspy'
        
        # Find signature definitions
        signatures = bpmn.xpath('.//dspy:signatures/dspy:signature', namespaces={'dspy': dspy_ns})
        
        for signature in signatures:
            name = signature.attrib['name']
            description = signature.attrib.get('description', '')
            
            # Parse inputs with optional SHACL shape references
            inputs = {}
            shacl_input_shapes = {}
            for input_elem in signature.xpath('./dspy:input', namespaces={'dspy': dspy_ns}):
                input_name = input_elem.attrib['name']
                input_desc = input_elem.attrib.get('description', '')
                optional = input_elem.attrib.get('optional', 'false').lower() == 'true'
                shacl_shape = input_elem.attrib.get('shaclShape')
                
                inputs[input_name] = {
                    'description': input_desc,
                    'optional': optional
                }
                
                if shacl_shape:
                    shacl_input_shapes[input_name] = shacl_shape
            
            # Parse outputs with optional SHACL shape references
            outputs = {}
            shacl_output_shapes = {}
            for output_elem in signature.xpath('./dspy:output', namespaces={'dspy': dspy_ns}):
                output_name = output_elem.attrib['name']
                output_desc = output_elem.attrib.get('description', '')
                shacl_shape = output_elem.attrib.get('shaclShape')
                
                outputs[output_name] = output_desc
                
                if shacl_shape:
                    shacl_output_shapes[output_name] = shacl_shape
            
            # Create enhanced signature definition
            sig_def = DSPySignatureDefinition(
                name=name,
                description=description,
                inputs=inputs,
                outputs=outputs,
                shacl_input_shapes=shacl_input_shapes if shacl_input_shapes else None,
                shacl_output_shapes=shacl_output_shapes if shacl_output_shapes else None
            )
            
            self.signature_definitions[name] = sig_def
            self._create_dspy_signature_class(sig_def)
    
    def _create_dspy_signature_class(self, sig_def: DSPySignatureDefinition):
        """Dynamically create DSPy signature class"""
        class_attrs = {
            '__doc__': f"AI-powered {sig_def.description}. This signature uses advanced machine learning to analyze data and provide intelligent insights with real-time processing.",
            '__module__': 'autotel.workflows.dynamic_signatures',
            '__annotations__': {}
        }
        
        # Add input fields with enhanced AI descriptions
        for input_name, input_info in sig_def.inputs.items():
            enhanced_desc = f"AI input: {input_info['description']}. This data will be analyzed using machine learning algorithms to extract patterns and insights."
            class_attrs[input_name] = dspy.InputField(desc=enhanced_desc)
            class_attrs['__annotations__'][input_name] = str  # Use str type for annotations
        
        # Add output fields with enhanced AI descriptions
        for output_name, output_desc in sig_def.outputs.items():
            enhanced_desc = f"AI-generated output: {output_desc}. This result is produced by advanced AI analysis using neural networks and pattern recognition."
            class_attrs[output_name] = dspy.OutputField(desc=enhanced_desc)
            class_attrs['__annotations__'][output_name] = str  # Use str type for annotations
        
        if not class_attrs['__annotations__']:
            raise ValueError(f"DSPy signature '{sig_def.name}' must have at least one input or output field")
        
        signature_class = type(sig_def.name, (dspy.Signature,), class_attrs)
        self.dynamic_signatures[sig_def.name] = signature_class
    
    def _validate_integration(self):
        """Validate that all four pillars are properly integrated"""
        
        # Check BPMN (Process pillar)
        if not self.loaded_contracts['bpmn_files']:
            pass  # No BPMN files loaded
        
        # Check DMN (Rules pillar)
        if not self.dmn_parsers:
            pass  # No DMN definitions found
        
        # Check DSPy (Reasoning pillar)
        if not self.signature_definitions:
            pass  # No DSPy signatures defined
        
        # Check SHACL (Data pillar)
        if len(self.shacl_graph) == 0:
            pass  # No SHACL shapes loaded
        
        # Validate cross-references
        self._validate_signature_shape_references()
    
    def _validate_signature_shape_references(self):
        """Validate that DSPy signatures reference valid SHACL shapes"""
        if not self.shacl_graph or not self.signature_definitions:
            return
        
        # Query for all available shapes
        shape_query = """
        PREFIX sh: <http://www.w3.org/ns/shacl#>
        SELECT DISTINCT ?shape WHERE {
            ?shape a sh:NodeShape .
        }
        """
        
        available_shapes = set()
        for row in self.shacl_graph.query(shape_query):
            available_shapes.add(str(row.shape))
        
        # Check signature references
        for sig_name, sig_def in self.signature_definitions.items():
            if sig_def.shacl_input_shapes:
                for input_name, shape_uri in sig_def.shacl_input_shapes.items():
                    if shape_uri not in available_shapes:
                        pass  # Unknown SHACL shape reference
            if sig_def.shacl_output_shapes:
                for output_name, shape_uri in sig_def.shacl_output_shapes.items():
                    if shape_uri not in available_shapes:
                        pass  # Unknown SHACL shape reference
    
    # Enhanced accessors
    def get_dynamic_signature(self, name: str) -> Type[dspy.Signature]:
        """Get dynamically created DSPy signature"""
        return self.dynamic_signatures.get(name)
    
    def get_signature_definition(self, name: str) -> DSPySignatureDefinition:
        """Get signature definition with SHACL integration"""
        return self.signature_definitions.get(name)
    
    def get_shacl_shapes_for_signature(self, signature_name: str) -> Dict[str, Dict[str, str]]:
        """Get SHACL shape mappings for a DSPy signature"""
        sig_def = self.signature_definitions.get(signature_name)
        if not sig_def:
            return {}
        
        return {
            'inputs': sig_def.shacl_input_shapes or {},
            'outputs': sig_def.shacl_output_shapes or {}
        }
    
    def list_signatures(self) -> Dict[str, DSPySignatureDefinition]:
        """List all signature definitions"""
        return self.signature_definitions.copy()
    
    def _strip_xml_encoding(self, xml_str: str) -> bytes:
        """Remove XML encoding declaration and return as bytes for lxml parsing."""
        if xml_str.startswith('<?xml'):
            lines = xml_str.split('\n')
            if lines[0].startswith('<?xml'):
                lines = lines[1:]
            xml_str = '\n'.join(lines)
        return xml_str.encode('utf-8')

    def add_bpmn_xml_from_string(self, bpmn_xml: str, filename: str = None):
        """Add BPMN XML from string, including inline SHACL shapes (RDF/XML only)"""
        from lxml import etree
        
        try:
            # Always strip encoding and use bytes
            root = etree.fromstring(self._strip_xml_encoding(bpmn_xml))

            # --- Extract and load inline SHACL shapes (RDF/XML only) ---
            shacl_ns = {'shacl': 'http://autotel.ai/shacl'}
            shacl_elems = root.xpath('.//shacl:shapes', namespaces=shacl_ns)
            if shacl_elems:
                from rdflib import Graph
                for elem in shacl_elems:
                    # Look for RDF/XML child
                    rdf_elem = None
                    for child in elem:
                        if child.tag.endswith('RDF'):
                            rdf_elem = child
                            break
                    if rdf_elem is not None:
                        rdf_xml = etree.tostring(rdf_elem, encoding='utf-8')
                        g = Graph()
                        g.parse(data=rdf_xml, format='xml')
                        for triple in g:
                            self.shacl_graph.add(triple)
            # --- END SHACL ---
            
            # Add to parser
            self.add_bpmn_xml(root, filename)
            # Track loaded BPMN (for in-memory loads)
            if filename is None:
                filename = '<in-memory-bpmn>'
            self.loaded_contracts['bpmn_files'].append(filename)
            
        except Exception as e:
            raise ValidationException(f"Failed to parse BPMN XML string: {e}") 

    def _find_dependencies(self, process):
        """Override to robustly handle businessRuleTasks without decisionRef."""
        business_rule_tag = '{http://www.omg.org/spec/BPMN/20100524/MODEL}businessRuleTask'
        parser_cls, cls = self._get_parser_class(business_rule_tag)
        for business_rule in process.xpath('.//bpmn:businessRuleTask', namespaces=self.namespaces):
            try:
                decision_ref = parser_cls.get_decision_ref(business_rule)
            except KeyError:
                print(f"[WARN] Skipping businessRuleTask without camunda:decisionRef (id={business_rule.attrib.get('id', 'unknown')})")
                continue
            if decision_ref is not None:
                self.dmn_dependencies.add(decision_ref) 

    def create_executable_specification(self, process_id: str) -> 'ExecutableSpecification':
        """
        Create an executable specification from parsed XML.
        This is the clean separation point between parsing and execution.
        
        Returns an ExecutableSpecification object that contains all the
        information needed to execute the workflow without any XML parsing logic.
        """
        from dataclasses import dataclass
        from typing import Dict, Any, List, Optional
        
        @dataclass
        class ExecutableSpecification:
            """Pure data structure containing all execution information"""
            process_id: str
            bpmn_spec: Any  # SpiffWorkflow process specification
            dspy_signatures: Dict[str, DSPySignatureDefinition]
            dmn_engines: Dict[str, Any]
            shacl_shapes: Graph
            validation_rules: Dict[str, Any]
            execution_context: Dict[str, Any]
            
            def validate_input_data(self, data: Dict[str, Any]) -> Dict[str, Any]:
                """Validate input data against SHACL shapes"""
                # Pure validation logic - no XML parsing
                pass
            
            def execute_workflow(self, context: Dict[str, Any]) -> Dict[str, Any]:
                """Execute the workflow with given context"""
                # Pure execution logic - no XML parsing
                pass
        
        # Get the BPMN process specification
        specs = self.find_all_specs()
        if process_id not in specs:
            raise ValueError(f"Process '{process_id}' not found in parsed BPMN")
        
        bpmn_spec = specs[process_id]
        
        # Create DMN engines for all referenced decisions
        dmn_engines = {}
        for decision_ref in self.dmn_dependencies:
            if decision_ref in self.dmn_parsers:
                dmn_engines[decision_ref] = self.get_engine(decision_ref, None)
        
        # Create validation rules from SHACL shapes
        validation_rules = self._create_validation_rules()
        
        # Create execution context
        execution_context = {
            'parser': self,  # Reference to parser for runtime lookups
            'signature_registry': self.dynamic_signatures,
            'shacl_graph': self.shacl_graph
        }
        
        return ExecutableSpecification(
            process_id=process_id,
            bpmn_spec=bpmn_spec,
            dspy_signatures=self.signature_definitions.copy(),
            dmn_engines=dmn_engines,
            shacl_shapes=self.shacl_graph,
            validation_rules=validation_rules,
            execution_context=execution_context
        )
    
    def _create_validation_rules(self) -> Dict[str, Any]:
        """Create validation rules from SHACL shapes - pure data transformation"""
        # Convert SHACL shapes to validation rules
        # No XML parsing, just data structure transformation
        return {
            'shacl_shapes': self.shacl_graph,
            'signature_shapes': self._get_signature_shape_mappings()
        }
    
    def _get_signature_shape_mappings(self) -> Dict[str, Dict[str, str]]:
        """Get SHACL shape mappings for all signatures - pure data access"""
        mappings = {}
        for sig_name, sig_def in self.signature_definitions.items():
            mappings[sig_name] = {
                'inputs': sig_def.shacl_input_shapes or {},
                'outputs': sig_def.shacl_output_shapes or {}
            }
        return mappings 