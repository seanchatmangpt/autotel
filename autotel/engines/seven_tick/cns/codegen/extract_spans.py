#!/usr/bin/env python3
"""
Extract telemetry spans from TTL ontology and generate JSON for code generation.
"""

import sys
import json
import os
from pathlib import Path

try:
    import rdflib
    from rdflib import Graph, Namespace, RDF, RDFS
except ImportError:
    print("Error: rdflib not found. Install with: pip install rdflib")
    sys.exit(1)

def extract_spans_from_ttl(ttl_file):
    """
    Extract telemetry spans, functions, and patterns from TTL file.
    
    Args:
        ttl_file: Path to TTL file
        
    Returns:
        Dictionary with spans, functions, patterns, and metadata
    """
    g = Graph()
    
    try:
        g.parse(ttl_file, format="ttl")
    except Exception as e:
        print(f"Error parsing TTL file {ttl_file}: {e}")
        return None
    
    # Define namespaces
    CNS = Namespace("https://schema.chatman.ai/cns#")
    
    spans = []
    functions = []
    patterns = []
    
    # Extract all telemetry spans
    for span in g.subjects(RDF.type, CNS.TelemetrySpan):
        span_uri = str(span)
        span_name = span_uri.split("#")[-1]
        
        # Get the function this span provides
        function_obj = g.value(span, CNS.providesFunction)
        if function_obj:
            function_name = str(function_obj).split("#")[-1]
        else:
            function_name = span_name.replace("Span", "")
        
        # Extract label
        label_obj = g.value(span, RDFS.label)
        label = str(label_obj) if label_obj else span_name
        
        spans.append({
            "spanName": span_name,
            "functionName": function_name,
            "label": label
        })
    
    # Extract all functions
    for func in g.subjects(RDF.type, CNS.Function):
        func_uri = str(func)
        func_name = func_uri.split("#")[-1]
        
        # Extract API signature
        api_obj = g.value(func, CNS.providesAPI)
        api = str(api_obj) if api_obj else ""
        
        # Extract cycle cost
        cycle_cost_obj = g.value(func, CNS.cycleCost)
        cycle_cost = int(cycle_cost_obj) if cycle_cost_obj else 0
        
        # Extract label
        label_obj = g.value(func, RDFS.label)
        label = str(label_obj) if label_obj else func_name
        
        functions.append({
            "functionName": func_name,
            "api": api,
            "cycleCost": cycle_cost,
            "label": label
        })
    
    # Extract all patterns
    for pattern in g.subjects(RDF.type, CNS.Pattern):
        pattern_uri = str(pattern)
        pattern_name = pattern_uri.split("#")[-1]
        
        # Extract label
        label_obj = g.value(pattern, RDFS.label)
        label = str(label_obj) if label_obj else pattern_name
        
        # Extract comment
        comment_obj = g.value(pattern, RDFS.comment)
        comment = str(comment_obj) if comment_obj else ""
        
        patterns.append({
            "patternName": pattern_name,
            "label": label,
            "comment": comment
        })
    
    return {
        "spans": spans,
        "functions": functions,
        "patterns": patterns,
        "metadata": {
            "source": str(ttl_file),
            "extractedAt": "2025-07-19T00:00:00Z",
            "spanCount": len(spans),
            "functionCount": len(functions),
            "patternCount": len(patterns)
        }
    }

def generate_function_signatures(functions):
    """
    Generate C function signatures for injection.
    
    Args:
        functions: List of function dictionaries
        
    Returns:
        Dictionary mapping function names to signatures
    """
    signatures = {}
    
    for func in functions:
        func_name = func["functionName"]
        api = func["api"]
        
        # Extract return type and parameters from API string
        # Format: "ReturnType function_name(ParamType1, ParamType2, ...)"
        if "(" in api and ")" in api:
            # Split on first parenthesis
            parts = api.split("(", 1)
            if len(parts) == 2:
                return_type = parts[0].strip()
                params_part = parts[1].rstrip(")")
                
                # Build signature - use the original API as the signature
                signatures[func_name] = api
    
    return signatures

def generate_spans_json(data, output_file):
    """
    Generate JSON file with spans, functions, and patterns data.
    
    Args:
        data: Dictionary with spans, functions, patterns, and metadata
        output_file: Output JSON file path
    """
    # Generate function signatures
    data["functionSignatures"] = generate_function_signatures(data["functions"])
    
    # Ensure output directory exists
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_file, 'w') as f:
        json.dump(data, f, indent=2)
    
    print(f"Generated {output_file} with {len(data['spans'])} spans, {len(data['functions'])} functions, {len(data['patterns'])} patterns")

def main():
    """Main function."""
    if len(sys.argv) < 3:
        print("Usage: python extract_spans.py <ttl_file> <output_json>")
        print("Example: python extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json")
        sys.exit(1)
    
    ttl_file = sys.argv[1]
    output_json = sys.argv[2]
    
    if not os.path.exists(ttl_file):
        print(f"Error: TTL file {ttl_file} not found")
        sys.exit(1)
    
    print(f"Extracting spans, functions, and patterns from {ttl_file}...")
    data = extract_spans_from_ttl(ttl_file)
    
    if not data:
        print("Warning: No data found in TTL file")
        sys.exit(1)
    
    print(f"Found {len(data['spans'])} telemetry spans:")
    for span in data['spans']:
        print(f"  - {span['functionName']}: {span['label']}")
    
    print(f"Found {len(data['functions'])} functions:")
    for func in data['functions']:
        print(f"  - {func['functionName']} ({func['cycleCost']} cycles)")
    
    print(f"Found {len(data['patterns'])} patterns:")
    for pattern in data['patterns']:
        print(f"  - {pattern['patternName']}: {pattern['label']}")
    
    generate_spans_json(data, output_json)
    print("✓ Extraction complete")

if __name__ == "__main__":
    main() 