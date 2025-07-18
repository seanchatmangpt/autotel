# IMPORTANT RULE: Never Use "Semantic"

## Rule
**NEVER use the word "semantic" in any form**. This applies to:
- Source code
- Documentation  
- Comments
- Variable names
- File names
- Benchmarks

## Replacements

Use these alternatives instead:

| ❌ Don't Use | ✅ Use Instead |
|-------------|----------------|
| Semantic Web | Ontology Web, Knowledge Graph |
| semantic | ontology |
| Semantic | Ontology |
| SEMANTIC | ONTOLOGY |
| semantic processing | ontology processing |
| semantic engine | ontology engine |
| semantic query | ontology query |
| semantic stack | ontology stack |
| semantic technology | ontology technology |

## Examples

❌ **Wrong**: "Seven Tick is a semantic web engine"
✅ **Correct**: "Seven Tick is an ontology engine"

❌ **Wrong**: "Integrated semantic query benchmark"  
✅ **Correct**: "Integrated ontology query benchmark"

❌ **Wrong**: `benchmark_semantic.c`
✅ **Correct**: `benchmark_ontology.c`

## Enforcement

All code and documentation has been updated to follow this rule:
- Renamed `benchmark_semantic.c` → `benchmark_ontology.c`
- Renamed `test_semantic.c` → `test_ontology.c`
- Updated all references in code and documentation
- File `SEMANTIC_WEB.md` → `ONTOLOGY_ENGINE.md`

This rule is permanent and must be followed in all future development.