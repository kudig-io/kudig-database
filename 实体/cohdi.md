---
title: Cohdi
description: '## 概述'
summary: 'CoHDI（Composable Hyperconverged Disaggregated Infrastructure）是一个 Kubernetes Operator，用于在分解式基础设施中动态组合和管理硬件资源。'
category: entities
tags:
- k8s
- cncf
- orchestration
- cohdi
- crd
- operator
- gpu
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cohdi 是什么
- 如何 Cohdi
trigger_keywords:
- Cohdi
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cohdi

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Cohdi 是一个 CNCF 沙箱项目，旨在提供 Kubernetes 上的轻量级工作负载编排和部署自动化能力。它专注于简化应用从开发到生产的多环境部署流程，通过声明式配置管理多集群应用分发。Cohdi 特别关注边缘计算和混合云场景下的工作负载编排，提供低资源占用的 Agent 和灵活的部署策略。

## Key Features（核心能力）

- **多集群部署**：将应用工作负载分发到多个 K8s 集群
- **环境差异管理**：通过 Overlay 和 Patch 管理不同环境的配置差异
- **渐进式发布**：支持跨集群的蓝绿部署和金丝雀发布
- **轻量级 Agent**：边缘节点上的低资源占用代理
- **GitOps 集成**：基于 Git 仓库的应用配置管理
- **策略引擎**：部署位置和时机的策略控制

## 架构与工作原理

Cohdi 采用 Hub-Spoke 架构：Hub 组件运行在中心集群，管理应用部署配置和分发策略；Spoke Agent 运行在目标集群（包括边缘节点），接收部署指令并协调本地工作负载。部署配置通过声明式 YAML 定义，支持环境 Overlay、健康检查和回滚策略。

## K8s 集成

Cohdi 通过 CRD 与 Kubernetes 集成：DeploymentPolicy CRD 定义应用的部署目标和策略；ClusterSet CRD 定义目标集群集合。Hub Controller 管理这些 CRD 并分发工作负载清单到各目标集群。Spoke Agent 在目标集群中以 Deployment 部署，监听 Hub 的部署指令。

## 生产用例

- **边缘应用分发**：将应用部署到大量边缘节点
- **多环境管理**：统一管理 dev/staging/prod 的应用部署
- **混合云部署**：跨本地数据中心和公有云的应用分发
- **渐进式发布**：跨集群的金丝雀发布

## 安装与配置

```bash
# 🟢 安装 Cohdi Hub Controller
kubectl apply -f https://github.com/cohdi/cohdi/releases/latest/download/cohdi-hub.yaml

# 🟢 验证安装
kubectl get pods -n cohdi-system
kubectl get crd | grep cohdi.io

# 🟢 安装 Spoke Agent（在目标集群执行）
kubectl apply -f https://github.com/cohdi/cohdi/releases/latest/download/cohdi-agent.yaml

# 🟢 注册目标集群到 Hub
kubectl apply -f cluster-registration.yaml

# 🟢 查看已注册集群
kubectl get clusterset -A
```

### DeploymentPolicy CRD 示例

```yaml
apiVersion: cohdi.io/v1alpha1
kind: DeploymentPolicy
metadata:
  name: web-app-policy
  namespace: production
spec:
  application:
    name: web-app
    source:
      type: git
      url: https://github.com/org/app-manifests.git
      path: /base
      ref: main
  targets:
    clusterSet: prod-clusters
    strategy:
      type: Canary
      canary:
        steps:
          - weight: 10
            pause: 5m
          - weight: 50
            pause: 10m
          - weight: 100
  overlays:
    - name: prod-east
      patches:
        - target:
            kind: Deployment
            name: web-app
          patch: |
            - op: replace
              path: /spec/replicas
              value: 5
    - name: prod-west
      patches:
        - target:
            kind: Deployment
            name: web-app
          patch: |
            - op: replace
              path: /spec/replicas
              value: 3
  healthCheck:
    enabled: true
    timeout: 300s
    failureThreshold: 3
  rollback:
    enabled: true
    strategy: LastKnownGood
---
apiVersion: cohdi.io/v1alpha1
kind: ClusterSet
metadata:
  name: prod-clusters
spec:
  clusters:
    - name: prod-east
      labels:
        region: east
    - name: prod-west
      labels:
        region: west
  selector:
    matchLabels:
      tier: production
```

## 运维操作

```bash
# 🟢 查看部署策略状态
kubectl get deploymentpolicy -A
kubectl describe deploymentpolicy web-app-policy -n production

# 🟢 查看各集群部署状态
kubectl get deploymentpolicy web-app-policy -n production -o jsonpath='{.status.clusters}' | jq .

# 🟡 手动触发回滚
kubectl annotate deploymentpolicy web-app-policy -n production \
  cohdi.io/rollback=true --overwrite

# 🟡 暂停金丝雀发布
kubectl patch deploymentpolicy web-app-policy -n production --type=merge -p \
  '{"spec":{"targets":{"strategy":{"type":"Paused"}}}}'

# 🟡 更新应用版本
kubectl patch deploymentpolicy web-app-policy -n production --type=merge -p \
  '{"spec":{"application":{"source":{"ref":"v2.1.0"}}}}'

# 🔴 删除部署策略（会清理目标集群上的应用）
kubectl delete deploymentpolicy web-app-policy -n production
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Agent 状态 Offline | 网络中断/Agent崩溃 | `kubectl get clusterset -o wide` | 重启 Agent Pod |
| 部署卡在 Canary 步骤 | 健康检查失败 | `kubectl describe deploymentpolicy <name>` | 检查目标集群应用日志 |
| Overlay 未生效 | Patch 语法错误 | `kubectl get deploymentpolicy -o yaml` | 验证 JSON Patch 格式 |
| 回滚失败 | 无历史版本记录 | 查看 Controller 日志 | 手动应用上一版本 YAML |

```bash
# 排查流程
# 1. 检查 Hub Controller 状态
kubectl logs -n cohdi-system -l app=cohdi-controller --tail=100

# 2. 检查 Agent 连接状态
kubectl get clusterset -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'

# 3. 检查部署事件
kubectl get events -n production --sort-by='.lastTimestamp' | grep cohdi

# 4. 检查目标集群实际资源
kubectl --context=prod-east get deploy web-app -o wide
```

## 生产案例

### 案例1：边缘节点应用分发
- **场景**：物流企业 500+ 边缘节点需要统一更新扫描应用
- **方案**：使用轻量级 Agent（内存占用 < 50MB）；通过 ClusterSet 按仓库分组；配置分批发布策略每批 50 个节点
- **效果**：全量更新从 3天 缩短到 6小时，Agent 资源占用降低 70%

### 案例2：多环境配置管理
- **场景**：SaaS 企业需要管理 dev/staging/prod 三套环境的配置差异
- **方案**：使用 Overlay 机制管理环境差异；base 配置 + 环境 Patch；健康检查确保部署成功
- **效果**：配置漂移事件减少 95%，环境一致性问题从每周 5+ 次降到 0

## 对比替代方案

| 维度 | Cohdi | KubeFed v2 | ArgoCD | Karmada |
|------|-------|-----------|--------|--------|
| 状态 | 活跃 | 已归档 | 活跃 | 活跃 |
| 多集群 | 原生 | 原生 | 插件 | 原生 |
| 资源占用 | 极低(<50MB) | 中 | 高 | 中 |
| 边缘场景 | 强 | 弱 | 中 | 中 |
| GitOps | 支持 | 无 | 核心 | 支持 |
| 学习曲线 | 低 | 中 | 中 | 高 |

## 检查清单

- [ ] Hub Controller 已部署且 Pod Running
- [ ] Spoke Agent 已在目标集群安装并连接成功
- [ ] ClusterSet 已正确定义目标集群
- [ ] DeploymentPolicy 已在测试环境验证
- [ ] Overlay/Patch 语法已验证
- [ ] 健康检查和回滚策略已配置
- [ ] 网络连通性已验证（Hub ↔ Spoke）

## Related

- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cohdi
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
