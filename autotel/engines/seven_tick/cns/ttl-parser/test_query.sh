#!/bin/bash

# TTL Parser Query Engine Test Script

echo "=== TTL Parser Query Engine Test ==="
echo

# Build the query-enabled parser
echo "Building query-enabled parser..."
make clean
make ttl-parser-query

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo

# Test queries
echo "=== Query Test 1: Find all persons ==="
echo "Query: ?s rdf:type foaf:Person"
./bin/ttl-parser-query --query "?s a foaf:Person" examples/sample_query.ttl
echo

echo "=== Query Test 2: Find all names ==="
echo "Query: ?s foaf:name ?name"
./bin/ttl-parser-query --query "?s foaf:name ?name" examples/sample_query.ttl
echo

echo "=== Query Test 3: Find specific person properties ==="
echo "Query: ex:person1 ?p ?o"
./bin/ttl-parser-query --query "ex:person1 ?p ?o" examples/sample_query.ttl
echo

echo "=== Query Test 4: JSON output ==="
echo "Query: ?s a ?type (JSON format)"
./bin/ttl-parser-query --query "?s a ?type" --json examples/sample_query.ttl
echo

echo "=== Query Test 5: With Statistics ==="
echo "Query with statistics enabled"
./bin/ttl-parser-query --query "?s ?p ?o" --stats examples/sample_query.ttl
echo

echo "=== All tests completed ==="