#!/usr/bin/env python3
"""Round 6: 高频引用但尚无词条的 CN 生态术语批量展开（25个）"""
from pathlib import Path
BASE = Path("系统基础/topic-dictionary")

def w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists():
        return False
    tks = "\n".join(f"- {k}" for k in dict.fromkeys([zh, en, "dictionary"]))
    tg = "\n".join(f"- {t}" for t in tags)
    r = rel or "- [[系统基础/topic-dictionary/k8s-glossary|K8s Glossary]]"
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
    # ── Security ──
    ("security", "sops", "SOPS（Secrets OPerationS）", "SOPS",
     ["security", "secrets", "encryption"],
     "SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key Vault、age 或 PGP 作为密钥后端，实现 GitOps 友好的密钥管理。",
     "- **文件级加密**：对值（value）加密，保留键（key）和结构不变，便于 diff 和 review\n- **多密钥后端**：同时支持 AWS KMS、GCP KMS、Azure Key Vault、age、PGP\n- **审计与权限**：通过 .sops.yaml 配置加密规则（creation rules），按路径匹配密钥\n- **GitOps 集成**：加密后的文件可安全提交到 Git，配合 External Secrets 或 Sealed Secrets 使用",
     "- 支持加密/解密/原地编辑（in-place edit）操作\n- `sops --encrypt --in-place secrets.yaml` 加密文件\n- `sops --decrypt secrets.yaml` 解密到标准输出\n- 支持 SOPS + age 轻量方案，无需云 KMS\n- 与 External Secrets Operator 配合实现自动注入",
     "- GitOps 仓库中的 Secret/ConfigMap 加密存储\n- CI/CD pipeline 中的敏感配置管理\n- 多环境（dev/staging/prod）密钥分离\n- 合规要求下的密钥轮转与审计",
     "- https://github.com/getsops/sops\n- https://fluxcd.io/flux/guides/mozilla-sops/",
     "- [[系统基础/topic-dictionary/security/external-secrets|External Secrets]]\n- [[系统基础/topic-dictionary/security/vault|Vault]]\n- [[系统基础/topic-dictionary/security/opa|OPA]]"),

    ("security", "dex", "Dex 身份认证", "Dex",
     ["security", "identity", "oidc"],
     "Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitHub 等），为 Kubernetes 和其他应用提供统一的身份认证层。",
     "- **联邦身份**：充当 IdP 聚合层，统一 LDAP、SAML、GitHub、GitLab、Microsoft 等认证源\n- **OIDC 标准**：完整实现 OpenID Connect 协议，兼容所有 OIDC 客户端\n- **Kubernetes 原生**：广泛用于 K8s API Server 的 OIDC 认证配置\n- **轻量部署**：单二进制，可运行在 K8s 内或独立部署",
     "- 支持多种 Connector（LDAP、SAML 2.0、GitHub、GitLab、Bitbucket、Microsoft 等）\n- Token 刷新（refresh token）和离线访问\n- 连接器级别的组映射（group mapping）\n- 自定义模板的登录页面\n- 与 gangway/oauth2-proxy 配合实现 K8s 登录流程",
     "- Kubernetes 集群的统一身份认证网关\n- 多集群场景下的联邦认证\n- 企业 LDAP/AD 与 K8s RBAC 的桥接\n- 开发环境的 GitHub OAuth 快速接入",
     "- https://dexidp.io/\n- https://github.com/dexidp/dex",
     "- [[系统基础/topic-dictionary/security/oauth2-proxy|oauth2-proxy]]\n- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/security/vault|Vault]]"),

    ("security", "oauth2-proxy", "oauth2-proxy 认证代理", "oauth2-proxy",
     ["security", "authentication", "proxy"],
     "oauth2-proxy 是一个反向代理，为后端应用提供 OAuth2/OIDC 认证层。常用于为没有内置认证功能的 Kubernetes Dashboard、Prometheus、Grafana 等服务添加登录保护。",
     "- **认证代理**：在应用前端拦截请求，验证 OAuth2/OIDC Token\n- **多 Provider**：支持 Google、GitHub、GitLab、OIDC、Azure AD 等\n- **Kubernetes 友好**：以 Sidecar 或独立 Ingress 方式部署\n- **Cookie 管理**：加密 Cookie 存储认证状态，支持刷新",
     "- 基于 Cookie 的会话管理（支持 Redis 后端存储会话）\n- 邮件域名白名单、邮箱验证等访问控制\n- 配合 nginx-ingress 的 `auth-url` / `auth-signin` 注解使用\n- 支持 htpasswd 文件作为后备认证\n- 请求头注入用户信息（X-Auth-Request-User/Email）",
     "- 为 Prometheus/Grafana/K8s Dashboard 添加 SSO 登录\n- 内部服务的统一认证网关\n- 基于邮箱域名的简单访问控制\n- 与 Dex 配合实现企业级 SSO",
     "- https://oauth2-proxy.github.io/oauth2-proxy/\n- https://github.com/oauth2-proxy/oauth2-proxy",
     "- [[系统基础/topic-dictionary/security/dex|Dex]]\n- [[系统基础/topic-dictionary/networking/traefik|Traefik]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]"),

    ("security", "athenz", "Athenz 身份认证与授权", "Athenz",
     ["security", "identity", "authorization"],
     "Athenz 是 Yahoo 开源并捐赠给 CNCF 的服务平台，提供基于 X.509 证书的服务身份认证和细粒度角色授权（RBAC），专为大规模微服务和云原生环境设计。",
     "- **双功能**：同时提供服务身份认证（Service Authentication）和角色授权（Authorization）\n- **X.509 短证书**：自动签发和轮转短期服务身份证书，零信任架构基础\n- **集中策略管理**：中心化管理跨服务的访问策略\n- **大规模验证**：Yahoo 生产环境支撑数十万服务实例",
     "- ZMS（Athenz Management Service）：策略和域名管理\n- ZTS（Athenz Token Service）：Token 和证书签发\n- 支持 Kubernetes Workload Identity 集成\n- Athenz 域名模型：`<domain>.<service>` 命名体系\n- REST API 和 CLI 管理工具",
     "- 大规模微服务间的 mTLS 身份认证\n- 跨组织的服务访问授权管理\n- 零信任网络中的服务身份基础设施\n- 多云/混合云环境的统一身份层",
     "- https://www.athenz.io/\n- https://github.com/AthenZ/athenz",
     "- [[系统基础/topic-dictionary/security/spiffe-spire-identity|SPIFFE/SPIRE]]\n- [[系统基础/topic-dictionary/security/cert-manager|cert-manager]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]"),

    # ── Networking ──
    ("networking", "antrea", "Antrea 网络方案", "Antrea",
     ["networking", "cni", "ebpf"],
     "Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy、流量可视化、多集群网络等企业级功能，是 Calico/Cilium 之外的另一主流 CNI 选择。",
     "- **OVS 数据面**：基于 Open vSwitch 的高性能转发引擎\n- **完整 NetworkPolicy**：支持 K8s NetworkPolicy + Antrea 扩展策略（FQDN 策略、NodeNetworkPolicy）\n- **流量可视化**：内置 Flow Exporter 和 ClickHouse 集成\n- **多集群支持**：Antrea Multi-cluster 实现跨集群网络互通",
     "- OVS 流表驱动的转发规则管理\n- 支持 WireGuard 加密隧道\n- Egress / ExternalIP 管理\n- Traceflow 端到端连通性诊断\n- 与 Theia 可视化平台集成\n- 支持 Antrea Proxy（kube-proxy 替代）",
     "- 企业级 K8s 网络方案选型\n- 需要高级 NetworkPolicy（FQDN、Node 级别）\n- 网络流量审计与可视化需求\n- 多集群网络互联场景",
     "- https://antrea.io/\n- https://github.com/antrea-io/antrea",
     "- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/networking/cni|CNI]]\n- [[系统基础/topic-dictionary/networking/networkpolicy|NetworkPolicy]]"),

    ("networking", "submariner", "Submariner 多集群网络", "Submariner",
     ["networking", "multi-cluster", "cni"],
     "Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群 Service 发现和 Pod 直通，无需依赖外部网络方案。",
     "- **跨集群网络**：在不同 K8s 集群间建立安全的 IPsec/WireGuard 隧道\n- **Service 发现**：基于 MCS（Multi-Cluster Services）API 实现跨集群服务发现\n- **CNI 无关**：兼容 Flannel、Calico、Cilium、OVN 等各种 CNI\n- **Gateway 模型**：每个集群通过 Gateway 节点建立隧道连接",
     "- 支持 IPsec 和 WireGuard 两种隧道协议\n- Globalnet 解决集群 CIDR 重叠问题\n- 与 K8s MCS API 标准对齐\n- Submariner Operator 简化部署\n- 内置连接状态监控和健康检查\n- 支持 Headless Service 和 StatefulSet 跨集群访问",
     "- 多集群应用的服务间通信\n- 集群迁移期间的流量平滑切换\n- 混合云/多云环境的网络打通\n- 开发/测试环境的跨集群联调",
     "- https://submariner.io/\n- https://github.com/submariner-io/submariner",
     "- [[系统基础/topic-dictionary/networking/cilium|Cilium Cluster Mesh]]\n- [[系统基础/topic-dictionary/networking/linkerd|Linkerd]]\n- [[系统基础/topic-dictionary/networking/consul|Consul]]"),

    ("networking", "contour", "Contour Ingress 控制器", "Contour",
     ["networking", "ingress", "envoy"],
     "Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress 和 Gateway API，提供高性能的 L7 负载均衡和流量管理能力。",
     "- **Envoy 驱动**：使用 Envoy 作为数据面，控制面用 Go 编写\n- **双 API 支持**：同时支持 Kubernetes Ingress 和 Gateway API\n- **HTTProxy CRD**：Contour 自定义的路由配置资源，支持丰富的流量策略\n- **CNCF Sandbox**：CNCF 沙箱项目",
     "- 动态 Envoy 配置（通过 xDS API）\n- TLS 终止与 SNI 路由\n- 流量分割（权重路由）用于金丝雀发布\n- WebSocket / gRPC 代理\n- 速率限制（集成 ratelimit 服务）\n- Contour 支持多 Gateway 部署",
     "- 替代 nginx-ingress 的高性能 Ingress 方案\n- 需要 Envoy 级别流量控制的场景\n- Gateway API 的早期采纳\n- 金丝雀发布和流量镜像需求",
     "- https://projectcontour.io/\n- https://github.com/projectcontour/contour",
     "- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/envoy-gateway|Envoy Gateway]]\n- [[系统基础/topic-dictionary/networking/traefik|Traefik]]"),

    ("networking", "bfe", "BFE 负载均衡引擎", "BFE",
     ["networking", "load-balancer", "proxy"],
     "BFE（Baidu Front End）是百度开源的七层负载均衡引擎，已在百度内部大规模使用，支持多租户、高级流量管理和丰富的扩展插件，适用于超大规模互联网架构。",
     "- **高性能七层代理**：基于 Go 实现的高性能 HTTP/HTTPS 反向代理\n- **多租户**：原生支持多租户流量隔离和独立配置\n- **插件体系**：丰富的插件机制支持流量染色、限流、灰度等功能\n- **大规模验证**：百度生产环境每日处理万亿级请求",
     "- 基于集群的负载均衡和故障转移\n- 精确流量调度（基于 Header/Cookie/IP 等）\n- TLS 卸载与会话复用\n- 与 K8s Ingress 集成（通过 bfe-ingress-controller）\n- 健康检查与慢启动\n- Prometheus 指标导出",
     "- 超大规模 Web 服务的入口负载均衡\n- 需要多租户隔离的平台架构\n- 国产自主可控的负载均衡方案\n- 复杂的灰度发布和流量调度场景",
     "- https://www.bfe-networks.net/\n- https://github.com/bfenetworks/bfe",
     "- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/traefik|Traefik]]\n- [[系统基础/topic-dictionary/networking/consul|Consul]]"),

    ("networking", "k8gb", "K8GB 全球负载均衡", "K8GB",
     ["networking", "dns", "multi-cluster"],
     "K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基于 DNS 和 GSLB 策略将用户请求路由到最优集群。",
     "- **DNS 级负载均衡**：通过 CoreDNS 插件或外部 DNS 提供商实现 GSLB\n- **健康检查驱动**：基于端点健康状态自动摘除故障集群\n- **多策略路由**：支持 Round Robin、地理位置、故障转移等策略\n- **CNCF Sandbox**：轻量级的全球流量管理方案",
     "- GslbIngress CRD 定义全局流量策略\n- 集成 Infoblox、Route53、NS1 等 DNS 提供商\n- 基于 Prometheus 的健康检查指标\n- 支持加权 Round Robin 和 GeoIP 路由\n- 零停机集群维护和故障转移\n- 与 Flagger / Argo Rollouts 配合使用",
     "- 多区域/多集群的高可用部署\n- 灾难恢复场景下的流量切换\n- 基于地理位置的用户路由\n- 灰度发布中的全球流量分配",
     "- https://www.k8gb.io/\n- https://github.com/k8gb-io/k8gb",
     "- [[系统基础/topic-dictionary/networking/consul|Consul]]\n- [[系统基础/topic-dictionary/networking/linkerd|Linkerd]]\n- [[系统基础/topic-dictionary/operations/flagger|Flagger]]"),

    # ── Observability ──
    ("observability", "fluentd", "Fluentd 日志收集", "Fluentd",
     ["observability", "logging", "cnCF"],
     "Fluentd 是 CNCF 毕业项目，统一日志收集层，支持 500+ 插件连接各种数据源和目标，是 Kubernetes 环境中日志收集的事实标准之一。",
     "- **统一日志层**：在应用和数据存储之间提供统一的日志收集和处理层\n- **插件生态**：500+ 社区插件覆盖几乎所有日志源和目标\n- **JSON 优先**：默认使用 JSON 格式处理日志，便于结构化查询\n- **CNCF 毕业项目**：经过大规模生产验证",
     "- Tag 驱动的事件路由（`<match>` / `<filter>` 配置）\n- Buffer 机制确保可靠传输（文件 + 内存双层缓冲）\n- 高可用模式（forward 协议集群间传输）\n- Fluent Bit 作为轻量采集端 + Fluentd 作为聚合端的分层架构\n- Kubernetes 元数据自动富化\n- 日志解析（parse）插件支持多行日志、正则、JSON 等",
     "- Kubernetes 集群统一日志收集\n- 日志转发到 Elasticsearch/OpenSearch/Loki\n- 日志过滤、脱敏、格式转换\n- 多租户日志隔离和路由\n- 边缘场景下使用 Fluent Bit 替代",
     "- https://www.fluentd.org/\n- https://github.com/fluent/fluentd",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/loki|Loki]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]"),

    # ── Operations / Chaos ──
    ("operations", "litmus", "Litmus 混沌工程", "Litmus",
     ["operations", "chaos-engineering", "cncf"],
     "LitmusChaos 是 CNCF 孵化项目，提供 Kubernetes 原生的混沌工程平台，内置 300+ 预定义混沌实验，支持通过 ChaosCenter 进行集中管理和可观测性。",
     "- **Kubernetes 原生**：以 CRD 方式定义混沌实验（ChaosExperiment/ChaosEngine/ChaosResult）\n- **300+ 实验**：ChaosHub 提供大量预定义实验（Pod/Network/Node/DNS/Kafka 等）\n- **ChaosCenter**：Web UI 集中管理实验编排、调度和结果分析\n- **CNCF 孵化**：活跃的开源混沌工程社区",
     "- 实验编排：多步骤串/并行组合混沌实验\n- 弹性探针（Probes）：HTTP/CMD/Prometheus/Continuous 验证\n- GitOps 集成：通过 Argo 管理混沌实验\n- 混沌实验评分（Resilience Score）量化系统弹性\n- 支持 Argo Workflows 编排复杂故障注入流程\n- 与 Prometheus/Grafana 集成可视化",
     "- 生产环境弹性验证\n- CI/CD Pipeline 中的自动化弹性测试\n- 故障演练和红蓝对抗\n- 新服务上线前的 Chaos Day\n- SLO 验证和容量规划",
     "- https://litmuschaos.io/\n- https://github.com/litmuschaos/litmus",
     "- [[系统基础/topic-dictionary/operations/chaos-engineering|混沌工程]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]"),

    ("operations", "chaos-mesh", "Chaos Mesh 混沌工程平台", "Chaos Mesh",
     ["operations", "chaos-engineering", "cncf"],
     "Chaos Mesh 是 PingCAP 开源并捐赠给 CNCF 的混沌工程平台，提供 Web UI 和声明式 API，支持对 Kubernetes、物理机和云环境注入各类故障。",
     "- **声明式故障注入**：通过 YAML CRD 定义故障类型、目标和持续时间\n- **Web Dashboard**：可视化创建和管理混沌实验\n- **多平台支持**：Kubernetes、物理机（Chaosd）、AWS/GCP 等\n- **CNCF 孵化项目**：PingCAP 主导开发",
     "- 丰富的故障类型：PodChaos、NetworkChaos、IOChaos、TimeChaos、StressChaos、JVMChaos、HTTPChaos\n- 精确的目标选择（Label/Annotation/Namespace 筛选）\n- 故障自动恢复和超时保护\n- 实验调度（定时/周期性故障注入）\n- PhysicalMachineChaos 支持裸金属故障注入\n- 与 Prometheus 集成导出实验指标",
     "- 分布式系统的弹性验证\n- 数据库（TiDB 等）的故障注入测试\n- 网络分区和延迟模拟\n- 定时故障演练（Cron 调度）\n- 微服务依赖链的级联故障验证",
     "- https://chaos-mesh.org/\n- https://github.com/chaos-mesh/chaos-mesh",
     "- [[系统基础/topic-dictionary/operations/litmus|LitmusChaos]]\n- [[系统基础/topic-dictionary/operations/chaos-engineering|混沌工程]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

    # ── Scheduling / Multi-Cluster ──
    ("scheduling", "volcano", "Volcano 批处理调度", "Volcano",
     ["scheduling", "batch", "ai-ml"],
     "Volcano 是 CNCF 孵化项目，专为 Kubernetes 上的批处理、AI/ML、HPC 等高性能计算工作负载设计的批量调度系统，弥补原生调度器在 gang-scheduling 和公平共享方面的不足。",
     "- **Gang Scheduling**：保证一组 Pod 全部调度成功或全部不调度（all-or-nothing）\n- **公平共享**：基于 Queue 的多租户资源公平分配\n- **AI/ML 优化**：为 TensorFlow、PyTorch、MPI 等训练框架优化调度\n- **CNCF 孵化**：华为开源，AI/ML 领域广泛使用",
     "- Queue CRD 定义资源配额和优先级\n- Job CRD 定义批量任务（gang-scheduling + task 类型）\n- 抢占（Preemption）和回填（Backfill）策略\n- Binpack 插件优化 GPU 资源利用率\n- 支持 MPI Operator 和 TensorFlow Operator\n- 与 Kubeflow 深度集成",
     "- AI/ML 分布式训练任务调度\n- 大数据批处理（Spark、Flink）\n- HPC 高性能计算\n- 多租户 GPU 集群的公平调度\n- 需要 Gang Scheduling 的任何工作负载",
     "- https://volcano.sh/\n- https://github.com/volcano-sh/volcano",
     "- [[系统基础/topic-dictionary/scheduling/scheduler|Scheduler]]\n- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]\n- [[系统基础/topic-dictionary/specialized-workloads/ray|Ray]]"),

    ("platform-engineering", "karmada", "Karmada 多集群管理", "Karmada",
     ["platform-engineering", "multi-cluster", "cncf"],
     "Karmada 是 CNCF 孵化项目，提供 Kubernetes 多集群的统一管理和应用分发能力，支持跨集群调度、故障迁移和资源聚合，是 Federation v2 的演进方案。",
     "- **多集群联邦**：将多个 K8s 集群统一管理为一个逻辑集群\n- **应用分发**：将应用声明式分发到指定集群集合\n- **跨集群调度**：根据集群能力、亲和性和资源自动选择目标集群\n- **CNCF 孵化**：华为开源，社区活跃",
     "- PropagationPolicy / ClusterPropagationPolicy 定义应用分发策略\n- 多集群资源视图（karmadactl get pods --all-clusters）\n- 跨集群故障迁移（Failover）\n- 资源解释器（Resource Interpreter）适配自定义资源\n- 与 HPA/VPA 配合的跨集群弹性伸缩\n- Karmada Dashboard Web UI",
     "- 企业多集群统一管理\n- 跨区域/多云的应用部署\n- 集群故障时的自动迁移\n- 灰度发布中的多集群流量管理\n- 集群资源统一视图和容量规划",
     "- https://karmada.io/\n- https://github.com/karmada-io/karmada",
     "- [[系统基础/topic-dictionary/networking/submariner|Submariner]]\n- [[系统基础/topic-dictionary/networking/clusternet|Clusternet]]\n- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]"),

    ("platform-engineering", "open-cluster-management", "Open Cluster Management", "OCM",
     ["platform-engineering", "multi-cluster", "cncf"],
     "Open Cluster Management（OCM）是 Red Hat 主导的 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理框架，包括集群注册、策略下发、应用部署和可观测性聚合。",
     "- **Hub-Spoke 架构**：中心 Hub 集群管理多个 Spoke（被管理）集群\n- **策略引擎**：通过 Policy 框架实现跨集群配置合规管理\n- **应用生命周期**：Placement + Subscription 实现应用分发\n- **CNCF Sandbox**：Red Hat ACM 的开源核心",
     "- Klusterlet Agent 部署在被管理集群\n- ManifestWork 向被管理集群下发资源\n- Placement API 选择目标集群\n- Policy 框架（配置合规、安全策略、Operator 部署）\n- Search API 跨集群资源搜索\n- Application 模型管理多集群应用",
     "- 企业级多集群运维管理\n- 跨集群安全策略和合规检查\n- 集中式应用分发和生命周期管理\n- 多集群可观测性聚合\n- 混合云集群的统一控制面",
     "- https://open-cluster-management.io/\n- https://github.com/open-cluster-management-io/ocm",
     "- [[系统基础/topic-dictionary/platform-engineering/karmada|Karmada]]\n- [[系统基础/topic-dictionary/platform-engineering/rancher|Rancher]]\n- [[系统基础/topic-dictionary/security/opa|OPA]]"),

    ("networking", "clusternet", "Clusternet 多集群网络", "Clusternet",
     ["networking", "multi-cluster", "cncf"],
     "Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API 访问和资源分发，无需修改底层网络。",
     "- **API 代理**：通过代理方式访问子集群 API，无需直连\n- **应用分发**：支持 ManifestWork 式的应用分发\n- **Scheduler 插件**：多集群调度策略\n- **CNCF Sandbox**：轻量级多集群管理方案",
     "- Hub 集群 + Agent 部署模式\n- ServiceExport / ServiceImport 多集群服务发现\n- 跨集群 Helm Chart 安装\n- 多集群调度框架插件\n- 支持边缘集群（弱网环境）\n- 与 Karmada 互补的多集群方案",
     "- 多集群 API 统一访问\n- 跨集群应用分发和管理\n- 边缘集群的集中管理\n- 弱网环境下的集群互联\n- 多集群 Helm 应用编排",
     "- https://clusternet.io/\n- https://github.com/clusternet/clusternet",
     "- [[系统基础/topic-dictionary/platform-engineering/karmada|Karmada]]\n- [[系统基础/topic-dictionary/networking/submariner|Submariner]]\n- [[系统基础/topic-dictionary/platform-engineering/rancher|Rancher]]"),

    # ── Tooling ──
    ("tooling", "telepresence", "Telepresence 远程开发", "Telepresence",
     ["tooling", "development", "debugging"],
     "Telepresence 是 Ambassadeur Labs 开源的 Kubernetes 远程开发工具，将本地开发环境与远程 K8s 集群网络打通，开发者可在本地编码同时访问集群内服务和被集群内服务回调。",
     "- **网络打通**：本地进程可直接访问集群内 Service（DNS + IP 透明）\n- **流量拦截**：将集群中指定服务的流量重定向到本地进程\n- **本地开发体验**：保留本地 IDE、调试器，无需在集群中部署\n- **CNCF Sandbox**：远程开发领域的标准工具",
     "- `telepresence connect` 建立本地到集群的网络隧道\n- `telepresence intercept` 拦截指定 Service 流量到本地\n- 支持 Preview URL 分享开发中的服务\n- 环境变量自动注入\n- 多命名空间访问\n- 与 VS Code / JetBrains 集成",
     "- 微服务架构中的本地开发和调试\n- 服务间调用的端到端测试\n- 避免本地搭建完整 K8s 环境\n- Code Review 中的 Preview 环境搭建\n- 远程集群的 API 调试",
     "- https://www.telepresence.io/\n- https://github.com/telepresenceio/telepresence",
     "- [[系统基础/topic-dictionary/tooling/skaffold|Skaffold]]\n- [[系统基础/topic-dictionary/networking/linkerd|Linkerd]]\n- [[系统基础/topic-dictionary/networking/consul|Consul]]"),

    ("tooling", "opentofu", "OpenTofu IaC 工具", "OpenTofu",
     ["tooling", "iac", "open-source"],
     "OpenTofu 是 Terraform 的开源分支（Linux Foundation 托管），在 HashiCorp 更改许可证后由社区发起，保持 MPL 2.0 开源许可，API 与 Terraform 1.x 兼容。",
     "- **Terraform 分支**：从 Terraform 1.5.x fork，保持 API 兼容\n- **MPL 2.0 许可**：保持真正的开源许可，无商业限制\n- **Linux Foundation**：由 Linux Foundation 托管，社区治理\n- **Provider 兼容**：可使用现有 Terraform Provider 生态",
     "- `tofu init/plan/apply/destroy` 与 Terraform 命令兼容\n- 支持 Terraform Registry 中的 Provider 和 Module\n- State 后端兼容（S3、GCS、Consul 等）\n- 社区驱动的 Provider 开发\n- 与 Terragrunt 等工具链兼容\n- OpenTofu Registry 独立 Provider 仓库",
     "- Terraform 的开源替代方案\n- 需要真正开源许可的企业环境\n- Kubernetes 基础设施管理（EKS/GKE/AKS）\n- 多云基础设施编排\n- GitOps 中的基础设施管理",
     "- https://opentofu.org/\n- https://github.com/opentofu/opentofu",
     "- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/platform-engineering/backstage|Backstage]]"),

    ("tooling", "buildpacks", "Cloud Native Buildpacks", "CNB",
     ["tooling", "ci-cd", "container"],
     "Cloud Native Buildpacks（CNB）是 CNCF 孵化项目，将应用源代码自动转化为容器镜像，无需编写 Dockerfile，支持多语言和多框架，是 Heroku Buildpacks 的云原生演进。",
     "- **无 Dockerfile**：自动检测语言和框架，生成优化的容器镜像\n- **可复现构建**：相同输入产生相同的镜像输出（Reproducible Builds）\n- **多阶段优化**：自动分离构建依赖和运行时依赖\n- **CNCF 孵化**：VMware/Pivotal 主导，社区活跃",
     "- Builder 镜像：包含 Detect + Build 阶段的执行环境\n- Pack CLI：命令行工具（`pack build`）\n- Buildpack 检测顺序和组管理\n- 层缓存（Layer Caching）优化构建速度\n- Rebase：仅替换基础镜像层，无需重新构建\n- Platform API 与 K8s Tekton/Jenkins 集成",
     "- 无需维护 Dockerfile 的应用容器化\n- 多语言 monorepo 的统一构建流程\n- 安全补丁的快速应用（Rebase）\n- CI/CD Pipeline 中的标准化构建\n- PaaS 平台底层的镜像构建引擎",
     "- https://buildpacks.io/\n- https://github.com/buildpacks/pack",
     "- [[系统基础/topic-dictionary/tooling/docker|Docker]]\n- [[系统基础/topic-dictionary/tooling/tekton|Tekton]]\n- [[系统基础/topic-dictionary/tooling/podman|Podman]]"),

    ("tooling", "bootc", "bootc 容器启动系统", "bootc",
     ["tooling", "container", "os"],
     "bootc 是 Red Hat 开源的项目，将 OCI 容器镜像作为操作系统的基础，实现以容器方式管理和更新整个操作系统，是 Fedora/CentOS 的下一代系统交付方式。",
     "- **容器即 OS**：使用 OCI 镜像定义完整的操作系统\n- **原子更新**：通过 `bootc upgrade` 实现系统的原子更新和回滚\n- **OSTree 底层**：基于 OSTree 的文件系统管理和引导\n- **Red Hat 主导**：Fedora Bootc / RHEL Image Mode 的核心技术",
     "- `bootc build` 从 Containerfile 构建可引导的系统镜像\n- `bootc upgrade` 拉取新镜像并部署\n- `bootc rollback` 回滚到上一版本\n- `bootc switch` 切换到不同的镜像源\n- 与 Podman / Buildah 集成\n- 支持 Kubernetes 风格的配置注入（/usr 只读 + /var 可写）",
     "- 边缘设备和 IoT 的系统管理\n- 不可变基础设施（Immutable Infrastructure）\n- 大规模裸金属/VM 的系统交付\n- 操作系统的安全补丁快速部署\n- Kubernetes 节点操作系统的统一管理",
     "- https://containers.github.io/bootc/\n- https://github.com/containers/bootc",
     "- [[系统基础/topic-dictionary/fundamentals/docker|Docker]]\n- [[系统基础/topic-dictionary/tooling/podman|Podman]]\n- [[系统基础/topic-dictionary/fundamentals/runc|runc]]"),

    # ── Messaging / Platform ──
    ("platform-engineering", "nats", "NATS 消息系统", "NATS",
     ["platform-engineering", "messaging", "cncf"],
     "NATS 是 CNCF 孵化项目，高性能的轻量级消息系统，支持 Core Pub/Sub、JetStream 持久化和 Request/Reply 模式，在 IoT、边缘计算和微服务场景中广泛使用。",
     "- **极致轻量**：单二进制，内存占用极低（MB 级）\n- **多模式**：Core（Pub/Sub）+ JetStream（持久化流）+ Request/Reply\n- **集群原生**：支持 Leaf Node、Super Cluster 等拓扑\n- **CNCF 孵化**：活跃的开源消息中间件社区",
     "- Core NATS：低延迟 Pub/Sub（微秒级）\n- JetStream：持久化消息流（类似 Kafka 轻量替代）\n- Subject 通配符匹配（`>` 和 `*`）\n- 消费者组（Consumer Groups）和工作队列\n- 多租户（Account 隔离）\n- NKeys 和 JWT 认证",
     "- 微服务间的轻量级消息传递\n- IoT 设备的数据收集和分发\n- 边缘计算场景的本地消息总线\n- 事件驱动架构的轻量替代方案\n- Kubernetes 内部的事件和通知系统",
     "- https://nats.io/\n- https://github.com/nats-io/nats-server",
     "- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative Eventing]]\n- [[系统基础/topic-dictionary/networking/grpc|gRPC]]\n- [[系统基础/topic-dictionary/platform-engineering/dapr|Dapr]]"),

    ("platform-engineering", "dapr", "Dapr 分布式应用运行时", "Dapr",
     ["platform-engineering", "microservices", "cncf"],
     "Dapr（Distributed Application Runtime）是 CNCF 孵化项目，为微服务提供标准化的构建块（Building Blocks），通过 Sidecar 模式抽象服务发现、状态管理、消息发布等分布式系统通用能力。",
     "- **Sidecar 架构**：以 Sidecar 方式部署，应用通过 HTTP/gRPC 调用 Dapr API\n- **构建块模式**：状态管理、服务调用、发布订阅、密钥管理、Actor 等\n- **组件可插拔**：每种构建块支持多种后端实现（Redis、Kafka、Azure 等）\n- **CNCF 孵化**：微软开源，社区活跃",
     "- Service Invocation：服务间调用（mTLS + 重试 + 追踪）\n- State Management：KV 状态存储抽象\n- Pub/Sub：消息发布订阅（多 Broker 支持）\n- Bindings：外部系统集成（输入/输出绑定）\n- Actors：虚拟 Actor 模型\n- Workflow：持久化工作流引擎\n- Configuration API 和 Secret Store",
     "- 微服务应用的标准化运行时\n- 多云/混合云的应用可移植性\n- 事件驱动的微服务架构\n- 状态管理和服务编排\n- .NET/Java/Go/Python 等多语言微服务",
     "- https://dapr.io/\n- https://github.com/dapr/dapr",
     "- [[系统基础/topic-dictionary/platform-engineering/nats|NATS]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]"),

    # ── Runtime ──
    ("fundamentals", "youki", "youki 容器运行时", "youki",
     ["fundamentals", "container-runtime", "rust"],
     "youki 是用 Rust 编写的 OCI 容器运行时，兼容 runc 接口，旨在提供更高安全性和性能的低开销容器运行时实现，是 runc 的 Rust 替代方案。",
     "- **Rust 实现**：利用 Rust 的内存安全特性减少运行时漏洞\n- **OCI 兼容**：完整实现 OCI Runtime Specification\n- **runc 替代**：可直接替换 runc 使用\n- **社区驱动**：containers 组织下的活跃开源项目",
     "- 兼容 CRI-O 和 containerd 的 runtime shim\n- 支持 cgroups v1 和 v2\n- seccomp 和 capabilities 安全策略\n- Rootless 模式支持\n- 性能与 runc 相当，内存占用更低\n- WasmEdge 集成支持 WebAssembly 工作负载",
     "- 需要更高安全保证的容器运行时\n- runc 的替代方案评估\n- 边缘设备的低开销容器运行\n- Rust 生态的容器基础设施\n- 安全合规要求严格的环境",
     "- https://github.com/containers/youki\n- https://youki-dev.github.io/youki/",
     "- [[系统基础/topic-dictionary/fundamentals/runc|runc]]\n- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|containerd]]"),

    # ── Configuration ──
    ("configuration", "kcl", "KCL 配置语言", "KCL",
     ["configuration", "language", "cncf"],
     "KCL（Kusion Configuration Language）是蚂蚁集团开源并捐赠给 CNCF 的配置语言，专为云原生场景设计，提供类型系统、模块化和策略校验能力，是 YAML/Helm 的编程式替代方案。",
     "- **编程式配置**：类型系统、循环、条件、函数等编程能力\n- **云原生专注**：内置 Kubernetes 模型和校验规则\n- **模块复用**：包管理和模块系统支持配置复用\n- **CNCF Sandbox**：蚂蚁集团开源",
     "- 强类型系统（类型推断 + 类型检查）\n- 内置 Schema 和 Validation\n- KPM 包管理器（类似 pip/go mod）\n- 与 Helm/Terraform/Crossplane 等工具集成\n- 配置策略检查（Policy as Code）\n- IDE 支持（VS Code 插件、LSP）",
     "- 大规模 Kubernetes 配置的编程化管理\n- 替代 Helm/Kustomize 的复杂配置场景\n- 配置策略的自动化校验\n- 多环境配置的统一管理\n- 基础设施即代码（IaC）配置编写",
     "- https://kcl-lang.io/\n- https://github.com/kcl-lang/kcl",
     "- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]"),

    # ── Security / Confidential ──
    ("security", "confidential-containers", "机密容器", "Confidential Containers",
     ["security", "tee", "cncf"],
     "Confidential Containers（CoCo）是 CNCF Sandbox 项目，将机密计算（TEE）能力引入 Kubernetes，通过硬件隔离保护容器内的数据和代码，即使基础设施提供者也无法访问。",
     "- **硬件 TEE**：利用 Intel SGX/TDX、AMD SEV、ARM CCA 等硬件安全扩展\n- **Kubernetes 集成**：通过 RuntimeClass 透明使用机密容器\n- **零信任**：保护数据在使用中的机密性（Data in Use）\n- **CNCF Sandbox**：Intel/IBM/微软等联合推动",
     "- Kata Containers + TEE 后端（Guest attestation）\n- 远程证明（Remote Attestation）验证运行环境\n- Peer Pods 支持裸金属和云 VM\n- 机密计算友好的密钥管理（密钥只在 TEE 内可用）\n- CoCo Operator 简化部署和配置\n- 与 Key Broker Service（KBS）集成",
     "- 多方数据协作（数据可用但不可见）\n- 金融/医疗等高敏感数据处理\n- 多租户环境下的强隔离\n- 云环境中保护租户工作负载\n- 合规要求下的数据加密计算",
     "- https://confidentialcontainers.org/\n- https://github.com/confidential-containers",
     "- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/security/vault|Vault]]\n- [[系统基础/topic-dictionary/security/spiffe-spire-identity|SPIFFE/SPIRE]]"),

    # ── Database ──
    ("storage", "cloudnativepg", "CloudNativePG 云原生 PostgreSQL", "CloudNativePG",
     ["storage", "database", "operator"],
     "CloudNativePG 是 EDB 开源的 Kubernetes PostgreSQL Operator，以 GitOps 友好的方式管理 PostgreSQL 集群的全生命周期，支持高可用、备份恢复和滚动升级。",
     "- **Kubernetes 原生**：通过 CRD 声明式管理 PostgreSQL 集群\n- **高可用**：基于流复制的自动故障转移\n- **GitOps 友好**：所有配置通过 YAML 声明\n- **CNCF Sandbox**：活跃的 PostgreSQL on K8s 社区",
     "- Cluster CRD 定义 PG 集群（实例数、存储、资源配置）\n- 基于 Patroni 的高可用和自动故障转移\n- 连续 WAL 归档和 PITR（Point-in-Time Recovery）\n- 滚动升级和在线参数变更\n- 读写分离连接池（内置 PgBouncer）\n- 多集群部署支持",
     "- Kubernetes 上的 PostgreSQL 生产部署\n- GitOps 方式管理数据库生命周期\n- 需要自动故障转移的高可用数据库\n- 数据库版本升级的零停机方案\n- 多租户数据库实例管理",
     "- https://cloudnative-pg.io/\n- https://github.com/cloudnative-pg/cloudnative-pg",
     "- [[系统基础/topic-dictionary/storage/persistent-volumes|PV/PVC]]\n- [[系统基础/topic-dictionary/operations/velero|Velero]]\n- [[系统基础/topic-dictionary/platform-engineering/rancher|Rancher]]"),

    # ── Storage / AI ──
    ("storage", "fluid", "Fluid 数据编排", "Fluid",
     ["storage", "ai-ml", "cncf"],
     "Fluid 是 CNCF Sandbox 项目，为 Kubernetes 上的 AI/ML 和大数据工作负载提供数据编排和加速能力，通过 Dataset + Runtime 抽象统一管理异构存储系统的数据访问。",
     "- **数据抽象**：Dataset CRD 统一描述数据的元数据和访问方式\n- **运行时抽象**：Runtime CRD 管理数据缓存引擎（Alluxio/JindoFS/JuiceFS/GooseFS）\n- **数据感知调度**：将计算任务调度到数据所在位置\n- **CNCF Sandbox**：阿里巴巴开源",
     "- 支持多种缓存 Runtime（Alluxio、JindoFS、JuiceFS、GooseFS、Vineyard）\n- 数据预热（Data Prefetching）\n- 弹性数据集（Elastic Dataset）动态扩缩\n- 与 Spark/TensorFlow/PyTorch Operator 集成\n- 数据迁移和复制\n- 跨命名空间数据共享",
     "- AI 训练任务的数据加速\n- 大数据分析（Spark/Flink）的数据本地化\n- 多云/混合存储的统一访问层\n- 训练数据的预热和缓存管理\n- 大规模数据集的弹性管理",
     "- https://fluid-cloudnative.github.io/\n- https://github.com/fluid-cloudnative/fluid",
     "- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]\n- [[系统基础/topic-dictionary/storage/ceph|Ceph]]\n- [[系统基础/topic-dictionary/storage/minio|MinIO]]"),

    # ── Dev ──
    ("tooling", "lima", "Lima Linux 虚拟机", "Lima",
     ["tooling", "development", "linux"],
     "Lima（Linux on Mac）是一个轻量级工具，在 macOS 上自动创建和管理 Linux 虚拟机，主要用于容器运行时（如 containerd/Docker）的开发和测试，是 colima 的底层引擎。",
     "- **轻量 VM**：在 macOS 上快速启动 Linux 虚拟机\n- **文件共享**：自动将宿主机目录共享到 VM（virtiofs）\n- **端口转发**：自动将 VM 端口转发到宿主机\n- **colima 底层**：colima（容器运行时管理器）基于 Lima 构建",
     "- `limactl start` 创建 VM（支持多种 Linux 发行版模板）\n- 内置 containerd/Docker/Podman 模板\n- virtiofs 高性能文件共享\n- 端口自动转发\n- 支持多 VM 管理（`limactl list`）\n- YAML 模板定义 VM 配置",
     "- macOS 上的 Linux 容器开发环境\n- 容器运行时的本地测试\n- CI/CD 中的 Linux 环境模拟\n- Kubernetes 组件的本地开发\n- colima 的底层引擎",
     "- https://lima-vm.io/\n- https://github.com/lima-vm/lima",
     "- [[系统基础/topic-dictionary/tooling/minikube|Minikube]]\n- [[系统基础/topic-dictionary/tooling/k3s|K3s]]\n- [[系统基础/topic-dictionary/fundamentals/docker|Docker]]"),
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
