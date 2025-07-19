-- SQL AOT Template Queries for CNS Benchmarking
-- ==============================================

-- NAME: quarterly_sales_report
-- PARAM: quarter_num (int)
SELECT region_id, SUM(revenue) as total_revenue
FROM sales
WHERE quarter = :quarter_num
GROUP BY region_id;

-- NAME: high_value_customers
-- PARAM: min_value (float)
SELECT customer_id, customer_name, lifetime_value
FROM customers
WHERE lifetime_value > :min_value
ORDER BY lifetime_value DESC
LIMIT 100;

-- NAME: product_performance
-- PARAM: category_name (string)
SELECT product_id, product_name, SUM(sales_amount) as total_sales
FROM products p
JOIN order_items oi ON p.product_id = oi.product_id
WHERE p.category = :category_name
GROUP BY p.product_id, p.product_name
ORDER BY total_sales DESC;

-- NAME: monthly_revenue_trend
-- PARAM: start_year (int)
-- PARAM: start_month (int)
SELECT year, month, SUM(amount) as monthly_revenue
FROM orders
WHERE year >= :start_year AND (year > :start_year OR month >= :start_month)
AND status = 3
GROUP BY year, month
ORDER BY year, month;

-- NAME: customer_segment_analysis
-- PARAM: region_filter (int)
SELECT segment, COUNT(*) as customer_count, AVG(lifetime_value) as avg_ltv
FROM customers
WHERE region_id = :region_filter
GROUP BY segment;