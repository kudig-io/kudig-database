---
title: Cartography
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- postgresql
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cartography 是什么
- 如何 Cartography
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cartography
- cncf
- landscape
---

# Cartography

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://lyft.github.io/cartography/ |
| **GitHub** | https://github.com/lyft/cartography |
| **许可证** | Apache-2.0 |
| **开发语言** | Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Cartography 是一个基础设施资产图谱工具，能够自动收集多云环境（AWS、GCP、Azure）、SaaS 服务（GitHub、Okta、GSuite）和安全工具（CrowdStrike、Duo）的资产信息，并将其存储在 Neo4j 图数据库中，构建完整的基础设施关系图谱。安全团队和运维团队可以通过 Cypher 查询语言进行跨资源的关联分析、攻击面评估和合规审计。

### 核心特性

- **多云资产收集**: 自动收集 AWS、GCP、Azure 的 IAM、网络、计算、存储等资源信息
- **图关系建模**: 使用 Neo4j 图数据库存储资产及其关系，支持复杂的关联查询
- **安全分析**: 发现暴露的安全组、过期证书、过度授权的 IAM 角色等安全风险
- **SaaS 集成**: 收集 GitHub、Okta、GSuite 等 SaaS 服务的用户和权限信息
- **定时同步**: 周期性同步基础设施状态，追踪资源变化
- **可扩展**: 插件化 Intel Module 架构，可自定义数据源

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Cartography Engine                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │            Intel Modules (数据采集)            │    │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────┐   │    │
│  │  │ AWS  │ │ GCP  │ │Azure │ │ GitHub   │   │    │
│  │  │Module│ │Module│ │Module│ │ Module   │   │    │
│  │  └──┬───┘ └──┬───┘ └──┬───┘ └────┬─────┘   │    │
│  │     │        │        │           │          │    │
│  │  ┌──▼────┐ ┌─▼────┐ ┌▼──────┐ ┌──▼───────┐ │    │
│  │  │Okta   │ │CStrike││K8s    │ │Custom   │ │    │
│  │  │Module │ │Module │ │Module │ │Module   │ │    │
│  │  └──┬────┘ └──┬───┘ └──┬────┘ └──┬──────┘ │    │
│  └─────┼─────────┼────────┼─────────┼─────────┘    │
│        └─────────┴────────┴─────────┘               │
│                       │                              │
│              ┌────────▼────────┐                     │
│              │  Transform &    │                     │
│              │  Load Pipeline  │                     │
│              └────────┬────────┘                     │
└───────────────────────┼──────────────────────────────┘
                        │
               ┌────────▼────────┐
               │    Neo4j Graph   │
               │    Database      │
               │                  │
               │  (Nodes)         │
               │  EC2Instance ──► │
               │  S3Bucket ──►    │
               │  IAMRole ──►     │
               │  SecurityGroup   │
               │  (Relationships) │
               │  MEMBER_OF       │
               │  HAS_ACCESS_TO   │
               │  EXPOSED_TO      │
               └──────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 pip 安装
pip install cartography

# 安装 Neo4j (Docker)
docker run -d \
  --name neo4j \
  -p 7474:7474 -p 7687:7687 \
  -e NEO4J_AUTH=neo4j/password \
  neo4j:5-community
```

### 同步 AWS 资产

```bash
# 配置 AWS 凭据
export AWS_DEFAULT_REGION=us-east-1

# 运行 Cartography 同步
cartography \
  --neo4j-uri bolt://localhost:7687 \
  --neo4j-user neo4j \
  --neo4j-password-env-var NEO4J_PASSWORD
```

### 安全查询示例

```cypher
// 查找公开暴露的 S3 存储桶
MATCH (s:S3Bucket)
WHERE s.anonymous_access = true
RETURN s.name, s.arn

// 查找过度授权的 IAM 角色 (拥有 * 权限)
MATCH (role:AWSRole)-[:POLICY]->(policy:AWSPolicy)-[:STATEMENT]->(stmt)
WHERE stmt.effect = 'Allow' AND stmt.action CONTAINS '*' AND stmt.resource = '*'
RETURN role.name, policy.name

// 查找暴露到公网的 EC2 实例
MATCH (ec2:EC2Instance)-[:MEMBER_OF_EC2_SECURITY_GROUP]->(sg:EC2SecurityGroup)
      -[:INBOUND_RULE]->(rule)
WHERE rule.fromport <= 22 AND rule.toport >= 22
      AND rule.cidrip = '0.0.0.0/0'
RETURN ec2.instanceid, ec2.publicipaddress, sg.groupname

// 查找跨账号 IAM 信任关系
MATCH (role:AWSRole)-[:TRUSTS_AWS_PRINCIPAL]->(principal:AWSPrincipal)
WHERE principal.arn CONTAINS ':root'
RETURN role.arn, principal.arn
```

---

## 高级功能

### 多云同步配置

```bash
# 同步 AWS + GCP + Azure
cartography \
  --neo4j-uri bolt://neo4j:7687 \
  --aws-sync-all-profiles \
  --gcp-project-id my-gcp-project \
  --azure-tenant-id $AZURE_TENANT_ID \
  --azure-client-id $AZURE_CLIENT_ID \
  --azure-client-secret-env-var AZURE_CLIENT_SECRET

# 仅同步特定 AWS 模块
cartography \
  --neo4j-uri bolt://neo4j:7687 \
  --selected-modules aws ec2,iam,s3
```

### GitHub 组织资产同步

```bash
# 同步 GitHub 组织信息
cartography \
  --neo4j-uri bolt://neo4j:7687 \
  --github-config-env-var GITHUB_TOKEN

# 查询: 谁拥有仓库管理员权限
# MATCH (u:GitHubUser)-[r:ADMIN]->(repo:GitHubRepository)
# RETURN u.login, collect(repo.name)
```

### Kubernetes 集群资产同步

```bash
# 同步 K8s 集群信息
cartography \
  --neo4j-uri bolt://neo4j:7687 \
  --k8s-kubeconfig ~/.kube/config

# 查询: 拥有 ClusterAdmin 权限的 ServiceAccount
# MATCH (sa:KubernetesServiceAccount)-[:BOUND_TO]->(crb:KubernetesClusterRoleBinding)
#       -[:GRANTS]->(cr:KubernetesClusterRole)
# WHERE cr.name = 'cluster-admin'
# RETURN sa.name, sa.namespace
```

### 定时同步 (CronJob)

```yaml
# cartography-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cartography-sync
  namespace: security
spec:
  schedule: "0 */6 * * *"  # 每 6 小时同步
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: cartography
              image: ghcr.io/lyft/cartography:latest
              args:
                - "--neo4j-uri=bolt://neo4j.security:7687"
              env:
                - name: NEO4J_PASSWORD
                  valueFrom:
                    secretKeyRef:
                      name: neo4j-creds
                      key: password
                - name: AWS_REGION
                  value: "us-east-1"
          restartPolicy: OnFailure
          serviceAccountName: cartography
```

---

## 与其他方案对比

| 特性 | Cartography | CloudQuery | Steampipe | AWS Config |
|:---|:---|:---|:---|:---|
| 数据存储 | Neo4j 图数据库 | PostgreSQL/多种 | PostgreSQL | AWS 内部 |
| 关系建模 | 图关系原生 | 关系表 | 关系表 | 规则驱动 |
| 多云支持 | AWS/GCP/Azure | 多种 | 多种 | 仅 AWS |
| SaaS 集成 | GitHub/Okta 等 | 多种插件 | 多种插件 | 无 |
| 查询语言 | Cypher | SQL | SQL | 规则引擎 |
| 攻击面分析 | 图遍历天然支持 | 需 JOIN | 需 JOIN | 有限 |

---

## 最佳实践

1. **定期同步**: 配置 CronJob 每 4-6 小时同步一次，保持资产图谱时效性
2. **多账号**: 使用 AWS Organization 跨账号角色假设，统一收集所有账号资产
3. **安全查询库**: 建立团队共享的安全查询模板库，标准化风险检测流程
4. **数据保留**: 配置 Neo4j 节点过期策略，清理历史数据避免存储膨胀
5. **权限最小化**: Cartography 使用的凭据应仅授予只读权限

---

## 参考资源

- [Cartography 官方文档](https://lyft.github.io/cartography/)
- [Cartography GitHub](https://github.com/lyft/cartography)
- [Cartography 查询示例](https://lyft.github.io/cartography/usage/schema.html)
- [Neo4j Cypher 指南](https://neo4j.com/docs/cypher-manual/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
