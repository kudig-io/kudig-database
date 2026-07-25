---
title: Kubernetes FTA Top Events Index
description: Kubernetes FTA Top Events Index — Kubernetes 生产运维知识库
summary: Kubernetes FTA Top Events Index — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- troubleshooting
- top-events
- index
- etcd
- kubelet
- scheduler
- flannel
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes FTA Top Events Index 是什么
- 如何 Kubernetes FTA Top Events Index
trigger_keywords:
- Kubernetes
- FTA
- Top
- Events
- Index
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes FTA Top Events Index

## Top Events Overview (TE-1 through TE-16)

| ID | Top Event | Severity | Impact | Typical Symptoms | ACK-Specific |
|----|-----------|----------|--------|-----------------|-------------|
| TE-1 | Cluster completely unavailable | P0 | Entire cluster | kubectl cannot connect, all services down | ECS/ESSD/SLB |
| TE-2 | Application [[Service|service]] unavailable | P0 | Specific app | HTTP 5xx errors, users cannot access | ASM/ARMS/Terway |
| TE-3 | Pod startup failure | P1 | Specific Pod | Pod in Pending/Error state | ACK scheduler |
| TE-4 | Network communication anomaly | P1 | Network layer | DNS failure, Pod communication broken | Terway ENI/IPVLAN |
| TE-5 | Storage access failure | P1 | Storage layer | PVC bind failure, volume mount failure | OSS/CSI/NAS |
| TE-6 | Resource scheduling anomaly | P2 | Scheduling | Pod unschedulable, scheduler abnormal | ACK resource quota |
| TE-7 | Security authentication failure | P1 | Security | Auth/authz failure, certificate expiry | ACK RAM/PSP |
| TE-8 | Monitoring/alerting anomaly | P2 | Observability | Metrics missing, alerts not firing | ARMS/MSP |
| TE-9 | Terway network failure | P1 | Pod network | Pod cannot get IP/network unreachable | Terway exclusive |
| TE-10 | ASM service mesh failure | P1 | Mesh traffic | Sidecar cannot connect, mTLS failure | ASM exclusive |
| TE-11 | ACK-One multi-cluster anomaly | P1 | Multi-cluster | Cluster registration failure, config sync delay | ACK-One exclusive |
| TE-12 | Resource quota exceeded | P2 | Account level | API object creation failure, quota exhausted | ACK exclusive |
| TE-13 | Change management failure | P1 | Change process | Upgrade failure, rollback, config drift | GitOps/RAC |
| TE-14 | Capacity planning failure | P2 | Resource capacity | Node resource exhausted, storage capacity insufficient | Auto-scaling |
| TE-15 | Disaster recovery failure | P0 | Business continuity | Backup/restore failure, DR drill failure | Backup/DR |
| TE-16 | Observability integrity missing | P2 | Monitoring blindspot | Key metrics missing, trace broken | OTel/observability |

## Detailed Fault Tree: TE-1 Cluster Unavailable

```
# 🟢 低风险：只读/信息收集，通常无副作用
TE-1: Cluster completely unavailable [OR gate] P0
│
├── IE-1.1 Control plane failure [OR gate]
│   ├── BE-1.1 API Server failure
│   │   ├── API Server OOM (from etcd data volume pressure)
│   │   ├── API Server certificate expiry
│   │   ├── API Server network unreachable (SLB/security group)
│   │   ├── API Server startup parameter error
│   │   └── API Server dependency failure
│   ├── BE-1.2 etcd cluster failure
│   │   ├── etcd disk space exhausted (ESSD downgrade, snapshot accumulation)
│   │   ├── etcd quorum lost (network partition, split-brain)
│   │   ├── etcd data corruption (ESSD write-back inconsistency)
│   │   ├── etcd performance degraded (WAL log accumulation)
│   │   └── etcd certificate issues
│   ├── BE-1.3 Scheduler failure
│   └── BE-1.4 Controller Manager failure
│
├── IE-1.2 Worker node batch failure [AND gate - needs majority]
│   ├── BE-1.5 Kubelet service failure (OOM, API disconnect, cert expiry)
│   ├── BE-1.6 Container runtime failure (containerd, Docker, CNI)
│   └── BE-1.7 Node network failure
│
├── IE-1.3 Network infrastructure failure [OR gate]
│   ├── BE-1.8 CNI plugin failure (Terway ENI/IPVLAN, Flannel)
│   └── BE-1.9 Core network device failure (switch, NAT gateway)
│
└── IE-1.4 Cloud IaaS layer failure [OR gate] (ACK-specific)
    ├── BE-1.10 ECS instance batch failure (spot interruption, ENI disconnect, hardware)
    ├── BE-1.11 SLB failure (backend server health, listener config)
    └── BE-1.12 VPC network failure (route table, security group)
```
## Detailed Fault Tree: TE-2 Application Unavailable

```
TE-2: Application service unavailable [OR gate] P0
├── IE-2.1 Pod runtime anomaly [OR gate]
│   ├── BE-2.1 CrashLoopBackOff
│   ├── BE-2.2 ImagePullBackOff
│   ├── BE-2.3 OOMKilled
│   └── BE-2.4 Evicted
├── IE-2.2 Service/Endpoint anomaly [OR gate]
├── IE-2.3 Ingress/IngressController anomaly [OR gate]
├── IE-2.4 ASM service mesh failure [OR gate]
└── IE-2.5 ARMS application monitoring failure [OR gate]
```

## OOMKilled Bottom Event Structure (Reference)

Each bottom event in the FTA includes structured diagnostic information:

```yaml
bottom_event:
  id: "BE-2.3"
  name: "OOMKilled"
  description: "Container terminated by Linux OOM Killer due to memory exceeding limits"

  observable:
    metrics:
      - "container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.95"
    logs:
      - "OOMKilled in container"
      - "Exit Code: 137"
    events:
      - "kubectl get events --field-selector reason=OOMKilling"

  root_causes:
    - "Application memory leak"
    - "JVM heap size set too large"
    - "Resource limits set too low"
    - "Traffic spike causing memory surge"
    - "Sidecar container memory not accounted"

  diagnosis_commands:
    - "kubectl describe pod <pod> | grep -A5 'Last State'"
    - "kubectl top pod <pod> --containers"
    - "kubectl logs <pod> --previous"

  healing_actions:
    - id: "HA-2.3.1"
      description: "Increase memory limits"
      risk: "low"
      auto_healable: true
    - id: "HA-2.3.2"
      description: "Analyze memory leak"
      risk: "none"
      auto_healable: false
```

## FTA 顶事件索引表

### 按严重级别分类

| 级别 | 顶事件 | FTA 文件 | 平均修复时间 |
|------|--------|----------|--------------|
| P0 | 集群完全不可用 | kubeadm-fta, certificate-fta | 30-60min |
| P0 | 核心服务中断 | service-fta, ingress-fta | 15-30min |
| P1 | Pod 批量异常 | pod-fta, deployment-fta | 10-20min |
| P1 | 节点 NotReady | node-fta, nodepool-fta | 15-30min |
| P1 | 网络分区 | terway-fta, flannel-fta | 20-40min |
| P2 | 性能下降 | hpa-fta, monitoring-fta | 30-60min |
| P2 | 存储异常 | csi-fta, statefulset-fta | 20-40min |

### 按发生频率排序

| 排名 | 顶事件 | 频率 | 典型根因 |
|------|--------|------|----------|
| 1 | Pod CrashLoopBackOff | 35% | 配置错误/资源不足 |
| 2 | 节点 NotReady | 20% | kubelet/证书/网络 |
| 3 | Service 不可达 | 15% | selector/Endpoints |
| 4 | PVC 挂载失败 | 10% | CSI/配额 |
| 5 | HPA 失效 | 8% | metrics-server |
| 6 | 证书过期 | 7% | 未配置自动轮换 |
| 7 | 其他 | 5% | - |

### FTA 使用建议

1. **快速定位**: 根据症状匹配顶事件，进入对应 FTA
2. **树遍历**: 按诊断命令速查表逐步执行
3. **根因确认**: 到达叶节点后验证根因
4. **修复执行**: 按风险等级选择修复方案

## Related

- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[26-技能/04-工作负载/pod/方法论/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[22-概念/01-核心架构/etcd Operational Reference.md|etcd Operational Reference]]
- [[37-归档/troubleshooting-diagnostics/kubernetes-fault-distribution-and-mttr-en.md|Kubernetes Fault Distribution and MTTR]]
- [[29-文档/indexes/ERROR-FTA-MAP.md|KUDIG 错误码 → FTA 映射]] — Cross-reference
- [[29-文档/indexes/COMMAND-DOC-MAP.md|KUDIG 命令 → 文档映射]] — Cross-reference
- [[29-文档/indexes/API-DOC-MAP.md|KUDIG API → 文档映射]] — Cross-reference


<!-- risk-assessed -->
