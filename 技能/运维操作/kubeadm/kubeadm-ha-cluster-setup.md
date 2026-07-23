---
title: kubeadm 高可用集群搭建
description: '## 概述'
summary: '生产环境 Kubernetes 集群必须部署高可用控制面以消除单点问题。kubeadm 支持两种 etcd 高可用拓扑，并通过 `--control-plane-endpoint` 参数统一配置负载均衡入口。'
category: skills
tags:
- k8s
- kubeadm
- high-availability
- stacked-etcd
- external-etcd
- kube-vip
- leader-election
- load-balancer
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 高可用集群搭建 是什么
- 如何 kubeadm 高可用集群搭建
trigger_keywords:
- kubeadm
- 高可用集群搭建
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubeadm 高可用集群搭建

## 概述

生产环境 Kubernetes 集群必须部署高可用控制面以消除单点问题。kubeadm 支持两种 etcd 高可用拓扑，并通过 `--control-plane-endpoint` 参数统一配置负载均衡入口。

## 两种 etcd 拓扑对比

| 维度 | Stacked etcd（堆叠模式） | External etcd（外部模式） |
|------|------------------------|-------------------------|
| etcd 位置 | 与控制面组件同节点 | 独立节点 |
| 部署复杂度 | 低（kubeadm 原生支持） | 高（需独立维护 etcd） |
| 资源隔离 | 共享 | 完全隔离 |
| 性能 | 受控制面负载影响 | 更稳定 |
| 适用规模 | 中小型集群 | 大型生产集群 |
| 最小节点数 | 3（奇数保证 etcd 仲裁） | 3 CP + 3 etcd |

## Stacked etcd 架构

```
负载均衡器 (HAProxy/[[实体/kube-vip.md|kube-vip]]/云厂商 CLB)
control-plane-endpoint:6443
       │              │              │
  ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
  │ CP 节点 1 │   │ CP 节点 2 │   │ CP 节点 3 │
  │ apiserver│   │ apiserver│   │ apiserver│
  │ scheduler│   │ scheduler│   │ scheduler│
  │ ctrl-mgr │   │ ctrl-mgr │   │ ctrl-mgr │
  │ etcd     │◄─►│ etcd     │◄─►│ etcd     │
  └─────────┘   └─────────┘   └─────────┘
```

## 核心配置

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
controlPlaneEndpoint: "loadbalancer.example.com:6443"
etcd:
  local:
    dataDir: /var/lib/etcd
networking:
  podSubnet: "10.244.0.0/16"
  serviceSubnet: "10.96.0.0/12"
kubernetesVersion: "v1.32.0"
```

## 负载均衡器要求

| 要求 | 说明 |
|------|------|
| TCP 6443 四层负载均衡 | API Server 使用 HTTPS |
| TLS Passthrough | 透传 TLS 到后端，不解密 |
| 健康检查 | TCP 或 HTTPS 检查，自动剔除不健康后端 |
| 无需会话保持 | API Server 是无状态的 |

常用方案：kube-vip（推荐）、HAProxy + Keepalived、云厂商 CLB、nginx stream proxy

## 新增控制面节点流程

```bash
# 1. 在第一个控制面节点上传证书
kubeadm init phase upload-certs --upload-certs
# 输出 certificate-key: xxxxxxxx

# 2. 在新控制面节点上执行 join
kubeadm join control-plane-endpoint:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <key>
```

### 证书分发流程

新控制面节点加入时：

1. 从 ConfigMap 下载加密证书（`kubeadm-certs`）
2. 使用 certificate-key（AES-256-GCM）解密证书
3. 将证书写入 `/etc/kubernetes/pki/`
4. 生成该节点特有的证书（含本节点 IP 的 SAN）
5. 向 etcd 集群添加新成员
6. 启动静态 Pod

## Leader Election 机制

Controller Manager 和 Scheduler 通过内置的 Leader Election 实现 HA：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--leader-elect` | `true` | 启用选主 |
| `--leader-elect-lease-duration` | 15s | 租约时长 |
| `--leader-elect-renew-deadline` | 10s | 续约超时 |
| `--leader-elect-retry-period` | 2s | 重试间隔 |

查看当前 Leader：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get lease -n kube-system
kubectl describe lease kube-controller-manager -n kube-system
kubectl describe lease kube-scheduler -n kube-system
```
## etcd 仲裁规则

| 节点数 | 容忍问题数 | 说明 |
|--------|-----------|------|
| 1 | 0 | 开发环境 |
| 3 | 1 | 小型生产 |
| 5 | 2 | 大型生产 |

超过 5 个节点会降低写入性能（Raft 复制开销增加）。

## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| etcd 成员添加失败 | 新节点 IP 不在证书 SAN 中 | 重新生成 etcd 证书 |
| API Server 连不上 | 负载均衡器未配置 TLS passthrough | 配置透传模式 |
| 证书解密失败 | certificate-key 错误 | 重新 `upload-certs` |
| join 超时 | API Server 未就绪 | 等待 API Server 启动 |

## 相关技能

- [[技能/kubeadm-cluster-lifecycle.md|[[kubeadm 集群创建生命周期|kubeadm 集群创建生命周期]]]]
- [[概念/high-availability-patterns.md|高可用模式]]
- [[技能/kubeadm-cluster-deletion.md|[[kubeadm 集群删除操作|kubeadm 集群删除操作]]]]
- [[etcd|etcd]]
- [[实体/kube-apiserver.md|kube-apiserver]]

## 生产案例

### 案例 1: HA 集群 etcd 集群脑裂

| 时间 | 事件 |
|------|------|
| 03:00 | 网络分区导致 etcd 集群失去多数派 |
| 03:01 | API Server 报错 "etcd cluster is unavailable" |
| 03:05 | 2/3 etcd 节点在同一分区，1/3 在另一分区 |
| 03:10 | 🔴 恢复网络连通性，etcd 自动重新选举 |
| 03:15 | 集群恢复 |

**根因**: 交换机故障导致网络分区，etcd 少数派无法提供服务。

### 案例 2: LB VIP 故障导致控制平面不可达

**现象**: 所有 kubectl 命令超时，但控制平面 Pod 正常。

**诊断**: 前置 HAProxy/Keepalived VIP 未漂移

**修复**: 🟡 重启 keepalived 或切换到备用 LB

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 控制平面完全不可用 | 检查 LB + etcd 集群 |
| P1 | 单控制平面节点故障 | 确认 LB 健康检查正常剔除 |
| P2 | etcd 性能下降 | 检查磁盘 IOPS |

## 面试要点

1. **Q: kubeadm HA 集群的架构设计？**
   A: 3/5 个控制平面节点 + 前置 LB(VIP/HAProxy/云 LB) + Stacked/External etcd。LB 健康检查 :6443/healthz，故障节点自动剔除。

2. **Q: etcd 集群的容错能力？**
   A: N 个 etcd 节点容忍 (N-1)/2 个故障。3 节点容忍 1，5 节点容忍 2。必须奇数节点避免脑裂。生产推荐 5 节点或 3 节点 + 定期备份。

3. **Q: 控制平面 LB 的方案选择？**
   A: ① 云 LB(SLB/ELB): 最简单，云托管 ② HAProxy+Keepalived: 自建，灵活 ③ kube-vip: 云原生，ARP/BGP VIP ④ Nginx: 简单但功能有限。生产推荐云 LB 或 kube-vip。

## Related

- [[技能/k8s-cluster-configuration-guide.md|k8s-cluster-configuration-guide]] — Kubernetes 集群配置最佳实践
- [[实体/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[kube-vip]] — kube-vip
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
