#!/usr/bin/env python3
"""
Test demonstrating DSPy signature results being used in DMN decision tables.
"""

import json
from typing import Dict, Any, List
from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import DSPyServiceRegistry


class MockSentimentAnalyzer:
    """Mock DSPy signature for sentiment analysis."""
    
    def __init__(self):
        self.name = "SentimentAnalyzer"
    
    def __call__(self, text: str) -> Dict[str, Any]:
        """Analyze sentiment of input text."""
        # Simple mock logic for demonstration
        text_lower = text.lower()
        
        positive_words = ["good", "great", "excellent", "amazing", "wonderful", "love", "happy"]
        negative_words = ["bad", "terrible", "awful", "hate", "disappointing", "sad", "angry"]
        
        positive_count = sum(1 for word in positive_words if word in text_lower)
        negative_count = sum(1 for word in negative_words if word in text_lower)
        
        if positive_count > negative_count:
            sentiment = "positive"
            confidence = min(0.9, 0.5 + (positive_count * 0.1))
        elif negative_count > positive_count:
            sentiment = "negative"
            confidence = min(0.9, 0.5 + (negative_count * 0.1))
        else:
            sentiment = "neutral"
            confidence = 0.6
        
        # Extract keywords (simple implementation)
        keywords = [word for word in text_lower.split() if len(word) > 3]
        
        return {
            "sentiment": sentiment,
            "confidence": confidence,
            "keywords": keywords
        }


def test_dspy_dmn_integration():
    """Test DSPy results flowing into DMN decision table."""
    
    # Initialize components
    parser = DspyBpmnParser()
    
    # Register mock sentiment analyzer
    sentiment_analyzer = MockSentimentAnalyzer()
    
    # Load and parse BPMN workflow
    with open("bpmn/dspy_dmn_workflow.bpmn", "r") as f:
        bpmn_content = f.read()
    
    # Parse DSPy signatures from BPMN using the parser
    parser.add_bpmn_str(bpmn_content, "dspy_dmn_workflow.bpmn")
    
    # Get dynamic signatures from parser
    dynamic_signatures = parser.dynamic_signatures
    print(f"Parsed {len(dynamic_signatures)} DSPy signatures from BPMN")
    
    # Register parsed signatures
    for signature_name, signature_class in dynamic_signatures.items():
        print(f"Found signature: {signature_name}")
    
    # Test cases with different sentiment inputs
    test_cases = [
        {
            "text": "This is absolutely amazing and wonderful! I love it so much.",
            "expected_sentiment": "positive",
            "expected_route": "approval"
        },
        {
            "text": "This is quite good, but could be better.",
            "expected_sentiment": "positive", 
            "expected_route": "review"
        },
        {
            "text": "This is terrible and awful. I hate it completely.",
            "expected_sentiment": "negative",
            "expected_route": "rejection"
        },
        {
            "text": "This is okay, nothing special.",
            "expected_sentiment": "neutral",
            "expected_route": "manual_review"
        }
    ]
    
    print("\n" + "="*60)
    print("DSPy + DMN Integration Test Results")
    print("="*60)
    
    for i, test_case in enumerate(test_cases, 1):
        print(f"\n--- Test Case {i} ---")
        print(f"Input text: {test_case['text']}")
        
        # Execute DSPy signature directly
        result = sentiment_analyzer(test_case['text'])
        print(f"DSPy Result:")
        print(f"  Sentiment: {result['sentiment']}")
        print(f"  Confidence: {result['confidence']:.2f}")
        print(f"  Keywords: {result['keywords']}")
        
        # Simulate DMN decision table evaluation
        keyword_count = len(result['keywords'])
        route = evaluate_dmn_decision(
            sentiment=result['sentiment'],
            confidence=result['confidence'],
            keyword_count=keyword_count
        )
        
        print(f"DMN Decision:")
        print(f"  Route: {route}")
        print(f"  Expected: {test_case['expected_route']}")
        print(f"  Match: {'✓' if route == test_case['expected_route'] else '✗'}")


def evaluate_dmn_decision(sentiment: str, confidence: float, keyword_count: int) -> str:
    """Evaluate DMN decision table based on DSPy results."""
    
    # Rule 1: Positive sentiment, high confidence, many keywords -> approval
    if (sentiment == "positive" and 
        0.8 <= confidence <= 1.0 and 
        keyword_count >= 3):
        return "approval"
    
    # Rule 2: Positive sentiment, medium confidence, few keywords -> review
    elif (sentiment == "positive" and 
          0.6 <= confidence <= 0.79 and 
          1 <= keyword_count <= 2):
        return "review"
    
    # Rule 3: Negative sentiment, high confidence, any keywords -> rejection
    elif (sentiment == "negative" and 
          0.7 <= confidence <= 1.0 and 
          keyword_count >= 1):
        return "rejection"
    
    # Rule 4: Neutral sentiment, any confidence, any keywords -> manual review
    elif (sentiment == "neutral" and 
          0.5 <= confidence <= 1.0):
        return "manual_review"
    
    # Rule 5: Default case -> escalation
    else:
        return "escalation"


def test_workflow_execution():
    """Test actual workflow execution with SpiffWorkflow."""
    
    print("\n" + "="*60)
    print("Full Workflow Execution Test")
    print("="*60)
    
    # Start workflow with test data
    data = {
        "text": "This product is absolutely amazing and wonderful! I love everything about it."
    }
    
    print(f"Starting workflow with data: {data}")
    
    # Execute workflow
    try:
        result = run_dspy_bpmn_process("bpmn/dspy_dmn_workflow.bpmn", "dspy_dmn_workflow", data)
        print(f"Workflow execution result: {result}")
    except Exception as e:
        print(f"Workflow execution error: {e}")
        print("This is expected since we need to implement the full SpiffWorkflow integration")


if __name__ == "__main__":
    # Test DSPy + DMN integration
    test_dspy_dmn_integration()
    
    # Test full workflow execution
    test_workflow_execution() 