---
title: RBAC × Multi-tenancy
summary: RBAC 与多租户的交叉：Kubernetes 原生 RBAC 如何支撑租户隔离，及其在软/硬多租户模型中的边界。
category: synthesis
tags:
- rbac
- multi-tenancy
- isolation
- kubernetes
- security
tier: supporting
sources:
- 概念/rbac-authorization.md
- 概念/multi-tenancy-isolation.md
- 概念/network-policy.md
- 实体/capsule.md
- 实体/kyverno.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.74
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# RBAC × Multi-tenancy

## The Connection

多租户（Multi-tenancy）的目标是让多个团队/客户共享同一集群而互不干扰，RBAC（Role-Based Access Control）是 Kubernetes 提供的"谁能对哪些资源做什么"的原生授权机制。RBAC 通过四类对象协同工作——`Role`/`ClusterRole` 定义权限规则（resource + verb + resourceNames），`RoleBinding`/`ClusterRoleBinding` 将规则绑定到主体（User/Group/ServiceAccount）。请求经过认证（Authentication）后进入 RBAC 授权链，再由准入控制器（Admission）做策略校验，三层串联构成 Kubernetes 的访问控制栈。在"软多租户"（信任租户）模型中，RBAC + namespace 划分已足够；在"硬多租户"（不信任租户）模型中，RBAC 只是第一层，还需 NetworkPolicy、节点隔离、甚至沙箱运行时（gVisor/Kata Containers）配合。RBAC 的核心局限在于：它只控制 API Server 的写/读操作，无法限制已被授权 Pod 的运行时行为（如探测 `/proc`、发起网络侧信道攻击），因此 RBAC 是多租户治理的授权基线，但绝不是隔离的全部。^[inferred]

## Where They Co-occur

- **Namespace 即租户边界**：每个租户一个/一组 namespace，RBAC RoleBinding 把租户身份限制在自己的 namespace 内，ResourceQuota/LimitRange 约束资源消耗上限。
- **ClusterRole 聚合**：平台团队用 ClusterRole 提供通用能力（如只读监控），租户管理员用 Role 在自己 namespace 内二次授权；`aggregationRule` 可按标签动态合并多个 ClusterRole。
- **Capsule/Kiosk**：在 RBAC 之上构建"租户"抽象，自动管理 namespace 配额、网络策略、RBAC 绑定的生命周期，实现租户自助创建 namespace 而不失控。
- **Kyverno/OPA 准入**：RBAC 决定"能否做"，策略引擎决定"做的对不对"——例如禁止租户创建特权 Pod、绑定 cluster-admin 或使用 `hostPath`/`hostNetwork`。
- **NetworkPolicy 联动**：RBAC 防越权操作（控制面），NetworkPolicy 防越权访问（数据面），二者共同构成租户隔离的网络与控制面防线。
- **Service Account 即工作负载身份**：跨租户访问通过 SA + RBAC 表达，是服务网格 mTLS 身份的基础；`TokenRequest` API 支持限时、受众绑定的令牌投影，降低长期 token 泄露风险。
- **审计日志驱动合规**：RBAC 授权决策记录在 audit log 中，配合 Falco/Tetragon 的运行时事件，可重建"谁在何时做了什么"的完整租户活动审计链。
- **Hierarchical Namespace Controller (HNC)**：通过 namespace 父子层级自动继承 RBAC 绑定和配额，简化深层组织结构下的权限管理。
- **Token Review 与 Subject Access Review**：`kubectl auth can-i` 底层调用 `SubjectAccessReview` API，允许工具链（如 Dashboard、ArgoCD）在执行操作前检查权限，避免"操作到一半才发现 RBAC 拒绝"的中途失败。
- **RBAC 审计与权限最小化**：工具如 `audit2rbac` 通过分析 audit log 自动生成最小权限 Role，替代过度授权的 cluster-admin 绑定——这是租户隔离最常见的风险来源。
- **Pod Security Admission (PSA)**：K8s 1.25+ 内建的 PSA 替代了已弃用的 PodSecurityPolicy，通过 namespace 级别的 `privileged/baseline/restricted` 标签强制容器安全基线，与 RBAC 互补。
- **Impersonation + RBAC**：平台组件（如 ArgoCD、Velero）使用 `Impersonate-User` header 代替租户身份执行操作，RBAC 按被 impersonate 的用户授权——确保"平台代用户做事"时权限不超过用户自身。
- **RBAC 静态分析**：工具如 `kubeaudit`、`rbac-lookup`、`rakkess` 分析集群中所有 RoleBinding/ClusterRoleBinding，发现过度授权（如 `*` verb 绑定到 `*` resources）、僵尸绑定（绑定到已删除的 SA）和权限提升路径。
- **Audit Log 驱动合规**：K8s audit policy 按 `Metadata`/`Request`/`RequestResponse` 级别记录 API 调用，多租户场景需按 `user.username` 和 `resourceVersion` 过滤出每个租户的活动日志，满足合规审计要求。
- **ServiceAccount 自动挂载**：K8s 自动为 Pod 挂载 SA token（`automountServiceAccountToken: true`），多租户环境应设为 `false` 并显式声明需要的 SA——避免 Pod 默认获得 namespace 内的 API 访问权限。
- **break-glass 流程**：紧急排障需要临时 cluster-admin 权限时，应通过审批工单 + 时限 token（如 30 分钟过期）+ 全量 audit log 的 break-glass 机制，而非直接共享 admin kubeconfig。

## Cross-cutting Insight

RBAC 解决"身份能做什么"，多租户解决"边界在哪"。把 RBAC 当作多租户隔离的全部是危险的——RBAC 无法阻止已被授权的租户利用内核漏洞、资源耗尽或侧信道攻击邻居。真正的多租户需要"授权（RBAC）+ 准入（策略）+ 网络（NetworkPolicy）+ 计算（配额/优先级/沙箱）"四层叠加，RBAC 是其中最薄但最不可或缺的一层。从威胁模型角度看，RBAC 防御的是"未授权的 API 调用"——它是第一道闸门，决定了攻击者能否创建恶意 Pod、读取其他租户的 Secret 或修改控制面配置。一旦这道闸门被绕过（如 cluster-admin token 泄露），后续防线（NetworkPolicy 隔离、运行时安全检测）只能限制损害范围而非阻止入侵。因此生产级多租户不仅需要正确的 RBAC 策略，还需要持续的权限审计（如 `kubectl rbac-who-can`、rbac-lookup）和异常绑定检测（如新出现的 ClusterRoleBinding 告警），才能维持隔离态势。^[inferred]

## Tensions and Trade-offs

| 维度 | RBAC 授权侧重 | 多租户隔离侧重 | 结合注意事项 |
|---|---|---|---|
| 边界 | 按 resource/verb 划权 | 按 namespace/节点划界 | 需 namespace 严格映射租户 |
| 信任模型 | 假设主体已被认证 | 假设租户可能恶意 | 硬多租户需叠加沙箱 |
| 自助化 | 租户想自助管理权限 | 平台需防权限蔓延 | 用策略引擎约束可绑定的 Role |
| 爆炸半径 | cluster-admin 误授=全集群失守 | 单租户越权=单租户损失 | 默认最小权 + 审计告警 |
| 跨租户协作 | 需细粒度跨 namespace 授权 | 隔离要求限制协作 | 需显式、可审计的例外通道 |
| 资源隔离 | RBAC 不限制资源量 | 需配额 + LimitRange 兜底 | ResourceQuota 是 RBAC 的资源层补充 |
| 审计合规 | 授权日志在 audit log | 租户需独立审计轨迹 | 多租户需按 namespace 过滤审计输出 |

## Open Questions

- 在硬多租户场景下，RBAC + NetworkPolicy + 沙箱运行时的组合，其"隔离强度"如何量化评估？是否有标准化的渗透测试基线？
- 当租户需要跨 namespace 协作（如共享 CRD）时，如何在不破坏隔离的前提下提供最小授权通道？
- GitOps 模式下，租户的 RBAC 绑定经由 PR 管理，如何防止恶意 PR 自行提权？是否需要在 ArgoCD/Flux 同步前加 OPA 策略门禁？
- Hierarchical Namespace Controller 走向 GA 后，是否会取代 Capsule/Kiosk 成为多租户 namespace 管理的标准方案？

## Related

- [[概念/rbac-authorization.md|RBAC 授权]]
- [[概念/multi-tenancy-isolation.md|多租户隔离]]
- [[实体/capsule.md|Capsule]]
- [[实体/kyverno.md|Kyverno]]
- [[实体/opa.md|OPA]]
- [[概念/network-policy.md|网络策略]]
- [[综合/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]
- [[综合/argocd-gitops.md|ArgoCD × GitOps]]


<!-- risk-assessed -->
