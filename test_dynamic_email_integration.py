#!/usr/bin/env python3
"""
Test Dynamic DMN + DSPy + Jinja2 Email Integration
Demonstrates how DMN can choose based on DSPy data and generate dynamic email explanations.
"""

import json
import time
from datetime import datetime
from typing import Dict, Any

# Import our workflow execution system
from autotel.workflows.spiff import run_dspy_bpmn_process

def main():
    """Main test function - provides input values for XML-defined DSPy signatures"""
    
    # Test scenarios with different risk levels
    test_scenarios = [
        {
            "name": "Low Risk Transaction",
            "data": {
                "customer_name": "John Smith",
                "customer_email": "john.smith@example.com",
                "transaction_amount": 150.00,
                "transaction_type": "purchase",
                "merchant_name": "Amazon.com",
                "risk_score": 15,
                "fraud_probability": 2.5
            }
        },
        {
            "name": "Medium Risk Transaction",
            "data": {
                "customer_name": "Jane Doe",
                "customer_email": "jane.doe@example.com",
                "transaction_amount": 2500.00,
                "transaction_type": "transfer",
                "merchant_name": "International Bank",
                "risk_score": 55,
                "fraud_probability": 12.0
            }
        },
        {
            "name": "High Risk Transaction",
            "data": {
                "customer_name": "Bob Wilson",
                "customer_email": "bob.wilson@example.com",
                "transaction_amount": 15000.00,
                "transaction_type": "withdrawal",
                "merchant_name": "Unknown ATM",
                "risk_score": 85,
                "fraud_probability": 25.0
            }
        },
        {
            "name": "Critical Fraud Alert",
            "data": {
                "customer_name": "Test Account",
                "customer_email": "test@demo.com",
                "transaction_amount": 50000.00,
                "transaction_type": "transfer",
                "merchant_name": "Suspicious Entity",
                "risk_score": 95,
                "fraud_probability": 45.0
            }
        }
    ]
    
    for i, scenario in enumerate(test_scenarios, 1):
        # Execute the BPMN workflow with the provided input data
        result = run_dspy_bpmn_process(
            "bpmn/dynamic_email_workflow.bpmn",
            "DynamicEmailWorkflow",
            scenario['data']
        )
        
        # Add delay between scenarios to show different timestamps
        if i < len(test_scenarios):
            time.sleep(2)

if __name__ == "__main__":
    main() 