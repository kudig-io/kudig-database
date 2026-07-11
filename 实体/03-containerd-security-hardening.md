---
title: containerd 安全加固
description: '# containerd 安全加固'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 03-containerd-security-hardening
- containerd
- falco
- networkpolicy
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
- containerd 安全加固 是什么
- 如何 containerd 安全加固
trigger_keywords:
- containerd
- 安全加固
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 安全加固

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 安全加固是一套针对 containerd 容器运行时的安全最佳实践和配置指南。它涵盖运行时二进制文件保护、套接字权限控制、镜像内容信任、Seccomp/AppArmor/SELinux 策略配置、审计日志、容器镜像扫描等多个维度。通过系统性加固，可将 containerd 从默认配置提升到满足生产级安全合规要求的水平，防御容器逃逸、提权攻击等安全威胁。

## Key Features（核心能力）

- **套接字权限控制**：限制 containerd.sock 文件权限，仅允许 root 或特定组访问
- **内容信任**：启用 containerd image verification 和 Cosign/Notary 签名验证
- **Seccomp 默认策略**：通过默认 seccomp profile 限制系统调用
- **AppArmor/SELinux**：通过 MAC 策略限制容器进程行为
- **审计日志**：启用 containerd audit log 记录所有容器操作
- **镜像扫描**：集成 Trivy/Grype 进行容器镜像漏洞扫描

## 架构与工作原理

安全加固从多个层面实施：二进制层面（只读文件系统、文件完整性监控）；配置层面（最小权限配置、禁用不必要功能）；运行时层面（Seccomp/AppArmor/SELinux 策略约束容器行为）；网络层面（限制 containerd gRPC 端口暴露）；镜像层面（签名验证、漏洞扫描）。通过纵深防御策略，在各层建立安全控制点。

## K8s 集成

在 Kubernetes 中，containerd 安全加固通过 KubeletConfiguration（如 protectKernelDefaults、seccompDefault）、Pod Security Admission（seccompProfile、apparmorProfile）以及节点级配置文件（config.toml）实现。CRI 支持将 seccomp 和 AppArmor profile 通过 Pod SecurityContext 传递给 containerd。

## 生产用例

- **生产环境合规**：满足 CIS Benchmark 和等保 2.0 对容器运行时的安全要求
- **多租户集群安全**：防止容器逃逸和横向移动攻击
- **金融/政府场景**：满足严格的安全审计和访问控制要求
- **安全事件防御**：通过运行时安全策略减少攻击面和影响范围

## 安装与快速开始

```bash
# 检查 containerd 配置
containerd config dump | grep -E 'security|seccomp|apparmor'

# 应用加固配置
cat > /etc/containerd/config.toml << 'EOF'
[plugins."io.containerd.grpc.v1.cri"]
  enable_unprivileged_ports = false
  enable_unprivileged_icmp = false
EOF
systemctl restart containerd
```

## 对比替代方案

相比 Docker 运行时，containerd 默认更精简且攻击面更小。相比 CRI-O，两者安全模型类似，但 containerd 生态更成熟。

## Related

- [[inclavare-containers]] — Inclavare Containers
- [[bank-vaults]] — Bank-Vaults
- [[thanos]] — Thanos
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-containerd-security-hardening


<!-- risk-assessed -->
