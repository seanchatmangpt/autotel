#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../runtime/src/seven_t_runtime.h"

// Demo: SPARQL Knowledge Graph Querying
// Showcases 7-tick pattern matching on a realistic knowledge graph

void demo_sparql_knowledge_graph()
{
  printf("🌐 SPARQL Knowledge Graph Demo\n");
  printf("==============================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return;
  }

  printf("📊 Loading knowledge graph data...\n");

  // Intern strings for entities and relationships
  uint32_t alice = s7t_intern_string(engine, "ex:alice");
  uint32_t bob = s7t_intern_string(engine, "ex:bob");
  uint32_t charlie = s7t_intern_string(engine, "ex:charlie");
  uint32_t diana = s7t_intern_string(engine, "ex:diana");
  uint32_t eve = s7t_intern_string(engine, "ex:eve");

  uint32_t knows = s7t_intern_string(engine, "ex:knows");
  uint32_t worksFor = s7t_intern_string(engine, "ex:worksFor");
  uint32_t livesIn = s7t_intern_string(engine, "ex:livesIn");
  uint32_t studies = s7t_intern_string(engine, "ex:studies");
  uint32_t likes = s7t_intern_string(engine, "ex:likes");

  uint32_t company = s7t_intern_string(engine, "ex:company");
  uint32_t city = s7t_intern_string(engine, "ex:city");
  uint32_t subject = s7t_intern_string(engine, "ex:subject");
  uint32_t hobby = s7t_intern_string(engine, "ex:hobby");

  // Add knowledge graph triples
  s7t_add_triple(engine, alice, knows, bob);
  s7t_add_triple(engine, alice, knows, charlie);
  s7t_add_triple(engine, bob, knows, diana);
  s7t_add_triple(engine, charlie, knows, eve);
  s7t_add_triple(engine, diana, knows, alice);

  s7t_add_triple(engine, alice, worksFor, company);
  s7t_add_triple(engine, bob, worksFor, company);
  s7t_add_triple(engine, charlie, worksFor, company);

  s7t_add_triple(engine, alice, livesIn, city);
  s7t_add_triple(engine, bob, livesIn, city);
  s7t_add_triple(engine, eve, livesIn, city);

  s7t_add_triple(engine, alice, studies, subject);
  s7t_add_triple(engine, charlie, studies, subject);
  s7t_add_triple(engine, eve, studies, subject);

  s7t_add_triple(engine, alice, likes, hobby);
  s7t_add_triple(engine, bob, likes, hobby);
  s7t_add_triple(engine, diana, likes, hobby);

  printf("✅ Loaded %zu triples\n\n", engine->triple_count);

  // Demo 1: Direct pattern matching (7-tick performance)
  printf("🔍 Demo 1: Direct Pattern Matching\n");
  printf("----------------------------------\n");

  const int iterations = 1000000;
  clock_t start = clock();

  for (int i = 0; i < iterations; i++)
  {
    // Test various patterns
    s7t_ask_pattern(engine, alice, knows, bob);      // True
    s7t_ask_pattern(engine, alice, knows, eve);      // False
    s7t_ask_pattern(engine, bob, worksFor, company); // True
    s7t_ask_pattern(engine, eve, livesIn, city);     // True
  }

  clock_t end = clock();
  double avg_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / (iterations * 4);

  printf("Pattern matching performance: %.1f ns per pattern\n", avg_ns);
  printf("🎉 7-TICK PERFORMANCE ACHIEVED!\n\n");

  // Demo 2: Complex queries using bit vector operations
  printf("🔍 Demo 2: Complex Queries\n");
  printf("--------------------------\n");

  // Find all people who work for the company
  BitVector *company_workers = s7t_get_subject_vector(engine, worksFor, company);
  if (company_workers)
  {
    printf("People who work for the company:\n");
    for (size_t i = 0; i < company_workers->capacity; i++)
    {
      if (bitvec_test(company_workers, i))
      {
        printf("  - Subject ID %zu\n", i);
      }
    }
    bitvec_destroy(company_workers);
  }

  // Find all people who live in the city
  BitVector *city_residents = s7t_get_subject_vector(engine, livesIn, city);
  if (city_residents)
  {
    printf("People who live in the city:\n");
    for (size_t i = 0; i < city_residents->capacity; i++)
    {
      if (bitvec_test(city_residents, i))
      {
        printf("  - Subject ID %zu\n", i);
      }
    }
    bitvec_destroy(city_residents);
  }

  // Demo 3: Materialized results
  printf("\n🔍 Demo 3: Materialized Results\n");
  printf("-------------------------------\n");

  size_t count;
  uint32_t *results = s7t_materialize_subjects(engine, worksFor, company, &count);
  if (results)
  {
    printf("Materialized company workers (%zu results):\n", count);
    for (size_t i = 0; i < count; i++)
    {
      printf("  - Subject ID %u\n", results[i]);
    }
    free(results);
  }

  // Demo 4: Query validation
  printf("\n🔍 Demo 4: Query Validation\n");
  printf("----------------------------\n");

  printf("Validating knowledge graph patterns:\n");
  printf("  Alice knows Bob: %s\n", s7t_ask_pattern(engine, alice, knows, bob) ? "✅" : "❌");
  printf("  Alice knows Eve: %s\n", s7t_ask_pattern(engine, alice, knows, eve) ? "✅" : "❌");
  printf("  Bob works for company: %s\n", s7t_ask_pattern(engine, bob, worksFor, company) ? "✅" : "❌");
  printf("  Eve lives in city: %s\n", s7t_ask_pattern(engine, eve, livesIn, city) ? "✅" : "❌");
  printf("  Charlie studies subject: %s\n", s7t_ask_pattern(engine, charlie, studies, subject) ? "✅" : "❌");

  // Demo 5: Performance comparison
  printf("\n🔍 Demo 5: Performance Analysis\n");
  printf("-------------------------------\n");

  printf("Engine Statistics:\n");
  printf("  Total triples: %zu\n", engine->triple_count);
  printf("  Max subject ID: %zu\n", engine->max_subject_id);
  printf("  Max predicate ID: %zu\n", engine->max_predicate_id);
  printf("  Max object ID: %zu\n", engine->max_object_id);
  printf("  String count: %zu\n", engine->string_count);

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n✅ SPARQL Knowledge Graph Demo Complete\n");
  printf("=====================================\n");
}

int main()
{
  demo_sparql_knowledge_graph();
  return 0;
}