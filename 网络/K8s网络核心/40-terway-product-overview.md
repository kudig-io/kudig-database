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
tier: core
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

### 产品定位

Terway 是阿里云容器服务 ACK (Alibaba Cloud Container Service for Kubernetes) 的官方 CNI 插件，深度集成阿里云 VPC 网络基础设施，为 Kubernetes Pod 提供原生 VPC 网络接入能力。

**核心价值**:
- **原生 VPC 集成**: Pod IP 直接来自 VPC 地址段，无需 Overlay 封装
- **高性能网络**: ENI 直通模式性能接近物理机 (95%+)
- **云原生安全**: 直接复用阿里云安全组、网络 ACL 等能力
- **弹性 IP 管理**: 支持 ENI 辅助 IP，提高 IP 利用率

### 与其他 CNI 对比

| 特性 | Terway | Flannel | Calico | Cilium |
|-----|--------|---------|--------|--------|
| **网络模式** | VPC 原生 | Overlay (VXLAN) | BGP/Overlay | eBPF/Overlay |
| **Pod IP 来源** | VPC 地址段 | 独立 CIDR | 独立 CIDR | 独立 CIDR |
| **性能** | 95%+ | 70-80% | 85-90% | 90%+ |
| **NetworkPolicy** | ✅ 安全组 | ❌ | ✅ | ✅ eBPF |
| **云集成** | 深度集成 | 无 | 无 | 无 |
| **多网卡** | ✅ Multus | ❌ | ❌ | ✅ |
| **固定 IP** | ✅ | ❌ | ❌ | ❌ |
| **适用场景** | 阿里云 ACK | 通用 | 通用 | 高性能/安全 |

### 网络模式选择指南

```
[选择网络模式]
    │
    ├── [需要最高性能?]
    │       │
    │       ├── 是 → [节点规模 < 50?]
    │       │           │
    │       │           ├── 是 → ENI 独占模式
    │       │           │
    │       │           └── 否 → ENI 多 IP 模式 (ENIIP)
    │       │
    │       └── 否 → 继续
    │
    ├── [需要高密度?]
    │       │
    │       ├── 是 → ENI 多 IP 模式 (ENIIP) 或 IPVlan
    │       │
    │       └── 否 → 继续
    │
    ├── [内核版本 >= 4.19?]
    │       │
    │       ├── 是 → IPVlan 模式 (高性能 + 高密度)
    │       │
    │       └── 否 → ENI 多 IP 模式 (ENIIP)
    │
    └── [兼容性优先?]
            │
            └── 是 → VPC 模式 (类似 Flannel)
```

### 典型使用场景

| 场景 | 推荐模式 | 说明 |
|-----|---------|------|
| **Web 应用** | ENIIP | 平衡性能与密度 |
| **数据库** | ENI 独占 | 需要稳定 IP 和高性能 |
| **微服务** | ENIIP | 高密度部署 |
| **大数据** | IPVlan | 高吞吐需求 |
| **边缘计算** | VPC | 资源受限环境 |
| **多租户** | ENI 独占 | 网络隔离需求 |

### 部署与配置

#### 安装 Terway

Terway 作为 ACK 集群的默认 CNI，在创建集群时自动安装：

```bash
# 🟢 低风险：检查 Terway 安装状态
kubectl get pods -n kube-system -l app=terway-eniip

# 🟢 低风险：查看 Terway 版本
kubectl get ds -n kube-system terway-eniip -o jsonpath='{.spec.template.spec.containers[0].image}'

# 🟢 低风险：查看 Terway 配置
kubectl get cm -n kube-system eni-config -o yaml
```

#### 核心配置项

```yaml
# eni-config ConfigMap 关键配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 5,        # IP 池最大大小
      "min_pool_size": 1,        # IP 池最小大小
      "vswitches": {             # vSwitch 配置
        "cn-hangzhou-h": ["vsw-bp1234567890abcdef"],
        "cn-hangzhou-i": ["vsw-bp0987654321fedcba"]
      },
      "security_group": "sg-bp1234567890abcdef",
      "service_cidr": "172.16.0.0/16",
      "enable_trunk": false,     # 是否启用 Trunk ENI
      "enable_ipvlan": false,    # 是否启用 IPVlan
      "enable_ebpf": true        # 是否启用 eBPF
    }
```

### 版本兼容性

| Terway 版本 | ACK 版本 | Kubernetes 版本 | 主要特性 |
|------------|---------|----------------|----------|
| v1.5.x | 1.24+ | 1.24-1.26 | 基础 ENI/ENIIP 支持 |
| v1.6.x | 1.26+ | 1.26-1.28 | IPVlan 模式、eBPF 策略 |
| v1.7.x | 1.28+ | 1.28+ | Trunk ENI、多网卡增强 |
| v1.8.x | 1.30+ | 1.30+ | 性能优化、稳定性增强 |

### 配额与限制

| 资源 | 默认配额 | 说明 |
|-----|---------|------|
| ENI 数量/实例 | 4-8 (视规格) | 可提工单扩容 |
| 辅助 IP/ENI | 10-20 (视规格) | 可提工单扩容 |
| vSwitch IP | 取决于 CIDR | 建议 /20 或更大 |
| 安全组规则 | 200 条/安全组 | 可提工单扩容 |

### 监控与运维

```bash
# 🟢 低风险：查看 Terway 指标
kubectl exec -n kube-system <terway-pod> -- curl -s http://localhost:19090/metrics

# 🟢 低风险：查看 ENI 使用情况
kubectl get podeni -A -o custom-columns=NAME:.metadata.name,ENI:.spec.eniId,IP:.spec.ipAddress,STATUS:.status.phase

# 🟢 低风险：查看节点 IP 池状态
kubectl exec -n kube-system <terway-pod> -- terway-cli mapping

# 🟢 低风险：查看 Terway 日志
kubectl logs -n kube-system -l app=terway-eniip --tail=100
```

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|----------|
| Pod 无法获取 IP | ENI 配额耗尽 | `kubectl get podeni -A` | 提工单扩容或释放闲置 ENI |
| Pod 启动超时 | vSwitch IP 不足 | `kubectl describe pod <pod>` | 扩大 vSwitch CIDR 或添加新 vSwitch |
| 网络不通 | 安全组规则缺失 | `kubectl get podeni <pod> -o yaml` | 检查安全组入站/出站规则 |
| Terway Pod CrashLoop | 配置错误/权限不足 | `kubectl logs -n kube-system -l app=terway-eniip` | 检查 eni-config 和 RAM 角色 |
| 固定 IP 失效 | Pod 重建后 IP 变化 | `kubectl get podnetworking -A` | 配置 `k8s.aliyun.com/pod-with-eip: "true"` |
| 跨节点通信失败 | VPC 路由表异常 | `ip route show` | 检查 VPC 路由条目 |

## 监控告警

### PrometheusRule 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-alerts
  namespace: monitoring
spec:
  groups:
  - name: terway.rules
    rules:
    - alert: TerwayENIPoolLow
      expr: terway_eni_ip_pool_available < 5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ENI IP 池可用数量不足"
        description: "节点 {{ $labels.node }} 可用 IP 少于 5 个"
    - alert: TerwayENIQuotaExhausted
      expr: terway_eni_quota_remaining == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "ENI 配额已耗尽"
    - alert: TerwayPodNotReady
      expr: kube_pod_status_ready{condition="false"} * on(pod) group_left() kube_pod_labels{label_app="terway-eniip"} == 1
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Terway Pod 未就绪"
```

### 关键指标

```bash
# 🟢 低风险：查看 Terway 指标端点
kubectl exec -n kube-system <terway-pod> -- curl -s http://localhost:19090/metrics

# 关键指标说明：
# terway_eni_ip_pool_available - IP 池可用数量
# terway_eni_ip_pool_total - IP 池总容量
# terway_eni_quota_remaining - ENI 配额剩余
# terway_ipam_request_duration_seconds - IPAM 请求延迟
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| **vSwitch 规划** | 使用 /20 或更大 CIDR | 避免 IP 耗尽 |
| **多可用区** | 配置多个 vSwitch | 提高可用性 |
| **IP 池大小** | min_pool_size=1, max_pool_size=5 | 平衡预热与资源占用 |
| **安全组** | 最小权限原则 | 仅开放必要端口 |
| **固定 IP** | 数据库/有状态服务启用 | 避免 IP 变化影响连接 |
| **监控** | 部署 Prometheus + 告警 | 实时掌握 ENI 使用情况 |
| **升级** | 滚动升级，先测试环境 | 避免全集群故障 |
| **备份** | 定期备份 eni-config | 便于灾难恢复 |

## 容量规划计算

```
# 单节点最大 Pod 数计算

ENI 独占模式:
  Max Pods = ENI 配额 - 1 (主 ENI 保留)
  示例: 8 ENI 配额 → 7 Pods/节点

ENIIP 模式:
  Max Pods = (ENI 配额 - 1) × 每 ENI 辅助 IP 数
  示例: 8 ENI × 20 IP = 140 Pods/节点

IPVlan 模式:
  Max Pods = ENIIP 模式 × 1.5 (性能优化)
  示例: 140 × 1.5 = 210 Pods/节点

# 集群总容量
Cluster Capacity = 节点数 × Pods/节点 × 0.8 (预留 20% 缓冲)
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|networkpolicy]]

## Related

- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 40-terway-product-overview

<!-- risk-assessed -->
