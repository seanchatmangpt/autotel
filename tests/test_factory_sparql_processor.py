"""
Test SPARQL Processor - AUTOTEL-104

Tests the isolated SPARQL processor that parses custom <sparql:query> and <sparql:template> blocks from XML
into SPARQLQueryDefinition and SPARQLQueryTemplate data objects.
"""

import pytest
import os
from autotel.processors.sparql_processor import SPARQLProcessor
from autotel.schemas.sparql_types import SPARQLQueryDefinition, SPARQLQueryTemplate
from factories import SPARQLXMLFactory, SPARQLQueryDefinitionFactory, SPARQLQueryTemplateFactory


class TestSPARQLProcessor:
    """Test suite for SPARQLProcessor class."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.processor = SPARQLProcessor()
        self.simple_sparql_xml = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:sparql="http://autotel.ai/sparql">
  <sparql:prefix name="rdf" uri="http://www.w3.org/1999/02/22-rdf-syntax-ns#" />
  <sparql:prefix name="rdfs" uri="http://www.w3.org/2000/01/rdf-schema#" />
  
  <sparql:query name="find_classes" description="Find all classes in the ontology">
    SELECT ?class WHERE { ?class a rdfs:Class }
    <sparql:parameter name="limit" type="integer" required="false" default="100" />
  </sparql:query>
  
  <sparql:query name="find_properties" description="Find all properties">
    SELECT ?property WHERE { ?property a rdf:Property }
  </sparql:query>
  
  <sparql:template name="find_by_type" description="Find resources by type">
    SELECT ?resource WHERE { ?resource a ?type }
    <sparql:validation type="regex" pattern="^[a-zA-Z][a-zA-Z0-9]*$" message="Type must be alphanumeric" />
    <sparql:example>
      <type>http://www.w3.org/2000/01/rdf-schema#Class</type>
    </sparql:example>
  </sparql:template>
</root>"""
    
    def test_sparql_processor_integrity(self):
        """Test that SPARQLProcessor faithfully translates XML into data objects."""
        result = self.processor.parse(self.simple_sparql_xml)
        
        assert isinstance(result, dict)
        assert "queries" in result
        assert "templates" in result
        assert len(result["queries"]) == 2
        assert len(result["templates"]) == 1
        
        # Check first query
        query1 = result["queries"][0]
        assert isinstance(query1, SPARQLQueryDefinition)
        assert query1.name == "find_classes"
        assert query1.description == "Find all classes in the ontology"
        assert query1.query_type == "SELECT"
        assert "SELECT ?class WHERE { ?class a rdfs:Class }" in query1.query
        
        # Check template
        template1 = result["templates"][0]
        assert isinstance(template1, SPARQLQueryTemplate)
        assert template1.name == "find_by_type"
        assert template1.description == "Find resources by type"
        assert "?type" in template1.parameters
        assert len(template1.validation_rules) == 1
        assert len(template1.examples) == 1
    
    def test_parse_valid_sparql(self):
        """Test parsing valid SPARQL XML."""
        result = self.processor.parse(self.simple_sparql_xml)
        
        assert isinstance(result, dict)
        assert "queries" in result
        assert "templates" in result
        
        # Verify all queries are SPARQLQueryDefinition objects
        for query in result["queries"]:
            assert isinstance(query, SPARQLQueryDefinition)
            assert hasattr(query, 'name')
            assert hasattr(query, 'description')
            assert hasattr(query, 'query')
            assert hasattr(query, 'query_type')
            assert hasattr(query, 'parameters')
            assert hasattr(query, 'prefixes')
        
        # Verify all templates are SPARQLQueryTemplate objects
        for template in result["templates"]:
            assert isinstance(template, SPARQLQueryTemplate)
            assert hasattr(template, 'name')
            assert hasattr(template, 'description')
            assert hasattr(template, 'template')
            assert hasattr(template, 'parameters')
            assert hasattr(template, 'validation_rules')
            assert hasattr(template, 'examples')
    
    def test_parse_missing_name_attribute(self):
        """Test parsing query without required name attribute raises ValueError."""
        xml_missing_name = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:sparql="http://autotel.ai/sparql">
  <sparql:query description="Missing name attribute">
    SELECT ?s ?p ?o WHERE { ?s ?p ?o }
  </sparql:query>
</root>"""
        
        with pytest.raises(ValueError, match="SPARQL query is missing required 'name' attribute"):
            self.processor.parse(xml_missing_name)
    
    def test_parse_invalid_xml(self):
        """Test parsing invalid XML raises ValueError."""
        invalid_xml = "<invalid>xml<invalid>"
        
        with pytest.raises(ValueError, match="Invalid XML format"):
            self.processor.parse(invalid_xml)
    
    def test_parse_no_queries(self):
        """Test parsing XML with no SPARQL queries returns empty lists."""
        xml_without_queries = """<?xml version="1.0" encoding="UTF-8"?>
<root xmlns:sparql="http://autotel.ai/sparql">
  <some_element>No SPARQL queries here</some_element>
</root>"""
        
        result = self.processor.parse(xml_without_queries)
        
        assert isinstance(result, dict)
        assert "queries" in result
        assert "templates" in result
        assert len(result["queries"]) == 0
        assert len(result["templates"]) == 0
    
    def test_parse_file_valid(self):
        """Test parsing from file."""
        # Create a temporary SPARQL file
        test_file = "test_simple.sparql.xml"
        try:
            with open(test_file, 'w') as f:
                f.write(self.simple_sparql_xml)
            
            result = self.processor.parse_file(test_file)
            
            assert isinstance(result, dict)
            assert "queries" in result
            assert "templates" in result
            assert len(result["queries"]) == 2
            assert len(result["templates"]) == 1
            
        finally:
            if os.path.exists(test_file):
                os.remove(test_file)
    
    def test_parse_file_nonexistent(self):
        """Test parsing nonexistent file raises FileNotFoundError."""
        with pytest.raises(FileNotFoundError):
            self.processor.parse_file("nonexistent.sparql.xml")
    
    def test_validate_query(self):
        """Test SPARQL query validation."""
        valid_queries = [
            "SELECT ?s ?p ?o WHERE { ?s ?p ?o }",
            "ASK WHERE { ?s ?p ?o }",
            "CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }",
            "DESCRIBE ?s WHERE { ?s ?p ?o }"
        ]
        
        invalid_queries = [
            "INVALID ?s ?p ?o WHERE { ?s ?p ?o }",
            "SELECT ?s ?p ?o",
            "WHERE { ?s ?p ?o }"
        ]
        
        for query in valid_queries:
            assert self.processor.validate_query(query) is True
        
        for query in invalid_queries:
            assert self.processor.validate_query(query) is False
    
    def test_extract_variables(self):
        """Test variable extraction from SPARQL queries."""
        query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o . FILTER(?s = ?subject) }"
        variables = self.processor.extract_variables(query)
        
        assert "s" in variables
        assert "p" in variables
        assert "o" in variables
        assert "subject" in variables
        assert len(variables) == 4
    
    def test_processor_isolation(self):
        """Test that SPARQLProcessor is completely isolated from other DSLs."""
        # Verify processor doesn't have any other DSL parsing methods
        assert not hasattr(self.processor, 'parse_bpmn_processes')
        assert not hasattr(self.processor, 'parse_dmn_decisions')
        assert not hasattr(self.processor, 'parse_shacl_shapes')
        assert not hasattr(self.processor, 'parse_owl_ontologies')
        assert not hasattr(self.processor, 'parse_dspy_signatures')
    
    def test_sparql_query_definition_dataclass(self):
        """Test SPARQLQueryDefinition dataclass functionality."""
        query = SPARQLQueryDefinition(
            name="test_query",
            description="Test query",
            query="SELECT ?s ?p ?o WHERE { ?s ?p ?o }",
            query_type="SELECT",
            parameters={"limit": {"type": "integer", "required": False, "default": "100"}},
            prefixes={"rdf": "http://www.w3.org/1999/02/22-rdf-syntax-ns#"}
        )
        
        assert query.name == "test_query"
        assert query.description == "Test query"
        assert query.query_type == "SELECT"
        assert len(query.parameters) == 1
        assert len(query.prefixes) == 1
    
    def test_sparql_query_template_dataclass(self):
        """Test SPARQLQueryTemplate dataclass functionality."""
        template = SPARQLQueryTemplate(
            name="test_template",
            description="Test template",
            template="SELECT ?s ?p ?o WHERE { ?s ?p ?o . FILTER(?s = ?subject) }",
            parameters=["subject"],
            validation_rules=[{"type": "regex", "pattern": "^[a-zA-Z][a-zA-Z0-9]*$", "message": "Invalid subject"}],
            examples=[{"subject": "http://example.com/resource"}]
        )
        
        assert template.name == "test_template"
        assert template.description == "Test template"
        assert "subject" in template.parameters
        assert len(template.validation_rules) == 1
        assert len(template.examples) == 1


class TestSPARQLProcessorWithFactoryBoy:
    """Test suite for SPARQLProcessor using Factory Boy."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.processor = SPARQLProcessor()
    
    def test_factory_generated_xml_parsing(self):
        """Test parsing factory-generated SPARQL XML."""
        sparql_xml = SPARQLXMLFactory().xml_content
        
        assert "<?xml version=" in sparql_xml
        assert "<root xmlns:sparql=" in sparql_xml
        assert "<sparql:query" in sparql_xml or "<sparql:template" in sparql_xml
        
        result = self.processor.parse(sparql_xml)
        
        assert isinstance(result, dict)
        assert "queries" in result
        assert "templates" in result
        assert len(result["queries"]) >= 0
        assert len(result["templates"]) >= 0
    
    def test_factory_generated_query_definition(self):
        """Test SPARQLQueryDefinitionFactory creates valid objects."""
        query = SPARQLQueryDefinitionFactory()
        
        assert hasattr(query, 'name')
        assert hasattr(query, 'description')
        assert hasattr(query, 'query')
        assert hasattr(query, 'query_type')
        assert hasattr(query, 'parameters')
        assert hasattr(query, 'prefixes')
        
        assert isinstance(query.name, str)
        assert isinstance(query.description, str)
        assert isinstance(query.query, str)
        assert isinstance(query.query_type, str)
        assert isinstance(query.parameters, dict)
        assert isinstance(query.prefixes, dict)
        
        assert query.query_type in ['SELECT', 'ASK', 'CONSTRUCT', 'DESCRIBE']
    
    def test_factory_generated_query_template(self):
        """Test SPARQLQueryTemplateFactory creates valid objects."""
        template = SPARQLQueryTemplateFactory()
        
        assert hasattr(template, 'name')
        assert hasattr(template, 'description')
        assert hasattr(template, 'template')
        assert hasattr(template, 'parameters')
        assert hasattr(template, 'validation_rules')
        assert hasattr(template, 'examples')
        
        assert isinstance(template.name, str)
        assert isinstance(template.description, str)
        assert isinstance(template.template, str)
        assert isinstance(template.parameters, list)
        assert isinstance(template.validation_rules, list)
        assert isinstance(template.examples, list)
        
        assert len(template.parameters) > 0
    
    def test_multiple_factory_generated_queries(self):
        """Test processing multiple factory-generated queries."""
        queries = [SPARQLQueryDefinitionFactory() for _ in range(3)]
        
        for query in queries:
            assert query.name != ""
            assert query.query != ""
            assert query.query_type in ['SELECT', 'ASK', 'CONSTRUCT', 'DESCRIBE']
    
    def test_multiple_factory_generated_templates(self):
        """Test processing multiple factory-generated templates."""
        templates = [SPARQLQueryTemplateFactory() for _ in range(2)]
        
        for template in templates:
            assert template.name != ""
            assert template.template != ""
            assert len(template.parameters) > 0


if __name__ == "__main__":
    pytest.main([__file__]) 