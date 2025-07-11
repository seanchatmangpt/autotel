"""
AutoTel LinkML Schema Processing
"""

import yaml
from typing import Dict, Any, List, Optional
from pydantic import BaseModel, Field

from ..core.framework import BaseModelWithTelemetry

class LinkMLProcessor:
    """Processes LinkML ontology into Pydantic models"""
    
    def __init__(self):
        self.generated_models = {}
        self.operations = {}
    
    def process_ontology(self, ontology_path: str):
        """Process ontology and generate models"""
        
        with open(ontology_path, 'r') as f:
            schema = yaml.safe_load(f)
        
        classes = schema.get('classes', {})
        enums = schema.get('enums', {})
        
        # Generate enums first
        enum_classes = {}
        for enum_name, enum_def in enums.items():
            enum_classes[enum_name] = self._create_enum(enum_name, enum_def)
        
        # Collect all model class names for forward references
        model_names = [name for name, class_def in classes.items() if self._is_data_model(class_def)]
        
        # Generate models
        for class_name, class_def in classes.items():
            if self._is_data_model(class_def):
                model_class = self._create_pydantic_model(class_name, class_def, enum_classes, model_names)
                self.generated_models[class_name] = model_class
            elif self._is_operation(class_def):
                self.operations[class_name] = class_def
        
        # Now update forward references
        namespace = {name: cls for name, cls in self.generated_models.items()}
        namespace.update(enum_classes)
        
        for model_class in self.generated_models.values():
            try:
                model_class.model_rebuild(globalns=namespace)
            except Exception as e:
                print(f"Warning: Could not rebuild model {model_class.__name__}: {e}")
                # Continue anyway
        
        print(f"✅ Generated {len(self.generated_models)} models and {len(self.operations)} operations")
        return self.generated_models
    
    def _is_data_model(self, class_def):
        """Check if class is a data model (noun)"""
        is_abstract = class_def.get('abstract', False)
        is_operation = 'Operation' in str(class_def.get('is_a', ''))
        return not is_abstract and not is_operation
    
    def _is_operation(self, class_def):
        """Check if class is an operation (verb)"""
        is_operation = 'Operation' in str(class_def.get('is_a', ''))
        operation_keywords = ['process', 'validate', 'check', 'create', 'send']
        class_name = class_def.get('name', '').lower()
        return is_operation or any(keyword in class_name for keyword in operation_keywords)
    
    def _create_enum(self, enum_name, enum_def):
        """Create Python enum from LinkML enum"""
        from enum import Enum
        
        values = enum_def.get('permissible_values', {})
        enum_dict = {key.upper(): key for key in values.keys()}
        
        return Enum(enum_name, enum_dict)
    
    def _create_pydantic_model(self, class_name, class_def, enum_classes, model_names):
        """Create Pydantic model from LinkML class"""
        fields = {}
        
        for attr_name, attr_def in class_def.get('attributes', {}).items():
            field_type, field_config = self._convert_attribute(attr_def, enum_classes, model_names)
            fields[attr_name] = (field_type, Field(**field_config))
        
        # Create model class
        model_class = type(class_name, (BaseModelWithTelemetry,), fields)
        return model_class
    
    def _convert_attribute(self, attr_def, enum_classes, model_names):
        """Convert LinkML attribute to Pydantic field"""
        from typing import Optional, List, Union
        
        attr_type = attr_def.get('range', 'string')
        required = not attr_def.get('optional', False)
        
        # Handle different types
        if attr_type in enum_classes:
            field_type = enum_classes[attr_type]
        elif attr_type in model_names:
            field_type = model_names[attr_type]
        elif attr_type == 'string':
            field_type = str
        elif attr_type == 'integer':
            field_type = int
        elif attr_type == 'float':
            field_type = float
        elif attr_type == 'boolean':
            field_type = bool
        else:
            field_type = str  # Default to string
        
        # Handle lists
        if attr_def.get('multivalued', False):
            field_type = List[field_type]
        
        # Handle optional fields
        if not required:
            field_type = Optional[field_type]
        
        field_config = {
            'description': attr_def.get('description', ''),
            'default': attr_def.get('default') if not required else None
        }
        
        return field_type, field_config 