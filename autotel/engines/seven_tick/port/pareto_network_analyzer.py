#!/usr/bin/env python3
"""
Pareto Network Analyzer - Correct 80/20 Implementation
Replaces naive "5 Whys" with empirical causal network analysis
"""

import numpy as np
import networkx as nx
from dataclasses import dataclass, field
from typing import Dict, List, Set, Tuple, Optional
from collections import defaultdict
import json

@dataclass
class CausalNode:
    """Represents a node in the causal network"""
    id: str
    name: str
    impact: float = 0.0
    fix_cost: float = 1.0
    measurement_confidence: float = 0.0
    metadata: dict = field(default_factory=dict)

@dataclass
class Intervention:
    """Represents a potential intervention"""
    nodes: Set[str]
    total_cost: float
    expected_impact: float
    confidence: float
    
    @property
    def roi(self) -> float:
        """Return on Investment"""
        return self.expected_impact / self.total_cost if self.total_cost > 0 else float('inf')

class ParetoNetworkAnalyzer:
    """
    Implements correct 80/20 analysis for complex systems
    Replaces "5 Whys" with empirical causal network analysis
    """
    
    def __init__(self):
        self.causal_network = nx.DiGraph()
        self.measurements = defaultdict(list)
        self.current_pareto_set = set()
        self.history = []
        
    def add_causal_relationship(self, cause: str, effect: str, 
                               strength: float = 1.0, 
                               relationship_type: str = "causes"):
        """Add a causal relationship to the network"""
        # Ensure nodes exist before adding edge
        for node in [cause, effect]:
            if node not in self.causal_network.nodes:
                self.causal_network.add_node(node)
            if 'data' not in self.causal_network.nodes[node]:
                self.causal_network.nodes[node]['data'] = CausalNode(
                    id=node, 
                    name=node
                )
        
        self.causal_network.add_edge(
            cause, effect, 
            weight=strength,
            type=relationship_type
        )
    
    def measure_impact(self, node: str, measurement: float, 
                      confidence: float = 1.0):
        """Record an impact measurement for a node"""
        # Ensure node exists
        if node not in self.causal_network.nodes:
            self.causal_network.add_node(node)
            self.causal_network.nodes[node]['data'] = CausalNode(
                id=node, 
                name=node
            )
        
        self.measurements[node].append({
            'value': measurement,
            'confidence': confidence,
            'timestamp': len(self.history)
        })
        
        # Update node data
        node_data = self.causal_network.nodes[node]['data']
        node_data.impact = self._calculate_weighted_average(
            self.measurements[node]
        )
        node_data.measurement_confidence = confidence
    
    def _calculate_weighted_average(self, measurements: List[dict]) -> float:
        """Calculate confidence-weighted average of measurements"""
        if not measurements:
            return 0.0
            
        total_weight = sum(m['confidence'] for m in measurements)
        if total_weight == 0:
            return 0.0
            
        weighted_sum = sum(
            m['value'] * m['confidence'] for m in measurements
        )
        return weighted_sum / total_weight
    
    def calculate_network_effects(self, intervention_nodes: Set[str]) -> float:
        """
        Calculate total impact including network effects
        This is what "5 Whys" completely misses
        """
        # Direct impacts
        direct_impact = sum(
            self.causal_network.nodes[node]['data'].impact 
            for node in intervention_nodes
            if node in self.causal_network
        )
        
        # Network effects (propagated impact)
        affected_nodes = set()
        for start_node in intervention_nodes:
            # Find all downstream nodes
            descendants = nx.descendants(self.causal_network, start_node)
            affected_nodes.update(descendants)
        
        # Calculate propagated impact
        propagated_impact = 0.0
        for node in affected_nodes:
            if node not in intervention_nodes:  # Avoid double counting
                # Calculate path-weighted impact
                max_path_weight = 0.0
                for start_node in intervention_nodes:
                    if nx.has_path(self.causal_network, start_node, node):
                        paths = nx.all_simple_paths(
                            self.causal_network, start_node, node
                        )
                        for path in paths:
                            path_weight = 1.0
                            for i in range(len(path) - 1):
                                edge_weight = self.causal_network[path[i]][path[i+1]].get('weight', 1.0)
                                path_weight *= edge_weight
                            max_path_weight = max(max_path_weight, path_weight)
                
                node_impact = self.causal_network.nodes[node]['data'].impact
                propagated_impact += node_impact * max_path_weight
        
        # Feedback loops bonus (emergent effects)
        feedback_multiplier = self._calculate_feedback_effects(intervention_nodes)
        
        total_impact = (direct_impact + propagated_impact) * feedback_multiplier
        return total_impact
    
    def _calculate_feedback_effects(self, nodes: Set[str]) -> float:
        """Calculate amplification from feedback loops"""
        feedback_strength = 1.0
        
        # Find cycles that include intervention nodes
        try:
            cycles = nx.simple_cycles(self.causal_network)
            for cycle in cycles:
                cycle_nodes = set(cycle)
                if cycle_nodes.intersection(nodes):
                    # Calculate cycle strength
                    cycle_weight = 1.0
                    for i in range(len(cycle)):
                        j = (i + 1) % len(cycle)
                        edge_weight = self.causal_network[cycle[i]][cycle[j]].get('weight', 1.0)
                        cycle_weight *= edge_weight
                    
                    # Feedback loops amplify impact
                    if cycle_weight > 0:
                        feedback_strength += cycle_weight * 0.1
        except:
            pass  # Graph might not have cycles
            
        return min(feedback_strength, 2.0)  # Cap at 2x amplification
    
    def find_pareto_interventions(self, budget: float = float('inf'), 
                                 min_roi: float = 0.0) -> List[Intervention]:
        """
        Find Pareto-optimal interventions
        This is the correct way to identify the vital few
        """
        nodes = list(self.causal_network.nodes())
        interventions = []
        
        # Generate intervention combinations (with pruning)
        from itertools import combinations
        
        for r in range(1, min(len(nodes), 6)):  # Limit complexity
            for node_combo in combinations(nodes, r):
                node_set = set(node_combo)
                
                # Calculate cost
                total_cost = sum(
                    self.causal_network.nodes[n]['data'].fix_cost 
                    for n in node_set
                )
                
                if total_cost > budget:
                    continue
                
                # Calculate expected impact with network effects
                expected_impact = self.calculate_network_effects(node_set)
                
                # Calculate confidence
                confidence = np.mean([
                    self.causal_network.nodes[n]['data'].measurement_confidence
                    for n in node_set
                ])
                
                intervention = Intervention(
                    nodes=node_set,
                    total_cost=total_cost,
                    expected_impact=expected_impact,
                    confidence=confidence
                )
                
                if intervention.roi >= min_roi:
                    interventions.append(intervention)
        
        # Find Pareto frontier
        pareto_interventions = self._find_pareto_frontier(interventions)
        
        # Sort by ROI
        pareto_interventions.sort(key=lambda x: x.roi, reverse=True)
        
        return pareto_interventions
    
    def _find_pareto_frontier(self, interventions: List[Intervention]) -> List[Intervention]:
        """Find interventions on the Pareto frontier"""
        pareto_frontier = []
        
        for candidate in interventions:
            is_dominated = False
            
            for other in interventions:
                if other == candidate:
                    continue
                
                # Check if other dominates candidate
                if (other.expected_impact >= candidate.expected_impact and
                    other.total_cost <= candidate.total_cost and
                    (other.expected_impact > candidate.expected_impact or
                     other.total_cost < candidate.total_cost)):
                    is_dominated = True
                    break
            
            if not is_dominated:
                pareto_frontier.append(candidate)
        
        return pareto_frontier
    
    def calculate_dynamic_80_20(self) -> Dict[str, float]:
        """
        Calculate which nodes currently represent 80% of impact
        This changes over time, unlike static "5 Whys" root causes
        """
        # Get all nodes with their impacts
        node_impacts = []
        for node in self.causal_network.nodes():
            node_data = self.causal_network.nodes[node]['data']
            if node_data.impact > 0:
                node_impacts.append((node, node_data.impact))
        
        # Sort by impact
        node_impacts.sort(key=lambda x: x[1], reverse=True)
        
        # Find 80% threshold
        total_impact = sum(impact for _, impact in node_impacts)
        cumulative_impact = 0.0
        pareto_nodes = []
        
        for node, impact in node_impacts:
            cumulative_impact += impact
            pareto_nodes.append(node)
            
            if cumulative_impact >= 0.8 * total_impact:
                break
        
        # Update current Pareto set
        self.current_pareto_set = set(pareto_nodes)
        
        # Calculate statistics
        stats = {
            'total_nodes': len(self.causal_network.nodes()),
            'pareto_nodes': len(pareto_nodes),
            'pareto_percentage': len(pareto_nodes) / len(self.causal_network.nodes()) * 100,
            'impact_percentage': cumulative_impact / total_impact * 100 if total_impact > 0 else 0,
            'nodes': pareto_nodes
        }
        
        return stats
    
    def simulate_intervention(self, intervention: Intervention, steps: int = 10) -> List[float]:
        """
        Simulate the effects of an intervention over time
        Shows why single-point "root cause" analysis fails
        """
        impacts = []
        
        # Create a copy of the network for simulation
        sim_network = self.causal_network.copy()
        
        for step in range(steps):
            step_impact = 0.0
            
            # Direct effects
            for node in intervention.nodes:
                if node in sim_network:
                    step_impact += sim_network.nodes[node]['data'].impact
            
            # Propagate effects through network
            propagation_factor = 0.8 ** step  # Decay over time
            network_impact = self.calculate_network_effects(intervention.nodes)
            step_impact += network_impact * propagation_factor
            
            # Add emergence factor (systems can adapt)
            emergence = np.random.normal(1.0, 0.1)
            step_impact *= emergence
            
            impacts.append(step_impact)
        
        return impacts
    
    def export_analysis(self, filename: str):
        """Export the complete analysis"""
        analysis = {
            'network': {
                'nodes': [
                    {
                        'id': node,
                        'impact': self.causal_network.nodes[node]['data'].impact,
                        'in_pareto_set': node in self.current_pareto_set
                    }
                    for node in self.causal_network.nodes()
                ],
                'edges': [
                    {
                        'source': u,
                        'target': v,
                        'weight': d.get('weight', 1.0)
                    }
                    for u, v, d in self.causal_network.edges(data=True)
                ]
            },
            'pareto_analysis': self.calculate_dynamic_80_20(),
            'top_interventions': [
                {
                    'nodes': list(i.nodes),
                    'cost': i.total_cost,
                    'impact': i.expected_impact,
                    'roi': i.roi
                }
                for i in self.find_pareto_interventions()[:10]
            ]
        }
        
        with open(filename, 'w') as f:
            json.dump(analysis, f, indent=2)


# Example: Analyzing the CNS v8 Turtle Loop
if __name__ == "__main__":
    analyzer = ParetoNetworkAnalyzer()
    
    # Build causal network (not linear like "5 Whys")
    # Performance issues have multiple interacting causes
    
    # Direct causes
    analyzer.add_causal_relationship("large_input_data", "slow_processing", 0.7)
    analyzer.add_causal_relationship("complex_patterns", "slow_processing", 0.5)
    analyzer.add_causal_relationship("inefficient_parsing", "slow_processing", 0.8)
    analyzer.add_causal_relationship("excessive_validation", "slow_processing", 0.6)
    
    # Network effects
    analyzer.add_causal_relationship("slow_processing", "memory_pressure", 0.7)
    analyzer.add_causal_relationship("memory_pressure", "cache_misses", 0.9)
    analyzer.add_causal_relationship("cache_misses", "slow_processing", 0.4)  # Feedback!
    
    # Hidden factors that "5 Whys" misses
    analyzer.add_causal_relationship("cache_misses", "cpu_stalls", 0.8)
    analyzer.add_causal_relationship("cpu_stalls", "context_switches", 0.6)
    analyzer.add_causal_relationship("context_switches", "slow_processing", 0.3)
    
    # Measure actual impacts
    analyzer.measure_impact("large_input_data", 0.3, confidence=0.9)
    analyzer.measure_impact("complex_patterns", 0.15, confidence=0.8)
    analyzer.measure_impact("inefficient_parsing", 0.25, confidence=0.95)
    analyzer.measure_impact("excessive_validation", 0.20, confidence=0.9)
    analyzer.measure_impact("memory_pressure", 0.1, confidence=0.7)
    analyzer.measure_impact("cache_misses", 0.05, confidence=0.8)
    
    # Set intervention costs
    for node in analyzer.causal_network.nodes():
        node_data = analyzer.causal_network.nodes[node]['data']
        if "parsing" in node:
            node_data.fix_cost = 2.0  # Moderate effort
        elif "validation" in node:
            node_data.fix_cost = 1.5  # Easier to fix
        elif "cache" in node:
            node_data.fix_cost = 3.0  # Hard to optimize
        else:
            node_data.fix_cost = 1.0
    
    # Find optimal interventions
    print("=== Pareto Network Analysis vs 5 Whys ===\n")
    
    print("5 Whys would say: 'Root cause is large input data'")
    print("(Linear thinking, misses network effects)\n")
    
    print("Pareto Network Analysis reveals:")
    interventions = analyzer.find_pareto_interventions(budget=5.0)
    for i, intervention in enumerate(interventions[:5]):
        print(f"\nIntervention {i+1}:")
        print(f"  Fix: {', '.join(intervention.nodes)}")
        print(f"  Cost: {intervention.total_cost:.1f}")
        print(f"  Expected Impact: {intervention.expected_impact:.2f}")
        print(f"  ROI: {intervention.roi:.2f}")
    
    # Show dynamic 80/20
    stats = analyzer.calculate_dynamic_80_20()
    print(f"\nDynamic 80/20 Analysis:")
    print(f"  {stats['pareto_percentage']:.1f}% of nodes create {stats['impact_percentage']:.1f}% of impact")
    print(f"  Vital few: {', '.join(stats['nodes'])}")
    
    # Export full analysis
    analyzer.export_analysis("pareto_network_analysis.json")
    print("\nFull analysis exported to pareto_network_analysis.json")