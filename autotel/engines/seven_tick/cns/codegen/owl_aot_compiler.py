#!/usr/bin/env python3
"""
OWL AOT Compiler for CNS 7-Tick Engine

This compiler implements the "Reasoner is the Build System" philosophy:
- All OWL reasoning happens offline during compilation
- Runtime C code is "deliberately dumb and fast"
- Generates optimized C headers with pre-computed inferences

Based on the OWL-AOT.md specification.
"""

import argparse
import os
import sys
from pathlib import Path
from rdflib import Graph, Namespace, RDF, RDFS, OWL, URIRef, Literal
from rdflib.plugins.sparql import prepareQuery
import logging

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class OWLAOTCompiler:
    """
    Ahead-of-Time compiler for OWL ontologies.
    Generates optimized C code for 7-tick performance.
    """
    
    def __init__(self, output_dir="generated"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # RDF graph to hold all ontology data
        self.graph = Graph()
        
        # Entity mappings for C code generation
        self.class_mappings = {}
        self.property_mappings = {}
        self.entity_counter = 1000  # Start IDs at 1000 to avoid conflicts
        
        # Materialized facts (subject, predicate, object)
        self.materialized_triples = set()
        
        # Inference rules extracted from ontology
        self.subclass_rules = []
        self.transitive_properties = set()
        self.symmetric_properties = set()
        self.inverse_properties = {}
        
    def load_ontology(self, ontology_file):
        """Load an OWL ontology file into the graph."""
        logger.info(f"Loading ontology: {ontology_file}")
        try:
            self.graph.parse(ontology_file, format="turtle")
            logger.info(f"Loaded {len(self.graph)} triples")
        except Exception as e:
            logger.error(f"Failed to load {ontology_file}: {e}")
            raise
    
    def extract_ontology_rules(self):
        """Extract OWL rules and axioms from the loaded ontology."""
        logger.info("Extracting OWL rules and axioms...")
        
        # Extract subclass relationships
        subclass_query = """
        SELECT ?sub ?super WHERE {
            ?sub rdfs:subClassOf ?super .
            FILTER(!isBlank(?sub) && !isBlank(?super))
        }
        """
        for row in self.graph.query(subclass_query):
            sub_name = self._get_local_name(row.sub)
            super_name = self._get_local_name(row.super)
            self.subclass_rules.append({
                'type': 'subClassOf',
                'sub': sub_name,
                'super': super_name,
                'sub_uri': str(row.sub),
                'super_uri': str(row.super)
            })
            logger.debug(f"Subclass: {sub_name} rdfs:subClassOf {super_name}")
        
        # Extract transitive properties
        transitive_query = """
        SELECT ?prop WHERE {
            ?prop rdf:type owl:TransitiveProperty .
        }
        """
        for row in self.graph.query(transitive_query):
            prop_name = self._get_local_name(row.prop)
            self.transitive_properties.add(prop_name)
            logger.debug(f"Transitive property: {prop_name}")
        
        # Extract symmetric properties
        symmetric_query = """
        SELECT ?prop WHERE {
            ?prop rdf:type owl:SymmetricProperty .
        }
        """
        for row in self.graph.query(symmetric_query):
            prop_name = self._get_local_name(row.prop)
            self.symmetric_properties.add(prop_name)
            logger.debug(f"Symmetric property: {prop_name}")
        
        # Extract inverse properties
        inverse_query = """
        SELECT ?prop1 ?prop2 WHERE {
            ?prop1 owl:inverseOf ?prop2 .
        }
        """
        for row in self.graph.query(inverse_query):
            prop1_name = self._get_local_name(row.prop1)
            prop2_name = self._get_local_name(row.prop2)
            self.inverse_properties[prop1_name] = prop2_name
            self.inverse_properties[prop2_name] = prop1_name
            logger.debug(f"Inverse properties: {prop1_name} <-> {prop2_name}")
        
        logger.info(f"Extracted {len(self.subclass_rules)} subclass rules")
        logger.info(f"Found {len(self.transitive_properties)} transitive properties")
        logger.info(f"Found {len(self.symmetric_properties)} symmetric properties")
        logger.info(f"Found {len(self.inverse_properties)//2} inverse property pairs")
    
    def materialize_inferences(self):
        """Compute the deductive closure - all possible inferences."""
        logger.info("Materializing inferences (computing deductive closure)...")
        
        # Start with all asserted triples
        for triple in self.graph:
            self.materialized_triples.add(triple)
        
        # Apply inference rules until no new triples are generated
        changed = True
        iteration = 0
        while changed and iteration < 100:  # Prevent infinite loops
            changed = False
            iteration += 1
            old_count = len(self.materialized_triples)
            
            # Apply subclass transitivity: if A subClassOf B and B subClassOf C, then A subClassOf C
            self._apply_subclass_transitivity()
            
            # Apply transitive property rules
            self._apply_transitive_properties()
            
            # Apply symmetric property rules
            self._apply_symmetric_properties()
            
            # Apply inverse property rules
            self._apply_inverse_properties()
            
            new_count = len(self.materialized_triples)
            if new_count > old_count:
                changed = True
                logger.debug(f"Iteration {iteration}: {new_count - old_count} new triples")
        
        logger.info(f"Materialization complete after {iteration} iterations")
        logger.info(f"Total materialized triples: {len(self.materialized_triples)}")
    
    def _apply_subclass_transitivity(self):
        """Apply transitivity rule for rdfs:subClassOf."""
        new_triples = set()
        
        for s1, p1, o1 in self.materialized_triples:
            if p1 == RDFS.subClassOf:
                for s2, p2, o2 in self.materialized_triples:
                    if p2 == RDFS.subClassOf and o1 == s2:
                        # s1 subClassOf o1, o1 subClassOf o2 => s1 subClassOf o2
                        new_triple = (s1, RDFS.subClassOf, o2)
                        if new_triple not in self.materialized_triples:
                            new_triples.add(new_triple)
        
        self.materialized_triples.update(new_triples)
    
    def _apply_transitive_properties(self):
        """Apply transitivity rules for transitive properties."""
        new_triples = set()
        
        for prop_name in self.transitive_properties:
            prop_uri = URIRef(f"http://example.org/{prop_name}")  # Simplified URI
            
            for s1, p1, o1 in self.materialized_triples:
                if p1 == prop_uri:
                    for s2, p2, o2 in self.materialized_triples:
                        if p2 == prop_uri and o1 == s2:
                            # s1 prop o1, o1 prop o2 => s1 prop o2
                            new_triple = (s1, prop_uri, o2)
                            if new_triple not in self.materialized_triples:
                                new_triples.add(new_triple)
        
        self.materialized_triples.update(new_triples)
    
    def _apply_symmetric_properties(self):
        """Apply symmetry rules for symmetric properties."""
        new_triples = set()
        
        for prop_name in self.symmetric_properties:
            prop_uri = URIRef(f"http://example.org/{prop_name}")
            
            for s, p, o in self.materialized_triples:
                if p == prop_uri:
                    # s prop o => o prop s
                    new_triple = (o, prop_uri, s)
                    if new_triple not in self.materialized_triples:
                        new_triples.add(new_triple)
        
        self.materialized_triples.update(new_triples)
    
    def _apply_inverse_properties(self):
        """Apply inverse property rules."""
        new_triples = set()
        
        for prop1_name, prop2_name in self.inverse_properties.items():
            prop1_uri = URIRef(f"http://example.org/{prop1_name}")
            prop2_uri = URIRef(f"http://example.org/{prop2_name}")
            
            for s, p, o in self.materialized_triples:
                if p == prop1_uri:
                    # s prop1 o => o prop2 s
                    new_triple = (o, prop2_uri, s)
                    if new_triple not in self.materialized_triples:
                        new_triples.add(new_triple)
        
        self.materialized_triples.update(new_triples)
    
    def generate_entity_mappings(self):
        """Generate numeric ID mappings for all entities."""
        logger.info("Generating entity ID mappings...")
        
        # Collect all unique entities
        all_entities = set()
        for s, p, o in self.materialized_triples:
            if not isinstance(s, Literal):
                all_entities.add(s)
            if not isinstance(o, Literal):
                all_entities.add(o)
            all_entities.add(p)
        
        # Extract classes from ontology structure
        classes = set()
        properties = set()
        
        # Add classes from subclass rules
        for rule in self.subclass_rules:
            classes.add(rule['sub'])
            classes.add(rule['super'])
        
        # Add classes that are declared as owl:Class
        class_query = """
        SELECT ?cls WHERE {
            ?cls rdf:type owl:Class .
        }
        """
        for row in self.graph.query(class_query):
            classes.add(self._get_local_name(row.cls))
        
        # Properties include RDF/OWL properties and declared properties
        for entity in all_entities:
            entity_name = self._get_local_name(entity)
            if entity_name not in classes:
                # Check if it's a known property
                if (entity in [RDFS.subClassOf, RDF.type, OWL.inverseOf] or 
                    entity_name in self.transitive_properties or 
                    entity_name in self.symmetric_properties):
                    properties.add(entity_name)
                else:
                    # Check if declared as property in ontology
                    prop_query = f"""
                    SELECT ?p WHERE {{
                        ?p rdf:type owl:ObjectProperty .
                        FILTER(str(?p) = "{entity}")
                    }}
                    """
                    if list(self.graph.query(prop_query)):
                        properties.add(entity_name)
                    else:
                        properties.add(entity_name)  # Default to property
        
        # Generate ID mappings
        class_id = 0x10000  # Classes start at 65536
        for cls in sorted(classes):
            self.class_mappings[cls] = class_id
            class_id += 1
        
        property_id = 0x20000  # Properties start at 131072
        for prop in sorted(properties):
            self.property_mappings[prop] = property_id
            property_id += 1
        
        logger.info(f"Generated {len(self.class_mappings)} class IDs")
        logger.info(f"Generated {len(self.property_mappings)} property IDs")
    
    def generate_ontology_ids_header(self):
        """Generate ontology_ids.h with entity ID definitions."""
        logger.info("Generating ontology_ids.h...")
        
        header_content = '''/*
 * Ontology Entity IDs - Generated by CNS OWL AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 * 
 * This implements the "Reasoner is the Build System" philosophy:
 * - All OWL semantics compiled away at build time
 * - Runtime C code works with numeric IDs for maximum performance
 */

#ifndef ONTOLOGY_IDS_H
#define ONTOLOGY_IDS_H

#include <stdint.h>

/* Entity ID Type */
typedef uint32_t cns_entity_id_t;

/* Base ID ranges for different entity types */
#define CNS_CLASS_ID_BASE    0x10000
#define CNS_PROPERTY_ID_BASE 0x20000
#define CNS_INSTANCE_ID_BASE 0x30000

/* ===== GENERATED CLASS IDs ===== */
'''
        
        for cls, cls_id in sorted(self.class_mappings.items()):
            header_content += f"#define CNS_{cls.upper()}_CLASS {cls_id}\n"
        
        header_content += "\n/* ===== GENERATED PROPERTY IDs ===== */\n"
        
        for prop, prop_id in sorted(self.property_mappings.items()):
            header_content += f"#define CNS_{prop.upper()}_PROPERTY {prop_id}\n"
        
        header_content += "\n#endif /* ONTOLOGY_IDS_H */\n"
        
        output_file = self.output_dir / "ontology_ids.h"
        with open(output_file, 'w') as f:
            f.write(header_content)
        
        logger.info(f"Generated {output_file}")
    
    def generate_ontology_rules_header(self):
        """Generate ontology_rules.h with optimized inference functions."""
        logger.info("Generating ontology_rules.h...")
        
        header_content = '''/*
 * Ontology Inference Rules - Generated by CNS OWL AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 * 
 * These functions implement OWL semantics as optimized C code:
 * - Static inline functions for maximum performance (7-tick compliant)
 * - Pre-computed inferences using || short-circuit evaluation
 * - Direct integration with CNS SPARQL engine
 */

#ifndef ONTOLOGY_RULES_H
#define ONTOLOGY_RULES_H

#include "ontology_ids.h"
#include "cns/engines/sparql.h"
#include <stdbool.h>

/* Global SPARQL engine - assumed to be available */
extern CNSSparqlEngine* g_sparql_engine;

/* ===== SUBCLASS CHECKING FUNCTIONS ===== */
'''
        
        # Generate subclass checking functions - group by superclass to avoid duplicates
        superclass_functions = {}
        for rule in self.subclass_rules:
            super_class = rule['super']
            if super_class not in superclass_functions:
                superclass_functions[super_class] = []
            superclass_functions[super_class].append(rule['sub'])
        
        for super_class, subclasses in superclass_functions.items():
            super_upper = super_class.upper()
            
            # Build the check conditions
            conditions = [f"cns_sparql_ask_pattern(g_sparql_engine, entity_id, CNS_TYPE_PROPERTY, CNS_{super_upper}_CLASS)"]
            for sub_class in subclasses:
                sub_upper = sub_class.upper()
                conditions.append(f"cns_sparql_ask_pattern(g_sparql_engine, entity_id, CNS_TYPE_PROPERTY, CNS_{sub_upper}_CLASS)")
            
            subclass_list = ", ".join(subclasses)
            
            header_content += f'''
/**
 * @brief Checks if an entity is of type {super_class} (including subclasses: {subclass_list})
 * @param entity_id The entity ID to check
 * @return true if entity is {super_class} or any of its subclasses
 */
static inline bool is_{super_class}(cns_entity_id_t entity_id) {{
    return {" ||\\n           ".join(conditions)};
}}
'''
        
        # Generate transitive property functions
        header_content += "\n/* ===== TRANSITIVE PROPERTY FUNCTIONS ===== */\n"
        
        for prop in self.transitive_properties:
            prop_upper = prop.upper()
            header_content += f'''
/**
 * @brief Checks transitive {prop} relationship (pre-computed at build time)
 * @param subject_id Subject entity ID
 * @param object_id Object entity ID  
 * @return true if subject {prop} object (direct or transitive)
 */
static inline bool check_{prop}_transitive(cns_entity_id_t subject_id, cns_entity_id_t object_id) {{
    /* Transitive closure pre-computed - just a single lookup */
    return cns_sparql_ask_pattern(g_sparql_engine, subject_id, CNS_{prop_upper}_PROPERTY, object_id);
}}
'''
        
        # Generate symmetric property functions
        header_content += "\n/* ===== SYMMETRIC PROPERTY FUNCTIONS ===== */\n"
        
        for prop in self.symmetric_properties:
            prop_upper = prop.upper()
            header_content += f'''
/**
 * @brief Checks symmetric {prop} relationship
 * @param entity1_id First entity ID
 * @param entity2_id Second entity ID
 * @return true if entity1 {prop} entity2 OR entity2 {prop} entity1
 */
static inline bool check_{prop}_symmetric(cns_entity_id_t entity1_id, cns_entity_id_t entity2_id) {{
    /* Check both directions for symmetry */
    return cns_sparql_ask_pattern(g_sparql_engine, entity1_id, CNS_{prop_upper}_PROPERTY, entity2_id) ||
           cns_sparql_ask_pattern(g_sparql_engine, entity2_id, CNS_{prop_upper}_PROPERTY, entity1_id);
}}
'''
        
        # Generate inverse property functions
        header_content += "\n/* ===== INVERSE PROPERTY FUNCTIONS ===== */\n"
        
        for prop1, prop2 in self.inverse_properties.items():
            if prop1 < prop2:  # Avoid duplicates
                prop1_upper = prop1.upper()
                prop2_upper = prop2.upper()
                header_content += f'''
/**
 * @brief Checks inverse relationship between {prop1} and {prop2}
 * @param subject_id Subject entity ID
 * @param object_id Object entity ID
 * @return true if subject {prop1} object OR object {prop2} subject
 */
static inline bool check_{prop1}_{prop2}_inverse(cns_entity_id_t subject_id, cns_entity_id_t object_id) {{
    /* Check both inverse directions */
    return cns_sparql_ask_pattern(g_sparql_engine, subject_id, CNS_{prop1_upper}_PROPERTY, object_id) ||
           cns_sparql_ask_pattern(g_sparql_engine, object_id, CNS_{prop2_upper}_PROPERTY, subject_id);
}}
'''
        
        header_content += "\n#endif /* ONTOLOGY_RULES_H */\n"
        
        output_file = self.output_dir / "ontology_rules.h"
        with open(output_file, 'w') as f:
            f.write(header_content)
        
        logger.info(f"Generated {output_file}")
    
    def generate_materialized_triples_data(self):
        """Generate materialized triples as C data structures."""
        logger.info("Generating materialized_triples.h...")
        
        header_content = '''/*
 * Materialized Triple Set - Generated by CNS OWL AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 * 
 * This contains the complete deductive closure - all possible inferences
 * pre-computed at build time for maximum runtime performance.
 */

#ifndef MATERIALIZED_TRIPLES_H
#define MATERIALIZED_TRIPLES_H

#include "ontology_ids.h"
#include <stddef.h>

/* Triple structure for materialized facts */
typedef struct {
    cns_entity_id_t subject;
    cns_entity_id_t predicate;
    cns_entity_id_t object;
} cns_materialized_triple_t;

/* Generated materialized triples */
static const cns_materialized_triple_t materialized_triples[] = {
'''
        
        # Convert RDF triples to C array
        for s, p, o in sorted(self.materialized_triples):
            if isinstance(s, Literal) or isinstance(o, Literal):
                continue  # Skip literals for now
            
            s_name = self._get_local_name(s)
            p_name = self._get_local_name(p)
            o_name = self._get_local_name(o)
            
            s_id = self.class_mappings.get(s_name, self.property_mappings.get(s_name, 0))
            p_id = self.property_mappings.get(p_name, 0)
            o_id = self.class_mappings.get(o_name, self.property_mappings.get(o_name, 0))
            
            if s_id and p_id and o_id:
                header_content += f"    {{{s_id}, {p_id}, {o_id}}}, /* {s_name} {p_name} {o_name} */\n"
        
        header_content += '''};

/* Number of materialized triples */
static const size_t materialized_triples_count = sizeof(materialized_triples) / sizeof(materialized_triples[0]);

#endif /* MATERIALIZED_TRIPLES_H */
'''
        
        output_file = self.output_dir / "materialized_triples.h"
        with open(output_file, 'w') as f:
            f.write(header_content)
        
        logger.info(f"Generated {output_file}")
    
    def _get_local_name(self, uri):
        """Extract local name from URI."""
        uri_str = str(uri)
        if '#' in uri_str:
            return uri_str.split('#')[-1]
        elif '/' in uri_str:
            return uri_str.split('/')[-1]
        else:
            return uri_str
    
    def compile(self, ontology_files):
        """Main compilation process."""
        logger.info("Starting OWL AOT compilation...")
        
        # Load all ontology files
        for ontology_file in ontology_files:
            self.load_ontology(ontology_file)
        
        # Extract rules and compute inferences
        self.extract_ontology_rules()
        self.materialize_inferences()
        self.generate_entity_mappings()
        
        # Generate C headers
        self.generate_ontology_ids_header()
        self.generate_ontology_rules_header()
        self.generate_materialized_triples_data()
        
        logger.info("OWL AOT compilation complete!")
        logger.info(f"Generated files in: {self.output_dir}")

def main():
    parser = argparse.ArgumentParser(
        description='OWL AOT Compiler - Generate optimized C code from OWL ontologies'
    )
    parser.add_argument(
        'ontologies',
        nargs='+',
        help='OWL ontology files (Turtle format)'
    )
    parser.add_argument(
        '--output',
        '-o',
        default='generated',
        help='Output directory for generated C headers (default: generated)'
    )
    parser.add_argument(
        '--verbose',
        '-v',
        action='store_true',
        help='Enable verbose logging'
    )
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Validate input files
    for ontology_file in args.ontologies:
        if not os.path.exists(ontology_file):
            logger.error(f"Ontology file not found: {ontology_file}")
            sys.exit(1)
    
    # Run compilation
    try:
        compiler = OWLAOTCompiler(args.output)
        compiler.compile(args.ontologies)
        logger.info("✅ AOT compilation successful!")
    except Exception as e:
        logger.error(f"❌ AOT compilation failed: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()