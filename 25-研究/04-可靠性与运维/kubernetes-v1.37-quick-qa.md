---
title: Kubernetes v1.37 新功能快问快答
summary: 面向 SRE 的 Kubernetes v1.37 Garhwal 新功能速览，按控制面、可观测性、AI/ML 调度、资源成本、存储安全、节点网络与 DRA 分类归纳。
category: research
tags:
- research
- k8s
- kubernetes
- release
- upgrade
- sre
- faq
tier: supporting
created: '2026-08-28'
updated: '2026-08-28'
last_updated: '2026-08-28'
status: done
---

# Kubernetes v1.37 新功能快问快答

> Kubernetes **v1.37 Garhwal** 是 2026-08-26 发布的年度大版本，包含 67 项增强：16 Stable / 23 Beta / 27 Alpha。

## 快答：这个新版本有哪些新功能？

Kubernetes v1.37 主要新功能可以按 SRE 视角分成 6 类：

### 1. 控制面稳定性增强

- **Resilient Watchcache Initialization GA**  
  apiserver 启动/恢复时不再让大量 list/watch 请求打爆 etcd；超额请求返回 `429 + Retry-After`。
- **etcd RangeStream Beta**  
  大规模 list 请求流式化，降低 apiserver / etcd 内存尖峰。
- **Concurrent watch object decode Beta**  
  watch 事件并发解码，CRD 转换 webhook 不再串行阻塞冷缓存初始化。

### 2. 可观测性与指标

- **metrics.k8s.io API GA**  
  `kubectl top` / HPA 依赖的 metrics API 终于从 `v1beta1` 走到 Stable `v1`。
- **Native Histograms Beta**  
  Kubernetes 控制面指标支持 Prometheus 原生直方图。
- **CRI-full stats Beta**  
  kubelet 容器/Pod 统计逐步摆脱 cAdvisor 双源问题，转向 CRI 统一数据源。

### 3. AI/ML 与大规模调度

- **Gang Scheduling Beta**  
  面向 AI 训练/HPC，避免一组 Pod 只调度成功一部分导致资源死锁。
- **Workload-aware Preemption Beta**  
  抢占从单 Pod 粒度提升到工作负载粒度。
- **CompositePodGroup Alpha**  
  支持层级化 PodGroup，适合复杂 AI/ML 工作负载。
- **Job `spec.scheduling` Alpha**  
  Job 可原生声明拓扑约束、调度策略、中断策略、资源声明。

### 4. 资源管理与成本优化

- **HPA scale to zero Beta，默认启用**  
  object / external metrics 可把工作负载缩到 0，适合队列消费者、批任务、GPU 空闲场景。
- **Memory QoS Beta，默认启用**  
  基于 cgroup v2 做内存保护与限流。
- **PVC last used Beta，默认启用**  
  PVC 增加 `Unused` 条件，可用于僵尸 PVC 清理和 FinOps。
- **Pod-level resource managers Beta**  
  Pod 级 CPU / 内存 / 拓扑资源管理，默认关闭。

### 5. 存储与安全能力

- **SELinuxMount / SELinuxChangePolicy GA**  
  卷挂载从递归 relabel 转向 `-o context=`，更快但有共卷多标签兼容风险。
- **Pod Certificates GA**  
  原生给 Pod 分发 X.509 证书，服务 mTLS 场景。
- **ClusterTrustBundles GA**  
  集群级信任锚分发机制稳定。
- **StorageVersionMigration GA**  
  内置存储版本迁移 API，CRD / API 升级后可声明式迁移存量对象。

### 6. 节点、网络与 DRA

- **Node Declared Features GA**  
  节点通过 `.status.declaredFeatures` 声明能力，帮助控制面处理版本偏差。
- **DRA 多项 GA / Alpha 增强**  
  包括设备 taints/tolerations、NUMA 属性、ResourceClaim 状态、设备兼容组、派生属性等，主要服务 GPU/NIC/专用硬件调度。
- **nftables 性能增强**  
  kube-proxy nftables 后端改用 netlink 直接操作规则。
- **nftables localhost NodePort Alpha**  
  支持本机访问 NodePort 的 nftables 用户态代理，仅 TCP。

## 一句话总结

**v1.37 是控制面韧性 + AI/ML 调度 + nftables 迁移 + 成本治理能力的一次大版本增强。**

## SRE 最该先看什么？

1. **升级阻断项**：静态 Pod 引用 Secret/ConfigMap、cgroup v1、kube-dns、SELinux 共卷多标签风险。
2. **网络迁移项**：kube-proxy `ipvs` 废弃，v1.40 默认禁用，v1.43 移除。
3. **大集群控制面**：watchcache 429 行为、etcd RangeStream、并发 watch 解码。
4. **成本治理**：HPA scale to zero、PVC `Unused` 条件、Memory QoS。
5. **AI/ML 平台**：Gang Scheduling、DRA、CompositePodGroup、Job `spec.scheduling`。

## Related

- [[25-研究/04-可靠性与运维/kubernetes-v1.37-release-research|Kubernetes v1.37 发布专题研究]]
- [[01-集群基础/06-升级路径/05-kubernetes-v1.37-upgrade-guide|Kubernetes v1.37 升级实操指南]]
