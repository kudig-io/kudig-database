---
title: KubeVirt
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- containerd
- hpa
- operator
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVirt 是什么
- 如何 KubeVirt
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeVirt
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---

title: KubeVirt
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- containerd
- hpa
- operator
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KubeVirt 是什么
- 如何 KubeVirt
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeVirt
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# KubeVirt

> **成熟度**: Incubating | **加入时间**: 2019-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubevirt.io |
| **GitHub** | https://github.com/kubevirt/kubevirt |
| **文档** | https://kubevirt.io/user-guide |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Provisioning |

---

## 项目概述

### 简介
KubeVirt 是 Kubernetes 的虚拟机管理扩展，使 Kubernetes 能够像管理容器一样管理虚拟机。它实现了容器和 VM 工作负载的统一编排。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Red Hat 创建 |
| 2019-04 | 加入 CNCF Sandbox |
| 2022-04 | 晋升为 CNCF Incubating |

### 核心定位
KubeVirt 是混合工作负载的解决方案，让企业能够在统一的 Kubernetes 平台上运行传统 VM 应用和现代容器应用。

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    KubeVirt 架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Control Plane                             ││
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ││
│  │  │ virt-api      │  │virt-controller│  │virt-handler   │   ││
│  │  │ (API 入口)    │  │ (VM 生命周期) │  │ (节点代理)    │   ││
│  │  └───────────────┘  └───────────────┘  └───────────────┘   ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  Node                                                            │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  ┌─────────────────────────────────────────────────────┐   ││
│  │  │                  virt-launcher Pod                   │   ││
│  │  │  ┌─────────────────────────────────────────────┐    │   ││
│  │  │  │              libvirtd                        │    │   ││
│  │  │  │  ┌───────────────────────────────────────┐  │    │   ││
│  │  │  │  │              QEMU/KVM                 │  │    │   ││
│  │  │  │  │         ┌─────────────────┐          │  │    │   ││
│  │  │  │  │         │   Guest VM      │          │  │    │   ││
│  │  │  │  │         │  (Linux/Windows)│          │  │    │   ││
│  │  │  │  │         └─────────────────┘          │  │    │   ││
│  │  │  │  └───────────────────────────────────────┘  │    │   ││
│  │  │  └─────────────────────────────────────────────┘    │   ││
│  │  └─────────────────────────────────────────────────────┘   ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用示例

### 创建虚拟机

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: my-vm
spec:
  running: true
  template:
    metadata:
      labels:
        kubevirt.io/vm: my-vm
    spec:
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
            - name: rootdisk
              disk:
                bus: virtio
            - name: cloudinitdisk
              disk:
                bus: virtio
          interfaces:
            - name: default
              masquerade: {}
      networks:
        - name: default
          pod: {}
      volumes:
        - name: rootdisk
          containerDisk:
            image: quay.io/kubevirt/fedora-cloud-container-disk-demo
        - name: cloudinitdisk
          cloudInitNoCloud:
            userData: |
              #cloud-config
              password: password
              chpasswd: { expire: False }
```

### 热迁移

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachineInstanceMigration
metadata:
  name: migrate-my-vm
spec:
  vmiName: my-vm
```

---

## 核心功能

| 功能 | 说明 |
|:---|:---|
| **热迁移** | VM 实时迁移到其他节点 |
| **快照** | VM 状态快照和恢复 |
| **GPU 透传** | GPU 直接分配给 VM |
| **ContainerDisk** | 使用容器镜像作为 VM 磁盘 |
| **网络集成** | 与 Pod 网络、Multus 集成 |

---

## 安装

```bash
# 安装 KubeVirt Operator
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.1.0/kubevirt-operator.yaml

# 部署 KubeVirt
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.1.0/kubevirt-cr.yaml

# 安装 virtctl CLI
kubectl krew install virt
```

---

## 参考资源

- [官方文档](https://kubevirt.io/user-guide)
- [GitHub Repo](https://github.com/kubevirt/kubevirt)
- [CNCF 项目页面](https://www.cncf.io/projects/kubevirt/)
- [CDI (Containerized Data Im[[domain-19-landscape-references/sandbox/porter/porter.md|porter]])](https://github.com/kubevirt/containerized-data-importer)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
