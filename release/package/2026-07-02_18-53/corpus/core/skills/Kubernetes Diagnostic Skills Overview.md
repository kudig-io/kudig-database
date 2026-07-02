---
title: Kubernetes Diagnostic Skills Overview
description: Kubernetes Diagnostic Skills Overview — Kubernetes 生产运维知识库
summary: Kubernetes Diagnostic Skills Overview — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- troubleshooting
- skills
- sop
- diagnosis
- etcd
- kubelet
- prometheus
- istio
- coredns
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Diagnostic Skills Overview 是什么
- 如何 Kubernetes Diagnostic Skills Overview
trigger_keywords:
- Kubernetes
- Diagnostic
- Skills
- Overview
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- etcd-basics
- tls-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Diagnostic Skills Overview

## Diagnostic Skills Catalog

The KUDIG knowledge base defines 19 structured diagnostic skills, each mapping to a specific Kubernetes operational scenario. All skills are rated **advanced** difficulty with 20-40 minute estimated reading time.

### Core Infrastructure Skills

| # | [[SKILL|Skill]] | FTA Mapping | Est. MTTR | Key Diagnostic Commands |
|---|-------|-------------|-----------|------------------------|
| 1 | Node NotReady | TE-1 -> IE-1.2 -> BE-1.5 | 15-30m | `kubectl get nodes`, `describe node`, `journalctl -u [[kubelet|kubelet]]` |
| 2 | Pod CrashLoopBackOff/OOMKilled | TE-2 -> IE-2.1 -> BE-2.1/2.3 | 10-25m | `describe pod`, `logs --previous`, `kubectl top pod` |
| 3 | Pod Pending | TE-3 -> IE-3.1 | 15-30m | `describe pod | grep Events`, `describe nodes | grep Allocated` |
| 4 | DNS Resolution Failure | TE-4 -> IE-4.1 | 20-40m | `nslookup`, `dig`, `[[CoreDNS|coredns]] logs`, `kubectl get ep kube-dns` |
| 5 | Service Connectivity | TE-2 -> IE-2.2 | 20-40m | `kubectl get ep`, `iptables-save`, `kube-proxy logs` |

### Storage and Configuration Skills

| # | Skill | FTA Mapping | Est. MTTR | Key Diagnostic Commands |
|---|-------|-------------|-----------|------------------------|
| 6 | Certificate Expiry/TLS Failure | TE-7 -> IE-7.1 | 10-30m | `openssl x509 -enddate`, `cert-manager logs` |
| 7 | PVC/PV/CSI Storage Failure | TE-5 -> IE-5.1/5.2 | 10-25m | `kubectl get pvc`, `describe pvc`, `csi driver logs` |
| 8 | Deployment Rollout/Rollback Failure | TE-2 -> IE-2.1 | 10-25m | `kubectl rollout status`, `kubectl rollout history` |
| 9 | RBAC/ResourceQuota Failure | TE-7 -> IE-7.2 | 10-20m | `kubectl auth can-i`, `kubectl describe resourcequota` |
| 10 | Image Pull/Registry Failure | TE-3 -> IE-3.2 | 10-20m | `describe pod | grep ImagePull`, `docker pull debug` |

### Advanced Operations Skills

| # | Skill | FTA Mapping | Est. MTTR | Key Diagnostic Commands |
|---|-------|-------------|-----------|------------------------|
| 11 | etcd/Control Plane Failure | TE-1 -> IE-1.1 -> BE-1.2 | 15-25m | `etcdctl endpoint health`, `etcdctl member list` |
| 12 | HPA/VPA/Autoscaler Failure | TE-6 -> IE-6.2 | 10-20m | `kubectl get hpa`, `kubectl describe hpa`, `metrics-server logs` |
| 13 | Ingress/Gateway Failure | TE-2 -> IE-2.3 | 10-20m | `kubectl get ingress`, `nginx/istio-ingress logs` |
| 14 | ConfigMap/Secret Failure | TE-3 -> IE-3.3 | 10-20m | `kubectl get cm/secret`, `describe pod | grep MountVolume` |
| 15 | Monitoring/Alerting Failure | TE-8 -> IE-8.1/8.2 | 10-20m | `prometheus targets`, `alertmanager status` |
| 16 | Logging Pipeline Failure | TE-8 -> IE-8.3 | 10-20m | `fluentd/elasticsearch pods`, `log pipeline health` |

### Performance and Security Skills

| # | Skill | FTA Mapping | Est. MTTR | Key Diagnostic Commands |
|---|-------|-------------|-----------|------------------------|
| 17 | Performance Bottleneck/Tuning | TE-2 -> TE-6 | 10-20m | `kubectl top`, `prometheus queries`, `pprof` |
| 18 | Security Incident Response | TE-7 | 10-20m | `kubectl audit logs`, `network policy check`, `forensics` |
| 19 | Skill Local Demo Guide | N/A | 5m | Local skill execution testing |

## Skill Structure

Each diagnostic skill follows a standard structure:

1. **Skill Overview**: Trigger conditions, impact scope, severity classification
2. **Quick Decision Tree**: 3-step diagnostic flowchart to narrow down root cause
3. **Diagnostic Commands**: Specific commands with expected output and interpretation
4. **Root Cause Catalog**: Common causes ranked by probability
5. **Remediation Playbook**: Step-by-step fix procedures with risk assessment
6. **Escalation Path**: When and how to escalate to higher-level support
7. **Version Matrix**: K8s version compatibility and known version-specific issues

## Integration with FTA

Diagnostic skills are the **operationalized form** of FTA bottom events. Each skill:
- Maps to specific FTA paths (TE -> IE -> BE)
- Provides the `observable` data (metrics, logs, events) for each BE
- Implements the `diagnosis_commands` and `healing_actions` defined in FTA
- Feeds execution results back to the [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]] learning loop

## Related

- [[cert-manager]] — cert-manager
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[skills/skill-reference-remediation-playbook.md|skill-reference-remediation-playbook]] — Remediation Playbook
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[entities/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]]
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[entities/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]
- [[entities/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]


<!-- risk-assessed -->
