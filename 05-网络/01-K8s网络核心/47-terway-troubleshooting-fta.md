---
title: Terway 故障排查
description: '# Terway 故障排查'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 故障排查 是什么
- 如何 Terway 故障排查
- Terway 故障排查 故障排查
- Terway 故障排查 排障步骤
trigger_keywords:
- Terway
- 故障排查
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
fta_id: FTA-47_TERWAY_TROUBLESHOOTING-001
component: 47 Terway Troubleshooting
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 故障排查

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 07 - Terway 故障树速查 (FTA Troubleshooting Quick Reference)

## 技术细节

### 故障树总览 (FTA)

```
                    [Pod 网络异常]
                         │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
   [IP 分配失败]   [网络不通]      [性能下降]
        │               │               │
   ┌────┴────┐    ┌────┴────┐    ┌────┴────┐
   ▼         ▼    ▼         ▼    ▼         ▼
[ENI 配额] [IP 池] [路由] [安全组] [带宽] [CPU]
   │         │      │      │      │      │
   ▼         ▼      ▼      ▼      ▼      ▼
[扩容]   [清理]  [检查]  [放行]  [升级]  [调优]
```

### 故障场景 1: Pod IP 分配失败

**症状**: Pod 长时间处于 `ContainerCreating`，Events 显示 `failed to allocate IP`

```bash
# 🟢 低风险：检查 Terway 日志
kubectl logs -n kube-system -l app=terway-eniip --tail=100

# 🟢 低风险：检查 ENI 配额
aliyun ecs DescribeNetworkInterfaces --RegionId cn-hangzhou --InstanceId <instance-id>

# 🟢 低风险：检查节点 IP 池状态
kubectl get cm -n kube-system eni-config -o yaml

# 🟢 低风险：检查 Pod 网络状态
kubectl describe pod <pod-name> -n <namespace>
```

**常见原因与解决方案**:

| 原因 | 诊断命令 | 解决方案 |
|-----|---------|----------|
| ENI 配额不足 | `aliyun ecs DescribeNetworkInterfaces` | 提交工单扩容配额 |
| vSwitch IP 耗尽 | `aliyun vpc DescribeVSwitches` | 扩容 vSwitch CIDR |
| Terway Pod 异常 | `kubectl get pods -n kube-system -l app=terway` | 重启 Terway Pod |
| RAM 权限不足 | 检查 ECS 实例角色 | 附加 AliyunECSFullAccess 策略 |

### 故障场景 2: Pod 网络不通

**症状**: Pod 已 Running，但无法访问其他 Pod 或外部服务

```bash
# 🟢 低风险：检查 Pod 网络命名空间
kubectl exec -it <pod-name> -n <namespace> -- ip addr

# 🟢 低风险：检查路由表
kubectl exec -it <pod-name> -n <namespace> -- ip route

# 🟢 低风险：测试连通性
kubectl exec -it <pod-name> -n <namespace> -- ping <target-ip>

# 🟢 低风险：检查 DNS 解析
kubectl exec -it <pod-name> -n <namespace> -- nslookup kubernetes.default

# 🟢 低风险：检查安全组规则
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId <sg-id>
```

**诊断决策树**:

```
[Pod 网络不通]
    │
    ├── [同节点 Pod 不通?]
    │       │
    │       ├── 是 → 检查 veth pair 和网桥
    │       │       kubectl exec <pod> -- ip link
    │       │
    │       └── 否 → 继续
    │
    ├── [跨节点 Pod 不通?]
    │       │
    │       ├── 是 → 检查 VPC 路由表
    │       │       aliyun vpc DescribeRouteTableList
    │       │
    │       └── 否 → 继续
    │
    ├── [访问 Service 不通?]
    │       │
    │       ├── 是 → 检查 kube-proxy/iptables
    │       │       kubectl exec <pod> -- iptables -t nat -L -n
    │       │
    │       └── 否 → 继续
    │
    └── [访问外部不通?]
            │
            └── 是 → 检查 NAT 网关/安全组
                    aliyun ecs DescribeSecurityGroupAttribute
```

### 故障场景 3: 网络性能下降

**症状**: 网络延迟增加、吞吐量下降

```bash
# 🟢 低风险：检查网络延迟
kubectl exec -it <pod-name> -n <namespace> -- ping -c 10 <target-ip>

# 🟢 低风险：检查带宽
kubectl exec -it <pod-name> -n <namespace> -- iperf3 -c <target-ip>

# 🟢 低风险：检查 ENI 状态
kubectl exec -it <pod-name> -n <namespace> -- ethtool -S eth0

# 🟢 低风险：检查节点网络负载
sar -n DEV 1 5

# 🟢 低风险：检查 Terway 指标
curl -s http://localhost:19090/metrics | grep terway
```

**性能优化检查清单**:

| 检查项 | 命令 | 正常值 |
|-------|------|-------|
| ENI 队列数 | `ethtool -l eth0` | ≥ 4 |
| 中断亲和性 | `cat /proc/interrupts` | 均匀分布 |
| TCP 参数 | `sysctl net.ipv4.tcp_*` | 已调优 |
| MTU 设置 | `ip link show` | 1500/9000 |

### 故障场景 4: Terway Pod 异常

**症状**: Terway Pod CrashLoopBackOff 或 NotReady

```bash
# 🟢 低风险：检查 Terway Pod 状态
kubectl get pods -n kube-system -l app=terway-eniip -o wide

# 🟢 低风险：查看 Terway 日志
kubectl logs -n kube-system <terway-pod> --previous

# 🟢 低风险：检查 Terway 配置
kubectl get cm -n kube-system eni-config -o yaml

# 🟡 中风险：重启 Terway Pod
kubectl delete pod -n kube-system <terway-pod>

# 🟢 低风险：检查节点资源
kubectl describe node <node-name> | grep -A5 "Allocated resources"
```

**常见错误日志与解决方案**:

| 错误日志 | 原因 | 解决方案 |
|---------|------|----------|
| `failed to create ENI` | ENI 配额不足 | 扩容配额或清理闲置 ENI |
| `vSwitch not found` | vSwitch 配置错误 | 检查 eni-config ConfigMap |
| `permission denied` | RAM 权限不足 | 检查实例角色权限 |
| `context deadline exceeded` | API 调用超时 | 检查网络连通性 |
| `IP pool exhausted` | IP 池耗尽 | 扩容 vSwitch 或清理 Pod |

## 日志分析

### Terway 日志位置

```bash
# Terway 主日志
kubectl logs -n kube-system -l app=terway-eniip -f

# 节点上的 Terway 日志
journalctl -u terway -f

# CNI 插件日志
cat /var/log/terway/cni.log
```

### 关键日志模式

```bash
# 搜索 IP 分配错误
kubectl logs -n kube-system -l app=terway-eniip | grep -i "allocate\|assign"

# 搜索 ENI 操作错误
kubectl logs -n kube-system -l app=terway-eniip | grep -i "eni\|network interface"

# 搜索 API 调用错误
kubectl logs -n kube-system -l app=terway-eniip | grep -i "openapi\|ecs.aliyuncs"
```

## 网络连通性测试

### 测试脚本

```bash
#!/bin/bash
# 🟢 低风险：Terway 网络连通性测试脚本
set -euo pipefail

NAMESPACE=${1:-default}
POD_NAME=${2:-test-pod}

echo "=== Terway 网络连通性测试 ==="

# 1. 创建测试 Pod
echo "[1] 创建测试 Pod..."
kubectl run $POD_NAME --image=nicolaka/netshoot -n $NAMESPACE --rm -it --restart=Never -- bash <<'EOF'
echo "--- Pod 网络信息 ---"
ip addr
ip route

echo "--- DNS 测试 ---"
nslookup kubernetes.default
nslookup www.aliyun.com

echo "--- 连通性测试 ---"
ping -c 3 kubernetes.default.svc.cluster.local
ping -c 3 8.8.8.8

echo "--- Service 测试 ---"
curl -s -o /dev/null -w "%{http_code}" https://kubernetes.default.svc.cluster.local/healthz

echo "--- 带宽测试 ---"
# iperf3 -c <target>
EOF

echo "=== 测试完成 ==="
```

## 紧急恢复

### Terway 完全重启

```bash
# 🔴 高风险：Terway 完全重启（可能导致短暂网络中断）
# 1. 备份配置
kubectl get cm -n kube-system eni-config -o yaml > /tmp/eni-config-backup.yaml

# 2. 删除所有 Terway Pod
kubectl delete pods -n kube-system -l app=terway-eniip

# 3. 等待重建
kubectl wait --for=condition=Ready pod -n kube-system -l app=terway-eniip --timeout=300s

# 4. 验证
kubectl get pods -n kube-system -l app=terway-eniip
```

### 单节点网络修复

```bash
# 🟡 中风险：单节点 Terway 修复
NODE_NAME=<node-name>

# 1. 驱逐节点上的 Pod
kubectl drain $NODE_NAME --ignore-daemonsets --delete-emptydir-data

# 2. 重启节点上的 Terway
kubectl delete pod -n kube-system -l app=terway-eniip --field-selector spec.nodeName=$NODE_NAME

# 3. 等待 Terway 就绪
kubectl wait --for=condition=Ready pod -n kube-system -l app=terway-eniip --field-selector spec.nodeName=$NODE_NAME --timeout=120s

# 4. 恢复调度
kubectl uncordon $NODE_NAME
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]

## 快速诊断脚本

```bash
#!/bin/bash
# 🟢 低风险：Terway 快速诊断

echo "=== Terway Pod 状态 ==="
kubectl get pods -n kube-system -l app=terway-eniip -o wide

echo -e "\n=== ENI 使用情况 ==="
kubectl get podeni -A --no-headers | wc -l

echo -e "\n=== 最近日志 ==="
kubectl logs -n kube-system -l app=terway-eniip --tail=20

echo -e "\n=== 诊断完成 ==="
```

## Related

- [[26-技能/05-网络/service/诊断排障/ts-networking.md|ts-networking]] — 网络故障排查
- [[k8gb]] — K8GB
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 47-terway-troubleshooting-fta

<!-- risk-assessed -->
