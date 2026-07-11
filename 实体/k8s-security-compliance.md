---
title: 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
description: '# 安全合规'
summary: '# 安全合规'
category: reference
tags:
- k8s
- security
- rbac
- networkpolicy
- runtime-security
- zero-trust
- istio
- falco
- ebpf
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全合规：RBAC、网络安全策略、运行时安全与零信任架构 是什么
- 如何 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
trigger_keywords:
- 安全合规：RBAC
- 网络安全策略
- 运行时安全与零信任架构
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 安全合规

> **CNCF 状态**: 实践指南 | **类别**: Security & Compliance | **主要语言**: YAML, Go

## 概述

Kubernetes 安全合规实践是一个涵盖集群安全加固、合规审计、策略执行的综合性方法论体系。它整合了 CIS Benchmark、NIST SP 800-190、Pod Security Standards、OPA Gatekeeper 等多个安全框架和工具，为 K8s 生产环境提供从控制平面到工作负载的全栈安全保障。该体系涵盖身份认证、RBAC、网络策略、Pod 安全、密钥管理、审计日志、供应链安全等多个维度，帮助企业满足 SOC2、PCI-DSS、等保 2.0 等合规要求。

## Key Features（核心能力）

- **CIS Benchmark 合规**：对 K8s 控制平面和节点进行 CIS 安全基线扫描（kube-bench）
- **Pod Security Standards**：通过 PSA 强制执行 Privileged/Baseline/Restricted 三个安全级别
- **策略即代码**：使用 OPA Gatekeeper 或 Kyverno 定义和执行安全策略
- **运行时安全**：通过 Falco、Tracee 等工具检测异常行为和容器逃逸
- **供应链安全**：SBOM 生成、镜像签名验证、SLSA 合规
- **审计与合规报告**：自动生成合规报告，满足各类审计要求

## 架构与工作原理

安全合规体系分为四个层次：基础设施层（etcd 加密、API Server TLS、节点加固）、集群层（RBAC、NetworkPolicy、Admission Controller）、工作负载层（Pod Security、镜像扫描、运行时检测）、合规层（审计日志、合规报告、策略引擎）。通过纵深防御策略（Defense in Depth）在各层实施安全控制，形成多层防护网。

## K8s 集成

安全合规实践直接在 Kubernetes API 对象层面实施：RBAC（ClusterRole/RoleBinding）控制 API 访问；NetworkPolicy 限制 Pod 间通信；Pod Security Admission 替代旧版 PSP；ValidatingWebhook 在 API 准入阶段执行安全策略；Audit Policy 记录 API 访问日志。kube-bench、kube-hunter 等工具可自动扫描集群安全配置和漏洞。

## 生产用例

- **金融行业合规**：满足 PCI-DSS 等金融安全标准对容器化工作负载的要求
- **多租户隔离**：通过 RBAC + NetworkPolicy + Pod Security 实现租户间强隔离
- **供应链安全**：在 CI/CD 中强制镜像签名验证和安全扫描
- **安全事件响应**：通过审计日志和运行时检测快速定位和响应安全事件

## 安装与快速开始

```bash
# kube-bench CIS Benchmark 扫描
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml

# Kyverno 策略引擎
kubectl create -f https://github.com/kyverno/kyverno/releases/download/v1.12.0/install.yaml
```

## 对比替代方案

相比传统主机安全方案，K8s 安全合规实践需要覆盖声明式 API、动态调度、网络抽象等云原生特性。相比单个安全工具，该体系强调多工具协同和纵深防御策略。

## Related

- [[实体/tetragon.md|tetragon]] — Tetragon
- [[istio]] — Istio
- [[falco]] — Falco
- [[linkerd]] — Linkerd
- [[kubearmor]] — KubeArmor

- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]
- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]


<!-- risk-assessed -->
