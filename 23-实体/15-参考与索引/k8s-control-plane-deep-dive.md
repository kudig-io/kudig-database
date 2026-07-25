---
title: 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
description: '# 控制平面深度剖析'
summary: '1. **认证（Authentication）**：x509 证书、Bearer Token、OIDC 等'
category: reference
tags:
- k8s
- control-plane
- apiserver
- scheduler
- kube-controller-manager
- cri
- csi
- cni
- etcd
- controller-manager
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI 是什么
- 如何 控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI
trigger_keywords:
- 控制平面深度剖析：API
- Server
- Scheduler
- KCM
- CRI
- CSI
- CNI
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制平面深度剖析

## 概述

Kubernetes 控制平面是集群的大脑，由 API Server、Scheduler、Controller Manager 和 etcd 组成。通过 CRI/CSI/CNI 三大接口与容器运行时、存储和网络交互。深入理解控制平面组件的工作原理是 Kubernetes 生产运维和性能调优的基础。

## API Server 请求处理链路

每个 API 请求经过四阶段处理：

1. **认证（Authentication）**：x509 证书、Bearer Token、OIDC、Webhook Token 认证。API Server 支持多种认证方式同时启用，只要一种通过即可
2. **授权（Authorization）**：RBAC（推荐）、ABAC、Node Authorizer、Webhook。RBAC 通过 Role/ClusterRole 和 RoleBinding/ClusterRoleBinding 控制资源访问权限
3. **准入控制（Admission Control）**：
   - Mutating Webhooks：修改请求对象（如注入 Sidecar、ServiceAccount、默认标签）
   - Validating Webhooks：验证请求合法性（如镜像策略、资源配额）
   - 内置准入控制器：AlwaysPullImages、NamespaceLifecycle、ResourceQuota
4. **持久化**：通过乐观锁写入 etcd 并返回响应。resourceVersion 确保并发安全

API Server 是唯一直接读写 etcd 的组件，所有其他组件通过 API Server 交互。

## Scheduler 调度算法

两阶段调度流程：

**阶段一：过滤（Filtering）**
- NodeResourcesFit：节点 CPU/内存/GPU 是否满足 Pod 请求
- NodeAffinity：节点亲和性（required/preferred）
- TaintToleration：污点容忍（NoSchedule/NoExecute/PreferNoSchedule）
- PodTopologySpread：拓扑分布约束（跨 Zone/Region 分散）
- VolumeZone：PV 所在 Zone 必须匹配节点

**阶段二：打分（Scoring）**
- LeastRequestedPriority：资源使用率最低优先（均衡分配）
- BalancedResourceAllocation：CPU/Memory 使用率均衡
- ImageLocality：镜像已存在节点优先（减少拉取时间）
- InterPodAffinity：Pod 亲和/反亲和（co-locate 或 spread）
- NodeAffinityPriority：Preferred 亲和性权重

打分后选择最高分节点。默认调度策略可通过调度器配置自定义。

## KCM 控制器清单

kube-controller-manager（KCM）内含 30+ 独立控制器，核心控制器包括：
- **Deployment Controller**：管理 ReplicaSet 滚动更新和回滚
- **ReplicaSet Controller**：确保 Pod 副本数匹配期望状态
- **Node Controller**：节点健康检测与驱逐（5min 标记 NotReady，40s 开始驱逐）
- **Service Controller**：与云厂商 LoadBalancer 集成
- **Endpoint Controller**：维护 Service 到 Pod IP 的映射
- **Job Controller**：批处理任务生命周期管理
- **HPA Controller**：基于 CPU/内存/自定义指标的水平自动扩缩容

## CRI/CSI/CNI 三大接口

| 接口 | 职责 | 主流实现 |
|------|------|----------|
| CRI（Container Runtime Interface） | 容器生命周期管理 | containerd, CRI-O |
| CSI（Container Storage Interface） | 存储卷挂载/卸载/快照 | Ceph CSI, EBS CSI, NFS CSI |
| CNI（Container Network Interface） | Pod 网络配置 | Calico, Cilium, Flannel |

三大接口的解耦设计使得 K8s 能够适配不同基础设施，无需修改核心代码。CRI 通过 gRPC 与容器运行时通信，CSI 通过 gRPC 与存储驱动交互，CNI 通过二进制插件配置网络。

## 实践要点

- API Server 性能：关注 request latency、watch cache hit rate
- 调度器性能：关注 scheduling throughput、pending pods
- etcd 性能：关注 commit latency、proposal rate

## 运维操作

```bash
# 🟢 查看控制平面组件状态
kubectl get componentstatuses
kubectl get --raw /healthz?verbose
kubectl get pods -n kube-system -l tier=control-plane

# 🟢 API Server 请求延迟监控
curl -sk https://<apiserver>:6443/metrics | grep apiserver_request_duration_seconds
kubectl get --raw /metrics | grep apiserver_current_inflight_requests

# 🟢 调度器状态检查
kubectl get events -A --field-selector reason=FailedScheduling
kubectl logs -n kube-system -l component=kube-scheduler --tail=50

# 🟢 etcd 健康检查
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health
etcdctl endpoint status --write-out=table

# 🟡 查看 API Server 审计日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=100
# 检查准入控制器配置
cat /etc/kubernetes/manifests/kube-apiserver.yaml | grep admission

# 🟡 检查 CRI/CSI/CNI 状态
crictl ps                    # CRI: 查看运行容器
crictl pods                  # CRI: 查看 Pod
kubectl get csidrivers       # CSI: 查看存储驱动
kubectl get pods -n kube-system -l k8s-app=calico-node  # CNI
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| API Server 响应慢 | etcd 延迟高/watch 过多 | `etcdctl endpoint status` | 优化 etcd 磁盘/减少 watch |
| Pod 长时间 Pending | 调度器过滤失败/资源不足 | `kubectl describe pod <name>` | 检查节点资源和调度约束 |
| 节点 NotReady | kubelet 与 API Server 断连 | `kubectl describe node <name>` | 检查网络/证书/kubelet 状态 |
| CSI 挂载失败 | 存储后端不可达/驱动异常 | `kubectl describe pvc <name>` | 检查 CSI driver Pod 和存储后端 |
| CNI 网络不通 | CNI 插件异常/IP 耗尽 | `kubectl logs -n kube-system -l k8s-app=calico-node` | 重启 CNI Pod 或扩展 IP 池 |

```
排查流程：
├─ 控制平面异常
│  ├─ kubectl get componentstatuses 检查组件健康
│  ├─ API Server 异常 → 检查 etcd 连接和证书
│  ├─ Scheduler 异常 → 检查调度器日志和配置
│  └─ KCM 异常 → 检查控制器日志
├─ 调度问题
│  ├─ kubectl describe pod 查看调度失败原因
│  ├─ 检查节点资源/污点/亲和性
│  └─ 检查 PV/PVC 状态（VolumeZone）
└─ CRI/CSI/CNI 问题
   ├─ CRI: crictl ps + crictl logs
   ├─ CSI: kubectl describe pvc + CSI driver 日志
   └─ CNI: 检查 CNI Pod + 节点网络配置
```

## 生产案例

### 案例 1：API Server 延迟飙升导致集群不可用

- **场景**: 生产集群 API Server P99 延迟从 50ms 飙升至 5s，kubectl 操作超时
- **排查**: etcd commit latency 从 5ms 升至 500ms，磁盘 IOPS 达到上限
- **方案**: etcd 迁移至 NVMe SSD，启用 API Priority and Fairness 限制大客户端
- **效果**: API Server P99 恢复至 80ms，etcd commit <10ms

### 案例 2：调度器吐量不足导致 Pod 积压

- **场景**: 批量创建 1000 个 Pod，调度器处理速度仅 50 pods/s，积压严重
- **排查**: 调度器日志显示大量 Filter 失败（NodeResourcesFit），节点资源磎片化
- **方案**: 启用 Cluster Autoscaler + 调整调度器并发数（--kube-api-qps）
- **效果**: 调度吐量提升至 200 pods/s，Pod 积压清空

## 检查清单

- [ ] API Server 证书有效期 > 30 天
- [ ] etcd 集群健康且 commit latency < 10ms
- [ ] 调度器无持续 Pending Pod
- [ ] KCM 所有控制器正常运行
- [ ] CRI 运行时版本与 K8s 兼容
- [ ] CSI 驱动健康且 PV 可用
- [ ] CNI 插件正常且 IP 池充足
- [ ] API Priority and Fairness 已配置

---

> 来源：.zread/wiki/drafts/7-kong-zhi-ping-mian-shen-du-pou-xi-api-server-scheduler-kcm-yu-cri-csi-cni.md

## Related

- [[deployment]] — Deployment
- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd


<!-- risk-assessed -->
