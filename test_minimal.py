#!/usr/bin/env python3
"""Minimal test to isolate OntologyCompiler issue."""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_minimal():
    """Minimal test to check if classes can be imported and instantiated."""
    
    print("🧪 Minimal Test")
    print("=" * 20)
    
    try:
        # Test 1: Import classes
        print("1. Importing classes...")
        from autotel.factory.processors.owl_processor import OWLProcessor
        from autotel.factory.ontology_compiler import OntologyCompiler
        print("✅ Classes imported successfully")
        
        # Test 2: Create instances
        print("2. Creating instances...")
        processor = OWLProcessor()
        compiler = OntologyCompiler()
        print("✅ Instances created successfully")
        
        # Test 3: Load OWL file
        print("3. Loading OWL file...")
        with open('examples/workflow_ontology.owl', 'r') as f:
            xml_content = f.read()
        print(f"✅ OWL file loaded ({len(xml_content)} characters)")
        
        # Test 4: Parse OWL (suppress telemetry)
        print("4. Parsing OWL...")
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            old_stdout = sys.stdout
            sys.stdout = f
            try:
                ontology_def = processor.parse_ontology_definition(xml_content, prefix='minimal_test')
                print("✅ OWL parsed successfully")
            finally:
                sys.stdout = old_stdout
                os.unlink(f.name)
        
        print(f"✅ Ontology definition created:")
        print(f"  - Classes: {len(ontology_def.classes)}")
        print(f"  - Properties: {len(ontology_def.object_properties) + len(ontology_def.data_properties)}")
        
        # Test 5: Compile ontology (suppress telemetry)
        print("5. Compiling ontology...")
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            old_stdout = sys.stdout
            sys.stdout = f
            try:
                ontology_schema = compiler.compile(ontology_def)
                print("✅ Ontology compiled successfully")
            finally:
                sys.stdout = old_stdout
                os.unlink(f.name)
        
        print(f"✅ Ontology schema created:")
        print(f"  - Compiled classes: {len(ontology_schema.classes)}")
        print(f"  - Semantic context: {len(ontology_schema.semantic_context)} keys")
        
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_minimal()
    if success:
        print("\n🎉 Minimal test PASSED!")
    else:
        print("\n💥 Minimal test FAILED!")
        sys.exit(1) 