"""
Test BPMN Processor - AUTOTEL-101

Tests the isolated BPMN processor that exclusively uses SpiffWorkflow.camunda.parser.CamundaParser
for parsing standard BPMN 2.0 elements, completely ignoring all custom DSLs.
"""

import pytest
import os
from autotel.factory.processors.bpmn_processor import BPMNProcessor
from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.specs import WorkflowSpec


class TestBPMNProcessor:
    """Test suite for BPMNProcessor class."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.processor = BPMNProcessor()
        self.simple_bpmn_xml = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="simple_process" isExecutable="true">
    
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Data">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:serviceTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
</bpmn:definitions>"""
    
    def test_bpmn_processor_conformance(self):
        """Test that BPMNProcessor output is identical to raw SpiffWorkflow library."""
        # Test with our processor
        our_spec = self.processor.parse(self.simple_bpmn_xml, "simple_process")
        
        # Test with raw SpiffWorkflow library
        raw_parser = CamundaParser()
        from lxml import etree
        bpmn_tree = etree.fromstring(self.simple_bpmn_xml.encode('utf-8'))
        raw_parser.add_bpmn_xml(bpmn_tree)
        raw_spec = raw_parser.get_spec("simple_process")
        
        # Verify both specs are valid WorkflowSpec objects
        assert isinstance(our_spec, WorkflowSpec)
        assert isinstance(raw_spec, WorkflowSpec)
        
        # Verify they have the same process ID
        assert our_spec.name == raw_spec.name
        assert our_spec.name == "simple_process"
        
        # Verify they have the same task specs (SpiffWorkflow creates additional tasks)
        assert len(our_spec.task_specs) == len(raw_spec.task_specs)
        
        # Verify they have the same task types
        our_task_types = {name: type(spec) for name, spec in our_spec.task_specs.items()}
        raw_task_types = {name: type(spec) for name, spec in raw_spec.task_specs.items()}
        assert our_task_types == raw_task_types
    
    def test_parse_valid_bpmn(self):
        """Test parsing valid BPMN XML."""
        spec = self.processor.parse(self.simple_bpmn_xml, "simple_process")
        
        assert isinstance(spec, WorkflowSpec)
        assert spec.name == "simple_process"
        assert len(spec.task_specs) == 6  # SpiffWorkflow creates additional tasks (Start, End, etc.)
    
    def test_parse_invalid_xml(self):
        """Test parsing invalid XML raises ValueError."""
        invalid_xml = "<invalid>xml</invalid>"
        
        with pytest.raises(ValueError, match="Failed to parse BPMN XML"):
            self.processor.parse(invalid_xml, "simple_process")
    
    def test_parse_nonexistent_process_id(self):
        """Test parsing with nonexistent process ID raises ValueError."""
        with pytest.raises(ValueError, match="Failed to parse BPMN XML"):
            self.processor.parse(self.simple_bpmn_xml, "nonexistent")
    
    def test_parse_file_valid(self):
        """Test parsing from file."""
        # Create a temporary BPMN file
        test_file = "test_simple.bpmn"
        try:
            with open(test_file, 'w') as f:
                f.write(self.simple_bpmn_xml)
            
            spec = self.processor.parse_file(test_file, "simple_process")
            
            assert isinstance(spec, WorkflowSpec)
            assert spec.name == "simple_process"
            
        finally:
            if os.path.exists(test_file):
                os.remove(test_file)
    
    def test_parse_file_nonexistent(self):
        """Test parsing nonexistent file raises FileNotFoundError."""
        with pytest.raises(FileNotFoundError):
            self.processor.parse_file("nonexistent.bpmn", "simple_process")
    
    def test_ignores_custom_dsls(self):
        """Test that BPMNProcessor ignores custom DSLs and only parses standard BPMN."""
        bpmn_with_custom_dsl = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="test_process" isExecutable="true">
    
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="Test Service">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <dspy:signature name="test_signature">
        <dspy:input name="input1" description="Test input" />
        <dspy:output name="output1" description="Test output" />
      </dspy:signature>
    </bpmn:serviceTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
  <shacl:shapes>
    <rdf:RDF>
      <!-- SHACL shapes would go here -->
    </rdf:RDF>
  </shacl:shapes>
  
</bpmn:definitions>"""
        
        # Should parse successfully despite custom DSLs
        spec = self.processor.parse(bpmn_with_custom_dsl, "test_process")
        
        assert isinstance(spec, WorkflowSpec)
        assert spec.name == "test_process"
        assert len(spec.task_specs) == 6  # SpiffWorkflow creates additional tasks
    
    def test_processor_isolation(self):
        """Test that BPMNProcessor is completely isolated from other DSLs."""
        # Verify processor only uses CamundaParser
        assert isinstance(self.processor.parser, CamundaParser)
        
        # Verify processor doesn't have any custom DSL parsing methods
        assert not hasattr(self.processor, 'parse_dspy_signatures')
        assert not hasattr(self.processor, 'parse_dmn_definitions')
        assert not hasattr(self.processor, 'parse_shacl_shapes')
        assert not hasattr(self.processor, 'parse_owl_ontologies')


if __name__ == "__main__":
    pytest.main([__file__]) 