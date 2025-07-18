#!/usr/bin/env python3
# Generate smaller test data
import sys
from generate_sprint_data import generate_sprint_health_data

num_patients = int(sys.argv[1]) if len(sys.argv) > 1 else 100
triples = generate_sprint_health_data(num_patients, 10)

with open("/tmp/small_test_data.ttl", "w") as f:
    f.write("@prefix : <http://example.org/sprint_health#> .\n")
    f.write("@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n\n")
    for triple in triples:
        f.write(triple + "\n")

print(f"Generated {len(triples)} triples for {num_patients} patients")
