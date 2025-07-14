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
            result = dspy_service(dspy_info['service'], **resolved_params)
            
            # SHACL Output Validation
            if hasattr(parser, 'shacl_graph') and sig_def.shacl_output_shapes:
                for output_name, shape_uri in sig_def.shacl_output_shapes.items():
                    if output_name in result:
                        self._validate_data_with_shacl(
                            {output_name: result[output_name]}, shape_uri, parser.shacl_graph, f"DSPy Output [{output_name}]"
                        )
            
            # Set results in task data and ensure they propagate to workflow
            if isinstance(result, dict):
                my_task.set_data(**result)
                # Also set in workflow data to ensure propagation
                my_task.workflow.set_data(**result)
                for k, v in result.items():
                    print(f"[DSPy] Set task data: {k}, type: {type(v)}")
            elif dspy_info.get('result'):
                my_task.set_data(**{dspy_info['result']: result})
                # Also set in workflow data to ensure propagation
                my_task.workflow.set_data(**{dspy_info['result']: result})
                print(f"[DSPy] Set task data with result key: {dspy_info['result']} = {result}")
        
        return super()._run_hook(my_task)
    
    def _validate_data_with_shacl(self, data_dict: dict, shape_uri: str, shacl_graph, context: str):
        """Validate DSPy data using SHACL shapes"""
        print(f"INFO: [{context}] Performing SHACL validation against shape: <{shape_uri}>")
        
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
        
        print(f"SUCCESS: [{context}] Data conforms to SHACL contract.")

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
            signature_name = service.attrib.get('signature')
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
            print(f"INFO: Loading SHACL data contract from: {shacl_path}")
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
        
        print(f"INFO: Loading AutoTel project from: {dir_path}")
        
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
            print(f"INFO: Loading DSPy signatures from: {dspy_path}")
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
            print(f"✅ Found embedded DMN definition in BPMN file")
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
            '__doc__': sig_def.description,
            '__module__': 'autotel.workflows.dynamic_signatures',
            '__annotations__': {}
        }
        
        # Add input fields
        for input_name, input_info in sig_def.inputs.items():
            class_attrs[input_name] = dspy.InputField(desc=input_info['description'])
            class_attrs['__annotations__'][input_name] = str  # Use str type for annotations
        
        # Add output fields
        for output_name, output_desc in sig_def.outputs.items():
            class_attrs[output_name] = dspy.OutputField(desc=output_desc)
            class_attrs['__annotations__'][output_name] = str  # Use str type for annotations
        
        if not class_attrs['__annotations__']:
            raise ValueError(f"DSPy signature '{sig_def.name}' must have at least one input or output field")
        
        signature_class = type(sig_def.name, (dspy.Signature,), class_attrs)
        self.dynamic_signatures[sig_def.name] = signature_class
        
        print(f"✅ Created DSPy signature: {sig_def.name}")
        print(f"   Inputs: {list(sig_def.inputs.keys())}")
        print(f"   Outputs: {list(sig_def.outputs.keys())}")
        if sig_def.shacl_input_shapes:
            print(f"   Input SHACL shapes: {sig_def.shacl_input_shapes}")
        if sig_def.shacl_output_shapes:
            print(f"   Output SHACL shapes: {sig_def.shacl_output_shapes}")
    
    def _validate_integration(self):
        """Validate that all four pillars are properly integrated"""
        print("\n🔍 Validating Four Pillars Integration:")
        
        # Check BPMN (Process pillar)
        if not self.loaded_contracts['bpmn_files']:
            print("⚠️  Warning: No BPMN files loaded")
        else:
            print(f"✅ Process pillar: {len(self.loaded_contracts['bpmn_files'])} BPMN files")
        
        # Check DMN (Rules pillar)
        if not self.dmn_parsers:
            print("⚠️  Warning: No DMN definitions found")
        else:
            print(f"✅ Rules pillar: {len(self.dmn_parsers)} DMN definitions")
        
        # Check DSPy (Reasoning pillar)
        if not self.signature_definitions:
            print("⚠️  Warning: No DSPy signatures defined")
        else:
            print(f"✅ Reasoning pillar: {len(self.signature_definitions)} DSPy signatures")
        
        # Check SHACL (Data pillar)
        if len(self.shacl_graph) == 0:
            print("⚠️  Warning: No SHACL shapes loaded")
        else:
            print(f"✅ Data pillar: {len(self.shacl_graph)} SHACL triples")
        
        # Validate cross-references
        self._validate_signature_shape_references()
        
        print("🎯 Four Pillars Integration Complete!\n")
    
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
                        print(f"⚠️  Warning: DSPy signature '{sig_name}' input '{input_name}' references unknown SHACL shape: {shape_uri}")
            
            if sig_def.shacl_output_shapes:
                for output_name, shape_uri in sig_def.shacl_output_shapes.items():
                    if shape_uri not in available_shapes:
                        print(f"⚠️  Warning: DSPy signature '{sig_name}' output '{output_name}' references unknown SHACL shape: {shape_uri}")
    
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