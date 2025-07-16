#!/usr/bin/env python3
"""
XML Processing Capabilities Test

Tests the complete XML processing pipeline for DSPy signatures from OWL and SHACL:
1. OWL XML parsing and ontology extraction
2. SHACL XML parsing and constraint extraction  
3. DSPy XML parsing and signature extraction
4. Integration and compilation
5. End-to-end execution with telemetry validation
"""

import pytest
import json
import sys
from pathlib import Path
from typing import Dict, Any

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.ontology_compiler import OntologyCompiler
from autotel.factory.validation_compiler import ValidationCompiler
from autotel.factory.dspy_compiler import DSPyCompiler
from autotel.factory.linker import SemanticLinker
from autotel.factory.executor import OntologyExecutor
from autotel.core.telemetry import create_telemetry_manager


class TestXMLProcessingCapabilities:
    """Test suite for XML processing capabilities."""

    def setup_method(self):
        """Set up test fixtures."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-xml-processing-test",
            require_linkml_validation=False
        )

    def test_owl_xml_processing(self):
        """Test OWL XML processing capabilities."""
        print("🧪 Testing OWL XML Processing")
        
        # Test OWL XML with classes, properties, and relationships
        owl_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/test/ontology"/>
    
    <owl:Class rdf:about="http://autotel.ai/test#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input data from user</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/test#Recommendation">
        <rdfs:label>Recommendation</rdfs:label>
        <rdfs:comment>AI-generated recommendation</rdfs:comment>
    </owl:Class>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/test#hasText">
        <rdfs:domain rdf:resource="http://autotel.ai/test#UserInput"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <rdfs:label>has text</rdfs:label>
    </owl:DatatypeProperty>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/test#hasConfidence">
        <rdfs:domain rdf:resource="http://autotel.ai/test#Recommendation"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
        <rdfs:label>has confidence</rdfs:label>
    </owl:DatatypeProperty>
    
</rdf:RDF>'''
        
        processor = OWLProcessor()
        
        with self.telemetry.start_span("test_owl_processing", "owl_testing") as span:
            try:
                # Parse OWL ontology
                ontology = processor.parse_ontology_definition(owl_xml, "test")
                
                # Validate parsing results
                assert len(ontology.classes) >= 2, "Should parse at least 2 classes"
                assert len(ontology.data_properties) >= 2, "Should parse at least 2 data properties"
                
                # Check specific classes
                user_input_class = None
                recommendation_class = None
                for class_name, class_def in ontology.classes.items():
                    if "UserInput" in class_name:
                        user_input_class = class_def
                    elif "Recommendation" in class_name:
                        recommendation_class = class_def
                
                assert user_input_class is not None, "UserInput class should be parsed"
                assert recommendation_class is not None, "Recommendation class should be parsed"
                
                span.set_attribute("owl_processing_success", True)
                span.set_attribute("classes_found", len(ontology.classes))
                span.set_attribute("properties_found", len(ontology.data_properties))
                
                print(f"   ✅ OWL XML processing successful")
                print(f"      Classes: {len(ontology.classes)}")
                print(f"      Data Properties: {len(ontology.data_properties)}")
                
            except Exception as e:
                span.set_attribute("owl_processing_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def test_shacl_xml_processing(self):
        """Test SHACL XML processing capabilities."""
        print("🧪 Testing SHACL XML Processing")
        
        # Test SHACL XML with validation constraints - Fixed RDF/XML syntax
        shacl_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:shacl="http://www.w3.org/ns/shacl#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    
    <shacl:NodeShape rdf:about="http://autotel.ai/test#UserInputShape">
        <shacl:targetClass rdf:resource="http://autotel.ai/test#UserInput"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="http://autotel.ai/test#hasText"/>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                <shacl:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</shacl:minCount>
                <shacl:maxCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</shacl:maxCount>
                <shacl:minLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</shacl:minLength>
                <shacl:maxLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1000</shacl:maxLength>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
    <shacl:NodeShape rdf:about="http://autotel.ai/test#RecommendationShape">
        <shacl:targetClass rdf:resource="http://autotel.ai/test#Recommendation"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="http://autotel.ai/test#hasConfidence"/>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
                <shacl:minInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#float">0.0</shacl:minInclusive>
                <shacl:maxInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#float">1.0</shacl:maxInclusive>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
</rdf:RDF>'''
        
        processor = SHACLProcessor()
        
        with self.telemetry.start_span("test_shacl_processing", "shacl_testing") as span:
            try:
                # Parse SHACL shapes
                shacl_graph = processor.parse(shacl_xml)
                
                # Validate parsing results
                assert shacl_graph is not None, "SHACL graph should be created"
                assert len(shacl_graph) > 0, "SHACL graph should contain triples"
                
                span.set_attribute("shacl_processing_success", True)
                span.set_attribute("triples_found", len(shacl_graph))
                
                print(f"   ✅ SHACL XML processing successful")
                print(f"      Triples: {len(shacl_graph)}")
                
            except Exception as e:
                span.set_attribute("shacl_processing_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def test_dspy_xml_processing(self):
        """Test DSPy XML processing capabilities."""
        print("🧪 Testing DSPy XML Processing")
        
        # Test DSPy XML with signatures, OWL references, and SHACL constraints - Fixed namespace syntax
        dspy_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:definitions xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:owl="http://www.w3.org/2002/07/owl#"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  targetNamespace="http://autotel.ai/dspy">

    <dspy:signature name="recommendation_signature" description="Generate recommendations from user input">
        <dspy:input name="user_input" 
                    description="User input text" 
                    type="string" 
                    optional="false"
                    owl:class="http://autotel.ai/test#UserInput"
                    shaclShape="http://autotel.ai/test#UserInputShape">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>1</shacl:minLength>
                <shacl:maxLength>1000</shacl:maxLength>
            </shacl:PropertyShape>
        </dspy:input>
        <dspy:output name="recommendation" 
                     description="AI recommendation" 
                     type="string"
                     owl:class="http://autotel.ai/test#Recommendation"
                     shaclShape="http://autotel.ai/test#RecommendationShape">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>10</shacl:minLength>
            </shacl:PropertyShape>
        </dspy:output>
        <dspy:output name="confidence" 
                     description="Confidence score" 
                     type="float"
                     owl:class="http://autotel.ai/test#Recommendation">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#float</shacl:datatype>
                <shacl:minInclusive>0.0</shacl:minInclusive>
                <shacl:maxInclusive>1.0</shacl:maxInclusive>
            </shacl:PropertyShape>
        </dspy:output>
    </dspy:signature>

    <dspy:model provider="test" name="test_model">
        <dspy:parameter key="temperature" value="0.7"/>
        <dspy:parameter key="max_tokens" value="1000"/>
    </dspy:model>

</dspy:definitions>'''
        
        processor = DSPyProcessor()
        
        with self.telemetry.start_span("test_dspy_processing", "dspy_testing") as span:
            try:
                # Parse DSPy signatures
                signatures = processor.parse(dspy_xml)
                assert len(signatures) == 1, "Should parse one signature"
                
                signature = signatures[0]
                assert signature.name == "recommendation_signature", "Signature name should match"
                assert len(signature.inputs) == 1, "Should have one input"
                assert len(signature.outputs) == 2, "Should have two outputs"
                
                # Check OWL class references
                user_input = signature.inputs.get("user_input", {})
                assert user_input.get("owl_class") == "http://autotel.ai/test#UserInput", "OWL class should be extracted"
                assert user_input.get("shacl_shape") == "http://autotel.ai/test#UserInputShape", "SHACL shape should be extracted"
                
                # Parse model configuration
                model_config = processor.parse_model_configuration(dspy_xml)
                assert model_config is not None, "Model configuration should be parsed"
                assert model_config.get("provider") == "test", "Provider should match"
                assert model_config.get("model_name") == "test_model", "Model name should match"
                
                # Parse validation rules
                validation_rules = processor.parse_validation_rules(dspy_xml)
                assert len(validation_rules) > 0, "Should extract validation rules"
                
                # Parse semantic context
                semantic_context = processor.parse_semantic_context(dspy_xml)
                assert len(semantic_context["ontology_classes"]) > 0, "Should extract ontology classes"
                
                span.set_attribute("dspy_processing_success", True)
                span.set_attribute("signatures_found", len(signatures))
                span.set_attribute("validation_rules_found", len(validation_rules))
                span.set_attribute("ontology_classes_found", len(semantic_context["ontology_classes"]))
                
                print(f"   ✅ DSPy XML processing successful")
                print(f"      Signatures: {len(signatures)}")
                print(f"      Validation Rules: {len(validation_rules)}")
                print(f"      Ontology Classes: {len(semantic_context['ontology_classes'])}")
                
            except Exception as e:
                span.set_attribute("dspy_processing_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def test_integrated_xml_processing(self):
        """Test integrated XML processing with all three formats."""
        print("🧪 Testing Integrated XML Processing")
        
        # Combined XML with all three formats - Fixed syntax
        integrated_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  xmlns:owl="http://www.w3.org/2002/07/owl#"
                  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                  xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
                  targetNamespace="http://autotel.ai/workflows">

    <!-- OWL Ontology -->
    <owl:Ontology rdf:about="http://autotel.ai/test/ontology"/>
    
    <owl:Class rdf:about="http://autotel.ai/test#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input data from user</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/test#Recommendation">
        <rdfs:label>Recommendation</rdfs:label>
        <rdfs:comment>AI-generated recommendation</rdfs:comment>
    </owl:Class>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/test#hasText">
        <rdfs:domain rdf:resource="http://autotel.ai/test#UserInput"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>

    <!-- SHACL Shapes -->
    <shacl:shapes>
        <rdf:RDF xmlns:shacl="http://www.w3.org/ns/shacl#">
            <shacl:NodeShape rdf:about="http://autotel.ai/test#UserInputShape">
                <shacl:targetClass rdf:resource="http://autotel.ai/test#UserInput"/>
                <shacl:property>
                    <shacl:PropertyShape>
                        <shacl:path rdf:resource="http://autotel.ai/test#hasText"/>
                        <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                        <shacl:minLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</shacl:minLength>
                        <shacl:maxLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1000</shacl:maxLength>
                    </shacl:PropertyShape>
                </shacl:property>
            </shacl:NodeShape>
        </rdf:RDF>
    </shacl:shapes>

    <!-- DSPy Signatures -->
    <dspy:signatures>
        <dspy:signature name="integrated_signature" description="Integrated signature with OWL and SHACL">
            <dspy:input name="user_input" 
                        description="User input text" 
                        type="string" 
                        optional="false"
                        owl:class="http://autotel.ai/test#UserInput"
                        shaclShape="http://autotel.ai/test#UserInputShape"/>
            <dspy:output name="recommendation" 
                         description="AI recommendation" 
                         type="string"
                         owl:class="http://autotel.ai/test#Recommendation"/>
        </dspy:signature>
    </dspy:signatures>

    <dspy:model provider="test" name="integrated_model">
        <dspy:parameter key="temperature" value="0.7"/>
    </dspy:model>

</bpmn:definitions>'''
        
        with self.telemetry.start_span("test_integrated_processing", "integrated_testing") as span:
            try:
                # Initialize processors
                owl_processor = OWLProcessor()
                shacl_processor = SHACLProcessor()
                dspy_processor = DSPyProcessor()
                
                # Parse all three formats
                ontology = owl_processor.parse_ontology_definition(integrated_xml, "test")
                shacl_graph = shacl_processor.parse(integrated_xml)
                dspy_signatures = dspy_processor.parse(integrated_xml)
                model_config = dspy_processor.parse_model_configuration(integrated_xml)
                
                # Validate integration
                assert len(ontology.classes) >= 2, "Should parse OWL classes"
                assert len(shacl_graph) > 0, "Should parse SHACL shapes"
                assert len(dspy_signatures) == 1, "Should parse DSPy signature"
                assert model_config is not None, "Should parse model configuration"
                
                # Check cross-references
                signature = dspy_signatures[0]
                user_input = signature.inputs.get("user_input", {})
                assert user_input.get("owl_class") == "http://autotel.ai/test#UserInput", "OWL class reference should work"
                assert user_input.get("shacl_shape") == "http://autotel.ai/test#UserInputShape", "SHACL shape reference should work"
                
                span.set_attribute("integrated_processing_success", True)
                span.set_attribute("owl_classes", len(ontology.classes))
                span.set_attribute("shacl_triples", len(shacl_graph))
                span.set_attribute("dspy_signatures", len(dspy_signatures))
                
                print(f"   ✅ Integrated XML processing successful")
                print(f"      OWL Classes: {len(ontology.classes)}")
                print(f"      SHACL Triples: {len(shacl_graph)}")
                print(f"      DSPy Signatures: {len(dspy_signatures)}")
                
            except Exception as e:
                span.set_attribute("integrated_processing_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def test_end_to_end_xml_pipeline(self):
        """Test end-to-end XML processing pipeline with compilation and execution."""
        print("🧪 Testing End-to-End XML Pipeline")
        
        # Create test XML data
        owl_xml, shacl_xml, dspy_xml = self._create_test_xml_data()
        
        with self.telemetry.start_span("test_e2e_xml_pipeline", "e2e_testing") as span:
            try:
                # Stage 1: Processors
                owl_processor = OWLProcessor()
                shacl_processor = SHACLProcessor()
                dspy_processor = DSPyProcessor()
                
                ontology_def = owl_processor.parse_ontology_definition(owl_xml, "test")
                shacl_graph = shacl_processor.parse(shacl_xml)
                dspy_signatures = dspy_processor.parse(dspy_xml)
                model_config = dspy_processor.parse_model_configuration(dspy_xml)
                
                # Stage 2: Compilers
                ontology_compiler = OntologyCompiler()
                validation_compiler = ValidationCompiler()
                dspy_compiler = DSPyCompiler()
                
                ontology_schema = ontology_compiler.compile(ontology_def)
                validation_rules = validation_compiler.compile(shacl_graph)
                dspy_signature = dspy_compiler.compile(
                    ontology_schema=ontology_schema,
                    validation_rules=validation_rules,
                    dspy_signatures=dspy_signatures,
                    dspy_modules=[],
                    model_config=model_config
                )
                
                # Stage 3: Linker and Executor
                linker = SemanticLinker()
                executor = OntologyExecutor()
                
                executable_system = linker.link(dspy_signature)
                test_inputs = {"user_input": "test user input for XML processing"}
                result = executor.execute(executable_system, test_inputs)
                
                # Validate results
                assert result.success is True, "Pipeline execution should succeed"
                assert "recommendation" in result.outputs, "Expected output should be present"
                assert len(result.telemetry_data.spans) > 0, "Telemetry should be generated"
                
                span.set_attribute("e2e_pipeline_success", True)
                span.set_attribute("execution_time", result.execution_time)
                span.set_attribute("outputs_count", len(result.outputs))
                
                print(f"   ✅ End-to-end XML pipeline successful")
                print(f"      Execution time: {result.execution_time:.4f}s")
                print(f"      Outputs: {list(result.outputs.keys())}")
                print(f"      Telemetry spans: {len(result.telemetry_data.spans)}")
                
            except Exception as e:
                span.set_attribute("e2e_pipeline_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _create_test_xml_data(self):
        """Create test XML data for end-to-end testing."""
        owl_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/test/ontology"/>
    
    <owl:Class rdf:about="http://autotel.ai/test#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input data from user</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/test#Recommendation">
        <rdfs:label>Recommendation</rdfs:label>
        <rdfs:comment>AI-generated recommendation</rdfs:comment>
    </owl:Class>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/test#hasText">
        <rdfs:domain rdf:resource="http://autotel.ai/test#UserInput"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
</rdf:RDF>'''
        
        shacl_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:shacl="http://www.w3.org/ns/shacl#">
    
    <shacl:NodeShape rdf:about="http://autotel.ai/test#UserInputShape">
        <shacl:targetClass rdf:resource="http://autotel.ai/test#UserInput"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="http://autotel.ai/test#hasText"/>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                <shacl:minLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</shacl:minLength>
                <shacl:maxLength rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1000</shacl:maxLength>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
</rdf:RDF>'''
        
        dspy_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:definitions xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:owl="http://www.w3.org/2002/07/owl#"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  targetNamespace="http://autotel.ai/dspy">

    <dspy:signature name="e2e_test_signature" description="End-to-end test signature">
        <dspy:input name="user_input" 
                    description="User input text" 
                    type="string" 
                    optional="false"
                    owl:class="http://autotel.ai/test#UserInput"
                    shaclShape="http://autotel.ai/test#UserInputShape"/>
        <dspy:output name="recommendation" 
                     description="AI recommendation" 
                     type="string"
                     owl:class="http://autotel.ai/test#Recommendation"/>
    </dspy:signature>

    <dspy:model provider="test" name="e2e_test_model">
        <dspy:parameter key="temperature" value="0.7"/>
    </dspy:model>

</dspy:definitions>'''
        
        return owl_xml, shacl_xml, dspy_xml


def run_xml_processing_validation():
    """Run the complete XML processing capabilities validation."""
    print("🚀 Starting XML Processing Capabilities Validation")
    print("=" * 60)
    
    test_suite = TestXMLProcessingCapabilities()
    test_suite.setup_method()
    
    # Run all tests
    test_methods = [
        test_suite.test_owl_xml_processing,
        test_suite.test_shacl_xml_processing,
        test_suite.test_dspy_xml_processing,
        test_suite.test_integrated_xml_processing,
        test_suite.test_end_to_end_xml_pipeline,
    ]
    
    passed = 0
    failed = 0
    
    for test_method in test_methods:
        try:
            test_method()
            passed += 1
        except Exception as e:
            failed += 1
            print(f"   ❌ {test_method.__name__} failed: {e}")
    
    print("\n" + "=" * 60)
    print(f"📊 XML Processing Capabilities Validation Results:")
    print(f"   ✅ Passed: {passed}")
    print(f"   ❌ Failed: {failed}")
    print(f"   📈 Success Rate: {passed/(passed+failed)*100:.1f}%")
    
    if failed == 0:
        print("\n🎉 All XML processing capabilities are working correctly!")
        print("✅ OWL XML processing validated")
        print("✅ SHACL XML processing validated")
        print("✅ DSPy XML processing validated")
        print("✅ Integrated XML processing validated")
        print("✅ End-to-end XML pipeline validated")
        return True
    else:
        print(f"\n⚠️  {failed} XML processing capability(ies) need attention")
        return False


if __name__ == "__main__":
    success = run_xml_processing_validation()
    exit(0 if success else 1) 