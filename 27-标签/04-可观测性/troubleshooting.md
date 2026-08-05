---
title: troubleshooting
description: 故障诊断标签枢纽 — 涵盖 Kubernetes 全栈故障排查方法论、FTA/FEBM 结构化诊断、排障工具链、生产案例与技能体系的完整知识索引
category: tag-index
tags:
- troubleshooting
- debugging
- diagnostics
- incident-response
- root-cause-analysis
tier: core
difficulty: intermediate-to-advanced
domain: fault-diagnosis
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-21'
last_updated: '2026-07-21'
---

# troubleshooting Tag Hub

> 故障诊断领域页面 — FTA 故障树分析、FEBM 基于证据的排障方法论、结构化诊断流程、排障工具链、生产故障案例等。

## 核心定义

**故障诊断（Troubleshooting）** 是在 Kubernetes 生产环境中识别、定位和解决系统异常的系统化过程。它涵盖从症状发现、证据收集、根因分析到修复验证的完整闭环，是 SRE 和平台工程师的核心能力。

### 诊断方法论矩阵

| 方法论 | 全称 | 核心思想 | 适用场景 |
|--------|------|----------|----------|
| FTA | Fault Tree Analysis | 自顶向下分解故障树，逐层排除 | 复杂多因素故障、系统性问题 |
| FEBM | Fault-Evidence-Based Method | 基于证据链的假设-验证循环 | 不确定根因的疑难故障 |
| 二分法 | Binary Search | 逐层缩小故障范围 | 网络连通性、调用链问题 |
| 对比法 | Differential Diagnosis | 对比正常/异常环境差异 | 配置变更引发的故障 |
| 时间线法 | Timeline Analysis | 按时间线还原事件序列 | 突发性能退化、级联故障 |

### 故障分类体系

| 故障层级 | 典型症状 | 影响范围 |
|---------|----------|----------|
| 集群级 | API Server 不可用、etcd 异常 | 全集群不可用 |
| 节点级 | Node NotReady、kubelet 异常 | 单节点工作负载受影响 |
| 网络级 | DNS 解析失败、Service 不通 | 服务间通信中断 |
| 存储级 | PVC Pending、IO 延迟高 | 有状态应用异常 |
| 应用级 | CrashLoopBackOff、OOMKilled | 单应用不可用 |
| 性能级 | 延迟升高、吞吐下降 | 用户体验退化 |

## 诊断流程框架

### 标准排障五步法

```
1. 发现症状 → 2. 收集证据 → 3. 形成假设 → 4. 验证假设 → 5. 修复验证
     ↑                                                    │
     └────────────── 未解决则迭代 ←────────────────────────┘
```

### 关键诊断命令

```bash
# 第一步：全局健康检查
kubectl get nodes -o wide
kubectl get pods -A --field-selector=status.phase!=Running
kubectl get events -A --sort-by='.lastTimestamp' | tail -30

# 第二步：Pod 级诊断
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous --all-containers
kubectl exec -it <pod> -n <ns> -- cat /proc/1/status

# 第三步：网络诊断
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash
nslookup <service>.<namespace>.svc.cluster.local
curl -v http://<service>:<port>/healthz

# 第四步：资源分析
kubectl top pods -n <ns> --sort-by=memory
kubectl describe node <node> | grep -A 10 "Allocated resources"

# 第五步：控制平面诊断
kubectl get --raw /healthz?verbose
kubectl logs -n kube-system -l component=kube-apiserver --tail=50
etcdctl endpoint health --cluster
```

## 故障树分析 (FTA)

- [[19-故障诊断/06-FTA故障树/README|FTA 故障树方法论索引]]
- [[19-故障诊断/06-FTA故障树/fta-methodology-and-agentic-practices|FTA 方法论概览]]
- [[19-故障诊断/06-FTA故障树/list/pod-fta|Pod 故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/node-fta|节点故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/networkpolicy-fta|网络故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/csi-fta|存储故障树分析]]

## FEBM 基于证据的排障

- [[19-故障诊断/07-FEBM方法论/README|FEBM 方法论索引]]
- [[19-故障诊断/07-FEBM方法论/01-febm-theory-foundations|FEBM 方法论概览]]
- [[19-故障诊断/07-FEBM方法论/02-febm-technical-implementation|证据收集指南]]
- [[19-故障诊断/07-FEBM方法论/01-febm-theory-foundations|假设验证流程]]

## 核心排障 (Core Troubleshooting)

- [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting|Pod 生命周期排障]]
- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis|调度与资源排障]]
- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting|CNI 网络排障]]
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting|存储 CSI 排障]]
- [[19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting|控制平面排障]]

## 资源排障 (Resource Troubleshooting)

- [[19-故障诊断/02-资源排障/01-node-comprehensive-troubleshooting|节点综合排障]]
- [[19-故障诊断/02-资源排障/02-service-comprehensive-troubleshooting|Service 综合排障]]
- [[19-故障诊断/02-资源排障/06-pvc-storage-troubleshooting|PVC 存储排障]]
- [[19-故障诊断/02-资源排障/08-networkpolicy-troubleshooting|NetworkPolicy 排障]]

## 基础设施排障 (Infrastructure Troubleshooting)

- [[19-故障诊断/03-基础设施排障/01-network-connectivity-troubleshooting|网络连通性排障]]
- [[19-故障诊断/03-基础设施排障/08-security-troubleshooting|安全故障排查]]

## 高级排障 (Advanced Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-03-networking/04-networkpolicy-troubleshooting|高级网络策略排障]]
- [[19-故障诊断/04-高级排障/structural-04-storage/04-storage-performance-troubleshooting|存储性能排障]]
- [[19-故障诊断/04-高级排障/structural-04-storage/05-storageclass-troubleshooting|StorageClass 排障]]
- [[19-故障诊断/04-高级排障/structural-06-security-auth/03-pod-security-troubleshooting|Pod 安全排障]]
- [[19-故障诊断/04-高级排障/structural-12-monitoring-observability/01-monitoring-observability-troubleshooting|监控可观测性排障]]

## 技能体系 (Skill System)

- [[19-故障诊断/08-技能体系/skill-set/k8s-pod-crashloop/SKILL-DEEP-DIVE|Pod CrashLoopBackOff 深度解析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE|K8s Node NotReady 深度解析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-rbac-quota/DIALOGUE|RBAC 权限问题对话]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-security-incident/DIALOGUE|安全事件响应对话]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-certificate-expiry/DIALOGUE|证书过期对话]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-monitoring-alerting/DIALOGUE|监控告警对话]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-logging-pipeline/DIALOGUE|日志管道对话]]

## 排障工具 (Troubleshooting Tools)

- [[19-故障诊断/11-工具/README|排障工具索引]]
- [[09-可观测性/07-工具/06-troubleshooting-tools|排障工具集]]
- [[09-可观测性/07-工具/07-performance-profiling-tools|性能分析工具]]

## 多故障场景 (Multi-Fault Scenarios)

- [[19-故障诊断/09-多故障场景/README|多故障场景索引]]

## 生产故障案例

| 案例 | 症状 | 根因 | 修复 |
|------|------|------|------|
| Node NotReady | 节点状态 NotReady | kubelet 证书过期 | 轮换证书，重启 kubelet |
| Pod CrashLoop | 容器反复重启 | OOMKilled / 配置错误 | 调整 limits / 修复配置 |
| DNS 解析超时 | 服务间调用超时 | CoreDNS Pod 不足 | 扩容 CoreDNS + NodeLocal DNS |
| PVC Pending | 存储卷无法绑定 | StorageClass 不存在 / 配额不足 | 创建 SC / 扩容配额 |
| etcd 延迟高 | API 响应慢 | 磁盘 IO 瓶颈 | 升级 SSD / 压缩碎片整理 |
| 级联驱逐 | 大量 Pod 被驱逐 | 节点内存压力 | 调整驱逐阈值 + 扩容 |

## 常见故障快速定位表

| 症状 | 首先检查 | 关键命令 |
|------|----------|----------|
| Pod Pending | 调度失败原因 | `kubectl describe pod` → Events |
| Pod CrashLoopBackOff | 容器日志 | `kubectl logs --previous` |
| Pod OOMKilled | 内存使用 | `kubectl top pod` + limits |
| Service 不通 | Endpoint 是否存在 | `kubectl get ep <svc>` |
| DNS 解析失败 | CoreDNS 状态 | `kubectl logs -n kube-system -l k8s-app=kube-dns` |
| Node NotReady | kubelet 状态 | `systemctl status kubelet` + `journalctl -u kubelet` |
| ImagePullBackOff | 镜像/凭证 | `kubectl describe pod` → 检查 imagePullSecrets |
| Evicted | 节点资源压力 | `kubectl describe node` → Conditions |

## 概念 (Concepts)

- [[19-故障诊断/00-总览/01-systematic-troubleshooting-methodology|故障排查方法论]]
- [[19-故障诊断/01-核心排障/Production Troubleshooting Playbook|生产故障排查 Playbook]]
- [[19-故障诊断/06-FTA故障树/glossary/fault-tree-analysis|故障树分析]]
- [[23-实体/15-参考与索引/fta-febm-methodology|根因分析]]

## 实体 (Entities)

- [[23-实体/15-参考与索引/k8s-production-operations|Kubernetes Production Operations]]

## 故障排查全景

### 排查方法论

| 方法 | 说明 |
|---|---|
| FTA | 故障树分析，自上而下 |
| FEBM | 基于证据，自下而上 |
| 二分法 | 逐步缩小范围 |
| 对比法 | 正常/异常对比 |

### 排查工具链

| 层次 | 工具 |
|---|---|
| 集群 | kubectl, kubectx |
| 节点 | ssh, journalctl |
| 网络 | tcpdump, curl |
| 日志 | kubectl logs, stern |
| 监控 | Prometheus, Grafana |

## 面试要点

1. **Q：故障排查的核心原则？**
   A：观察现象→形成假设→验证假设→确认根因→修复验证→复盘归档。

2. **Q：如何加速故障定位？**
   A：完善监控、FTA 故障树、历史案例、自动化工具、经验积累。

3. **Q：故障复盘的关键要素？**
   A：时间线、根因、影响、修复过程、改进措施、责任认定(无责文化)。

## Related Tags

- [[27-标签/01-核心平台/k8s|k8s — Kubernetes 核心]]
- [[27-标签/05-交付与运维/production|production — 生产运营]]
- [[27-标签/05-交付与运维/reliability|reliability — 可靠性工程]]
- [[27-标签/04-可观测性/observability|observability — 可观测性]]
- [[27-标签/05-交付与运维/sre|sre — 站点可靠性工程]]
- [[27-标签/02-网络与存储/networking|networking — 网络排障]]
- [[27-标签/02-网络与存储/storage|storage — 存储排障]]
- [[27-标签/03-安全与合规/security|security — 安全排障]]
