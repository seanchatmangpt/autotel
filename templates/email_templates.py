"""
Jinja2 Email Templates for DMN + DSPy Integration Explanations
These templates generate human-readable explanations of AI analysis and decision results.
"""

from jinja2 import Template
from typing import Dict, Any
from datetime import datetime
import uuid

# Standard Approval Email Template
STANDARD_APPROVAL_TEMPLATE = Template("""
Subject: Transaction Approved - {{ customer_name }}

Dear {{ customer_name }},

Your transaction has been **automatically approved** by our AI-powered risk assessment system.

**Transaction Details:**
- Amount: ${{ "%.2f"|format(transaction_amount) }}
- Type: {{ transaction_type|title }}
- Merchant: {{ merchant_name }}
- Date: {{ transaction_date }}

**AI Analysis Results:**
- Risk Score: {{ risk_score }}/100 (Low Risk)
- Fraud Probability: {{ "%.1f"|format(fraud_probability) }}%
- Confidence Level: {{ confidence_level }}

**Key Risk Factors Identified:**
{% for factor in risk_factors %}
- {{ factor }}
{% endfor %}

**AI Recommendations:**
{{ recommendations }}

**Decision Explanation:**
Based on our AI analysis, your transaction falls within our standard approval parameters. The risk assessment indicates this is a routine transaction that meets our security criteria.

**Next Steps:**
Your transaction will be processed immediately. No further action is required.

If you have any questions, please contact our support team.

Best regards,
{{ company_name }} AI Risk Management System
""")

# Review Required Email Template
REVIEW_REQUIRED_TEMPLATE = Template("""
Subject: Transaction Under Review - {{ customer_name }}

Dear {{ customer_name }},

Your transaction has been flagged for **manual review** by our AI-powered risk assessment system.

**Transaction Details:**
- Amount: ${{ "%.2f"|format(transaction_amount) }}
- Type: {{ transaction_type|title }}
- Merchant: {{ merchant_name }}
- Date: {{ transaction_date }}

**AI Analysis Results:**
- Risk Score: {{ risk_score }}/100 (Medium Risk)
- Fraud Probability: {{ "%.1f"|format(fraud_probability) }}%
- Confidence Level: {{ confidence_level }}

**Risk Factors Requiring Review:**
{% for factor in risk_factors %}
- {{ factor }}
{% endfor %}

**AI Recommendations:**
{{ recommendations }}

**Decision Explanation:**
Our AI system has identified several risk factors that require human review. This is a standard security measure to ensure the safety of your account.

**Next Steps:**
1. A human reviewer will examine your transaction within 24 hours
2. You will receive an update via email once the review is complete
3. If approved, your transaction will be processed immediately

**What This Means:**
- Your account remains secure and active
- This review is precautionary and does not indicate fraud
- Most reviewed transactions are approved within 24 hours

If you have any questions, please contact our support team.

Best regards,
{{ company_name }} AI Risk Management System
""")

# Investigation Required Email Template
INVESTIGATION_REQUIRED_TEMPLATE = Template("""
Subject: Transaction Investigation Required - {{ customer_name }}

Dear {{ customer_name }},

Your transaction has been flagged for **investigation** by our AI-powered risk assessment system.

**Transaction Details:**
- Amount: ${{ "%.2f"|format(transaction_amount) }}
- Type: {{ transaction_type|title }}
- Merchant: {{ merchant_name }}
- Date: {{ transaction_date }}

**AI Analysis Results:**
- Risk Score: {{ risk_score }}/100 (High Risk)
- Fraud Probability: {{ "%.1f"|format(fraud_probability) }}%
- Confidence Level: {{ confidence_level }}

**High-Risk Factors Identified:**
{% for factor in risk_factors %}
- {{ factor }}
{% endfor %}

**AI Recommendations:**
{{ recommendations }}

**Decision Explanation:**
Our AI system has identified multiple high-risk indicators that require immediate investigation. This is a critical security measure to protect your account.

**Next Steps:**
1. Our investigation team will contact you within 2 hours
2. You may be asked to provide additional verification
3. Your transaction is temporarily on hold pending investigation
4. You will receive regular updates throughout the process

**Important Security Notes:**
- Do not share your account credentials with anyone
- Our team will never ask for your password via email
- If you did not initiate this transaction, contact us immediately

**Contact Information:**
- Security Hotline: {{ security_phone }}
- Email: {{ security_email }}

Best regards,
{{ company_name }} AI Risk Management System
""")

# Critical Alert Email Template
CRITICAL_ALERT_TEMPLATE = Template("""
Subject: CRITICAL ALERT - Immediate Action Required - {{ customer_name }}

Dear {{ customer_name }},

**CRITICAL SECURITY ALERT** - Your transaction has been flagged for immediate action by our AI-powered risk assessment system.

**Transaction Details:**
- Amount: ${{ "%.2f"|format(transaction_amount) }}
- Type: {{ transaction_type|title }}
- Merchant: {{ merchant_name }}
- Date: {{ transaction_date }}

**AI Analysis Results:**
- Risk Score: {{ risk_score }}/100 (CRITICAL RISK)
- Fraud Probability: {{ "%.1f"|format(fraud_probability) }}%
- Confidence Level: {{ confidence_level }}

**CRITICAL Risk Factors:**
{% for factor in risk_factors %}
- {{ factor }}
{% endfor %}

**AI Recommendations:**
{{ recommendations }}

**Decision Explanation:**
Our AI system has identified CRITICAL risk indicators that require immediate intervention. This transaction has been automatically blocked for your protection.

**IMMEDIATE ACTIONS REQUIRED:**
1. **CALL OUR SECURITY HOTLINE IMMEDIATELY**: {{ security_phone }}
2. Your account has been temporarily secured
3. Do not attempt any additional transactions
4. Our security team will contact you within 30 minutes

**If You Did NOT Initiate This Transaction:**
- Your account may have been compromised
- Contact us immediately at {{ security_phone }}
- Do not respond to any suspicious emails or calls
- Change your passwords on other accounts

**Security Measures Activated:**
- Account temporarily frozen
- All pending transactions on hold
- Enhanced monitoring activated
- Security team notified

**Emergency Contact:**
- Security Hotline: {{ security_phone }} (24/7)
- Emergency Email: {{ security_email }}
- Reference Code: {{ reference_code }}

This is a critical security alert. Please take immediate action.

Best regards,
{{ company_name }} AI Risk Management System
""")

# Fraud Alert Email Template
FRAUD_ALERT_TEMPLATE = Template("""
Subject: FRAUD DETECTED - Account Security Alert - {{ customer_name }}

Dear {{ customer_name }},

**FRAUD DETECTED** - Our AI-powered fraud detection system has identified suspicious activity on your account.

**Transaction Details:**
- Amount: ${{ "%.2f"|format(transaction_amount) }}
- Type: {{ transaction_type|title }}
- Merchant: {{ merchant_name }}
- Date: {{ transaction_date }}

**AI Fraud Analysis Results:**
- Fraud Probability: {{ "%.1f"|format(fraud_probability) }}% (HIGH RISK)
- Risk Level: {{ risk_level }}
- Confidence Level: {{ confidence_level }}

**Fraud Indicators Detected:**
{% for indicator in fraud_indicators %}
- {{ indicator }}
{% endfor %}

**AI Recommendations:**
{{ recommendations }}

**Decision Explanation:**
Our AI fraud detection system has identified multiple fraud indicators that match known fraudulent patterns. This transaction has been automatically blocked.

**IMMEDIATE SECURITY ACTIONS:**
1. **CALL FRAUD HOTLINE**: {{ fraud_phone }} (24/7)
2. Your account has been secured
3. All transactions are blocked
4. Fraud investigation team activated

**If This Transaction is LEGITIMATE:**
- Contact our fraud team immediately
- Be prepared to provide verification documents
- We will work to restore your account access quickly

**If You Did NOT Initiate This Transaction:**
- Your account has been compromised
- Contact us immediately
- Do not respond to suspicious communications
- Consider freezing your credit reports

**Fraud Prevention Tips:**
- Never share account credentials
- Use strong, unique passwords
- Enable two-factor authentication
- Monitor your account regularly

**Emergency Contacts:**
- Fraud Hotline: {{ fraud_phone }} (24/7)
- Fraud Email: {{ fraud_email }}
- Case Number: {{ case_number }}

**Account Status:**
- Account: TEMPORARILY FROZEN
- All Transactions: BLOCKED
- Investigation: IN PROGRESS

This is a critical fraud alert. Immediate action is required.

Best regards,
{{ company_name }} AI Fraud Detection System
""")

# Template Registry
EMAIL_TEMPLATES = {
    "standard_approval": STANDARD_APPROVAL_TEMPLATE,
    "review_required": REVIEW_REQUIRED_TEMPLATE,
    "investigation_required": INVESTIGATION_REQUIRED_TEMPLATE,
    "critical_alert": CRITICAL_ALERT_TEMPLATE,
    "fraud_alert": FRAUD_ALERT_TEMPLATE
}

def generate_email_explanation(template_type: str, context: Dict[str, Any]) -> Dict[str, str]:
    """
    Generate email explanation using Jinja2 templates based on DMN decision and DSPy analysis.
    
    Args:
        template_type: Type of email template to use
        context: Dictionary containing all data for template rendering
        
    Returns:
        Dictionary with email_subject and email_body
    """
    if template_type not in EMAIL_TEMPLATES:
        raise ValueError(f"Unknown template type: {template_type}")
    
    template = EMAIL_TEMPLATES[template_type]
    
    # Generate dynamic timestamp
    current_time = datetime.now()
    timestamp = current_time.strftime("%Y-%m-%d %H:%M:%S UTC")
    date_only = current_time.strftime("%B %d, %Y")
    time_only = current_time.strftime("%I:%M %p UTC")
    
    # Generate dynamic transaction ID if not provided
    transaction_id = context.get("transaction_id", f"TXN-{uuid.uuid4().hex[:8].upper()}")
    
    # Add default values for missing context with dynamic data
    default_context = {
        "company_name": "AutoTel Financial Services",
        "security_phone": "1-800-SECURITY",
        "security_email": "security@autotel.com",
        "fraud_phone": "1-800-FRAUD-ALERT",
        "fraud_email": "fraud@autotel.com",
        "reference_code": f"REF-{transaction_id}",
        "case_number": f"CASE-{transaction_id}",
        "transaction_date": context.get("transaction_date", date_only),
        "generated_at": timestamp,
        "generated_date": date_only,
        "generated_time": time_only,
        "transaction_id": transaction_id,
        "analysis_timestamp": timestamp,
        "decision_timestamp": timestamp
    }
    
    # Merge context with defaults
    full_context = {**default_context, **context}
    
    # Render template
    rendered = template.render(**full_context)
    
    # Split into subject and body
    lines = rendered.strip().split('\n')
    subject = lines[0].replace('Subject: ', '')
    body = '\n'.join(lines[1:]).strip()
    
    return {
        "email_subject": subject,
        "email_body": body,
        "email_metadata": {
            "template_type": template_type,
            "priority": context.get("priority_level", "NORMAL"),
            "recipients": context.get("recipient_groups", "customer"),
            "generated_at": timestamp,
            "transaction_id": transaction_id,
            "analysis_id": f"ANALYSIS-{uuid.uuid4().hex[:8].upper()}"
        }
    } 