---
title: 生产工单 Case Study 索引
category: case-study
tags: [production, incident, troubleshooting, sre]
created: "2026-05-23"
updated: "2026-05-23"
---

# 生产工单 Case Study 索引

> 基于真实生产环境的问题工单闭环案例，每个案例包含：问题现象 → 诊断过程 → 根因分析 → 修复动作 → 验证 → 复盘。

---

## 按严重程度分类

### 🔴 P0 — 业务中断

| 日期 | 案例 | 影响范围 | MTTR | 根因 |
|:---|:---|:---|:---:|:---|
| 2026-01-15 | [[synthesis/case-studies/2026-01-15-node-notready-pod-eviction|Node NotReady 导致大规模 Pod 驱逐]] | 3 NS, 47 Pods | 25min | kubelet 证书过期 |
| 2026-02-05 | [[synthesis/case-studies/2026-02-05-etcd-inconsistency-503|etcd 数据不一致导致 API Server 503]] | 全集群 | 35min | etcd 网络分区 |
| 2026-01-22 | [[synthesis/case-studies/2026-01-22-coredns-discovery-failure|CoreDNS 问题导致服务发现中断]] | 全集群 | 18min | CoreDNS 内存泄漏 |
| 2026-08-05 | [[synthesis/case-studies/2026-08-05-istio-mtls-strict|Istio mTLS 严格模式导致服务连通性中断]] | 多 NS | 28min | mTLS 策略升级 |

### 🟠 P1 — 性能降级 / 局部中断

| 日期 | 案例 | 影响范围 | MTTR | 根因 |
|:---|:---|:---|:---:|:---|
| 2026-02-18 | [[synthesis/case-studies/2026-02-18-hpa-thrashing|HPA 配置错误导致无限扩缩容]] | 2 NS, 120 Pods | 42min | metrics-server 延迟 |
| 2026-03-15 | [[synthesis/case-studies/2026-03-15-oomkilled-java-restart|OOMKilled 导致 Java 应用反复重启]] | 1 NS, 8 Pods | 55min | JVM 堆内存未限制 |
| 2026-03-28 | [[synthesis/case-studies/2026-03-28-networkpolicy-misconfig|NetworkPolicy 误配导致服务间通信中断]] | 2 NS | 30min | 策略标签选择器错误 |
| 2026-04-10 | [[synthesis/case-studies/2026-04-10-ingress-502-bad-gateway|Ingress 配置错误导致 502 Bad Gateway]] | 1 NS, 15 Pods | 22min | 后端服务端口变更 |
| 2026-05-12 | Cluster Autoscaler 缩容导致节点驱逐延迟 | 2 Nodes, 47 Pods | 32min | PreStop hook 卡住 |
| 2026-05-20 | [[synthesis/case-studies/2026-07-08-prometheus-high-cardinality-oom|Prometheus 高基数导致 OOM]] | 监控集群 | 28min | 高基数指标标签 |
| 2026-07-08 | [[synthesis/case-studies/2026-07-08-prometheus-high-cardinality-oom|Prometheus 高基数 OOM（第二起）]] | 监控集群 | 20min | path 标签动态 ID |
| 2026-07-20 | [[synthesis/case-studies/2026-07-20-velero-backup-failure|Velero 备份失败导致无法恢复]] | 备份系统 | 45min | 存储凭据过期 |

### 🟡 P2 — 局部异常 / 开发环境

| 日期 | 案例 | 影响范围 | MTTR | 根因 |
|:---|:---|:---|:---:|:---|
| 2026-03-02 | [[synthesis/case-studies/2026-03-02-certificate-expiry-kubelet|证书过期导致 kubelet 无法上报心跳]] | 1 Node | 15min | 证书轮转失败 |
| 2026-04-22 | [[synthesis/case-studies/2026-04-22-pvc-unbound-statefulset|PVC 未绑定导致 StatefulSet 无法启动]] | 1 NS, 3 Pods | 20min | StorageClass 参数错误 |
| 2026-04-28 | DaemonSet 节点亲和性导致部分节点未部署 | 3 Nodes | 18min | 节点标签不匹配 |
| 2026-05-28 | [[synthesis/case-studies/2026-05-28-daemonset-affinity-miss|DaemonSet 亲和性缺失]] | 2 Nodes | 12min | 节点标签变更 |
| 2026-05-01 | [[synthesis/case-studies/2026-05-01-imagepullbackoff-registry-auth|ImagePullBackOff 镜像仓库认证失败]] | 1 NS, 6 Pods | 10min | 镜像拉取 Secret 过期 |
| 2026-05-05 | [[synthesis/case-studies/2026-06-10-cronjob-concurrency-backlog|CronJob 并发策略导致任务堆积]] | 1 NS, 47 Pods | 22min | concurrencyPolicy=Allow |
| 2026-06-10 | [[synthesis/case-studies/2026-06-10-cronjob-concurrency-backlog|CronJob 并发堆积（第二起）]] | 1 NS, 23 Pods | 15min | 未设置 activeDeadlineSeconds |
| 2026-05-10 | [[synthesis/case-studies/2026-06-25-resourcequota-exceeded|ResourceQuota 超限导致新 Pod 无法创建]] | 1 NS | 15min | 僵尸 Pod 占满 quota |
| 2026-06-25 | [[synthesis/case-studies/2026-06-25-resourcequota-exceeded|ResourceQuota 超限（第二起）]] | 1 NS | 12min | Job 未清理 |
| 2026-05-15 | [[synthesis/case-studies/2026-05-15-configmap-no-rolling-update|ConfigMap 更新未触发滚动更新]] | 1 NS, 10 Pods | 8min | 未设置 hash 注解 |

---

## 按问题域分类

### 控制平面
- [[synthesis/case-studies/2026-02-05-etcd-inconsistency-503|etcd 数据不一致]]
- [[synthesis/case-studies/2026-01-22-coredns-discovery-failure|CoreDNS 服务发现中断]]
- [[synthesis/case-studies/2026-03-02-certificate-expiry-kubelet|证书过期]]

### 工作负载
- [[synthesis/case-studies/2026-01-15-node-notready-pod-eviction|Node NotReady Pod 驱逐]]
- [[synthesis/case-studies/2026-03-15-oomkilled-java-restart|OOMKilled Java 重启]]
- [[synthesis/case-studies/2026-04-22-pvc-unbound-statefulset|PVC 未绑定]]
- DaemonSet 未部署
- [[synthesis/case-studies/2026-05-28-daemonset-affinity-miss|DaemonSet 亲和性缺失]]
- [[synthesis/case-studies/2026-05-01-imagepullbackoff-registry-auth|ImagePullBackOff]]
- [[synthesis/case-studies/2026-05-15-configmap-no-rolling-update|ConfigMap 未触发更新]]

### 网络
- [[synthesis/case-studies/2026-03-28-networkpolicy-misconfig|NetworkPolicy 误配]]
- [[synthesis/case-studies/2026-04-10-ingress-502-bad-gateway|Ingress 502]]
- [[synthesis/case-studies/2026-08-05-istio-mtls-strict|Istio mTLS 严格模式]]

### 自动伸缩
- [[synthesis/case-studies/2026-02-18-hpa-thrashing|HPA 无限扩缩容]]
- CA 缩容延迟

### 批处理
- [[synthesis/case-studies/2026-06-10-cronjob-concurrency-backlog|CronJob 任务堆积]]
- [[synthesis/case-studies/2026-06-10-cronjob-concurrency-backlog|CronJob 堆积（第二起）]]
- [[synthesis/case-studies/2026-06-25-resourcequota-exceeded|ResourceQuota 超限]]
- [[synthesis/case-studies/2026-06-25-resourcequota-exceeded|ResourceQuota 超限（第二起）]]

### 可观测性
- [[synthesis/case-studies/2026-07-08-prometheus-high-cardinality-oom|Prometheus OOM]]
- [[synthesis/case-studies/2026-07-08-prometheus-high-cardinality-oom|Prometheus OOM（第二起）]]

### 存储与备份
- [[synthesis/case-studies/2026-07-20-velero-backup-failure|Velero 备份失败]]

---

## 使用方式

### SRE Agent 训练
每个 Case Study 包含完整的诊断时间线和修复命令，可直接用于：
- Fine-tuning 数据构造
- Few-shot prompt 示例
- 工单分类模型训练

### 问题演练脚本
```bash
# 基于 Case Study 构造混沌实验
kubectl apply -f case-studies/chaos-templates/
```

### 知识图谱关联
每个 Case Study 通过 `wikilink` 关联到对应的 Skill、FTA 和 Domain 文档。

---

## 统计

- **总案例数**: 22
- **P0 严重**: 4
- **P1 严重**: 8
- **P2 严重**: 10
- **平均 MTTR**: 24min
- **覆盖问题域**: 控制平面、工作负载、网络、自动伸缩、批处理、可观测性、存储备份
