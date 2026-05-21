---
title: Helm 速查卡
description: Helm 3.x 包管理器快速参考, 覆盖 Chart 开发、仓库管理、发布回滚、安全审计
category: cheatsheet
tags:
- helm
- k8s
- package-manager
- chart
- cheatsheet
- quick-reference
- prometheus
- argocd
- postgresql
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- DevOps 工程师
estimated_read_time: 15min
intent_queries:
- Helm 常用命令速查
- Helm Chart 开发模板语法
- Helm 回滚和版本管理
- Helm 安全最佳实践
- Helm 仓库管理命令
trigger_keywords:
- Helm
- Chart
- release
- values
- template
- rollback
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
---

# Helm 生产环境速查卡

> **适用版本**: Helm 3.12 - 3.15 | **最后更新**: 2026-05

---

## 安装

```bash
# macOS
brew install helm

# Linux
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# 验证
helm version
```

---

## 仓库管理

```bash
# 添加常用仓库
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo add jetstack https://charts.jetstack.io

# 更新仓库
helm repo update

# 搜索
helm search repo nginx
helm search hub prometheus  # 搜索 Artifact Hub
```

---

## 安装/升级/回滚

```bash
# 安装
helm install my-release bitnami/nginx -n my-ns --create-ns

# 自定义 values
helm install my-release bitnami/nginx -f values.yaml
helm install my-release bitnami/nginx --set replicaCount=3

# 干运行 (预览)
helm install my-release bitnami/nginx --dry-run --debug

# 升级
helm upgrade my-release bitnami/nginx -f values.yaml

# 升级或安装 (幂等)
helm upgrade --install my-release bitnami/nginx -f values.yaml

# 回滚
helm history my-release        # 查看历史
helm rollback my-release 1     # 回滚到版本 1

# 卸载
helm uninstall my-release -n my-ns
```

---

## 查询与调试

```bash
# 列出所有 release
helm list -A

# 查看 release 状态
helm status my-release

# 查看 release values (合并后)
helm get values my-release

# 查看用户自定义 values
helm get values my-release --all

# 查看 manifest (渲染后的 YAML)
helm get manifest my-release

# 查看 notes
helm get notes my-release

# 查看 hooks
helm get hooks my-release
```

---

## Chart 开发

```bash
# 创建新 Chart
helm create my-chart

# Lint 检查
helm lint my-chart/

# 模板渲染 (调试)
helm template my-chart/ -f values.yaml
helm template my-chart/ --set image.tag=v2.0

# 打包
helm package my-chart/

# 推送到 OCI 仓库
helm push my-chart-1.0.0.tgz oci://registry.example.com/charts
```

### Chart.yaml 关键字段

```yaml
apiVersion: v2
name: my-app
version: 1.0.0        # Chart 版本 (语义化)
appVersion: "2.0.0"    # 应用版本
description: "My application Helm chart"
type: application      # application | library
dependencies:
  - name: postgresql
    version: "12.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
```

### 模板语法速查

```yaml
# 值引用
{{ .Values.replicaCount }}
{{ .Release.Name }}
{{ .Chart.AppVersion }}

# 条件
{{- if .Values.ingress.enabled }}
...
{{- end }}

# 循环
{{- range .Values.env }}
- name: {{ .name }}
  value: {{ .value | quote }}
{{- end }}

# 定义命名模板 (helpers.tpl)
{{- define "mychart.fullname" -}}
{{ .Release.Name }}-{{ .Chart.Name }}
{{- end }}

# 必填值校验
{{ required "A valid .Values.db.host is required" .Values.db.host }}

# 默认值
{{ .Values.image.tag | default .Chart.AppVersion }}
```

---

## 生产最佳实践

| 实践 | 说明 |
|------|------|
| 版本锁定 | `Chart.lock` 提交到 Git, 避免依赖漂移 |
| values 分环境 | base-values.yaml + env-values.yaml 分层 |
| dry-run 先行 | 生产环境升级前必须 `--dry-run --debug` |
| 回滚策略 | 保留最近 10 个 revision: `--history-max 10` |
| 安全审计 | `helm lint` + `kubeval` + `conftest` |
| OCI 仓库 | Helm 3.8+ 推荐 OCI 仓库替代传统 HTTP 仓库 |
| RBAC | 限制 Helm serviceaccount 的 namespace 权限 |
| Secrets | 使用 `helm-secrets` 插件加密敏感 values |

---

## 常见问题

| 问题 | 排查命令 |
|------|----------|
| 安装卡住 | `helm install --timeout 5m --wait` |
| 模板渲染失败 | `helm template --debug` 查看错误 |
| release 状态异常 | `helm history my-release` + `helm rollback` |
| 资源冲突 | `kubectl get events -n my-ns --sort-by='.lastTimestamp'` |
| 卸载残留 | `helm uninstall --no-hooks` 或手动清理 CRD |

## Related

- [[domain-19-landscape-references/98-merged-indexes/README-from-domain-19-landscape-references|Domain-34: CNCF Landscape 开源项目]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/infrastructure-as-code|Infrastructure as Code]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.1|helm v3.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.1|helm v2.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.2|helm v3.2 Release Notes]]
