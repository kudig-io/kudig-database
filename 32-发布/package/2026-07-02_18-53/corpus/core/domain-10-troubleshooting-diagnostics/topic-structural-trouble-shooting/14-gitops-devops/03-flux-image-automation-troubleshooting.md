---
title: Flux 镜像自动化故障排查指南 [topic-structural-trouble-shooting]
description: 'title: Flux 镜像自动化故障排查指南'
summary: 'title: Flux 镜像自动化故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- prometheus
- coredns
- flux
- docker
- harbor
- opa
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Flux 镜像自动化故障排查指南 是什么
- 如何 Flux 镜像自动化故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Flux 镜像自动化故障排查指南 故障排查
- Flux 镜像自动化故障排查指南 排障步骤
trigger_keywords:
- Flux
- 镜像自动化故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: [[Flux|Flux]] 镜像自动化故障排查指南
description: '# Flux 镜像自动化故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[Prometheus|prometheus]]
- [[CoreDNS|coredns]]
- flux
- docker
- harbor
- opa
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Flux 镜像自动化故障排查指南 是什么
- 如何 Flux 镜像自动化故障排查指南
- Flux 镜像自动化故障排查指南 故障排查
- Flux 镜像自动化故障排查指南 排障步骤
trigger_keywords:
- Flux
- 镜像自动化故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Flux 镜像自动化故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Flux CD v2.2+ | **最后更新**: 2026-04 | **难度**: 中级

---

## 0. 10 分钟快速诊断

1. **ImageRepository 状态**：`flux get image repositories`，确认 `READY` 列状态。
2. **ImagePolicy 状态**：`flux get image policies`，确认策略是否正确匹配标签。
3. **ImageUpdateAutomation 状态**：`flux get image update`，确认自动化是否启用。
4. **Git 写权限**：检查 ImageUpdateAutomation 引用的 GitRepository 是否有写入权限。
5. **最近扫描日志**：查看 image-reflector-controller 日志中的扫描和策略评估结果。
6. **快速缓解**：
   - 镜像扫描失败：检查仓库认证 Secret 和 registry 可达性。
   - 策略不匹配：验证 ImagePolicy 的 semver/regex 规则。
   - 自动提交失败：确认 GitRepository 的 `secretRef` 包含写权限的凭据。
7. **证据留存**：保存 ImageRepository、ImagePolicy、ImageUpdateAutomation 的 YAML、控制器日志、Git commit 历史。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 镜像仓库扫描失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| ImageRepository 未就绪 | `auth failed` / `unauthorized` | image-reflector-controller | `flux get image repositories` |
| 仓库不可达 | `dial tcp: connect: connection refused` | image-reflector-controller | Controller 日志 |
| 扫描超时 | `scan timeout` | image-reflector-controller | Controller 日志 |
| 镜像标签解析失败 | `failed to parse image reference` | image-reflector-controller | Controller 日志 |
| 证书错误 | `x509: certificate signed by unknown authority` | image-reflector-controller | Controller 日志 |

#### 1.1.2 ImagePolicy 策略不匹配

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 策略未返回候选标签 | `no candidates found` | image-reflector-controller | Controller 日志 |
| semver 解析失败 | `invalid semver range` | image-reflector-controller | Controller 日志 |
| regex 不匹配 | `regex does not match any tags` | image-reflector-controller | Controller 日志 |
| 过滤后无可用标签 | `all tags filtered out` | image-reflector-controller | Controller 日志 |

#### 1.1.3 ImageUpdateAutomation 执行失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 自动化未运行 | `no updates made` | image-automation-controller | `flux get image update` |
| Git 提交失败 | `git push failed` | image-automation-controller | Controller 日志 |
| 写权限不足 | `authentication required` | image-automation-controller | Controller 日志 |
| 分支不存在 | `reference not found` | image-automation-controller | Controller 日志 |
| 补丁应用失败 | `failed to apply patch` | image-automation-controller | Controller 日志 |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **新版本发布但未自动更新** | 镜像仓库有新标签，但 Git 仓库未收到更新 PR | ImagePolicy 的 semver 范围排除了新版本 | 检查并放宽 semver 范围 |
| **Git 仓库提交风暴** | ImageUpdateAutomation 每分钟产生一个 commit | 扫描间隔过短，且 ImagePolicy 不稳定 | 调大扫描间隔，使用更精确的匹配规则 |
| **私有仓库认证失效** | 镜像扫描突然全部失败 | Harbor/Nexus 的机器人账户密码过期 | 更新 imagePullSecret 并重启扫描 |
| **多架构镜像导致策略混乱** | ImagePolicy 返回了错误架构的标签 | 未配置 ImageRepository 的 `exclusionList` | 排除非目标架构的标签 |

### 1.2 报错查看方式汇总

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Flux CLI 查看镜像自动化状态
flux get image all
flux get image repositories
flux get image policies
flux get image update

# 查看具体资源
flux get image repository <name> -n <namespace>
flux get image policy <name> -n <namespace>
flux get image update <name> -n <namespace>

# 查看控制器日志
kubectl logs -n flux-system deployment/image-reflector-controller --tail=200
kubectl logs -n flux-system deployment/image-automation-controller --tail=200

# 查看 ImageRepository 的扫描结果
kubectl get imagerepository -A -o json | jq '.items[] | {name: .metadata.name, ready: .status.conditions[-1].status, lastScan: .status.lastScanResult.scanTime}'

# 查看 ImagePolicy 的策略结果
kubectl get imagepolicy -A -o json | jq '.items[] | {name: .metadata.name, latestImage: .status.latestImage}'
```
---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

Flux 镜像自动化由两个控制器协同工作：

```
ImageRepository
        │
        ▼ 定期扫描
┌─────────────────────────────┐
│ image-reflector-controller  │ ──► 扫描镜像仓库，获取所有标签
└──────────────┬──────────────┘
               │
               ▼
ImagePolicy ──► 根据策略过滤标签，选出最新镜像
               │
               ▼
ImageUpdateAutomation
               │
               ▼
┌─────────────────────────────┐
│ image-automation-controller │ ──► 读取 Git 仓库，应用镜像更新
│                             │     生成 commit 并 push
└─────────────────────────────┘
```

**关键概念**：
- **ImageRepository**：定义要扫描的镜像仓库和扫描间隔
- **ImagePolicy**：定义如何从扫描到的标签中选择最新版本（semver、alphabetical、numerical）
- **ImageUpdateAutomation**：定义如何自动更新 Git 仓库中的镜像引用
- **Policy 标记**：在 Git 仓库的 YAML 中通过注释 `# {"$imagepolicy": "policy-name"}` 标记需要自动更新的字段

### 2.2 排查逻辑决策树

```
Flux 镜像自动化问题
    ├── ImageRepository 未就绪
    │   ├── 认证失败？
    │   │   ├── Secret 不存在？──► 创建正确的 imagePullSecret
    │   │   ├── Secret 类型错误？──► 使用 kubernetes.io/dockerconfigjson
    │   │   └── 凭据过期？──► 更新 Secret 中的用户名/密码
    │   ├── 网络不可达？
    │   │   ├── 镜像仓库域名解析失败？──► 检查 CoreDNS
    │   │   ├── NetworkPolicy 阻断？──► 放通 flux-system → registry
    │   │   └── 证书不被信任？──► 添加 CA 到 trust store 或配置 insecure
    │   └── 扫描间隔不合理？
    │       └── 间隔过短导致限流？──► 调大 interval
    ├── ImagePolicy 无候选
    │   ├── semver 范围不匹配？──► 检查 policy 的 semver range
    │   ├── regex 不正确？──► 测试 regex 是否匹配目标标签
    │   ├── 过滤列表排除了所有标签？──► 检查 filterTags/exclusionList
    │   └── ImageRepository 未扫描到标签？──► 检查 repository 状态
    └── ImageUpdateAutomation 失败
        ├── Git 仓库不可写？
        │   ├── 凭据无写权限？──► 使用具有 write 权限的 token
        │   ├── 分支受保护？──► 使用允许的分支或配置 bypass
        │   └── 提交签名验证失败？──► 配置 signingKey
        ├── 补丁应用失败？
        │   ├── policy marker 格式错误？──► 检查注释格式
        │   └── 目标文件路径错误？──► 检查 update.path
        └── 自动化未启用？
            └── suspend=true？──► 设置 suspend=false
```

### 2.3 详细诊断命令

#### 镜像自动化全景诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Flux 镜像自动化全景诊断脚本

echo "=== Flux 镜像自动化全景诊断 ==="

# 1. ImageRepository 状态
echo "1. ImageRepository 状态:"
flux get image repositories --all-namespaces 2>/dev/null || \
  kubectl get imagerepositories -A -o json | jq -r '
    .items[] | "  \(.metadata.namespace)/\(.metadata.name): ready=\(.status.conditions[-1].status // "unknown"), lastScan=\(.status.lastScanResult.scanTime // "never")"
  '

# 2. ImagePolicy 状态
echo ""
echo "2. ImagePolicy 状态:"
flux get image policies --all-namespaces 2>/dev/null || \
  kubectl get imagepolicies -A -o json | jq -r '
    .items[] | "  \(.metadata.namespace)/\(.metadata.name): latestImage=\(.status.latestImage // "none")"
  '

# 3. ImageUpdateAutomation 状态
echo ""
echo "3. ImageUpdateAutomation 状态:"
flux get image update --all-namespaces 2>/dev/null || \
  kubectl get imageupdateautomations -A -o json | jq -r '
    .items[] | "  \(.metadata.namespace)/\(.metadata.name): ready=\(.status.conditions[-1].status // "unknown")"
  '

# 4. 控制器错误日志
echo ""
echo "4. image-reflector-controller 错误:"
kubectl logs -n flux-system deployment/image-reflector-controller --tail=200 2>/dev/null | \
  grep -iE "error|fail|unable" | tail -10

echo ""
echo "5. image-automation-controller 错误:"
kubectl logs -n flux-system deployment/image-automation-controller --tail=200 2>/dev/null | \
  grep -iE "error|fail|unable" | tail -10

# 5. 最近的 Git 更新
echo ""
echo "6. 最近的 ImageUpdateAutomation 更新:"
kubectl get imageupdateautomations -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): lastPush=\(.status.lastPushCommitTime // "never"), lastApply=\(.status.lastApplyTime // "never")"
'
```
#### 镜像仓库扫描深度诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 镜像仓库扫描深度诊断
# 用法: ./diagnose-image-repo.sh <imagerepository-name> <namespace>

REPO_NAME=${1:-""}
NAMESPACE=${2:-"flux-system"}

if [ -z "$REPO_NAME" ]; then
  echo "用法: $0 <imagerepository-name> [namespace]"
  exit 1
fi

echo "=== ImageRepository $NAMESPACE/$REPO_NAME 深度诊断 ==="

# 1. ImageRepository 配置
echo "1. ImageRepository 配置:"
kubectl get imagerepository $REPO_NAME -n $NAMESPACE -o json | jq -r '
  {
    image: .spec.image,
    interval: .spec.interval,
    exclusionList: .spec.exclusionList,
    filterTags: .spec.filterTags
  }'

# 2. 认证 Secret
echo ""
echo "2. 认证 Secret 检查:"
SECRET_NAME=$(kubectl get imagerepository $REPO_NAME -n $NAMESPACE -o jsonpath='{.spec.secretRef.name}')
if [ -n "$SECRET_NAME" ]; then
  echo "  引用的 Secret: $SECRET_NAME"
  SECRET_TYPE=$(kubectl get secret $SECRET_NAME -n $NAMESPACE -o jsonpath='{.type}')
  echo "  Secret 类型: $SECRET_TYPE"
  if [ "$SECRET_TYPE" = "kubernetes.io/dockerconfigjson" ]; then
    echo "  Docker 配置:"
    kubectl get secret $SECRET_NAME -n $NAMESPACE -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq '.' 2>/dev/null || echo "    无法解析"
  fi
else
  echo "  未配置认证 Secret（假设为公共仓库）"
fi

# 3. 扫描结果
echo ""
echo "3. 最近扫描结果:"
kubectl get imagerepository $REPO_NAME -n $NAMESPACE -o json | jq -r '
  {
    lastScanTime: .status.lastScanResult.scanTime,
    tagCount: .status.lastScanResult.tagCount,
    cataloged: .status.lastScanResult.cataloged
  }'

# 4. 手动测试镜像可达性
echo ""
echo "4. 手动测试镜像仓库可达性:"
IMAGE=$(kubectl get imagerepository $REPO_NAME -n $NAMESPACE -o jsonpath='{.spec.image}')
REGISTRY=$(echo $IMAGE | cut -d'/' -f1)
echo "  镜像: $IMAGE"
echo "  Registry: $REGISTRY"
echo "  测试 DNS 解析:"
nslookup $REGISTRY 2>/dev/null | grep -E "Address:" | tail -1

# 5. 使用 skopeo 手动扫描（如已安装）
echo ""
echo "5. skopeo 手动扫描测试:"
if command -v skopeo &>/dev/null; then
  if [ -n "$SECRET_NAME" ]; then
    # 提取认证信息
    AUTH_FILE=$(mktemp)
    kubectl get secret $SECRET_NAME -n $NAMESPACE -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d > $AUTH_FILE
    skopeo list-tags --authfile $AUTH_FILE docker://$IMAGE 2>/dev/null | head -20
    rm -f $AUTH_FILE
  else
    skopeo list-tags docker://$IMAGE 2>/dev/null | head -20
  fi
else
  echo "  skopeo 未安装，跳过手动扫描"
fi
```
---

## 3. 解决方案与风险控制

### 3.1 镜像仓库认证修复

#### 方案一：Docker Hub / 公共仓库认证

```yaml
# Docker Hub 认证 Secret
apiVersion: v1
kind: Secret
metadata:
  name: dockerhub-auth
  namespace: flux-system
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: <base64-encoded-docker-config>
---
# 使用命令生成:
# kubectl create secret docker-registry dockerhub-auth \
#   --docker-username=myuser \
#   --docker-password=mypassword \
#   --docker-email=myemail@example.com \
#   --docker-server=https://index.docker.io/v1/ \
#   -n flux-system
---
# ImageRepository 引用 Secret
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: my-app-repo
  namespace: flux-system
spec:
  image: docker.io/myorg/my-app
  interval: 5m
  secretRef:
    name: dockerhub-auth
```

#### 方案二：Harbor / 私有仓库自签名证书

```yaml
# 配置 ImageRepository 信任私有 CA
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: harbor-repo
  namespace: flux-system
spec:
  image: harbor.example.com/project/my-app
  interval: 5m
  secretRef:
    name: harbor-auth
  certSecretRef:
    name: harbor-ca-cert  # 包含 CA 证书的 Secret
---
# CA 证书 Secret
apiVersion: v1
kind: Secret
metadata:
  name: harbor-ca-cert
  namespace: flux-system
type: Opaque
data:
  ca.crt: <base64-encoded-ca-certificate>
```

### 3.2 ImagePolicy 配置优化

```yaml
# Semver 策略示例
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: my-app-policy
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: my-app-repo
  policy:
    semver:
      range: ">=1.0.0 <2.0.0"  # 只自动更新 1.x 版本
---
# Regex 策略示例（用于非 semver 标签，如 Git commit SHA）
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: my-app-commit-policy
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: my-app-repo
  filterTags:
    pattern: '^main-[a-f0-9]+-(?PP<ts>[0-9]+)'
    extract: '$ts'
  policy:
    numerical:
      order: asc
---
# 排除特定标签
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: my-app-repo
  namespace: flux-system
spec:
  image: docker.io/myorg/my-app
  interval: 5m
  exclusionList:
    - "^.*-amd64$"      # 排除 amd64 特定标签
    - "^.*-arm64$"      # 排除 arm64 特定标签
    - "^.*-debug$"      # 排除 debug 标签
    - "^latest$"        # 排除 latest
```

### 3.3 ImageUpdateAutomation 配置

```yaml
# ImageUpdateAutomation 完整配置
apiVersion: image.toolkit.fluxcd.io/v1beta1
kind: ImageUpdateAutomation
metadata:
  name: my-app-updates
  namespace: flux-system
spec:
  interval: 5m                    # 检查更新的间隔
  sourceRef:
    kind: GitRepository
    name: my-app-repo             # 引用已配置的 GitRepository
  git:
    checkout:
      ref:
        branch: main              # 从 main 分支检出
    commit:
      author:
        name: Flux Bot
        email: flux@example.com
      messageTemplate: |
        Automated image update
        
        Images:
        {{ range .Updated.Images -}}
        - {{.}}
        {{ end }}
        
        Files:
        {{ range .Updated.Files -}}
        - {{.}}
        {{ end }}
      signingKey:
        secretRef:
          name: flux-gpg-signing-key  # GPG 签名密钥（可选）
    push:
      branch: main                  # 推送到 main 分支
      # branch: flux-image-updates  # 或推送到独立分支，通过 PR 合并
  policy:
    alphabetical:
      order: asc
---
# GitRepository 写权限配置（必须使用有写权限的 token）
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: my-app-repo
  namespace: flux-system
spec:
  interval: 1m
  url: https://github.com/myorg/my-app.git
  ref:
    branch: main
  secretRef:
    name: github-write-token      # 包含写权限 PAT 的 Secret
```

### 3.4 Git 仓库中的 Policy Marker

```yaml
# 在 Git 仓库的 Deployment YAML 中使用 policy marker
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        # {"$imagepolicy": "flux-system:my-app-policy"}
        # {"$imagepolicy": "flux-system:my-app-policy:tag"}
        image: docker.io/myorg/my-app:1.2.3
        ports:
        - containerPort: 8080
---
# Kustomize 配置中也可以使用 marker
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
images:
- name: docker.io/myorg/my-app
  # {"$imagepolicy": "flux-system:my-app-policy"}
  newTag: 1.2.3
```

### 3.5 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 修改 ImagePolicy semver 范围 | ⭐ 低 | 影响下一次自动选择的版本 | 恢复原始 range，Flux 不会自动降级 |
| 修改 ImageUpdateAutomation 推送分支 | ⭐ 低 | 影响 commit 目标分支 | 恢复原始 branch 配置 |
| 删除 ImageRepository | ⭐ 低 | 停止扫描，不影响已部署镜像 | 重新创建 ImageRepository |
| 手动回滚镜像版本 | ⭐ 低 | 应用使用旧版本镜像 | ImageUpdateAutomation 可能再次更新，需暂停 |
| 暂停 ImageUpdateAutomation | ⭐ 低 | 停止自动更新 | 设置 suspend=false 恢复 |
| 更新仓库认证 Secret | ⭐ 低 | 扫描可能短暂失败 | 无需回滚，验证新 Secret 后自动恢复 |

### 3.6 验证与监控

#### 镜像自动化健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Flux 镜像自动化健康检查脚本

REPORT_FILE="/var/log/kubernetes/flux-image-health-$(date +%Y%m%d-%H%M%S).log"

echo "=== Flux 镜像自动化健康检查 $(date) ===" | tee $REPORT_FILE

# 1. 资源状态概览
echo "1. ImageRepository 状态:" | tee -a $REPORT_FILE
kubectl get imagerepositories -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].status // "unknown") (\(.status.conditions[-1].reason // ""))"
' | tee -a $REPORT_FILE

echo "" | tee -a $REPORT_FILE
echo "2. ImagePolicy 状态:" | tee -a $REPORT_FILE
kubectl get imagepolicies -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): latest=\(.status.latestImage // "none")"
' | tee -a $REPORT_FILE

echo "" | tee -a $REPORT_FILE
echo "3. ImageUpdateAutomation 状态:" | tee -a $REPORT_FILE
kubectl get imageupdateautomations -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].status // "unknown")"
' | tee -a $REPORT_FILE

# 2. 检查是否有未就绪的 ImageRepository
FAILED_REPOS=$(kubectl get imagerepositories -A -o json 2>/dev/null | jq '[.items[] | select(.status.conditions[-1].status != "True")] | length')
echo "" | tee -a $REPORT_FILE
echo "4. 未就绪的 ImageRepository: $FAILED_REPOS" | tee -a $REPORT_FILE

echo "" | tee -a $REPORT_FILE
echo "报告已保存: $REPORT_FILE" | tee -a $REPORT_FILE
```
#### Prometheus 监控告警

```yaml
# Flux 镜像自动化监控告警
groups:
- name: flux-image-automation
  rules:
  - alert: FluxImageRepositoryNotReady
    expr: |
      gotk_reconcile_condition{kind="ImageRepository",type="Ready",status!="True"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Flux ImageRepository 未就绪"
      description: "ImageRepository {{ $labels.name }} 在 {{ $labels.namespace }} 中未就绪"

  - alert: FluxImagePolicyNoCandidate
    expr: |
      imagepolicy_status{condition="Ready",status!="True"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Flux ImagePolicy 无可用候选"
      description: "ImagePolicy {{ $labels.name }} 未找到匹配的镜像标签"

  - alert: FluxImageUpdateAutomationFailed
    expr: |
      gotk_reconcile_condition{kind="ImageUpdateAutomation",type="Ready",status!="True"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Flux ImageUpdateAutomation 失败"
      description: "ImageUpdateAutomation {{ $labels.name }} 在 {{ $labels.namespace }} 中失败"

  - alert: FluxImageUpdateNoRecentPush
    expr: |
      time() - imageupdateautomation_status_last_push_commit_time > 3600 * 24
    for: 2h
    labels:
      severity: info
    annotations:
      summary: "Flux 镜像更新长时间未推送"
      description: "ImageUpdateAutomation {{ $labels.name }} 超过 24 小时未推送更新"
```

### 3.7 最佳实践

1. **semver 范围 conservatism**：生产环境使用 `~1.2.0` 或 `>=1.2.0 <1.3.0`，避免自动更新 major 版本
2. **独立更新分支**：ImageUpdateAutomation 推送到独立分支（如 `flux-image-updates`），通过 CI + PR 合并到 main
3. **排除不稳定标签**：在 ImageRepository 中排除 `latest`、`dev`、`canary` 等非稳定标签
4. **扫描间隔调优**：生产镜像仓库扫描间隔建议 5-10 分钟，开发环境可缩短到 1 分钟
5. **GPG 签名**：为 ImageUpdateAutomation 配置 GPG 签名，确保 Git 提交可追溯
6. **多架构过滤**：使用 `exclusionList` 排除非目标架构的标签，避免策略选择错误架构
7. **监控仓库配额**：私有镜像仓库（如 Harbor）通常有项目配额，监控避免推送失败

### 典型问题案例

#### 案例一：GitHub PAT 权限不足导致提交失败

**问题描述**：ImageUpdateAutomation 状态显示 `True`，但 Git 仓库从未收到更新 commit。

**根本原因**：GitRepository 使用的 GitHub PAT 只有 `read:content` 权限，缺少 `write:content` 权限。

**解决方案**：
1. 重新生成 GitHub PAT，确保勾选 `repo` 完整权限
2. 更新 GitRepository 引用的 Secret
3. 在 GitHub 的 Fine-grained PAT 中，确保对目标仓库有 Contents 写权限

#### 案例二：ImagePolicy 选择了错误的架构标签

**问题描述**：自动更新后应用启动失败，镜像为 arm64 架构，但节点为 amd64。

**根本原因**：镜像仓库中同时存在 `1.0.0-amd64` 和 `1.0.0-arm64` 标签，ImagePolicy 的 semver 选择了字母顺序较后（或扫描顺序）的标签。

**解决方案**：
1. 在 ImageRepository 中配置 `exclusionList` 排除 `*-arm64` 标签
2. 使用统一的 `1.0.0` 多架构 manifest 标签，而非架构特定标签

#### 案例三：ImageUpdateAutomation 产生大量无意义提交

**问题描述**：Git 仓库中出现大量 "Automated image update" 提交，每次间隔仅几分钟。

**根本原因**：镜像仓库中的标签列表不稳定（如 CI 每次构建都推送带时间戳的标签），ImagePolicy 的数值策略频繁选择最新标签。

**解决方案**：
1. 将 ImageRepository 的扫描间隔从 1m 调整到 30m
2. 优化 ImagePolicy，使用更严格的 semver 范围替代数值策略
3. 在 CI 中仅推送符合发布策略的标签到生产仓库

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[domain-17-system-foundation/速查卡/git.md|git]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/01-gitops-devops-troubleshooting|01-gitops-devops-troubleshooting]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/02-tekton-troubleshooting|02-tekton-troubleshooting]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/04-backup-restore-troubleshooting|04-backup-restore-troubleshooting]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/01-gitops-devops-troubleshooting|01-gitops-devops-troubleshooting]]


<!-- risk-assessed -->
