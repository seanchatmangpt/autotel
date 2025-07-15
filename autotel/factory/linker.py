"""
Semantic Linker - Links Parsed Objects into Executable Capabilities

Implements AUTOTEL-105: Receive parsed, isolated objects (BPMN spec, DMN decisions, 
DSPy definitions) and link them together by injecting executable capabilities into 
the BPMN task specifications.
"""

from typing import Dict, Any, List
from SpiffWorkflow.specs import WorkflowSpec
# DMN decisions are handled by CamundaParser
from .processors.dspy_processor import DSPySignatureDefinition


class SemanticLinkerError(Exception):
    """Exception raised when semantic linking fails."""
    pass


class SemanticLinker:
    """
    Semantic linker that injects executable capabilities into BPMN task specifications.
    
    This class receives the parsed, isolated objects from the processors and links
    them together by injecting executable capabilities into the BPMN task specifications.
    """
    
    def __init__(self):
        """Initialize the semantic linker."""
        pass
    
    def link(self, 
             bpmn_spec: WorkflowSpec,
             dmn_decisions: Dict[str, Any],
             dspy_signatures: List[DSPySignatureDefinition],
             shacl_graph=None,
             owl_graph=None) -> WorkflowSpec:
        """
        Link parsed objects by injecting executable capabilities into BPMN task specifications.
        
        Args:
            bpmn_spec: WorkflowSpec from BPMN processor
            dmn_decisions: Dictionary of DmnDecision objects from DMN processor
            dspy_signatures: List of DSPySignatureDefinition objects from DSPy processor
            shacl_graph: Optional rdflib.Graph from SHACL processor
            owl_graph: Optional rdflib.Graph from OWL processor
            
        Returns:
            WorkflowSpec: Modified workflow spec with injected executable capabilities
            
        Raises:
            SemanticLinkerError: If referenced decisionRef or signature name is not found
        """
        try:
            # Convert DSPy signatures list to dictionary for easier lookup
            dspy_signatures_dict = {sig.name: sig for sig in dspy_signatures}
            
            # Iterate through all tasks in the BPMN spec
            for task_spec in bpmn_spec.task_specs.values():
                self._link_task(task_spec, dmn_decisions, dspy_signatures_dict)
            
            return bpmn_spec
            
        except Exception as e:
            raise SemanticLinkerError(f"Failed to link semantic objects: {str(e)}")
    
    def _link_task(self, 
                   task_spec: Any,
                   dmn_decisions: Dict[str, Any],
                   dspy_signatures: Dict[str, DSPySignatureDefinition]):
        """
        Link a single task by injecting executable capabilities.
        
        Args:
            task_spec: Individual task specification
            dmn_decisions: Dictionary of DmnDecision objects
            dspy_signatures: Dictionary of DSPySignatureDefinition objects
            
        Raises:
            SemanticLinkerError: If referenced decisionRef or signature name is not found
        """
        # Check if this is a BusinessRuleTask with DMN decision reference
        if hasattr(task_spec, 'decisionRef') and task_spec.decisionRef:
            decision_ref = task_spec.decisionRef
            if decision_ref not in dmn_decisions:
                raise SemanticLinkerError(
                    f"Referenced DMN decision '{decision_ref}' not found in provided decisions"
                )
            
            # Inject the DMN decision as an executable capability
            task_spec.executable_dmn = dmn_decisions[decision_ref]
        
        # Check if this is a ServiceTask with DSPy service extension
        if hasattr(task_spec, 'extensions') and task_spec.extensions:
            dspy_service = task_spec.extensions.get('dspy_service')
            if dspy_service and 'signature' in dspy_service:
                signature_name = dspy_service['signature']
                if signature_name not in dspy_signatures:
                    raise SemanticLinkerError(
                        f"Referenced DSPy signature '{signature_name}' not found in provided signatures"
                    )
                
                # Inject the DSPy signature definition as an executable capability
                task_spec.executable_dspy_signature = dspy_signatures[signature_name]
    
    def validate_linkage(self, 
                        bpmn_spec: WorkflowSpec,
                        dmn_decisions: Dict[str, Any],
                        dspy_signatures: List[DSPySignatureDefinition]) -> Dict[str, Any]:
        """
        Validate that all referenced capabilities exist before linking.
        
        Args:
            bpmn_spec: WorkflowSpec from BPMN processor
            dmn_decisions: Dictionary of DmnDecision objects from DMN processor
            dspy_signatures: List of DSPySignatureDefinition objects from DSPy processor
            
        Returns:
            Dict[str, Any]: Validation report with missing references
            
        Raises:
            SemanticLinkerError: If validation fails
        """
        dspy_signatures_dict = {sig.name: sig for sig in dspy_signatures}
        
        missing_dmn_refs = []
        missing_dspy_refs = []
        
        # Check all tasks for missing references
        for task_spec in bpmn_spec.task_specs.values():
            # Check DMN references
            if hasattr(task_spec, 'decisionRef') and task_spec.decisionRef:
                if task_spec.decisionRef not in dmn_decisions:
                    missing_dmn_refs.append(task_spec.decisionRef)
            
            # Check DSPy references
            if hasattr(task_spec, 'extensions') and task_spec.extensions:
                dspy_service = task_spec.extensions.get('dspy_service')
                if dspy_service and 'signature' in dspy_service:
                    signature_name = dspy_service['signature']
                    if signature_name not in dspy_signatures_dict:
                        missing_dspy_refs.append(signature_name)
        
        validation_report = {
            'valid': len(missing_dmn_refs) == 0 and len(missing_dspy_refs) == 0,
            'missing_dmn_refs': missing_dmn_refs,
            'missing_dspy_refs': missing_dspy_refs,
            'total_tasks': len(bpmn_spec.task_specs),
            'available_dmn_decisions': list(dmn_decisions.keys()),
            'available_dspy_signatures': list(dspy_signatures_dict.keys())
        }
        
        if not validation_report['valid']:
            raise SemanticLinkerError(
                f"Validation failed: Missing DMN refs: {missing_dmn_refs}, "
                f"Missing DSPy refs: {missing_dspy_refs}"
            )
        
        return validation_report 