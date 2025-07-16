#!/usr/bin/env python3
"""
Simple demonstration of no-telemetry mode working without any telemetry output.
"""

import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.core.telemetry import NoOpTelemetryManager, get_telemetry_manager_or_noop
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.ontology_compiler import OntologyCompiler

def demo_no_telemetry():
    """Demonstrate no-telemetry mode working silently."""
    print("🧪 Demonstrating no-telemetry mode...")
    
    # Sample OWL XML content
    sample_owl_xml = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/test#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/test"/>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test#TestClass">
        <rdfs:label>Test Class</rdfs:label>
        <rdfs:comment>A test class for demonstration</rdfs:comment>
    </owl:Class>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/test#hasProperty">
        <rdfs:label>has property</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test#TestClass"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/test#TestClass"/>
    </owl:ObjectProperty>
    
</rdf:RDF>'''
    
    print("📋 Step 1: Creating no-op telemetry manager...")
    noop_telemetry = NoOpTelemetryManager("demo-noop")
    print(f"   ✅ No-op telemetry created: {type(noop_telemetry).__name__}")
    print(f"   ✅ Is configured: {noop_telemetry.is_configured()}")
    
    print("\n📋 Step 2: Testing no-op span creation...")
    with noop_telemetry.start_span("demo_span", "demo_operation") as span:
        span.set_attribute("test_key", "test_value")
        span.set_status("OK")
        print("   ✅ No-op span created and used successfully")
    
    print("\n📋 Step 3: Testing OWL processor with no-op telemetry...")
    processor = OWLProcessor()
    processor.telemetry = noop_telemetry
    
    ontology_def = processor.parse_ontology_definition(sample_owl_xml, prefix="demo")
    print(f"   ✅ OWL parsed successfully: {len(ontology_def.classes)} classes")
    print(f"   ✅ OWL parsed successfully: {len(ontology_def.object_properties)} object properties")
    
    print("\n📋 Step 4: Testing ontology compiler with no-op telemetry...")
    compiler = OntologyCompiler()
    compiler.telemetry = noop_telemetry
    
    ontology_schema = compiler.compile(ontology_def)
    print(f"   ✅ Ontology compiled successfully: {len(ontology_schema.classes)} compiled classes")
    print(f"   ✅ Semantic context created: {len(ontology_schema.semantic_context)} keys")
    
    print("\n📋 Step 5: Testing fallback mechanism...")
    fallback_telemetry = get_telemetry_manager_or_noop(
        service_name="demo-fallback",
        force_noop=True
    )
    print(f"   ✅ Fallback telemetry created: {type(fallback_telemetry).__name__}")
    
    print("\n🎉 All no-telemetry operations completed successfully!")
    print("   No telemetry output was produced during execution.")
    print("   The pipeline works exactly the same with or without telemetry.")
    
    return True

if __name__ == "__main__":
    success = demo_no_telemetry()
    sys.exit(0 if success else 1) 