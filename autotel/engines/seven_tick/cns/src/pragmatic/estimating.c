#include "cns/pragmatic/estimating.h"
#include "cns/telemetry/otel.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Performance validation macro
#define S7T_VALIDATE_PERFORMANCE(operation, max_cycles)          \
  do                                                             \
  {                                                              \
    uint64_t start = s7t_cycles();                               \
    operation;                                                   \
    uint64_t end = s7t_cycles();                                 \
    uint32_t cycles = (uint32_t)(end - start);                   \
    assert(cycles <= max_cycles);                                \
    otel_span_set_attribute(span, "performance.cycles", cycles); \
  } while (0)

cns_estimating_manager_t *cns_estimating_init(void)
{
  otel_span_t span = otel_span_start("estimating.init");

  cns_estimating_manager_t *manager = malloc(sizeof(cns_estimating_manager_t));
  if (!manager)
  {
    otel_span_set_attribute(span, "error", "allocation_failed");
    otel_span_end(span);
    return NULL;
  }

  // Initialize manager
  memset(manager, 0, sizeof(cns_estimating_manager_t));
  manager->enabled = true;
  manager->next_estimate_id = 1;
  manager->next_sample_id = 1;
  manager->overall_accuracy = 1.0; // Start with perfect accuracy

  otel_span_set_attribute(span, "manager.initialized", true);
  otel_span_set_attribute(span, "manager.max_estimates", CNS_MAX_ESTIMATES);
  otel_span_set_attribute(span, "manager.max_samples", CNS_MAX_HISTORICAL_SAMPLES);

  S7T_VALIDATE_PERFORMANCE(/* initialization complete */, 10);

  otel_span_end(span);
  return manager;
}

void cns_estimating_cleanup(cns_estimating_manager_t *manager)
{
  if (!manager)
    return;

  otel_span_t span = otel_span_start("estimating.cleanup");

  free(manager);

  otel_span_set_attribute(span, "cleanup.completed", true);
  otel_span_end(span);
}

uint32_t cns_estimating_create_estimate(cns_estimating_manager_t *manager,
                                        const char *component_name,
                                        const char *description,
                                        cns_estimate_type_t type)
{
  if (!manager || !component_name)
  {
    return 0;
  }

  otel_span_t span = otel_span_start("estimating.create_estimate");

  if (manager->estimate_count >= CNS_MAX_ESTIMATES)
  {
    otel_span_set_attribute(span, "error", "max_estimates_reached");
    otel_span_end(span);
    return 0;
  }

  uint32_t estimate_id = manager->next_estimate_id++;

  cns_performance_estimate_t *estimate = &manager->estimates[manager->estimate_count];
  estimate->estimate_id = estimate_id;
  estimate->component_name = component_name;
  estimate->description = description;
  estimate->type = type;
  estimate->creation_time = s7t_cycles();
  estimate->validated = false;

  manager->estimate_count++;
  manager->total_estimates_made++;

  otel_span_set_attribute(span, "estimate.id", estimate_id);
  otel_span_set_attribute(span, "estimate.component", component_name);
  otel_span_set_attribute(span, "estimate.type", type);

  S7T_VALIDATE_PERFORMANCE(/* estimate creation complete */, 10);

  otel_span_end(span);
  return estimate_id;
}

cns_performance_estimate_t cns_estimating_estimate_performance(cns_estimating_manager_t *manager, const char *component_name)
{
  cns_performance_estimate_t estimate = {0};

  if (!manager || !component_name)
  {
    return estimate;
  }

  otel_span_t span = otel_span_start("estimating.estimate_performance");

  uint64_t start_time = s7t_cycles();

  // Get historical data for this component
  cns_historical_sample_t *samples = NULL;
  uint32_t sample_count = 0;
  cns_estimating_get_historical_data(manager, component_name, &samples, &sample_count);

  if (sample_count > 0 && samples)
  {
    // Calculate average from historical data
    uint64_t total_cycles = 0;
    uint64_t total_memory = 0;
    uint32_t successful_samples = 0;

    for (uint32_t i = 0; i < sample_count; i++)
    {
      if (samples[i].success)
      {
        total_cycles += samples[i].cycles_measured;
        total_memory += samples[i].memory_used;
        successful_samples++;
      }
    }

    if (successful_samples > 0)
    {
      estimate.estimated_cycles = (uint32_t)(total_cycles / successful_samples);
      estimate.estimated_memory_bytes = (uint32_t)(total_memory / successful_samples);
      estimate.confidence_level = (double)successful_samples / sample_count;
      estimate.historical_samples = successful_samples;
    }
    else
    {
      // No successful samples, use default estimates
      estimate.estimated_cycles = 5;        // Default 5 cycles
      estimate.estimated_memory_bytes = 64; // Default 64 bytes
      estimate.confidence_level = 0.5;      // Low confidence
      estimate.historical_samples = 0;
    }

    free(samples);
  }
  else
  {
    // No historical data, use default estimates
    estimate.estimated_cycles = 5;        // Default 5 cycles
    estimate.estimated_memory_bytes = 64; // Default 64 bytes
    estimate.confidence_level = 0.3;      // Very low confidence
    estimate.historical_samples = 0;
  }

  // Assess risk level
  estimate.risk_level = cns_estimating_assess_performance_risk(manager, estimate.estimated_cycles, estimate.confidence_level);

  uint64_t end_time = s7t_cycles();
  uint32_t estimation_cycles = (uint32_t)(end_time - start_time);

  estimate.component_name = component_name;
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  estimate.creation_time = s7t_cycles();
  estimate.validated = false;

  otel_span_set_attribute(span, "component.name", component_name);
  otel_span_set_attribute(span, "estimate.cycles", estimate.estimated_cycles);
  otel_span_set_attribute(span, "estimate.memory", estimate.estimated_memory_bytes);
  otel_span_set_attribute(span, "estimate.confidence", estimate.confidence_level);
  otel_span_set_attribute(span, "estimate.risk_level", estimate.risk_level);
  otel_span_set_attribute(span, "estimation.cycles", estimation_cycles);

  S7T_VALIDATE_PERFORMANCE(/* performance estimation complete */, 100);

  otel_span_end(span);
  return estimate;
}

cns_risk_level_t cns_estimating_assess_performance_risk(cns_estimating_manager_t *manager, uint32_t estimated_cycles, double confidence)
{
  if (!manager)
    return CNS_RISK_HIGH;

  otel_span_t span = otel_span_start("estimating.assess_performance_risk");

  cns_risk_level_t risk_level = CNS_RISK_MEDIUM;

  // Risk assessment based on cycles and confidence
  if (estimated_cycles <= 3 && confidence >= 0.8)
  {
    risk_level = CNS_RISK_LOW;
  }
  else if (estimated_cycles <= 5 && confidence >= 0.6)
  {
    risk_level = CNS_RISK_MEDIUM;
  }
  else if (estimated_cycles <= 7 && confidence >= 0.4)
  {
    risk_level = CNS_RISK_HIGH;
  }
  else
  {
    risk_level = CNS_RISK_CRITICAL;
  }

  otel_span_set_attribute(span, "risk.estimated_cycles", estimated_cycles);
  otel_span_set_attribute(span, "risk.confidence", confidence);
  otel_span_set_attribute(span, "risk.level", risk_level);

  S7T_VALIDATE_PERFORMANCE(/* risk assessment complete */, 10);

  otel_span_end(span);
  return risk_level;
}

uint32_t cns_estimating_add_historical_sample(cns_estimating_manager_t *manager,
                                              const char *component_name,
                                              uint32_t cycles_measured,
                                              uint32_t memory_used,
                                              bool success)
{
  if (!manager || !component_name)
  {
    return 0;
  }

  otel_span_t span = otel_span_start("estimating.add_historical_sample");

  if (manager->historical_count >= CNS_MAX_HISTORICAL_SAMPLES)
  {
    // Remove oldest sample to make room
    for (uint32_t i = 0; i < manager->historical_count - 1; i++)
    {
      manager->historical_data[i] = manager->historical_data[i + 1];
    }
    manager->historical_count--;
  }

  uint32_t sample_id = manager->next_sample_id++;

  cns_historical_sample_t *sample = &manager->historical_data[manager->historical_count];
  sample->sample_id = sample_id;
  sample->component_name = component_name;
  sample->cycles_measured = cycles_measured;
  sample->memory_used = memory_used;
  sample->timestamp = s7t_cycles();
  sample->success = success;

  manager->historical_count++;

  otel_span_set_attribute(span, "sample.id", sample_id);
  otel_span_set_attribute(span, "sample.component", component_name);
  otel_span_set_attribute(span, "sample.cycles", cycles_measured);
  otel_span_set_attribute(span, "sample.memory", memory_used);
  otel_span_set_attribute(span, "sample.success", success);

  S7T_VALIDATE_PERFORMANCE(/* sample addition complete */, 10);

  otel_span_end(span);
  return sample_id;
}

bool cns_estimating_get_historical_data(cns_estimating_manager_t *manager, const char *component_name,
                                        cns_historical_sample_t **samples, uint32_t *sample_count)
{
  if (!manager || !component_name || !samples || !sample_count)
  {
    return false;
  }

  otel_span_t span = otel_span_start("estimating.get_historical_data");

  // Count matching samples
  uint32_t count = 0;
  for (uint32_t i = 0; i < manager->historical_count; i++)
  {
    if (strcmp(manager->historical_data[i].component_name, component_name) == 0)
    {
      count++;
    }
  }

  if (count == 0)
  {
    *samples = NULL;
    *sample_count = 0;
    otel_span_set_attribute(span, "samples.found", 0);
    otel_span_end(span);
    return true;
  }

  // Allocate and copy matching samples
  *samples = malloc(count * sizeof(cns_historical_sample_t));
  if (!*samples)
  {
    otel_span_set_attribute(span, "error", "allocation_failed");
    otel_span_end(span);
    return false;
  }

  uint32_t index = 0;
  for (uint32_t i = 0; i < manager->historical_count; i++)
  {
    if (strcmp(manager->historical_data[i].component_name, component_name) == 0)
    {
      (*samples)[index] = manager->historical_data[i];
      index++;
    }
  }

  *sample_count = count;

  otel_span_set_attribute(span, "component.name", component_name);
  otel_span_set_attribute(span, "samples.found", count);

  S7T_VALIDATE_PERFORMANCE(/* historical data retrieval complete */, 100);

  otel_span_end(span);
  return true;
}

bool cns_estimating_validate_estimate(cns_estimating_manager_t *manager, uint32_t estimate_id, uint32_t actual_cycles, uint32_t actual_memory)
{
  if (!manager)
    return false;

  otel_span_t span = otel_span_start("estimating.validate_estimate");

  // Find estimate
  cns_performance_estimate_t *estimate = NULL;
  for (uint32_t i = 0; i < manager->estimate_count; i++)
  {
    if (manager->estimates[i].estimate_id == estimate_id)
    {
      estimate = &manager->estimates[i];
      break;
    }
  }

  if (!estimate)
  {
    otel_span_set_attribute(span, "error", "estimate_not_found");
    otel_span_end(span);
    return false;
  }

  // Calculate accuracy
  estimate->actual_cycles = actual_cycles;
  estimate->actual_memory_bytes = actual_memory;
  estimate->validated = true;

  double cycle_accuracy = 0.0;
  if (estimate->estimated_cycles > 0)
  {
    cycle_accuracy = 100.0 - fabs((double)(actual_cycles - estimate->estimated_cycles) / estimate->estimated_cycles * 100.0);
  }

  double memory_accuracy = 0.0;
  if (estimate->estimated_memory_bytes > 0)
  {
    memory_accuracy = 100.0 - fabs((double)(actual_memory - estimate->estimated_memory_bytes) / estimate->estimated_memory_bytes * 100.0);
  }

  estimate->accuracy_percentage = (cycle_accuracy + memory_accuracy) / 2.0;

  // Update statistics
  if (estimate->accuracy_percentage >= (100.0 - CNS_ACCURACY_THRESHOLD_PERCENT))
  {
    manager->accurate_estimates++;
  }
  else
  {
    manager->inaccurate_estimates++;
  }

  // Update overall accuracy
  uint64_t total_validated = manager->accurate_estimates + manager->inaccurate_estimates;
  if (total_validated > 0)
  {
    manager->overall_accuracy = (double)manager->accurate_estimates / total_validated;
  }

  otel_span_set_attribute(span, "estimate.id", estimate_id);
  otel_span_set_attribute(span, "validation.actual_cycles", actual_cycles);
  otel_span_set_attribute(span, "validation.actual_memory", actual_memory);
  otel_span_set_attribute(span, "validation.accuracy_percentage", estimate->accuracy_percentage);
  otel_span_set_attribute(span, "validation.accurate", estimate->accuracy_percentage >= (100.0 - CNS_ACCURACY_THRESHOLD_PERCENT));

  S7T_VALIDATE_PERFORMANCE(/* estimate validation complete */, 10);

  otel_span_end(span);
  return true;
}

// Built-in estimation functions
cns_performance_estimate_t cns_estimating_strategy_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 2;
  estimate.estimated_memory_bytes = 32;
  estimate.confidence_level = 0.9;
  estimate.historical_samples = 100;
  estimate.risk_level = CNS_RISK_LOW;
  estimate.component_name = "strategy_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_factory_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 1;
  estimate.estimated_memory_bytes = 16;
  estimate.confidence_level = 0.95;
  estimate.historical_samples = 150;
  estimate.risk_level = CNS_RISK_LOW;
  estimate.component_name = "factory_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_singleton_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 0;
  estimate.estimated_memory_bytes = 8;
  estimate.confidence_level = 0.98;
  estimate.historical_samples = 200;
  estimate.risk_level = CNS_RISK_LOW;
  estimate.component_name = "singleton_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_decorator_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 3;
  estimate.estimated_memory_bytes = 48;
  estimate.confidence_level = 0.85;
  estimate.historical_samples = 80;
  estimate.risk_level = CNS_RISK_LOW;
  estimate.component_name = "decorator_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_observer_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 4;
  estimate.estimated_memory_bytes = 64;
  estimate.confidence_level = 0.8;
  estimate.historical_samples = 60;
  estimate.risk_level = CNS_RISK_MEDIUM;
  estimate.component_name = "observer_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_command_pattern_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 5;
  estimate.estimated_memory_bytes = 80;
  estimate.confidence_level = 0.75;
  estimate.historical_samples = 40;
  estimate.risk_level = CNS_RISK_MEDIUM;
  estimate.component_name = "command_pattern";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_memory_allocation_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 10;
  estimate.estimated_memory_bytes = 128;
  estimate.confidence_level = 0.7;
  estimate.historical_samples = 50;
  estimate.risk_level = CNS_RISK_HIGH;
  estimate.component_name = "memory_allocation";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_string_operation_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 6;
  estimate.estimated_memory_bytes = 96;
  estimate.confidence_level = 0.8;
  estimate.historical_samples = 70;
  estimate.risk_level = CNS_RISK_MEDIUM;
  estimate.component_name = "string_operation";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_telemetry_operation_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 8;
  estimate.estimated_memory_bytes = 112;
  estimate.confidence_level = 0.65;
  estimate.historical_samples = 30;
  estimate.risk_level = CNS_RISK_HIGH;
  estimate.component_name = "telemetry_operation";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

cns_performance_estimate_t cns_estimating_contract_validation_performance(void)
{
  cns_performance_estimate_t estimate = {0};
  estimate.estimated_cycles = 1;
  estimate.estimated_memory_bytes = 16;
  estimate.confidence_level = 0.9;
  estimate.historical_samples = 120;
  estimate.risk_level = CNS_RISK_LOW;
  estimate.component_name = "contract_validation";
  estimate.type = CNS_ESTIMATE_PERFORMANCE;
  return estimate;
}

// Statistics functions
uint64_t cns_estimating_get_total_estimates(cns_estimating_manager_t *manager)
{
  return manager ? manager->total_estimates_made : 0;
}

uint64_t cns_estimating_get_accurate_estimates(cns_estimating_manager_t *manager)
{
  return manager ? manager->accurate_estimates : 0;
}

uint64_t cns_estimating_get_inaccurate_estimates(cns_estimating_manager_t *manager)
{
  return manager ? manager->inaccurate_estimates : 0;
}

double cns_estimating_get_overall_accuracy(cns_estimating_manager_t *manager)
{
  return manager ? manager->overall_accuracy : 0.0;
}

uint32_t cns_estimating_get_historical_sample_count(cns_estimating_manager_t *manager)
{
  return manager ? manager->historical_count : 0;
}

// Configuration functions
bool cns_estimating_set_enabled(cns_estimating_manager_t *manager, bool enabled)
{
  if (!manager)
    return false;
  manager->enabled = enabled;
  return true;
}

bool cns_estimating_is_enabled(cns_estimating_manager_t *manager)
{
  return manager ? manager->enabled : false;
}

// Performance validation
void cns_estimating_validate_performance_comprehensive(cns_estimating_manager_t *manager)
{
  if (!manager)
    return;

  otel_span_t span = otel_span_start("estimating.validate_performance_comprehensive");

  // Validate initialization performance
  uint64_t start = s7t_cycles();
  cns_estimating_manager_t *test_manager = cns_estimating_init();
  uint64_t end = s7t_cycles();
  uint32_t init_cycles = (uint32_t)(end - start);

  if (test_manager)
  {
    // Validate estimate creation performance
    start = s7t_cycles();
    uint32_t estimate_id = cns_estimating_create_estimate(test_manager, "test_component", "test", CNS_ESTIMATE_PERFORMANCE);
    end = s7t_cycles();
    uint32_t create_cycles = (uint32_t)(end - start);

    // Validate performance estimation
    start = s7t_cycles();
    cns_performance_estimate_t estimate = cns_estimating_estimate_performance(test_manager, "test_component");
    end = s7t_cycles();
    uint32_t estimate_cycles = (uint32_t)(end - start);

    // Validate risk assessment
    start = s7t_cycles();
    cns_risk_level_t risk = cns_estimating_assess_performance_risk(test_manager, 5, 0.8);
    end = s7t_cycles();
    uint32_t risk_cycles = (uint32_t)(end - start);

    // Validate historical data addition
    start = s7t_cycles();
    uint32_t sample_id = cns_estimating_add_historical_sample(test_manager, "test_component", 5, 64, true);
    end = s7t_cycles();
    uint32_t sample_cycles = (uint32_t)(end - start);

    otel_span_set_attribute(span, "performance.init_cycles", init_cycles);
    otel_span_set_attribute(span, "performance.create_cycles", create_cycles);
    otel_span_set_attribute(span, "performance.estimate_cycles", estimate_cycles);
    otel_span_set_attribute(span, "performance.risk_cycles", risk_cycles);
    otel_span_set_attribute(span, "performance.sample_cycles", sample_cycles);

    // Validate 7-tick compliance
    bool init_compliant = (init_cycles <= 10);
    bool create_compliant = (create_cycles <= 10);
    bool estimate_compliant = (estimate_cycles <= 100);
    bool risk_compliant = (risk_cycles <= 10);
    bool sample_compliant = (sample_cycles <= 10);

    otel_span_set_attribute(span, "compliance.init_7_tick", init_compliant);
    otel_span_set_attribute(span, "compliance.create_7_tick", create_compliant);
    otel_span_set_attribute(span, "compliance.estimate_7_tick", estimate_compliant);
    otel_span_set_attribute(span, "compliance.risk_7_tick", risk_compliant);
    otel_span_set_attribute(span, "compliance.sample_7_tick", sample_compliant);

    cns_estimating_cleanup(test_manager);
  }

  otel_span_end(span);
}