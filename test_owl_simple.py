#!/usr/bin/env python3
"""Simple OWL processor test"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from autotel.factory.processors.owl_processor import OWLProcessor

def main():
    print("Testing OWL processor...")
    
    # Load the OWL file
    with open('examples/workflow_ontology.owl', 'r') as f:
        content = f.read()
    
    print(f"Loaded {len(content)} characters of OWL content")
    
    # Create processor and parse
    processor = OWLProcessor()
    ontology = processor.parse_ontology_definition(content, prefix='cli')
    
    # Print results
    print(f"✅ Successfully parsed ontology:")
    print(f"  - Classes: {len(ontology.classes)}")
    print(f"  - Object Properties: {len(ontology.object_properties)}")
    print(f"  - Data Properties: {len(ontology.data_properties)}")
    print(f"  - Individuals: {len(ontology.individuals)}")
    print(f"  - Axioms: {len(ontology.axioms)}")
    
    # Show some sample data
    if ontology.classes:
        print(f"\nSample classes:")
        for i, (name, data) in enumerate(list(ontology.classes.items())[:3]):
            print(f"  {i+1}. {name}: {data.get('label', 'No label')}")

if __name__ == "__main__":
    main() 