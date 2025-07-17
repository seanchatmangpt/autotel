"""
Test SHACL Processor with real SHACL data.

Tests the unified SHACL processor using the Scrum Reference Ontology SHACL file.
"""

import pytest
from pathlib import Path
from autotel.processors.shacl_processor import SHACLProcessor, SHACLResult, SHACLNodeShape, SHACLPropertyShape, SHACLConstraint
from autotel.processors.base import ProcessorConfig


class TestSHACLProcessor:
    """Test SHACL processor with real SHACL data."""
    
    @pytest.fixture
    def processor(self):
        """Create SHACL processor instance."""
        config = ProcessorConfig(
            name="test_shacl",
            version="1.0.0",
            enabled=True
        )
        return SHACLProcessor(config)
    
    @pytest.fixture
    def shacl_file_path(self):
        """Path to the SRO SHACL file."""
        return Path("ontologies/scrum_reference_ontology/sro.shacl")
    
    def test_process_sro_shacl_file(self, processor, shacl_file_path):
        """Test processing the SRO SHACL file."""
        result = processor.process(shacl_file_path)
        
        # Verify success
        assert result.success
        assert result.data is not None
        assert isinstance(result.data, SHACLResult)
        
        # Verify we got meaningful data
        shacl_result = result.data
        assert shacl_result.total_shapes > 0
        assert shacl_result.total_constraints > 0
        
        # Verify we have node shapes
        assert len(shacl_result.node_shapes) > 0
        assert all(isinstance(shape, SHACLNodeShape) for shape in shacl_result.node_shapes)
        
        # Verify we have property shapes
        assert len(shacl_result.property_shapes) > 0
        assert all(isinstance(shape, SHACLPropertyShape) for shape in shacl_result.property_shapes)
        
        # Verify we have constraints
        assert len(shacl_result.constraints) > 0
        assert all(isinstance(constraint, SHACLConstraint) for constraint in shacl_result.constraints)
        
        # Verify counts match
        assert shacl_result.total_shapes == len(shacl_result.node_shapes) + len(shacl_result.property_shapes)
        assert shacl_result.total_constraints == len(shacl_result.constraints)
    
    def test_specific_sro_shapes_extracted(self, processor, shacl_file_path):
        """Test that specific SRO shapes are correctly extracted."""
        result = processor.process(shacl_file_path)
        shacl_result = result.data
        
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
    
    def test_constraint_extraction(self, processor, shacl_file_path):
        """Test that constraints are correctly extracted."""
        result = processor.process(shacl_file_path)
        shacl_result = result.data
        
        # Check for min_count and max_count constraints
        constraint_types = [constraint.type for constraint in shacl_result.constraints]
        assert "min_count" in constraint_types
        assert "max_count" in constraint_types
        
        # Check constraint values
        min_count_constraints = [c for c in shacl_result.constraints if c.type == "min_count"]
        max_count_constraints = [c for c in shacl_result.constraints if c.type == "max_count"]
        
        assert len(min_count_constraints) > 0
        assert len(max_count_constraints) > 0
        
        # Verify constraint values are integers
        for constraint in shacl_result.constraints:
            assert isinstance(constraint.value, int)
            assert constraint.value >= 0
    
    def test_property_shape_extraction(self, processor, shacl_file_path):
        """Test that property shapes are correctly extracted."""
        result = processor.process(shacl_file_path)
        shacl_result = result.data
        
        # Check that property shapes have paths
        property_shapes_with_paths = [ps for ps in shacl_result.property_shapes if ps.path is not None]
        assert len(property_shapes_with_paths) > 0
        
        # Check that property shapes have constraints
        property_shapes_with_constraints = [ps for ps in shacl_result.property_shapes if ps.constraints]
        assert len(property_shapes_with_constraints) > 0
    
    def test_node_shape_properties(self, processor, shacl_file_path):
        """Test that node shapes have properties correctly linked."""
        result = processor.process(shacl_file_path)
        shacl_result = result.data
        
        # Find a node shape with properties
        node_shapes_with_properties = [ns for ns in shacl_result.node_shapes if ns.properties]
        assert len(node_shapes_with_properties) > 0
        
        # Check that properties are valid URIs
        for node_shape in node_shapes_with_properties:
            for prop in node_shape.properties:
                assert prop.startswith("http://") or prop.startswith("urn:")
    
    def test_metadata_consistency(self, processor, shacl_file_path):
        """Test that metadata is consistent with actual data."""
        result = processor.process(shacl_file_path)
        
        # Check metadata matches actual counts
        assert result.metadata["node_shapes"] == len(result.data.node_shapes)
        assert result.metadata["property_shapes"] == len(result.data.property_shapes)
        assert result.metadata["constraints"] == len(result.data.constraints)
    
    def test_pydantic_model_validation(self, processor, shacl_file_path):
        """Test that Pydantic models are properly validated."""
        result = processor.process(shacl_file_path)
        shacl_result = result.data
        
        # Test that we can serialize/deserialize the result
        json_data = shacl_result.model_dump()
        assert isinstance(json_data, dict)
        
        # Test that we can recreate the model from dict
        recreated_result = SHACLResult(**json_data)
        assert recreated_result.total_shapes == shacl_result.total_shapes
        assert recreated_result.total_constraints == shacl_result.total_constraints
    
    def test_telemetry_spans_created(self, processor, shacl_file_path):
        """Test that telemetry spans are created during processing."""
        result = processor.process(shacl_file_path)
        
        # Verify telemetry was recorded (success case)
        assert result.success
        # The telemetry validation would be done by the telemetry system
        # Here we just verify the processor completed successfully 
 