#!/usr/bin/env python3
"""Round 7: 高频引用缺失术语批量展开（25个）"""
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
    # ── Networking ──
    ("networking", "connect-rpc", "Connect RPC 协议", "Connect RPC",
     ["networking", "rpc", "protocol"],
     "Connect 是 Buf 开发的开源 RPC 协议，兼容 gRPC 和 Protobuf，但使用标准 HTTP 语义（HTTP/1.1 和 HTTP/2），简化了浏览器和移动端的 API 调用，是 gRPC-Web 的现代替代方案。",
     "- **协议兼容**：兼容 gRPC 和 gRPC-Web 的 wire format\n- **HTTP 语义**：基于标准 HTTP 方法，无需特殊的 gRPC 代理\n- **浏览器友好**：原生支持浏览器端调用，无需 grpc-web proxy\n- **多语言 SDK**：支持 Go、TypeScript、Swift、Kotlin、Java 等",
     "- 自动代码生成（基于 protobuf schema）\n- Connect-Go / Connect-ES / Connect-Swift 等语言实现\n- 支持 Unary、Server Streaming、Client Streaming、Bidi Streaming\n- 与 Envoy / Nginx 等代理无缝集成\n- 错误处理标准化（Connect Protocol Error）\n- connectrpc 命令行工具",
     "- gRPC API 的浏览器端调用\n- 移动应用的 RPC 通信\n- 替代 REST 的类型安全 API\n- gRPC-Web 的现代化升级\n- 微服务间的高性能 RPC 通信",
     "- https://connectrpc.com/\n- https://github.com/connectrpc/connect-go",
     "- [[domain-17-system-foundation/topic-dictionary/networking/grpc|gRPC]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy|Envoy]]\n- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]"),

    ("networking", "ovn-kubernetes", "OVN-Kubernetes 网络方案", "OVN-Kubernetes",
     ["networking", "cni", "ovn"],
     "OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red Hat 主导开发，是 OpenShift 的默认网络方案，提供完整的 L2/L3 网络、NetworkPolicy 和硬件加速能力。",
     "- **OVN 数据面**：基于 OpenFlow 的虚拟网络，支持硬件卸载\n- **完整 NetworkPolicy**：支持 Ingress/Egress 和 FQDN 策略\n- **OpenShift 默认**：Red Hat OpenShift 的标准 CNI\n- **硬件加速**：支持 SmartNIC/DPU 卸载",
     "- OVN Northbound/Southbound 数据库架构\n- OVS（Open vSwitch）作为节点数据面\n- 支持 Hybrid Overlay（Windows + Linux 节点混合）\n- EgressFirewall / EgressQoS / EgressService CRD\n- AdminNetworkPolicy（K8s 增强网络策略）\n- IPAM 管理和多子网支持",
     "- OpenShift / OCP 集群的标准网络方案\n- 需要硬件加速的企业网络\n- Windows + Linux 混合节点集群\n- 需要 AdminNetworkPolicy 的多租户环境\n- 大规模集群的高性能网络",
     "- https://github.com/ovn-kubernetes/ovn-kubernetes\n- https://docs.openshift.com/container-platform/latest/networking/understanding-networking.html",
     "- [[domain-17-system-foundation/topic-dictionary/networking/antrea|Antrea]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cilium|Cilium]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cni|CNI]]"),

    ("networking", "kuma", "Kuma 服务网格", "Kuma",
     ["networking", "service-mesh", "envoy"],
     "Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM 环境，以易用性和多网格（multi-mesh）架构著称。",
     "- **Envoy 驱动**：基于 Envoy Proxy 的数据面\n- **通用平台**：同时支持 Kubernetes 和 VM/裸金属\n- **多网格**：原生支持多网格隔离架构\n- **CNCF Sandbox**：Kong 主导，社区活跃",
     "- Mesh CRD 定义网格实例（多网格隔离）\n- TrafficPermission / TrafficRoute / TrafficLog 策略\n- mTLS 自动管理（内置 CA）\n- 速率限制和熔断\n- MeshGateway 支持入口流量\n- Kong Mesh 商业版提供企业功能\n- Kuma GUI 可视化管理",
     "- 轻量级服务网格部署\n- K8s + VM 混合环境的服务治理\n- 多团队/多环境的网格隔离\n- 需要简单操作体验的服务网格\n- Istio 的轻量替代方案",
     "- https://kuma.io/\n- https://github.com/kumahq/kuma",
     "- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]\n- [[domain-17-system-foundation/topic-dictionary/networking/linkerd|Linkerd]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy|Envoy]]"),

    ("networking", "kube-vip", "kube-vip 虚拟 IP", "kube-vip",
     ["networking", "ha", "vip"],
     "kube-vip 为 Kubernetes 集群提供虚拟 IP（VIP）和负载均衡能力，用于控制面高可用（API Server VIP）和 Service 的 LoadBalancer 类型实现，无需外部负载均衡器。",
     "- **虚拟 IP**：通过 ARP/NDP 或 BGP 广播 VIP\n- **控制面 HA**：为 kubeadm 集群提供 API Server 高可用 VIP\n- **Service LB**：实现 Service Type LoadBalancer（裸金属/本地环境）\n- **轻量部署**：静态 Pod 或 DaemonSet 方式运行",
     "- ARP 模式（L2 局域网 VIP 漂移）\n- BGP 模式（L3 路由宣告，适合大规模）\n- Leader Election 确保单活 VIP\n- Service 自动检测（监控 LoadBalancer 类型 Service）\n- 等价路由（ECMP）负载均衡\n- 支持 IPVS 内核级负载均衡",
     "- kubeadm 集群的控制面高可用\n- 裸金属/边缘环境的 LoadBalancer 实现\n- 替代 MetalLB 的轻量方案\n- 多集群的入口流量管理\n- 无外部 LB 的内部服务暴露",
     "- https://kube-vip.io/\n- https://github.com/kube-vip/kube-vip",
     "- [[domain-17-system-foundation/topic-dictionary/networking/metallb|MetalLB]]\n- [[domain-17-system-foundation/topic-dictionary/networking/consul|Consul]]\n- [[domain-17-system-foundation/topic-dictionary/networking/k8gb|K8GB]]"),

    ("networking", "aeraki-mesh", "Aeraki Mesh 七层网格", "Aeraki Mesh",
     ["networking", "service-mesh", "l7"],
     "Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩展到 TCP 和任意七层协议（Dubbo、Thrift、Redis 等）。",
     "- **协议扩展**：将 Istio 的流量管理扩展到任意 L7 协议\n- **Dubbo 支持**：完整支持 Apache Dubbo 协议的流量治理\n- **Redis 支持**：Redis 协议的流量镜像、故障注入等\n- **腾讯开源**：基于腾讯大规模微服务实践",
     "- Aeraki Protocol Framework 协议扩展框架\n- 支持 Dubbo、Thrift、Redis、MySQL 等非 HTTP 协议\n- Aeraki Mesh CRD 定义七层路由规则\n- 与 Istio 控制面无缝集成\n- MetaProtocol 元协议框架（协议无关的流量治理）\n- LazyXDS 按需加载优化大规模集群性能",
     "- 使用 Dubbo/Thrift 等传统 RPC 框架的微服务网格化\n- 需要非 HTTP 协议流量治理的场景\n- Istio 生态的协议扩展\n- 传统微服务向服务网格迁移\n- 多协议混合环境的统一管理",
     "- https://www.aeraki.net/\n- https://github.com/aeraki-mesh/aeraki",
     "- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy|Envoy]]\n- [[domain-17-system-foundation/topic-dictionary/networking/linkerd|Linkerd]]"),

    # ── Security ──
    ("security", "notary-project", "Notary Project 容器签名", "Notary Project",
     ["security", "supply-chain", "signing"],
     "Notary Project（原 Docker Notary v2）是 CNCF 孵化项目，提供容器镜像和其他 OCI 制品的数字签名和验证能力，是软件供应链安全的基石组件。",
     "- **OCI 签名**：为容器镜像和 OCI 制品附加数字签名\n- **签名验证**：在拉取和部署时验证签名的完整性和来源\n- **CNCF 孵化**：Docker/Microsoft/VMware 等联合推动\n- **跨 Registry**：签名与制品分离存储，支持跨 Registry 传播",
     "- `notation sign` 对 OCI 制品签名\n- `notation verify` 验证签名\n- 支持多种密钥后端（本地文件、Azure Key Vault、AWS KMS）\n- Trust Store 和 Trust Policy 管理\n- 签名存储在 OCI Registry 的 Referrers API\n- 与 Kyverno/OPA Gatekeeper/Ratify 集成验证",
     "- CI/CD Pipeline 中的镜像签名和验证\n- 生产部署前的镜像来源验证\n- 合规要求下的软件供应链审计\n- 多环境镜像复制时的完整性保障\n- Kubernetes Admission 策略中的签名验证",
     "- https://notaryproject.dev/\n- https://github.com/notaryproject/notation",
     "- [[domain-17-system-foundation/topic-dictionary/security/ratify|Ratify]]\n- [[domain-17-system-foundation/topic-dictionary/security/in-toto|in-toto]]\n- [[domain-17-system-foundation/topic-dictionary/security/trivy|Trivy]]"),

    ("security", "in-toto", "in-toto 供应链安全", "in-toto",
     ["security", "supply-chain", "verification"],
     "in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构建和分发过程中未被篡改。",
     "- **完整性框架**：定义供应链步骤（Steps）和检查（Inspections）的完整布局\n- **元数据记录**：每个步骤的输入/输出哈希、命令、执行者签名\n- **验证链**：从源代码到最终制品的端到端验证\n- **CNCF 孵化**：与 TUF/Sigstore 构成供应链安全三件套",
     "- Layout 定义：供应链步骤序列和验证规则\n- Link 元数据：每个步骤的材料（materials）和产品（products）\n- 函数签名验证（Functionary verification）\n- 子布局（Sublayouts）支持嵌套供应链\n- ITE-5/ITE-6 规范标准化\n- `in-toto-run` / `in-toto-verify` CLI 工具",
     "- CI/CD Pipeline 的完整性验证\n- 软件发布流程的审计追踪\n- 第三方依赖的来源验证\n- SLSA 合规的供应链证明\n- 与 Sigstore/TUF 集成的综合安全方案",
     "- https://in-toto.io/\n- https://github.com/in-toto/in-toto",
     "- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]\n- [[domain-17-system-foundation/topic-dictionary/security/ratify|Ratify]]\n- [[domain-17-system-foundation/topic-dictionary/security/supply-chain-security|供应链安全]]"),

    ("security", "ratify", "Ratify 准入验证", "Ratify",
     ["security", "admission", "supply-chain"],
     "Ratify 是微软开源的 Kubernetes 准入验证框架，与 OPA Gatekeeper 配合，在 Pod 部署时验证容器镜像的签名、SBOM 和漏洞扫描结果等供应链元数据。",
     "- **准入验证**：作为 External Data Provider 为 Gatekeeper 提供验证数据\n- **多验证器**：支持 Notary 签名、Cosign 签名、SBOM 验证、漏洞扫描验证\n- **可扩展**：插件式验证器架构\n- **Azure 背景**：微软主导，与 Azure 生态深度集成",
     "- 与 OPA Gatekeeper 的 External Data 集成\n- Notation / Cosign 签名验证\n- SBOM 存在性和格式验证\n- 漏洞扫描结果验证（Trivy/Grype）\n- Certificate Store 管理签名证书\n- VerificationResult 标准化输出",
     "- 生产集群的镜像签名强制验证\n- CI/CD 中的供应链安全检查门控\n- 合规要求下的 SBOM 验证\n- 多来源镜像的统一准入策略\n- 与 Kyverno/Gatekeeper 配合的策略引擎",
     "- https://ratify.dev/\n- https://github.com/ratify-project/ratify",
     "- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]\n- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA Gatekeeper]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]"),

    ("security", "keycloak", "Keycloak 身份管理", "Keycloak",
     ["security", "identity", "sso"],
     "Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是 Kubernetes 生态中最常用的外部身份提供者之一。",
     "- **SSO 平台**：统一的单点登录和身份管理\n- **多协议**：支持 OIDC、SAML 2.0、LDAP、Kerberos\n- **用户管理**：完整的用户/组/角色管理和自助服务\n- **Red Hat 支持**：Red Hat SSO 的开源上游",
     "- Realm（域）隔离的多租户管理\n- Identity Broker（联邦身份代理）连接外部 IdP\n- 社交登录（Google/GitHub/Facebook 等）\n- 用户自助服务（注册/密码重置/账户管理）\n- Fine-Grained Admin Permissions\n- OTP/MFA 多因素认证\n- 与 Dex 互补（Keycloak 作为 Dex 后端）",
     "- 企业级 SSO 和身份管理平台\n- Kubernetes 集群的外部 OIDC 提供者\n- 多应用/多服务的统一认证授权\n- 用户自助服务和生命周期管理\n- 合规要求下的审计和访问控制",
     "- https://www.keycloak.org/\n- https://github.com/keycloak/keycloak",
     "- [[domain-17-system-foundation/topic-dictionary/security/dex|Dex]]\n- [[domain-17-system-foundation/topic-dictionary/security/oauth2-proxy|oauth2-proxy]]\n- [[domain-17-system-foundation/topic-dictionary/security/rbac|RBAC]]"),

    # ── Workloads ──
    ("workloads", "openkruise", "OpenKruise 增强工作负载", "OpenKruise",
     ["workloads", "operator", "cncf"],
     "OpenKruise 是阿里巴巴开源的 CNCF 孵化项目，为 Kubernetes 提供增强型工作负载管理能力，包括原地升级、Sidecar 管理、镜像预热等原生 K8s 缺失的高级功能。",
     "- **增强工作负载**：扩展 K8s 原生工作负载的能力边界\n- **生产验证**：阿里巴巴大规模生产环境使用\n- **CNCF 孵化**：活跃的增强工作负载社区\n- **兼容原生**：不替换而是增强，与原生 K8s 资源互补",
     "- CloneSet：增强版 Deployment（支持原地升级、指定删除、分批发布）\n- Advanced StatefulSet：增强版 StatefulSet（原地升级、无序扩缩）\n- SidecarSet：统一管理 Sidecar 容器注入和升级\n- NodeImage / ImagePullJob：镜像预热和按需拉取\n- ResourceDistribution：跨命名空间资源分发\n- Advanced DaemonSet：增强版 DaemonSet",
     "- 大规模集群的工作负载管理\n- 需要原地升级（不重建 Pod）的场景\n- Sidecar 容器的统一管理和升级\n- 镜像预热加速大规模扩容\n- 分批发布和金丝雀发布的精细化控制",
     "- https://openkruise.io/\n- https://github.com/openkruise/kruise",
     "- [[domain-17-system-foundation/topic-dictionary/workloads/deployment|Deployment]]\n- [[domain-17-system-foundation/topic-dictionary/workloads/statefulset|StatefulSet]]\n- [[domain-17-system-foundation/topic-dictionary/workloads/daemonset|DaemonSet]]"),

    # ── Storage ──
    ("storage", "vineyard", "Vineyard 分布式数据共享", "Vineyard",
     ["storage", "ai-ml", "data-sharing"],
     "Vineyard（v6d）是 CNCF Sandbox 项目，为 Kubernetes 上的 AI/ML 和大数据工作负载提供高效的分布式内存数据共享，通过零拷贝机制在多个计算任务间共享中间数据。",
     "- **内存数据共享**：通过共享内存实现进程间零拷贝数据交换\n- **分布式**：跨节点的数据共享和分布式对象管理\n- **AI/ML 优化**：专为 ML Pipeline 中的中间数据共享设计\n- **CNCF Sandbox**：阿里巴巴开源",
     "- Blob（不可变数据对象）和 Metadata（可变元数据对象）\n- 基于 mmap 的零拷贝共享\n- Distributed Object Manager 跨节点管理\n- 与 Kubernetes CSI 集成（Vineyard CSI Driver）\n- SDK 支持 Python/C++/Java/Rust\n- 与 Ray/Spark/Dask/Mars 等框架集成",
     "- ML Pipeline 中间数据的零拷贝共享\n- 分布式训练中的数据分发\n- 大规模数据处理任务的内存优化\n- 多租户环境下的数据隔离与共享\n- 替代文件系统中转的内存级数据交换",
     "- https://v6d.io/\n- https://github.com/v6d-io/v6d",
     "- [[domain-17-system-foundation/topic-dictionary/storage/fluid|Fluid]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/ray|Ray]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]"),

    ("storage", "openebs", "OpenEBS 容器存储", "OpenEBS",
     ["storage", "csi", "cncf"],
     "OpenEBS 是 Maya Data 开源的 CNCF Sandbox 项目，为 Kubernetes 提供容器附加存储（CAS），支持多种存储引擎（Local PV、Replicated PV、ZFS Local PV），是有状态应用的存储方案。",
     "- **容器附加存储**：将存储引擎容器化，与应用同生命周期管理\n- **多引擎**：Local PV / Replicated PV（Mayastor）/ ZFS Local PV / LVM Local PV\n- **Kubernetes 原生**：通过 CSI 驱动集成\n- **CNCF Sandbox**：活跃的容器存储社区",
     "- Local PV Hostpath / Device 模式\n- Mayastor：基于 SPDK/NVMe-oF 的高性能复制引擎\n- ZFS Local PV：利用 ZFS 特性的本地存储\n- LVM Local PV：基于 LVM 的本地卷管理\n- 快照和克隆支持\n- CStor（已弃用，迁移至 Mayastor）",
     "- 有状态应用（数据库/消息队列）的持久化存储\n- 需要本地存储高性能 I/O 的场景\n- 云和裸金属环境的统一存储方案\n- 开发/测试环境的快速存储配置\n- 存储数据的快照和克隆",
     "- https://openebs.io/\n- https://github.com/openebs/openebs",
     "- [[domain-17-system-foundation/topic-dictionary/storage/rook|Rook]]\n- [[domain-17-system-foundation/topic-dictionary/storage/longhorn|Longhorn]]\n- [[domain-17-system-foundation/topic-dictionary/storage/ceph|Ceph]]"),

    # ── Scheduling ──
    ("scheduling", "koordinator", "Koordinator 增强调度", "Koordinator",
     ["scheduling", "qos", "cncf"],
     "Koordinator 是阿里巴巴开源的 CNCF Sandbox 项目，提供 Kubernetes 增强调度和资源编排能力，专注于混部（Colocation）场景下的资源利用率提升和 QoS 保障。",
     "- **混部调度**：在线服务和离线任务混合部署，提升资源利用率\n- **QoS 保障**：精细化的资源隔离和干扰控制\n- **设备调度**：GPU/RDMA/FPGA 等异构资源的统一调度\n- **CNCF Sandbox**：阿里巴巴主导",
     "- QoS 动态超卖（Dynamic Resource Overcommitment）\n- CPU Burst 和 CFS Burst 弹性调度\n- 设备插件（GPU Share / RDMA / FPGA）\n- Gang Scheduling 和 Coscheduling\n- 弹性配额（ElasticQuota）多级资源管理\n- Node Resource Manager 精细资源管控",
     "- 在线/离线混部提升集群利用率\n- GPU 共享和细粒度调度\n- 需要严格 QoS 保障的多租户环境\n- 大规模集群的资源弹性超卖\n- AI 训练与在线服务的资源协同",
     "- https://koordinator.sh/\n- https://github.com/koordinator-sh/koordinator",
     "- [[domain-17-system-foundation/topic-dictionary/scheduling/volcano|Volcano]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/scheduler|Scheduler]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/qos|QoS]]"),

    # ── Observability ──
    ("observability", "kepler", "Kepler 能耗监控", "Kepler",
     ["observability", "energy", "sustainability"],
     "Kepler（Kubernetes Efficient Power Level Exporter）是 CNCF Sandbox 项目，通过 eBPF 和 CPU 模型估算 Kubernetes 中每个 Pod 的能耗，导出为 Prometheus 指标，支持绿色计算和碳足迹追踪。",
     "- **能耗估算**：通过 eBPF 采集 CPU/DRAM/GPU 能耗指标\n- **Pod 粒度**：将节点级能耗拆分到 Pod 级别\n- **Prometheus 导出**：标准 Prometheus metrics 格式\n- **CNCF Sandbox**：Red Hat/IBM 主导的绿色计算项目",
     "- eBPF 采集 CPU C-state 和能耗计数器\n- 基于机器学习模型的能耗估算（RAPL + Model）\n- GPU 能耗采集（NVIDIA DCGM）\n- Kepler Dashboard（Grafana 预置看板）\n- 碳排放计算（结合区域电力碳强度数据）\n- OpenTelemetry 集成",
     "- 数据中心碳足迹追踪\n- Kubernetes 集群的能耗优化\n- 绿色计算和可持续发展报告\n- 成本核算中的能耗分摊\n- 工作负载的能效对比（Perf/Watt）",
     "- https://sustainable-computing.io/\n- https://github.com/sustainable-computing-io/kepler",
     "- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]\n- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[domain-17-system-foundation/topic-dictionary/observability/grafana|Grafana]]"),

    # ── Operations ──
    ("operations", "k8sgpt", "K8sGPT AI 诊断助手", "K8sGPT",
     ["operations", "ai", "diagnostics"],
     "K8sGPT 是 CNCF Sandbox 项目，利用 AI/LLM 技术自动扫描 Kubernetes 集群中的问题并提供诊断建议，将复杂的 K8s 故障排查简化为自然语言交互。",
     "- **AI 驱动**：集成多种 LLM 后端（OpenAI/Azure/Local）进行智能诊断\n- **自动扫描**：检测集群中的异常资源和问题\n- **自然语言输出**：以人类可读的方式解释问题和解决方案\n- **CNCF Sandbox**：活跃的 AI+K8s 社区",
     "- `k8sgpt analyze` 扫描集群问题\n- Analyzer 插件架构（Pod/Service/Ingress/PVC 等分析器）\n- 多 LLM 后端支持（OpenAI、Azure OpenAI、LocalAI、Amazon Bedrock）\n- Filter 机制（按命名空间/类型筛选）\n- 自定义 AI Provider\n- 与 Prometheus/Grafana 集成可视化",
     "- K8s 集群的快速健康检查\n- 复杂问题的 AI 辅助诊断\n- 运维新手的问题排查引导\n- 日常巡检中的异常检测\n- 故障根因分析的第一步",
     "- https://k8sgpt.ai/\n- https://github.com/k8sgpt-ai/k8sgpt",
     "- [[domain-17-system-foundation/topic-dictionary/operations/chaos-engineering|混沌工程]]\n- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]\n- [[domain-17-system-foundation/topic-dictionary/operations/k8up|K8up]]"),

    ("operations", "k8up", "K8up 备份 Operator", "K8up",
     ["operations", "backup", "operator"],
     "K8up 是 VSHN 开源的 Kubernetes 备份 Operator，基于 restic 实现增量备份，通过 CRD 声明式管理 PVC 数据的自动备份和恢复，是 Velero 的轻量级替代方案。",
     "- **Operator 模式**：通过 CRD 声明式管理备份策略\n- **restic 后端**：基于 restic 的增量、加密、去重备份\n- **PVC 级别**：自动发现并备份集群中的所有 PVC\n- **多后端**：支持 S3/GCS/Azure/Swift 等存储后端",
     "- Schedule CRD 定义备份计划（Cron 表达式）\n- PreBackupPod 备份前执行自定义脚本（如数据库 dump）\n- 自动 PVC 发现和备份\n- Restore CRD 管理恢复操作\n- Archive CRD 归档旧备份\n- 与 Prometheus 集成导出备份指标",
     "- 有状态应用的定时备份\n- 数据库的 Pre-backup dump + 增量备份\n- 轻量级备份方案（替代 Velero 的全集群备份）\n- 多租户环境的独立备份策略\n- 备份合规和保留策略管理",
     "- https://k8up.io/\n- https://github.com/k8up-io/k8up",
     "- [[domain-17-system-foundation/topic-dictionary/operations/velero|Velero]]\n- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes|PV/PVC]]\n- [[domain-17-system-foundation/topic-dictionary/operations/backup-disaster-recovery|备份与灾难恢复]]"),

    # ── Platform Engineering ──
    ("platform-engineering", "opengitops", "OpenGitOps 标准", "OpenGitOps",
     ["platform-engineering", "gitops", "cncf"],
     "OpenGitOps 是 CNCF Sandbox 项目，定义了 GitOps 的原则和标准，提供 GitOps 最佳实践的参考实现和合规认证，推动 GitOps 工具和平台的互操作性。",
     "- **GitOps 标准**：定义 GitOps 的四项核心原则\n- **互操作性**：推动不同 GitOps 工具的标准化\n- **CNCF Sandbox**：Flux/Argo/OpenTofu 等社区联合推动\n- **参考实现**：提供 GitOps 原则的参考代码",
     "- GitOps 四原则：声明式、版本化和不可变、自动拉取、持续协调\n- GitOps Agent 参考实现\n- GitOps 合规认证程序\n- 与 Argo CD / Flux 等工具的对齐\n- GitOps Days 社区活动\n- 文档和最佳实践指南",
     "- GitOps 实践的标准化和规范化\n- 企业 GitOps 转型的参考框架\n- GitOps 工具选型的合规评估\n- 多团队 GitOps 流程的统一\n- GitOps 培训和能力建设",
     "- https://opengitops.dev/\n- https://github.com/open-gitops/project",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/argo|Argo]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/flux|Flux]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/tekton|Tekton]]"),

    ("platform-engineering", "score", "Score 工作负载规范", "Score",
     ["platform-engineering", "workload", "cncf"],
     "Score 是 CNCF Sandbox 项目，定义了一个平台无关的工作负载描述规范（score.yaml），开发者只需编写一次工作负载描述，即可通过 Score CLI 转换为 Kubernetes、Docker Compose、Helm 等平台的具体配置。",
     "- **平台无关**：一份 score.yaml 描述工作负载需求\n- **多目标**：转换为 K8s YAML、Docker Compose、Helm Chart 等\n- **开发者友好**：隐藏平台复杂性，专注工作负载需求\n- **CNCF Sandbox**：Humanitec 主导",
     "- `score.yaml` 声明式工作负载描述\n- score-compose：转换为 Docker Compose\n- score-k8s：转换为 Kubernetes manifests\n- score-helm：生成 Helm Chart\n- Resource 声明（数据库/缓存/消息队列等依赖）\n- score-spec：转换为 Score 内部规范",
     "- 开发者自助服务平台的底层规范\n- 多环境（dev/staging/prod）的配置一致性\n- 降低开发者对 K8s 的认知负担\n- 平台团队标准化工作负载定义\n- IDP（Internal Developer Platform）的工作负载模型",
     "- https://score.dev/\n- https://github.com/score-spec/spec",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage|Backstage]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize|Kustomize]]"),

    ("platform-engineering", "operator-framework", "Operator Framework 框架", "Operator Framework",
     ["platform-engineering", "operator", "sdk"],
     "Operator Framework 是 Red Hat 开源的 Kubernetes Operator 开发和管理框架，包含 Operator SDK、OLM（Operator Lifecycle Manager）和 OperatorHub，是 Operator 开发的行业标准工具链。",
     "- **Operator SDK**：Go/Ansible/Helm/Python 多语言 Operator 开发脚手架\n- **OLM**：Operator 的生命周期管理（安装/升级/卸载）\n- **OperatorHub**：Operator 的发现和分发市场\n- **Red Hat 主导**：OpenShift 生态的核心工具链",
     "- `operator-sdk init` 初始化项目（Go/Ansible/Helm/Java）\n- `operator-sdk generate` 代码和 CRD 生成\n- OLM Catalog 管理 Operator 版本和更新通道\n- Scorecard 测试框架\n- Bundle Format 打包标准\n- OperatorHub.io 社区市场",
     "- Kubernetes Operator 的标准化开发\n- Operator 的版本管理和自动升级\n- 企业内部 Operator 的分发和管理\n- Red Hat OpenShift 的 Operator 认证\n- Operator 生态的集成和发布",
     "- https://operatorframework.io/\n- https://github.com/operator-framework/operator-sdk",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubevela|KubeVela]]\n- [[domain-17-system-foundation/topic-dictionary/workloads/deployment|Deployment]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane|Crossplane]]"),

    # ── Tooling ──
    ("tooling", "distribution", "CNCF Distribution 镜像仓库", "Distribution",
     ["tooling", "registry", "container"],
     "Distribution 是 CNCF 毕业项目，提供 OCI 兼容的容器镜像仓库实现（即 Docker Registry v2），是大多数私有 Registry（Harbor、GHCR 等）的底层引擎。",
     "- **OCI 标准**：完整实现 OCI Distribution Specification\n- **Registry v2**：Docker Registry 的官方开源实现\n- **广泛基础**：Harbor、GitLab Registry、AWS ECR 等基于此构建\n- **CNCF 毕业**：经过大规模生产验证",
     "- Pull/Push API（Manifest + Blob/Layer 管理）\n- Tag 和 Digest 两种寻址方式\n- Token-based Authentication（Bearer Token）\n- 存储驱动（Filesystem/S3/GCS/Azure/OSS）\n- 垃圾回收（`registry garbage-collect`）\n- Referrers API（OCI 1.1 附件引用）",
     "- 企业内部私有镜像仓库\n- 边缘场景的轻量镜像缓存\n- CI/CD Pipeline 的镜像存储后端\n- 开发环境 Registry 的本地替代\n- OCI 制品（Helm Chart/WASM 等）存储",
     "- https://github.com/distribution/distribution\n- https://distribution.github.io/distribution/",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/harbor|Harbor]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/docker|Docker]]\n- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]"),

    ("tooling", "headlamp", "Headlamp K8s 仪表盘", "Headlamp",
     ["tooling", "dashboard", "ui"],
     "Headlamp 是 Kinvolk（现微软）开源的 Kubernetes 管理仪表盘，提供集群资源可视化、日志查看和终端操作，是 K8s Dashboard 的现代替代方案，支持插件扩展。",
     "- **现代 UI**：基于 React + TypeScript 的现代 Web 界面\n- **插件架构**：可扩展的插件系统\n- **多集群**：支持同时管理多个集群\n- **Kinvolk 出品**：Flatcar Container Linux 团队开发",
     "- 集群资源概览（Pods/Services/Deployments 等）\n- 实时日志查看和终端 Shell\n- YAML 编辑器（在线编辑资源）\n- 插件市场（社区和企业插件）\n- 多集群管理和切换\n- 自定义主题和品牌定制",
     "- Kubernetes 集群的可视化管理\n- 替代 K8s Dashboard 的现代方案\n- 开发者的日常集群操作界面\n- 运维团队的集群监控仪表盘\n- 需要品牌定制的企业管理平台",
     "- https://headlamp.io/\n- https://github.com/headlamp-k8s/headlamp",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl|kubectl]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/stern|Stern]]\n- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]"),

    ("tooling", "k0s", "K0s 轻量级 Kubernetes", "K0s",
     ["tooling", "distribution", "edge"],
     "K0s 是 Mirantis 开源的轻量级 Kubernetes 发行版，单二进制部署，资源占用极低，适用于边缘计算、IoT 和嵌入式设备的 Kubernetes 部署场景。",
     "- **单二进制**：所有组件打包为单个可执行文件\n- **低资源**：最低 512MB 内存即可运行\n- **完整 K8s**：100% 兼容上游 Kubernetes API\n- **Mirantis 维护**：企业级支持和长期维护",
     "- `k0s install controller/worker` 安装命令\n- 内置 Containerd 和 Konnectivity\n- 支持 Control Plane 隔离模式\n- K0sctl 多节点集群部署工具\n- 自动证书管理和轮转\n- 扩展机制（Helm/K0s 扩展）",
     "- 边缘设备和 IoT 的 K8s 部署\n- 开发环境的快速 K8s 搭建\n- CI/CD Pipeline 中的临时集群\n- 嵌入式设备的容器编排\n- K3s 的替代方案",
     "- https://k0sproject.io/\n- https://github.com/k0sproject/k0s",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/k3s|K3s]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/minikube|Minikube]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm|kubeadm]]"),

    ("tooling", "kpt", "kpt 包管理工具", "kpt",
     ["tooling", "package", "configuration"],
     "kpt 是 Google 开源的 Kubernetes 包管理工具，基于 Git 仓库管理 K8s 配置包，支持包的获取、定制和自动更新，是 Helm/Kustomize 之外的配置管理方案。",
     "- **Git 原生**：以 Git 仓库作为包的存储和分发机制\n- **声明式定制**：通过 KRM Function 管道化配置转换\n- **自动更新**：上游包更新可自动合并到下游定制\n- **Google 开源**：Config Sync 的底层工具",
     "- `kpt pkg get` 从 Git 获取配置包\n- `kpt fn render` 执行 KRM Function 管道\n- `kpt live apply` 声明式应用到集群\n- KRM Function 生态（Starlark/Go/Container）\n- Package 层级和子包管理\n- 与 Config Sync / Argo CD 集成",
     "- GitOps 配置的管理和分发\n- 多环境配置的包化管理\n- 配置模板的版本控制和更新\n- KRM Function 的声明式配置转换\n- 大规模 K8s 配置的组织和管理",
     "- https://kpt.dev/\n- https://github.com/GoogleContainerTools/kpt",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/helm|Helm]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/flux|Flux]]"),

    # ── Fundamentals ──
    ("fundamentals", "flatcar", "Flatcar 容器操作系统", "Flatcar",
     ["fundamentals", "os", "container"],
     "Flatcar Container Linux 是 Kinvolk（现微软）维护的不可变容器操作系统，是 CoreOS Container Linux 的社区分支，专为运行容器工作负载优化，提供自动更新和最小化攻击面。",
     "- **不可变 OS**：只读根文件系统，通过原子更新交付\n- **容器优化**：仅包含运行容器所需的最小系统组件\n- **自动更新**：内置 update_engine 自动下载和应用更新\n- **CoreOS 继承**：CoreOS Container Linux 的社区继任者",
     "- Ignition 系统配置（替代 cloud-init）\n- A/B 分区双系统（更新失败自动回滚）\n- 最小化攻击面（无包管理器，无 SSH 密码登录）\n- 自动安全更新\n- 支持多种平台（AWS/Azure/GCP/Bare Metal/QEMU）\n- 与 Fleet/Locksmith 协调更新策略",
     "- Kubernetes 节点的标准化操作系统\n- 边缘/IoT 设备的不可变系统\n- 安全合规要求下的最小化 OS\n- 大规模集群的自动更新管理\n- CoreOS 停服后的替代方案",
     "- https://www.flatcar.org/\n- https://github.com/flatcar/Flatcar",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/bootc|bootc]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker|Docker]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd|containerd]]"),

    # ── Multi-tenancy ──
    ("security", "capsule", "Capsule 多租户管理", "Capsule",
     ["security", "multi-tenancy", "cncf"],
     "Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略管理，无需引入额外的控制面组件。",
     "- **轻量多租户**：通过 CRD 和 Admission Webhook 实现，无需额外控制面\n- **命名空间隔离**：每个租户拥有独立的命名空间集合\n- **策略继承**：租户级策略自动应用到其所有命名空间\n- **CNCF Sandbox**：Clastix 主导开发",
     "- Tenant CRD 定义租户及其命名空间\n- NetworkPolicy 自动注入（租户间隔离）\n- ResourceQuota / LimitRange 按租户管理\n- 存储类限制（每租户可用 StorageClass）\n- Ingress 类限制（每租户可用 IngressClass）\n- 节点选择器限制（NodeSelector 按租户隔离）",
     "- 企业内部的 K8s 多租户管理\n- 开发团队的资源隔离\n- SaaS 平台的租户管理\n- 共享集群的安全隔离\n- 替代 vCluster / OCM 的轻量方案",
     "- https://capsule.clastix.io/\n- https://github.com/clastix/capsule",
     "- [[domain-17-system-foundation/topic-dictionary/security/rbac|RBAC]]\n- [[domain-17-system-foundation/topic-dictionary/security/networkpolicy|NetworkPolicy]]\n- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]"),

    # ── Specialized ──
    ("specialized-workloads", "kubevirt", "KubeVirt 虚拟化", "KubeVirt",
     ["specialized-workloads", "virtualization", "cncf"],
     "KubeVirt 是 Red Hat 开源的 CNCF 孵化项目，在 Kubernetes 上提供虚拟机管理能力，通过 CRD 定义和运行 VM，实现容器和虚拟机工作负载在同一集群中的统一管理。",
     "- **K8s 上的 VM**：通过 VirtualMachine CRD 管理虚拟机\n- **容器和 VM 统一**：VM 和 Pod 共享同一集群的调度和网络\n- **CNCF 孵化**：Red Hat 主导，OpenShift Virtualization 的上游\n- **成熟生态**：与 CDI、Kubevirt-CSI 等组件配合",
     "- VirtualMachine / VirtualMachineInstance CRD\n- CDI（Containerized Data Importer）镜像管理\n- DataVolume 声明 VM 磁盘\n- 热迁移（Live Migration）\n- 模板和实例类型（InstanceType）\n- GPU/SRIOV 直通\n- 与 Multus CNI 配合的多网络 VM",
     "- 传统 VM 工作负载迁移到 K8s\n- 容器和 VM 混合工作负载管理\n- 数据库等不适合容器化的 VM 工作负载\n- VDI（虚拟桌面）在 K8s 上的部署\n- 云 VM 与容器应用的统一编排",
     "- https://kubevirt.io/\n- https://github.com/kubevirt/kubevirt",
     "- [[domain-17-system-foundation/topic-dictionary/workloads/pod|Pod]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cni|CNI]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kata-containers|Kata Containers]]"),

    # ── Platform / CD ──
    ("operations", "pipecd", "PipeCD 持续交付", "PipeCD",
     ["operations", "ci-cd", "gitops"],
     "PipeCD 是 Cybozu 开源的 CNCF Sandbox 持续交付平台，支持 Kubernetes、ECS、Lambda、Terraform 等多种部署目标的统一 GitOps 管理，提供金丝雀、蓝绿等高级部署策略。",
     "- **多目标**：统一支持 K8s/ECS/Lambda/Cloud Run/Terraform\n- **GitOps**：以 Git 仓库为唯一配置源\n- **高级策略**：金丝雀、蓝绿、渐进式交付\n- **CNCF Sandbox**：Cybozu 主导",
     "- Application CRD 定义部署目标\n- Analysis 自动化分析（Prometheus/DataDog/Stackdriver）\n- 渐进式交付（Canary / Blue-Green / Rolling）\n- Web UI 可视化管理\n- 多集群 / 多环境管理\n- Encryption 敏感配置加密\n- Notification 集成（Slack/Teams）",
     "- 多平台（K8s + Serverless + VM）的统一 CD\n- 需要渐进式交付的生产部署\n- GitOps 实践中的持续交付\n- 多团队 / 多环境的部署管理\n- 自动化分析驱动的安全发布",
     "- https://pipecd.dev/\n- https://github.com/pipe-cd/pipecd",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/argo|Argo]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/flux|Flux]]\n- [[domain-17-system-foundation/topic-dictionary/operations/flagger|Flagger]]"),

    # ── Service Mesh ──
    ("operations", "meshery", "Meshery 服务网格管理", "Meshery",
     ["operations", "service-mesh", "cncf"],
     "Meshery 是 CNCF Sandbox 项目，提供服务网格和云原生基础设施的统一管理平台，支持 10+ 种服务网格的生命周期管理、性能基准测试和配置管理。",
     "- **多网格管理**：支持 Istio/Linkerd/Consul/Kuma/App Mesh 等 10+ 网格\n- **性能测试**：内置 SMP（Service Mesh Performance）基准测试\n- **配置管理**：跨网格的配置管理和策略执行\n- **CNCF Sandbox**：Layer5 主导，社区活跃",
     "- Meshery Operator 管理 Mesh 生命周期\n- SMP（Service Mesh Performance）标准化性能指标\n- Meshery Designs 可视化架构设计\n- OAM（Open Application Model）集成\n- WASM 过滤器管理\n- MeshSync 集群状态同步\n- 200+ 集成（Adapters）",
     "- 多服务网格的统一管理和对比评估\n- 服务网格的性能基准测试\n- 服务网格迁移的辅助工具\n- 云原生架构的可视化设计\n- 多团队环境的网格治理",
     "- https://meshery.io/\n- https://github.com/meshery/meshery",
     "- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]\n- [[domain-17-system-foundation/topic-dictionary/networking/linkerd|Linkerd]]\n- [[domain-17-system-foundation/topic-dictionary/networking/kuma|Kuma]]"),
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
