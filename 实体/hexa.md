---
title: Hexa
description: '## 概述'
summary: 'Hexa 是一个统一的策略编排引擎，使用 IDQL (Identity Query Language) 作为通用策略语言，实现跨多个云平台和授权系统的访问控制策略管理。它支持将策略从一个授权系统（如 AWS IAM、Azure RBAC、Google IAP）翻译和同步到另一个系统，避免了在不同平台上重复维护相似策略的问题。'
category: entities
tags:
- k8s
- cncf
- security
- hexa
- istio
- opa
- rbac
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hexa 是什么
- 如何 Hexa
trigger_keywords:
- Hexa
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Hexa

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Hexa 是一个 CNCF 沙箱项目，旨在提供 Kubernetes 多集群应用编排和策略管理能力。它专注于解决多集群环境下的应用一致性部署、配置同步和生命周期管理问题。Hexa 通过统一的控制平面管理跨集群的应用部署状态，支持基于策略的工作负载分发和差异化配置。项目特别关注多租户和多团队场景下的集群资源管理。

## Key Features（核心能力）

- **多集群应用编排**：将应用部署到多个 K8s 集群并保持一致性
- **策略驱动分发**：基于标签、容量、地域的策略控制部署目标
- **配置同步**：跨集群的 ConfigMap、Secret 等配置资源同步
- **健康监控**：监控多集群应用部署健康状态
- **差异化配置**：支持不同集群的定制化配置覆盖
- **GitOps 兼容**：与 Git 仓库集成的声明式管理

## 架构与工作原理

Hexa 采用 Hub-Spoke 控制平面架构：Hub Controller 运行在管理集群，监听应用部署 CRD 并根据分发策略将工作负载推送到目标集群；Spoke Agent 运行在目标集群，接收部署指令并协调本地 K8s 资源。分发策略通过 Policy CRD 定义，支持基于集群标签、资源容量和地理位置的智能分发。

## K8s 集成

Hexa 通过 CRD 与 Kubernetes 集成：ApplicationDistribution CRD 定义应用的多集群部署配置；ClusterPolicy CRD 定义分发策略。Hub Controller 管理这些 CRD 的生命周期，通过 K8s API 连接到各目标集群，推送工作负载配置。Spoke Agent 在目标集群中执行实际的资源创建和状态上报。

## 生产用例

- **多集群应用部署**：将应用一致性部署到生产、DR、边缘集群
- **多租户资源管理**：为不同团队分配集群资源并管理部署
- **灾难恢复**：跨集群的应用快速切换和恢复
- **地理分布部署**：按地域策略将应用部署到最近的集群

## 安装与配置

```bash
# 🟢 安装 Hexa Operator
kubectl apply -f https://github.com/hexa-org/hexa/releases/latest/download/hexa-operator.yaml

# 🟢 验证安装
kubectl get pods -n hexa-system
kubectl get crd | grep hexa.org

# 🟢 配置 IDQL 策略引擎
kubectl apply -f hexa-policy-engine.yaml

# 🟢 注册授权系统 Provider
kubectl apply -f provider-registration.yaml

# 🟢 查看已注册的 Provider
kubectl get provider -A
```

### IDQL 策略示例

```yaml
apiVersion: hexa.org/v1alpha1
kind: PolicyMapping
metadata:
  name: app-access-policy
  namespace: hexa-system
spec:
  # 源授权系统
  source:
    provider: aws-iam
    integration: prod-aws-account
  # 目标授权系统
  target:
    provider: k8s-rbac
    integration: prod-cluster
  # IDQL 策略定义
  policy:
    idql: |
      allow
      subject = "user:developer@company.com"
      action = "read"
      resource = "namespace:production/*"
      when time.hour >= 9 and time.hour <= 18
---
apiVersion: hexa.org/v1alpha1
kind: Provider
metadata:
  name: aws-iam-provider
  namespace: hexa-system
spec:
  type: aws-iam
  config:
    region: us-east-1
    credentialsSecret: aws-creds
---
apiVersion: hexa.org/v1alpha1
kind: Provider
metadata:
  name: k8s-rbac-provider
  namespace: hexa-system
spec:
  type: kubernetes-rbac
  config:
    kubeconfigSecret: prod-cluster-kubeconfig
    namespaces:
      - production
      - staging
```

## 运维操作

```bash
# 🟢 查看策略映射状态
kubectl get policymapping -A
kubectl describe policymapping app-access-policy -n hexa-system

# 🟢 查看 Provider 连接状态
kubectl get provider -A -o wide

# 🟡 强制同步策略
kubectl annotate policymapping app-access-policy -n hexa-system \
  hexa.org/force-sync=$(date +%s) --overwrite

# 🟡 禁用策略映射
kubectl patch policymapping app-access-policy -n hexa-system --type=merge -p \
  '{"spec":{"enabled":false}}'

# 🔴 删除策略映射（会清理目标系统上的对应规则）
kubectl delete policymapping app-access-policy -n hexa-system
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Provider 状态 Disconnected | 凭据过期/网络问题 | `kubectl get provider -o wide` | 更新 Secret 中的凭据 |
| 策略同步失败 | IDQL 语法错误 | `kubectl describe policymapping <name>` | 验证 IDQL 表达式语法 |
| 目标系统未更新 | RBAC 权限不足 | 查看 Operator 日志 | 检查目标集群权限 |
| 策略冲突 | 多策略覆盖同一资源 | `kubectl get policymapping -A` | 调整策略优先级 |

```bash
# 排查流程
# 1. 检查 Operator 状态
kubectl logs -n hexa-system -l app=hexa-operator --tail=100

# 2. 检查 Provider 连接
kubectl get provider -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'

# 3. 检查策略同步事件
kubectl get events -n hexa-system --sort-by='.lastTimestamp' | grep hexa

# 4. 验证 IDQL 策略解析
kubectl exec -n hexa-system deploy/hexa-operator -- hexa-cli validate-policy /policies/app-access.idql
```

## 生产案例

### 案例1：多云统一访问控制
- **场景**：企业同时使用 AWS、Azure 和 K8s，需要统一管理开发团队的访问权限
- **方案**：使用 IDQL 定义统一策略；通过 Provider 同步到 AWS IAM、Azure RBAC 和 K8s RBAC；单一策略变更自动同步到所有平台
- **效果**：策略管理时间减少 80%，跨平台权限不一致问题降为 0

### 案例2：合规审计自动化
- **场景**：金融企业需要确保所有平台的访问策略符合最小权限原则
- **方案**：IDQL 策略内置时间约束和 MFA 要求；定期扫描检测策略漂移；自动生成合规报告
- **效果**：合规审计时间从 2周 缩短到 2天，策略违规检测实时化

## 对比替代方案

| 维度 | Hexa | OPA/Gatekeeper | Cedar | AWS IAM |
|------|------|---------------|-------|--------|
| 跨平台同步 | 核心能力 | 无 | 无 | 单平台 |
| 策略语言 | IDQL | Rego | Cedar | JSON |
| K8s 集成 | 强 | 原生 | 中 | 无 |
| 学习曲线 | 中 | 高 | 中 | 低 |
| 多云支持 | 强 | 弱 | 弱 | 无 |

## 检查清单

- [ ] Hexa Operator 已部署且 Pod Running
- [ ] Provider 已注册且状态为 Connected
- [ ] 目标系统凭据已配置为 K8s Secret
- [ ] IDQL 策略已在测试环境验证
- [ ] 策略同步已确认生效
- [ ] 回滚方案已准备（策略删除的影响）

## Related

- [[03-istio-security-hardening]] — Istio 安全加固
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[paralus]] — Paralus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hexa
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference


<!-- risk-assessed -->
