---
title: "Headless Service"
category: concepts
tags: ["core-concept", "domain-03-networking-traffic", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
---

# Headless Service

Headless Service 是 Kubernetes 中一种特殊的 Service 类型，其核心特征是将 `spec.clusterIP` 显式设置为 `"None"`，从而不分配虚拟 ClusterIP。

## 核心定义

在普通 Service 中，kube-proxy 通过 ClusterIP 提供负载均衡，请求被转发到后端 Pod。而 Headless Service 不做这层抽象，DNS 查询直接返回后端 Pod 的 IP 列表。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-headless-service
spec:
  clusterIP: "None"  # 关键配置
  selector:
    app: my-app
  ports:
    - port: 80
```

## 适用场景

Headless Service 主要适用于以下场景：

- **StatefulSet 的有序网络标识**：为每个 Pod 提供稳定的、可预测的 DNS 名称
- **直接访问 Pod IP**：客户端需要知道所有后端 Pod 的具体地址，自行决定连接策略
- **自定义服务发现**：应用层需要实现特定的负载均衡或分片逻辑
- **数据库集群**：如 MongoDB、Cassandra、Redis Cluster 等需要节点间直接通信的分布式系统

## DNS 解析行为

与普通 Service 不同，Headless Service 的 DNS 解析返回所有符合条件的后端 Pod IP 的 A 记录：

```
dig my-headless-service.default.svc.cluster.local
# 返回所有 Pod IP，而非单一的 ClusterIP
```

对于设置了 `publishNotReadyAddresses: true` 的 Headless Service，DNS 甚至会包含未就绪的 Pod，适用于需要提前发现所有成员的集群初始化场景。

## StatefulSet + Headless Service

这是 Headless Service 最典型的组合。StatefulSet 配合 Headless Service 为每个 Pod 提供**稳定的网络标识**：

```
<pod-name>.<service-name>.<namespace>.svc.cluster.local
```

例如，名为 `web-0` 的 Pod 在 `default` 命名空间中，通过 Headless Service `web-svc` 可被解析为：

```
web-0.web-svc.default.svc.cluster.local
```

这种命名稳定性对于有状态应用至关重要，即使 Pod 被重新调度到不同节点，其 DNS 名称保持不变。

## 与普通 Service 的对比

| 特性 | 普通 Service | Headless Service |
|------|-------------|------------------|
| ClusterIP | 自动分配 | `None` |
| DNS 解析 | 返回 ClusterIP | 返回 Pod IP 列表 |
| 负载均衡 | kube-proxy 负责 | 客户端自行处理 |
| 适用工作负载 | 无状态 Deployment | 有状态 StatefulSet |

## 远程顾问诊断要点

StatefulSet Pod 域名解析失败时，应按以下顺序排查：

- **确认 Headless Service 已创建**：检查与 StatefulSet 同名的 Service 是否存在，且 `clusterIP` 为 `"None"`
- **检查 Service selector**：确认 Service 的 `selector` 与 StatefulSet Pod 的标签匹配
- **检查 DNS 配置**：确认 CoreDNS 正常运行，`/etc/resolv.conf` 中的搜索域包含 `svc.cluster.local`
- **验证 DNS 解析**：在 Pod 内执行 `nslookup <pod-name>.<service-name>` 测试解析结果
- **检查网络策略**：确认没有 NetworkPolicy 阻止 DNS 查询或 Pod 间通信

更多有状态应用排错方法请参考 [[statefulset-troubleshooting]]，服务发现相关内容参见 [[service-discovery]]。
