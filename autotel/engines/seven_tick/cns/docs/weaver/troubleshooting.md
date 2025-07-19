# CNS Weaver Troubleshooting

This guide helps you diagnose and resolve common issues with the CNS Weaver system.

## Common Issues and Solutions

### 1. Missing Dependencies

**Problem**: `ModuleNotFoundError: No module named 'rdflib'`

**Solution**:
```bash
pip install rdflib
```

**Problem**: `ImportError: No module named 'jansson'`

**Solution**:
```bash
# On macOS
brew install jansson

# On Ubuntu/Debian
sudo apt-get install libjansson-dev

# On CentOS/RHEL
sudo yum install jansson-devel
```

### 2. TTL Parsing Errors

**Problem**: `Error parsing TTL file: [Errno 2] No such file or directory`

**Solution**:
```bash
# Check if TTL file exists
ls -la docs/ontology/cns-core.ttl

# Verify file path is correct
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
```

**Problem**: `rdflib.plugins.parsers.notation3.BadSyntax: ...`

**Solution**:
- Check TTL syntax for missing semicolons or periods
- Verify namespace declarations
- Ensure proper RDF structure

Example of correct TTL syntax:
```ttl
@prefix cns: <https://schema.chatman.ai/cns#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

cns:testFunction a cns:Function ;
    cns:providesAPI "int cns_test_function(void)" ;
    cns:cycleCost 2 ;
    rdfs:label "Test Function" .
```

### 3. Template Generation Errors

**Problem**: `Error rendering template: ...`

**Solution**:
- Check Jinja template syntax
- Verify template variables are properly defined
- Ensure template files exist

**Debug template issues**:
```bash
# Create debug template
cat > debug.j2 << 'EOF'
// Debug template
Spans: {{ spans|length }}
Functions: {{ functions|length }}
Patterns: {{ patterns|length }}

{% for span in spans %}
Span: {{ span.spanName }} -> {{ span.functionName }}
{% endfor %}
EOF

# Test with debug template
python3 codegen/weaver_simple.py build/cns_spans.json debug.j2 templates/otel_inject.c.j2 src cns.h
```

### 4. Permission Errors

**Problem**: `Permission denied` when writing output files

**Solution**:
```bash
# Check directory permissions
ls -la src/
ls -la build/

# Fix permissions if needed
chmod 755 src/
chmod 755 build/

# Or create directories with proper permissions
mkdir -p src build
chmod 755 src build
```

### 5. JSON Structure Issues

**Problem**: Generated JSON has incorrect structure

**Solution**:
```bash
# Validate JSON structure
python3 -c "
import json
with open('build/cns_spans.json', 'r') as f:
    data = json.load(f)
    print('Keys:', list(data.keys()))
    print('Spans:', len(data.get('spans', [])))
    print('Functions:', len(data.get('functions', [])))
    print('Patterns:', len(data.get('patterns', [])))
"
```

### 6. Generated Code Issues

**Problem**: Generated C code has syntax errors

**Solution**:
```bash
# Check generated header
gcc -c -I. src/cns_otel.h -o /dev/null

# Check generated inject file
gcc -c -I. src/cns_otel_inject.c -o /dev/null

# If errors, check template syntax
head -20 src/cns_otel.h
head -20 src/cns_otel_inject.c
```

### 7. Performance Issues

**Problem**: Weaver runs slowly with large TTL files

**Solution**:
```bash
# Profile the extraction process
python3 -m cProfile -o profile.stats codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json

# Analyze profile
python3 -c "
import pstats
p = pstats.Stats('profile.stats')
p.sort_stats('cumulative').print_stats(10)
"
```

## Validation and Diagnostics

### Run Full Validation

```bash
# Run comprehensive validation
python3 docs/validate_weaver.py
```

### Manual Validation Steps

```bash
# Step 1: Check prerequisites
python3 -c "import rdflib; print('rdflib version:', rdflib.__version__)"
python3 -c "import json; print('json available')"

# Step 2: Test TTL parsing
python3 -c "
import rdflib
g = rdflib.Graph()
g.parse('docs/ontology/cns-core.ttl', format='ttl')
print('TTL parsed successfully')
print('Triples:', len(g))
"

# Step 3: Test span extraction
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/test_spans.json

# Step 4: Test code generation
python3 codegen/weaver_simple.py build/test_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h

# Step 5: Check generated files
ls -la src/cns_otel.*
wc -l src/cns_otel.h src/cns_otel_inject.c
```

### Debug Mode

Enable debug output by modifying the scripts:

```python
# Add to extract_spans.py
import logging
logging.basicConfig(level=logging.DEBUG)

# Add to weaver_simple.py
import logging
logging.basicConfig(level=logging.DEBUG)
```

## Environment Issues

### Python Version Issues

**Problem**: Script fails with Python 2.x

**Solution**:
```bash
# Ensure Python 3 is used
python3 --version

# Or set alias
alias python=python3
```

### Path Issues

**Problem**: Scripts can't find files

**Solution**:
```bash
# Check current directory
pwd

# Use absolute paths
python3 /full/path/to/cns/codegen/extract_spans.py /full/path/to/cns/docs/ontology/cns-core.ttl /full/path/to/cns/build/cns_spans.json

# Or set PYTHONPATH
export PYTHONPATH=/full/path/to/cns:$PYTHONPATH
```

### Build System Integration Issues

**Problem**: CMake integration fails

**Solution**:
```cmake
# Add debug output to CMake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    COMMAND ${CMAKE_COMMAND} -E echo "Extracting spans..."
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
    COMMENT "Extracting spans from TTL ontology"
    VERBATIM
)
```

## Performance Optimization

### Large TTL Files

For large ontology files:

```bash
# Use streaming processing
python3 -c "
import rdflib
from rdflib import Graph, Namespace, RDF, RDFS

# Process in chunks
g = Graph()
g.parse('large-ontology.ttl', format='ttl')

# Extract only what's needed
CNS = Namespace('https://schema.chatman.ai/cns#')
spans = list(g.subjects(RDF.type, CNS.TelemetrySpan))
print(f'Found {len(spans)} spans')
"
```

### Memory Issues

If the weaver uses too much memory:

```bash
# Monitor memory usage
python3 -m memory_profiler codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json

# Or use system monitoring
/usr/bin/time -v python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
```

## Getting Help

### Self-Diagnosis

1. **Run validation script**: `python3 docs/validate_weaver.py`
2. **Check logs**: Look for error messages in output
3. **Verify files**: Ensure all required files exist
4. **Test components**: Test each component individually

### Common Debug Commands

```bash
# Check file existence
find . -name "*.ttl" -o -name "*.py" -o -name "*.j2"

# Check file permissions
ls -la codegen/ templates/ docs/ontology/

# Test Python imports
python3 -c "import sys; print('Python path:', sys.path)"
python3 -c "import rdflib; print('rdflib available')"

# Validate JSON
python3 -c "import json; json.load(open('build/cns_spans.json')); print('JSON valid')"

# Check generated C code
gcc -fsyntax-only -I. src/cns_otel.h
gcc -fsyntax-only -I. src/cns_otel_inject.c
```

### Reporting Issues

When reporting issues, include:

1. **Environment**: OS, Python version, rdflib version
2. **Command**: Exact command that failed
3. **Error message**: Complete error output
4. **Files**: Relevant file contents (TTL, templates)
5. **Validation output**: Results from `validate_weaver.py`

Example issue report:
```
Environment: macOS 12.0, Python 3.9.7, rdflib 6.0.0
Command: python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
Error: [Include complete error message]
Files: [Include relevant TTL and template files]
Validation: [Include validation script output]
``` 