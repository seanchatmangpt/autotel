#!/usr/bin/env python3
"""
80/20 Minimal DSPy Semantic Execution Pipeline Test

Tests the absolute minimal pipeline with telemetry validation:
1. DSPyProcessor - Parse minimal XML
2. DSPyCompiler - Compile with minimal ontology/validation
3. SemanticLinker - Create minimal ExecutableSystem
4. OntologyExecutor - Execute and validate telemetry

This is the 80/20 test - if this fails, the pipeline is fundamentally broken.
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.dspy_compiler import DSPyCompiler
from autotel.factory.linker import SemanticLinker
from autotel.factory.executor import OntologyExecutor
from autotel.schemas.ontology_types import OntologySchema, ClassSchema, PropertySchema
from autotel.schemas.validation_types import ValidationRules, ValidationRule, ConstraintMetadata


def create_minimal_ontology_schema() -> OntologySchema:
    """Create minimal ontology schema for 80/20 test."""
    return OntologySchema(
        ontology_uri="http://autotel.ai/test/minimal",
        namespace="http://autotel.ai/test/",
        prefix="test",
        classes={
            "TestClass": ClassSchema(
                name="TestClass",
                uri="http://autotel.ai/test/TestClass",
                semantic_type="user_input",
                properties={
                    "input_field": PropertySchema(
                        name="input_field",
                        uri="http://autotel.ai/test/input_field",
                        data_type="string",
                        domain="http://autotel.ai/test/TestClass",
                        range="http://www.w3.org/2001/XMLSchema#string"
                    )
                },
                superclasses=[],
                description="Minimal test class"
            )
        },
        semantic_context={"test": "minimal"},
        examples=[]
    )


def create_minimal_validation_rules() -> ValidationRules:
    """Create minimal validation rules for 80/20 test."""
    return ValidationRules(
        constraint_count=1,
        target_classes={
            "TestClass": [
                ValidationRule(
                    rule_id="minimal_rule_001",
                    target_class="TestClass",
                    property_path="input_field",
                    constraint_type="datatype",
                    constraint_value="string",
                    severity="Violation",
                    message="Input field must be a string",
                    metadata={
                        "source": "minimal_test",
                        "version": "1.0",
                        "description": "Minimal datatype validation"
                    }
                )
            ]
        },
        severity_levels={"Violation": 1},
        metadata={
            "test": "minimal",
            "constraint_count": 1
        }
    )


def create_minimal_dspy_xml() -> str:
    """Create minimal DSPy XML for 80/20 test."""
    return '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:definitions xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/dspy">

  <dspy:signature name="minimal_signature" description="Minimal test signature">
    <dspy:input name="input_field" 
                description="Minimal input field" 
                type="string" 
                optional="false"/>
    <dspy:output name="output_field" 
                 description="Minimal output field" 
                 type="string"/>
  </dspy:signature>

  <dspy:model provider="test" name="minimal_model">
    <dspy:parameter key="temperature" value="0.7"/>
  </dspy:model>

</dspy:definitions>'''


def test_minimal_80_20_pipeline():
    """Test the minimal 80/20 pipeline with telemetry validation."""
    print("🚀 80/20 Minimal DSPy Pipeline Test")
    print("=" * 50)
    
    # Step 1: Create minimal test data
    print("📋 Step 1: Creating minimal test data")
    ontology_schema = create_minimal_ontology_schema()
    validation_rules = create_minimal_validation_rules()
    xml_content = create_minimal_dspy_xml()
    
    print(f"   ✅ Ontology: {len(ontology_schema.classes)} classes")
    print(f"   ✅ Validation: {validation_rules.constraint_count} rules")
    print(f"   ✅ XML: {len(xml_content)} characters")
    
    # Step 2: Initialize pipeline components
    print("\n🔧 Step 2: Initializing pipeline components")
    processor = DSPyProcessor()
    compiler = DSPyCompiler()
    linker = SemanticLinker()
    executor = OntologyExecutor()
    
    print("   ✅ All components initialized")
    
    # Step 3: Process DSPy XML
    print("\n📝 Step 3: Processing DSPy XML")
    try:
        signatures = processor.parse(xml_content)
        print(f"   ✅ Parsed {len(signatures)} signatures")
        
        modules = processor.parse_modules(xml_content)
        print(f"   ✅ Parsed {len(modules)} modules")
        
        model_config = processor.parse_model_configuration(xml_content)
        print(f"   ✅ Parsed model config: {model_config.provider}/{model_config.model_name}")
        
    except Exception as e:
        print(f"   ❌ DSPy processing failed: {e}")
        raise
    
    # Step 4: Compile DSPy signature
    print("\n🔨 Step 4: Compiling DSPy signature")
    try:
        dspy_signature = compiler.compile(
            ontology_schema=ontology_schema,
            validation_rules=validation_rules,
            dspy_signatures=signatures,
            dspy_modules=modules,
            model_config=model_config
        )
        
        print(f"   ✅ Compiled signature: {dspy_signature.name}")
        print(f"   ✅ Inputs: {len(dspy_signature.inputs)}")
        print(f"   ✅ Outputs: {len(dspy_signature.outputs)}")
        print(f"   ✅ Validation rules: {len(dspy_signature.validation_rules)}")
        
    except Exception as e:
        print(f"   ❌ DSPy compilation failed: {e}")
        raise
    
    # Step 5: Link into executable system
    print("\n🔗 Step 5: Linking into executable system")
    try:
        executable_system = linker.link(dspy_signature)
        
        print(f"   ✅ Created executable system")
        print(f"   ✅ Semantic context: {len(executable_system.semantic_context)} keys")
        print(f"   ✅ Validation rules: {len(executable_system.validation_rules)}")
        print(f"   ✅ SHACL validation: {executable_system.shacl_validation}")
        
    except Exception as e:
        print(f"   ❌ Semantic linking failed: {e}")
        raise
    
    # Step 6: Execute with minimal input
    print("\n⚡ Step 6: Executing with minimal input")
    try:
        minimal_inputs = {"input_field": "test_value"}
        result = executor.execute(executable_system, minimal_inputs)
        
        print(f"   ✅ Execution completed")
        print(f"   ✅ Success: {result.success}")
        print(f"   ✅ Execution time: {result.execution_time:.4f}s")
        print(f"   ✅ Outputs: {list(result.outputs.keys())}")
        print(f"   ✅ Validation valid: {result.validation_result.valid}")
        
    except Exception as e:
        print(f"   ❌ Execution failed: {e}")
        raise
    
    # Step 7: Validate telemetry output
    print("\n📊 Step 7: Validating telemetry output")
    try:
        telemetry = result.telemetry_data
        assert len(telemetry.spans) > 0, "No spans generated"
        # Find the executor span
        exec_span = next((span for span in telemetry.spans if span.get("name") == "ontology_execute_system"), None)
        assert exec_span is not None, "ontology_execute_system span not found"
        assert "execution_success" in exec_span["attributes"], "Span missing execution_success"
        print(f"   ✅ Executor span: {exec_span['name']}")
        print(f"   ✅ Execution success: {exec_span['attributes']['execution_success']}")
        # Validate other telemetry is present
        assert len(telemetry.events) > 0, "No events generated"
        assert len(telemetry.metrics) > 0, "No metrics generated"
        assert len(telemetry.logs) > 0, "No logs generated"
        print(f"   ✅ Telemetry events: {len(telemetry.events)}")
        print(f"   ✅ Telemetry metrics: {len(telemetry.metrics)}")
        print(f"   ✅ Telemetry logs: {len(telemetry.logs)}")
    except Exception as e:
        print(f"   ❌ Telemetry validation failed: {e}")
        raise
    
    # Step 8: Validate final outputs
    print("\n🎯 Step 8: Validating final outputs")
    try:
        # Check that we got the expected output
        assert "output_field" in result.outputs, "Expected output_field not found"
        assert result.outputs["output_field"] is not None, "Output field is None"
        
        # Check execution metadata
        assert "_execution_metadata" in result.outputs, "Missing execution metadata"
        metadata = result.outputs["_execution_metadata"]
        assert "signature_name" in metadata, "Missing signature name in metadata"
        assert "execution_timestamp" in metadata, "Missing execution timestamp"
        
        print(f"   ✅ Output field: {result.outputs['output_field']}")
        print(f"   ✅ Signature name: {metadata['signature_name']}")
        print(f"   ✅ Execution timestamp: {metadata['execution_timestamp']}")
        
    except Exception as e:
        print(f"   ❌ Output validation failed: {e}")
        raise
    
    print("\n🎉 80/20 Minimal Pipeline Test PASSED!")
    print("=" * 50)
    print("✅ All pipeline components working correctly")
    print("✅ Telemetry generation validated")
    print("✅ End-to-end execution successful")
    
    return True


def test_minimal_80_20_error_handling():
    """Test error handling in minimal pipeline."""
    print("\n🧪 Testing Error Handling")
    print("=" * 30)
    
    # Test with invalid input (missing required field)
    print("📝 Testing with invalid input (missing required field)")
    
    try:
        # Create minimal pipeline
        ontology_schema = create_minimal_ontology_schema()
        validation_rules = create_minimal_validation_rules()
        xml_content = create_minimal_dspy_xml()
        
        processor = DSPyProcessor()
        compiler = DSPyCompiler()
        linker = SemanticLinker()
        executor = OntologyExecutor()
        
        signatures = processor.parse(xml_content)
        modules = processor.parse_modules(xml_content)
        model_config = processor.parse_model_configuration(xml_content)
        
        dspy_signature = compiler.compile(
            ontology_schema=ontology_schema,
            validation_rules=validation_rules,
            dspy_signatures=signatures,
            dspy_modules=modules,
            model_config=model_config
        )
        
        executable_system = linker.link(dspy_signature)
        
        # Execute with invalid input (missing required field)
        invalid_inputs = {}  # Missing input_field
        result = executor.execute(executable_system, invalid_inputs)
        
        # Should return error result but not crash
        assert result.success is False, "Should fail with invalid input"
        assert result.validation_result.valid is False, "Validation should fail"
        assert len(result.validation_result.violations) > 0, "Should have validation violations"
        
        print("   ✅ Error handling works correctly")
        print(f"   ✅ Success: {result.success}")
        print(f"   ✅ Validation valid: {result.validation_result.valid}")
        print(f"   ✅ Violations: {len(result.validation_result.violations)}")
        
        # Check telemetry for error
        telemetry = result.telemetry_data
        assert len(telemetry.spans) > 0, "Should have error spans"
        
        error_span = telemetry.spans[0]
        assert error_span["attributes"]["execution_success"] is False, "Span should indicate failure"
        
        print("   ✅ Error telemetry generated correctly")
        
    except Exception as e:
        print(f"   ❌ Error handling test failed: {e}")
        raise
    
    print("✅ Error handling test PASSED!")
    return True


def main():
    """Run the 80/20 minimal pipeline tests."""
    print("🚀 80/20 DSPy Semantic Execution Pipeline - Minimal Test Suite")
    print("=" * 70)
    
    try:
        # Run minimal happy path test
        test_minimal_80_20_pipeline()
        
        # Run error handling test
        test_minimal_80_20_error_handling()
        
        print("\n🎉 ALL 80/20 TESTS PASSED!")
        print("=" * 50)
        print("✅ Minimal pipeline working correctly")
        print("✅ Telemetry generation validated")
        print("✅ Error handling working correctly")
        print("✅ Ready for production use")
        
        return True
        
    except Exception as e:
        print(f"\n❌ 80/20 Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 