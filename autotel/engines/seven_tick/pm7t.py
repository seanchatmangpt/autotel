"""
Process Mining 7-Tick Engine Python Interface
Provides high-performance process mining with <10ns operation guarantees
"""

import ctypes
import os
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Union
import numpy as np
import time

class PM7T:
    """
    Python interface to PM7T C engine.
    Provides process mining capabilities with 7-tick performance guarantees.
    """
    
    def __init__(self, initial_capacity: int = 10000):
        """
        Initialize PM7T engine.
        
        Args:
            initial_capacity: Initial capacity for event log
        """
        # Load C library
        lib_path = Path(__file__).parent / "lib" / "libpm7t.so"
        if not lib_path.exists():
            self._build_library()
        
        self.lib = ctypes.CDLL(str(lib_path))
        
        # Define C structures and functions
        self._define_structures()
        self._setup_functions()
        
        # Create event log
        self.event_log = self.lib.pm7t_create_event_log(initial_capacity)
        if not self.event_log:
            raise RuntimeError("Failed to create PM7T event log")
        
        # Activity and resource mapping
        self.activity_to_id = {}
        self.id_to_activity = {}
        self.resource_to_id = {}
        self.id_to_resource = {}
        self.next_activity_id = 0
        self.next_resource_id = 0
        
        # Performance tracking
        self.stats = {
            "events_added": 0,
            "traces_extracted": 0,
            "models_discovered": 0,
            "total_ticks": 0
        }
    
    def _build_library(self):
        """Build the PM7T C library."""
        makefile_path = Path(__file__).parent / "Makefile"
        if makefile_path.exists():
            os.system(f"cd {Path(__file__).parent} && make libpm7t.so")
        else:
            # Create a simple Makefile
            makefile_content = """
CC = gcc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
LDFLAGS = -shared

libpm7t.so: c_src/pm7t.c
\t$(CC) $(CFLAGS) $(LDFLAGS) -o lib/$@ $<

clean:
\trm -f lib/*.so
"""
            makefile_path.write_text(makefile_content)
            os.makedirs(Path(__file__).parent / "lib", exist_ok=True)
            os.system(f"cd {Path(__file__).parent} && make libpm7t.so")
    
    def _define_structures(self):
        """Define C structures using ctypes."""
        
        class Event(ctypes.Structure):
            _fields_ = [
                ("case_id", ctypes.c_uint32),
                ("activity_id", ctypes.c_uint32),
                ("timestamp", ctypes.c_uint64),
                ("resource_id", ctypes.c_uint32),
                ("cost", ctypes.c_uint32)
            ]
        
        class EventLog(ctypes.Structure):
            _fields_ = [
                ("events", ctypes.POINTER(Event)),
                ("capacity", ctypes.c_size_t),
                ("size", ctypes.c_size_t)
            ]
        
        class Trace(ctypes.Structure):
            _fields_ = [
                ("activities", ctypes.POINTER(ctypes.c_uint32)),
                ("size", ctypes.c_size_t)
            ]
        
        class TraceLog(ctypes.Structure):
            _fields_ = [
                ("traces", ctypes.POINTER(Trace)),
                ("capacity", ctypes.c_size_t),
                ("size", ctypes.c_size_t)
            ]
        
        class Transition(ctypes.Structure):
            _fields_ = [
                ("from_activity", ctypes.c_uint32),
                ("to_activity", ctypes.c_uint32),
                ("frequency", ctypes.c_uint32),
                ("probability", ctypes.c_double)
            ]
        
        class ProcessModel(ctypes.Structure):
            _fields_ = [
                ("transitions", ctypes.POINTER(Transition)),
                ("capacity", ctypes.c_size_t),
                ("size", ctypes.c_size_t),
                ("num_activities", ctypes.c_uint32)
            ]
        
        class ActivityStats(ctypes.Structure):
            _fields_ = [
                ("activity_id", ctypes.c_uint32),
                ("frequency", ctypes.c_uint32),
                ("avg_duration", ctypes.c_double),
                ("min_duration", ctypes.c_double),
                ("max_duration", ctypes.c_double)
            ]
        
        class ProcessStats(ctypes.Structure):
            _fields_ = [
                ("activities", ctypes.POINTER(ActivityStats)),
                ("capacity", ctypes.c_size_t),
                ("size", ctypes.c_size_t)
            ]
        
        class ConformanceResult(ctypes.Structure):
            _fields_ = [
                ("fitness", ctypes.c_double),
                ("precision", ctypes.c_double),
                ("generalization", ctypes.c_double),
                ("simplicity", ctypes.c_double)
            ]
        
        class CasePerformance(ctypes.Structure):
            _fields_ = [
                ("case_id", ctypes.c_uint32),
                ("start_time", ctypes.c_uint64),
                ("end_time", ctypes.c_uint64),
                ("duration", ctypes.c_uint64),
                ("num_activities", ctypes.c_uint32)
            ]
        
        class PerformanceAnalysis(ctypes.Structure):
            _fields_ = [
                ("cases", ctypes.POINTER(CasePerformance)),
                ("capacity", ctypes.c_size_t),
                ("size", ctypes.c_size_t),
                ("avg_duration", ctypes.c_double),
                ("min_duration", ctypes.c_double),
                ("max_duration", ctypes.c_double),
                ("throughput", ctypes.c_double)
            ]
        
        # Store structures
        self.Event = Event
        self.EventLog = EventLog
        self.Trace = Trace
        self.TraceLog = TraceLog
        self.Transition = Transition
        self.ProcessModel = ProcessModel
        self.ActivityStats = ActivityStats
        self.ProcessStats = ProcessStats
        self.ConformanceResult = ConformanceResult
        self.CasePerformance = CasePerformance
        self.PerformanceAnalysis = PerformanceAnalysis
    
    def _setup_functions(self):
        """Configure C function signatures."""
        # Event log management
        self.lib.pm7t_create_event_log.argtypes = [ctypes.c_size_t]
        self.lib.pm7t_create_event_log.restype = ctypes.POINTER(self.EventLog)
        
        self.lib.pm7t_destroy_event_log.argtypes = [ctypes.POINTER(self.EventLog)]
        self.lib.pm7t_destroy_event_log.restype = None
        
        self.lib.pm7t_add_event.argtypes = [
            ctypes.POINTER(self.EventLog),
            ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64,
            ctypes.c_uint32, ctypes.c_uint32
        ]
        self.lib.pm7t_add_event.restype = ctypes.c_int
        
        self.lib.pm7t_get_event_count.argtypes = [ctypes.POINTER(self.EventLog)]
        self.lib.pm7t_get_event_count.restype = ctypes.c_size_t
        
        # Trace extraction
        self.lib.pm7t_extract_traces.argtypes = [ctypes.POINTER(self.EventLog)]
        self.lib.pm7t_extract_traces.restype = ctypes.POINTER(self.TraceLog)
        
        self.lib.pm7t_destroy_trace_log.argtypes = [ctypes.POINTER(self.TraceLog)]
        self.lib.pm7t_destroy_trace_log.restype = None
        
        self.lib.pm7t_get_trace_count.argtypes = [ctypes.POINTER(self.TraceLog)]
        self.lib.pm7t_get_trace_count.restype = ctypes.c_size_t
        
        # Process discovery
        self.lib.pm7t_discover_alpha_algorithm.argtypes = [ctypes.POINTER(self.TraceLog)]
        self.lib.pm7t_discover_alpha_algorithm.restype = ctypes.POINTER(self.ProcessModel)
        
        self.lib.pm7t_discover_heuristic_miner.argtypes = [
            ctypes.POINTER(self.TraceLog),
            ctypes.c_double
        ]
        self.lib.pm7t_discover_heuristic_miner.restype = ctypes.POINTER(self.ProcessModel)
        
        self.lib.pm7t_discover_inductive_miner.argtypes = [ctypes.POINTER(self.TraceLog)]
        self.lib.pm7t_discover_inductive_miner.restype = ctypes.POINTER(self.ProcessModel)
        
        self.lib.pm7t_destroy_process_model.argtypes = [ctypes.POINTER(self.ProcessModel)]
        self.lib.pm7t_destroy_process_model.restype = None
        
        # Process analysis
        self.lib.pm7t_analyze_process.argtypes = [ctypes.POINTER(self.EventLog)]
        self.lib.pm7t_analyze_process.restype = ctypes.POINTER(self.ProcessStats)
        
        self.lib.pm7t_destroy_process_stats.argtypes = [ctypes.POINTER(self.ProcessStats)]
        self.lib.pm7t_destroy_process_stats.restype = None
        
        # Conformance checking
        self.lib.pm7t_calculate_fitness.argtypes = [
            ctypes.POINTER(self.ProcessModel),
            ctypes.POINTER(self.TraceLog)
        ]
        self.lib.pm7t_calculate_fitness.restype = ctypes.c_double
        
        self.lib.pm7t_check_conformance.argtypes = [
            ctypes.POINTER(self.ProcessModel),
            ctypes.POINTER(self.TraceLog)
        ]
        self.lib.pm7t_check_conformance.restype = self.ConformanceResult
        
        # Performance analysis
        self.lib.pm7t_analyze_performance.argtypes = [ctypes.POINTER(self.EventLog)]
        self.lib.pm7t_analyze_performance.restype = ctypes.POINTER(self.PerformanceAnalysis)
        
        self.lib.pm7t_destroy_performance_analysis.argtypes = [
            ctypes.POINTER(self.PerformanceAnalysis)
        ]
        self.lib.pm7t_destroy_performance_analysis.restype = None
    
    def _get_or_create_activity_id(self, activity: str) -> int:
        """Get or create numeric ID for activity."""
        if activity not in self.activity_to_id:
            self.activity_to_id[activity] = self.next_activity_id
            self.id_to_activity[self.next_activity_id] = activity
            self.next_activity_id += 1
        return self.activity_to_id[activity]
    
    def _get_or_create_resource_id(self, resource: str) -> int:
        """Get or create numeric ID for resource."""
        if not resource:
            return 0
        if resource not in self.resource_to_id:
            self.resource_to_id[resource] = self.next_resource_id
            self.id_to_resource[self.next_resource_id] = resource
            self.next_resource_id += 1
        return self.resource_to_id[resource]
    
    def add_event(self, case_id: int, activity: str, timestamp: int,
                  resource: str = "", cost: int = 0) -> bool:
        """
        Add event to the log (7-tick operation).
        
        Args:
            case_id: Case identifier
            activity: Activity name
            timestamp: Event timestamp (Unix time in nanoseconds)
            resource: Resource name (optional)
            cost: Event cost (optional)
            
        Returns:
            bool: True if successful
        """
        start_ticks = time.perf_counter_ns()
        
        activity_id = self._get_or_create_activity_id(activity)
        resource_id = self._get_or_create_resource_id(resource)
        
        result = self.lib.pm7t_add_event(
            self.event_log, case_id, activity_id, timestamp, resource_id, cost
        )
        
        end_ticks = time.perf_counter_ns()
        self.stats["total_ticks"] += (end_ticks - start_ticks)
        self.stats["events_added"] += 1
        
        return result == 0
    
    def add_events_batch(self, events: List[Dict]) -> int:
        """
        Add multiple events in batch for better performance.
        
        Args:
            events: List of event dictionaries
            
        Returns:
            int: Number of successfully added events
        """
        count = 0
        for event in events:
            if self.add_event(**event):
                count += 1
        return count
    
    def extract_traces(self) -> 'TraceLog':
        """Extract traces from event log."""
        traces = self.lib.pm7t_extract_traces(self.event_log)
        if not traces:
            raise RuntimeError("Failed to extract traces")
        self.stats["traces_extracted"] += 1
        return traces
    
    def discover_process(self, algorithm: str = "alpha",
                        dependency_threshold: float = 0.9) -> 'ProcessModel':
        """
        Discover process model from event log.
        
        Args:
            algorithm: Discovery algorithm ("alpha", "heuristic", "inductive")
            dependency_threshold: Threshold for heuristic miner
            
        Returns:
            ProcessModel: Discovered process model
        """
        traces = self.extract_traces()
        
        try:
            if algorithm == "alpha":
                model = self.lib.pm7t_discover_alpha_algorithm(traces)
            elif algorithm == "heuristic":
                model = self.lib.pm7t_discover_heuristic_miner(
                    traces, ctypes.c_double(dependency_threshold)
                )
            elif algorithm == "inductive":
                model = self.lib.pm7t_discover_inductive_miner(traces)
            else:
                raise ValueError(f"Unknown algorithm: {algorithm}")
            
            if not model:
                raise RuntimeError(f"Failed to discover process using {algorithm}")
            
            self.stats["models_discovered"] += 1
            return model
            
        finally:
            self.lib.pm7t_destroy_trace_log(traces)
    
    def check_conformance(self, model: 'ProcessModel') -> Dict[str, float]:
        """
        Check conformance between model and log.
        
        Args:
            model: Process model to check
            
        Returns:
            dict: Conformance metrics (fitness, precision, generalization, simplicity)
        """
        traces = self.extract_traces()
        
        try:
            result = self.lib.pm7t_check_conformance(model, traces)
            return {
                "fitness": result.fitness,
                "precision": result.precision,
                "generalization": result.generalization,
                "simplicity": result.simplicity
            }
        finally:
            self.lib.pm7t_destroy_trace_log(traces)
    
    def analyze_performance(self) -> Dict[str, Union[float, List[Dict]]]:
        """
        Analyze performance metrics of the process.
        
        Returns:
            dict: Performance analysis results
        """
        perf = self.lib.pm7t_analyze_performance(self.event_log)
        if not perf:
            raise RuntimeError("Failed to analyze performance")
        
        try:
            # Extract case performances
            cases = []
            for i in range(perf.contents.size):
                case = perf.contents.cases[i]
                cases.append({
                    "case_id": case.case_id,
                    "start_time": case.start_time,
                    "end_time": case.end_time,
                    "duration": case.duration,
                    "num_activities": case.num_activities
                })
            
            return {
                "avg_duration": perf.contents.avg_duration,
                "min_duration": perf.contents.min_duration,
                "max_duration": perf.contents.max_duration,
                "throughput": perf.contents.throughput,
                "cases": cases
            }
        finally:
            self.lib.pm7t_destroy_performance_analysis(perf)
    
    def analyze_process(self) -> Dict[str, List[Dict]]:
        """
        Analyze process statistics.
        
        Returns:
            dict: Process statistics by activity
        """
        stats = self.lib.pm7t_analyze_process(self.event_log)
        if not stats:
            raise RuntimeError("Failed to analyze process")
        
        try:
            activities = []
            for i in range(stats.contents.size):
                activity = stats.contents.activities[i]
                activities.append({
                    "activity": self.id_to_activity.get(activity.activity_id, f"Unknown_{activity.activity_id}"),
                    "frequency": activity.frequency,
                    "avg_duration": activity.avg_duration,
                    "min_duration": activity.min_duration,
                    "max_duration": activity.max_duration
                })
            
            return {"activities": activities}
        finally:
            self.lib.pm7t_destroy_process_stats(stats)
    
    def get_model_transitions(self, model: 'ProcessModel') -> List[Dict]:
        """
        Extract transitions from process model.
        
        Args:
            model: Process model
            
        Returns:
            list: List of transition dictionaries
        """
        transitions = []
        for i in range(model.contents.size):
            trans = model.contents.transitions[i]
            transitions.append({
                "from": self.id_to_activity.get(trans.from_activity, f"Unknown_{trans.from_activity}"),
                "to": self.id_to_activity.get(trans.to_activity, f"Unknown_{trans.to_activity}"),
                "frequency": trans.frequency,
                "probability": trans.probability
            })
        return transitions
    
    def get_event_count(self) -> int:
        """Get current number of events in log."""
        return self.lib.pm7t_get_event_count(self.event_log)
    
    def get_stats(self) -> Dict[str, Union[int, float]]:
        """Get performance statistics."""
        return self.stats.copy()
    
    def clear(self):
        """Clear event log and reset engine."""
        if hasattr(self, 'event_log') and self.event_log:
            self.lib.pm7t_destroy_event_log(self.event_log)
        
        # Recreate event log
        self.event_log = self.lib.pm7t_create_event_log(10000)
        if not self.event_log:
            raise RuntimeError("Failed to recreate event log")
        
        # Reset mappings
        self.activity_to_id.clear()
        self.id_to_activity.clear()
        self.resource_to_id.clear()
        self.id_to_resource.clear()
        self.next_activity_id = 0
        self.next_resource_id = 0
        
        # Reset stats
        self.stats = {
            "events_added": 0,
            "traces_extracted": 0,
            "models_discovered": 0,
            "total_ticks": 0
        }
    
    def __del__(self):
        """Cleanup resources."""
        if hasattr(self, 'event_log') and self.event_log:
            self.lib.pm7t_destroy_event_log(self.event_log)


# Example usage
if __name__ == "__main__":
    # Create PM7T engine
    pm = PM7T()
    
    # Add some events
    events = [
        {"case_id": 1, "activity": "Start", "timestamp": 1000000000, "resource": "System"},
        {"case_id": 1, "activity": "Check", "timestamp": 1000001000, "resource": "John"},
        {"case_id": 1, "activity": "Process", "timestamp": 1000002000, "resource": "Mary"},
        {"case_id": 1, "activity": "End", "timestamp": 1000003000, "resource": "System"},
        
        {"case_id": 2, "activity": "Start", "timestamp": 1000010000, "resource": "System"},
        {"case_id": 2, "activity": "Check", "timestamp": 1000011000, "resource": "John"},
        {"case_id": 2, "activity": "Process", "timestamp": 1000012000, "resource": "John"},
        {"case_id": 2, "activity": "End", "timestamp": 1000013000, "resource": "System"},
    ]
    
    # Add events
    for event in events:
        pm.add_event(**event)
    
    print(f"Added {pm.get_event_count()} events")
    
    # Discover process
    model = pm.discover_process(algorithm="alpha")
    print(f"Discovered model with {model.contents.size} transitions")
    
    # Check conformance
    conformance = pm.check_conformance(model)
    print(f"Conformance: {conformance}")
    
    # Analyze performance
    performance = pm.analyze_performance()
    print(f"Average case duration: {performance['avg_duration']} ns")
    print(f"Throughput: {performance['throughput']} cases/second")
    
    # Get process statistics
    stats = pm.analyze_process()
    print("\nActivity statistics:")
    for activity in stats["activities"]:
        print(f"  {activity['activity']}: {activity['frequency']} occurrences")
    
    # Clean up
    pm.clear()