---
title: KUDIG 命令 → 文档映射
description: '| `kubectl logs` | 查看容器日志 | [[故障诊断/README.md|README]]
  | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] |'
summary: '| `kubectl logs` | 查看容器日志 | [[故障诊断/README.md|README]]
  | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] |'
category: general
tags:
- k8s
- etcd
- prometheus
- helm
- hpa
- ingress
- rbac
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 命令 → 文档映射 是什么
- 如何 KUDIG 命令 → 文档映射
trigger_keywords:
- KUDIG
- 命令
- 文档映射
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: KUDIG 命令 → 文档映射
description: KUDIG 命令 → 文档映射
category: docs
tags:
- k8s
- command
- mapping
relationships:
- target: "[[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]"
  type: related_to
- target: "[[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]"
  type: related_to
- target: "[[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]"
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

# KUDIG 命令 → 文档映射

> 创建时间: 2026-05-20
> 用途: 为 Agent 工具调用建立命令到文档的映射

---

## kubectl 核心命令映射

| 命令 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `kubectl get pods` | 查看 Pod 状态 | [[工作负载/核心工作负载/11-pod-lifecycle-events.md|11 pod lifecycle events]] | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] |
| `kubectl describe pod` | Pod 详情和事件 | [[工作负载/核心工作负载/11-pod-lifecycle-events.md|11 pod lifecycle events]] | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] |
| `kubectl logs` | 查看容器日志 | [[故障诊断/README.md|README]] | [[故障诊断/FTA故障树/list/pod-fta.md|pod fta]] |
| `kubectl exec` | 进入容器 | [[工作负载/核心工作负载/11-pod-lifecycle-events.md|11 pod lifecycle events]] | - |
| `kubectl get events` | 查看集群事件 | [[系统基础/README.md|README]] | - |
| `kubectl get nodes` | 节点状态 | [[故障诊断/README.md|README]] | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] |
| `kubectl describe node` | 节点详情 | [[故障诊断/README.md|README]] | [[故障诊断/FTA故障树/list/node-fta.md|node fta]] |
| `kubectl rollout status` | 滚动更新状态 | 工作负载/02-deployment-production-patterns | [[故障诊断/FTA故障树/list/deployment-fta.md|deployment fta]] |
| `kubectl rollout undo` | 回滚 | 集群基础/18-upgrade-migration-strategy | [[故障诊断/FTA故障树/list/cluster-upgrade-fta.md|cluster upgrade fta]] |
| `kubectl apply -f` | 应用配置 | [[清单模式/README.md|README]] | - |
| `kubectl delete` | 删除资源 | [[清单模式/README.md|README]] | - |
| `kubectl port-forward` | 端口转发 | [[网络/README.md|README]] | [[故障诊断/FTA故障树/list/service-fta.md|service fta]] |
| `kubectl scale` | 扩缩容 | 工作负载/02-deployment-production-patterns | [[故障诊断/FTA故障树/list/hpa-fta.md|hpa fta]] |
| `kubectl top nodes` | 节点资源使用 | [[可观测性/README.md|README]] | - |
| `kubectl top pods` | Pod 资源使用 | [[可观测性/README.md|README]] | - |
| `kubectl auth can-i` | 权限检查 | [[安全/README.md|README]] | - |
| `kubectl get pv/pvc` | 存储状态 | [[存储/README.md|README]] | [[故障诊断/FTA故障树/list/csi-fta.md|csi fta]] |
| `kubectl get ingress` | Ingress 状态 | [[网络/README.md|README]] | [[故障诊断/FTA故障树/list/ingress-fta.md|ingress fta]] |
| `kubectl get svc` | Service 状态 | [[网络/README.md|README]] | [[故障诊断/FTA故障树/list/service-fta.md|service fta]] |
| `kubectl get configmap/secret` | 配置和密钥 | [[安全/README.md|README]] | [[故障诊断/FTA故障树/list/rbac-fta.md|rbac fta]] |

## etcdctl 命令映射

| 命令 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `etcdctl member list` | 成员列表 | 集群基础/11-etcd-deep-dive | [[故障诊断/FTA故障树/list/etcd-fta.md|etcd fta]] |
| `etcdctl endpoint health` | 健康检查 | 集群基础/11-etcd-deep-dive | [[故障诊断/FTA故障树/list/etcd-fta.md|etcd fta]] |
| `etcdctl snapshot save` | 备份 | [[可靠性/README.md|README]] | [[故障诊断/FTA故障树/list/backup-restore-fta.md|backup restore fta]] |
| `etcdctl snapshot restore` | 恢复 | [[可靠性/README.md|README]] | [[故障诊断/FTA故障树/list/backup-restore-fta.md|backup restore fta]] |

## 监控命令映射

| 命令 | 用途 | 参考文档 |
|---|---|---|
| `promtool check rules` | PromQL 规则检查 | [[可观测性/README.md|README]] |
| `promtool check config` | Prometheus 配置检查 | [[可观测性/README.md|README]] |
| `amtool check-config` | Alertmanager 配置检查 | [[可观测性/README.md|README]] |

## Helm 命令映射

| 命令 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `helm install` | 安装 Chart | [[系统基础/速查卡/helm.md|helm]] | [[故障诊断/FTA故障树/list/helm-fta.md|helm fta]] |
| `helm upgrade` | 升级 Chart | [[系统基础/速查卡/helm.md|helm]] | [[故障诊断/FTA故障树/list/helm-fta.md|helm fta]] |
| `helm rollback` | 回滚 | [[系统基础/速查卡/helm.md|helm]] | [[故障诊断/FTA故障树/list/helm-fta.md|helm fta]] |
| `helm list` | 列出 Release | [[系统基础/速查卡/helm.md|helm]] | - |
| `helm template` | 渲染模板 | [[系统基础/速查卡/helm.md|helm]] | - |

---

*本文档是命令映射的权威来源，新增命令时应注册。*

---

## Related

- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]


<!-- risk-assessed -->
