#!/bin/bash

# Rule: NEVER use the word "semantic" - replace with ontology-related terms

# Define replacement mappings
declare -A replacements=(
    ["Semantic Web"]="Ontology Web"
    ["semantic web"]="ontology web"
    ["Semantic"]="Ontology"
    ["semantic"]="ontology"
    ["SEMANTIC"]="ONTOLOGY"
)

# Files to process
files=(
    "demo.py"
    "ontology7t.py"
    "IMPLEMENTATION_SUMMARY.md"
    "IMPLEMENTATION_STATUS.md"
    "README.md"
    "__init__.py"
    "PERFORMANCE_REPORT.md"
    "c_src/benchmark_semantic.c"
    "c_src/test_semantic.c"
    "c_src/Makefile"
)

# Process each file
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "Processing $file..."
        # Create backup
        cp "$file" "$file.bak"
        
        # Apply replacements
        for search in "${!replacements[@]}"; do
            replace="${replacements[$search]}"
            sed -i '' "s/$search/$replace/g" "$file"
        done
    fi
done

# Rename files
mv c_src/benchmark_semantic.c c_src/benchmark_ontology.c
mv c_src/test_semantic.c c_src/test_ontology.c

echo "Replacement complete!"