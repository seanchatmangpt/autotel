"""
Test DSPy Processor - AUTOTEL-103

Tests the isolated DSPy processor that parses custom <dspy:signature> blocks from XML
into simple, non-executable DSPySignatureDefinition data objects.
"""

import pytest
import os
from autotel.factory.processors.dspy_processor import DSPyProcessor, DSPySignatureDefinition


class TestDSPyProcessor:
    """Test suite for DSPyProcessor class."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.processor = DSPyProcessor()
        self.simple_dspy_xml = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature name="customer_analysis" description="Analyze customer data for risk assessment">
    <dspy:input name="customer_data" description="Customer information" optional="false" />
    <dspy:input name="risk_factors" description="Risk assessment factors" optional="true" />
    <dspy:output name="analysis_result" description="Analysis results" />
    <dspy:output name="recommendation" description="Recommendation based on analysis" />
  </dspy:signature>
  
  <dspy:signature name="fraud_detection" description="Detect potential fraud in transactions">
    <dspy:input name="transaction_data" description="Transaction information" optional="false" />
    <dspy:output name="fraud_score" description="Fraud risk score" />
    <dspy:output name="fraud_indicators" description="List of fraud indicators" />
  </dspy:signature>
</root>"""
    
    def test_dspy_processor_integrity(self):
        """Test that DSPyProcessor faithfully translates XML into data objects."""
        signatures = self.processor.parse(self.simple_dspy_xml)
        
        assert isinstance(signatures, list)
        assert len(signatures) == 2
        
        # Check first signature
        sig1 = signatures[0]
        assert isinstance(sig1, DSPySignatureDefinition)
        assert sig1.name == "customer_analysis"
        assert sig1.description == "Analyze customer data for risk assessment"
        assert len(sig1.inputs) == 2
        assert len(sig1.outputs) == 2
        
        # Check inputs
        assert "customer_data" in sig1.inputs
        assert sig1.inputs["customer_data"]["description"] == "Customer information"
        assert sig1.inputs["customer_data"]["optional"] == "False"  # Case sensitive
        
        assert "risk_factors" in sig1.inputs
        assert sig1.inputs["risk_factors"]["description"] == "Risk assessment factors"
        assert sig1.inputs["risk_factors"]["optional"] == "True"  # Case sensitive
        
        # Check outputs
        assert "analysis_result" in sig1.outputs
        assert sig1.outputs["analysis_result"] == "Analysis results"
        assert "recommendation" in sig1.outputs
        assert sig1.outputs["recommendation"] == "Recommendation based on analysis"
        
        # Check second signature
        sig2 = signatures[1]
        assert isinstance(sig2, DSPySignatureDefinition)
        assert sig2.name == "fraud_detection"
        assert sig2.description == "Detect potential fraud in transactions"
        assert len(sig2.inputs) == 1
        assert len(sig2.outputs) == 2
    
    def test_parse_valid_dspy(self):
        """Test parsing valid DSPy XML."""
        signatures = self.processor.parse(self.simple_dspy_xml)
        
        assert isinstance(signatures, list)
        assert len(signatures) == 2
        
        # Verify all signatures are DSPySignatureDefinition objects
        for sig in signatures:
            assert isinstance(sig, DSPySignatureDefinition)
            assert hasattr(sig, 'name')
            assert hasattr(sig, 'description')
            assert hasattr(sig, 'inputs')
            assert hasattr(sig, 'outputs')
    
    def test_parse_missing_name_attribute(self):
        """Test parsing signature without required name attribute raises ValueError."""
        xml_missing_name = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature description="Missing name attribute">
    <dspy:input name="input1" description="Test input" />
    <dspy:output name="output1" description="Test output" />
  </dspy:signature>
</root>"""
        
        with pytest.raises(ValueError, match="DSPy signature is missing required 'name' attribute"):
            self.processor.parse(xml_missing_name)
    
    def test_parse_invalid_xml(self):
        """Test parsing invalid XML raises ValueError."""
        invalid_xml = "<invalid>xml<invalid>"
        
        with pytest.raises(ValueError, match="Invalid XML format"):
            self.processor.parse(invalid_xml)
    
    def test_parse_no_signatures(self):
        """Test parsing XML with no DSPy signatures returns empty list."""
        xml_without_signatures = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:dspy="http://autotel.ai/dspy">
  <some_element>No DSPy signatures here</some_element>
</root>"""
        
        signatures = self.processor.parse(xml_without_signatures)
        
        assert isinstance(signatures, list)
        assert len(signatures) == 0
    
    def test_parse_file_valid(self):
        """Test parsing from file."""
        # Create a temporary DSPy file
        test_file = "test_simple.dspy.xml"
        try:
            with open(test_file, 'w') as f:
                f.write(self.simple_dspy_xml)
            
            signatures = self.processor.parse_file(test_file)
            
            assert isinstance(signatures, list)
            assert len(signatures) == 2
            
        finally:
            if os.path.exists(test_file):
                os.remove(test_file)
    
    def test_parse_file_nonexistent(self):
        """Test parsing nonexistent file raises FileNotFoundError."""
        with pytest.raises(FileNotFoundError):
            self.processor.parse_file("nonexistent.dspy.xml")
    
    def test_find_signatures_in_bpmn(self):
        """Test finding DSPy signatures embedded in BPMN XML."""
        bpmn_with_dspy = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start" />
    <bpmn:serviceTask id="ServiceTask_1" name="Test Service">
      <dspy:signature name="embedded_signature" description="Embedded in BPMN">
        <dspy:input name="input1" description="Test input" />
        <dspy:output name="output1" description="Test output" />
      </dspy:signature>
    </bpmn:serviceTask>
    <bpmn:endEvent id="EndEvent_1" name="End" />
  </bpmn:process>
  
</bpmn:definitions>"""
        
        signatures = self.processor.find_signatures_in_bpmn(bpmn_with_dspy)
        
        assert isinstance(signatures, list)
        assert len(signatures) == 1
        
        sig = signatures[0]
        assert isinstance(sig, DSPySignatureDefinition)
        assert sig.name == "embedded_signature"
        assert sig.description == "Embedded in BPMN"
    
    def test_find_signatures_in_malformed_bpmn(self):
        """Test finding signatures in malformed BPMN returns empty list."""
        malformed_bpmn = "<invalid>bpmn<invalid>"
        
        signatures = self.processor.find_signatures_in_bpmn(malformed_bpmn)
        
        assert isinstance(signatures, list)
        assert len(signatures) == 0
    
    def test_processor_isolation(self):
        """Test that DSPyProcessor is completely isolated from other DSLs."""
        # Verify processor doesn't have any other DSL parsing methods
        assert not hasattr(self.processor, 'parse_bpmn_processes')
        assert not hasattr(self.processor, 'parse_dmn_decisions')
        assert not hasattr(self.processor, 'parse_shacl_shapes')
        assert not hasattr(self.processor, 'parse_owl_ontologies')
    
    def test_dspy_signature_definition_dataclass(self):
        """Test DSPySignatureDefinition dataclass functionality."""
        sig = DSPySignatureDefinition(
            name="test_signature",
            description="Test signature",
            inputs={"input1": {"description": "Test input", "optional": "false"}},
            outputs={"output1": "Test output"}
        )
        
        assert sig.name == "test_signature"
        assert sig.description == "Test signature"
        assert len(sig.inputs) == 1
        assert len(sig.outputs) == 1
        assert "input1" in sig.inputs
        assert "output1" in sig.outputs


if __name__ == "__main__":
    pytest.main([__file__]) 