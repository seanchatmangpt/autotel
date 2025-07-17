# AutoTel Project Cleanup Guide (Updated)

## 🎯 Project Overview

AutoTel is an enterprise-grade BPMN 2.0 orchestration engine with **zero-touch telemetry integration** using **LinkML schema validation**. The project demonstrates schema-driven observability where all telemetry operations are validated against LinkML schemas, eliminating hardcoded strings.

## ⚠️ **BREAKAGE RISK ASSESSMENT & CHECKLIST**

### 🔍 **CRITICAL BREAKAGE POINTS IDENTIFIED**

#### **1. Schema File References (HIGH RISK)**
```
❌ BREAKS: autotel/core/telemetry.py
├── Line: schema_path = Path(__file__).parent.parent.parent / "otel_traces_schema.yaml"
├── Issue: Hardcoded path assumes file is in root directory
├── Impact: Telemetry validation will fail
└── Fix Required: Update path to "schemas/otel_traces_schema.yaml"

❌ BREAKS: autotel/cli.py
├── Lines: "otel_traces_schema.yaml", "workflow_telemetry_schema.yaml"
├── Issue: CLI expects schema files in root directory
├── Impact: CLI commands will fail to find schema files
└── Fix Required: Update CLI to look in schemas/ directory

❌ BREAKS: autotel/core/framework.py
├── Line: init_result = framework.initialize("autotel_demo_ontology.yaml")
├── Issue: References file that's already in examples/ directory
├── Impact: Framework initialization will fail
└── Fix Required: Update path to "examples/autotel_demo_ontology.yaml"
```

#### **2. Test File References (MEDIUM RISK)**
```
❌ BREAKS: tests/run_comprehensive_tests.py
├── Lines: Multiple references to "tests/test_comprehensive_suite.py"
├── Issue: Test runner expects test files in tests/ directory
├── Impact: Test suite execution will fail
└── Fix Required: Update test runner paths

❌ BREAKS: tests/test_camunda_engine.py
├── Line: bpmn_path = "trivial.bpmn"
├── Issue: Expects trivial.bpmn in root directory
├── Impact: Camunda engine tests will fail
└── Fix Required: Update path to "schemas/trivial.bpmn"
```

#### **3. Self-References in Test Files (LOW RISK)**
```
❌ BREAKS: test_pipeline.py
├── Line: span.set_attribute("script_name", "test_pipeline.py")
├── Issue: Self-reference for telemetry
├── Impact: Telemetry will show wrong script name
└── Fix Required: Update script name reference
```

### 📋 **COMPREHENSIVE BREAKAGE CHECKLIST**

#### **Before Moving Files:**
- [ ] **Backup current state**: `git stash` or create backup branch
- [ ] **Document current functionality**: Run all tests and note results
- [ ] **Check active development**: Ensure no uncommitted changes
- [ ] **Verify environment**: Confirm `.venv/` is working

#### **Schema Files (5 files) → `schemas/`:**
- [ ] **Update `autotel/core/telemetry.py`**: Fix `otel_traces_schema.yaml` path
- [ ] **Update `autotel/cli.py`**: Fix schema file paths in CLI commands
- [ ] **Update `autotel/core/framework.py`**: Fix `autotel_demo_ontology.yaml` path
- [ ] **Update `tests/test_camunda_engine.py`**: Fix `trivial.bpmn` path
- [ ] **Test telemetry functionality**: Verify schema validation still works
- [ ] **Test CLI commands**: Verify all CLI schema operations work
- [ ] **Test framework initialization**: Verify framework starts correctly

#### **Test Files (34 files) → `tests/`:**
- [ ] **Update `tests/run_comprehensive_tests.py`**: Fix test file paths
- [ ] **Check for cross-test imports**: Verify no test files import each other
- [ ] **Update any hardcoded test paths**: Check for absolute paths
- [ ] **Test test execution**: Run `python -m pytest tests/`
- [ ] **Test individual test files**: Verify each test can run independently
- [ ] **Check test discovery**: Ensure pytest can find all tests

#### **Script Files (6 files) → `scripts/`:**
- [ ] **Check for script dependencies**: Verify no scripts import each other
- [ ] **Update any hardcoded paths**: Check for absolute file references
- [ ] **Test script execution**: Run each script from new location
- [ ] **Check for CLI integration**: Verify scripts work with main CLI
- [ ] **Update documentation**: Fix any references to script locations

#### **Documentation Files (31 files) → `docs/`:**
- [ ] **Check for internal links**: Fix any markdown links between docs
- [ ] **Update any file references**: Fix paths to other project files
- [ ] **Check for image references**: Fix any image paths
- [ ] **Update documentation index**: Fix any file listings
- [ ] **Test documentation build**: Verify docs can be built/generated
- [ ] **Check for external references**: Fix any links to moved files

#### **After Moving Files:**
- [ ] **Run full test suite**: `python -m pytest tests/`
- [ ] **Test CLI functionality**: `python -m autotel.cli --help`
- [ ] **Test telemetry**: Verify OpenTelemetry spans are generated
- [ ] **Test schema validation**: Verify LinkML validation works
- [ ] **Test framework initialization**: Verify AutoTel framework starts
- [ ] **Test all scripts**: Run each moved script
- [ ] **Check for import errors**: Look for any remaining broken imports
- [ ] **Verify documentation**: Check that docs are accessible

#### **Configuration Updates Required:**
- [ ] **Update `pyproject.toml`**: Add new directories to package data
- [ ] **Update `.gitignore`**: Ensure new directories are tracked
- [ ] **Update `setup.sh`**: Fix any hardcoded paths
- [ ] **Update `Makefile`**: Fix any build/test targets
- [ ] **Update CI/CD**: Fix any pipeline paths

### 🚨 **HIGH PRIORITY FIXES REQUIRED**

#### **1. Critical Path Updates**
```python
# autotel/core/telemetry.py - Line ~XX
# BEFORE:
schema_path = Path(__file__).parent.parent.parent / "otel_traces_schema.yaml"
# AFTER:
schema_path = Path(__file__).parent.parent.parent / "schemas" / "otel_traces_schema.yaml"

# autotel/cli.py - Lines ~XX
# BEFORE:
"otel_traces_schema.yaml",
"workflow_telemetry_schema.yaml"
# AFTER:
"schemas/otel_traces_schema.yaml",
"schemas/workflow_telemetry_schema.yaml"

# autotel/core/framework.py - Line ~XX
# BEFORE:
init_result = framework.initialize("autotel_demo_ontology.yaml")
# AFTER:
init_result = framework.initialize("examples/autotel_demo_ontology.yaml")
```

#### **2. Test Path Updates**
```python
# tests/test_camunda_engine.py - Line ~XX
# BEFORE:
bpmn_path = "trivial.bpmn"
# AFTER:
bpmn_path = "schemas/trivial.bpmn"

# tests/run_comprehensive_tests.py - Multiple lines
# BEFORE:
"tests/test_comprehensive_suite.py"
# AFTER:
"tests/test_comprehensive_suite.py"  # Already correct
```

### 🔧 **SAFE MIGRATION STRATEGY**

#### **Phase 1: Preparation**
1. Create backup branch: `git checkout -b backup-before-cleanup`
2. Document current state: Run all tests and note results
3. Create new directories: `mkdir -p docs scripts schemas`

#### **Phase 2: Fix Critical Paths First**
1. Update `autotel/core/telemetry.py` schema path
2. Update `autotel/cli.py` schema file references
3. Update `autotel/core/framework.py` ontology path
4. Test core functionality before moving files

#### **Phase 3: Move Files with Testing**
1. Move schema files → test immediately
2. Move test files → test immediately
3. Move script files → test immediately
4. Move documentation files → test immediately

#### **Phase 4: Final Validation**
1. Run complete test suite
2. Test all CLI commands
3. Test all scripts
4. Verify documentation accessibility

### 📊 **Current Environment Analysis**

### ✅ **ACTIVE ENVIRONMENT (KEEP - Currently In Use)**
```
.venv/                           # ✅ ACTIVE - Currently in use
├── Created by: uv 0.7.20
├── Python: 3.13.0
├── Status: ACTIVE (VIRTUAL_ENV=/Users/sac/autotel/.venv)
├── Prompt: autotel-prototype
├── Current Python: /Users/sac/autotel/.venv/bin/python
└── Package Manager: uv 0.7.20
```

### ❌ **GENERATED/CACHE FILES (SAFE TO REMOVE)**
```
.lh/                             # ❌ Cursor IDE cache (132 files, 7MB+)
├── Contains: JSON cache files for Cursor IDE
├── Files: .cursorrules.json, .gitignore.json, *.md.json, etc.
└── Safe to remove: Yes

__pycache__/                     # ❌ Python cache directories
├── ./tests/__pycache__/
├── ./__pycache__/
├── ./spiff-example-cli/.../__pycache__/ (multiple locations)
└── Safe to remove: Yes

autotel_prototype.egg-info/      # ❌ Python egg info
├── Generated during package installation
└── Safe to remove: Yes

autotel_process_instances.sqlite3 # ❌ Generated SQLite DB
├── Size: 12KB
├── Auto-created by application
└── Safe to remove: Yes (will be recreated automatically)
```

## 📁 Root Directory Analysis (93 Files Total)

### ✅ **ROOT-LEVEL CONFIGURATION FILES (KEEP IN ROOT)**
```
├── .cursorrules                 # ✅ Cursor IDE rules (must be in root)
├── .gitattributes               # ✅ Git attributes (must be in root)
├── .gitignore                   # ✅ Git ignore rules (must be in root)
├── pyproject.toml               # ✅ Package configuration (must be in root)
├── requirements.txt             # ✅ Dependencies (must be in root)
├── uv.lock                      # ✅ UV dependency lock (502KB, must be in root)
├── setup.sh                     # ✅ Setup script (must be in root)
├── Makefile                     # ✅ Build automation (must be in root)
└── LICENSE                      # ✅ License file (must be in root)
```

### ✅ **ACTIVE DEVELOPMENT FILES (KEEP - Recent Commits)**
```
autotel/                          # ✅ Main package - ACTIVE DEVELOPMENT
├── __init__.py                   # ✅ Active since initial commit
├── cli.py                        # ✅ Recent commits (last 5 commits)
├── core/
│   ├── __init__.py              # ✅ Active development
│   ├── framework.py             # ✅ Active since initial commit
│   ├── orchestrator.py          # ✅ Recent active development
│   └── telemetry.py             # ✅ Recent active development
├── factory/                      # ✅ Recent active development
├── schemas/                      # ✅ Recent active development
├── utils/                        # ✅ Recent active development
└── workflows/                    # ✅ Recent active development
```

### 📁 **FILES TO MOVE TO APPROPRIATE SUBDIRECTORIES**

#### **Documentation Files (31 files) → Move to `docs/`**
```
# All .md files should move to docs/
docs/
├── 80_20_IMPLEMENTATION_SUMMARY.md
├── AUTOTEL_PRD.md
├── AUTOTEL_V5_PROJECT_CHARTER.md
├── AUTOTEL_XML_SPECIFICATION.md
├── c4_validation_diagrams.md
├── CLEANUP.md
├── COMPLETE_INTEGRATION_SUMMARY.md
├── COMPLETE_SYSTEM_ANALYSIS.md
├── DEFINITION-OF-DONE-v1.md
├── DMN_USAGE_GUIDE.md
├── DOCUMENTATION_INDEX.md
├── DSPY_DMN_INTEGRATION_GUIDE.md
├── DYNAMIC_USER_INTERACTION_SUMMARY.md
├── FACTORY_DOCUMENTATION.md
├── GAPS_AND_MISTAKES.md
├── git_implementation_history.md
├── OTEL_PROCESSOR_DOCUMENTATION.md
├── OTEL_PROCESSOR_VALIDATION_SUMMARY.md
├── README.md (keep in root, copy to docs/)
├── README_PIPELINE.md
├── SPIFF_CUSTOM_DEFINITIONS_TUTORIAL.md
├── TELEMETRY_VALIDATION_ANALYSIS.md
├── TELEMETRY-FIRST-CAPABILITIES.md
├── TEST_RUNNER_GUIDE.md
├── updated_spiff_tutorial.md
├── USER_INTERACTION_SUMMARY.md
├── WALKING_SKELETON_HUMAN_LLM_BPMN.md
└── WEAVER_TELEMETRY_PLANNING.md
```

#### **Test Files (34 files) → Move to `tests/`**
```
# All test_*.py files should move to tests/
tests/
├── test_80_20_telemetry_validation.py
├── test_all_processors_integration.py
├── test_camunda_parser.py
├── test_camunda_simple_dspy.py
├── test_camunda_simple.py
├── test_chain_of_thought.py
├── test_complete_integration.py
├── test_complex_bpmn.py
├── test_complex_camunda.py
├── test_dspy_execution_with_telemetry.py
├── test_dynamic_dspy_jinja_bpmn.py
├── test_dynamic_email_integration.py
├── test_dynamic_signature_dspy.py
├── test_e2e_pipeline.py
├── test_jinja_processor.py
├── test_jinja2_support.py
├── test_minimal.py
├── test_no_telemetry_demo.py
├── test_ontology_compiler_end_to_end.py
├── test_ontology_compiler_mock.py
├── test_ontology_compiler_quiet.py
├── test_ontology_compiler_simple.py
├── test_ontology_dspy_integration.py
├── test_owl_only.py
├── test_owl_processor_end_to_end.py
├── test_owl_simple.py
├── test_pipeline.py
├── test_simple_camunda.py
├── test_simple_spiff.py
├── test_telemetry_demo.py
├── test_user_interaction_dspy_jinja.py
├── test_user_interaction_scenarios.py
├── test_user_interaction.py
└── test_working_telemetry.py
```

#### **Schema Files (5 files) → Move to `schemas/`**
```
# Schema files should move to schemas/
schemas/
├── autotel_telemetry.yaml
├── otel_traces_schema.yaml
├── workflow_telemetry_schema.yaml
├── workflow_telemetry_span.linkml.yaml
└── trivial.bpmn
```

#### **Scripts (6 files) → Move to `scripts/`**
```
# Utility scripts should move to scripts/
scripts/
├── run_demo.py
├── run_tests.py
├── service_and_decision_capability.py
├── validate_camunda_examples.py
├── validate_service_and_decision.py
└── autotel_cli.py (legacy file - safe to remove)
```

### ❌ **GENERATED/LOG FILES (SAFE TO REMOVE)**
```
├── debug_run.log                 # ❌ Debug log file (17KB)
├── debug_run2.log                # ❌ Debug log file (18KB)
├── debug_run3.log                # ❌ Debug log file (18KB)
├── compiled_ontology_mock.json   # ❌ Generated mock data (8KB)
├── parsed_ontology.json          # ❌ Generated parsed data (14KB)
├── telemetry_export.json         # ❌ Generated telemetry export (1KB)
└── autotel_process_instances.sqlite3 # ❌ Generated SQLite DB (12KB)
```

## 🧹 **COMPREHENSIVE CLEANUP & ORGANIZATION ACTIONS**

### Phase 1: Remove Generated/Cache Files
```bash
# Remove Cursor IDE cache (safe)
rm -rf .lh/

# Remove Python cache directories (safe)
rm -rf __pycache__/
find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null

# Remove generated files (safe)
rm -rf autotel_prototype.egg-info/
rm -f autotel_process_instances.sqlite3

# Remove debug log files (safe)
rm -f debug_run*.log

# Remove generated JSON files (safe)
rm -f compiled_ontology_mock.json
rm -f parsed_ontology.json
rm -f telemetry_export.json

# Remove legacy file (safe - not in git history)
rm -f autotel_cli.py

# KEEP the active environment
# .venv/ - DO NOT REMOVE (currently active)
```

### Phase 2: Create Directory Structure
```bash
# Create organized directory structure
mkdir -p docs
mkdir -p scripts
mkdir -p schemas
mkdir -p examples
```

### Phase 3: Fix Critical Paths BEFORE Moving Files
```bash
# Update critical file paths in code
# (This must be done BEFORE moving files)

# 1. Update autotel/core/telemetry.py
# Change: Path(__file__).parent.parent.parent / "otel_traces_schema.yaml"
# To: Path(__file__).parent.parent.parent / "schemas" / "otel_traces_schema.yaml"

# 2. Update autotel/cli.py
# Change: "otel_traces_schema.yaml", "workflow_telemetry_schema.yaml"
# To: "schemas/otel_traces_schema.yaml", "schemas/workflow_telemetry_schema.yaml"

# 3. Update autotel/core/framework.py
# Change: "autotel_demo_ontology.yaml"
# To: "examples/autotel_demo_ontology.yaml"

# 4. Update tests/test_camunda_engine.py
# Change: "trivial.bpmn"
# To: "schemas/trivial.bpmn"
```

### Phase 4: Move Files to Appropriate Directories
```bash
# Move documentation files
mv *.md docs/ 2>/dev/null || true
# Keep README.md in root
cp docs/README.md . 2>/dev/null || true

# Move test files
mv test_*.py tests/ 2>/dev/null || true

# Move schema files
mv *.yaml schemas/ 2>/dev/null || true
mv *.linkml.yaml schemas/ 2>/dev/null || true
mv trivial.bpmn schemas/ 2>/dev/null || true

# Move scripts
mv run_*.py scripts/ 2>/dev/null || true
mv service_and_decision_capability.py scripts/ 2>/dev/null || true
mv validate_*.py scripts/ 2>/dev/null || true
```

### Phase 5: Verify Organization
```bash
# Check what was moved
echo "=== Root directory files ==="
ls -la | grep -v "^d" | grep -v "^total" | wc -l

echo "=== Documentation files ==="
ls -la docs/ | grep -E "\.md$" | wc -l

echo "=== Test files ==="
ls -la tests/ | grep -E "test_.*\.py$" | wc -l

echo "=== Schema files ==="
ls -la schemas/ | grep -E "\.(yaml|bpmn)$" | wc -l

echo "=== Script files ==="
ls -la scripts/ | grep -E "\.py$" | wc -l

# Verify active environment still works
source .venv/bin/activate
python --version
uv --version
```

## 📊 **FINAL PROJECT STRUCTURE**

After cleanup and organization, the project will have this clean structure:

```
autotel/
├── .venv/                        # ✅ ACTIVE ENVIRONMENT (KEEP)
├── autotel/                      # ✅ Main package (ACTIVE DEVELOPMENT)
├── bpmn/                         # ✅ BPMN process definitions
├── docs/                         # ✅ Documentation (31 files moved)
├── examples/                     # ✅ Example files
├── jira/                         # ✅ JIRA documentation
├── ontologies/                   # ✅ Ontology files
├── schemas/                      # ✅ Schema files (5 files moved)
├── scripts/                      # ✅ Utility scripts (6 files moved)
├── spiff-example-cli/            # ✅ SpiffWorkflow examples
├── tests/                        # ✅ Test suite (34 files moved)
├── .cursorrules                  # ✅ IDE rules (ROOT)
├── .gitattributes                # ✅ Git attributes (ROOT)
├── .gitignore                    # ✅ Git ignore (ROOT)
├── LICENSE                       # ✅ License (ROOT)
├── Makefile                      # ✅ Build automation (ROOT)
├── pyproject.toml               # ✅ Package config (ROOT)
├── README.md                    # ✅ Main documentation (ROOT)
├── requirements.txt             # ✅ Dependencies (ROOT)
├── setup.sh                     # ✅ Setup script (ROOT)
└── uv.lock                      # ✅ UV lock file (ROOT)
```

## 🎯 **KEY INSIGHTS FROM ANALYSIS**

### 1. **Root Directory Should Be Clean**
- Only **9 configuration files** should remain in root
- **76 files** should be moved to appropriate subdirectories
- **8 files** should be removed (generated/cache)

### 2. **Proper Organization Benefits**
- **Documentation**: All `.md` files → `docs/`
- **Tests**: All `test_*.py` files → `tests/`
- **Schemas**: All `.yaml` and `.bpmn` files → `schemas/`
- **Scripts**: All utility scripts → `scripts/`

### 3. **Root-Level Requirements**
- **Configuration files** must stay in root (pyproject.toml, .gitignore, etc.)
- **README.md** should stay in root for GitHub visibility
- **Active environment** (`.venv/`) must stay in root

### 4. **Active Development Pattern**
- `autotel/` module has **continuous development** since initial commit
- Recent commits show **active feature development** in core modules
- All moved files are **recently added** and actively used

## 🚀 **RECOMMENDED APPROACH**

### ✅ **DO: Safe Cleanup & Organization**
1. Remove generated/cache files (8 files)
2. Keep active `.venv/` environment
3. **Fix critical paths BEFORE moving files**
4. Move documentation to `docs/` (31 files)
5. Move tests to `tests/` (34 files)
6. Move schemas to `schemas/` (5 files)
7. Move scripts to `scripts/` (6 files)
8. Keep only 9 configuration files in root

### ❌ **DON'T: Risky Operations**
1. Don't remove `.venv/` (currently active)
2. Don't move configuration files from root
3. Don't remove recent documentation
4. Don't reorganize without checking git history
5. Don't overwrite active development
6. **Don't move files before fixing critical paths**

## 📈 **BENEFITS OF ORGANIZATION**

- **✅ Clean Root Directory**: Only 9 essential files remain
- **✅ Professional Structure**: Files organized by purpose
- **✅ Easy Navigation**: Clear directory structure
- **✅ Maintainable**: Logical file organization
- **✅ Scalable**: Easy to add new files to appropriate directories
- **✅ Standard Practice**: Follows Python project conventions

## 🔍 **VALIDATION CHECKLIST**

Before any cleanup action:
- [ ] Verify `.venv/` is active environment
- [ ] Check git history for file activity
- [ ] Confirm file is not in recent commits
- [ ] Test functionality after cleanup
- [ ] Ensure no active development is lost
- [ ] Verify all files moved to appropriate directories
- [ ] **Run complete test suite after each phase**
- [ ] **Test all CLI commands after moving schema files**
- [ ] **Test telemetry functionality after moving schema files**

## 📊 **CLEANUP IMPACT SUMMARY**

### Files to Remove (8 files):
- `.lh/` (7MB+ cache files)
- `__pycache__/` directories (multiple locations)
- `autotel_prototype.egg-info/` (generated)
- `autotel_process_instances.sqlite3` (auto-created)
- `debug_run*.log` (3 debug log files)
- `compiled_ontology_mock.json` (generated)
- `parsed_ontology.json` (generated)
- `telemetry_export.json` (generated)
- `autotel_cli.py` (legacy file)

### Files to Move (76 files):
- **31 documentation files** → `docs/`
- **34 test files** → `tests/`
- **5 schema files** → `schemas/`
- **6 script files** → `scripts/`

### Files to Keep in Root (9 files):
- `.venv/` (active environment)
- All `autotel/` module files (active development)
- **9 configuration files** (must stay in root)

---

**Status**: Updated with comprehensive breakage analysis and checklist
**Priority**: Safe cleanup + proper file organization with critical path fixes
**Risk Level**: Medium - requires careful path updates before moving files
**Active Environment**: `.venv/` (uv 0.7.20, Python 3.13.0)
**Total Files**: 93 (76 to move, 8 to remove, 9 to keep in root)
**Critical Fixes Required**: 4 path updates in core files 