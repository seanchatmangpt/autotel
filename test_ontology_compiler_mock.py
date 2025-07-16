#!/usr/bin/env python3
"""
Mock OntologyCompiler test with no-op telemetry.
"""

import sys
import os
import json
from contextlib import contextmanager
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.ontology_compiler import OntologyCompiler
from pathlib import Path

class MockTelemetryManager:
    """Mock telemetry manager that provides no-op spans."""
    
    def __init__(self):
        self.service_name = "mock-telemetry"
    
    @contextmanager
    def start_span(self, name: str, operation_type: str, **kwargs):
        """No-op span context manager."""
        class MockSpan:
            def set_attribute(self, key, value):
                pass
            def set_status(self, status):
                pass
        
        span = MockSpan()
        try:
            yield span
        finally:
            pass
    
    def record_metric(self, metric_name: str, value, **attributes):
        """No-op metric recording."""
        pass

def test_ontology_compiler_mock():
    """Test OntologyCompiler with mock telemetry."""
    
    print("🧪 Testing OntologyCompiler (Mock Telemetry)")
    print("=" * 50)
    
    # Load the sample OWL file
    owl_file = Path("examples/workflow_ontology.owl")
    if not owl_file.exists():
        print(f"❌ Sample OWL file not found: {owl_file}")
        return False
    
    print(f"📁 Loading OWL file: {owl_file}")
    
    try:
        with open(owl_file, 'r') as f:
            xml_content = f.read()
        
        print(f"📄 Loaded {len(xml_content)} characters of OWL content")
        
        # Parse OWL with processor (mock telemetry)
        print("\n🔍 Parsing OWL with OWLProcessor...")
        
        # Create processor with mock telemetry
        processor = OWLProcessor()
        processor.telemetry = MockTelemetryManager()
        
        ontology_def = processor.parse_ontology_definition(xml_content, prefix='mock_test')
        
        print(f"✅ OWL parsed successfully:")
        print(f"  - Classes: {len(ontology_def.classes)}")
        print(f"  - Object Properties: {len(ontology_def.object_properties)}")
        print(f"  - Data Properties: {len(ontology_def.data_properties)}")
        print(f"  - Individuals: {len(ontology_def.individuals)}")
        print(f"  - Axioms: {len(ontology_def.axioms)}")
        
        # Compile with OntologyCompiler (mock telemetry)
        print("\n⚙️ Compiling with OntologyCompiler...")
        
        # Create compiler with mock telemetry
        compiler = OntologyCompiler()
        compiler.telemetry = MockTelemetryManager()
        
        ontology_schema = compiler.compile(ontology_def)
        
        print(f"✅ Ontology compiled successfully:")
        print(f"  - Compiled Classes: {len(ontology_schema.classes)}")
        print(f"  - Semantic Context: {len(ontology_schema.semantic_context)} keys")
        print(f"  - Examples: {len(ontology_schema.examples)}")
        
        # Show semantic types breakdown
        print(f"\n📊 Semantic Types Breakdown:")
        for semantic_type, classes in ontology_schema.semantic_context['semantic_types'].items():
            print(f"  - {semantic_type}: {len(classes)} classes")
            if classes:
                print(f"    Examples: {', '.join(classes[:3])}")
        
        # Show property types breakdown
        print(f"\n🔗 Property Types Breakdown:")
        for prop_type, properties in ontology_schema.semantic_context['property_types'].items():
            print(f"  - {prop_type}: {len(properties)} properties")
            if properties:
                print(f"    Examples: {', '.join(properties[:3])}")
        
        # Show sample class details
        print(f"\n🏗️ Sample Class Details:")
        for i, (class_name, class_schema) in enumerate(list(ontology_schema.classes.items())[:3]):
            print(f"  {i+1}. {class_name} ({class_schema.semantic_type})")
            print(f"     URI: {class_schema.uri}")
            print(f"     Properties: {len(class_schema.properties)}")
            print(f"     Superclasses: {len(class_schema.superclasses)}")
            print(f"     Description: {class_schema.description[:100]}...")
        
        # Export compiled ontology
        output_file = "compiled_ontology_mock.json"
        print(f"\n💾 Exporting compiled ontology to {output_file}...")
        
        # Convert to dict for JSON serialization
        ontology_dict = {
            'ontology_uri': ontology_schema.ontology_uri,
            'namespace': ontology_schema.namespace,
            'prefix': ontology_schema.prefix,
            'classes': {
                name: {
                    'name': cls.name,
                    'uri': cls.uri,
                    'semantic_type': cls.semantic_type,
                    'properties': {
                        prop_name: {
                            'name': prop.name,
                            'uri': prop.uri,
                            'data_type': prop.data_type,
                            'domain': prop.domain,
                            'range': prop.range,
                            'cardinality': prop.cardinality
                        } for prop_name, prop in cls.properties.items()
                    },
                    'superclasses': cls.superclasses,
                    'description': cls.description
                } for name, cls in ontology_schema.classes.items()
            },
            'semantic_context': ontology_schema.semantic_context,
            'examples': ontology_schema.examples
        }
        
        with open(output_file, 'w') as f:
            json.dump(ontology_dict, f, indent=2)
        
        print(f"✅ Compiled ontology exported to {output_file}")
        
        return True
        
    except Exception as e:
        print(f"❌ Error during compilation: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_ontology_compiler_mock()
    if success:
        print("\n🎉 OntologyCompiler mock test PASSED!")
    else:
        print("\n💥 OntologyCompiler mock test FAILED!")
        sys.exit(1) 