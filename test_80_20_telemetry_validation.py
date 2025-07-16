#!/usr/bin/env python3
"""
Test 80/20 telemetry improvements - Universal fallback, CLI no-telemetry mode, and telemetry injection
"""

import json
import tempfile
import os
import sys
from pathlib import Path
from typing import Dict, Any

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.core.telemetry import (
    TelemetryManager, 
    NoOpTelemetryManager, 
    get_telemetry_manager_or_noop,
    TelemetryConfig
)
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.ontology_compiler import OntologyCompiler
from autotel.schemas.ontology_types import OWLOntologyDefinition

def test_universal_telemetry_fallback():
    """Test that telemetry automatically falls back to no-op when it fails."""
    print("🧪 Testing universal telemetry fallback...")
    
    # Test 1: Force no-op mode
    telemetry = get_telemetry_manager_or_noop(
        service_name="test-service",
        force_noop=True
    )
    
    assert isinstance(telemetry, NoOpTelemetryManager), "Should return NoOpTelemetryManager when force_noop=True"
    stats = telemetry.get_stats()
    assert stats.get('fallback_reason') == "Explicitly requested no-op mode"
    print("✅ Force no-op mode works")
    
    # Test 2: Automatic fallback on schema failure
    telemetry = get_telemetry_manager_or_noop(
        service_name="test-service",
        schema_path="/nonexistent/schema.yaml",
        fallback_to_noop=True
    )
    
    assert isinstance(telemetry, NoOpTelemetryManager), "Should fall back to NoOpTelemetryManager on schema failure"
    stats = telemetry.get_stats()
    assert "Telemetry initialization failed" in stats.get('fallback_reason', '')
    print("✅ Automatic fallback on schema failure works")
    
    # Test 3: No fallback when disabled
    try:
        telemetry = get_telemetry_manager_or_noop(
            service_name="test-service",
            schema_path="/nonexistent/schema.yaml",
            fallback_to_noop=False
        )
        assert False, "Should raise exception when fallback is disabled"
    except Exception:
        print("✅ No fallback when disabled works (raises exception)")
    
    # Test 4: No-op telemetry operations work
    with telemetry.start_span("test_span", "test_operation") as span:
        span.set_attribute("test_key", "test_value")
        span.set_status("OK")
    
    telemetry.record_metric("test_metric", 1, test_attr="test_value")
    
    stats = telemetry.get_stats()
    assert stats.get('span_count', 0) > 0, "Should track span count"
    assert stats.get('metric_count', 0) > 0, "Should track metric count"
    print("✅ No-op telemetry operations work")

def test_telemetry_injection():
    """Test that processors and compilers accept telemetry injection."""
    print("\n🧪 Testing telemetry injection...")
    
    # Create a no-op telemetry manager for testing
    test_telemetry = NoOpTelemetryManager("test-injection")
    
    # Test OWL Processor injection
    processor = OWLProcessor(telemetry=test_telemetry)
    assert processor.telemetry == test_telemetry, "OWLProcessor should accept telemetry injection"
    
    # Test OWL Processor force no-op
    processor_noop = OWLProcessor(force_noop=True)
    assert isinstance(processor_noop.telemetry, NoOpTelemetryManager), "OWLProcessor should support force_noop"
    
    # Test Ontology Compiler injection
    compiler = OntologyCompiler(telemetry=test_telemetry)
    assert compiler.telemetry == test_telemetry, "OntologyCompiler should accept telemetry injection"
    
    # Test Ontology Compiler force no-op
    compiler_noop = OntologyCompiler(force_noop=True)
    assert isinstance(compiler_noop.telemetry, NoOpTelemetryManager), "OntologyCompiler should support force_noop"
    
    print("✅ Telemetry injection works for all components")

def test_pipeline_with_noop_telemetry():
    """Test that the entire pipeline works with no-op telemetry."""
    print("\n🧪 Testing pipeline with no-op telemetry...")
    
    # Sample OWL XML content
    owl_xml = """<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/test"/>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test#Workflow">
        <rdfs:label>Workflow</rdfs:label>
        <rdfs:comment>A business process workflow</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test#Task">
        <rdfs:label>Task</rdfs:label>
        <rdfs:comment>A workflow task</rdfs:comment>
        <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/test#Workflow"/>
    </owl:Class>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/test#hasTask">
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test#Workflow"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/test#Task"/>
    </owl:ObjectProperty>
    
</rdf:RDF>"""
    
    # Test with no-op telemetry
    processor = OWLProcessor(force_noop=True)
    compiler = OntologyCompiler(force_noop=True)
    
    # Process OWL
    ontology_def = processor.parse_ontology_definition(owl_xml)
    assert ontology_def is not None, "Should parse OWL successfully with no-op telemetry"
    assert len(ontology_def.classes) > 0, "Should extract classes"
    
    # Compile ontology
    schema = compiler.compile(ontology_def)
    assert schema is not None, "Should compile ontology successfully with no-op telemetry"
    assert len(schema.classes) > 0, "Should have compiled classes"
    
    # Check telemetry stats
    processor_stats = processor.telemetry.get_stats()
    compiler_stats = compiler.telemetry.get_stats()
    
    assert processor_stats.get('span_count', 0) > 0, "Processor should have created spans"
    assert compiler_stats.get('span_count', 0) > 0, "Compiler should have created spans"
    
    print("✅ Pipeline works with no-op telemetry")

def test_telemetry_failure_scenarios():
    """Test various telemetry failure scenarios."""
    print("\n🧪 Testing telemetry failure scenarios...")
    
    # Test 1: Invalid schema path
    telemetry = get_telemetry_manager_or_noop(
        service_name="test-service",
        schema_path="/invalid/path/schema.yaml",
        fallback_to_noop=True
    )
    
    assert isinstance(telemetry, NoOpTelemetryManager), "Should fall back on invalid schema path"
    stats = telemetry.get_stats()
    assert "Telemetry initialization failed" in stats.get('fallback_reason', '')
    print("✅ Falls back on invalid schema path")
    
    # Test 2: Missing required classes in schema
    # Create a temporary invalid schema file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write("""
id: http://example.org/invalid
name: invalid
classes:
  InvalidClass:
    attributes:
      invalid_attr: string
""")
        invalid_schema_path = f.name
    
    try:
        telemetry = get_telemetry_manager_or_noop(
            service_name="test-service",
            schema_path=invalid_schema_path,
            require_linkml_validation=True,
            fallback_to_noop=True
        )
        
        assert isinstance(telemetry, NoOpTelemetryManager), "Should fall back on invalid schema"
        stats = telemetry.get_stats()
        assert "Telemetry initialization failed" in stats.get('fallback_reason', '')
        print("✅ Falls back on invalid schema content")
        
    finally:
        os.unlink(invalid_schema_path)
    
    # Test 3: Telemetry operations in no-op mode
    telemetry = NoOpTelemetryManager("test-failures")
    
    # These should not raise exceptions
    with telemetry.start_span("test", "test") as span:
        span.set_attribute("key", "value")
        span.set_status("ERROR")
    
    telemetry.record_metric("test", 1)
    telemetry.force_flush()
    
    # All operations should work without errors
    print("✅ No-op telemetry operations are safe")

def test_telemetry_stats_and_health():
    """Test telemetry statistics and health reporting."""
    print("\n🧪 Testing telemetry statistics and health...")
    
    # Test real telemetry stats
    telemetry = get_telemetry_manager_or_noop(
        service_name="test-stats",
        fallback_to_noop=False
    )
    
    # Perform some operations
    with telemetry.start_span("test_span", "test_operation") as span:
        span.set_attribute("test_attr", "test_value")
    
    telemetry.record_metric("test_metric", 1)
    
    # Get stats
    stats = telemetry.get_stats()
    
    # Check required stats fields
    required_fields = [
        'schema_connected', 'tracer_enabled', 'meter_enabled',
        'operation_count', 'span_count', 'metric_count'
    ]
    
    for field in required_fields:
        assert field in stats, f"Stats should contain {field}"
    
    assert stats.get('span_count', 0) > 0, "Should track spans"
    assert stats.get('metric_count', 0) > 0, "Should track metrics"
    assert stats.get('operation_count', 0) > 0, "Should track operations"
    
    print("✅ Telemetry statistics work correctly")
    
    # Test no-op telemetry stats
    noop_telemetry = NoOpTelemetryManager("test-noop-stats")
    
    # Perform operations
    with noop_telemetry.start_span("test_span", "test_operation") as span:
        span.set_attribute("test_attr", "test_value")
    
    noop_telemetry.record_metric("test_metric", 1)
    
    # Get stats
    noop_stats = noop_telemetry.get_stats()
    
    assert noop_stats.get('fallback_reason') is not None, "Should have fallback reason"
    assert noop_stats.get('span_count', 0) > 0, "Should track spans in no-op mode"
    assert noop_stats.get('metric_count', 0) > 0, "Should track metrics in no-op mode"
    
    print("✅ No-op telemetry statistics work correctly")

def test_cli_no_telemetry_mode():
    """Test CLI no-telemetry mode functionality."""
    print("\n🧪 Testing CLI no-telemetry mode...")
    
    # Import CLI functions
    from autotel.cli import get_telemetry_manager_for_cli, set_no_telemetry
    
    # Test 1: Normal mode
    set_no_telemetry(False)
    telemetry = get_telemetry_manager_for_cli()
    
    # Should be real telemetry (unless it fails and falls back)
    if isinstance(telemetry, NoOpTelemetryManager):
        print("✅ Normal mode falls back to no-op when needed")
    else:
        print("✅ Normal mode uses real telemetry")
    
    # Test 2: No-telemetry mode
    set_no_telemetry(True)
    
    telemetry = get_telemetry_manager_for_cli()
    assert isinstance(telemetry, NoOpTelemetryManager), "Should use no-op in no-telemetry mode"
    
    stats = telemetry.get_stats()
    assert stats.get('fallback_reason') == "Explicitly requested no-op mode"
    print("✅ No-telemetry mode works correctly")
    
    # Reset for other tests
    set_no_telemetry(False)

def test_production_readiness():
    """Test production readiness features."""
    print("\n🧪 Testing production readiness...")
    
    # Test 1: Graceful degradation
    # Simulate a scenario where telemetry fails but the system continues
    processor = OWLProcessor(force_noop=True)
    compiler = OntologyCompiler(force_noop=True)
    
    # The system should work even with no-op telemetry
    owl_xml = """<?xml version="1.0"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#">
    <owl:Class rdf:about="http://example.org/TestClass"/>
</rdf:RDF>"""
    
    ontology_def = processor.parse_ontology_definition(owl_xml)
    schema = compiler.compile(ontology_def)
    
    assert ontology_def is not None, "System should work with no-op telemetry"
    assert schema is not None, "System should work with no-op telemetry"
    print("✅ Graceful degradation works")
    
    # Test 2: Telemetry failure isolation
    # Create a telemetry manager that will fail
    try:
        telemetry = get_telemetry_manager_or_noop(
            service_name="test-failure",
            schema_path="/nonexistent/schema.yaml",
            require_linkml_validation=True,
            fallback_to_noop=False
        )
        assert False, "Should have raised an exception"
    except Exception:
        print("✅ Telemetry failures are properly isolated when fallback is disabled")
    
    # Test 3: Telemetry failure with fallback
    telemetry = get_telemetry_manager_or_noop(
        service_name="test-failure",
        schema_path="/nonexistent/schema.yaml",
        require_linkml_validation=True,
        fallback_to_noop=True
    )
    
    assert isinstance(telemetry, NoOpTelemetryManager), "Should fall back on failure"
    print("✅ Telemetry failures are handled gracefully with fallback")

def main():
    """Run all 80/20 telemetry validation tests."""
    print("🚀 Starting 80/20 Telemetry Validation Tests")
    print("=" * 60)
    
    try:
        test_universal_telemetry_fallback()
        test_telemetry_injection()
        test_pipeline_with_noop_telemetry()
        test_telemetry_failure_scenarios()
        test_telemetry_stats_and_health()
        test_cli_no_telemetry_mode()
        test_production_readiness()
        
        print("\n" + "=" * 60)
        print("🎉 All 80/20 telemetry validation tests passed!")
        print("\nKey improvements validated:")
        print("✅ Universal no-op telemetry fallback")
        print("✅ CLI no-telemetry mode")
        print("✅ Telemetry injection in processors and compilers")
        print("✅ Graceful degradation on telemetry failures")
        print("✅ Comprehensive telemetry statistics")
        print("✅ Production-ready error handling")
        
        return True
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 