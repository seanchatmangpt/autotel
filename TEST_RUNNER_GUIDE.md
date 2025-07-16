# AutoTel Test Runner Guide

This guide shows you how to control test execution using pytest decorators and the test runner script.

## Quick Commands

### Using the Test Runner Script
```bash
# Run fast tests only (unit tests, no LLM calls)
python run_tests.py fast

# Run all tests except manual ones
python run_tests.py all

# Run only DSPy/LLM tests
python run_tests.py dspy

# Run integration tests
python run_tests.py integration

# Run slow tests
python run_tests.py slow

# Run manual tests (requires --runxfail)
python run_tests.py manual

# Run specific test file
python run_tests.py dspy --file test_user_interaction_dspy_jinja.py

# Verbose output
python run_tests.py dspy --verbose
```

### Using Pytest Directly
```bash
# Default (fast tests only)
pytest

# Run all tests except manual
pytest -m "not skip"

# Run only DSPy tests
pytest -m "dspy"

# Run integration tests
pytest -m "integration"

# Run slow tests
pytest -m "slow"

# Run multiple categories
pytest -m "dspy or integration"

# Exclude categories
pytest -m "not slow and not dspy"

# Run manual tests
pytest --runxfail
```

## Pytest Decorators

### Skip Tests
```python
@pytest.mark.skip(reason="Manual test only - requires user interaction")
def test_manual_dspy_verification():
    """Manual test to verify DSPy is hitting the model"""
    pass
```

### Mark Tests by Type
```python
@pytest.mark.slow
@pytest.mark.integration
@pytest.mark.dspy
def test_dspy_response_timing():
    """Test DSPy response times to verify model calls"""
    pass
```

### Conditional Skip
```python
@pytest.mark.skipif(not hasattr(sys, 'real_prefix'), reason="Requires virtual environment")
def test_environment_specific():
    pass
```

## Test Categories

### 🏃 Fast Tests (`@pytest.mark.unit`)
- Unit tests
- No external dependencies
- No LLM calls
- Run by default

### 🤖 DSPy Tests (`@pytest.mark.dspy`)
- Tests that use DSPy/LLM calls
- May be slow due to model inference
- Include randomization to prevent caching
- Run with: `python run_tests.py dspy`

### 🔗 Integration Tests (`@pytest.mark.integration`)
- End-to-end workflow tests
- May involve multiple components
- Can be slow
- Run with: `python run_tests.py integration`

### 🐌 Slow Tests (`@pytest.mark.slow`)
- Tests that take a long time
- May involve heavy computation
- Run with: `python run_tests.py slow`

### 👤 Manual Tests (`@pytest.mark.skip`)
- Tests requiring manual intervention
- Skipped by default
- Run with: `python run_tests.py manual`

## Configuration

The `pytest.ini` file configures default behavior:

```ini
[tool:pytest]
# Default settings - run fast tests only
addopts = 
    -v
    --tb=short
    --strict-markers
    -m "not slow and not integration and not dspy"
    --durations=10
```

## Best Practices

### 1. Mark Your Tests Appropriately
```python
# Fast unit test
def test_simple_function():
    assert 2 + 2 == 4

# DSPy test
@pytest.mark.dspy
@pytest.mark.slow
def test_llm_response():
    result = dspy_module("test input")
    assert result is not None

# Integration test
@pytest.mark.integration
def test_full_workflow():
    # Test complete workflow
    pass
```

### 2. Use Descriptive Skip Reasons
```python
@pytest.mark.skip(reason="Requires GPU - run manually on GPU machine")
def test_gpu_acceleration():
    pass
```

### 3. Group Related Tests
```python
@pytest.mark.dspy
class TestDSPyFunctionality:
    def test_response_generation(self):
        pass
    
    def test_cache_control(self):
        pass
```

## Common Patterns

### Testing DSPy with Randomization
```python
@pytest.mark.dspy
def test_dspy_no_caching():
    """Test that DSPy is actually hitting the model"""
    import uuid
    import time
    
    random_seed = f'{uuid.uuid4().hex[:8]}_{int(time.time())}'
    result = dspy_module(random_seed=random_seed)
    
    # Verify response time is realistic
    assert result.response_time > 1.0, "Response too fast - possible caching"
```

### Conditional Testing
```python
@pytest.mark.skipif(
    os.getenv('CI') == 'true',
    reason="LLM tests disabled in CI"
)
def test_llm_integration():
    pass
```

## Troubleshooting

### Warnings About Unknown Markers
If you see warnings about unknown pytest markers, make sure your `pytest.ini` includes the marker definitions:

```ini
markers =
    slow: marks tests as slow
    integration: marks tests as integration tests
    dspy: marks tests that use DSPy/LLM calls
    unit: marks tests as unit tests
    manual: marks tests that require manual intervention
```

### Tests Not Running
- Check that your test functions start with `test_`
- Verify the test file matches the pattern in `pytest.ini`
- Use `pytest --collect-only` to see what tests are discovered

### Slow Test Execution
- Use `pytest -m "not slow"` to skip slow tests
- Use `pytest -k "not test_slow_function"` to skip specific tests
- Consider parallel execution with `pytest-xdist`

## Examples

### Running Different Test Suites
```bash
# Development workflow
python run_tests.py fast          # Quick feedback
python run_tests.py dspy          # Test LLM integration
python run_tests.py integration   # Full workflow tests

# CI/CD pipeline
python run_tests.py all           # Run everything except manual

# Debugging
python run_tests.py dspy --file test_specific.py --verbose
```

This setup gives you fine-grained control over test execution, allowing you to run fast tests during development and comprehensive tests when needed. 