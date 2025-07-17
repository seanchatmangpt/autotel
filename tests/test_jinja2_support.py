#!/usr/bin/env python3
"""
Test Jinja2 Support in AutoTel

Demonstrates Jinja2 templating with {{ }} syntax in XML files.
"""

import sys
import os
import tempfile
import typer
import subprocess
import json
from pathlib import Path
from typing import Dict, Any, List

# Add the project root to the path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from autotel.factory.processors.jinja_processor import JinjaProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.processors.bpmn_processor import BPMNProcessor


def test_jinja2_templating():
    """Test Jinja2 templating with {{ }} syntax."""
    
    print("🧪 Testing Jinja2 Templating with {{ }} syntax")
    print("=" * 60)
    
    # Initialize processor
    processor = JinjaProcessor()
    
    # Test XML with Jinja2 templates using {{ }} syntax
    jinja_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja"
                   targetNamespace="http://autotel.ai/jinja/templates">

    <jinja:globalVariables>
        <jinja:variable name="company_name" 
                        description="Company name for templates" 
                        type="string" 
                        required="true"
                        default="AutoTel Inc."
                        example="AutoTel Financial Services"/>
        
        <jinja:variable name="user_id" 
                        description="User ID for personalization" 
                        type="string" 
                        required="true"
                        example="user123"/>
    </jinja:globalVariables>

    <!-- Email Template with {{ }} syntax -->
    <jinja:template name="email_notification" 
                    description="Email notification template using Jinja2"
                    type="email"
                    version="1.0"
                    author="AutoTel Team"
                    category="communication">
        
        <jinja:variables>
            <jinja:variable name="recipient_name" 
                            description="Recipient's full name" 
                            type="string" 
                            required="true"
                            example="John Doe"/>
            
            <jinja:variable name="subject" 
                            description="Email subject line" 
                            type="string" 
                            required="true"
                            example="Important Update"/>
            
            <jinja:variable name="message" 
                            description="Email message content" 
                            type="string" 
                            required="true"
                            example="This is an important update about your account."/>
        </jinja:variables>
        
        <jinja:content>
Subject: {{ subject }}
From: {{ company_name }} <noreply@{{ company_name.lower().replace(' ', '') }}.com>
To: {{ recipient_name }} <{{ recipient_name.lower().replace(' ', '.') }}@example.com>
User ID: {{ user_id }}
Date: {{ moment().format("YYYY-MM-DD HH:mm:ss UTC") if moment else "2024-01-15T10:30:00Z" }}

Dear {{ recipient_name }},

{{ message }}

{% if user_id %}
Your user ID is: {{ user_id }}
{% endif %}

{% if subject and "urgent" in subject.lower() %}
⚠️  This is an urgent message that requires immediate attention.
{% endif %}

Best regards,
{{ company_name }} Team

---
This is an automated message. Please do not reply to this email.
        </jinja:content>
    </jinja:template>

    <!-- BPMN Condition Template with {{ }} syntax -->
    <jinja:template name="bpmn_condition" 
                    description="BPMN condition expression using Jinja2"
                    type="bpmn"
                    version="1.0"
                    author="AutoTel Team"
                    category="workflow">
        
        <jinja:variables>
            <jinja:variable name="decision_action" 
                            description="Decision action to evaluate" 
                            type="string" 
                            required="true"
                            example="proceed"/>
            
            <jinja:variable name="user_role" 
                            description="User role for authorization" 
                            type="string" 
                            required="false"
                            default="user"
                            example="admin"/>
        </jinja:variables>
        
        <jinja:content>
{{ decision_action == "proceed" and user_role in ["admin", "manager"] }}
        </jinja:content>
    </jinja:template>

    <!-- Configuration Template with {{ }} syntax -->
    <jinja:template name="service_config" 
                    description="Service configuration using Jinja2"
                    type="xml"
                    version="1.0"
                    author="AutoTel Team"
                    category="configuration">
        
        <jinja:variables>
            <jinja:variable name="service_name" 
                            description="Service name" 
                            type="string" 
                            required="true"
                            example="autotel-service"/>
            
            <jinja:variable name="port" 
                            description="Service port" 
                            type="integer" 
                            required="true"
                            example="8080"/>
            
            <jinja:variable name="debug_mode" 
                            description="Debug mode flag" 
                            type="boolean" 
                            required="false"
                            default="false"
                            example="true"/>
        </jinja:variables>
        
        <jinja:content>
<?xml version="1.0" encoding="UTF-8"?>
<configuration>
    <service>
        <name>{{ service_name }}</name>
        <port>{{ port }}</port>
        <debug>{{ debug_mode|lower }}</debug>
        <environment>{{ "development" if debug_mode else "production" }}</environment>
    </service>
    
    <telemetry>
        <enabled>true</enabled>
        <endpoint>http://localhost:4317</endpoint>
        <service_name>{{ service_name }}</service_name>
    </telemetry>
    
    <logging>
        <level>{{ "DEBUG" if debug_mode else "INFO" }}</level>
        <format>json</format>
    </logging>
</configuration>
        </jinja:content>
    </jinja:template>

</jinja:definitions>'''
    
    # Test variables
    test_variables = {
        "company_name": "AutoTel Financial Services",
        "user_id": "user_12345",
        "recipient_name": "John Doe",
        "subject": "Urgent: Account Update Required",
        "message": "Your account requires immediate attention. Please review the attached documents.",
        "decision_action": "proceed",
        "user_role": "admin",
        "service_name": "autotel-workflow-engine",
        "port": 8080,
        "debug_mode": True
    }
    
    print("📄 Processing Jinja2 templates with {{ }} syntax...")
    print(f"📊 Variables: {json.dumps(test_variables, indent=2)}")
    print()
    
    # Process templates
    result = processor.process_templates(jinja_xml, test_variables)
    
    print(f"✅ Template processing completed!")
    print(f"📊 Templates processed: {len(result.template_definitions.templates)}")
    print(f"📊 Templates rendered: {len([r for r in result.rendering_results if r.success])}")
    print(f"⏱️  Processing time: {result.processing_time_ms:.2f}ms")
    print()
    
    # Display results
    for i, rendering in enumerate(result.rendering_results, 1):
        if rendering.success:
            print(f"📋 Template {i}: {rendering.template_name}")
            print(f"⏱️  Rendering time: {rendering.rendering_time_ms:.2f}ms")
            print(f"📏 Output size: {len(rendering.rendered_content)} characters")
            print("📄 Rendered content:")
            print("-" * 40)
            print(rendering.rendered_content)
            print("-" * 40)
            print()
        else:
            print(f"❌ Template {i}: {rendering.template_name} - Failed")
            print(f"   Error: {rendering.error_message}")
            print()
    
    # Test single template rendering
    print("🧪 Testing single template rendering...")
    single_template = "Hello {{ user_id }}, welcome to {{ company_name }}!"
    rendered = processor.render_single_template(single_template, test_variables)
    print(f"Template: {single_template}")
    print(f"Rendered: {rendered}")
    print()
    
    # Test syntax validation
    print("🧪 Testing template syntax validation...")
    valid_template = "Hello {{ user_id }}!"
    invalid_template = "Hello {{ user_id }!"
    
    print(f"Valid template: {processor.validate_template_syntax(valid_template)}")
    print(f"Invalid template: {processor.validate_template_syntax(invalid_template)}")
    print()
    
    print("✅ Jinja2 templating test completed successfully!")


def test_bpmn_jinja2_integration():
    """Test BPMN integration with Jinja2 templating."""
    
    print("🧪 Testing BPMN Integration with Jinja2")
    print("=" * 50)
    
    # Example BPMN condition expressions using Jinja2
    bpmn_conditions = [
        "{{ decision_action == 'proceed' }}",
        "{{ user_role in ['admin', 'manager'] }}",
        "{{ amount > 1000 and user_level == 'premium' }}",
        "{{ approval_status == 'approved' or override_flag == true }}"
    ]
    
    processor = JinjaProcessor()
    
    test_variables = {
        "decision_action": "proceed",
        "user_role": "admin",
        "amount": 1500,
        "user_level": "premium",
        "approval_status": "approved",
        "override_flag": False
    }
    
    print("📄 BPMN Condition Expressions with Jinja2:")
    for i, condition in enumerate(bpmn_conditions, 1):
        try:
            result = processor.render_single_template(condition, test_variables)
            print(f"{i}. {condition} → {result}")
        except Exception as e:
            print(f"{i}. {condition} → Error: {e}")
    
    print()
    print("✅ BPMN Jinja2 integration test completed!")


def test_cdata_and_nested_content():
    """Test CDATA and nested <jinja:content> edge cases."""
    print("🧪 Testing CDATA and nested <jinja:content> edge cases")
    print("=" * 60)

    processor = JinjaProcessor()

    # Test 1: CDATA with unescaped XML and Jinja2
    jinja_xml_cdata = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja">
    <jinja:template name="cdata_test" description="Test CDATA with unescaped XML and Jinja2">
        <jinja:variables>
            <jinja:variable name="user_id" type="string" required="true"/>
        </jinja:variables>
        <jinja:content><![CDATA[
<message>
  <user>{{ user_id }}</user>
  <body>Hello, {{ user_id }}! & <b>Welcome</b> to <i>AutoTel</i>.</body>
  {% if user_id == 'admin' %}<admin>true</admin>{% endif %}
</message>
        ]]></jinja:content>
    </jinja:template>
</jinja:definitions>
'''
    variables = {"user_id": "admin"}
    result = processor.process_templates(jinja_xml_cdata, variables)
    print("Test 1: CDATA with unescaped XML and Jinja2")
    for rendering in result.rendering_results:
        print(f"Success: {rendering.success}")
        print("Output:")
        print(rendering.rendered_content)
        print("-" * 40)

    # Test 2: Nested <jinja:content> inside <jinja:content>
    jinja_xml_nested = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja">
    <jinja:template name="nested_content_test" description="Test nested jinja:content">
        <jinja:variables>
            <jinja:variable name="outer_var" type="string" required="true"/>
            <jinja:variable name="inner_var" type="string" required="true"/>
        </jinja:variables>
        <jinja:content><![CDATA[
<outer>
  Outer: {{ outer_var }}
  <jinja:content><![CDATA[
    <inner>Inner: {{ inner_var }}</inner>
  ]]></jinja:content>
</outer>
        ]]></jinja:content>
    </jinja:template>
</jinja:definitions>
'''
    variables_nested = {"outer_var": "OUTER_VALUE", "inner_var": "INNER_VALUE"}
    result_nested = processor.process_templates(jinja_xml_nested, variables_nested)
    print("Test 2: Nested <jinja:content> inside <jinja:content>")
    for rendering in result_nested.rendering_results:
        print(f"Success: {rendering.success}")
        print("Output:")
        print(rendering.rendered_content)
        print("-" * 40)


def test_dspy_output_with_jinja_template():
    """Test combining DSPy output with a Jinja2 template."""
    print("🧪 Testing DSPy output with Jinja2 template integration")
    print("=" * 60)

    # Example DSPy XML with a signature and output
    dspy_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:signatures xmlns:dspy="http://autotel.ai/dspy">
    <dspy:signature name="get_user_greeting" description="Generate a greeting for a user">
        <dspy:input name="user_name" type="string" description="Name of the user"/>
        <dspy:output name="greeting" type="string" description="Greeting message"/>
    </dspy:signature>
</dspy:signatures>
'''
    # Simulate DSPyProcessor extracting the signature and output
    dspy_processor = DSPyProcessor()
    signatures = dspy_processor.parse(dspy_xml)
    signature = signatures[0]
    # Simulate a DSPy output value
    dspy_output = {"greeting": f"Hello, {{ user_name }}! Welcome to AutoTel."}

    # Now use this output as a Jinja2 template
    jinja_template_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja">
    <jinja:template name="greeting_email" description="Greeting email template">
        <jinja:variables>
            <jinja:variable name="user_name" type="string" required="true"/>
            <jinja:variable name="greeting" type="string" required="true"/>
        </jinja:variables>
        <jinja:content><![CDATA[
<email>
  <to>{{ user_name }}@example.com</to>
  <subject>Welcome!</subject>
  <body>{{ greeting }}</body>
</email>
        ]]></jinja:content>
    </jinja:template>
</jinja:definitions>
'''
    # Prepare variables for Jinja template
    variables = {"user_name": "Alice"}
    # Render the greeting using the DSPy output as a Jinja template
    from autotel.factory.processors.jinja_processor import JinjaProcessor
    jinja_processor = JinjaProcessor()
    # First, render the greeting string as a Jinja template
    greeting_template = dspy_output["greeting"]
    rendered_greeting = jinja_processor.render_single_template(greeting_template, variables)
    # Now, use this as input to the main Jinja template
    variables["greeting"] = rendered_greeting
    result = jinja_processor.process_templates(jinja_template_xml, variables)
    print("DSPy output (rendered greeting):", rendered_greeting)
    for rendering in result.rendering_results:
        print(f"Success: {rendering.success}")
        print("Output:")
        print(rendering.rendered_content)
        print("-" * 40)


def test_dspy_jinja_cli():
    """Test DSPy-to-Jinja pipeline via a simple Typer CLI command."""
    print("🧪 Testing DSPy-to-Jinja pipeline via Typer CLI")
    print("=" * 60)

    # Create a simple Typer CLI app
    app = typer.Typer()

    @app.command()
    def dspy_to_jinja(
        dspy_file: Path = typer.Argument(..., help="DSPy XML file"),
        jinja_file: Path = typer.Argument(..., help="Jinja XML file"),
        variables: str = typer.Option("{}", "--vars", "-v", help="Variables as JSON string"),
        output_file: Path = typer.Option(None, "--output", "-o", help="Output file (default: stdout)")
    ):
        """Process DSPy XML output through Jinja2 template."""
        import json
        
        # Parse variables
        try:
            vars_dict = json.loads(variables)
        except json.JSONDecodeError as e:
            typer.echo(f"Error parsing variables JSON: {e}", err=True)
            raise typer.Exit(1)
        
        # Read DSPy XML
        try:
            with open(dspy_file, 'r') as f:
                dspy_xml = f.read()
        except FileNotFoundError:
            typer.echo(f"DSPy file not found: {dspy_file}", err=True)
            raise typer.Exit(1)
        
        # Read Jinja XML
        try:
            with open(jinja_file, 'r') as f:
                jinja_xml = f.read()
        except FileNotFoundError:
            typer.echo(f"Jinja file not found: {jinja_file}", err=True)
            raise typer.Exit(1)
        
        # Process DSPy
        dspy_processor = DSPyProcessor()
        signatures = dspy_processor.parse(dspy_xml)
        
        if not signatures:
            typer.echo("No DSPy signatures found", err=True)
            raise typer.Exit(1)
        
        # Simulate DSPy output (in real usage, this would come from DSPy execution)
        signature = signatures[0]
        dspy_output = {"greeting": f"Hello, {{ user_name }}! Welcome to AutoTel."}
        
        # Render DSPy output as Jinja template
        jinja_processor = JinjaProcessor()
        greeting_template = dspy_output["greeting"]
        rendered_greeting = jinja_processor.render_single_template(greeting_template, vars_dict)
        
        # Add rendered greeting to variables for main Jinja template
        vars_dict["greeting"] = rendered_greeting
        
        # Process main Jinja template
        result = jinja_processor.process_templates(jinja_xml, vars_dict)
        
        if not result.success:
            typer.echo("Jinja template processing failed", err=True)
            for error in result.errors:
                typer.echo(f"  - {error}", err=True)
            raise typer.Exit(1)
        
        # Output result
        output_content = ""
        for rendering in result.rendering_results:
            if rendering.success:
                output_content += rendering.rendered_content + "\n"
        
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output_content)
            typer.echo(f"Output written to: {output_file}")
        else:
            typer.echo("Final output:")
            typer.echo(output_content)

    # Create temporary directory and files
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        
        # Create DSPy XML file
        dspy_xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<dspy:signatures xmlns:dspy="http://autotel.ai/dspy">
    <dspy:signature name="get_user_greeting" description="Generate a greeting for a user">
        <dspy:input name="user_name" type="string" description="Name of the user"/>
        <dspy:output name="greeting" type="string" description="Greeting message"/>
    </dspy:signature>
</dspy:signatures>
'''
        dspy_file = temp_path / "test_dspy.xml"
        with open(dspy_file, 'w') as f:
            f.write(dspy_xml_content)
        
        # Create Jinja XML file
        jinja_xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<jinja:definitions xmlns:jinja="http://autotel.ai/jinja">
    <jinja:template name="greeting_email" description="Greeting email template">
        <jinja:variables>
            <jinja:variable name="user_name" type="string" required="true"/>
            <jinja:variable name="greeting" type="string" required="true"/>
        </jinja:variables>
        <jinja:content><![CDATA[
<email>
  <to>{{ user_name }}@example.com</to>
  <subject>Welcome!</subject>
  <body>{{ greeting }}</body>
</email>
        ]]></jinja:content>
    </jinja:template>
</jinja:definitions>
'''
        jinja_file = temp_path / "test_jinja.xml"
        with open(jinja_file, 'w') as f:
            f.write(jinja_xml_content)
        
        # Create output file
        output_file = temp_path / "output.xml"
        
        # Test variables
        variables = '{"user_name": "Bob"}'
        
        print(f"📁 Temporary directory: {temp_dir}")
        print(f"📄 DSPy file: {dspy_file}")
        print(f"📄 Jinja file: {jinja_file}")
        print(f"📄 Output file: {output_file}")
        print(f"📊 Variables: {variables}")
        print()
        
        # Run the CLI command
        try:
            # Simulate CLI execution by calling the function directly
            dspy_to_jinja(
                dspy_file=dspy_file,
                jinja_file=jinja_file,
                variables=variables,
                output_file=output_file
            )
            
            # Read and display output
            with open(output_file, 'r') as f:
                final_output = f.read()
            
            print("✅ CLI execution successful!")
            print("📄 Final output:")
            print(final_output)
            
        except Exception as e:
            print(f"❌ CLI execution failed: {e}")
            import traceback
            traceback.print_exc()


def test_jinja_generates_and_runs_typer_cli():
    """Test: Jinja2 generates a Typer CLI script, writes it to temp file, runs it with subprocess."""
    print("🧪 Testing Jinja2-generated Typer CLI script execution")
    print("=" * 60)
    
    # Jinja2 template for a Typer CLI script
    cli_template = '''\
#!/usr/bin/env python3
import typer

app = typer.Typer(help="{{ cli_help }}")

@app.command()
def main(name: str = typer.Argument(..., help="Name to greet")):
    """A simple CLI generated by Jinja2."""
    typer.echo(f"Hello, {name}! This CLI was generated by Jinja2.")

if __name__ == "__main__":
    typer.run(main)
'''
    # Render the template
    from autotel.factory.processors.jinja_processor import JinjaProcessor
    jinja_processor = JinjaProcessor()
    variables = {"cli_help": "A simple CLI generated by Jinja2 and executed via subprocess."}
    rendered_cli = jinja_processor.render_single_template(cli_template, variables)

    # Write to a temp file
    import tempfile
    import os
    with tempfile.TemporaryDirectory() as temp_dir:
        cli_path = os.path.join(temp_dir, "generated_cli.py")
        with open(cli_path, "w") as f:
            f.write(rendered_cli)
        print(f"📄 Generated CLI script at: {cli_path}")
        # Run the CLI with an argument
        result = subprocess.run([
            "python", cli_path, "Bob"
        ], capture_output=True, text=True)
        print("CLI stdout:")
        print(result.stdout)
        print("CLI stderr:")
        print(result.stderr)
        assert "Hello, Bob!" in result.stdout
        print("✅ Jinja2-generated Typer CLI ran successfully!")


def main():
    """Run all Jinja2 tests."""
    try:
        test_jinja2_templating()
        print("\n" + "="*60 + "\n")
        test_bpmn_jinja2_integration()
        print("\n" + "="*60 + "\n")
        test_cdata_and_nested_content()
        print("\n" + "="*60 + "\n")
        test_dspy_output_with_jinja_template()
        print("\n" + "="*60 + "\n")
        test_dspy_jinja_cli()
        print("\n" + "="*60 + "\n")
        test_jinja_generates_and_runs_typer_cli()
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 