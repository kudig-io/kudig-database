---
title: Tinkerbell [entities]
description: '## 概述'
summary: 'Tinkerbell 是一个裸金属服务器自动化配置（provisioning）框架，用于在物理服务器上自动安装操作系统和执行配置任务。它替代传统的 PXE/Cobbler 方案，通过声明式的工作流定义和容器化的操作步骤实现裸金属服务器的云原生式管理。'
category: entities
tags:
- k8s
- cncf
- metal
- tinkerbell
- prometheus
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tinkerbell 是什么
- 如何 Tinkerbell
trigger_keywords:
- Tinkerbell
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tinkerbell

> **CNCF 状态**: Sandbox | **类别**: Metal | **主要语言**: Go

## 概述

Tinkerbell 是一个 CNCF 沙箱项目，由 Equinix Metal（原 Packet）开源，是一个专为裸机服务器设计的自动化配置和部署平台。它在 Kubernetes 上运行，提供从服务器发现、操作系统安装到工作负载部署的全流程自动化。Tinkerbell 特别适合边缘计算、私有数据中心和裸机 Kubernetes 集群的场景，解决了物理服务器从零到可用状态的自动化问题。

## Key Features（核心能力）

- **裸机自动化**：通过 iPXE 网络启动实现物理服务器的全自动 OS 安装
- **Workflow 引擎**：基于 Action 的工作流定义，支持自定义部署步骤
- **Hardware 管理**：通过 Hardware CRD 管理物理服务器元数据和状态
- **Template 系统**：可复用的部署模板，支持多种 OS 镜像
- **DHCP/PXE 服务**：内置 DHCP 和 TFTP 服务，支持网络启动
- **K8s 原生**：所有资源以 CRD 形式管理

## 架构与工作原理

Tinkerbell 架构包含多个微服务组件：Boots 提供 DHCP 和 iPXE 引导服务；Hegel 提供基于硬件元数据的 metadata 服务（类似云厂商的 metadata API）；Tink Server 是核心控制器，管理 Workflow 和 Template；Tink Worker 运行在目标裸机上的临时容器中，执行 Workflow 中的 Action。所有组件作为 K8s Pod 运行，通过 CRD 管理硬件和工作流。

## K8s 集成

Tinkerbell 本身运行在 Kubernetes 上，所有组件以 Deployment/DaemonSet 形式部署。Hardware、Template、Workflow 通过 CRD 定义。Tink Controller 监听 Workflow CRD 并协调执行。在裸机 K8s 集群场景中，Tinkerbell 负责节点的初始 OS 安装和 K8s 组件部署，可与 Cluster API 的 CAPMVM provider 集成实现裸机节点的自动扩缩容。

## 生产用例

- **裸机 K8s 集群**：自动化部署物理服务器上的 Kubernetes 节点
- **边缘计算**：远程批量配置边缘数据中心的物理服务器
- **私有云建设**：替代传统裸机管理工具（如 Foreman/MaaS）
- **OS 批量部署**：大规模数据中心的操作系统自动化安装

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add tinkerbell https://tinkerbell.github.io/helm
helm repo update

# 🟢 安装 Tinkerbell Stack
helm install tinkerbell tinkerbell/tinkerbell-stack \
  -n tinkerbell --create-namespace \
  --set boots.enabled=true \
  --set hegel.enabled=true \
  --set tink.controller.enabled=true \
  --set tink.worker.enabled=true

# 🟢 验证安装
kubectl get pods -n tinkerbell
kubectl get crd | grep tinkerbell.org

# 🟢 配置 DHCP 服务（Boots）
kubectl edit configmap boots-config -n tinkerbell

# 🟢 查看可用硬件
kubectl get hardware -A
```

### Hardware + Workflow CRD 示例

```yaml
apiVersion: tinkerbell.org/v1alpha1
kind: Hardware
metadata:
  name: server-01
  namespace: tinkerbell
spec:
  bmc:
    address: 192.168.1.101
    username: admin
    passwordRef:
      name: server-01-bmc-creds
  interfaces:
    - netboot:
        allowPXE: true
        allowWorkflow: true
      dhcp:
        mac: "aa:bb:cc:dd:ee:01"
        ip:
          address: 192.168.1.51
          netmask: 255.255.255.0
          gateway: 192.168.1.1
        hostname: server-01
      metadata:
        instance:
          operating_system:
            distro: ubuntu
            version: "22.04"
---
apiVersion: tinkerbell.org/v1alpha1
kind: Template
metadata:
  name: ubuntu-2204-install
  namespace: tinkerbell
spec:
  data: |
    version: "0.1"
    name: ubuntu-install
    global_timeout: 1800
    tasks:
      - name: "os-install"
        worker: "{{.device_1}}"
        volumes:
          - /dev:/dev
          - /dev/console:/dev/console
          - /lib/firmware:/lib/firmware:ro
        actions:
          - name: "stream-ubuntu-image"
            image: quay.io/tinkerbell-actions/image2disk:v1.0.0
            timeout: 600
            environment:
              DEST_DISK: /dev/sda
              IMG_URL: "http://images.internal/ubuntu-22.04.raw.gz"
              COMPRESSED: true
          - name: "install-grub"
            image: quay.io/tinkerbell-actions/writefile:v1.0.0
            timeout: 90
            environment:
              DEST_DISK: /dev/sda
              FS_TYPE: ext4
              DEST_PATH: /boot/grub/grub.cfg
              CONTENTS: |
                set default=0
                set timeout=5
                menuentry 'Ubuntu' {
                  linux /boot/vmlinuz root=/dev/sda1 ro
                  initrd /boot/initrd.img
                }
---
apiVersion: tinkerbell.org/v1alpha1
kind: Workflow
metadata:
  name: provision-server-01
  namespace: tinkerbell
spec:
  templateRef: ubuntu-2204-install
  hardwareRef: server-01
  hardwareMap:
    device_1: aa:bb:cc:dd:ee:01
```

## 运维操作

```bash
# 🟢 查看硬件清单
kubectl get hardware -A -o wide

# 🟢 查看工作流状态
kubectl get workflow -A
kubectl describe workflow provision-server-01 -n tinkerbell

# 🟢 查看模板列表
kubectl get template -A

# 🟡 重新触发工作流（重启服务器 PXE）
kubectl patch hardware server-01 -n tinkerbell --type=merge -p \
  '{"spec":{"interfaces":[{"netboot":{"allowPXE":true,"allowWorkflow":true}}]}}'

# 🟡 取消运行中的工作流
kubectl delete workflow provision-server-01 -n tinkerbell

# 🔴 清除硬件注册（服务器将无法 PXE 启动）
kubectl delete hardware server-01 -n tinkerbell

# 🟢 查看 Boots DHCP 日志
kubectl logs -n tinkerbell -l app=boots --tail=50

# 🟢 查看 Hegel metadata 服务
kubectl logs -n tinkerbell -l app=hegel --tail=50
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 服务器未 PXE 启动 | DHCP/Boots 配置错误 | `kubectl logs -l app=boots` | 检查 DHCP 范围和 MAC 匹配 |
| Workflow 卡在 Action | 镜像下载失败 | `kubectl describe workflow <name>` | 检查 IMG_URL 可达性 |
| Hardware 状态 Unknown | BMC 连接失败 | 检查 BMC 地址和凭据 | 验证 IPMI 连通性 |
| 磁盘写入失败 | 磁盘路径错误 | 查看 Action 日志 | 确认 DEST_DISK 路径 |

```bash
# 排查流程
# 1. 检查 DHCP 服务是否正常
kubectl logs -n tinkerbell -l app=boots --tail=100 | grep -i dhcp

# 2. 检查 iPXE 引导链
kubectl logs -n tinkerbell -l app=boots --tail=100 | grep -i ipxe

# 3. 检查 Workflow Action 状态
kubectl get workflow provision-server-01 -n tinkerbell -o jsonpath='{.status}' | jq .

# 4. 检查 Hegel metadata 响应
kubectl exec -n tinkerbell deploy/hegel -- curl -s http://localhost:50061/healthcheck
```

## 生产案例

### 案例1：裸机 K8s 集群自动化部署
- **场景**：私有云数据中心需要批量部署 50 台物理服务器作为 K8s 节点
- **方案**：使用 Tinkerbell + Cluster API (CAPMVM)；定义标准化 Ubuntu 22.04 + kubeadm 工作流；通过 Hardware CRD 批量注册服务器 BMC 信息
- **效果**：50 台服务器从零到 K8s Ready 仅需 2小时，替代原来 2天 的手工安装

### 案例2：边缘数据中心远程配置
- **场景**：运营商 100+ 边缘机房需要远程重装服务器 OS
- **方案**：Tinkerbell 部署在中心集群；通过 BMC IPMI 远程触发 PXE 重启；自定义 Workflow 包含 OS 安装 + 网络配置 + 监控 Agent
- **效果**：无需现场工程师，远程完成 OS 重装，单次操作从 4小时 缩短到 30分钟

## 对比替代方案

| 维度 | Tinkerbell | MAAS | Foreman | Metal³ |
|------|-----------|------|---------|--------|
| K8s 原生 | 是 | 否 | 否 | 是 |
| 工作流引擎 | 强 | 中 | 中 | 弱 |
| 社区 | CNCF | Canonical | Red Hat | CNCF |
| 学习曲线 | 中 | 低 | 高 | 中 |
| 资产管理 | 弱 | 强 | 强 | 弱 |
| 边缘场景 | 强 | 中 | 中 | 强 |

## 检查清单

- [ ] Tinkerbell Stack 已部署且所有 Pod Running
- [ ] DHCP/TFTP 服务已正确配置（Boots）
- [ ] BMC 凭据已创建为 K8s Secret
- [ ] Hardware CRD 已正确注册服务器信息
- [ ] Workflow Template 已在测试服务器验证
- [ ] OS 镜像 URL 可访问
- [ ] 网络规划已完成（PXE 网络与业务网络隔离）

## Related

- [[headlamp]] — Headlamp
- [[实体/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tinkerbell
- [[实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
