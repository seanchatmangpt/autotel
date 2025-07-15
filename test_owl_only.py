#!/usr/bin/env python3
"""
Test OWL Processor Only - Different Data

Tests the OWL processor with various OWL ontologies to ensure it works correctly
with different data structures and content.
"""

from autotel.factory.processors.owl_processor import OWLProcessor
from rdflib import Graph

# Test OWL XML with different ontology structures
TEST_OWL_XML_1 = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/test#"
     xml:base="http://autotel.ai/ontology/test"
     xmlns:owl="http://www.w3.org/2002/07/owl#"
     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
     xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema#">

  <owl:Ontology rdf:about="http://autotel.ai/ontology/test"/>

  <owl:Class rdf:about="http://autotel.ai/ontology/test#Person">
    <rdfs:label>Person</rdfs:label>
    <rdfs:comment>A human being</rdfs:comment>
  </owl:Class>

  <owl:Class rdf:about="http://autotel.ai/ontology/test#Employee">
    <rdfs:label>Employee</rdfs:label>
    <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/test#Person"/>
  </owl:Class>

  <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/test#manages">
    <rdfs:domain rdf:resource="http://autotel.ai/ontology/test#Employee"/>
    <rdfs:range rdf:resource="http://autotel.ai/ontology/test#Person"/>
  </owl:ObjectProperty>

  <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/test#Alice">
    <rdf:type rdf:resource="http://autotel.ai/ontology/test#Employee"/>
    <rdfs:label>Alice</rdfs:label>
  </owl:NamedIndividual>

</rdf:RDF>'''

TEST_OWL_XML_2 = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/test2#"
     xml:base="http://autotel.ai/ontology/test2"
     xmlns:owl="http://www.w3.org/2002/07/owl#"
     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
     xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema#"
     xmlns:test2="http://autotel.ai/ontology/test2#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/test2">
        <rdfs:label>Test Ontology 2</rdfs:label>
        <rdfs:comment>Ontology with restrictions and individuals</rdfs:comment>
    </owl:Ontology>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test2#Employee">
        <rdfs:label>Employee</rdfs:label>
        <rdfs:comment>An employee of the company</rdfs:comment>
        <rdfs:subClassOf>
            <owl:Restriction>
                <owl:onProperty rdf:resource="http://autotel.ai/ontology/test2#hasEmployeeId"/>
                <owl:minCardinality rdf:datatype="http://www.w3.org/2001/XMLSchema#nonNegativeInteger">1</owl:minCardinality>
            </owl:Restriction>
        </rdfs:subClassOf>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test2#Manager">
        <rdfs:label>Manager</rdfs:label>
        <rdfs:comment>A manager who supervises employees</rdfs:comment>
        <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/test2#Employee"/>
    </owl:Class>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/test2#hasEmployeeId">
        <rdfs:label>has employee ID</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test2#Employee"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/test2#manages">
        <rdfs:label>manages</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test2#Manager"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/test2#Employee"/>
    </owl:ObjectProperty>
    
    <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/test2#JohnDoe">
        <rdf:type rdf:resource="http://autotel.ai/ontology/test2#Manager"/>
        <rdfs:label>John Doe</rdfs:label>
        <test2:hasEmployeeId>EMP001</test2:hasEmployeeId>
    </owl:NamedIndividual>
    
    <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/test2#JaneSmith">
        <rdf:type rdf:resource="http://autotel.ai/ontology/test2#Employee"/>
        <rdfs:label>Jane Smith</rdfs:label>
        <test2:hasEmployeeId>EMP002</test2:hasEmployeeId>
    </owl:NamedIndividual>
    
</rdf:RDF>'''

TEST_OWL_XML_3 = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/test3#"
     xml:base="http://autotel.ai/ontology/test3"
     xmlns:owl="http://www.w3.org/2002/07/owl#"
     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
     xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
    
    <owl:Ontology rdf:about="http://autotel.ai/ontology/test3">
        <rdfs:label>Test Ontology 3</rdfs:label>
        <rdfs:comment>Ontology with equivalent classes and complex properties</rdfs:comment>
    </owl:Ontology>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test3#Vehicle">
        <rdfs:label>Vehicle</rdfs:label>
        <rdfs:comment>A vehicle that can transport people or goods</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test3#Car">
        <rdfs:label>Car</rdfs:label>
        <rdfs:comment>A four-wheeled vehicle</rdfs:comment>
        <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/test3#Vehicle"/>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test3#Automobile">
        <rdfs:label>Automobile</rdfs:label>
        <rdfs:comment>Another term for car</rdfs:comment>
        <owl:equivalentClass rdf:resource="http://autotel.ai/ontology/test3#Car"/>
    </owl:Class>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/test3#hasColor">
        <rdfs:label>has color</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test3#Vehicle"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/test3#hasYear">
        <rdfs:label>has year</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test3#Vehicle"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#integer"/>
    </owl:DatatypeProperty>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/test3#hasOwner">
        <rdfs:label>has owner</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/test3#Vehicle"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/test3#Person"/>
    </owl:ObjectProperty>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/test3#Person">
        <rdfs:label>Person</rdfs:label>
        <rdfs:comment>A human person</rdfs:comment>
    </owl:Class>
    
</rdf:RDF>'''

def test_owl_processor():
    """Test OWL processor with different data"""
    processor = OWLProcessor()
    
    print("🧪 Testing OWL Processor with Different Data")
    print("=" * 60)
    
    # Test 1: Basic ontology with simple classes
    print("\n📋 Test 1: Basic Ontology (Product/Customer/Person)")
    print("-" * 40)
    
    graph1 = processor.parse(TEST_OWL_XML_1)
    print(f"Parsed {len(graph1)} triples")
    
    ontology_def1 = processor.parse_ontology_definition(TEST_OWL_XML_1, "test1")
    print(f"Ontology URI: {ontology_def1.ontology_uri}")
    print(f"Classes: {len(ontology_def1.classes)}")
    print(f"Object Properties: {len(ontology_def1.object_properties)}")
    print(f"Data Properties: {len(ontology_def1.data_properties)}")
    print(f"Individuals: {len(ontology_def1.individuals)}")
    
    # Show class details
    for class_uri, class_def in ontology_def1.classes.items():
        print(f"  Class: {class_def.name} - {class_def.label}")
        if class_def.superclasses:
            print(f"    Superclasses: {class_def.superclasses}")
    
    # Test 2: Ontology with restrictions and individuals
    print("\n📋 Test 2: Ontology with Restrictions and Individuals")
    print("-" * 40)
    
    graph2 = processor.parse(TEST_OWL_XML_2)
    print(f"Parsed {len(graph2)} triples")
    
    ontology_def2 = processor.parse_ontology_definition(TEST_OWL_XML_2, "test2")
    print(f"Ontology URI: {ontology_def2.ontology_uri}")
    print(f"Classes: {len(ontology_def2.classes)}")
    print(f"Object Properties: {len(ontology_def2.object_properties)}")
    print(f"Data Properties: {len(ontology_def2.data_properties)}")
    print(f"Individuals: {len(ontology_def2.individuals)}")
    
    # Show individuals
    for ind_uri, ind_def in ontology_def2.individuals.items():
        print(f"  Individual: {ind_def.name} - {ind_def.label}")
        print(f"    Class types: {ind_def.class_types}")
    
    # Test 3: Ontology with equivalent classes
    print("\n📋 Test 3: Ontology with Equivalent Classes")
    print("-" * 40)
    
    graph3 = processor.parse(TEST_OWL_XML_3)
    print(f"Parsed {len(graph3)} triples")
    
    ontology_def3 = processor.parse_ontology_definition(TEST_OWL_XML_3, "test3")
    print(f"Ontology URI: {ontology_def3.ontology_uri}")
    print(f"Classes: {len(ontology_def3.classes)}")
    print(f"Object Properties: {len(ontology_def3.object_properties)}")
    print(f"Data Properties: {len(ontology_def3.data_properties)}")
    print(f"Individuals: {len(ontology_def3.individuals)}")
    print(f"Axioms: {len(ontology_def3.axioms)}")
    
    # Show axioms
    for axiom in ontology_def3.axioms:
        print(f"  Axiom: {axiom['type']} - {axiom['subject']} -> {axiom['object']}")
    
    # Test 4: Combined graph
    print("\n📋 Test 4: Combined Graph from All Ontologies")
    print("-" * 40)
    
    combined_graph = graph1 + graph2 + graph3
    print(f"Combined graph has {len(combined_graph)} triples")
    
    print("\n✅ OWL Processor Tests Complete!")

if __name__ == "__main__":
    test_owl_processor() 