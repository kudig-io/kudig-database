---
title: kubelet
description: kubelet — Kubernetes 生产运维知识库
summary: 'kubelet runs on every worker node and is responsible for:'
category: entities
tags:
- k8s
- kubelet
- node
- agent
- cri
- cgroups
- containerd
- cri-o
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 是什么
- 如何 kubelet
trigger_keywords:
- kubelet
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kubelet

## Role

kubelet runs on every worker node and is responsible for:
- Watching API Server for Pod assignments
- Managing container lifecycle via CRI ([[containerd|containerd]]/CRI-O)
- Mounting volumes via CSI
- Running health probes (liveness, readiness, startup)
- Reporting node and Pod status
- Evicting [[pods|Pods]] under resource pressure

## Key Subsystems

| Subsystem | Function |
|-----------|----------|
| **PLEG** (Pod Lifecycle Event Generator) | Monitors [[22-概念/15-运行时与系统/container-runtime.md|container runtime]], generates state change events that trigger syncPod |
| **Probe Manager** | Runs liveness, readiness, and startup probes |
| **Volume Manager** | Mounts/unmounts volumes, interacts with CSI drivers |
| **Eviction Manager** | Monitors node resources, evicts Pods when thresholds crossed |
| **cAdvisor** | Collects container resource metrics (CPU, memory, network, disk I/O) |
| **Status Manager** | Reports Pod and Node status to API Server |

## CRI (Container Runtime Interface)

kubelet communicates with container runtimes via gRPC-based CRI:
- `RunPodSandbox`: Create Pod network namespace
- `CreateContainer` / `StartContainer`: Container lifecycle
- `PullImage`: Pull container images
- `ListImages` / `RemoveImage`: Image management

## Key Configuration

| Parameter | Purpose | Recommended |
|-----------|---------|-------------|
| `--container-runtime-endpoint` | CRI socket | unix:///run/containerd/containerd.sock |
| `--cgroup-driver` | cgroup driver | systemd (must match runtime) |
| `--max-pods` | Max Pods per node | 110 (default), 500+ in cloud |
| `--eviction-hard` | Hard eviction threshold | memory.available<100Mi |
| `--pod-infra-container-image` | pause container image | registry.k8s.io/pause:3.9 |

## Certificate Rotation

kubelet auto-rotates its client certificate (`--rotate-certificates`), preventing certificate expiration issues.

## 运维操作

```bash
# 🟢 检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet --since "10 min ago" -f

# 🟢 查看 kubelet 配置
cat /var/lib/kubelet/config.yaml
kubectl get node <node> -o jsonpath='{.status.conditions}'

# 🟢 检查 kubelet 指标
curl -sk https://localhost:10250/metrics | grep kubelet_ | head -20
curl -sk https://localhost:10250/metrics/cadvisor | head -20

# 🟢 查看 Pod 状态和事件
kubectl get pods --field-selector spec.nodeName=<node> -A
kubectl get events --field-selector source.component=kubelet

# 🟢 检查 kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 🟡 重启 kubelet（会短暂影响节点上 Pod）
systemctl restart kubelet

# 🟢 检查驱逐状态
kubectl describe node <node> | grep -A10 "Conditions"
kubectl get pods -A --field-selector=status.phase=Failed
```

### KubeletConfiguration 完整示例

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
# 资源管理
maxPods: 110
podsPerCore: 10
systemReserved:
  cpu: 200m
  memory: 512Mi
  ephemeral-storage: 1Gi
kubeReserved:
  cpu: 200m
  memory: 512Mi
  ephemeral-storage: 1Gi
# 驱逐策略
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
# 运行时
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd
# 证书
rotateCertificates: true
serverTLSBootstrap: true
# 日志
containerLogMaxSize: "50Mi"
containerLogMaxFiles: 5
# 探针
nodeStatusUpdateFrequency: 10s
nodeStatusReportFrequency: 5m
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点 NotReady | kubelet 停止/证书过期 | `systemctl status kubelet`; `journalctl -u kubelet` | 重启 kubelet/轮换证书 |
| PLEG is not healthy | 容器运行时响应慢 | `journalctl -u kubelet` 查看 PLEG | 检查 containerd/磁盘 IO |
| Pod 驱逐 | 资源压力触发阈值 | `kubectl describe node` 查看 Conditions | 调整 eviction 阈值/扩容 |
| 探针失败重启 | 探针配置不合理 | `kubectl describe pod` 查看 Events | 调整 initialDelaySeconds/timeout |
| 磁盘压力 | 镜像/日志占满磁盘 | `df -h`; `du -sh /var/lib/containerd` | 清理镜像/调整 GC 阈值 |
| 证书过期 | 轮换失败 | `openssl x509 -dates` | 手动轮换/检查 CSR 审批 |

### 排查流程

```
kubelet 异常排查
├── 节点 NotReady？
│   ├── kubelet 服务运行？→ systemctl status kubelet
│   ├── 证书有效？→ openssl x509 -dates
│   ├── API Server 可达？→ curl -k https://<apiserver>:6443/healthz
│   └── 容器运行时正常？→ crictl info
├── Pod 异常？
│   ├── PLEG 错误 → 检查容器运行时/磁盘 IO
│   ├── 探针失败 → 检查应用健康端点/调整探针参数
│   └── 驱逐 → 检查节点资源/调整阈值
└── 性能问题？
    ├── CPU 高 → 检查 Pod 数量/探针频率
    ├── 内存高 → 检查 cAdvisor/日志缓冲
    └── 磁盘 IO → 检查镜像拉取/日志写入
```

## 生产案例

### 案例1：PLEG 不健康导致节点 NotReady

- **场景**：多个节点同时报 "PLEG is not healthy"，Pod 状态不更新
- **排查**：`journalctl -u kubelet` 显示 PLEG relist 超时；`iostat` 显示磁盘 IO 等待 90%+
- **方案**：升级存储驱动（HDD→SSD）；调整 containerd 并发数；设置 imageGCHighThresholdPercent=70 提前回收
- **效果**：PLEG relist 时间从 10s 降至 500ms，节点稳定

### 案例2：kubelet 证书过期导致节点失联

- **场景**：集群运行 1 年后多个节点突然 NotReady
- **排查**：`openssl x509 -in kubelet-client-current.pem -noout -dates` 显示证书已过期；CSR 未被审批
- **方案**：手动审批挂起的 CSR；确认 `--rotate-certificates=true`；设置证书过期监控告警
- **效果**：添加证书过期前 30 天告警，永不再发生

## 对比替代方案

| 组件 | 角色 | 与 kubelet 关系 |
|------|------|------|
| kubelet | 节点代理，管理 Pod 生命周期 | 核心组件 |
| virtual-kubelet | 虚拟节点，连接外部计算 | 替代 kubelet 的节点抽象 |
| KubeEdge edgecore | 边缘节点代理 | 边缘场景的 kubelet 替代 |
| k3s agent | 轻量节点代理 | 包含精简版 kubelet |

## 检查清单

- [ ] kubelet 服务正常运行且开机自启
- [ ] 证书轮换已启用 (rotateCertificates: true)
- [ ] cgroup driver 与容器运行时一致 (systemd)
- [ ] 驱逐阈值已配置且合理
- [ ] 系统资源预留已配置 (systemReserved/kubeReserved)
- [ ] 容器日志大小限制已配置
- [ ] kubelet 指标已接入 Prometheus
- [ ] 证书过期监控告警已配置

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/07-调度与资源/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — Container Runtime
- [[pod-lifecycle|Pod Lifecycle]]
- [[22-概念/07-调度与资源/resource-management.md|Resource Management]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[23-实体/02-K8s核心组件/container-runtime.md|Container Runtime]]

- 15-kubelet-deep-dive
- 33-kubelet-eviction-thresholds
- 20-kubelet-configuration
- [[19-故障诊断/04-高级排障/structural-02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- virtual-kubelet
- [[26-技能/03-节点/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/deployment/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/14-troubleshooting-guide|16 - Kubernetes 故障排查专家级指南]]
- [[01-集群基础/01-架构总览/16-kubernetes-core-components-v1.29-v1.33-update|Kubernetes 核心组件 v1.29 - v1.33 新特性速查]]
- [[01-集群基础/02-设计原则/01-design-principles-foundations|01 - Kubernetes 设计原则与哲学 (Foundations)]]
- [[01-集群基础/03-控制平面/02-plane-components-interaction|控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)]]
- [[02-工作负载/01-核心工作负载/18-node-management-operations|27 - 节点与节点池管理 (Node & NodePool Management)]]
- [[03-清单模式/01-YAML参考/32-lease-event-node|32 - Lease / Event / Node YAML 配置参考]]
- [[03-清单模式/01-YAML参考/34-component-configuration|34. Kubernetes 组件配置（Component Configuration）]]
- [[05-网络/01-K8s网络核心/13-dns-service-discovery|33 - 服务发现与 DNS 配置 (Service Discovery & DNS)]]
- [[05-网络/01-K8s网络核心/40-terway-gc-mechanism|38 - Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism)]]
- [[10-平台工程/02-运维/19-kubernetes-v1.33-platform-ops-guide|Kubernetes v1.29-v1.33 平台运维新特性指南]]
- [[10-平台工程/02-运维/09-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[10-平台工程/03-治理/02-performance-benchmarking-tuning|性能基准测试与调优 (Performance Benchmarking & Tuning)]]
- [[15-AI基础设施/02-AI-Agents/15-agent-corpus-gap-analysis|Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]]]
- [[15-AI基础设施/02-AI-Agents/49-openclaw-memory-mechanism|OpenClaw MEMORY.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/30-agent-harness-engineering|Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/11-cost-latency-optimization|成本与延迟优化策略 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/45-openclaw-user-mechanism|OpenClaw USER.md 机制深度解析 (AI基础设施)]]
- [[17-系统基础/02-硬件/18-hardware-failure-case-studies|硬件问题实战案例库]]
- [[17-系统基础/04-K8s事件/11-storage-volume-events|11 - 存储与卷事件]]
- [[17-系统基础/04-K8s事件/03-image-pull-events|03 - 镜像拉取事件]]
- [[17-系统基础/04-K8s事件/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[17-系统基础/04-K8s事件/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]]
- [[17-系统基础/04-K8s事件/05-scheduling-preemption-events|05 - 调度与抢占事件]]
- [[17-系统基础/04-K8s事件/01-event-system-architecture|01 - Kubernetes 事件系统架构与 API 参考]]
- [[17-系统基础/04-K8s事件/10-service-networking-events|10 - Service 与网络事件]]
- [[17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers|Resource Management for Pods and Containers]]
- [[17-系统基础/06-知识字典/configuration/liveness-readiness-and-startup-probes|Liveness, Readiness, and Startup Probes]]
- [[17-系统基础/06-知识字典/configuration/resource-management-for-windows-nodes|Resource Management for Windows nodes]]
- [[17-系统基础/06-知识字典/fundamentals/namespaces|命名空间]]
- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2|About cgroup v2（关于 cgroup v2）]]
- [[17-系统基础/06-知识字典/fundamentals/nodes|Nodes（节点）]]
- [[17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane|Communication between Nodes and the Control Plane（节点与控制平面之间的通信）]]
- [[17-系统基础/06-知识字典/fundamentals/garbage-collection|Garbage Collection（垃圾回收）]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/networking/dns-for-services-and-pods|DNS for Services and Pods]]
- [[17-系统基础/06-知识字典/networking/telco-cloud-and-5g-mec|电信云与 5G 多接入边缘计算（MEC）]]
- [[17-系统基础/06-知识字典/networking/ipv4-ipv6-dual-stack|IPv4/IPv6 dual-stack]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components|Kubernetes 系统组件指标]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/operations/swap-memory-management|Swap 内存管理]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[17-系统基础/06-知识字典/operations/node-autoscaling|节点自动扩缩容（Node Autoscaling）]]
- [[17-系统基础/06-知识字典/operations/certificates|Certificates（PKI 证书与要求）]]
- [[17-系统基础/06-知识字典/platform-engineering/api-priority-and-fairness|API 优先级与公平性（API Priority and Fairness）]]
- [[17-系统基础/06-知识字典/platform-engineering/dynamic-resource-allocation-good-practices|动态资源分配（DRA）集群管理员最佳实践]]
- [[17-系统基础/06-知识字典/scheduling/node-declared-features|Node Declared Features]]
- [[17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[17-系统基础/06-知识字典/scheduling/api-initiated-eviction|API-initiated Eviction]]
- [[17-系统基础/06-知识字典/security/service-accounts|服务账号]]
- [[17-系统基础/06-知识字典/security/hardening-guide---authentication-mechanisms|加固指南 - 认证机制]]
- [[17-系统基础/06-知识字典/security/process-id-limits-and-reservations|Process ID Limits And Reservations（进程 ID 限制与预留）]]
- [[17-系统基础/06-知识字典/security/kubernetes-api-server-bypass-risks|Kubernetes API Server 绕过风险]]
- [[17-系统基础/06-知识字典/security/node-resource-managers|Node Resource Managers（节点资源管理器）]]
- [[17-系统基础/06-知识字典/storage/ephemeral-volumes|Ephemeral Volumes（临时卷）]]
- [[17-系统基础/06-知识字典/storage/projected-volumes|Projected Volumes（投射卷）]]
- [[17-系统基础/06-知识字典/storage/local-ephemeral-storage|Local ephemeral storage（本地临时存储）]]
- [[17-系统基础/06-知识字典/storage/volumes|Volumes（卷）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring|Volume Health Monitoring（卷健康监控）]]
- [[17-系统基础/06-知识字典/tooling/cli-commands|查看所有 Pod 及其详细信息]]
- [[17-系统基础/06-知识字典/workloads/images|容器镜像（Images）]]
- [[17-系统基础/06-知识字典/workloads/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[17-系统基础/06-知识字典/workloads/pod-hostname|Pod Hostname]]
- [[17-系统基础/06-知识字典/workloads/container-lifecycle-hooks|容器生命周期钩子（Container Lifecycle Hooks）]]
- [[19-故障诊断/04-高级排障/structural-symptom-mapping-layer|症状快速映射层 (Symptom-SOP-RootCause Mapping) [topic-structural-trouble-shooting]]]
- [[19-故障诊断/06-FTA故障树/22-industry-standardization|第二十二章：行业标准化建议 (故障诊断)]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow|诊断工作流 / Diagnostic Workflow]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/root-cause-catalog|根因分类 / Root Cause Catalog]]
- [[22-概念/12-研究/kubernetes-version-evolution|Kubernetes 版本演进]]
- [[22-概念/15-运行时与系统/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]]
- [[26-技能/01-集群运维/cluster-upgrade/reference/skill-reference-version-matrix|Version Matrix]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion|kubeadm 集群删除操作]]
- [[26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism|kubelet 资源驱逐机制]]
- [[26-技能/03-节点/nodepool/nodepool-fta|NodePool 异常故障树分析 (skills)]]
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist|Day 1: 新人首日检查清单]]
- [[26-技能/04-工作负载/pod/培训/learn-README|新人上手快速路径（Quick Start）]]
- [[26-技能/04-工作负载/pod/培训/inner-training/week-1-ack-acr-lifecycle/day-7-cluster-certificate|Day 7: K8S 集群证书]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/week-1-foundation/day-6-k8s-cluster|Day 6: K8s 架构深化 + 集群配置]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz|K8S Fundamentals Quiz]]
- [[26-技能/04-工作负载/pod/安全/structural-03-pod-security-troubleshooting|Pod 安全与 SecurityContext 故障排查指南 [topic-structural-trouble-shooting]]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow|Diagnostic Workflow]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog|Root Cause Catalog]]
- [[26-技能/04-工作负载/pod/概念原理/字典-pod-lifecycle|Pod Lifecycle (concepts)]]
- [[26-技能/04-工作负载/pod/清单规范/01-pod-specification-complete|03 - Pod 完整规格说明书]]
- [[26-技能/04-工作负载/pod/清单规范/04-poddisruptionbudget-reference|28 - PodDisruptionBudget YAML 配置参考]]
- [[26-技能/04-工作负载/pod/生命周期与事件/01-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[26-技能/04-工作负载/pod/诊断排障/技能体系-02-pod-crashloop-oomkilled|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]
- [[26-技能/04-工作负载/pod/诊断排障/structural-01-pod-troubleshooting|Pod 故障排查与运行机制深度指南 [topic-structural-trouble-shooting]]]
- [[26-技能/04-工作负载/pod/调度/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[26-技能/04-工作负载/pod/调度/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[26-技能/04-工作负载/pod/调度/字典-pod-overhead|Pod Overhead]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/resource-management-for-pods-and-containers|Resource Management for Pods and Containers]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[26-技能/04-工作负载/pod/配置与字典/dns-for-services-and-pods|DNS for Services and Pods]]
- [[26-技能/04-工作负载/pod/配置与字典/pod-hostname|Pod Hostname]]
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage|存储故障排查]]
- [[26-技能/07-安全/rbac/培训/day-11-risk-assessment/01-risk-assessment-hands-on|Day 11: K8s 安全风险识别与防护实操]]
