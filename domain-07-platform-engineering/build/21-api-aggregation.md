---
title: 32 - API聚合层配置
description: '# 32 - API聚合层配置'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- apiserver
- prometheus
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API聚合层配置 是什么
- 如何 API聚合层配置
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- API聚合层配置
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: domain
  path: ../domain-15-specialized-tech/
  label: '相关知识域: domain-15-specialized-tech'
- type: domain
  path: ../domain-10-troubleshooting-diagnostics/
  label: '相关知识域: domain-10-troubleshooting-diagnostics'
created: "2026-05-23"
---

# 32 - API聚合层配置

<!-- chunk: API聚合架构 -->
## API聚合架构

| 组件 | 功能 | 说明 |
|-----|------|------|
| kube-aggregator | API路由 | 内置于kube-apiserver |
| APIService | 服务注册 | 声明API组/版本 |
| Extension API Server | 扩展服务器 | 自定义API实现 |

<!-- chunk: APIService配置 -->
## APIService配置

| 字段 | 类型 | 说明 |
|-----|-----|------|
| `spec.group` | string | API组名 |
| `spec.version` | string | API版本 |
| `spec.[[Service|service]].name` | string | 后端服务名 |
| `spec.service.namespace` | string | 后端服务命名空间 |
| `spec.service.port` | int | 后端服务端口(默认443) |
| `spec.caBundle` | []byte | CA证书 |
| `spec.groupPriorityMinimum` | int | 组优先级最小值 |
| `spec.versionPriority` | int | 版本优先级 |
| `spec.insecureSkipTLSVerify` | bool | 跳过TLS验证(不推荐) |

<!-- chunk: APIService示例 -->
## APIService示例

```yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io
spec:
  service:
    name: metrics-server
    namespace: kube-system
    port: 443
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 100
  versionPriority: 100
  caBundle: <base64-encoded-ca>
---
# 本地APIService(内置API)
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1.
spec:
  group: ""
  version: v1
  groupPriorityMinimum: 18000
  versionPriority: 1
  # 无service字段表示由kube-apiserver本地处理
```

<!-- chunk: 内置聚合API -->
## 内置聚合API

| APIService | 服务 | 功能 |
|-----------|------|------|
| `v1beta1.metrics.k8s.io` | metrics-server | 资源指标 |
| `v1.custom.metrics.k8s.io` | prometheus-adapter | 自定义指标 |
| `v1beta1.external.metrics.k8s.io` | - | 外部指标 |

<!-- chunk: Extension API Server开发 -->
## Extension API Server开发

| 步骤 | 说明 |
|-----|------|
| 1. 实现API处理 | REST handler |
| 2. 配置TLS | 服务器证书 |
| 3. 部署服务 | Deployment+Service |
| 4. 创建APIService | 注册到聚合层 |
| 5. 配置RBAC | 授权访问 |

<!-- chunk: Extension Server示例结构 -->
## Extension Server示例结构

```go
// 使用apiserver-builder或自定义实现
package main

import (
    "k8s.io/apiserver/pkg/server"
    "k8s.io/apiserver/pkg/server/options"
)

func main() {
    // 配置TLS
    opts := options.NewSecureServingOptions()
    
    // 注册API处理器
    apiGroupInfo := server.NewDefaultAPIGroupInfo(...)
    
    // 启动服务器
    server.PrepareRun().Run(stopCh)
}
```

<!-- chunk: 认证代理配置 -->
## 认证代理配置

| 参数 | 说明 |
|-----|------|
| `--requestheader-client-ca-file` | 代理客户端CA |
| `--requestheader-allowed-names` | 允许的CN |
| `--requestheader-extra-headers-prefix` | 额外头前缀 |
| `--requestheader-group-headers` | 组头名称 |
| `--requestheader-username-headers` | 用户名头名称 |
| `--proxy-client-cert-file` | 代理客户端证书 |
| `--proxy-client-key-file` | 代理客户端密钥 |

<!-- chunk: 聚合层故障排查 -->
## 聚合层故障排查

| 问题 | 诊断命令 | 解决方案 |
|-----|---------|---------|
| APIService不可用 | `kubectl get apiservices` | 检查后端服务 |
| 证书问题 | `kubectl describe apiservice` | 更新caBundle |
| 网络不通 | `kubectl logs kube-apiserver` | 检查服务连通性 |
| 权限不足 | `kubectl auth can-i` | 配置RBAC |

<!-- chunk: 状态检查命令 -->
## 状态检查命令

```bash
# 查看所有APIService
kubectl get apiservices

# 查看聚合API状态
kubectl get apiservices v1beta1.metrics.k8s.io -o yaml

# 检查可用性
kubectl api-resources --api-group=metrics.k8s.io

# 测试API
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
```

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | APIService状态条件改进 |
| v1.27 | 聚合发现API改进 |
| v1.28 | API优先级和公平性增强 |
| v1.29 | 聚合层性能优化 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-07-platform-engineering KUDIG Database — Global MOC
- [[domain-07-platform-engineering/README|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- index.md|Domain-9 平台运维 — 开源项目索引]]
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 19-lease-leader-election
- 20-crd-operator-development
- 22-client-libraries
- 23-cli-enhancement-tools
