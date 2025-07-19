#!/usr/bin/env python3
"""
SPARQL AOT Compiler - 80/20 Implementation
=========================================

Implements the 20% of SPARQL features that handle 80% of use cases:
1. Basic triple patterns (?s ?p ?o, s ?p ?o, ?s p ?o, etc.)
2. Simple BGP (Basic Graph Pattern) joins
3. Type filters (?s rdf:type :Class)
4. Simple FILTER expressions (>, <, =)

This compiler transforms SPARQL queries into optimized C functions
that can execute within the 7-tick performance budget.
"""

import re
import json
import argparse
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from enum import Enum

class OpType(Enum):
    SCAN_TYPE = "SCAN_TYPE"
    SCAN_SUBJECT = "SCAN_SUBJECT"
    SCAN_PREDICATE = "SCAN_PREDICATE"
    SCAN_OBJECT = "SCAN_OBJECT"
    FILTER_GT = "FILTER_GT"
    FILTER_LT = "FILTER_LT"
    FILTER_EQ = "FILTER_EQ"
    HASH_JOIN = "HASH_JOIN"
    PROJECT = "PROJECT"
    LIMIT = "LIMIT"

@dataclass
class TriplePattern:
    subject: str
    predicate: str
    object: str
    
    def is_variable(self, term: str) -> bool:
        return term.startswith('?')
    
    def get_bound_terms(self) -> Dict[str, str]:
        """Get non-variable terms"""
        bound = {}
        if not self.is_variable(self.subject):
            bound['subject'] = self.subject
        if not self.is_variable(self.predicate):
            bound['predicate'] = self.predicate
        if not self.is_variable(self.object):
            bound['object'] = self.object
        return bound

@dataclass
class MicroOp:
    op: OpType
    params: Dict
    output_reg: int
    estimated_cycles: float

@dataclass
class QueryPlan:
    name: str
    patterns: List[TriplePattern]
    filters: List[Dict]
    operations: List[MicroOp]
    result_vars: List[str]

class SparqlAOTCompiler:
    def __init__(self):
        # Map common RDF vocabulary to IDs
        self.vocab_map = {
            'rdf:type': 'RDF_TYPE',
            'rdfs:label': 'RDFS_LABEL',
            'foaf:name': 'FOAF_NAME',
            'foaf:knows': 'FOAF_KNOWS',
            'dc:title': 'DC_TITLE',
            'dc:creator': 'DC_CREATOR',
            ':Person': 'PERSON_CLASS',
            ':Document': 'DOCUMENT_CLASS',
            ':Organization': 'ORGANIZATION_CLASS'
        }
        
        self.register_counter = 0
    
    def next_register(self) -> int:
        """Get next available register number"""
        reg = self.register_counter
        self.register_counter += 1
        return reg
    
    def parse_simple_sparql(self, sparql: str) -> QueryPlan:
        """Parse a simple SPARQL query (80/20 features only)"""
        lines = sparql.strip().split('\n')
        
        # Extract query name from comment
        query_name = "unnamed_query"
        for line in lines:
            if line.strip().startswith('# QUERY:'):
                query_name = line.strip().split(':', 1)[1].strip()
                break
        
        # Find SELECT and WHERE clauses
        select_vars = []
        patterns = []
        filters = []
        
        in_where = False
        
        for line in lines:
            line = line.strip()
            
            if line.startswith('SELECT'):
                # Extract variables
                select_part = line.replace('SELECT', '').strip()
                select_vars = [var.strip() for var in select_part.split() if var.startswith('?')]
            
            elif line.startswith('WHERE'):
                in_where = True
            
            elif in_where and line and not line.startswith('#'):
                if line.startswith('FILTER'):
                    # Parse simple filters
                    filter_match = re.search(r'FILTER\s*\(\s*\?(\w+)\s*([><=]+)\s*([^\)]+)\s*\)', line)
                    if filter_match:
                        var, op, value = filter_match.groups()
                        filters.append({
                            'variable': f'?{var}',
                            'operator': op,
                            'value': value.strip()
                        })
                
                elif '.' in line or line.endswith('}'):
                    # Parse triple pattern
                    triple_text = line.replace('.', '').replace('}', '').strip()
                    if triple_text:
                        parts = triple_text.split()
                        if len(parts) >= 3:
                            patterns.append(TriplePattern(
                                subject=parts[0],
                                predicate=parts[1], 
                                object=parts[2]
                            ))
        
        return QueryPlan(
            name=query_name,
            patterns=patterns,
            filters=filters,
            operations=[],
            result_vars=select_vars
        )
    
    def compile_to_micro_ops(self, plan: QueryPlan) -> QueryPlan:
        """Compile query plan to micro-operations"""
        self.register_counter = 0
        operations = []
        
        # Process each triple pattern
        for i, pattern in enumerate(plan.patterns):
            bound_terms = pattern.get_bound_terms()
            output_reg = self.next_register()
            
            # Choose optimal scan strategy based on bound terms
            if 'predicate' in bound_terms and bound_terms['predicate'] == 'rdf:type':
                # Type scan - most efficient for type queries
                type_class = bound_terms.get('object', 'UNKNOWN_CLASS')
                mapped_class = self.vocab_map.get(type_class, f'ID_{type_class.replace(":", "_")}')
                
                operations.append(MicroOp(
                    op=OpType.SCAN_TYPE,
                    params={
                        'type_id': mapped_class,
                        'subject_var': pattern.subject if pattern.is_variable(pattern.subject) else None
                    },
                    output_reg=output_reg,
                    estimated_cycles=1.5
                ))
            
            elif 'subject' in bound_terms:
                # Subject scan
                operations.append(MicroOp(
                    op=OpType.SCAN_SUBJECT,
                    params={
                        'subject_id': bound_terms['subject'],
                        'predicate_var': pattern.predicate if pattern.is_variable(pattern.predicate) else None,
                        'object_var': pattern.object if pattern.is_variable(pattern.object) else None
                    },
                    output_reg=output_reg,
                    estimated_cycles=2.0
                ))
            
            elif 'predicate' in bound_terms:
                # Predicate scan
                pred_id = self.vocab_map.get(bound_terms['predicate'], f'ID_{bound_terms["predicate"].replace(":", "_")}')
                operations.append(MicroOp(
                    op=OpType.SCAN_PREDICATE,
                    params={
                        'predicate_id': pred_id,
                        'subject_var': pattern.subject if pattern.is_variable(pattern.subject) else None,
                        'object_var': pattern.object if pattern.is_variable(pattern.object) else None
                    },
                    output_reg=output_reg,
                    estimated_cycles=2.5
                ))
            
            elif 'object' in bound_terms:
                # Object scan (least efficient, but needed for completeness)
                operations.append(MicroOp(
                    op=OpType.SCAN_OBJECT,
                    params={
                        'object_id': bound_terms['object'],
                        'subject_var': pattern.subject if pattern.is_variable(pattern.subject) else None,
                        'predicate_var': pattern.predicate if pattern.is_variable(pattern.predicate) else None
                    },
                    output_reg=output_reg,
                    estimated_cycles=3.0
                ))
        
        # Add joins for multiple patterns (simplified)
        if len(plan.patterns) > 1:
            join_reg = self.next_register()
            operations.append(MicroOp(
                op=OpType.HASH_JOIN,
                params={
                    'left_reg': 0,
                    'right_reg': 1,
                    'join_var': self._find_common_variable(plan.patterns[0], plan.patterns[1])
                },
                output_reg=join_reg,
                estimated_cycles=4.0
            ))
        
        # Add filters
        for filter_expr in plan.filters:
            filter_reg = self.next_register()
            op_type = {
                '>': OpType.FILTER_GT,
                '<': OpType.FILTER_LT,
                '=': OpType.FILTER_EQ
            }.get(filter_expr['operator'], OpType.FILTER_GT)
            
            operations.append(MicroOp(
                op=op_type,
                params={
                    'variable': filter_expr['variable'],
                    'value': filter_expr['value']
                },
                output_reg=filter_reg,
                estimated_cycles=0.8
            ))
        
        # Add projection
        if plan.result_vars:
            project_reg = self.next_register()
            operations.append(MicroOp(
                op=OpType.PROJECT,
                params={
                    'variables': plan.result_vars
                },
                output_reg=project_reg,
                estimated_cycles=1.2
            ))
        
        plan.operations = operations
        return plan
    
    def _find_common_variable(self, p1: TriplePattern, p2: TriplePattern) -> Optional[str]:
        """Find common variable between two patterns for joins"""
        vars1 = {p1.subject, p1.predicate, p1.object}
        vars2 = {p2.subject, p2.predicate, p2.object}
        
        common = vars1.intersection(vars2)
        for var in common:
            if var.startswith('?'):
                return var
        return None
    
    def generate_c_function(self, plan: QueryPlan) -> str:
        """Generate optimized C function from query plan"""
        func_name = plan.name
        
        # Calculate total estimated cycles
        total_cycles = sum(op.estimated_cycles for op in plan.operations)
        
        # Generate function header
        c_code = f'''/**
 * @brief COMPILED SPARQL QUERY: {func_name}
 * Estimated cycles: {total_cycles:.1f}
 * 7-tick compliant: {"✅ YES" if total_cycles <= 7.0 else "❌ NO"}
 */
static inline int {func_name}(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {{
    // Registers for intermediate results
'''
        
        # Declare registers
        max_reg = max((op.output_reg for op in plan.operations), default=0)
        for i in range(max_reg + 1):
            c_code += f'    uint32_t reg{i}_ids[S7T_SQL_MAX_ROWS];\n'
            c_code += f'    float reg{i}_values[S7T_SQL_MAX_ROWS];\n'
        
        c_code += '\n    // Execute micro-operation tape\n'
        
        # Generate operations
        current_count = 'count0'
        for i, op in enumerate(plan.operations):
            if op.op == OpType.SCAN_TYPE:
                c_code += f'''
    // OP: SCAN_TYPE - Find all subjects of type {op.params.get('type_id', 'UNKNOWN')}
    int count{i} = s7t_scan_by_type(engine, {op.params.get('type_id', 'UNKNOWN')}, 
                                    reg{op.output_reg}_ids, S7T_SQL_MAX_ROWS);
'''
                current_count = f'count{i}'
            
            elif op.op == OpType.SCAN_PREDICATE:
                c_code += f'''
    // OP: SCAN_PREDICATE - Find all triples with predicate {op.params.get('predicate_id', 'UNKNOWN')}
    int count{i} = s7t_scan_by_predicate(engine, {op.params.get('predicate_id', 'UNKNOWN')},
                                         reg{op.output_reg}_ids, S7T_SQL_MAX_ROWS);
'''
                current_count = f'count{i}'
            
            elif op.op == OpType.FILTER_GT:
                value = op.params.get('value', '0')
                c_code += f'''
    // OP: FILTER_GT - Filter values > {value}
    int count{i} = s7t_simd_filter_gt_f32(reg{op.output_reg-1}_values, {current_count}, 
                                          {value}f, reg{op.output_reg}_ids);
'''
                current_count = f'count{i}'
            
            elif op.op == OpType.HASH_JOIN:
                c_code += f'''
    // OP: HASH_JOIN - Join registers {op.params.get('left_reg', 0)} and {op.params.get('right_reg', 1)}
    int count{i} = s7t_hash_join(engine, 
                                 reg{op.params.get('left_reg', 0)}_ids, count{op.params.get('left_reg', 0)},
                                 reg{op.params.get('right_reg', 1)}_ids, count{op.params.get('right_reg', 1)},
                                 reg{op.output_reg}_ids);
'''
                current_count = f'count{i}'
            
            elif op.op == OpType.PROJECT:
                c_code += f'''
    // OP: PROJECT - Extract final result columns
    int final_count = ({current_count} < max_results) ? {current_count} : max_results;
    s7t_project_results(engine, reg{op.output_reg-1}_ids, final_count, results);
'''
        
        c_code += f'''
    return final_count;
}}
'''
        
        return c_code
    
    def generate_query_header(self, plans: List[QueryPlan]) -> str:
        """Generate complete header file with all compiled queries"""
        header = '''#ifndef SPARQL_QUERIES_AOT_H
#define SPARQL_QUERIES_AOT_H

#include "cns/engines/sparql.h"
#include "ontology_ids.h"

// Result structure for compiled queries
typedef struct {
    uint32_t subject_id;
    uint32_t predicate_id;
    uint32_t object_id;
    float value;
} QueryResult;

// Low-level kernel functions (to be implemented)
int s7t_scan_by_type(CNSSparqlEngine* engine, uint32_t type_id, uint32_t* results, int max_results);
int s7t_scan_by_predicate(CNSSparqlEngine* engine, uint32_t pred_id, uint32_t* results, int max_results);
int s7t_simd_filter_gt_f32(float* values, int count, float threshold, uint32_t* results);
int s7t_hash_join(CNSSparqlEngine* engine, uint32_t* left, int left_count, uint32_t* right, int right_count, uint32_t* results);
void s7t_project_results(CNSSparqlEngine* engine, uint32_t* ids, int count, QueryResult* results);

// Compiled query functions
'''
        
        # Generate all functions
        for plan in plans:
            header += self.generate_c_function(plan)
            header += '\n'
        
        # Generate dispatcher
        header += '''
// Query dispatcher
static inline int execute_compiled_sparql_query(const char* query_name, CNSSparqlEngine* engine, QueryResult* results, int max_results) {
'''
        
        for plan in plans:
            header += f'    if (strcmp(query_name, "{plan.name}") == 0) {{\n'
            header += f'        return {plan.name}(engine, results, max_results);\n'
            header += f'    }}\n'
        
        header += '''    return -1; // Query not found
}

#endif // SPARQL_QUERIES_AOT_H
'''
        
        return header

def main():
    parser = argparse.ArgumentParser(description='SPARQL AOT Compiler (80/20 Implementation)')
    parser.add_argument('input', help='Input SPARQL file')
    parser.add_argument('--output', '-o', default='sparql_queries.h', help='Output header file')
    parser.add_argument('--json', help='Output JSON plan file')
    
    args = parser.parse_args()
    
    compiler = SparqlAOTCompiler()
    
    try:
        # Read input file
        with open(args.input, 'r') as f:
            sparql_content = f.read()
        
        # Split into individual queries (separated by blank lines)
        query_texts = [q.strip() for q in sparql_content.split('\n\n') if q.strip()]
        
        plans = []
        for query_text in query_texts:
            if query_text and 'SELECT' in query_text:
                # Parse query
                plan = compiler.parse_simple_sparql(query_text)
                
                # Compile to micro-ops
                plan = compiler.compile_to_micro_ops(plan)
                
                plans.append(plan)
                
                print(f"✅ Compiled query: {plan.name}")
                total_cycles = sum(op.estimated_cycles for op in plan.operations)
                print(f"   Estimated cycles: {total_cycles:.1f}")
                print(f"   7-tick compliant: {'✅ YES' if total_cycles <= 7.0 else '❌ NO'}")
        
        # Generate output
        if plans:
            header_code = compiler.generate_query_header(plans)
            
            with open(args.output, 'w') as f:
                f.write(header_code)
            
            print(f"\n🎯 Generated {len(plans)} compiled queries to {args.output}")
            
            # Generate JSON plan if requested
            if args.json:
                json_plans = []
                for plan in plans:
                    json_plan = {
                        'query_name': plan.name,
                        'operations': [
                            {
                                'op': op.op.value,
                                'params': op.params,
                                'output_reg': op.output_reg,
                                'estimated_cycles': op.estimated_cycles
                            }
                            for op in plan.operations
                        ],
                        'total_cycles': sum(op.estimated_cycles for op in plan.operations),
                        'seven_tick_compliant': sum(op.estimated_cycles for op in plan.operations) <= 7.0
                    }
                    json_plans.append(json_plan)
                
                with open(args.json, 'w') as f:
                    json.dump(json_plans, f, indent=2)
                
                print(f"📊 Generated query plans to {args.json}")
        else:
            print("❌ No valid SPARQL queries found in input file")
            return 1
        
        return 0
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1

if __name__ == '__main__':
    exit(main())