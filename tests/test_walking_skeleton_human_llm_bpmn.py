import os
from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.utils.dspy_services import dspy_registry
import dspy
import pytest

BPMN_PATH = os.path.join(os.path.dirname(__file__), '../bpmn/human_llm_task_process.bpmn')

def test_human_llm_bpmn_walking_skeleton():
    """Basic walking skeleton test for LLM+human BPMN workflow"""
    # Simulate a real support ticket
    context = {"ticket_text": "My internet is down. Can you help?"}

    # Patch function to register a real implementation for the dynamic signature
    def patch_signature(parser):
        sig_cls = parser.get_dynamic_signature("human_help_llm")
        def real_forward(self, ticket_text):
            return {"suggestion": f"Dear customer, please reboot your router. If the problem persists, contact support. (You said: {ticket_text})"}
        sig_cls.forward = real_forward

    # Run the process, patching the signature registry after parsing
    result = run_dspy_bpmn_process(BPMN_PATH, "HumanLLMTaskProcess", context, signature_patch_fn=patch_signature)

    # Check that the LLM suggestion is present and the process completed
    assert "suggestion" in result
    assert result["suggestion"].strip() != ""
    # The human task should be completed (in this test, auto-completed)
    assert result.get("ticket_text") == context["ticket_text"]

    # (Optional) Print for debug
    print("LLM suggestion:", result["suggestion"])
    print("Final workflow data:", result)

def test_realistic_customer_support_scenario():
    """Realistic customer support scenario with LLM assistance"""
    # Simulate a complex customer support case
    ticket_text = "My internet has been slow for 3 days. I've tried restarting the router but it's still bad."
    context = {
        "customer_id": "CUST-12345",
        "ticket_text": ticket_text,
        "customer_tier": "premium",
        "escalation_level": 1
    }

    def patch_signature(parser):
        # Override the DSPy service call directly
        def custom_dspy_service(service_name, **kwargs):
            if service_name == "human_help_llm":
                ticket_text = kwargs.get("ticket_text", "")
                if "slow" in ticket_text.lower():
                    if "3 days" in ticket_text:
                        return {
                            "suggestion": "This appears to be a persistent connectivity issue. I recommend:\n1. Check for ISP outages in your area\n2. Run a speed test\n3. Contact technical support for line diagnostics\n4. Consider escalating to tier 2 support"
                        }
                    else:
                        return {
                            "suggestion": "For slow internet issues, try:\n1. Restart your router\n2. Check for background downloads\n3. Test on different devices\n4. Contact support if problem persists"
                        }
                else:
                    return {
                        "suggestion": "I understand you're having connectivity issues. Let me help you troubleshoot this step by step."
                    }
            return {"suggestion": "Please provide more details about your issue so we can assist you better."}
        
        # Monkey patch the dspy_service function
        import autotel.utils.dspy_services
        autotel.utils.dspy_services.dspy_service = custom_dspy_service

    # Create a temporary BPMN with the actual ticket text
    import tempfile
    import shutil
    
    # Read the original BPMN
    with open(BPMN_PATH, 'r') as f:
        bpmn_content = f.read()
    
    # Replace the parameter value with the actual ticket text
    bpmn_content = bpmn_content.replace(
        '<dspy:param name="ticket_text" value="ticket_text" />',
        f'<dspy:param name="ticket_text" value="{ticket_text}" />'
    )
    
    # Write to temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(bpmn_content)
        temp_bpmn_path = f.name

    try:
        # Run the process with the temporary BPMN
        result = run_dspy_bpmn_process(temp_bpmn_path, "HumanLLMTaskProcess", context, signature_patch_fn=patch_signature)

        # Verify the realistic scenario worked
        assert "suggestion" in result
        suggestion = result["suggestion"]
        # The suggestion is already a string, not JSON
        assert "persistent connectivity issue" in suggestion.lower()
        assert "tier 2 support" in suggestion
        assert result.get("customer_id") == "CUST-12345"
        assert result.get("customer_tier") == "premium"

        print("=== Realistic Customer Support Scenario ===")
        print(f"Customer: {result['customer_id']} (Tier: {result['customer_tier']})")
        print(f"Issue: {result['ticket_text']}")
        print(f"LLM Suggestion: {suggestion}")
        print("=== End Scenario ===")
    finally:
        # Clean up temporary file
        os.unlink(temp_bpmn_path)

def test_llm_human_collaboration_workflow():
    """Test demonstrating the full LLM+human collaboration workflow"""
    # This test shows how the BPMN orchestrates LLM assistance for human tasks
    
    # Simulate a technical support scenario
    ticket_text = "Cannot connect to VPN after system update"
    context = {
        "issue_type": "network_connectivity",
        "ticket_text": ticket_text,
        "user_technical_level": "intermediate",
        "business_impact": "high"
    }

    def patch_signature(parser):
        # Override the DSPy service call directly
        def custom_dspy_service(service_name, **kwargs):
            if service_name == "human_help_llm":
                ticket_text = kwargs.get("ticket_text", "")
                if "vpn" in ticket_text.lower() and "system update" in ticket_text.lower():
                    return {
                        "suggestion": "VPN connectivity issues after system updates are common. Here's a systematic approach:\n\n1. **Immediate Actions:**\n   - Check VPN client version compatibility\n   - Verify network adapter settings\n   - Clear VPN client cache\n\n2. **For Human Agent:**\n   - Review system update logs\n   - Check corporate VPN policy changes\n   - Consider rolling back recent updates\n   - Escalate to network team if needed\n\n3. **Customer Communication:**\n   - Explain this is a known issue post-updates\n   - Provide temporary workarounds\n   - Set realistic resolution timeline"
                    }
                else:
                    return {
                        "suggestion": "I'll help you troubleshoot this connectivity issue. Let me gather some information first."
                    }
            return {"suggestion": "Please provide more details about your issue so we can assist you better."}
        
        # Monkey patch the dspy_service function
        import autotel.utils.dspy_services
        autotel.utils.dspy_services.dspy_service = custom_dspy_service

    # Create a temporary BPMN with the actual ticket text
    import tempfile
    
    # Read the original BPMN
    with open(BPMN_PATH, 'r') as f:
        bpmn_content = f.read()
    
    # Replace the parameter value with the actual ticket text
    bpmn_content = bpmn_content.replace(
        '<dspy:param name="ticket_text" value="ticket_text" />',
        f'<dspy:param name="ticket_text" value="{ticket_text}" />'
    )
    
    # Write to temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(bpmn_content)
        temp_bpmn_path = f.name

    try:
        # Run the collaborative workflow
        result = run_dspy_bpmn_process(temp_bpmn_path, "HumanLLMTaskProcess", context, signature_patch_fn=patch_signature)

        # Verify the collaborative workflow
        assert "suggestion" in result
        suggestion = result["suggestion"]
        # The suggestion is already a string, not JSON
        assert "vpn" in suggestion.lower()
        assert "system update" in suggestion.lower()
        assert "human agent" in suggestion.lower()
        assert result.get("business_impact") == "high"

        print("=== LLM+Human Collaboration Workflow ===")
        print(f"Issue Type: {result['issue_type']}")
        print(f"Business Impact: {result['business_impact']}")
        print(f"User Level: {result['user_technical_level']}")
        print(f"LLM Assistance: {suggestion[:200]}...")
        print("=== Collaboration Complete ===")
    finally:
        # Clean up temporary file
        os.unlink(temp_bpmn_path)

if __name__ == "__main__":
    # Run all tests to demonstrate the walking skeleton
    test_human_llm_bpmn_walking_skeleton()
    test_realistic_customer_support_scenario()
    test_llm_human_collaboration_workflow()
    print("\n🎉 All LLM+Human BPMN walking skeleton tests passed!")
    print("✅ Basic workflow orchestration")
    print("✅ Realistic customer support scenarios")
    print("✅ LLM+Human collaboration patterns") 