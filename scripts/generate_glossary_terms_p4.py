#!/usr/bin/env python3
"""P4: 扩充 Configuration（10个）和 Multi-Cloud（5个）分类"""
from pathlib import Path
BASE = Path("domain-17-system-foundation/topic-dictionary")

def w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists():
        print(f"  = {cat}/{fn}.md (已存在)")
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
    print(f"  + {cat}/{fn}.md")
    return True

TERMS = [
    # ── Configuration ──
    ("configuration", "helm-values", "Helm Values 配置值", "Helm Values",
     ["configuration", "helm", "templating"],
     "Helm Values 是 Helm Chart 的参数化配置机制，通过 values.yaml 文件定义模板变量，实现同一 Chart 在不同环境下的差异化部署，是 Helm 模板系统的核心配置入口。",
     "- **参数化**：values.yaml 定义 Chart 的所有可配置参数\n- **层级覆盖**：支持 --set/--values/-f 多层覆盖\n- **Go Template**：在模板中通过 `.Values.xxx` 引用\n- **默认值**：Chart 内置的 values.yaml 作为默认",
     "- values.yaml 默认值文件\n- `--set key=value` 命令行覆盖\n- `--values file.yaml` 文件覆盖\n- `--set-file` 从文件读取值\n- 嵌套值（`global.image.tag`）\n- 条件渲染（`{{ if .Values.enabled }}`）\n- values.schema.json 校验",
     "- 多环境（dev/staging/prod）的差异化部署\n- Chart 参数化复用\n- 应用配置的外部化管理\n- CI/CD 中的动态配置注入\n- 最佳实践：默认值兜底、分层覆盖、schema 校验、避免过度嵌套",
     "- https://helm.sh/docs/chart_template_guide/values_files/\n- https://helm.sh/docs/intro/using_helm/",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/helm|Helm]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/configmap|ConfigMap]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/env|Environment Variables]]"),

    ("configuration", "kustomization", "Kustomization 配置清单", "Kustomization",
     ["configuration", "kustomize", "overlay"],
     "Kustomization 是 Kustomize 的核心配置文件，通过 kustomization.yaml 定义基础资源（bases）和叠加层（overlays），实现声明式、无模板的 K8s 配置管理。",
     "- **无模板**：直接操作 YAML，不使用模板语言\n- **叠加模式**：base + overlay 的分层配置\n- **K8s 内置**：kubectl apply -k 原生支持\n- **声明式**：所有变更通过 patch 声明",
     "- kustomization.yaml 入口文件\n- resources 声明基础资源列表\n- bases 引入其他 kustomization\n- patchesStrategicMerge 策略合并补丁\n- patchesJson6902 JSON Patch\n- commonLabels/commonAnnotations 全局标签\n- namePrefix/nameSuffix 名称前缀\n- generators（ConfigMap/Secret 生成器）",
     "- 多环境配置的差异化管理\n- 上游 YAML 的定制化修改\n- GitOps 配置管理（Flux/ArgoCD）\n- 团队间的配置隔离\n- 最佳实践：overlay 不超过 3 层、bases 保持纯净、用 components 替代重复 overlay",
     "- https://kubectl.docs.kubernetes.io/references/kustomize/kustomization/\n- https://kustomize.io/",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kpt|kpt]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/helm|Helm]]"),

    ("configuration", "resource-quota", "资源配额", "ResourceQuota",
     ["configuration", "multi-tenancy", "resource-management"],
     "ResourceQuota 是 Kubernetes 命名空间级别的资源配额机制，限制命名空间可使用的计算资源（CPU/Memory）和对象数量（Pod/PVC/Service），是多租户资源治理的核心手段。",
     "- **命名空间级**：限制每个命名空间的资源总量\n- **多维度**：计算资源 + 存储资源 + 对象数量\n- **硬限制**：超过配额后请求被拒绝\n- **优先级**：不保证公平，先到先得",
     "- `spec.hard` 定义资源上限\n- 计算资源：requests.cpu/memory、limits.cpu/memory\n- 存储资源：requests.storage、persistentvolumeclaims\n- 对象计数：count/pods、count/services 等\n- 作用域：Terminating/NotTerminating/BestEffort/NotBestEffort\n- 配额生效延迟（非实时）\n- 与 LimitRange 配合使用",
     "- 多租户的资源隔离和公平分配\n- 防止单命名空间耗尽集群资源\n- 成本控制和预算管理\n- 开发/测试环境的资源限制\n- 最佳实践：配合 LimitRange 设默认值、预留缓冲、监控配额使用率",
     "- https://kubernetes.io/docs/concepts/policy/resource-quotas/\n- https://kubernetes.io/docs/concepts/policy/limit-range/",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/limit-range|LimitRange]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace|Namespace]]\n- [[domain-17-system-foundation/topic-dictionary/security/rbac|RBAC]]"),

    ("configuration", "limit-range", "限制范围", "LimitRange",
     ["configuration", "resource-management", "defaults"],
     "LimitRange 是 Kubernetes 命名空间级别的资源默认值和约束机制，为 Pod/Container 自动设置资源的 requests/limits 默认值，并强制最大最小值约束。",
     "- **默认值注入**：为未设置 requests/limits 的容器自动填充\n- **约束强制**：拒绝超出最大/最小范围的请求\n- **命名空间级**：每个命名空间独立配置\n- **与 ResourceQuota 配合**：防止资源滥用",
     "- `spec.limits` 定义约束列表\n- type: Container/Pod/PersistentVolumeClaim\n- default 默认 limits 值\n- defaultRequest 默认 requests 值\n- max/min 最大最小约束\n- maxLimitRequestRatio 限制比率\n- 仅对新 Pod 生效（不追溯已有 Pod）",
     "- 为未设置资源限制的容器提供默认值\n- 防止过大或过小的资源请求\n- 存储卷的大小约束\n- 配合 ResourceQuota 确保配额可用\n- 最佳实践：设合理的默认值、max 不超过节点容量、配合 VPA 自动调优",
     "- https://kubernetes.io/docs/concepts/policy/limit-range/\n- https://kubernetes.io/docs/tasks/administer-cluster/manage-resources/",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/resource-quota|ResourceQuota]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/vpa|VPA]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace|Namespace]]"),

    ("configuration", "taint-toleration", "污点和容忍", "Taints and Tolerations",
     ["configuration", "scheduling", "node"],
     "Taints（污点）和 Tolerations（容忍）是 Kubernetes 的调度约束机制，节点通过 Taint 排斥不匹配的 Pod，Pod 通过 Toleration 声明接受特定污点，实现节点级的工作负载隔离。",
     "- **节点排斥**：Taint 让节点拒绝不匹配的 Pod\n- **Pod 容忍**：Toleration 让 Pod 接受特定污点\n- **三效果**：NoSchedule/PreferNoSchedule/NoExecute\n- **系统级**：Master/控制节点的标准隔离方案",
     "- `taint`: key=value:effect\n- NoSchedule：新 Pod 不调度到此节点\n- PreferNoSchedule：尽量不调度（软限制）\n- NoExecute：驱逐已运行的不容忍 Pod\n- tolerationSeconds：NoExecute 的容忍时间\n- 系统污点：node.kubernetes.io/not-ready/unreachable\n- DaemonSet 自动容忍常见污点",
     "- 专用节点（GPU/SSD/高配）的工作负载隔离\n- Master/控制节点的保护性污点\n- 节点维护时的 Pod 驱逐\n- 多租户的节点隔离\n- 最佳实践：专用节点用 NoSchedule、故障用 NoExecute+tolerationSeconds、避免滥用",
     "- https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/\n- https://kubernetes.io/docs/reference/labels-annotations-taints/",
     "- [[domain-17-system-foundation/topic-dictionary/scheduling/node-affinity|Node Affinity]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-topology-spread|Topology Spread]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster|Cluster]]"),

    ("configuration", "downward-api", "Downward API 元数据注入", "Downward API",
     ["configuration", "env", "volume"],
     "Downward API 是 Kubernetes 将 Pod/Container 元数据（名称、命名空间、标签、资源限制等）注入到容器内部的机制，支持环境变量和 Volume 文件两种方式。",
     "- **元数据暴露**：将 Pod 自身信息注入到容器\n- **双通道**：环境变量（env.valueFrom.fieldRef）和 Volume\n- **只读**：容器只能读取，不能修改\n- **实时性**：Volume 方式支持标签/注解变更的自动更新",
     "- fieldRef 注入 Pod 元数据（name/namespace/uid/labels/annotations）\n- resourceFieldRef 注入资源信息（limits.cpu/requests.memory）\n- DownwardAPIVolumeFile 写入 Volume 文件\n- 支持的字段：metadata.name/namespace/labels/annotations、spec.nodeName/nodeIP、status.podIP/hostIP\n- Volume 方式支持热更新\n- 环境变量方式需重启生效",
     "- 容器内获取自身 Pod 信息\n- 日志标签注入（pod_name/namespace）\n- 服务自注册的 IP 获取\n- 资源感知的自适应配置\n- 最佳实践：热更新用 Volume、简单值用 env、避免循环依赖",
     "- https://kubernetes.io/docs/concepts/workloads/pods/downward-api/\n- https://kubernetes.io/docs/tasks/inject-data-application/downward-api-volume-expose-pod-information/",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/env|Environment Variables]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/configmap|ConfigMap]]\n- [[domain-17-system-foundation/topic-dictionary/workloads/pod|Pod]]"),

    ("configuration", "priority-class", "优先级类", "PriorityClass",
     ["configuration", "scheduling", "preemption"],
     "PriorityClass 是 Kubernetes 的 Pod 优先级定义资源，通过 priorityClassName 关联到 Pod，实现高优先级 Pod 对低优先级 Pod 的抢占（Preemption），保障关键工作负载的调度优先权。",
     "- **优先级声明**：定义命名空间的 Pod 优先级等级\n- **抢占机制**：高优先级 Pod 可驱逐低优先级 Pod\n- **全局资源**：PriorityClass 是集群级资源\n- **内置优先级**：system-cluster-critical/system-node-critical",
     "- `value`：优先级数值（越大越优先，最大 10^9）\n- `globalDefault`：未指定时是否为默认优先级\n- `preemptionPolicy`：PreemptLowerPriority/Never\n- `description`：优先级说明\n- 系统优先级：2000000000+/1000000000+\n- Scheduler 在资源不足时触发抢占\n- 抢占过程：找牺牲 Pod → 驱逐 → 调度",
     "- 生产关键应用的优先级保障\n- 批处理任务的低优先级标记\n- DaemonSet 的系统级优先级\n- 资源竞争时的调度策略\n- 最佳实践：分层优先级（系统>生产>测试>批处理）、避免全部高优先级",
     "- https://kubernetes.io/docs/concepts/scheduling-eviction/pod-priority-preemption/\n- https://kubernetes.io/docs/concepts/configuration/pod-priority-preemption/",
     "- [[domain-17-system-foundation/topic-dictionary/scheduling/pod-disruption-budget|PDB]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/preemption|Preemption]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/resource-quota|ResourceQuota]]"),

    ("configuration", "strategic-merge-patch", "策略合并补丁", "Strategic Merge Patch",
     ["configuration", "patch", "api"],
     "Strategic Merge Patch 是 Kubernetes 特有的 JSON 合并策略，针对列表类型提供智能合并（按 key 合并而非替换），是 kubectl apply 和 K8s 控制器的默认补丁策略。",
     "- **K8s 特有**：区别于标准 JSON Merge Patch\n- **列表合并**：按 patchStrategy 定义的 key 合并列表元素\n- **默认策略**：kubectl apply 使用此策略\n- **CRD 支持**：通过 kubebuilder 注解定义",
     "- `$patch: replace` 替换整个字段\n- `$patch: delete` 删除字段\n- `$patch: merge` 合并（默认）\n- patchStrategy: merge（按 key 合并列表）\n- patchMergeKey: 合并的标识字段（如 name/port）\n- 保留列表（retainKeys）策略\n- 与 JSON Patch（RFC 6902）和 Server-Side Apply 对比",
     "- kubectl apply 的底层合并逻辑\n- Operator 的状态合并\n- 声明式配置的部分更新\n- kubectl patch 命令使用\n- 最佳实践：了解 patchStrategy、复杂更新用 SSA、避免意外覆盖",
     "- https://kubernetes.io/docs/tasks/manage-kubernetes-objects/update-api-object-kubectl-patch/\n- https://github.com/kubernetes/community/blob/master/contributors/devel/sig-api-machinery/strategic-merge-patch.md",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl|kubectl]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/server-side-apply|Server-Side Apply]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize|Kustomize]]"),

    ("configuration", "server-side-apply", "服务端 Apply SSA", "Server-Side Apply",
     ["configuration", "api", "field-management"],
     "Server-Side Apply（SSA）是 Kubernetes 1.22+ GA 的配置管理特性，在 API Server 端执行声明式合并，支持多管理者（Manager）的字段所有权追踪和冲突检测。",
     "- **服务端合并**：API Server 执行 merge 逻辑\n- **字段所有权**：追踪每个字段由哪个 Manager 管理\n- **冲突检测**：多个 Manager 修改同一字段时告警\n- **GA 特性**：K8s 1.22 起正式可用",
     "- `fieldManager` 声明管理者身份\n- `force` 强制获取字段所有权\n- ManagedFields 元数据追踪\n- 与 Client-Side Apply（CSA）对比\n- `kubectl apply --server-side`\n- Controller 的 SSA 模式（controller-gen）\n- 与 Strategic Merge Patch 的差异",
     "- 多控制器的声明式管理\n- Controller 开发的最佳实践\n- GitOps 工具的配置应用\n- 复杂对象的增量更新\n- 最佳实践：指定 fieldManager、理解冲突处理、控制器用 SSA",
     "- https://kubernetes.io/docs/reference/using-api/server-side-apply/\n- https://kubernetes.io/blog/2021/08/06/server-side-apply-ga/",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/strategic-merge-patch|Strategic Merge Patch]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl|kubectl]]\n- [[domain-17-system-foundation/topic-dictionary/operations/argo|Argo]]"),

    ("configuration", "validating-webhook", "准入校验 Webhook", "ValidatingAdmissionWebhook",
     ["configuration", "admission", "webhook"],
     "ValidatingAdmissionWebhook 是 Kubernetes 准入控制器的扩展机制，允许外部服务拦截 API 请求进行自定义校验，在资源写入 etcd 前执行策略检查。",
     "- **准入控制**：拦截 API Server 请求进行校验\n- **只读校验**：只验证不修改请求内容\n- **外部扩展**：通过 Webhook 调用外部服务\n- **失败策略**：Fail（拒绝）/Ignore（放行）",
     "- ValidatingWebhookConfiguration 注册\n- rules 匹配资源类型和操作\n- namespaceSelector/objectSelector 过滤范围\n- failurePolicy: Fail/Ignore\n- sideEffects: None/NoneOnDryRun\n- admissionReviewVersions 版本协商\n- 超时配置（默认 10s）",
     "- 自定义策略校验（命名规范/标签要求）\n- 安全合规检查（镜像来源/权限级别）\n- 成本管控（资源限制验证）\n- 与 OPA/Kyverno 集成\n- 最佳实践：快速响应（<1s）、Fail 策略要慎重、做好灰度",
     "- https://kubernetes.io/docs/reference/access-authn-authz/extensible-admission-controllers/\n- https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definition-versioning/",
     "- [[domain-17-system-foundation/topic-dictionary/security/opa|OPA]]\n- [[domain-17-system-foundation/topic-dictionary/security/kyverno|Kyverno]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resource|Custom Resource]]"),

    # ── Multi-Cloud ──
    ("multi-cloud", "cluster-api", "Cluster API 集群生命周期", "Cluster API",
     ["multi-cloud", "lifecycle", "cncf"],
     "Cluster API（CAPI）是 CNCF 孵化项目，使用 Kubernetes 声明式 API 管理集群的生命周期（创建/升级/扩缩/删除），是声明式集群管理的标准框架。",
     "- **声明式集群管理**：CRD 定义集群/机器/基础设施\n- **Provider 模型**：基础设施/引导/控制平面 Provider\n- **CNCF 孵化**：K8s SIG Cluster Lifecycle 核心项目\n- **GitOps 友好**：集群状态纳入 Git 管理",
     "- Cluster CRD 定义目标集群\n- Machine/MachineSet/MachineDeployment 工作节点管理\n- Infrastructure Provider（AWS/Azure/GCP/Docker/OpenStack）\n- Bootstrap Provider（kubeadm/ignition）\n- Control Plane Provider（kubeadm/K3s）\n- ClusterClass 集群模板\n- 自动化升级（滚动更新）",
     "- 大规模集群的声明式管理\n- 多基础设施的集群自动化\n- GitOps 式集群生命周期\n- 集群的自动扩缩容\n- 最佳实践：ClusterClass 标准化、Git 管理、渐进式升级",
     "- https://cluster-api.sigs.k8s.io/\n- https://github.com/kubernetes-sigs/cluster-api",
     "- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster|Cluster]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm|kubeadm]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubestellar|KubeStellar]]"),

    ("multi-cloud", "federation", "K8s 集群联邦", "Federation",
     ["multi-cloud", "federation", "multi-cluster"],
     "Kubernetes Federation（Federation v2/KubeFed）是多集群管理的标准和实现，通过联邦控制平面统一管理跨多个 K8s 集群的资源和策略，实现全局调度和策略一致性。",
     "- **多集群管理**：统一管理多个 K8s 集群\n- **策略传播**：全局策略下发到成员集群\n- **KubeFed v2**：当前主流的联邦实现\n- **灵活联邦**：不要求所有集群完全统一",
     "- FederatedTypeConfig 定义联邦资源类型\n- 资源模板（Template）+ 放置策略（Placement）\n- Override 集群级覆盖\n- 联邦 DNS（跨集群服务发现）\n- 联邦 RBAC（全局权限管理）\n- 策略控制器（合规检查）\n- 多集群 Ingress",
     "- 全球分布的多集群管理\n- 多区域容灾和高可用\n- 统一的策略和合规管理\n- 跨区域流量调度\n- 最佳实践：从单集群开始、渐进式联邦、策略先行",
     "- https://github.com/kubernetes-sigs/kubefed\n- https://kubernetes.io/docs/concepts/cluster-administration/federation/",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada|Karmada]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/open-cluster-management|OCM]]\n- [[domain-17-system-foundation/topic-dictionary/scheduling/kubefleet|KubeFleet]]"),

    ("multi-cloud", "multi-cluster-service", "多集群服务 MCS", "Multi-Cluster Service",
     ["multi-cloud", "service-discovery", "networking"],
     "Multi-Cluster Service（MCS）是 Google/Anthos 推动的多集群服务发现标准，通过 ServiceImport/ServiceExport CRD 实现跨集群的服务注册和发现，已被纳入 Gateway API 生态。",
     "- **跨集群发现**：服务在不同集群间自动发现\n- **Gateway API 集成**：GAMMA  initiative 的核心组件\n- **标准化**：SIG Multicluster 推动的标准 API\n- **透明路由**：客户端无需知道服务在哪个集群",
     "- ServiceExport CRD 声明导出的服务\n- ServiceImport CRD 表示在其他集群导入的服务\n- EndpointSlice 跨集群同步\n- 支持 ClusterIP/Headless 两种导入模式\n- 与 Service Mesh 集成（Istio/Linkerd）\n- DNS 自动注册（.svc.clusterset.local）\n- 网络连通性前提（VPC Peering/VPN）",
     "- 多集群微服务的统一访问\n- 跨区域容灾的服务切换\n- 蓝绿部署的跨集群流量\n- 服务网格的多集群扩展\n- 最佳实践：先确保网络连通、配合 Service Mesh、做好服务版本管理",
     "- https://github.com/kubernetes-sigs/mcs-api\n- https://gateway-api.sigs.k8s.io/guides/",
     "- [[domain-17-system-foundation/topic-dictionary/networking/service|Service]]\n- [[domain-17-system-foundation/topic-dictionary/networking/service-mesh|Service Mesh]]\n- [[domain-17-system-foundation/topic-dictionary/networking/envoy-gateway|Envoy Gateway]]"),

    ("multi-cloud", "crossplane-composition", "Crossplane 资源组合", "Crossplane Composition",
     ["multi-cloud", "crossplane", "iac"],
     "Crossplane Composition 是 Crossplane 的组合式基础设施管理特性，通过 CompositeResourceDefinition（XRD）和 Composition 将底层云资源抽象为面向平台用户的高级 API。",
     "- **抽象层**：将底层云资源包装为平台 API\n- **XRD**：CompositeResourceDefinition 定义新资源类型\n- **Composition**：定义 XRD 到具体资源的映射\n- **多 Provider**：AWS/Azure/GCP/K8s 等 Provider",
     "- XRD 定义面向用户的抽象 API\n- Composition 定义资源组合和转换逻辑\n- Composition Functions 可编程的转换逻辑\n- Patch Sets 声明式参数传递\n- Multiple Compositions 支持\n- 环境配置（EnvironmentConfig）\n- Usage（资源使用记录）",
     "- 内部开发平台（IDP）的基础设施 API\n- 多云资源的统一管理接口\n- 自助式基础设施服务\n- 基础设施的标准化和合规\n- 最佳实践：合理的抽象层级、版本演进策略、充分的测试",
     "- https://docs.crossplane.io/latest/concepts/compositions/\n- https://github.com/crossplane/crossplane",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/backstage|Backstage]]\n- [[domain-17-system-foundation/topic-dictionary/multi-cloud/cluster-api|Cluster API]]"),

    ("multi-cloud", "cloud-credential-operator", "云凭证管理 CCO", "Cloud Credential Operator",
     ["multi-cloud", "credentials", "openshift"],
     "Cloud Credential Operator（CCO）是 Red Hat 开源的 K8s Operator，自动管理云提供商凭证（IAM Roles/Service Accounts），为集群组件和 Operator 安全分发最小权限的云访问凭证。",
     "- **凭证自动化**：自动创建和管理云凭证\n- **最小权限**：为每个组件生成精确的 IAM 策略\n- **多 Provider**：AWS/Azure/GCP/OpenStack\n- **OpenShift 核心**：OpenShift 安装流程的核心组件",
     "- CredentialsRequest CRD 声明云凭证需求\n- Mint 模式（自动创建 IAM 用户/角色）\n- Passthrough 模式（使用共享凭证）\n- Manual 模式（管理员手动配置）\n- STS/Workload Identity 集成\n- 凭证轮转和审计\n- ccoctl 命令行工具",
     "- 集群组件的云权限自动管理\n- 最小权限原则的执行\n- 多账户/多项目的凭证隔离\n- 合规要求下的凭证审计\n- 最佳实践：使用 STS/Workload Identity、定期审计、最小权限",
     "- https://github.com/openshift/cloud-credential-operator\n- https://docs.openshift.com/container-platform/latest/authentication/managing_cloud_provider_credentials/",
     "- [[domain-17-system-foundation/topic-dictionary/multi-cloud/cluster-api|Cluster API]]\n- [[domain-17-system-foundation/topic-dictionary/security/spiffe|SPIFFE]]\n- [[domain-17-system-foundation/topic-dictionary/security/vault|Vault]]"),
]

created, skipped = 0, 0
for t in TERMS:
    if w(*t):
        created += 1
    else:
        skipped += 1

print(f"\n新创建: {created}, 跳过: {skipped}")
