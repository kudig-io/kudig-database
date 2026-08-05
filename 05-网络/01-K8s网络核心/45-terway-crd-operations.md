---
title: Terway CRD 资源操作
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
- Terway CRD 资源操作 是什么
- 如何 Terway CRD 资源操作
trigger_keywords:
- Terway
- CRD
- 资源操作
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway CRD 资源操作

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)

## 技术细节

### Terway CRD 概览

Terway 使用以下 CRD 管理网络资源：

| CRD | 作用域 | 用途 |
|-----|-------|------|
| **PodENI** | Namespaced | 记录 Pod 的 ENI 绑定信息 |
| **PodNetworking** | Namespaced | 定义 Pod 网络配置模板 |
| **NetworkAttachmentDefinition** | Namespaced | Multus 多网卡配置 |
| **Node** | Cluster | 节点网络资源状态 |

### PodENI CRD

#### 查看 PodENI 资源

```bash
# 🟢 低风险：查看所有 PodENI
kubectl get podeni -A

# 🟢 低风险：查看特定 Pod 的 ENI 信息
kubectl get podeni <pod-name> -n <namespace> -o yaml

# 🟢 低风险：查看 ENI 绑定状态
kubectl get podeni -A -o custom-columns=NAME:.metadata.name,POD:.spec.podName,ENI:.spec.eniId,IP:.spec.ipAddress
```

#### PodENI 资源结构

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodENI
metadata:
  name: nginx-deployment-xxx
  namespace: default
  labels:
    app: nginx
spec:
  podName: nginx-deployment-xxx
  podNamespace: default
  eniId: eni-bp1234567890abcdef
  eniType: Secondary
  ipAddress: 192.168.1.100
  macAddress: 00:16:3e:xx:xx:xx
  vSwitchId: vsw-bp1234567890abcdef
  securityGroupIds:
    - sg-bp1234567890abcdef
  zoneId: cn-hangzhou-h
status:
  phase: Bound
  conditions:
    - type: Ready
      status: "True"
      lastTransitionTime: "2026-07-11T10:00:00Z"
```

#### PodENI 状态说明

| 状态 | 含义 | 常见原因 |
|-----|------|----------|
| `Pending` | 等待 ENI 分配 | ENI 配额不足、API 调用中 |
| `Allocating` | 正在分配 ENI | 创建 ENI 中 |
| `Binding` | 正在绑定 ENI | 绑定到实例中 |
| `Bound` | ENI 已绑定 | 正常状态 |
| `Releasing` | 正在释放 ENI | Pod 删除中 |
| `Failed` | 分配失败 | 配额不足、权限错误 |

### PodNetworking CRD

#### 创建 PodNetworking

```yaml
# 🟡 中风险：创建 PodNetworking 资源
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: high-performance-network
  namespace: production
spec:
  # 网络模式
  networkType: ENI
  
  # vSwitch 选择
  vSwitchOptions:
    - vsw-bp1234567890abcdef  # cn-hangzhou-h
    - vsw-bp0987654321fedcba  # cn-hangzhou-i
  
  # 安全组
  securityGroupIds:
    - sg-bp1234567890abcdef
  
  # 资源配额
  eniQuota:
    maxENI: 4
    maxIPPerENI: 10
  
  # 调度策略
  schedulingStrategy:
    zoneBalance: true  # 跨可用区均衡
    
  # 网络策略
  networkPolicy:
    enableNetworkPolicy: true
    defaultDeny: false
```

#### 应用 PodNetworking

```bash
# 🟡 中风险：应用 PodNetworking
kubectl apply -f podnetworking.yaml

# 🟢 低风险：查看 PodNetworking
kubectl get podnetworking -A

# 🟢 低风险：查看详细信息
kubectl get podnetworking high-performance-network -n production -o yaml
```

#### Pod 关联 PodNetworking

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:
  template:
    metadata:
      annotations:
        # 指定 PodNetworking
        k8s.aliyun.com/pod-networking: high-performance-network
        # 或直接指定 vSwitch
        k8s.aliyun.com/vswitch: vsw-bp1234567890abcdef
        # 指定安全组
        k8s.aliyun.com/security-group: sg-bp1234567890abcdef
    spec:
      containers:
        - name: nginx
          image: nginx:latest
```

### NetworkAttachmentDefinition (Multus)

#### 创建多网卡配置

```yaml
# 🟡 中风险：创建 NetworkAttachmentDefinition
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: terway-secondary
  namespace: default
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "terway-secondary",
      "type": "terway",
      "eniType": "Secondary",
      "vSwitch": "vsw-bp1234567890abcdef",
      "securityGroup": "sg-bp1234567890abcdef",
      "ipam": {
        "type": "terway-ipam"
      }
    }
```

#### Pod 多网卡配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-nic-pod
  annotations:
    # 指定附加网卡
    k8s.v1.cni.cncf.io/networks: terway-secondary
spec:
  containers:
    - name: app
      image: nicolaka/netshoot
      command: ["sleep", "infinity"]
```

#### 验证多网卡

```bash
# 🟢 低风险：查看 Pod 网卡
kubectl exec -it multi-nic-pod -- ip addr

# 预期输出:
# 1: lo: <LOOPBACK,UP> ...
# 2: eth0: <BROADCAST,MULTICAST,UP> ...  # 主网卡
# 3: net1: <BROADCAST,MULTICAST,UP> ...  # 附加网卡
```

### CRD 运维操作

#### 清理孤儿 PodENI

```bash
#!/bin/bash
# 🟡 中风险：清理孤儿 PodENI 资源
set -euo pipefail

echo "=== 清理孤儿 PodENI ==="

# 获取所有 PodENI
kubectl get podeni -A -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"' | while read podeni; do
  NS=$(echo $podeni | cut -d'/' -f1)
  NAME=$(echo $podeni | cut -d'/' -f2)
  
  # 检查对应 Pod 是否存在
  POD_NAME=$(kubectl get podeni $NAME -n $NS -o jsonpath='{.spec.podName}' 2>/dev/null || echo "")
  
  if [ -n "$POD_NAME" ]; then
    if ! kubectl get pod $POD_NAME -n $NS &>/dev/null; then
      echo "发现孤儿 PodENI: $NS/$NAME (Pod: $POD_NAME 不存在)"
      # kubectl delete podeni $NAME -n $NS  # 取消注释以删除
    fi
  fi
done

echo "=== 清理完成 ==="
```

#### 监控 CRD 状态

```bash
# 🟢 低风险：查看 PodENI 状态分布
kubectl get podeni -A -o json | jq -r '.items[].status.phase' | sort | uniq -c

# 🟢 低风险：查看 Failed 状态的 PodENI
kubectl get podeni -A --field-selector status.phase=Failed

# 🟢 低风险：查看特定节点的 ENI 使用情况
kubectl get podeni -A -o json | jq -r '.items[] | select(.spec.nodeName=="<node-name>") | .spec.eniId' | wc -l
```

### CRD 故障排查

#### PodENI 卡在 Pending

```bash
# 🟢 低风险：检查 PodENI 事件
kubectl describe podeni <pod-name> -n <namespace>

# 🟢 低风险：检查 Terway 日志
kubectl logs -n kube-system -l app=terway-eniip --tail=50 | grep -i "pending\|allocate"

# 常见原因:
# 1. ENI 配额不足 → 扩容配额
# 2. vSwitch IP 耗尽 → 扩容 vSwitch
# 3. Terway Pod 异常 → 重启 Terway
```

#### PodENI 状态异常

```bash
# 🟢 低风险：检查 ENI 实际状态
ENI_ID=$(kubectl get podeni <pod-name> -n <namespace> -o jsonpath='{.spec.eniId}')
aliyun ecs DescribeNetworkInterfaces --NetworkInterfaceId.1 $ENI_ID

# 🟡 中风险：强制重建 PodENI
kubectl delete podeni <pod-name> -n <namespace>
kubectl delete pod <pod-name> -n <namespace>
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[networkpolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[42-terway-product-overview]] — Terway 产品概览
- [[46-terway-operations-manual]] — Terway 运维手册
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[43-terway-architecture-deep-dive]]
- [[44-terway-usage-guide]]
- [[48-terway-performance-tuning]]
- [[47-terway-testing-validation]]
- [[49-terway-troubleshooting-fta]]
- 43-terway-crd-operations

<!-- risk-assessed -->
