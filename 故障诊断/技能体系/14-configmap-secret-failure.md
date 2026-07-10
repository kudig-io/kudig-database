---
title: ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting
description: '# ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting'
summary: 'ConfigMap 和 Secret 是 [[Kubernetes|Kubernetes]] 中管理应用配置和敏感数据的核心资源。配置管理问题会导致 Pod 无法启动、应用行为异常、敏感数据泄露风险等问题。'
category: configuration
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- helm
- argocd
- flux
tier: supporting
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting
  是什么
- 如何 ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting
trigger_keywords:
- configmap not found
- secret not found
- mount failed configmap
- environment variable missing
- config hot reload failed
- secret decryption error
- immutable configmap
- subpath mount
- external secrets sync failed
- vault agent inject failed
- 配置未生效
- 环境变量为空
- 配置挂载失败
- Secret 解密失败
- 配置热更新
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- gitops-basics
- etcd-basics
- policy-basics
skill_id: SKILL-14_CONFIGMAP_SECRET_FAILURE-001
skill_name: ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<!-- condition: kubectl describe pod <pod> -n <ns> | grep -E 'ConfigMap.*not found|Secret.*not found|mount failed' 显示配置挂载错误 -->

# ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting

---

## 1. 概述

ConfigMap 和 Secret 是 [[Kubernetes|Kubernetes]] 中管理应用配置和敏感数据的核心资源。配置管理问题会导致 Pod 无法启动、应用行为异常、敏感数据泄露风险等问题。随着 External Secrets Operators]] Operator、Vault Agent Injector 等外部 Secret 管理方案的普及，配置管理的复杂度和故障模式也在增加。

### 典型触发场景

1. **ConfigMap/Secret 引用错误**: Pod 引用了不存在的 ConfigMap/Secret，或 Key 名称拼写错误，导致 Pod 启动失败 (CreateContainerConfigError)
2. **配置更新不生效**: 更新了 ConfigMap/Secret 内容，但使用 SubPath 挂载的配置文件未自动更新，或应用未感知配置变化
3. **外部 Secret 同步失败**: External Secrets Operator 与 Vault/AWS Secrets Manager 等外部存储的认证失败或同步延迟
4. **Secret 加密解密异常**: KMS Provider 配置错误导致 etcd 中加密的 Secret 无法解密
5. **Immutable ConfigMap/Secret 阻止更新**: 尝试修改标记为 immutable 的配置资源被拒绝

### 前置条件

- **RBAC 权限**: 对 configmaps、secrets、pods、events 的 get/list/watch 权限；外部 Secret 诊断需要 externalsecrets、secretstores 的访问权限
- **kubectl 访问**: kubectl v1.28+ 并已配置集群访问
- **工具要求**: base64（解码 Secret 数据）、jq（可选，解析 JSON）
- **外部 Secret 环境**: 如涉及 External Secrets Operator 或 Vault，需了解其部署架构

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | Pod 启动失败，Events 显示 `configmap "xxx" not found` 或 `secret "xxx" not found` / Pod fails with configmap/secret not found | `kubectl describe pod POD -n NS` Events 部分出现 MountVolume.SetUp failed | 0.95 | ConfigMap/Secret 刚创建尚未同步到 kubelet 缓存（等待几秒重试） |
| SP-02 | Pod 启动失败，状态为 `CreateContainerConfigError` / Pod stuck in CreateContainerConfigError | `kubectl get pods -n NS` STATUS 列显示 CreateContainerConfigError | 0.90 | 容器镜像拉取问题（先排除 ImagePullBackOff） |
| SP-03 | 环境变量注入值为空，应用因缺少配置无法正常运行 / Environment variables from ConfigMap/Secret are empty | `kubectl exec POD -n NS -- env | grep KEY` 返回空或变量不存在 | 0.85 | 应用自身覆盖了环境变量；ConfigMap/Secret 中该 Key 值本身为空 |
| SP-04 | Volume 挂载的配置文件内容为空或仍是旧版本 / Mounted config file is empty or stale | `kubectl exec POD -n NS -- cat /path/to/config` 内容与 ConfigMap 不符 | 0.85 | 容器启动脚本修改了配置文件；应用自身生成了同名文件覆盖挂载 |
| SP-05 | SubPath 挂载的配置文件更新 ConfigMap 后未自动刷新 / SubPath mounted file not auto-updating | 更新 ConfigMap 后，`kubectl exec POD -- cat /path/subpath-file` 仍显示旧内容 | 0.95 | 这是 SubPath 的预期行为，非 bug，需使用其他方案实现热更新 |
| SP-06 | 尝试修改 Immutable ConfigMap/Secret 时报错 `field is immutable` / Cannot modify immutable ConfigMap/Secret | `kubectl apply/edit` 操作返回 `Invalid value: true: field is immutable` | 0.98 | 非问题，需删除重建或使用新名称 |
| SP-07 | ExternalSecret 资源 status 为 NotReady 或 SecretSyncedError / ExternalSecret sync failed | `kubectl get externalsecret -n NS` STATUS 列非 Ready；`kubectl describe externalsecret` 显示错误 | 0.90 | External Secrets Operator 正在重启或升级中 |
| SP-08 | Vault Agent sidecar 容器异常或 init container 失败 / Vault Agent injection failed | `kubectl get pods -n NS` 显示 Pod 有 vault-agent 容器处于 Error/CrashLoopBackOff | 0.85 | Vault 服务端正在维护；annotation 配置错误 |
| SP-09 | apiserver 日志显示 Secret 解密失败 (KMS provider error) / Secret decryption failed with KMS error | `kubectl logs kube-apiserver-xxx -n kube-system` 包含 `failed to decrypt` 或 KMS 相关错误 | 0.80 | KMS 服务短暂不可用（网络抖动） |
| SP-10 | 创建 ConfigMap 失败，提示超过大小限制 / ConfigMap creation fails due to size limit | `kubectl create/apply` 返回 `etcd: request is too large` 或 `ConfigMap size exceeds limit` | 0.98 | etcd 配置了更大的 max-request-bytes（非默认） |
| SP-11 | 配置更新后应用行为未变化，需重启 Pod 才生效 / Config changes not reflected without Pod restart | 更新 ConfigMap 后应用日志无变化，重启 Pod 后生效 | 0.80 | 应用不支持配置热更新；使用了环境变量方式注入（启动时固化） |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Pod 启动失败，提示找不到 ConfigMap"
- "更新了配置文件，但应用没有读取到新配置"
- "Secret 挂载失败，容器无法启动"
- "环境变量读取为空，应用报 NullPointerException"
- "External Secrets 同步状态一直是 Error"
- "Vault Agent 注入失败，Pod 一直在 Init 状态"
- "修改 ConfigMap 报错 immutable"
- "配置文件太大，创建失败"

**English ticket descriptions**:
- "Pod failed to start: configmap not found"
- "Config changes not taking effect, SubPath mount"
- "Secret mount failed, CreateContainerConfigError"
- "Environment variable from secret is empty"
- "External Secrets not syncing, stuck in NotReady"
- "Vault agent sidecar crash loop"
- "Cannot update immutable configmap"
- "ConfigMap too large, exceeds etcd limit"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| Pod 因 ImagePullBackOff 启动失败，非配置问题 | SKILL-POD-001 | 镜像拉取问题，与 ConfigMap/Secret 无关 |
| Secret 权限问题（RBAC 禁止访问）| SKILL-SEC-001 | ServiceAccount 无权读取 Secret，属于安全/权限类问题 |
| 应用配置解析错误（YAML/JSON 语法错误）| 应用层问题 | 配置内容正确挂载，但格式错误由应用负责 |
| etcd 整体不可用导致所有资源无法读取 | 控制平面问题 | etcd 集群问题，超出本 Skill 范围 |
| 网络策略阻止 Pod 访问外部 Secret 存储 | SKILL-NET-001 | NetworkPolicy 问题，非配置管理范畴 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 检查受影响 Pod 数量和状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找因配置问题无法启动的 Pod
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded | grep -E "CreateContainerConfigError|Error|Init" | wc -l
# 或更精确地检查特定 ConfigMap/Secret
CM_NAME="<configmap-name>"
kubectl get pods -A -o json | jq -r ".items[] | select(.spec.volumes[]?.configMap?.name==\"$CM_NAME\") | .metadata.namespace + \"/\" + .metadata.name"
```
> **判断规则**:
> - 受影响 Pod > 10 个 → **P1**
> - 受影响 Pod 3-10 个 → **P2**
> - 受影响 Pod 1-2 个 → **P3**

**Step T2**: 确认受影响的 Namespace 和工作负载类型
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查受影响的 namespace 分布
kubectl get pods -A --field-selector status.phase=Pending -o jsonpath='{range .items[*]}{.metadata.namespace}{"\n"}{end}' | sort | uniq -c | sort -rn | head -10
```
> **判断规则**:
> - 涉及 `kube-system` 或核心基础设施 namespace → 升级为 **P1**
> - 涉及生产关键业务 namespace → **P1**
> - 仅影响开发/测试 namespace → 保持 T1 分级

**Step T3**: 检查 ConfigMap/Secret 引用的关键程度
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看问题 Pod 的事件
POD_NAME="<pod-name>"
NS="<namespace>"
kubectl describe pod $POD_NAME -n $NS | grep -A5 "Events:"
```
> **判断规则**:
> - Events 显示 `FailedMount` + `configmap/secret not found` → 配置引用问题
> - Events 显示 `InvalidKeyRef` → Key 名称错误
> - 无明显配置相关错误 → 可能非本 Skill 范围

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| >10 个 Pod 因配置问题无法启动 **或** 涉及核心服务 ConfigMap/Secret | **P1** | 大规模配置问题，影响业务可用性 | 15min 内响应，30min 内修复 |
| 3-10 个 Pod 受影响 **或** 外部 Secret 同步异常影响多个应用 | **P2** | 中等规模影响，部分服务降级 | 30min 内响应，2h 内修复 |
| 1-2 个 Pod 受影响 **或** 配置更新不生效需人工干预 | **P3** | 小范围影响，可通过重启 Pod 临时缓解 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE**：

- **KMS Provider 完全不可用**: 所有 Secret 解密失败，导致整个集群 Secret 无法读取
- **External Secrets Operator CRD 被删除**: ExternalSecret、SecretStore 等 CRD 意外删除
- **Vault 服务端完全不可达**: 所有 Vault Agent 注入失败，影响依赖 Vault 的全部应用
- **etcd 数据加密密钥丢失**: EncryptionConfiguration 中的密钥与 etcd 中存储的不匹配
- **安全事件**: 怀疑 Secret 泄露或被恶意修改

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 基础配置检查（只读，零风险）

> **目标**: 通过 kubectl 检查 ConfigMap/Secret 存在性、Pod 引用正确性。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 检查 ConfigMap/Secret 是否存在
- **命令**:
  ```bash
  # 检查 ConfigMap
  kubectl get configmap <cm-name> -n <namespace>
  
  # 检查 Secret
  kubectl get secret <secret-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: 资源存在则显示名称、数据条目数、AGE
- **判断规则**:
  - 资源不存在 (Error from server: not found) → **RC-001**，跳转 Section 5
  - 资源存在 → 继续 D1.2 检查引用
- **版本差异**: 无

**Step D1.2**: 检查 Pod 对 ConfigMap/Secret 的引用方式
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A30 "volumes:|env:|envFrom:"
  ```
- **超时**: 10s
- **预期输出模式**: Pod spec 中的 volumes、env、envFrom 配置
- **判断规则**:
  - `configMapKeyRef` 或 `secretKeyRef` 引用了不存在的 Key → **RC-002**
  - `optional: false`（默认）且资源不存在 → Pod 启动失败
  - `optional: true` 但资源不存在 → **RC-012**（静默失败，可能导致应用异常）
  - 使用 `subPath` 挂载 → 标记，可能涉及 **RC-003**（热更新问题）
- **版本差异**: 无

**Step D1.3**: 比对 ConfigMap/Secret 的 Key 与 Pod 引用的 Key
- **命令**:
  ```bash
  # 查看 ConfigMap 的所有 Key
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'
  
  # 查看 Secret 的所有 Key
  kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'
  
  # 对比 Pod 引用的 Key
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].env[*].valueFrom.configMapKeyRef.key}' 
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].env[*].valueFrom.secretKeyRef.key}'
  ```
- **超时**: 15s
- **预期输出模式**: Key 列表
- **判断规则**:
  - Pod 引用的 Key 不在 ConfigMap/Secret 中 → **RC-002**
  - Key 名称存在但大小写不匹配 → 常见错误，确认为 **RC-002**
- **版本差异**: 无

**Step D1.4**: 检查挂载路径和文件内容
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查挂载点是否存在
  kubectl exec <pod-name> -n <namespace> -- ls -la /path/to/config
  
  # 检查文件内容（仅确认非空，不输出敏感数据）
  kubectl exec <pod-name> -n <namespace> -- wc -l /path/to/config/filename
  ```
- **超时**: 15s
- **预期输出模式**: 文件列表和行数
- **判断规则**:
  - 挂载路径不存在或为空 → Volume 挂载失败
  - 文件行数为 0 → ConfigMap 数据为空或挂载异常
  - 文件内容与 ConfigMap 不一致 → 可能是缓存问题或 SubPath 问题
- **版本差异**: 无
- **注意**: 如果 Pod 处于 Pending/Error 状态，此步骤无法执行，跳过

**Step D1.5**: 验证环境变量注入
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查特定环境变量是否存在和非空
  kubectl exec <pod-name> -n <namespace> -- printenv | grep -E "^<ENV_KEY>="
  
  # 或检查所有环境变量
  kubectl exec <pod-name> -n <namespace> -- env | wc -l
  ```
- **超时**: 10s
- **预期输出模式**: 环境变量名=值
- **判断规则**:
  - 环境变量不存在 → ConfigMap/Secret 引用失败或 optional: true 导致静默跳过
  - 环境变量存在但值为空 → ConfigMap/Secret 中该 Key 的 value 为空字符串
- **版本差异**: 无

**Step D1.6**: 查看 ConfigMap/Secret 完整数据结构
- **命令**:
  ```bash
  # 查看 ConfigMap 详情（包含 data 和 binaryData）
  kubectl get configmap <cm-name> -n <namespace> -o yaml
  
  # 查看 Secret 详情（data 为 base64 编码）
  kubectl get secret <secret-name> -n <namespace> -o yaml
  # 注意: 不要在日志中输出解码后的 Secret 值
  ```
- **超时**: 10s
- **预期输出模式**: 完整 YAML
- **判断规则**:
  - `data` 字段为空 → ConfigMap/Secret 无数据
  - `immutable: true` → **RC-004**，该资源无法修改
  - Secret 的 `type` 字段检查（如 kubernetes.io/tls、Opaque 等）
- **版本差异**: 
  - **[v1.21+]**: Immutable ConfigMaps/Secrets GA，`immutable: true` 字段可用

---

### Phase 2: 高级配置诊断（只读，零风险）

> **目标**: 深入检查 SubPath、Projected Volume、容量限制、加密配置等高级场景。
> **预计耗时**: 5-10 分钟

**Step D2.1**: SubPath 挂载行为诊断
- **命令**:
  ```bash
  # 检查 Pod 是否使用 SubPath 挂载
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*].volumeMounts[*]}{.name}: subPath={.subPath}{"\n"}{end}'
  
  # 检查 kubelet 的 ConfigMap 缓存同步时间
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.metadata.resourceVersion}'
  ```
- **超时**: 10s
- **预期输出模式**: SubPath 配置和资源版本
- **判断规则**:
  - 存在 `subPath` 配置 → **重要**: SubPath 挂载的文件**不会自动更新**，这是 Kubernetes 设计行为
  - 如需配置热更新，必须使用完整 Volume 挂载或 Reloader 等工具
- **版本差异**: 无

**Step D2.2**: 检查 Immutable 属性
- **命令**:
  ```bash
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.immutable}'
  kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.immutable}'
  ```
- **超时**: 5s
- **预期输出模式**: `true` 或空
- **判断规则**:
  - 输出 `true` → **RC-004**，资源不可修改，需删除重建
  - 输出为空 → 资源可修改
- **版本差异**:
  - **[v1.21+]**: Immutable ConfigMaps/Secrets GA

**Step D2.3**: Projected Volume 诊断
- **命令**:
  ```bash
  # 检查是否使用 Projected Volume 组合多个 ConfigMap/Secret
  kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A30 "projected:"
  ```
- **超时**: 10s
- **预期输出模式**: Projected Volume 配置
- **判断规则**:
  - Projected Volume 中任一 source 的 ConfigMap/Secret 不存在 → 挂载失败
  - 多个 source 有相同文件名 → 后者覆盖前者
- **版本差异**: 无

**Step D2.4**: 检查 ConfigMap 大小
- **命令**:
  ```bash
  # 获取 ConfigMap 大小（字节数）
  kubectl get configmap <cm-name> -n <namespace> -o json | wc -c
  
  # 或更精确地计算 data 部分大小
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.data}' | wc -c
  ```
- **超时**: 10s
- **预期输出模式**: 字节数
- **判断规则**:
  - 大小接近或超过 1MB (1048576 bytes) → **RC-005**，接近 etcd 单对象大小限制
  - 大小 < 100KB → 正常范围
  - 大小 100KB-1MB → 建议拆分或迁移到外部存储
- **版本差异**: 
  - etcd 默认 max-request-bytes 为 1.5MB，但 ConfigMap 数据推荐不超过 1MB

**Step D2.5**: 验证 Secret 数据完整性
- **命令**:
  ```bash
  # 检查 Secret 数据是否可正确 base64 解码（不输出实际值）
  kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.<key>}' | base64 -d | wc -c
  # 预期: 返回字符数，无 base64 解码错误
  ```
- **超时**: 10s
- **预期输出模式**: 解码后的字符数
- **判断规则**:
  - base64 解码失败 → Secret 数据损坏或格式错误
  - 解码成功但字符数为 0 → 该 Key 的值为空
- **版本差异**: 无

**Step D2.6**: 检查 EncryptionConfiguration（控制平面）
- **命令**:
  ```bash
  # 检查 kube-apiserver 是否启用了加密
  # 需要 SSH 到控制平面节点或检查 kube-apiserver Pod
  kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -o '\-\-encryption-provider-config[^ ]*'
  
  # 如果可以访问控制平面节点
  # cat /etc/kubernetes/encryption-config.yaml
  ```
- **超时**: 15s
- **预期输出模式**: 加密配置文件路径
- **判断规则**:
  - 存在 `--encryption-provider-config` → 集群启用了 Secret 加密
  - 配置指向的文件不存在或格式错误 → **RC-008**
- **版本差异**:
  - **[v1.29+]**: KMS v2 GA，推荐使用 KMS v2 API

**Step D2.7**: 检查 KMS Provider 状态
- **命令**:
  ```bash
  # 检查 kube-apiserver 日志中的 KMS 相关错误
  kubectl logs -n kube-system -l component=kube-apiserver --tail=100 | grep -iE "kms|encrypt|decrypt|secret"
  
  # 检查 KMS 健康状态（如果使用 kms-plugin）
  kubectl get --raw /healthz/kms-provider-0 2>/dev/null || echo "KMS healthz endpoint not available"
  ```
- **超时**: 20s
- **预期输出模式**: 日志条目或健康状态
- **判断规则**:
  - 日志包含 `failed to decrypt` → **RC-008** KMS 解密失败
  - 日志包含 `kms plugin not found` → KMS 插件未正确安装
  - 日志包含 `connection refused` → KMS 服务不可达
- **版本差异**:
  - **[v1.29+]**: KMS v2 支持 `/healthz/kms-provider-{index}` 端点

**Step D2.8**: 检查配置更新时间戳
- **命令**:
  ```bash
  # 查看 ConfigMap/Secret 的最后修改时间
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.metadata.creationTimestamp} resourceVersion={.metadata.resourceVersion}'
  
  # 对比 Pod 的启动时间
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.startTime}'
  ```
- **超时**: 10s
- **预期输出模式**: 时间戳
- **判断规则**:
  - ConfigMap 更新时间晚于 Pod 启动时间 → Pod 使用的是旧配置
  - 如果使用环境变量注入，需要重启 Pod 才能获取新值
  - 如果使用 Volume 挂载（非 SubPath），kubelet 会自动同步（默认 1 分钟内）
- **版本差异**: 无

---

### Phase 3: 外部 Secret 管理诊断（只读，零风险）

> **目标**: 诊断 External Secrets Operator、Vault Agent Injector 等外部 Secret 管理方案。
> **前提**: 集群已部署相关组件
> **预计耗时**: 5-10 分钟

**Step D3.1**: 检查 External Secrets Operator 状态
- **命令**:
  ```bash
  # 检查 ESO 控制器 Deployment
  kubectl get deploy -n external-secrets
  kubectl get pods -n external-secrets
  
  # 检查 ESO CRDs
  kubectl get crd | grep external-secrets
  ```
- **超时**: 15s
- **预期输出模式**: Deployment 和 Pod 状态
- **判断规则**:
  - Deployment 不存在 → External Secrets Operator 未安装
  - Pod 处于非 Running 状态 → ESO 控制器异常，需要先修复控制器
  - CRD 缺失 → ESO 安装不完整
- **版本差异**: 无

**Step D3.2**: 检查 ExternalSecret 资源状态
- **命令**:
  ```bash
  # 列出所有 ExternalSecret 及其状态
  kubectl get externalsecret -A
  
  # 查看特定 ExternalSecret 详情
  kubectl describe externalsecret <name> -n <namespace>
  ```
- **超时**: 15s
- **预期输出模式**: ExternalSecret 状态列表
- **判断规则**:
  - STATUS 为 `SecretSynced` → 同步成功
  - STATUS 为 `SecretSyncedError` → **RC-006** 同步失败，查看 conditions
  - STATUS 为 `NotReady` → SecretStore 连接问题或认证失败
- **版本差异**: 无

**Step D3.3**: 检查 SecretStore/ClusterSecretStore 配置
- **命令**:
  ```bash
  # 列出 SecretStore
  kubectl get secretstore -A
  kubectl get clustersecretstore
  
  # 检查 SecretStore 状态和配置
  kubectl describe secretstore <name> -n <namespace>
  ```
- **超时**: 15s
- **预期输出模式**: SecretStore 状态
- **判断规则**:
  - SecretStore 不存在但 ExternalSecret 引用它 → **RC-006**
  - SecretStore 状态 NotReady → 外部 Secret 存储连接失败
  - conditions 中有 `SecretAccessFailed` → 认证凭据错误
- **版本差异**: 无

**Step D3.4**: 检查 Vault Agent Injector 状态
- **命令**:
  ```bash
  # 检查 Vault Agent Injector Deployment
  kubectl get deploy -n vault vault-agent-injector
  kubectl get pods -n vault -l app.kubernetes.io/name=vault-agent-injector
  
  # 检查 MutatingWebhookConfiguration
  kubectl get mutatingwebhookconfiguration | grep vault
  ```
- **超时**: 15s
- **预期输出模式**: Deployment 和 Webhook 状态
- **判断规则**:
  - vault-agent-injector Deployment 不存在 → Vault 注入器未安装
  - Pod 处于非 Running 状态 → 注入器异常
  - MutatingWebhook 缺失 → 注入器未正确配置
- **版本差异**: 无

**Step D3.5**: 检查 Vault 认证状态
- **命令**:
  ```bash
  # 检查 Pod 中的 vault-agent 容器日志
  kubectl logs <pod-name> -n <namespace> -c vault-agent
  
  # 如果是 init container
  kubectl logs <pod-name> -n <namespace> -c vault-agent-init
  ```
- **超时**: 15s
- **预期输出模式**: Vault agent 日志
- **判断规则**:
  - 日志包含 `permission denied` → **RC-007** Vault 认证失败
  - 日志包含 `connection refused` → Vault 服务不可达
  - 日志包含 `secret not found` → Vault 中不存在该 Secret 路径
  - 日志包含 `token renewal` 错误 → Token 续期失败
- **版本差异**: 无

**Step D3.6**: 检查 Secret 同步延迟
- **命令**:
  ```bash
  # 检查 ExternalSecret 的 refreshInterval 和 lastSyncTime
  kubectl get externalsecret <name> -n <namespace> -o jsonpath='{.spec.refreshInterval} lastSync={.status.syncedResourceVersion}'
  
  # 检查目标 Secret 是否已创建
  kubectl get secret -n <namespace> | grep <expected-secret-name>
  ```
- **超时**: 10s
- **预期输出模式**: 刷新间隔和同步版本
- **判断规则**:
  - 目标 Secret 不存在 → ExternalSecret 尚未成功同步
  - refreshInterval 过长 → 同步延迟可能是正常行为
  - syncedResourceVersion 长时间未更新 → 同步卡住
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | 修复难度 |
|--------|------|------|---------|---------|
| RC-001 | **ConfigMap/Secret 不存在** — Pod 引用的 ConfigMap 或 Secret 资源在目标 namespace 中不存在，导致 FailedMount 或 CreateContainerConfigError | ~22% | D1.1 返回 not found；Events 显示 `configmap/secret "xxx" not found` | 🟢 |
| RC-002 | **Key 名称不匹配** — ConfigMap/Secret 存在但 Pod 引用的 Key 不在其中，常见原因是拼写错误或大小写不一致 | ~18% | D1.3 对比显示 Key 不存在；Events 显示 `key "xxx" not found` | 🟢 |
| RC-003 | **SubPath 挂载不自动更新** — 使用 SubPath 方式挂载单个文件，ConfigMap 更新后 Pod 内文件不会自动刷新（Kubernetes 设计行为） | ~12% | D2.1 确认使用 SubPath；配置更新后 exec 进 Pod 查看文件仍是旧版本 | 🟢 |
| RC-004 | **Immutable ConfigMap/Secret 阻止更新** — 资源标记为 immutable: true，任何修改尝试被 apiserver 拒绝 | ~8% | D2.2 返回 true；kubectl apply/edit 报错 `field is immutable` | 🟡 |
| RC-005 | **ConfigMap 超过 1MB 大小限制** — ConfigMap 数据超过 etcd 单对象大小限制，创建或更新失败 | ~6% | D2.4 显示大小接近/超过 1MB；etcd 日志显示 request too large | 🟡 |
| RC-006 | **External Secrets 认证失败** — ExternalSecret 无法从外部 Secret 存储（AWS SM、GCP SM、Azure KV、HashiCorp Vault）获取数据 | ~6% | D3.2/D3.3 显示 NotReady/SecretSyncedError；SecretStore conditions 显示认证错误 | 🟡 |
| RC-007 | **Vault Agent 注入异常** — Vault Agent Injector 的 sidecar 或 init container 失败，无法将 Secret 注入到 Pod | ~5% | D3.4/D3.5 显示 vault-agent 容器异常；日志包含认证或连接错误 | 🟡 |
| RC-008 | **KMS Provider 解密失败** — apiserver 无法使用 KMS 提供的密钥解密 etcd 中的 Secret 数据 | ~5% | D2.7 日志包含 decrypt 错误；所有 Secret 读取失败 | 🔴 |
| RC-009 | **应用未监听配置文件变化（需要 Reloader）** — ConfigMap 通过 Volume 挂载且已更新，但应用不监听文件变化，需要重启才能读取新配置 | ~5% | D2.8 ConfigMap 已更新；应用日志无配置变更记录；重启后生效 | 🟢 |
| RC-010 | **Namespace 间 Secret 引用限制** — Pod 尝试引用其他 namespace 的 ConfigMap/Secret，Kubernetes 不支持跨 namespace 引用 | ~4% | D1.2 显示引用的 ConfigMap/Secret 在其他 namespace | 🟢 |
| RC-011 | **配置漂移（手动修改 vs GitOps 源）** — ConfigMap/Secret 被手动修改后与 GitOps 源不一致，导致同步覆盖或冲突 | ~4% | [[实体/flux.md|Flux]] 显示 OutOfSync；kubectl get 与 Git 仓库内容不一致 | 🟡 |
| RC-012 | **Optional 引用的 ConfigMap/Secret 缺失导致静默失败** — 使用 `optional: true` 引用不存在的资源，Pod 启动成功但配置为空，应用行为异常 | ~5% | D1.2 显示 optional: true；D1.5 环境变量不存在但 Pod Running | 🟢 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 创建缺失的 ConfigMap/Secret
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认资源确实不存在
  kubectl get configmap <cm-name> -n <namespace>
  kubectl get secret <secret-name> -n <namespace>
  # 预期: Error from server (NotFound)
  
  # 确认有创建权限
  kubectl auth can-i create configmaps -n <namespace>
  kubectl auth can-i create secrets -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建 ConfigMap（从文件）
  kubectl create configmap <cm-name> -n <namespace> --from-file=<path/to/config>
  
  # 创建 ConfigMap（从 literal）
  kubectl create configmap <cm-name> -n <namespace> --from-literal=key1=value1 --from-literal=key2=value2
  
  # 创建 Secret（从文件）
  kubectl create secret generic <secret-name> -n <namespace> --from-file=<path/to/secret>
  
  # 创建 Secret（从 literal）
  kubectl create secret generic <secret-name> -n <namespace> --from-literal=username=xxx --from-literal=password=xxx
  ```
- **后置验证**:
  ```bash
  # 确认资源已创建
  kubectl get configmap <cm-name> -n <namespace>
  kubectl get secret <secret-name> -n <namespace>
  
  # 检查引用该配置的 Pod 是否恢复
  kubectl get pods -n <namespace> -l <selector>
  # 预期: Pod 状态变为 Running
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete configmap <cm-name> -n <namespace>
  kubectl delete secret <secret-name> -n <namespace>
  ```

#### REM-002: 修正 Key 名称引用
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 确认正确的 Key 名称
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'
  
  # 确认 Pod 引用的错误 Key
  kubectl get deployment <deploy-name> -n <namespace> -o yaml | grep -A5 "configMapKeyRef|secretKeyRef"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方式一：修改 Deployment 中的 Key 引用（推荐，修改消费方）
  kubectl edit deployment <deploy-name> -n <namespace>
  # 将 key: wrong-key 修改为 key: correct-key
  
  # 方式二：在 ConfigMap 中添加正确的 Key（如果消费方无法修改）
  kubectl patch configmap <cm-name> -n <namespace> --type merge -p '{"data":{"correct-key":"value"}}'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 确认新 Pod 正常启动
  kubectl get pods -n <namespace> -l <selector> -w
  
  # 确认环境变量正确注入
  kubectl exec <new-pod-name> -n <namespace> -- printenv | grep <ENV_KEY>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 如果修改了 Deployment，可以回滚
  kubectl rollout undo deployment <deploy-name> -n <namespace>
  
  # 如果修改了 ConfigMap，恢复原始 Key
  kubectl patch configmap <cm-name> -n <namespace> --type json -p '[{"op":"remove","path":"/data/correct-key"}]'
  ```

#### REM-003: 替换 SubPath 为完整 Volume 挂载
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认当前使用 SubPath
  kubectl get deployment <deploy-name> -n <namespace> -o yaml | grep -A10 "volumeMounts:" | grep subPath
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修改 Deployment，移除 subPath 使用完整 Volume 挂载
  kubectl edit deployment <deploy-name> -n <namespace>
  
  # 修改前（SubPath）:
  # volumeMounts:
  # - name: config-volume
  #   mountPath: /app/config/app.yaml
  #   subPath: app.yaml
  
  # 修改后（完整 Volume）:
  # volumeMounts:
  # - name: config-volume
  #   mountPath: /app/config
  #   # 移除 subPath
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 等待新 Pod 启动
  kubectl rollout status deployment <deploy-name> -n <namespace>
  
  # 更新 ConfigMap 测试热更新
  kubectl patch configmap <cm-name> -n <namespace> --type merge -p '{"data":{"test-key":"test-value"}}'
  
  # 等待 1-2 分钟，检查 Pod 内配置是否更新
  kubectl exec <pod-name> -n <namespace> -- cat /app/config/test-key
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment <deploy-name> -n <namespace>
  ```
- **注意**: 完整 Volume 挂载会替换整个目录，确保挂载路径下无需保留的其他文件

#### REM-004: 部署 Reloader 实现配置热更新
- **适用根因**: RC-009, RC-003
- **前置检查**:
  ```bash
  # 检查是否已安装 Reloader
  kubectl get deploy -n kube-system | grep reloader
  helm list -A | grep reloader
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 使用 Helm 安装 Reloader
  helm repo add stakater https://stakater.github.io/stakater-charts
  helm repo update
  helm install reloader stakater/reloader -n kube-system
  
  # 在 Deployment 上添加 annotation 启用自动重载
  kubectl annotate deployment <deploy-name> -n <namespace> reloader.stakater.com/auto="true"
  
  # 或指定监听特定 ConfigMap
  kubectl annotate deployment <deploy-name> -n <namespace> configmap.reloader.stakater.com/reload="<cm-name>"
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 确认 Reloader 运行正常
  kubectl get pods -n kube-system -l app=reloader
  
  # 更新 ConfigMap 测试自动重启
  kubectl patch configmap <cm-name> -n <namespace> --type merge -p '{"data":{"trigger-reload":"yes"}}'
  
  # 观察 Pod 是否自动重启
  kubectl get pods -n <namespace> -l <selector> -w
  ```
- **回滚命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 移除 annotation
  kubectl annotate deployment <deploy-name> -n <namespace> reloader.stakater.com/auto-
  
  # 卸载 Reloader
  helm uninstall reloader -n kube-system  # ⚠️ 删除 release 及关联资源
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: 删除并重建 Immutable ConfigMap/Secret
- **适用根因**: RC-004
- **影响说明**: 删除 Immutable 资源会导致引用该资源的 Pod 短暂失去配置。建议使用新名称创建资源并逐步迁移。
- **审批提示**: "建议删除 immutable 的 ConfigMap/Secret `<name>` 并使用新内容重建。删除期间引用该资源的 Pod 可能受影响。是否批准？"
- **前置检查**:
  ```bash
  # 确认资源确实是 immutable
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.immutable}'
  
  # 列出所有引用该 ConfigMap 的 Pod/Deployment
  kubectl get pods -A -o json | jq -r ".items[] | select(.spec.volumes[]?.configMap?.name==\"<cm-name>\") | .metadata.namespace + \"/\" + .metadata.name"
  
  # 备份当前配置
  kubectl get configmap <cm-name> -n <namespace> -o yaml > /tmp/<cm-name>-backup.yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方式一：删除并重建（会有短暂中断）
  kubectl delete configmap <cm-name> -n <namespace>
  # 修改备份文件，移除 immutable: true，更新 data
  kubectl apply -f /tmp/<cm-name>-updated.yaml
  
  # 方式二：使用新名称创建（推荐，无中断）
  kubectl create configmap <cm-name>-v2 -n <namespace> --from-file=<config-files>
  # 然后修改 Deployment 引用新的 ConfigMap 名称
  ```
- **后置验证**:
  ```bash
  # 确认新 ConfigMap 可修改
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.immutable}'
  # 预期: 空（非 immutable）
  
  # 确认 Pod 正常运行
  kubectl get pods -n <namespace> -l <selector>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 从备份恢复原始 ConfigMap
  kubectl apply -f /tmp/<cm-name>-backup.yaml
  ```

#### REM-006: 修复 External Secrets 认证配置
- **适用根因**: RC-006
- **影响说明**: 修改 SecretStore 的认证配置会影响所有使用该 SecretStore 的 ExternalSecret 同步。
- **审批提示**: "建议更新 SecretStore `<name>` 的认证配置。修改后所有关联的 ExternalSecret 将重新同步。是否批准？"
- **前置检查**:
  ```bash
  # 检查 SecretStore 当前状态和错误
  kubectl describe secretstore <store-name> -n <namespace>
  
  # 检查认证 Secret 是否存在
  kubectl get secret <auth-secret> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 更新认证 Secret（例如 AWS credentials）
  kubectl create secret generic aws-credentials -n <namespace> \
    --from-literal=access-key=AKIAXXXXXXXXXX \
    --from-literal=secret-access-key=xxxxx \
    --dry-run=client -o yaml | kubectl apply -f -
  
  # 或更新 SecretStore 配置
  kubectl edit secretstore <store-name> -n <namespace>
  
  # 强制 ExternalSecret 重新同步
  kubectl annotate externalsecret <es-name> -n <namespace> force-sync=$(date +%s)
  ```
- **后置验证**:
  ```bash
  # 检查 SecretStore 状态
  kubectl get secretstore <store-name> -n <namespace>
  # 预期: Ready
  
  # 检查 ExternalSecret 同步状态
  kubectl get externalsecret <es-name> -n <namespace>
  # 预期: SecretSynced
  
  # 确认目标 Secret 已创建/更新
  kubectl get secret <target-secret> -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 恢复原认证 Secret
  kubectl apply -f /tmp/auth-secret-backup.yaml
  ```

#### REM-007: 修复 Vault Agent Injector
- **适用根因**: RC-007
- **影响说明**: 修复 Vault Agent Injector 可能需要重启 Pod 以重新触发注入。
- **审批提示**: "建议修复 Vault Agent 配置。受影响的 Pod 可能需要重启以获取正确的 Secret 注入。是否批准？"
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 Vault Agent Injector 状态
  kubectl get pods -n vault -l app.kubernetes.io/name=vault-agent-injector
  kubectl logs -n vault -l app.kubernetes.io/name=vault-agent-injector --tail=50
  
  # 检查 Vault 服务可达性
  kubectl exec -n vault vault-0 -- vault status
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 常见问题 1: Vault token 过期，重新配置 Kubernetes auth
  kubectl exec -n vault vault-0 -- vault write auth/kubernetes/config \
    kubernetes_host="https://$KUBERNETES_PORT_443_TCP_ADDR:443"
  
  # 常见问题 2: ServiceAccount 未授权，创建 Vault policy 和 role
  kubectl exec -n vault vault-0 -- vault policy write <app-name> - <<EOF
  path "secret/data/<app-name>/*" {
    capabilities = ["read"]
  }
  EOF
  
  kubectl exec -n vault vault-0 -- vault write auth/kubernetes/role/<app-name> \
    bound_service_account_names=<sa-name> \
    bound_service_account_namespaces=<namespace> \
    policies=<app-name> \
    ttl=1h
  
  # 重启受影响的 Pod 以重新触发注入
  kubectl rollout restart deployment <deploy-name> -n <namespace>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查新 Pod 的 vault-agent 容器状态
  kubectl get pod <new-pod> -n <namespace> -o jsonpath='{.status.initContainerStatuses[*].name}={.status.initContainerStatuses[*].ready}'
  
  # 检查 Secret 文件是否正确注入
  kubectl exec <new-pod> -n <namespace> -- ls -la /vault/secrets/
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 回滚 Deployment
  kubectl rollout undo deployment <deploy-name> -n <namespace>
  ```

#### REM-008: ConfigMap 拆分或迁移到外部存储
- **适用根因**: RC-005
- **影响说明**: 将大型 ConfigMap 拆分为多个小 ConfigMap 或迁移到 PersistentVolume、S3 等外部存储，需要修改应用读取配置的方式。
- **审批提示**: "建议将超大 ConfigMap 拆分或迁移到外部存储。此操作需要修改应用配置读取逻辑。是否批准？"
- **前置检查**:
  ```bash
  # 确认 ConfigMap 大小
  kubectl get configmap <cm-name> -n <namespace> -o json | wc -c
  
  # 分析数据结构，确定拆分方案
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 方案一：拆分为多个 ConfigMap
  # 将大文件按模块拆分
  kubectl get configmap <cm-name> -n <namespace> -o jsonpath='{.data.part1}' > /tmp/part1.yaml
  kubectl create configmap <cm-name>-part1 -n <namespace> --from-file=/tmp/part1.yaml
  
  # 方案二：使用 PV 存储配置
  # 创建 PVC 并将配置上传到 PV
  # 修改 Deployment 使用 PVC 替代 ConfigMap
  
  # 方案三：使用 S3/OSS 等对象存储
  # 应用启动时从对象存储拉取配置
  ```
- **后置验证**:
  ```bash
  # 确认拆分后的 ConfigMap 大小
  kubectl get configmap <cm-name>-part1 -n <namespace> -o json | wc -c
  
  # 确认应用正常读取配置
  kubectl logs <pod-name> -n <namespace> | grep -i config
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 恢复原 Deployment 配置
  kubectl rollout undo deployment <deploy-name> -n <namespace>
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-009: 修复 KMS Provider 配置
- **适用根因**: RC-008
- **影响说明**: KMS 配置错误可能导致所有 Secret 无法读取。修复过程需要重启 kube-apiserver，可能导致短暂的控制平面不可用。
- **操作步骤**:
  1. **备份当前加密配置**:
     ```bash
     # SSH 到控制平面节点
     cp /etc/kubernetes/encryption-config.yaml /etc/kubernetes/encryption-config.yaml.bak
     ```
  2. **检查 KMS 服务状态**:
     ```bash
     # 如果使用本地 KMS 插件（如 Vault Transit、AWS KMS）
     systemctl status <kms-plugin-service>
     
     # 测试 KMS 连接
     # 具体命令取决于 KMS 提供商
     ```
  3. **修复 KMS 配置**:
     ```bash
     # 编辑加密配置
     vim /etc/kubernetes/encryption-config.yaml
     
     # 确保 KMS 提供商配置正确
     # apiVersion: apiserver.config.k8s.io/v1
     # kind: EncryptionConfiguration
     # resources:
     #   - resources:
     #       - secrets
     #     providers:
     #       - kms:
     #           name: <kms-provider-name>
     #           endpoint: unix:///var/run/kms-plugin/socket.sock
     #           ...
     ```
  4. **重启 kube-apiserver**:
     ```bash
     # 如果使用 static pod
     mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
     sleep 10
     mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
     
     # 等待 apiserver 恢复
     kubectl get nodes
     ```
- **安全检查**:
  - 确保有多个 control plane 节点（HA 模式），避免单点问题
  - 在变更前通知相关团队
  - 准备好回滚方案
- **回滚方案**:
  ```bash
  # 恢复备份的配置
  cp /etc/kubernetes/encryption-config.yaml.bak /etc/kubernetes/encryption-config.yaml
  # 重启 apiserver
  ```

#### REM-010: EncryptionConfiguration 密钥轮换
- **适用根因**: RC-008
- **影响说明**: 密钥轮换涉及重新加密所有 Secret。如果密钥管理不当，可能导致数据无法解密。
- **操作步骤**:
  1. **在加密配置中添加新密钥（作为首选）**:
     ```yaml
     # /etc/kubernetes/encryption-config.yaml
     apiVersion: apiserver.config.k8s.io/v1
     kind: EncryptionConfiguration
     resources:
       - resources:
           - secrets
         providers:
           - kms:
               name: kms-v2
               endpoint: unix:///var/run/kms-plugin/socket.sock
           - kms:
               name: kms-v1  # 旧密钥保留用于解密
               endpoint: unix:///var/run/kms-plugin-old/socket.sock
     ```
  2. **重启 apiserver 应用新配置**
  3. **触发所有 Secret 重新加密**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 使用 etcd 直接操作或 kubectl 更新所有 Secret
     kubectl get secrets -A -o json | kubectl replace -f -
     ```
  4. **验证使用新密钥加密**:
     ```bash
     # 检查 etcd 中 Secret 的加密前缀
     ETCDCTL_API=3 etcdctl get /registry/secrets/<ns>/<name> | hexdump -C | head
     ```
  5. **移除旧密钥配置**
- **安全检查**:
  - 在低峰期执行
  - 确保新旧密钥都可用
  - 逐步移除旧密钥
- **回滚方案**:
  - 保留旧密钥配置直到确认所有 Secret 使用新密钥加密
  - 如需回滚，恢复原配置并保留解密能力

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: Secret 加密后端完全重建
- **适用根因**: RC-008（极端情况：加密密钥完全丢失）
- **审批要求**: 需要高级 SRE + 安全团队 + 架构师审批
- **数据备份**: 此操作不可逆，所有现有加密的 Secret 将无法解密
- **操作步骤**:
  1. **评估数据恢复可能性**:
     - 检查 KMS 备份
     - 检查 etcd 快照
     - 联系 KMS 服务提供商
  2. **如果密钥无法恢复，准备重建**:
     - 收集所有需要重建的 Secret 列表
     - 从应用配置、CI/CD 系统、密码管理工具收集明文值
  3. **删除旧 Secret 并重建**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 临时禁用加密（使用 identity 提供商）
     # 修改 encryption-config.yaml
     # providers:
     #   - identity: {}
     
     # 重启 apiserver
     
     # 重建所有 Secret
     kubectl delete secret <name> -n <namespace>
     kubectl create secret generic <name> -n <namespace> --from-literal=...
     ```
  4. **配置新的加密密钥**
  5. **重新加密所有 Secret**
- **回滚方案**:
  - 此操作不可回滚
  - 确保有所有 Secret 值的备份来源

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: 确认 ConfigMap/Secret 存在且内容正确
kubectl get configmap <cm-name> -n <namespace>
kubectl get secret <secret-name> -n <namespace>
# 预期: 资源存在

# V2: 确认 Pod 正常启动
kubectl get pods -n <namespace> -l <selector>
# 预期: STATUS 为 Running

# V3: 确认没有配置相关的 Events
kubectl get events -n <namespace> --field-selector reason=FailedMount,reason=CreateContainerConfigError --sort-by=.lastTimestamp
# 预期: 无新的错误事件

# V4: 确认配置正确挂载到 Pod
kubectl exec <pod-name> -n <namespace> -- ls -la /path/to/config
kubectl exec <pod-name> -n <namespace> -- cat /path/to/config/<filename> | head -5
# 预期: 文件存在且内容正确

# V5: 确认环境变量正确注入
kubectl exec <pod-name> -n <namespace> -- printenv | grep <ENV_KEY>
# 预期: 环境变量存在且值正确

# V6: 确认应用读取到正确配置
kubectl logs <pod-name> -n <namespace> --tail=20 | grep -i config
# 预期: 应用日志显示配置加载成功
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pod 状态 | `kubectl get pods -n <namespace> -l <selector>` | 持续 Running | 任何 Pod 变为非 Running |
| ExternalSecret 同步状态 | `kubectl get externalsecret -A` | 全部 SecretSynced | 任何 NotReady 或 Error |
| 应用健康检查 | 应用 liveness/readiness probe | 全部通过 | 任何探针失败 |
| 配置相关错误 | `kubectl get events -A --field-selector reason=FailedMount` | 无新事件 | 新的 FailedMount 事件 |
| Vault Agent 容器状态 | `kubectl get pods -n <namespace> -o jsonpath='{.items[*].status.containerStatuses[?(@.name=="vault-agent")].ready}'` | 全部 true | 任何 false |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 引用配置的 Pod 全部处于 Running 状态
- [ ] ConfigMap/Secret 内容正确且可被 Pod 读取
- [ ] 无新的 FailedMount、CreateContainerConfigError 事件
- [ ] ExternalSecret 状态为 SecretSynced（如适用）
- [ ] 应用日志显示配置加载成功
- [ ] 应用健康检查（liveness/readiness probe）通过
- [ ] 配置热更新功能正常工作（如适用）
- [ ] 根因已明确并记录

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| ExternalSecret 同步 | `kubectl get externalsecret -A` 状态监控 | 每 15 分钟 | 同步失败 → 检查外部 Secret 存储连接 |
| 配置漂移 | GitOps 工具（ArgoCD/Flux）同步状态 | 持续 | OutOfSync → 确认变更来源 |
| Secret 过期 | 证书/Token 过期时间监控 | 每日 | 即将过期 → 提前轮换 |
| Vault Token 续期 | Vault Agent 日志监控 | 每小时 | 续期失败 → 检查 Vault auth |
| ConfigMap 大小 | `kubectl get cm -A -o json | jq '.items[].data | length'` | 每日 | 接近限制 → 提前拆分 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **严重性升级** | 初始分级为 P3 但影响面扩大（更多 Pod 失败） | 诊断过程中受影响 Pod 数增加 |
| **KMS 完全问题** | 所有 Secret 读取失败 | D2.7 显示 KMS 完全不可用 |
| **安全疑虑** | 怀疑 Secret 泄露或被恶意修改 | 任何诊断步骤发现安全异常 |

### 8.2 升级消息模板

```
【{severity}】ConfigMap/Secret 配置问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {config_type} `{config_name}` 在 namespace `{namespace}` 中存在问题
- 影响范围:
  - 受影响 Pod: {affected_pod_count} 个
  - 受影响 Namespace: {affected_namespaces}
  - 涉及外部 Secret 管理: {external_secret_involved}
- 已完成诊断:
  - Phase 1 基础检查: {phase1_summary}
  - Phase 2 高级诊断: {phase2_summary}
  - Phase 3 外部 Secret 诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-CONFIG-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设
4. **关键资源快照**:
   ```bash
   # 问题 Pod 描述
   kubectl describe pod <pod-name> -n <namespace> > pod-describe.txt
   # ConfigMap/Secret 内容
   kubectl get configmap <cm-name> -n <namespace> -o yaml > cm-content.yaml
   kubectl get secret <secret-name> -n <namespace> -o yaml > secret-content.yaml
   # 相关事件
   kubectl get events -n <namespace> --sort-by=.lastTimestamp > events.txt
   # ExternalSecret 状态（如适用）
   kubectl describe externalsecret <es-name> -n <namespace> > externalsecret.txt

   ```
5. **事件时间线**: 最近 30 分钟内的关键事件

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Immutable ConfigMaps/Secrets | GA | GA | GA | GA | GA |
| Projected Volume | GA | GA | GA | GA | GA |
| CSI Volume for Secrets | GA | GA | GA | GA | GA |
| KMS v2 | beta | GA | GA | GA | GA |
| SubPath 行为（不自动更新） | 维持 | 维持 | 维持 | 维持 | 维持 |
| Secret 自动轮换（CSI Driver） | GA | GA | GA | GA | GA |
| 环境变量从 ConfigMap 热更新 | 不支持 | 不支持 | 不支持 | 不支持 | 不支持 |
| Volume 挂载自动更新（非 SubPath） | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get cm -o jsonpath` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /healthz/kms-provider-*` | KMS v1 | KMS v1/v2 | KMS v2 | KMS v2 | KMS v2 |
| `kubectl create secret --from-env-file` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl diff` for ConfigMap | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| ConfigMap | v1 (core) | v1 | v1 | v1 | v1 |
| Secret | v1 (core) | v1 | v1 | v1 | v1 |
| ExternalSecret (ESO) | external-secrets.io/v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 |
| SecretStore (ESO) | external-secrets.io/v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 |
| EncryptionConfiguration | apiserver.config.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: Immutable ConfigMaps/Secrets 完全稳定。标记为 immutable 的资源只能删除重建，不能修改。

- **[v1.29+]**: KMS v2 GA。推荐使用 KMS v2 API，提供更好的性能和密钥轮换支持：
  - KMS v2 支持 `/healthz/kms-provider-{index}` 健康检查端点
  - 建议从 KMS v1 迁移到 v2

- **[v1.30+]**: 
  - kubelet 同步 ConfigMap 的默认间隔（`--sync-frequency`）为 1 分钟
  - 非 SubPath Volume 挂载的配置更新通常在 1-2 分钟内生效

- **[v1.31+]**: 
  - 改进的 Secret 加密日志，便于诊断 KMS 问题
  - 支持更多 CSI Secret Store 提供商

- **[v1.32+]**: 
  - InPlacePodVerticalScaling 对 projected volume 的支持
  - Secret 轮换事件更详细

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 SubPath 不更新误判为 ConfigMap 内容错误** | 更新 ConfigMap 后 Pod 内配置不变 | SubPath 挂载是设计行为，不支持自动更新 | D2.1 首先确认是否使用 SubPath；如是，推荐 REM-003 或 REM-004 方案 |
| **将环境变量不更新误判为 ConfigMap 同步失败** | 更新 ConfigMap 后环境变量仍是旧值 | 环境变量在容器启动时固化，不会自动更新 | 说明这是预期行为，需重启 Pod 或使用 Reloader |
| **将 optional: true 引起的问题误判为应用 bug** | Pod Running 但应用报空指针异常 | ConfigMap/Secret 不存在，optional: true 导致静默跳过 | D1.2 检查 optional 配置；D1.5 验证环境变量是否存在 |
| **将网络问题误判为 External Secrets 配置错误** | ExternalSecret NotReady | 实际是 NetworkPolicy 阻止 ESO 访问外部存储 | 先检查 ESO 控制器日志中的网络错误 |
| **将权限问题误判为 Secret 不存在** | Pod 启动失败，提示 Secret 不存在 | ServiceAccount 无权读取该 Secret | 检查 `kubectl auth can-i get secrets -n <ns> --as=system:serviceaccount:<ns>:<sa>` |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| ConfigMap/Secret 深度排查 | `故障诊断/19-configmap-secret-troubleshooting.md` | 超出本 Skill 覆盖的复杂场景 |
| Kubernetes Secret 加密 | `安全/` | KMS Provider、EncryptionConfiguration 详细配置 |
| External Secrets Operator | External Secrets 官方文档 | ESO 高级配置和故障排查 |
| HashiCorp Vault 集成 | Vault 官方文档 + `安全/` | Vault Agent Injector 深度配置 |
| kubelet 配置同步机制 | `集群基础/` | 理解 ConfigMap Volume 更新的内部机制 |
| GitOps 配置管理 | `专项技术/09-gitops-workflow-argocd.md` | 配置漂移检测和一致性管理 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、11 个修复操作 | 配置管理问题是高频问题，需要系统化的诊断流程 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **Sealed Secrets**: Bitnami Sealed Secrets 的加密/解密故障排查
2. **SOPS**: Mozilla SOPS 与 GitOps 集成的配置管理问题
3. **Confidential Computing**: 机密计算环境下的 Secret 管理
4. **多租户隔离**: 跨租户 ConfigMap/Secret 访问控制
5. **大规模 ConfigMap 同步**: 大量 ConfigMap 变更时的 kubelet 性能问题
6. **边缘场景**: 弱网环境下 External Secrets 同步的特殊处理

## Related

- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]

```

<!-- risk-assessed -->
