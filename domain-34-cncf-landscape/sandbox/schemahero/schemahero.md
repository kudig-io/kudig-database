# SchemaHero

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://schemahero.io/ |
| **GitHub** | https://github.com/schemahero/schemahero |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

SchemaHero 是一个 Kubernetes 原生的数据库 Schema 迁移工具。它采用声明式方法管理数据库表结构，开发者只需定义期望的 Schema 状态，SchemaHero 自动计算并执行所需的 DDL 变更。支持 PostgreSQL、MySQL、CockroachDB、SQLite 等数据库。

### 核心特性

- **声明式 Schema**: 定义期望的表结构，自动计算迁移 DDL
- **Kubernetes CRD**: 使用 Table 和 Database CRD 管理 Schema
- **安全迁移**: 生成迁移计划供审批后再执行
- **多数据库**: PostgreSQL, MySQL, CockroachDB, SQLite, Cassandra
- **GitOps 集成**: Schema 定义纳入 Git 管理，与 ArgoCD/Flux 集成
- **回滚保护**: 危险操作需要手动确认

---

## 快速开始

### 安装

```bash
# 安装 SchemaHero Operator
kubectl apply -f https://raw.githubusercontent.com/schemahero/schemahero/main/deploy/operator.yaml

# 安装 CLI
brew install schemahero/tap/schemahero
```

### 定义数据库连接

```yaml
apiVersion: databases.schemahero.io/v1alpha4
kind: Database
metadata:
  name: my-database
spec:
  connection:
    postgres:
      uri:
        valueFrom:
          secretKeyRef:
            name: postgres-credentials
            key: uri
  # 或 MySQL
  # connection:
  #   mysql:
  #     uri:
  #       value: "user:password@tcp(mysql:3306)/mydb"
```

### 定义表结构

```yaml
apiVersion: schemas.schemahero.io/v1alpha4
kind: Table
metadata:
  name: users
database: my-database
spec:
  name: users
  schema:
    postgres:
      primaryKey:
        - id
      columns:
        - name: id
          type: uuid
          default: "gen_random_uuid()"
          constraints:
            notNull: true
        - name: email
          type: varchar(255)
          constraints:
            notNull: true
            unique: true
        - name: name
          type: varchar(100)
        - name: created_at
          type: timestamptz
          default: "now()"
        - name: updated_at
          type: timestamptz
      indexes:
        - columns: [email]
          name: idx_users_email
          isUnique: true
---
apiVersion: schemas.schemahero.io/v1alpha4
kind: Table
metadata:
  name: orders
database: my-database
spec:
  name: orders
  schema:
    postgres:
      primaryKey:
        - id
      columns:
        - name: id
          type: serial
        - name: user_id
          type: uuid
          constraints:
            notNull: true
        - name: amount
          type: decimal(10,2)
        - name: status
          type: varchar(20)
          default: "'pending'"
      foreignKeys:
        - columns: [user_id]
          references:
            table: users
            columns: [id]
```

### 审批和执行迁移

```bash
# 查看待执行的迁移
kubectl schemahero get migrations

# 查看迁移详情（生成的 DDL）
kubectl schemahero describe migration <migration-name>
# 输出: ALTER TABLE users ADD COLUMN name varchar(100);

# 批准迁移
kubectl schemahero approve migration <migration-name>

# 拒绝迁移
kubectl schemahero reject migration <migration-name>
```

---

## 最佳实践

1. **声明式管理**: 只定义期望的 Schema 状态，让 SchemaHero 计算变更
2. **审批流程**: 生产环境始终启用审批流程，审查 DDL 后再执行
3. **GitOps**: 将 Table CRD 存储在 Git 中，通过 ArgoCD/Flux 管理
4. **增量变更**: 每次只修改一个表结构，便于追踪和回滚
5. **数据库密钥**: 使用 Kubernetes Secret 管理数据库连接字符串

---

## 参考资源

- [SchemaHero 官方文档](https://schemahero.io/docs/)
- [SchemaHero GitHub](https://github.com/schemahero/schemahero)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
