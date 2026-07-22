---
title: 结构化排障方法论：配置优先、全组件排障指南
description: '## 配置优先原则'
summary: '## 配置优先原则'
category: reference
tags:
- k8s
- troubleshooting
- structured-troubleshooting
- configuration-first
- diagnostic
- etcd
- kubelet
- scheduler
- coredns
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 结构化排障方法论：配置优先、全组件排障指南 是什么
- 如何 结构化排障方法论：配置优先、全组件排障指南
- 结构化排障方法论：配置优先、全组件排障指南 故障排查
- 结构化排障方法论：配置优先、全组件排障指南 排障步骤
trigger_keywords:
- 结构化排障方法论：配置优先
- 全组件排障指南
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 结构化排障方法论

> **CNCF 状态**: 方法论 | **类别**: Troubleshooting | **主要语言**: Markdown, YAML

## 概述

Kubernetes 结构化故障排查是一套系统化的故障诊断方法论，为 K8s 生产环境提供标准化的排障流程。它将复杂的分布式系统故障分解为可管理的诊断步骤，涵盖分层排查（Pod → Node → Network → Control Plane）、证据收集、假设验证和根因定位。该方法论整合了 SRE 最佳实践、K8s 资源模型知识和运维工具链，帮助工程师在面对复杂故障时保持清晰的诊断思路，避免盲目试错。

## Key Features（核心能力）

- **分层排查模型**：从应用层到基础设施层逐层诊断（Pod → Service → Network → Node → Control Plane）
- **证据收集清单**：标准化的诊断命令和检查项清单
- **时间线分析**：基于 Events 时间线重建故障发生过程
- **假设验证框架**：系统化的假设生成和验证流程
- **自动化工具集成**：与 kubectl、k9s、HolmesGPT 等工具集成
- **知识库积累**：将故障案例转化为可复用的诊断模式

## 架构与工作原理

结构化排查方法论遵循 PDCA（Plan-Do-Check-Act）循环：Plan 阶段根据故障现象确定排查方向和优先级；Do 阶段执行诊断命令收集证据（Pod Status、Events、日志、指标）；Check 阶段分析证据验证或排除假设；Act 阶段确定根因并执行修复。排查从最上层（用户感知的问题）开始，逐步向下钻取到根本原因。每一步的证据都记录在诊断报告中，便于协作和复盘。

## K8s 集成

排查流程直接操作 K8s API 对象：从 kubectl describe pod 检查 Pod Status 和 Events 开始；通过 kubectl logs 查看应用日志；通过 kubectl get events 重建事件时间线；通过 kubectl exec 进入 Pod 测试网络连通性；通过 kubectl top 和 kubectl get nodes 检查资源使用和节点健康。对于控制平面问题，检查 kube-apiserver、etcd、scheduler 的日志和指标。

## 生产用例

- **Pod 启动失败**：系统化排查 ImagePullBackOff、CrashLoopBackOff、OOMKilled 等常见问题
- **网络不可达**：DNS 解析、Service Endpoints、NetworkPolicy 的分层诊断
- **性能降级**：从应用延迟到资源争用的性能瓶颈定位
- **控制平面异常**：API Server 延迟、etcd 性能、调度器问题的诊断

## 安装与配置

### 基础诊断工具链

```bash
# 🟢 标准排查命令链（Pod 层）
kubectl get pods -n <ns> -o wide
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous
kubectl logs <pod> -n <ns> -c <container> --since=1h
kubectl get events -n <ns> --sort-by=.lastTimestamp
kubectl top pods -n <ns> --sort-by=memory
```

### 分层诊断命令集

```bash
# 🟢 第一层：Pod/容器层
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[*].state}'
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.conditions}'
kubectl exec -it <pod> -n <ns> -- cat /etc/resolv.conf
kubectl exec -it <pod> -n <ns> -- wget -qO- http://localhost:8080/healthz

# 🟢 第二层：Service/网络层
kubectl get svc -n <ns> -o wide
kubectl get endpoints <svc> -n <ns>
kubectl get endpointslice -n <ns> -l kubernetes.io/service-name=<svc>
kubectl exec -it <pod> -n <ns> -- nslookup <svc>.<ns>.svc.cluster.local
kubectl exec -it <pod> -n <ns> -- curl -sv http://<svc>:<port>/ 2>&1 | head -30

# 🟢 第三层：Node/系统层
kubectl get nodes -o wide
kubectl describe node <node> | grep -A5 "Conditions"
kubectl top nodes
kubectl get pods -A --field-selector spec.nodeName=<node>

# 🟢 第四层：控制平面层
kubectl get componentstatuses 2>/dev/null || kubectl get --raw /healthz?verbose
kubectl -n kube-system get pods -l component=kube-apiserver
kubectl -n kube-system logs -l component=kube-scheduler --tail=50
kubectl -n kube-system logs -l component=etcd --tail=50
```

### 高级诊断工具

```bash
# 🟢 网络诊断
kubectl run debug-pod --rm -it --image=nicolaka/netshoot -- bash
# 在 debug-pod 内：
tcpdump -i eth0 -nn port 80 -c 20
iptables -t nat -L -n -v
conntrack -L | grep <target-ip>

# 🟢 资源诊断
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].resources}'
kubectl describe node <node> | grep -A20 "Allocated resources"

# 🟡 临时调试容器（K8s 1.23+）
kubectl debug -it <pod> -n <ns> --image=busybox --target=<container>
kubectl debug node/<node> -it --image=ubuntu
```

## 全组件诊断清单

| 组件 | 检查命令 | 关键指标 | 常见故障 |
|------|----------|----------|----------|
| kube-apiserver | `kubectl get --raw /healthz?verbose` | request_duration_seconds | 证书过期、etcd连接断开 |
| etcd | `etcdctl endpoint health` | db_size, leader_changes | 磁盘IO慢、成员失联 |
| kube-scheduler | `kubectl logs -n kube-system -l component=kube-scheduler` | scheduling_attempt_total | 资源不足、亲和性冲突 |
| kubelet | `journalctl -u kubelet --since 10m` | running_pods, started_pods | 证书轮转失败、磁盘压力 |
| CoreDNS | `kubectl logs -n kube-system -l k8s-app=kube-dns` | dns_request_duration_seconds | 上游DNS超时、缓存溢出 |
| containerd | `journalctl -u containerd --since 10m` | containerd_container_count | 镜像拉取失败、存储满 |
| kube-proxy | `kubectl logs -n kube-system -l k8s-app=kube-proxy` | sync_proxy_rules_duration | iptables规则过多、endpoint同步慢 |
| CNI插件 | `kubectl logs -n kube-system -l k8s-app=<cni>` | ipam_allocations | IP池耗尽、路由冲突 |

## 故障排查流程

### 分层排查决策树

```
用户报告问题
├── Pod 无法访问？
│   ├── kubectl get pod → 状态异常？
│   │   ├── ImagePullBackOff → 检查镜像名/密钥/网络
│   │   ├── CrashLoopBackOff → kubectl logs --previous
│   │   ├── OOMKilled → 调整 resources.limits.memory
│   │   └── Pending → 检查调度约束/资源/污点
│   └── 状态正常但不可达？→ 进入网络层
├── Service 不可达？
│   ├── kubectl get endpoints → 无端点？→ 检查 selector/label
│   ├── DNS 解析失败？→ 检查 CoreDNS/NetworkPolicy
│   └── 连接超时？→ 检查 NetworkPolicy/防火墙/CNI
├── 性能降级？
│   ├── kubectl top → 资源瓶颈？→ HPA/VPA/扩容
│   ├── 延迟增加？→ 检查网络/存储/依赖服务
│   └── 间歇性故障？→ 检查资源竞争/连接池/超时
└── 控制平面异常？
    ├── API 延迟高？→ etcd 性能/apiserver 负载
    ├── 调度延迟？→ scheduler 队列/资源碎片
    └── 节点 NotReady？→ kubelet/容器运行时/网络
```

### 证据收集模板

```bash
# 🟢 一键收集诊断信息
NAMESPACE="default"
POD="my-app-xxx"
OUTPUT_DIR="/tmp/k8s-debug-$(date +%Y%m%d-%H%M%S)"
mkdir -p $OUTPUT_DIR

kubectl get pod $POD -n $NAMESPACE -o yaml > $OUTPUT_DIR/pod-spec.yaml
kubectl describe pod $POD -n $NAMESPACE > $OUTPUT_DIR/pod-describe.txt
kubectl logs $POD -n $NAMESPACE --previous > $OUTPUT_DIR/pod-logs-prev.txt 2>&1
kubectl logs $POD -n $NAMESPACE --since=1h > $OUTPUT_DIR/pod-logs-1h.txt
kubectl get events -n $NAMESPACE --sort-by=.lastTimestamp > $OUTPUT_DIR/events.txt
kubectl top pod $POD -n $NAMESPACE > $OUTPUT_DIR/resource-usage.txt 2>&1
kubectl get endpoints -n $NAMESPACE > $OUTPUT_DIR/endpoints.txt
kubectl get networkpolicy -n $NAMESPACE -o yaml > $OUTPUT_DIR/netpol.yaml 2>&1
```

## 生产案例

### 案例1：DNS 解析间歇性超时

- **场景**：生产集群 Pod 间通信偶发 5s 超时，每天 20-30 次
- **排查过程**：
  1. `kubectl logs -n kube-system -l k8s-app=kube-dns` → 发现 ndots:5 导致大量无效查询
  2. `kubectl exec pod -- nslookup svc.ns.svc.cluster.local` → 正常
  3. `kubectl exec pod -- nslookup external.com` → 触发 5 次搜索域拼接查询
  4. CoreDNS metrics 显示 qps 是正常值 10 倍
- **方案**：Pod DNS 策略添加 `ndots: "2"` + 外部域名使用 FQDN（末尾加`.`）
- **效果**：DNS 查询量降低 80%，超时消失

### 案例2：节点 NotReady 级联故障

- **场景**：3 个节点同时变为 NotReady，200+ Pod 被驱逐
- **排查过程**：
  1. `kubectl describe node` → 所有节点 MemoryPressure=True
  2. `journalctl -u kubelet` → PLEG is not healthy
  3. `journalctl -u containerd` → 磁盘 IO 等待超时
  4. `dmesg` → 发现存储控制器固件 bug 导致 IO hang
- **方案**：升级存储固件 + 设置 kubelet imageGCHighThresholdPercent=70 提前回收
- **效果**：IO 问题消除，添加 Node Problem Detector 预防

## 对比替代方案

| 方法 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| 结构化排查（本文） | 可重复、可追溯、全面 | 需要经验判断优先级 | 复杂/未知故障 |
| 试错式排障 | 快速、直觉驱动 | 不可重复、易遗漏 | 已知简单问题 |
| HolmesGPT/AI辅助 | 自动化、知识全面 | 依赖数据质量、无法处理新型故障 | 常见故障快速定位 |
| Runbook自动化 | 标准化、无需人工 | 覆盖面有限、维护成本高 | 高频重复故障 |
| 混沌工程验证 | 预防性、验证恢复能力 | 实施复杂、有风险 | 验证架构韧性 |

## 检查清单

- [ ] 确认故障影响范围（单Pod/单Service/全集群）
- [ ] 收集 Events 时间线，确定故障起始时间
- [ ] 检查最近变更（Deployment/ConfigMap/Node/网络策略）
- [ ] 分层排查：Pod → Service → Network → Node → Control Plane
- [ ] 每步记录证据和结论，避免重复排查
- [ ] 确认根因后制定修复方案（临时缓解 + 根本修复）
- [ ] 修复后验证：功能恢复 + 监控指标正常 + 无副作用
- [ ] 编写故障报告（时间线/根因/修复/预防措施）

## Related

- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd


<!-- risk-assessed -->
