---
title: SQL 速查卡
description: 关系型数据库查询与运维快速参考，覆盖 MySQL 8.0 / PostgreSQL 14 / SQLite 3
category: cheatsheet
tags:
- sql
- mysql
- postgresql
- database
- cheatsheet
- quick-reference
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SQL 速查卡 是什么
- 如何 SQL 速查卡
trigger_keywords:
- SQL
- 速查卡
- cheat
- sheet
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: ../domain-28-database-middleware/
  desc: 数据库与中间件专题
- path: ../topic-cheat-sheet/linux.md
  desc: Linux 速查卡
---


# SQL 速查表

> 关系型数据库查询与运维快速参考 | MySQL 8.0 / PostgreSQL 14 / SQLite 3 | **最后更新**: 2026-05

---

## 目录

- [基础查询](#基础查询)
- [条件过滤](#条件过滤)
- [聚合与分组](#聚合与分组)
- [表连接](#表连接)
- [子查询](#子查询)
- [数据修改](#数据修改)
- [表结构操作](#表结构操作)
- [索引与优化](#索引与优化)
- [数据库管理](#数据库管理)
- [常用函数](#常用函数)

---

## 基础查询

### SELECT 语句

```sql
-- 基础查询
SELECT * FROM users;
SELECT id, name, email FROM users;

-- 去重
SELECT DISTINCT department FROM employees;

-- 别名
SELECT name AS 姓名, salary AS 薪资 FROM employees;
SELECT name 姓名, salary 薪资 FROM employees;  -- AS 可省略

-- 限制结果
SELECT * FROM users LIMIT 10;                    -- MySQL/PostgreSQL/SQLite
SELECT * FROM users FETCH FIRST 10 ROWS ONLY;    -- 标准 SQL
SELECT TOP 10 * FROM users;                       -- SQL Server
SELECT * FROM users WHERE ROWNUM <= 10;          -- Oracle

-- 分页
SELECT * FROM users LIMIT 10 OFFSET 20;          -- 第3页，每页10条
SELECT * FROM users LIMIT 20, 10;                -- MySQL 语法
```

### 排序

```sql
-- 基础排序
SELECT * FROM users ORDER BY age;
SELECT * FROM users ORDER BY age DESC;           -- 降序
SELECT * FROM users ORDER BY age ASC;            -- 升序（默认）

-- 多字段排序
SELECT * FROM employees ORDER BY department ASC, salary DESC;

-- 按位置排序
SELECT name, salary FROM employees ORDER BY 2 DESC;  -- 按第2列（salary）

-- NULL 值处理
SELECT * FROM users ORDER BY age NULLS FIRST;    -- PostgreSQL/Oracle
SELECT * FROM users ORDER BY age NULLS LAST;
SELECT * FROM users ORDER BY ISNULL(age), age;   -- MySQL
```

---

## 条件过滤

### WHERE 子句

```sql
-- 比较运算符
SELECT * FROM users WHERE age > 18;
SELECT * FROM products WHERE price <= 100;
SELECT * FROM employees WHERE salary = 5000;
SELECT * FROM employees WHERE salary <> 5000;    -- 不等于
SELECT * FROM employees WHERE salary != 5000;    -- 不等于（MySQL/SQLite）

-- 范围查询
SELECT * FROM products WHERE price BETWEEN 100 AND 500;
SELECT * FROM orders WHERE order_date BETWEEN '2024-01-01' AND '2024-12-31';

-- 列表查询
SELECT * FROM users WHERE department IN ('IT', 'HR', 'Finance');
SELECT * FROM users WHERE id NOT IN (1, 2, 3);

-- 模糊查询
SELECT * FROM users WHERE name LIKE '张%';       -- 以"张"开头
SELECT * FROM users WHERE name LIKE '%伟';       -- 以"伟"结尾
SELECT * FROM users WHERE name LIKE '%明%';      -- 包含"明"
SELECT * FROM users WHERE name LIKE '李_';       -- 单个字符匹配

-- 通配符
-- % : 匹配任意字符序列（包括空）
-- _ : 匹配单个字符
-- [] : 字符集合（SQL Server）
-- [^] : 排除字符集合（SQL Server）

-- 正则表达式（MySQL/PostgreSQL）
SELECT * FROM users WHERE name REGEXP '^[ABC]';  -- MySQL
SELECT * FROM users WHERE name ~ '^[ABC]';       -- PostgreSQL
```

### 逻辑运算符

```sql
-- AND / OR / NOT
SELECT * FROM users WHERE age > 18 AND status = 'active';
SELECT * FROM users WHERE department = 'IT' OR department = 'HR';
SELECT * FROM users WHERE NOT status = 'banned';

-- 组合使用
SELECT * FROM users 
WHERE (department = 'IT' OR department = 'HR') 
  AND status = 'active';

-- IS NULL / IS NOT NULL
SELECT * FROM users WHERE email IS NULL;
SELECT * FROM users WHERE phone IS NOT NULL;

-- EXISTS
SELECT * FROM departments d
WHERE EXISTS (
    SELECT 1 FROM employees e 
    WHERE e.dept_id = d.id
);
```

---

## 聚合与分组

### 聚合函数

```sql
-- 计数
SELECT COUNT(*) FROM users;                      -- 总行数
SELECT COUNT(id) FROM users;                     -- 非NULL计数
SELECT COUNT(DISTINCT department) FROM employees; -- 去重计数

-- 数值计算
SELECT SUM(salary) FROM employees;
SELECT AVG(salary) FROM employees;
SELECT MAX(salary) FROM employees;
SELECT MIN(salary) FROM employees;

-- 字符串聚合
SELECT GROUP_CONCAT(name) FROM users;            -- MySQL/SQLite
SELECT GROUP_CONCAT(name SEPARATOR ' | ') FROM users;  -- 指定分隔符
SELECT STRING_AGG(name, ', ') FROM users;        -- PostgreSQL/SQL Server
SELECT LISTAGG(name, ', ') FROM users;           -- Oracle
```

### GROUP BY

```sql
-- 基础分组
SELECT department, COUNT(*) as count 
FROM employees 
GROUP BY department;

-- 多字段分组
SELECT department, position, AVG(salary) as avg_salary
FROM employees
GROUP BY department, position;

-- HAVING 过滤
SELECT department, AVG(salary) as avg_salary
FROM employees
GROUP BY department
HAVING AVG(salary) > 5000;

-- WHERE vs HAVING
SELECT department, COUNT(*) as count
FROM employees
WHERE status = 'active'           -- 先过滤行
GROUP BY department
HAVING COUNT(*) > 10;             -- 再过滤组
```

---

## 表连接

### JOIN 类型

```sql
-- 内连接（INNER JOIN）
SELECT e.name, d.name as department
FROM employees e
INNER JOIN departments d ON e.dept_id = d.id;

-- 左连接（LEFT JOIN）
SELECT e.name, d.name as department
FROM employees e
LEFT JOIN departments d ON e.dept_id = d.id;

-- 右连接（RIGHT JOIN）
SELECT e.name, d.name as department
FROM employees e
RIGHT JOIN departments d ON e.dept_id = d.id;

-- 全外连接（FULL OUTER JOIN）
SELECT e.name, d.name as department
FROM employees e
FULL OUTER JOIN departments d ON e.dept_id = d.id;

-- 交叉连接（CROSS JOIN）
SELECT p.name, c.name
FROM products p
CROSS JOIN categories c;

-- 自连接
SELECT e1.name as employee, e2.name as manager
FROM employees e1
LEFT JOIN employees e2 ON e1.manager_id = e2.id;
```

### 连接示意图

```
INNER JOIN:     两个表的交集
LEFT JOIN:      左表全部 + 右表匹配
RIGHT JOIN:     右表全部 + 左表匹配
FULL JOIN:      两个表的并集
CROSS JOIN:     笛卡尔积
```

### 多表连接

```sql
SELECT 
    o.id,
    c.name as customer,
    p.name as product,
    o.quantity,
    o.quantity * p.price as total
FROM orders o
JOIN customers c ON o.customer_id = c.id
JOIN products p ON o.product_id = p.id
WHERE o.status = 'completed';
```

---

## 子查询

### 标量子查询

```sql
-- 单行单列结果
SELECT name, salary,
    (SELECT AVG(salary) FROM employees) as avg_salary
FROM employees;
```

### 行子查询

```sql
-- 多列单行结果
SELECT *
FROM employees
WHERE (department_id, position) = (
    SELECT department_id, position
    FROM employees
    WHERE name = '张三'
);
```

### 表子查询

```sql
-- 多行多列结果
SELECT * FROM (
    SELECT department, AVG(salary) as avg_salary
    FROM employees
    GROUP BY department
) as dept_avg
WHERE avg_salary > 5000;

-- 使用 WITH (CTE)
WITH dept_avg AS (
    SELECT department, AVG(salary) as avg_salary
    FROM employees
    GROUP BY department
)
SELECT * FROM dept_avg WHERE avg_salary > 5000;

-- 递归 CTE
WITH RECURSIVE subordinates AS (
    -- 基础查询：直接下属
    SELECT id, name, manager_id, 0 as level
    FROM employees
    WHERE manager_id = 1
    
    UNION ALL
    
    -- 递归查询：间接下属
    SELECT e.id, e.name, e.manager_id, s.level + 1
    FROM employees e
    INNER JOIN subordinates s ON e.manager_id = s.id
)
SELECT * FROM subordinates;
```

### 关联子查询

```sql
-- 相关子查询
SELECT e1.name, e1.salary
FROM employees e1
WHERE e1.salary > (
    SELECT AVG(e2.salary)
    FROM employees e2
    WHERE e2.department = e1.department  -- 关联条件
);

-- 使用窗口函数替代（更高效）
SELECT name, salary, avg_dept_salary
FROM (
    SELECT 
        name, 
        salary,
        AVG(salary) OVER (PARTITION BY department) as avg_dept_salary
    FROM employees
) t
WHERE salary > avg_dept_salary;
```

---

## 数据修改

### INSERT

```sql
-- 插入单行
INSERT INTO users (name, email, age) 
VALUES ('张三', 'zhangsan@example.com', 25);

-- 插入多行
INSERT INTO users (name, email, age) VALUES
('张三', 'zhangsan@example.com', 25),
('李四', 'lisi@example.com', 30),
('王五', 'wangwu@example.com', 28);

-- 插入查询结果
INSERT INTO employees_backup
SELECT * FROM employees WHERE status = 'active';

-- 插入或更新（UPSERT）
-- MySQL
INSERT INTO users (id, name, email) VALUES (1, '张三', 'zs@example.com')
ON DUPLICATE KEY UPDATE name = '张三', email = 'zs@example.com';

-- PostgreSQL
INSERT INTO users (id, name, email) VALUES (1, '张三', 'zs@example.com')
ON CONFLICT (id) DO UPDATE SET name = '张三', email = 'zs@example.com';

-- SQLite
INSERT OR REPLACE INTO users (id, name, email) 
VALUES (1, '张三', 'zs@example.com');
```

### UPDATE

```sql
-- 基础更新
UPDATE users SET age = 26 WHERE id = 1;

-- 更新多列
UPDATE employees 
SET salary = salary * 1.1, updated_at = NOW()
WHERE department = 'IT';

-- 使用子查询更新
UPDATE employees e
SET salary = (
    SELECT AVG(salary) * 1.2
    FROM employees
    WHERE department = e.department
)
WHERE performance_rating = 'A';

-- 更新并返回（PostgreSQL）
UPDATE users SET age = 26 WHERE id = 1 RETURNING *;
```

### DELETE

```sql
-- 基础删除
DELETE FROM users WHERE id = 1;

-- 删除所有数据
DELETE FROM users;                  -- 保留表结构
TRUNCATE TABLE users;               -- 更快，重置自增

-- 使用子查询删除
DELETE FROM employees
WHERE department_id IN (
    SELECT id FROM departments 
    WHERE name = '临时部门'
);

-- 删除并返回（PostgreSQL）
DELETE FROM users WHERE id = 1 RETURNING *;

-- 限制删除数量（MySQL）
DELETE FROM logs WHERE created_at < DATE_SUB(NOW(), INTERVAL 30 DAY) LIMIT 1000;
```

---

## 表结构操作

### 创建表

```sql
-- 基础建表
CREATE TABLE users (
    id INT PRIMARY KEY AUTO_INCREMENT,      -- MySQL
    -- id SERIAL PRIMARY KEY,                 -- PostgreSQL
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE,
    age INT CHECK (age >= 0),
    status ENUM('active', 'inactive') DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP  -- MySQL
);

-- 带注释（MySQL/PostgreSQL）
CREATE TABLE products (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '主键ID',
    name VARCHAR(200) NOT NULL COMMENT '产品名称',
    price DECIMAL(10,2) COMMENT '价格',
    INDEX idx_name (name)                    -- 表级索引定义
) COMMENT='产品表';
```

### 修改表

```sql
-- 添加列
ALTER TABLE users ADD COLUMN phone VARCHAR(20);
ALTER TABLE users ADD COLUMN address VARCHAR(255) AFTER email;  -- MySQL

-- 修改列
ALTER TABLE users MODIFY COLUMN age SMALLINT;                    -- MySQL
ALTER TABLE users ALTER COLUMN age TYPE SMALLINT;                -- PostgreSQL

-- 重命名列
ALTER TABLE users CHANGE COLUMN phone mobile VARCHAR(20);        -- MySQL
ALTER TABLE users RENAME COLUMN phone TO mobile;                 -- PostgreSQL

-- 删除列
ALTER TABLE users DROP COLUMN phone;

-- 添加约束
ALTER TABLE users ADD CONSTRAINT fk_dept 
    FOREIGN KEY (dept_id) REFERENCES departments(id);

-- 添加索引
ALTER TABLE users ADD INDEX idx_email (email);
CREATE INDEX idx_name ON users (name);

-- 删除索引
ALTER TABLE users DROP INDEX idx_email;                          -- MySQL
DROP INDEX idx_email ON users;                                    -- SQL Server
DROP INDEX idx_email;                                             -- PostgreSQL/Oracle

-- 重命名表
ALTER TABLE old_users RENAME TO users;                           -- MySQL/PostgreSQL
EXEC sp_rename 'old_users', 'users';                             -- SQL Server
RENAME old_users TO users;                                        -- Oracle

-- 删除表
DROP TABLE users;
DROP TABLE IF EXISTS users;                                       -- 安全删除
DROP TABLE users CASCADE;                                         -- 级联删除依赖
```

---

## 索引与优化

### 索引管理

```sql
-- 创建索引
CREATE INDEX idx_name ON users (name);
CREATE UNIQUE INDEX idx_email ON users (email);
CREATE INDEX idx_name_age ON users (name, age);                  -- 复合索引

-- 查看索引
SHOW INDEX FROM users;                                            -- MySQL
\\di users;                                                        -- PostgreSQL (psql)
SELECT * FROM sqlite_master WHERE type='index';                   -- SQLite

-- 删除索引
DROP INDEX idx_name ON users;                                     -- MySQL/SQL Server
DROP INDEX idx_name;                                               -- PostgreSQL/Oracle

-- 分析表
ANALYZE TABLE users;                                              -- MySQL
ANALYZE users;                                                    -- PostgreSQL
```

### 查询优化

```sql
-- 查看执行计划
EXPLAIN SELECT * FROM users WHERE name = '张三';                  -- MySQL
EXPLAIN ANALYZE SELECT * FROM users WHERE name = '张三';          -- PostgreSQL
EXPLAIN QUERY PLAN SELECT * FROM users WHERE name = '张三';       -- SQLite

-- 强制使用索引
SELECT * FROM users USE INDEX (idx_name) WHERE name = '张三';     -- MySQL
SELECT /*+ INDEX(users idx_name) */ * FROM users WHERE name = '张三'; -- Oracle
```

### 性能优化原则

| 原则 | 说明 |
|:---|:---|
| **选择性** | 高选择性字段（如 email）适合建索引，低选择性（如性别）不适合 |
| **最左前缀** | 复合索引 (a,b,c) 可以支持 a、ab、abc 查询，但不支持 bc |
| **覆盖索引** | 查询字段都在索引中，避免回表查询 |
| **避免函数** | 避免在索引字段上使用函数或运算，会导致索引失效 |
| **定期维护** | 定期 ANALYZE 更新统计信息，重建碎片索引 |

---

## 数据库管理

### 用户权限

```sql
-- 创建用户
CREATE USER 'appuser'@'localhost' IDENTIFIED BY 'password';       -- MySQL
CREATE USER appuser WITH PASSWORD 'password';                     -- PostgreSQL

-- 授权
GRANT SELECT, INSERT, UPDATE ON database.* TO 'appuser'@'localhost';
GRANT ALL PRIVILEGES ON database.* TO 'appuser'@'localhost';
GRANT SELECT ON database.table TO appuser;                        -- PostgreSQL

-- 撤销权限
REVOKE INSERT ON database.* FROM 'appuser'@'localhost';

-- 查看权限
SHOW GRANTS FOR 'appuser'@'localhost';                            -- MySQL
\\du                                                              -- PostgreSQL (查看所有用户)
\\dp                                                              -- PostgreSQL (查看权限)

-- 删除用户
DROP USER 'appuser'@'localhost';
```

### 事务控制

```sql
-- 基础事务
BEGIN;                            -- 或 START TRANSACTION
UPDATE accounts SET balance = balance - 100 WHERE id = 1;
UPDATE accounts SET balance = balance + 100 WHERE id = 2;
COMMIT;                           -- 提交
-- ROLLBACK;                      -- 回滚

-- 保存点
BEGIN;
UPDATE accounts SET balance = balance - 100 WHERE id = 1;
SAVEPOINT sp1;
UPDATE accounts SET balance = balance + 100 WHERE id = 2;
-- 发生错误时回滚到保存点
ROLLBACK TO sp1;
COMMIT;

-- 事务隔离级别
SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED;
SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ;
SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE;
```

### 备份恢复

```bash
# MySQL 备份恢复
mysqldump -u root -p database > backup.sql                    # 备份
mysql -u root -p database < backup.sql                        # 恢复
mysqldump -u root -p --all-databases > all_databases.sql      # 备份所有库

# PostgreSQL 备份恢复
pg_dump -U postgres database > backup.sql                     # SQL 格式备份
pg_dump -U postgres -Fc database > backup.dump                # 自定义格式
createdb new_database
pg_restore -U postgres -d new_database backup.dump            # 恢复

# SQLite 备份恢复
sqlite3 database ".backup backup.db"                          # 备份
sqlite3 database ".dump" > backup.sql                         # SQL 导出
sqlite3 new_database < backup.sql                             # 恢复
```

---

## 常用函数

### 字符串函数

```sql
-- 连接
SELECT CONCAT('Hello', ' ', 'World');            -- MySQL/PostgreSQL/Oracle
SELECT 'Hello' || ' ' || 'World';                 -- PostgreSQL/Oracle/SQLite
SELECT 'Hello' + ' ' + 'World';                   -- SQL Server

-- 长度
SELECT LENGTH('Hello');                           -- PostgreSQL/SQLite
SELECT CHAR_LENGTH('Hello');                      -- MySQL
SELECT LEN('Hello');                              -- SQL Server

-- 截取
SELECT SUBSTRING('Hello World', 1, 5);            -- 提取 'Hello'
SELECT LEFT('Hello World', 5);                    -- 左边5个字符
SELECT RIGHT('Hello World', 5);                   -- 右边5个字符

-- 查找替换
SELECT REPLACE('Hello World', 'World', 'SQL');    -- 替换
SELECT INSTR('Hello World', 'World');             -- 位置（MySQL/Oracle）
SELECT POSITION('World' IN 'Hello World');        -- 位置（PostgreSQL）
SELECT CHARINDEX('World', 'Hello World');         -- 位置（SQL Server）

-- 大小写
SELECT UPPER('hello');                            -- HELLO
SELECT LOWER('WORLD');                            -- world
```

### 日期函数

```sql
-- 当前日期时间
SELECT NOW();                                     -- MySQL/PostgreSQL
SELECT CURRENT_TIMESTAMP;                         -- 标准 SQL
SELECT GETDATE();                                 -- SQL Server
SELECT SYSDATE FROM DUAL;                         -- Oracle
SELECT DATE('now');                               -- SQLite

-- 日期提取
SELECT YEAR(created_at), MONTH(created_at), DAY(created_at);  -- MySQL/SQL Server
SELECT EXTRACT(YEAR FROM created_at);             -- PostgreSQL/Oracle
SELECT strftime('%Y', created_at);                -- SQLite

-- 日期计算
SELECT DATE_ADD(NOW(), INTERVAL 7 DAY);           -- MySQL
SELECT NOW() + INTERVAL '7 days';                 -- PostgreSQL
SELECT DATE('now', '+7 days');                    -- SQLite
SELECT DATEADD(day, 7, GETDATE());                -- SQL Server

-- 格式化
SELECT DATE_FORMAT(NOW(), '%Y-%m-%d');            -- MySQL
SELECT TO_CHAR(NOW(), 'YYYY-MM-DD');              -- PostgreSQL/Oracle
SELECT FORMAT(GETDATE(), 'yyyy-MM-dd');           -- SQL Server
SELECT strftime('%Y-%m-%d', 'now');               -- SQLite
```

### 数值函数

```sql
SELECT ABS(-10);                                  -- 绝对值: 10
SELECT ROUND(3.14159, 2);                         -- 四舍五入: 3.14
SELECT CEIL(3.2);                                 -- 向上取整: 4
SELECT FLOOR(3.8);                                -- 向下取整: 3
SELECT MOD(10, 3);                                -- 取余: 1
SELECT POWER(2, 10);                              -- 幂: 1024
SELECT SQRT(16);                                  -- 平方根: 4
SELECT RAND();                                    -- 随机数 (MySQL)
SELECT RANDOM();                                  -- 随机数 (PostgreSQL/SQLite)
```

### 条件函数

```sql
-- IF 函数
SELECT IF(age >= 18, 'Adult', 'Minor') FROM users;             -- MySQL
SELECT IIF(age >= 18, 'Adult', 'Minor') FROM users;            -- SQL Server

-- CASE 表达式
SELECT 
    name,
    CASE 
        WHEN age < 13 THEN 'Child'
        WHEN age < 20 THEN 'Teenager'
        WHEN age < 60 THEN 'Adult'
        ELSE 'Senior'
    END as age_group
FROM users;

-- 简单 CASE
SELECT 
    name,
    CASE department
        WHEN 'IT' THEN 'Information Technology'
        WHEN 'HR' THEN 'Human Resources'
        ELSE 'Other'
    END as dept_full
FROM employees;

-- COALESCE（返回第一个非NULL值）
SELECT COALESCE(mobile, phone, email) as contact FROM users;

-- NULLIF（相等返回NULL）
SELECT NULLIF(salary, 0) FROM employees;          -- salary为0时返回NULL
```

---

## 相关文档

- [domain-28-enterprise-database-middleware/](../domain-28-enterprise-database-middleware/) - 数据库中间件
