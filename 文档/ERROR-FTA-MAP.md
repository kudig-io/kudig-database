---
title: KUDIG 错误码 → FTA 映射
description: '| 错误状态 | 含义 | FTA | 快速排查 |'
summary: '| 错误状态 | 含义 | FTA | 快速排查 |'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- docker
- ingress
- gateway
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 错误码 → FTA 映射 是什么
- 如何 KUDIG 错误码 → FTA 映射
trigger_keywords:
- KUDIG
- 错误码
- FTA
- 映射
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: KUDIG 错误码 → FTA 映射
description: KUDIG 错误码 → FTA 映射
category: docs
tags:
- k8s
- fta
- error-code
- mapping
relationships:
- target: "[[技能/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]"
  type: related_to
- target: "[[技能/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]"
  type: related_to
- target: "[[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]"
  type: related_to
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# KUDIG 错误码 → FTA 映射

> 创建时间: 2026-05-20
> 用途: 为 Agent 建立常见错误码到 FTA 问题树的映射

---

## Pod 相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `CrashLoopBackOff` | 容器反复崩溃重启 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | `kubectl logs <pod>` → `kubectl describe pod <pod>` |
| `ImagePullBackOff` | 镜像拉取失败 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | `kubectl describe pod` → 检查 image 和 secret |
| `ErrImagePull` | 镜像拉取错误 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | `docker pull <image>` 手动测试 |
| `CreateContainerConfigError` | 容器配置错误 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | 检查 ConfigMap/Secret 是否存在 |
| `InvalidImageName` | 镜像名无效 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | 检查 image 字段格式 |
| `RunContainerError` | 容器运行错误 | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] | `kubectl describe pod` → 查看事件 |

## 调度相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `Pending` | Pod 等待调度 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `kubectl describe pod` → 查看调度事件 |
| `Unschedulable` | 无法调度 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | 检查节点资源、亲和性、污点 |
| `FailedScheduling` | 调度失败 | [[故障诊断/FTA故障树/list/scheduler-fta.md|scheduler fta]] | `kubectl describe pod` 查看原因 |

## 节点相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `NotReady` | 节点不健康 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `kubectl describe node` → 检查 kubelet |
| `NodeLost` | 节点失联 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | SSH 到节点 → 检查网络和服务 |
| `MemoryPressure` | 内存压力 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `free -m` → 检查大内存 Pod |
| `DiskPressure` | 磁盘压力 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `df -h` → 清理无用镜像和日志 |
| `PIDPressure` | PID 耗尽 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `cat /proc/sys/kernel/pid_max` |
| `Evicted` | Pod 被驱逐 | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] | `kubectl describe pod` → 查看驱逐原因 |

## 网络相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `Connection refused` | 连接被拒绝 | [[故障诊断/FTA故障树/list/service-fta.md|service fta]] | `kubectl get endpoints` → 检查 Pod 是否在运行 |
| `DNS lookup failed` | DNS 解析失败 | [[故障诊断/FTA故障树/list/dns-fta.md|dns fta]] | `nslookup` → 检查 CoreDNS |
| `Timeout` | 连接超时 | [[故障诊断/FTA故障树/list/networkpolicy-fta.md|networkpolicy fta]] | `kubectl exec` → `curl` 测试 |
| `502 Bad Gateway` | 网关错误 | [[故障诊断/FTA故障树/list/ingress-fta.md|ingress fta]] | 检查 Ingress 和后端 Pod |
| `503 Service Unavailable` | 服务不可用 | [[故障诊断/FTA故障树/list/service-fta.md|service fta]] | 检查 Endpoints 是否有后端 |

## 存储相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `Pending PVC` | PVC 未绑定 | [[故障诊断/FTA故障树/list/csi-fta.md|csi fta]] | `kubectl describe pvc` → 检查 StorageClass |
| `VolumeMount failed` | 挂载失败 | [[故障诊断/FTA故障树/list/csi-fta.md|csi fta]] | `kubectl describe pod` → 检查 PV 状态 |
| `ReadOnlyFilesystem` | 只读文件系统 | [[故障诊断/FTA故障树/list/csi-fta.md|csi fta]] | 检查 accessModes 和节点磁盘 |
| `NodePublishVolume failed` | CSI 挂载失败 | [[故障诊断/FTA故障树/list/csi-fta.md|csi fta]] | 检查 CSI Driver 和存储后端 |

## 控制平面错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `connection refused:6443` | API Server 不可用 | [[故障诊断/FTA故障树/list/apiserver-fta.md|apiserver fta]] | 检查 apiserver 进程和证书 |
| `etcd cluster unavailable` | etcd 不可用 | [[故障诊断/FTA故障树/list/etcd-fta.md|etcd fta]] | `etcdctl endpoint health` |
| `leader election lost` | Leader 选举失败 | [[故障诊断/FTA故障树/list/controller-manager-fta.md|controller manager fta]] | 检查网络分区和时钟同步 |

## 证书相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `x509: certificate has expired` | 证书过期 | [[故障诊断/FTA故障树/list/certificate-fta.md|certificate fta]] | `kubeadm certs check-expiration` |
| `TLS handshake error` | TLS 握手失败 | [[故障诊断/FTA故障树/list/certificate-fta.md|certificate fta]] | 检查证书链和 CA |

## 安全相关错误

| 错误状态 | 含义 | FTA | 快速排查 |
|---|---|---|---|
| `forbidden: User cannot` | RBAC 权限不足 | - | `kubectl auth can-i` → 检查 RoleBinding |
| `Secret not found` | Secret 不存在 | [[故障诊断/FTA故障树/list/rbac-fta.md|rbac fta]] | `kubectl get secret` → 检查创建 |

---

## 使用方式

Agent 在检测到错误码时，按以下流程路由:
1. 查表找到对应 FTA 文档
2. 读取 FTA 中的决策树快速定位
3. 执行推荐的诊断命令
4. 根据诊断结果选择修复方案

---

*本文档是错误码映射的权威来源，新增错误类型时应注册。*

---

## Related

- [[技能/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[技能/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]


<!-- risk-assessed -->
