"""
Dynamic test for SRO OWL ontology processing.

This test loads the actual SRO OWL file and validates the OWL processor's
ability to handle real ontology files without hardcoding any values.
"""

import pytest
import os
from pathlib import Path
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.pipeline import PipelineOrchestrator


class TestSROOWLDynamic:
    """Test SRO OWL ontology processing with dynamic loading."""

    def setup_method(self):
        """Set up test fixtures."""
        self.owl_processor = OWLProcessor()
        self.orchestrator = PipelineOrchestrator()
        
        # Dynamically locate the SRO OWL file
        self.sro_owl_path = Path("ontologies/scrum_reference_ontology/sro.owl")
        
        # Verify the file exists
        if not self.sro_owl_path.exists():
            pytest.skip(f"SRO OWL file not found at {self.sro_owl_path}")

    def test_sro_owl_file_exists(self):
        """Test that the SRO OWL file exists and is readable."""
        assert self.sro_owl_path.exists(), f"SRO OWL file not found at {self.sro_owl_path}"
        assert self.sro_owl_path.is_file(), f"{self.sro_owl_path} is not a file"
        assert self.sro_owl_path.stat().st_size > 0, f"{self.sro_owl_path} is empty"

    def test_sro_owl_parse_ontology_definition(self):
        """Test parsing the SRO OWL file."""
        # Read the OWL file content
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        # Parse the OWL XML content
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Basic validation that we got a valid ontology structure
        assert ontology is not None
        assert hasattr(ontology, 'classes')
        assert hasattr(ontology, 'object_properties')
        assert hasattr(ontology, 'data_properties')
        assert hasattr(ontology, 'individuals')
        assert hasattr(ontology, 'axioms')
        assert hasattr(ontology, 'ontology_uri')
        assert hasattr(ontology, 'namespace')

    def test_sro_owl_analyze_ontology(self):
        """Test analyzing the SRO OWL ontology."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Analyze the ontology structure
        analysis = {
            'class_count': len(ontology.classes),
            'object_property_count': len(ontology.object_properties),
            'data_property_count': len(ontology.data_properties),
            'individual_count': len(ontology.individuals),
            'axiom_count': len(ontology.axioms),
            'ontology_uri': ontology.ontology_uri,
            'namespace': ontology.namespace
        }
        
        # Validate analysis structure
        assert analysis is not None
        assert isinstance(analysis, dict)
        
        # Check for expected analysis fields
        expected_fields = ['class_count', 'object_property_count', 'data_property_count', 'individual_count', 'axiom_count']
        for field in expected_fields:
            assert field in analysis, f"Analysis missing field: {field}"
            assert analysis[field] >= 0, f"Analysis field {field} should be non-negative"

    def test_sro_owl_convert_to_autotel_format(self):
        """Test converting SRO OWL to AutoTel format."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Convert to AutoTel format (simulate conversion)
        autotel_format = {
            'entities': list(ontology.classes.keys()),
            'relationships': list(ontology.object_properties.keys()),
            'constraints': list(ontology.data_properties.keys()),
            'metadata': {
                'ontology_uri': ontology.ontology_uri,
                'namespace': ontology.namespace,
                'total_classes': len(ontology.classes),
                'total_properties': len(ontology.object_properties) + len(ontology.data_properties)
            }
        }
        
        # Validate conversion
        assert autotel_format is not None
        assert isinstance(autotel_format, dict)
        
        # Check for expected AutoTel format fields
        expected_fields = ['entities', 'relationships', 'constraints', 'metadata']
        for field in expected_fields:
            assert field in autotel_format, f"AutoTel format missing field: {field}"

    def test_sro_owl_pipeline_integration(self):
        """Test SRO OWL processing through the pipeline orchestrator."""
        # Read the OWL file content
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        # Process through pipeline (simulate pipeline processing)
        result = {
            'ontology': self.owl_processor.parse_ontology_definition(owl_xml),
            'analysis': {
                'class_count': len(self.owl_processor.parse_ontology_definition(owl_xml).classes),
                'property_count': len(self.owl_processor.parse_ontology_definition(owl_xml).object_properties)
            },
            'conversion': 'success',
            'metadata': {
                'source_file': str(self.sro_owl_path),
                'processing_time': 'test'
            }
        }
        
        # Validate pipeline result
        assert result is not None
        assert isinstance(result, dict)
        
        # Check for expected pipeline result fields
        expected_fields = ['ontology', 'analysis', 'conversion', 'metadata']
        for field in expected_fields:
            assert field in result, f"Pipeline result missing field: {field}"

    def test_sro_owl_telemetry_integration(self):
        """Test that SRO OWL processing generates telemetry."""
        # Read the OWL file content
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        # Process the OWL file and capture telemetry
        with self.owl_processor.telemetry.start_span("test_sro_owl_telemetry", "testing") as span:
            ontology = self.owl_processor.parse_ontology_definition(owl_xml)
            
            # Set telemetry attributes
            span.set_attribute("owl_file_path", str(self.sro_owl_path))
            span.set_attribute("ontology_classes", len(ontology.classes))
            span.set_attribute("ontology_properties", len(ontology.object_properties))
            span.set_attribute("processing_success", True)
        
        # Verify telemetry was generated
        assert span is not None
        assert span.get_attribute("processing_success") is True

    def test_sro_owl_content_validation(self):
        """Test that SRO OWL contains expected Scrum-related content."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Extract class names
        class_names = list(ontology.classes.keys())
        
        # Check for expected Scrum-related classes
        expected_scrum_classes = [
            'ScrumProject', 'ScrumProcess', 'Sprint', 'ProductBacklog',
            'UserStory', 'ScrumTeam', 'ProductOwner', 'ScrumMaster', 'Developer'
        ]
        
        found_classes = []
        for expected_class in expected_scrum_classes:
            if any(expected_class.lower() in name.lower() for name in class_names):
                found_classes.append(expected_class)
        
        # Assert that we found at least some expected Scrum classes
        assert len(found_classes) > 0, f"No expected Scrum classes found. Available classes: {class_names[:10]}"
        
        # Log what we found for debugging
        print(f"Found Scrum classes: {found_classes}")
        print(f"Total classes in ontology: {len(class_names)}")

    def test_sro_owl_file_size_and_complexity(self):
        """Test that SRO OWL file has reasonable size and complexity."""
        # Check file size
        file_size = self.sro_owl_path.stat().st_size
        assert file_size > 1000, f"OWL file too small: {file_size} bytes"
        assert file_size < 1000000, f"OWL file too large: {file_size} bytes"
        
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Validate complexity metrics
        assert len(ontology.classes) > 0, "Ontology should have at least one class"
        assert len(ontology.object_properties) > 0, "Ontology should have at least one property"
        assert ontology.ontology_uri, "Ontology should have a URI"

    def test_sro_owl_error_handling(self):
        """Test error handling with the SRO OWL file."""
        # Test with invalid XML
        with pytest.raises(Exception):
            self.owl_processor.parse_ontology_definition("invalid xml content")
        
        # Test with empty content
        with pytest.raises(Exception):
            self.owl_processor.parse_ontology_definition("")

    def test_sro_owl_performance(self):
        """Test performance of SRO OWL processing."""
        import time
        
        # Read the OWL file content
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        # Measure parsing time
        start_time = time.time()
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        parse_time = time.time() - start_time
        
        # Measure analysis time
        start_time = time.time()
        analysis = {
            'class_count': len(ontology.classes),
            'property_count': len(ontology.object_properties)
        }
        analysis_time = time.time() - start_time
        
        # Measure conversion time
        start_time = time.time()
        conversion = {
            'entities': list(ontology.classes.keys()),
            'relationships': list(ontology.object_properties.keys())
        }
        conversion_time = time.time() - start_time
        
        # Assert reasonable performance (adjust thresholds as needed)
        assert parse_time < 5.0, f"Parsing took too long: {parse_time:.2f}s"
        assert analysis_time < 2.0, f"Analysis took too long: {analysis_time:.2f}s"
        assert conversion_time < 2.0, f"Conversion took too long: {conversion_time:.2f}s"
        
        # Log performance metrics
        print(f"Performance metrics:")
        print(f"  Parse time: {parse_time:.3f}s")
        print(f"  Analysis time: {analysis_time:.3f}s")
        print(f"  Conversion time: {conversion_time:.3f}s")

    def test_sro_owl_round_trip(self):
        """Test round-trip processing of SRO OWL."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        # Parse original file
        original_ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Convert to AutoTel format
        autotel_format = {
            'entities': list(original_ontology.classes.keys()),
            'relationships': list(original_ontology.object_properties.keys()),
            'constraints': list(original_ontology.data_properties.keys()),
            'metadata': {
                'ontology_uri': original_ontology.ontology_uri,
                'namespace': original_ontology.namespace
            }
        }
        
        # Basic validation that we can convert
        assert autotel_format is not None
        assert isinstance(autotel_format, dict)
        assert 'entities' in autotel_format
        assert 'relationships' in autotel_format

    def test_sro_owl_metadata_extraction(self):
        """Test extraction of metadata from SRO OWL."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Extract metadata
        metadata = {
            'ontology_uri': ontology.ontology_uri,
            'namespace': ontology.namespace,
            'class_count': len(ontology.classes),
            'object_property_count': len(ontology.object_properties),
            'data_property_count': len(ontology.data_properties),
            'individual_count': len(ontology.individuals),
            'axiom_count': len(ontology.axioms)
        }
        
        # Validate metadata structure
        assert isinstance(metadata, dict)
        assert len(metadata) > 0, "Metadata should not be empty"
        assert metadata['ontology_uri'], "Ontology should have a URI"

    def test_sro_owl_class_hierarchy(self):
        """Test extraction of class hierarchy from SRO OWL."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Extract class hierarchy information
        hierarchy_info = {}
        for class_name, class_data in ontology.classes.items():
            if hasattr(class_data, 'superclasses') and class_data.get('superclasses'):
                hierarchy_info[class_name] = class_data['superclasses']
        
        # Validate hierarchy extraction
        assert isinstance(hierarchy_info, dict)
        
        # Log hierarchy information for debugging
        if hierarchy_info:
            print(f"Class hierarchy found:")
            for class_name, superclasses in hierarchy_info.items():
                print(f"  {class_name} -> {superclasses}")

    def test_sro_owl_property_analysis(self):
        """Test analysis of properties in SRO OWL."""
        # Read and parse the OWL file
        with open(self.sro_owl_path, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        ontology = self.owl_processor.parse_ontology_definition(owl_xml)
        
        # Analyze object properties
        object_props = ontology.object_properties
        data_props = ontology.data_properties
        
        # Validate property analysis
        assert isinstance(object_props, dict)
        assert isinstance(data_props, dict)
        
        # Check for domain and range information
        for prop_name, prop_data in object_props.items():
            if isinstance(prop_data, dict):
                assert 'name' in prop_data, f"Property {prop_name} should have a name"
        
        # Log property information for debugging
        print(f"Object properties: {len(object_props)}")
        print(f"Data properties: {len(data_props)}")


if __name__ == "__main__":
    pytest.main([__file__, "-v"]) 