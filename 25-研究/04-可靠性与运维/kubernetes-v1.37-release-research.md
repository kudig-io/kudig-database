---
title: Kubernetes v1.37 发布专题研究
summary: K8s v1.37 (Garhwal) 年度大版本专题研究 — 16 项 Stable / 23 项 Beta / 27 项 Alpha 全景解读，ipvs 废弃时间线、静态 Pod 硬约束、SELinux 挂载变更与 SRE 分阶段行动清单。
category: research
tags:
- research
- k8s
- kubernetes
- release
- upgrade
- sre
- nftables
- dra
tier: supporting
created: '2026-08-28'
updated: '2026-08-28'
last_updated: '2026-08-28'
status: done
---

# Kubernetes v1.37 发布专题研究

> 发布代号 **Garhwal**（गढ़वाल），2026-08-26 正式发布。
> 年度大版本：67 项增强 — **16 Stable / 23 Beta / 27 Alpha**，叠加多项影响生产运维的废弃与移除。

## 研究背景

v1.37 是 2026 年的第三个版本（年度大版本节奏 4 月/8 月/12 月中的 8 月档），本次发布的三条主线对 SRE 基础设施影响最大：

1. **API 服务器韧性收官**：watchcache 初始化加固（KEP-4568）完全锁定，etcd RangeStream（KEP-4520 系）、并发 watch 解码等性能特性进入 Beta — 大集群控制面稳定性显著改善
2. **AI/ML 基础设施成为一等公民**：Gang scheduling Beta、层级化 CompositePodGroup Alpha、Job 原生 `spec.scheduling`、DRA 五项 GA — GPU 集群调度栈全面升级
3. **网络后端代际更替启动**：kube-proxy ipvs 模式废弃（KEP-5495）、nftables 成为未来默认（KEP-5343）、netlink 直连性能优化

---

## 一、Stable 毕业（16 项）

| 特性 | KEP | 领域 | SRE 关键点 |
|------|-----|------|-----------|
| Resilient Watchcache Initialization（含 WatchCacheInitializationPostStartHook 锁定） | 4568 | API Machinery | 启动/恢复期 list/watch 不再冲击 etcd，超额请求 429+Retry-After；**客户端必须实现退避** |
| metrics.k8s.io API | 5207 | Instrumentation | 9 年 Beta 终转正；新增 `v1`，`v1beta1` 按弃用策略继续服务 |
| SELinuxMount / SELinuxChangePolicy | 1710 | Storage | `-o context=` 挂载替代递归重标；不同标签 Pod 共卷会启动失败（行为锁定在 v1.38） |
| Pod Certificates | 4317 | Auth | Pod 级 X.509 证书（mTLS）原生分发，替代手工 cert-manager 类方案的部分场景 |
| ClusterTrustBundles | 3257 | Auth | 信任锚集群级分发 GA |
| KYAML | 5295 | CLI/API Machinery | `kubectl -o kyaml`，根治 YAML "Norway bug"（裸 `no`/`on` 被解析为布尔） |
| Node Declared Features | 5328 | Node | 节点声明特性支持（`.status.declaredFeatures`），控制面跨版本偏差自适应 |
| Storage Version Migrator 内置 | 4192 | API Machinery | `storagemigration.k8s.io/v1` GA；升级/改加密后存量数据迁移不再依赖外部组件 |
| DRA: ResourceClaim Status 网络接口数据 | 4817 | DRA | 设备状态标准化（含 NIC 信息） |
| DRA: device taints and tolerations | 5055 | DRA | 设备污点/容忍，GPU 故障隔离 |
| DRA: 扩展资源经 DRA Driver 处理 | 5004 | DRA | extended resource 向 DRA 迁移的桥接 |
| DRA: 标准 numaNode 设备属性 | 6072 | DRA | GPU/NIC/CPU 同 NUMA 协同调度 |
| Pod Status 资源健康状态（Device Plugin/DRA） | 4680 | Node | Pod 状态直接可见设备健康 |
| HPA 可配置 tolerance | 4951 | Autoscaling | 抖动抑制参数化 |
| Services 名称宽松校验 | 5311 | Network | 服务名规范放宽 |
| Pod hostname 任意 FQDN | 4762 | Network | setHostnameAsFQDN 放开限制 |
| sandbox 创建条件（PodStatus） | 3085 | Node | Pod Sandbox 沙箱状态条件化 |

## 二、Beta 毕业重点（23 项选摘）

### 面向 AI/ML

- **Gang scheduling**：成组调度 Beta，解决部分调度导致的死锁与资源空转
- **Workload-aware preemption**：抢占从 Pod 粒度升级到工作负载粒度
- **DRA ResourceClaim for workloads**：Workload/PodGroup API 关联 ResourceClaim
- **Job `spec.scheduling`**（Alpha 延续）：批量作业原生配置拓扑约束/中断策略/资源声明

### 可观测性与性能

- **Native histograms**：控制面指标原生直方图，替代静态 bucket
- **cAdvisor-less, CRI-full stats**：kubelet 统计来源从 cAdvisor 收敛到 CRI，消除双源歧义
- **etcd RangeStream**：大 list 请求流式化，缓解 watchcache 预热期内存尖峰
- **Concurrent watch object decode**：CRD 转换 webhook 不再串行阻塞 watch 流（冷缓存初始化从分钟级改善）
- **Stale controller mitigation**：controller-manager 缓存陈旧性缓解
- **client-go contextual logging 完成**：全链路 context 传播

### 存储与成本

- **PVC last used**（`PersistentVolumeClaimUnusedSinceTime`，默认启用）：`Unused` 条件 + 最后使用时间，僵尸 PVC 治理终于有原生信号
- **Storage capacity scoring of Nodes**：动态供给卷参与节点评分
- **CSI attach limits × Cluster Autoscaler**：扩容节点数计算纳入卷挂载上限
- **HPA scale to zero**（默认启用）：object/external 指标可缩至 0 副本；CPU/内存指标不支持；状态条件 `ScaledToZero` 区分自动缩零与手工停用
- **Memory QoS (cgroups v2)**（默认启用）：内存保护与限流进 QoS 体系

### API 服务器与安全

- **Manifest-based admission control**（KEP-5793）：`AdmissionConfiguration.staticManifestsDir` 从磁盘加载 webhook/CEL 策略 — etcd 不可用时策略仍生效，可保护 API 侧准入资源本身
- **Undecryptable resources 改进**：加密密钥丢失时的资源处理优化
- **Kubelet-in-UserNS**（KEP-2033）：rootless 容器运行 Alpha→Beta

## 三、Alpha 前瞻（27 项选摘）

| 特性 | KEP / 门禁 | 价值判断 |
|------|-----------|---------|
| Pod 级 Checkpoint/Restore | 5823 `PodLevelCheckpointRestore` | JVM/ML 大实例热迁移与秒级暖启动；需运行时支持 CRI RPC |
| StatefulSet Recreate 策略 | 3541 `StatefulSetRecreateStrategy` | 有状态应用"全停换新"场景；注意改策略不触发滚动 |
| 就地扩容抢占 | 5836 `SchedulerPreemptionForPodResize` | 打通 in-place resize 的资源死结 |
| 内存卷就地扩容 | 6030 `InPlacePodVerticalScalingMemoryBackedVolumes` | emptyDir(memory) sizeLimit 可变 |
| 节点生命周期条件 | 5683 | `DrainInProgress`/`Drained`/`MaintenancePlanned` 等标准条件，自动化运维终于有官方信号源 |
| 层级化 CompositePodGroup | 6012 `CompositePodGroup` | 多级 gang 调度（AI prefill/decode 分层） |
| nftables localhost NodePort | 6032 `KubeProxyNFTablesLocalhostNodePorts` | 本机访问 NodePort 场景补齐（仅 TCP） |
| gRPC 探针 TLS / h2c 探针 | 4939 / 5999 | 探针协议现代化 |
| 卷健康监控 | 1432 `CSIVolumeHealth` | 标准化卷健康状态枚举（Inaccessible/DataLoss/Degraded…） |
| 快照拓扑感知 | 5943 `VolumeSnapshotTopology` | 跨区恢复快照的定位约束 |
| DRA 三项增强 | 6080/5963/5945 | 派生属性、兼容组、免节点预处理 |

## 四、废弃与移除（重点跟进）

### 4.1 kube-proxy ipvs 模式废弃（KEP-5495）

- **时间线**：v1.37 废弃告警 → v1.40 默认禁用（门禁可选） → **v1.43 完全移除**
- **根因**：内核 ipvs API 无法完整实现 K8s Service，ipvs 模式底层仍在用 iptables；nftables 后端是官方终态（KEP-5343）
- **行动**：盘点全部集群 proxy 模式 → 预发验证 nftables → 分批切换

```bash
kubectl -n kube-system get configmap kube-proxy -o jsonpath='{.data.config\.conf}' | grep 'mode:'
```

### 4.2 kube-dns 废弃

- 子项目已退役；v1.40 后不再出新包。仍运行 kube-dns 的集群缺乏 EndpointSlices/双栈支持
- **行动**：迁移 CoreDNS（node-local-dns 已拆分独立维护）

### 4.3 静态 Pod 禁止引用 Secret/ConfigMap（硬移除）

- bug 修复转为强约束，逃生门 `PreventStaticPodAPIReferences` **已移除**
- **行动**：`grep -rn "configMapRef\|secretRef" /etc/kubernetes/manifests/` 全量扫描并改造

### 4.4 其他

- `kubectl run --filename/-f` 废弃（本就只按 CLI 参数构建 Pod）
- **cgroup v1 淘汰推进**（KEP-5573）：v1.35 起 `failCgroupV1` 默认 true，v1.37 仍可覆盖但属短期方案；Memory QoS、内存卷就地扩容仅 cgroup v2 可用
- **nftables 默认化过渡**：未显式指定 proxy mode 时回退 iptables 产生告警

## 五、SRE 分阶段行动清单

### 升级前（升级窗口前 2-4 周）

1. [ ] 盘点 kube-proxy 模式，制定 ipvs → nftables 迁移计划（v1.40 死线）
2. [ ] 扫描静态 Pod 的 Secret/ConfigMap 引用并改造（**升级阻断项**）
3. [ ] 确认全部节点 cgroup v2（否则列入迁移或临时 `failCgroupV1: false`）
4. [ ] 检测 kube-dns 遗留部署，先迁 CoreDNS
5. [ ] SELinux 启用的集群：核查 CSI 驱动 `seLinuxMount` 支持与共卷多标签工作负载
6. [ ] 审计自研 Operator/控制器的 429 处理（Retry-After + 指数退避）
7. [ ] etcd 快照 + 静态清单备份

### 升级窗口

1. [ ] 按 v1.36 → v1.37 逐级升级（先控制平面后节点，逐批 drain）
2. [ ] 大集群 apiserver 重启窗口观察 429/恢复行为
3. [ ] 升级后验证 metrics API、StatefulSet 滚动更新、静态 Pod

### 升级后（1-2 周内）

1. [ ] 评估 HPA scale to zero 用于批处理/GPU 队列消费型负载的成本收益
2. [ ] 利用 PVC `Unused` 条件启动僵尸 PVC 治理（对接 FinOps 流程）
3. [ ] AI/ML 集群：试点 Gang scheduling + Job `spec.scheduling`
4. [ ] 建立节点生命周期条件（`DrainInProgress` 等，Alpha）的自动化验证环境
5. [ ] 将 ipvs 迁移进度纳入集群治理看板

## 参考链接

- [Kubernetes v1.37: Garhwal 发布公告](https://kubernetes.io/blog/2026/08/26/kubernetes-v1-37-release/)
- [Kubernetes v1.37 Sneak Peek](https://kubernetes.io/blog/2026/07/31/kubernetes-v1-37-sneak-peek/)
- [Kubernetes 1.37 Deep dive into new alpha features](https://palark.com/blog/kubernetes-1-37-release-features/)
- [KEP-5495: Deprecate ipvs mode in kube-proxy](https://www.kubernetes.dev/resources/keps/5495/)
- [KEP-5343: nftables proxy](https://github.com/kubernetes/enhancements/blob/master/keps/sig-network/3866-nftables-proxy/README.md)
- [Metrics API GA](https://kubernetes.io/blog/2026/08/27/kubernetes-v1-37-metrics-api-ga/)
- [SELinux Volume Label Changes GA 影响](https://kubernetes.io/blog/2026/04/22/breaking-changes-in-selinux-volume-labeling/)

## Related

- [[01-集群基础/06-升级路径/05-kubernetes-v1.37-upgrade-guide|Kubernetes v1.37 升级实操指南]]
- [[01-集群基础/06-升级路径/04-kubernetes-v1.33-upgrade-guide|Kubernetes v1.33 升级实操指南]]
- [[25-研究/04-可靠性与运维/observability-evolution|K8s 可观测性体系演进研究]]
- [[25-研究/04-可靠性与运维/kubernetes-autoscaling-strategies|K8s 自动扩缩容策略研究]]
