"""
AutoTel - Enterprise BPMN 2.0 Orchestration with Zero-Touch Telemetry
"""

__version__ = "0.1.0"
__author__ = "AutoTel Team"
__email__ = "team@autotel.dev"


# Core exports
from .core.orchestrator import Orchestrator, ProcessStatus, TaskStatus, ProcessInstance
from .core.framework import Framework, LinkMLProcessor, SemanticWorkflowEngine
from .core.telemetry import TelemetryManager

# Schema exports
from .schemas.linkml import LinkMLProcessor as SchemaProcessor
from .schemas.validation import validate_telemetry_schema

# Workflow exports
from .workflows.spiff import SpiffCapabilityChecker

# Main framework instance
__all__ = [
    # Core
    "Orchestrator",
    "Framework", 
    "TelemetryManager",
    "ProcessStatus",
    "TaskStatus",
    "ProcessInstance",
    
    # Schemas
    "SchemaProcessor",
    "validate_telemetry_schema",
    
    # Workflows
    "SpiffCapabilityChecker",
    
    # Version
    "__version__",
] 