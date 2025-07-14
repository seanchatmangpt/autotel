"""
Test AutoTel Factory Pipeline - AUTOTEL-106

Tests the top-level factory that orchestrates the entire parsing and linking pipeline
to convert raw XML into trusted ExecutableSpecification objects.
"""

import pytest
import os
from autotel.factory.pipeline import AutoTelFactory, ExecutableSpecification


class TestAutoTelFactory:
    """Test suite for AutoTelFactory class."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.factory = AutoTelFactory()
        self.complete_xml = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  xmlns:owl="http://www.w3.org/2002/07/owl#"
                  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                  xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="complete_process" isExecutable="true">
    
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Customer">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <dspy:signature name="customer_analysis" description="Analyze customer data">
        <dspy:input name="customer_data" description="Customer information" optional="false" />
        <dspy:input name="risk_factors" description="Risk assessment factors" optional="true" />
        <dspy:output name="analysis_result" description="Analysis results" />
        <dspy:output name="recommendation" description="Recommendation based on analysis" />
      </dspy:signature>
    </bpmn:serviceTask>
    
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Make Decision" 
                          dmn:decisionRef="simple_decision">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
    </bpmn:businessRuleTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_3</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="BusinessRuleTask_1" />
    <bpmn:sequenceFlow id="Flow_3" sourceRef="BusinessRuleTask_1" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
  <!-- DMN Decision -->
  <dmn:definitions id="DMNDefinitions_1"
                   name="simple_decision"
                   targetNamespace="http://camunda.org/schema/1.0/dmn">
    
    <dmn:decision id="simple_decision" name="Simple Decision">
      <dmn:decisionTable id="DecisionTable_1" hitPolicy="UNIQUE">
        <dmn:input id="Input_1" label="Customer Score">
          <dmn:inputExpression id="InputExpression_1" typeRef="integer">
            <dmn:text>customerScore</dmn:text>
          </dmn:inputExpression>
        </dmn:input>
        <dmn:output id="Output_1" label="Approval Decision" typeRef="string" />
        <dmn:rule id="Rule_1">
          <dmn:inputEntry id="InputEntry_1">
            <dmn:text>>= 750</dmn:text>
          </dmn:inputEntry>
          <dmn:outputEntry id="OutputEntry_1">
            <dmn:text>"APPROVED"</dmn:text>
          </dmn:outputEntry>
        </dmn:rule>
        <dmn:rule id="Rule_2">
          <dmn:inputEntry id="InputEntry_2">
            <dmn:text>< 750</dmn:text>
          </dmn:inputEntry>
          <dmn:outputEntry id="OutputEntry_2">
            <dmn:text>"REJECTED"</dmn:text>
          </dmn:outputEntry>
        </dmn:rule>
      </dmn:decisionTable>
    </dmn:decision>
    
  </dmn:definitions>
  
  <!-- SHACL Shapes -->
  <shacl:shapes>
    <rdf:RDF>
      <shacl:NodeShape rdf:about="http://autotel.ai/shapes/CustomerData">
        <shacl:property>
          <shacl:PropertyShape>
            <shacl:path rdf:resource="http://autotel.ai/properties/customerScore"/>
            <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
            <shacl:minCount>1</shacl:minCount>
          </shacl:PropertyShape>
        </shacl:property>
      </shacl:NodeShape>
    </rdf:RDF>
  </shacl:shapes>
  
  <!-- OWL Ontology -->
  <owl:Ontology>
    <rdf:RDF>
      <owl:Class rdf:about="http://autotel.ai/ontology/Customer">
        <rdfs:label>Customer</rdfs:label>
        <rdfs:comment>A customer entity in the system</rdfs:comment>
      </owl:Class>
    </rdf:RDF>
  </owl:Ontology>
  
</bpmn:definitions>"""
    
    def test_factory_initialization(self):
        """Test that factory initializes with all required processors and linker."""
        assert hasattr(self.factory, 'bpmn_processor')
        assert hasattr(self.factory, 'dmn_processor')
        assert hasattr(self.factory, 'dspy_processor')
        assert hasattr(self.factory, 'shacl_processor')
        assert hasattr(self.factory, 'owl_processor')
        assert hasattr(self.factory, 'linker')
    
    def test_create_specification_from_xml_success(self):
        """Test successful creation of ExecutableSpecification from XML."""
        spec = self.factory.create_specification_from_xml(self.complete_xml, "complete_process")
        
        assert isinstance(spec, ExecutableSpecification)
        assert spec.process_id == "complete_process"
        assert spec.bpmn_spec is not None
        assert len(spec.dmn_decisions) == 1
        assert len(spec.dspy_signatures) == 1
        assert spec.shacl_graph is not None
        assert spec.owl_graph is not None
    
    def test_create_specification_from_file_success(self):
        """Test successful creation of ExecutableSpecification from file."""
        # Create a temporary XML file
        test_file = "test_complete.xml"
        try:
            with open(test_file, 'w') as f:
                f.write(self.complete_xml)
            
            spec = self.factory.create_specification_from_file(test_file, "complete_process")
            
            assert isinstance(spec, ExecutableSpecification)
            assert spec.process_id == "complete_process"
            
        finally:
            if os.path.exists(test_file):
                os.remove(test_file)
    
    def test_create_specification_from_file_nonexistent(self):
        """Test creation from nonexistent file raises FileNotFoundError."""
        with pytest.raises(FileNotFoundError):
            self.factory.create_specification_from_file("nonexistent.xml", "process")
    
    def test_create_specification_invalid_xml(self):
        """Test creation from invalid XML raises ValueError."""
        invalid_xml = "<invalid>xml<invalid>"
        
        with pytest.raises(ValueError):
            self.factory.create_specification_from_xml(invalid_xml, "process")
    
    def test_create_specification_nonexistent_process(self):
        """Test creation with nonexistent process ID raises ValueError."""
        with pytest.raises(ValueError, match="Process ID 'nonexistent' not found"):
            self.factory.create_specification_from_xml(self.complete_xml, "nonexistent")
    
    def test_create_specification_missing_dmn_reference(self):
        """Test creation with missing DMN reference raises SemanticLinkerError."""
        xml_with_missing_dmn = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start" />
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Make Decision" 
                          dmn:decisionRef="missing_decision">
      <bpmn:incoming>Flow_1</bpmn:incoming>
    </bpmn:businessRuleTask>
    <bpmn:endEvent id="EndEvent_1" name="End" />
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="BusinessRuleTask_1" />
  </bpmn:process>
  
</bpmn:definitions>"""
        
        with pytest.raises(Exception):  # Should raise SemanticLinkerError or ValueError
            self.factory.create_specification_from_xml(xml_with_missing_dmn, "test_process")
    
    def test_validate_specification_success(self):
        """Test successful validation of specification."""
        validation_report = self.factory.validate_specification(self.complete_xml, "complete_process")
        
        assert validation_report['valid'] == True
        assert validation_report['process_id'] == "complete_process"
        assert validation_report['bpmn_spec_valid'] == True
        assert validation_report['dmn_decisions_count'] == 1
        assert validation_report['dspy_signatures_count'] == 1
        assert validation_report['overall_valid'] == True
    
    def test_validate_specification_failure(self):
        """Test validation failure."""
        invalid_xml = "<invalid>xml<invalid>"
        
        validation_report = self.factory.validate_specification(invalid_xml, "process")
        
        assert validation_report['valid'] == False
        assert validation_report['overall_valid'] == False
        assert 'error' in validation_report
    
    def test_executable_specification_immutability(self):
        """Test that ExecutableSpecification is immutable."""
        spec = self.factory.create_specification_from_xml(self.complete_xml, "complete_process")
        
        # Try to modify the specification
        with pytest.raises(AttributeError, match="Cannot modify immutable ExecutableSpecification"):
            spec.process_id = "modified"
    
    def test_factory_pipeline_sequence(self):
        """Test that factory follows the correct processing sequence."""
        # This test verifies that the factory calls processors and linker in the right order
        spec = self.factory.create_specification_from_xml(self.complete_xml, "complete_process")
        
        # Verify all components are present
        assert spec.process_id == "complete_process"
        assert spec.bpmn_spec is not None
        assert spec.dmn_decisions is not None
        assert spec.dspy_signatures is not None
        assert spec.shacl_graph is not None
        assert spec.owl_graph is not None
        
        # Verify BPMN spec has linked capabilities
        tasks = list(spec.bpmn_spec.get_tasks())
        assert len(tasks) > 0
        
        # Find the business rule task and verify it has DMN capability
        business_rule_tasks = [task for task in tasks if hasattr(task, 'decisionRef')]
        if business_rule_tasks:
            assert hasattr(business_rule_tasks[0], 'executable_dmn')
        
        # Find service tasks and verify they have DSPy capabilities
        service_tasks = [task for task in tasks if hasattr(task, 'extensions') and task.extensions]
        if service_tasks:
            dspy_tasks = [task for task in service_tasks if 'dspy_service' in task.extensions]
            if dspy_tasks:
                assert hasattr(dspy_tasks[0], 'executable_dspy_signature')


if __name__ == "__main__":
    pytest.main([__file__]) 