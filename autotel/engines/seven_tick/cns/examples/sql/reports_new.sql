-- sample_report
SELECT 
    entity_id,
    entity_type,
    performance_score,
    created_at
FROM performance_metrics 
WHERE performance_score > :min_score
    AND created_at >= :start_date
ORDER BY performance_score DESC
LIMIT :max_results;

-- count_by_type
SELECT 
    entity_type,
    COUNT(*) as total_count,
    AVG(performance_score) as avg_score
FROM performance_metrics
WHERE created_at >= :start_date
GROUP BY entity_type
ORDER BY total_count DESC;

-- top_performers
SELECT 
    entity_id,
    entity_name,
    performance_score,
    benchmark_cycles
FROM performance_metrics pm
JOIN entity_details ed ON pm.entity_id = ed.id
WHERE pm.performance_score >= :threshold
    AND pm.benchmark_cycles <= :max_cycles
ORDER BY pm.performance_score DESC, pm.benchmark_cycles ASC
LIMIT 10;