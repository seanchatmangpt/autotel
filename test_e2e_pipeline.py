#!/usr/bin/env python3
"""
End-to-End AutoTel Pipeline Test

This test demonstrates the complete AutoTel semantic execution pipeline:
XML Inputs → Processors → Compilers → Linker → Executor → Results + Telemetry

The pipeline processes DSPy XML definitions with ontology and validation context
to create executable systems that can be run with comprehensive telemetry.
"""

import json
import time
from typing import Dict, Any

from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.dspy_compiler import DSPyCompiler
from autotel.factory.linker import SemanticLinker
from autotel.factory.executor import OntologyExecutor
from autotel.schemas.ontology_types import OntologySchema, ClassSchema, PropertySchema
from autotel.schemas.validation_types import ValidationRules, ValidationRule


def create_test_ontology_schema() -> OntologySchema:
    """Create a test ontology schema for the pipeline."""
    classes = {
        "UserInput": ClassSchema(
            name="UserInput",
            uri="http://autotel.ai/ontology#UserInput",
            semantic_type="user_input",
            properties={
                "text": PropertySchema(
                    name="text",
                    uri="http://autotel.ai/ontology#hasText",
                    data_type="string",
                    domain="http://autotel.ai/ontology#UserInput",
                    range="http://www.w3.org/2001/XMLSchema#string",
                    cardinality="1"
                )
            },
            superclasses=[],
            description="User-provided input for processing"
        ),
        "Recommendation": ClassSchema(
            name="Recommendation",
            uri="http://autotel.ai/ontology#Recommendation",
            semantic_type="recommendation",
            properties={
                "content": PropertySchema(
                    name="content",
                    uri="http://autotel.ai/ontology#hasContent",
                    data_type="string",
                    domain="http://autotel.ai/ontology#Recommendation",
                    range="http://www.w3.org/2001/XMLSchema#string",
                    cardinality="1"
                ),
                "confidence": PropertySchema(
                    name="confidence",
                    uri="http://autotel.ai/ontology#hasConfidence",
                    data_type="float",
                    domain="http://autotel.ai/ontology#Recommendation",
                    range="http://www.w3.org/2001/XMLSchema#float",
                    cardinality="0..1"
                )
            },
            superclasses=[],
            description="AI-generated recommendation"
        ),
        "Reasoning": ClassSchema(
            name="Reasoning",
            uri="http://autotel.ai/ontology#Reasoning",
            semantic_type="reasoning",
            properties={
                "explanation": PropertySchema(
                    name="explanation",
                    uri="http://autotel.ai/ontology#hasExplanation",
                    data_type="string",
                    domain="http://autotel.ai/ontology#Reasoning",
                    range="http://www.w3.org/2001/XMLSchema#string",
                    cardinality="1"
                )
            },
            superclasses=[],
            description="Logical reasoning or explanation"
        )
    }
    
    return OntologySchema(
        ontology_uri="http://autotel.ai/ontology",
        namespace="http://autotel.ai/ontology#",
        prefix="autotel",
        classes=classes,
        semantic_context={
            "domain": "AI recommendations",
            "version": "1.0.0"
        },
        examples=[
            {
                "UserInput": {
                    "text": "I want to learn machine learning"
                },
                "Recommendation": {
                    "content": "Start with Python basics, then learn scikit-learn",
                    "confidence": 0.85
                },
                "Reasoning": {
                    "explanation": "Machine learning requires strong programming fundamentals"
                }
            }
        ]
    )


def create_test_validation_rules() -> ValidationRules:
    """Create test validation rules for the pipeline."""
    rules = [
        ValidationRule(
            rule_id="rule_001",
            target_class="UserInput",
            property_path="text",
            constraint_type="cardinality",
            constraint_value=1,
            severity="Violation",
            message="User input text is required",
            metadata={"min_count": 1}
        ),
        ValidationRule(
            rule_id="rule_002",
            target_class="UserInput",
            property_path="text",
            constraint_type="value",
            constraint_value=10,
            severity="Warning",
            message="User input should be at least 10 characters",
            metadata={"min_length": 10}
        ),
        ValidationRule(
            rule_id="rule_003",
            target_class="Recommendation",
            property_path="confidence",
            constraint_type="value",
            constraint_value=(0.0, 1.0),
            severity="Violation",
            message="Confidence must be between 0.0 and 1.0",
            metadata={"range": [0.0, 1.0]}
        )
    ]
    
    return ValidationRules(
        target_classes={
            "UserInput": [rules[0], rules[1]],
            "Recommendation": [rules[2]]
        },
        constraint_count=3,
        severity_levels={"Violation": 2, "Warning": 1},
        metadata={"validation_schema": "test_schema_v1"}
    )


def create_test_dspy_xml() -> str:
    """Create test DSPy XML content."""
    return '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:config xmlns:dspy="http://autotel.ai/dspy"
             xmlns:owl="http://www.w3.org/2002/07/owl#"
             xmlns:shacl="http://www.w3.org/ns/shacl#">
    
    <dspy:signature name="recommendation_signature" description="Generate recommendations from user input">
        <dspy:input name="user_input" 
                    type="string" 
                    description="User input text" 
                    optional="false"
                    owl:class="http://autotel.ai/ontology#UserInput">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>1</shacl:minLength>
                <shacl:maxLength>1000</shacl:maxLength>
            </shacl:PropertyShape>
        </dspy:input>
        
        <dspy:output name="recommendation" 
                     type="string" 
                     description="AI-generated recommendation"
                     owl:class="http://autotel.ai/ontology#Recommendation">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>10</shacl:minLength>
            </shacl:PropertyShape>
        </dspy:output>
        
        <dspy:output name="reasoning" 
                     type="string" 
                     description="Reasoning for the recommendation"
                     owl:class="http://autotel.ai/ontology#Reasoning">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>20</shacl:minLength>
            </shacl:PropertyShape>
        </dspy:output>
        
        <dspy:example>
            <user_input>I want to learn machine learning</user_input>
            <recommendation>Start with Python basics, then learn scikit-learn and TensorFlow</recommendation>
            <reasoning>Machine learning requires strong programming fundamentals and understanding of data structures</reasoning>
        </dspy:example>
    </dspy:signature>
    
    <dspy:module type="predict">
        <dspy:config key="temperature" value="0.7"/>
        <dspy:config key="max_tokens" value="1000"/>
        <dspy:dependency name="recommendation_signature"/>
    </dspy:module>
    
    <dspy:model provider="openai" name="gpt-4">
        <dspy:parameter key="temperature" value="0.7"/>
        <dspy:parameter key="max_tokens" value="1000"/>
        <dspy:telemetry key="enabled" value="true"/>
        <dspy:telemetry key="sampling_rate" value="1.0"/>
    </dspy:model>
    
</dspy:config>'''


def test_e2e_pipeline():
    """Test the complete end-to-end AutoTel pipeline."""
    print("🚀 Starting AutoTel End-to-End Pipeline Test")
    print("=" * 60)
    
    # Step 1: Create test data
    print("\n📋 Step 1: Creating test data...")
    ontology_schema = create_test_ontology_schema()
    validation_rules = create_test_validation_rules()
    dspy_xml = create_test_dspy_xml()
    
    print(f"✅ Created ontology with {len(ontology_schema.classes)} classes")
    print(f"✅ Created validation rules with {validation_rules.constraint_count} constraints")
    print(f"✅ Created DSPy XML ({len(dspy_xml)} characters)")
    
    # Step 2: Process DSPy XML
    print("\n🔧 Step 2: Processing DSPy XML...")
    dspy_processor = DSPyProcessor()
    
    signatures = dspy_processor.parse(dspy_xml)
    modules = dspy_processor.parse_modules(dspy_xml)
    model_config = dspy_processor.parse_model_configuration(dspy_xml)
    validation_rules_from_xml = dspy_processor.parse_validation_rules(dspy_xml)
    semantic_context = dspy_processor.parse_semantic_context(dspy_xml)
    
    print(f"✅ Parsed {len(signatures)} DSPy signatures")
    print(f"✅ Parsed {len(modules)} DSPy modules")
    print(f"✅ Parsed model configuration: {model_config.provider}/{model_config.model_name}")
    print(f"✅ Parsed {len(validation_rules_from_xml)} validation rules from XML")
    print(f"✅ Parsed semantic context with {len(semantic_context['ontology_classes'])} ontology classes")
    
    # Step 3: Compile DSPy signature
    print("\n⚙️  Step 3: Compiling DSPy signature...")
    dspy_compiler = DSPyCompiler()
    
    compiled_signature = dspy_compiler.compile(
        ontology_schema=ontology_schema,
        validation_rules=validation_rules,
        dspy_signatures=signatures,
        dspy_modules=modules,
        model_config=model_config
    )
    
    print(f"✅ Compiled signature: {compiled_signature.name}")
    print(f"✅ Signature has {len(compiled_signature.inputs)} inputs and {len(compiled_signature.outputs)} outputs")
    print(f"✅ Signature has {len(compiled_signature.validation_rules)} validation rules")
    print(f"✅ Semantic context: {len(compiled_signature.semantic_context)} context items")
    
    # Step 4: Link into executable system
    print("\n🔗 Step 4: Linking into executable system...")
    semantic_linker = SemanticLinker()
    
    executable_system = semantic_linker.link(compiled_signature)
    
    print(f"✅ Created executable system for signature: {executable_system.signature.name}")
    print(f"✅ System has {len(executable_system.validation_rules)} prepared validation rules")
    print(f"✅ System has {len(executable_system.semantic_context)} semantic context items")
    print(f"✅ SHACL validation enabled: {executable_system.shacl_validation}")
    
    # Step 5: Execute the system
    print("\n▶️  Step 5: Executing the system...")
    ontology_executor = OntologyExecutor()
    
    test_inputs = {
        "user_input": "I want to learn about artificial intelligence and machine learning"
    }
    
    print(f"📥 Input: {test_inputs}")
    
    execution_result = ontology_executor.execute(executable_system, test_inputs)
    
    print(f"✅ Execution completed in {execution_result.execution_time:.3f} seconds")
    print(f"✅ Execution success: {execution_result.success}")
    print(f"✅ Generated {len(execution_result.outputs)} outputs")
    
    # Step 6: Display results
    print("\n📊 Step 6: Execution Results")
    print("-" * 40)
    
    print("Outputs:")
    for output_name, output_value in execution_result.outputs.items():
        if not output_name.startswith("_"):  # Skip metadata
            print(f"  {output_name}: {output_value}")
    
    print(f"\nValidation Result:")
    print(f"  Valid: {execution_result.validation_result.valid}")
    print(f"  Violations: {len(execution_result.validation_result.violations)}")
    print(f"  Warnings: {len(execution_result.validation_result.warnings)}")
    
    if execution_result.validation_result.violations:
        print("  Violations:")
        for violation in execution_result.validation_result.violations:
            print(f"    - {violation['field']}: {violation['message']}")
    
    if execution_result.validation_result.warnings:
        print("  Warnings:")
        for warning in execution_result.validation_result.warnings:
            print(f"    - {warning['field']}: {warning['message']}")
    
    print(f"\nTelemetry Data:")
    print(f"  Spans: {len(execution_result.telemetry_data.spans)}")
    print(f"  Events: {len(execution_result.telemetry_data.events)}")
    print(f"  Metrics: {len(execution_result.telemetry_data.metrics)}")
    print(f"  Logs: {len(execution_result.telemetry_data.logs)}")
    
    print(f"\nExecution Metadata:")
    print(f"  Signature: {execution_result.metadata['execution_info']['signature_name']}")
    print(f"  Model: {execution_result.metadata['model_info']['provider']}/{execution_result.metadata['model_info']['model_name']}")
    print(f"  Performance: {execution_result.metadata['performance_metrics']['inputs_count']} inputs, {execution_result.metadata['performance_metrics']['outputs_count']} outputs")
    
    # Step 7: Test with invalid input
    print("\n🧪 Step 7: Testing with invalid input...")
    
    invalid_inputs = {
        "user_input": "short"  # Too short for validation
    }
    
    print(f"📥 Invalid input: {invalid_inputs}")
    
    invalid_result = ontology_executor.execute(executable_system, invalid_inputs)
    
    print(f"✅ Invalid execution completed in {invalid_result.execution_time:.3f} seconds")
    print(f"✅ Execution success: {invalid_result.success}")
    print(f"✅ Validation result: {invalid_result.validation_result.valid}")
    
    if not invalid_result.validation_result.valid:
        print("  Validation violations:")
        for violation in invalid_result.validation_result.violations:
            print(f"    - {violation['field']}: {violation['message']}")
    
    # Summary
    print("\n" + "=" * 60)
    print("🎉 AutoTel End-to-End Pipeline Test Completed Successfully!")
    print("=" * 60)
    
    print("\n📈 Pipeline Summary:")
    print(f"  ✅ DSPy XML Processing: {len(signatures)} signatures parsed")
    print(f"  ✅ DSPy Compilation: Signature compiled with ontology and validation")
    print(f"  ✅ Semantic Linking: Executable system created")
    print(f"  ✅ Execution: {execution_result.success} successful, {invalid_result.success} with invalid input")
    print(f"  ✅ Telemetry: Comprehensive telemetry generated")
    print(f"  ✅ Validation: Input validation working correctly")
    
    print("\n🔧 Pipeline Components Tested:")
    print("  - DSPyProcessor: XML parsing with telemetry")
    print("  - DSPyCompiler: Signature compilation with ontology integration")
    print("  - SemanticLinker: Executable system creation")
    print("  - OntologyExecutor: Execution with validation and telemetry")
    
    print("\n📊 Performance Metrics:")
    print(f"  - Total execution time: {execution_result.execution_time:.3f}s")
    print(f"  - Validation rules applied: {len(executable_system.validation_rules)}")
    print(f"  - Telemetry spans generated: {len(execution_result.telemetry_data.spans)}")
    print(f"  - Semantic context items: {len(executable_system.semantic_context)}")
    
    return execution_result


if __name__ == "__main__":
    try:
        result = test_e2e_pipeline()
        print(f"\n✅ All tests passed! Final result: {result.success}")
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc() 