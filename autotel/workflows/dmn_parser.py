"""
Simple DMN Parser and Executor
Handles DMN decision tables for BPMN workflows
"""

import xml.etree.ElementTree as ET
from typing import Dict, Any, List, Optional
from dataclasses import dataclass

@dataclass
class DMNRule:
    """Represents a DMN decision rule"""
    rule_id: str
    description: str
    input_entries: List[str]
    output_entries: List[str]

@dataclass
class DMNDecision:
    """Represents a DMN decision"""
    decision_id: str
    name: str
    inputs: List[Dict[str, str]]  # [{"id": "input1", "label": "Input 1", "expression": "var_name"}]
    outputs: List[Dict[str, str]]  # [{"id": "output1", "label": "Output 1", "type": "string"}]
    rules: List[DMNRule]

class DMNParser:
    """Simple DMN parser for decision tables"""
    
    def __init__(self):
        self.decisions: Dict[str, DMNDecision] = {}
    
    def parse_dmn_xml(self, xml_content: str) -> Dict[str, DMNDecision]:
        """Parse DMN XML and extract decisions (robust to default/prefixed and http/https namespaces)"""
        root = ET.fromstring(xml_content)
        ns_variants = [
            {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'},
            {'dmn': 'https://www.omg.org/spec/DMN/20191111/MODEL/'},
            {'': 'http://www.omg.org/spec/DMN/20191111/MODEL/'},
            {'': 'https://www.omg.org/spec/DMN/20191111/MODEL/'},
        ]
        dmn_definitions = []
        for ns in ns_variants:
            if '' in ns:
                # Default namespace: use {namespace}tag
                ns_uri = ns['']
                dmn_definitions += root.findall(f'.//{{{ns_uri}}}definitions')
                if root.tag == f'{{{ns_uri}}}definitions':
                    dmn_definitions.append(root)
            else:
                dmn_definitions += root.findall('.//dmn:definitions', ns)
                if root.tag.endswith('definitions'):
                    dmn_definitions.append(root)
        for dmn_def in dmn_definitions:
            # Determine namespace for this element
            if dmn_def.tag.startswith('{'):
                ns_uri = dmn_def.tag.split('}')[0][1:]
                def tag(t): return f'{{{ns_uri}}}{t}'
            else:
                ns_uri = None
                def tag(t): return t
            decisions = dmn_def.findall(tag('decision'))
            for decision in decisions:
                decision_id = decision.get('id')
                name = decision.get('name', decision_id)
                decision_table = decision.find(tag('decisionTable'))
                if not decision_table:
                    continue
                # Parse inputs
                inputs = []
                for input_elem in decision_table.findall(tag('input')):
                    input_id = input_elem.get('id')
                    label = input_elem.get('label', input_id)
                    expression_elem = input_elem.find(tag('inputExpression'))
                    expression = expression_elem.find(tag('text')).text if expression_elem is not None else input_id
                    inputs.append({
                        'id': input_id,
                        'label': label,
                        'expression': expression
                    })
                # Parse outputs
                outputs = []
                for output_elem in decision_table.findall(tag('output')):
                    output_id = output_elem.get('id')
                    label = output_elem.get('label', output_id)
                    output_type = output_elem.get('typeRef', 'string')
                    outputs.append({
                        'id': output_id,
                        'label': label,
                        'type': output_type
                    })
                # Parse rules
                rules = []
                for rule_elem in decision_table.findall(tag('rule')):
                    rule_id = rule_elem.get('id')
                    description = rule_elem.find(tag('description'))
                    description_text = description.text if description is not None else ""
                    # Parse input entries
                    input_entries = []
                    for input_entry in rule_elem.findall(tag('inputEntry')):
                        text_elem = input_entry.find(tag('text'))
                        input_entries.append(text_elem.text if text_elem is not None else "-")
                    # Parse output entries
                    output_entries = []
                    for output_entry in rule_elem.findall(tag('outputEntry')):
                        text_elem = output_entry.find(tag('text'))
                        output_entries.append(text_elem.text if text_elem is not None else "")
                    rules.append(DMNRule(
                        rule_id=rule_id,
                        description=description_text,
                        input_entries=input_entries,
                        output_entries=output_entries
                    ))
                dmn_decision = DMNDecision(
                    decision_id=decision_id,
                    name=name,
                    inputs=inputs,
                    outputs=outputs,
                    rules=rules
                )
                self.decisions[decision_id] = dmn_decision
        return self.decisions

class DMNExecutor:
    """Execute DMN decisions"""
    
    def __init__(self, parser: DMNParser):
        self.parser = parser
    
    def execute_decision(self, decision_id: str, context: Dict[str, Any]) -> Dict[str, Any]:
        """Execute a DMN decision with given context"""
        decision = self.parser.decisions.get(decision_id)
        if not decision:
            raise ValueError(f"Decision '{decision_id}' not found")
        
        # Evaluate each rule
        for rule in decision.rules:
            if self._evaluate_rule(rule, decision.inputs, context):
                # Rule matched, return outputs
                result = {}
                for i, output in enumerate(decision.outputs):
                    if i < len(rule.output_entries):
                        output_value = rule.output_entries[i]
                        # Remove quotes if present
                        if output_value.startswith('"') and output_value.endswith('"'):
                            output_value = output_value[1:-1]
                        result[output['id']] = output_value
                
                return result
        
        # No rule matched
        return {}

    def _evaluate_rule(self, rule: DMNRule, inputs: List[Dict[str, str]], context: Dict[str, Any]) -> bool:
        """Evaluate if a rule matches the given context"""
        for i, input_def in enumerate(inputs):
            if i >= len(rule.input_entries):
                continue
            
            input_entry = rule.input_entries[i]
            if input_entry == "-":  # Don't care
                continue
            
            # Get the actual value from context
            var_name = input_def['expression']
            actual_value = context.get(var_name)
            
            if not self._evaluate_condition(input_entry, actual_value):
                return False
        
        return True
    
    def _evaluate_condition(self, condition: str, value: Any) -> bool:
        """Evaluate a DMN condition against a value"""
        if condition == "-":  # Don't care
            return True
        
        # Simple condition evaluation
        if condition.startswith('"') and condition.endswith('"'):
            # String literal
            expected = condition[1:-1]
            return str(value) == expected
        
        elif condition.startswith('>='):
            # Greater than or equal
            try:
                threshold = float(condition[3:])
                return float(value) >= threshold
            except (ValueError, TypeError):
                return False
        
        elif condition.startswith('<'):
            # Less than
            try:
                threshold = float(condition[1:])
                return float(value) < threshold
            except (ValueError, TypeError):
                return False
        
        elif condition.startswith('>'):
            # Greater than
            try:
                threshold = float(condition[1:])
                return float(value) > threshold
            except (ValueError, TypeError):
                return False
        
        elif condition.startswith('contains '):
            # Contains check
            search_term = condition[9:].strip('"')
            return search_term in str(value)
        
        else:
            # Direct comparison
            return str(value) == condition

# Global DMN parser instance
dmn_parser = DMNParser() 