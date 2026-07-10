---
title: On-Call 快速参考卡
description: '| Pod 重启循环 | `kubectl get pods -o wide --field-selector status.phase=Running`
  | 查日志定位根因 |'
summary: '| Pod 重启循环 | `kubectl get pods -o wide --field-selector status.phase=Running`
  | 查日志定位根因 |'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- prometheus
- coredns
- elasticsearch
- hpa
- vpa
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- On-Call 快速参考卡 是什么
- 如何 On-Call 快速参考卡
trigger_keywords:
- On-Call
- 快速参考卡
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# On-Call 快速参考卡

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: on-call 工程师快速响应告警的袖珍手册
> **格式**: A4 单页 / 告警→命令→修复 三步速查

---

## 使用方法

1. **收到告警** → 在本卡片找到对应的告警类型
2. **按列执行** → 第一列诊断命令 → 第二列修复命令
3. **验证恢复** → 执行后用验证命令确认
4. **升级条件** → 如遇「→ 升级」标记，立即升级人工

---

## 一、节点与控制平面

### Node NotReady

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| 节点 NotReady | `kubectl get nodes -o wide` | `kubectl uncordon <node>` (低风险) |
| kubelet 连接失败 | `journalctl -u kubelet --since "10m" | grep -i "connection refused"` | `systemctl restart kubelet` (需审批) |
| 证书过期 | `openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates` | `kubeadm alpha certs renew kubelet.conf` (需审批) |
| 磁盘压力 | `df -h /` | `kubectl cordon <node> && kubectl drain <node> --ignore-daemonsets` (高风险) |

> **升级条件**: 控制平面节点 NotReady、多节点同时 NotReady

---

### Pod CrashLoopBackOff / OOMKilled

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| Pod 重启循环 | `kubectl get pods -o wide --field-selector status.phase=Running` | 查日志定位根因 |
| OOMKilled (137) | `kubectl describe pod <pod> | grep -A5 "Last State"` | 增加 memory limit: `kubectl patch resource limit` (低风险) |
| 退出码 1 | `kubectl logs <pod> --previous` | 修正应用配置 |
| 探针失败 | `kubectl describe pod <pod> | grep -A10 "Liveness|Readiness"` | 修正探针配置 |

> **升级条件**: 核心业务 Pod CrashLoop、应用无法启动

---

### Pod Pending (调度失败)

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| FailedScheduling | `kubectl get events -A --field-selector reason=FailedScheduling` | 见具体错误：资源不足/污点/亲和性 |
| Insufficient cpu/memory | `kubectl describe node` 查资源 | 增加资源或迁移工作负载 |
| node(s) had taint | `kubectl describe node <node> | grep Taints` | `kubectl patch pod` 添加 toleration (低风险) |
| PVC pending | `kubectl describe pvc` | 检查 StorageClass 是否存在 |

> **升级条件**: 30分钟未解决、核心服务 Pending

---

## 二、网络与 DNS

### DNS 解析失败

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| NXDOMAIN / timeout | `kubectl exec -it <pod> -- nslookup kubernetes.default` | `kubectl rollout restart deployment coredns - kube-system` |
| CoreDNS 不健康 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | `kubectl delete pod -n kube-system -l k8s-app=kube-dns` (低风险) |
| DNS 响应慢 | `kubectl exec -it <pod> -- time nslookup kubernetes.default` | 检查 upstream DNS 配置 |

> **升级条件**: 整个集群 DNS 不可用

---

### Service 无 Endpoints / 连通性问题

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| Service 无 EP | `kubectl get endpoints <svc> -n <ns>` | 检查 selector 匹配 + Pod 状态 |
| ClusterIP 不通 | `kubectl run test --image=busybox --restart=Never -- nslookup <svc>` | 重启 kube-proxy: `kubectl rollout restart daemonset kube-proxy - kube-system` |
| Ingress 502 | `kubectl describe ingress` + `kubectl get pods -n ingress-nginx` | 检查 backend Service + Pod 健康 |

> **升级条件**: 多个 Service 同时不可用、Ingress Controller 问题

---

## 三、存储与卷

### PVC Pending / 存储问题

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| PVC Pending | `kubectl describe pvc` 查原因 | 创建缺失的 StorageClass |
| 挂载失败 | `kubectl describe pod | grep -A5 "Volumes"` | 检查 PVC 绑定状态 |
| 云盘存储异常 | `kubectl get pods -n kube-system | grep csi` | 重启云盘 CSI driver (需审批) |
| StorageClass 缺失 | `kubectl get storageclass` | 创建 StorageClass 或指定默认 |

> **升级条件**: 有状态应用 PVC 问题、数据不可用

---

## 四、安全与证书

### 证书过期 / TLS 错误

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| x509 has expired | `openssl s_client -connect <host>:443 -check_ss_exp` | `kubeadm certs renew all` (需审批) |
| kubelet 证书过期 | `journalctl -u kubelet | grep -i certificate` | 重启 kubelet 触发自动轮换 (需审批) |
| kubeconfig 过期 | `kubectl config view` | `kubeadm kubeconfig user` 重新生成 |

> **升级条件**: 多个组件证书同时过期、控制平面证书问题

---

### RBAC Forbidden / 权限不足

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| Forbidden | `kubectl auth can-i <verb> <resource>` | 创建/更新 RoleBinding |
| ResourceQuota exceeded | `kubectl describe resourcequota` | 增加配额或清理资源 |
| ServiceAccount 权限不足 | `kubectl get sa <sa> -n <ns> -o yaml` | 创建 Role 并绑定 |

> **升级条件**: 系统组件权限不足、安全策略冲突

---

## 五、弹性伸缩与工作负载

### HPA/VPA 不触发

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| HPA ScalingActive=False | `kubectl describe hpa` 查原因 | 修复 metrics-server: `kubectl rollout restart deployment -n kube-system metrics-server` |
| 资源请求未设置 | `kubectl top pods` (如无数据) | 为 Pod 添加 resource requests (低风险) |
| HPA backoff | `kubectl describe hpa` | 等待 backoff 结束或降低阈值 |

> **升级条件**: 业务高峰无法扩容、PVP 缩容失败

---

### Deployment 滚动更新卡住

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| 滚动更新卡住 | `kubectl rollout status deployment/<name> -n <ns>` | `kubectl rollout undo deployment/<name>` (低风险) |
| ReplicaSet 未创建 | `kubectl get rs -n <ns>` | 检查镜像拉取 + 资源状态 |
| 探针失败导致 | `kubectl describe pod` 查探针状态 | 修正探针配置或回滚版本 |

> **升级条件**: 核心服务无法更新、回滚也失败

---

## 六、可观测性

### Prometheus/监控问题

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| 指标缺失 | `kubectl exec -it <prometheus-pod> -- wget -O- localhost:9090/metrics` | 检查 target 配置 |
| Prometheus 不健康 | `kubectl get pods -n monitoring` | `kubectl rollout restart statefulset prometheus - monitoring` |
| AlertManager 不发告警 | `kubectl logs -n monitoring alertmanager-*` | 检查 receiver 配置 |

> **升级条件**: 整个监控栈不可用

---

### 日志收集中断

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| 日志缺失 | `kubectl get pods -n logging` | `kubectl rollout restart daemonset -n logging fluent-bit` (低风险) |
| [[fluentd|Fluentd]] 不发送 | `kubectl logs -n logging fluentd-* --tail=100` | 检查 output 配置 (Elasticsearch 连接) |

> **升级条件**: 所有日志不可用、审计日志缺失

---

## 七、安全事件

### 安全告警 / 异常访问

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| 异常 Pod 创建 | `kubectl get events --sort-by=.lastTimestamp | tail -50` | `kubectl get pod -A -o yaml | grep "image:"` |
| 可疑网络连接 | `kubectl exec -it <pod> -- netstat -tlnp` | 隔离 Pod: `kubectl delete pod <pod>` (需审批) |
| 审计日志异常 | `kubectl logs -n kube-system kube-apiserver-* --audit` | 标记并升级安全团队 |

> **升级条件**: 确认入侵、凭据泄露、横向移动 → 立即升级

---

## 八、性能问题

### 响应延迟高 / 资源瓶颈

| 告警现象 | 诊断命令 | 修复命令 |
|---------|---------|---------|
| API Server 慢 | `kubectl top apiserver` (metrics) | 检查 etcd 延迟 + LIST 请求 |
| Pod CPU 高 | `kubectl top pod -n <ns>` | 增加 CPU limit / 优化应用 |
| Pod 内存高 | `kubectl top pod -n <ns>` | 增加 memory limit / 查内存泄漏 |
| 磁盘 I/O 高 | `iostat -x 1` (节点上) | 迁移 Pod 到其他节点 |

> **升级条件**: API Server 完全无响应、控制平面性能退化

---

## 紧急升级速查

| 条件 | 操作 |
|------|------|
| 控制平面节点 NotReady | → 立即升级 SRE 值班 |
| etcd 不健康 / 数据损坏 | → 立即升级 SRE + 数据库团队 |
| 多节点同时 NotReady | → 立即升级 SRE 值班 |
| 证书全部过期 | → 立即升级 SRE + 安全团队 |
| 安全事件 (入侵/数据泄露) | → 立即升级安全团队 |
| 存储卷只读/数据不可用 | → 立即升级 SRE + 存储团队 |
| 整个集群 DNS 不可用 | → 立即升级 SRE 值班 |

---

## 常用快速命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 节点状态速查
kubectl get nodes -o wide | grep -v Ready

# Pod 状态速查 (所有命名空间异常)
kubectl get pods -A | grep -v Running | grep -v Completed

# 事件速查 (最近错误)
kubectl get events -A --sort-by=.lastTimestamp | tail -100 | grep -i "error|failed|warning"

# 资源使用速查
kubectl top nodes && kubectl top pods -A | sort -k3 -rn | head -20

# 证书过期速查
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates 2>/dev/null || echo "Not found"

# CoreDNS 健康速查
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# kube-proxy 状态速查
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide

# 滚动重启 (低风险修复)
kubectl rollout restart deployment <name> -n <ns>
kubectl rollout restart daemonset <name> -n <ns>
```
---

## 备注

- **低风险 (🟢)**: 可自动执行或自主决策
- **中风险 (🟡)**: 建议人工确认后执行
- **高风险 (🔴)**: 需审批，建议升级处理
- **所有修复**: 执行后必须验证，并更新工单

---

**关联文档**:
- [故障诊断/topic-skills/README.md](../故障诊断/技能体系/README.md) — 完整 Skill 文档
- [P1-4: 决策树 Mermaid 可视化](./P1-4-decision-tree-mermaid-visualization.md)
- [故障诊断/](../故障诊断/) — 详细问题排查文档

<!-- risk-assessed -->
