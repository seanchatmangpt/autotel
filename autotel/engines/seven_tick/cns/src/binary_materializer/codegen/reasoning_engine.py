#!/usr/bin/env python3
"""
8H Cognitive Cycle Reasoning Engine
Implements the Eightfold Path reasoning methodology for AOT compilation
"""

import json
import time
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple, Set, Union
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
import threading
import queue
from concurrent.futures import ThreadPoolExecutor, as_completed

logger = logging.getLogger(__name__)

class EightfoldStage(Enum):
    """Eightfold Path stages for reasoning"""
    RIGHT_UNDERSTANDING = "right_understanding"
    RIGHT_THOUGHT = "right_thought"
    RIGHT_SPEECH = "right_speech"
    RIGHT_ACTION = "right_action"
    RIGHT_LIVELIHOOD = "right_livelihood"
    RIGHT_EFFORT = "right_effort"
    RIGHT_MINDFULNESS = "right_mindfulness"
    RIGHT_CONCENTRATION = "right_concentration"

class ReasoningType(Enum):
    """Types of reasoning operations"""
    DEDUCTIVE = "deductive"
    INDUCTIVE = "inductive"
    ABDUCTIVE = "abductive"
    ANALOGICAL = "analogical"
    CAUSAL = "causal"
    TEMPORAL = "temporal"
    SPATIAL = "spatial"
    MODAL = "modal"

@dataclass
class ReasoningStep:
    """Individual step in reasoning process"""
    id: str
    stage: EightfoldStage
    reasoning_type: ReasoningType
    input_facts: List[Dict[str, Any]]
    output_facts: List[Dict[str, Any]]
    rules_applied: List[str]
    confidence: float
    processing_time: float
    memory_used: int
    path_id: str
    step_index: int

@dataclass
class ReasoningCycle:
    """Complete 8H reasoning cycle"""
    id: str
    cycle_number: int
    start_time: float
    end_time: float
    steps: List[ReasoningStep] = field(default_factory=list)
    total_facts_processed: int = 0
    total_rules_applied: int = 0
    convergence_achieved: bool = False
    depth: int = 1
    complexity_score: float = 0.0
    eightfold_coverage: Dict[str, bool] = field(default_factory=dict)

@dataclass
class Proof:
    """Logical proof generated during reasoning"""
    id: str
    conclusion: Dict[str, Any]
    premises: List[Dict[str, Any]]
    reasoning_chain: List[str]
    proof_type: str  # "deductive", "constructive", "semantic"
    validity: bool
    soundness: bool
    completeness_estimate: float
    
@dataclass
class KnowledgeBase:
    """Knowledge base for reasoning"""
    facts: List[Dict[str, Any]] = field(default_factory=list)
    rules: List[Dict[str, Any]] = field(default_factory=list)
    axioms: List[Dict[str, Any]] = field(default_factory=list)
    derived_facts: List[Dict[str, Any]] = field(default_factory=list)
    fact_index: Dict[str, List[int]] = field(default_factory=dict)
    rule_index: Dict[str, List[int]] = field(default_factory=dict)

class ReasoningEngine:
    """8H Cognitive Cycle Reasoning Engine"""
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        self.config = config or self._default_config()
        self.knowledge_base = KnowledgeBase()
        self.reasoning_cycles: List[ReasoningCycle] = []
        self.proofs: List[Proof] = []
        self.current_cycle: Optional[ReasoningCycle] = None
        
        # Performance tracking
        self.stats = {
            'total_cycles': 0,
            'total_steps': 0,
            'total_facts_processed': 0,
            'total_rules_applied': 0,
            'average_cycle_time': 0.0,
            'convergence_rate': 0.0
        }
        
        # Thread pool for parallel reasoning
        if self.config['parallel_reasoning']:
            self.thread_pool = ThreadPoolExecutor(
                max_workers=self.config.get('max_threads', 4)
            )
        
        # Eightfold stage processors
        self.stage_processors = {
            EightfoldStage.RIGHT_UNDERSTANDING: self._process_understanding,
            EightfoldStage.RIGHT_THOUGHT: self._process_thought,
            EightfoldStage.RIGHT_SPEECH: self._process_speech,
            EightfoldStage.RIGHT_ACTION: self._process_action,
            EightfoldStage.RIGHT_LIVELIHOOD: self._process_livelihood,
            EightfoldStage.RIGHT_EFFORT: self._process_effort,
            EightfoldStage.RIGHT_MINDFULNESS: self._process_mindfulness,
            EightfoldStage.RIGHT_CONCENTRATION: self._process_concentration
        }
    
    def _default_config(self) -> Dict[str, Any]:
        """Default configuration for reasoning engine"""
        return {
            'max_cycles': 8,
            'parallel_reasoning': True,
            'proof_generation': True,
            'cognitive_model': '8H',
            'convergence_threshold': 0.95,
            'max_depth': 5,
            'fact_limit': 10000,
            'rule_limit': 1000,
            'timeout_seconds': 60,
            'enable_caching': True,
            'debug_mode': False
        }
    
    def reason(self, ontology: Dict[str, Any], constraints: Dict[str, Any]) -> Dict[str, Any]:
        """Execute 8H reasoning cycles on ontology and constraints"""
        logger.info("Starting 8H reasoning process")
        
        # Initialize knowledge base
        self._initialize_knowledge_base(ontology, constraints)
        
        # Execute reasoning cycles
        cycles = self._execute_reasoning_cycles()
        
        # Generate proofs if enabled
        proofs = []
        if self.config['proof_generation']:
            proofs = self._generate_proofs()
        
        # Compile results
        results = self._compile_reasoning_results(cycles, proofs)
        
        return results
    
    def _initialize_knowledge_base(self, ontology: Dict[str, Any], 
                                  constraints: Dict[str, Any]) -> None:
        """Initialize knowledge base from ontology and constraints"""
        logger.info("Initializing knowledge base")
        
        # Extract facts from ontology
        for cls in ontology.get('classes', []):
            self.knowledge_base.facts.append({
                'type': 'class',
                'uri': cls['uri'],
                'label': cls['label'],
                'properties': cls.get('properties', []),
                'parent_classes': cls.get('parent_classes', []),
                'eightfold_mapping': cls.get('eightfold_mapping')
            })
        
        for prop in ontology.get('properties', []):
            self.knowledge_base.facts.append({
                'type': 'property',
                'uri': prop['uri'],
                'label': prop['label'],
                'property_type': prop['type'],
                'domain': prop.get('domain'),
                'range': prop.get('range'),
                'characteristics': prop.get('characteristics', [])
            })
        
        # Extract rules from ontology
        for rule in ontology.get('rules', []):
            self.knowledge_base.rules.append({
                'id': rule['id'],
                'type': rule['type'],
                'antecedent': rule['antecedent'],
                'consequent': rule['consequent'],
                'confidence': rule.get('confidence', 1.0),
                'eightfold_stage': rule.get('eightfold_stage')
            })
        
        # Extract constraints as axioms
        for shape in constraints.get('shapes', []):
            self.knowledge_base.axioms.append({
                'type': 'shacl_shape',
                'target': shape['target'],
                'constraints': shape['constraints']
            })
        
        # Build indices
        self._build_knowledge_indices()
        
        logger.info(f"Knowledge base initialized: {len(self.knowledge_base.facts)} facts, "
                   f"{len(self.knowledge_base.rules)} rules, {len(self.knowledge_base.axioms)} axioms")
    
    def _build_knowledge_indices(self) -> None:
        """Build indices for efficient knowledge access"""
        # Fact index by type and URI
        for i, fact in enumerate(self.knowledge_base.facts):
            fact_type = fact.get('type')
            fact_uri = fact.get('uri')
            
            if fact_type:
                if fact_type not in self.knowledge_base.fact_index:
                    self.knowledge_base.fact_index[fact_type] = []
                self.knowledge_base.fact_index[fact_type].append(i)
            
            if fact_uri:
                if fact_uri not in self.knowledge_base.fact_index:
                    self.knowledge_base.fact_index[fact_uri] = []
                self.knowledge_base.fact_index[fact_uri].append(i)
        
        # Rule index by type and stage
        for i, rule in enumerate(self.knowledge_base.rules):
            rule_type = rule.get('type')
            stage = rule.get('eightfold_stage')
            
            if rule_type:
                if rule_type not in self.knowledge_base.rule_index:
                    self.knowledge_base.rule_index[rule_type] = []
                self.knowledge_base.rule_index[rule_type].append(i)
            
            if stage:
                if stage not in self.knowledge_base.rule_index:
                    self.knowledge_base.rule_index[stage] = []
                self.knowledge_base.rule_index[stage].append(i)
    
    def _execute_reasoning_cycles(self) -> List[ReasoningCycle]:
        """Execute multiple reasoning cycles until convergence"""
        cycles = []
        max_cycles = self.config['max_cycles']
        
        for cycle_num in range(max_cycles):
            logger.info(f"Starting reasoning cycle {cycle_num + 1}")
            
            cycle = self._execute_single_cycle(cycle_num)
            cycles.append(cycle)
            
            # Check convergence
            if cycle.convergence_achieved:
                logger.info(f"Convergence achieved at cycle {cycle_num + 1}")
                break
            
            # Update knowledge base with derived facts
            self.knowledge_base.derived_facts.extend(
                step.output_facts for step in cycle.steps
            )
        
        self.reasoning_cycles = cycles
        self._update_statistics()
        
        return cycles
    
    def _execute_single_cycle(self, cycle_num: int) -> ReasoningCycle:
        """Execute a single 8H reasoning cycle"""
        start_time = time.time()
        
        cycle = ReasoningCycle(
            id=f"cycle_{cycle_num}",
            cycle_number=cycle_num,
            start_time=start_time,
            end_time=0.0
        )
        
        self.current_cycle = cycle
        
        # Execute all 8 stages of the Eightfold Path
        for stage in EightfoldStage:
            step = self._execute_reasoning_step(cycle_num, len(cycle.steps), stage)
            if step:
                cycle.steps.append(step)
                cycle.total_facts_processed += len(step.output_facts)
                cycle.total_rules_applied += len(step.rules_applied)
        
        # Calculate cycle metrics
        cycle.end_time = time.time()
        cycle.depth = self._calculate_reasoning_depth(cycle)
        cycle.complexity_score = self._calculate_complexity_score(cycle)
        cycle.eightfold_coverage = self._calculate_eightfold_coverage(cycle)
        cycle.convergence_achieved = self._check_convergence(cycle)
        
        return cycle
    
    def _execute_reasoning_step(self, cycle_num: int, step_index: int, 
                               stage: EightfoldStage) -> Optional[ReasoningStep]:
        """Execute a single reasoning step for a specific Eightfold stage"""
        step_start = time.time()
        
        # Get relevant facts and rules for this stage
        input_facts = self._get_stage_facts(stage)
        applicable_rules = self._get_stage_rules(stage)
        
        if not input_facts and not applicable_rules:
            return None
        
        # Process the stage
        processor = self.stage_processors[stage]
        output_facts, rules_applied = processor(input_facts, applicable_rules)
        
        # Calculate step metrics
        step_time = time.time() - step_start
        
        step = ReasoningStep(
            id=f"step_{cycle_num}_{step_index}",
            stage=stage,
            reasoning_type=self._determine_reasoning_type(stage),
            input_facts=input_facts,
            output_facts=output_facts,
            rules_applied=rules_applied,
            confidence=self._calculate_step_confidence(output_facts, rules_applied),
            processing_time=step_time,
            memory_used=self._estimate_memory_usage(output_facts),
            path_id=f"path_{stage.value}",
            step_index=step_index
        )
        
        return step
    
    def _get_stage_facts(self, stage: EightfoldStage) -> List[Dict[str, Any]]:
        """Get facts relevant to a specific Eightfold stage"""
        stage_facts = []
        
        # Get facts based on Eightfold mapping
        for fact in self.knowledge_base.facts:
            eightfold_mapping = fact.get('eightfold_mapping')
            if eightfold_mapping and eightfold_mapping.get('stage') == stage.value:
                stage_facts.append(fact)
        
        # If no specific mapping, get facts based on stage semantics
        if not stage_facts:
            stage_facts = self._get_facts_by_stage_semantics(stage)
        
        return stage_facts[:100]  # Limit for performance
    
    def _get_stage_rules(self, stage: EightfoldStage) -> List[Dict[str, Any]]:
        """Get rules relevant to a specific Eightfold stage"""
        stage_key = stage.value
        rule_indices = self.knowledge_base.rule_index.get(stage_key, [])
        
        return [self.knowledge_base.rules[i] for i in rule_indices]
    
    def _get_facts_by_stage_semantics(self, stage: EightfoldStage) -> List[Dict[str, Any]]:
        """Get facts based on stage semantic meaning"""
        if stage == EightfoldStage.RIGHT_UNDERSTANDING:
            # Focus on definitional and taxonomic facts
            return [f for f in self.knowledge_base.facts 
                   if f.get('type') in ['class', 'taxonomy']]
        
        elif stage == EightfoldStage.RIGHT_THOUGHT:
            # Focus on planning and design facts
            return [f for f in self.knowledge_base.facts 
                   if 'design' in str(f).lower() or 'plan' in str(f).lower()]
        
        elif stage == EightfoldStage.RIGHT_SPEECH:
            # Focus on communication and interface facts
            return [f for f in self.knowledge_base.facts 
                   if f.get('type') == 'property']
        
        elif stage == EightfoldStage.RIGHT_ACTION:
            # Focus on operational and functional facts
            return [f for f in self.knowledge_base.facts 
                   if 'function' in str(f).lower() or 'operation' in str(f).lower()]
        
        else:
            # Default: return all facts
            return self.knowledge_base.facts[:50]
    
    def _determine_reasoning_type(self, stage: EightfoldStage) -> ReasoningType:
        """Determine the type of reasoning for a stage"""
        stage_reasoning_map = {
            EightfoldStage.RIGHT_UNDERSTANDING: ReasoningType.DEDUCTIVE,
            EightfoldStage.RIGHT_THOUGHT: ReasoningType.ABDUCTIVE,
            EightfoldStage.RIGHT_SPEECH: ReasoningType.ANALOGICAL,
            EightfoldStage.RIGHT_ACTION: ReasoningType.CAUSAL,
            EightfoldStage.RIGHT_LIVELIHOOD: ReasoningType.TEMPORAL,
            EightfoldStage.RIGHT_EFFORT: ReasoningType.INDUCTIVE,
            EightfoldStage.RIGHT_MINDFULNESS: ReasoningType.MODAL,
            EightfoldStage.RIGHT_CONCENTRATION: ReasoningType.SPATIAL
        }
        
        return stage_reasoning_map.get(stage, ReasoningType.DEDUCTIVE)
    
    # Stage processors
    def _process_understanding(self, facts: List[Dict[str, Any]], 
                             rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Understanding stage - definitional reasoning"""
        output_facts = []
        rules_applied = []
        
        # Apply taxonomic reasoning
        for fact in facts:
            if fact.get('type') == 'class':
                # Derive subclass relationships
                for parent in fact.get('parent_classes', []):
                    derived = {
                        'type': 'derived_relationship',
                        'relation': 'subClassOf',
                        'subject': fact['uri'],
                        'object': parent,
                        'stage': 'right_understanding'
                    }
                    output_facts.append(derived)
        
        # Apply deductive rules
        for rule in rules:
            if rule.get('type') == 'inference':
                # Apply rule if antecedent matches
                if self._matches_antecedent(facts, rule['antecedent']):
                    derived = self._apply_consequent(rule['consequent'])
                    output_facts.append(derived)
                    rules_applied.append(rule['id'])
        
        return output_facts, rules_applied
    
    def _process_thought(self, facts: List[Dict[str, Any]], 
                        rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Thought stage - planning reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate design implications
        for fact in facts:
            if 'design' in str(fact).lower():
                # Create planning facts
                derived = {
                    'type': 'planning_fact',
                    'based_on': fact['uri'],
                    'implication': 'requires_implementation',
                    'stage': 'right_thought'
                }
                output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_speech(self, facts: List[Dict[str, Any]], 
                       rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Speech stage - communication reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate interface implications
        for fact in facts:
            if fact.get('type') == 'property':
                # Create communication facts
                derived = {
                    'type': 'interface_fact',
                    'property': fact['uri'],
                    'communication_type': 'data_exchange',
                    'stage': 'right_speech'
                }
                output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_action(self, facts: List[Dict[str, Any]], 
                       rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Action stage - operational reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate operational implications
        for fact in facts:
            if 'function' in str(fact).lower():
                # Create action facts
                derived = {
                    'type': 'action_fact',
                    'operation': fact.get('uri'),
                    'execution_context': 'runtime',
                    'stage': 'right_action'
                }
                output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_livelihood(self, facts: List[Dict[str, Any]], 
                           rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Livelihood stage - sustainability reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate sustainability implications
        for fact in facts:
            derived = {
                'type': 'sustainability_fact',
                'resource': fact.get('uri'),
                'lifecycle': 'managed',
                'stage': 'right_livelihood'
            }
            output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_effort(self, facts: List[Dict[str, Any]], 
                       rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Effort stage - optimization reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate optimization implications
        for fact in facts:
            derived = {
                'type': 'optimization_fact',
                'target': fact.get('uri'),
                'optimization_type': 'performance',
                'stage': 'right_effort'
            }
            output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_mindfulness(self, facts: List[Dict[str, Any]], 
                            rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Mindfulness stage - monitoring reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate monitoring implications
        for fact in facts:
            derived = {
                'type': 'monitoring_fact',
                'observable': fact.get('uri'),
                'metric_type': 'behavioral',
                'stage': 'right_mindfulness'
            }
            output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _process_concentration(self, facts: List[Dict[str, Any]], 
                             rules: List[Dict[str, Any]]) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Process Right Concentration stage - integration reasoning"""
        output_facts = []
        rules_applied = []
        
        # Generate integration implications
        for fact in facts:
            derived = {
                'type': 'integration_fact',
                'component': fact.get('uri'),
                'integration_level': 'system',
                'stage': 'right_concentration'
            }
            output_facts.append(derived)
        
        return output_facts, rules_applied
    
    def _matches_antecedent(self, facts: List[Dict[str, Any]], 
                           antecedent: List[Dict[str, Any]]) -> bool:
        """Check if antecedent conditions match available facts"""
        # Simplified matching - would be more sophisticated in practice
        return len(antecedent) <= len(facts)
    
    def _apply_consequent(self, consequent: Dict[str, Any]) -> Dict[str, Any]:
        """Apply rule consequent to generate new fact"""
        return {
            'type': 'derived_fact',
            'content': consequent,
            'generated_by': 'rule_application'
        }
    
    def _calculate_step_confidence(self, output_facts: List[Dict[str, Any]], 
                                  rules_applied: List[str]) -> float:
        """Calculate confidence score for a reasoning step"""
        if not output_facts:
            return 0.0
        
        # Base confidence on number of rules applied and facts generated
        fact_score = min(len(output_facts) / 10.0, 1.0)
        rule_score = min(len(rules_applied) / 5.0, 1.0)
        
        return (fact_score + rule_score) / 2.0
    
    def _estimate_memory_usage(self, facts: List[Dict[str, Any]]) -> int:
        """Estimate memory usage for facts"""
        return sum(len(json.dumps(fact)) for fact in facts)
    
    def _calculate_reasoning_depth(self, cycle: ReasoningCycle) -> int:
        """Calculate the depth of reasoning in the cycle"""
        max_depth = 1
        
        for step in cycle.steps:
            # Count derivation chains
            depth = 1
            for fact in step.output_facts:
                if 'derived' in fact.get('type', ''):
                    depth += 1
            max_depth = max(max_depth, depth)
        
        return max_depth
    
    def _calculate_complexity_score(self, cycle: ReasoningCycle) -> float:
        """Calculate complexity score for the cycle"""
        if not cycle.steps:
            return 0.0
        
        # Factors: number of steps, facts processed, rules applied
        step_factor = len(cycle.steps) / 8.0  # Normalize to 8 stages
        fact_factor = cycle.total_facts_processed / 100.0
        rule_factor = cycle.total_rules_applied / 10.0
        
        return min((step_factor + fact_factor + rule_factor) / 3.0, 1.0)
    
    def _calculate_eightfold_coverage(self, cycle: ReasoningCycle) -> Dict[str, bool]:
        """Calculate which Eightfold stages were covered"""
        coverage = {}
        
        for stage in EightfoldStage:
            coverage[stage.value] = any(
                step.stage == stage for step in cycle.steps
            )
        
        return coverage
    
    def _check_convergence(self, cycle: ReasoningCycle) -> bool:
        """Check if reasoning has converged"""
        threshold = self.config['convergence_threshold']
        
        # Simple convergence check based on fact generation
        if cycle.total_facts_processed == 0:
            return True
        
        # Check if we're generating fewer new facts
        if len(self.reasoning_cycles) > 0:
            prev_cycle = self.reasoning_cycles[-1]
            reduction_ratio = cycle.total_facts_processed / max(prev_cycle.total_facts_processed, 1)
            return reduction_ratio < (1.0 - threshold)
        
        return False
    
    def _generate_proofs(self) -> List[Proof]:
        """Generate logical proofs from reasoning cycles"""
        proofs = []
        proof_id = 0
        
        for cycle in self.reasoning_cycles:
            for step in cycle.steps:
                for output_fact in step.output_facts:
                    if step.rules_applied:
                        proof = Proof(
                            id=f"proof_{proof_id}",
                            conclusion=output_fact,
                            premises=step.input_facts,
                            reasoning_chain=[f"step_{step.id}"] + step.rules_applied,
                            proof_type="deductive",
                            validity=True,
                            soundness=step.confidence > 0.5,
                            completeness_estimate=step.confidence
                        )
                        proofs.append(proof)
                        proof_id += 1
        
        self.proofs = proofs
        return proofs
    
    def _update_statistics(self) -> None:
        """Update reasoning statistics"""
        self.stats['total_cycles'] = len(self.reasoning_cycles)
        self.stats['total_steps'] = sum(len(cycle.steps) for cycle in self.reasoning_cycles)
        self.stats['total_facts_processed'] = sum(
            cycle.total_facts_processed for cycle in self.reasoning_cycles
        )
        self.stats['total_rules_applied'] = sum(
            cycle.total_rules_applied for cycle in self.reasoning_cycles
        )
        
        if self.reasoning_cycles:
            total_time = sum(
                cycle.end_time - cycle.start_time for cycle in self.reasoning_cycles
            )
            self.stats['average_cycle_time'] = total_time / len(self.reasoning_cycles)
            
            convergent_cycles = sum(
                1 for cycle in self.reasoning_cycles if cycle.convergence_achieved
            )
            self.stats['convergence_rate'] = convergent_cycles / len(self.reasoning_cycles)
    
    def _compile_reasoning_results(self, cycles: List[ReasoningCycle], 
                                  proofs: List[Proof]) -> Dict[str, Any]:
        """Compile final reasoning results"""
        return {
            'cycles': [self._serialize_cycle(cycle) for cycle in cycles],
            'proofs': [self._serialize_proof(proof) for proof in proofs],
            'depth': max((cycle.depth for cycle in cycles), default=1),
            'total_facts_derived': sum(len(step.output_facts) 
                                     for cycle in cycles 
                                     for step in cycle.steps),
            'convergence_achieved': any(cycle.convergence_achieved for cycle in cycles),
            'statistics': self.stats,
            'metadata': {
                'engine': '8H Cognitive Cycle Reasoning Engine',
                'version': '1.0.0',
                'timestamp': datetime.now().isoformat(),
                'config': self.config
            }
        }
    
    def _serialize_cycle(self, cycle: ReasoningCycle) -> Dict[str, Any]:
        """Serialize reasoning cycle to dictionary"""
        return {
            'id': cycle.id,
            'cycle_number': cycle.cycle_number,
            'start_time': cycle.start_time,
            'end_time': cycle.end_time,
            'duration': cycle.end_time - cycle.start_time,
            'steps': [self._serialize_step(step) for step in cycle.steps],
            'total_facts_processed': cycle.total_facts_processed,
            'total_rules_applied': cycle.total_rules_applied,
            'convergence_achieved': cycle.convergence_achieved,
            'depth': cycle.depth,
            'complexity_score': cycle.complexity_score,
            'eightfold_coverage': cycle.eightfold_coverage
        }
    
    def _serialize_step(self, step: ReasoningStep) -> Dict[str, Any]:
        """Serialize reasoning step to dictionary"""
        return {
            'id': step.id,
            'stage': step.stage.value,
            'reasoning_type': step.reasoning_type.value,
            'input_fact_count': len(step.input_facts),
            'output_fact_count': len(step.output_facts),
            'rules_applied': step.rules_applied,
            'confidence': step.confidence,
            'processing_time': step.processing_time,
            'memory_used': step.memory_used,
            'path_id': step.path_id,
            'step_index': step.step_index
        }
    
    def _serialize_proof(self, proof: Proof) -> Dict[str, Any]:
        """Serialize proof to dictionary"""
        return {
            'id': proof.id,
            'conclusion': proof.conclusion,
            'premises_count': len(proof.premises),
            'reasoning_chain': proof.reasoning_chain,
            'proof_type': proof.proof_type,
            'validity': proof.validity,
            'soundness': proof.soundness,
            'completeness_estimate': proof.completeness_estimate
        }


def main():
    """Test the reasoning engine"""
    import argparse
    
    parser = argparse.ArgumentParser(description="8H Reasoning Engine")
    parser.add_argument("--test", action="store_true", help="Run test reasoning")
    parser.add_argument("--cycles", type=int, default=3, help="Number of cycles")
    
    args = parser.parse_args()
    
    if args.test:
        # Test ontology
        test_ontology = {
            'classes': [
                {
                    'uri': 'http://example.org#Person',
                    'label': 'Person',
                    'properties': [{'uri': 'name', 'label': 'name'}],
                    'eightfold_mapping': {'stage': 'right_understanding'}
                }
            ],
            'properties': [
                {
                    'uri': 'http://example.org#name',
                    'label': 'name',
                    'type': 'DatatypeProperty'
                }
            ],
            'rules': [
                {
                    'id': 'rule_1',
                    'type': 'inference',
                    'antecedent': [{'subject': '?x', 'predicate': 'rdf:type', 'object': 'Person'}],
                    'consequent': {'subject': '?x', 'predicate': 'hasName', 'object': '?name'},
                    'eightfold_stage': 'right_understanding'
                }
            ]
        }
        
        test_constraints = {'shapes': []}
        
        # Run reasoning
        config = {'max_cycles': args.cycles}
        engine = ReasoningEngine(config)
        results = engine.reason(test_ontology, test_constraints)
        
        print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()