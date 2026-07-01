#!/usr/bin/env python3
"""Round 9: 剩余高频缺失术语批量展开（25个）"""
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
    ("networking", "kubeslice", "KubeSlice 多集群网络", "KubeSlice",
     ["networking", "multi-cluster", "slice"],
     "KubeSlice 是 Avesha 开源的 CNCF Sandbox 项目，通过创建跨集群的网络切片（Slice）实现多集群安全隔离的网络互通，无需修改底层 CNI 即可打通多个 K8s 集群。",
     "- **网络切片**：创建跨集群的隔离网络通道（Slice）\n- **CNI 无关**：兼容任何底层 CNI 实现\n- **安全隔离**：mTLS 加密的跨集群通信\n- **CNCF Sandbox**：Avesha 主导",
     "- Slice CRD 定义跨集群网络切片\n- SliceGateway 建立集群间安全隧道\n- SliceConfig 定义访问策略\n- 支持跨集群 Service 发现\n- DNS 集成（跨集群 DNS 解析）\n- 带宽限制和流量管理",
     "- 多集群应用的安全网络互通\n- 混合云/多云的网络连接\n- 微服务的跨集群部署\n- 替代 Submariner 的多集群方案\n- 网络隔离要求严格的多租户环境",
     "- https://kubeslice.io/\n- https://github.com/kubeslice/kubeslice-controller",
     "- [[domain-17-system-foundation/topic-dictionary/networking/submariner|Submariner]]\n- [[domain-17-system-foundation/topic-dictionary/networking/clusternet|Clusternet]]\n- [[domain-17-system-foundation/topic-dictionary/networking/k8gb|K8GB]]"),

    ("networking", "easegress", "Easegress 流量编排", "Easegress",
     ["networking", "gateway", "service-mesh"],
     "Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Service Mesh Controller 于一体，支持 HTTP/TCP/MQTT 等多协议。",
     "- **全场景**：API Gateway + Service Mesh + Serverless Runtime\n- **多协议**：HTTP/2、gRPC、WebSocket、MQTT、TCP\n- **CNCF Sandbox**：MegaEase 主导\n- **Go 编写**：高性能低资源占用",
     "- Pipeline 流量处理管道\n- Filter 链式过滤器（限流/认证/重试/路由等）\n- 服务注册与发现（K8s/Consul/Eureka/Nacos）\n- 分布式一致性（基于 Raft）\n- Serverless Runtime（Wasm + 函数运行时）\n- Prometheus 指标导出",
     "- API 网关和反向代理\n- 微服务的流量治理\n- MQTT IoT 设备流量管理\n- Serverless 函数的网关层\n- 传统系统现代化改造的流量层",
     "- https://megaease.com/easegress/\n- https://github.com/megaease/easegress",
     "- [[domain-17-system-foundation/topic-dictionary/networking/traefik|Traefik]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy-gateway|Envoy Gateway]]\n- [[domain-17-system-foundation/topic-dictionary/networking/contour|Contour]]"),

    ("networking", "spiderpool", "Spiderpool IP 池管理", "Spiderpool",
     ["networking", "ipam", "cni"],
     "Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的 IP 地址管理（IPAM），解决容器使用固定 IP 和 Underlay 网络的挑战。",
     "- **Underlay IPAM**：为 Pod 分配 Underlay 网络的固定 IP\n- **多 CNI 兼容**：支持 Macvlan、IPVLAN、SR-IOV、IB SR-IOV\n- **CNCF Sandbox**：DaoCloud 主导\n- **固定 IP**：支持 Pod 固定 IP 和 IP 池管理",
     "- SpiderIPPool / SpiderSubnet / SpiderEndpoint CRD\n- 固定 IP（Pod Annotation 指定 IP）\n- IP 池管理和自动回收\n- 多网卡 IPAM（Multus 集成）\n- IP 冲突检测和自动修复\n- Webhook 验证 IP 合法性\n- IPv4/IPv6 双栈支持",
     "- 需要 Pod 固定 IP 的场景（金融/电信）\n- Underlay 网络的 K8s 部署\n- 多网卡 Pod 的 IP 管理\n- SR-IOV 高性能网络的 IP 分配\n- 传统网络环境的 K8s 集成",
     "- https://spiderpool.dev/\n- https://github.com/spidernet-io/spiderpool",
     "- [[domain-17-system-foundation/topic-dictionary/networking/cni|CNI]]\n- [[domain-17-system-foundation/topic-dictionary/networking/metallb|MetalLB]]\n- [[domain-17-system-foundation/topic-dictionary/networking/antrea|Antrea]]"),

    ("networking", "network-service-mesh", "Network Service Mesh", "NSM",
     ["networking", "multi-cluster", "cncf"],
     "Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等），将网络功能从硬件解耦到软件定义。",
     "- **网络服务网格**：将网络功能软件化，按需编排\n- **L2/L3 VPN**：跨集群的 L2/L3 网络连接\n- **CNCF Sandbox**：活跃的 NFV/SDN 社区\n- **与 K8s 集成**：基于 K8s CRD 管理网络服务",
     "- NetworkService / NetworkServiceEndpoint CRD\n- NSMGR（Network Service Mesh Registry）\n- Forwarder 数据面（VPP/memif/Kernel）\n- 多集群 L2/L3 VPN\n- 与 Multus CNI 集成\n- 支持 Intel VPP 高性能转发",
     "- 5G/Telco 的网络功能虚拟化\n- 跨集群 L2/L3 VPN 连接\n- 传统网络设备的软件化替代\n- 多租户网络隔离\n- 云原生 NFV 基础设施",
     "- https://networkservicemesh.io/\n- https://github.com/networkservicemesh/networkservicemesh",
     "- [[domain-17-system-foundation/topic-dictionary/networking/submariner|Submariner]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cni|CNI]]\n- [[domain-17-system-foundation/topic-dictionary/networking/loxilb|LoxiLB]]"),

    ("networking", "kmesh", "KMesh 内核级服务网格", "KMesh",
     ["networking", "service-mesh", "ebpf"],
     "KMesh 是华为开源的 CNCF Sandbox 项目，基于 eBPF 和可编程硬件在内核态实现服务网格数据面，将 L4 流量管理下沉到内核，显著降低 Sidecar 的资源开销和延迟。",
     "- **内核态数据面**：基于 eBPF 在内核层处理流量\n- **无 Sidecar**：消除 Envoy/Istio Sidecar 的资源开销\n- **CNCF Sandbox**：华为主导\n- **Istio 兼容**：复用 Istio 控制面",
     "- eBPF 程序在内核态处理 L4 流量\n- Waypoint Proxy 模式（L7 用户态处理）\n- 兼容 Istio 控制面（xDS API）\n- 支持 HTTP/gRPC 流量管理\n- 零信任 mTLS 在内核态实现\n- 与 Istio Ambient Mesh 互补",
     "- 资源敏感的服务网格部署\n- 需要超低延迟的微服务通信\n- Sidecar 开销不可接受的场景\n- Istio Ambient 的增强方案\n- 大规模集群的服务网格",
     "- https://kmesh.io/\n- https://github.com/kmesh-net/kmesh",
     "- [[domain-17-system-foundation/topic-dictionary/networking/istio|Istio]]\n- [[domain-17-system-foundation/topic-dictionary/networking/cilium|Cilium]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy|Envoy]]"),

    # ── Security ──
    ("security", "spiffe", "SPIFFE 身份标准", "SPIFFE",
     ["security", "identity", "cncf"],
     "SPIFFE（Secure Production Identity Framework for Everyone）是 CNCF 毕业项目，定义了工作负载身份的标准规范（SPIFFE ID + SVID），为跨平台和跨组织的微服务提供统一的安全身份框架。",
     "- **身份标准**：定义工作负载身份的标准格式（spiffe://trust-domain/path）\n- **SVID**：SPIFFE Verifiable Identity Document（X.509 或 JWT）\n- **CNCF 毕业**：经过大规模生产验证\n- **平台无关**：适用于任何平台和运行时",
     "- SPIFFE ID 格式：`spiffe://<trust-domain>/<workload-path>`\n- X.509-SVID：基于 X.509 证书的身份文档\n- JWT-SVID：基于 JWT Token 的身份文档\n- Trust Bundle：信任根分发机制\n- Workload API：工作负载获取身份的标准接口\n- Federation：跨信任域联邦",
     "- 微服务间的统一身份框架\n- 零信任网络中的工作负载认证\n- 跨组织/跨集群的身份联邦\n- 与 Istio/Envoy/SPIRE 集成\n- 合规要求下的身份管理标准化",
     "- https://spiffe.io/\n- https://github.com/spiffe/spiffe",
     "- [[domain-17-system-foundation/topic-dictionary/security/spire|SPIRE]]\n- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity|SPIFFE/SPIRE]]\n- [[domain-17-system-foundation/topic-dictionary/security/cert-manager|cert-manager]]"),

    ("security", "kubescape", "Kubescape 安全扫描", "Kubescape",
     ["security", "scanning", "compliance"],
     "Kubescape 是 ARMO 开源的 CNCF Sandbox 项目，提供 Kubernetes 集群的全方位安全扫描，包括配置审计、漏洞检测、RBAC 分析和合规检查，是集群安全评估的瑞士军刀。",
     "- **全方位扫描**：配置/漏洞/RBAC/镜像/网络策略一键扫描\n- **合规框架**：内置 NSA/CISA/MITRE/CIS 等合规基准\n- **CNCF Sandbox**：ARMO 主导\n- **左移安全**：支持 CI/CD 和 IDE 集成",
     "- `kubescape scan` 一键安全扫描\n- 支持多种框架（NSA/CISA/CIS/MITRE/SOC2）\n- RBAC 可视化分析\n- 镜像漏洞扫描（集成 Grype/Trivy）\n- NetworkPolicy 生成建议\n- 修复建议自动生成\n- Helm Chart 安全扫描",
     "- K8s 集群安全基线评估\n- 合规审计（NSA/CIS/SOC2）\n- CI/CD Pipeline 的安全门控\n- RBAC 权限审计和优化\n- 新集群上线前的安全检查",
     "- https://kubescape.io/\n- https://github.com/kubescape/kubescape",
     "- [[domain-17-system-foundation/topic-dictionary/security/trivy|Trivy]]\n- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]"),

    ("security", "kubewarden", "Kubewarden 策略引擎", "Kubewarden",
     ["security", "policy", "wasm"],
     "Kubewarden 是 SUSE 开源的 CNCF Sandbox 项目，使用 WebAssembly（Wasm）作为策略执行引擎，支持用 Rust/Go/TypeScript/Rego 等多种语言编写 Admission 策略。",
     "- **Wasm 策略引擎**：使用 WebAssembly 沙箱执行策略\n- **多语言**：支持 Rust/Go/TypeScript/Rego/Kubernetes CEL 编写策略\n- **CNCF Sandbox**：SUSE 主导\n- **安全沙箱**：Wasm 提供强隔离的策略执行环境",
     "- AdmissionPolicy / ClusterAdmissionPolicy CRD\n- Wasm 模块作为策略执行单元\n- PolicyServer 管理策略执行\n- 策略可从 OCI Registry 分发\n- 支持上下文感知（Context Aware）策略\n- Kubewarden Inspector 策略审计\n- 与 Kyverno/OPA 策略互补",
     "- Admission 策略的 Wasm 安全执行\n- 多语言策略开发\n- 策略即代码（Policy as Code）\n- 需要强隔离的策略执行环境\n- 从 OCI Registry 分发和管理策略",
     "- https://kubewarden.io/\n- https://github.com/kubewarden",
     "- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]\n- [[domain-17-system-foundation/topic-dictionary/security/gatekeeper|Gatekeeper]]"),

    ("security", "parsec", "PARSEC 机密计算", "PARSEC",
     ["security", "tee", "cncf"],
     "PARSEC（Platform AbstRaction for SECurity）是 CNCF Sandbox 项目，为应用提供统一的加密和安全服务 API，屏蔽底层 TEE（可信执行环境）和 HSM 的差异，简化机密计算的集成。",
     "- **安全 API 抽象**：统一的加密/签名/认证 API\n- **TEE 无关**：支持 Intel SGX/TDX、ARM TrustZone、TPM 等\n- **CNCF Sandbox**：Arm/Intel 联合推动\n- **简化集成**：应用无需关心底层安全硬件",
     "- Parsec API 定义标准安全操作接口\n- 多种后端 Provider（PKCS#11/TPM/Mbed Crypto/Trusted Service）\n- 密钥管理（创建/使用/删除）\n- 加密/解密/签名/验证\n- 认证和证明\n- SDK 支持 Rust/C/Go/Python/Java",
     "- 机密计算应用的快速集成\n- 多云/多硬件的安全抽象\n- IoT 设备的安全服务\n- 密钥管理的统一接口\n- TEE 应用的开发和部署",
     "- https://parallaxsecond.github.io/parsec/\n- https://github.com/parallaxsecond/parsec",
     "- [[domain-17-system-foundation/topic-dictionary/security/confidential-containers|Confidential Containers]]\n- [[domain-17-system-foundation/topic-dictionary/security/vault|Vault]]\n- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity|SPIFFE/SPIRE]]"),

    # ── Platform Engineering ──
    ("platform-engineering", "openchoreo", "OpenChoreo 开发者平台", "OpenChoreo",
     ["platform-engineering", "idp", "developer"],
     "OpenChoreo 是 WSO2 开源的内部开发者平台（IDP），基于 Kubernetes 构建，提供应用全生命周期管理、CI/CD、可观测性和 API 管理的统一平台，是 Backstage + Argo 的开箱即用整合。",
     "- **IDP 平台**：内部开发者平台的一站式方案\n- **K8s 原生**：基于 Kubernetes 的应用管理\n- **全生命周期**：从代码到生产的完整流程\n- **WSO2 开源**：基于 WSO2 Choreo 商业平台的开源核心",
     "- 组件模型（Component）定义应用和服务\n- 内置 CI/CD Pipeline\n- API 管理和网关\n- 可观测性集成（Metrics/Logs/Traces）\n- 环境管理（Dev/Staging/Prod）\n- 多租户组织管理\n- Git 驱动的配置管理",
     "- 企业级内部开发者平台\n- 应用全生命周期管理\n- API 管理和微服务治理\n- 多团队协作的开发平台\n- Backstage + Argo 的集成替代",
     "- https://openchoreo.dev/\n- https://github.com/openchoreo/openchoreo",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage|Backstage]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/argo|Argo]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/score|Score]]"),

    ("platform-engineering", "kubestellar", "KubeStellar 多集群分发", "KubeStellar",
     ["platform-engineering", "multi-cluster", "cncf"],
     "KubeStellar 是 IBM 开源的 CNCF Sandbox 项目，提供基于 Kubernetes 原生的多集群工作负载分发和同步，利用 KCP（Kubernetes Control Plane）实现跨集群的声明式资源管理。",
     "- **KCP 架构**：基于 KCP 的多集群控制面\n- **透明分发**：应用无需修改即可分发到多集群\n- **CNCF Sandbox**：IBM Research 主导\n- **K8s 原生**：不引入新的抽象层",
     "- BindingPolicy 定义资源分发策略\n- Location 描述目标集群特征\n- Inventory 集群注册和发现\n- 基于标签的集群选择\n- 资源状态聚合\n- 与 Karmada/OCM 互补的多集群方案",
     "- 大规模多集群的应用分发\n- 边缘集群的集中管理\n- 多区域部署的透明管理\n- 企业多集群治理\n- 集群生命周期管理",
     "- https://kubestellar.io/\n- https://github.com/kubestellar/kubestellar",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada|Karmada]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/open-cluster-management|OCM]]\n- [[domain-17-system-foundation/topic-dictionary/networking/clusternet|Clusternet]]"),

    # ── Fundamentals ──
    ("fundamentals", "wasmedge", "WasmEdge WASM 运行时", "WasmEdge",
     ["fundamentals", "wasm", "runtime"],
     "WasmEdge 是 Second State 开源的 CNCF Sandbox 项目，高性能 WebAssembly 运行时，专为云原生和边缘计算优化，支持 AI 推理、网络服务和嵌入式设备的 Wasm 执行。",
     "- **高性能 WASM**：JIT 编译执行，接近原生性能\n- **AI 推理**：内置 TensorFlow/PyTorch/ONNX 等 AI 推理扩展\n- **CNCF Sandbox**：Second State 主导\n- **多场景**：云/边/端统一的 Wasm 运行时",
     "- 支持 WASI（WebAssembly System Interface）\n- 网络 Socket（WASI-NN/WASI-Socket）\n- AI 推理插件（TensorFlow Lite/PyTorch/Whisper）\n- Kubernetes RuntimeClass 集成\n- 支持 JavaScript/Python/Rust Wasm 模块\n- AOT（Ahead-of-Time）编译优化",
     "- Serverless 函数的 Wasm 运行时\n- AI 推理服务的边缘部署\n- 微服务的轻量级运行时\n- 插件系统的安全沙箱\n- Kubernetes 上的 Wasm 工作负载",
     "- https://wasmedge.org/\n- https://github.com/WasmEdge/WasmEdge",
     "- [[domain-17-system-foundation/topic-dictionary/fundamentals/runc|runc]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/spinkube|SpinKube]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/kuasar|Kuasar]]"),

    # ── Observability ──
    ("observability", "opengemini", "openGemini 时序数据库", "openGemini",
     ["observability", "database", "tsdb"],
     "openGemini 是华为开源的 CNCF Sandbox 时序数据库，兼容 InfluxDB 协议，专为 IoT 和可观测性场景优化，提供高写入吞吐和低存储成本。",
     "- **InfluxDB 兼容**：兼容 InfluxDB 查询语言和 API\n- **高吞吐**：百万级数据点/秒写入\n- **CNCF Sandbox**：华为主导\n- **云原生**：存算分离架构",
     "- SQL-like 查询语言（类 InfluxQL）\n- 存算分离（支持 S3/HDFS 存储后端）\n- 时序数据自动压缩和降采样\n- 集群模式和 HA\n- 内置数据分区和保留策略\n- Prometheus Remote Write 接收",
     "- IoT 设备指标的时序存储\n- 可观测性数据的长期存储\n- InfluxDB 的国产替代方案\n- 大规模时序数据的高性能查询\n- 与 Prometheus/Grafana 集成的监控栈",
     "- https://opengemini.github.io/\n- https://github.com/openGemini/openGemini",
     "- [[domain-17-system-foundation/topic-dictionary/observability/prometheus|Prometheus]]\n- [[domain-17-system-foundation/topic-dictionary/observability/thanos|Thanos]]\n- [[domain-17-system-foundation/topic-dictionary/observability/mimir|Mimir]]"),

    # ── Operations ──
    ("operations", "krkn", "Krkn 混沌工程", "Krkn",
     ["operations", "chaos-engineering", "openshift"],
     "Krkn（原 Kraken）是 Red Hat 开源的混沌工程工具，专注于 Kubernetes/OpenShift 的故障注入，支持 Pod/Node/Network/Cloud 等多种故障场景，是 OpenShift 生态的混沌工程首选。",
     "- **K8s/OpenShift 专注**：深度集成 OpenShift 生态\n- **多故障类型**：Pod/Node/Network/Cloud/Time/PVC 故障\n- **场景驱动**：YAML 定义混沌场景\n- **Red Hat 支持**：OpenShift 测试的核心工具",
     "- Pod Disruption（删除/重启/网络隔离）\n- Node Disruption（关机等）\n- Network Chaos（延迟/丢包/DNS 故障）\n- Time Skew（时钟偏移）\n- Cloud 故障（AWS/Azure/GCP 实例停止）\n- 与 Prometheus/Grafana 集成指标",
     "- OpenShift 集群的弹性验证\n- 生产环境的故障演练\n- 网络故障的模拟和验证\n- 云资源故障的影响评估\n- CI/CD 中的弹性测试",
     "- https://krkn-chaos.dev/\n- https://github.com/krkn-chaos/krkn",
     "- [[domain-17-system-foundation/topic-dictionary/operations/litmus|LitmusChaos]]\n- [[domain-17-system-foundation/topic-dictionary/operations/chaos-mesh|Chaos Mesh]]\n- [[domain-17-system-foundation/topic-dictionary/operations/chaos-engineering|混沌工程]]"),

    ("operations", "kubean", "Kubean 集群部署", "Kubean",
     ["operations", "deployment", "cluster"],
     "Kubean 是 DaoCloud 开源的 CNCF Sandbox 项目，基于 Kubespray 提供 Kubernetes 集群的声明式部署和生命周期管理，通过 Operator 模式实现集群的自动化安装和运维。",
     "- **Kubespray 封装**：将 Kubespray 封装为 K8s Operator\n- **声明式管理**：通过 CRD 定义集群规格\n- **CNCF Sandbox**：DaoCloud 主导\n- **多环境**：支持物理机/VM/云环境部署",
     "- Cluster / Operation CRD 定义集群和运维操作\n- 支持离线安装（Air-gapped）\n- 多 CNI 支持（Calico/Cilium/Flannel/Macvlan 等）\n- 集群升级和证书轮转\n- 节点扩缩容\n- 多 OS 支持（CentOS/Ubuntu/Debian/AlmaLinux）",
     "- 生产级 K8s 集群的自动化部署\n- 离线环境的集群安装\n- 集群版本升级和运维\n- 多集群的统一部署管理\n- Kubespray 的 Operator 化使用",
     "- https://kubean.io/\n- https://github.com/kubean-io/kubean",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm|kubeadm]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/k3s|K3s]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/rancher|Rancher]]"),

    # ── Scheduling / AI ──
    ("scheduling", "hami", "HAMi 异构资源调度", "HAMi",
     ["scheduling", "gpu", "heterogeneous"],
     "HAMi（Heterogeneous AI Computing Middleware）是 CNCF Sandbox 项目，为 Kubernetes 提供 GPU/NPU/DCU 等异构计算资源的细粒度共享和调度，解决 AI 工作负载的资源碎片化问题。",
     "- **异构调度**：统一管理 GPU/NPU/DCU/RDMA 等异构资源\n- **GPU 共享**：GPU 显存和算力的细粒度切分\n- **CNCF Sandbox**：中国移动等联合推动\n- **AI 优化**：专为 AI/ML 工作负载设计",
     "- 虚拟 GPU（vGPU）切分（1/100 精度）\n- 支持 NVIDIA/AMD/华为昇腾/海光 DCU\n- GPU 显存隔离和算力隔离\n- 资源用量监控和统计\n- 与 Volcano 调度器集成\n- 支持 MIG（Multi-Instance GPU）",
     "- AI 训练集群的 GPU 资源共享\n- 推理服务的 GPU 细粒度分配\n- 多种异构加速卡的统一管理\n- GPU 利用率的优化和降本\n- 多租户 AI 平台的资源隔离",
     "- https://github.com/Project-HAMi/HAMi\n- https://project-hami.io/",
     "- [[domain-17-system-foundation/topic-dictionary/scheduling/koordinator|Koordinator]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/volcano|Volcano]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/kaito|KAITO]]"),

    # ── Storage ──
    ("storage", "piraeus-datastore", "Piraeus 分布式存储", "Piraeus",
     ["storage", "replication", "cncf"],
     "Piraeus Datastore 是 LINBIT 开源的 CNCF Sandbox 项目，基于 DRBD/LINSTOR 为 Kubernetes 提供高性能的分布式块存储复制，实现有状态应用的同步复制和高可用。",
     "- **DRBD 复制**：基于 DRBD 的块级同步/异步复制\n- **LINSTOR 管理**：自动化存储资源管理\n- **CNCF Sandbox**：LINBIT 主导\n- **CSI 驱动**：标准 K8s CSI 集成",
     "- LINSTOR CSI Driver\n- 同步复制（R1）和异步复制（R2）\n- 存储池管理和自动配置\n- 快照和克隆\n- 自动故障转移\n- 加密存储卷\n- 多站点复制",
     "- 数据库的高可用存储\n- 需要同步复制的有状态应用\n- 裸金属环境的分布式存储\n- 替代 Ceph RBD 的轻量方案\n- 多站点的存储复制",
     "- https://piraeus.io/\n- https://github.com/piraeusdatastore/piraeus-operator",
     "- [[domain-17-system-foundation/topic-dictionary/storage/rook|Rook]]\n- [[domain-17-system-foundation/topic-dictionary/storage/longhorn|Longhorn]]\n- [[domain-17-system-foundation/topic-dictionary/storage/openebs|OpenEBS]]"),

    # ── Specialized ──
    ("specialized-workloads", "spin", "Spin WASM 框架", "Spin",
     ["specialized-workloads", "wasm", "serverless"],
     "Spin 是 Fermyon 开源的 WebAssembly 应用开发框架，支持用 Rust/Go/Python/JavaScript/TypeScript 编写 Serverless Wasm 应用，是 SpinKube 的底层开发框架。",
     "- **多语言 SDK**：Rust/Go/Python/JS/TS 编写 Wasm 组件\n- **Serverless 模型**：基于 HTTP/Redis 触发的函数执行\n- **Fermyon 主导**：Wasm 应用平台的开源核心\n- **组件模型**：基于 WebAssembly Component Model",
     "- `spin new` 创建应用模板\n- `spin build` 编译为 Wasm\n- `spin up` 本地运行\n- 支持 HTTP 触发器和 Redis 触发器\n- KV/SQLite 内置存储\n- SpinKube 部署到 K8s\n- Fermyon Cloud 托管部署",
     "- Serverless API 的快速开发\n- 边缘计算的函数运行时\n- 微服务的 Wasm 化改造\n- 多语言 Wasm 应用的统一开发\n- 安全沙箱中的插件执行",
     "- https://www.fermyon.com/spin\n- https://github.com/fermyon/spin",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/spinkube|SpinKube]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/wasmedge|WasmEdge]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative|Knative]]"),

    ("specialized-workloads", "wasmcloud", "wasmCloud WASM 平台", "wasmCloud",
     ["specialized-workloads", "wasm", "distributed"],
     "wasmCloud 是 CNCF Sandbox 项目，提供基于 WebAssembly 的分布式应用运行时，通过 Actor 模型和能力接口（Capability）构建安全、可移植的分布式系统。",
     "- **Actor 模型**：基于 Actor 的分布式应用架构\n- **能力接口**：标准化的能力抽象（HTTP/KV/Messaging/Logging）\n- **CNCF Sandbox**：Cosmonic 主导\n- **安全沙箱**：Wasm 提供强隔离的执行环境",
     "- wash CLI 开发工具\n- Actor（组件）和 Provider（能力提供者）\n- Lattice（分布式运行时网格）\n- WIT（Wasm Interface Types）定义接口\n- OCI Registry 分发组件\n- 多语言支持（Rust/Go/TypeScript/Python）",
     "- 分布式微服务的 Wasm 化\n- 跨云/边的可移植应用\n- 安全隔离的插件架构\n- IoT/边缘的分布式应用\n- 能力驱动的组件化开发",
     "- https://wasmcloud.com/\n- https://github.com/wasmCloud/wasmCloud",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/spin|Spin]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/wasmedge|WasmEdge]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/dapr|Dapr]]"),

    # ── Tooling ──
    ("tooling", "slimtoolkit", "SlimToolkit 容器优化", "SlimToolkit",
     ["tooling", "container", "optimization"],
     "SlimToolkit（原 DockerSlim）是 CNCF Sandbox 项目，通过静态和动态分析自动缩小容器镜像体积（通常减少 10-30 倍），同时保持应用功能完整，是容器镜像优化的标准工具。",
     "- **镜像瘦身**：自动分析并删除镜像中未使用的文件\n- **CNCF Sandbox**：社区主导的容器优化工具\n- **安全加固**：减少攻击面（删除不必要的包和工具）\n- **零修改**：无需修改 Dockerfile 或应用代码",
     "- `slim build` 分析并生成精简镜像\n- 静态分析（文件系统扫描）\n- 动态分析（运行容器并追踪文件访问）\n- HTTP 探针（自动触发 API 端点）\n- 安全配置文件生成（Seccomp/AppArmor）\n- 镜像层级分析和可视化",
     "- CI/CD Pipeline 中的镜像自动优化\n- 生产镜像的安全加固\n- 镜像体积的成本优化\n- 安全合规的攻击面减少\n- 开发镜像到生产镜像的转换",
     "- https://slimtoolkit.org/\n- https://github.com/slimtoolkit/slim",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/docker|Docker]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/buildpacks|Buildpacks]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/copa|Copa]]"),

    ("tooling", "werf", "werf CI/CD 工具", "werf",
     ["tooling", "ci-cd", "gitops"],
     "werf 是 Flant 开源的 CNCF Sandbox 项目，一站式 CI/CD 工具，集成构建、部署和运维功能，支持 GitOps 工作流，将 Dockerfile/Helm/K8s 整合为统一的工作流。",
     "- **一站式**：构建 + 推送 + 部署的完整 CI/CD 流程\n- **GitOps 原生**：以 Git 为唯一配置源\n- **CNCF Sandbox**：Flant 主导\n- **多环境**：支持 dev/staging/prod 环境管理",
     "- werf.yaml 定义构建和部署\n- Stapel/Buildah 构建引擎\n- Helm Chart 集成部署\n- 三态 Git 重设（Three-stage Git-based rebasing）\n- 自动清理过期镜像\n- Namespace/Release 管理\n- werf converge 一键部署",
     "- GitOps 的完整 CI/CD Pipeline\n- Helm Chart 的自动化部署\n- 开发环境的快速搭建\n- 多环境的应用管理\n- 替代 Argo/Flux 的一站式方案",
     "- https://werf.io/\n- https://github.com/werf/werf",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/argo|Argo]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/flux|Flux]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/helm|Helm]]"),

    ("tooling", "zot", "zot OCI 注册表", "zot",
     ["tooling", "registry", "oci"],
     "zot 是 Cisco 开源的 CNCF Sandbox 项目，轻量级 OCI 原生容器注册表，专为边缘和嵌入式场景优化，资源占用极低，支持 OCI 1.1 规范。",
     "- **轻量级**：单二进制，极低资源占用\n- **OCI 原生**：完整实现 OCI Distribution Spec\n- **CNCF Sandbox**：Cisco 主导\n- **边缘优化**：适用于资源受限的环境",
     "- 支持 OCI Image/Artifact/Index\n- Referrers API（OCI 1.1 附件引用）\n- 搜索 API（OCI 搜索规范）\n- 多架构镜像支持\n- 同步复制（zot-to-zot）\n- 认证（Bearer/Basic/LDAP）\n- 存储驱动（文件系统/S3）",
     "- 边缘设备的本地 OCI Registry\n- 开发环境的轻量镜像仓库\n- CI/CD Pipeline 的临时 Registry\n- IoT 设备的镜像分发\n- OCI 制品（Helm/WASM/SBOM）的存储",
     "- https://zotregistry.dev/\n- https://github.com/project-zot/zot",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/distribution|Distribution]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/harbor|Harbor]]\n- [[domain-17-system-foundation/topic-dictionary/security/notary-project|Notary Project]]"),

    # ── Workloads ──
    ("workloads", "slimfaas", "SlimFaas 轻量 FaaS", "SlimFaas",
     ["workloads", "serverless", "faas"],
     "SlimFaas 是 Axa France 开源的超轻量级 Kubernetes FaaS（Function as a Service）平台，以极低的复杂度和资源开销在 K8s 上运行函数，是 Knative/OpenFaaS 的极简替代。",
     "- **超轻量**：极简的 FaaS 实现，资源占用极低\n- **K8s 原生**：基于 K8s Deployment/HPA 实现\n- **零冷启动**：支持保持 Pod 常驻避免冷启动\n- **Axa France**：企业级 Serverless 实践",
     "- SlimData（内置轻量持久化）\n- 基于 HPA 的自动扩缩\n- HTTP 触发器（同步/异步）\n- 事件驱动（Pub/Sub 模式）\n- 多语言函数支持\n- 资源限制和 QoS 管理",
     "- 内部系统的轻量 Serverless 需求\n- Knative/OpenFaaS 的极简替代\n- 微服务的函数化处理\n- 事件驱动的异步处理\n- 开发团队的自助 Serverless 平台",
     "- https://github.com/AxaFrance/SlimFaas\n- https://axafrance.github.io/SlimFaas/",
     "- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/openfaas|OpenFaaS]]\n- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/keda|KEDA]]"),

    # ── Platform / Config ──
    ("configuration", "schemahero", "SchemaHero 数据库 Schema 管理", "SchemaHero",
     ["configuration", "database", "gitops"],
     "SchemaHero 是 Replicated 开源的 CNCF Sandbox 项目，以 GitOps 方式管理数据库 Schema 变更，通过声明式 YAML 定义表结构，自动生成和执行 Migration SQL。",
     "- **GitOps Schema**：YAML 声明式管理数据库 Schema\n- **自动 Migration**：自动生成 ALTER/CREATE SQL\n- **CNCF Sandbox**：Replicated 主导\n- **多数据库**：支持 PostgreSQL/MySQL/CockroachDB/SQLite",
     "- Table CRD 声明式定义表结构\n- 自动检测 Schema 差异\n- 生成并执行 Migration SQL\n- 支持索引、约束、外键\n- Plan/Apply 两阶段审核\n- K8s Operator 模式部署",
     "- 数据库 Schema 的版本控制\n- GitOps 方式的数据库变更管理\n- 微服务数据库的独立 Schema 管理\n- CI/CD Pipeline 中的 Schema 迁移\n- 合规要求下的 Schema 变更审计",
     "- https://schemahero.io/\n- https://github.com/schemahero/schemahero",
     "- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg|CloudNativePG]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/flux|Flux]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/argo|Argo]]"),

    ("platform-engineering", "kcp", "KCP 多租户控制面", "KCP",
     ["platform-engineering", "multi-tenancy", "api"],
     "KCP（Kubernetes-like Control Plane）是 Red Hat 开源的 CNCF Sandbox 项目，提供 Kubernetes 兼容的 API 控制面，但不运行容器，用于构建多租户平台和管理跨集群资源。",
     "- **K8s API 兼容**：提供标准 K8s API 但不运行 Pod\n- **多租户**：Workspace 模型实现层次化多租户\n- **CNCF Sandbox**：Red Hat 主导\n- **元控制面**：管理其他 K8s 集群的控制面",
     "- Workspace 层次化命名空间\n- APIBinding/APIExport 跨 Workspace 资源共享\n- Syncer 将资源同步到实际 K8s 集群\n- 多集群资源视图\n- 与 KubeStellar/OCM 集成\n- Placement API 资源放置",
     "- SaaS 平台的多租户控制面\n- 内部开发者平台的 API 层\n- 多集群资源编排的元控制面\n- K8s API 的定制化扩展\n- 服务目录和自助服务平台",
     "- https://kcp.io/\n- https://github.com/kcp-dev/kcp",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubestellar|KubeStellar]]\n- [[domain-17-system-foundation/topic-dictionary/security/capsule|Capsule]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage|Backstage]]"),
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
