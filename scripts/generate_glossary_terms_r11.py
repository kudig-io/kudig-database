#!/usr/bin/env python3
"""Round 11: 剩余缺失术语批量展开（25个）"""
from pathlib import Path
BASE = Path("domain-17-system-foundation/topic-dictionary")

def w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists():
        return False
    tks = "\n".join(f"- {k}" for k in dict.fromkeys([zh, en, "dictionary"]))
    tg = "\n".join(f"- {t}" for t in tags)
    r = rel or "- [[domain-17-system-foundation/topic-dictionary/k8s-glossary|K8s Glossary]]"
    fp.parent.mkdir(parents=True, exist_ok=True)
    fp.write_text(f"""---
title: {zh}
description: '{ov[:80]}...'
category: dictionary
tags:
- k8s
- glossary
{tg}
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- {zh} 是什么
- {en} 详解
trigger_keywords:
{tks}
prerequisites:
- kubernetes
created: 2026-06
---

# {zh}（{en}）

## 概述

{ov}

## 核心概念/原理

{core}

## 关键机制或特性

{mech}

## 使用场景与最佳实践

{use}

## 参考链接

{refs}

## Related

{r}
""", encoding="utf-8")
    return True

TERMS = [
    # ── Tooling ──
    ("tooling", "stacker", "Stacker 容器构建", "Stacker",
     ["tooling", "container", "build"],
     "Stacker 是 Project Atomic（Red Hat）开源的容器镜像构建工具，使用声明式 YAML 定义构建步骤，支持层缓存和 OCI 格式输出，是 Buildah/Kaniko 之外的容器构建方案。",
     "- **声明式构建**：YAML 定义镜像构建步骤\n- **层缓存**：智能缓存未变更的层\n- **OCI 输出**：生成标准 OCI 镜像\n- **无 Daemon**：无需 Docker Daemon 即可构建",
     "- stacker.yaml 定义构建流程\n- 支持从基础镜像/Dockerfile/OCI 开始\n- 层绑定（bind）和导入（import）\n- 构建参数化\n- 多阶段构建支持\n- 签名和推送",
     "- CI/CD Pipeline 的容器构建\n- 无 Docker Daemon 环境的镜像构建\n- 声明式镜像定义\n- 层缓存优化的构建流程\n- Buildah/Kaniko 的替代方案",
     "- https://stackerbuild.io/\n- https://github.com/project-stacker/stacker",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/buildpacks|Buildpacks]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/shipwright|Shipwright]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/docker|Docker]]"),

    ("tooling", "ko", "ko Go 容器构建", "ko",
     ["tooling", "go", "container"],
     "ko 是 Google 开源的工具，无需 Dockerfile 即可将 Go 程序构建为容器镜像，直接编译 Go 二进制并打包为 OCI 镜像，是 Go 生态的容器化标准工具。",
     "- **Go 原生**：无需 Dockerfile，直接编译 Go 代码\n- **极快构建**：利用 Go 编译缓存，构建速度极快\n- **Google 开源**：Knative/Tekton 等项目的标准构建工具\n- **多架构**：支持 amd64/arm64 等多架构构建",
     "- `ko build` 编译并推送镜像\n- `ko resolve` 替换 YAML 中的镜像引用\n- `ko apply` 构建并直接 kubectl apply\n- 多架构构建（`--platform`）\n- `.ko.yaml` 配置基础镜像\n- SBOM 自动生成\n- 与 GitHub Actions 集成",
     "- Go 微服务的容器化\n- Knative/Tekton 开发流程\n- CI/CD 中的快速构建\n- 多架构镜像的生成\n- Go 项目的容器化最佳实践",
     "- https://ko.build/\n- https://github.com/ko-build/ko",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/buildpacks|Buildpacks]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/docker|Docker]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/podman|Podman]]"),

    ("tooling", "devspace", "DevSpace 云开发环境", "DevSpace",
     ["tooling", "development", "k8s"],
     "DevSpace 是 Loft Labs 开源的云原生开发工具，为 Kubernetes 提供一键式开发环境搭建、实时同步和热重载，简化 K8s 上的开发工作流。",
     "- **一键环境**：`devspace dev` 一键进入 K8s 开发环境\n- **实时同步**：文件变更实时同步到容器\n- **Loft Labs**：vcluster 团队出品\n- **DevContainer 兼容**：支持 devcontainer.json",
     "- devspace.yaml 声明式开发环境配置\n- 文件双向同步（rsync 式）\n- 端口转发自动配置\n- 终端代理（直接在 K8s Pod 中执行命令）\n- Helm/Kubectl 部署集成\n- 多服务并行开发\n- Plugin 扩展",
     "- K8s 微服务的本地开发\n- 多服务联调环境\n- 替代 Telepresence 的开发方案\n- 团队的标准化开发环境\n- 开发/测试环境的快速搭建",
     "- https://devspace.sh/\n- https://github.com/loft-sh/devspace",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/telepresence|Telepresence]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/skaffold|Skaffold]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/devfile|Devfile]]"),

    ("tooling", "xregistry", "xRegistry 增强型注册表", "xRegistry",
     ["tooling", "registry", "oci"],
     "xRegistry 是 CNCF 社区的增强型 OCI 注册表实现，在标准 Distribution 基础上扩展了搜索、标签管理、配额控制和多仓库同步等企业级功能。",
     "- **增强 OCI**：在标准 OCI Registry 基础上扩展\n- **企业功能**：搜索/配额/同步/审计\n- **CNCF 社区**：活跃的 Registry 增强社区\n- **多后端**：支持文件系统/S3/GCS 存储",
     "- 全文搜索（镜像/标签/注释）\n- 配额管理（存储/拉取频率限制）\n- 多仓库同步（跨区域复制）\n- 访问控制（RBAC）\n- 审计日志\n- 标签管理（不可变标签/保留策略）\n- Garbage Collection 增强",
     "- 企业内部的高级 OCI Registry\n- 多区域镜像同步\n- 存储配额和成本控制\n- 合规要求下的审计和保留策略\n- Harbor 的轻量替代方案",
     "- https://github.com/xregistry/xregistry",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/distribution|Distribution]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/harbor|Harbor]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/zot|zot]]"),

    ("tooling", "dragonfly", "Dragonfly P2P 分发", "Dragonfly",
     ["tooling", "distribution", "p2p"],
     "Dragonfly 是阿里巴巴开源的 CNCF 孵化项目，基于 P2P（点对点）技术加速容器镜像和文件的分发，解决大规模集群中镜像拉取的性能瓶颈问题。",
     "- **P2P 加速**：节点间共享已下载的层，减少 Registry 压力\n- **大规模验证**：阿里巴巴/蚂蚁集团生产环境使用\n- **CNCF 孵化**：阿里开源\n- **透明代理**：无需修改容器运行时配置",
     "- DFDaemon 节点代理\n- Scheduler 调度 P2P 节点\n- Manager 集群管理\n- 支持 Docker/Containerd/Podman/Nydus\n- 预热（Preheating）机制\n- 分片下载和断点续传\n- 与 Harbor 集成",
     "- 大规模集群的镜像拉取加速\n- CI/CD 构建产物的快速分发\n- 跨区域的镜像同步加速\n- Registry 带宽瓶颈的缓解\n- K8s 扩容时的镜像分发优化",
     "- https://d7y.io/\n- https://github.com/dragonflyoss/Dragonfly2",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/distribution|Distribution]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/harbor|Harbor]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd|containerd]]"),

    ("tooling", "microcks", "Microcks API 模拟测试", "Microcks",
     ["tooling", "testing", "api"],
     "Microcks 是 CNCF Sandbox 项目，提供 API 的模拟和测试能力，支持 OpenAPI/AsyncAPI/gRPC/GraphQL 等多种 API 规范的 Mock 生成和契约测试。",
     "- **API Mocking**：从 API 规范自动生成 Mock 服务\n- **契约测试**：验证 API 实现是否符合规范\n- **CNCF Sandbox**：活跃的 API 测试社区\n- **多协议**：REST/SOAP/gRPC/GraphQL/AsyncAPI",
     "- 导入 OpenAPI/AsyncAPI/Postman/GraphQL 规范\n- 自动生成 Mock 端点和响应\n- 契约测试（Conformance Testing）\n- 测试数据管理（Dataset）\n- 延迟和错误模拟\n- Kubernetes Operator 部署\n- CLI 和 Web UI",
     "- 微服务 API 的集成测试\n- API 规范的 Mock 服务\n- 消费者驱动的契约测试\n- 前后端并行开发\n- API 变更的影响评估",
     "- https://microcks.io/\n- https://github.com/microcks/microcks",
     "- [[domain-17-system-foundation/topic-dictionary/networking/grpc|gRPC]]\n- [[domain-17-system-foundation/topic-dictionary/networking/connect-rpc|Connect RPC]]\n- [[domain-17-system-foundation/topic-dictionary/operations/kube-burner|kube-burner]]"),

    # ── Security ──
    ("security", "cedar", "Cedar 策略语言", "Cedar",
     ["security", "policy", "authorization"],
     "Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified Permissions 采用。",
     "- **策略语言**：专为授权决策设计的声明式语言\n- **AWS 背景**：Amazon Verified Permissions 的核心引擎\n- **形式化验证**：支持策略的形式化验证\n- **应用集成**：嵌入到应用中的授权引擎",
     "- Entity（用户/资源/动作的定义）\n- Policy（when/unless 条件的策略规则）\n- 层次化资源模型\n- 策略组（Policy Set）管理\n- 策略评估（is-authorized API）\n- 形式化验证工具\n- SDK（Rust/Java/Go）",
     "- 应用的细粒度授权策略\n- 多租户 SaaS 的权限管理\n- AWS 资源的 IAM 策略\n- 替代 OPA 的轻量策略方案\n- 需要形式化验证的安全策略",
     "- https://www.cedarpolicy.com/\n- https://github.com/cedar-policy/cedar",
     "- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]\n- [[domain-17-system-foundation/topic-dictionary/security/openfga|OpenFGA]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]"),

    ("security", "cartography", "Cartography 资产图谱", "Cartography",
     ["security", "asset-management", "graph"],
     "Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。",
     "- **资产图谱**：自动发现和关联云基础设施资产\n- **Neo4j 可视化**：图数据库驱动的资产关系视图\n- **Lyft 开源**：经过 Lyft 大规模生产验证\n- **多云支持**：AWS/GCP/Azure/K8s 资产采集",
     "- 自动化资产采集（Cron 调度）\n- 多云资产关联（EC2→S3→IAM→VPC）\n- Kubernetes 资产采集\n- 安全分析查询（Cypher 查询语言）\n- 自定义分析插件\n- 差异检测（变更追踪）\n- Grafana Dashboard 集成",
     "- 云基础设施的资产盘点\n- 安全态势的可视化分析\n- 资产关系的自动化发现\n- 合规审计的资产报告\n- 安全团队的攻击面分析",
     "- https://cartography-cncf.github.io/cartography/\n- https://github.com/lyft/cartography",
     "- [[domain-17-system-foundation/topic-dictionary/security/kubescape|Kubescape]]\n- [[domain-17-system-foundation/topic-dictionary/security/trivy|Trivy]]\n- [[domain-17-system-foundation/topic-dictionary/security/cloud-custodian|Cloud Custodian]]"),

    ("security", "keylime", "Keylime 远程证明", "Keylime",
     ["security", "attestation", "tpm"],
     "Keylime 是 MITRE 开源的 CNCF Sandbox 项目，基于 TPM（可信平台模块）提供远程证明（Remote Attestation）能力，验证远程系统的完整性和可信状态。",
     "- **远程证明**：验证远程系统的启动和运行状态\n- **TPM 基础**：利用 TPM 2.0 硬件信任根\n- **CNCF Sandbox**：MITRE 主导\n- **Linux 专注**：为 Linux 系统设计",
     "- Agent（被测系统）+ Verifier（验证者）架构\n- TPM Quote 采集和验证\n- IMA（Integrity Measurement Architecture）日志\n- 可信启动链验证\n- 密钥分发和绑定\n- 证书管理\n- REST API 和 CLI",
     "- 服务器启动完整性验证\n- 边缘设备的信任验证\n- 合规要求的系统完整性监控\n- 零信任架构的硬件信任根\n- 机密计算的远程证明",
     "- https://keylime.dev/\n- https://github.com/keylime/keylime",
     "- [[domain-17-system-foundation/topic-dictionary/security/confidential-containers|Confidential Containers]]\n- [[domain-17-system-foundation/topic-dictionary/security/parsec|PARSEC]]\n- [[domain-17-system-foundation/topic-dictionary/security/spire|SPIRE]]"),

    ("security", "open-policy-containers", "OPCo 策略容器", "Open Policy Containers",
     ["security", "policy", "oci"],
     "Open Policy Containers（OPCo）将安全策略打包为 OCI 镜像，通过标准容器 Registry 分发和管理策略，实现策略的版本控制和跨平台分发。",
     "- **策略即 OCI**：将策略打包为标准 OCI 镜像\n- **Registry 分发**：通过容器 Registry 管理策略\n- **多引擎**：支持 OPA/Rego/Kyverno 等策略引擎\n- **OCI 标准**：利用 OCI Artifact 规范",
     "- `policy push/pull/sign` 管理策略镜像\n- 支持 Rego/Kyverno/Cedar 策略格式\n- OCI Artifact 存储策略\n- 策略签名和验证（Cosign/Notation）\n- 策略版本管理和标签\n- 与 Gatekeeper/Kyverno 集成",
     "- 策略的版本控制和分发\n- 多集群的策略同步\n- GitOps 策略管理\n- 策略的安全签名和验证\n- 策略库的集中管理",
     "- https://openpolicycontainers.com/\n- https://github.com/opcr-io/policy",
     "- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]\n- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]"),

    ("security", "containerssh", "ContainerSSH SSH 代理", "ContainerSSH",
     ["security", "ssh", "container"],
     "ContainerSSH 是开源的 SSH 服务器，将 SSH 连接代理到 Kubernetes Pod 或 Docker 容器中运行，为运维人员提供安全的容器 Shell 访问方式。",
     "- **SSH 代理**：SSH 连接到容器/Pod 内部\n- **认证代理**：支持 OIDC/LDAP/Kerberos 认证\n- **安全审计**：完整的 SSH 会话审计和录制\n- **多后端**：Kubernetes/Docker/本地 Shell",
     "- SSH 协议服务器（标准 SSH 客户端连接）\n- 后端：Kubernetes/Docker/Local\n- OIDC/LDAP 认证后端\n- 会话录制和回放\n- 配置注入（环境变量/卷）\n- 速率限制和访问控制\n- Prometheus 指标",
     "- 运维人员的安全 Shell 访问\n- 替代 `kubectl exec` 的 SSH 方案\n- 合规要求下的会话审计\n- 开发团队的容器远程访问\n- 跳板机/堡垒机的容器化替代",
     "- https://containerssh.github.io/\n- https://github.com/ContainerSSH/ContainerSSH",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl|kubectl]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/stern|Stern]]\n- [[domain-17-system-foundation/topic-dictionary/security/rbac|RBAC]]"),

    # ── Observability ──
    ("observability", "drasi", "Drasi 变更检测", "Drasi",
     ["observability", "change-detection", "microsoft"],
     "Drasi 是微软开源的 CNCF Sandbox 项目，实时检测基础设施和应用状态的变化，通过连续查询（Continuous Query）监控数据变化并触发反应。",
     "- **变更检测**：实时监控数据状态的变化\n- **连续查询**：基于 Cypher 的连续查询引擎\n- **CNCF Sandbox**：微软主导\n- **事件驱动**：变化触发自动化反应",
     "- Source 定义数据源（K8s/Gremlin/PostgreSQL）\n- ContinuousQuery 定义监控条件\n- Reaction 定义变化响应\n- 基于 Cypher 图查询语言\n- 状态变化追踪（Added/Updated/Deleted）\n- Kubernetes 资源变化监控\n- Webhook/Log/Teams 反应",
     "- 基础设施变更的实时监控\n- K8s 资源状态变化的告警\n- 应用配置的漂移检测\n- 安全事件的实时响应\n- 运维自动化的事件触发",
     "- https://drasi.dev/\n- https://github.com/drasI-project/drasI",
     "- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]\n- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[domain-17-system-foundation/topic-dictionary/operations/kuberhealthy|Kuberhealthy]]"),

    # ── Networking ──
    ("networking", "sermant", "Sermant 服务治理", "Sermant",
     ["networking", "service-mesh", "java"],
     "Sermant 是华为开源的 CNCF Sandbox 项目，基于 Java Agent 的无代理服务治理框架，无需 Sidecar 即可实现流量管理、灰度发布和服务可观测性。",
     "- **Java Agent**：无 Sidecar 的服务治理\n- **零侵入**：通过字节码增强实现，应用无需修改\n- **CNCF Sandbox**：华为主导\n- **服务网格替代**：轻量级的服务治理方案",
     "- 流量管理（路由/灰度/限流/熔断）\n- 标签路由（基于 Header/参数）\n- 服务可观测性（追踪/指标）\n- 插件体系（可扩展治理能力）\n- Sermant Backend 管控面\n- 与 Istio 控制面兼容\n- 支持 Spring Cloud/Dubbo",
     "- Java 微服务的无侵入治理\n- 传统应用的灰度发布\n- Sidecar 不可用场景的替代\n- 服务路由和流量管理\n- 微服务的可观测性接入",
     "- https://sermant.io/\n- https://github.com/sermant-io/Sermant",
     "- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]\n- [[domain-17-system-foundation/topic-dictionary/networking/linkerd|Linkerd]]\n- [[domain-17-system-foundation/topic-dictionary/networking/kuma|Kuma]]"),

    ("networking", "kube-ovn", "Kube-OVN CNI", "Kube-OVN",
     ["networking", "cni", "ovn"],
     "Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态 IP/VPC/多子网/安全组等）。",
     "- **OVN/OVS 数据面**：高性能的虚拟网络\n- **企业网络**：VPC/子网/安全组/静态 IP\n- **CNCF Sandbox**：阿里云主导\n- **多租户网络**：完整的网络隔离能力",
     "- Subnet CRD（VPC/子网管理）\n- 固定 IP（Pod Annotation）\n- 安全组（Security Group）\n- QoS 带宽限制\n- 网络 ACL\n- 多网卡支持（Multus）\n- DPDK 加速",
     "- 企业级 K8s 网络方案\n- 需要 VPC/固定 IP 的场景\n- 多租户网络隔离\n- 安全组和 ACL 的精细控制\n- 电信/金融行业的网络合规",
     "- https://kubeovn.github.io/\n- https://github.com/kubeovn/kube-ovn",
     "- [[domain-17-system-foundation/topic-dictionary/networking/ovn-kubernetes|OVN-Kubernetes]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cilium|Cilium]]\n- [[domain-17-system-foundation/topic-dictionary/networking/antrea|Antrea]]"),

    ("networking", "kuadrant", "Kuadrant API 管理", "Kuadrant",
     ["networking", "api-management", "gateway"],
     "Kuadrant 是 Red Hat 开源的 CNCF Sandbox 项目，基于 Gateway API 提供 API 管理能力（认证/授权/限流），为 Kubernetes API 网关添加策略层。",
     "- **Gateway API 增强**：为 K8s Gateway 添加策略管理\n- **CNCF Sandbox**：Red Hat 主导\n- **策略层**：认证/授权/限流/速率控制\n- **多网关**：兼容 Envoy Gateway/Istio 等",
     "- AuthPolicy（认证和授权策略）\n- RateLimitPolicy（速率限制策略）\n- DNSPolicy（DNS 管理）\n- TLSPolicy（TLS 管理）\n- 与 Gateway API 无缝集成\n- OPA 策略引擎后端\n- 多网关供应商支持",
     "- API 网关的策略管理\n- 微服务的认证和授权\n- API 限流和保护\n- Gateway API 的企业增强\n- 多网关的统一策略管理",
     "- https://kuadrant.io/\n- https://github.com/Kuadrant/kuadrant-operator",
     "- [[domain-17-system-foundation/topic-dictionary/networking/envoy-gateway|Envoy Gateway]]\n- [[domain-17-system-foundation/topic-dictionary/networking/kgateway|KGateway]]\n- [[domain-17-system-foundation/topic-dictionary/security/openfga|OpenFGA]]"),

    ("networking", "akri", "Akri 边缘设备发现", "Akri",
     ["networking", "edge", "iot"],
     "Akri 是微软开源的 CNCF Sandbox 项目，在 Kubernetes 上自动发现和暴露边缘设备（摄像头/GPU/USB 等），将异构硬件资源抽象为 K8s 可调度的资源。",
     "- **设备发现**：自动发现连接到节点的边缘设备\n- **CNCF Sandbox**：微软主导\n- **K8s 资源**：将设备暴露为 K8s 扩展资源\n- **边缘优化**：专为 IoT/边缘计算设计",
     "- Configuration CRD 定义设备发现规则\n- Instance CRD 表示发现的设备实例\n- Discovery Handler（ONVIF/OPC-UA/uDev 等）\n- 设备自动调度和绑定 Pod\n- 设备健康检查\n- Prometheus 指标\n- 自定义 Discovery Handler",
     "- IoT 设备的 K8s 管理\n- 边缘节点的硬件资源发现\n- 智能摄像头的 AI 推理\n- GPU/加速器的自动分配\n- 工业设备的容器化接入",
     "- https://docs.akri.sh/\n- https://github.com/project-akri/akri",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kubeedge|KubeEdge]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/hami|HAMi]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/openyurt|OpenYurt]]"),

    # ── Operations ──
    ("operations", "tinkerbell", "Tinkerbell 裸金属部署", "Tinkerbell",
     ["operations", "provisioning", "bare-metal"],
     "Tinkerbell 是 Equinix Metal 开源的 CNCF Sandbox 项目，提供裸金属服务器的声明式操作系统部署和生命周期管理，是 PXE/Kickstart 的现代化替代方案。",
     "- **裸金属部署**：自动化裸金属服务器的 OS 安装\n- **声明式**：CRD 定义硬件配置和安装工作流\n- **CNCF Sandbox**：Equinix Metal 主导\n- **容器化操作**：使用容器镜像执行安装步骤",
     "- Hardware CRD 定义硬件资源\n- Template CRD 定义安装工作流\n- Workflow CRD 执行状态\n- Action 容器镜像（安装步骤）\n- Hook（OS 安装镜像）\n- Tink Server/Worker 架构\n- iPXE 引导",
     "- 裸金属服务器的自动化部署\n- 数据中心的服务器生命周期管理\n- 边缘节点的 OS 安装\n- 裸金属 K8s 节点的自动化部署\n- PXE/Kickstart 的现代化替代",
     "- https://tinkerbell.org/\n- https://github.com/tinkerbell/tink",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm|kubeadm]]\n- [[domain-17-system-foundation/topic-dictionary/operations/kubean|Kubean]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/flatcar|Flatcar]]"),

    ("operations", "kured", "Kured 节点重启", "Kured",
     ["operations", "node", "reboot"],
     "Kured（KUbernetes REboot Daemon）是开源的 K8s 节点重启守护进程，在节点需要重启时（如内核更新）安全地逐节点重启，确保工作负载的平滑迁移。",
     "- **安全重启**：逐节点排空（cordon + drain）后重启\n- **锁机制**：确保同一时间只有一个节点重启\n- **社区成熟**：广泛使用的节点维护工具\n- **轻量部署**：DaemonSet 方式运行",
     "- 检测重启信号（`/var/run/reboot-required`）\n- 节点排空（Pod 迁移）\n- 节点重启\n- 节点恢复（uncordon）\n- 分布式锁（K8s Lock API）\n- 时间窗口控制\n- Prometheus 指标",
     "- 内核更新后的节点重启\n- 安全补丁的自动化应用\n- 节点维护的自动化编排\n- 大规模集群的滚动重启\n- OS 更新的自动化管理",
     "- https://kubereboot.github.io/kured/\n- https://github.com/kubereboot/kured",
     "- [[domain-17-system-foundation/topic-dictionary/operations/upgrade|升级]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/flatcar|Flatcar]]\n- [[domain-17-system-foundation/topic-dictionary/operations/kubean|Kubean]]"),

    ("operations", "holmesgpt", "HolmesGPT AI 排障", "HolmesGPT",
     ["operations", "ai", "diagnostics"],
     "HolmesGPT 是 Robusta 开源的 AI 辅助 Kubernetes 排障工具，利用 LLM 分析告警和日志，自动生成故障诊断报告和修复建议，是 K8sGPT 的增强替代方案。",
     "- **AI 排障**：利用 LLM 分析告警/日志/指标\n- **多数据源**：集成 Prometheus/Grafana/Loki/Elasticsearch\n- **Robusta 出品**：K8s 可观测性平台团队\n- **自动诊断**：告警触发后自动分析根因",
     "- 告警自动分析（Alert → Root Cause）\n- 多数据源集成（Prometheus/Loki/ES）\n- Runbook 自动执行\n- 多 LLM 后端（OpenAI/Azure/Local）\n- Slack/Teams 集成\n- 历史事件学习\n- 修复建议生成",
     "- On-Call 告警的快速诊断\n- 复杂故障的 AI 辅助分析\n- Runbook 的自动化执行\n- 运维团队的 AI 助手\n- 事件管理的效率提升",
     "- https://github.com/robusta-dev/holmesgpt\n- https://home.robusta.dev/",
     "- [[domain-17-system-foundation/topic-dictionary/operations/k8sgpt|K8sGPT]]\n- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]\n- [[domain-17-system-foundation/topic-dictionary/observability/loki|Loki]]"),

    # ── Platform ──
    ("platform-engineering", "armada", "Armada 批量调度", "Armada",
     ["platform-engineering", "batch", "multi-cluster"],
     "Armada 是 G-Research 开源的 CNCF Sandbox 项目，专为大规模批量工作负载设计的多集群调度系统，管理跨多个 K8s 集群的队列和作业优先级。",
     "- **多集群批量调度**：跨多个 K8s 集群调度批处理作业\n- **队列管理**：多级队列和优先级抢占\n- **CNCF Sandbox**：G-Research（量化对冲基金）主导\n- **大规模**：支撑数十万核的批量计算",
     "- JobSet CRD 定义批量作业集\n- Queue CRD 多级队列管理\n- 优先级和抢占策略\n- 跨集群作业分发\n- 资源公平共享（Fair Share）\n- 作业状态聚合\n- Lookout UI 作业监控",
     "- 量化研究的批量计算\n- AI 训练任务的多集群调度\n- 大规模数据处理 Pipeline\n- 多团队的计算资源公平分配\n- HPC 工作负载的 K8s 管理",
     "- https://armadaproject.io/\n- https://github.com/armadaproject/armada",
     "- [[domain-17-system-foundation/topic-dictionary/scheduling/volcano|Volcano]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/koordinator|Koordinator]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada|Karmada]]"),

    ("platform-engineering", "kusionstack", "Kusion 配置管理", "KusionStack",
     ["platform-engineering", "configuration", "iac"],
     "KusionStack 是蚂蚁集团开源的 CNCF Sandbox 项目，面向应用的配置管理平台，使用 KCL 语言定义应用配置，整合 Kubernetes/Terraform/云资源为统一的应用交付。",
     "- **应用配置管理**：面向应用的统一配置定义\n- **KCL 语言**：类型安全的配置语言\n- **CNCF Sandbox**：蚂蚁集团主导\n- **多后端**：K8s/Terraform/云 API 统一交付",
     "- AppConfiguration 模型定义应用\n- KCL 语言编写配置\n- Module 可复用配置模块\n- Workspace 多环境管理\n- 预览（Preview）变更影响\n- 与 Kubernetes/Terraform 集成\n- Kusion API Server",
     "- 企业内部的应用配置标准化\n- 多环境（dev/staging/prod）配置管理\n- IaC 的编程化管理\n- 开发者自助的应用交付\n- 复杂应用的声明式定义",
     "- https://kusionstack.io/\n- https://github.com/kusionstack/kusion",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/kcl|KCL]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane|Crossplane]]"),

    # ── Storage ──
    ("storage", "cubefs", "CubeFS 分布式文件系统", "CubeFS",
     ["storage", "filesystem", "cncf"],
     "CubeFS（原 CFS）是京东开源的 CNCF 孵化项目，云原生分布式文件系统，支持 POSIX/SDK/S3 多协议访问，为 AI 训练和大数据提供高吞吐的共享文件存储。",
     "- **分布式文件**：POSIX 兼容的分布式文件系统\n- **多协议**：POSIX/SDK/S3/HDFS 访问\n- **CNCF 孵化**：京东/OPPO 等联合推动\n- **AI 优化**：为 AI 训练优化的大文件吞吐",
     "- Master/MetaNode/DataNode/ObjectNode 架构\n- 多副本和纠删码（Erasure Coding）\n- 元数据分区和水平扩展\n- S3 兼容 API\n- 快照和克隆\n- 多租户配额管理\n- CSI 驱动",
     "- AI 训练的共享文件存储\n- 大数据分析的分布式文件系统\n- 容器化应用的高性能存储\n- 多租户文件存储平台\n- 对象存储和文件存储的统一",
     "- https://cubefs.io/\n- https://github.com/cubefs/cubefs",
     "- [[domain-17-system-foundation/topic-dictionary/storage/ceph|Ceph]]\n- [[domain-17-system-foundation/topic-dictionary/storage/fluid|Fluid]]\n- [[domain-17-system-foundation/topic-dictionary/storage/minio|MinIO]]"),

    ("storage", "hwameistor", "HwameiStor 本地存储", "HwameiStor",
     ["storage", "local-storage", "ha"],
     "HwameiStor 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供高可用本地存储管理，自动管理本地磁盘并通过数据复制实现本地卷的高可用。",
     "- **本地存储管理**：自动发现和管理节点本地磁盘\n- **高可用**：本地卷的数据复制和故障转移\n- **CNCF Sandbox**：DaoCloud 主导\n- **CSI 驱动**：标准 K8s CSI 集成",
     "- LocalDiskNode 自动发现本地磁盘\n- LocalVolume 本地卷管理\n- 数据复制（同步/异步）\n- 卷迁移（节点故障时自动迁移）\n- 磁盘健康检查\n- 存储池管理\n- 卷扩容",
     "- 本地磁盘的高可用管理\n- 数据库的本地存储方案\n- 存储成本优化（利用本地磁盘）\n- 边缘设备的存储管理\n- 需要高 IOPS 的有状态应用",
     "- https://hwameistor.io/\n- https://github.com/hwameistor/hwameistor",
     "- [[domain-17-system-foundation/topic-dictionary/storage/openebs|OpenEBS]]\n- [[domain-17-system-foundation/topic-dictionary/storage/longhorn|Longhorn]]\n- [[domain-17-system-foundation/topic-dictionary/storage/rook|Rook]]"),

    # ── Specialized ──
    ("specialized-workloads", "modelpack", "ModelPack 模型打包", "ModelPack",
     ["specialized-workloads", "ai-ml", "oci"],
     "ModelPack 是将 AI/ML 模型打包为 OCI 镜像的工具和规范，利用容器 Registry 分发和版本管理 AI 模型，实现模型的标准化管理和部署。",
     "- **模型即镜像**：将 AI 模型打包为 OCI 镜像\n- **Registry 分发**：通过标准容器 Registry 分发模型\n- **版本管理**：利用镜像标签管理模型版本\n- **跨平台**：与 K8s/Kserve/Seldon 集成",
     "- 模型文件打包为 OCI Layer\n- 模型元数据（框架/精度/指标）\n- 模型签名和验证\n- 多 Registry 同步\n- 与 KServe/Seldon/BentoML 集成\n- Helm Chart 模型部署\n- 模型拉取和缓存",
     "- AI 模型的版本管理\n- 模型的分发和部署\n- MLOps 的模型 Registry\n- 多环境模型的同步\n- 模型的签名和合规管理",
     "- https://github.com/modelpack/modelpack",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/seldon|Seldon]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/kaito|KAITO]]"),

    ("specialized-workloads", "kitops", "KitOps ML 打包", "KitOps",
     ["specialized-workloads", "ai-ml", "oci"],
     "KitOps 是 Jozu 开源的 CNCF Sandbox 项目，为 AI/ML 模型和数据集提供 OCI 打包和分发能力，将 ML 模型管理纳入标准的 DevOps 工具链。",
     "- **ML OCI 打包**：模型/数据集/代码的 OCI 打包\n- **DevOps 集成**：ML 资产纳入标准 CI/CD\n- **CNCF Sandbox**：Jozu 主导\n- **Kitfile 规范**：声明式定义 ML 包内容",
     "- Kitfile YAML 定义 ML 包\n- `kit pack` 打包为 OCI 镜像\n- `kit push/pull` 推送/拉取到 Registry\n- 模型/数据集/代码统一管理\n- 签名和验证（Cosign/Notation）\n- Kitfile 参数化\n- 多 Registry 支持",
     "- AI 模型的 DevOps 管理\n- 模型版本控制和分发\n- ML Pipeline 的资产打包\n- 团队协作的模型共享\n- 合规要求下的模型审计",
     "- https://kitops.ml/\n- https://github.com/jozu-ai/kitops",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/modelpack|ModelPack]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]"),
]

created, skipped = 0, 0
for t in TERMS:
    r = w(*t)
    if r:
        created += 1
        print(f"  + {t[0]}/{t[1]}.md")
    else:
        skipped += 1
        print(f"  = {t[0]}/{t[1]}.md (已存在)")

print(f"\n新创建: {created}, 跳过: {skipped}")
