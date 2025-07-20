#!/usr/bin/env python3
"""
SHACL Validator for AOT Compiler Pipeline
Validates constraints and generates validation code
"""

import re
import json
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple, Set
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum

logger = logging.getLogger(__name__)

class ConstraintType(Enum):
    """Types of SHACL constraints"""
    CARDINALITY = "cardinality"
    DATATYPE = "datatype"
    PATTERN = "pattern"
    VALUE_RANGE = "value_range"
    NODE_KIND = "node_kind"
    CLOSED = "closed"
    LOGICAL = "logical"
    PROPERTY_PAIR = "property_pair"
    QUALIFIED_VALUE = "qualified_value"
    SPARQL = "sparql"

@dataclass
class ValidationRule:
    """Represents a validation rule"""
    id: str
    type: ConstraintType
    target: str  # Class or property URI
    constraint: Dict[str, Any]
    severity: str = "violation"  # violation, warning, info
    message: Optional[str] = None
    code_template: Optional[str] = None
    performance_hint: Optional[str] = None

@dataclass
class ValidationResult:
    """Result of validation"""
    valid: bool
    violations: List[Dict[str, Any]] = field(default_factory=list)
    warnings: List[Dict[str, Any]] = field(default_factory=list)
    info: List[Dict[str, Any]] = field(default_factory=list)
    statistics: Dict[str, int] = field(default_factory=dict)

@dataclass
class OptimizedConstraint:
    """Optimized constraint for code generation"""
    original: ValidationRule
    optimization_type: str  # "inline", "lookup_table", "bitmap", "function"
    optimized_code: str
    estimated_cost: float  # Relative cost estimate
    memory_usage: int  # Bytes

class SHACLValidator:
    """SHACL Validator and Constraint Compiler"""
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        self.config = config or self._default_config()
        self.rules: List[ValidationRule] = []
        self.optimized_constraints: List[OptimizedConstraint] = []
        self.validation_cache: Dict[str, ValidationResult] = {}
        self.constraint_index: Dict[str, List[ValidationRule]] = {}
        
        # Optimization strategies
        self.optimization_strategies = {
            ConstraintType.CARDINALITY: self._optimize_cardinality,
            ConstraintType.DATATYPE: self._optimize_datatype,
            ConstraintType.PATTERN: self._optimize_pattern,
            ConstraintType.VALUE_RANGE: self._optimize_value_range,
            ConstraintType.NODE_KIND: self._optimize_node_kind
        }
    
    def _default_config(self) -> Dict[str, Any]:
        """Default configuration for SHACL validation"""
        return {
            'validation_level': 'strict',  # strict, normal, lenient
            'constraint_optimization': True,
            'report_format': 'detailed',  # detailed, summary, minimal
            'cache_enabled': True,
            'parallel_validation': True,
            'generate_validation_code': True,
            'inline_simple_constraints': True,
            'use_lookup_tables': True,
            'max_constraint_complexity': 100
        }
    
    def validate(self, ontology: Dict[str, Any], constraints: Dict[str, Any]) -> Dict[str, Any]:
        """Validate ontology against SHACL constraints"""
        logger.info("Validating SHACL constraints")
        
        # Extract validation rules
        self._extract_rules(constraints)
        
        # Build constraint index
        self._build_constraint_index()
        
        # Perform validation
        result = self._perform_validation(ontology)
        
        # Optimize constraints if enabled
        if self.config['constraint_optimization']:
            self._optimize_constraints()
        
        # Generate validation report
        report = self._generate_report(result)
        
        return report
    
    def _extract_rules(self, constraints: Dict[str, Any]) -> None:
        """Extract validation rules from SHACL constraints"""
        rule_id = 0
        
        # Extract shape-based rules
        for shape in constraints.get('shapes', []):
            target = shape.get('target')
            
            # Property constraints
            for prop_constraint in shape.get('constraints', {}).get('properties', []):
                rules = self._extract_property_rules(prop_constraint, target, rule_id)
                self.rules.extend(rules)
                rule_id += len(rules)
            
            # Node constraints
            node_constraints = shape.get('constraints', {}).get('node', {})
            if node_constraints:
                rules = self._extract_node_rules(node_constraints, target, rule_id)
                self.rules.extend(rules)
                rule_id += len(rules)
        
        # Extract direct rules
        for rule in constraints.get('rules', []):
            validation_rule = self._create_rule_from_constraint(rule, rule_id)
            if validation_rule:
                self.rules.append(validation_rule)
                rule_id += 1
        
        logger.info(f"Extracted {len(self.rules)} validation rules")
    
    def _extract_property_rules(self, prop_constraint: Dict[str, Any], 
                               target: str, base_id: int) -> List[ValidationRule]:
        """Extract rules from property constraints"""
        rules = []
        rule_id = base_id
        
        path = prop_constraint.get('path')
        if not path:
            return rules
        
        # Cardinality constraints
        if 'minCount' in prop_constraint:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.CARDINALITY,
                target=target,
                constraint={
                    'path': path,
                    'minCount': prop_constraint['minCount']
                },
                message=f"Property {path} must have at least {prop_constraint['minCount']} values"
            ))
            rule_id += 1
        
        if 'maxCount' in prop_constraint:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.CARDINALITY,
                target=target,
                constraint={
                    'path': path,
                    'maxCount': prop_constraint['maxCount']
                },
                message=f"Property {path} must have at most {prop_constraint['maxCount']} values"
            ))
            rule_id += 1
        
        # Datatype constraint
        if 'datatype' in prop_constraint:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.DATATYPE,
                target=target,
                constraint={
                    'path': path,
                    'datatype': prop_constraint['datatype']
                },
                message=f"Property {path} must have datatype {prop_constraint['datatype']}"
            ))
            rule_id += 1
        
        # Pattern constraint
        if 'pattern' in prop_constraint:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.PATTERN,
                target=target,
                constraint={
                    'path': path,
                    'pattern': prop_constraint['pattern'],
                    'flags': prop_constraint.get('flags', '')
                },
                message=f"Property {path} must match pattern {prop_constraint['pattern']}"
            ))
            rule_id += 1
        
        # Value range constraints
        if 'minInclusive' in prop_constraint or 'maxInclusive' in prop_constraint:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.VALUE_RANGE,
                target=target,
                constraint={
                    'path': path,
                    'minInclusive': prop_constraint.get('minInclusive'),
                    'maxInclusive': prop_constraint.get('maxInclusive'),
                    'minExclusive': prop_constraint.get('minExclusive'),
                    'maxExclusive': prop_constraint.get('maxExclusive')
                },
                message=f"Property {path} must be within specified range"
            ))
            rule_id += 1
        
        return rules
    
    def _extract_node_rules(self, node_constraints: Dict[str, Any], 
                           target: str, base_id: int) -> List[ValidationRule]:
        """Extract rules from node constraints"""
        rules = []
        rule_id = base_id
        
        # Node kind constraint
        if 'nodeKind' in node_constraints:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.NODE_KIND,
                target=target,
                constraint={
                    'nodeKind': node_constraints['nodeKind']
                },
                message=f"Node must be of kind {node_constraints['nodeKind']}"
            ))
            rule_id += 1
        
        # Closed constraint
        if 'closed' in node_constraints:
            rules.append(ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.CLOSED,
                target=target,
                constraint={
                    'closed': node_constraints['closed'],
                    'ignoredProperties': node_constraints.get('ignoredProperties', [])
                },
                message="Node must not have properties other than those explicitly allowed"
            ))
            rule_id += 1
        
        return rules
    
    def _create_rule_from_constraint(self, constraint: Dict[str, Any], 
                                   rule_id: int) -> Optional[ValidationRule]:
        """Create validation rule from generic constraint"""
        # Determine constraint type
        if 'property' in constraint and 'domain' in constraint:
            return ValidationRule(
                id=f"rule_{rule_id}",
                type=ConstraintType.PROPERTY_PAIR,
                target=constraint['property'],
                constraint={
                    'domain': constraint['domain'],
                    'range': constraint.get('range')
                },
                message=f"Property {constraint['property']} domain/range constraint"
            )
        
        return None
    
    def _build_constraint_index(self) -> None:
        """Build index for efficient constraint lookup"""
        self.constraint_index.clear()
        
        for rule in self.rules:
            target = rule.target
            if target not in self.constraint_index:
                self.constraint_index[target] = []
            self.constraint_index[target].append(rule)
        
        logger.info(f"Built constraint index with {len(self.constraint_index)} targets")
    
    def _perform_validation(self, ontology: Dict[str, Any]) -> ValidationResult:
        """Perform actual validation"""
        result = ValidationResult(valid=True)
        
        # Validate classes
        for cls in ontology.get('classes', []):
            class_uri = cls['uri']
            
            # Get applicable rules
            rules = self.constraint_index.get(class_uri, [])
            
            for rule in rules:
                validation = self._validate_rule(cls, rule)
                if not validation['valid']:
                    result.valid = False
                    
                    if rule.severity == "violation":
                        result.violations.append(validation)
                    elif rule.severity == "warning":
                        result.warnings.append(validation)
                    else:
                        result.info.append(validation)
        
        # Compile statistics
        result.statistics = {
            'total_rules': len(self.rules),
            'rules_checked': len(self.rules),
            'violations': len(result.violations),
            'warnings': len(result.warnings),
            'info': len(result.info)
        }
        
        return result
    
    def _validate_rule(self, node: Dict[str, Any], rule: ValidationRule) -> Dict[str, Any]:
        """Validate a single rule against a node"""
        validation = {
            'valid': True,
            'rule_id': rule.id,
            'rule_type': rule.type.value,
            'target': node.get('uri', 'unknown'),
            'message': rule.message
        }
        
        try:
            if rule.type == ConstraintType.CARDINALITY:
                validation['valid'] = self._validate_cardinality(node, rule.constraint)
            elif rule.type == ConstraintType.DATATYPE:
                validation['valid'] = self._validate_datatype(node, rule.constraint)
            elif rule.type == ConstraintType.PATTERN:
                validation['valid'] = self._validate_pattern(node, rule.constraint)
            elif rule.type == ConstraintType.VALUE_RANGE:
                validation['valid'] = self._validate_value_range(node, rule.constraint)
            elif rule.type == ConstraintType.NODE_KIND:
                validation['valid'] = self._validate_node_kind(node, rule.constraint)
            else:
                # Unsupported constraint type
                validation['valid'] = True
                validation['message'] = f"Constraint type {rule.type} not implemented"
                
        except Exception as e:
            validation['valid'] = False
            validation['error'] = str(e)
        
        return validation
    
    def _validate_cardinality(self, node: Dict[str, Any], constraint: Dict[str, Any]) -> bool:
        """Validate cardinality constraint"""
        path = constraint['path']
        
        # Count occurrences of property
        count = 0
        for prop in node.get('properties', []):
            if prop.get('uri') == path or prop.get('label') == path:
                count += 1
        
        # Check constraints
        if 'minCount' in constraint and count < constraint['minCount']:
            return False
        if 'maxCount' in constraint and count > constraint['maxCount']:
            return False
        
        return True
    
    def _validate_datatype(self, node: Dict[str, Any], constraint: Dict[str, Any]) -> bool:
        """Validate datatype constraint"""
        # This is a simplified validation
        # In practice, would check actual property values
        return True
    
    def _validate_pattern(self, node: Dict[str, Any], constraint: Dict[str, Any]) -> bool:
        """Validate pattern constraint"""
        pattern = constraint['pattern']
        flags = constraint.get('flags', '')
        
        # Compile regex
        regex_flags = 0
        if 'i' in flags:
            regex_flags |= re.IGNORECASE
        if 'm' in flags:
            regex_flags |= re.MULTILINE
        
        try:
            regex = re.compile(pattern, regex_flags)
            # Would check actual values here
            return True
        except re.error:
            return False
    
    def _validate_value_range(self, node: Dict[str, Any], constraint: Dict[str, Any]) -> bool:
        """Validate value range constraint"""
        # This is a simplified validation
        # In practice, would check actual property values
        return True
    
    def _validate_node_kind(self, node: Dict[str, Any], constraint: Dict[str, Any]) -> bool:
        """Validate node kind constraint"""
        required_kind = constraint['nodeKind']
        
        # Check if node matches required kind
        if required_kind == 'IRI':
            return node.get('uri', '').startswith('http')
        elif required_kind == 'BlankNode':
            return node.get('uri', '').startswith('_:')
        elif required_kind == 'Literal':
            return 'value' in node
        
        return True
    
    def _optimize_constraints(self) -> None:
        """Optimize constraints for code generation"""
        logger.info("Optimizing constraints for code generation")
        
        for rule in self.rules:
            if rule.type in self.optimization_strategies:
                optimizer = self.optimization_strategies[rule.type]
                optimized = optimizer(rule)
                if optimized:
                    self.optimized_constraints.append(optimized)
    
    def _optimize_cardinality(self, rule: ValidationRule) -> OptimizedConstraint:
        """Optimize cardinality constraints"""
        constraint = rule.constraint
        
        if 'minCount' in constraint and constraint['minCount'] == 1:
            # Required property - can use simple null check
            code = f"""
static inline bool validate_{rule.id}(const void* node) {{
    return ((const struct_t*)node)->{self._to_c_identifier(constraint['path'])} != NULL;
}}
"""
            return OptimizedConstraint(
                original=rule,
                optimization_type="inline",
                optimized_code=code,
                estimated_cost=1.0,
                memory_usage=0
            )
        
        # General cardinality check
        code = f"""
static bool validate_{rule.id}(const void* node) {{
    int count = count_property(node, "{constraint['path']}");
    """
        
        if 'minCount' in constraint:
            code += f"if (count < {constraint['minCount']}) return false;\n    "
        if 'maxCount' in constraint:
            code += f"if (count > {constraint['maxCount']}) return false;\n    "
        
        code += "return true;\n}"
        
        return OptimizedConstraint(
            original=rule,
            optimization_type="function",
            optimized_code=code,
            estimated_cost=5.0,
            memory_usage=0
        )
    
    def _optimize_datatype(self, rule: ValidationRule) -> OptimizedConstraint:
        """Optimize datatype constraints"""
        constraint = rule.constraint
        datatype = constraint['datatype']
        
        # Create type checking code
        type_checks = {
            'http://www.w3.org/2001/XMLSchema#integer': 'is_valid_integer',
            'http://www.w3.org/2001/XMLSchema#string': 'is_valid_string',
            'http://www.w3.org/2001/XMLSchema#boolean': 'is_valid_boolean',
            'http://www.w3.org/2001/XMLSchema#float': 'is_valid_float',
            'http://www.w3.org/2001/XMLSchema#double': 'is_valid_double'
        }
        
        check_func = type_checks.get(datatype, 'is_valid_type')
        
        code = f"""
static inline bool validate_{rule.id}(const void* value) {{
    return {check_func}(value, "{datatype}");
}}
"""
        
        return OptimizedConstraint(
            original=rule,
            optimization_type="inline",
            optimized_code=code,
            estimated_cost=2.0,
            memory_usage=0
        )
    
    def _optimize_pattern(self, rule: ValidationRule) -> OptimizedConstraint:
        """Optimize pattern constraints"""
        constraint = rule.constraint
        pattern = constraint['pattern']
        
        # For simple patterns, generate specialized code
        if self._is_simple_pattern(pattern):
            code = self._generate_simple_pattern_code(rule.id, pattern)
            optimization_type = "inline"
            cost = 3.0
        else:
            # For complex patterns, use regex engine
            code = f"""
static regex_t regex_{rule.id};
static bool regex_{rule.id}_compiled = false;

static bool validate_{rule.id}(const char* value) {{
    if (!regex_{rule.id}_compiled) {{
        regcomp(&regex_{rule.id}, "{pattern}", REG_EXTENDED);
        regex_{rule.id}_compiled = true;
    }}
    return regexec(&regex_{rule.id}, value, 0, NULL, 0) == 0;
}}
"""
            optimization_type = "function"
            cost = 10.0
        
        return OptimizedConstraint(
            original=rule,
            optimization_type=optimization_type,
            optimized_code=code,
            estimated_cost=cost,
            memory_usage=256 if optimization_type == "function" else 0
        )
    
    def _optimize_value_range(self, rule: ValidationRule) -> OptimizedConstraint:
        """Optimize value range constraints"""
        constraint = rule.constraint
        
        conditions = []
        if 'minInclusive' in constraint:
            conditions.append(f"value >= {constraint['minInclusive']}")
        if 'maxInclusive' in constraint:
            conditions.append(f"value <= {constraint['maxInclusive']}")
        if 'minExclusive' in constraint:
            conditions.append(f"value > {constraint['minExclusive']}")
        if 'maxExclusive' in constraint:
            conditions.append(f"value < {constraint['maxExclusive']}")
        
        condition_str = " && ".join(conditions)
        
        code = f"""
static inline bool validate_{rule.id}(double value) {{
    return {condition_str};
}}
"""
        
        return OptimizedConstraint(
            original=rule,
            optimization_type="inline",
            optimized_code=code,
            estimated_cost=1.0,
            memory_usage=0
        )
    
    def _optimize_node_kind(self, rule: ValidationRule) -> OptimizedConstraint:
        """Optimize node kind constraints"""
        constraint = rule.constraint
        node_kind = constraint['nodeKind']
        
        kind_checks = {
            'IRI': 'is_iri_node',
            'BlankNode': 'is_blank_node',
            'Literal': 'is_literal_node'
        }
        
        check_func = kind_checks.get(node_kind, 'check_node_kind')
        
        code = f"""
static inline bool validate_{rule.id}(const void* node) {{
    return {check_func}(node);
}}
"""
        
        return OptimizedConstraint(
            original=rule,
            optimization_type="inline",
            optimized_code=code,
            estimated_cost=1.0,
            memory_usage=0
        )
    
    def _generate_report(self, result: ValidationResult) -> Dict[str, Any]:
        """Generate validation report"""
        report = {
            'valid': result.valid,
            'timestamp': datetime.now().isoformat(),
            'statistics': result.statistics,
            'constraints': {
                'total': len(self.rules),
                'optimized': len(self.optimized_constraints),
                'by_type': self._count_by_type()
            }
        }
        
        if self.config['report_format'] == 'detailed':
            report['violations'] = result.violations
            report['warnings'] = result.warnings
            report['info'] = result.info
            report['optimization_summary'] = self._generate_optimization_summary()
        elif self.config['report_format'] == 'summary':
            report['violation_count'] = len(result.violations)
            report['warning_count'] = len(result.warnings)
            report['info_count'] = len(result.info)
        
        return report
    
    def _count_by_type(self) -> Dict[str, int]:
        """Count constraints by type"""
        counts = {}
        for rule in self.rules:
            type_name = rule.type.value
            counts[type_name] = counts.get(type_name, 0) + 1
        return counts
    
    def _generate_optimization_summary(self) -> Dict[str, Any]:
        """Generate optimization summary"""
        summary = {
            'total_optimized': len(self.optimized_constraints),
            'optimization_types': {},
            'estimated_total_cost': 0.0,
            'estimated_memory_usage': 0
        }
        
        for opt in self.optimized_constraints:
            opt_type = opt.optimization_type
            summary['optimization_types'][opt_type] = \
                summary['optimization_types'].get(opt_type, 0) + 1
            summary['estimated_total_cost'] += opt.estimated_cost
            summary['estimated_memory_usage'] += opt.memory_usage
        
        return summary
    
    def generate_validation_code(self) -> str:
        """Generate complete validation code"""
        if not self.config['generate_validation_code']:
            return ""
        
        code_parts = [
            "/* Generated SHACL Validation Code */",
            "#include <stdbool.h>",
            "#include <stdint.h>",
            "#include <string.h>",
            "#include <regex.h>",
            "",
            "/* Forward declarations */",
            "static int count_property(const void* node, const char* property);",
            "static bool is_valid_integer(const void* value, const char* datatype);",
            "static bool is_valid_string(const void* value, const char* datatype);",
            "static bool is_valid_boolean(const void* value, const char* datatype);",
            "static bool is_valid_float(const void* value, const char* datatype);",
            "static bool is_valid_double(const void* value, const char* datatype);",
            "static bool is_iri_node(const void* node);",
            "static bool is_blank_node(const void* node);",
            "static bool is_literal_node(const void* node);",
            "",
            "/* Optimized validation functions */",
        ]
        
        # Add optimized constraint functions
        for opt in self.optimized_constraints:
            code_parts.append(opt.optimized_code)
        
        # Add master validation function
        code_parts.append(self._generate_master_validator())
        
        return '\n'.join(code_parts)
    
    def _generate_master_validator(self) -> str:
        """Generate master validation function"""
        code = """
/* Master validation function */
typedef struct validation_result {
    bool valid;
    int violation_count;
    int warning_count;
    char* messages[100];
    int message_count;
} validation_result_t;

validation_result_t validate_all(const void* data) {
    validation_result_t result = {.valid = true, .violation_count = 0, .warning_count = 0, .message_count = 0};
    
"""
        
        # Add calls to all validation functions
        for opt in self.optimized_constraints:
            rule_id = opt.original.id
            code += f"""    if (!validate_{rule_id}(data)) {{
        result.valid = false;
        result.violation_count++;
        if (result.message_count < 100) {{
            result.messages[result.message_count++] = "{opt.original.message}";
        }}
    }}
    
"""
        
        code += """    return result;
}
"""
        
        return code
    
    # Helper methods
    def _to_c_identifier(self, name: str) -> str:
        """Convert to C identifier"""
        return re.sub(r'[^a-zA-Z0-9_]', '_', name).lower()
    
    def _is_simple_pattern(self, pattern: str) -> bool:
        """Check if pattern is simple enough for specialized code"""
        # Simple patterns: starts with, ends with, contains
        simple_patterns = [r'^\^', r'\$$', r'^[a-zA-Z0-9_]+$']
        return any(re.match(sp, pattern) for sp in simple_patterns)
    
    def _generate_simple_pattern_code(self, rule_id: str, pattern: str) -> str:
        """Generate specialized code for simple patterns"""
        if pattern.startswith('^'):
            prefix = pattern[1:].replace('\\', '\\\\')
            return f"""
static inline bool validate_{rule_id}(const char* value) {{
    return strncmp(value, "{prefix}", {len(prefix)}) == 0;
}}
"""
        elif pattern.endswith('$'):
            suffix = pattern[:-1].replace('\\', '\\\\')
            return f"""
static inline bool validate_{rule_id}(const char* value) {{
    size_t val_len = strlen(value);
    size_t suffix_len = {len(suffix)};
    return val_len >= suffix_len && strcmp(value + val_len - suffix_len, "{suffix}") == 0;
}}
"""
        else:
            return f"""
static inline bool validate_{rule_id}(const char* value) {{
    return strstr(value, "{pattern}") != NULL;
}}
"""


def main():
    """Test the SHACL validator"""
    import argparse
    
    parser = argparse.ArgumentParser(description="SHACL Validator")
    parser.add_argument("--test", action="store_true", help="Run test validation")
    parser.add_argument("--generate-code", action="store_true", help="Generate validation code")
    
    args = parser.parse_args()
    
    if args.test:
        # Test constraints
        test_constraints = {
            'shapes': [{
                'target': 'http://example.org#Person',
                'constraints': {
                    'properties': [
                        {
                            'path': 'name',
                            'minCount': 1,
                            'maxCount': 1,
                            'datatype': 'http://www.w3.org/2001/XMLSchema#string'
                        },
                        {
                            'path': 'age',
                            'datatype': 'http://www.w3.org/2001/XMLSchema#integer',
                            'minInclusive': 0,
                            'maxInclusive': 150
                        }
                    ]
                }
            }]
        }
        
        # Test ontology
        test_ontology = {
            'classes': [{
                'uri': 'http://example.org#Person',
                'label': 'Person',
                'properties': [
                    {'uri': 'name', 'label': 'name'},
                    {'uri': 'age', 'label': 'age'}
                ]
            }]
        }
        
        # Validate
        validator = SHACLValidator()
        report = validator.validate(test_ontology, test_constraints)
        
        print(json.dumps(report, indent=2))
        
        if args.generate_code:
            code = validator.generate_validation_code()
            print("\nGenerated Validation Code:")
            print(code)


if __name__ == "__main__":
    main()