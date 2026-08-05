---
title: Chaos Mesh 企业级部署
description: '# Chaos Mesh 企业级部署'
summary: '# Chaos Mesh 企业级部署'
category: domain
tags:
- chaos-mesh
- chaos-engineering
- kubernetes
- deployment
- controller-manager
- helm
- containerd
- daemonset
- rbac
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Chaos Mesh 企业级部署 是什么
- 如何 Chaos Mesh 企业级部署
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Chaos
- Mesh
- 企业级部署
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Chaos Mesh 企业级部署

## 架构组件

```
Chaos Mesh 架构:
├── chaos-operator-manager: 管理 CRD 和控制器生命周期
├── chaos-daemon: DaemonSet，在每个节点上执行实际故障注入
├── chaos-dashboard: Web UI 和 API 服务
└── chaos-mesh-controller-manager: 核心控制器
```

## [[Helm|Helm]] 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm repo
helm repo add chaos-mesh https://charts.chaos-mesh.org
helm repo update

# 安装（生产环境配置）
helm install chaos-mesh chaos-mesh/chaos-mesh \
  --namespace chaos-mesh \
  --create-namespace \
  --set chaosDaemon.runtime=containerd \
  --set chaosDaemon.socketPath=/run/containerd/containerd.sock \
  --set dashboard.securityMode=true \
  --set controllerManager.enableFilterNamespace=true
```
## 安全加固

```yaml
# 启用 RBAC 和多租户
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: chaos-experimenter
rules:
- apiGroups: ["chaos-mesh.org"]
  resources: ["*"]
  verbs: ["get", "list", "create", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: chaos-experimenter-binding
subjects:
- kind: ServiceAccount
  name: chaos-experimenter
  namespace: default
roleRef:
  kind: Role
  name: chaos-experimenter
  apiGroup: rbac.authorization.k8s.io
```

## 实验类型清单

| 类型 | 说明 | 安全级别 |
|------|------|---------|
| PodChaos | Pod 问题/终止/容器重启 | 中 |
| NetworkChaos | 网络延迟/丢包/分区 | 高 |
| IOChaos | 文件系统 I/O 问题 | 中 |
| StressChaos | CPU/内存压力测试 | 中 |
| DNSChaos | DNS 问题 | 高 |
| TimeChaos | 时间偏移 | 低 |
| HTTPChaos | HTTP 请求/响应篡改 | 高 |
| JVMChaos | JVM 级别问题 | 中 |

## 相关

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/03-chaos-engineering/01-chaos-engineering-overview|01 chaos engineering overview]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/06-chaos-engineering/02-chaos-experiment-design|03 chaos experiment design]]

```

<!-- risk-assessed -->
