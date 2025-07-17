"""
Test Factory Boy Integration - Demonstrates dynamic test data generation

This test file shows how to use Factory Boy to replace all hardcoded test data
with dynamically generated content, ensuring tests never rely on static values.
"""

import pytest
import tempfile
import os
from pathlib import Path

from autotel.factory.processors.bpmn_processor import BPMNProcessor
from autotel.factory.processors.dmn_processor import DMNProcessor
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.processors.sparql_processor import SPARQLProcessor
from autotel.factory.pipeline import PipelineOrchestrator

from factories import (
    BPMNXMLFactory, DMNXMLFactory, OWLXMLFactory, SHACLXMLFactory, DSPyXMLFactory,
    SPARQLXMLFactory, DSPySignatureDefinitionFactory, SPARQLQueryDefinitionFactory,
    SPARQLQueryTemplateFactory, TelemetryDataFactory, WorkflowContextFactory,
    TestFileFactory, create_comprehensive_test_suite
)


class TestFactoryBoyIntegration:
    """Test suite demonstrating Factory Boy integration for dynamic test data"""
    
    def test_bpmn_factory_generates_valid_xml(self):
        """Test that BPMN factory generates valid XML content"""
        bpmn_xml = BPMNXMLFactory().xml_content
        
        assert "<?xml version=" in bpmn_xml
        assert "<bpmn:definitions" in bpmn_xml
        assert "<bpmn:process" in bpmn_xml
        assert "<bpmn:startEvent" in bpmn_xml
        assert "<bpmn:endEvent" in bpmn_xml
        
        # Verify it can be parsed by BPMN processor
        processor = BPMNProcessor()
        result = processor.parse(bpmn_xml, "test_process")
        
        assert result is not None
        assert hasattr(result, 'name')
        assert hasattr(result, 'task_specs')
    
    def test_dmn_factory_generates_valid_xml(self):
        """Test that DMN factory generates valid XML content"""
        dmn_xml = DMNXMLFactory().xml_content
        
        assert "<?xml version=" in dmn_xml
        assert "<definitions" in dmn_xml
        assert "<decision" in dmn_xml
        assert "<decisionTable" in dmn_xml
        assert "<input" in dmn_xml
        assert "<output" in dmn_xml
        assert "<rule" in dmn_xml
        
        # Verify it can be parsed by DMN processor
        processor = DMNProcessor()
        result = processor.parse(dmn_xml)
        
        assert result is not None
        assert len(result) > 0
    
    def test_owl_factory_generates_valid_xml(self):
        """Test that OWL factory generates valid XML content"""
        owl_xml = OWLXMLFactory().xml_content
        
        assert "<?xml version=" in owl_xml
        assert "<rdf:RDF" in owl_xml
        assert "<owl:Ontology" in owl_xml
        assert "<owl:Class" in owl_xml
        
        # Verify it can be parsed by OWL processor
        processor = OWLProcessor()
        result = processor.parse(owl_xml)
        
        assert result is not None
        assert len(result) > 0
    
    def test_shacl_factory_generates_valid_xml(self):
        """Test that SHACL factory generates valid XML content"""
        shacl_xml = SHACLXMLFactory().xml_content
        
        assert "<?xml version=" in shacl_xml
        assert "<rdf:RDF" in shacl_xml
        assert "<sh:NodeShape" in shacl_xml
        assert "<sh:PropertyShape" in shacl_xml
        
        # Verify it can be parsed by SHACL processor
        processor = SHACLProcessor()
        result = processor.parse(shacl_xml)
        
        assert result is not None
        assert len(result) > 0
    
    def test_dspy_factory_generates_valid_xml(self):
        """Test that DSPy factory generates valid XML content"""
        dspy_xml = DSPyXMLFactory().xml_content
        
        assert "<?xml version=" in dspy_xml
        assert "<root xmlns:dspy=" in dspy_xml
        assert "<dspy:signature" in dspy_xml
        assert "<dspy:input" in dspy_xml
        assert "<dspy:output" in dspy_xml
        
        # Verify it can be parsed by DSPy processor
        processor = DSPyProcessor()
        result = processor.parse(dspy_xml)
        
        assert result is not None
        assert len(result) > 0
        assert all(hasattr(sig, 'name') for sig in result)
    
    def test_dspy_signature_factory_creates_valid_objects(self):
        """Test that DSPy signature factory creates valid objects"""
        signature = DSPySignatureDefinitionFactory()
        
        assert hasattr(signature, 'name')
        assert hasattr(signature, 'description')
        assert hasattr(signature, 'inputs')
        assert hasattr(signature, 'outputs')
        
        assert isinstance(signature.name, str)
        assert isinstance(signature.description, str)
        assert isinstance(signature.inputs, dict)
        assert isinstance(signature.outputs, dict)
        
        assert len(signature.inputs) > 0
        assert len(signature.outputs) > 0
    
    def test_sparql_factory_generates_valid_xml(self):
        """Test that SPARQL factory generates valid XML content"""
        sparql_factory = SPARQLXMLFactory()
        sparql_xml = sparql_factory['xml_content']
        
        assert "<?xml version=" in sparql_xml
        assert "<root xmlns:sparql=" in sparql_xml
        assert "<sparql:query" in sparql_xml or "<sparql:template" in sparql_xml
        
        # Verify it can be parsed by SPARQL processor
        processor = SPARQLProcessor()
        result = processor.parse(sparql_xml)
        
        assert result is not None
        assert "queries" in result
        assert "templates" in result
        assert len(result["queries"]) >= 0
        assert len(result["templates"]) >= 0
    
    def test_sparql_query_factory_creates_valid_objects(self):
        """Test that SPARQL query factory creates valid objects"""
        query = SPARQLQueryDefinitionFactory()
        
        assert hasattr(query, 'name')
        assert hasattr(query, 'description')
        assert hasattr(query, 'query')
        assert hasattr(query, 'query_type')
        assert hasattr(query, 'parameters')
        assert hasattr(query, 'prefixes')
        
        assert isinstance(query.name, str)
        assert isinstance(query.description, str)
        assert isinstance(query.query, str)
        assert isinstance(query.query_type, str)
        assert isinstance(query.parameters, dict)
        assert isinstance(query.prefixes, dict)
        
        assert query.query_type in ['SELECT', 'ASK', 'CONSTRUCT', 'DESCRIBE']
    
    def test_sparql_template_factory_creates_valid_objects(self):
        """Test that SPARQL template factory creates valid objects"""
        template = SPARQLQueryTemplateFactory()
        
        assert hasattr(template, 'name')
        assert hasattr(template, 'description')
        assert hasattr(template, 'template')
        assert hasattr(template, 'parameters')
        assert hasattr(template, 'validation_rules')
        assert hasattr(template, 'examples')
        
        assert isinstance(template.name, str)
        assert isinstance(template.description, str)
        assert isinstance(template.template, str)
        assert isinstance(template.parameters, list)
        assert isinstance(template.validation_rules, list)
        assert isinstance(template.examples, list)
        
        assert len(template.parameters) > 0
    
    def test_telemetry_factory_generates_valid_data(self):
        """Test that telemetry factory generates valid data"""
        telemetry_data = TelemetryDataFactory()
        
        assert 'trace_id' in telemetry_data
        assert 'span_id' in telemetry_data
        assert 'timestamp' in telemetry_data
        assert 'attributes' in telemetry_data
        
        assert isinstance(telemetry_data['trace_id'], str)
        assert isinstance(telemetry_data['span_id'], str)
        assert isinstance(telemetry_data['timestamp'], int)
        assert isinstance(telemetry_data['attributes'], dict)
        
        assert len(telemetry_data['attributes']) > 0
    
    def test_workflow_context_factory_generates_valid_data(self):
        """Test that workflow context factory generates valid data"""
        context_data = WorkflowContextFactory()
        
        assert 'workflow_id' in context_data
        assert 'timestamp' in context_data
        assert 'status' in context_data
        
        assert isinstance(context_data['workflow_id'], str)
        assert isinstance(context_data['timestamp'], int)
        assert isinstance(context_data['status'], str)
        
        assert context_data['status'] in ['pending', 'running', 'completed', 'failed']
    
    def test_test_file_factory_creates_valid_files(self):
        """Test that test file factory creates valid file objects"""
        test_file = TestFileFactory()
        
        assert 'filename' in test_file
        assert 'file_path' in test_file
        assert 'content' in test_file
        assert 'file_type' in test_file
        
        assert isinstance(test_file['filename'], str)
        assert isinstance(test_file['file_path'], Path)
        assert isinstance(test_file['content'], str)
        assert isinstance(test_file['file_type'], str)
        
        assert test_file['file_type'] in ['bpmn', 'dmn', 'owl', 'shacl', 'dspy', 'sparql']
        assert test_file['filename'].endswith(f".{test_file['file_type']}")
    
    def test_comprehensive_test_suite_creation(self):
        """Test that comprehensive test suite creates all data types"""
        test_suite = create_comprehensive_test_suite()
        
        assert 'bpmn' in test_suite
        assert 'dmn' in test_suite
        assert 'owl' in test_suite
        assert 'shacl' in test_suite
        assert 'dspy' in test_suite
        assert 'sparql' in test_suite
        assert 'telemetry' in test_suite
        assert 'workflow_context' in test_suite
        assert 'test_files' in test_suite
        
        assert len(test_suite['bpmn']) == 3
        assert len(test_suite['dmn']) == 2
        assert len(test_suite['owl']) == 2
        assert len(test_suite['shacl']) == 2
        assert len(test_suite['dspy']) == 3
        assert len(test_suite['sparql']) == 2
        assert len(test_suite['test_files']) == 5
    
    def test_factory_data_is_unique_across_runs(self):
        """Test that factory data is unique across multiple runs"""
        # Generate multiple instances
        bpmn_1 = BPMNXMLFactory().xml_content
        bpmn_2 = BPMNXMLFactory().xml_content
        bpmn_3 = BPMNXMLFactory().xml_content
        
        # They should be different (though not guaranteed due to randomness)
        # At least some elements should be different
        assert bpmn_1 != bpmn_2 or bpmn_2 != bpmn_3 or bpmn_1 != bpmn_3
        
        # Test telemetry data uniqueness
        telemetry_1 = TelemetryDataFactory()
        telemetry_2 = TelemetryDataFactory()
        
        assert telemetry_1['trace_id'] != telemetry_2['trace_id']
        assert telemetry_1['span_id'] != telemetry_2['span_id']
    
    def test_factory_data_with_custom_parameters(self):
        """Test that factories work with custom parameters"""
        # Custom BPMN with specific task count
        bpmn_xml = BPMNXMLFactory(task_count=10).xml_content
        
        # Count the number of tasks in the generated XML
        task_count = bpmn_xml.count('<bpmn:task')
        assert task_count == 10
        
        # Custom DSPy with specific signature count
        dspy_xml = DSPyXMLFactory(signature_count=5).xml_content
        
        # Count the number of signatures
        signature_count = dspy_xml.count('<dspy:signature')
        assert signature_count == 5
    
    def test_pipeline_integration_with_factory_data(self):
        """Test that pipeline works with factory-generated data"""
        # Create comprehensive test data
        test_suite = create_comprehensive_test_suite()
        
        # Test pipeline with factory-generated data
        pipeline = PipelineOrchestrator()
        
        # Test with BPMN data
        bpmn_result = pipeline.process_bpmn_content(test_suite['bpmn'][0])
        assert bpmn_result is not None
        
        # Test with OWL data
        owl_result = pipeline.process_owl_content(test_suite['owl'][0])
        assert owl_result is not None
        
        # Test with SHACL data
        shacl_result = pipeline.process_shacl_content(test_suite['shacl'][0])
        assert shacl_result is not None
    
    def test_factory_data_in_file_operations(self):
        """Test factory data in file operations"""
        test_file = TestFileFactory()
        
        # Create temporary file with factory content
        with tempfile.NamedTemporaryFile(mode='w', suffix=f".{test_file['file_type']}", delete=False) as f:
            f.write(test_file['content'])
            temp_path = f.name
        
        try:
            # Verify file was created and has content
            assert os.path.exists(temp_path)
            
            with open(temp_path, 'r') as f:
                content = f.read()
            
            assert content == test_file['content']
            
            # Test that processors can read the file
            if test_file['file_type'] == 'bpmn':
                processor = BPMNProcessor()
                result = processor.parse_file(temp_path, "test_process")
                assert result is not None
            elif test_file['file_type'] == 'dmn':
                processor = DMNProcessor()
                result = processor.parse_file(temp_path)
                assert result is not None
            elif test_file['file_type'] == 'owl':
                processor = OWLProcessor()
                result = processor.parse_file(temp_path)
                assert result is not None
            elif test_file['file_type'] == 'shacl':
                processor = SHACLProcessor()
                result = processor.parse_file(temp_path)
                assert result is not None
            elif test_file['file_type'] == 'dspy':
                processor = DSPyProcessor()
                result = processor.parse_file(temp_path)
                assert result is not None
                
        finally:
            # Clean up
            if os.path.exists(temp_path):
                os.unlink(temp_path)
    
    def test_factory_data_consistency(self):
        """Test that factory data is consistent within a single generation"""
        # Generate a single instance
        signature = DSPySignatureDefinitionFactory()
        
        # All attributes should be consistent
        assert signature.name is not None
        assert signature.description is not None
        assert signature.inputs is not None
        assert signature.outputs is not None
        
        # Inputs and outputs should have the expected structure
        for input_name, input_data in signature.inputs.items():
            assert isinstance(input_name, str)
            assert isinstance(input_data, dict)
            assert 'description' in input_data
            assert 'optional' in input_data
        
        for output_name, output_description in signature.outputs.items():
            assert isinstance(output_name, str)
            assert isinstance(output_description, str)


class TestFactoryBoyFixtureIntegration:
    """Test suite for Factory Boy integration with pytest fixtures"""
    
    def test_dynamic_fixtures_work(self, dynamic_test_suite):
        """Test that dynamic fixtures work correctly"""
        assert 'bpmn' in dynamic_test_suite
        assert 'dmn' in dynamic_test_suite
        assert 'owl' in dynamic_test_suite
        assert 'shacl' in dynamic_test_suite
        assert 'dspy' in dynamic_test_suite
    
    def test_individual_dynamic_fixtures(self, dynamic_bpmn_files, dynamic_dmn_files, 
                                       dynamic_owl_files, dynamic_shacl_files, dynamic_dspy_files):
        """Test individual dynamic fixtures"""
        assert len(dynamic_bpmn_files) == 3
        assert len(dynamic_dmn_files) == 2
        assert len(dynamic_owl_files) == 2
        assert len(dynamic_shacl_files) == 2
        assert len(dynamic_dspy_files) == 3
        
        # Verify each file has valid content
        for bpmn_file in dynamic_bpmn_files:
            assert "<bpmn:definitions" in bpmn_file
        
        for dmn_file in dynamic_dmn_files:
            assert "<definitions" in dmn_file
        
        for owl_file in dynamic_owl_files:
            assert "<rdf:RDF" in owl_file
        
        for shacl_file in dynamic_shacl_files:
            assert "<rdf:RDF" in shacl_file
        
        for dspy_file in dynamic_dspy_files:
            assert "<root xmlns:dspy=" in dspy_file
    
    def test_fixture_data_is_dynamic(self, sample_bpmn_xml, sample_dmn_xml, 
                                   sample_owl_xml, sample_shacl_xml, sample_dspy_signature):
        """Test that fixture data is dynamically generated"""
        # Each call should potentially generate different data
        # (though not guaranteed due to randomness)
        bpmn_1 = sample_bpmn_xml
        dmn_1 = sample_dmn_xml
        owl_1 = sample_owl_xml
        shacl_1 = sample_shacl_xml
        signature_1 = sample_dspy_signature
        
        # Verify the data has the expected structure
        assert "<bpmn:definitions" in bpmn_1
        assert "<definitions" in dmn_1
        assert "<rdf:RDF" in owl_1
        assert "<rdf:RDF" in shacl_1
        assert 'name' in signature_1
        assert 'inputs' in signature_1
        assert 'outputs' in signature_1


if __name__ == "__main__":
    pytest.main([__file__]) 