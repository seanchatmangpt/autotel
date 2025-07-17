"""
Integration test for refactored factory with unified processors.

This test verifies that the factory can process actual data through the pipeline
using the unified processors.
"""

import pytest
from autotel.factory import PipelineOrchestrator


class TestFactoryIntegration:
    """Integration tests for factory with unified processors."""
    
    def test_owl_processing_integration(self):
        """Test OWL processing through the pipeline."""
        orchestrator = PipelineOrchestrator()
        
        # Sample OWL XML
        owl_xml = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://example.com/ontology#"
         xml:base="http://example.com/ontology"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:xml="http://www.w3.org/XML/1998/namespace"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Ontology rdf:about="http://example.com/ontology"/>
    <owl:Class rdf:about="#Person">
        <rdfs:label>Person</rdfs:label>
        <rdfs:comment>A human being</rdfs:comment>
    </owl:Class>
    <owl:Class rdf:about="#Organization">
        <rdfs:label>Organization</rdfs:label>
        <rdfs:comment>An organization</rdfs:comment>
    </owl:Class>
    <owl:ObjectProperty rdf:about="#worksFor">
        <rdfs:label>works for</rdfs:label>
        <rdfs:domain rdf:resource="#Person"/>
        <rdfs:range rdf:resource="#Organization"/>
    </owl:ObjectProperty>
</rdf:RDF>'''
        
        # Process OWL content
        result = orchestrator.process_owl_content(owl_xml)
        
        # Verify result structure
        assert "ontology_definition" in result
        assert "metadata" in result
        
        # Verify metadata
        metadata = result["metadata"]
        assert metadata["classes_count"] == 2
        assert metadata["object_properties_count"] == 1
        assert metadata["data_properties_count"] == 0
        assert metadata["individuals_count"] == 0
        assert metadata["axioms_count"] == 0
    
    def test_shacl_processing_integration(self):
        """Test SHACL processing through the pipeline."""
        orchestrator = PipelineOrchestrator()
        
        # Sample SHACL XML
        shacl_xml = '''<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    <sh:NodeShape rdf:about="#PersonShape">
        <sh:targetClass rdf:resource="http://example.com/ontology#Person"/>
        <sh:property rdf:resource="#PersonNameProperty"/>
    </sh:NodeShape>
    <sh:PropertyShape rdf:about="#PersonNameProperty">
        <sh:path rdf:resource="http://example.com/ontology#name"/>
        <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
        <sh:maxCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:maxCount>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </sh:PropertyShape>
</rdf:RDF>'''
        
        # Process SHACL content
        result = orchestrator.process_shacl_content(shacl_xml)
        
        # Verify result structure
        assert "node_shapes" in result
        assert "property_shapes" in result
        assert "constraints" in result
        assert "total_shapes" in result
        assert "total_constraints" in result
        
        # Verify counts
        assert result["total_shapes"] >= 1
        assert result["total_constraints"] >= 1
    
    def test_dspy_processing_integration(self):
        """Test DSPy processing through the pipeline."""
        orchestrator = PipelineOrchestrator()
        
        # Sample DSPy XML
        dspy_xml = '''<?xml version="1.0"?>
<dspy:config xmlns:dspy="http://autotel.ai/dspy">
    <dspy:signature name="recommend_person" description="Recommend a person based on criteria">
        <dspy:input name="criteria" description="Search criteria" optional="false"/>
        <dspy:output name="recommendation" description="Person recommendation"/>
        <dspy:example>
            <criteria>software engineer</criteria>
            <recommendation>John Doe</recommendation>
        </dspy:example>
    </dspy:signature>
</dspy:config>'''
        
        # Process DSPy content
        result = orchestrator.process_dspy_content(dspy_xml)
        
        # Verify result structure
        assert "signatures" in result
        assert "metadata" in result
        
        # Verify metadata
        metadata = result["metadata"]
        assert metadata["signatures_count"] == 1
        assert metadata["total_signatures"] == 1
        
        # Verify signature
        signatures = result["signatures"]
        assert len(signatures) == 1
        signature = signatures[0]
        assert signature.name == "recommend_person"
        assert "criteria" in signature.inputs
        assert "recommendation" in signature.outputs
    
    def test_empty_content_handling(self):
        """Test that empty content is handled gracefully."""
        orchestrator = PipelineOrchestrator()
        
        # Test empty OWL content
        result = orchestrator.process_owl_content("")
        assert "error" in result
        assert "Empty OWL content provided" in result["error"]
        
        # Test empty SHACL content
        result = orchestrator.process_shacl_content("")
        assert "error" in result
        assert "Empty SHACL content provided" in result["error"]
        
        # Test empty DSPy content
        result = orchestrator.process_dspy_content("")
        assert "error" in result
        assert "Empty DSPy content provided" in result["error"]
    
    def test_pipeline_components_initialization(self):
        """Test that all pipeline components are properly initialized."""
        orchestrator = PipelineOrchestrator()
        
        # Verify all components are initialized
        assert orchestrator.owl_processor is not None
        assert orchestrator.shacl_processor is not None
        assert orchestrator.dspy_processor is not None
        assert orchestrator.otel_processor is not None
        assert orchestrator.ontology_compiler is not None
        assert orchestrator.validation_compiler is not None
        assert orchestrator.dspy_compiler is not None
        assert orchestrator.linker is not None
        assert orchestrator.executor is not None
        assert orchestrator.telemetry is not None


if __name__ == "__main__":
    pytest.main([__file__]) 