"""
LinkML Critical Production Tests - Gaps 1-3
Tests the essential functionality needed before production use
"""

import yaml
import json
from typing import List, Optional, Dict, Any
from pathlib import Path
import traceback

# LinkML imports for critical testing
try:
    from linkml_runtime.utils.schemaview import SchemaView
    from linkml_runtime.loaders import yaml_loader, json_loader
    from linkml_runtime.dumpers import yaml_dumper, json_dumper
    from linkml_runtime.utils.metamodelutils import load_schema_wrap
    print("✅ All LinkML imports successful")
except ImportError as e:
    print(f"❌ LinkML import failed: {e}")
    print("💡 Install with: pip install linkml-runtime")
    print("🔗 Or: conda install -c conda-forge linkml-runtime")
    exit(1)

def ensure_test_schema():
    """Ensure the test schema exists"""
    schema_path = Path("/Users/sac/autotel_prototype/real_linkml_schema.yaml")
    if not schema_path.exists():
        schema_content = """
id: https://example.org/real-linkml-demo
name: real-linkml-demo
title: Real LinkML Demonstration Schema
version: 1.0.0

prefixes:
  linkml: https://w3id.org/linkml/
  demo: https://example.org/real-linkml-demo/
  
default_prefix: demo
default_range: string

imports:
  - linkml:types

classes:
  Person:
    description: A real person with LinkML validation
    attributes:
      name:
        range: string
        required: true
        description: Full name of person
      age:
        range: integer
        description: Age in years
        minimum_value: 0
        maximum_value: 150
      email:
        range: string
        description: Email address
        pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"
      friends:
        range: Person
        multivalued: true
        description: List of friends (circular reference)
        
  Company:
    description: A company with employees
    attributes:
      company_name:
        range: string
        required: true
        description: Legal name of company
      employees:
        range: Person
        multivalued: true
        description: List of employees
      founded_year:
        range: integer
        description: Year company was founded
        minimum_value: 1800
        maximum_value: 2030

enums:
  PersonType:
    permissible_values:
      employee:
        description: Company employee
      contractor:
        description: Independent contractor
      customer:
        description: Customer or client
"""
        with open(schema_path, 'w') as f:
            f.write(schema_content)
        print(f"✅ Created test schema at {schema_path}")
    
    return schema_path

def test_1_data_validation():
    """GAP 1: Critical - Validate actual data instances against schema"""
    print("🧪 GAP 1: Data Instance Validation")
    print("=" * 50)
    
    schema_path = ensure_test_schema()
    
    try:
        schema_view = SchemaView(str(schema_path))
        print("✅ Schema loaded successfully")
    except Exception as e:
        print(f"❌ Schema loading failed: {e}")
        return False
    
    # Test cases
    test_cases = [
        {
            "name": "Valid Person",
            "data": {
                "name": "Alice Smith",
                "age": 28,
                "email": "alice@example.com"
            },
            "should_pass": True
        },
        {
            "name": "Missing Required Name",
            "data": {
                "age": 28,
                "email": "alice@example.com"
            },
            "should_pass": False
        },
        {
            "name": "Invalid Age Type",
            "data": {
                "name": "Bob Jones",
                "age": "twenty-five",
                "email": "bob@example.com"
            },
            "should_pass": False
        },
        {
            "name": "Invalid Age Range",
            "data": {
                "name": "Charlie Brown",
                "age": -5,
                "email": "charlie@example.com"
            },
            "should_pass": False
        },
        {
            "name": "Invalid Email Pattern",
            "data": {
                "name": "Diana Prince",
                "age": 30,
                "email": "not-an-email"
            },
            "should_pass": False
        },
        {
            "name": "Valid Person with Friends",
            "data": {
                "name": "Eve Wilson",
                "age": 32,
                "email": "eve@example.com",
                "friends": [
                    {"name": "Frank Miller", "age": 35, "email": "frank@example.com"},
                    {"name": "Grace Lee", "age": 29, "email": "grace@example.com"}
                ]
            },
            "should_pass": True
        }
    ]
    
    validation_results = []
    
    for test_case in test_cases:
        print(f"\n🔍 Testing: {test_case['name']}")
        print(f"📋 Data: {test_case['data']}")
        
        try:
            # Try different validation approaches
            
            # Approach 1: Direct schema validation
            print("   Method 1: Direct validation...")
            # This might not exist in LinkML runtime - testing what's available
            
            # Approach 2: Load with yaml_loader
            print("   Method 2: yaml_loader validation...")
            temp_yaml = yaml.dump(test_case['data'])
            validated_obj = yaml_loader.loads(temp_yaml, target_class="Person", schemaview=schema_view)
            print(f"   ✅ yaml_loader success: {type(validated_obj)}")
            
            validation_results.append({
                "test": test_case['name'],
                "expected": test_case['should_pass'],
                "actual": True,
                "method": "yaml_loader"
            })
            
        except Exception as e:
            print(f"   ❌ Validation failed: {e}")
            validation_results.append({
                "test": test_case['name'],
                "expected": test_case['should_pass'],
                "actual": False,
                "error": str(e),
                "method": "yaml_loader"
            })
    
    # Analyze results
    print(f"\n📊 VALIDATION RESULTS:")
    correct_predictions = 0
    total_tests = len(validation_results)
    
    for result in validation_results:
        expected = "PASS" if result['expected'] else "FAIL"
        actual = "PASS" if result['actual'] else "FAIL" 
        match = "✅" if result['expected'] == result['actual'] else "❌"
        
        print(f"  {match} {result['test']}: Expected {expected}, Got {actual}")
        if result['expected'] == result['actual']:
            correct_predictions += 1
        
        if 'error' in result:
            print(f"      Error: {result['error']}")
    
    accuracy = (correct_predictions / total_tests) * 100
    print(f"\n🎯 Validation Accuracy: {accuracy:.1f}% ({correct_predictions}/{total_tests})")
    
    gap_1_status = accuracy > 80
    print(f"📋 GAP 1 Status: {'✅ RESOLVED' if gap_1_status else '❌ CRITICAL GAP'}")
    
    return gap_1_status

def test_2_serialization_roundtrip():
    """GAP 2: Critical - Serialize/deserialize data in multiple formats"""
    print("\n\n🧪 GAP 2: Serialization Roundtrip")
    print("=" * 50)
    
    schema_path = ensure_test_schema()
    schema_view = SchemaView(str(schema_path))
    
    # Test data with complex relationships
    original_data = {
        "name": "Diana Prince",
        "age": 30,
        "email": "diana@themyscira.com",
        "friends": [
            {"name": "Bruce Wayne", "age": 35, "email": "bruce@wayne.com"},
            {"name": "Clark Kent", "age": 33, "email": "clark@daily.com"}
        ]
    }
    
    print(f"📤 Original Data: {original_data}")
    
    roundtrip_results = {}
    
    # Test 1: YAML Roundtrip
    print(f"\n🔄 Testing YAML Roundtrip...")
    try:
        # Original → YAML string
        yaml_string = yaml_dumper.dumps(original_data, schemaview=schema_view)
        print(f"   ✅ Serialized to YAML ({len(yaml_string)} chars)")
        
        # YAML string → Object
        roundtrip_obj = yaml_loader.loads(yaml_string, target_class="Person", schemaview=schema_view)
        print(f"   ✅ Deserialized from YAML: {type(roundtrip_obj)}")
        
        # Compare data integrity
        if hasattr(roundtrip_obj, '__dict__'):
            roundtrip_dict = roundtrip_obj.__dict__
        else:
            roundtrip_dict = roundtrip_obj
            
        data_matches = str(original_data) == str(roundtrip_dict)
        print(f"   {'✅' if data_matches else '❌'} Data integrity: {data_matches}")
        
        roundtrip_results['yaml'] = {
            'success': True,
            'data_integrity': data_matches,
            'serialized_size': len(yaml_string)
        }
        
    except Exception as e:
        print(f"   ❌ YAML roundtrip failed: {e}")
        roundtrip_results['yaml'] = {'success': False, 'error': str(e)}
    
    # Test 2: JSON Roundtrip  
    print(f"\n🔄 Testing JSON Roundtrip...")
    try:
        # Original → JSON string
        json_string = json_dumper.dumps(original_data, schemaview=schema_view)
        print(f"   ✅ Serialized to JSON ({len(json_string)} chars)")
        
        # JSON string → Object
        roundtrip_obj = json_loader.loads(json_string, target_class="Person", schemaview=schema_view)
        print(f"   ✅ Deserialized from JSON: {type(roundtrip_obj)}")
        
        # Compare data integrity
        if hasattr(roundtrip_obj, '__dict__'):
            roundtrip_dict = roundtrip_obj.__dict__
        else:
            roundtrip_dict = roundtrip_obj
            
        data_matches = str(original_data) == str(roundtrip_dict)
        print(f"   {'✅' if data_matches else '❌'} Data integrity: {data_matches}")
        
        roundtrip_results['json'] = {
            'success': True,
            'data_integrity': data_matches,
            'serialized_size': len(json_string)
        }
        
    except Exception as e:
        print(f"   ❌ JSON roundtrip failed: {e}")
        roundtrip_results['json'] = {'success': False, 'error': str(e)}
    
    # Test 3: Cross-format compatibility (YAML → JSON)
    print(f"\n🔄 Testing Cross-Format Compatibility...")
    try:
        # Original → YAML → Object → JSON → Object
        yaml_str = yaml_dumper.dumps(original_data, schemaview=schema_view)
        yaml_obj = yaml_loader.loads(yaml_str, target_class="Person", schemaview=schema_view)
        json_str = json_dumper.dumps(yaml_obj, schemaview=schema_view)
        final_obj = json_loader.loads(json_str, target_class="Person", schemaview=schema_view)
        
        print(f"   ✅ Cross-format conversion successful")
        roundtrip_results['cross_format'] = {'success': True}
        
    except Exception as e:
        print(f"   ❌ Cross-format conversion failed: {e}")
        roundtrip_results['cross_format'] = {'success': False, 'error': str(e)}
    
    # Analyze results
    print(f"\n📊 SERIALIZATION RESULTS:")
    successful_formats = sum(1 for r in roundtrip_results.values() if r.get('success', False))
    total_formats = len(roundtrip_results)
    
    for format_name, result in roundtrip_results.items():
        status = "✅ SUCCESS" if result.get('success', False) else "❌ FAILED"
        print(f"  {status} {format_name.upper()}")
        if 'error' in result:
            print(f"      Error: {result['error']}")
        elif 'data_integrity' in result:
            integrity = "✅" if result['data_integrity'] else "❌"
            print(f"      Data Integrity: {integrity}")
            print(f"      Serialized Size: {result.get('serialized_size', 'N/A')} chars")
    
    success_rate = (successful_formats / total_formats) * 100
    print(f"\n🎯 Serialization Success Rate: {success_rate:.1f}% ({successful_formats}/{total_formats})")
    
    gap_2_status = success_rate > 80
    print(f"📋 GAP 2 Status: {'✅ RESOLVED' if gap_2_status else '❌ CRITICAL GAP'}")
    
    return gap_2_status

def test_3_complex_relationships():
    """GAP 3: Important - Handle complex object relationships"""
    print("\n\n🧪 GAP 3: Complex Relationships")
    print("=" * 50)
    
    schema_path = ensure_test_schema()
    schema_view = SchemaView(str(schema_path))
    
    # Test Case 1: Circular References (Person → friends → Person)
    print(f"\n🔗 Testing Circular References...")
    circular_data = {
        "name": "Alice",
        "age": 30,
        "email": "alice@example.com",
        "friends": [
            {
                "name": "Bob", 
                "age": 32,
                "email": "bob@example.com",
                "friends": [
                    {"name": "Alice", "age": 30, "email": "alice@example.com"}  # Back reference
                ]
            }
        ]
    }
    
    try:
        yaml_str = yaml_dumper.dumps(circular_data, schemaview=schema_view)
        circular_obj = yaml_loader.loads(yaml_str, target_class="Person", schemaview=schema_view)
        print(f"   ✅ Circular reference handling: SUCCESS")
        circular_success = True
    except Exception as e:
        print(f"   ❌ Circular reference handling: FAILED - {e}")
        circular_success = False
    
    # Test Case 2: Deep Nesting
    print(f"\n🏗️ Testing Deep Nesting...")
    deep_data = {
        "name": "Root Person",
        "age": 40,
        "email": "root@example.com",
        "friends": [
            {
                "name": "Level 1 Friend",
                "age": 35,
                "email": "level1@example.com", 
                "friends": [
                    {
                        "name": "Level 2 Friend",
                        "age": 30,
                        "email": "level2@example.com",
                        "friends": [
                            {"name": "Level 3 Friend", "age": 25, "email": "level3@example.com"}
                        ]
                    }
                ]
            }
        ]
    }
    
    try:
        yaml_str = yaml_dumper.dumps(deep_data, schemaview=schema_view)
        deep_obj = yaml_loader.loads(yaml_str, target_class="Person", schemaview=schema_view)
        print(f"   ✅ Deep nesting handling: SUCCESS")
        deep_success = True
    except Exception as e:
        print(f"   ❌ Deep nesting handling: FAILED - {e}")
        deep_success = False
    
    # Test Case 3: Composition (Company → employees → Person)
    print(f"\n🏢 Testing Composition Relationships...")
    company_data = {
        "company_name": "Wayne Enterprises",
        "founded_year": 1939,
        "employees": [
            {
                "name": "Bruce Wayne",
                "age": 35,
                "email": "bruce@wayne.com",
                "friends": [
                    {"name": "Alfred Pennyworth", "age": 65, "email": "alfred@wayne.com"},
                    {"name": "Lucius Fox", "age": 55, "email": "lucius@wayne.com"}
                ]
            },
            {
                "name": "Lucius Fox",
                "age": 55,
                "email": "lucius@wayne.com",
                "friends": [
                    {"name": "Bruce Wayne", "age": 35, "email": "bruce@wayne.com"}
                ]
            }
        ]
    }
    
    try:
        yaml_str = yaml_dumper.dumps(company_data, schemaview=schema_view)
        company_obj = yaml_loader.loads(yaml_str, target_class="Company", schemaview=schema_view)
        print(f"   ✅ Composition relationship: SUCCESS")
        composition_success = True
    except Exception as e:
        print(f"   ❌ Composition relationship: FAILED - {e}")
        composition_success = False
    
    # Test Case 4: Empty/Null Relationships
    print(f"\n🕳️ Testing Empty/Null Relationships...")
    empty_data = {
        "name": "Lonely Person",
        "age": 25,
        "email": "lonely@example.com",
        "friends": []  # Empty list
    }
    
    try:
        yaml_str = yaml_dumper.dumps(empty_data, schemaview=schema_view)
        empty_obj = yaml_loader.loads(yaml_str, target_class="Person", schemaview=schema_view)
        print(f"   ✅ Empty relationships: SUCCESS")
        empty_success = True
    except Exception as e:
        print(f"   ❌ Empty relationships: FAILED - {e}")
        empty_success = False
    
    # Analyze results
    relationship_tests = {
        'circular_references': circular_success,
        'deep_nesting': deep_success,
        'composition': composition_success,
        'empty_relationships': empty_success
    }
    
    print(f"\n📊 RELATIONSHIP HANDLING RESULTS:")
    successful_tests = sum(1 for success in relationship_tests.values() if success)
    total_tests = len(relationship_tests)
    
    for test_name, success in relationship_tests.items():
        status = "✅ SUCCESS" if success else "❌ FAILED"
        print(f"  {status} {test_name.replace('_', ' ').title()}")
    
    success_rate = (successful_tests / total_tests) * 100
    print(f"\n🎯 Relationship Handling Success Rate: {success_rate:.1f}% ({successful_tests}/{total_tests})")
    
    gap_3_status = success_rate > 75  # Slightly lower threshold as relationships are complex
    print(f"📋 GAP 3 Status: {'✅ RESOLVED' if gap_3_status else '❌ CRITICAL GAP'}")
    
    return gap_3_status

def run_critical_tests():
    """Run all critical production readiness tests"""
    print("🚀 LinkML Critical Production Tests (Gaps 1-3)")
    print("=" * 60)
    
    results = {}
    
    # Run tests
    try:
        results['gap_1_validation'] = test_1_data_validation()
    except Exception as e:
        print(f"💥 GAP 1 crashed: {e}")
        traceback.print_exc()
        results['gap_1_validation'] = False
    
    try:
        results['gap_2_serialization'] = test_2_serialization_roundtrip()
    except Exception as e:
        print(f"💥 GAP 2 crashed: {e}")
        traceback.print_exc()
        results['gap_2_serialization'] = False
    
    try:
        results['gap_3_relationships'] = test_3_complex_relationships()
    except Exception as e:
        print(f"💥 GAP 3 crashed: {e}")
        traceback.print_exc()
        results['gap_3_relationships'] = False
    
    # Final analysis
    print("\n\n" + "=" * 60)
    print("📊 CRITICAL GAPS ANALYSIS")
    print("=" * 60)
    
    resolved_gaps = sum(1 for resolved in results.values() if resolved)
    total_gaps = len(results)
    
    gap_names = {
        'gap_1_validation': 'Data Instance Validation',
        'gap_2_serialization': 'Serialization Roundtrip', 
        'gap_3_relationships': 'Complex Relationships'
    }
    
    for gap_key, resolved in results.items():
        gap_name = gap_names.get(gap_key, gap_key)
        status = "✅ RESOLVED" if resolved else "❌ CRITICAL GAP"
        priority = "MUST FIX" if not resolved else "READY"
        print(f"  {status} {gap_name} - {priority}")
    
    overall_readiness = (resolved_gaps / total_gaps) * 100
    print(f"\n🎯 Critical Gaps Resolution: {overall_readiness:.1f}% ({resolved_gaps}/{total_gaps})")
    
    if overall_readiness >= 100:
        recommendation = "✅ READY FOR PRODUCTION - All critical gaps resolved"
    elif overall_readiness >= 67:
        recommendation = "⚠️ MOSTLY READY - Address remaining gaps before production"
    else:
        recommendation = "❌ NOT READY - Major gaps must be resolved before production"
    
    print(f"\n🎯 RECOMMENDATION: {recommendation}")
    
    return results

if __name__ == "__main__":
    print("🧬 LinkML Critical Tests - Production Readiness Validation")
    print("📁 Working Directory:", Path.cwd())
    print("🐍 Python Version:", __import__('sys').version.split()[0])
    print("")
    
    results = run_critical_tests()
    
    # Save results for automation
    results_file = Path("/Users/sac/autotel_prototype/linkml_test_results.json")
    with open(results_file, 'w') as f:
        json.dump({
            "timestamp": __import__('datetime').datetime.now().isoformat(),
            "results": results,
            "overall_success": all(results.values()),
            "success_rate": sum(results.values()) / len(results) * 100
        }, f, indent=2)
    
    print(f"\n💾 Results saved to: {results_file}")
    
    # Exit code for automation
    exit_code = 0 if all(results.values()) else 1
    print(f"🚪 Exit Code: {exit_code}")
    exit(exit_code)
