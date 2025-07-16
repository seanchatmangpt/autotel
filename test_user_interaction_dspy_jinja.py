#!/usr/bin/env python3
"""
Test User Interactions with DSPy and Jinja2 for Dynamic Answers

Demonstrates how to use DSPy to generate dynamic responses and Jinja2 to format them
in user interaction workflows.
"""

import json
import sys
import time
import random
import uuid
from datetime import datetime
from typing import Dict, Any, List
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.defaults import UserTask, ManualTask, NoneTask

from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.processors.jinja_processor import JinjaProcessor

import dspy
from jinja2 import Template
import pytest

# Configure DSPy with no caching and higher temperature for more variation
ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.9)  # Higher temperature for more randomness
dspy.configure(lm=ollama_lm)

# Add random seed to ensure different responses
random.seed(int(time.time()))

class CustomerServiceSignature(dspy.Signature):
    """Analyze customer inquiry and provide personalized response"""
    customer_inquiry = dspy.InputField(desc="Customer's question or issue")
    customer_tier = dspy.InputField(desc="Customer tier (standard, premium, enterprise)")
    previous_interactions = dspy.InputField(desc="Previous interaction history")
    random_seed = dspy.InputField(desc="Random seed to ensure unique responses")
    
    response = dspy.OutputField(desc="Personalized response to customer")
    escalation_needed = dspy.OutputField(desc="Whether escalation is needed (true/false)")
    confidence_score = dspy.OutputField(desc="Confidence in the response (0-1)")

class TechnicalDiagnosisSignature(dspy.Signature):
    """Diagnose technical issues and provide solutions"""
    technical_issue = dspy.InputField(desc="Technical problem description")
    system_info = dspy.InputField(desc="System information and logs")
    customer_expertise = dspy.InputField(desc="Customer's technical expertise level")
    random_seed = dspy.InputField(desc="Random seed to ensure unique responses")
    
    diagnosis = dspy.OutputField(desc="Technical diagnosis of the issue")
    solution_steps = dspy.OutputField(desc="Step-by-step solution")
    complexity_level = dspy.OutputField(desc="Complexity level (basic, intermediate, advanced)")

class BillingResolutionSignature(dspy.Signature):
    """Resolve billing disputes and provide explanations"""
    billing_issue = dspy.InputField(desc="Billing problem description")
    account_details = dspy.InputField(desc="Account and billing details")
    dispute_history = dspy.InputField(desc="Previous dispute history")
    random_seed = dspy.InputField(desc="Random seed to ensure unique responses")
    
    resolution = dspy.OutputField(desc="Billing resolution explanation")
    adjustment_amount = dspy.OutputField(desc="Any billing adjustments needed")
    policy_reference = dspy.OutputField(desc="Relevant policy reference")

class EscalationAnalysisSignature(dspy.Signature):
    """Analyze whether escalation is needed"""
    issue_complexity = dspy.InputField(desc="Complexity of the issue")
    customer_satisfaction = dspy.InputField(desc="Customer satisfaction level")
    resolution_attempts = dspy.InputField(desc="Number of resolution attempts")
    random_seed = dspy.InputField(desc="Random seed to ensure unique responses")
    
    escalation_recommendation = dspy.OutputField(desc="Whether to escalate (true/false)")
    escalation_reason = dspy.OutputField(desc="Reason for escalation decision")
    priority_level = dspy.OutputField(desc="Priority level (low, medium, high, urgent)")

class CustomerFeedbackSignature(dspy.Signature):
    """Generate customer feedback and follow-up"""
    interaction_summary = dspy.InputField(desc="Summary of the interaction")
    resolution_quality = dspy.OutputField(desc="Quality of resolution provided")
    customer_sentiment = dspy.InputField(desc="Customer sentiment analysis")
    random_seed = dspy.InputField(desc="Random seed to ensure unique responses")
    
    feedback_message = dspy.OutputField(desc="Personalized feedback message")
    follow_up_actions = dspy.OutputField(desc="Recommended follow-up actions")
    satisfaction_score = dspy.OutputField(desc="Predicted satisfaction score (1-10)")

# Create DSPy modules
customer_service_module = dspy.ChainOfThought(CustomerServiceSignature)
technical_diagnosis_module = dspy.ChainOfThought(TechnicalDiagnosisSignature)
billing_resolution_module = dspy.ChainOfThought(BillingResolutionSignature)
escalation_analysis_module = dspy.ChainOfThought(EscalationAnalysisSignature)
customer_feedback_module = dspy.ChainOfThought(CustomerFeedbackSignature)

# Jinja2 templates for dynamic responses
email_templates = {
    "technical_support": Template("""
Dear {{ customer_name }},

Thank you for contacting our technical support team regarding your {{ issue_type }} issue.

**Issue Summary:**
{{ issue_description }}

**Our Analysis:**
{{ dspy_diagnosis }}

**Recommended Solution:**
{{ dspy_solution }}

**Next Steps:**
{% if complexity == 'basic' %}
This appears to be a straightforward issue that you should be able to resolve following the steps above.
{% elif complexity == 'intermediate' %}
This issue requires some technical knowledge. If you need assistance, please don't hesitate to contact us.
{% else %}
Due to the complexity of this issue, we recommend scheduling a call with our technical team.
{% endif %}

**Reference Number:** {{ reference_id }}
**Response Time:** {{ response_time }}

If you have any questions or need further assistance, please reply to this email or call our support line.

Best regards,
{{ agent_name }}
Technical Support Team
{{ company_name }}
    """),
    
    "billing_dispute": Template("""
Dear {{ customer_name }},

Thank you for bringing your billing concern to our attention. We take all billing inquiries seriously and have thoroughly reviewed your account.

**Issue Reviewed:**
{{ billing_issue }}

**Our Investigation:**
{{ dspy_resolution }}

**Resolution:**
{% if adjustment_amount > 0 %}
We have identified an error and will be issuing a credit of ${{ adjustment_amount }} to your account.
{% else %}
After careful review, we found that the charges are correct according to our service agreement.
{% endif %}

**Policy Reference:** {{ policy_reference }}

**Next Steps:**
- The adjustment (if applicable) will appear on your next statement
- Please allow 3-5 business days for processing
- You can track this in your account dashboard

**Reference Number:** {{ reference_id }}

If you have any questions about this resolution, please contact our billing department directly.

Best regards,
{{ agent_name }}
Billing Resolution Team
{{ company_name }}
    """),
    
    "general_inquiry": Template("""
Dear {{ customer_name }},

Thank you for reaching out to us with your inquiry about {{ inquiry_topic }}.

**Your Question:**
{{ customer_question }}

**Our Response:**
{{ dspy_response }}

**Additional Information:**
{% if customer_tier == 'enterprise' %}
As an enterprise customer, you have access to our dedicated support team and priority response times.
{% elif customer_tier == 'premium' %}
As a premium customer, you enjoy enhanced support and faster response times.
{% else %}
For additional support, consider upgrading to our premium plan for enhanced service.
{% endif %}

**Confidence Level:** {{ confidence_score }}%

**Reference Number:** {{ reference_id }}

We hope this addresses your inquiry. If you need any clarification or have follow-up questions, please don't hesitate to ask.

Best regards,
{{ agent_name }}
Customer Service Team
{{ company_name }}
    """)
}

def generate_random_seed():
    """Generate a unique random seed for each DSPy call"""
    return f"{uuid.uuid4().hex[:8]}_{int(time.time())}"

def call_dspy_with_timing(module, **kwargs):
    """Call DSPy module with timing and randomization"""
    # Add random seed to ensure unique responses
    kwargs['random_seed'] = generate_random_seed()
    
    print(f"🤖 Calling DSPy with random seed: {kwargs['random_seed']}")
    start_time = time.time()
    
    try:
        result = module(**kwargs)
        end_time = time.time()
        duration = end_time - start_time
        
        print(f"⏱️  DSPy response time: {duration:.2f} seconds")
        
        # Extract results
        if hasattr(result, '_store'):
            output = dict(result._store)
        else:
            output = {k: v for k, v in vars(result).items() if not k.startswith('_')}
        
        return output, duration
        
    except Exception as e:
        print(f"❌ DSPy error: {str(e)}")
        return None, 0

@pytest.mark.slow
@pytest.mark.integration
@pytest.mark.dspy
def test_dspy_response_timing():
    """Test DSPy response times to verify model calls"""
    print('🤖 Testing DSPy response times with randomization...')
    print('=' * 60)
    
    for i in range(3):
        random_seed = f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
        question = f'What is {random.randint(1, 100)} + {random.randint(1, 100)}?'
        
        print(f'\n🔄 Test {i+1}: {question}')
        print(f'🎲 Random seed: {random_seed}')
        
        start_time = time.time()
        result = customer_service_module(
            customer_inquiry=question,
            customer_tier="standard",
            previous_interactions="No previous interactions",
            random_seed=random_seed
        )
        end_time = time.time()
        
        duration = end_time - start_time
        print(f'⏱️  Response time: {duration:.2f} seconds')
        print(f'💬 Answer: {result.response[:100]}...')
        
        if duration < 0.5:
            print('⚠️  WARNING: Very fast response - possible caching!')
        elif duration < 2.0:
            print('⚠️  SUSPICIOUS: Fast response - might be cached')
        else:
            print('✅ Good response time - likely hitting model')
        
        time.sleep(1)  # Brief pause between calls
    
    print('\n🎯 Test completed!')

@pytest.mark.slow
@pytest.mark.integration
@pytest.mark.dspy
def test_customer_service_scenarios():
    """Test customer service scenarios with DSPy and Jinja2"""
    scenarios = [
        {
            "name": "Enterprise Technical Issue",
            "data": {
                "name": "Dr. Emily Watson",
                "tier": "enterprise",
                "inquiry": f"Critical system outage affecting {random.randint(100, 500)} users. Database connection failures with error code {random.randint(1000, 9999)}. Need immediate assistance.",
                "system_info": f"Production environment, {random.choice(['AWS', 'Azure', 'GCP'])} cloud, PostgreSQL {random.choice(['13', '14', '15'])}",
                "expertise": "advanced",
                "satisfaction": 0.3,
                "attempts": 2,
                "sentiment": "frustrated",
                "history": "Previous critical issues resolved successfully"
            }
        },
        {
            "name": "Premium Billing Dispute",
            "data": {
                "name": "Marcus Thompson",
                "tier": "premium",
                "inquiry": f"Disputed charge of ${random.randint(50, 200)} on invoice #{random.randint(10000, 99999)}. Service was not used during billing period.",
                "account_details": f"Premium plan, account active for {random.randint(6, 24)} months",
                "dispute_history": "First billing dispute",
                "satisfaction": 0.4,
                "attempts": 1,
                "sentiment": "concerned"
            }
        }
    ]
    
    for scenario in scenarios:
        print(f"\n🎭 Testing scenario: {scenario['name']}")
        
        # Test customer service analysis
        service_result, service_time = call_dspy_with_timing(
            customer_service_module,
            customer_inquiry=scenario['data']['inquiry'],
            customer_tier=scenario['data']['tier'],
            previous_interactions=scenario['data'].get('history', 'No previous interactions'),
        )
        
        assert service_result is not None, "Customer service analysis failed"
        assert 'response' in service_result, "Response field missing"
        assert 'confidence_score' in service_result, "Confidence score missing"
        
        # Test escalation analysis
        escalation_result, escalation_time = call_dspy_with_timing(
            escalation_analysis_module,
            issue_complexity=service_result.get('confidence_score', 0.5),
            customer_satisfaction=scenario['data'].get('satisfaction', 0.7),
            resolution_attempts=scenario['data'].get('attempts', 1),
        )
        
        assert escalation_result is not None, "Escalation analysis failed"
        assert 'escalation_recommendation' in escalation_result, "Escalation recommendation missing"
        
        print(f"✅ Scenario {scenario['name']} completed successfully")

@pytest.mark.slow
@pytest.mark.integration
@pytest.mark.dspy
def test_advanced_dspy_registry():
    """Test advanced DSPy registry with cache controls"""
    from autotel.utils.advanced_dspy_services import advanced_dspy_registry
    
    # Register a test signature
    advanced_dspy_registry.register_dynamic_signature(
        'test_signature',
        {'question': 'A test question', 'random_seed': 'Random seed'},
        {'answer': 'The answer'},
        'Test signature for timing',
        disable_cache=True
    )
    
    print('🔧 Testing Advanced DSPy Registry with Cache Controls')
    print('=' * 60)
    
    # Test with caching disabled
    start_time = time.time()
    result1 = advanced_dspy_registry.call_signature('test_signature', 
        question='What is 2+2?', 
        random_seed=uuid.uuid4().hex[:8])
    end_time = time.time()
    
    duration1 = end_time - start_time
    print(f'⏱️  Response time (cache disabled): {duration1:.2f} seconds')
    
    # Test with explicit cache disable
    advanced_dspy_registry.disable_caching('test_signature')
    start_time = time.time()
    result2 = advanced_dspy_registry.call_signature('test_signature', 
        question='What is 3+3?', 
        random_seed=uuid.uuid4().hex[:8])
    end_time = time.time()
    
    duration2 = end_time - start_time
    print(f'⏱️  Response time (explicit disable): {duration2:.2f} seconds')
    
    # Show stats
    stats = advanced_dspy_registry.get_stats()
    print(f'\n📊 Cache Statistics:')
    print(f'   Hits: {stats["cache_stats"]["hits"]}')
    print(f'   Misses: {stats["cache_stats"]["misses"]}')
    print(f'   Hit Rate: {stats["cache_stats"]["hit_rate"]:.2%}')
    print(f'   Disabled: {stats["cache_stats"]["disabled"]}')
    
    assert stats["cache_stats"]["disabled"] is True, "Cache should be disabled"
    assert stats["cache_stats"]["misses"] > 0, "Should have cache misses"
    assert duration1 > 1.0, "Response time should be realistic for LLM call"
    assert duration2 > 1.0, "Response time should be realistic for LLM call"

@pytest.mark.skip(reason="Manual test only - requires user interaction")
def test_manual_dspy_verification():
    """Manual test to verify DSPy is hitting the model"""
    print("This test is skipped by default. Run with --runxfail to execute manually.")
    print("Use this test to manually verify DSPy behavior when needed.")

@pytest.mark.slow
@pytest.mark.integration
@pytest.mark.dspy
def test_jinja2_template_rendering():
    """Test Jinja2 template rendering with DSPy outputs"""
    # Test data
    template_context = {
        "customer_name": "Test Customer",
        "issue_type": "technical",
        "issue_description": "Test issue description",
        "dspy_diagnosis": "Test diagnosis from DSPy",
        "dspy_solution": "Test solution from DSPy",
        "complexity": "intermediate",
        "reference_id": f"TEST-{uuid.uuid4().hex[:8].upper()}",
        "response_time": "5.0 seconds",
        "agent_name": "Test Agent",
        "company_name": "Test Company"
    }
    
    # Render template
    email_template = email_templates["technical_support"]
    email_content = email_template.render(**template_context)
    
    # Verify template rendering
    assert "Test Customer" in email_content, "Customer name should be in email"
    assert "Test diagnosis from DSPy" in email_content, "DSPy diagnosis should be in email"
    assert "Test solution from DSPy" in email_content, "DSPy solution should be in email"
    assert "TEST-" in email_content, "Reference ID should be in email"
    
    print("✅ Jinja2 template rendering test passed")

# Pytest configuration
def pytest_configure(config):
    """Configure pytest markers"""
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers", "integration: marks tests as integration tests"
    )
    config.addinivalue_line(
        "markers", "dspy: marks tests that use DSPy/LLM calls"
    )

# Main function for direct execution
def main():
    """Run the dynamic user interaction test with randomization"""
    
    print("🚀 Starting Dynamic User Interaction Test with DSPy and Jinja2")
    print("=" * 80)
    
    # Test scenarios with randomization
    scenarios = [
        {
            "name": "Enterprise Technical Issue",
            "data": {
                "name": "Dr. Emily Watson",
                "tier": "enterprise",
                "inquiry": f"Critical system outage affecting {random.randint(100, 500)} users. Database connection failures with error code {random.randint(1000, 9999)}. Need immediate assistance.",
                "system_info": f"Production environment, {random.choice(['AWS', 'Azure', 'GCP'])} cloud, PostgreSQL {random.choice(['13', '14', '15'])}",
                "expertise": "advanced",
                "satisfaction": 0.3,
                "attempts": 2,
                "sentiment": "frustrated",
                "history": "Previous critical issues resolved successfully"
            }
        },
        {
            "name": "Premium Billing Dispute",
            "data": {
                "name": "Marcus Thompson",
                "tier": "premium",
                "inquiry": f"Disputed charge of ${random.randint(50, 200)} on invoice #{random.randint(10000, 99999)}. Service was not used during billing period.",
                "account_details": f"Premium plan, account active for {random.randint(6, 24)} months",
                "dispute_history": "First billing dispute",
                "satisfaction": 0.4,
                "attempts": 1,
                "sentiment": "concerned"
            }
        },
        {
            "name": "Standard General Inquiry",
            "data": {
                "name": "Lisa Chen",
                "tier": "standard",
                "inquiry": f"Question about feature availability for {random.choice(['mobile app', 'web dashboard', 'API integration'])}. Considering upgrade to premium.",
                "satisfaction": 0.8,
                "attempts": 1,
                "sentiment": "curious",
                "history": "New customer, first inquiry"
            }
        }
    ]
    
    all_results = []
    
    for scenario in scenarios:
        print(f"\n{'='*60}")
        print(f"🎭 SCENARIO: {scenario['name']}")
        print(f"{'='*60}")
        
        result = simulate_customer_interaction(scenario['name'], scenario['data'])
        all_results.append(result)
        
        # Add delay between scenarios to ensure fresh responses
        time.sleep(2)
    
    # Summary statistics
    print(f"\n{'='*80}")
    print("📊 TEST SUMMARY")
    print(f"{'='*80}")
    
    total_time = sum(r['performance_metrics']['total_processing_time'] for r in all_results)
    avg_time = total_time / len(all_results)
    avg_dspy_time = sum(r['performance_metrics']['average_dspy_time'] for r in all_results) / len(all_results)
    
    print(f"Total scenarios run: {len(all_results)}")
    print(f"Total processing time: {total_time:.2f} seconds")
    print(f"Average scenario time: {avg_time:.2f} seconds")
    print(f"Average DSPy response time: {avg_dspy_time:.2f} seconds")
    
    escalation_count = sum(1 for r in all_results if r['escalation_decision'])
    print(f"Escalations recommended: {escalation_count}/{len(all_results)}")
    
    avg_confidence = sum(r['confidence_score'] for r in all_results) / len(all_results)
    print(f"Average confidence score: {avg_confidence:.2f}")
    
    # Check for response time patterns
    dspy_times = []
    for result in all_results:
        for time_val in result['performance_metrics']['dspy_calls'].values():
            if time_val > 0:
                dspy_times.append(time_val)
    
    if dspy_times:
        min_time = min(dspy_times)
        max_time = max(dspy_times)
        print(f"DSPy response time range: {min_time:.2f}s - {max_time:.2f}s")
        
        if max_time - min_time < 0.5:
            print("⚠️  WARNING: Very consistent response times - possible caching detected!")
        else:
            print("✅ Good response time variation - likely hitting model")
    
    print(f"\n🎯 Test completed successfully!")
    print(f"📝 Results saved to: test_user_interaction_dspy_jinja_results.json")
    
    # Save detailed results
    with open('test_user_interaction_dspy_jinja_results.json', 'w') as f:
        json.dump(all_results, f, indent=2, default=str)

class DynamicUserInteractionSimulator:
    def __init__(self):
        self.parser = BpmnParser()
        self.workflow = None
        self.step_count = 0
        self.max_steps = 25
        
        # Initialize processors
        self.dspy_processor = DSPyProcessor()
        self.jinja_processor = JinjaProcessor()
        
        # DSPy signatures for dynamic responses
        self.dspy_signatures = {
            "customer_greeting": {
                "inputs": ["customer_name", "issue_type", "customer_mood"],
                "outputs": ["greeting_message", "tone"],
                "description": "Generate personalized greeting based on customer context"
            },
            "technical_response": {
                "inputs": ["issue_description", "customer_technical_level", "previous_attempts"],
                "outputs": ["solution_steps", "explanation", "confidence_level"],
                "description": "Generate technical support response"
            },
            "billing_response": {
                "inputs": ["billing_issue", "customer_history", "amount_involved"],
                "outputs": ["resolution_plan", "apology_level", "compensation_offer"],
                "description": "Generate billing dispute response"
            },
            "escalation_decision": {
                "inputs": ["customer_satisfaction", "issue_complexity", "escalation_history"],
                "outputs": ["should_escalate", "escalation_reason", "priority_level"],
                "description": "Decide whether to escalate the issue"
            }
        }
        
        # Jinja2 templates for formatting responses
        self.jinja_templates = {
            "greeting_template": """
Dear {{ customer_name }},

{{ greeting_message }}

{% if tone == "formal" %}
We understand this is an important matter and we're here to help.
{% elif tone == "friendly" %}
We're excited to assist you with your {{ issue_type }} inquiry!
{% else %}
We're ready to help you resolve your {{ issue_type }} issue.
{% endif %}

How can we best assist you today?

Best regards,
{{ agent_name }}
{{ company_name }} Support Team
""",
            
            "technical_solution_template": """
**Technical Solution for: {{ issue_description }}**

**Solution Steps:**
{% for step in solution_steps %}
{{ loop.index }}. {{ step }}
{% endfor %}

**Explanation:**
{{ explanation }}

**Confidence Level:** {{ confidence_level }}%

{% if confidence_level < 70 %}
⚠️ **Note:** This solution may require additional troubleshooting.
{% endif %}

**Next Steps:**
Please try these steps and let us know if you need further assistance.
""",
            
            "billing_resolution_template": """
**Billing Issue Resolution**

**Issue:** {{ billing_issue }}

**Our Resolution Plan:**
{{ resolution_plan }}

{% if apology_level == "high" %}
We sincerely apologize for any inconvenience this may have caused.
{% elif apology_level == "medium" %}
We apologize for the confusion.
{% endif %}

{% if compensation_offer %}
**Compensation:** {{ compensation_offer }}
{% endif %}

**Next Steps:**
Please review this resolution and let us know if you have any questions.
""",
            
            "escalation_notice_template": """
**Issue Escalation Notice**

{% if should_escalate %}
⚠️ **This issue has been escalated to our senior support team.**

**Reason for Escalation:**
{{ escalation_reason }}

**Priority Level:** {{ priority_level }}

**What This Means:**
- A senior specialist will review your case within 2 hours
- You'll receive a direct call from our escalation team
- We'll provide regular updates on the resolution progress

**Estimated Resolution Time:** 
{% if priority_level == "high" %}4-6 hours{% elif priority_level == "medium" %}8-12 hours{% else %}24 hours{% endif %}
{% else %}
✅ **Issue can be resolved at this level.**

**Resolution Plan:**
{{ resolution_plan }}
{% endif %}
"""
        }
        
    def generate_dspy_response(self, signature_name: str, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """Generate dynamic response using DSPy simulation."""
        print(f"    🤖 DSPy generating response for: {signature_name}")
        
        if signature_name == "customer_greeting":
            customer_name = inputs.get("customer_name", "Customer")
            issue_type = inputs.get("issue_type", "inquiry")
            customer_mood = inputs.get("customer_mood", "neutral")
            
            # Simulate DSPy response generation
            greetings = {
                "friendly": f"Hello {customer_name}! 👋",
                "formal": f"Good day {customer_name},",
                "empathetic": f"Hi {customer_name}, I understand you're having an issue."
            }
            
            tones = {
                "friendly": "friendly",
                "formal": "formal", 
                "empathetic": "empathetic"
            }
            
            return {
                "greeting_message": greetings.get(customer_mood, greetings["friendly"]),
                "tone": tones.get(customer_mood, "friendly")
            }
            
        elif signature_name == "technical_response":
            issue_desc = inputs.get("issue_description", "technical issue")
            tech_level = inputs.get("customer_technical_level", "beginner")
            attempts = inputs.get("previous_attempts", 0)
            
            # Simulate technical response generation
            if "email" in issue_desc.lower():
                steps = [
                    "Check your internet connection",
                    "Clear your browser cache and cookies",
                    "Try accessing from a different browser",
                    "Check if the email service is working"
                ]
                confidence = 85 if tech_level == "advanced" else 70
            else:
                steps = [
                    "Restart the application",
                    "Check system requirements",
                    "Update to the latest version",
                    "Contact technical support if issue persists"
                ]
                confidence = 75 if tech_level == "advanced" else 60
            
            return {
                "solution_steps": steps,
                "explanation": f"Based on your {tech_level} technical level, here's a step-by-step solution.",
                "confidence_level": confidence
            }
            
        elif signature_name == "billing_response":
            issue = inputs.get("billing_issue", "billing dispute")
            history = inputs.get("customer_history", "new")
            amount = inputs.get("amount_involved", 0)
            
            # Simulate billing response generation
            if amount > 1000:
                apology = "high"
                compensation = f"${amount * 0.1:.2f} credit to your account"
            elif amount > 100:
                apology = "medium"
                compensation = f"${amount * 0.05:.2f} credit to your account"
            else:
                apology = "low"
                compensation = None
            
            return {
                "resolution_plan": f"We'll investigate the {issue} and provide a resolution within 24 hours.",
                "apology_level": apology,
                "compensation_offer": compensation
            }
            
        elif signature_name == "escalation_decision":
            satisfaction = inputs.get("customer_satisfaction", 3)
            complexity = inputs.get("issue_complexity", "medium")
            history = inputs.get("escalation_history", 0)
            
            # Simulate escalation decision
            should_escalate = (
                satisfaction < 3 or 
                complexity == "high" or 
                history > 2
            )
            
            if should_escalate:
                if satisfaction < 3:
                    reason = "Customer satisfaction is low"
                    priority = "high"
                elif complexity == "high":
                    reason = "Issue requires specialized expertise"
                    priority = "medium"
                else:
                    reason = "Multiple previous escalation attempts"
                    priority = "medium"
            else:
                reason = "Issue can be resolved at current level"
                priority = "low"
            
            return {
                "should_escalate": should_escalate,
                "escalation_reason": reason,
                "priority_level": priority
            }
        
        return {"error": "Unknown signature"}
    
    def render_jinja_template(self, template_name: str, variables: Dict[str, Any]) -> str:
        """Render Jinja2 template with variables."""
        print(f"    🎨 Jinja2 rendering template: {template_name}")
        
        if template_name not in self.jinja_templates:
            return f"Template {template_name} not found"
        
        template = self.jinja_templates[template_name]
        
        try:
            rendered = self.jinja_processor.render_single_template(template, variables)
            return rendered
        except Exception as e:
            return f"Template rendering error: {e}"
    
    def simulate_user_thinking(self, task_name: str):
        """Simulate realistic user thinking time"""
        print(f"    🤔 User is thinking about {task_name}...")
        time.sleep(0.3)
    
    def simulate_user_typing(self, task_name: str):
        """Simulate user typing responses"""
        print(f"    ⌨️  User is typing response for {task_name}...")
        time.sleep(0.2)
    
    def handle_dynamic_user_task(self, task, scenario_data: Dict[str, Any]):
        """Handle user tasks with dynamic DSPy + Jinja2 responses"""
        task_name = task.task_spec.name
        print(f"    👤 Dynamic user task: {task_name}")
        
        # Simulate user thinking
        self.simulate_user_thinking(task_name)
        
        # Map task names to DSPy signatures and Jinja templates
        task_mapping = {
            "UserTask_1": {
                "dspy_signature": "customer_greeting",
                "jinja_template": "greeting_template",
                "inputs": {
                    "customer_name": scenario_data.get("customer_name", "Customer"),
                    "issue_type": scenario_data.get("issue_type", "inquiry"),
                    "customer_mood": scenario_data.get("customer_mood", "friendly")
                }
            },
            "UserTask_2": {
                "dspy_signature": "technical_response",
                "jinja_template": "technical_solution_template",
                "inputs": {
                    "issue_description": scenario_data.get("issue_description", "technical issue"),
                    "customer_technical_level": scenario_data.get("technical_level", "beginner"),
                    "previous_attempts": scenario_data.get("attempts", 0)
                }
            },
            "UserTask_3": {
                "dspy_signature": "billing_response",
                "jinja_template": "billing_resolution_template",
                "inputs": {
                    "billing_issue": scenario_data.get("billing_issue", "billing dispute"),
                    "customer_history": scenario_data.get("customer_history", "new"),
                    "amount_involved": scenario_data.get("amount", 0)
                }
            },
            "UserTask_5": {
                "dspy_signature": "escalation_decision",
                "jinja_template": "escalation_notice_template",
                "inputs": {
                    "customer_satisfaction": scenario_data.get("satisfaction", 3),
                    "issue_complexity": scenario_data.get("complexity", "medium"),
                    "escalation_history": scenario_data.get("escalation_history", 0)
                }
            }
        }
        
        if task_name in task_mapping:
            mapping = task_mapping[task_name]
            
            # Generate DSPy response
            dspy_response = self.generate_dspy_response(
                mapping["dspy_signature"], 
                mapping["inputs"]
            )
            
            # Simulate user typing
            self.simulate_user_typing(task_name)
            
            # Prepare variables for Jinja template
            jinja_variables = {
                **mapping["inputs"],
                **dspy_response,
                "agent_name": "Alex",
                "company_name": "AutoTel Support"
            }
            
            # Render Jinja template
            formatted_response = self.render_jinja_template(
                mapping["jinja_template"], 
                jinja_variables
            )
            
            # Update task data
            task.data.update({
                **mapping["inputs"],
                **dspy_response,
                "formatted_response": formatted_response,
                "dspy_signature_used": mapping["dspy_signature"],
                "jinja_template_used": mapping["jinja_template"]
            })
            
            # Special handling for escalation decision
            if task_name == "UserTask_5":
                task.data["resolved"] = not dspy_response.get("should_escalate", False)
            
            print(f"    ✅ Dynamic response generated and formatted")
            
        else:
            # Fallback for unmapped tasks
            print(f"    ⚠️  No dynamic mapping for {task_name}, using defaults")
            task.data.update({
                "default_response": "User provided information",
                "timestamp": time.time()
            })
        
        task.complete()
    
    def run_dynamic_scenario(self, scenario_name: str, scenario_data: Dict[str, Any]):
        """Run a dynamic user interaction scenario with DSPy + Jinja2."""
        print(f"\n🎭 Running Dynamic Scenario: {scenario_name.upper()}")
        print("=" * 60)
        print(f"📊 Scenario Data: {json.dumps(scenario_data, indent=2)}")
        print()
        
        # Create BPMN workflow (using the same structure as before)
        bpmn_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="DynamicCustomerServiceWorkflow" isExecutable="true">
    
    <!-- Start Event -->
    <bpmn:startEvent id="StartEvent_1" name="Customer Contact">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <!-- Initial Assessment -->
    <bpmn:userTask id="UserTask_1" name="Dynamic Greeting">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Decision Gateway -->
    <bpmn:exclusiveGateway id="Gateway_1" name="Issue Type">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
      <bpmn:outgoing>Flow_4</bpmn:outgoing>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Technical Support Path -->
    <bpmn:userTask id="UserTask_2" name="Dynamic Technical Support">
      <bpmn:incoming>Flow_3</bpmn:incoming>
      <bpmn:outgoing>Flow_6</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Billing Support Path -->
    <bpmn:userTask id="UserTask_3" name="Dynamic Billing Support">
      <bpmn:incoming>Flow_4</bpmn:incoming>
      <bpmn:outgoing>Flow_7</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- General Inquiry Path -->
    <bpmn:userTask id="UserTask_4" name="General Inquiry">
      <bpmn:incoming>Flow_5</bpmn:incoming>
      <bpmn:outgoing>Flow_8</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Resolution Check -->
    <bpmn:userTask id="UserTask_5" name="Dynamic Resolution Check">
      <bpmn:incoming>Flow_6</bpmn:incoming>
      <bpmn:incoming>Flow_7</bpmn:incoming>
      <bpmn:incoming>Flow_8</bpmn:incoming>
      <bpmn:outgoing>Flow_9</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Final Decision -->
    <bpmn:exclusiveGateway id="Gateway_2" name="Issue Resolved?">
      <bpmn:incoming>Flow_9</bpmn:incoming>
      <bpmn:outgoing>Flow_10</bpmn:outgoing>
      <bpmn:outgoing>Flow_11</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Escalation -->
    <bpmn:userTask id="UserTask_6" name="Escalate to Supervisor">
      <bpmn:incoming>Flow_11</bpmn:incoming>
      <bpmn:outgoing>Flow_12</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- End Events -->
    <bpmn:endEvent id="EndEvent_1" name="Issue Resolved">
      <bpmn:incoming>Flow_10</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:endEvent id="EndEvent_2" name="Escalated">
      <bpmn:incoming>Flow_12</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="UserTask_1"/>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="UserTask_1" targetRef="Gateway_1"/>
    <bpmn:sequenceFlow id="Flow_3" sourceRef="Gateway_1" targetRef="UserTask_2">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "technical"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_4" sourceRef="Gateway_1" targetRef="UserTask_3">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "billing"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_5" sourceRef="Gateway_1" targetRef="UserTask_4">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "general"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_6" sourceRef="UserTask_2" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_7" sourceRef="UserTask_3" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_8" sourceRef="UserTask_4" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_9" sourceRef="UserTask_5" targetRef="Gateway_2"/>
    <bpmn:sequenceFlow id="Flow_10" sourceRef="Gateway_2" targetRef="EndEvent_1">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">resolved == True</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_11" sourceRef="Gateway_2" targetRef="UserTask_6">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">resolved == False</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_12" sourceRef="UserTask_6" targetRef="EndEvent_2"/>
    
  </bpmn:process>
  
</bpmn:definitions>'''
        
        # Create new parser for this scenario
        parser = BpmnParser()
        
        # Parse BPMN XML
        from lxml import etree
        bpmn_tree = etree.fromstring(bpmn_xml.encode('utf-8'))
        parser.add_bpmn_xml(bpmn_tree, f"dynamic_workflow_{scenario_name}.bpmn")
        
        # Get process specs
        specs = parser.find_all_specs()
        process_id = "DynamicCustomerServiceWorkflow"
        spec = specs[process_id]
        self.workflow = BpmnWorkflow(spec)
        
        print(f"✅ Starting dynamic {scenario_name} scenario...")
        
        # Execute workflow
        self.step_count = 0
        while not self.workflow.is_completed() and self.step_count < self.max_steps:
            self.step_count += 1
            ready_tasks = self.workflow.get_tasks(state=TaskState.READY)
            
            print(f"📞 Step {self.step_count}: {len(ready_tasks)} ready tasks")
            
            if not ready_tasks:
                print("    ⏸️  No ready tasks - workflow may be stuck")
                break
                
            for task in ready_tasks:
                task_name = task.task_spec.name
                task_type = type(task.task_spec).__name__
                
                print(f"  🎯 Running: {task_name} ({task_type})")
                
                # Handle different task types
                if isinstance(task.task_spec, UserTask):
                    # Handle dynamic user tasks with DSPy + Jinja2
                    self.handle_dynamic_user_task(task, scenario_data)
                    
                elif isinstance(task.task_spec, (ManualTask, NoneTask)):
                    print(f"    🛠️  Manual task completed")
                    task.complete()
                    
                else:
                    # For gateways, script tasks, etc.
                    # Ensure required data is available
                    if "Gateway" in task_name:
                        if "issue_type" not in task.data and "issue_type" in self.workflow.data:
                            task.data["issue_type"] = self.workflow.data["issue_type"]
                        if "resolved" not in task.data and "resolved" in self.workflow.data:
                            task.data["resolved"] = self.workflow.data["resolved"]
                    
                    try:
                        task.run()
                        print(f"    ✅ Task completed successfully")
                    except Exception as e:
                        print(f"    ❌ ERROR running task: {e}")
                        return False
                
                print(f"    📊 Task data keys: {list(task.data.keys())}")
                if "formatted_response" in task.data:
                    print(f"    📄 Response preview: {task.data['formatted_response'][:100]}...")
        
        # Check completion
        if self.workflow.is_completed():
            print(f"🎉 Dynamic scenario completed in {self.step_count} steps!")
            
            # Determine outcome
            if "resolved" in self.workflow.data and self.workflow.data["resolved"]:
                print(f"✅ Outcome: Issue RESOLVED")
            else:
                print(f"⚠️  Outcome: Issue ESCALATED")
                
            return True
        else:
            print(f"⏰ Scenario did not complete in {self.max_steps} steps")
            return False


def simulate_customer_interaction(scenario: str, customer_data: Dict[str, Any]) -> Dict[str, Any]:
    """Simulate a complete customer interaction with DSPy and Jinja2"""
    
    print(f"\n🎭 Starting scenario: {scenario}")
    print(f"📊 Customer data: {json.dumps(customer_data, indent=2)}")
    
    # Step 1: Initial customer service analysis
    print("\n🔍 Step 1: Customer Service Analysis")
    service_result, service_time = call_dspy_with_timing(
        customer_service_module,
        customer_inquiry=customer_data['inquiry'],
        customer_tier=customer_data['tier'],
        previous_interactions=customer_data.get('history', 'No previous interactions'),
    )
    
    if not service_result:
        return {"error": "Failed to analyze customer inquiry"}
    
    # Step 2: Technical diagnosis (if technical issue)
    technical_result = None
    technical_time = 0
    if 'technical' in customer_data['inquiry'].lower():
        print("\n🔧 Step 2: Technical Diagnosis")
        technical_result, technical_time = call_dspy_with_timing(
            technical_diagnosis_module,
            technical_issue=customer_data['inquiry'],
            system_info=customer_data.get('system_info', 'No system info provided'),
            customer_expertise=customer_data.get('expertise', 'intermediate'),
        )
    
    # Step 3: Billing resolution (if billing issue)
    billing_result = None
    billing_time = 0
    if 'billing' in customer_data['inquiry'].lower() or 'charge' in customer_data['inquiry'].lower():
        print("\n💰 Step 3: Billing Resolution")
        billing_result, billing_time = call_dspy_with_timing(
            billing_resolution_module,
            billing_issue=customer_data['inquiry'],
            account_details=customer_data.get('account_details', 'Standard account'),
            dispute_history=customer_data.get('dispute_history', 'No previous disputes'),
        )
    
    # Step 4: Escalation analysis
    print("\n📈 Step 4: Escalation Analysis")
    escalation_result, escalation_time = call_dspy_with_timing(
        escalation_analysis_module,
        issue_complexity=service_result.get('confidence_score', 0.5),
        customer_satisfaction=customer_data.get('satisfaction', 0.7),
        resolution_attempts=customer_data.get('attempts', 1),
    )
    
    # Step 5: Customer feedback generation
    print("\n💬 Step 5: Customer Feedback Generation")
    feedback_result, feedback_time = call_dspy_with_timing(
        customer_feedback_module,
        interaction_summary=f"Handled {scenario} inquiry",
        resolution_quality=service_result.get('confidence_score', 0.8),
        customer_sentiment=customer_data.get('sentiment', 'neutral'),
    )
    
    # Step 6: Generate email using Jinja2
    print("\n📧 Step 6: Generating Email Response")
    
    # Determine template type
    if technical_result:
        template_type = "technical_support"
        template_context = {
            "customer_name": customer_data.get('name', 'Valued Customer'),
            "issue_type": "technical",
            "issue_description": customer_data['inquiry'],
            "dspy_diagnosis": technical_result.get('diagnosis', 'Issue analyzed'),
            "dspy_solution": technical_result.get('solution_steps', 'Solution provided'),
            "complexity": technical_result.get('complexity_level', 'intermediate'),
            "reference_id": f"TECH-{uuid.uuid4().hex[:8].upper()}",
            "response_time": f"{service_time + technical_time:.1f} seconds",
            "agent_name": "Alex Chen",
            "company_name": "TechCorp Support"
        }
    elif billing_result:
        template_type = "billing_dispute"
        template_context = {
            "customer_name": customer_data.get('name', 'Valued Customer'),
            "billing_issue": customer_data['inquiry'],
            "dspy_resolution": billing_result.get('resolution', 'Issue resolved'),
            "adjustment_amount": billing_result.get('adjustment_amount', 0),
            "policy_reference": billing_result.get('policy_reference', 'Standard Policy'),
            "reference_id": f"BILL-{uuid.uuid4().hex[:8].upper()}",
            "agent_name": "Sarah Johnson",
            "company_name": "TechCorp Billing"
        }
    else:
        template_type = "general_inquiry"
        template_context = {
            "customer_name": customer_data.get('name', 'Valued Customer'),
            "inquiry_topic": "general inquiry",
            "customer_question": customer_data['inquiry'],
            "dspy_response": service_result.get('response', 'Response provided'),
            "customer_tier": customer_data['tier'],
            "confidence_score": int(service_result.get('confidence_score', 0.8) * 100),
            "reference_id": f"GEN-{uuid.uuid4().hex[:8].upper()}",
            "agent_name": "Mike Rodriguez",
            "company_name": "TechCorp Customer Service"
        }
    
    # Render email template
    email_template = email_templates[template_type]
    email_content = email_template.render(**template_context)
    
    # Calculate total processing time
    total_time = service_time + technical_time + billing_time + escalation_time + feedback_time
    
    # Compile results
    results = {
        "scenario": scenario,
        "customer_data": customer_data,
        "dspy_results": {
            "customer_service": service_result,
            "technical_diagnosis": technical_result,
            "billing_resolution": billing_result,
            "escalation_analysis": escalation_result,
            "customer_feedback": feedback_result
        },
        "email_response": {
            "template_type": template_type,
            "content": email_content,
            "context": template_context
        },
        "performance_metrics": {
            "total_processing_time": total_time,
            "dspy_calls": {
                "customer_service": service_time,
                "technical_diagnosis": technical_time,
                "billing_resolution": billing_time,
                "escalation_analysis": escalation_time,
                "customer_feedback": feedback_time
            },
            "average_dspy_time": total_time / 5 if total_time > 0 else 0
        },
        "escalation_decision": escalation_result.get('escalation_recommendation', False) if escalation_result else False,
        "confidence_score": service_result.get('confidence_score', 0.0) if service_result else 0.0
    }
    
    print(f"\n✅ Scenario completed in {total_time:.2f} seconds")
    print(f"📊 Average DSPy response time: {results['performance_metrics']['average_dspy_time']:.2f} seconds")
    print(f"🚨 Escalation needed: {results['escalation_decision']}")
    print(f"🎯 Confidence score: {results['confidence_score']:.2f}")
    
    return results


if __name__ == "__main__":
    main() 