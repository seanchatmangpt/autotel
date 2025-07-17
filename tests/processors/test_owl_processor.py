import pytest
from pathlib import Path
from autotel.processors.owl_processor import OWLProcessor
from autotel.processors.base import ProcessorConfig
from autotel.schemas.ontology_types import OWLOntologyDefinition

@pytest.fixture
def processor():
    config = ProcessorConfig(name="owl_processor")
    return OWLProcessor(config=config)

@pytest.fixture
def owl_file_path():
    return Path("ontologies/scrum_reference_ontology/sro.owl")

def test_owl_processor_happy_path(processor, owl_file_path):
    xml_content = owl_file_path.read_text(encoding="utf-8")
    result = processor.process(xml_content)
    assert result.success
    assert isinstance(result.data, OWLOntologyDefinition)
    assert result.data.ontology_uri.startswith("http://example.com/sro")
    assert result.data.namespace.startswith("http://example.com/sro")
    assert result.data.prefix == "owl"
    # Check for key classes
    for class_name in [
        "ScrumProject", "ScrumProcess", "Sprint", "UserStory", "ProductBacklogDefinition"
    ]:
        assert class_name in result.data.classes
        class_data = result.data.classes[class_name]
        assert class_data["uri"].startswith("http://example.com/sro#")
    # Check for key object properties
    for prop_name in [
        "scrum_process", "product_backlog_definition", "sprints", "product_backlog"
    ]:
        assert prop_name in result.data.object_properties
        prop_data = result.data.object_properties[prop_name]
        assert prop_data["uri"].startswith("http://example.com/sro#")
    # Check for key data properties
    for prop_name in ["importance", "effort"]:
        assert prop_name in result.data.data_properties
        prop_data = result.data.data_properties[prop_name]
        assert prop_data["uri"].startswith("http://example.com/sro#")
    # Check for individuals and axioms (should be empty or minimal in SRO)
    assert isinstance(result.data.individuals, dict)
    assert isinstance(result.data.axioms, list) 
 