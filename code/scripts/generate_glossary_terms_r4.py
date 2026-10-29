#!/usr/bin/env python3
"""Round 4: 高引用频率 CN 生态术语 + 方法论术语批量展开"""

import os
from pathlib import Path

BASE = Path("系统基础/topic-dictionary")

def w(cat, fn, zh, en, tags, overview, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists():
        return False
    tks = list(dict.fromkeys([zh, en, "dictionary"]))
    tk = "\n".join(f"- {k}" for k in tks)
    tg = "\n".join(f"- {t}" for t in tags)
    r = rel or "- [[系统基础/topic-dictionary/k8s-glossary|K8s Glossary]]"
    c = f"""---
title: {zh}
description: '{overview[:80]}...'
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
{tk}
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# {zh}

> **英文名**: {en}

## 概述

{overview}

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
"""
    fp.parent.mkdir(parents=True, exist_ok=True)
    fp.write_text(c)
    return True

# ─── TERMS ──────────────────────────────────────────────

TERMS = [
# === GitOps / CI-CD ===
("operations", "gitops", "GitOps", "GitOps",
 ["gitops", "cicd", "methodology"],
 "GitOps 是一种以 Git 仓库作为基础设施和应用配置的唯一真实来源（Single Source of Truth）的运维方法论。通过声明式配置和自动化拉取（Pull）模式，实现基础设施即代码（IaC）的持续交付。",
 "### 核心原则\n\n| 原则 | 说明 |\n|------|------|\n| 声明式 | 所有配置以期望状态描述 |\n| 版本化 | Git 作为配置的唯一来源 |\n| 自动拉取 | 控制器主动从 Git 拉取变更 |\n| 持续调谐 | 实际状态持续向期望状态收敛 |\n\n### Push vs Pull 模式\n\n- **Push**：CI 流水线直接 `kubectl apply`（传统方式）\n- **Pull**：集群内控制器从 Git 拉取并同步（GitOps 方式）",
 "- **Argo CD**：最流行的 GitOps 控制器，支持多集群管理。\n- **Flux**：CNCF 毕业项目，原生多租户支持。\n- **Kustomize/Helm**：GitOps 中常用的配置渲染工具。\n- **密封密钥（Sealed Secrets）**：在 Git 中安全存储加密的 Secret。\n- 支持渐进式发布（配合 Argo Rollouts/Flagger）。",
 "- 所有 K8s 资源定义存放在 Git 仓库中，通过 PR 管理变更。\n- 使用 Argo CD 或 Flux 实现自动同步。\n- 敏感信息使用 Sealed Secrets 或 External Secrets Operator。\n- 环境分离策略：按目录或按分支管理多环境配置。\n- 配置漂移检测：GitOps 控制器自动检测并修复漂移。",
 "- [OpenGitOps](https://opengitops.dev/)",
 "- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/flux|Flux]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

("operations", "flux", "Flux", "Flux",
 ["flux", "gitops", "cncf"],
 "Flux 是 CNCF 毕业项目，提供 Kubernetes 原生的 GitOps 持续交付能力。它通过自动化从 Git 仓库拉取配置并同步到集群，支持多租户、多集群和 Helm 原生集成。",
 "### Flux v2 架构\n\nFlux v2 基于 Kubernetes Controller 模式，由多个专用控制器组成：\n\n| 控制器 | 功能 |\n|--------|------|\n| Source Controller | 管理 Git/Helm/OCI 等外部源 |\n| Kustomize Controller | 渲染和部署 Kustomize 资源 |\n| Helm Controller | 管理 Helm Release 生命周期 |\n| Notification Controller | 处理告警和 Provider 集成 |\n| Image Automation | 自动更新镜像版本到 Git |",
 "- **Source 抽象**：GitRepository、HelmRepository、OCIRepository、Bucket 等。\n- **Kustomization**：声明式的 Kustomize 部署流水线。\n- **HelmRelease**：声明式的 Helm 部署，支持 valuesFrom。\n- **Image Update**：自动检测新镜像版本并提交 PR 到 Git。\n- **多租户**：通过 RBAC 和 Namespace 隔离不同团队。",
 "- 作为 Argo CD 的替代方案，特别适合多租户场景。\n- 使用 Image Automation 实现镜像版本的自动更新。\n- 配合 Kustomize 管理多环境配置差异。\n- 使用 Flux 的 Webhook 接收实现即时同步。\n- 监控 Flux 控制器的 reconciliation 状态。",
 "- [Flux Official](https://fluxcd.io/)",
 "- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/gitops|GitOps]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

("operations", "tekton", "Tekton", "Tekton",
 ["tekton", "cicd", "pipeline", "cncf"],
 "Tekton 是 CNCF 孵化项目，提供 Kubernetes 原生的 CI/CD 流水线框架。它将 CI/CD 的每一步建模为 Kubernetes CRD（Task、Pipeline），实现了真正云原生的持续集成和持续交付。",
 "### 核心 CRD\n\n| 资源 | 功能 |\n|------|------|\n| Task | 最小执行单元（一组有序的 Steps） |\n| Pipeline | 多个 Task 的编排（DAG 依赖图） |\n| TaskRun | Task 的一次执行实例 |\n| PipelineRun | Pipeline 的一次执行实例 |\n| Trigger | 外部事件触发 PipelineRun |\n\n### 与 Jenkins 对比\n\n| 特性 | Jenkins | Tekton |\n|------|---------|--------|\n| 运行环境 | 独立 VM/容器 | K8s 原生 Pod |\n| 扩展性 | 插件（Groovy） | CRD + 容器 |\n| 弹性 | Master-Agent | Serverless Pod |",
 "- **Task 共享 Workspace**：通过 PVC 在 Task 之间传递数据。\n- **Catalog**：社区贡献的预构建 Task（如 git-clone、buildpacks）。\n- **Results**：Task 输出结果供下游 Task 引用。\n- **When Expressions**：条件执行 Task。\n- **Finally**：Pipeline 结束后的清理/通知 Task。",
 "- 云原生 CI/CD 优先选择 Tekton 替代 Jenkins。\n- 使用 Tekton Catalog 复用社区 Task 减少重复开发。\n- 配合 Triggers 实现 Webhook 触发的自动构建。\n- 使用 Tekton Dashboard 或 Tekton Results 查看执行历史。\n- 为 Pipeline 设置合理的超时时间和重试策略。",
 "- [Tekton Official](https://tekton.dev/)",
 "- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/gitops|GitOps]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/workloads/job|Job]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

("operations", "velero", "Velero", "Velero",
 ["velero", "backup", "disaster-recovery", "cncf"],
 "Velero 是 CNCF 孵化项目，提供 Kubernetes 集群资源和持久卷的备份、恢复和迁移能力。它是 Kubernetes 灾备方案的标准工具，支持将备份数据存储到 S3、GCS、Azure Blob 等对象存储。",
 "### 核心概念\n\n- **Backup**：集群资源 + PV 数据的一次备份。\n- **Restore**：从备份恢复资源到集群。\n- **Schedule**：定时自动备份策略。\n- **Backup Storage Location**：备份存储目标（S3/GCS 等）。\n- **Volume Snapshot Location**：PV 快照存储目标。\n\n### 备份范围\n\n| 类型 | 说明 |\n|------|------|\n| 集群资源 | 所有 K8s API 资源（YAML） |\n| PV 数据 | 通过 CSI 快照或 Restic/Kopia |\n| 命名空间级 | 按 Namespace 选择性备份 |",
 "- **CSI 快照**：使用 CSI VolumeSnapshot 实现 PV 的即时快照。\n- **Restic/Kopia**：文件级备份，适用于不支持 CSI 快照的存储。\n- **资源过滤**：按 Label、Namespace、资源类型选择性备份。\n- **跨集群迁移**：备份源集群，恢复到目标集群。\n- 支持备份前的 Hook（如数据库 flush）。",
 "- 生产集群必须配置定期备份策略。\n- 使用 Schedule 资源定义每日/每周自动备份。\n- 定期测试 Restore 流程确保备份可用。\n- 备份数据加密存储，配置合理的保留策略。\n- 使用 Velero 进行集群迁移（on-prem → 云）。",
 "- [Velero Official](https://velero.io/)",
 "- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/storage/storage-class|Storage Class]]\n- [[系统基础/topic-dictionary/operations/upgrade|Upgrade]]\n- [[系统基础/topic-dictionary/workloads/statefulset|StatefulSet]]\n- [[系统基础/topic-dictionary/storage/rook|Rook]]"),

# === Observability ===
("observability", "loki", "Loki", "Loki",
 ["loki", "logging", "observability", "grafana"],
 "Loki 是 Grafana Labs 开源的日志聚合系统，被称为「日志界的 Prometheus」。它采用标签索引（而非全文索引）存储日志，大幅降低存储成本，是云原生日志方案的优选。",
 "### 核心架构\n\n| 组件 | 功能 |\n|------|------|\n| Distributor | 接收日志流，校验和分发 |\n| Ingester | 暂存日志并压缩写入存储 |\n| Querier | 执行 LogQL 查询 |\n| Query Frontend | 查询缓存和分片 |\n| Compactor | 合并和压缩索引块 |\n\n### 与 ELK 对比\n\n| 特性 | ELK | Loki |\n|------|-----|------|\n| 索引方式 | 全文索引 | 标签索引 |\n| 存储成本 | 高 | 低（10-20x） |\n| 查询语言 | Kibana Query | LogQL |\n| 适用规模 | 大规模全文检索 | 标签驱动的日志查询 |",
 "- **LogQL**：类 PromQL 的日志查询语言，支持标签过滤和日志解析。\n- **多租户**：通过 `X-Scope-OrgID` Header 隔离租户。\n- **对象存储**：日志块存储在 S3/GCS/MinIO。\n- **Promtail/Alloy**：日志采集 Agent（类似 Fluentd）。\n- 与 Grafana 深度集成，Dashboard 中联合查询 Metrics + Logs。",
 "- 云原生日志方案优先选择 Loki 替代 ELK。\n- 使用 Kubernetes 标签（pod、namespace、container）作为 Loki 标签。\n- 避免高基数标签（如 request_id），使用 LogQL 过滤。\n- 配置日志保留策略（retention）控制存储成本。\n- 配合 Promtail 或 Grafana Alloy 采集容器日志。",
 "- [Loki Official](https://grafana.com/oss/loki/)",
 "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/logging|Logging]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/observability/jaeger|Jaeger]]"),

("observability", "promql", "PromQL", "PromQL",
 ["promql", "prometheus", "observability", "query-language"],
 "PromQL（Prometheus Query Language）是 Prometheus 监控系统内置的查询语言，用于实时查询和分析时间序列数据。它是云原生可观测性领域最重要的技能之一。",
 "### 核心语法\n\n```\n# 瞬时向量（当前值）\nhttp_requests_total{method=\"GET\"}\n\n# 范围向量（过去5分钟）\nhttp_requests_total{method=\"GET\"}[5m]\n\n# 函数\nrate(http_requests_total[5m])     # 每秒速率\nsum(rate(http_requests_total[5m])) by (service)  # 按服务聚合\nhistogram_quantile(0.99, rate(duration_bucket[5m]))  # P99 延迟\n```\n\n### 常用函数\n\n| 函数 | 用途 |\n|------|------|\n| `rate()` | 计数器每秒增长率 |\n| `increase()` | 时间段内的增量 |\n| `histogram_quantile()` | 分位数计算 |\n| `label_replace()` | 标签改写 |",
 "- **瞬时向量 vs 范围向量**：`metric` 返回最新值，`metric[5m]` 返回时间范围。\n- **聚合运算符**：`sum`、`avg`、`max`、`min`、`count`、`topk` 等。\n- **二元运算符**：支持向量之间的加减乘除和匹配。\n- **子查询**：`rate(metric[5m])[30m:1m]` 嵌套查询。\n- Recording Rules 预计算复杂查询减少查询延迟。",
 "- 掌握 PromQL 是 SRE/运维工程师的必备技能。\n- 使用 `rate()` 而非 `irate()` 用于告警（更平滑）。\n- 配置 Recording Rules 预计算常用的高开销查询。\n- 使用 Grafana 变量实现动态 PromQL 查询。\n- 了解 PromQL 的 Staleness 机制（5 分钟过期标记）。",
 "- [PromQL Reference](https://prometheus.io/docs/prometheus/latest/querying/basics/)",
 "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/alertmanager|Alertmanager]]\n- [[系统基础/topic-dictionary/observability/thanos|Thanos]]\n- [[系统基础/topic-dictionary/observability/metrics-server|Metrics Server]]"),

# === Security ===
("security", "opa", "Open Policy Agent", "OPA (Open Policy Agent)",
 ["opa", "policy", "security", "cncf"],
 "Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraform 等场景中执行统一的策略决策。",
 "### 核心概念\n\n- **Rego**：OPA 的策略编写语言，声明式、逻辑编程风格。\n- **Policy**：定义允许/拒绝条件的规则集合。\n- **Input**：请求上下文（JSON 格式）。\n- **Decision**：OPA 返回的 allow/deny 结果。\n\n```rego\npackage kubernetes.admission\n\ndeny[msg] {\n  input.request.kind.kind == \"Pod\"\n  not input.request.object.spec.containers[_].securityContext.runAsNonRoot\n  msg := \"Pod must set runAsNonRoot=true\"\n}\n```",
 "- **Gatekeeper**：OPA 的 Kubernetes 原生实现，通过 CRD 管理策略。\n- **ConstraintTemplate**：参数化的策略模板。\n- **Audit**：定期审计已有资源是否违反策略。\n- **Mutation**：自动修正不符合策略的资源。\n- **外部数据**：引用 ConfigMap 等外部数据辅助决策。",
 "- 使用 OPA Gatekeeper 替代 PSP 实现 Pod 安全策略。\n- 定义约束：禁止 latest 标签镜像、要求 resource limits 等。\n- 使用 ConstraintTemplate 构建团队可复用的策略库。\n- 配合 CI/CD 在部署前进行策略检查（dry-run）。\n- 启用 Audit 功能定期扫描集群中的违规资源。",
 "- [OPA Official](https://www.openpolicyagent.org/)",
 "- [[系统基础/topic-dictionary/security/kyverno|Kyverno]]\n- [[系统基础/topic-dictionary/security/admission-controller|Admission Controller]]\n- [[系统基础/topic-dictionary/security/pod-security-policy|Pod Security Policy]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/security/webhook|Webhook]]"),

("security", "kyverno", "Kyverno", "Kyverno",
 ["kyverno", "policy", "security", "cncf"],
 "Kyverno 是 CNCF 孵化项目，专为 Kubernetes 设计的策略引擎。与 OPA Gatekeeper 不同，Kyverno 使用 YAML 编写策略，无需学习新语言（如 Rego），降低了策略管理的学习曲线。",
 "### 核心概念\n\n- **ClusterPolicy**：集群范围的策略。\n- **Policy**：命名空间范围的策略。\n- **规则类型**：\n\n| 类型 | 功能 |\n|------|------|\n| Validate | 验证资源是否符合规则 |\n| Mutate | 自动修改资源 |\n| Generate | 自动生成资源 |\n| VerifyImages | 验证容器镜像签名 |\n\n### 与 OPA Gatekeeper 对比\n\n| 特性 | OPA Gatekeeper | Kyverno |\n|------|---------------|--------|\n| 策略语言 | Rego（DSL） | YAML |\n| 学习曲线 | 较高 | 较低 |\n| 变更能力 | 仅 Validate | Validate + Mutate + Generate |",
 "- **Mutate 规则**：自动注入 sidecar、添加默认 labels。\n- **Generate 规则**：自动为新命名空间创建 NetworkPolicy/ResourceQuota。\n- **Image Verify**：验证镜像的 Sigstore/Cosign 签名。\n- **Reports**：生成策略违规报告。\n- **Exceptions**：为特定资源定义策略例外。",
 "- 团队熟悉 YAML 但不想学 Rego 时选择 Kyverno。\n- 使用 Mutate 规则自动为所有 Pod 添加安全上下文。\n- 使用 Generate 规则自动为新 Namespace 创建默认策略。\n- 配合 Sigstore/Cosign 实现镜像签名验证。\n- 使用 Kyverno CLI 在 CI 中测试策略。",
 "- [Kyverno Official](https://kyverno.io/)",
 "- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/security/admission-controller|Admission Controller]]\n- [[系统基础/topic-dictionary/security/pod-security-policy|Pod Security Policy]]\n- [[系统基础/topic-dictionary/security/trivy|Trivy]]\n- [[系统基础/topic-dictionary/security/webhook|Webhook]]"),

("security", "vault", "HashiCorp Vault", "Vault",
 ["vault", "secrets-management", "security"],
 "HashiCorp Vault 是业界领先的密钥管理系统，提供密钥存储、动态凭证生成、加密服务和 PKI 证书管理。在 Kubernetes 环境中，Vault 是集中式密钥管理的标准方案。",
 "### 核心功能\n\n| 功能 | 说明 |\n|------|------|\n| Secret Engine | 密钥存储和管理（KV、数据库、PKI 等） |\n| Auth Method | 身份认证（K8s、LDAP、AppRole 等） |\n| Policy | 访问控制策略 |\n| Transit | 加密即服务（Encryption as a Service） |\n| PKI | 动态证书签发和吊销 |\n\n### K8s 集成方式\n\n- **Vault Agent Sidecar**：自动注入密钥到 Pod。\n- **Vault CSI Provider**：通过 CSI 卷挂载密钥。\n- **External Secrets Operator**：同步 Vault 密钥到 K8s Secret。",
 "- **动态凭证**：按需生成短生命周期的数据库凭证、AWS 凭证等。\n- **Kubernetes Auth**：使用 ServiceAccount Token 认证 Pod 身份。\n- **Auto-Unseal**：使用云 KMS 自动解封 Vault。\n- **审计日志**：记录所有密钥访问操作。\n- **Secret Rotation**：自动轮转数据库密码和 API 密钥。",
 "- 生产环境使用 Vault 替代 K8s Secret 管理敏感信息。\n- 启用 K8s Auth Method 实现 Pod 级别的密钥访问。\n- 使用 Vault Agent Sidecar 自动注入密钥（无需修改应用代码）。\n- 配置短期凭证（TTL < 1h）减少密钥泄露风险。\n- 启用审计日志满足合规要求。",
 "- [Vault Official](https://www.vaultproject.io/)",
 "- [[系统基础/topic-dictionary/configuration/secret|Secret]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/security/certificate-authority|Certificate Authority]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/security/service-account|Service Account]]"),

# === Networking ===
("networking", "traefik", "Traefik", "Traefik",
 ["traefik", "ingress", "reverse-proxy", "gateway-api"],
 "Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为 Kubernetes Ingress Controller 和 Gateway API 实现，以自动服务发现和实时配置更新著称。",
 "### 核心架构\n\n- **EntryPoints**：入口点（HTTP/HTTPS/TCP 端口）。\n- **Routers**：路由规则（匹配 Host/Path/Header）。\n- **Services**：后端服务（负载均衡组）。\n- **Middlewares**：请求处理链（认证、限流、重定向等）。\n- **Providers**：配置源（K8s Ingress/Gateway API/Docker 等）。\n\n### 与 Nginx Ingress 对比\n\n| 特性 | Traefik | Nginx Ingress |\n|------|---------|---------------|\n| 配置更新 | 热更新（无 reload） | reload |\n| Dashboard | 内置 | 无 |\n| Gateway API | 原生支持 | 支持 |\n| 中间件 | 丰富的 Middleware | Annotation |",
 "- **自动服务发现**：监听 K8s API 自动发现 Ingress/Gateway 资源。\n- **Middleware 链**：RateLimit、CircuitBreaker、Auth、Compress 等。\n- **Let's Encrypt**：自动签发和续期 TLS 证书。\n- **Dashboard**：内置 Web UI 查看路由和中间件状态。\n- **TCP/UDP**：支持非 HTTP 协议的流量代理。",
 "- 中小集群可选择 Traefik 替代 Nginx Ingress Controller。\n- 使用 Middleware 实现限流、认证、压缩等功能。\n- 启用自动 TLS（Let's Encrypt）简化证书管理。\n- 配合 Gateway API 实现更精细的流量管理。\n- 使用 Traefik Pilot 或 Prometheus 监控代理指标。",
 "- [Traefik Official](https://doc.traefik.io/traefik/)",
 "- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/networking/loadbalancer|LoadBalancer]]"),

# === Storage ===
("storage", "ceph", "Ceph", "Ceph",
 ["ceph", "storage", "distributed-storage"],
 "Ceph 是最广泛使用的开源分布式存储系统，提供块存储（RBD）、对象存储（RGW）和文件存储（CephFS）三种接口。通过 Rook 集成到 Kubernetes 中，是大规模集群存储的首选方案。",
 "### 核心架构\n\n| 组件 | 功能 |\n|------|------|\n| OSD | 对象存储守护进程，管理物理磁盘 |\n| MON | 集群状态监控和 CRUSH Map 维护 |\n| MDS | CephFS 元数据服务 |\n| MGR | 集群管理和 Dashboard |\n| RGW | S3/Swift 兼容的对象存储网关 |\n\n### CRUSH 算法\n\nCRUSH（Controlled Replication Under Scalable Hashing）决定数据如何分布到 OSD，无需中心化的元数据查询。",
 "- **RBD（块设备）**：Kubernetes PV 的主要来源，支持快照和克隆。\n- **CephFS（文件系统）**：支持 ReadWriteMany 的共享存储。\n- **RGW（对象网关）**：S3 兼容接口，适合备份和大数据。\n- **数据冗余**：副本（Replicated）或纠删码（Erasure Coding）。\n- **自动恢复**：OSD 故障后自动重平衡数据。",
 "- 通过 Rook-Ceph Operator 在 K8s 中部署和管理 Ceph 集群。\n- 为不同工作负载创建不同的 Pool 和 StorageClass。\n- 数据库使用 RBD（块存储）获得最佳 IOPS。\n- 共享文件存储使用 CephFS。\n- 监控 Ceph 集群健康状态：`ceph health detail`。",
 "- [Ceph Official](https://docs.ceph.com/)",
 "- [[系统基础/topic-dictionary/storage/rook|Rook]]\n- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/storage/storage-class|Storage Class]]\n- [[系统基础/topic-dictionary/storage/csi|CSI]]\n- [[系统基础/topic-dictionary/storage/volume|Volume]]"),

# === AI/ML ===
("specialized-workloads", "kubeflow", "Kubeflow", "Kubeflow",
 ["kubeflow", "ml", "ai", "cncf"],
 "Kubeflow 是 CNCF 孵化项目，为 Kubernetes 上的机器学习工作负载提供完整的工具链。它涵盖 ML Pipeline、Notebook、超参调优、模型训练和部署的全生命周期管理。",
 "### 核心组件\n\n| 组件 | 功能 |\n|------|------|\n| Kubeflow Pipelines | ML 工作流编排（基于 Argo） |\n| Notebooks | Jupyter Notebook 管理 |\n| Katib | 超参数调优和神经架构搜索 |\n| Training Operators | 分布式训练（TF/PyTorch/MXNet） |\n| KServe | 模型推理服务（独立项目） |\n\n### ML 工作流\n\n```\n数据准备 → 特征工程 → 模型训练 → 超参调优 → 模型评估 → 部署服务\n  (Pipeline)  (Notebook)  (Training)  (Katib)   (Pipeline)  (KServe)\n```",
 "- **Pipeline SDK**：Python SDK 定义 ML 工作流步骤。\n- **分布式训练**：PyTorchJob/TFJob 管理多 GPU/多节点训练。\n- **资源调度**：GPU 调度、优先级队列、资源隔离。\n- **模型注册**：版本化管理训练好的模型。\n- **Experiment Tracking**：跟踪实验参数和指标。",
 "- 需要标准化 ML 工作流时引入 Kubeflow Pipelines。\n- GPU 训练任务使用 Training Operators 管理。\n- 使用 Katib 自动化超参搜索。\n- 配合 KServe 实现模型的在线推理服务。\n- 注意 Kubeflow 的资源开销，小型团队可考虑轻量替代方案。",
 "- [Kubeflow Official](https://www.kubeflow.org/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[系统基础/topic-dictionary/workloads/job|Job]]\n- [[系统基础/topic-dictionary/scheduling/resource-request|Resource Request]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]"),

("specialized-workloads", "kserve", "KServe", "KServe",
 ["kserve", "ml", "inference", "cncf"],
 "KServe（原 KFServing）是 CNCF 孵化项目，为 Kubernetes 提供标准化的机器学习模型推理（Inference）服务。它支持自动扩缩容、金丝雀发布和多模型服务，是 ML 部署的标准方案。",
 "### 核心概念\n\n- **InferenceService**：模型服务的核心 CRD。\n- **Predictor**：模型推理器（支持 TensorFlow/PyTorch/SKLearn/XGBoost 等）。\n- **Transformer**：请求/响应的预处理/后处理。\n- **Explainer**：模型可解释性服务（Alibi/AIX）。\n\n### 特性\n\n| 特性 | 说明 |\n|------|------|\n| 自动扩缩 | 缩到零（Scale-to-Zero） |\n| 金丝雀发布 | 模型版本的渐进式切换 |\n| 多模型 | ModelMesh 支持大量模型共享资源 |\n| GPU 调度 | 自动管理 GPU 资源分配 |",
 "- **Serverless 模式**：基于 Knative，支持缩到零降低成本。\n- **RawDeployment 模式**：不依赖 Knative，适合简单场景。\n- **ModelMesh**：在少量 Pod 中加载大量模型，适合大规模模型服务。\n- **V2 协议**：标准化的推理 API（Predict/Explain）。\n- 支持 ONNX Runtime、Triton 等多种推理引擎。",
 "- ML 模型上线使用 KServe 替代自建的推理服务。\n- 配合 Kubeflow 实现训练到部署的全自动化。\n- 使用金丝雀发布逐步切换新模型版本。\n- 低成本场景启用 Scale-to-Zero。\n- 大规模模型服务使用 ModelMesh 优化资源利用。",
 "- [KServe Official](https://kserve.github.io/website/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]\n- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/specialized-workloads/keda|KEDA]]"),

("specialized-workloads", "ray", "Ray", "Ray",
 ["ray", "distributed-computing", "ai", "ml"],
 "Ray 是一个通用的分布式计算框架，擅长大规模 AI/ML 工作负载。通过 KubeRay Operator 部署到 Kubernetes 中，提供弹性 GPU 集群、分布式训练和模型服务能力，已成为 AI 基础设施的事实标准之一。",
 "### 核心架构\n\n| 组件 | 功能 |\n|------|------|\n| Ray Head | 集群管理、调度、GCS（Global Control Store） |\n| Ray Worker | 执行分布式任务的计算节点 |\n| Ray Dashboard | Web UI 监控和调试 |\n| KubeRay Operator | K8s 原生部署和管理 |\n\n### Ray 生态\n\n- **Ray Train**：分布式训练（PyTorch、TensorFlow、HuggingFace）。\n- **Ray Tune**：超参调优和实验管理。\n- **Ray Serve**：在线模型推理和组合。\n- **Ray Data**：大规模数据处理。",
 "- **弹性伸缩**：RayCluster 根据负载自动扩缩 Worker 节点。\n- **GPU 调度**：支持 GPU 亲和性和共享（fractional GPU）。\n- **Ray Job**：一次性提交和运行分布式任务。\n- **Fault Tolerance**：Worker 故障自动恢复。\n- 与 Kubernetes 生态集成（Ingress、RBAC、ResourceQuota）。",
 "- 大规模 AI 训练使用 Ray Train 替代单机训练。\n- 使用 Ray Serve 部署 ML 模型的在线推理服务。\n- 配合 KubeRay Operator 实现 Ray 集群的 K8s 原生管理。\n- 使用 Ray Autoscaler 实现按需 GPU 资源伸缩。\n- 通过 Ray Dashboard 监控任务执行和资源使用。",
 "- [Ray Official](https://docs.ray.io/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]\n- [[系统基础/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[系统基础/topic-dictionary/workloads/job|Job]]\n- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]"),

# === Runtime ===
("fundamentals", "cri-o", "CRI-O", "CRI-O",
 ["cri-o", "cri", "container-runtime", "cncf"],
 "CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，实现了 Kubernetes CRI（Container Runtime Interface）标准。它是 containerd 的主要替代方案，以最小化攻击面和资源开销著称。",
 "### 与 containerd 对比\n\n| 特性 | CRI-O | containerd |\n|------|-------|------------|\n| 定位 | 专为 K8s 设计 | 通用容器运行时 |\n| 功能范围 | 仅 CRI | CRI + 独立容器管理 |\n| 复杂度 | 更小 | 更大 |\n| OCI 兼容 | 完全 | 完全 |\n| CNCF 状态 | 未入 CNCF | Graduated |\n\n### 架构\n\n```\nkubelet → CRI → CRI-O → OCI Runtime (runc/crun)\n                    ↓\n              conmon (per-container monitor)\n```",
 "- **CRI 专用**：仅实现 Kubernetes CRI，不暴露额外 API。\n- **conmon**：每个容器的监控进程，收集退出码和资源使用。\n- **支持多种 OCI 运行时**：runc（标准）、crun（C 实现，更快）、kata（VM 隔离）。\n- 配置文件：`/etc/crio/crio.conf`。\n- 与 kubelet 版本严格对应。",
 "- 追求最小攻击面的安全敏感环境优先选择 CRI-O。\n- 使用 crun 替代 runc 提升容器启动速度。\n- 配置镜像 mirror 加速拉取。\n- 监控 CRI-O 的 `crio_operations_latency` 指标。\n- 确保 CRI-O 版本与 Kubernetes 版本匹配。",
 "- [CRI-O Official](https://cri-o.io/)",
 "- [[系统基础/topic-dictionary/fundamentals/cri|CRI]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|Containerd]]\n- [[系统基础/topic-dictionary/fundamentals/kubelet|Kubelet]]\n- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]\n- [[系统基础/topic-dictionary/fundamentals/container|Container]]"),

# === Platform ===
("platform-engineering", "crossplane", "Crossplane", "Crossplane",
 ["crossplane", "iac", "platform-engineering", "cncf"],
 "Crossplane 是 CNCF 孵化项目，将 Kubernetes 的控制循环扩展到基础设施管理领域。它使用 K8s CRD 声明式管理云资源（AWS/Azure/GCP），实现了基础设施即代码（IaC）的 Kubernetes 原生化。",
 "### 核心概念\n\n- **Provider**：云厂商的 CRD 扩展（AWS/Azure/GCP 等）。\n- **Managed Resource (MR)**：单个云资源的 K8s 表示（如 RDS、S3）。\n- **Composite Resource (XR)**：组合多个 MR 的抽象层。\n- **Composition**：定义 XR 如何映射到具体的 MR。\n- **Claim**：命名空间级别的资源请求（XR 的简化接口）。\n\n### 与 Terraform 对比\n\n| 特性 | Terraform | Crossplane |\n|------|-----------|------------|\n| 模型 | Plan + Apply | 持续调谐 |\n| 状态管理 | tfstate 文件 | K8s etcd |\n| 漂移修复 | 手动 terraform apply | 自动 |\n| 管理界面 | CLI | K8s API |",
 "- **持续调谐**：Controller 持续将云资源推向期望状态。\n- **Composition 抽象**：团队通过 Claim 请求资源，无需了解底层细节。\n- **跨云管理**：同一套 API 管理 AWS/Azure/GCP 资源。\n- **Provider Config**：管理云凭证和连接配置。\n- **Observe-Only**：导入已有云资源到 Crossplane 管理。",
 "- 平台团队使用 Crossplane 构建自助式基础设施平台。\n- 定义 Composition 让开发者通过 Claim 请求数据库/存储/网络。\n- 配合 Argo CD 实现应用 + 基础设施的统一 GitOps。\n- 从 Terraform 迁移时使用 `provider-terraform` 桥接。\n- 使用 Crossplane 的 Drift Detection 自动修复配置漂移。",
 "- [Crossplane Official](https://www.crossplane.io/)",
 "- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/gitops|GitOps]]\n- [[系统基础/topic-dictionary/platform-engineering/custom-resource|Custom Resource]]\n- [[系统基础/topic-dictionary/platform-engineering/manifest|Manifest]]"),

# === Autoscaling ===
("scheduling", "keda", "KEDA", "KEDA (Kubernetes Event-Driven Autoscaling)",
 ["keda", "autoscaling", "cncf"],
 "KEDA（Kubernetes Event-Driven Autoscaling）是 CNCF 毕业项目，为 Kubernetes 工作负载提供基于事件驱动的自动扩缩容能力。它扩展了 HPA，支持 Kafka、RabbitMQ、Prometheus 等 50+ 种外部事件源。",
 "### 核心架构\n\n- **ScaledObject**：定义扩缩目标和触发器。\n- **ScaledJob**：为 Job/CronJob 定义事件驱动的批量处理。\n- **Scaler**：事件源适配器（Kafka/Prometheus/SQL 等）。\n- **Metrics Adapter**：向 K8s HPA 暴露外部指标。\n\n### 与 HPA 对比\n\n| 特性 | HPA | KEDA |\n|------|-----|------|\n| 指标源 | CPU/Memory/Custom | 50+ 外部事件源 |\n| 缩到零 | 不支持（除 Custom） | 支持 |\n| 事件驱动 | 间接 | 原生 |",
 "- **Scale-to-Zero**：无事件时将 Pod 缩到 0，节省资源。\n- **丰富的 Scaler**：Kafka lag、Prometheus 指标、数据库队列长度等。\n- **ScaledJob**：按消息队列积压量批量创建 Job 消费者。\n- **Fallback**：Scaler 故障时的降级策略。\n- 兼容标准 HPA 的 min/max/desired 语义。",
 "- 消费者类工作负载（消息队列处理）使用 KEDA 替代 HPA。\n- 配置 Kafka lag 触发器实现自动消费扩缩容。\n- 使用 Scale-to-Zero 降低非高峰时段的资源成本。\n- 配合 ScaledJob 处理批量异步任务。\n- 设置合理的 cooldownPeriod 避免频繁扩缩。",
 "- [KEDA Official](https://keda.sh/)",
 "- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/scheduling/vpa|VPA]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/workloads/job|Job]]"),

# === Serverless ===
("specialized-workloads", "knative", "Knative", "Knative",
 ["knative", "serverless", "cncf"],
 "Knative 是 CNCF 孵化项目，为 Kubernetes 提供 Serverless 能力。它包含 Serving（自动扩缩容 + 缩到零）和 Eventing（事件驱动架构）两大模块，让开发者无需管理基础设施即可运行应用。",
 "### 核心组件\n\n| 组件 | 功能 |\n|------|------|\n| Serving | HTTP 请求驱动的自动扩缩容 |\n| Eventing | 事件生产和消费的标准化 |\n| Revision | 不可变的配置快照（类似 ReplicaSet） |\n| Route | 流量路由到不同 Revision |\n\n### Scale-to-Zero\n\n```\n请求到达 → Activator 拦截 → 扩容 Pod → 流量转发 → 空闲超时 → 缩到零\n```",
 "- **Revision 管理**：每次配置变更自动创建新 Revision。\n- **流量拆分**：Route 支持按比例分配流量到多个 Revision（金丝雀）。\n- **Concurrency**：控制每个 Pod 的并发请求数。\n- **Eventing Broker**：标准化的事件发布和订阅（CloudEvents）。\n- **Trigger**：基于事件属性过滤并路由到 Knative Service。",
 "- 轻量级 HTTP 服务使用 Knative Serving 部署（缩到零节省成本）。\n- 使用 Revision 流量拆分实现金丝雀发布。\n- 配合 KServe 部署 ML 模型推理服务。\n- 使用 Eventing 构建事件驱动的微服务架构。\n- 设置合理的 `minScale` 避免冷启动延迟。",
 "- [Knative Official](https://knative.dev/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[系统基础/topic-dictionary/specialized-workloads/keda|KEDA]]\n- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]"),

# === Dev/Local ===
("tooling", "minikube", "Minikube", "Minikube",
 ["minikube", "local-development", "tooling"],
 "Minikube 是本地运行单节点 Kubernetes 集群的工具，支持 Docker、HyperKit、VirtualBox 等多种驱动。它是 K8s 学习、开发和测试的标准工具，可在个人电脑上快速启动完整的 K8s 环境。",
 "### 支持的驱动\n\n| 驱动 | 平台 | 说明 |\n|------|------|------|\n| Docker | macOS/Linux/Windows | 推荐，使用 Docker 容器模拟节点 |\n| HyperKit | macOS | 轻量级 VM |\n| Hyper-V | Windows | Windows 原生虚拟化 |\n| KVM2 | Linux | Linux 原生虚拟化 |\n\n### 常用命令\n\n```bash\nminikube start                    # 启动集群\nminikube start --cpus=4 --memory=8192  # 自定义资源\nminikube dashboard                # 打开 Web UI\nminikube addons enable ingress    # 启用插件\nminikube tunnel                   # 暴露 LoadBalancer Service\n```",
 "- **Addons**：一键启用 Ingress、Metrics Server、Dashboard 等。\n- **Multi-Node**：`--nodes=3` 模拟多节点集群。\n- **Mount**：将本地目录挂载到集群中。\n- **Registry**：内置私有镜像仓库。\n- **Profile**：管理多个 Minikube 集群实例。",
 "- K8s 新人学习使用 Minikube 快速搭建本地环境。\n- 开发调试使用 `minikube tunnel` 测试 LoadBalancer 服务。\n- CI/CD 流水线中使用 Minikube 运行集成测试。\n- 使用 Addons 快速启用 Ingress 和 Metrics Server。\n- 考虑使用 Kind 作为更轻量的替代方案。",
 "- [Minikube Official](https://minikube.sigs.k8s.io/)",
 "- [[系统基础/topic-dictionary/tooling/kubectl|Kubectl]]\n- [[系统基础/topic-dictionary/tooling/kubeadm|Kubeadm]]\n- [[系统基础/topic-dictionary/fundamentals/cluster|Cluster]]\n- [[系统基础/topic-dictionary/fundamentals/node|Node]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]"),

# === Edge ===
("platform-engineering", "kubeedge", "KubeEdge", "KubeEdge",
 ["kubeedge", "edge-computing", "cncf", "iot"],
 "KubeEdge 是 CNCF 孵化项目，将 Kubernetes 的能力扩展到边缘计算场景。它在云边之间建立安全通信通道，让边缘节点可以离线自治运行，适合 IoT、CDN、零售等边缘场景。",
 "### 核心架构\n\n| 组件 | 位置 | 功能 |\n|------|------|------|\n| CloudCore | 云端（K8s 集群） | 管理边缘节点和下发配置 |\n| EdgeCore | 边缘节点 | 运行 Pod、设备管理、离线自治 |\n| EdgeMesh | 边缘 | 边缘节点间的服务网格 |\n| Device Controller | 边缘 | IoT 设备管理 |\n\n### 云边协同\n\n- **配置下发**：云端创建资源，自动同步到边缘。\n- **状态上报**：边缘节点状态异步上报到云端。\n- **离线自治**：边缘节点断网后继续运行，恢复后自动同步。",
 "- **离线自治**：边缘节点网络中断后仍可运行工作负载。\n- **轻量级**：EdgeCore 资源占用极小（适合 ARM 设备）。\n- **设备管理**：通过 Device CRD 管理 IoT 设备（MQTT/Modbus）。\n- **EdgeMesh**：边缘节点间的服务发现和负载均衡。\n- 支持 ARM64 架构。",
 "- IoT/边缘场景使用 KubeEdge 将 K8s 能力下沉到边缘。\n- 利用离线自治能力应对不稳定的边缘网络。\n- 使用 Device Controller 统一管理 IoT 设备。\n- 边缘节点优先部署 DaemonSet 类型的监控和日志 Agent。\n- 合理规划云边网络带宽，避免大量资源同步。",
 "- [KubeEdge Official](https://kubeedge.io/)",
 "- [[系统基础/topic-dictionary/fundamentals/node|Node]]\n- [[系统基础/topic-dictionary/fundamentals/cluster|Cluster]]\n- [[系统基础/topic-dictionary/workloads/daemonset|DaemonSet]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

# === Container Registry ===
("tooling", "harbor", "Harbor", "Harbor",
 ["harbor", "container-registry", "security", "cncf"],
 "Harbor 是 CNCF 毕业项目，提供企业级容器镜像和 Helm Chart 的托管、扫描和分发服务。它内置漏洞扫描（Trivy）、镜像签名（Notary）、RBAC 和复制策略，是私有容器仓库的首选方案。",
 "### 核心功能\n\n| 功能 | 说明 |\n|------|------|\n| Image Repository | 容器镜像托管 |\n| Vulnerability Scanning | 自动 Trivy/Clair 扫描 |\n| Image Signing | Notary/Cosign 签名验证 |\n| Replication | 跨地域镜像复制策略 |\n| RBAC | 项目级别的访问控制 |\n| Webhook | 镜像推送/拉取事件通知 |\n\n### 与 Docker Hub 对比\n\n| 特性 | Harbor | Docker Hub |\n|------|--------|------------|\n| 部署方式 | 自建/私有 | 公有云 |\n| 漏洞扫描 | 内置 | 无 |\n| 镜像签名 | 内置 | 无 |\n| 复制策略 | 灵活 | 无 |",
 "- **Project**：Harbor 的逻辑隔离单元（类似 K8s Namespace）。\n- **Tag Retention**：自动清理过期或多余的镜像 Tag。\n- **Proxy Cache**：代理缓存公共 Registry 加速拉取。\n- **P2P 分发**：通过 Dragonfly 实现高效镜像分发。\n- 支持 OIDC/LDAP/AD 认证集成。",
 "- 企业环境部署 Harbor 作为私有容器镜像仓库。\n- 配置自动漏洞扫描，阻止高风险镜像部署。\n- 使用复制策略同步镜像到多个数据中心。\n- 启用镜像签名验证确保部署的镜像未被篡改。\n- 配置 Tag Retention 策略自动清理过期镜像。",
 "- [Harbor Official](https://goharbor.io/)",
 "- [[系统基础/topic-dictionary/security/trivy|Trivy]]\n- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

# === Observability: Tempo ===
("observability", "tempo", "Tempo", "Tempo",
 ["tempo", "tracing", "observability", "grafana"],
 "Grafana Tempo 是 Grafana Labs 开源的大规模分布式追踪后端，专为对象存储设计。它以低成本存储追踪数据，与 Grafana 和 Loki 深度集成，是 Prometheus + Loki + Tempo 可观测性三件套的追踪组件。",
 "### 核心架构\n\n| 组件 | 功能 |\n|------|------|\n| Distributor | 接收和分发 span 数据 |\n| Ingester | 缓冲并写入对象存储 |\n| Compactor | 合并和压缩 trace 块 |\n| Querier | 按 TraceID 查询 |\n| Query-Frontend | 查询加速和缓存 |\n\n### 设计理念\n\n- **仅按 TraceID 索引**：不做全文索引，极大降低存储成本。\n- **对象存储原生**：数据存储在 S3/GCS/MinIO。\n- **与 Grafana 集成**：在 Grafana 中联合查询 Metrics + Logs + Traces。",
 "- **OTLP 原生**：直接接收 OpenTelemetry 协议数据。\n- **低成本**：存储成本比 Jaeger（ES 后端）低 5-10 倍。\n- **Metrics-from-Traces**：从 span 数据自动生成指标。\n- **TraceQL**：结构化查询语言（类似 LogQL）。\n- 支持多租户隔离。",
 "- Grafana 生态用户选择 Tempo 替代 Jaeger 存储追踪数据。\n- 配合 OpenTelemetry Collector 采集和路由 span 数据。\n- 在 Grafana 中实现 Metrics → Logs → Traces 的关联查询。\n- 使用 TraceQL 进行高级追踪数据查询。\n- 配置合理的采样率控制存储成本。",
 "- [Tempo Official](https://grafana.com/oss/tempo/)",
 "- [[系统基础/topic-dictionary/observability/jaeger|Jaeger]]\n- [[系统基础/topic-dictionary/observability/loki|Loki]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]"),
]

# ─── MAIN ──────────────────────────────────────────────
def main():
    created, skipped = [], []
    for t in TERMS:
        cat, fn, zh, en, tags, ov, core, mech, use, refs, rel = t
        ok = w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel)
        (created if ok else skipped).append(f"{cat}/{fn}.md")
    print(f"新创建: {len(created)}")
    for f in created: print(f"  + {f}")
    if skipped:
        print(f"跳过: {len(skipped)}")
        for f in skipped: print(f"  ~ {f}")

if __name__ == '__main__':
    main()
