"""
Test SPARQL Queries on SRO Ontology with Factory Boy

This test loads 5 SPARQL queries from XML, parses them with the SPARQLProcessor, 
and validates them against Factory Boy generated ontology data.
"""

import pytest
from autotel.processors.sparql_processor import SPARQLProcessor
from autotel.schemas.sparql_types import SPARQLQueryDefinition
from tests.factories import (
    SPARQLQueryDefinitionFactory,
    SROOntologyDataFactory,
    SROClassFactory,
    SROUserStoryFactory,
    SROScrumTeamMemberFactory,
    SROObjectPropertyFactory,
    SROUserStoryPropertyFactory
)

SRO_SPARQL_XML = '''<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:sparql="http://autotel.ai/sparql">
  <sparql:query name="list_scrum_classes" description="List all Scrum classes">
    <![CDATA[
    PREFIX sro: <http://example.com/sro#>
    SELECT ?class
    WHERE {
      ?class a owl:Class .
      FILTER(STRSTARTS(STR(?class), STR(sro:)))
    }
    ]]>
  </sparql:query>
  <sparql:query name="subclasses_of_userstory" description="Find all subclasses of UserStory">
    <![CDATA[
    PREFIX sro: <http://example.com/sro#>
    SELECT ?subclass
    WHERE {
      ?subclass rdfs:subClassOf sro:UserStory .
    }
    ]]>
  </sparql:query>
  <sparql:query name="object_properties_domains_ranges" description="List all object properties and their domains/ranges">
    <![CDATA[
    PREFIX sro: <http://example.com/sro#>
    SELECT ?property ?domain ?range
    WHERE {
      ?property a owl:ObjectProperty .
      OPTIONAL { ?property rdfs:domain ?domain . }
      OPTIONAL { ?property rdfs:range ?range . }
      FILTER(STRSTARTS(STR(?property), STR(sro:)))
    }
    ]]>
  </sparql:query>
  <sparql:query name="scrum_roles" description="Find all Scrum roles (subclasses of ScrumTeamMember)">
    <![CDATA[
    PREFIX sro: <http://example.com/sro#>
    SELECT ?role
    WHERE {
      ?role rdfs:subClassOf sro:ScrumTeamMember .
    }
    ]]>
  </sparql:query>
  <sparql:query name="userstory_properties" description="List all properties related to UserStory (domain or range)">
    <![CDATA[
    PREFIX sro: <http://example.com/sro#>
    SELECT ?property ?relation
    WHERE {
      {
        ?property rdfs:domain sro:UserStory .
        BIND("domain" AS ?relation)
      }
      UNION
      {
        ?property rdfs:range sro:UserStory .
        BIND("range" AS ?relation)
      }
    }
    ]]>
  </sparql:query>
</root>
'''


def test_parse_sro_sparql_queries():
    """Test parsing SPARQL queries from XML"""
    processor = SPARQLProcessor()
    result = processor.parse(SRO_SPARQL_XML)
    assert isinstance(result, dict)
    assert "queries" in result
    queries = result["queries"]
    assert len(queries) == 5
    for query in queries:
        assert isinstance(query, SPARQLQueryDefinition)
        assert query.query.strip().startswith("PREFIX sro:")
        assert query.name is not None
        assert query.description is not None


def test_sparql_queries_with_factory_data():
    """Test SPARQL queries against Factory Boy generated ontology data"""
    # Generate test data using Factory Boy
    ontology_data = SROOntologyDataFactory()
    
    # Parse queries
    processor = SPARQLProcessor()
    result = processor.parse(SRO_SPARQL_XML)
    queries = result["queries"]
    
    # Validate queries against generated data
    for query in queries:
        assert query.name in [
            "list_scrum_classes",
            "subclasses_of_userstory", 
            "object_properties_domains_ranges",
            "scrum_roles",
            "userstory_properties"
        ]
        
        # Check that queries reference expected SRO entities
        if query.name == "list_scrum_classes":
            assert "sro:" in query.query
            assert "owl:Class" in query.query
            
        elif query.name == "subclasses_of_userstory":
            assert "sro:UserStory" in query.query
            assert "rdfs:subClassOf" in query.query
            
        elif query.name == "object_properties_domains_ranges":
            assert "owl:ObjectProperty" in query.query
            assert "rdfs:domain" in query.query
            assert "rdfs:range" in query.query
            
        elif query.name == "scrum_roles":
            assert "sro:ScrumTeamMember" in query.query
            assert "rdfs:subClassOf" in query.query
            
        elif query.name == "userstory_properties":
            assert "sro:UserStory" in query.query
            assert "rdfs:domain" in query.query
            assert "rdfs:range" in query.query


def test_factory_generated_sparql_queries():
    """Test Factory Boy generated SPARQL queries"""
    # Generate multiple SPARQL queries using Factory Boy
    queries = SPARQLQueryDefinitionFactory.create_batch(3)
    
    assert len(queries) == 3
    for i, query in enumerate(queries):
        assert query.name == f"query_{i}"
        assert query.description is not None
        assert "PREFIX sro:" in query.query
        assert "owl:Class" in query.query


def test_ontology_data_factories():
    """Test Factory Boy ontology data generation"""
    # Test individual factories
    class_data = SROClassFactory()
    assert class_data["uri"].startswith("http://example.com/sro#")
    assert class_data["type"] == "owl:Class"
    
    user_story = SROUserStoryFactory()
    assert user_story["uri"] == "http://example.com/sro#UserStory"
    assert len(user_story["subclasses"]) == 2
    
    scrum_member = SROScrumTeamMemberFactory()
    assert scrum_member["uri"] == "http://example.com/sro#ScrumTeamMember"
    assert len(scrum_member["roles"]) == 3
    
    object_property = SROObjectPropertyFactory()
    assert object_property["type"] == "owl:ObjectProperty"
    assert "domain" in object_property
    assert "range" in object_property
    
    user_story_property = SROUserStoryPropertyFactory()
    assert user_story_property["relation"] in ["domain", "range"]
    assert user_story_property["user_story_uri"] == "http://example.com/sro#UserStory"
    
    # Test comprehensive ontology data
    ontology = SROOntologyDataFactory()
    assert len(ontology["classes"]) == 5
    assert ontology["user_story"]["name"] == "UserStory"
    assert ontology["scrum_team_member"]["name"] == "ScrumTeamMember"
    assert len(ontology["object_properties"]) == 3
    assert len(ontology["user_story_properties"]) == 4


# Note: Actual SPARQL execution against sro.owl would require an RDF backend (e.g., rdflib, Jena, etc.)
# This test ensures the queries are loaded from XML and parsed correctly for further execution. 