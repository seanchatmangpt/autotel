#!/usr/bin/env python3
"""
Validation script for BPMN Service Task and Business Rule Task functionality.
Tests basic parsing and execution of the two task types.
Uses only library imports, not project modules.
"""

import xml.etree.ElementTree as ET
import json
import logging
from pathlib import Path
from typing import Dict, Any, Optional

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)


class BPMNValidator:
    """Validates BPMN files for service tasks and business rule tasks"""
    
    def __init__(self):
        self.bpmn_ns = {'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'}
        self.camunda_ns = {'camunda': 'http://camunda.org/schema/1.0/bpmn'}
        self.spiff_ns = {'spiffworkflow': 'http://spiffworkflow.org/bpmn/schema/1.0/core'}
    
    def validate_bpmn_file(self, bpmn_path: str) -> Dict[str, Any]:
        """Validate a BPMN file for service tasks and business rule tasks"""
        logger.info(f"Validating BPMN file: {bpmn_path}")
        
        try:
            tree = ET.parse(bpmn_path)
            root = tree.getroot()
            
            # Check for CDATA sections
            if self._has_cdata_sections(root):
                raise ValueError("CDATA sections are not allowed")
            
            # Find all processes
            processes = root.findall('.//bpmn:process', self.bpmn_ns)
            if not processes:
                raise ValueError("No BPMN processes found")
            
            results = {
                'file': bpmn_path,
                'valid': True,
                'processes': [],
                'service_tasks': [],
                'business_rule_tasks': [],
                'errors': []
            }
            
            for process in processes:
                process_id = process.get('id', 'unknown')
                process_name = process.get('name', 'unnamed')
                
                process_info = {
                    'id': process_id,
                    'name': process_name,
                    'executable': process.get('isExecutable', 'false') == 'true'
                }
                results['processes'].append(process_info)
                
                # Find service tasks
                service_tasks = process.findall('.//bpmn:serviceTask', self.bpmn_ns)
                for task in service_tasks:
                    task_info = self._extract_service_task_info(task)
                    task_info['process_id'] = process_id
                    results['service_tasks'].append(task_info)
                
                # Find business rule tasks
                business_rule_tasks = process.findall('.//bpmn:businessRuleTask', self.bpmn_ns)
                for task in business_rule_tasks:
                    task_info = self._extract_business_rule_task_info(task)
                    task_info['process_id'] = process_id
                    results['business_rule_tasks'].append(task_info)
            
            logger.info(f"✓ BPMN validation successful")
            logger.info(f"  Processes: {len(results['processes'])}")
            logger.info(f"  Service Tasks: {len(results['service_tasks'])}")
            logger.info(f"  Business Rule Tasks: {len(results['business_rule_tasks'])}")
            
            return results
            
        except Exception as e:
            logger.error(f"✗ BPMN validation failed: {e}")
            return {
                'file': bpmn_path,
                'valid': False,
                'error': str(e),
                'processes': [],
                'service_tasks': [],
                'business_rule_tasks': [],
                'errors': [str(e)]
            }
    
    def _has_cdata_sections(self, root: ET.Element) -> bool:
        """Check if XML contains CDATA sections"""
        for elem in root.iter():
            if elem.text and '<![CDATA[' in elem.text:
                return True
        return False
    
    def _extract_service_task_info(self, task: ET.Element) -> Dict[str, Any]:
        """Extract information from a service task"""
        task_info = {
            'id': task.get('id', 'unknown'),
            'name': task.get('name', 'unnamed'),
            'type': 'serviceTask',
            'service_operator': None,
            'result_variable': None
        }
        
        # Look for SpiffWorkflow service task operator
        service_operators = task.findall('.//spiffworkflow:serviceTaskOperator', self.spiff_ns)
        if service_operators:
            operator = service_operators[0]
            task_info['service_operator'] = operator.get('id', 'unknown')
            task_info['result_variable'] = operator.get('resultVariable')
        
        return task_info
    
    def _extract_business_rule_task_info(self, task: ET.Element) -> Dict[str, Any]:
        """Extract information from a business rule task"""
        task_info = {
            'id': task.get('id', 'unknown'),
            'name': task.get('name', 'unnamed'),
            'type': 'businessRuleTask',
            'decision_ref': None
        }
        
        # Look for Camunda decision reference
        decision_ref = task.get('{http://camunda.org/schema/1.0/bpmn}decisionRef')
        if decision_ref:
            task_info['decision_ref'] = decision_ref
        
        return task_info


class DMNValidator:
    """Validates DMN files for decision tables"""
    
    def __init__(self):
        self.dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
    
    def validate_dmn_file(self, dmn_path: str) -> Dict[str, Any]:
        """Validate a DMN file for decision tables"""
        logger.info(f"Validating DMN file: {dmn_path}")
        
        try:
            tree = ET.parse(dmn_path)
            root = tree.getroot()
            
            # Check for CDATA sections
            if self._has_cdata_sections(root):
                raise ValueError("CDATA sections are not allowed")
            
            # Find all decisions
            decisions = root.findall('.//dmn:decision', self.dmn_ns)
            if not decisions:
                raise ValueError("No DMN decisions found")
            
            results = {
                'file': dmn_path,
                'valid': True,
                'decisions': [],
                'errors': []
            }
            
            for decision in decisions:
                decision_info = self._extract_decision_info(decision)
                results['decisions'].append(decision_info)
            
            logger.info(f"✓ DMN validation successful")
            logger.info(f"  Decisions: {len(results['decisions'])}")
            
            return results
            
        except Exception as e:
            logger.error(f"✗ DMN validation failed: {e}")
            return {
                'file': dmn_path,
                'valid': False,
                'error': str(e),
                'decisions': [],
                'errors': [str(e)]
            }
    
    def _has_cdata_sections(self, root: ET.Element) -> bool:
        """Check if XML contains CDATA sections"""
        for elem in root.iter():
            if elem.text and '<![CDATA[' in elem.text:
                return True
        return False
    
    def _extract_decision_info(self, decision: ET.Element) -> Dict[str, Any]:
        """Extract information from a DMN decision"""
        decision_info = {
            'id': decision.get('id', 'unknown'),
            'name': decision.get('name', 'unnamed'),
            'decision_table': None
        }
        
        # Look for decision table
        decision_tables = decision.findall('.//dmn:decisionTable', self.dmn_ns)
        if decision_tables:
            table = decision_tables[0]
            table_info = {
                'id': table.get('id', 'unknown'),
                'hit_policy': table.get('hitPolicy', 'UNIQUE'),
                'inputs': [],
                'outputs': [],
                'rules': []
            }
            
            # Extract inputs
            inputs = table.findall('.//dmn:input', self.dmn_ns)
            for input_elem in inputs:
                input_info = {
                    'id': input_elem.get('id', 'unknown'),
                    'label': input_elem.get('label', ''),
                    'expression': self._get_input_expression(input_elem)
                }
                table_info['inputs'].append(input_info)
            
            # Extract outputs
            outputs = table.findall('.//dmn:output', self.dmn_ns)
            for output_elem in outputs:
                output_info = {
                    'id': output_elem.get('id', 'unknown'),
                    'label': output_elem.get('label', ''),
                    'type': output_elem.get('typeRef', 'string')
                }
                table_info['outputs'].append(output_info)
            
            # Extract rules
            rules = table.findall('.//dmn:rule', self.dmn_ns)
            for rule in rules:
                rule_info = {
                    'id': rule.get('id', 'unknown'),
                    'input_entries': [],
                    'output_entries': []
                }
                
                # Input entries
                input_entries = rule.findall('.//dmn:inputEntry', self.dmn_ns)
                for entry in input_entries:
                    rule_info['input_entries'].append(entry.text or '')
                
                # Output entries
                output_entries = rule.findall('.//dmn:outputEntry', self.dmn_ns)
                for entry in output_entries:
                    rule_info['output_entries'].append(entry.text or '')
                
                table_info['rules'].append(rule_info)
            
            decision_info['decision_table'] = table_info
        
        return decision_info
    
    def _get_input_expression(self, input_elem: ET.Element) -> Optional[str]:
        """Get the input expression text"""
        expressions = input_elem.findall('.//dmn:inputExpression', self.dmn_ns)
        if expressions:
            return expressions[0].text
        return None


def validate_workflow_files(bpmn_path: str, dmn_path: str) -> Dict[str, Any]:
    """Validate both BPMN and DMN files for a complete workflow"""
    logger.info("=== Validating Service Task and Business Rule Task Workflow ===")
    
    bpmn_validator = BPMNValidator()
    dmn_validator = DMNValidator()
    
    # Validate BPMN file
    bpmn_results = bpmn_validator.validate_bpmn_file(bpmn_path)
    
    # Validate DMN file
    dmn_results = dmn_validator.validate_dmn_file(dmn_path)
    
    # Cross-reference validation
    cross_references = []
    if bpmn_results['valid'] and dmn_results['valid']:
        # Check if business rule tasks reference valid decisions
        for task in bpmn_results['business_rule_tasks']:
            decision_ref = task.get('decision_ref')
            if decision_ref:
                decision_ids = [d['id'] for d in dmn_results['decisions']]
                if decision_ref in decision_ids:
                    cross_references.append({
                        'task_id': task['id'],
                        'decision_ref': decision_ref,
                        'status': 'VALID'
                    })
                else:
                    cross_references.append({
                        'task_id': task['id'],
                        'decision_ref': decision_ref,
                        'status': 'INVALID - Decision not found',
                        'available_decisions': decision_ids
                    })
    
    # Overall validation result
    overall_valid = (
        bpmn_results['valid'] and 
        dmn_results['valid'] and 
        all(ref['status'] == 'VALID' for ref in cross_references)
    )
    
    results = {
        'overall_valid': overall_valid,
        'bpmn': bpmn_results,
        'dmn': dmn_results,
        'cross_references': cross_references,
        'summary': {
            'service_tasks': len(bpmn_results['service_tasks']),
            'business_rule_tasks': len(bpmn_results['business_rule_tasks']),
            'decisions': len(dmn_results['decisions']),
            'valid_cross_references': len([ref for ref in cross_references if ref['status'] == 'VALID'])
        }
    }
    
    # Print summary
    logger.info("=== Validation Summary ===")
    logger.info(f"Overall Valid: {'✓' if overall_valid else '✗'}")
    logger.info(f"Service Tasks: {results['summary']['service_tasks']}")
    logger.info(f"Business Rule Tasks: {results['summary']['business_rule_tasks']}")
    logger.info(f"Decisions: {results['summary']['decisions']}")
    logger.info(f"Valid Cross-references: {results['summary']['valid_cross_references']}")
    
    if cross_references:
        logger.info("Cross-references:")
        for ref in cross_references:
            status_icon = "✓" if ref['status'] == 'VALID' else "✗"
            logger.info(f"  {status_icon} {ref['task_id']} -> {ref['decision_ref']}: {ref['status']}")
    
    return results


def main():
    """Main validation function"""
    # File paths
    bpmn_file = "bpmn/simple_service_and_decision.bpmn"
    dmn_file = "bpmn/simple_decision.dmn"
    
    # Check if files exist
    if not Path(bpmn_file).exists():
        logger.error(f"BPMN file not found: {bpmn_file}")
        return False
    
    if not Path(dmn_file).exists():
        logger.error(f"DMN file not found: {dmn_file}")
        return False
    
    # Run validation
    results = validate_workflow_files(bpmn_file, dmn_file)
    
    # Return success/failure
    return results['overall_valid']


if __name__ == "__main__":
    success = main()
    exit(0 if success else 1) 