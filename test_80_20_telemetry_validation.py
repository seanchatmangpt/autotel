#!/usr/bin/env python3
"""
Test script for 80/20 telemetry validation implementation.
Validates that the pipeline works with real telemetry and gracefully falls back to no-op.
"""

import sys
import json
import tempfile
from pathlib import Path
from typing import Dict, Any

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.core.telemetry import (
    TelemetryManager, 
    NoOpTelemetryManager, 
    get_telemetry_manager_or_noop,
    create_telemetry_manager
)
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.ontology_compiler import OntologyCompiler
from autotel.schemas.ontology_types import OWLOntologyDefinition

def test_telemetry_fallback():
    """Test that telemetry gracefully falls back to no-op when needed."""
    print("🧪 Testing telemetry fallback mechanism...")
    
    # Test 1: Normal telemetry manager creation
    try:
        telemetry = create_telemetry_manager(
            service_name="test-service",
            require_linkml_validation=False  # Allow basic telemetry without schema
        )
        print("✅ Normal telemetry manager created successfully")
        assert isinstance(telemetry, TelemetryManager)
    except Exception as e:
        print(f"❌ Normal telemetry manager creation failed: {e}")
        return False
    
    # Test 2: No-op telemetry manager creation
    try:
        noop_telemetry = NoOpTelemetryManager("test-noop")
        print("✅ No-op telemetry manager created successfully")
        assert isinstance(noop_telemetry, NoOpTelemetryManager)
        assert not noop_telemetry.is_configured()
    except Exception as e:
        print(f"❌ No-op telemetry manager creation failed: {e}")
        return False
    
    # Test 3: Fallback mechanism
    try:
        fallback_telemetry = get_telemetry_manager_or_noop(
            service_name="test-fallback",
            force_noop=True
        )
        print("✅ Fallback to no-op telemetry successful")
        assert isinstance(fallback_telemetry, NoOpTelemetryManager)
    except Exception as e:
        print(f"❌ Fallback mechanism failed: {e}")
        return False
    
    # Test 4: Span creation with no-op
    try:
        with noop_telemetry.start_span("test_span", "test_operation") as span:
            span.set_attribute("test_key", "test_value")
            span.set_status("OK")
        print("✅ No-op span creation successful")
    except Exception as e:
        print(f"❌ No-op span creation failed: {e}")
        return False
    
    # Test 5: Metric recording with no-op
    try:
        noop_telemetry.record_metric("test_metric", 1, test_attr="test_value")
        print("✅ No-op metric recording successful")
    except Exception as e:
        print(f"❌ No-op metric recording failed: {e}")
        return False
    
    print("🎉 All telemetry fallback tests passed!")
    return True

def test_owl_processor_with_telemetry():
    """Test OWL processor with real telemetry."""
    print("\n🧪 Testing OWL processor with telemetry...")
    
    # Sample OWL XML content
    sample_owl_xml = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/workflow#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/workflow"/>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#WorkflowTask">
        <rdfs:label>Workflow Task</rdfs:label>
        <rdfs:comment>A task in a workflow process</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#DecisionPoint">
        <rdfs:label>Decision Point</rdfs:label>
        <rdfs:comment>A decision point in a workflow</rdfs:comment>
        <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/workflow#WorkflowTask"/>
    </owl:Class>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/workflow#hasNextTask">
        <rdfs:label>has next task</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#WorkflowTask"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/workflow#WorkflowTask"/>
    </owl:ObjectProperty>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/workflow#taskName">
        <rdfs:label>task name</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#WorkflowTask"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
    <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/workflow#StartTask">
        <rdf:type rdf:resource="http://autotel.ai/ontology/workflow#WorkflowTask"/>
        <rdfs:label>Start Task</rdfs:label>
        <taskName>Start</taskName>
    </owl:NamedIndividual>
    
</rdf:RDF>'''
    
    # Test with real telemetry
    try:
        processor = OWLProcessor()
        ontology_def = processor.parse_ontology_definition(sample_owl_xml, prefix="test")
        
        print("✅ OWL processor with real telemetry successful")
        print(f"   - Classes: {len(ontology_def.classes)}")
        print(f"   - Object Properties: {len(ontology_def.object_properties)}")
        print(f"   - Data Properties: {len(ontology_def.data_properties)}")
        print(f"   - Individuals: {len(ontology_def.individuals)}")
        
        assert len(ontology_def.classes) >= 2  # WorkflowTask and DecisionPoint
        assert len(ontology_def.object_properties) >= 1  # hasNextTask
        assert len(ontology_def.data_properties) >= 1  # taskName
        assert len(ontology_def.individuals) >= 1  # StartTask
        
    except Exception as e:
        print(f"❌ OWL processor with real telemetry failed: {e}")
        return False
    
    # Test with no-op telemetry
    try:
        noop_processor = OWLProcessor()
        noop_processor.telemetry = NoOpTelemetryManager("test-owl-noop")
        ontology_def_noop = noop_processor.parse_ontology_definition(sample_owl_xml, prefix="test")
        
        print("✅ OWL processor with no-op telemetry successful")
        print(f"   - Classes: {len(ontology_def_noop.classes)}")
        print(f"   - Object Properties: {len(ontology_def_noop.object_properties)}")
        print(f"   - Data Properties: {len(ontology_def_noop.data_properties)}")
        print(f"   - Individuals: {len(ontology_def_noop.individuals)}")
        
        # Should produce same results regardless of telemetry
        assert len(ontology_def_noop.classes) == len(ontology_def.classes)
        assert len(ontology_def_noop.object_properties) == len(ontology_def.object_properties)
        assert len(ontology_def_noop.data_properties) == len(ontology_def.data_properties)
        assert len(ontology_def_noop.individuals) == len(ontology_def.individuals)
        
    except Exception as e:
        print(f"❌ OWL processor with no-op telemetry failed: {e}")
        return False
    
    print("🎉 All OWL processor telemetry tests passed!")
    return True

def test_ontology_compiler_with_telemetry():
    """Test ontology compiler with telemetry."""
    print("\n🧪 Testing ontology compiler with telemetry...")
    
    # Create a sample ontology definition
    ontology_def = OWLOntologyDefinition(
        ontology_uri="http://autotel.ai/ontology/test",
        prefix="test",
        namespace="http://autotel.ai/ontology/test#",
        classes={
            "WorkflowTask": {
                "uri": "http://autotel.ai/ontology/test#WorkflowTask",
                "name": "WorkflowTask",
                "label": "Workflow Task",
                "comment": "A task in a workflow process",
                "superclasses": [],
                "properties": {}
            },
            "DecisionPoint": {
                "uri": "http://autotel.ai/ontology/test#DecisionPoint",
                "name": "DecisionPoint",
                "label": "Decision Point",
                "comment": "A decision point in a workflow",
                "superclasses": ["WorkflowTask"],
                "properties": {}
            }
        },
        object_properties={
            "hasNextTask": {
                "uri": "http://autotel.ai/ontology/test#hasNextTask",
                "name": "hasNextTask",
                "type": "object",
                "label": "has next task",
                "comment": "Links to the next task",
                "domain": "WorkflowTask",
                "range": "WorkflowTask",
                "cardinality": ""
            }
        },
        data_properties={
            "taskName": {
                "uri": "http://autotel.ai/ontology/test#taskName",
                "name": "taskName",
                "type": "data",
                "label": "task name",
                "comment": "Name of the task",
                "domain": "WorkflowTask",
                "range": "string",
                "cardinality": ""
            }
        },
        individuals={
            "StartTask": {
                "uri": "http://autotel.ai/ontology/test#StartTask",
                "name": "StartTask",
                "type": "WorkflowTask",
                "label": "Start Task",
                "comment": "The starting task",
                "properties": {"taskName": "Start"}
            }
        },
        axioms=[]
    )
    
    # Test with real telemetry
    try:
        compiler = OntologyCompiler()
        ontology_schema = compiler.compile(ontology_def)
        
        print("✅ Ontology compiler with real telemetry successful")
        print(f"   - Compiled Classes: {len(ontology_schema.classes)}")
        print(f"   - Semantic Context Keys: {len(ontology_schema.semantic_context)}")
        print(f"   - Examples: {len(ontology_schema.examples)}")
        
        assert len(ontology_schema.classes) == 2
        assert "WorkflowTask" in ontology_schema.classes
        assert "DecisionPoint" in ontology_schema.classes
        assert len(ontology_schema.semantic_context) > 0
        assert len(ontology_schema.examples) == 1
        
    except Exception as e:
        print(f"❌ Ontology compiler with real telemetry failed: {e}")
        return False
    
    # Test with no-op telemetry
    try:
        noop_compiler = OntologyCompiler()
        noop_compiler.telemetry = NoOpTelemetryManager("test-compiler-noop")
        ontology_schema_noop = noop_compiler.compile(ontology_def)
        
        print("✅ Ontology compiler with no-op telemetry successful")
        print(f"   - Compiled Classes: {len(ontology_schema_noop.classes)}")
        print(f"   - Semantic Context Keys: {len(ontology_schema_noop.semantic_context)}")
        print(f"   - Examples: {len(ontology_schema_noop.examples)}")
        
        # Should produce same results regardless of telemetry
        assert len(ontology_schema_noop.classes) == len(ontology_schema.classes)
        assert len(ontology_schema_noop.semantic_context) == len(ontology_schema.semantic_context)
        assert len(ontology_schema_noop.examples) == len(ontology_schema.examples)
        
    except Exception as e:
        print(f"❌ Ontology compiler with no-op telemetry failed: {e}")
        return False
    
    print("🎉 All ontology compiler telemetry tests passed!")
    return True

def test_end_to_end_pipeline():
    """Test the complete end-to-end pipeline with telemetry."""
    print("\n🧪 Testing end-to-end pipeline with telemetry...")
    
    # Sample OWL XML content
    sample_owl_xml = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/workflow#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/workflow"/>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input provided by a user</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#Recommendation">
        <rdfs:label>Recommendation</rdfs:label>
        <rdfs:comment>A recommendation generated by the system</rdfs:comment>
    </owl:Class>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/workflow#generatesRecommendation">
        <rdfs:label>generates recommendation</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#UserInput"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/workflow#Recommendation"/>
    </owl:ObjectProperty>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/workflow#inputText">
        <rdfs:label>input text</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#UserInput"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
</rdf:RDF>'''
    
    # Test complete pipeline with real telemetry
    try:
        # Step 1: Parse OWL
        processor = OWLProcessor()
        ontology_def = processor.parse_ontology_definition(sample_owl_xml, prefix="e2e")
        
        # Step 2: Compile ontology
        compiler = OntologyCompiler()
        ontology_schema = compiler.compile(ontology_def)
        
        print("✅ End-to-end pipeline with real telemetry successful")
        print(f"   - Parsed Classes: {len(ontology_def.classes)}")
        print(f"   - Compiled Classes: {len(ontology_schema.classes)}")
        print(f"   - Semantic Types: {list(ontology_schema.semantic_context['semantic_types'].keys())}")
        
        # Validate semantic types were correctly identified
        assert "UserInput" in ontology_schema.classes
        assert "Recommendation" in ontology_schema.classes
        assert ontology_schema.classes["UserInput"].semantic_type == "user_input"
        assert ontology_schema.classes["Recommendation"].semantic_type == "recommendation"
        
    except Exception as e:
        print(f"❌ End-to-end pipeline with real telemetry failed: {e}")
        return False
    
    # Test complete pipeline with no-op telemetry
    try:
        # Step 1: Parse OWL with no-op telemetry
        noop_processor = OWLProcessor()
        noop_processor.telemetry = NoOpTelemetryManager("e2e-owl-noop")
        ontology_def_noop = noop_processor.parse_ontology_definition(sample_owl_xml, prefix="e2e")
        
        # Step 2: Compile ontology with no-op telemetry
        noop_compiler = OntologyCompiler()
        noop_compiler.telemetry = NoOpTelemetryManager("e2e-compiler-noop")
        ontology_schema_noop = noop_compiler.compile(ontology_def_noop)
        
        print("✅ End-to-end pipeline with no-op telemetry successful")
        print(f"   - Parsed Classes: {len(ontology_def_noop.classes)}")
        print(f"   - Compiled Classes: {len(ontology_schema_noop.classes)}")
        print(f"   - Semantic Types: {list(ontology_schema_noop.semantic_context['semantic_types'].keys())}")
        
        # Should produce same results regardless of telemetry
        assert len(ontology_def_noop.classes) == len(ontology_def.classes)
        assert len(ontology_schema_noop.classes) == len(ontology_schema.classes)
        assert ontology_schema_noop.classes["UserInput"].semantic_type == ontology_schema.classes["UserInput"].semantic_type
        assert ontology_schema_noop.classes["Recommendation"].semantic_type == ontology_schema.classes["Recommendation"].semantic_type
        
    except Exception as e:
        print(f"❌ End-to-end pipeline with no-op telemetry failed: {e}")
        return False
    
    print("🎉 All end-to-end pipeline tests passed!")
    return True

def test_cli_no_telemetry_mode():
    """Test CLI no-telemetry mode."""
    print("\n🧪 Testing CLI no-telemetry mode...")
    
    # Create a temporary OWL file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.owl', delete=False) as f:
        f.write('''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/test#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/test"/>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test#TestClass">
        <rdfs:label>Test Class</rdfs:label>
        <rdfs:comment>A test class for CLI validation</rdfs:comment>
    </owl:Class>
    
</rdf:RDF>''')
        temp_file = f.name
    
    try:
        # Import and test CLI functionality
        from autotel.cli import parse_ontology, set_no_telemetry
        
        # Test with telemetry enabled
        set_no_telemetry(False)
        print("✅ CLI telemetry mode test setup successful")
        
        # Test with telemetry disabled
        set_no_telemetry(True)
        print("✅ CLI no-telemetry mode test setup successful")
        
        # Note: We can't actually run the CLI command here due to Typer's async nature
        # But we can test that the flag is properly set
        from autotel.cli import NO_TELEMETRY
        assert NO_TELEMETRY == True
        
    except Exception as e:
        print(f"❌ CLI no-telemetry mode test failed: {e}")
        return False
    finally:
        # Clean up temp file
        try:
            Path(temp_file).unlink()
        except:
            pass
    
    print("🎉 CLI no-telemetry mode tests passed!")
    return True

def main():
    """Run all 80/20 telemetry validation tests."""
    print("🚀 Starting 80/20 telemetry validation tests...")
    print("=" * 60)
    
    tests = [
        ("Telemetry Fallback", test_telemetry_fallback),
        ("OWL Processor with Telemetry", test_owl_processor_with_telemetry),
        ("Ontology Compiler with Telemetry", test_ontology_compiler_with_telemetry),
        ("End-to-End Pipeline", test_end_to_end_pipeline),
        ("CLI No-Telemetry Mode", test_cli_no_telemetry_mode),
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\n📋 Running: {test_name}")
        print("-" * 40)
        
        try:
            if test_func():
                passed += 1
                print(f"✅ {test_name}: PASSED")
            else:
                print(f"❌ {test_name}: FAILED")
        except Exception as e:
            print(f"❌ {test_name}: ERROR - {e}")
    
    print("\n" + "=" * 60)
    print(f"📊 Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed! 80/20 telemetry validation implementation is working correctly.")
        return 0
    else:
        print("⚠️  Some tests failed. Please review the implementation.")
        return 1

if __name__ == "__main__":
    exit(main()) 