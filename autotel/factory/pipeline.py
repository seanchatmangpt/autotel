"""
AutoTel Factory Pipeline - Top-Level Factory

Implements AUTOTEL-106: Single, simple entry point that orchestrates the entire 
parsing and linking pipeline to convert raw XML into trusted ExecutableSpecification.
"""

from typing import Dict, Any, Optional
from dataclasses import dataclass
from pathlib import Path
from rdflib import Graph

from SpiffWorkflow.specs import WorkflowSpec
# DMN decisions are handled by CamundaParser

from .processors.bpmn_processor import BPMNProcessor
from .processors.dmn_processor import DMNProcessor
from .processors.dspy_processor import DSPyProcessor, DSPySignatureDefinition
from .processors.shacl_processor import SHACLProcessor
from .processors.owl_processor import OWLProcessor
from .linker import SemanticLinker, SemanticLinkerError


@dataclass
class ExecutableSpecification:
    """
    Immutable object containing all linked artifacts for execution.
    
    This is the final output of the AutoTel Factory pipeline, containing
    all the parsed and linked components ready for runtime execution.
    """
    process_id: str
    bpmn_spec: WorkflowSpec
    dmn_decisions: Dict[str, Any]
    dspy_signatures: Dict[str, DSPySignatureDefinition]
    shacl_graph: Graph
    owl_graph: Graph
    
    def __post_init__(self):
        """Make the object immutable after initialization."""
        object.__setattr__(self, '_frozen', True)
    
    def __setattr__(self, name, value):
        """Prevent modification after initialization."""
        if hasattr(self, '_frozen') and self._frozen:
            raise AttributeError(f"Cannot modify immutable ExecutableSpecification")
        super().__setattr__(name, value)


class AutoTelFactory:
    """
    Top-level factory that orchestrates the entire parsing and linking pipeline.
    
    This class provides a single, simple entry point for converting raw XML files
    into trusted ExecutableSpecification objects.
    """
    
    def __init__(self):
        """Initialize the factory with all required processors and linker."""
        self.bpmn_processor = BPMNProcessor()
        self.dmn_processor = DMNProcessor()
        self.dspy_processor = DSPyProcessor()
        self.shacl_processor = SHACLProcessor()
        self.owl_processor = OWLProcessor()
        self.linker = SemanticLinker()
    
    def create_specification_from_file(self, path: str, process_id: str) -> ExecutableSpecification:
        """
        Create an ExecutableSpecification from a file path.
        
        Args:
            path: Path to the XML file containing all components
            process_id: ID of the BPMN process to extract
            
        Returns:
            ExecutableSpecification: Valid, immutable specification object
            
        Raises:
            FileNotFoundError: If file does not exist
            ValueError: If parsing fails
            SemanticLinkerError: If linking fails
        """
        try:
            # Read the XML file
            with open(path, 'r', encoding='utf-8') as f:
                xml_content = f.read()
            
            return self.create_specification_from_xml(xml_content, process_id)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"File not found: {path}")
        except Exception as e:
            raise ValueError(f"Failed to read file '{path}': {str(e)}")
    
    def create_specification_from_xml(self, xml_content: str, process_id: str) -> ExecutableSpecification:
        """
        Create an ExecutableSpecification from XML content.
        
        Args:
            xml_content: XML string containing all components
            process_id: ID of the BPMN process to extract
            
        Returns:
            ExecutableSpecification: Valid, immutable specification object
            
        Raises:
            ValueError: If parsing fails
            SemanticLinkerError: If linking fails
        """
        try:
            # Step 1: Parse BPMN specification
            bpmn_spec = self.bpmn_processor.parse(xml_content, process_id)
            
            # Step 2: Parse DMN decisions
            dmn_decisions = self.dmn_processor.parse(xml_content)
            
            # Step 3: Parse DSPy signatures
            dspy_signatures = self.dspy_processor.parse(xml_content)
            dspy_signatures_dict = {sig.name: sig for sig in dspy_signatures}
            
            # Step 4: Parse SHACL shapes
            shacl_graph = self.shacl_processor.parse(xml_content)
            
            # Step 5: Parse OWL ontologies
            owl_graph = self.owl_processor.parse(xml_content)
            
            # Step 6: Link all components together
            linked_bpmn_spec = self.linker.link(
                bpmn_spec=bpmn_spec,
                dmn_decisions=dmn_decisions,
                dspy_signatures=dspy_signatures,
                shacl_graph=shacl_graph,
                owl_graph=owl_graph
            )
            
            # Step 7: Create and return the executable specification
            return ExecutableSpecification(
                process_id=process_id,
                bpmn_spec=linked_bpmn_spec,
                dmn_decisions=dmn_decisions,
                dspy_signatures=dspy_signatures_dict,
                shacl_graph=shacl_graph,
                owl_graph=owl_graph
            )
            
        except Exception as e:
            if isinstance(e, (ValueError, SemanticLinkerError)):
                raise
            else:
                raise ValueError(f"Failed to create specification: {str(e)}")
    
    def validate_specification(self, xml_content: str, process_id: str) -> Dict[str, Any]:
        """
        Validate that a specification can be created without actually creating it.
        
        Args:
            xml_content: XML string containing all components
            process_id: ID of the BPMN process to extract
            
        Returns:
            Dict[str, Any]: Validation report
        """
        try:
            # Parse all components
            bpmn_spec = self.bpmn_processor.parse(xml_content, process_id)
            dmn_decisions = self.dmn_processor.parse(xml_content)
            dspy_signatures = self.dspy_processor.parse(xml_content)
            
            # Validate linkage
            validation_report = self.linker.validate_linkage(
                bpmn_spec=bpmn_spec,
                dmn_decisions=dmn_decisions,
                dspy_signatures=dspy_signatures
            )
            
            # Add additional validation info
            validation_report.update({
                'process_id': process_id,
                'bpmn_spec_valid': True,
                'dmn_decisions_count': len(dmn_decisions),
                'dspy_signatures_count': len(dspy_signatures),
                'overall_valid': validation_report['valid']
            })
            
            return validation_report
            
        except Exception as e:
            return {
                'valid': False,
                'error': str(e),
                'process_id': process_id,
                'overall_valid': False
            } 