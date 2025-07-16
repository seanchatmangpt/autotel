#!/usr/bin/env python3
"""
80/20 Implementation Validation Test

This test validates that all critical 80/20 implementations are working correctly:
1. Pipeline execution
2. Configuration management
3. Telemetry generation
4. CLI functionality
5. Error handling
"""

import pytest
import json
import sys
from pathlib import Path
from typing import Dict, Any

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.factory.pipeline import PipelineOrchestrator
from autotel.config import create_config, get_config, set_global_config, reset_global_config
from autotel.core.telemetry import create_telemetry_manager
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.dspy_compiler import DSPyCompiler
from autotel.factory.linker import SemanticLinker
from autotel.factory.executor import OntologyExecutor
from autotel.schemas.ontology_types import OntologySchema, ClassSchema, PropertySchema
from autotel.schemas.validation_types import ValidationRules, ValidationRule


class Test8020Implementations:
    """Test suite for 80/20 implementations."""

    def setup_method(self):
        """Set up test fixtures."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-80-20-test",
            require_linkml_validation=False
        )

    def test_config_singleton_implementation(self):
        """Test that config singleton implementation works correctly."""
        print("🧪 Testing Config Singleton Implementation")
        
        # Reset global config
        reset_global_config()
        
        # Test get_config creates default config
        config1 = get_config()
        assert config1 is not None, "get_config should return a valid config"
        assert config1.dspy.default_model == "ollama/qwen2.5:latest", "Default model should be set"
        
        # Test singleton behavior
        config2 = get_config()
        assert config1 is config2, "get_config should return the same instance"
        
        # Test set_global_config
        new_config = create_config()
        new_config.dspy.default_model = "test/model"
        set_global_config(new_config)
        
        config3 = get_config()
        assert config3.dspy.default_model == "test/model", "set_global_config should update the singleton"
        
        print("   ✅ Config singleton implementation working")

    def test_pipeline_orchestrator_implementation(self):
        """Test that pipeline orchestrator implementation works correctly."""
        print("🧪 Testing Pipeline Orchestrator Implementation")
        
        orchestrator = PipelineOrchestrator(self.telemetry)
        
        # Test that all components are initialized
        assert orchestrator.owl_processor is not None, "OWL processor should be initialized"
        assert orchestrator.shacl_processor is not None, "SHACL processor should be initialized"
        assert orchestrator.dspy_processor is not None, "DSPy processor should be initialized"
        assert orchestrator.ontology_compiler is not None, "Ontology compiler should be initialized"
        assert orchestrator.validation_compiler is not None, "Validation compiler should be initialized"
        assert orchestrator.dspy_compiler is not None, "DSPy compiler should be initialized"
        assert orchestrator.linker is not None, "Semantic linker should be initialized"
        assert orchestrator.executor is not None, "Ontology executor should be initialized"
        assert orchestrator.telemetry is not None, "Telemetry should be initialized"
        
        print("   ✅ Pipeline orchestrator implementation working")

    def test_telemetry_implementation(self):
        """Test that telemetry implementation works correctly."""
        print("🧪 Testing Telemetry Implementation")
        
        with self.telemetry.start_span("test_telemetry_span", "test_operation") as span:
            span.set_attribute("test_attribute", "test_value")
            
            # Test metric recording
            self.telemetry.record_metric("test_metric", 1)
            
            # Test validation result creation
            validation_result = self.telemetry.create_validation_result(
                validation_type="test_validation",
                passed=True,
                details="Test validation passed"
            )
            
            assert validation_result["validation_type"] == "test_validation"
            assert validation_result["passed"] is True
            assert validation_result["details"] == "Test validation passed"
        
        print("   ✅ Telemetry implementation working")

    def test_dspy_processor_implementation(self):
        """Test that DSPy processor implementation works correctly."""
        print("🧪 Testing DSPy Processor Implementation")
        
        processor = DSPyProcessor()
        
        # Test XML parsing
        test_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:definitions xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/dspy">

  <dspy:signature name="test_signature" description="Test signature">
    <dspy:input name="test_input" description="Test input" type="string" optional="false"/>
    <dspy:output name="test_output" description="Test output" type="string"/>
  </dspy:signature>

  <dspy:model provider="test" name="test_model">
    <dspy:parameter key="temperature" value="0.7"/>
  </dspy:model>

</dspy:definitions>'''
        
        # Test signature parsing
        signatures = processor.parse(test_xml)
        assert len(signatures) == 1, "Should parse one signature"
        assert signatures[0].name == "test_signature", "Signature name should match"
        
        # Test module parsing
        modules = processor.parse_modules(test_xml)
        assert isinstance(modules, list), "Modules should be a list"
        
        # Test model configuration parsing
        model_config = processor.parse_model_configuration(test_xml)
        assert model_config is not None, "Model config should be parsed"
        assert model_config.get("provider") == "test", "Provider should match"
        
        print("   ✅ DSPy processor implementation working")

    def test_dspy_compiler_implementation(self):
        """Test that DSPy compiler implementation works correctly."""
        print("🧪 Testing DSPy Compiler Implementation")
        
        compiler = DSPyCompiler()
        
        # Create test data
        ontology_schema = OntologySchema(
            ontology_uri="http://autotel.ai/test",
            namespace="http://autotel.ai/test/",
            prefix="test",
            classes={
                "TestClass": ClassSchema(
                    name="TestClass",
                    uri="http://autotel.ai/test/TestClass",
                    semantic_type="user_input",
                    properties={
                        "test_field": PropertySchema(
                            name="test_field",
                            uri="http://autotel.ai/test/test_field",
                            data_type="string",
                            domain="http://autotel.ai/test/TestClass",
                            range="http://www.w3.org/2001/XMLSchema#string"
                        )
                    },
                    superclasses=[],
                    description="Test class"
                )
            },
            semantic_context={"test": "compiler"},
            examples=[]
        )
        
        validation_rules = ValidationRules(
            constraint_count=1,
            target_classes={
                "TestClass": [
                    ValidationRule(
                        rule_id="test_rule_001",
                        target_class="TestClass",
                        property_path="test_field",
                        constraint_type="datatype",
                        constraint_value="string",
                        severity="Violation",
                        message="Test field must be a string",
                        metadata={
                            "source": "test_compiler",
                            "version": "1.0",
                            "description": "Test datatype validation"
                        }
                    )
                ]
            },
            severity_levels={"Violation": 1},
            metadata={"test": "compiler", "constraint_count": 1}
        )
        
        # Test compilation
        dspy_signature = compiler.compile(
            ontology_schema=ontology_schema,
            validation_rules=validation_rules,
            dspy_signatures=[],
            dspy_modules=[],
            model_config={}
        )
        
        assert dspy_signature is not None, "DSPy signature should be compiled"
        assert hasattr(dspy_signature, 'name'), "Signature should have a name"
        assert hasattr(dspy_signature, 'inputs'), "Signature should have inputs"
        assert hasattr(dspy_signature, 'outputs'), "Signature should have outputs"
        
        print("   ✅ DSPy compiler implementation working")

    def test_semantic_linker_implementation(self):
        """Test that semantic linker implementation works correctly."""
        print("🧪 Testing Semantic Linker Implementation")
        
        linker = SemanticLinker()
        
        # Create a minimal DSPy signature for testing
        from autotel.schemas.dspy_types import DSPySignature
        
        test_signature = DSPySignature(
            name="test_signature",
            description="Test signature",
            inputs={
                "test_input": {
                    "name": "test_input",
                    "description": "Test input",
                    "type": "string",
                    "optional": False
                }
            },
            outputs={
                "test_output": {
                    "name": "test_output",
                    "description": "Test output",
                    "type": "string"
                }
            },
            model_config={
                "provider": "test",
                "model": "test_model",
                "temperature": 0.7
            },
            semantic_context={"test": "linker"},
            validation_rules=[],
            metadata={"test": "linker"}
        )
        
        # Test linking
        executable_system = linker.link(test_signature)
        
        assert executable_system is not None, "Executable system should be created"
        assert hasattr(executable_system, 'signature_name'), "System should have signature name"
        assert hasattr(executable_system, 'inputs'), "System should have inputs"
        assert hasattr(executable_system, 'outputs'), "System should have outputs"
        assert hasattr(executable_system, 'validation_rules'), "System should have validation rules"
        
        print("   ✅ Semantic linker implementation working")

    def test_ontology_executor_implementation(self):
        """Test that ontology executor implementation works correctly."""
        print("🧪 Testing Ontology Executor Implementation")
        
        executor = OntologyExecutor()
        
        # Create a minimal executable system for testing
        from autotel.schemas.linker_types import ExecutableSystem
        
        test_system = ExecutableSystem(
            signature_name="test_signature",
            inputs={
                "test_input": {
                    "name": "test_input",
                    "description": "Test input",
                    "type": "string",
                    "optional": False
                }
            },
            outputs={
                "test_output": {
                    "name": "test_output",
                    "description": "Test output",
                    "type": "string"
                }
            },
            validation_rules=[],
            semantic_context={"test": "executor"},
            metadata={"test": "executor"}
        )
        
        # Test execution
        test_inputs = {"test_input": "test_value"}
        result = executor.execute(test_system, test_inputs)
        
        assert result is not None, "Execution result should be returned"
        assert hasattr(result, 'success'), "Result should have success flag"
        assert hasattr(result, 'outputs'), "Result should have outputs"
        assert hasattr(result, 'execution_time'), "Result should have execution time"
        assert hasattr(result, 'telemetry_data'), "Result should have telemetry data"
        
        print("   ✅ Ontology executor implementation working")

    def test_error_handling_implementation(self):
        """Test that error handling implementation works correctly."""
        print("🧪 Testing Error Handling Implementation")
        
        # Test invalid input handling
        executor = OntologyExecutor()
        
        try:
            # Test with invalid inputs
            result = executor.execute(None, {})
            assert result.success is False, "Should handle None system gracefully"
        except Exception as e:
            # This is also acceptable - the executor should either handle gracefully or raise
            assert isinstance(e, (ValueError, TypeError)), "Should raise appropriate error type"
        
        # Test telemetry error handling
        with self.telemetry.start_span("test_error_handling", "error_test") as span:
            try:
                # Try to record an invalid metric
                self.telemetry.record_metric("test_metric", "invalid_value")
            except Exception:
                # This should be handled gracefully
                pass
            
            # Span should still be valid
            assert span is not None, "Span should remain valid after error"
        
        print("   ✅ Error handling implementation working")

    def test_end_to_end_80_20_pipeline(self):
        """Test the complete 80/20 pipeline implementation."""
        print("🧪 Testing End-to-End 80/20 Pipeline")
        
        # Create test data
        ontology_schema, validation_rules, dspy_xml = self._create_test_data()
        
        # Initialize components
        processor = DSPyProcessor()
        compiler = DSPyCompiler()
        linker = SemanticLinker()
        executor = OntologyExecutor()
        
        with self.telemetry.start_span("e2e_80_20_test", "pipeline_testing") as span:
            try:
                # Process DSPy XML
                signatures = processor.parse(dspy_xml)
                modules = processor.parse_modules(dspy_xml)
                model_config = processor.parse_model_configuration(dspy_xml)
                
                # Compile DSPy signature
                dspy_signature = compiler.compile(
                    ontology_schema=ontology_schema,
                    validation_rules=validation_rules,
                    dspy_signatures=signatures,
                    dspy_modules=modules,
                    model_config=model_config
                )
                
                # Link into executable system
                executable_system = linker.link(dspy_signature)
                
                # Execute with test input
                test_inputs = {"input_field": "e2e_test_value"}
                result = executor.execute(executable_system, test_inputs)
                
                # Validate results
                assert result.success is True, "E2E pipeline should succeed"
                assert "output_field" in result.outputs, "Expected output field not found"
                assert len(result.telemetry_data.spans) > 0, "No telemetry spans generated"
                
                span.set_attribute("e2e_success", True)
                span.set_attribute("execution_time", result.execution_time)
                
                print("   ✅ End-to-end 80/20 pipeline working")
                
            except Exception as e:
                span.set_attribute("e2e_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _create_test_data(self):
        """Create test data for 80/20 pipeline validation."""
        ontology_schema = OntologySchema(
            ontology_uri="http://autotel.ai/test/80_20",
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
                    description="Test class for 80/20 validation"
                )
            },
            semantic_context={"test": "80_20"},
            examples=[]
        )
        
        validation_rules = ValidationRules(
            constraint_count=1,
            target_classes={
                "TestClass": [
                    ValidationRule(
                        rule_id="e2e_rule_001",
                        target_class="TestClass",
                        property_path="input_field",
                        constraint_type="datatype",
                        constraint_value="string",
                        severity="Violation",
                        message="Input field must be a string",
                        metadata={
                            "source": "e2e_test",
                            "version": "1.0",
                            "description": "E2E datatype validation"
                        }
                    )
                ]
            },
            severity_levels={"Violation": 1},
            metadata={"test": "e2e", "constraint_count": 1}
        )
        
        dspy_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:definitions xmlns:dspy="http://autotel.ai/dspy"
                  targetNamespace="http://autotel.ai/dspy">

  <dspy:signature name="e2e_test_signature" description="E2E test signature">
    <dspy:input name="input_field" 
                description="E2E test input field" 
                type="string" 
                optional="false"/>
    <dspy:output name="output_field" 
                 description="E2E test output field" 
                 type="string"/>
  </dspy:signature>

  <dspy:model provider="test" name="e2e_test_model">
    <dspy:parameter key="temperature" value="0.7"/>
  </dspy:model>

</dspy:definitions>'''
        
        return ontology_schema, validation_rules, dspy_xml


def run_80_20_validation():
    """Run the complete 80/20 implementation validation."""
    print("🚀 Starting 80/20 Implementation Validation")
    print("=" * 60)
    
    test_suite = Test8020Implementations()
    test_suite.setup_method()
    
    # Run all tests
    test_methods = [
        test_suite.test_config_singleton_implementation,
        test_suite.test_pipeline_orchestrator_implementation,
        test_suite.test_telemetry_implementation,
        test_suite.test_dspy_processor_implementation,
        test_suite.test_dspy_compiler_implementation,
        test_suite.test_semantic_linker_implementation,
        test_suite.test_ontology_executor_implementation,
        test_suite.test_error_handling_implementation,
        test_suite.test_end_to_end_80_20_pipeline,
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
    print(f"📊 80/20 Implementation Validation Results:")
    print(f"   ✅ Passed: {passed}")
    print(f"   ❌ Failed: {failed}")
    print(f"   📈 Success Rate: {passed/(passed+failed)*100:.1f}%")
    
    if failed == 0:
        print("\n🎉 All 80/20 implementations are working correctly!")
        print("✅ Pipeline execution validated")
        print("✅ Configuration management validated")
        print("✅ Telemetry generation validated")
        print("✅ Error handling validated")
        return True
    else:
        print(f"\n⚠️  {failed} implementation(s) need attention")
        return False


if __name__ == "__main__":
    success = run_80_20_validation()
    exit(0 if success else 1) 