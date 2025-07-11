.PHONY: help install test lint format clean docs run demo setup

# Default target
help:
	@echo "AutoTel Prototype - Development Commands"
	@echo "========================================"
	@echo "install    - Install dependencies"
	@echo "setup      - Setup development environment"
	@echo "test       - Run tests"
	@echo "lint       - Run linting checks"
	@echo "format     - Format code with black and isort"
	@echo "clean      - Clean up generated files"
	@echo "docs       - Build documentation"
	@echo "run        - Run the orchestrator"
	@echo "demo       - Run demo with sample BPMN process"
	@echo ""

# Install dependencies
install:
	@echo "Installing dependencies..."
	pip install -r requirements.txt

# Setup development environment
setup: install
	@echo "Setting up development environment..."
	pip install -e .
	@echo "Development environment ready!"

# Run tests
test:
	@echo "Running tests..."
	pytest tests/ -v --cov=bpmn_orchestrator --cov-report=html --cov-report=term

# Run linting
lint:
	@echo "Running linting checks..."
	flake8 bpmn_orchestrator.py autotel_prototype.py tests/ --max-line-length=88 --ignore=E203,W503
	mypy bpmn_orchestrator.py autotel_prototype.py --ignore-missing-imports

# Format code
format:
	@echo "Formatting code..."
	black bpmn_orchestrator.py autotel_prototype.py tests/ --line-length=88
	isort bpmn_orchestrator.py autotel_prototype.py tests/ --profile=black

# Clean up generated files
clean:
	@echo "Cleaning up..."
	find . -type f -name "*.pyc" -delete
	find . -type d -name "__pycache__" -delete
	find . -type d -name "*.egg-info" -exec rm -rf {} +
	rm -rf build/
	rm -rf dist/
	rm -rf htmlcov/
	rm -rf .pytest_cache/
	rm -rf .mypy_cache/
	rm -rf docs/_build/
	@echo "Cleanup complete!"

# Build documentation
docs:
	@echo "Building documentation..."
	cd docs && make html
	@echo "Documentation built in docs/_build/html/"

# Run the orchestrator
run:
	@echo "Starting AutoTel BPMN Orchestrator..."
	python bpmn_orchestrator.py

# Run demo with sample process
demo:
	@echo "Running AutoTel demo..."
	python run_demo.py

# Development workflow
dev: format lint test

# Pre-commit checks
pre-commit: format lint test

# Full development setup
full-setup: setup format lint test docs 