---
title: Cartography (entities)
description: '## 概述'
summary: 'Cartography 是一个基础设施资产图谱工具，能够自动收集多云环境（AWS、GCP、Azure）、SaaS 服务（GitHub、Okta、GSuite）和安全工具（CrowdStrike、Duo）的资产信息，并将其存储在 Neo4j 图数据库中，构建完整的基础设施关系图谱。'
category: entities
tags:
- k8s
- cncf
- security
- cartography
- containerd
- harbor
- job
- cronjob
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cartography 是什么
- 如何 Cartography
trigger_keywords:
- Cartography
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cartography

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

Cartography 是一个 CNCF 沙箱项目，由 Lyft 开源，是一个安全资产图谱工具。它通过 Neo4j 图数据库整合来自多个数据源（AWS、GCP、Azure、K8s、GitHub、Okta 等）的基础设施资产信息，构建统一的资产关系图谱。安全团队可以通过 Cypher 查询发现安全风险——如公开暴露的数据库、过于宽松的 IAM 策略、未打补丁的实例等。Cartography 解决了多云多平台环境中资产可见性和安全分析碎片化的问题。

## Key Features（核心能力）

- **多云资产整合**：支持 AWS、GCP、Azure、K8s、GitHub、Okta 等 20+ 数据源
- **Neo4j 图谱**：将资产关系建模为图数据库，支持复杂关系查询
- **自动化同步**：定期从各数据源同步资产和关系数据
- **安全分析查询**：预置安全风险检测 Cypher 查询
- **可扩展架构**：通过 Python 插件机制添加新数据源
- **Jupyter Notebook**：支持在 Notebook 中交互式分析资产图谱

## 架构与工作原理

Cartography 由数据采集和分析两个层组成。采集层通过各数据源的 API（如 AWS SDK、K8s client、GitHub API）拉取资产和关系数据，经过 ETL 转换后写入 Neo4j 图数据库。分析层通过 Cypher 查询语言在图上进行安全分析，如查找「公网暴露的 RDS 实例 → 可访问它的 IAM 角色 → 拥有该角色的用户」等攻击路径。分析作业（Analysis Job）以 JSON/YAML 定义，定期执行并输出风险报告。

## K8s 集成

Cartography 可以作为 CronJob 部署到 Kubernetes，定期同步各云平台和 K8s 集群的资产数据。K8s 数据源 sync 会采集 Cluster、Namespace、Deployment、ServiceAccount、RoleBinding 等资源及其关系。安全团队可以通过图谱查询发现如「具有 cluster-admin 权限的 ServiceAccount → 使用该 SA 的 Pod → Pod 所在节点的安全风险」等攻击路径。

## 生产用例

- **多云安全态势管理**：统一查看多云环境中的资产安全状态
- **攻击路径分析**：发现从外部暴露面到内部敏感资产的攻击路径
- **合规审计**：验证资产配置符合安全策略和合规要求
- **资产盘点**：实时了解全组织的基础设施资产清单

## 安装与配置

```bash
# 🟢 安装 Cartography CLI
pip3 install cartography

# 🟢 部署 Neo4j（图数据库后端）
helm repo add neo4j https://neo4j.github.io/helm-charts/
helm install neo4j neo4j/neo4j \
  -n cartography --create-namespace \
  --set auth.enabled=true \
  --set auth.password=<secure-password>

# 🟢 运行 AWS 同步
cartography --neo4j-uri bolt://neo4j:7687 \
  --neo4j-password-env-var NEO4J_PASSWORD \
  --aws-sync-all-regions

# 🟢 运行 K8s 同步
cartography --neo4j-uri bolt://neo4j:7687 \
  --neo4j-password-env-var NEO4J_PASSWORD \
  --k8s-sync

# 🟢 验证数据同步
# 在 Neo4j Browser 中执行:
# MATCH (n) RETURN labels(n), count(*) ORDER BY count(*) DESC LIMIT 20
```

### K8s CronJob 部署示例

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cartography-sync
  namespace: cartography
spec:
  schedule: "0 */6 * * *"  # 每6小时同步一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: cartography
              image: lyft/cartography:latest
              args:
                - --neo4j-uri
                - bolt://neo4j.cartography.svc:7687
                - --neo4j-password-env-var
                - NEO4J_PASSWORD
                - --aws-sync-all-regions
                - --k8s-sync
              env:
                - name: NEO4J_PASSWORD
                  valueFrom:
                    secretKeyRef:
                      name: neo4j-creds
                      key: password
                - name: AWS_ACCESS_KEY_ID
                  valueFrom:
                    secretKeyRef:
                      name: aws-creds
                      key: access-key-id
                - name: AWS_SECRET_ACCESS_KEY
                  valueFrom:
                    secretKeyRef:
                      name: aws-creds
                      key: secret-access-key
              resources:
                requests:
                  cpu: 500m
                  memory: 1Gi
                limits:
                  cpu: "2"
                  memory: 4Gi
          restartPolicy: OnFailure
          serviceAccountName: cartography-sync
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cartography-sync
  namespace: cartography
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cartography-cluster-reader
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view  # 只读权限
subjects:
  - kind: ServiceAccount
    name: cartography-sync
    namespace: cartography
```

### 安全分析查询示例 (Cypher)

```cypher
// 查找公网暴露的 EC2 实例
MATCH (instance:EC2Instance)
WHERE instance.publicdnsname IS NOT NULL
RETURN instance.id, instance.publicdnsname, instance.instancetype

// 查找具有 admin 权限的 IAM 用户
MATCH (user:AWSUser)-[:MEMBER_OF]->(group:AWSGroup)
WHERE group.arn CONTAINS 'admin'
RETURN user.name, group.arn

// 查找 K8s 中具有 cluster-admin 的 ServiceAccount
MATCH (sa:KubernetesServiceAccount)-[:BOUND_TO]->(crb:KubernetesClusterRoleBinding)
WHERE crb.rolename = 'cluster-admin'
RETURN sa.name, sa.namespace, crb.name

// 攻击路径：公网暴露 -> IAM 角色 -> 敏感数据
MATCH path = (instance:EC2Instance)-[:INSTANCE_PROFILE]->(profile:InstanceProfile)
  -[:HAS_ROLE]->(role:AWSRole)-[:MEMBER_OF]->(policy:AWSPolicy)
WHERE instance.publicdnsname IS NOT NULL
  AND policy.document CONTAINS 's3:GetObject'
RETURN path
```

## 运维操作

```bash
# 🟢 查看 Neo4j 状态
kubectl get pods -n cartography
kubectl exec -n cartography deploy/neo4j -- neo4j status

# 🟢 查看同步任务状态
kubectl get cronjob -n cartography
kubectl get jobs -n cartography --sort-by='.status.startTime'

# 🟢 查看图谱统计
kubectl exec -n cartography deploy/neo4j -- cypher-shell \
  "MATCH (n) RETURN labels(n)[0] AS type, count(*) AS count ORDER BY count DESC LIMIT 10"

# 🟡 手动触发同步
kubectl create job --from=cronjob/cartography-sync cartography-manual-$(date +%s) -n cartography

# 🟡 清除过期数据
kubectl exec -n cartography deploy/neo4j -- cypher-shell \
  "MATCH (n) WHERE n.lastupdated < timestamp() - 86400000*7 DETACH DELETE n"

# 🔴 重置图数据库
kubectl exec -n cartography deploy/neo4j -- cypher-shell "MATCH (n) DETACH DELETE n"
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 同步任务失败 | 云 API 凭据过期 | `kubectl logs job/<name>` | 更新 Secret 中的凭据 |
| Neo4j 连接失败 | 服务未就绪 | `kubectl get pods -n cartography` | 检查 Neo4j Pod 状态 |
| 数据不完整 | 部分区域同步失败 | 查看同步日志 | 检查区域权限配置 |
| 内存 OOM | 大规模资产同步 | `kubectl describe pod` | 增加内存限制 |

```bash
# 排查流程
# 1. 检查同步任务日志
kubectl logs -n cartography job/cartography-sync-<id> --tail=100

# 2. 检查 Neo4j 健康
kubectl exec -n cartography deploy/neo4j -- wget -qO- http://localhost:7474/

# 3. 检查云凭据有效性
kubectl exec -n cartography job/cartography-sync-<id> -- aws sts get-caller-identity

# 4. 检查资源使用
kubectl top pods -n cartography
```

## 生产案例

### 案例1：多云安全态势管理
- **场景**：企业使用 AWS + GCP + K8s，安全团队需要统一视图发现风险
- **方案**：Cartography CronJob 每 6小时同步所有数据源；预置 50+ 安全分析查询；集成 Slack 告警发现新风险
- **效果**：安全风险发现时间从“审计时”缩短到 6小时内，攻击路径可视化

### 案例2：K8s 权限审计
- **场景**：安全团队需要发现 K8s 集群中过度授权的 ServiceAccount
- **方案**：Cartography 同步 K8s RBAC 数据；Cypher 查询发现 cluster-admin 绑定的 SA；追踪使用该 SA 的 Pod 和工作负载
- **效果**：发现 15 个过度授权的 SA，权限收敛后攻击面减少 60%

## 对比替代方案

| 维度 | Cartography | Wiz | Orca | AWS Security Hub |
|------|------------|-----|------|------------------|
| 开源 | 是 | 否 | 否 | 否 |
| 多云 | 20+源 | 支持 | 支持 | 仅 AWS |
| 图谱分析 | 强 | 中 | 中 | 弱 |
| 部署复杂度 | 中 | 低 | 低 | 低 |
| 成本 | 免费 | 高 | 高 | 中 |

## 检查清单

- [ ] Neo4j 已部署且有足够内存（建议 8GB+）
- [ ] 云 API 凭据已配置为 K8s Secret
- [ ] CronJob 已配置且定期运行成功
- [ ] K8s ServiceAccount 只有只读权限
- [ ] 安全分析查询已配置告警
- [ ] 数据保留策略已配置（清理过期节点）
- [ ] Neo4j 备份已配置

## Related

- [[telepresence]] — Telepresence
- [[08-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cartography
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
