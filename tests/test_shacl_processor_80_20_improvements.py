"""Test suite for SHACL processor 80/20 improvements with telemetry validation."""

import pytest
import json
import time
from typing import Dict, Any

from autotel.factory.processors.shacl_processor import SHACLProcessor, PluggableValidator
from autotel.schemas.validation_types import ValidationRule


class TestSHACLProcessor8020Improvements:
    """Test suite for SHACL processor 80/20 improvements."""

    @pytest.fixture
    def shacl_processor(self):
        """Create SHACL processor instance."""
        return SHACLProcessor()

    @pytest.fixture
    def sample_shacl_xml(self):
        """Sample SHACL XML with extended constraints."""
        return """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:shacl="http://www.w3.org/ns/shacl#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    
    <shacl:NodeShape rdf:about="#UserInputShape">
        <shacl:targetClass rdf:resource="#UserInput"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasText"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                <shacl:minLength>1</shacl:minLength>
                <shacl:maxLength>1000</shacl:maxLength>
                <shacl:pattern>^[a-zA-Z0-9\\s]+$</shacl:pattern>
            </shacl:PropertyShape>
        </shacl:property>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasConfidence"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
                <shacl:minInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#float">0.0</shacl:minInclusive>
                <shacl:maxInclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#float">1.0</shacl:maxInclusive>
            </shacl:PropertyShape>
        </shacl:property>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasDescription"/>
                <shacl:minCount>0</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                <shacl:nodeKind rdf:resource="http://www.w3.org/ns/shacl#IRI"/>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
    <shacl:NodeShape rdf:about="#RecommendationShape">
        <shacl:targetClass rdf:resource="#Recommendation"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasScore"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
                <shacl:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
                <shacl:minExclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">0</shacl:minExclusive>
                <shacl:maxExclusive rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">100</shacl:maxExclusive>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
    
</rdf:RDF>"""

    def test_extensible_constraint_types(self, shacl_processor, sample_shacl_xml):
        """Test extensible constraint types support."""
        # Parse SHACL XML
        result = shacl_processor.parse(sample_shacl_xml)
        
        # Validate constraint types are extracted
        constraints = result["constraints"]
        assert len(constraints) > 0
        
        # Check for extended constraint types
        constraint_types = [c.get("type") for c in constraints]
        expected_types = ["min_count", "max_count", "datatype", "min_length", "max_length", 
                         "pattern", "min_inclusive", "max_inclusive", "node_kind", "min_exclusive", "max_exclusive"]
        
        for expected_type in expected_types:
            assert expected_type in constraint_types, f"Expected constraint type {expected_type} not found"
        
        # Validate telemetry records constraint types
        assert "constraint_types_found" in result["metadata"].constraint_types
        assert len(result["metadata"].constraint_types) >= 5

    def test_pluggable_validators(self, shacl_processor):
        """Test pluggable validator system."""
        # Test validator initialization
        initial_validators = shacl_processor.validators.copy()
        assert len(initial_validators) >= 5  # cardinality, datatype, string, range, logical
        
        # Test validator capabilities
        validator_names = [v.name for v in initial_validators]
        expected_validators = ["cardinality", "datatype", "string", "range", "logical"]
        
        for expected_name in expected_validators:
            assert expected_name in validator_names, f"Expected validator {expected_name} not found"
        
        # Test custom validator addition
        def custom_validator(value, constraints, context):
            return {"valid": True, "details": {}}
        
        custom_val = PluggableValidator("custom_test", custom_validator, ["custom_type"])
        shacl_processor.add_validator(custom_val)
        
        # Verify validator was added
        assert len(shacl_processor.validators) == len(initial_validators) + 1
        assert "custom_test" in [v.name for v in shacl_processor.validators]

    def test_advanced_telemetry_integration(self, shacl_processor, sample_shacl_xml):
        """Test advanced telemetry integration."""
        # Parse SHACL XML and capture telemetry
        result = shacl_processor.parse(sample_shacl_xml)
        
        # Validate performance metrics
        assert "performance" in result
        performance = result["performance"]
        
        # Check required performance metrics
        assert "parsing_duration_ms" in performance
        assert "cache_hit_rate" in performance
        assert "constraint_types" in performance
        
        # Validate performance values
        assert performance["parsing_duration_ms"] > 0
        assert 0 <= performance["cache_hit_rate"] <= 1
        assert len(performance["constraint_types"]) > 0
        
        # Validate metadata includes performance information
        metadata = result["metadata"]
        assert metadata.cache_enabled is True
        assert metadata.validator_count >= 5
        assert "performance_metrics" in metadata.__dict__

    def test_schema_driven_error_messages(self, shacl_processor, sample_shacl_xml):
        """Test schema-driven error message generation."""
        # Parse SHACL XML
        result = shacl_processor.parse(sample_shacl_xml)
        validation_rules = result["validation_rules"]
        
        # Validate error messages are generated
        assert len(validation_rules) > 0
        
        for rule in validation_rules:
            # Check message format
            assert rule.message is not None
            assert len(rule.message) > 0
            
            # Check message contains property path
            assert rule.property_path in rule.message
            
            # Check severity context
            if rule.severity == "Warning":
                assert "WARNING:" in rule.message
            elif rule.severity == "Info":
                assert "INFO:" in rule.message

    def test_performance_optimizations(self, shacl_processor, sample_shacl_xml):
        """Test performance optimizations including caching."""
        # First parse - should populate cache
        start_time = time.time()
        result1 = shacl_processor.parse(sample_shacl_xml)
        first_parse_time = time.time() - start_time
        
        # Second parse - should use cache
        start_time = time.time()
        result2 = shacl_processor.parse(sample_shacl_xml)
        second_parse_time = time.time() - start_time
        
        # Validate cache is working (second parse should be faster or similar)
        assert second_parse_time <= first_parse_time * 1.5  # Allow some variance
        
        # Validate cache hit rate is reported
        assert result1["performance"]["cache_hit_rate"] >= 0
        assert result2["performance"]["cache_hit_rate"] >= 0
        
        # Test cache clearing
        shacl_processor.clear_cache()
        assert len(shacl_processor._get_cached_graph.cache_info()) == 0

    def test_extended_validation_with_telemetry(self, shacl_processor, sample_shacl_xml):
        """Test extended validation with comprehensive telemetry."""
        # Parse SHACL XML
        result = shacl_processor.parse(sample_shacl_xml)
        validation_rules = result["validation_rules"]
        
        # Test data
        test_data = {
            "hasText": "Valid text input",
            "hasConfidence": 0.8,
            "hasDescription": "http://example.com/description",
            "hasScore": 75
        }
        
        # Validate data
        validation_result = shacl_processor.validate_data(test_data, validation_rules)
        
        # Validate validation results structure
        assert "valid" in validation_result
        assert "violations" in validation_result
        assert "warnings" in validation_result
        assert "info" in validation_result
        assert "performance" in validation_result
        
        # Validate performance metrics
        performance = validation_result["performance"]
        assert "validation_duration_ms" in performance
        assert "constraint_type_distribution" in performance
        assert "validator_usage" in performance
        assert "cache_hit_rate" in performance
        
        # Validate performance values
        assert performance["validation_duration_ms"] > 0
        assert len(performance["constraint_type_distribution"]) > 0
        assert len(performance["validator_usage"]) > 0

    def test_constraint_categorization(self, shacl_processor):
        """Test constraint categorization system."""
        # Test constraint categorization
        test_constraints = [
            "min_count", "max_count", "datatype", "min_length", "max_length",
            "pattern", "min_inclusive", "max_inclusive", "node_kind"
        ]
        
        for constraint in test_constraints:
            category = shacl_processor._categorize_constraint(constraint)
            assert category != "unknown", f"Constraint {constraint} should be categorized"
            
            # Validate category is valid
            valid_categories = ["cardinality", "datatype", "value", "range", "string", "logical", "comparison", "custom"]
            assert category in valid_categories, f"Invalid category {category} for constraint {constraint}"

    def test_validator_statistics(self, shacl_processor):
        """Test validator statistics functionality."""
        stats = shacl_processor.get_validator_stats()
        
        # Validate stats structure
        assert "total_validators" in stats
        assert "validator_names" in stats
        assert "constraint_types_supported" in stats
        assert "cache_hit_rate" in stats
        
        # Validate stats values
        assert stats["total_validators"] >= 5
        assert len(stats["validator_names"]) >= 5
        assert len(stats["constraint_types_supported"]) > 0
        assert 0 <= stats["cache_hit_rate"] <= 1

    def test_error_handling_with_telemetry(self, shacl_processor):
        """Test error handling with telemetry integration."""
        # Test with invalid XML
        invalid_xml = "<invalid>xml</invalid>"
        
        try:
            shacl_processor.parse(invalid_xml)
            assert False, "Should have raised an exception"
        except Exception as e:
            # Validate error was handled properly
            assert str(e) is not None
            assert len(str(e)) > 0

    def test_comprehensive_telemetry_validation(self, shacl_processor, sample_shacl_xml):
        """Test comprehensive telemetry validation for all improvements."""
        # Parse SHACL XML
        result = shacl_processor.parse(sample_shacl_xml)
        
        # Validate all telemetry attributes are present
        assert "graph" in result
        assert "node_shapes" in result
        assert "property_shapes" in result
        assert "constraints" in result
        assert "validation_rules" in result
        assert "metadata" in result
        assert "performance" in result
        
        # Validate metadata completeness
        metadata = result["metadata"]
        assert metadata.total_constraints > 0
        assert len(metadata.constraint_types) > 0
        assert len(metadata.constraint_categories) > 0
        assert metadata.node_shapes_count > 0
        assert metadata.property_shapes_count > 0
        assert len(metadata.target_classes) > 0
        assert metadata.validator_count >= 5
        assert metadata.cache_enabled is True
        assert "performance_metrics" in metadata.__dict__
        
        # Validate performance metrics
        performance = result["performance"]
        assert performance["parsing_duration_ms"] > 0
        assert 0 <= performance["cache_hit_rate"] <= 1
        assert len(performance["constraint_types"]) > 0
        
        # Validate constraint extraction
        constraints = result["constraints"]
        assert len(constraints) > 0
        
        # Validate validation rules
        validation_rules = result["validation_rules"]
        assert len(validation_rules) > 0
        
        # Test data validation
        test_data = {
            "hasText": "Test input",
            "hasConfidence": 0.5,
            "hasDescription": "http://example.com/desc",
            "hasScore": 50
        }
        
        validation_result = shacl_processor.validate_data(test_data, validation_rules)
        
        # Validate validation telemetry
        assert validation_result["performance"]["validation_duration_ms"] > 0
        assert len(validation_result["performance"]["constraint_type_distribution"]) > 0
        assert len(validation_result["performance"]["validator_usage"]) > 0

    def test_80_20_improvements_summary(self, shacl_processor, sample_shacl_xml):
        """Test summary of all 80/20 improvements."""
        # Test all improvements in one comprehensive test
        
        # 1. Extensible constraint types
        result = shacl_processor.parse(sample_shacl_xml)
        constraint_types = [c.get("type") for c in result["constraints"]]
        assert len(constraint_types) >= 10  # Should support many constraint types
        
        # 2. Pluggable validators
        assert len(shacl_processor.validators) >= 5
        validator_names = [v.name for v in shacl_processor.validators]
        assert "cardinality" in validator_names
        assert "datatype" in validator_names
        assert "string" in validator_names
        assert "range" in validator_names
        
        # 3. Advanced telemetry
        assert "performance" in result
        assert result["performance"]["parsing_duration_ms"] > 0
        assert result["performance"]["cache_hit_rate"] >= 0
        
        # 4. Schema-driven error messages
        validation_rules = result["validation_rules"]
        for rule in validation_rules:
            assert rule.message is not None
            assert len(rule.message) > 0
            assert rule.property_path in rule.message
        
        # 5. Performance optimizations
        # Test caching
        result2 = shacl_processor.parse(sample_shacl_xml)
        assert result2["performance"]["cache_hit_rate"] >= 0
        
        # Test cache clearing
        shacl_processor.clear_cache()
        assert len(shacl_processor._get_cached_graph.cache_info()) == 0
        
        # 6. Validation with telemetry
        test_data = {"hasText": "test", "hasConfidence": 0.5, "hasScore": 50}
        validation_result = shacl_processor.validate_data(test_data, validation_rules)
        
        assert "performance" in validation_result
        assert validation_result["performance"]["validation_duration_ms"] > 0
        assert len(validation_result["performance"]["constraint_type_distribution"]) > 0
        assert len(validation_result["performance"]["validator_usage"]) > 0
        
        # 7. Validator statistics
        stats = shacl_processor.get_validator_stats()
        assert stats["total_validators"] >= 5
        assert len(stats["validator_names"]) >= 5
        assert len(stats["constraint_types_supported"]) > 0
        
        # 8. Constraint categorization
        for constraint_type in constraint_types:
            category = shacl_processor._categorize_constraint(constraint_type)
            assert category != "unknown"
        
        # Validate telemetry integration
        assert result["metadata"].validator_count >= 5
        assert result["metadata"].cache_enabled is True
        assert len(result["metadata"].constraint_types) > 0
        assert len(result["metadata"].constraint_categories) > 0 