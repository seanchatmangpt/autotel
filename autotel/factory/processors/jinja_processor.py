"""Jinja XML processor for AutoTel semantic execution pipeline.

This processor handles Jinja2 template parsing from XML definitions and rendering
with variable substitution using {{ }} syntax.
"""

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Union
from xml.etree import ElementTree as ET

from jinja2 import Environment, Template, TemplateError, UndefinedError

from ...schemas.jinja_types import (
    JinjaProcessingResult,
    JinjaRenderingResult,
    JinjaTemplate,
    JinjaTemplateDefinition,
    JinjaTemplateType,
    JinjaValidationResult,
    JinjaVariable,
    JinjaVariableType,
)


@dataclass
class JinjaProcessorError:
    """Error information for Jinja processing."""
    template_name: Optional[str]
    error_type: str
    message: str
    line_number: Optional[int] = None


class JinjaProcessor:
    """Processor for Jinja2 templates defined in XML format.
    
    Supports parsing Jinja2 templates from XML definitions and rendering them
    with variable substitution using {{ }} syntax.
    """
    
    def __init__(self):
        """Initialize the Jinja processor."""
        self.jinja_env = Environment(
            trim_blocks=True,
            lstrip_blocks=True
        )
        
        # Add custom filters
        self.jinja_env.filters['json'] = lambda obj: json.dumps(obj, indent=2)
        self.jinja_env.filters['safe_json'] = lambda obj: json.dumps(obj, indent=2, ensure_ascii=False)
    
    def parse_template_definitions(self, xml_content: str) -> JinjaTemplateDefinition:
        """Parse Jinja template definitions from XML content.
        
        Args:
            xml_content: XML string containing Jinja template definitions
            
        Returns:
            JinjaTemplateDefinition with parsed templates and variables
        """
        try:
            root = ET.fromstring(xml_content)
            
            # Extract namespace
            namespace = self._extract_namespace(root)
            
            # Parse global variables
            global_variables = self._parse_global_variables(root, namespace)
            
            # Parse templates
            templates = self._parse_templates(root, namespace)
            
            return JinjaTemplateDefinition(
                templates=templates,
                global_variables=global_variables
            )
            
        except ET.ParseError as e:
            raise ValueError(f"Invalid XML content: {e}")
    
    def process_templates(
        self, 
        xml_content: str, 
        variables: Dict[str, Any]
    ) -> JinjaProcessingResult:
        """Process Jinja templates from XML content with variables.
        
        Args:
            xml_content: XML string containing Jinja template definitions
            variables: Dictionary of variables for template rendering
            
        Returns:
            JinjaProcessingResult with validation and rendering results
        """
        start_time = time.time()
        
        try:
            # Parse template definitions
            template_definitions = self.parse_template_definitions(xml_content)
            
            # Validate templates
            validation_results = []
            for template in template_definitions.templates:
                validation = self._validate_template(template, variables)
                validation_results.append(validation)
            
            # Render templates
            rendering_results = []
            for template in template_definitions.templates:
                rendering = self._render_template(template, variables)
                rendering_results.append(rendering)
            
            processing_time_ms = (time.time() - start_time) * 1000
            
            return JinjaProcessingResult(
                template_definitions=template_definitions,
                validation_results=validation_results,
                rendering_results=rendering_results,
                processing_time_ms=processing_time_ms,
                success=all(r.success for r in rendering_results),
                errors=[]
            )
            
        except Exception as e:
            processing_time_ms = (time.time() - start_time) * 1000
            return JinjaProcessingResult(
                template_definitions=JinjaTemplateDefinition(),
                validation_results=[],
                rendering_results=[],
                processing_time_ms=processing_time_ms,
                success=False,
                errors=[str(e)]
            )
    
    def _extract_namespace(self, root: ET.Element) -> str:
        """Extract Jinja namespace from XML root element."""
        # Look for jinja namespace in attributes
        for key, value in root.attrib.items():
            if key.startswith('xmlns:jinja'):
                return value
        
        # Default namespace
        return "http://autotel.ai/jinja"
    
    def _parse_global_variables(self, root: ET.Element, namespace: str) -> List[JinjaVariable]:
        """Parse global variables from XML."""
        variables = []
        
        global_vars_elem = root.find(f'.//{{{namespace}}}globalVariables')
        if global_vars_elem is not None:
            for var_elem in global_vars_elem.findall(f'{{{namespace}}}variable'):
                variable = self._parse_variable(var_elem, namespace)
                variables.append(variable)
        
        return variables
    
    def _parse_templates(self, root: ET.Element, namespace: str) -> List[JinjaTemplate]:
        """Parse templates from XML."""
        templates = []
        
        for template_elem in root.findall(f'.//{{{namespace}}}template'):
            template = self._parse_template(template_elem, namespace)
            templates.append(template)
        
        return templates
    
    def _parse_template(self, template_elem: ET.Element, namespace: str) -> JinjaTemplate:
        """Parse a single template from XML element."""
        # Extract basic attributes
        name = template_elem.get('name', '')
        description = template_elem.get('description', '')
        template_type_str = template_elem.get('type', 'text')
        version = template_elem.get('version', '1.0')
        author = template_elem.get('author', '')
        category = template_elem.get('category', '')
        
        # Parse template type
        try:
            template_type = JinjaTemplateType(template_type_str)
        except ValueError:
            template_type = JinjaTemplateType.XML
        
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
        
        # Parse content (support CDATA and all text)
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
        """Recursively extract all text, CDATA, and tail from an element (for CDATA support)."""
        if elem is None:
            return ''
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
        var_type_str = var_elem.get('type', 'string')
        required = var_elem.get('required', 'false').lower() == 'true'
        default = var_elem.get('default', '')
        example = var_elem.get('example', '')
        
        # Parse variable type
        try:
            var_type = JinjaVariableType(var_type_str)
        except ValueError:
            var_type = JinjaVariableType.STRING
        
        return JinjaVariable(
            name=name,
            description=description,
            type=var_type,
            required=required,
            default_value=default,
            example_value=example
        )
    
    def _validate_template(self, template: JinjaTemplate, variables: Dict[str, Any]) -> JinjaValidationResult:
        """Validate a template against provided variables."""
        missing_variables = []
        invalid_variables = []
        type_errors = []
        
        # Check for missing required variables
        for var in template.variables:
            if var.required and var.name not in variables:
                missing_variables.append(var.name)
        
        # Check for invalid variable types
        for var in template.variables:
            if var.name in variables:
                value = variables[var.name]
                if not self._validate_variable_type(var, value):
                    invalid_variables.append(var.name)
        
        # Validate template syntax
        try:
            self.jinja_env.from_string(template.template_content)
        except TemplateError as e:
            type_errors.append(f"Template syntax error: {e}")
        
        return JinjaValidationResult(
            template_name=template.name,
            valid=len(missing_variables) == 0 and len(invalid_variables) == 0 and len(type_errors) == 0,
            missing_variables=missing_variables,
            invalid_variables=invalid_variables,
            type_errors=type_errors
        )
    
    def _validate_variable_type(self, variable: JinjaVariable, value: Any) -> bool:
        """Validate that a value matches the expected variable type."""
        if variable.type == JinjaVariableType.STRING:
            return isinstance(value, str)
        elif variable.type == JinjaVariableType.INTEGER:
            return isinstance(value, int)
        elif variable.type == JinjaVariableType.FLOAT:
            return isinstance(value, (int, float))
        elif variable.type == JinjaVariableType.BOOLEAN:
            return isinstance(value, bool)
        elif variable.type == JinjaVariableType.OBJECT:
            return isinstance(value, dict)
        elif variable.type == JinjaVariableType.ARRAY:
            return isinstance(value, list)
        else:
            return True  # Unknown type, assume valid
    
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
                rendered_content="",
                variables_used=[],
                rendering_time_ms=rendering_time_ms,
                success=False,
                errors=[str(e)]
            )
    
    def render_single_template(self, template_content: str, variables: Dict[str, Any]) -> str:
        """Render a single template string with variables.
        
        Args:
            template_content: Jinja2 template content
            variables: Dictionary of variables for rendering
            
        Returns:
            Rendered template string
        """
        template = self.jinja_env.from_string(template_content)
        return template.render(**variables)
    
    def validate_template_syntax(self, template_content: str) -> bool:
        """Validate Jinja2 template syntax.
        
        Args:
            template_content: Jinja2 template content
            
        Returns:
            True if syntax is valid, False otherwise
        """
        try:
            self.jinja_env.from_string(template_content)
            return True
        except TemplateError:
            return False 