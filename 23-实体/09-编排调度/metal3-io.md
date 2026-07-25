---
title: Metal3
description: 'summary: "Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。"'
summary: 'Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力，实现裸金属即服务。'
category: general
tags:
- k8s
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Metal3 是什么
- 如何 Metal3
trigger_keywords:
- Metal3
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Metal3

> **CNCF 状态**: Incubating | **类别**: Metal/Bare Metal | **主要语言**: Go

## 概述

Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力，由 Nordix、Equinor Metal、Red Hat 等推动开发，2021 年加入 CNCF 孵化。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"（Bare Metal as a Service）。Metal3 将裸金属服务器抽象为 Kubernetes 原生资源（BareMetalHost CRD），通过 IPMI/Redfish BMC 协议控制服务器的开机/关机/重启，通过 Ironic（OpenStack 组件）进行 PXE 网络启动和操作系统安装。这使得裸金属服务器的管理可以像虚拟机一样通过 Kubernetes API 声明式操作，是私有云 Kubernetes 基础设施管理的关键组件。

## 核心能力

- **Kubernetes 原生**: 通过 BareMetalHost CRD 声明式管理裸金属服务器
- **Cluster API 集成**: 与 Cluster API 统一的集群生命周期管理
- **自动发现**: 通过 IPMI/Redfish 发现和注册裸金属服务器
- **配置管理**: 基于 Ironic 自动化 PXE 启动和操作系统安装
- **生命周期管理**: 开机、关机、重装、回收等裸金属全生命周期
- **无代理**: 使用 BMC 协议（IPMI/Redfish），无需在服务器上安装任何代理

## 架构

Metal3 基于 Ironic + Cluster API 构建：

- **Metal3 BareMetal Operator**: 管理 BareMetalHost CRD 的生命周期
- **BareMetalHost CRD**: 裸金属服务器抽象，定义 BMC 地址、硬件规格、镜像、用户数据
- **Ironic (conductor)**: 核心裸金属供应引擎，执行 PXE/iPXE 启动和 OS 安装
- **Ironic Inspector**: 硬件自动发现和规格检测
- **Metal3 Provider (CAPMVM)**: Cluster API 的 Metal3 Provider，将 BareMetalHost 与 Cluster API Machine 关联
- **BMC (Baseboard Management Controller)**: 服务器主板上的远程管理接口（IPMI/Redfish）

供应流程：`BareMetalHost CRD → Operator → Ironic (PXE) → OS 安装 → 节点 Ready`

## K8s 集成

Metal3 以 Kubernetes Operator 方式运行。管理集群中部署 Metal3 BareMetal Controller 和 Ironic，通过 BareMetalHost CRD 管理裸金属服务器。每个 BareMetalHost 定义了 BMC 地址和凭据（通过 Secret 引用）、硬件规格和目标镜像。Controller 调用 Ironic 执行 PXE 启动和 OS 安装。与 Cluster API 集成时，CAPMVM Provider 将 BareMetalHost 与 Cluster API Machine 关联，实现裸金属上 Kubernetes 集群的自动化部署。与 [[kubernetes-architecture-overview|Kubernetes 架构]] 中的 Node、Machine 等资源统一管理。

## 生产场景

1. **私有云裸金属集群**: 在裸金属服务器上自动化部署 Kubernetes 集群（替代 vSphere/OpenStack）
2. **裸金属弹性扩容**: 根据负载自动开机新服务器并加入集群
3. **裸金属回收**: 节点下线时自动清除数据并恢复到可用状态
4. **多租户裸金属**: 为不同团队分配专用裸金属服务器，通过 BMC 物理隔离

## 安装与配置

```bash
# 安装 Metal3 baremetal-operator
kubectl apply -f https://github.com/metal3-io/baremetal-operator/releases/latest/download/baremetal-operator.yaml

# 等待 Operator 就绪
kubectl wait --for=condition=available deployment/baremetal-operator-controller-manager -n baremetal-operator-system --timeout=180s

# 创建 BMC 凭据
kubectl create secret generic bmc-credentials \
  --from-literal=username=admin --from-literal=password='SecureP@ss!' \
  -n metal3
```

```yaml
# BareMetalHost CRD 完整示例
apiVersion: metal3.io/v1alpha1
kind: BareMetalHost
metadata:
  name: worker-node-01
  namespace: metal3
  labels:
    hardware-type: compute
    rack: A-12
spec:
  online: true
  bmc:
    address: redfish+https://192.168.1.100/redfish/v1/Systems/1
    credentialsName: bmc-credentials
    disableCertificateVerification: false
  bootMACAddress: 00:11:22:33:44:55
  bootMode: UEFI
  rootDeviceHints:
    deviceName: /dev/sda
  image:
    url: http://image-server.internal/ubuntu-22.04-metal.qcow2
    checksum: sha256:a1b2c3d4e5f6...
    checksumType: sha256
  userData:
    namespace: metal3
    name: worker-user-data
  networkData:
    namespace: metal3
    name: worker-network-config
---
# 网络配置 Secret
apiVersion: v1
kind: Secret
metadata:
  name: worker-network-config
  namespace: metal3
stringData:
  networkData: |
    links:
    - id: eno1
      type: phy
      ethernet_mac_address: 00:11:22:33:44:55
    networks:
    - id: provision
      type: ipv4
      link: eno1
      ip_address: 10.0.1.11/24
    services:
      dns:
      - 10.0.0.1
```

## 运维操作

```bash
# 🟢 低风险：查看裸金属主机状态
kubectl get baremetalhosts -A
kubectl describe baremetalhost worker-node-01 -n metal3

# 🟢 低风险：查看供应进度
kubectl get baremetalhost worker-node-01 -n metal3 -o jsonpath='{.status.provisioning.state}'

# 🟡 中风险：开机/关机裸金属服务器
kubectl patch baremetalhost worker-node-01 -n metal3 --type merge -p '{"spec":{"online":true}}'
kubectl patch baremetalhost worker-node-01 -n metal3 --type merge -p '{"spec":{"online":false}}'

# 🔴 高风险：重装操作系统（数据丢失）
kubectl annotate baremetalhost worker-node-01 -n metal3 inspect.metal3.io= --overwrite
kubectl patch baremetalhost worker-node-01 -n metal3 --type merge \
  -p '{"spec":{"image":{"url":"http://image-server/ubuntu-24.04.qcow2","checksum":"sha256:new..."}}}'

# 🔴 高风险：删除 BareMetalHost（服务器回收）
kubectl delete baremetalhost worker-node-01 -n metal3

# 🟢 低风险：查看 Ironic 日志
kubectl logs -l app=ironic -n metal3-system -f
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| BMH 卡在 Registering | BMC 地址不可达 | `ipmitool -I lanplus -H 192.168.1.100 -U admin -P pass power status` | 检查 BMC 网络、凭据、防火墙 |
| 供应失败 (Provisioning Error) | PXE 启动失败 | `kubectl logs -l app=ironic-conductor` | 检查 DHCP/TFTP 配置、bootMACAddress |
| 镜像下载失败 | 镜像服务器不可达 | `curl -I http://image-server/image.qcow2` | 检查镜像 URL、网络、checksum |
| BMH 状态为 Error | Ironic 内部错误 | `kubectl describe bmh <name> -o yaml` | 查看 status.errorMessage，重启 Ironic |
| 节点未加入 K8s 集群 | cloud-init 失败 | 登录节点 `journalctl -u cloud-init` | 检查 userData Secret 内容 |

```
排查流程：
├── BMC 连接失败？
│   ├── ipmitool/redfish 手动测试 BMC 连通性
│   ├── 检查 bmc-credentials Secret
│   └── 确认防火墙允许 IPMI(623)/Redfish(443)
├── PXE 启动失败？
│   ├── 检查 DHCP 配置（next-server、filename）
│   ├── 确认 bootMACAddress 正确
│   └── 查看 Ironic conductor 日志
└── OS 安装后节点未就绪？
    ├── 检查 userData/cloud-init 配置
    ├── SSH 登录节点查看 cloud-init 日志
    └── 确认 kubeadm join 参数正确
```

## 生产案例

### 案例 1：裸金属集群自动化扩容

- **场景**：私有云 K8s 集群需要在大促前扩容 10 个裸金属 Worker 节点
- **排查**：手动 PXE + OS 安装需要 2 天，无法满足业务时间窗口
- **方案**：使用 Metal3 + Cluster API，定义 MachineDeployment replicas=10，自动发现、供应、加入集群
- **效果**：10 个节点从裸金属到 K8s Ready 仅需 45 分钟，全程无人工干预

### 案例 2：BMC 凭据泄露紧急响应

- **场景**：安全扫描发现 BMC 默认密码未修改，存在远程管理风险
- **排查**：扫描 200 台服务器，发现 35 台使用默认 BMC 凭据
- **方案**：通过 Metal3 批量更新 bmc-credentials Secret，启用 Redfish 证书认证，禁用 IPMI v1.5
- **效果**：30 分钟内完成全部 BMC 凭据轮换，消除远程管理攻击面

## 对比

| 特性 | Metal3 | Tinkerbell | MAAS | Ironic (standalone) |
|------|--------|-----------|------|---------------------|
| K8s 原生 | ✅ CRD | ✡ | ❌ | ❌ |
| Cluster API | ✅ | ✅ | ❌ | ❌ |
| BMC 支持 | ✅ IPMI/Redfish | ✅ | ✅ | ✅ |
| CNCF 状态 | Incubating | Sandbox | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Metal3 属于 **Metal/Bare Metal** 类别，为云原生应用提供裸金属基础设施管理能力。

## 参考链接

- [[deployment]]
- [[crd-custom-resources]]
- [[operator-pattern]]
- [[controller-pattern]]
- [[secrets-management]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- index/node-index|Node 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
