# SpiffWorkflow Custom Definitions Tutorial

## Overview

This tutorial demonstrates how to create custom definitions in SpiffWorkflow from a Python perspective, based on the working patterns in AutoTel and spiff-example-cli. You'll learn how to extend SpiffWorkflow with custom task types, parsers, and objects.

## Table of Contents

1. [Custom Task Types](#custom-task-types)
2. [Custom Parsers](#custom-parsers)
3. [Custom Objects](#custom-objects)
4. [Custom Event Definitions](#custom-event-definitions)
5. [Custom Script Environments](#custom-script-environments)
6. [Integration Patterns](#integration-patterns)
7. [Complete Examples](#complete-examples)

## Custom Task Types

### 1. Basic Custom Task

Create a custom task by inheriting from existing SpiffWorkflow task classes:

```python
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask
from SpiffWorkflow.spiff.specs import SpiffBpmnTask

class CustomServiceTask(ServiceTask):
    """Custom service task with additional functionality"""
    
    def __init__(self, wf_spec, bpmn_id, **kwargs):
        super().__init__(wf_spec, bpmn_id, **kwargs)
        self.custom_property = kwargs.get('custom_property', 'default')
    
    def _run_hook(self, my_task):
        """Override the execution hook to add custom logic"""
        # Your custom logic here
        print(f"Executing custom service task: {self.bpmn_name}")
        
        # Call parent method to continue normal execution
        return super()._run_hook(my_task)
```

### 2. DSPy Service Task (AutoTel Pattern)

Based on the AutoTel implementation, here's how to create a DSPy-enabled service task:

```python
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask

class DspyServiceTask(ServiceTask):
    """Service task that can execute DSPy services"""
    
    def _run_hook(self, my_task):
        """Override to handle DSPy service execution"""
        ext = getattr(self, 'extensions', None)
        if ext and ext.get('dspy_service'):
            # Handle DSPy service tasks
            from autotel.utils.dspy_services import dspy_service
            dspy_info = ext['dspy_service']
            resolved_params = {k: my_task.get_data(v) for k, v in dspy_info['params'].items()}
            result = dspy_service(dspy_info['service'], **resolved_params)
            
            # Set each output field on the task if result is a dict
            if isinstance(result, dict):
                my_task.set_data(**result)
            elif dspy_info['result']:
                my_task.set_data(**{dspy_info['result']: result})
        
        # Call the parent method to continue normal execution
        return super()._run_hook(my_task)
```

### 3. Custom Start Event

Based on the spiff-example-cli pattern:

```python
from SpiffWorkflow.bpmn.specs.event_definitions import NoneEventDefinition
from SpiffWorkflow.bpmn.specs.event_definitions.timer import TimerEventDefinition
from SpiffWorkflow.bpmn.specs.mixins import StartEventMixin
from SpiffWorkflow.spiff.specs import SpiffBpmnTask

class CustomStartEvent(StartEventMixin, SpiffBpmnTask):
    """Custom start event with timer support"""

    def __init__(self, wf_spec, bpmn_id, event_definition, **kwargs):
        if isinstance(event_definition, TimerEventDefinition):
            super().__init__(wf_spec, bpmn_id, NoneEventDefinition(), **kwargs)
            self.timer_event = event_definition
        else:
            super().__init__(wf_spec, bpmn_id, event_definition, **kwargs)
            self.timer_event = None
```

## Custom Parsers

### 1. Basic Task Parser

Create a custom parser to handle XML extensions:

```python
from SpiffWorkflow.bpmn.parser.TaskParser import TaskParser

class CustomTaskParser(TaskParser):
    """Custom parser for service tasks with extensions"""
    
    def parse_extensions(self):
        """Parse custom extensions from XML"""
        extensions = super().parse_extensions()
        
        # Parse custom namespace
        custom_ns = 'http://yourcompany.com/custom'
        custom_elements = self.node.xpath('.//custom:service', namespaces={'custom': custom_ns})
        
        if custom_elements:
            service = custom_elements[0]
            service_name = service.attrib['name']
            result_var = service.attrib.get('result')
            params = {}
            
            for param in service.xpath('./custom:param', namespaces={'custom': custom_ns}):
                param_name = param.attrib['name']
                param_value = param.attrib['value']
                params[param_name] = param_value
            
            extensions['custom_service'] = {
                'service': service_name,
                'result': result_var,
                'params': params,
            }
        
        return extensions
```

### 2. DSPy Task Parser (AutoTel Pattern)

```python
from SpiffWorkflow.bpmn.parser.TaskParser import TaskParser

class DspyTaskParser(TaskParser):
    """Parser for DSPy service tasks"""
    
    def parse_extensions(self):
        extensions = super().parse_extensions()
        dspy_ns = 'http://autotel.ai/dspy'
        dspy_services = self.node.xpath('.//dspy:service', namespaces={'dspy': dspy_ns})
        
        if dspy_services:
            service = dspy_services[0]
            service_name = service.attrib['name']
            result_var = service.attrib.get('result')
            params = {}
            
            for param in service.xpath('./dspy:param', namespaces={'dspy': dspy_ns}):
                param_name = param.attrib['name']
                param_value = param.attrib['value']
                params[param_name] = param_value
            
            extensions['dspy_service'] = {
                'service': service_name,
                'result': result_var,
                'params': params,
            }
        
        return extensions
```

### 3. Custom BPMN Parser

Extend the base parser to use your custom parsers:

```python
from SpiffWorkflow.spiff.parser import SpiffBpmnParser
from SpiffWorkflow.bpmn.parser.util import full_tag

class CustomBpmnParser(SpiffBpmnParser):
    """Custom BPMN parser with custom task types"""
    
    # Override parser classes for custom task types
    OVERRIDE_PARSER_CLASSES = SpiffBpmnParser.OVERRIDE_PARSER_CLASSES.copy()
    OVERRIDE_PARSER_CLASSES[full_tag('serviceTask')] = (CustomTaskParser, CustomServiceTask)
    OVERRIDE_PARSER_CLASSES[full_tag('startEvent')] = (StartEventParser, CustomStartEvent)
    
    def __init__(self):
        super().__init__()
        self.custom_definitions = {}
    
    def add_bpmn_xml(self, bpmn, filename=None):
        """Override to add custom parsing logic"""
        # Your custom parsing logic here
        self._parse_custom_definitions(bpmn)
        
        # Call parent method
        super().add_bpmn_xml(bpmn, filename)
    
    def _parse_custom_definitions(self, bpmn):
        """Parse custom definitions from BPMN XML"""
        # Implementation here
        pass
```

## Custom Objects

### 1. Custom Data Objects

Based on the spiff-example-cli pattern:

```python
from collections import namedtuple
from dataclasses import dataclass
from typing import Dict, Any

# Option 1: Using namedtuple (spiff-example-cli pattern)
ProductInfo = namedtuple('ProductInfo', ['color', 'size', 'style', 'price'])

# Option 2: Using dataclass (modern approach)
@dataclass
class CustomerData:
    """Custom customer data object"""
    customer_id: str
    name: str
    email: str
    preferences: Dict[str, Any]
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization"""
        return {
            'customer_id': self.customer_id,
            'name': self.name,
            'email': self.email,
            'preferences': self.preferences,
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'CustomerData':
        """Create from dictionary"""
        return cls(**data)
```

### 2. Serialization Functions

```python
def customer_data_to_dict(obj):
    """Convert CustomerData to dictionary"""
    return obj.to_dict()

def customer_data_from_dict(dct):
    """Create CustomerData from dictionary"""
    return CustomerData.from_dict(dct)

# For namedtuple objects
def product_info_to_dict(obj):
    """Convert ProductInfo to dictionary"""
    return {
        'color': obj.color,
        'size': obj.size,
        'style': obj.style,
        'price': obj.price,
    }

def product_info_from_dict(dct):
    """Create ProductInfo from dictionary"""
    return ProductInfo(**dct)
```

## Custom Event Definitions

### 1. Custom Event Definition

```python
from SpiffWorkflow.bpmn.specs.event_definitions import EventDefinition

class CustomEventDefinition(EventDefinition):
    """Custom event definition"""
    
    def __init__(self, event_type: str, **kwargs):
        super().__init__()
        self.event_type = event_type
        self.custom_properties = kwargs
    
    def fires(self, my_task, event_definition, correlation_properties=None):
        """Determine if this event should fire"""
        # Your custom logic here
        return True
    
    def update_task_data(self, my_task):
        """Update task data when event fires"""
        my_task.set_data(event_type=self.event_type, **self.custom_properties)
```

### 2. Event Parser

```python
from SpiffWorkflow.spiff.parser.event_parsers import StartEventParser

class CustomStartEventParser(StartEventParser):
    """Parser for custom start events"""
    
    def create_task(self, spec_class, bpmn_id, event_definition, **kwargs):
        """Create custom start event task"""
        return CustomStartEvent(self.spec, bpmn_id, event_definition, **kwargs)
```

## Custom Script Environments

### 1. Basic Script Environment

```python
from SpiffWorkflow.bpmn.script_engine import TaskDataEnvironment

class CustomScriptEnvironment(TaskDataEnvironment):
    """Custom script environment with additional functions"""
    
    def __init__(self):
        super().__init__({
            'custom_function': self.custom_function,
            'datetime': datetime,
            'json': json,
        })
    
    def custom_function(self, data):
        """Custom function available in scripts"""
        return f"Processed: {data}"
```

### 2. Service Task Environment (spiff-example-cli pattern)

```python
from SpiffWorkflow.bpmn.script_engine import TaskDataEnvironment
import json

class ServiceTaskEnvironment(TaskDataEnvironment):
    """Environment for service tasks with custom service calls"""

    def __init__(self):
        super().__init__({
            'custom_service': self.call_service,
            'datetime': datetime,
        })

    def call_service(self, task_data, operation_name, operation_params):
        """Handle custom service calls"""
        if operation_name == 'lookup_product_info':
            product_info = lookup_product_info(operation_params['product_name']['value'])
            result = product_info_to_dict(product_info)
        elif operation_name == 'custom_operation':
            result = self.custom_operation(operation_params)
        else:
            raise Exception(f"Unknown service: {operation_name}")
        
        return json.dumps(result)
    
    def custom_operation(self, params):
        """Custom operation implementation"""
        # Your custom logic here
        return {"result": "success"}
```

## Integration Patterns

### 1. Complete Engine Setup

```python
import logging
from SpiffWorkflow.spiff.parser import SpiffBpmnParser
from SpiffWorkflow.spiff.specs.defaults import UserTask, ManualTask
from SpiffWorkflow.spiff.serializer.config import SPIFF_CONFIG
from SpiffWorkflow.bpmn.specs.mixins.none_task import NoneTask
from SpiffWorkflow.bpmn.script_engine import TaskDataEnvironment

# Configure logging
logger = logging.getLogger('custom_engine')
logger.setLevel(logging.INFO)

# Initialize serializer
dirname = 'wfdata'
FileSerializer.initialize(dirname)

# Configure registry with custom objects
registry = FileSerializer.configure(SPIFF_CONFIG)
registry.register(CustomerData, customer_data_to_dict, customer_data_from_dict)
registry.register(ProductInfo, product_info_to_dict, product_info_from_dict)

serializer = FileSerializer(dirname, registry=registry)

# Create custom parser
parser = CustomBpmnParser()

# Configure task handlers
handlers = {
    UserTask: UserTaskHandler,
    ManualTask: ManualTaskHandler,
    NoneTask: ManualTaskHandler,
    CustomServiceTask: CustomServiceTaskHandler,
}

# Create script environment
script_env = CustomScriptEnvironment()

# Create engine
engine = BpmnEngine(parser, serializer, script_env)
```

### 2. Serialization Configuration

```python
from SpiffWorkflow.spiff.serializer.config import SPIFF_CONFIG
from SpiffWorkflow.spiff.serializer.task_spec import SpiffBpmnTaskConverter

class CustomServiceTaskConverter(SpiffBpmnTaskConverter):
    """Converter for custom service tasks"""
    
    def to_dict(self, spec):
        dct = super().to_dict(spec)
        dct['custom_property'] = spec.custom_property
        return dct

    def from_dict(self, dct):
        spec = super().from_dict(dct)
        spec.custom_property = dct.get('custom_property', 'default')
        return spec

# Register converter
SPIFF_CONFIG[CustomServiceTask] = CustomServiceTaskConverter
```

## Complete Examples

### Example 1: Custom Analytics Service Task

```python
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask
from SpiffWorkflow.bpmn.parser.TaskParser import TaskParser
from SpiffWorkflow.bpmn.parser.util import full_tag

class AnalyticsServiceTask(ServiceTask):
    """Service task for analytics processing"""
    
    def __init__(self, wf_spec, bpmn_id, **kwargs):
        super().__init__(wf_spec, bpmn_id, **kwargs)
        self.analytics_type = kwargs.get('analytics_type', 'basic')
    
    def _run_hook(self, my_task):
        """Execute analytics processing"""
        data = my_task.get_data('input_data')
        analytics_type = self.analytics_type
        
        # Process analytics
        if analytics_type == 'sentiment':
            result = self._analyze_sentiment(data)
        elif analytics_type == 'classification':
            result = self._classify_data(data)
        else:
            result = self._basic_analysis(data)
        
        my_task.set_data(analytics_result=result)
        return super()._run_hook(my_task)
    
    def _analyze_sentiment(self, data):
        # Sentiment analysis implementation
        return {"sentiment": "positive", "confidence": 0.85}
    
    def _classify_data(self, data):
        # Classification implementation
        return {"category": "feedback", "priority": "high"}
    
    def _basic_analysis(self, data):
        # Basic analysis implementation
        return {"processed": True, "count": len(data)}

class AnalyticsTaskParser(TaskParser):
    """Parser for analytics service tasks"""
    
    def parse_extensions(self):
        extensions = super().parse_extensions()
        analytics_ns = 'http://yourcompany.com/analytics'
        
        analytics_elements = self.node.xpath('.//analytics:config', namespaces={'analytics': analytics_ns})
        if analytics_elements:
            config = analytics_elements[0]
            extensions['analytics_type'] = config.attrib.get('type', 'basic')
        
        return extensions

# Integration
class CustomBpmnParser(SpiffBpmnParser):
    OVERRIDE_PARSER_CLASSES = SpiffBpmnParser.OVERRIDE_PARSER_CLASSES.copy()
    OVERRIDE_PARSER_CLASSES[full_tag('serviceTask')] = (AnalyticsTaskParser, AnalyticsServiceTask)
```

### Example 2: Custom Decision Task

```python
from SpiffWorkflow.bpmn.specs.defaults import BusinessRuleTask
from SpiffWorkflow.dmn.specs import BusinessRuleTaskMixin

class CustomDecisionTask(BusinessRuleTaskMixin, BusinessRuleTask):
    """Custom decision task with enhanced logic"""
    
    def __init__(self, wf_spec, bpmn_id, **kwargs):
        super().__init__(wf_spec, bpmn_id, **kwargs)
        self.decision_logic = kwargs.get('decision_logic', {})
    
    def _run_hook(self, my_task):
        """Execute custom decision logic"""
        # Get input data
        input_data = my_task.get_data()
        
        # Apply custom decision logic
        decision_result = self._apply_decision_logic(input_data)
        
        # Set decision result
        my_task.set_data(decision_result=decision_result)
        
        return super()._run_hook(my_task)
    
    def _apply_decision_logic(self, input_data):
        """Apply custom decision logic"""
        # Your custom decision logic here
        if input_data.get('score', 0) > 80:
            return {"action": "approve", "reason": "high_score"}
        elif input_data.get('score', 0) > 60:
            return {"action": "review", "reason": "medium_score"}
        else:
            return {"action": "reject", "reason": "low_score"}
```

### Example 3: Custom Object with Serialization

```python
from dataclasses import dataclass
from typing import Dict, Any, List
import json

@dataclass
class WorkflowContext:
    """Custom workflow context object"""
    workflow_id: str
    user_id: str
    metadata: Dict[str, Any]
    history: List[Dict[str, Any]]
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary"""
        return {
            'workflow_id': self.workflow_id,
            'user_id': self.user_id,
            'metadata': self.metadata,
            'history': self.history,
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'WorkflowContext':
        """Create from dictionary"""
        return cls(**data)
    
    def add_history_entry(self, entry: Dict[str, Any]):
        """Add entry to history"""
        self.history.append(entry)

# Serialization functions
def workflow_context_to_dict(obj):
    return obj.to_dict()

def workflow_context_from_dict(dct):
    return WorkflowContext.from_dict(dct)

# Usage in engine setup
registry.register(WorkflowContext, workflow_context_to_dict, workflow_context_from_dict)
```

## Best Practices

### 1. Naming Conventions
- Use descriptive names for custom classes
- Follow SpiffWorkflow naming patterns
- Use consistent prefixes for related functionality

### 2. Error Handling
```python
def _run_hook(self, my_task):
    try:
        # Your custom logic
        result = self.custom_operation(my_task.get_data())
        my_task.set_data(result=result)
    except Exception as e:
        logger.error(f"Error in custom task {self.bpmn_name}: {e}")
        my_task.set_data(error=str(e))
        raise
    return super()._run_hook(my_task)
```

### 3. Logging and Debugging
```python
import logging

logger = logging.getLogger(__name__)

class CustomTask(ServiceTask):
    def _run_hook(self, my_task):
        logger.info(f"Executing custom task: {self.bpmn_name}")
        logger.debug(f"Task data: {my_task.get_data()}")
        
        # Your logic here
        
        logger.info(f"Completed custom task: {self.bpmn_name}")
        return super()._run_hook(my_task)
```

### 4. Testing Custom Definitions
```python
import unittest
from SpiffWorkflow.bpmn import BpmnWorkflow

class TestCustomTask(unittest.TestCase):
    def setUp(self):
        self.parser = CustomBpmnParser()
        self.parser.add_bpmn_file('test_workflow.bpmn')
    
    def test_custom_task_execution(self):
        spec = self.parser.get_spec('test_process')
        workflow = BpmnWorkflow(spec)
        workflow.set_data(input_data={'test': 'value'})
        
        workflow.run_all()
        
        result = workflow.get_data('result')
        self.assertIsNotNone(result)
```

## Conclusion

This tutorial covers the essential patterns for creating custom definitions in SpiffWorkflow from a Python perspective. The key points are:

1. **Inherit from existing SpiffWorkflow classes** for custom task types
2. **Override `_run_hook`** to add custom execution logic
3. **Create custom parsers** to handle XML extensions
4. **Register custom objects** with the serializer registry
5. **Use the `OVERRIDE_PARSER_CLASSES`** mechanism to integrate custom parsers
6. **Follow established patterns** from spiff-example-cli and AutoTel

By following these patterns, you can extend SpiffWorkflow with custom functionality while maintaining compatibility with the existing framework. 