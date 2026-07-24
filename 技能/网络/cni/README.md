---
title: CNI 网络插件故障诊断技能集
description: Kubernetes CNI 网络插件（Terway、Calico、Cilium、Flannel）的完整故障诊断技能体系，覆盖 IP 分配异常、路由故障、安全组冲突、MTU 不匹配、控制面依赖等场景
summary: CNI 网络插件诊断技能集入口，涵盖 Terway/Calico/Cilium/Flannel 四大 CNI 的故障树诊断
category: skill
tags:
- k8s
- networking
- cni
- terway
- flannel
- calico
- cilium
- troubleshooting
- fta
sources:
- 故障诊断/FTA故障树/list/terway-fta.md
- 故障诊断/FTA故障树/list/flannel-fta.md
- 故障诊断/FTA故障树/list/calico-fta.md
- 故障诊断/FTA故障树/list/cilium-fta.md
- code/terway-1.17.5/
- code/flannel-0.28.7/
- code/cni-main/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 平台工程师
estimated_read_time: 5min
intent_queries:
- CNI 网络插件故障怎么排查
- Pod 网络不通从哪里开始诊断
- Terway 和 Flannel 故障有什么区别
- 跨节点 Pod 通信失败怎么解决
trigger_keywords:
- CNI
- Terway
- Flannel
- Calico
- Cilium
- Pod 网络
- 跨节点不通
- IP 分配失败
prerequisites:
- kubectl-basics
- linux-networking-basics
---

> **生产环境安全提示**
>
> 本技能集包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# CNI 网络插件故障诊断技能集

## 概述

本技能集覆盖 Kubernetes 四大主流 CNI 网络插件的故障诊断：

- **Terway**（阿里云 ACK）：基于 VPC ENI 的原生网络，支持 ENI 独占/ENIIP 共享/固定 IP 模式
- **Flannel**：轻量级 Overlay 网络，支持 VXLAN/host-gw 后端
- **Calico**：BGP 路由 + NetworkPolicy，支持 IPIP/VXLAN/纯路由模式
- **Cilium**：基于 eBPF 的高性能网络，支持身份感知策略

**适用场景**：
- Pod 获取 IP 失败（ContainerCreating 超时）
- Pod 跨节点通信失败/超时
- CNI DaemonSet 异常（CrashLoopBackOff）
- MTU 不匹配导致间歇性超时
- 安全组/路由表/网络策略配置错误

---

## 技能文件索引

| # | 文件 | 覆盖场景 | 难度 | 预计阅读 |
|---|------|---------|------|---------|
| 01 | [terway-fta.md](terway-fta.md) | Terway ENI/IP 池/VPC 路由/安全组/控制面依赖 | 中级 | 20min |
| 02 | [flannel-fta.md](flannel-fta.md) | Flannel VXLAN/host-gw/接口/子网/MTU | 中级 | 18min |
| 03 | [calico-fta.md](calico-fta.md) | Calico BGP/IPIP/VXLAN/NetworkPolicy/Felix | 中级 | 15min |
| 04 | [cilium-fta.md](cilium-fta.md) | Cilium eBPF/身份/策略/代理/CRD | 高级 | 15min |

---

## 快速诊断入口

```bash
# 🟢 低风险：只读/信息收集，通常无副作用

# Step 1: 确认 CNI DaemonSet 状态
kubectl get ds -n kube-system | grep -E "terway|flannel|calico|cilium"

# Step 2: 检查异常 Pod 网络事件
kubectl get events -A --field-selector reason=FailedCreatePodSandBox --sort-by='.lastTimestamp' | tail -10

# Step 3: 测试跨节点 Pod 连通性
kubectl exec <pod-on-node-1> -- ping -c 3 <pod-ip-on-node-2>

# Step 4: 检查节点网络接口
ip addr show | grep -E "flannel|cali|lxc|eth"
ip route show | grep -E "flannel|cali|172\.|10\.244"
```

---

## 状态速查表

| 症状 | 常见原因 | 优先检查项 | 对应技能 |
|:---|:---|:---|:---|
| Pod ContainerCreating > 5min（IP 分配） | ENI 配额/IP 池耗尽/CNI Pod 异常 | CNI DaemonSet 状态 + 日志 | terway-01/flannel-02 |
| 跨节点 Pod 不通 | 安全组/路由/VXLAN 端口/MTU | 安全组规则 + 路由表 + ping 测试 | terway-01/flannel-02 |
| 同节点通、跨节点不通 | 安全组未放行 Pod CIDR | 安全组入方向规则 | terway-01 |
| 大报文超时、小报文正常 | MTU 不匹配（VXLAN 开销） | `ping -M do -s 1400` | flannel-02 |
| CNI Pod CrashLoopBackOff | 配置错误/镜像拉取/RBAC | CNI Pod 日志 | 对应 CNI 技能 |
| 新节点 Pod 网络不通 | 子网分配冲突/路由未同步 | Node podCIDR + 路由表 | flannel-02 |

---

## FTA 故障树路径映射

| 顶层事件 | 中间事件 | 底事件 | 对应技能 |
|---------|---------|--------|---------|
| TE-NET Pod 网络异常 | IE-1 CNI 组件异常 | BE-1.1 DaemonSet 崩溃 | 各 CNI 技能 RC-001 |
| TE-NET Pod 网络异常 | IE-2 IP 分配失败 | BE-2.1 ENI 配额/IP 池耗尽 | terway RC-002/005 |
| TE-NET Pod 网络异常 | IE-3 路由/隧道异常 | BE-3.1 VXLAN/BGP/路由表错误 | flannel/calico |
| TE-NET Pod 网络异常 | IE-4 安全策略阻断 | BE-4.1 安全组/NetworkPolicy | terway RC-007 |
| TE-NET Pod 网络异常 | IE-5 MTU 不匹配 | BE-5.1 VXLAN 封包开销 | flannel RC-007 |

---

## 版本兼容性矩阵

| CNI | 适用 K8s 版本 | 版本敏感点 |
|-----|-------------|-----------|
| Terway v1.2+ | ACK 1.22+ | ENIIP 共享模式；NetworkPolicy 需 v1.5+ |
| Flannel v0.21+ | 1.26+ | IPv6 双栈；ConfigMap 路径变更 |
| Calico v3.25+ | 1.24+ | eBPF 数据面需 5.3+ 内核 |
| Cilium v1.14+ | 1.26+ | Gateway API 支持需 v1.15+ |

> **通用提示**：排障前先确认 CNI 类型和版本：`kubectl get ds -n kube-system -o wide | grep -E "terway|flannel|calico|cilium"`

---

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[技能/网络/service/service-fta.md|Service 故障树]] — 同域关联
- [[技能/网络/networkpolicy/networkpolicy-fta.md|NetworkPolicy 故障树]] — 同域关联
- [[技能/节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]] — 跨域关联
- [[技能/网络/service/诊断排障/ts-networking.md|网络排障实战]] — 实战参考

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]
