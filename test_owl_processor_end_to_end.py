#!/usr/bin/env python3
"""
Test OWL processor end-to-end functionality.
"""

import sys
import os
import json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from autotel.factory.processors.owl_processor import OWLProcessor
from pathlib import Path

def test_owl_processor_end_to_end():
    """Test OWL processor with the sample workflow ontology."""
    
    print("🧪 Testing OWL Processor End-to-End")
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
        
        print(f"📄 File size: {len(xml_content)} bytes")
        
        # Create OWL processor
        processor = OWLProcessor()
        print("🔧 OWL processor created")
        
        # Parse the ontology
        print("🔄 Parsing ontology...")
        ontology = processor.parse_ontology_definition(xml_content, prefix="workflow")
        
        # Display results
        print("\n📊 Parsing Results:")
        print(f"  • Ontology URI: {ontology.ontology_uri}")
        print(f"  • Namespace: {ontology.namespace}")
        print(f"  • Classes: {len(ontology.classes)}")
        print(f"  • Object Properties: {len(ontology.object_properties)}")
        print(f"  • Data Properties: {len(ontology.data_properties)}")
        print(f"  • Individuals: {len(ontology.individuals)}")
        print(f"  • Axioms: {len(ontology.axioms)}")
        
        # Show some class details
        print("\n🏗️  Sample Classes:")
        for i, (class_name, class_data) in enumerate(list(ontology.classes.items())[:5]):
            print(f"  {i+1}. {class_name}")
            print(f"     URI: {class_data['uri']}")
            print(f"     Label: {class_data['label']}")
            print(f"     Superclasses: {class_data['superclasses']}")
        
        # Show some property details
        print("\n🔗 Sample Object Properties:")
        for i, (prop_name, prop_data) in enumerate(list(ontology.object_properties.items())[:3]):
            print(f"  {i+1}. {prop_name}")
            print(f"     URI: {prop_data['uri']}")
            print(f"     Domain: {prop_data['domain']}")
            print(f"     Range: {prop_data['range']}")
        
        # Export parsed ontology as JSON
        export_path = Path("parsed_ontology.json")
        with open(export_path, "w") as f:
            json.dump({
                "ontology_uri": ontology.ontology_uri,
                "namespace": ontology.namespace,
                "prefix": ontology.prefix,
                "classes": ontology.classes,
                "object_properties": ontology.object_properties,
                "data_properties": ontology.data_properties,
                "individuals": ontology.individuals,
                "axioms": ontology.axioms
            }, f, indent=2)
        print(f"\n💾 Parsed ontology exported to {export_path}")
        
        print("\n✅ OWL processor test completed successfully!")
        return True
        
    except Exception as e:
        print(f"❌ OWL processor test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_owl_processor_end_to_end()
    sys.exit(0 if success else 1) 