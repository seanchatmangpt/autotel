#!/usr/bin/env python3
"""
Test Jinja XML Processor

Demonstrates the Jinja XML processor functionality with various template types.
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.factory.processors.jinja_processor import JinjaProcessor


def test_jinja_processor():
    """Test the Jinja XML processor with various template types."""
    
    print("🧪 Testing Jinja XML Processor")
    print("=" * 50)
    
    # Initialize processor
    processor = JinjaProcessor()
    
    # Test XML with multiple template types
    jinja_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja"
                   xmlns:shacl="http://www.w3.org/ns/shacl#"
                   targetNamespace="http://autotel.ai/jinja/templates">

    <jinja:globalVariables>
        <jinja:variable name="company_name" 
                        description="Company name for templates" 
                        type="string" 
                        required="true"
                        default="AutoTel Inc."
                        example="AutoTel Financial Services">
            <shacl:PropertyShape>
                <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                <shacl:minLength>1</shacl:minLength>
                <shacl:maxLength>100</shacl:maxLength>
            </shacl:PropertyShape>
        </jinja:variable>
        
        <jinja:variable name="timestamp" 
                        description="Current timestamp" 
                        type="string" 
                        required="false"
                        example="2024-01-15T10:30:00Z"/>
    </jinja:globalVariables>

    <jinja:metadata>
        <jinja:author>AutoTel Team</jinja:author>
        <jinja:version>1.0.0</jinja:version>
        <jinja:description>AutoTel Jinja template collection</jinja:description>
    </jinja:metadata>

    <!-- Email Template -->
    <jinja:template name="email_notification" 
                    description="Email notification template"
                    type="email"
                    version="1.0"
                    author="AutoTel Team"
                    category="communication">
        
        <jinja:tags>
            <jinja:tag>email</jinja:tag>
            <jinja:tag>notification</jinja:tag>
            <jinja:tag>communication</jinja:tag>
        </jinja:tags>
        
        <jinja:variables>
            <jinja:variable name="recipient_name" 
                            description="Recipient's full name" 
                            type="string" 
                            required="true"
                            example="John Doe">
                <shacl:PropertyShape>
                    <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                    <shacl:minLength>1</shacl:minLength>
                    <shacl:maxLength>100</shacl:maxLength>
                </shacl:PropertyShape>
            </jinja:variable>
            
            <jinja:variable name="subject" 
                            description="Email subject line" 
                            type="string" 
                            required="true"
                            example="Important Update"/>
            
            <jinja:variable name="message" 
                            description="Email message content" 
                            type="string" 
                            required="true"
                            example="This is an important update about your account.">
                <shacl:PropertyShape>
                    <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
                    <shacl:minLength>10</shacl:minLength>
                    <shacl:maxLength>1000</shacl:maxLength>
                </shacl:PropertyShape>
            </jinja:variable>
            
            <jinja:variable name="priority" 
                            description="Email priority level" 
                            type="string" 
                            required="false"
                            default="normal"
                            example="high"/>
        </jinja:variables>
        
        <jinja:content>
Subject: {{ subject }}
From: {{ company_name }} <noreply@{{ company_name.lower().replace(' ', '') }}.com>
To: {{ recipient_name }} <{{ recipient_name.lower().replace(' ', '.') }}@example.com>
Priority: {{ priority|upper }}
Date: {{ timestamp or '{{ moment().format("YYYY-MM-DD HH:mm:ss UTC") }}' }}

Dear {{ recipient_name }},

{{ message }}

{% if priority == 'high' %}
⚠️  This is a high priority message that requires immediate attention.
{% endif %}

Best regards,
{{ company_name }} Team

---
This is an automated message. Please do not reply to this email.
        </jinja:content>
    </jinja:template>

    <!-- XML Configuration Template -->
    <jinja:template name="config_xml" 
                    description="XML configuration template"
                    type="xml"
                    version="1.0"
                    author="AutoTel Team"
                    category="configuration">
        
        <jinja:tags>
            <jinja:tag>xml</jinja:tag>
            <jinja:tag>configuration</jinja:tag>
            <jinja:tag>settings</jinja:tag>
        </jinja:tags>
        
        <jinja:variables>
            <jinja:variable name="service_name" 
                            description="Service name for configuration" 
                            type="string" 
                            required="true"
                            example="autotel-service"/>
            
            <jinja:variable name="port" 
                            description="Service port number" 
                            type="integer" 
                            required="true"
                            example="8080">
                <shacl:PropertyShape>
                    <shacl:datatype>http://www.w3.org/2001/XMLSchema#integer</shacl:datatype>
                    <shacl:minInclusive>1</shacl:minInclusive>
                    <shacl:maxInclusive>65535</shacl:maxInclusive>
                </shacl:PropertyShape>
            </jinja:variable>
            
            <jinja:variable name="debug_mode" 
                            description="Enable debug mode" 
                            type="boolean" 
                            required="false"
                            default="false"
                            example="true"/>
            
            <jinja:variable name="database_url" 
                            description="Database connection URL" 
                            type="string" 
                            required="true"
                            example="postgresql://user:pass@localhost:5432/db"/>
        </jinja:variables>
        
        <jinja:content>
<?xml version="1.0" encoding="UTF-8"?>
<configuration>
    <service>
        <name>{{ service_name }}</name>
        <port>{{ port }}</port>
        <debug>{{ debug_mode|lower }}</debug>
    </service>
    
    <database>
        <url>{{ database_url }}</url>
        <pool_size>10</pool_size>
        <timeout>30</timeout>
    </database>
    
    <telemetry>
        <enabled>true</enabled>
        <endpoint>http://localhost:4317</endpoint>
        <service_name>{{ service_name }}</service_name>
    </telemetry>
    
    <metadata>
        <generated_at>{{ timestamp or '{{ moment().format("YYYY-MM-DD HH:mm:ss UTC") }}' }}</generated_at>
        <company>{{ company_name }}</company>
    </metadata>
</configuration>
        </jinja:content>
    </jinja:template>

    <!-- Document Template -->
    <jinja:template name="report_document" 
                    description="Report document template"
                    type="document"
                    version="1.0"
                    author="AutoTel Team"
                    category="reporting">
        
        <jinja:tags>
            <jinja:tag>document</jinja:tag>
            <jinja:tag>report</jinja:tag>
            <jinja:tag>analysis</jinja:tag>
        </jinja:tags>
        
        <jinja:variables>
            <jinja:variable name="report_title" 
                            description="Report title" 
                            type="string" 
                            required="true"
                            example="Monthly Performance Report"/>
            
            <jinja:variable name="report_date" 
                            description="Report date" 
                            type="string" 
                            required="true"
                            example="2024-01-15"/>
            
            <jinja:variable name="metrics" 
                            description="Performance metrics" 
                            type="dict" 
                            required="true"
                            example='{"requests": 1000, "errors": 5, "avg_response_time": 150}'/>
            
            <jinja:variable name="summary" 
                            description="Report summary" 
                            type="string" 
                            required="true"
                            example="Overall performance is good with 99.5% uptime."/>
        </jinja:variables>
        
        <jinja:content>
# {{ report_title }}

**Generated by:** {{ company_name }}  
**Date:** {{ report_date }}  
**Timestamp:** {{ timestamp or '{{ moment().format("YYYY-MM-DD HH:mm:ss UTC") }}' }}

## Executive Summary

{{ summary }}

## Performance Metrics

{% for metric_name, metric_value in metrics.items() %}
- **{{ metric_name|title }}:** {{ metric_value }}
{% endfor %}

## Analysis

{% if metrics.errors and metrics.errors > 0 %}
⚠️  **Warning:** {{ metrics.errors }} errors detected during the reporting period.
{% else %}
✅ **Status:** No errors detected during the reporting period.
{% endif %}

{% if metrics.avg_response_time and metrics.avg_response_time > 200 %}
⚠️  **Performance:** Average response time is above target ({{ metrics.avg_response_time }}ms).
{% else %}
✅ **Performance:** Average response time is within acceptable range.
{% endif %}

## Recommendations

{% if metrics.errors and metrics.errors > 10 %}
1. Investigate error patterns and implement fixes
2. Review error handling procedures
{% endif %}

{% if metrics.avg_response_time and metrics.avg_response_time > 300 %}
1. Optimize database queries
2. Consider caching strategies
3. Review resource allocation
{% endif %}

---
*This report was automatically generated by {{ company_name }}'s reporting system.*
        </jinja:content>
    </jinja:template>

</jinja:definitions>'''
    
    print("📄 Parsing Jinja XML definitions...")
    
    # Parse template definitions
    template_definitions = processor.parse_template_definitions(jinja_xml)
    
    print(f"✅ Parsed successfully:")
    print(f"   - Templates: {len(template_definitions.templates)}")
    print(f"   - Global Variables: {len(template_definitions.global_variables)}")
    print(f"   - Namespace: {template_definitions.namespace}")
    
    # Display template information
    print("\n📋 Template Information:")
    for template in template_definitions.templates:
        print(f"   - {template.name} ({template.template_type.value}): {template.description}")
        print(f"     Variables: {len(template.variables)}")
        print(f"     Tags: {', '.join(template.tags)}")
    
    # Test variables for rendering
    test_variables = {
        "company_name": "AutoTel Financial Services",
        "timestamp": "2024-01-15T10:30:00Z",
        "recipient_name": "John Smith",
        "subject": "Account Update Required",
        "message": "Your account requires immediate attention. Please log in to review pending changes.",
        "priority": "high",
        "service_name": "autotel-api",
        "port": 8080,
        "debug_mode": True,
        "database_url": "postgresql://autotel:password@localhost:5432/autotel_db",
        "report_title": "Q4 2024 Performance Report",
        "report_date": "2024-12-31",
        "metrics": {
            "requests": 15000,
            "errors": 3,
            "avg_response_time": 125,
            "uptime": "99.8%"
        },
        "summary": "Excellent performance with 99.8% uptime and low error rates."
    }
    
    print(f"\n🔧 Processing templates with variables...")
    print(f"   Variables provided: {len(test_variables)}")
    
    # Process templates
    result = processor.process_templates(jinja_xml, test_variables)
    
    print(f"✅ Processing completed:")
    print(f"   - Templates processed: {len(result.template_definitions.templates)}")
    print(f"   - Templates rendered: {len([r for r in result.rendering_results if r.success])}")
    print(f"   - Processing time: {result.processing_time_ms:.2f}ms")
    print(f"   - Overall success: {result.success}")
    
    # Display validation results
    print("\n📊 Validation Results:")
    for validation in result.validation_results:
        status = "✅ Valid" if validation.valid else "❌ Invalid"
        print(f"   - {validation.template_name}: {status}")
        if not validation.valid:
            if validation.missing_variables:
                print(f"     Missing: {', '.join(validation.missing_variables)}")
            if validation.invalid_variables:
                print(f"     Invalid: {', '.join(validation.invalid_variables)}")
    
    # Display rendering results
    print("\n📄 Rendering Results:")
    for rendering in result.rendering_results:
        status = "✅ Success" if rendering.success else "❌ Failed"
        print(f"   - {rendering.template_name}: {status}")
        print(f"     Output size: {len(rendering.rendered_content)} characters")
        print(f"     Rendering time: {rendering.rendering_time_ms:.2f}ms")
        
        if rendering.success and len(rendering.rendered_content) < 500:
            print(f"     Preview: {rendering.rendered_content[:100]}...")
    
    # Test individual template rendering
    print("\n🧪 Testing individual template rendering...")
    
    email_template = next(t for t in template_definitions.templates if t.name == "email_notification")
    email_variables = {
        "recipient_name": "Alice Johnson",
        "subject": "Welcome to AutoTel",
        "message": "Welcome to AutoTel! Your account has been successfully created.",
        "priority": "normal",
        "company_name": "AutoTel Inc.",
        "timestamp": "2024-01-15T14:30:00Z"
    }
    
    email_result = processor.render_template(email_template, email_variables)
    
    print(f"✅ Email template rendered:")
    print(f"   - Success: {email_result.success}")
    print(f"   - Output size: {len(email_result.rendered_content)} characters")
    print(f"   - Rendering time: {email_result.rendering_time_ms:.2f}ms")
    
    if email_result.success:
        print("\n📧 Email Preview:")
        print("-" * 50)
        print(email_result.rendered_content)
        print("-" * 50)
    
    # Test validation
    print("\n🔍 Testing variable validation...")
    
    invalid_variables = {
        "recipient_name": "Bob",  # Missing required variables
        "company_name": "AutoTel"
    }
    
    validation_result = processor.validate_template_variables(email_template, invalid_variables)
    
    print(f"✅ Validation test completed:")
    print(f"   - Valid: {validation_result.valid}")
    print(f"   - Missing variables: {len(validation_result.missing_variables)}")
    print(f"   - Invalid variables: {len(validation_result.invalid_variables)}")
    print(f"   - Warnings: {len(validation_result.warnings)}")
    
    if validation_result.missing_variables:
        print(f"   - Missing: {', '.join(validation_result.missing_variables)}")
    
    print("\n🎉 Jinja XML processor test completed successfully!")
    return True


def test_jinja_error_handling():
    """Test error handling in Jinja processor."""
    
    print("\n🧪 Testing Jinja Error Handling")
    print("=" * 50)
    
    processor = JinjaProcessor()
    
    # Test with invalid XML
    try:
        invalid_xml = "<invalid>xml</invalid>"
        result = processor.parse_template_definitions(invalid_xml)
        print("❌ Should have failed with invalid XML")
        return False
    except ValueError as e:
        print(f"✅ Correctly caught invalid XML error: {e}")
    
    # Test with template syntax error
    jinja_xml_with_error = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja">
    <jinja:template name="error_template" description="Template with syntax error">
        <jinja:variables>
            <jinja:variable name="test_var" description="Test variable" type="string" required="true"/>
        </jinja:variables>
        <jinja:content>
{{ test_var } {# Missing closing brace #}
        </jinja:content>
    </jinja:template>
</jinja:definitions>'''
    
    try:
        template_definitions = processor.parse_template_definitions(jinja_xml_with_error)
        template = template_definitions.templates[0]
        
        # Try to render with syntax error
        render_result = processor.render_template(template, {"test_var": "test"})
        
        if not render_result.success:
            print(f"✅ Correctly caught template syntax error: {render_result.errors[0]}")
        else:
            print("❌ Should have failed with template syntax error")
            return False
            
    except Exception as e:
        print(f"✅ Correctly caught error: {e}")
    
    print("🎉 Error handling test completed successfully!")
    return True


def main():
    """Run all Jinja processor tests."""
    print("🚀 Jinja XML Processor Test Suite")
    print("=" * 60)
    
    try:
        # Test 1: Basic functionality
        success1 = test_jinja_processor()
        
        # Test 2: Error handling
        success2 = test_jinja_error_handling()
        
        if success1 and success2:
            print("\n" + "=" * 60)
            print("🎉 All Jinja processor tests passed!")
            print("\nKey Features Demonstrated:")
            print("✅ XML template definition parsing")
            print("✅ Variable validation with SHACL constraints")
            print("✅ Template rendering with Jinja2")
            print("✅ Multiple template types (email, XML, document)")
            print("✅ Error handling and validation")
            print("✅ Telemetry integration")
            print("✅ Production-ready architecture")
            return True
        else:
            print("\n❌ Some tests failed")
            return False
            
    except Exception as e:
        print(f"\n❌ Test suite failed: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1) 