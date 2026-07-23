---
title: kube-proxy
description: kube-proxy — Kubernetes 生产运维知识库
summary: kube-proxy 运行在每个节点上，负责实现 Kubernetes Service 的虚拟 IP 负载均衡和后端 Pod 流量转发。
category: entities
tags:
- k8s
- kube-proxy
- control-plane
- service
- network
- iptables
- ipvs
- nftables
- cni
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-proxy 是什么
- 如何 kube-proxy
trigger_keywords:
- kube-proxy
prerequisites:
- kubectl-basics
- kubernetes-concepts
- networking-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-proxy

## Role

kube-proxy runs on every node and is responsible for implementing Kubernetes Service abstraction:
- Maintaining network rules for Service virtual IPs (ClusterIP, NodePort, LoadBalancer, ExternalName)
- Forwarding traffic destined for a Service to healthy backend Pods
- Watching API Server for Service and EndpointSlice changes

It operates at OSI Layer 3/4 and works alongside the CNI plugin, which provides Pod-to-Pod L3 connectivity.

## Proxy Modes

| Mode | Mechanism | Pros | Cons |
|------|-----------|------|------|
| **iptables** (default) | iptables DNAT rules | Simple, well supported | O(n) rules, slow for large services, no graceful termination |
| **ipvs** | Linux IPVS virtual server | O(1) lookup, better performance, more algorithms | Requires kernel modules, slightly more complex |
| **nftables** | nftables rules | Modern kernel API, unified rules | Feature gate in K8s 1.31+, newer ecosystem |
| **kernelspace** (Windows) | Windows kernel virtual filtering | Windows support | Windows only |

## Key Configuration

| Parameter | Purpose | Recommended |
|-----------|---------|-------------|
| `--proxy-mode` | iptables / ipvs / nftables | `ipvs` for large clusters; `iptables` for small clusters |
| `--cluster-cidr` | Pod CIDR range | Must match cluster CIDR |
| `--healthz-bind-address` | Health endpoint bind address | `0.0.0.0:10256` |
| `--metrics-bind-address` | Metrics endpoint | `0.0.0.0:10249` |
| `--ipvs-scheduler` | IPVS scheduling algorithm | `rr` (round robin) or `lc` (least connections) |
| `--nodeport-addresses` | Limit NodePort bind addresses | Primary node IP |

## 运维操作

```bash
# 🟢 查看 kube-proxy Pod
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# 🟢 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# 🟢 查看当前代理模式
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "Using"

# 🟢 查看 Service 后端规则（iptables 模式）
iptables -t nat -L KUBE-SERVICES -n | grep <service-cluster-ip>

# 🟢 查看 IPVS 规则（ipvs 模式）
ipvsadm -Ln

# 🟢 查看 kube-proxy 指标
curl -s http://<node>:10249/metrics | grep kubeproxy

# 🟡 修改代理模式（通过 kube-proxy ConfigMap）
kubectl get cm kube-proxy -n kube-system -o yaml | grep mode
# 编辑后滚动重启 DaemonSet
kubectl rollout restart daemonset kube-proxy -n kube-system
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Service ClusterIP 不通 | kube-proxy 未运行 / 规则未下发 | `kubectl get pods -n kube-system -l k8s-app=kube-proxy` | 重启 kube-proxy Pod |
| 流量只到部分 Pod | EndpointSlice 未同步 / Pod 未 Ready | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` | 检查 Pod 状态 / kube-proxy 日志 |
| NodePort 外部访问失败 | NodePort 未监听 / 防火墙 | `ss -tlnp | grep <nodeport>` | 检查 kube-proxy 与防火墙规则 |
| IPVS 连接数不均 | 调度算法不合适 | `ipvsadm -Ln --stats` | 调整 `--ipvs-scheduler` |
| conntrack 表满 | 短连接风暴 | `conntrack -L | wc -l` | 增大 nf_conntrack_max / 使用 IPVS |
| 大规模服务性能差 | iptables 规则过多 | `iptables -t nat -L KUBE-SERVICES --line-numbers | wc -l` | 切换到 ipvs/nftables 模式 |

```bash
# 排查流程
# 1. 检查 kube-proxy 健康
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
curl http://<node>:10256/healthz

# 2. 检查 Service 与 EndpointSlice
kubectl get svc <svc>
kubectl get endpointslices -l kubernetes.io/service-name=<svc>

# 3. 检查转发规则
iptables -t nat -L KUBE-SERVICES -n | grep <cluster-ip>
# 或
ipvsadm -Ln

# 4. 检查 conntrack
conntrack -L | wc -l
sysctl net.netfilter.nf_conntrack_max

# 5. 查看 kube-proxy 日志
kubectl logs -n kube-system <kube-proxy-pod> | grep -i error
```

## 生产案例

### 案例1：conntrack 表满导致 Service 间歇性超时
- **场景**：高并发短连接服务， periodically 出现 5s 超时
- **排查**：`conntrack -L | wc -l` 接近 `nf_conntrack_max`；`dmesg` 出现 `nf_conntrack: table full`
- **方案**：将代理模式从 iptables 切换到 ipvs；提升 `net.netfilter.nf_conntrack_max`；缩短 `tcp_fin_timeout`
- **效果**：超时率从 0.5% 降至 0%

### 案例2：EndpointSlice 未同步导致流量黑洞
- **场景**：缩容后部分请求仍发送到已删除 Pod
- **排查**：`kubectl get endpointslices` 显示 stale endpoints；kube-proxy 日志显示 API Server watch 重连
- **方案**：检查 API Server 与 kube-proxy 网络；调整 kube-proxy 的 `--config-sync-period`；重启异常 kube-proxy Pod
- **效果**：EndpointSlice 同步恢复，无 stale 后端

## 检查清单

- [ ] kube-proxy DaemonSet 所有 Pod 正常 Running
- [ ] 代理模式符合集群规模（ipvs 推荐大规模）
- [ ] conntrack 表大小已根据规模调整
- [ ] Service / EndpointSlice 规则在所有节点一致
- [ ] kube-proxy 指标已接入 Prometheus
- [ ] 内核 IPVS 模块已加载（ipvs 模式）
- [ ] NodePort 端口范围与防火墙策略匹配

## Related

- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[实体/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI
- [[service]] — Service
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[集群基础/控制平面/16-kube-proxy-deep-dive.md|kube-proxy 深度解析]]
- [[故障诊断/高级排障/structural-02-node-components/02-kube-proxy-troubleshooting.md|kube-proxy 故障排查指南]]


<!-- risk-assessed -->
