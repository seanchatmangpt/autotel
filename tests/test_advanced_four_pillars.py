#!/usr/bin/env python3
"""
Comprehensive Test Suite for Advanced Four Pillars Integration
Tests BPMN + DMN + DSPy + SHACL working together with advanced features
"""

import json
import sys
import tempfile
import shutil
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser, DSPySignatureDefinition
from autotel.utils.advanced_dspy_services import advanced_dspy_registry, advanced_dspy_service
from rdflib import Graph, Namespace, URIRef, Literal
from rdflib.namespace import RDF, XSD, SH
import dspy

def test_four_pillars_parser_initialization():
    """Test the enhanced parser initialization with all four pillars"""
    print("🧪 Testing Four Pillars Parser Initialization")
    print("=" * 50)
    
    parser = DspyBpmnParser()
    
    # Check that all components are initialized
    assert hasattr(parser, 'signature_definitions'), "Parser should have signature_definitions"
    assert hasattr(parser, 'dynamic_signatures'), "Parser should have dynamic_signatures"
    assert hasattr(parser, 'shacl_graph'), "Parser should have shacl_graph"
    assert hasattr(parser, 'loaded_contracts'), "Parser should have loaded_contracts"
    
    # Check loaded contracts structure
    expected_keys = ['bpmn_files', 'dmn_files', 'dspy_signatures', 'shacl_shapes']
    for key in expected_keys:
        assert key in parser.loaded_contracts, f"loaded_contracts should have {key}"
        assert isinstance(parser.loaded_contracts[key], list), f"{key} should be a list"
    
    print("✅ Parser initialization successful")

def test_shacl_integration():
    """Test SHACL shapes loading and validation"""
    print("\n🧪 Testing SHACL Integration")
    print("=" * 30)
    
    parser = DspyBpmnParser()
    
    # Create a simple SHACL shapes file
    shacl_content = """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:autotel="http://autotel.ai/shapes#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">

  <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
        <sh:maxCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:maxCount>
        <sh:pattern>^CUST_[0-9]+$</sh:pattern>
        <sh:message>Customer ID must follow pattern CUST_XXXXX</sh:message>
      </sh:PropertyShape>
    </sh:property>
    
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#creditScore"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
        <sh:minInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">300</sh:minInclusive>
        <sh:maxInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">850</sh:maxInclusive>
        <sh:message>Credit score must be between 300 and 850</sh:message>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>

  <sh:NodeShape rdf:about="http://autotel.ai/shapes#RiskScoreShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#riskLevel"/>
        <sh:in rdf:parseType="Collection">
          <rdf:Description rdf:value="LOW"/>
          <rdf:Description rdf:value="MEDIUM"/>
          <rdf:Description rdf:value="HIGH"/>
        </sh:in>
        <sh:message>Risk level must be LOW, MEDIUM, or HIGH</sh:message>
      </sh:PropertyShape>
    </sh:property>
    
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#confidence"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#decimal"/>
        <sh:minInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#decimal">0.0</sh:minInclusive>
        <sh:maxInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#decimal">1.0</sh:maxInclusive>
        <sh:message>Confidence must be between 0.0 and 1.0</sh:message>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>

</rdf:RDF>"""
    
    # Create temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.shacl.xml', delete=False) as f:
        f.write(shacl_content)
        shacl_file = f.name
    
    try:
        # Load SHACL shapes
        parser.add_shacl_file(shacl_file)
        
        # Verify shapes were loaded
        assert len(parser.shacl_graph) > 0, "SHACL graph should contain shapes"
        assert shacl_file in parser.loaded_contracts['shacl_shapes'], "SHACL file should be tracked"
        
        # Test shape query
        shape_query = """
        PREFIX sh: <http://www.w3.org/ns/shacl#>
        SELECT DISTINCT ?shape WHERE {
            ?shape a sh:NodeShape .
        }
        """
        
        shapes = list(parser.shacl_graph.query(shape_query))
        assert len(shapes) >= 2, "Should find at least 2 shapes (CustomerShape and RiskScoreShape)"
        
        print("✅ SHACL integration successful")
        
    finally:
        # Cleanup
        Path(shacl_file).unlink()

def test_enhanced_dspy_signatures():
    """Test enhanced DSPy signatures with SHACL integration"""
    print("\n🧪 Testing Enhanced DSPy Signatures")
    print("=" * 40)
    
    # Create a BPMN XML with enhanced DSPy signatures
    bpmn_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/workflows">

  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data for risk assessment">
      <dspy:input name="customer_data" description="Customer information" 
                  shaclShape="http://autotel.ai/shapes#CustomerShape"/>
      <dspy:input name="historical_data" description="Historical transaction data" 
                  shaclShape="http://autotel.ai/shapes#TransactionShape"/>
      <dspy:output name="risk_assessment" description="AI-generated risk analysis" 
                   shaclShape="http://autotel.ai/shapes#RiskAssessmentShape"/>
    </dspy:signature>
  </dspy:signatures>

  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
  
</bpmn:definitions>'''
    
    # Create temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(bpmn_xml)
        bpmn_file = f.name
    
    try:
        parser = DspyBpmnParser()
        parser.add_bpmn_file(bpmn_file)
        
        # Check signature definition
        sig_def = parser.get_signature_definition("CustomerAnalysis")
        assert sig_def is not None, "Should find CustomerAnalysis signature"
        assert sig_def.name == "CustomerAnalysis", "Signature name should match"
        assert "customer_data" in sig_def.inputs, "Should have customer_data input"
        assert "risk_assessment" in sig_def.outputs, "Should have risk_assessment output"
        
        # Check SHACL shape references
        assert sig_def.shacl_input_shapes is not None, "Should have SHACL input shapes"
        assert "customer_data" in sig_def.shacl_input_shapes, "customer_data should have SHACL shape"
        assert sig_def.shacl_input_shapes["customer_data"] == "http://autotel.ai/shapes#CustomerShape"
        
        assert sig_def.shacl_output_shapes is not None, "Should have SHACL output shapes"
        assert "risk_assessment" in sig_def.shacl_output_shapes, "risk_assessment should have SHACL shape"
        assert sig_def.shacl_output_shapes["risk_assessment"] == "http://autotel.ai/shapes#RiskAssessmentShape"
        
        # Check dynamic signature creation
        dynamic_sig = parser.get_dynamic_signature("CustomerAnalysis")
        assert dynamic_sig is not None, "Should create dynamic signature class"
        assert issubclass(dynamic_sig, dspy.Signature), "Should be a DSPy signature"
        
        print("✅ Enhanced DSPy signatures successful")
        
    finally:
        # Cleanup
        Path(bpmn_file).unlink()

def test_advanced_dspy_services():
    """Test advanced DSPy services with SHACL validation"""
    print("\n🧪 Testing Advanced DSPy Services")
    print("=" * 35)
    
    # Test registry initialization
    assert advanced_dspy_registry is not None, "Advanced registry should be initialized"
    
    # Test signature registration
    signatures = advanced_dspy_registry.list_signatures()
    expected_signatures = ["customer_risk_analysis", "document_generation", 
                          "data_quality_assessment", "process_optimization"]
    
    for sig_name in expected_signatures:
        assert sig_name in signatures, f"Should have {sig_name} signature"
        sig_info = signatures[sig_name]
        assert sig_info.shacl_input_shapes is not None, f"{sig_name} should have SHACL input shapes"
        assert sig_info.shacl_output_shapes is not None, f"{sig_name} should have SHACL output shapes"
    
            # Add SHACL shapes to the registry for validation
        from rdflib import Graph, Namespace
        from rdflib.namespace import RDF, XSD
        
        shacl_graph = Graph()
        autotel_ns = Namespace("http://autotel.ai/shapes#")
        
        # Add a simple shape for testing
        from rdflib import URIRef
        shacl_graph.add((autotel_ns.CustomerShape, RDF.type, URIRef("http://www.w3.org/ns/shacl#NodeShape")))
        shacl_graph.add((autotel_ns.TransactionHistoryShape, RDF.type, URIRef("http://www.w3.org/ns/shacl#NodeShape")))
        
        advanced_dspy_registry.add_shacl_shapes(shacl_graph)
        
                # Test service calling
        result = advanced_dspy_service("customer_risk_analysis",
                                     customer_profile={"id": "CUST_123", "credit_score": 750},
                                     transaction_history={"transactions": []})
        
        assert "error" not in result, "Should not have error in result"
        assert "risk_score" in result, "Should have risk_score in result"
        assert "risk_factors" in result, "Should have risk_factors in result"
    
    print("✅ Advanced DSPy services successful")

def test_sealed_directory_loading():
    """Test loading a complete project from a sealed directory"""
    print("\n🧪 Testing Sealed Directory Loading")
    print("=" * 40)
    
    # Create a temporary project directory
    with tempfile.TemporaryDirectory() as temp_dir:
        project_dir = Path(temp_dir) / "autotel_project"
        project_dir.mkdir()
        
        # Create SHACL shapes file
        shacl_content = """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:autotel="http://autotel.ai/shapes#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
  <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>
</rdf:RDF>"""
        
        (project_dir / "data_shapes.shacl.xml").write_text(shacl_content)
        
        # Create DMN file
        dmn_content = """<?xml version="1.0" encoding="UTF-8"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/">
  <dmn:decision id="TestDecision" name="Test Decision">
    <dmn:decisionTable id="DecisionTable_1">
      <dmn:input id="input1" label="Input 1">
        <dmn:inputExpression typeRef="string">
          <dmn:text>input_value</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output id="output1" label="Output 1" typeRef="string"/>
      <dmn:rule id="rule1">
        <dmn:inputEntry id="inputEntry1"><dmn:text>"test"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry id="outputEntry1"><dmn:text>"result"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>"""
        
        (project_dir / "business_rules.dmn").write_text(dmn_content)
        
        # Create DSPy signatures file
        dspy_content = """<?xml version="1.0" encoding="UTF-8"?>
<dspy:signatures xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature name="TestAnalysis" description="Test analysis signature">
    <dspy:input name="test_data" description="Test data input"/>
    <dspy:output name="analysis_result" description="Analysis result"/>
  </dspy:signature>
</dspy:signatures>"""
        
        (project_dir / "ai_signatures.dspy.xml").write_text(dspy_content)
        
        # Create main BPMN process
        bpmn_content = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  targetNamespace="http://autotel.ai/workflows">
  <bpmn:process id="main_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>"""
        
        (project_dir / "process.bpmn").write_text(bpmn_content)
        
        # Test loading
        parser = DspyBpmnParser()
        parser.load_from_sealed_directory(project_dir)
        
        # Verify all components loaded
        assert len(parser.loaded_contracts['shacl_shapes']) == 1, "Should load 1 SHACL file"
        assert len(parser.loaded_contracts['dmn_files']) == 1, "Should load 1 DMN file"
        assert len(parser.loaded_contracts['dspy_signatures']) == 1, "Should load 1 DSPy file"
        assert len(parser.loaded_contracts['bpmn_files']) == 1, "Should load 1 BPMN file"
        
        # Verify integration validation ran
        assert len(parser.shacl_graph) > 0, "SHACL graph should be populated"
        assert len(parser.signature_definitions) == 1, "Should have 1 signature definition"
        assert len(parser.dmn_parsers) == 1, "Should have 1 DMN parser"
        
        print("✅ Sealed directory loading successful")

def test_validation_integration():
    """Test validation integration between all four pillars"""
    print("\n🧪 Testing Validation Integration")
    print("=" * 35)
    
    parser = DspyBpmnParser()
    
    # Add SHACL shapes
    shacl_content = """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:autotel="http://autotel.ai/shapes#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
  <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>
</rdf:RDF>"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.shacl.xml', delete=False) as f:
        f.write(shacl_content)
        shacl_file = f.name
    
    try:
        parser.add_shacl_file(shacl_file)
        
        # Add DSPy signature with SHACL reference
        bpmn_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/workflows">
  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
      <dspy:input name="customer_data" description="Customer information" 
                  shaclShape="http://autotel.ai/shapes#CustomerShape"/>
      <dspy:output name="analysis_result" description="Analysis result"/>
    </dspy:signature>
  </dspy:signatures>
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>'''
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
            f.write(bpmn_xml)
            bpmn_file = f.name
        
        try:
            parser.add_bpmn_file(bpmn_file)
            
            # Test validation integration
            sig_def = parser.get_signature_definition("CustomerAnalysis")
            assert sig_def is not None, "Should find signature definition"
            assert sig_def.shacl_input_shapes is not None, "Should have SHACL input shapes"
            assert "customer_data" in sig_def.shacl_input_shapes, "Should have customer_data shape"
            
            # Test shape reference validation
            shapes = parser.get_shacl_shapes_for_signature("CustomerAnalysis")
            assert "inputs" in shapes, "Should have input shapes"
            assert "customer_data" in shapes["inputs"], "Should have customer_data input shape"
            
            print("✅ Validation integration successful")
            
        finally:
            Path(bpmn_file).unlink()
    
    finally:
        Path(shacl_file).unlink()

def test_error_handling():
    """Test error handling in the advanced features"""
    print("\n🧪 Testing Error Handling")
    print("=" * 25)
    
    parser = DspyBpmnParser()
    
    # Test invalid SHACL file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.shacl.xml', delete=False) as f:
        f.write("Invalid XML content")
        invalid_shacl = f.name
    
    try:
        try:
            parser.add_shacl_file(invalid_shacl)
            assert False, "Should raise exception for invalid SHACL"
        except Exception as e:
            assert "Failed to parse SHACL file" in str(e), "Should have appropriate error message"
        
        # Test invalid DSPy signature
        invalid_bpmn = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/workflows">
  <dspy:signatures>
    <dspy:signature name="InvalidSignature" description="Invalid signature">
      <!-- Missing required input/output fields -->
    </dspy:signature>
  </dspy:signatures>
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>'''
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
            f.write(invalid_bpmn)
            invalid_bpmn_file = f.name
        
        try:
            try:
                parser.add_bpmn_file(invalid_bpmn_file)
                assert False, "Should raise exception for invalid signature"
            except Exception as e:
                assert "must have at least one input or output field" in str(e), "Should have appropriate error message"
        finally:
            Path(invalid_bpmn_file).unlink()
        
        print("✅ Error handling successful")
        
    finally:
        Path(invalid_shacl).unlink()

def test_advanced_workflow_execution():
    """Test complete advanced workflow execution"""
    print("\n🧪 Testing Advanced Workflow Execution")
    print("=" * 40)
    
    # Create a complete workflow with all four pillars
    workflow_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  targetNamespace="http://autotel.ai/workflows">

  <dspy:signatures>
    <dspy:signature name="CustomerAnalysis" description="Analyze customer data">
      <dspy:input name="customer_data" description="Customer information"/>
      <dspy:output name="risk_assessment" description="Risk analysis result"/>
    </dspy:signature>
  </dspy:signatures>

  <dmn:definitions>
          <dmn:decision id="ApprovalDecision" name="Approval Decision">
        <dmn:decisionTable id="ApprovalDecisionTable">
          <dmn:input id="riskLevel" label="Risk Level">
            <dmn:inputExpression typeRef="string">
              <dmn:text>risk_assessment</dmn:text>
            </dmn:inputExpression>
          </dmn:input>
          <dmn:output id="approved" label="Approved" typeRef="boolean"/>
          <dmn:rule id="rule1">
            <dmn:inputEntry id="inputEntry1"><dmn:text>"LOW"</dmn:text></dmn:inputEntry>
            <dmn:outputEntry id="outputEntry1"><dmn:text>true</dmn:text></dmn:outputEntry>
          </dmn:rule>
          <dmn:rule id="rule2">
            <dmn:inputEntry id="inputEntry2"><dmn:text>"HIGH"</dmn:text></dmn:inputEntry>
            <dmn:outputEntry id="outputEntry2"><dmn:text>false</dmn:text></dmn:outputEntry>
          </dmn:rule>
        </dmn:decisionTable>
      </dmn:decision>
  </dmn:definitions>

  <bpmn:process id="advanced_workflow" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    
    <bpmn:serviceTask id="analyze_customer" name="Analyze Customer">
      <bpmn:extensionElements>
        <dspy:service name="customer_analyzer" signature="CustomerAnalysis">
          <dspy:param name="customer_data" value="customer_data"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <bpmn:businessRuleTask id="approval_decision" name="Approval Decision" 
                           camunda:decisionRef="ApprovalDecision"/>
    
    <bpmn:exclusiveGateway id="approval_gateway" name="Approved?"/>
    
    <bpmn:endEvent id="end_approved" name="Approved"/>
    <bpmn:endEvent id="end_rejected" name="Rejected"/>
    
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="analyze_customer"/>
    <bpmn:sequenceFlow id="flow2" sourceRef="analyze_customer" targetRef="approval_decision"/>
    <bpmn:sequenceFlow id="flow3" sourceRef="approval_decision" targetRef="approval_gateway"/>
    
    <bpmn:sequenceFlow id="flow_approved" sourceRef="approval_gateway" targetRef="end_approved">
      <bpmn:conditionExpression>approved == true</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="flow_rejected" sourceRef="approval_gateway" targetRef="end_rejected">
      <bpmn:conditionExpression>approved == false</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
  </bpmn:process>
  
</bpmn:definitions>'''
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(workflow_xml)
        workflow_file = f.name
    
    try:
        parser = DspyBpmnParser()
        parser.add_bpmn_file(workflow_file)
        
        # Verify all components loaded
        assert len(parser.signature_definitions) == 1, "Should have 1 DSPy signature"
        assert len(parser.dmn_parsers) == 1, "Should have 1 DMN decision"
        
        # Test workflow specification
        spec = parser.get_spec("advanced_workflow")
        assert spec is not None, "Should get workflow specification"
        
        # Test task specifications
        assert "analyze_customer" in spec.task_specs, "Should have analyze_customer task"
        assert "approval_decision" in spec.task_specs, "Should have approval_decision task"
        
        print("✅ Advanced workflow execution successful")
        
    finally:
        Path(workflow_file).unlink()

def run_all_tests():
    """Run all advanced Four Pillars tests"""
    print("🚀 Running Advanced Four Pillars Integration Tests")
    print("=" * 60)
    
    try:
        test_four_pillars_parser_initialization()
        test_shacl_integration()
        test_enhanced_dspy_signatures()
        test_advanced_dspy_services()
        test_sealed_directory_loading()
        test_validation_integration()
        test_error_handling()
        test_advanced_workflow_execution()
        
        print("\n🎉 All Advanced Four Pillars Tests PASSED!")
        print("=" * 60)
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1) 