import pytest
from pathlib import Path
from autotel.processors.shacl_processor import SHACLProcessor, SHACLResult
from autotel.processors.base import ProcessorConfig

@pytest.fixture
def processor():
    config = ProcessorConfig(name="shacl_processor")
    return SHACLProcessor(config=config)

@pytest.fixture
def shacl_file_path():
    return Path("ontologies/scrum_reference_ontology/sro.shacl")

def test_shacl_processor_happy_path(processor, shacl_file_path):
    result = processor.process(shacl_file_path)
    assert result.success
    assert isinstance(result.data, SHACLResult)
    shacl_result = result.data
    assert shacl_result.total_shapes > 0
    assert shacl_result.total_constraints > 0
    assert len(shacl_result.node_shapes) > 0
    assert len(shacl_result.property_shapes) > 0
    assert len(shacl_result.constraints) > 0
    assert shacl_result.total_shapes == len(shacl_result.node_shapes) + len(shacl_result.property_shapes)
    assert shacl_result.total_constraints == len(shacl_result.constraints)
    # Check for specific node shapes we know should exist
    shape_ids = [shape.shape_id for shape in shacl_result.node_shapes]
    expected_shapes = [
        "http://example.com/sro#ScrumProjectShape",
        "http://example.com/sro#ScrumProcessShape",
        "http://example.com/sro#SprintShape",
        "http://example.com/sro#UserStoryShape"
    ]
    for expected_shape in expected_shapes:
        assert expected_shape in shape_ids, f"Expected shape {expected_shape} not found"
    # Check for constraint types
    constraint_types = [constraint.type for constraint in shacl_result.constraints]
    assert "min_count" in constraint_types
    assert "max_count" in constraint_types
    # Verify constraint values are integers
    for constraint in shacl_result.constraints:
        assert isinstance(constraint.value, int)
        assert constraint.value >= 0 
 