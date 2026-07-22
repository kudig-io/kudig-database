---
title: kubeadm 集群运维全景
description: '# kubeadm 集群运维全景'
summary: '本文档综合梳理 kubeadm 集群运维的全生命周期知识，涵盖集群创建、高可用部署、证书管理、节点运维、工作负载管理和集群删除六大领域。知识来源为 工作负载/topic-functions/ 下的 82 篇源文档，'
category: synthesis
tags:
- k8s
- kubeadm
- cluster-operations
- lifecycle
- pki
- ha
- upgrade
- deletion
- etcd
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 集群运维全景 是什么
- 如何 kubeadm 集群运维全景
trigger_keywords:
- kubeadm
- 集群运维全景
prerequisites:
- kubectl-basics
- prometheus-basics
- ebpf-basics
- etcd-basics
relationships:
- target: '[[实体/kubelet.md]]'
  type: uses
- target: '[[概念/kubernetes-pki-certificate-system.md]]'
  type: uses
- target: '[[实体/deployment.md]]'
  type: uses
- target: '[[概念/etcd x 高可用模式.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubeadm 集群运维全景

## 概述

本文档综合梳理 kubeadm 集群运维的全生命周期知识，涵盖集群创建、高可用部署、证书管理、节点运维、工作负载管理和集群删除六大领域。知识来源为 工作负载/topic-functions/ 下的 82 篇源文档，覆盖 5 个子专题：cluster-cert、cluster-create、cluster-delete、[[实体/deployment.md|deployment]]-create、node-create。

## 运维全景图

```
                    ┌─────────────────────────────────────────────┐
                    │          kubeadm 集群运维全景                 │
                    └─────────────────────┬───────────────────────┘
                                          │
          ┌───────────┬──────────┬────────┼────────┬──────────┬───────────┐
          ▼           ▼          ▼        ▼        ▼          ▼           ▼
     ┌────────┐  ┌────────┐ ┌──────┐ ┌───────┐ ┌──────┐  ┌──────┐ ┌──────┐
     │ 集群创建│  │ PKI证书│ │  HA  │ │ 节点  │ │工作负载│ │ 升级 │ │ 删除 │
     │        │  │  体系  │ │ 部署 │ │ 管理  │ │ 管理 │ │      │ │      │
     └───┬────┘  └───┬────┘ └──┬───┘ └───┬───┘ └──┬───┘  └──┬───┘ └──┬───┘
         │           │          │         │         │          │        │
         │ 12个阶段  │ 3组CA    │ stacked │ 注册    │ Deployment│ drain  │ reset
         │ preflight │ 14对证书 │ external│ CSR     │ ReplicaSet│ upgrade│ cleanup
         │ init      │ 轮换     │ LB      │ 状态    │ StatefulSet│ certs │ network
         │ join      │ 外部CA   │ leader  │ drain   │ DaemonSet │        │ manual
         │           │          │ election│ eviction│           │        │
```

## 六大领域概览

### 1. 集群创建（cluster-create，25 篇）

`kubeadm init` 采用最小化引导设计，将集群创建分解为 12 个有序阶段。核心关注点：
- 预检阶段确保系统环境符合要求
- PKI 证书生成是控制面安全的基础
- kubeadm 不安装 CNI，需手动配置网络
- 支持配置文件和命令行两种参数方式

### 2. PKI 证书体系（cluster-cert，17 篇）

三组独立 CA（kubernetes-ca、etcd-ca、front-proxy-ca）签发超过 14 组证书。核心关注点：
- 证书有效期默认 1 年（CA 10 年）
- 支持外部 CA 模式满足企业安全要求
- 控制面证书通过 `kubeadm certs renew` 手动轮换
- [[实体/kubelet.md|kubelet]] 证书通过 TLS Bootstrap 自动轮换

### 3. 高可用部署（cluster-create/08-ha 等）

生产集群必须消除单点问题。核心关注点：
- stacked etcd 适合中小型集群，external etcd 适合大型集群
- 负载均衡器需支持 TCP 6443 和 TLS Passthrough
- Controller Manager 和 Scheduler 通过 Leader Election 实现热备
- 新增控制面节点需要证书分发（upload-certs）

### 4. 节点生命周期（node-create，17 篇）

节点从注册到移除涉及多个组件协作。核心关注点：
- Bootstrap Token + CSR 完成节点认证
- kubelet 通过 syncNodeStatus 循环上报状态
- Node Lifecycle Controller 监控节点健康
- drain/eviction 管理节点维护期间的 Pod 调度

### 5. 工作负载管理（deployment-create，10 篇）

Deployment 通过 ReplicaSet 间接管理 Pod。核心关注点：
- RollingUpdate 通过 maxSurge/maxUnavailable 控制发布速度
- pause/resume 实现金丝雀验证
- 双 Deployment 和 Service Selector 切换实现蓝绿发布
- StatefulSet 提供稳定身份和有序管理

### 6. 集群删除（cluster-delete，13 篇）

集群删除需要多步骤配合。核心关注点：
- `kubeadm reset` 采用 best-effort 策略
- 不会自动清理 CNI/iptables 等网络配置
- 删除前必须备份 etcd 快照和 PKI 证书
- 异常场景（节点不可达、etcd 仲裁丢失）需要手动处理

## 关键关联关系

```
集群创建 ──────► PKI 证书（certs 阶段生成）
    │                   │
    │                   ▼
    │            证书轮换（kubeadm certs renew）
    │
    ▼
高可用部署 ──────► 证书分发（upload-certs）
    │
    ▼
节点管理 ──────► CSR 自动审批 ──────► 证书轮换
    │
    ▼
工作负载 ──────► 滚动更新 ──────► 金丝雀/蓝绿
    │
    ▼
集群升级 ──────► drain 节点 ──────► 升级 kubelet
    │
    ▼
集群删除 ──────► drain → delete node → reset → 手动清理
```

## 运维最佳实践

1. **证书管理**：设置 Prometheus 告警监控证书过期，维护窗口内主动轮换
2. **升级流程**：先检查证书有效期 → 轮换证书 → 执行 upgrade
3. **HA 部署**：3 节点 stacked etcd 是最小生产配置，5 节点可容忍 2 节点问题
4. **节点维护**：始终先 drain 再操作，配置 PDB 保护关键应用
5. **发布策略**：生产环境推荐金丝雀或蓝绿发布，避免直接全量更新
6. **删除操作**：删除前务必备份 etcd 快照，reset 后手动清理网络规则

## 源码实现分析

### kubeadm init 执行流程

```go
// k8s.io/kubernetes/cmd/kubeadm/app/cmd/init.go
// kubeadm init 执行阶段
func (i *initData) Run() error {
    // Phase 1: preflight checks（系统要求检查）
    // - 内核版本、cgroup 驱动、端口占用、swap 状态
    
    // Phase 2: certs（生成 PKI 证书体系）
    // - CA 证书、apiserver 证书、kubelet 客户端证书
    // - 存储在 /etc/kubernetes/pki/
    
    // Phase 3: kubeconfig（生成各组件 kubeconfig）
    // - admin.conf / controller-manager.conf / scheduler.conf / kubelet.conf
    
    // Phase 4: control-plane（启动控制平面静态 Pod）
    // - /etc/kubernetes/manifests/ 下生成 yaml
    // - kubelet 自动拉起 apiserver/controller-manager/scheduler
    
    // Phase 5: etcd（启动 etcd 静态 Pod）
    
    // Phase 6: addon（安装 CoreDNS + kube-proxy）
}
```

```
┌─────────────────────────────────────────────────────────┐
│     kubeadm 集群生命周期操作                        │
├─────────────────────────────────────────────────────────┤
│  创建: kubeadm init → join (worker) → join (control-plane)│
│  升级: kubeadm upgrade plan → apply → node upgrade    │
│  证书: kubeadm certs check-expiration → renew all     │
│  删除: kubeadm reset → 清理 iptables/CNI/etcd        │
│                                                         │
│  关键文件:                                             │
│  /etc/kubernetes/manifests/ (静态 Pod)                │
│  /etc/kubernetes/pki/ (证书)                          │
│  /etc/kubernetes/*.conf (kubeconfig)                  │
│  /var/lib/etcd/ (etcd 数据)                          │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：kubeadm 关键操作

```bash
# 🟢 检查集群状态
kubectl get nodes -o wide
kubectl get pods -n kube-system

# 🟢 检查证书过期
kubeadm certs check-expiration

# 🟡 更新证书（控制平面节点执行）
kubeadm certs renew all
systemctl restart kubelet
# 🔴 证书更新后必须重启 kubelet，否则新证书不生效

# 🟡 升级控制平面
kubeadm upgrade plan
kubeadm upgrade apply v1.30.0
# 🔴 升级前必须备份 etcd，且只能跳一个次版本

# 🟡 升级节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
kubeadm upgrade node
systemctl restart kubelet
kubectl uncordon <node>
```

## 面试要点

1. **kubeadm init 做了哪些事情？**
   - 预检查（内核/cgroup/端口/swap）
   - 生成 PKI 证书体系（CA + apiserver + kubelet + etcd）
   - 生成 kubeconfig 文件
   - 创建控制平面静态 Pod（apiserver/cm/scheduler/etcd）
   - 安装 CoreDNS 和 kube-proxy addon

2. **kubeadm 集群证书过期如何处理？**
   - `kubeadm certs check-expiration` 检查过期时间
   - `kubeadm certs renew all` 更新所有证书
   - 更新后必须重启 kubelet 和控制平面组件
   - 生产建议：设置证书过期告警（提前 30 天）

3. **kubeadm 升级的注意事项？**
   - 只能跳一个次版本（v1.29→v1.30，不能 v1.28→v1.30）
   - 升级前必须备份 etcd
   - 先升级控制平面，再逐节点升级 kubelet
   - 使用 pluto/kube-no-trouble 扫描弃用 API

4. **kubeadm 与托管集群（EKS/GKE）的对比？**
   - kubeadm：完全控制，但需自己管理升级/HA/证书
   - 托管集群：控制平面由云厂商管理，只关心节点
   - 生产建议：能力允许时优先用托管集群，减少运维负担

## 相关文档

- [[技能/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]]
- [[技能/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]]
- [[技能/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]]
- [[概念/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[技能/kubelet-certificate-rotation.md|kubelet 证书轮换机制]]
- [[概念/node-lifecycle-management.md|节点生命周期管理]]
- [[技能/deployment-rolling-update.md|Deployment 滚动更新策略]]
- [[技能/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]]
- [[技能/deployment-workload-selection.md|工作负载控制器选型]]

## Related

- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]]

- [[概念/声明式 API × 控制器模式.md|声明式 API × 控制器模式]]

- [[概念/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]]

- etcd x 高可用模式.md|etcd x 高可用模式]]

- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — [[概念/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[系统基础/知识字典/fundamentals/nodes.md|Nodes（节点）]]


<!-- risk-assessed -->
