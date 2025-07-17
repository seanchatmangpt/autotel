"""
Test OWL Processor with real SRO ontology data.

Validates the unified OWL processor using the Scrum Reference Ontology OWL file.
"""

import pytest
from pathlib import Path
from autotel.processors.owl_processor import OWLProcessor
from autotel.schemas.ontology_types import OWLOntologyDefinition


class TestOWLProcessor:
    """Test OWL processor with real OWL ontology data."""

    @pytest.fixture
    def processor(self):
        """Create OWL processor instance."""
        from autotel.processors.base import ProcessorConfig
        config = ProcessorConfig(name="owl_processor")
        return OWLProcessor(config=config)

    @pytest.fixture
    def owl_file_path(self):
        """Path to the SRO OWL file."""
        return Path("ontologies/scrum_reference_ontology/sro.owl")

    def test_process_sro_owl_file(self, processor, owl_file_path):
        """Test processing the SRO OWL file."""
        xml_content = owl_file_path.read_text(encoding="utf-8")
        result = processor.process(xml_content)

        # Verify result is correct type
        assert result.success
        assert isinstance(result.data, OWLOntologyDefinition)
        assert result.data.ontology_uri.startswith("http://example.com/sro")
        assert result.data.namespace.startswith("http://example.com/sro")
        assert result.data.prefix == "owl"

        # Check for key classes
        for class_name in [
            "ScrumProject", "ScrumProcess", "Sprint", "UserStory", "ProductBacklogDefinition"
        ]:
            assert class_name in result.data.classes, f"Expected class {class_name} not found"
            class_data = result.data.classes[class_name]
            assert class_data["uri"].startswith("http://example.com/sro#"), f"Class URI not expanded: {class_data['uri']}"

        # Check for key object properties
        for prop_name in [
            "scrum_process", "product_backlog_definition", "sprints", "product_backlog"
        ]:
            assert prop_name in result.data.object_properties, f"Expected object property {prop_name} not found"
            prop_data = result.data.object_properties[prop_name]
            assert prop_data["uri"].startswith("http://example.com/sro#"), f"Property URI not expanded: {prop_data['uri']}"

        # Check for key data properties
        for prop_name in ["importance", "effort"]:
            assert prop_name in result.data.data_properties, f"Expected data property {prop_name} not found"
            prop_data = result.data.data_properties[prop_name]
            assert prop_data["uri"].startswith("http://example.com/sro#"), f"Data property URI not expanded: {prop_data['uri']}"

        # Check for individuals and axioms (should be empty or minimal in SRO)
        assert isinstance(result.data.individuals, dict)
        assert isinstance(result.data.axioms, list)

    def test_owl_processor_metadata(self, processor):
        """Test that the processor has correct metadata and integrates with meta/base."""
        # Should have telemetry and meta attributes
        assert hasattr(processor, "telemetry")
        assert hasattr(processor, "config")
        assert hasattr(processor, "name")
        assert hasattr(processor, "version")
        assert hasattr(processor, "description")
        
        # Should work with base processor interface
        assert processor.__class__.__name__ == "OWLProcessor"
        assert "Processes OWL/RDF XML" in processor.__class__.__doc__
        assert processor.name == "owl_processor"
        assert processor.version == "1.0.0"
        assert processor.description == "Processes OWL/RDF XML into structured ontology definitions" 
 