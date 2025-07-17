el Jinja Processor (Unified Architecture)

Processes Jinja2 templates from XML definitions and renders them with variable substitution.
Implements happy-path parsing and template rendering with telemetry and contracts.
import time
from typing import Any, Dict, List, Optional
from pathlib import Path
from xml.etree import ElementTree as ET
from jinja2 import Environment, Template, TemplateError, UndefinedError
from pydantic import BaseModel, Field

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error

class JinjaVariable(BaseModel):
    """Represents a Jinja template variable."""
    name: str = Field(..., description="Variable name")
    description: str = Field(default="", description="Variable description")
    type: str = Field(default="string", description="Variable type")
    required: bool = Field(default=False, description="Whether variable is required")
    default_value: Optional[str] = Field(default=None, description="Default value")
    example_value: Optional[str] = Field(default=None, description="Example value")

class JinjaTemplate(BaseModel):
    """Represents a Jinja template definition."""
    name: str = Field(..., description="Template name")
    description: str = Field(default="", description="Template description")
    template_type: str = Field(default="text", description="Template type")
    template_content: str = Field(..., description="Template content")
    variables: List[JinjaVariable] = Field(default_factory=list, description="Template variables")
    version: str = Field(default="1.0", description="Template version")
    author: str = Field(default="", description="Template author")
    tags: List[str] = Field(default_factory=list, description="Template tags")

class JinjaValidationResult(BaseModel):
    """Result of template validation."""
    template_name: str = Field(..., description="Template name")
    valid: bool = Field(..., description="Whether template is valid")
    missing_variables: List[str] = Field(default_factory=list, description="Missing required variables")
    invalid_variables: List[str] = Field(default_factory=list, description="Invalid variables")
    syntax_errors: List[str] = Field(default_factory=list, description="Template syntax errors")

class JinjaRenderingResult(BaseModel):
    """Result of template rendering."""
    template_name: str = Field(..., description="Template name")
    rendered_content: str = Field(..., description="Rendered template content")
    variables_used: List[str] = Field(default_factory=list, description="Variables used in rendering")
    rendering_time_ms: float = Field(..., description="Rendering time in milliseconds")
    success: bool = Field(..., description="Whether rendering was successful")
    errors: List[str] = Field(default_factory=list, description="Rendering errors")

class JinjaResult(BaseModel):
    """Model for Jinja processing."""
    templates: List[JinjaTemplate] = Field(default_factory=list, description="Extracted templates")
    global_variables: List[JinjaVariable] = Field(default_factory=list, description="Global variables")
    validation_results: List[JinjaValidationResult] = Field(default_factory=list, description="Validation results")
    rendering_results: List[JinjaRenderingResult] = Field(default_factory=list, description="Rendering results")
    total_templates: int = Field(..., description="Total number of templates")
    total_variables: int = Field(..., description="Total number of variables")
    processing_time_ms: float = Field(..., description="Total processing time in milliseconds")

@processor_metadata(
    name="jinja_processor",
    version="1.0.0",
    capabilities=["jinja_parse", "template_rendering", "variable_substitution"],
    supported_formats=["xmljinja", "template"],
    author="AutoTel Team"
)
class JinjaProcessor(BaseProcessor):
    """
    Unified Jinja processor for AutoTel.
    Parses Jinja2 templates from XML and renders them with variable substitution.
    """
    
    def __init__(self, config: ProcessorConfig):
        super().__init__(config)
        self.jinja_env = Environment(
            trim_blocks=True,
            lstrip_blocks=True
        )
        # Add custom filters
        self.jinja_env.filters['json'] = lambda obj: json.dumps(obj, indent=2)
        self.jinja_env.filters['safe_json'] = lambda obj: json.dumps(obj, indent=2, ensure_ascii=False)
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse Jinja XML and render templates with variables (happy path only).
        Args:
            data: XML string, file path, or dict with 'xml' and 'variables' keys
        Returns:
            ProcessorResult with JinjaResult data
        """
        with create_processor_span("parse", "jinja_processor") as span:
            try:
                start_time = time.time()
                
                # Extract XML content and variables
                xml_content, variables = self._extract_input_data(data)
                
                # Parse templates
                templates = self._extract_templates(xml_content)
                global_variables = self._extract_global_variables(xml_content)
                
                # Validate templates
                validation_results = []
                for template in templates:
                    validation = self._validate_template(template, variables)
                    validation_results.append(validation)
                
                # Render templates
                rendering_results = []
                for template in templates:
                    rendering = self._render_template(template, variables)
                    rendering_results.append(rendering)
                
                processing_time_ms = (time.time() - start_time) * 1000
                
                jinja_result = JinjaResult(
                    templates=templates,
                    global_variables=global_variables,
                    validation_results=validation_results,
                    rendering_results=rendering_results,
                    total_templates=len(templates),
                    total_variables=len(global_variables) + sum(len(t.variables) for t in templates),
                    processing_time_ms=processing_time_ms
                )
                
                record_span_success(span, {
                    "templates": len(templates),
                    "global_variables": len(global_variables),
                    "validation_results": len(validation_results),
                    "rendering_results": len(rendering_results),
                    "processing_time_ms": processing_time_ms
                })
                
                return ProcessorResult.success_result(
                    data=jinja_result,
                    metadata={
                        "templates": len(templates),
                        "global_variables": len(global_variables),
                        "validation_results": len(validation_results),
                        "rendering_results": len(rendering_results),
                        "processing_time_ms": processing_time_ms
                    }
                )
            except Exception as e:
                record_span_error(span, e, {"input_type": type(data).__name__})
                return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})

    def _extract_input_data(self, data: Any) -> tuple[str, Dict[str, Any]]:
        """Extract XML content and variables from input data."""
        if isinstance(data, dict):
            xml_content = data.get('xml')
            variables = data.get('variables', {})
            if not xml_content:
                raise ValueError("Dictionary input must contain 'xml' key")
        else:
            xml_content = self._get_xml_content(data)
            variables = {}
        
        return xml_content, variables

    def _get_xml_content(self, data: Any) -> str:
        """Get XML content from string or file path."""
        if isinstance(data, str):
            if len(data) < 256 and Path(data).exists():
                return Path(data).read_text(encoding="utf-8")
            return data
        elif isinstance(data, Path):
            return data.read_text(encoding="utf-8")
        raise ValueError("Input must be XML string or file path")

    def _extract_namespace(self, root: ET.Element) -> str:
        """Extract Jinja namespace from XML root element."""
        # Look for jinja namespace in attributes
        for key, value in root.attrib.items():
            if key.startswith('xmlns:jinja'):
                return value
        
        # Default namespace
        return "http://autotel.ai/jinja"

    def _extract_global_variables(self, xml_content: str) -> List[JinjaVariable]:
        """Extract global variables from XML content."""
        try:
            root = ET.fromstring(xml_content)
            namespace = self._extract_namespace(root)
            variables = []
            
            global_vars_elem = root.find(f'.//{{{namespace}}}globalVariables')
            if global_vars_elem is not None:
                for var_elem in global_vars_elem.findall(f'{{{namespace}}}variable'):
                    variable = self._parse_variable(var_elem, namespace)
                    variables.append(variable)
            
            return variables
        except ET.ParseError:
            return []

    def _extract_templates(self, xml_content: str) -> List[JinjaTemplate]:
        """Extract templates from XML content."""
        try:
            root = ET.fromstring(xml_content)
            namespace = self._extract_namespace(root)
            templates = []
            
            for template_elem in root.findall(f'.//{{{namespace}}}template'):
                template = self._parse_template(template_elem, namespace)
                templates.append(template)
            
            return templates
        except ET.ParseError:
            return []

    def _parse_template(self, template_elem: ET.Element, namespace: str) -> JinjaTemplate:
        """Parse a single template from XML element."""
        # Extract basic attributes
        name = template_elem.get('name', '')
        description = template_elem.get('description', '')
        template_type = template_elem.get('type', 'text')
        version = template_elem.get('version', '1.0')
        author = template_elem.get('author', '')
        category = template_elem.get('category', '')
        
        # Parse tags
        tags = []
        tags_elem = template_elem.find(f'{{{namespace}}}tags')
        if tags_elem is not None:
            for tag_elem in tags_elem.findall(f'{{{namespace}}}tag'):
                tags.append(tag_elem.text or '')
        
        # Parse variables
        variables = []
        vars_elem = template_elem.find(f'{{{namespace}}}variables')
        if vars_elem is not None:
            for var_elem in vars_elem.findall(f'{{{namespace}}}variable'):
                variable = self._parse_variable(var_elem, namespace)
                variables.append(variable)
        
        # Parse content
        content_elem = template_elem.find(f'{{{namespace}}}content')
        content = self._extract_full_content(content_elem) if content_elem is not None else ''
        return JinjaTemplate(
            name=name,
            description=description,
            template_type=template_type,
            template_content=content,
            variables=variables,
            version=version,
            author=author,
            tags=tags
        )

    def _extract_full_content(self, elem: Optional[ET.Element]) -> str:
        """Extract all text content from an element including CDATA."""
        if elem is None:
            return ""
        parts = []
        if elem.text:
            parts.append(elem.text)
        for child in elem:
            parts.append(ET.tostring(child, encoding='unicode', method='xml'))
            if child.tail:
                parts.append(child.tail)
        return ''.join(parts)

    def _parse_variable(self, var_elem: ET.Element, namespace: str) -> JinjaVariable:
        """Parse a variable from XML element."""
        name = var_elem.get('name', '')
        description = var_elem.get('description', '')
        var_type = var_elem.get('type', 'string')
        required = var_elem.get('required', 'false').lower() == 'true'
        default = var_elem.get('default', '')
        example = var_elem.get('example', '')
        
        return JinjaVariable(
            name=name,
            description=description,
            type=var_type,
            required=required,
            default_value=default if default else None,
            example_value=example if example else None
        )

    def _validate_template(self, template: JinjaTemplate, variables: Dict[str, Any]) -> JinjaValidationResult:
        """Validate a template against provided variables."""
        missing_variables = []
        invalid_variables = []
        syntax_errors = []
        
        # Check for missing required variables
        for var in template.variables:
            if var.required and var.name not in variables:
                missing_variables.append(var.name)
        
        # Validate template syntax
        try:
            self.jinja_env.from_string(template.template_content)
        except TemplateError as e:
            syntax_errors.append(f"Template syntax error: {e}")
        
        return JinjaValidationResult(
            template_name=template.name,
            valid=len(missing_variables) == 0 and len(syntax_errors) == 0,
            missing_variables=missing_variables,
            invalid_variables=invalid_variables,
            syntax_errors=syntax_errors
        )

    def _render_template(self, template: JinjaTemplate, variables: Dict[str, Any]) -> JinjaRenderingResult:
        """Render a template with variables."""
        start_time = time.time()
        
        try:
            # Create Jinja2 template
            jinja_template = self.jinja_env.from_string(template.template_content)
            
            # Render template
            rendered_content = jinja_template.render(**variables)
            
            rendering_time_ms = (time.time() - start_time) * 1000      
            return JinjaRenderingResult(
                template_name=template.name,
                rendered_content=rendered_content,
                variables_used=list(variables.keys()),
                rendering_time_ms=rendering_time_ms,
                success=True
            )
            
        except (TemplateError, UndefinedError) as e:
            rendering_time_ms = (time.time() - start_time) * 1000      
            return JinjaRenderingResult(
                template_name=template.name,
                rendered_content="",         variables_used=[],
                rendering_time_ms=rendering_time_ms,
                success=False,
                errors=[str(e)]
            ) 