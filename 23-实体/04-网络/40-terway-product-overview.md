---
title: Terway 产品概览
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- flannel
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 产品概览 是什么
- 如何 Terway 产品概览
trigger_keywords:
- Terway
- 产品概览
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 产品概览

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 01 - Terway 产品概览 (Product Overview)

## 技术细节

### 3. 网络模式总览

Terway 提供五种网络模式，按性能和容量密度递增排列：

| 模式 | Pod IP 来源 | 网络接口 | 性能 (相对物理机) | 容量密度 | 内核要求 | 适用场景 |
|:---|:---|:---|:---:|:---:|:---|:---|
| **VPC** | VPC 路由表条目 | veth pair + Node 网络栈 | ~70% | 低 (受路由条目 48 条限制) | 无特殊要求 | 小规模集群、兼容性优先、已有 Flannel 迁移过渡 |
| **ENI** | 独占 ENI 主 IP | ENI 直通 | ~95% | 低 (受 ENI 配额限制) | 无特殊要求 | 核心数据库、网关、高性能隔离需求 |
| **ENIIP** | ENI 辅助 IP (Secondary IP) | veth pair + ENI | ~90% | 高 (推荐默认

### 5. 核心依赖

Terway 深度依赖以下阿里云基础设施和服务：

| 依赖 | 服务 | 说明 | 必需性 |
|:---|:---|:---|:---:|
| **VPC (专有网络)** | 阿里云 VPC | Pod 网络的底层承载平面，vSwitch 为 Pod 分配 VPC 内网 IP | 必需 |
| **ENI (弹性网卡)** | 阿里云 ECS ENI | ENI/ENIIP/IPVlan 模式的网络接口载体，每个 Pod 通过 ENI 接入 VPC | ENI 模式必需 |
| **OpenAPI** | 阿里云 ECS API | ENI 创建/删除/绑定/解绑，辅助 IP 分配/释放等操作 | 必需 |
| **RAM 角色** | 阿里云 RAM | Terway 通过 ECS 实例角色 (Instance RAM Role) 获取访问云资源的临时凭证 | 必需 |
| **安



## 与 K8s 网络模型的关系

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 eBPF 网络技术结合，可实现更高效的网络策略和流量管理。

## 运维操作

```bash
# 🟢 检查 Terway 组件状态
kubectl get pods -n kube-system -l app=terway-eniip
kubectl logs -n kube-system -l app=terway-eniip --tail=30

# 🟢 检查 Pod 网络分配
kubectl get pods -o wide -A | grep <node>
kubectl exec <pod> -- ip addr
kubectl exec <pod> -- ip route

# 🟢 检查 ENI 资源使用
kubectl get eni -A  # Terway CRD
kubectl get podeni -A -o wide

# 🟢 检查 IP 池状态
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli mapping
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show factory

# 🟢 检查 NetworkPolicy 状态
kubectl get networkpolicy -A
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show policy

# 🟡 重启 Terway DaemonSet (网络短暂中断)
kubectl rollout restart daemonset/terway-eniip -n kube-system

# 🟢 检查 eBPF 模式状态
kubectl exec -n kube-system <terway-pod> -c terway -- bpftool prog list
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 卡在 ContainerCreating | ENI/IP 分配失败 | `kubectl describe pod` | 检查 ENI 配额/vSwitch IP |
| Pod 无网络连通性 | 安全组规则限制 | 检查安全组入/出规则 | 放行 Pod CIDR 段 |
| 跨节点 Pod 不通 | VPC 路由缺失 | 检查 VPC 路由表 | 添加 Pod CIDR 路由 |
| IP 地址耗尽 | vSwitch IP 不足 | 检查 vSwitch 可用 IP | 扩容 vSwitch/添加新 vSwitch |
| NetworkPolicy 不生效 | eBPF 未启用 | 检查 Terway 配置 | 启用 eBPF 模式 |
| DNS 解析失败 | CoreDNS Pod 网络异常 | `kubectl exec pod -- nslookup` | 检查 CoreDNS Endpoint |

### 排查流程

```
Terway 网络异常
├── Pod 无法获取 IP
│   ├── kubectl describe pod → 查看 Events
│   ├── 检查 Terway Pod 日志
│   ├── 检查 ENI 配额 (ECS 控制台)
│   └── 检查 vSwitch 可用 IP 数
├── Pod 已分配 IP 但不通
│   ├── 同节点不通 → 检查 veth pair/bridge
│   ├── 跨节点不通 → 检查 VPC 路由/安全组
│   └── Service 不通 → 检查 kube-proxy/iptables
└── 性能问题
    ├── 检查 ENI 模式是否正确 (ENIIP vs VPC)
    ├── 检查 eBPF 是否启用
    └── 检查 MTU 配置
```

## 生产案例

### 案例 1: ENI 配额耗尽导致 Pod 无法调度

- **场景**: 节点扩容后新 Pod 持续 ContainerCreating，报 "bindEni: exceeded quota"
- **排查**: ECS 实例 ENI 配额为 8，已分配 8 个；新节点实例规格 ENI 配额较小
- **方案**: 升级实例规格增加 ENI 配额；切换到 ENIIP 模式提高单 ENI IP 密度；配置多 vSwitch 分散 IP 分配
- **效果**: Pod 密度从 8/节点提升至 64/节点；IP 分配成功率 99.9%

### 案例 2: 安全组规则导致跨命名空间通信失败

- **场景**: 部署 NetworkPolicy 后，部分合法流量被拦截
- **排查**: Terway eBPF 模式 + 安全组双重过滤；安全组未放行 Pod CIDR
- **方案**: 安全组放行集群 Pod CIDR 段；NetworkPolicy 作为细粒度控制；安全组作为粗粒度边界
- **效果**: 网络策略生效正常；安全分层清晰

## 生产部署建议

- 建议在生产环境中使用 ENIIP 模式以提高 IP 利用率
- 密切监控 ENI 资源使用情况，避免 IP 耗尽
- 配合 [[networkpolicy|NetworkPolicy]] 实现 Pod 间访问控制
- 配置多 vSwitch 提高 IP 分配容错性
- 启用 eBPF 模式提升 NetworkPolicy 性能
- 监控 Terway DaemonSet Pod 状态和 IP 池使用率

## 检查清单

- [ ] Terway DaemonSet 所有 Pod Running
- [ ] ENI/IP 分配成功率 > 99%
- [ ] vSwitch 可用 IP 充足 (>20%)
- [ ] 安全组规则允许 Pod CIDR 通信
- [ ] NetworkPolicy 功能验证通过
- [ ] eBPF 模式已启用 (如适用)
- [ ] 监控告警覆盖 IP 池使用率
- [ ] 多 vSwitch 容错已配置

## 参考链接

- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)
- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]

<!-- risk-assessed -->
