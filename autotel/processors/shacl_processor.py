"""
AutoTel SHACL Processor (Unified Architecture)

Processes SHACL/RDF XML into constraint graphs and validation rules.
Implements happy-path parsing and constraint extraction with telemetry and contracts.
"""

from typing import Any, Dict, List, Optional
from pathlib import Path
from rdflib import Graph, Namespace
from rdflib.namespace import RDF
from pydantic import BaseModel, Field

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error

# SHACL namespace
SHACL = Namespace("http://www.w3.org/ns/shacl#")

class SHACLConstraint(BaseModel):
    """Represents a SHACL constraint."""
    shape_id: str = Field(..., description="ID of the shape containing this constraint")
    type: str = Field(..., description="Type of constraint (min_count, max_count, etc.)")
    value: Any = Field(..., description="Constraint value")

class SHACLPropertyShape(BaseModel):
    """Represents a SHACL property shape."""
    shape_id: str = Field(..., description="ID of the property shape")
    path: Optional[str] = Field(None, description="Property path")
    constraints: Dict[str, Any] = Field(default_factory=dict, description="Property constraints")

class SHACLNodeShape(BaseModel):
    """Represents a SHACL node shape."""
    shape_id: str = Field(..., description="ID of the node shape")
    target_class: Optional[str] = Field(None, description="Target class for this shape")
    properties: List[str] = Field(default_factory=list, description="List of property shape IDs")

class SHACLResult(BaseModel):
    """Result model for SHACL processing."""
    node_shapes: List[SHACLNodeShape] = Field(default_factory=list, description="Extracted node shapes")
    property_shapes: List[SHACLPropertyShape] = Field(default_factory=list, description="Extracted property shapes")
    constraints: List[SHACLConstraint] = Field(default_factory=list, description="Extracted constraints")
    total_shapes: int = Field(..., description="Total number of shapes found")
    total_constraints: int = Field(..., description="Total number of constraints found")

@processor_metadata(
    name="shacl_processor",
    version="1.0.0",
    capabilities=["shacl_parse", "constraint_extraction"],
    supported_formats=["xml", "shacl", "rdf"],
    author="AutoTel Team"
)
class SHACLProcessor(BaseProcessor):
    """
    Unified SHACL processor for AutoTel.
    Parses SHACL XML and extracts node/property shapes and constraints.
    """
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse SHACL XML string and extract constraints (happy path only).
        Args:
            data: XML string or file path
        Returns:
            ProcessorResult with SHACLResult data
        """
        with create_processor_span("parse", "shacl") as span:
            try:
                xml_content = self._get_xml_content(data)
                graph = Graph()
                # Set publicID to expand QNames to full URIs
                graph.parse(data=xml_content, format="xml", publicID="http://example.com/sro#")
                
                node_shapes = self._extract_node_shapes(graph)
                property_shapes = self._extract_property_shapes(graph)
                constraints = self._extract_constraints(graph)
                
                shacl_result = SHACLResult(
                    node_shapes=node_shapes,
                    property_shapes=property_shapes,
                    constraints=constraints,
                    total_shapes=len(node_shapes) + len(property_shapes),
                    total_constraints=len(constraints)
                )
                
                record_span_success(span, {
                    "node_shapes": len(node_shapes),
                    "property_shapes": len(property_shapes),
                    "constraints": len(constraints)
                })
                
                return ProcessorResult.success_result(
                    data=shacl_result,
                    metadata={
                        "node_shapes": len(node_shapes),
                        "property_shapes": len(property_shapes),
                        "constraints": len(constraints)
                    }
                )
            except Exception as e:
                record_span_error(span, e, {"input_type": type(data).__name__})
                return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})

    def _get_xml_content(self, data: Any) -> str:
        """Get XML content from string or file path."""
        if isinstance(data, str):
            if Path(data).exists():
                return Path(data).read_text(encoding="utf-8")
            return data
        elif isinstance(data, Path):
            return data.read_text(encoding="utf-8")
        raise ValueError("Input must be XML string or file path")

    def _expand_uri(self, graph: Graph, value: str) -> str:
        """Expand a QName to a full URI using the graph's namespace manager, if needed."""
        if value.startswith("http://") or value.startswith("https://"):
            return value
        try:
            return str(graph.namespace_manager.expand_curie(value))
        except Exception:
            return value

    def _extract_node_shapes(self, graph: Graph) -> List[SHACLNodeShape]:
        """Extract node shapes from the graph (happy path, always full URIs for IDs and property paths)."""
        from rdflib.term import URIRef
        node_shapes = []
        for node_shape in graph.subjects(RDF.type, SHACL.NodeShape):
            shape_id = node_shape.toPython() if isinstance(node_shape, URIRef) else str(node_shape)
            shape_id = self._expand_uri(graph, shape_id)
            shape_data = {"shape_id": shape_id, "target_class": None, "properties": []}
            for target_class in graph.objects(node_shape, SHACL.targetClass):
                tc_val = target_class.toPython() if isinstance(target_class, URIRef) else str(target_class)
                shape_data["target_class"] = self._expand_uri(graph, tc_val)
            for property_shape in graph.objects(node_shape, SHACL.property):
                # property_shape is usually a blank node; resolve its sh:path if possible
                path_uri = None
                for path in graph.objects(property_shape, SHACL.path):
                    if isinstance(path, URIRef):
                        path_uri = path.toPython()
                    else:
                        path_uri = str(path)
                    path_uri = self._expand_uri(graph, path_uri)
                    break
                if path_uri:
                    shape_data["properties"].append(path_uri)
                else:
                    # fallback: use the property_shape node id (should be rare)
                    shape_data["properties"].append(str(property_shape))
            if shape_data["shape_id"]:
                node_shapes.append(SHACLNodeShape(**shape_data))
        return node_shapes

    def _extract_property_shapes(self, graph: Graph) -> List[SHACLPropertyShape]:
        """Extract property shapes from the graph (happy path)."""
        property_shapes = []
        for property_shape in graph.subjects(RDF.type, SHACL.PropertyShape):
            prop_data = {"shape_id": str(property_shape), "path": None, "constraints": {}}
            for path in graph.objects(property_shape, SHACL.path):
                prop_data["path"] = str(path)
            # Only a few common constraints for happy path
            for min_count in graph.objects(property_shape, SHACL.minCount):
                prop_data["constraints"]["min_count"] = int(min_count)
            for max_count in graph.objects(property_shape, SHACL.maxCount):
                prop_data["constraints"]["max_count"] = int(max_count)
            property_shapes.append(SHACLPropertyShape(**prop_data))
        return property_shapes

    def _extract_constraints(self, graph: Graph) -> List[SHACLConstraint]:
        """Extract constraints from property shapes (happy path)."""
        constraints = []
        for property_shape in graph.subjects(RDF.type, SHACL.PropertyShape):
            for min_count in graph.objects(property_shape, SHACL.minCount):
                constraints.append(SHACLConstraint(
                    shape_id=str(property_shape),
                    type="min_count",
                    value=int(min_count)
                ))
            for max_count in graph.objects(property_shape, SHACL.maxCount):
                constraints.append(SHACLConstraint(
                    shape_id=str(property_shape),
                    type="max_count",
                    value=int(max_count)
                ))
        return constraints 
 