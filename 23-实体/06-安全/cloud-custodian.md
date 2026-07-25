---
title: Cloud Custodian [entities]
description: '## 概述'
summary: 'Cloud Custodian 是云资源治理和管理的规则引擎，通过 YAML 策略实现云资源的合规性、成本优化和安全管理。它支持 AWS、Azure、GCP 等主流云平台和 Kubernetes。'
category: entities
tags:
- k8s
- cncf
- policy
- cloud-custodian
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Custodian 是什么
- 如何 Cloud Custodian
trigger_keywords:
- Cloud
- Custodian
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cloud Custodian

> **CNCF 状态**: Incubating | **类别**: Policy | **主要语言**: Python

## 概述

Cloud Custodian（简称 Custodian）是由 Capital One 开发的云资源治理规则引擎，2017 年进入 CNCF Incubating。它通过声明式的 **YAML 策略文件**定义云资源的管理规则——从资源筛选（filter）到执行操作（action），实现云资源的合规性检查、成本优化和安全治理。Custodian 支持 **AWS、Azure、GCP、阿里云、Kubernetes** 等多个云平台和基础设施。

Custodian 的核心理念是"**Policy as Code for Cloud**"。一条策略例如"找到所有公开可读的 S3 存储桶并发送告警"，可以用简洁的 YAML 表达。Custodian 支持 200+ 种资源类型和丰富的过滤/操作条件，覆盖 EC2、S3、RDS、K8s Pod、IAM 等几乎所有云资源。

## Key Features

- **声明式 YAML 策略**：`name` + `resource` + `filters` + `actions` 四段式策略定义
- **多云统一**：AWS、Azure、GCP、阿里云、Kubernetes 统一策略语法
- **实时事件驱动**：通过 CloudTrail/EventGrid 等事件触发实时策略执行
- **成本优化**：识别闲置资源（未挂载的 EBS、停止的 EC2）、调整过规格实例
- **安全合规**：检测公开的存储桶、未加密的卷、过度宽松的安全组
- **200+ 资源类型**：覆盖计算、存储、网络、数据库、安全等云资源

## Architecture

Custodian 由 **CLI 工具**（`custodian run` 执行策略）、**Policy Engine**（解析 YAML 策略，匹配资源并执行操作）和 **Cloud Provider Plugins**（AWS、Azure、GCP 等平台的资源 API 适配器）组成。策略可以以三种模式运行：Pull（定期轮询）、Event（实时事件驱动）和 Serverless（部署为 Lambda/Azure Function）。执行结果输出到日志和 metrics。

## K8s 集成

Cloud Custodian 通过 Kubernetes Provider 支持对 K8s 资源的策略执行。可以定义策略如"找到所有没有设置 resource limits 的 Pod 并告警"或"删除所有运行特权容器的 Pod"。Custodian 通过 kubeconfig 连接到集群 API Server，按策略筛选和操作 K8s 资源。

## 生产部署要点

- **先试运行**：始终使用 `--dryrun` 验证策略
- **渐进执行**：先标记 (mark-for-op)，后执行操作
- **细粒度权限**：为 Custodian 创建最小权限 IAM 角色
- **版本控制**：策略文件纳入 Git 管理
- **监控告警**：配置策略执行结果通知

## 生产场景

1. **安全合规扫描**：每日扫描所有公开的 S3 桶/安全组，自动告警和修复
2. **成本优化**：每周识别并清理闲置资源（未挂载 EBS、空闲弹性 IP），节省 10-30% 成本
3. **K8s 合规**：扫描集群中运行特权容器或无资源限制的 Pod
4. **Tag 合规**：强制所有资源必须有 team/cost-center 标签，未标记的自动标记或删除

## 安装与配置

### CLI 安装

```bash
# 安装 Cloud Custodian
pip install c7n                # 核心引擎
pip install c7n_aws            # AWS 支持
pip install c7n_gcp            # GCP 支持
pip install c7n_azure          # Azure 支持
pip install c7n_kube           # Kubernetes 支持

# 验证安装
custodian version
```

### 策略配置

```yaml
# policy.yml - AWS S3 公开桶检测
policies:
  - name: find-public-s3-buckets
    resource: aws.s3
    filters:
      - type: global-grants
    actions:
      - type: no-op
      - type: notify
        subject: "Public S3 Bucket Detected"
        to: ["security@company.com"]
        transport:
          type: sqs
          queue: https://sqs.us-east-1.amazonaws.com/123456789/security-alerts
---
# k8s-policy.yml - K8s 特权 Pod 检测
policies:
  - name: privileged-pods
    resource: k8s.pod
    filters:
      - type: value
        key: spec.containers[].securityContext.privileged
        value: true
        op: contains
    actions:
      - type: delete
```

### 执行策略

```bash
# Dryrun 模式（仅报告）
custodian run --dryrun -s output policy.yml

# 执行 K8s 策略
custodian run --output-dir output -c ~/.kube/config k8s-policy.yml

# 查看结果
cat output/find-public-s3-buckets/resources.json
```

## 运维操作

```bash
# 🟢 验证策略语法
custodian validate policy.yml

# 🟢 Dryrun 执行
custodian run --dryrun -s output policy.yml

# 🟡 执行策略（会修改资源）
custodian run -s output policy.yml

# 🟡 生成报告
custodian report -s output policy.yml

# 🔴 执行删除操作
custodian run -s output delete-policy.yml
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略验证失败 | YAML 语法错误 | `custodian validate policy.yml` | 修正 YAML |
| 无资源匹配 | 过滤器条件错误 | `custodian run --dryrun` | 调整 filters |
| 权限不足 | IAM/RBAC 配置 | 检查云账户权限 | 增加权限 |
| K8s 连接失败 | kubeconfig 错误 | `kubectl cluster-info` | 检查 kubeconfig |

**排查流程：**
```
策略执行失败
├── 验证策略语法 → custodian validate policy.yml
├── Dryrun 测试 → custodian run --dryrun
├── 检查权限 → aws sts get-caller-identity
├── 检查 K8s 连接 → kubectl cluster-info
└── 查看输出 → cat output/*/resources.json
```

## 生产案例

### 案例一：多云成本优化

- **场景**: 企业使用 AWS/Azure/GCP，需要统一成本治理
- **排查**: 使用 Cloud Custodian 识别闲置资源（未使用的 EBS、空闲 EC2）
- **方案**: 定期运行策略，自动标记和清理闲置资源
- **效果**: 云成本降低 25%，月节省 $50k

### 案例二：K8s 安全合规

- **场景**: 集群需要检测特权 Pod、缺失标签等安全问题
- **排查**: 使用 c7n_kube 策略检测不合规资源
- **方案**: CI/CD 中集成策略检查，阻止不合规部署
- **效果**: 安全合规率从 70% 提升至 99%

## 对比

| 特性 | Cloud Custodian | OPA/Gatekeeper | Kyverno | Falco | 适用场景 |
|------|----------------|----------------|---------|-------|----------|
| 多云 | ✅ AWS/Azure/GCP | ❌ K8s only | ❌ K8s only | ❌ K8s only | Custodian 多云 |
| 策略语言 | YAML | Rego | YAML | Rules | - |
| 事件驱动 | ✅ | ⚠️ | ⚠️ | ✅ | - |
| 成本优化 | ✅ | ❌ | ❌ | ❌ | FinOps |

## 参考链接

- [[pod-lifecycle]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[open-cluster-management]] — [[23-实体/09-编排调度/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cloud-custodian
- [[23-实体/09-编排调度/capsule.md|Capsule]]
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
