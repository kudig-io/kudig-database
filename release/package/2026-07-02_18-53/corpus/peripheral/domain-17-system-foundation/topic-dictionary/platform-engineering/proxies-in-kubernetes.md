---
title: Kubernetes 中的代理
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- ingress
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 中的代理 是什么
- 如何 Kubernetes 中的代理
trigger_keywords:
- Kubernetes
- 中的代理
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 中的代理

## 概述

在 Kubernetes 中，用户和集群管理员可能会遇到多种不同类型的代理。理解这些代理的区别和用途，对于正确访问集群服务、调试网络问题以及设计集群架构非常重要。

## 核心概念/原理

Kubernetes 中有五种主要的代理类型：

1. **kubectl proxy**
2. **apiserver proxy**
3. **kube-proxy**
4. **apiserver 前端的代理/负载均衡器**
5. **外部服务的云负载均衡器**

普通 Kubernetes 用户通常只需关注前两种类型，而集群管理员则需要确保后几种类型的正确配置。

## 关键机制或特性

### 1. kubectl proxy

- 运行在用户的桌面或某个 Pod 中。
- 将本地 localhost 地址代理到 Kubernetes apiserver。
- 客户端到代理使用 HTTP。
- 代理到 apiserver 使用 HTTPS。
- 自动定位 apiserver 并添加认证头。
- 常用于本地安全访问 [[domain-17-system-foundation/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]。

### 2. apiserver proxy

- 内置于 apiserver 中的堡垒代理。
- 将集群外部的用户连接到集群内部可能无法直接访问的 Cluster IP。
- 运行在 apiserver 进程内部。
- 客户端到代理使用 HTTPS（如果 apiserver 允许，也可以使用 HTTP）。
- 代理到目标可能使用 HTTP 或 HTTPS，由代理根据可用信息自动选择。
- 可用于访问 Node、Pod 或 [[Service|Service]]，访问 Service 时会进行负载均衡。

### 3. kube-proxy

- 运行在每个节点上。
- 代理 UDP、TCP 和 SCTP 流量。
- 不理解 HTTP 协议。
- 提供负载均衡功能。
- **仅用于访问 Service**。
- 通过 iptables/IPVS/nftables 等机制实现 Service 的网络转发。

### 4. apiserver 前端的代理/负载均衡器

- 存在和实现方式因集群而异（如 nginx、云厂商负载均衡器）。
- 位于所有客户端和一个或多个 apiservers 之间。
- 当有多个 apiserver 时，充当负载均衡器。
- 用于提供高可用性和外部访问入口。

### 5. 外部服务的云负载均衡器

- 由部分云提供商提供（如 AWS ELB、Google Cloud Load Balancer）。
- 当 Kubernetes Service 的类型为 `LoadBalancer` 时自动创建。
- 通常仅支持 UDP/TCP。
- SCTP 支持取决于云提供商的负载均衡器实现。
- 具体实现因云提供商而异。

### 请求重定向

代理已经取代了重定向（redirect）功能。重定向已被弃用。

## 使用场景

- **本地 API 调试**：使用 `kubectl proxy` 安全地访问 Kubernetes API，无需直接处理证书和认证。
- **访问集群内部资源**：通过 `apiserver proxy` 从外部访问特定的 Pod、Service 或节点端口，用于调试和测试。
- **Service 负载均衡**：`kube-proxy` 为集群内部和外部的 Service 访问提供透明的负载均衡。
- **高可用控制平面**：在多个 apiserver 实例前部署负载均衡器，提供故障转移和流量分发。
- **暴露服务到公网**：使用云负载均衡器将集群内的 Service 暴露到互联网。

## 最佳实践/注意事项

- 普通用户日常使用中主要接触 `kubectl proxy` 和 `apiserver proxy`，应理解两者的安全模型和适用场景。
- `kube-proxy` 工作在四层（UDP/TCP/SCTP），不解析 HTTP，因此不适用于基于 HTTP 内容的路由。
- 设计集群入口时，明确区分 `apiserver` 前端的负载均衡器（面向控制平面）与云负载均衡器（面向工作负载 Service）。
- 使用 `apiserver proxy` 访问 Service 时，代理会自动进行负载均衡，但不会保留源 IP（取决于具体配置）。
- 重定向功能已被弃用，新的设计和实现应优先使用代理机制。

## 代理类型对比矩阵

| 代理类型 | 运行位置 | 协议层 | 认证处理 | 负载均衡 | 典型用途 |
|---------|---------|--------|---------|---------|---------|
| kubectl proxy | 本地/Pod | L7 (HTTP→HTTPS) | 自动添加 | 无 | 本地调试 API |
| apiserver proxy | apiserver 进程 | L7 | 内置 | Service 级 | 访问内部资源 |
| kube-proxy | 每个节点 | L4 (TCP/UDP/SCTP) | 无 | iptables/IPVS | Service 流量转发 |
| apiserver 前端 LB | 控制平面前端 | L4/L7 | 取决于实现 | 多 apiserver | 控制平面高可用 |
| 云负载均衡器 | 云提供商 | L4 (TCP/UDP) | 取决于实现 | 外部流量 | 暴露 Service 到公网 |

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| `kubectl proxy` 启动后无法连接 | 端口被占用或 kubeconfig 错误 | `lsof -i :8001` 检查端口；`kubectl cluster-info` 验证连接 |
| apiserver proxy 返回 502/503 | 目标 Pod/Service 不可达 | `kubectl get endpoints <svc>` 检查后端；`kubectl logs` 查看目标 Pod |
| Service 无法访问（kube-proxy） | kube-proxy 未运行或 iptables 规则异常 | `kubectl -n kube-system get [[Pods|pods]] -l [[entities/kubernetes.md|[[Kubernetes 生产环境速查卡|k8s]]]]-app=kube-proxy`；`iptables-save | grep <svc-name>` |
| 外部无法访问 LoadBalancer Service | 云 LB 未就绪或安全组限制 | `kubectl get svc <name>` 检查 EXTERNAL-IP；云控制台检查 LB 状态和安全组 |
| apiserver 间歇性不可达 | 前端 LB 健康检查失败 | 检查 LB 目标组健康状态；`kubectl get componentstatuses` |
| kube-proxy 模式不匹配 | 期望 IPVS 但回退到 iptables | `kubectl -n kube-system logs <kube-proxy-pod> | grep "Using"` |

## 生产检查清单

- [ ] kube-proxy 模式已确认（iptables/IPVS/nftables），大规模集群使用 IPVS
- [ ] apiserver 前端负载均衡器配置健康检查（`/healthz` 或 `/readyz`）
- [ ] 多 apiserver 实例部署，前端 LB 支持故障转移
- [ ] LoadBalancer Service 的安全组/防火墙规则已收紧
- [ ] kube-proxy 的 `--conntrack-max-per-core` 和 `--conntrack-min` 参数根据节点规模调优
- [ ] `kubectl proxy` 仅用于开发调试，禁止在生产环境长期运行
- [ ] apiserver proxy 访问日志已启用，用于审计
- [ ] 监控 kube-proxy 的 `kubeproxy_sync_proxy_rules_duration_seconds` 指标

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启动 kubectl proxy（默认 8001 端口）
kubectl proxy --port=8001

# 通过 apiserver proxy 访问 Service
kubectl get --raw "/api/v1/namespaces/<ns>/services/<svc>/proxy/"

# 通过 apiserver proxy 访问 Pod
kubectl get --raw "/api/v1/namespaces/<ns>/pods/<pod>/proxy/"

# 检查 kube-proxy 运行状态
kubectl -n kube-system get pods -l k8s-app=kube-proxy

# 查看 kube-proxy 配置
kubectl -n kube-system get configmap kube-proxy -o yaml

# 检查 kube-proxy 模式（iptables/ipvs）
kubectl -n kube-system logs -l k8s-app=kube-proxy | grep -i "using"

# 查看 IPVS 规则（需 SSH 到节点）
ipvsadm -Ln

# 查看 iptables Service 规则（需 SSH 到节点）
iptables-save | grep -c KUBE-SVC

# 检查 Service 的 Endpoints
kubectl get endpoints <service-name>

# 检查 LoadBalancer Service 外部 IP
kubectl get svc -A --field-selector spec.type=LoadBalancer
```
## 交叉引用

- [network-plugins.md](./network-plugins.md) — CNI 插件与 kube-proxy 的协作
- [compute-storage-and-networking-extensions.md](./compute-storage-and-networking-extensions.md) — 网络扩展机制
- [api-priority-and-fairness.md](./api-priority-and-fairness.md) — apiserver 流量控制
- [../networking/service.md](../networking/service.md) — Service 类型与 kube-proxy 转发
- [../networking/ingress.md](../networking/ingress.md) — L7 入口流量管理

## 参考链接

- [Proxies in Kubernetes - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/proxies/)

## Related

- [[domain-17-system-foundation/知识字典/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[domain-17-system-foundation/知识字典/platform-engineering/api-group.md|API 组]]
- [[domain-17-system-foundation/知识字典/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]


<!-- risk-assessed -->
