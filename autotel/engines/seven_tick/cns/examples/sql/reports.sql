-- CNS SQL Query Examples for AOT Compilation
-- This file provides test queries for the 80/20 benchmark

-- Simple SELECT query (80% case)
-- quarterly_sales_report
SELECT id, name, sales_amount 
FROM sales 
WHERE quarter = ? AND year = ?;

-- Aggregation query (80% case)  
-- total_revenue_by_quarter
SELECT quarter, SUM(sales_amount) as total_revenue
FROM sales
WHERE year = ?
GROUP BY quarter
ORDER BY quarter;

-- JOIN query (20% case)
-- customer_order_details
SELECT c.name, o.order_date, o.total_amount
FROM customers c
JOIN orders o ON c.id = o.customer_id
WHERE o.order_date >= ? AND o.order_date <= ?;

-- Complex aggregation (20% case)
-- top_customers_by_revenue
SELECT c.name, SUM(o.total_amount) as total_spent
FROM customers c
JOIN orders o ON c.id = o.customer_id
WHERE o.order_date >= ?
GROUP BY c.id, c.name
HAVING SUM(o.total_amount) > ?
ORDER BY total_spent DESC
LIMIT 10;

-- Window function query (5% edge case)
-- monthly_sales_trend
SELECT 
    month,
    sales_amount,
    LAG(sales_amount) OVER (ORDER BY month) as prev_month,
    sales_amount - LAG(sales_amount) OVER (ORDER BY month) as growth
FROM monthly_sales
WHERE year = ?
ORDER BY month;