---
title: 节点端口
description: NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePor...
summary: NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePor...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点端口 是什么
- NodePort 详解
trigger_keywords:
- 节点端口
- NodePort
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点端口

> **英文名**: NodePort

## 概述

NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePort` 访问集群内部服务。

## 核心概念/原理

### 核心概念

- **端口范围**：默认 30000-32767，通过 `--service-node-port-range` 参数调整。
- **自动分配**：不指定 `nodePort` 时自动分配。
- **流量路径**：`客户端 → NodeIP:NodePort → kube-proxy → ClusterIP → Pod`。

### 示例

```yaml
apiVersion: v1
kind: Service
spec:
  type: NodePort
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080
```

## 关键机制或特性

- NodePort 在所有节点上暴露相同端口。
- `externalTrafficPolicy: Local` 保留客户端源 IP。
- NodePort 是 LoadBalancer 的基础（LoadBalancer 类型自动创建 NodePort）。

## 使用场景与最佳实践

- 开发/测试环境快速暴露服务。
- 生产环境优先使用 LoadBalancer 或 Ingress。
- 注意端口冲突和安全风险（暴露节点端口到外部）。

## 参考链接

- [NodePort - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#type-nodeport)

## Related

- [[系统基础/知识字典/networking/service.md|Service]]
- [[系统基础/知识字典/networking/ingress.md|Ingress]]
- [[系统基础/知识字典/networking/clusterip.md|Clusterip]]
- [[系统基础/知识字典/networking/loadbalancer.md|Loadbalancer]]
- [[系统基础/知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
