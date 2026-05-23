---
title: KUDIG 命令 → 文档映射
description: '| `kubectl logs` | 查看容器日志 | [[domain-10-troubleshooting-diagnostics/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]]
  |'
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
created: "2026-05-23"
---

---
title: KUDIG 命令 → 文档映射
description: KUDIG 命令 → 文档映射
category: docs
tags:
- k8s
- command
- mapping
relationships:
- target: '[[skills/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]'
  type: related_to
- target: '[[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]'
  type: related_to
- target: '[[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]'
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
| `kubectl get pods` | 查看 Pod 状态 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]] |
| `kubectl describe pod` | Pod 详情和事件 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]] |
| `kubectl logs` | 查看容器日志 | [[domain-10-troubleshooting-diagnostics/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta]] |
| `kubectl exec` | 进入容器 | [[domain-02-workloads-applications/00-core-workloads/11-pod-lifecycle-events]] | - |
| `kubectl get events` | 查看集群事件 | [[domain-17-system-foundation/README]] | - |
| `kubectl get nodes` | 节点状态 | [[domain-10-troubleshooting-diagnostics/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta]] |
| `kubectl describe node` | 节点详情 | [[domain-10-troubleshooting-diagnostics/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta]] |
| `kubectl rollout status` | 滚动更新状态 | domain-02-workloads-applications/02-deployment-production-patterns | [[domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta]] |
| `kubectl rollout undo` | 回滚 | domain-01-cluster-fundamentals/18-upgrade-migration-strategy | [[domain-10-troubleshooting-diagnostics/topic-fta/list/cluster-upgrade-fta]] |
| `kubectl apply -f` | 应用配置 | [[domain-18-manifests-patterns/README]] | - |
| `kubectl delete` | 删除资源 | [[domain-18-manifests-patterns/README]] | - |
| `kubectl port-forward` | 端口转发 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta]] |
| `kubectl scale` | 扩缩容 | domain-02-workloads-applications/02-deployment-production-patterns | [[domain-10-troubleshooting-diagnostics/topic-fta/list/hpa-fta]] |
| `kubectl top nodes` | 节点资源使用 | [[domain-06-observability/README]] | - |
| `kubectl top pods` | Pod 资源使用 | [[domain-06-observability/README]] | - |
| `kubectl auth can-i` | 权限检查 | [[domain-05-security-compliance/README]] | - |
| `kubectl get pv/pvc` | 存储状态 | [[domain-04-storage-data/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta]] |
| `kubectl get ingress` | Ingress 状态 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta]] |
| `kubectl get svc` | Service 状态 | [[domain-03-networking-traffic/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta]] |
| `kubectl get configmap/secret` | 配置和密钥 | [[domain-05-security-compliance/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/rbac-fta]] |

## etcdctl 命令映射

| 命令 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `etcdctl member list` | 成员列表 | domain-01-cluster-fundamentals/11-etcd-deep-dive | [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta]] |
| `etcdctl endpoint health` | 健康检查 | domain-01-cluster-fundamentals/11-etcd-deep-dive | [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta]] |
| `etcdctl snapshot save` | 备份 | [[domain-09-reliability-engineering/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta]] |
| `etcdctl snapshot restore` | 恢复 | [[domain-09-reliability-engineering/README]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta]] |

## 监控命令映射

| 命令 | 用途 | 参考文档 |
|---|---|---|
| `promtool check rules` | PromQL 规则检查 | [[domain-06-observability/README]] |
| `promtool check config` | Prometheus 配置检查 | [[domain-06-observability/README]] |
| `amtool check-config` | Alertmanager 配置检查 | [[domain-06-observability/README]] |

## Helm 命令映射

| 命令 | 用途 | 参考文档 | FTA |
|---|---|---|---|
| `helm install` | 安装 Chart | [[domain-17-system-foundation/topic-cheat-sheet/helm]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/helm-fta]] |
| `helm upgrade` | 升级 Chart | [[domain-17-system-foundation/topic-cheat-sheet/helm]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/helm-fta]] |
| `helm rollback` | 回滚 | [[domain-17-system-foundation/topic-cheat-sheet/helm]] | [[domain-10-troubleshooting-diagnostics/topic-fta/list/helm-fta]] |
| `helm list` | 列出 Release | [[domain-17-system-foundation/topic-cheat-sheet/helm]] | - |
| `helm template` | 渲染模板 | [[domain-17-system-foundation/topic-cheat-sheet/helm]] | - |

---

*本文档是命令映射的权威来源，新增命令时应注册。*

---

## Related

- [[skills/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]
