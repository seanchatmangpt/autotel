"""
Factory Boy factories for AutoTel test data generation

This module provides factories for generating test data dynamically,
eliminating the need for hardcoded values in tests.
"""

import factory
import uuid
import time
from pathlib import Path
from typing import Dict, List, Any, Optional
from dataclasses import dataclass

from autotel.factory.processors.dspy_processor import DSPySignatureDefinition
from autotel.schemas.sparql_types import SPARQLQueryDefinition, SPARQLQueryTemplate


class BPMNXMLFactory(factory.Factory):
    """Factory for generating BPMN XML content"""
    
    class Meta:
        model = dict
    
    process_id = factory.Faker('uuid4')
    process_name = factory.Faker('word')
    task_count = factory.Faker('random_int', min=1, max=5)
    include_gateways = factory.Faker('boolean')
    include_events = factory.Faker('boolean')
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate BPMN XML content dynamically"""
        tasks = []
        flows = []
        current_id = 1
        
        # Start event
        start_event_id = f"StartEvent_{current_id}"
        current_id += 1
        
        # Generate tasks
        for i in range(self.task_count):
            task_id = f"Task_{current_id}"
            tasks.append(f'    <bpmn:task id="{task_id}" name="{factory.Faker("word")}">')
            tasks.append(f'      <bpmn:incoming>Flow_{current_id-1}</bpmn:incoming>')
            tasks.append(f'      <bpmn:outgoing>Flow_{current_id}</bpmn:outgoing>')
            tasks.append('    </bpmn:task>')
            
            flows.append(f'    <bpmn:sequenceFlow id="Flow_{current_id-1}" sourceRef="{start_event_id if i == 0 else f"Task_{current_id-1}"}" targetRef="{task_id}" />')
            current_id += 1
        
        # End event
        end_event_id = f"EndEvent_{current_id}"
        tasks.append(f'    <bpmn:endEvent id="{end_event_id}" name="End">')
        tasks.append(f'      <bpmn:incoming>Flow_{current_id-1}</bpmn:incoming>')
        tasks.append('    </bpmn:endEvent>')
        flows.append(f'    <bpmn:sequenceFlow id="Flow_{current_id-1}" sourceRef="Task_{current_id-2}" targetRef="{end_event_id}" />')
        
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_{factory.Faker('random_int', min=1, max=1000)}"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="{self.process_id}" name="{self.process_name}" isExecutable="true">
    <bpmn:startEvent id="{start_event_id}" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
{chr(10).join(tasks)}
{chr(10).join(flows)}
  </bpmn:process>
</bpmn:definitions>'''


class DMNXMLFactory(factory.Factory):
    """Factory for generating DMN XML content"""
    
    class Meta:
        model = dict
    
    decision_id = factory.Faker('uuid4')
    decision_name = factory.Faker('word')
    input_count = factory.Faker('random_int', min=1, max=3)
    output_count = factory.Faker('random_int', min=1, max=2)
    rule_count = factory.Faker('random_int', min=1, max=5)
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate DMN XML content dynamically"""
        inputs = []
        outputs = []
        rules = []
        
        # Generate inputs
        for i in range(self.input_count):
            input_id = f"Input_{i+1}"
            inputs.append(f'''      <input id="{input_id}" label="{factory.Faker('word')}">
        <inputExpression id="InputExpression_{i+1}" typeRef="string">
          <text>input_{i+1}</text>
        </inputExpression>
      </input>''')
        
        # Generate outputs
        for i in range(self.output_count):
            output_id = f"Output_{i+1}"
            outputs.append(f'      <output id="{output_id}" label="{factory.Faker("word")}" typeRef="string" />')
        
        # Generate rules
        for i in range(self.rule_count):
            rule_id = f"Rule_{i+1}"
            input_entries = []
            output_entries = []
            
            for j in range(self.input_count):
                input_entries.append(f'''        <inputEntry id="InputEntry_{i+1}_{j+1}">
          <text>"{factory.Faker('word')}"</text>
        </inputEntry>''')
            
            for j in range(self.output_count):
                output_entries.append(f'''        <outputEntry id="OutputEntry_{i+1}_{j+1}">
          <text>"{factory.Faker('word')}"</text>
        </outputEntry>''')
            
            rules.append(f'''      <rule id="{rule_id}">
{chr(10).join(input_entries)}
{chr(10).join(output_entries)}
      </rule>''')
        
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/"
             xmlns:dmndi="https://www.omg.org/spec/DMN/20191111/DMNDI/"
             xmlns:dc="http://www.omg.org/spec/DD/20100524/DC/"
             xmlns:di="http://www.omg.org/spec/DD/20100524/DI/"
             id="Definitions_{factory.Faker('random_int', min=1, max=1000)}"
             name="DRD_{factory.Faker('random_int', min=1, max=1000)}"
             targetNamespace="http://camunda.org/schema/1.0/dmn">
  <decision id="{self.decision_id}" name="{self.decision_name}">
    <decisionTable id="DecisionTable_{factory.Faker('random_int', min=1, max=1000)}" hitPolicy="UNIQUE">
{chr(10).join(inputs)}
{chr(10).join(outputs)}
{chr(10).join(rules)}
    </decisionTable>
  </decision>
</definitions>'''


class OWLXMLFactory(factory.Factory):
    """Factory for generating OWL XML content"""
    
    class Meta:
        model = dict
    
    ontology_uri = factory.Faker('url')
    class_count = factory.Faker('random_int', min=2, max=5)
    property_count = factory.Faker('random_int', min=1, max=3)
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate OWL XML content dynamically"""
        classes = []
        properties = []
        
        # Generate classes
        for i in range(self.class_count):
            class_name = factory.Faker('word')
            classes.append(f'''    <owl:Class rdf:about="#{class_name}">
        <rdfs:label>{class_name}</rdfs:label>
        <rdfs:comment>{factory.Faker('sentence')}</rdfs:comment>
    </owl:Class>''')
        
        # Generate properties
        for i in range(self.property_count):
            prop_name = factory.Faker('word')
            properties.append(f'''    <owl:ObjectProperty rdf:about="#{prop_name}">
        <rdfs:label>{prop_name}</rdfs:label>
        <rdfs:comment>{factory.Faker('sentence')}</rdfs:comment>
    </owl:ObjectProperty>''')
        
        return f'''<?xml version="1.0"?>
<rdf:RDF xmlns="{self.ontology_uri}#"
         xml:base="{self.ontology_uri}"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:xml="http://www.w3.org/XML/1998/namespace"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Ontology rdf:about="{self.ontology_uri}"/>
{chr(10).join(classes)}
{chr(10).join(properties)}
</rdf:RDF>'''


class SHACLXMLFactory(factory.Factory):
    """Factory for generating SHACL XML content"""
    
    class Meta:
        model = dict
    
    shape_count = factory.Faker('random_int', min=1, max=3)
    property_count = factory.Faker('random_int', min=1, max=3)
    ontology_uri = factory.Faker('url')
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate SHACL XML content dynamically"""
        shapes = []
        
        for i in range(self.shape_count):
            shape_name = factory.Faker('word')
            properties = []
            
            for j in range(self.property_count):
                prop_name = factory.Faker('word')
                properties.append(f'''            <sh:property>
                <sh:PropertyShape>
                    <sh:path rdf:resource="{self.ontology_uri}#{prop_name}"/>
                    <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                    <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">{factory.Faker('random_int', min=0, max=1)}</sh:minCount>
                </sh:PropertyShape>
            </sh:property>''')
            
            shapes.append(f'''    <sh:NodeShape rdf:about="#{shape_name}Shape">
        <sh:targetClass rdf:resource="{self.ontology_uri}#{shape_name}"/>
{chr(10).join(properties)}
    </sh:NodeShape>''')
        
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
{chr(10).join(shapes)}
</rdf:RDF>'''


class DSPyXMLFactory(factory.Factory):
    """Factory for generating DSPy XML content"""
    
    class Meta:
        model = dict
    
    signature_count = factory.Faker('random_int', min=1, max=3)
    input_count = factory.Faker('random_int', min=1, max=3)
    output_count = factory.Faker('random_int', min=1, max=2)
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate DSPy XML content dynamically"""
        signatures = []
        
        for i in range(self.signature_count):
            signature_name = factory.Faker('word')
            inputs = []
            outputs = []
            
            # Generate inputs
            for j in range(self.input_count):
                input_name = factory.Faker('word')
                inputs.append(f'''    <dspy:input name="{input_name}" description="{factory.Faker('sentence')}" optional="{factory.Faker('boolean')}" />''')
            
            # Generate outputs
            for j in range(self.output_count):
                output_name = factory.Faker('word')
                outputs.append(f'''    <dspy:output name="{output_name}" description="{factory.Faker('sentence')}" />''')
            
            signatures.append(f'''  <dspy:signature name="{signature_name}" description="{factory.Faker('sentence')}">
{chr(10).join(inputs)}
{chr(10).join(outputs)}
  </dspy:signature>''')
        
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:dspy="http://autotel.ai/dspy">
{chr(10).join(signatures)}
</root>'''


class DSPySignatureDefinitionFactory(factory.Factory):
    """Factory for generating DSPySignatureDefinition objects"""
    
    class Meta:
        model = DSPySignatureDefinition
    
    name = factory.Faker('word')
    description = factory.Faker('sentence')
    input_count = factory.Faker('random_int', min=1, max=3)
    output_count = factory.Faker('random_int', min=1, max=2)
    
    @factory.lazy_attribute
    def inputs(self):
        """Generate dynamic inputs"""
        inputs = {}
        for i in range(self.input_count):
            input_name = factory.Faker('word')
            inputs[input_name] = {
                "description": factory.Faker('sentence'),
                "optional": str(factory.Faker('boolean'))
            }
        return inputs
    
    @factory.lazy_attribute
    def outputs(self):
        """Generate dynamic outputs"""
        outputs = {}
        for i in range(self.output_count):
            output_name = factory.Faker('word')
            outputs[output_name] = factory.Faker('sentence')
        return outputs


class SPARQLXMLFactory(factory.Factory):
    """Factory for generating SPARQL XML content"""
    
    class Meta:
        model = dict
    
    query_count = factory.Faker('random_int', min=1, max=3)
    template_count = factory.Faker('random_int', min=1, max=2)
    prefix_count = factory.Faker('random_int', min=1, max=3)
    
    @factory.lazy_attribute
    def xml_content(self):
        """Generate SPARQL XML content dynamically"""
        queries = []
        templates = []
        prefixes = []
        
        # Generate prefixes
        for i in range(self.prefix_count):
            prefix_name = factory.Faker('word').evaluate(None, None, {'locale': None})
            prefix_uri = factory.Faker('url').evaluate(None, None, {'locale': None})
            prefixes.append(f'''    <sparql:prefix name="{prefix_name.lower()}" uri="{prefix_uri}" />''')
        
        # Generate queries
        for i in range(self.query_count):
            query_name = factory.Faker('word').evaluate(None, None, {'locale': None})
            query_type = factory.Faker('random_element', elements=['SELECT', 'ASK', 'CONSTRUCT', 'DESCRIBE']).evaluate(None, None, {'locale': None})
            query_text = f"{query_type} ?s ?p ?o WHERE {{ ?s ?p ?o }}"
            
            queries.append(f'''  <sparql:query name="{query_name}" description="{factory.Faker('sentence').evaluate(None, None, {'locale': None})}">
    {query_text}
    <sparql:parameter name="limit" type="integer" required="false" default="100" />
  </sparql:query>''')
        
        # Generate templates
        for i in range(self.template_count):
            template_name = factory.Faker('word').evaluate(None, None, {'locale': None})
            template_text = "SELECT ?s ?p ?o WHERE { ?s ?p ?o . FILTER(?s = ?subject) }"
            
            templates.append(f'''  <sparql:template name="{template_name}" description="{factory.Faker('sentence').evaluate(None, None, {'locale': None})}">
    {template_text}
    <sparql:validation type="regex" pattern="^[a-zA-Z][a-zA-Z0-9]*$" message="Subject must be alphanumeric" />
    <sparql:example>
      <subject>http://example.com/resource</subject>
    </sparql:example>
  </sparql:template>''')
        
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:sparql="http://autotel.ai/sparql">
{chr(10).join(prefixes)}
{chr(10).join(queries)}
{chr(10).join(templates)}
</root>'''


class SPARQLQueryDefinitionFactory(factory.Factory):
    """Factory for generating SPARQLQueryDefinition objects"""
    
    class Meta:
        model = SPARQLQueryDefinition
    
    name = factory.Faker('word')
    description = factory.Faker('sentence')
    query_type = factory.Faker('random_element', elements=['SELECT', 'ASK', 'CONSTRUCT', 'DESCRIBE'])
    
    @factory.lazy_attribute
    def query(self):
        """Generate dynamic SPARQL query"""
        return f"{self.query_type} ?s ?p ?o WHERE {{ ?s ?p ?o }}"
    
    @factory.lazy_attribute
    def parameters(self):
        """Generate dynamic parameters"""
        parameters = {}
        param_count = factory.Faker('random_int', min=0, max=3).evaluate(None, None, {'locale': None})
        for i in range(param_count):
            param_name = factory.Faker('word').evaluate(None, None, {'locale': None})
            parameters[param_name] = {
                "type": factory.Faker('random_element', elements=['string', 'integer', 'boolean']).evaluate(None, None, {'locale': None}),
                "required": factory.Faker('boolean').evaluate(None, None, {'locale': None}),
                "default": factory.Faker('word').evaluate(None, None, {'locale': None}) if factory.Faker('boolean').evaluate(None, None, {'locale': None}) else None
            }
        return parameters
    
    @factory.lazy_attribute
    def prefixes(self):
        """Generate dynamic prefixes"""
        prefixes = {}
        prefix_count = factory.Faker('random_int', min=0, max=2).evaluate(None, None, {'locale': None})
        for i in range(prefix_count):
            prefix_name = factory.Faker('word').evaluate(None, None, {'locale': None}).lower()
            prefix_uri = factory.Faker('url').evaluate(None, None, {'locale': None})
            prefixes[prefix_name] = prefix_uri
        return prefixes


class SPARQLQueryTemplateFactory(factory.Factory):
    """Factory for generating SPARQLQueryTemplate objects"""
    
    class Meta:
        model = SPARQLQueryTemplate
    
    name = factory.Faker('word')
    description = factory.Faker('sentence')
    
    @factory.lazy_attribute
    def template(self):
        """Generate dynamic SPARQL template"""
        param_count = factory.Faker('random_int', min=1, max=3).evaluate(None, None, {'locale': None})
        params = []
        for i in range(param_count):
            param_name = factory.Faker('word').evaluate(None, None, {'locale': None})
            params.append(f"?{param_name}")
        param_str = " ".join(params)
        return f"SELECT {param_str} WHERE {{ ?s ?p ?o . FILTER(?s = ?subject) }}"
    
    @factory.lazy_attribute
    def parameters(self):
        """Generate dynamic parameter names"""
        param_count = factory.Faker('random_int', min=1, max=3).evaluate(None, None, {'locale': None})
        return [factory.Faker('word').evaluate(None, None, {'locale': None}) for _ in range(param_count)]
    
    @factory.lazy_attribute
    def validation_rules(self):
        """Generate dynamic validation rules"""
        rules = []
        validation_count = factory.Faker('random_int', min=0, max=2).evaluate(None, None, {'locale': None})
        for i in range(validation_count):
            rules.append({
                "type": factory.Faker('random_element', elements=['regex', 'range', 'enum']).evaluate(None, None, {'locale': None}),
                "pattern": factory.Faker('word').evaluate(None, None, {'locale': None}),
                "message": factory.Faker('sentence').evaluate(None, None, {'locale': None})
            })
        return rules
    
    @factory.lazy_attribute
    def examples(self):
        """Generate dynamic examples"""
        examples = []
        example_count = factory.Faker('random_int', min=0, max=2).evaluate(None, None, {'locale': None})
        param_count = factory.Faker('random_int', min=1, max=3).evaluate(None, None, {'locale': None})
        for i in range(example_count):
            example = {}
            for j in range(param_count):
                param_name = factory.Faker('word').evaluate(None, None, {'locale': None})
                example[param_name] = factory.Faker('word').evaluate(None, None, {'locale': None})
            examples.append(example)
        return examples


class TelemetryDataFactory(factory.Factory):
    """Factory for generating telemetry data"""
    
    class Meta:
        model = dict
    
    operation_name = factory.Faker('word')
    component_name = factory.Faker('word')
    attribute_count = factory.Faker('random_int', min=2, max=5)
    
    @factory.lazy_attribute
    def trace_id(self):
        return str(uuid.uuid4())
    
    @factory.lazy_attribute
    def span_id(self):
        return str(uuid.uuid4())
    
    @factory.lazy_attribute
    def timestamp(self):
        return int(time.time())
    
    @factory.lazy_attribute
    def attributes(self):
        """Generate dynamic attributes"""
        attrs = {
            "component": self.component_name,
            "operation": self.operation_name
        }
        
        for i in range(self.attribute_count):
            key = factory.Faker('word')
            value = factory.Faker('word')
            attrs[key] = value
        
        return attrs


class WorkflowContextFactory(factory.Factory):
    """Factory for generating workflow context data"""
    
    class Meta:
        model = dict
    
    context_size = factory.Faker('random_int', min=3, max=8)
    include_nested = factory.Faker('boolean')
    
    @factory.lazy_attribute
    def context_data(self):
        """Generate dynamic workflow context"""
        context = {
            "workflow_id": str(uuid.uuid4()),
            "timestamp": int(time.time()),
            "status": factory.Faker('random_element', elements=['pending', 'running', 'completed', 'failed'])
        }
        
        for i in range(self.context_size):
            key = factory.Faker('word')
            if self.include_nested and factory.Faker('boolean'):
                # Generate nested structure
                context[key] = {
                    "value": factory.Faker('word'),
                    "metadata": {
                        "type": factory.Faker('word'),
                        "version": factory.Faker('random_int', min=1, max=10)
                    }
                }
            else:
                context[key] = factory.Faker('word')
        
        return context


class TestFileFactory(factory.Factory):
    """Factory for generating test file paths and content"""
    
    class Meta:
        model = dict
    
    file_type = factory.Faker('random_element', elements=['bpmn', 'dmn', 'owl', 'shacl', 'dspy', 'sparql'])
    directory = factory.LazyFunction(lambda: Path.cwd() / "test_files")
    
    @factory.lazy_attribute
    def filename(self):
        """Generate filename based on type"""
        return f"test_{factory.Faker('word')}.{self.file_type}"
    
    @factory.lazy_attribute
    def file_path(self):
        """Generate full file path"""
        return self.directory / self.filename
    
    @factory.lazy_attribute
    def content(self):
        """Generate content based on file type"""
        if self.file_type == 'bpmn':
            return BPMNXMLFactory().xml_content
        elif self.file_type == 'dmn':
            return DMNXMLFactory().xml_content
        elif self.file_type == 'owl':
            return OWLXMLFactory().xml_content
        elif self.file_type == 'shacl':
            return SHACLXMLFactory().xml_content
        elif self.file_type == 'dspy':
            return DSPyXMLFactory().xml_content
        elif self.file_type == 'sparql':
            return SPARQLXMLFactory().xml_content
        else:
            return f"# Test {self.file_type} file\n{factory.Faker('text')}"


# Convenience functions for common test scenarios
def create_bpmn_test_data(process_count: int = 1) -> List[str]:
    """Create multiple BPMN test files"""
    return [BPMNXMLFactory().xml_content for _ in range(process_count)]


def create_dmn_test_data(decision_count: int = 1) -> List[str]:
    """Create multiple DMN test files"""
    return [DMNXMLFactory().xml_content for _ in range(decision_count)]


def create_owl_test_data(ontology_count: int = 1) -> List[str]:
    """Create multiple OWL test files"""
    return [OWLXMLFactory().xml_content for _ in range(ontology_count)]


def create_shacl_test_data(shape_count: int = 1) -> List[str]:
    """Create multiple SHACL test files"""
    return [SHACLXMLFactory().xml_content for _ in range(shape_count)]


def create_dspy_test_data(signature_count: int = 1) -> List[str]:
    """Create multiple DSPy test files"""
    return [DSPyXMLFactory().xml_content for _ in range(signature_count)]


def create_sparql_test_data(query_count: int = 1) -> List[str]:
    """Create multiple SPARQL test files"""
    return [SPARQLXMLFactory().xml_content for _ in range(query_count)]


def create_comprehensive_test_suite() -> Dict[str, Any]:
    """Create a comprehensive test suite with all data types"""
    return {
        "bpmn": create_bpmn_test_data(3),
        "dmn": create_dmn_test_data(2),
        "owl": create_owl_test_data(2),
        "shacl": create_shacl_test_data(2),
        "dspy": create_dspy_test_data(3),
        "sparql": create_sparql_test_data(2),
        "telemetry": TelemetryDataFactory(),
        "workflow_context": WorkflowContextFactory(),
        "test_files": [TestFileFactory() for _ in range(5)]
    } 