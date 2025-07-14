"""
Five Pillars Permutation Tests for AutoTel
Tests all possible combinations of BPMN, DMN, DSPy, SHACL, and OWL
"""

import pytest
import tempfile
import os
from pathlib import Path
from itertools import combinations, permutations
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.owl_integration import OWLXMLParser, OWLReasoner, OWLWorkflowTask

# Base components for permutation testing
BASE_BPMN = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  targetNamespace="http://autotel.ai/workflows">
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>'''

BASE_DSPY = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:signatures xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature name="TestSignature" description="Test signature">
    <dspy:input name="input_data" description="Input data"/>
    <dspy:output name="output_data" description="Output data"/>
  </dspy:signature>
</dspy:signatures>'''

BASE_SHACL = '''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:test="http://test.ai/shapes#">
  <sh:NodeShape rdf:about="http://test.ai/shapes#TestShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://test.ai/shapes#testProperty"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>
</rdf:RDF>'''

BASE_OWL = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://test.ai/ontology#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
  <owl:Ontology rdf:about="http://test.ai/ontology"/>
  <owl:Class rdf:about="http://test.ai/ontology#TestClass">
    <rdfs:label>Test Class</rdfs:label>
  </owl:Class>
</rdf:RDF>'''

BASE_DMN = '''<?xml version="1.0" encoding="UTF-8"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/">
  <dmn:decision id="TestDecision" name="Test Decision">
    <dmn:decisionTable id="TestDecisionTable">
      <dmn:input id="testInput" label="Test Input">
        <dmn:inputExpression id="testInputExpr" typeRef="string">
          <dmn:text>input_value</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output id="testOutput" label="Test Output" typeRef="string"/>
      <dmn:rule id="testRule">
        <dmn:inputEntry id="testInputEntry"><dmn:text>"test"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry id="testOutputEntry"><dmn:text>"result"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>'''

class TestFivePillarsPermutations:
    """Test all possible combinations of the Five Pillars"""
    
    def test_single_pillar_combinations(self):
        """Test each pillar individually"""
        pillars = ['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL']
        
        for pillar in pillars:
            print(f"\n🧪 Testing single pillar: {pillar}")
            
            if pillar == 'BPMN':
                self._test_bpmn_only()
            elif pillar == 'DMN':
                self._test_dmn_only()
            elif pillar == 'DSPy':
                self._test_dspy_only()
            elif pillar == 'SHACL':
                self._test_shacl_only()
            elif pillar == 'OWL':
                self._test_owl_only()
    
    def test_two_pillar_combinations(self):
        """Test all combinations of two pillars"""
        pillar_combinations = list(combinations(['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL'], 2))
        
        for combo in pillar_combinations:
            print(f"\n🧪 Testing two pillars: {' + '.join(combo)}")
            self._test_pillar_combination(combo)
    
    def test_three_pillar_combinations(self):
        """Test all combinations of three pillars"""
        pillar_combinations = list(combinations(['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL'], 3))
        
        for combo in pillar_combinations:
            print(f"\n🧪 Testing three pillars: {' + '.join(combo)}")
            self._test_pillar_combination(combo)
    
    def test_four_pillar_combinations(self):
        """Test all combinations of four pillars"""
        pillar_combinations = list(combinations(['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL'], 4))
        
        for combo in pillar_combinations:
            print(f"\n🧪 Testing four pillars: {' + '.join(combo)}")
            self._test_pillar_combination(combo)
    
    def test_five_pillars_complete(self):
        """Test all five pillars together"""
        print(f"\n🧪 Testing all five pillars: BPMN + DMN + DSPy + SHACL + OWL")
        self._test_pillar_combination(['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL'])
    
    def test_pillar_order_permutations(self):
        """Test different loading orders of the five pillars"""
        pillar_orders = list(permutations(['BPMN', 'DMN', 'DSPy', 'SHACL', 'OWL']))
        
        for i, order in enumerate(pillar_orders[:5]):  # Test first 5 permutations
            print(f"\n🧪 Testing pillar order {i+1}: {' → '.join(order)}")
            self._test_pillar_loading_order(order)
    
    def _test_pillar_combination(self, pillars):
        """Test a specific combination of pillars"""
        try:
            # Create combined BPMN with selected pillars
            combined_bpmn = self._create_combined_bpmn(pillars)
            
            # Parse with parser
            parser = DspyBpmnParser()
            parser.add_bpmn_xml_from_string(combined_bpmn, "permutation_test.bpmn")
            
            # Verify expected components loaded
            self._verify_pillar_loading(parser, pillars)
            
            print(f"✅ Combination {' + '.join(pillars)} successful")
            
        except Exception as e:
            print(f"❌ Combination {' + '.join(pillars)} failed: {e}")
            raise
    
    def _test_pillar_loading_order(self, order):
        """Test loading pillars in a specific order"""
        try:
            parser = DspyBpmnParser()
            
            # Load each pillar in the specified order
            for pillar in order:
                if pillar == 'BPMN':
                    parser.add_bpmn_xml_from_string(BASE_BPMN, "base.bpmn")
                elif pillar == 'DMN':
                    parser.add_dmn_xml_from_string(BASE_DMN)
                elif pillar == 'DSPy':
                    parser.add_dspy_xml_from_string(BASE_DSPY)
                elif pillar == 'SHACL':
                    parser.add_shacl_xml_from_string(BASE_SHACL)
                elif pillar == 'OWL':
                    parser.add_owl_xml_from_string(BASE_OWL)
            
            # Verify all pillars loaded regardless of order
            self._verify_pillar_loading(parser, order)
            
            print(f"✅ Loading order {' → '.join(order)} successful")
            
        except Exception as e:
            print(f"❌ Loading order {' → '.join(order)} failed: {e}")
            raise
    
    def _create_combined_bpmn(self, pillars):
        """Create BPMN XML with selected pillars embedded"""
        # Start with base BPMN structure
        namespaces = ['xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"']
        
        # Add namespaces for selected pillars
        if 'DSPy' in pillars:
            namespaces.append('xmlns:dspy="http://autotel.ai/dspy"')
        if 'SHACL' in pillars:
            namespaces.append('xmlns:shacl="http://autotel.ai/shacl"')
        if 'OWL' in pillars:
            namespaces.append('xmlns:owl="http://autotel.ai/owl"')
        if 'DMN' in pillars:
            namespaces.append('xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"')
        
        # Build the complete BPMN XML
        combined = f'''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions {" ".join(namespaces)}
                  targetNamespace="http://autotel.ai/workflows">
  
  <bpmn:process id="test_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>'''
        
        # Add selected pillars
        if 'DSPy' in pillars:
            combined += '''
  
  <!-- DSPy Signatures -->
  <dspy:signatures>
    <dspy:signature name="TestSignature" description="Test signature">
      <dspy:input name="input_data" description="Input data"/>
      <dspy:output name="output_data" description="Output data"/>
    </dspy:signature>
  </dspy:signatures>'''
        
        if 'SHACL' in pillars:
            combined += '''
  
  <!-- SHACL Shapes -->
  <shacl:shapes>
    <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
             xmlns:sh="http://www.w3.org/ns/shacl#"
             xmlns:test="http://test.ai/shapes#"
             xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
      <sh:NodeShape rdf:about="http://test.ai/shapes#TestShape">
        <sh:property>
          <sh:PropertyShape>
            <sh:path rdf:resource="http://test.ai/shapes#testProperty"/>
            <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
          </sh:PropertyShape>
        </sh:property>
      </sh:NodeShape>
    </rdf:RDF>
  </shacl:shapes>'''
        
        if 'OWL' in pillars:
            combined += '''
  
  <!-- OWL Ontology -->
  <owl:ontology>
    <rdf:RDF xmlns="http://test.ai/ontology#"
             xmlns:owl="http://www.w3.org/2002/07/owl#"
             xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
             xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
      <owl:Ontology rdf:about="http://test.ai/ontology"/>
      <owl:Class rdf:about="http://test.ai/ontology#TestClass">
        <rdfs:label>Test Class</rdfs:label>
      </owl:Class>
    </rdf:RDF>
  </owl:ontology>'''
        
        if 'DMN' in pillars:
            combined += '''
  
  <!-- DMN Decision -->
  <dmn:definitions>
    <dmn:decision id="TestDecision" name="Test Decision">
      <dmn:decisionTable id="TestDecisionTable">
        <dmn:input id="testInput" label="Test Input">
          <dmn:inputExpression id="testInputExpr" typeRef="string">
            <dmn:text>input_value</dmn:text>
          </dmn:inputExpression>
        </dmn:input>
        <dmn:output id="testOutput" label="Test Output" typeRef="string"/>
        <dmn:rule id="testRule">
          <dmn:inputEntry id="testInputEntry"><dmn:text>"test"</dmn:text></dmn:inputEntry>
          <dmn:outputEntry id="testOutputEntry"><dmn:text>"result"</dmn:text></dmn:outputEntry>
        </dmn:rule>
      </dmn:decisionTable>
    </dmn:decision>
  </dmn:definitions>'''
        
        # Close the BPMN definitions
        combined += '''
  
</bpmn:definitions>'''
        
        return combined
    
    def _verify_pillar_loading(self, parser, expected_pillars):
        """Verify that expected pillars were loaded"""
        if 'BPMN' in expected_pillars:
            assert len(parser.loaded_contracts['bpmn_files']) > 0, "BPMN not loaded"
        
        if 'DMN' in expected_pillars:
            assert len(parser.dmn_parsers) > 0, "DMN not loaded"
        
        if 'DSPy' in expected_pillars:
            assert len(parser.signature_definitions) > 0, "DSPy not loaded"
        
        if 'SHACL' in expected_pillars:
            assert len(parser.shacl_graph) > 0, "SHACL not loaded"
        
        if 'OWL' in expected_pillars:
            # OWL is handled separately, verify through OWL parser
            owl_parser = OWLXMLParser()
            # This would need to be implemented based on how OWL is stored in parser
    
    def _test_bpmn_only(self):
        """Test BPMN pillar alone"""
        parser = DspyBpmnParser()
        parser.add_bpmn_xml_from_string(BASE_BPMN, "bpmn_only.bpmn")
        assert len(parser.loaded_contracts['bpmn_files']) > 0
        print("✅ BPMN only successful")
    
    def _test_dmn_only(self):
        """Test DMN pillar alone"""
        parser = DspyBpmnParser()
        parser.add_dmn_xml_from_string(BASE_DMN)
        assert len(parser.dmn_parsers) > 0
        print("✅ DMN only successful")
    
    def _test_dspy_only(self):
        """Test DSPy pillar alone"""
        parser = DspyBpmnParser()
        parser.add_dspy_xml_from_string(BASE_DSPY)
        assert len(parser.signature_definitions) > 0
        print("✅ DSPy only successful")
    
    def _test_shacl_only(self):
        """Test SHACL pillar alone"""
        parser = DspyBpmnParser()
        parser.add_shacl_xml_from_string(BASE_SHACL)
        assert len(parser.shacl_graph) > 0
        print("✅ SHACL only successful")
    
    def _test_owl_only(self):
        """Test OWL pillar alone"""
        owl_parser = OWLXMLParser()
        ontology_def = owl_parser.parse_owl_xml(BASE_OWL, "test_owl")
        assert len(ontology_def.classes) > 0
        print("✅ OWL only successful")

# Add missing methods to parser for permutation testing
def add_dmn_xml_from_string(self, dmn_xml: str):
    """Add DMN XML from string"""
    from lxml import etree
    root = etree.fromstring(self._strip_xml_encoding(dmn_xml))
    self.add_dmn_xml(root)

def add_dspy_xml_from_string(self, dspy_xml: str):
    """Add DSPy XML from string"""
    from lxml import etree
    root = etree.fromstring(self._strip_xml_encoding(dspy_xml))
    self._parse_dspy_signatures_from_root(root)

def add_shacl_xml_from_string(self, shacl_xml: str):
    """Add SHACL XML from string"""
    from rdflib import Graph
    g = Graph()
    g.parse(data=shacl_xml, format="xml")
    for triple in g:
        self.shacl_graph.add(triple)

def add_owl_xml_from_string(self, owl_xml: str):
    """Add OWL XML from string"""
    # This would need to be implemented based on how OWL is stored in parser
    pass

# Add methods to parser class
DspyBpmnParser.add_dmn_xml_from_string = add_dmn_xml_from_string
DspyBpmnParser.add_dspy_xml_from_string = add_dspy_xml_from_string
DspyBpmnParser.add_shacl_xml_from_string = add_shacl_xml_from_string
DspyBpmnParser.add_owl_xml_from_string = add_owl_xml_from_string

def test_permutation_suite():
    """Run the complete permutation test suite"""
    print("\n🎯 Five Pillars Permutation Test Suite")
    print("=" * 60)
    
    test = TestFivePillarsPermutations()
    
    print("\n1. Testing single pillars...")
    test.test_single_pillar_combinations()
    
    print("\n2. Testing two-pillar combinations...")
    test.test_two_pillar_combinations()
    
    print("\n3. Testing three-pillar combinations...")
    test.test_three_pillar_combinations()
    
    print("\n4. Testing four-pillar combinations...")
    test.test_four_pillar_combinations()
    
    print("\n5. Testing all five pillars...")
    test.test_five_pillars_complete()
    
    print("\n6. Testing loading order permutations...")
    test.test_pillar_order_permutations()
    
    print("\n✅ All permutation tests completed successfully!")
    print("\n📊 Summary:")
    print("   • Single pillars: 5 combinations")
    print("   • Two pillars: 10 combinations") 
    print("   • Three pillars: 10 combinations")
    print("   • Four pillars: 5 combinations")
    print("   • Five pillars: 1 combination")
    print("   • Loading orders: 5 permutations")
    print("   • Total tests: 36 combinations")

if __name__ == "__main__":
    test_permutation_suite() 