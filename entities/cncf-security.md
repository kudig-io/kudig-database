---
title: CNCF 安全与合规项目全景
description: '# CNCF 安全与合规项目全景'
category: entities
tags:
- k8s
- cncf
- security
- policy
- identity
- supply-chain
- prometheus
- istio
- envoy
- cilium
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 安全与合规项目全景 是什么
- 如何 CNCF 安全与合规项目全景
trigger_keywords:
- CNCF
- 安全与合规项目全景
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- tls-basics
- policy-basics
---

# CNCF 安全与合规项目全景

> 聚合页面 | 涵盖 31 个 CNCF 安全项目

## 概述

云原生安全覆盖 **供应链安全**、**身份与访问管理**、**运行时安全**、**策略与合规** 四大领域。CNCF 安全项目形成纵深防御体系。

---

## 供应链安全（Supply Chain Security）

### [[in-toto]] — 毕业项目

in-toto 提供软件供应链完整性验证框架。

- 确保从源码到部署的每个步骤未被篡改
- 与 SLSA 框架深度集成

### [[tuf]] — 毕业项目

TUF（The Update Framework）保护软件更新过程免受中间人攻击。

### [[notary-project]] — 孵化项目

Notary Project 提供 OCI 镜像签名和验证。

- 支持 Cosign 兼容的签名方式
- 与 [[harbor]] 集成实现镜像策略

### [[ratify]] — 沙箱项目

Ratify 是可插拔的制品验证引擎，用于 K8s 准入控制。

### [[eraser]] — 沙箱项目

Eraser 自动清理 Kubernetes 节点上的未使用容器镜像。

### [[copa]] — 沙箱项目

COPA（Container Patching）无需重建镜像即可修补容器漏洞。

---

## 运行时安全（Runtime Security）

### [[falco]] — 毕业项目

Falco 是云原生运行时威胁检测引擎。

- 基于系统调用（syscall）和 K8s 审计日志
- 实时检测异常行为（异常进程、文件访问、网络连接）
- 与 [[prometheus]] 集成指标输出

### [[entities/tetragon.md|tetragon]] — 核心项目（基于 Cilium）

Tetragon 基于 eBPF 的安全可观测性和运行时执行。

- 内核级安全事件监控
- 策略执行（阻断恶意行为）
- 与 Cilium 生态集成

### [[entities/trivy.md|trivy]] — 核心工具

Trivy 是全功能安全扫描器。

- 容器镜像漏洞扫描
- IaC 配置审计
- SBOM 生成
- 密钥泄露检测

### [[kubearmor]] — 沙箱项目

KubeArmor 利用 LSM（Linux Security Module）进行 K8s 运行时强制访问控制。

### [[domain-19-landscape-references/sandbox/inspektor-gadget/inspektor-gadget.md|inspektor-gadget]] — 沙箱项目

Inspektor Gadget 基于 eBPF 的 K8s 调试和安全检查工具。

### [[confidential-containers]] — 沙箱项目

Confidential Containers 在 TEE（可信执行环境）中运行 K8s 工作负载。

### [[parsec]] — 沙箱项目

Parsec 提供跨平台的安全硬件抽象接口。

---

## 策略引擎（Policy Engine）

### [[opa]] — 毕业项目

OPA（Open Policy Agent）是通用策略引擎。

- Rego 声明式策略语言
- 用于 K8s 准入控制（Gatekeeper）、API 授权、数据过滤
- 与 [[kyverno]] 形成互补方案

### [[kyverno]] — 孵化项目

Kyverno 是 Kubernetes 原生策略引擎。

- 基于 YAML 的策略定义（无需学习 Rego）
- 资源验证、变异、生成
- 与 K8s RBAC 集成

### [[kubewarden]] — 沙箱项目

Kubewarden 使用 WebAssembly（Wasm）编写策略。

### [[open-policy-containers]] — 沙箱项目

Open Policy Containers 将 OPA 策略打包为 OCI 容器。

### [[cedar]] — 沙箱项目

Cedar 是 AWS 开源的细粒度授权策略语言。

### [[kubescape]] — 孵化项目

Kubescape 提供 K8s 集群安全扫描和合规检查。

- NSA/CISA 加固指南合规检测
- RBAC 分析和漏洞扫描

---

## 身份与访问管理（Identity & Access）

### [[spiffe]] / [[spire]] — 毕业项目

SPIFFE 提供统一的服务身份框架，SPIRE 是其参考实现。

- 自动颁发和轮换 X.509 证书
- 跨集群、跨云的服务身份互通
- mTLS 零信任网络基础

### [[cert-manager]] — 毕业项目

cert-manager 自动化 K8s TLS 证书管理。

- Let's Encrypt 自动签发
- 自签名 CA、HashiCorp Vault 集成
- 与 [[istio]]、[[envoy]] 等集成

### [[keycloak]] — 孵化项目

Keycloak 是开源身份和访问管理（IAM）平台。

- SSO、OAuth2、OIDC、SAML
- 用户联盟和社交登录

### [[openfga]] — 孵化项目

OpenFGA 是细粒度授权系统，基于 Google Zanzibar 模型。

### [[dex]] — 沙箱项目

Dex 是 OIDC 身份提供者代理，聚合多种上游身份源。

### [[oauth2-proxy]] — 沙箱项目

OAuth2 Proxy 为 Web 应用添加 OAuth2/OIDC 认证代理。

### [[paralus]] — 沙箱项目

Paralus 提供零信任 K8s 集群访问管理。

### [[athenz]] — 沙箱项目

Athenz 提供基于角色的服务和资源授权。

### [[tokenetes]] — 沙箱项目

Tokenetes 管理 K8s 中的服务令牌生命周期。

---

## 密钥与配置安全

### [[bank-vaults]] — 沙箱项目

Bank-Vaults 增强 HashiCorp Vault 的 K8s 集成。

### [[sops]] — 沙箱项目

SOPS（Secrets OPerationS）加密 YAML/JSON/ENV 密钥文件。

### [[external-secrets]] — 沙箱项目

External Secrets Operator 从外部密钥管理服务同步到 K8s Secrets。

### [[entities/vault.md|vault]] — 相关工具

HashiCorp Vault 提供集中式密钥管理（非 CNCF 但广泛使用）。

---

## 合规与审计

### [[cartography]] — 沙箱项目

Cartography 可视化云基础设施资产和关系图谱。

### [[oscal-compass]] — 沙箱项目

OSCAL Compass 将 NIST OSCAL 合规框架引入 K8s。

---

## 安全工具链

### [[containerssh]] — 沙箱项目

ContainerSSH 通过 SSH 动态创建容器会话。

### entities/copac — 沙箱项目（镜像修补）

COPA 无重建修补容器镜像漏洞。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 策略引擎（简单） | Kyverno（YAML） |
| 策略引擎（灵活） | OPA/Gatekeeper |
| 运行时检测 | Falco + Tetragon |
| 身份零信任 | SPIFFE/SPIRE + cert-manager |
| 镜像安全 | Trivy + Notary + Ratify |
| 密钥管理 | External Secrets + Vault |
| 合规审计 | Kubescape + OSCAL Compass |

---

## 相关页面

- [[entities/cncf-observability.md|cncf-observability]] — 可观测性
- [[entities/cncf-storage.md|cncf-storage]] — 存储与数据库
- [[entities/cncf-networking.md|cncf-networking]] — 网络与服务网格
- concepts/kubernetes-security-architecture — K8s 安全架构

## Related

- [[supply-chain-security]] — Software Supply Chain Security
- [[open-policy-containers]] — Open Policy Containers (OPCR)
- [[confidential-containers]] — Confidential Containers (CoCo)
- [[cilium]] — Cilium
- [[oauth2-proxy]] — OAuth2 Proxy
