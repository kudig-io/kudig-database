#!/usr/bin/env python3
"""Round 10: 剩余缺失术语批量展开（25个）"""
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
    # ── Tooling ──
    ("tooling", "artifact-hub", "Artifact Hub 制品市场", "Artifact Hub",
     ["tooling", "registry", "cncf"],
     "Artifact Hub 是 CNCF 孵化项目，云原生制品的集中发现和分发平台，支持 Helm Chart、OPA 策略、OOCI 镜像、Kustomize、Tekton 等多种云原生制品的搜索和发布。",
     "- **多制品类型**：Helm/OPA/Tekton/Kustomize/Keptn/CoreDNS 等\n- **CNCF 孵化**：云原生制品的标准市场\n- **搜索发现**：统一的搜索和元数据索引\n- **社区驱动**：开放的制品发布平台",
     "- 支持 Helm Chart、Container、OPA、Tinkerbell、Keda 等制品\n- 仓库管理和版本控制\n- 安全评分（基于 Trivy/Grype 扫描）\n- 用户评价和收藏\n- Star 和 Fork 机制\n- Webhook 通知\n- CLI 工具 `ah` 管理",
     "- 云原生制品的发现和搜索\n- Helm Chart 的发布和分发\n- OPA 策略库的共享\n- CI/CD 模板的市场\n- 安全评估的参考平台",
     "- https://artifacthub.io/\n- https://github.com/artifacthub/hub",
     "- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/distribution|Distribution]]\n- [[系统基础/topic-dictionary/tooling/harbor|Harbor]]"),

    ("tooling", "carvel", "Carvel K8s 工具集", "Carvel",
     ["tooling", "configuration", "vmware"],
     "Carvel（原 K14s）是 VMware 开源的 Kubernetes 工具集，包含 ytt（YAML 模板）、kapp（应用部署）、kbld（镜像构建）、kwt（网络隧道）等一组轻量级互补工具。",
     "- **工具集**：一组专注于单一功能的轻量级 CLI 工具\n- **可组合**：工具间通过标准输入输出自由组合\n- **VMware 开源**：Tanzu 生态的核心工具链\n- **UNIX 哲学**：每个工具做好一件事",
     "- ytt：YAML 模板引擎（Starlark 脚本）\n- kapp：声明式应用部署（diff + apply）\n- kbld：镜像构建和引用解析\n- kwt：K8s 网络隧道（本地访问集群网络）\n- vendir：依赖管理（下载 Helm/Git/HTTP 资源）\n- imgpkg：镜像打包和分发",
     "- YAML 配置的模板化和复用\n- K8s 应用的声明式部署\n- 镜像构建和引用自动化\n- 本地开发环境的网络打通\n- Helm Chart 的依赖管理",
     "- https://carvel.dev/\n- https://github.com/carvel-dev",
     "- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/tooling/kpt|kpt]]"),

    ("tooling", "cdk8s", "cdk8s 声明式 K8s CDK", "cdk8s",
     ["tooling", "cdk", "configuration"],
     "cdk8s（Cloud Development Kit for Kubernetes）是 CNCF Sandbox 项目，允许使用 TypeScript/Python/Java/Go 等编程语言定义 Kubernetes 资源，编译为标准 YAML 清单。",
     "- **编程式定义**：用编程语言（非 YAML）定义 K8s 资源\n- **类型安全**：利用编程语言的类型系统检查配置\n- **CNCF Sandbox**：AWS CDK 团队主导\n- **多语言**：TypeScript/Python/Java/Go",
     "- Constructs 组件模型（可复用资源组合）\n- Charts 图表（K8s 资源集合）\n- Apps 应用（Chart 集合）\n- cdk8s import 导入 CRD 类型\n- Helm Chart 集成（cdk8s-plus）\n- cdk8s synth 合成 YAML 输出\n- cdk8s-plus 高级抽象库",
     "- 复杂 K8s 配置的编程化管理\n- 需要类型安全的配置定义\n- 配置模板的复用和组合\n- Helm/Kustomize 的编程式替代\n- Infrastructure as Code 的统一",
     "- https://cdk8s.io/\n- https://github.com/cdk8s-team/cdk8s",
     "- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/configuration/kcl|KCL]]"),

    ("tooling", "devfile", "Devfile 开发环境规范", "Devfile",
     ["tooling", "development", "cncf"],
     "Devfile 是 CNCF Sandbox 项目，定义了云开发环境的声明式规范，用 YAML 描述开发环境的组件、命令和依赖，实现开发环境的可移植和可复现。",
     "- **开发环境标准**：统一描述开发环境的 YAML 规范\n- **可移植**：同一 Devfile 可在多种平台运行\n- **CNCF Sandbox**：Red Hat/OpenShift Dev Spaces 核心\n- **Registry**：社区 Devfile 仓库",
     "- devfile.yaml 定义开发环境\n- Components（容器/Volume/Git 组件）\n- Commands（build/run/test/debug）\n- 预置开发栈（Java/Node.js/Go/Python 等）\n- Devfile Registry 社区仓库\n- DevWorkspace Operator K8s 集成",
     "- 团队开发环境的标准化\n- 云端 IDE 的开发环境配置\n- 新成员的快速环境搭建\n- CI/CD 中的开发环境复现\n- 多平台开发环境的统一管理",
     "- https://devfile.io/\n- https://github.com/devfile/api",
     "- [[系统基础/topic-dictionary/tooling/telepresence|Telepresence]]\n- [[系统基础/topic-dictionary/tooling/minikube|Minikube]]\n- [[系统基础/topic-dictionary/platform-engineering/backstage|Backstage]]"),

    ("tooling", "atlantis", "Atlantis Terraform 自动化", "Atlantis",
     ["tooling", "terraform", "ci-cd"],
     "Atlantis 是开源的 Terraform Pull Request 自动化工具，在 PR 中自动执行 terraform plan/apply，为基础设施变更提供代码审查和自动化部署工作流。",
     "- **PR 驱动**：在 PR 中自动运行 terraform plan\n- **审查流程**：基础设施变更的代码审查和批准\n- **多仓库**：支持多个 Terraform 项目\n- **社区成熟**：广泛使用的 Terraform CI/CD 方案",
     "- Webhook 监听 PR 事件\n- 自动检测变更的 Terraform 目录\n- `atlantis plan` 在 PR 评论中展示计划\n- `atlantis apply` 在 PR 批准后执行\n- 多 workspace/目录管理\n- 支持 Terragrunt/OpenTofu\n- 自定义工作流（pre/post hooks）",
     "- Terraform 变更的 PR 审查流程\n- 基础设施变更的自动化部署\n- 多团队协作的 Terraform 管理\n- GitOps 式的基础设施管理\n- 合规要求下的变更审计",
     "- https://www.runatlantis.io/\n- https://github.com/runatlantis/atlantis",
     "- [[系统基础/topic-dictionary/tooling/opentofu|OpenTofu]]\n- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[系统基础/topic-dictionary/platform-engineering/argo|Argo]]"),

    ("tooling", "shipwright", "Shipwright 容器构建", "Shipwright",
     ["tooling", "container", "build"],
     "Shipwright 是 Red Hat 开源的 CNCF Sandbox 项目，在 Kubernetes 上提供声明式的容器镜像构建框架，支持 Buildpacks、Buildah、Kaniko 等多种构建策略。",
     "- **K8s 原生构建**：在集群内以 Pod 方式执行镜像构建\n- **多策略**：支持 Buildpacks/Buildah/Kaniko/Dockerfile\n- **CNCF Sandbox**：Red Hat/Tekton 生态组件\n- **Tekton 集成**：可作为 Tekton Pipeline 的构建步骤",
     "- Build / BuildRun CRD 定义构建任务\n- ClusterBuildStrategy / BuildStrategy 构建策略\n- 支持 Dockerfile/Buildpacks/Buildah/Ko 等\n- 源码从 Git/Bundle 获取\n- 推送到任意 OCI Registry\n- Tekton Task 集成\n- 构建参数化和模板",
     "- K8s 集群内的镜像构建\n- CI/CD Pipeline 的构建步骤\n- 多构建策略的统一框架\n- 无 Docker Daemon 的镜像构建\n- 企业内部的安全镜像构建",
     "- https://shipwright.io/\n- https://github.com/shipwright-io/build",
     "- [[系统基础/topic-dictionary/tooling/buildpacks|Buildpacks]]\n- [[系统基础/topic-dictionary/tooling/docker|Docker]]\n- [[系统基础/topic-dictionary/tooling/tekton|Tekton]]"),

    # ── Security ──
    ("security", "kubearmor", "KubeArmor 运行时安全", "KubeArmor",
     ["security", "runtime", "ebpf"],
     "KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security Modules）为 Kubernetes 提供运行时安全策略，限制容器的文件/网络/进程行为。",
     "- **eBPF + LSM**：在内核层拦截容器的系统调用\n- **运行时策略**：限制容器可访问的文件/网络/进程\n- **CNCF Sandbox**：Accuknox 主导\n- **可视化**：提供安全事件的可视化和告警",
     "- KubeArmorPolicy CRD 定义安全策略\n- 文件访问控制（读写/执行限制）\n- 网络访问控制（出站/入站限制）\n- 进程执行控制（允许/拒绝列表）\n- AppArmor/SELinux/BPF-LSM 后端\n- 安全事件日志和告警\n- KubeArmor VM（非 K8s 环境支持）",
     "- 容器运行时的安全加固\n- 最小权限原则的强制执行\n- 合规要求下的运行时安全策略\n- 零信任架构中的工作负载保护\n- 安全审计和合规报告",
     "- https://kubearmor.io/\n- https://github.com/kubearmor/KubeArmor",
     "- [[系统基础/topic-dictionary/security/falco|Falco]]\n- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/security/kyverno|Kyverno]]"),

    ("security", "openfga", "OpenFGA 授权引擎", "OpenFGA",
     ["security", "authorization", "cncf"],
     "OpenFGA 是 CNCF Sandbox 项目，高性能的关系型授权引擎，基于 Google Zanzibar 论文实现，为应用提供细粒度的权限检查（如 'user X can read document Y'）。",
     "- **Zanzibar 实现**：基于 Google Zanzibar 的关系型授权模型\n- **高性能**：微秒级权限检查延迟\n- **CNCF Sandbox**：Okta/Auth0 主导\n- **关系模型**：灵活的用户-对象-权限关系定义",
     "- Authorization Model 定义权限关系\n- Relationship Tuples 存储权限关系\n- Check API 权限检查\n- ListObjects API 列出可访问对象\n- WriteAuthorizationModel 动态更新模型\n- SDK（Go/JS/Python/Java/.NET）\n- Playground 可视化调试",
     "- 应用的细粒度授权\n- 文档/资源的权限管理\n- SaaS 产品的多租户权限\n- 社交网络的关注/好友关系\n- 替代 RBAC/ABAC 的灵活授权方案",
     "- https://openfga.dev/\n- https://github.com/openfga/openfga",
     "- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/security/keycloak|Keycloak]]"),

    ("security", "paralus", "Paralus 访问控制", "Paralus",
     ["security", "access-control", "multi-cluster"],
     "Paralus 是 CNCF Sandbox 项目，为 Kubernetes 提供集中式的访问控制和审计平台，支持 SSO、RBAC 和 kubectl 访问代理，是多集群权限管理的统一方案。",
     "- **集中访问控制**：统一管理多集群的 K8s 访问权限\n- **SSO 集成**：支持 OIDC/SAML/LDAP 身份源\n- **CNCF Sandbox**：Rafay 主导\n- **审计追踪**：完整的 kubectl 命令审计日志",
     "- Zero Trust Access 代理（无需 VPN）\n- 基于角色的 kubectl 访问控制\n- 多集群的 RBAC 统一管理\n- SSO 和 MFA 集成\n- 命令审计和回放\n- JIT（Just-in-Time）临时权限\n- 用户/组/项目层次管理",
     "- 多集群的 K8s 权限集中管理\n- 开发团队的 kubectl 安全访问\n- 合规要求下的访问审计\n- SSO 集成的统一认证\n- 临时权限的安全分发",
     "- https://www.paralus.io/\n- https://github.com/paralus/paralus",
     "- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/security/keycloak|Keycloak]]\n- [[系统基础/topic-dictionary/security/dex|Dex]]"),

    # ── Platform Engineering ──
    ("platform-engineering", "kubevela", "KubeVela 应用交付", "KubeVela",
     ["platform-engineering", "oam", "cncf"],
     "KubeVela 是阿里巴巴开源的 CNCF 孵化项目，基于 OAM（Open Application Model）的现代应用交付平台，提供声明式、可扩展、面向最终用户的应用管理能力。",
     "- **OAM 实现**：Open Application Model 的参考实现\n- **可扩展**：CUE 语言定义可复用的组件和工作流\n- **CNCF 孵化**：阿里巴巴主导\n- **多集群**：支持多集群应用分发",
     "- Application CRD 定义应用\n- ComponentDefinition / TraitDefinition 组件扩展\n- Workflow 步骤定义（部署/检查/通知等）\n- 多集群环境管理\n- Helm/Kustomize/Terraform 集成\n- VelaUX 可视化管理界面\n- Addon 插件市场",
     "- 平台团队的 IDP 底层引擎\n- 复杂应用的多集群交付\n- OAM 标准的应用管理\n- 开发者自助服务平台\n- GitOps 应用交付",
     "- https://kubevela.io/\n- https://github.com/kubevela/kubevela",
     "- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[系统基础/topic-dictionary/platform-engineering/argo|Argo]]\n- [[系统基础/topic-dictionary/platform-engineering/score|Score]]"),

    ("platform-engineering", "cadence", "Cadence 工作流引擎", "Cadence",
     ["platform-engineering", "workflow", "uber"],
     "Cadence 是 Uber 开源的分布式工作流引擎（后由 Uber 团队成立独立公司维护），为长时间运行的有状态应用提供持久化执行、重试和可见性能力。",
     "- **持久化工作流**：工作流状态持久化，崩溃后自动恢复\n- **长时间运行**：支持数月甚至数年的工作流\n- **Uber 开源**：经过 Uber 大规模生产验证\n- **Temporal 前身**：Temporal 是 Cadence 的演进版本",
     "- Workflow/Activity 编程模型\n- 信号（Signal）和查询（Query）\n- 定时器（Timer）和子工作流\n- 版本管理和迁移\n- 搜索属性（Search Attributes）\n- Cadence Web UI\n- 多租户 Domain 管理",
     "- 长时间运行的业务流程编排\n- 微服务的分布式事务协调\n- 基础设施自动化工作流\n- 数据 Pipeline 编排\n- 定时任务和 Cron 替代",
     "- https://cadenceworkflow.io/\n- https://github.com/cadence-workflow/cadence",
     "- [[系统基础/topic-dictionary/platform-engineering/dapr|Dapr]]\n- [[系统基础/topic-dictionary/workloads/serverless-workflow|Serverless Workflow]]\n- [[系统基础/topic-dictionary/platform-engineering/tekton|Tekton]]"),

    ("platform-engineering", "cozystack", "Cozystack 云操作系统", "Cozystack",
     ["platform-engineering", "cloud", "paas"],
     "Cozystack 是开源的 Kubernetes 云操作系统，在 K8s 之上提供完整的 PaaS 能力（VM/数据库/存储/K8s-as-a-Service），通过统一 API 管理多种基础设施服务。",
     "- **云操作系统**：在 K8s 上构建完整的云平台\n- **多服务**：VM/DB/存储/K8s 集群的统一管理\n- **开源**：完全开源的 PaaS 方案\n- **API 驱动**：统一的 RESTful API 管理所有服务",
     "- Tenant CRD 多租户管理\n- 虚拟化管理（KubeVirt 集成）\n- 数据库服务（PostgreSQL/MySQL/Redis）\n- 对象存储和块存储\n- Kubernetes-as-a-Service\n- 计费和使用计量\n- Cozystack Dashboard",
     "- 私有云/混合云的 PaaS 建设\n- 企业内部的基础设施服务平台\n- IDC 的云服务化转型\n- 开发和测试环境的自助服务\n- 多租户的云平台运营",
     "- https://cozystack.io/\n- https://github.com/cozystack/cozystack",
     "- [[系统基础/topic-dictionary/specialized-workloads/kubevirt|KubeVirt]]\n- [[系统基础/topic-dictionary/platform-engineering/rancher|Rancher]]\n- [[系统基础/topic-dictionary/platform-engineering/backstage|Backstage]]"),

    # ── Fundamentals ──
    ("fundamentals", "bpfman", "bpfman eBPF 管理器", "bpfman",
     ["fundamentals", "ebpf", "daemon"],
     "bpfman 是 Red Hat 开源的 CNCF Sandbox 项目，作为系统级守护进程管理 eBPF 程序的加载和生命周期，解决多个应用争用 eBPF 挂载点的冲突问题。",
     "- **eBPF 管理器**：集中管理 eBPF 程序的加载和卸载\n- **冲突解决**：多个程序挂载到同一 hook 点的优先级管理\n- **CNCF Sandbox**：Red Hat 主导\n- **系统服务**：以 systemd 服务方式运行",
     "- gRPC API 管理 eBPF 程序\n- 支持 XDP/TC/Tracepoint/Uprobe 等 hook 类型\n- 优先级和顺序管理\n- eBPF 映射（Map）的持久化\n- Kubernetes CSI 驱动（K8s 集成）\n- 与 Cilium/Tetragon 等兼容\n- bpfilter 防火墙集成",
     "- 多个 eBPF 应用的共存管理\n- eBPF 程序的生命周期管理\n- K8s 节点上 eBPF 的统一部署\n- 安全工具的 eBPF 程序管理\n- eBPF 开发者的标准接口",
     "- https://bpfman.io/\n- https://github.com/bpfman/bpfman",
     "- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/observability/pixie|Pixie]]\n- [[系统基础/topic-dictionary/security/falco|Falco]]"),

    ("fundamentals", "virtual-kubelet", "Virtual Kubelet 虚拟节点", "Virtual Kubelet",
     ["fundamentals", "node", "serverless"],
     "Virtual Kubelet 是 CNCF Sandbox 项目，通过 Kubelet 接口将外部服务（如 Serverless 平台/云 API/VM）伪装为 Kubernetes 节点，实现 Pod 调度到非 K8s 基础设施。",
     "- **虚拟节点**：将外部资源伪装为 K8s 节点\n- **CNCF Sandbox**：Microsoft/VMware 联合推动\n- **透明调度**：Pod 可透明调度到 Serverless 平台\n- **Provider 模式**：插件式 Provider 支持多种后端",
     "- Provider 接口（实现 Node/Pod 生命周期管理）\n- 内置 Provider：Azure ACI、AWS Fargate 等\n- Taints 控制 Pod 调度到虚拟节点\n- 与 K8s Service/Ingress 集成\n- Metrics/Log 代理\n- 自定义 Provider 开发",
     "- Serverless 容器的 K8s 调度\n- 突发扩容到云服务（burst to cloud）\n- 异构计算资源的统一管理\n- 多集群的 Pod 调度\n- 开发和测试环境的虚拟节点",
     "- https://virtual-kubelet.io/\n- https://github.com/virtual-kubelet/virtual-kubelet",
     "- [[系统基础/topic-dictionary/scheduling/cluster-autoscaler|Cluster Autoscaler]]\n- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/workloads/pod|Pod]]"),

    ("fundamentals", "hyperlight", "Hyperlight 微虚拟机", "Hyperlight",
     ["fundamentals", "microvm", "security"],
     "Hyperlight 是微软开源的项目，提供超轻量的安全微虚拟机（microVM），专为 Wasm 和容器工作负载设计，在 Windows/Linux Hypervisor 上实现毫秒级启动和极低开销的隔离。",
     "- **微虚拟机**：毫秒级启动的安全隔离环境\n- **Hypervisor 驱动**：利用 Hyper-V/KVM 硬件虚拟化\n- **微软开源**：Azure 基础设施的安全组件\n- **Wasm 友好**：专为 Wasm 工作负载优化",
     "- 基于 Hyper-V（Windows）/KVM（Linux）\n- 轻量 Guest OS（<10MB 内存）\n- 共享内存主机-Guest 通信\n- Wasm 运行时集成（WasmEdge/Wasmtime）\n- Rust 实现的安全 API\n- 与 containerd shim 集成",
     "- Serverless 函数的安全隔离\n- Wasm 工作负载的高性能沙箱\n- 多租户环境的安全隔离\n- 替代 Firecracker 的跨平台方案\n- 边缘设备的轻量虚拟化",
     "- https://github.com/hyperlight-dev/hyperlight\n- https://hyperlight.dev/",
     "- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/fundamentals/runc|runc]]\n- [[系统基础/topic-dictionary/fundamentals/urunc|urunc]]"),

    ("fundamentals", "kairos", "Kairos 不可变 OS", "Kairos",
     ["fundamentals", "os", "edge"],
     "Kairos（原 c3os）是 Spectro Cloud 开源的 CNCF Sandbox 项目，将任意 Linux 发行版转换为不可变的容器操作系统，支持用容器镜像管理整个 OS，适用于边缘和 Kubernetes 节点。",
     "- **容器即 OS**：用 OCI 镜像定义完整的操作系统\n- **任意发行版**：基于 Alpine/Ubuntu/openSUSE 等构建\n- **CNCF Sandbox**：Spectro Cloud 主导\n- **边缘优化**：专为边缘设备设计",
     "- A/B 分区原子升级和回滚\n- cloud-init 系统配置\n- P2P 网络（节点自发现和自组网）\n- K3s/K0s 内置集成\n- UKI（Unified Kernel Image）支持\n- Elemental Operator K8s 管理\n- 安全启动（Secure Boot）",
     "- 边缘设备的 OS 管理\n- K8s 节点的标准化 OS\n- 不可变基础设施的 OS 层\n- IoT 设备的远程 OS 更新\n- 多发行版的统一 OS 管理",
     "- https://kairos.io/\n- https://github.com/kairos-io/kairos",
     "- [[系统基础/topic-dictionary/fundamentals/flatcar|Flatcar]]\n- [[系统基础/topic-dictionary/tooling/bootc|bootc]]\n- [[系统基础/topic-dictionary/tooling/k3s|K3s]]"),

    ("fundamentals", "container2wasm", "container2wasm 容器转换", "container2wasm",
     ["fundamentals", "wasm", "container"],
     "container2wasm 是 containerd 作者之一 Kazuyoshi Kato 开源的工具，将 Linux 容器镜像转换为 WebAssembly 模块（WASI），使容器可以在 Wasm 运行时（浏览器/边缘设备）中运行。",
     "- **容器转 Wasm**：将 OCI 镜像转换为 .wasm 文件\n- **Linux 模拟**：通过 Wasm 模拟 Linux 系统调用\n- **广泛运行**：转换后可在浏览器/Wasm 运行时中运行\n- **containerd 生态**：与 containerd 深度集成",
     "- `ctr-remote` 转换工具\n- 支持 amd64/arm64 容器镜像\n- WASI Preview 1 输出\n- 与 WasmEdge/Wasmtime/Wasmer 兼容\n- 转换后的镜像可在浏览器中运行\n- 文件系统打包（ext4 in Wasm）",
     "- 容器工作负载的边缘部署\n- 浏览器中的容器应用演示\n- Wasm 运行时的容器兼容性\n- 安全沙箱中的容器执行\n- 跨架构的容器运行",
     "- https://github.com/ktock/container2wasm\n- https://ktock.medium.com/",
     "- [[系统基础/topic-dictionary/fundamentals/wasmedge|WasmEdge]]\n- [[系统基础/topic-dictionary/fundamentals/docker|Docker]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|containerd]]"),

    # ── Operations ──
    ("operations", "cloud-custodian", "Cloud Custodian 云治理", "Cloud Custodian",
     ["operations", "cloud", "governance"],
     "Cloud Custodian（c7n）是 CNCF Sandbox 项目，多云环境的统一治理引擎，通过声明式 YAML 策略管理云资源的合规性、安全和成本优化，支持 AWS/Azure/GCP。",
     "- **多云治理**：统一管理 AWS/Azure/GCP 的资源策略\n- **声明式策略**：YAML 定义资源过滤和操作\n- **CNCF Sandbox**：Capital One 主导\n- **事件驱动**：响应云事件自动执行策略",
     "- Policy YAML 定义治理规则\n- Filters 资源过滤（标签/年龄/大小/成本等）\n- Actions 资源操作（停止/终止/通知/标记）\n- 支持 Cron 和事件触发\n- 多账户/多区域管理\n- 输出到 S3/CloudWatch/SQS\n- c7n-org 多账户编排",
     "- 云资源的合规性检查\n- 闲置资源的自动清理\n- 安全配置的统一审计\n- 成本优化（自动关闭非工作时段资源）\n- 标签策略的强制执行",
     "- https://cloudcustodian.io/\n- https://github.com/cloud-custodian/cloud-custodian",
     "- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/observability/opencost|OpenCost]]\n- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]"),

    ("operations", "kuberhealthy", "Kuberhealthy 合成监控", "Kuberhealthy",
     ["operations", "monitoring", "synthetic"],
     "Kuberhealthy 是 CNCF Sandbox 项目，在 Kubernetes 上运行合成监控检查（Synthetic Checks），以 Pod 方式定期验证集群组件（DNS/API/存储/网络等）的健康状态。",
     "- **合成监控**：主动探测集群组件健康状态\n- **Pod 化检查**：每个检查以 Pod 方式运行\n- **CNCF Sandbox**：社区驱动的 K8s 监控工具\n- **Prometheus 集成**：标准 metrics 输出",
     "- KuberhealthyCheck CRD 定义检查任务\n- 内置检查（DNS/API Server/Deployment/Pod 状态）\n- 自定义检查（任意容器化检查脚本）\n- Prometheus metrics 导出\n- Grafana Dashboard 集成\n- 超时和重试配置\n- 告警集成（Alertmanager）",
     "- K8s 集群的主动健康检查\n- DNS/网络/存储的连通性验证\n- 升级前后的功能回归测试\n- 多集群的统一健康监控\n- SLO 验证的自动化检查",
     "- https://kuberhealthy.github.io/kuberhealthy/\n- https://github.com/kuberhealthy/kuberhealthy",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/operations/kube-burner|kube-burner]]\n- [[系统基础/topic-dictionary/observability/kepler|Kepler]]"),

    ("operations", "chaosblade", "ChaosBlade 混沌工程", "ChaosBlade",
     ["operations", "chaos-engineering", "alibaba"],
     "ChaosBlade 是阿里巴巴开源的混沌工程工具，支持对 Java/C++/Node.js 应用和 Kubernetes/Docker/物理机环境的故障注入，是国内最广泛使用的混沌工程框架之一。",
     "- **多平台**：K8s/Docker/物理机/云环境\n- **多语言**：Java/C++/Node.js/Go 应用级故障注入\n- **阿里开源**：经过双11大规模验证\n- **CNCF Landscape**：混沌工程领域代表项目",
     "- `blade create` 创建故障实验\n- Pod/Container/Node/Network/Process/JVM 故障类型\n- 应用级故障（方法延迟/异常/返回值修改）\n- 文件系统故障（读写延迟/磁盘满）\n- 网络故障（延迟/丢包/DNS 异常）\n- ChaosBlade Operator（K8s CRD 管理）\n- 实验自动恢复",
     "- 微服务的弹性验证\n- 生产环境的故障演练\n- 数据库/中间件的故障注入\n- Java 应用的方法级故障模拟\n- 双11前的全链路压测和故障演练",
     "- https://chaosblade.io/\n- https://github.com/chaosblade-io/chaosblade",
     "- [[系统基础/topic-dictionary/operations/chaos-mesh|Chaos Mesh]]\n- [[系统基础/topic-dictionary/operations/litmus|LitmusChaos]]\n- [[系统基础/topic-dictionary/operations/krkn|Krkn]]"),

    # ── Observability ──
    ("observability", "logging-operator", "Logging Operator 日志路由", "Logging Operator",
     ["observability", "logging", "operator"],
     "Logging Operator 是 Kube Logging（原 Banzai Cloud）开源的 CNCF Sandbox 项目，通过 Operator 模式管理 Kubernetes 日志采集和路由，统一 Fluent Bit + Fluentd/Flame 的部署和配置。",
     "- **Operator 模式**：CRD 管理日志采集和路由\n- **双层架构**：Fluent Bit（采集）+ Fluentd/Syslog-NG（聚合）\n- **CNCF Sandbox**：Kube Logging 社区主导\n- **多租户**：支持日志的租户隔离和路由",
     "- Logging CRD 定义日志基础设施\n- Flow / ClusterFlow 定义日志路由\n- Output / ClusterOutput 定义日志目标\n- 自动部署 Fluent Bit DaemonSet\n- 支持 Fluentd 和 Syslog-NG 后端\n- 日志过滤和转换（Filter）\n- 多租户日志隔离（Tenant CRD）",
     "- K8s 日志的统一采集和路由\n- 多租户环境的日志隔离\n- 日志转发到多种后端（ES/Loki/S3/Kafka）\n- 日志格式化和过滤\n- Fluentd/Fluent Bit 的自动化运维",
     "- https://kube-logging.dev/\n- https://github.com/kube-logging/logging-operator",
     "- [[系统基础/topic-dictionary/observability/fluentd|Fluentd]]\n- [[系统基础/topic-dictionary/observability/loki|Loki]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]"),

    # ── Storage ──
    ("storage", "oxia", "Oxia 元数据协调", "Oxia",
     ["storage", "metadata", "coordination"],
     "Oxia 是 DataStax 开源的分布式元数据协调服务，设计为 Apache Pulsar 的 ZooKeeper 替代品，提供高性能的分布式锁、序列号和元数据管理。",
     "- **ZooKeeper 替代**：专为云原生设计的元数据协调服务\n- **高性能**：基于 RocksDB + Raft 的高吞吐实现\n- **Pulsar 优化**：Apache Pulsar 的下一代元数据后端\n- **DataStax 开源**：活跃的分布式系统社区",
     "- Key-Value 存储（Get/Put/Delete）\n- 分布式锁（Lock/Unlock）\n- 序列号生成（Sequence）\n- Session 管理\n- Watch 通知机制\n- 快照和恢复\n- 多节点 Raft 集群",
     "- 分布式系统的元数据协调\n- 消息队列的元数据后端\n- 分布式锁和领导者选举\n- 配置中心的底层存储\n- ZooKeeper 的现代化替代",
     "- https://github.com/streamnative/oxia\n- https://oxia.dev/",
     "- [[系统基础/topic-dictionary/storage/etcd|etcd]]\n- [[系统基础/topic-dictionary/storage/tikv|TiKV]]\n- [[系统基础/topic-dictionary/storage/vineyard|Vineyard]]"),

    # ── Networking ──
    ("networking", "interlink", "InterLink HPC 互联", "InterLink",
     ["networking", "hpc", "virtual-kubelet"],
     "InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将 HPC（高性能计算）资源接入 Kubernetes，实现 K8s 工作负载在 HPC 集群上运行。",
     "- **HPC 集成**：将 HPC 集群（Slurm/HTCondor）接入 K8s\n- **Virtual Kubelet**：基于 VK Provider 模式实现\n- **CNCF Sandbox**：INFN 主导\n- **科学计算**：为科学研究提供 HPC 资源",
     "- Virtual Kubelet Provider for HPC\n- 支持 Slurm/HTCondor/Kubernetes 后端\n- Pod 到 HPC Job 的转换\n- 数据管理（输入/输出文件传输）\n- GPU/大内存节点的调度\n- Sidecar 容器支持\n- HPC 资源配额管理",
     "- AI/ML 训练的 HPC 资源利用\n- 科学计算工作负载的 K8s 管理\n- 混合云+HPC 的资源调度\n- 大规模模拟任务的资源弹性\n- 科研机构的计算资源统一管理",
     "- https://interlink-expect.github.io/\n- https://github.com/intertwin-eu/interLink",
     "- [[系统基础/topic-dictionary/fundamentals/virtual-kubelet|Virtual Kubelet]]\n- [[系统基础/topic-dictionary/scheduling/volcano|Volcano]]\n- [[系统基础/topic-dictionary/scheduling/hami|HAMi]]"),

    # ── Workloads ──
    ("workloads", "openfunction", "OpenFunction Serverless", "OpenFunction",
     ["workloads", "serverless", "cncf"],
     "OpenFunction 是青云科技开源的 CNCF Sandbox 项目，云原生 FaaS 平台，支持同步/异步函数、多种运行时和事件源，集成 Knative 和 OpenFuncAsync（Dapr）两种运行模式。",
     "- **双模式**：Knative（同步 HTTP）+ OpenFuncAsync（异步事件）\n- **多运行时**：支持 Node.js/Go/Python/Java/Rust\n- **CNCF Sandbox**：青云科技主导\n- **Dapr 集成**：利用 Dapr 的构建块能力",
     "- Function CRD 定义函数\n- Builder CRD 函数镜像构建\n- Serving CRD 函数运行时管理\n- Knative 同步服务\n- OpenFuncAsync 异步事件驱动（Dapr + KEDA）\n- Shipwright 镜像构建集成\n- 多事件源（Kafka/NATS/Redis 等）",
     "- 事件驱动的 Serverless 函数\n- 微服务的函数化拆分\n- 数据处理的异步 Pipeline\n- API 后端的 Serverless 化\n- 多运行时函数的统一管理",
     "- https://openfunction.dev/\n- https://github.com/openfunction/openfunction",
     "- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/specialized-workloads/openfaas|OpenFaaS]]\n- [[系统基础/topic-dictionary/scheduling/keda|KEDA]]"),

    # ── Scheduling ──
    ("scheduling", "kubefleet", "KubeFleet 多集群调度", "KubeFleet",
     ["scheduling", "multi-cluster", "fleet"],
     "KubeFleet 是微软开源的 CNCF Sandbox 项目，提供 Kubernetes 多集群的应用编排和调度，通过 Fleet 概念统一管理大量集群的应用分发和生命周期。",
     "- **Fleet 管理**：统一管理数百个 K8s 集群\n- **智能调度**：基于集群能力和亲和性选择目标\n- **CNCF Sandbox**：微软 Azure Fleet 的开源核心\n- **渐进式发布**：支持分批滚动部署",
     "- MemberCluster CRD 集群注册\n- InternalMemberCluster 集群状态\n- ClusterResourcePlacement 资源分发\n- ClusterSchedulingPolicy 调度策略\n- 分批滚动更新（Rolling Update）\n- 集群能力感知调度\n- Work CRD 资源同步",
     "- 大规模多集群应用分发\n- 边缘集群的集中管理\n- 应用的渐进式多集群发布\n- 集群资源能力的智能调度\n- 全球分布的应用编排",
     "- https://github.com/Azure/fleet\n- https://aka.ms/kubefleet",
     "- [[系统基础/topic-dictionary/platform-engineering/kubestellar|KubeStellar]]\n- [[系统基础/topic-dictionary/platform-engineering/karmada|Karmada]]\n- [[系统基础/topic-dictionary/platform-engineering/open-cluster-management|OCM]]"),

    # ── Multi-tenancy ──
    ("security", "tokenetes", "Tokenetes 令牌管理", "Tokenetes",
     ["security", "identity", "k8s"],
     "Tokenetes 是开源的 Kubernetes Token 管理服务，为 K8s 提供安全的短期令牌签发和验证能力，支持服务间认证、API 访问令牌和身份联盟场景。",
     "- **令牌管理**：K8s ServiceAccount Token 的增强管理\n- **短期令牌**：自动签发和轮转短期访问令牌\n- **身份联盟**：跨集群的令牌交换和验证\n- **K8s 增强**：补充 K8s 原生 Token 的能力",
     "- ServiceAccount Token 的签发和验证\n- Token 交换（Token Exchange RFC 8693）\n- 外部身份提供商集成\n- Token 的审计和监控\n- 短期令牌的自动轮转\n- 与 OIDC Federation 集成",
     "- 服务间的安全认证\n- 多集群的令牌联邦\n- 外部系统的 K8s 访问令牌\n- 合规要求下的令牌审计\n- 短期访问凭证的管理",
     "- https://github.com/tokenetes/tokenetes",
     "- [[系统基础/topic-dictionary/security/spiffe|SPIFFE]]\n- [[系统基础/topic-dictionary/security/spire|SPIRE]]\n- [[系统基础/topic-dictionary/security/keycloak|Keycloak]]"),
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
