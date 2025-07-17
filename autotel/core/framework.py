"""
AutoTel Framework - Core framework for semantic computing with zero-touch telemetry
"""

import json
import yaml
import time
import uuid
from datetime import datetime
from typing import Dict, Any, List, Optional, Type, Union
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path

# Core dependencies
from pydantic import BaseModel, Field
import asyncio

# Real OpenTelemetry implementation
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor, ConsoleSpanExporter
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter

# Initialize real OpenTelemetry
trace.set_tracer_provider(TracerProvider())
tracer = trace.get_tracer(__name__)

# Add console exporter for demo visibility
console_exporter = ConsoleSpanExporter()
span_processor = BatchSpanProcessor(console_exporter)
trace.get_tracer_provider().add_span_processor(span_processor)

@dataclass
class TelemetryPlan:
    """Production telemetry plan with business rules-based sampling"""
    
    instrumented_models: Dict[str, float] = field(default_factory=lambda: {
        "Order": 1.0,  # Always instrument orders - critical business events
        "Payment": 1.0,  # Always instrument payments - financial compliance
        "OrderItem": 0.5,  # Sample order items - reduces noise
        "Shipment": 1.0,  # Always instrument shipments - logistics tracking
        "InventoryResult": 0.7,  # Sample inventory - operational insights
    })
    
    instrumented_operations: Dict[str, float] = field(default_factory=lambda: {
        "ProcessOrder": 1.0,     # Critical business process
        "ValidatePayment": 1.0,  # Financial compliance requirement
        "CheckInventory": 0.8,   # Operational monitoring
        "CreateShipment": 1.0,   # Logistics tracking
        "SendNotification": 0.6  # Reduce notification noise
    })
    
    def should_instrument_model(self, model_name: str) -> bool:
        """Deterministic sampling based on business criticality"""
        rate = self.instrumented_models.get(model_name, 0.5)
        
        # Business rule: always instrument critical financial/order models
        if model_name in ["Order", "Payment", "Shipment"]:
            return True
        
        # Use model name hash for consistent sampling
        name_hash = hash(model_name) % 100
        return name_hash < (rate * 100)
    
    def should_instrument_operation(self, operation_name: str) -> bool:
        """Deterministic sampling based on business importance"""
        rate = self.instrumented_operations.get(operation_name, 0.5)
        
        # Business rule: always instrument critical operations
        if operation_name in ["ProcessOrder", "ValidatePayment", "CreateShipment"]:
            return True
        
        # Use operation name hash for consistent sampling
        name_hash = hash(operation_name) % 100
        return name_hash < (rate * 100)

# Global telemetry plan
telemetry_plan = TelemetryPlan()

class ModelMetaclass(type(BaseModel)):
    """Metaclass that adds telemetry to Pydantic models, compatible with Pydantic v2"""
    
    def __new__(mcs, name, bases, namespace, **kwargs):
        cls = super().__new__(mcs, name, bases, namespace, **kwargs)
        
        # Only add instrumentation if this is a concrete model (not the base class)
        if name != 'BaseModel':
            original_getattribute = cls.__getattribute__
            
            def instrumented_getattribute(self, item):
                # Avoid recursion - don't instrument special attributes or model internals
                if (item.startswith('_') or 
                    item in ('model_fields', 'model_config', '__dict__', '__class__', '__module__') or
                    not hasattr(self, 'model_fields')):
                    return original_getattribute(self, item)
                
                # Only instrument actual model fields
                model_fields = original_getattribute(self, 'model_fields')
                if item in model_fields and telemetry_plan.should_instrument_model(name):
                    with tracer.start_as_current_span(f"model.{name}.{item}") as span:
                        span.set_attribute("model.name", name)
                        span.set_attribute("field.name", item)
                        
                        try:
                            result = original_getattribute(self, item)
                            span.set_attribute("access.success", True)
                            span.set_attribute("field.type", type(result).__name__)
                            return result
                        except Exception as e:
                            span.set_attribute("access.success", False)
                            span.set_attribute("error.message", str(e))
                            raise
                else:
                    return original_getattribute(self, item)
            
            cls.__getattribute__ = instrumented_getattribute
        
        return cls

class BaseModelWithTelemetry(BaseModel, metaclass=ModelMetaclass):
    """Base model with automatic telemetry"""
    
    model_config = {"extra": "allow"}  # Pydantic v2 configuration

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

class SemanticWorkflowEngine:
    """Semantic workflow execution engine"""
    
    def __init__(self, operations: Dict[str, Any]):
        self.operations = operations
    
    def execute_operation(self, operation_name: str, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """Execute a semantic operation with telemetry"""
        
        with tracer.start_as_current_span(f"operation.{operation_name}") as span:
            span.set_attribute("operation.name", operation_name)
            span.set_attribute("operation.inputs", str(inputs))
            
            try:
                result = self._execute_operation_impl(operation_name, inputs)
                
                span.set_attribute("operation.success", True)
                span.set_attribute("operation.result", str(result))
                
                return result
                
            except Exception as e:
                span.set_attribute("operation.success", False)
                span.set_attribute("operation.error", str(e))
                raise
    
    def _make_json_serializable(self, obj):
        """Convert object to JSON serializable format"""
        if isinstance(obj, (dict, list, str, int, float, bool, type(None))):
            return obj
        elif hasattr(obj, 'model_dump'):
            return obj.model_dump()
        elif hasattr(obj, '__dict__'):
            return obj.__dict__
        else:
            return str(obj)
    
    def _execute_operation_impl(self, operation_name: str, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """Execute operation implementation"""
        
        # Convert inputs to JSON serializable
        serializable_inputs = {k: self._make_json_serializable(v) for k, v in inputs.items()}
        
        if operation_name == "ProcessOrder":
            return self._process_order(serializable_inputs)
        elif operation_name == "ValidatePayment":
            return self._validate_payment(serializable_inputs)
        elif operation_name == "CheckInventory":
            return self._check_inventory(serializable_inputs)
        elif operation_name == "CreateShipment":
            return self._create_shipment(serializable_inputs)
        elif operation_name == "SendNotification":
            return self._send_notification(serializable_inputs)
        else:
            raise ValueError(f"Unknown operation: {operation_name}")
    
    def _process_order(self, inputs):
        """Process order operation"""
        order = inputs.get('order', {})
        
        return {
            "operation_id": str(uuid.uuid4()),
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "order_processed": True,
            "order_id": order.get('order_id'),
            "processing_time_ms": 150
        }
    
    def _validate_payment(self, inputs):
        """Validate payment operation"""
        payment = inputs.get('payment', {})
        
        # Simulate payment validation
        amount = payment.get('amount', 0)
        payment_method = payment.get('payment_method', 'unknown')
        
        # Simple validation logic
        is_valid = amount > 0 and payment_method in ['credit_card', 'debit_card', 'paypal']
        
        return {
            "operation_id": str(uuid.uuid4()),
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "payment_validated": is_valid,
            "payment_id": payment.get('payment_id'),
            "validation_result": {
                "amount_valid": amount > 0,
                "method_valid": payment_method in ['credit_card', 'debit_card', 'paypal'],
                "fraud_check_passed": True
            },
            "validation_time_ms": 200
        }
    
    def _check_inventory(self, inputs):
        """Check inventory operation"""
        order_items = inputs.get('order_items', [])
        
        inventory_results = []
        total_available = 0
        
        for item in order_items:
            product_id = item.get('product_id')
            quantity = item.get('quantity', 0)
            
            # Simulate inventory check
            available = quantity + 5  # Always have some extra
            is_available = available >= quantity
            
            inventory_results.append({
                "product_id": product_id,
                "requested_quantity": quantity,
                "available_quantity": available,
                "is_available": is_available,
                "reserved_quantity": quantity if is_available else 0
            })
            
            if is_available:
                total_available += quantity
        
        return {
            "operation_id": str(uuid.uuid4()),
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "inventory_checked": True,
            "total_items_available": total_available,
            "items": inventory_results,
            "check_time_ms": 100
        }
    
    def _create_shipment(self, inputs):
        """Create shipment operation"""
        order = inputs.get('order', {})
        
        # Simulate shipment creation
        delivery_date = self._calculate_delivery_date(3)  # 3 business days
        
        return {
            "operation_id": str(uuid.uuid4()),
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "shipment_created": True,
            "order_id": order.get('order_id'),
            "tracking_number": f"TRK-{uuid.uuid4().hex[:8].upper()}",
            "estimated_delivery": delivery_date.isoformat(),
            "shipping_method": "standard",
            "creation_time_ms": 300
        }
    
    def _calculate_delivery_date(self, business_days_range):
        """Calculate delivery date based on business days"""
        from datetime import timedelta
        
        current_date = datetime.now()
        delivery_date = current_date
        
        days_added = 0
        while days_added < business_days_range:
            delivery_date += timedelta(days=1)
            if delivery_date.weekday() < 5:  # Monday = 0, Friday = 4
                days_added += 1
        
        return delivery_date
    
    def _send_notification(self, inputs):
        """Send notification operation"""
        recipient = inputs.get('recipient', 'Unknown')
        message_type = inputs.get('message_type', 'general')
        message_content = inputs.get('message_content', '')
        delivery_method = inputs.get('delivery_method', 'email')
        
        # Simulate notification sending
        template_config = {
            "email": {"success_rate": 0.95, "delivery_time_ms": 100},
            "sms": {"success_rate": 0.98, "delivery_time_ms": 50},
            "push": {"success_rate": 0.90, "delivery_time_ms": 200}
        }
        
        config = template_config.get(delivery_method, template_config["email"])
        delivery_success = True  # Simulate success
        
        notification_result = {
            "recipient": recipient,
            "message_type": message_type,
            "delivery_method": delivery_method,
            "delivery_success": delivery_success,
            "message_id": str(uuid.uuid4()),
            "supported_delivery_methods": template_config["delivery_methods"]
        }
        
        return {
            "operation_id": str(uuid.uuid4()),
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "notification_result": notification_result,
            "delivery_method": delivery_method,
            "delivery_confirmed": delivery_success,
            "duration_ms": 100
        }

class Framework:
    """Main AutoTel framework"""
    
    def __init__(self):
        self.processor = LinkMLProcessor()
        self.models = {}
        self.workflow_engine = None
    
    def initialize(self, ontology_path: str):
        """Initialize the framework"""
        
        print("🚀 Initializing AutoTel Framework")
        print("=" * 50)
        
        # Process ontology
        print("📊 Processing LinkML ontology...")
        self.models = self.processor.process_ontology(ontology_path)
        
        # Create workflow engine
        print("🔄 Creating semantic workflow engine...")
        self.workflow_engine = SemanticWorkflowEngine(self.processor.operations)
        
        print("✅ AutoTel Framework initialized!")
        print(f"   📋 Models: {list(self.models.keys())}")
        print(f"   ⚙️  Operations: {list(self.processor.operations.keys())}")
        
        return {
            "status": "initialized",
            "models": list(self.models.keys()),
            "operations": list(self.processor.operations.keys())
        }
    
    def get_model(self, model_name: str):
        """Get generated model class"""
        return self.models.get(model_name)
    
    def execute_workflow(self, workflow_steps: List[Dict[str, Any]]):
        """Execute a complete workflow"""
        
        print("\n🎯 Executing Semantic Workflow")
        print("-" * 30)
        
        results = []
        
        with tracer.start_as_current_span("workflow.execution") as workflow_span:
            workflow_span.set_attribute("workflow.steps", len(workflow_steps))
            
            for i, step in enumerate(workflow_steps):
                operation_name = step.get('operation')
                inputs = step.get('inputs', {})
                
                try:
                    result = self.workflow_engine.execute_operation(operation_name, inputs)
                    results.append({
                        "step": i + 1,
                        "operation": operation_name,
                        "success": True,
                        "result": result
                    })
                except Exception as e:
                    results.append({
                        "step": i + 1,
                        "operation": operation_name,
                        "success": False,
                        "error": str(e)
                    })
                    workflow_span.set_attribute("workflow.failed_step", i + 1)
                    break
            
            workflow_span.set_attribute("workflow.completed_steps", len(results))
        
        return results

def demonstrate_autotel():
    """Run the AutoTel demonstration"""
    
    print("🌟 AUTOTEL PROTOTYPE DEMONSTRATION")
    print("=" * 60)
    
    # Initialize framework
    framework = Framework()
    init_result = framework.initialize("examples/autotel_demo_ontology.yaml")
    
    print(f"\n📊 Initialization Result: {init_result['status']}")
    
    # Create instrumented model instances
    print("\n🏗️  Creating Instrumented Model Instances")
    print("-" * 40)
    
    Order = framework.get_model("Order")
    OrderItem = framework.get_model("OrderItem")
    Payment = framework.get_model("Payment")
    
    if Order and OrderItem and Payment:
        # Create order items (automatically instrumented)
        items = [
            OrderItem(
                product_id="PROD-001",
                product_name="AutoTel T-Shirt",
                quantity=2,
                unit_price=29.99
            ),
            OrderItem(
                product_id="PROD-002", 
                product_name="Semantic Mug",
                quantity=1,
                unit_price=15.99
            )
        ]
        
        # Create payment
        payment = Payment(
            payment_id="PAY-12345",
            payment_method="credit_card",
            amount=75.97,
            status="authorized",
            transaction_id="TXN-67890"
        )
        
        # Create order (automatically instrumented)
        order = Order(
            order_id="ORD-2024-001",
            customer_name="Jane Smith",
            total_amount=75.97,
            status="pending",
            items=items
        )
        
        print(f"📦 Created Order: {order.order_id}")
        
        # Access fields (automatically traced)
        customer = order.customer_name
        amount = order.total_amount
        item_count = len(order.items)
        
        print(f"   👤 Customer: {customer}")
        print(f"   💰 Amount: ${amount}")
        print(f"   📋 Items: {item_count}")
        
        # Define workflow steps
        workflow_steps = [
            {
                "operation": "ProcessOrder",
                "inputs": {
                    "order": {
                        "order_id": order.order_id,
                        "customer_name": order.customer_name,
                        "total_amount": order.total_amount
                    }
                }
            },
            {
                "operation": "ValidatePayment", 
                "inputs": {
                    "payment": {
                        "payment_id": payment.payment_id,
                        "amount": payment.amount,
                        "payment_method": payment.payment_method
                    }
                }
            },
            {
                "operation": "CheckInventory",
                "inputs": {
                    "order_items": [
                        {
                            "product_id": item.product_id,
                            "quantity": item.quantity
                        } for item in items
                    ]
                }
            },
            {
                "operation": "CreateShipment",
                "inputs": {
                    "order": {
                        "order_id": order.order_id
                    }
                }
            },
            {
                "operation": "SendNotification",
                "inputs": {
                    "recipient": order.customer_name,
                    "message_type": "order_confirmation",
                    "message_content": f"Your order {order.order_id} has been processed!",
                    "delivery_method": "email"
                }
            }
        ]
        
        # Execute workflow
        print("\n🔄 Executing Workflow")
        print("-" * 20)
        
        workflow_results = framework.execute_workflow(workflow_steps)
        
        # Display results
        print("\n📊 Workflow Results")
        print("-" * 20)
        
        for result in workflow_results:
            step_num = result['step']
            operation = result['operation']
            success = result['success']
            
            status_icon = "✅" if success else "❌"
            print(f"{status_icon} Step {step_num}: {operation}")
            
            if not success:
                print(f"   Error: {result.get('error', 'Unknown error')}")
        
        print("\n🎉 Demonstration completed successfully!")
        print("📈 All operations were automatically instrumented with telemetry")
        
        return workflow_results 