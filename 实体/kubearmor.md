---
title: KubeArmor (entities)
description: '## 概述'
summary: 'KubeArmor 是一个云原生运行时安全引擎，利用 Linux 安全模块 (LSM - AppArmor, BPF-LSM, SELinux) 在系统级别执行安全策略。它保护 Kubernetes Pod、容器和节点免受已知和未知的威胁，包括进程执行、文件访问和网络操作的细粒度控制。'
category: entities
tags:
- k8s
- cncf
- security
- kubearmor
- prometheus
- grafana
- cilium
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeArmor 是什么
- 如何 KubeArmor
trigger_keywords:
- KubeArmor
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeArmor

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

KubeArmor 是由 Accuknox 开发的云原生运行时安全引擎，2021 年加入 CNCF Sandbox。它利用 Linux 安全模块（LSM - AppArmor、BPF-LSM、SELinux）在系统级别执行安全策略，保护 Kubernetes Pod、容器和节点免受已知和未知的威胁。KubeArmor 提供进程执行、文件访问和网络操作的细粒度控制，是容器运行时安全防护的重要工具。

## 核心特性

- **LSM 强制执行**: 基于 AppArmor、BPF-LSM、SELinux 的内核级安全策略
- **进程控制**: 限制容器内可执行的进程（白名单/黑名单）
- **文件保护**: 控制文件和目录的读写执行访问
- **网络控制**: 限制容器的网络连接行为
- **系统调用过滤**: 细粒度的 syscall 控制（基于 seccomp）
- **安全遥测**: 实时安全事件日志和 Prometheus 指标

## 架构

KubeArmor 由 KubeArmor Operator（管理策略部署）、KubeArmor DaemonSet（每个节点的策略执行器）和 Policy CRD 组成。DaemonSet 中的 KubeArmor 进程监听 K8s API 获取 KubeArmorPolicy 和 KubeArmorHostPolicy CRD，将策略翻译为 AppArmor Profile 或 BPF-LSM 程序，加载到容器运行时和主机内核。当容器内进程触发策略规则时，LSM 拦截操作（Allow/Audit/Block），KubeArmor 将安全事件记录为日志并导出为 Prometheus 指标。karmor CLI 工具辅助策略生成和测试。

## Kubernetes 集成

KubeArmor 通过 KubeArmorPolicy CRD（命名空间级）和 KubeArmorHostPolicy CRD（节点级）声明式管理安全策略。策略通过标签选择器匹配目标 Pod。DaemonSet 以特权模式运行，加载 LSM 策略到节点内核。支持三种策略动作：Allow（白名单模式，仅允许列出的操作）、Audit（记录但不阻止）、Block（阻止并记录）。与容器运行时（containerd、CRI-O）集成，自动为容器应用 AppArmor Profile。

## 生产使用场景

1. **容器加固**: 限制容器只能执行必要的进程和访问必要的文件
2. **合规要求**: 满足 PCI-DSS、HIPAA 等安全合规对运行时防护的要求
3. **零信任安全**: 实施 "deny by default" 策略，最小化攻击面
4. **入侵检测**: 以 Audit 模式运行，检测异常进程执行和文件访问

## 安装

```bash
# Helm 安装
helm repo add kubearmor https://kubearmor.github.io/charts
helm install kubearmor kubearmor/kubearmor-operator -n kubearmor --create-namespace
# 应用安全策略
kubectl apply -f - <<EOF
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata: { name: ksp-block-exec }
spec:
  severity: 8
  selector:
    matchLabels: { app: web }
  process:
    matchPaths:
    - path: /bin/bash
  action: Block
EOF
# 生成策略建议
karmor recommend --pod web-app
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **KubeArmor** | LSM 内核级、策略丰富 | LSM 支持因 OS 而异 |
| Falco | 运行时检测、eBPF 原生 | 仅检测，不执行阻止 |
| Tetragon | eBPF 高性能 | 配置复杂 |
| NeuVector | 全栈安全 | 商业产品 |

## 架构定位

在 CNCF 生态中，KubeArmor 属于 **Security / Runtime Protection** 类别，是容器运行时强制执行（Enforcement）的代表性项目。它与 Falco（检测）、Cilium（网络）互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- networking.md|cilium-ebpf-networking]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[keycloak]] — Keycloak
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubearmor
- [[实体/tokenetes.md|Tokenetes]]
- [[实体/containerssh.md|ContainerSSH]]
- [[实体/parsec.md|Parsec]]
- [[实体/athenz.md|Athenz]]
- [[实体/keylime.md|Keylime]]
- [[实体/cartography.md|Cartography]]
- [[实体/bank-vaults.md|Bank-Vaults]]
- [[实体/hexa.md|Hexa]]
- [[实体/paralus.md|Paralus]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
