---
title: Helm 速查卡
description: Helm 3.x 包管理器快速参考, 覆盖 Chart 开发、仓库管理、发布回滚、安全审计
summary: brew install helm
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[Helm|Helm]] 生产环境速查卡

> **适用版本**: Helm 3.12 - 3.15 | **最后更新**: 2026-05

---

## 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# macOS
brew install helm

# Linux
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# 验证
helm version
```
---

## 仓库管理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `helm upgrade/install`：部署/升级 release

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
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
helm uninstall my-release -n my-ns  # ⚠️ 删除 release 及关联资源
```
---

## 查询与调试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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
| [[Secrets|Secrets]] | 使用 `helm-secrets` 插件加密敏感 values |

---

## 常见问题

| 问题 | 排查命令 |
|------|----------|
| 安装卡住 | `helm install --timeout 5m --wait` |
| 模板渲染失败 | `helm template --debug` 查看错误 |
| release 状态异常 | `helm history my-release` + `helm rollback` |
| 资源冲突 | `kubectl get events -n my-ns --sort-by='.lastTimestamp'` |
| 卸载残留 | `helm uninstall --no-hooks` 或手动清理 CRD |
| 依赖下载失败 | `helm dependency update` + 检查仓库连通性 |
| Values 不生效 | `helm get values <release>` 确认实际值 |
| Hook 失败 | `kubectl get pods -l app.kubernetes.io/managed-by=Helm` |

## 高级 Chart 开发

### 模板函数速查

```yaml
# 字符串操作
{{ .Values.name | upper }}
{{ .Values.name | lower }}
{{ .Values.name | title }}
{{ .Values.name | trunc 63 }}
{{ .Values.name | trimSuffix "-" }}
{{ printf "%s-%s" .Release.Name .Chart.Name }}
{{ .Values.name | default "myapp" }}
{{ .Values.name | quote }}
{{ .Values.name | b64enc }}

# 条件判断
{{- if .Values.ingress.enabled }}
...
{{- else if .Values.gateway.enabled }}
...
{{- else }}
...
{{- end }}

# 循环
{{- range $key, $value := .Values.env }}
- name: {{ $key }}
  value: {{ $value | quote }}
{{- end }}

{{- range .Values.containers }}
- name: {{ .name }}
  image: {{ .image }}
{{- end }}

# 包含与模板
{{ include "mychart.fullname" . }}
{{- define "mychart.labels" -}}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}

# 类型转换
{{ .Values.replicas | int }}
{{ .Values.enabled | toString }}
{{ .Values.config | toYaml | nindent 4 }}
{{ .Values.data | toJson }}
```

### _helpers.tpl 常用模板

```yaml
{{/* 生成完整名称 */}}
{{- define "mychart.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}

{{/* 通用标签 */}}
{{- define "mychart.labels" -}}
helm.sh/chart: {{ .Chart.Name }}-{{ .Chart.Version }}
{{ include "mychart.selectorLabels" . }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/* 选择器标签 */}}
{{- define "mychart.selectorLabels" -}}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}
```

### Chart.yaml 完整示例

```yaml
apiVersion: v2
name: myapp
description: A Helm chart for MyApp
type: application
version: 1.2.0        # Chart 版本
appVersion: "2.5.1"   # 应用版本
kubeVersion: ">=1.25.0-0"
home: https://github.com/org/myapp
sources:
  - https://github.com/org/myapp
maintainers:
  - name: Team
    email: team@example.com
dependencies:
  - name: postgresql
    version: "15.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
  - name: redis
    version: "18.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: redis.enabled
```

## Helm 插件

```bash
# 安装插件
helm plugin install https://github.com/databus23/helm-diff
helm plugin install https://github.com/jkroepke/helm-secrets
helm plugin install https://github.com/helm-unittest/helm-unittest
helm plugin list

# helm-diff: 升级前对比
helm diff upgrade myapp ./chart -f values.yaml

# helm-secrets: 加密 values
helm secrets encrypt secrets.yaml > secrets.yaml.enc
helm secrets install myapp ./chart -f secrets.yaml.enc

# helm-unittest: 单元测试
helm unittest ./mychart
```

## OCI 仓库操作

```bash
# 登录 OCI 仓库
helm registry login registry.example.com

# 推送 Chart
helm package ./mychart
helm push mychart-1.0.0.tgz oci://registry.example.com/charts

# 拉取 Chart
helm pull oci://registry.example.com/charts/mychart --version 1.0.0

# 安装
helm install myapp oci://registry.example.com/charts/mychart --version 1.0.0
```

## 生产故障排查流程

```
1. 检查 Release 状态
   helm status myapp -n production
   helm history myapp -n production

2. 查看渲染结果
   helm get manifest myapp -n production
   helm get values myapp -n production

3. 检查 K8s 资源
   kubectl get all -l app.kubernetes.io/instance=myapp -n production
   kubectl get events -n production --sort-by=.metadata.creationTimestamp

4. 模板调试
   helm template myapp ./chart -f values-prod.yaml --debug
   helm template myapp ./chart --show-only templates/deployment.yaml

5. 回滚
   helm rollback myapp <revision> -n production
   helm rollback myapp 0 -n production  # 回滚到上一版本
```

## 多环境 Values 管理

```
chart/
├── values.yaml           # 默认值
├── values-dev.yaml       # 开发环境
├── values-staging.yaml   # 预发环境
└── values-prod.yaml      # 生产环境

# 使用
helm install myapp ./chart -f values.yaml -f values-prod.yaml
helm upgrade myapp ./chart -f values-prod.yaml --set image.tag=v2.0
```

**Values 优先级**（从低到高）：
1. Chart 内 values.yaml
2. -f 指定的文件（后面的覆盖前面的）
3. --set 参数
4. --set-string / --set-json

## 版本兼容矩阵

| Helm 版本 | K8s 兼容 | 关键特性 |
|----------|----------|----------|
| 3.16 | 1.25+ | OCI GA、性能优化 |
| 3.15 | 1.25+ | --force-conflicts SSA |
| 3.14 | 1.24+ | 改进的依赖管理 |
| 3.13 | 1.24+ | 插件系统增强 |
| 3.12 | 1.23+ | 改进的 --wait 逻辑 |

## 安全检查清单

- [ ] Chart 来源可信（官方仓库/内部仓库）
- [ ] 使用 `helm lint` 检查 Chart 质量
- [ ] 生产升级前执行 `helm diff upgrade`
- [ ] 敏感值使用 helm-secrets 加密
- [ ] 限制 Helm ServiceAccount 的 RBAC 权限
- [ ] 设置 `--history-max` 避免 Secret 堆积
- [ ] 使用 `--atomic` 确保失败自动回滚
- [ ] 定期清理失败的 Release
- [ ] Chart 版本化，避免覆盖已发布版本
- [ ] 使用 `--wait` 确保资源就绪

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/k8s-advanced-ecosystem.md|[[硬件知识体系、CNCF 全景生态与 eBPF 平台工程|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]]]] — Cross-reference
- gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[实体/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[实体/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[实体/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[概念/cli-tools-evolution.md|CLI 工具演进]] — Cross-reference
- [[概念/infrastructure-as-code.md|Infrastructure as Code]] — Cross-reference
- [[概念/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[技能/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[技能/develop-crd-operator.md|Develop CRD Operator]] — Cross-reference
- [[技能/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[技能/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[技能/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[技能/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[实体/argocd.md|ArgoCD]] — Cross-reference
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]
- [[生态参考/领域索引/helm-index.md|Helm 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- helm v3.1 Release Notes
- helm v2.1 Release Notes
- helm v3.2 Release Notes

```

<!-- risk-assessed -->
