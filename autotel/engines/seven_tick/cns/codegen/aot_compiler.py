#!/usr/bin/env python3
"""
CNS Ahead-of-Time (AOT) Compiler.
Parses ontologies, shapes, and queries, and generates optimized C headers.
"""
import sys
import os
import argparse
from pathlib import Path
from rdflib import Graph
import json
import re

def parse_ontology_rules(graph):
    """Extract OWL/RDFS axioms from the graph"""
    print("    - Parsing OWL/RDFS axioms...")
    
    rules = []
    
    # Query for subClassOf relationships
    subclass_query = """
    PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
    SELECT ?sub ?super WHERE {
        ?sub rdfs:subClassOf ?super .
    }
    """
    
    for row in graph.query(subclass_query):
        sub_class = str(row.sub).split('#')[-1] if '#' in str(row.sub) else str(row.sub).split('/')[-1]
        super_class = str(row.super).split('#')[-1] if '#' in str(row.super) else str(row.super).split('/')[-1]
        
        rules.append({
            "type": "subClassOf",
            "sub": sub_class,
            "super": super_class,
            "sub_uri": str(row.sub),
            "super_uri": str(row.super)
        })
    
    # Query for property domains and ranges
    domain_query = """
    PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
    SELECT ?prop ?domain WHERE {
        ?prop rdfs:domain ?domain .
    }
    """
    
    for row in graph.query(domain_query):
        prop_name = str(row.prop).split('#')[-1] if '#' in str(row.prop) else str(row.prop).split('/')[-1]
        domain_name = str(row.domain).split('#')[-1] if '#' in str(row.domain) else str(row.domain).split('/')[-1]
        
        rules.append({
            "type": "domain",
            "property": prop_name,
            "domain": domain_name,
            "prop_uri": str(row.prop),
            "domain_uri": str(row.domain)
        })
    
    return {"rules": rules}

def parse_shacl_shapes(graph):
    """Parse SHACL shapes from the graph"""
    print("    - Parsing SHACL shapes...")
    
    shapes = []
    
    # Query for SHACL node shapes
    shapes_query = """
    PREFIX sh: <http://www.w3.org/ns/shacl#>
    SELECT ?shape ?target WHERE {
        ?shape a sh:NodeShape .
        OPTIONAL { ?shape sh:targetClass ?target }
    }
    """
    
    for row in graph.query(shapes_query):
        shape_name = str(row.shape).split('#')[-1] if '#' in str(row.shape) else str(row.shape).split('/')[-1]
        target_name = str(row.target).split('#')[-1] if '#' in str(row.target) and row.target else "Any"
        
        # Query for properties of this shape
        prop_query = f"""
        PREFIX sh: <http://www.w3.org/ns/shacl#>
        SELECT ?prop ?path ?minCount ?maxCount ?datatype WHERE {{
            <{row.shape}> sh:property ?prop .
            ?prop sh:path ?path .
            OPTIONAL {{ ?prop sh:minCount ?minCount }}
            OPTIONAL {{ ?prop sh:maxCount ?maxCount }}
            OPTIONAL {{ ?prop sh:datatype ?datatype }}
        }}
        """
        
        constraints = []
        for prop_row in graph.query(prop_query):
            path_name = str(prop_row.path).split('#')[-1] if '#' in str(prop_row.path) else str(prop_row.path).split('/')[-1]
            constraint = {
                "path": path_name,
                "path_uri": str(prop_row.path)
            }
            if prop_row.minCount:
                constraint["minCount"] = int(prop_row.minCount)
            if prop_row.maxCount:
                constraint["maxCount"] = int(prop_row.maxCount)
            if prop_row.datatype:
                constraint["datatype"] = str(prop_row.datatype)
            
            constraints.append(constraint)
        
        shapes.append({
            "name": shape_name,
            "target": target_name,
            "constraints": constraints,
            "shape_uri": str(row.shape),
            "target_uri": str(row.target) if row.target else None
        })
    
    return {"shapes": shapes}

def parse_sql_queries(sql_file_path):
    """Parse named SQL queries from a SQL file"""
    print("    - Parsing named SQL queries...")
    
    if not os.path.exists(sql_file_path):
        print(f"      Warning: SQL file {sql_file_path} not found, skipping...")
        return {"queries": []}
    
    with open(sql_file_path, 'r') as f:
        content = f.read()
    
    queries = []
    
    # Simple regex to find named queries (comments followed by SQL)
    # Format: -- query_name: description
    # SELECT ...
    pattern = r'--\s*(\w+):\s*([^\n]*)\n([^;]+;)'
    
    for match in re.finditer(pattern, content, re.MULTILINE | re.DOTALL):
        query_name = match.group(1)
        description = match.group(2).strip()
        sql_text = match.group(3).strip()
        
        # Extract parameters (simple heuristic looking for :param syntax)
        params = re.findall(r':(\w+)', sql_text)
        
        queries.append({
            "name": query_name,
            "description": description,
            "params": list(set(params)),  # Remove duplicates
            "sql": sql_text
        })
    
    return {"queries": queries}

def generate_ontology_ids_header(ontology_ir, output_dir):
    """Generate ontology_ids.h with entity ID definitions"""
    header_content = """/*
 * Ontology Entity IDs - Generated by CNS AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 */

#ifndef ONTOLOGY_IDS_H
#define ONTOLOGY_IDS_H

#include <stdint.h>

/* Entity ID Type */
typedef uint32_t cns_entity_id_t;

/* Base ID ranges */
#define CNS_CLASS_ID_BASE    0x10000
#define CNS_PROPERTY_ID_BASE 0x20000
#define CNS_INSTANCE_ID_BASE 0x30000

/* Generated Class IDs */
"""
    
    class_id = 0x10000
    property_id = 0x20000
    
    # Generate class IDs from rules
    classes = set()
    properties = set()
    
    for rule in ontology_ir["rules"]:
        if rule["type"] == "subClassOf":
            classes.add(rule["sub"])
            classes.add(rule["super"])
        elif rule["type"] == "domain":
            properties.add(rule["property"])
            classes.add(rule["domain"])
    
    for cls in sorted(classes):
        header_content += f"#define CNS_{cls.upper()}_CLASS {class_id}\n"
        class_id += 1
    
    header_content += "\n/* Generated Property IDs */\n"
    for prop in sorted(properties):
        header_content += f"#define CNS_{prop.upper()}_PROPERTY {property_id}\n"
        property_id += 1
    
    header_content += "\n#endif /* ONTOLOGY_IDS_H */\n"
    
    with open(Path(output_dir) / "ontology_ids.h", "w") as f:
        f.write(header_content)

def generate_ontology_rules_header(ontology_ir, output_dir):
    """Generate ontology_rules.h with inference rules"""
    header_content = """/*
 * Ontology Inference Rules - Generated by CNS AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 */

#ifndef ONTOLOGY_RULES_H
#define ONTOLOGY_RULES_H

#include "ontology_ids.h"
#include <stdbool.h>

/* Inference Functions */
"""
    
    # Generate subclass checking functions
    for rule in ontology_ir["rules"]:
        if rule["type"] == "subClassOf":
            sub_upper = rule["sub"].upper()
            super_upper = rule["super"].upper()
            
            header_content += f"""
static inline bool is_{rule['sub']}_subclass_of_{rule['super']}(cns_entity_id_t entity_id) {{
    /* {rule['sub']} rdfs:subClassOf {rule['super']} */
    return entity_id == CNS_{sub_upper}_CLASS;
}}
"""
    
    # Generate domain checking functions
    for rule in ontology_ir["rules"]:
        if rule["type"] == "domain":
            prop_upper = rule["property"].upper()
            domain_upper = rule["domain"].upper()
            
            header_content += f"""
static inline bool check_{rule['property']}_domain(cns_entity_id_t subject_type) {{
    /* {rule['property']} rdfs:domain {rule['domain']} */
    return subject_type == CNS_{domain_upper}_CLASS;
}}
"""
    
    header_content += "\n#endif /* ONTOLOGY_RULES_H */\n"
    
    with open(Path(output_dir) / "ontology_rules.h", "w") as f:
        f.write(header_content)

def generate_shacl_validators_header(shacl_ir, output_dir):
    """Generate shacl_validators.h with validation functions"""
    header_content = """/*
 * SHACL Validators - Generated by CNS AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 */

#ifndef SHACL_VALIDATORS_H
#define SHACL_VALIDATORS_H

#include "ontology_ids.h"
#include <stdbool.h>
#include <stddef.h>

/* Validation Result */
typedef struct {
    bool valid;
    const char* error_message;
    cns_entity_id_t focus_node;
} cns_validation_result_t;

/* Shape Validators */
"""
    
    for shape in shacl_ir["shapes"]:
        shape_name = shape["name"].lower()
        
        header_content += f"""
static inline cns_validation_result_t validate_{shape_name}(cns_entity_id_t focus_node, void* graph_data) {{
    cns_validation_result_t result = {{.valid = true, .error_message = NULL, .focus_node = focus_node}};
    
    /* Validate constraints for {shape['name']} */
"""
        
        for constraint in shape["constraints"]:
            if "minCount" in constraint:
                header_content += f"""    /* Check minCount for {constraint['path']} */
    if (count_property_values(focus_node, CNS_{constraint['path'].upper()}_PROPERTY, graph_data) < {constraint['minCount']}) {{
        result.valid = false;
        result.error_message = "Insufficient {constraint['path']} values";
        return result;
    }}
"""
            
            if "maxCount" in constraint:
                header_content += f"""    /* Check maxCount for {constraint['path']} */
    if (count_property_values(focus_node, CNS_{constraint['path'].upper()}_PROPERTY, graph_data) > {constraint['maxCount']}) {{
        result.valid = false;
        result.error_message = "Too many {constraint['path']} values";
        return result;
    }}
"""
        
        header_content += """    
    return result;
}
"""
    
    # Add utility function declaration
    header_content += """
/* Utility function - implement in your graph engine */
extern int count_property_values(cns_entity_id_t subject, cns_entity_id_t property, void* graph_data);

#endif /* SHACL_VALIDATORS_H */
"""
    
    with open(Path(output_dir) / "shacl_validators.h", "w") as f:
        f.write(header_content)

def generate_sql_queries_header(sql_ir, output_dir):
    """Generate sql_queries.h with query functions"""
    header_content = """/*
 * SQL Query Functions - Generated by CNS AOT Compiler
 * DO NOT EDIT - This file is automatically generated
 */

#ifndef SQL_QUERIES_H
#define SQL_QUERIES_H

#include <stdint.h>

/* Query parameter types */
typedef struct {
    const char* name;
    int type; /* 0=int, 1=string, 2=float */
    union {
        int int_val;
        const char* str_val;
        double float_val;
    } value;
} cns_query_param_t;

/* Query result structure */
typedef struct {
    int row_count;
    int column_count;
    char** data; /* Row-major format */
} cns_query_result_t;

/* Generated Query Functions */
"""
    
    for query in sql_ir["queries"]:
        query_name = query["name"]
        params = query["params"]
        
        # Generate parameter struct
        if params:
            header_content += f"""
/* Parameters for {query_name} */
typedef struct {{
"""
            for param in params:
                header_content += f"    const char* {param};\n"
            
            header_content += f"}} {query_name}_params_t;\n"
        
        # Generate query function
        param_type = f"{query_name}_params_t* params" if params else "void"
        
        header_content += f"""
/* Execute {query_name}: {query.get('description', '')} */
static inline cns_query_result_t execute_{query_name}({param_type}) {{
    /* SQL: {query['sql'].replace('*/', '* /')} */
    static const char query_text[] = "{query['sql'].replace('"', '\\"').replace('\n', '\\n')}";
    
    /* TODO: Implement actual SQL execution */
    cns_query_result_t result = {{0, 0, NULL}};
    return result;
}}
"""
    
    header_content += "\n#endif /* SQL_QUERIES_H */\n"
    
    with open(Path(output_dir) / "sql_queries.h", "w") as f:
        f.write(header_content)

def main():
    parser = argparse.ArgumentParser(description='CNS AOT Compiler')
    parser.add_argument('--ontologies', required=True, help='Directory containing TTL ontology files')
    parser.add_argument('--sql', required=True, help='Directory containing SQL files')
    parser.add_argument('--output', required=True, help='Output directory for generated headers')
    
    args = parser.parse_args()
    
    print("🚀 Starting CNS AOT Compiler...")
    
    # Create output directory
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Process Ontologies (OWL/SHACL)
    print("📖 Processing ontologies...")
    full_graph = Graph()
    
    ontology_dir = Path(args.ontologies)
    if ontology_dir.exists():
        for ttl_file in ontology_dir.glob("*.ttl"):
            print(f"    - Loading {ttl_file}")
            try:
                full_graph.parse(str(ttl_file), format="turtle")
            except Exception as e:
                print(f"    - Warning: Failed to parse {ttl_file}: {e}")
    
    ontology_ir = parse_ontology_rules(full_graph)
    shacl_ir = parse_shacl_shapes(full_graph)
    
    # 2. Process SQL Queries
    print("📊 Processing SQL queries...")
    sql_dir = Path(args.sql)
    sql_files = list(sql_dir.glob("*.sql")) if sql_dir.exists() else []
    
    sql_ir = {"queries": []}
    for sql_file in sql_files:
        file_ir = parse_sql_queries(sql_file)
        sql_ir["queries"].extend(file_ir["queries"])
    
    # 3. Generate C Code from IR
    print("🔧 Generating C headers...")
    
    generate_ontology_ids_header(ontology_ir, output_dir)
    generate_ontology_rules_header(ontology_ir, output_dir)
    generate_shacl_validators_header(shacl_ir, output_dir)
    generate_sql_queries_header(sql_ir, output_dir)
    
    print(f"✅ AOT Compilation Complete. Generated headers are in {output_dir}")
    print(f"    - Generated {len(ontology_ir['rules'])} ontology rules")
    print(f"    - Generated {len(shacl_ir['shapes'])} SHACL shapes")
    print(f"    - Generated {len(sql_ir['queries'])} SQL queries")

if __name__ == "__main__":
    main()