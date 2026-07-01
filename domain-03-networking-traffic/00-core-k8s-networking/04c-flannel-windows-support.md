---
title: Flannel Windows 节点支持
description: Flannel 在 Windows 节点上的配置与使用，涵盖 Windows 网络模型、HDC 后端、已知限制和故障排查
summary: Flannel 在 Windows 节点上的配置与使用，涵盖 Windows 网络模型、HDC 后端、已知限制和故障排查
category: networking
tags:
- k8s
- networking
- flannel
- windows
- hdc
- cni
- apiserver
- kubelet
- controller-manager
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Flannel Windows 配置
- Kubernetes Windows 网络
- HDC 后端
trigger_keywords:
- Flannel
- Windows
- HDC
- Kubernetes
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- prometheus-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---



# Flannel Windows 节点支持

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25+ | Flannel v0.20+ | Windows Server 2019+ | **最后更新**: 2026-05

---

<!-- chunk: 1. 概述 -->
## 1. 概述

Flannel 对 Windows 节点的支持通过 HDC (Host Device Communication) 后端实现，与 Linux 节点的 VXLAN/host-gw 后端不同。

### 1.1 支持矩阵

| 功能 | Linux | Windows |
|:-----|:-----:|:-------:|
| VXLAN 后端 | ✓ | ✗ |
| host-gw 后端 | ✓ | ✗ |
| HDC 后端 | ✗ | ✓ |
| UDP 后端 | ✓ | ✗ |
| WireGuard 后端 | ✓ | ✗ |
| IPv6 Dual Stack | ✓ | ✗ |
| [[NetworkPolicy|NetworkPolicy]] | ✓ (需 Canal) | ✗ |

---

<!-- chunk: 2. 架构原理 -->
## 2. 架构原理

### 2.1 Windows 网络模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    Windows Node                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐     ┌─────────────┐                           │
│  │   Pod A     │     │   Pod B     │                           │
│  │ vEthernet   │     │ vEthernet   │                           │
│  │ (HNS Network)│    │ (HNS Network)│                          │
│  └──────┬──────┘     └──────┬──────┘                           │
│         │                   │                                   │
│         └─────────┬─────────┘                                   │
│                   ▼                                             │
│         ┌─────────────────────┐                                 │
│         │   HNS Network      │                                 │
│         │   (NAT/Bridge)     │                                 │
│         └─────────┬───────────┘                                 │
│                   │                                             │
│                   ▼                                             │
│         ┌─────────────────────┐                                 │
│         │   External Adapter  │                                 │
│         │   (Pod 通信出口)     │                                 │
│         └─────────────────────┘                                 │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 HDC 工作流程

```
Pod A (Windows) ──▶ HNS Network ──▶ External Adapter ──▶ Pod B (Linux)
                  (vxlan-remotedriven?)     UDP 4789
```

---

<!-- chunk: 3. 前置要求 -->
## 3. 前置要求

### 3.1 Windows 版本

```powershell
# 需要 Windows Server 2019 (LTSC) 或 Windows Server 2022
# 确保安装了 HNS 模块
Get-Module -ListAvailable | Where-Object {$_.Name -eq "HNSTechs"}
```

### 3.2 容器运行时

```powershell
# 使用 containerd (推荐)
# 或使用 Docker EE with Windows Containers
```

### 3.3 Kubernetes 组件版本

```bash
# kube-apiserver, kube-controller-manager, kubelet 需要支持 Windows
kubectl version --short

# kubelet 版本需要与 kube-apiserver 匹配
```

---

<!-- chunk: 4. 配置步骤 -->
## 4. 配置步骤

### 4.1 Linux 控制平面配置

无需特殊配置，Flannel 在 Linux 侧使用标准 VXLAN 或 host-gw 后端。

```yaml
# Flannel ConfigMap (Linux 节点)
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "vxlan"
      }
    }
```

### 4.2 Windows 节点 Flannel 配置

Windows 节点需要单独配置 HDC 后端：

```powershell
# 在 Windows 节点上创建 flanneld.conf
$env:ProgramData\flanneld\config.json

{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "hdc",
    "IP": "<windows-node-internal-ip>",
    "IPv6Network": ""
  }
}
```

### 4.3 安装 Flanneld (Windows)

```powershell
# 下载 flanneld
Invoke-WebRequest -Uri "https://github.com/flannel-io/flannel/releases/latest/download/flanneld.exe" -OutFile "C:\flannel\flanneld.exe"

# 创建服务
sc create flanneld binPath= "C:\flannel\flanneld.exe --kubeconfig=C:\k\config"
sc description flanneld "Flannel Network Plugin"
sc config flanneld start= demand
```

### 4.4 HNS 网络创建

```powershell
# 创建 HNS Network (Flannel 会自动创建，也可手动)
# 使用 Kubernetes Node 注解指定网络类型

# 为节点添加注解启用 overlay
kubectl annotate node <windows-node> node.kubernetes.io/exclude-from-overlay-load-balancing=true

# 查看 HNS 网络
Get-HNSNetwork | Format-List

# 查看 HNS Endpoint (Pod vEthernet)
Get-HNSEndpoint | Format-List
```

---

<!-- chunk: 5. 跨平台通信 -->
## 5. 跨平台通信

### 5.1 Windows Pod ↔ Linux Pod 通信

```
Windows Pod A (10.244.1.2)
        │
        ▼
HNS Network (10.244.1.0/24)
        │
        ▼
vSwitch ──▶ External Network ──▶ VXLAN Tunnel ──▶ Linux Node
           (UDP 4789)                                 │
                                                     ▼
                                              Linux Pod B (10.244.2.2)
```

### 5.2 验证跨平台连通

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 从 Linux Pod 测试到 Windows Pod
kubectl exec -it <linux-pod> -- ping -c 3 10.244.1.2

# 从 Windows Pod 测试到 Linux Pod
kubectl exec -it <windows-pod> -- ping 10.244.2.2
```

---

<!-- chunk: 6. 故障排查 -->
## 6. 故障排查

### 6.1 常见问题

| 问题 | 原因 | 解决方案 |
|:-----|:-----|:--------|
| Windows Pod 无 IP | HNS 网络未创建 | 检查 flanneld 是否正常运行 |
| 无法跨节点通信 | VXLAN 端口被阻断 | 开放 UDP 4789 |
| HNS 网络类型错误 | 配置不正确 | 使用 flanneld 自动创建 |
| Pod 网络延迟高 | HNS NAT 模式开销 | 考虑使用 Host-gw (如果支持) |

### 6.2 排查命令

```powershell
# 1. 检查 flanneld 服务状态
Get-Service flanneld

# 2. 查看 flanneld 日志
# 日志位置: C:\ProgramData\flanneld\flanneld.log
Get-Content C:\ProgramData\flanneld\flanneld.log -Tail 50

# 3. 检查 HNS 网络
Get-HNSNetwork | Format-Table Name, Type, AddressPrefix, Gateway

# 4. 检查 HNS Endpoint
Get-HNSEndpoint | Format-Table Name, IPAddress, MacAddress

# 5. 检查 Pod vSwitch 绑定
Get-VMNetworkAdapter -VMName <pod-name>

# 6. 检查网络连通性
Test-NetConnection -ComputerName <target-ip> -Port 4789
```

### 6.3 日志分析

```powershell
# 查看 flanneld 详细日志
# 添加 --v=2 到启动参数获取更详细日志
sc stop flanneld
sc config flanneld binPath= "C:\flannel\flanneld.exe --v=2"
sc start flanneld

# 日志文件位置
# C:\ProgramData\flanneld\flanneld.log
```

---

<!-- chunk: 7. Windows 容器网络限制 -->
## 7. Windows 容器网络限制

| 限制 | 说明 |
|:-----|:----|
| 无 IPv6 支持 | Windows 节点暂不支持 IPv6 |
| 无 NetworkPolicy | Windows 不支持 Flannel NetworkPolicy |
| HNS NAT 模式 | 性能略低于 Linux bridge |
| 容器类型 | 仅支持 Windows Containers，不支持 Process isolation |

---

<!-- chunk: 8. 生产环境建议 -->
## 8. 生产环境建议

### 8.1 节点池隔离

```yaml
# 使用 NodeSelector 隔离 Windows 节点
apiVersion: v1
kind: Pod
metadata:
  name: myapp-windows
spec:
  nodeSelector:
    kubernetes.io/os: windows
  containers:
  - name: myapp
    image: mcr.microsoft.com/windows/servercore:ltsc2019
```

### 8.2 网络策略注意事项

```bash
# 由于 Flannel 不支持 Windows NetworkPolicy
# 建议在 Linux 节点前使用 Kubernetes NetworkPolicy
# 或使用服务网格（如 Istio）进行流量控制

# 检查 Windows 节点上的 pod 是否正确隔离
kubectl get pods -o wide -n <namespace> | grep -i windows
```

### 8.3 监控配置

```yaml
# Prometheus metrics 从 flanneld 暴露
# Windows 节点需要开放 10250 端口
kind: Service
metadata:
  name: flanneld-metrics
  namespace: kube-flannel
spec:
  clusterIP: None
  selector:
    app: flannel
  ports:
  - port: 10250
    targetPort: 10250
```

---

<!-- chunk: 9. 与其他 CNI 对比 -->
## 9. 与其他 CNI 对比

| 特性 | Flannel | Calico | Azure CNI |
|:-----|:-------:|:------:|:---------:|
| Windows 支持 | ✓ | ✓ | ✓ |
| 性能 | 中 | 中 | 高 |
| NetworkPolicy | ✗ | ✓ | ✓ |
| 复杂度 | 低 | 中 | 高 |
| 云厂商集成 | 一般 | 良好 | 原生 |

---

<!-- chunk: 10. 升级注意事项 -->
## 10. 升级注意事项

```powershell
# 升级前备份配置
Copy-Item -Path C:\ProgramData\flanneld -Destination C:\ProgramData\flanneld.bak -Recurse

# 升级 flanneld
Stop-Service flanneld
Invoke-WebRequest -Uri "https://github.com/flannel-io/flannel/releases/download/v0.24.2/flanneld.exe" -OutFile "C:\flannel\flanneld.exe"
Start-Service flanneld

# 验证版本
C:\flannel\flanneld.exe --version
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic KUDIG Database — Global MOC
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel 多集群场景与子网冲突处理

## See Also

- 04a-flannel-wireguard-backend
- 04b-flannel-ipv6-dual-stack
- 04d-flannel-multi-cluster
- 04e-flannel-command-reference

## Related

- [[domain-19-landscape-references/topic-index/flannel-index.md|Flannel 知识图谱索引]]
