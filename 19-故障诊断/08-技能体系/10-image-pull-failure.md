---
title: 镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting
description: '## 1. 概述'
summary: '镜像拉取问题是 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 集群中**最常见的 Pod 启动失败原因之一**，约占所有 Pod 异常工单的 20-30%。当容器镜像无法成功拉取时，Pod 将持续处于 `ImagePullBackOff` 或 `ErrImagePull` 状态，'
category: pod
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- coredns
- helm
- containerd
- cri-o
- docker
tier: core
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
- 镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting 是什么
- 如何 镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting
trigger_keywords:
- ImagePullBackOff
- ErrImagePull
- image pull failed
- registry authentication
- pull rate limit
- image not found
- manifest unknown
- unauthorized access
- imagePullSecrets
- air-gap registry
- 镜像拉取失败
- 镜像拉不下来
- 仓库认证失败
- 镜像找不到
- 私有仓库配置
- 离线镜像
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- gpu-scheduling-basics
- policy-basics
skill_id: SKILL-10_IMAGE_PULL_FAILURE-001
skill_name: 镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting
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




---


# 镜像拉取与仓库故障诊断 / Image Pull & Registry Troubleshooting

---

## 1. 概述

镜像拉取问题是 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 集群中**最常见的 Pod 启动失败原因之一**，约占所有 Pod 异常工单的 20-30%。当容器镜像无法成功拉取时，Pod 将持续处于 `ImagePullBackOff` 或 `ErrImagePull` 状态，导致服务无法启动或扩容失败。对于生产环境中的关键服务，镜像拉取问题可能直接导致业务中断。

### 典型触发场景

1. **镜像名称/Tag 错误**: 开发人员配置了不存在的镜像名或 Tag，导致仓库返回 404 Not Found
2. **私有仓库认证失败**: imagePullSecrets 未配置、配置错误或凭证过期，仓库返回 401 Unauthorized
3. **仓库限速 (Rate Limit)**: Docker Hub 等公共仓库对匿名/免费用户实施拉取限制，返回 429 Too Many Requests
4. **网络/代理问题**: 节点无法访问仓库（防火墙、代理配置、DNS 解析失败），连接超时
5. **TLS/证书问题**: 私有仓库使用自签名证书但节点未配置信任，TLS 握手失败
6. **多架构镜像问题**: 镜像不支持当前节点的 CPU 架构（如 ARM64 节点拉取 AMD64-only 镜像）
7. **镜像策略阻断**: Admission Webhook (如 Gatekeeper、[[kyverno|Kyverno]]) 拒绝镜像因不符合安全策略
8. **离线环境同步**: Air-Gap 环境中镜像未同步到内部仓库

### 前置条件

- **RBAC 权限**: 至少需要对 [[pods|pods]]、events、secrets、serviceaccounts 的 get/list 权限；配置 imagePullSecrets 需要 secrets 的 create/update 权限
- **节点访问**: 深度诊断（Phase 2+）可能需要 SSH 访问节点或使用 `kubectl debug node/`
- **工具要求**: kubectl (v1.28+), crictl (节点诊断), curl/openssl (网络/TLS 诊断)
- **可选工具**: crane/skopeo (镜像 manifest 检查), cosign/notation (签名验证)

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod 状态显示 `ImagePullBackOff` / Pod stuck in ImagePullBackOff | `kubectl get pods -n NS` 并检查 STATUS 列为 ImagePullBackOff | 0.95 | Pod 正在被删除或重建中短暂出现该状态 |
| S2 | Pod 状态显示 `ErrImagePull` / Pod shows ErrImagePull status | `kubectl get pods -n NS` 并检查 STATUS 列为 ErrImagePull | 0.95 | 首次拉取失败后 kubelet 正在重试 |
| S3 | Events 显示 "unauthorized: authentication required" / Unauthorized error in events | `kubectl describe pod POD -n NS` 事件包含 unauthorized 或 authentication required | 0.90 | 临时性 token 过期，正在自动刷新（云托管仓库） |
| S4 | Events 显示 "manifest unknown" 或 "not found" / Manifest not found | `kubectl describe pod POD -n NS` 事件包含 manifest unknown, not found, tag not found | 0.90 | 镜像正在构建中，Tag 即将可用（CI/CD 时序问题） |
| S5 | Events 显示 "toomanyrequests: Rate exceeded" (429) / Rate limit exceeded | `kubectl describe pod POD -n NS` 事件包含 429, toomanyrequests, rate limit | 0.95 | 短时间大量拉取后的限速，等待冷却期后自动恢复 |
| S6 | 镜像拉取超时 "context deadline exceeded" / Pull timeout | `kubectl describe pod POD -n NS` 事件包含 timeout, context deadline exceeded | 0.80 | 网络临时抖动，重试后可能成功 |
| S7 | "no matching manifest for linux/arm64" (多架构问题) / Architecture mismatch | `kubectl describe pod POD -n NS` 事件包含 no matching manifest, platform not supported | 0.95 | 镜像 manifest list 正在更新中（极少见） |
| S8 | TLS 握手失败 "x509: certificate signed by unknown authority" / TLS certificate error | `kubectl describe pod POD -n NS` 事件包含 x509, certificate, TLS handshake | 0.90 | 仓库证书正在轮转中（应有 automation） |
| S9 | Admission Webhook 拒绝镜像 "image policy violation" / Image policy denied | `kubectl describe pod POD -n NS` 事件包含 admission, denied, policy, blocked | 0.85 | 镜像已更新符合策略但 Pod 未重建 |
| S10 | Init 容器镜像拉取失败导致 Pod 永久 Init / Init container image pull failure | `kubectl get pod POD -n NS` 显示 Init:ImagePullBackOff 或 Init:ErrImagePull | 0.90 | Init 容器正在完成初始化（正常流程） |
| S11 | 镜像 hash 不匹配 "image verification failed" / Image digest mismatch | `kubectl describe pod POD -n NS` 事件包含 digest mismatch, verification failed | 0.95 | 仓库内容刚更新，缓存不一致（极少见） |
| S12 | "server gave HTTP response to HTTPS client" / Protocol mismatch | `kubectl describe pod POD -n NS` 事件包含 HTTP response to HTTPS | 0.95 | 无 |
| S13 | 镜像拉取成功但容器启动失败（非本 Skill） / Image pulled but container fails | `kubectl describe pod POD -n NS` 显示镜像已拉取但容器 CrashLoopBackOff | 0.10 | 这是容器运行时问题，应使用 SKILL-POD-001 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Pod 一直 ImagePullBackOff，拉不下来镜像"
- "部署后服务起不来，显示 ErrImagePull"
- "私有仓库镜像拉取失败，报认证错误"
- "Docker Hub 限速了，镜像拉不下来"
- "内网环境镜像拉取超时"
- "新版本镜像找不到，manifest unknown"
- "ARM 服务器拉不了 x86 镜像"
- "镜像仓库证书问题，TLS 握手失败"
- "Admission 策略拦截了镜像"
- "Init 容器镜像拉失败，Pod 卡住"
- "离线环境镜像同步有问题"

**English ticket descriptions**:
- "Pod stuck in ImagePullBackOff, cannot pull image"
- "Deployment failed with ErrImagePull error"
- "Private registry authentication failed, unauthorized"
- "Docker Hub rate limit exceeded, 429 error"
- "Image pull timeout in air-gap environment"
- "Image tag not found, manifest unknown error"
- "Cannot pull AMD64 image on ARM64 node"
- "Registry TLS certificate not trusted"
- "Image blocked by admission policy"
- "Init container image pull failed"
- "Harbor sync failed in disconnected cluster"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 镜像拉取成功，但容器 CrashLoopBackOff | SKILL-POD-001 | 镜像本身正常，是应用程序问题 |
| 节点 NotReady 导致所有 Pod 异常 | SKILL-NODE-001 | 节点级问题，非镜像问题 |
| 网络策略阻止 Pod 间通信 | SKILL-NET-001 | 网络策略问题，非镜像拉取问题 |
| 镜像已拉取但 Pod Pending（资源不足） | SKILL-POD-002 | 调度问题 |
| 仅仓库管理员无法登录仓库 Web UI | 非 K8s 范畴 | 仓库自身管理问题 |
| CI/CD Pipeline 中的 docker build 失败 | 非 K8s 范畴 | 镜像构建问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 统计受影响 Pod 数量和影响范围（10s）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 统计所有 namespace 中处于 ImagePullBackOff/ErrImagePull 状态的 Pod
kubectl get pods --all-namespaces --no-headers | grep -E "ImagePullBackOff|ErrImagePull" | wc -l

# 查看受影响的 namespace 分布
kubectl get pods --all-namespaces --no-headers | grep -E "ImagePullBackOff|ErrImagePull" | awk '{print $1}' | sort | uniq -c | sort -rn
```
> **判断规则**:
> - 受影响 Pod > 50 个或涉及 kube-system → **P0**（集群级镜像基础设施问题）
> - 受影响 Pod 10-50 个 → **P1**（多服务受影响）
> - 受影响 Pod 1-9 个 → **P2**（单个或少量服务）
> - 新部署的测试 Pod → **P3**

**Step T2**: 检查是否为同一镜像问题（30s）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 提取受影响 Pod 的镜像列表
kubectl get pods --all-namespaces -o jsonpath='{range .items[?(@.status.phase!="Running")]}{.spec.containers[*].image}{"\n"}{end}' | sort | uniq -c | sort -rn | head -10
```
> **判断规则**:
> - 单一镜像影响多个 Pod → 可能是镜像本身问题（不存在/构建失败）
> - 多个不同镜像均失败 → 可能是仓库/网络/认证基础设施问题

**Step T3**: 快速测试镜像拉取能力（60s）
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在任一节点上测试拉取公共镜像（验证基础网络）
kubectl run test-pull --image=docker.io/library/busybox:latest --rm -it --restart=Never --command -- echo "Pull succeeded"
# 或使用 kubectl debug
kubectl debug node/<node-name> -it --image=busybox:latest -- echo "Pull succeeded"
```
> **判断规则**:
> - 公共镜像也拉取失败 → 网络/代理/containerd 基础问题
> - 公共镜像成功，特定镜像失败 → 镜像/仓库/认证问题

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| kube-system 核心组件镜像拉取失败 **或** >50 个业务 Pod 受影响 | **P0** | 集群核心功能受损或大规模业务影响 | 立即响应，15min 内确认根因 |
| 生产环境关键服务镜像拉取失败 **或** 10-50 个 Pod 受影响 | **P1** | 生产业务降级 | 15min 内响应，30min 内修复 |
| 单个服务/少量 Pod 镜像拉取失败 | **P2** | 影响有限，冗余服务可承担流量 | 30min 内响应，2h 内修复 |
| 测试环境/新部署服务 **或** 已知是镜像构建中 | **P3** | 不影响生产 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **集群镜像基础设施问题**: 所有新 Pod 均无法拉取任何镜像（containerd/CRI-O 可能问题）
- **仓库完全不可用**: Harbor/私有仓库服务宕机，所有依赖该仓库的服务受影响
- **凭证泄露风险**: 发现 imagePullSecrets 被意外暴露或疑似泄露
- **安全事件**: 发现镜像被篡改或 digest 不匹配，怀疑供应链攻击
- **级联问题**: 镜像问题导致 kube-proxy、CNI 等核心组件无法启动

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 Pod 和镜像状态信息，无需 SSH 登录节点。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 Pod 详细事件和状态
- **命令**:
  ```bash
  kubectl describe pod <pod-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: 关注 Events 部分的 Warning 事件和 Status 中的 containerStatuses
- **判断规则**:
  - Events 包含 `Failed to pull image` + `unauthorized` → RC-002（认证失败）
  - Events 包含 `manifest unknown` 或 `not found` → RC-001（镜像名/Tag 错误）
  - Events 包含 `toomanyrequests` 或 `429` → RC-004（限速）
  - Events 包含 `timeout` 或 `context deadline exceeded` → RC-003（网络）或 RC-010（大镜像）
  - Events 包含 `x509` 或 `certificate` → RC-006（TLS 证书）
  - Events 包含 `no matching manifest` → RC-008（多架构）
  - Events 包含 `denied` 或 `policy` → RC-009（安全策略）
- **版本差异**: 无

**Step D1.2**: 验证镜像全名和 Tag
- **命令**:
  ```bash
  # 获取 Pod 中所有容器的镜像
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*]}Container: {.name} Image: {.image}{"\n"}{end}'
  
  # 同时检查 Init 容器
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.initContainers[*]}InitContainer: {.name} Image: {.image}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: 完整镜像路径（registry/namespace/image:tag 或 @digest）
- **判断规则**:
  - 镜像名未包含 registry 前缀（如 `nginx:latest`）→ 默认使用 Docker Hub，可能受限速影响
  - 镜像 Tag 为 `latest` → 可能存在缓存问题或 Tag 被覆盖
  - 镜像名有明显拼写错误 → RC-001
  - 镜像使用 digest（@sha256:xxx）但 digest 不存在 → RC-001
- **版本差异**: 无

**Step D1.3**: 检查 imagePullSecrets 配置
- **命令**:
  ```bash
  # 检查 Pod 的 imagePullSecrets
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'
  
  # 检查 ServiceAccount 的默认 imagePullSecrets
  SA_NAME=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.serviceAccountName}')
  kubectl get sa ${SA_NAME:-default} -n <namespace> -o jsonpath='{.imagePullSecrets}'
  
  # 验证 Secret 是否存在
  kubectl get secrets -n <namespace> -l type=kubernetes.io/dockerconfigjson
  ```
- **超时**: 10s
- **预期输出模式**: Secret 名称列表或空
- **判断规则**:
  - Pod 和 ServiceAccount 均无 imagePullSecrets 但需要访问私有仓库 → RC-005
  - imagePullSecrets 指向的 Secret 不存在 → RC-005
  - Secret 存在但可能已过期/配置错误 → 继续 D1.4 验证
- **版本差异**: 无

**Step D1.4**: 验证 imagePullSecret 内容（如存在）
- **命令**:
  ```bash
  # 获取 Secret 并解码查看配置（不显示密码）
  SECRET_NAME="<secret-name>"
  kubectl get secret ${SECRET_NAME} -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq 'del(.auths[].auth) | del(.auths[].password)'
  ```
- **超时**: 5s
- **预期输出模式**: JSON 格式的 docker config，包含 registry URL 和 username
- **判断规则**:
  - Registry URL 与镜像中的 registry 不匹配 → RC-005（配置错误）
  - Username 为空或格式异常 → RC-005（配置错误）
  - Registry URL 正确 → 凭证可能过期（RC-002）或需要在节点侧验证
- **版本差异**: 无

**Step D1.5**: 检查 imagePullPolicy
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*]}{.name}: {.imagePullPolicy}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: Always / IfNotPresent / Never
- **判断规则**:
  - `Never` 但镜像不在节点本地 → 必然失败（配置问题）
  - `IfNotPresent` + `:latest` Tag → 可能使用了旧的缓存镜像
  - `Always` → 每次都从仓库拉取，对网络和仓库可用性要求高
- **版本差异**: 无

**Step D1.6**: 检查节点上的镜像缓存
- **命令**:
  ```bash
  # 获取 Pod 所在节点
  NODE=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}')
  
  # 使用 kubectl debug 检查节点镜像缓存（v1.28+）
  kubectl debug node/${NODE} -it --image=docker.io/library/busybox:latest -- crictl images | grep -i "<image-keyword>"
  ```
- **超时**: 30s
- **预期输出模式**: 镜像列表
- **判断规则**:
  - 镜像已存在但 imagePullPolicy=Always 仍在拉取 → 仓库连通性问题
  - 镜像不存在且 imagePullPolicy=Never → 配置错误
  - 镜像不存在 + 其他正常 → 继续深度诊断
- **版本差异**:
  - **[v1.28+]**: `kubectl debug node/` 完全支持
  - 旧版本需要 SSH 访问节点

---

### Phase 2: 深度诊断（只读，零风险，需节点访问）

> **目标**: 在节点侧验证镜像拉取、网络连通性和仓库认证。
> **前提**: 需要 SSH 访问或 `kubectl debug node/` 权限
> **预计耗时**: 5-15 分钟

**Step D2.1**: 仓库认证测试
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 准备测试 Secret（dry-run 模式，不实际创建）
  kubectl create secret docker-registry test-auth \
    --docker-server=<registry> \
    --docker-username=<user> \
    --docker-password=<pass> \
    --dry-run=client -o yaml
  
  # 如果需要测试现有 Secret 的有效性
  SECRET_NAME="<existing-secret>"
  kubectl get secret ${SECRET_NAME} -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d > /tmp/config.json
  # 然后在节点上使用此配置测试
  ```
- **超时**: 10s
- **预期输出模式**: Secret YAML
- **判断规则**:
  - 生成成功 → 配置格式正确，需要在节点测试实际认证
  - 参数错误 → 需要获取正确的仓库凭证
- **版本差异**: 无

**Step D2.2**: 节点级镜像拉取测试
- **命令**:
  ```bash
  # SSH 到节点或使用 kubectl debug
  # 方式 1: SSH
  ssh <node-ip> "crictl pull <full-image-name>"
  
  # 方式 2: kubectl debug（推荐）
  kubectl debug node/<node-name> -it --image=docker.io/library/alpine:latest -- /bin/sh -c "
    # 安装必要工具
    apk add --no-cache curl jq
    # 测试 containerd 拉取（使用 ctr，因为 crictl 需要配置）
    # 或直接检查拉取错误信息
  "
  
  # 对于 containerd 运行时
  ssh <node-ip> "ctr -n k8s.io image pull <full-image-name>"
  
  # 对于 CRI-O 运行时
  ssh <node-ip> "crictl pull <full-image-name>"
  ```
- **超时**: 120s
- **预期输出模式**: 拉取进度和结果
- **判断规则**:
  - `unauthorized` → RC-002（认证失败）
  - `manifest unknown` → RC-001（镜像/Tag 不存在）
  - `toomanyrequests` / `429` → RC-004（限速）
  - `timeout` / `deadline exceeded` → RC-003 或 RC-010
  - `x509` / `certificate` → RC-006
  - 成功拉取 → 可能是 imagePullSecrets 在 Pod 级别未正确配置
- **版本差异**:
  - containerd: 使用 `crictl` 或 `ctr`
  - CRI-O: 使用 `crictl` 或 `podman`

**Step D2.3**: 网络连通性诊断
- **命令**:
  ```bash
  # 从节点测试仓库连通性
  # 方式 1: SSH
  ssh <node-ip> "curl -v --max-time 10 https://<registry>/v2/"
  
  # 方式 2: kubectl debug
  kubectl debug node/<node-name> -it --image=docker.io/library/curlimages/curl:latest -- \
    curl -v --max-time 10 https://<registry>/v2/
  
  # 对于 Docker Hub
  curl -v --max-time 10 https://registry-1.docker.io/v2/
  
  # 对于阿里云 ACR
  curl -v --max-time 10 https://registry.cn-hangzhou.aliyuncs.com/v2/
  
  # 对于 AWS ECR（需要区域）
  curl -v --max-time 10 https://<account>.dkr.ecr.<region>.amazonaws.com/v2/
  
  # 对于 Google GCR/Artifact Registry
  curl -v --max-time 10 https://gcr.io/v2/
  
  # 对于 GitHub GHCR
  curl -v --max-time 10 https://ghcr.io/v2/
  ```
- **超时**: 15s
- **预期输出模式**: HTTP 响应（200/401/404 等）
- **判断规则**:
  - 连接超时 → RC-003（网络问题）
  - HTTP 401 Unauthorized → 仓库可达，需要认证（正常响应）
  - HTTP 200 → 仓库可达，可匿名访问（公开仓库）
  - SSL/TLS 错误 → RC-006（证书问题）
  - DNS 解析失败 → RC-003（DNS 问题）
- **版本差异**: 无

**Step D2.4**: DNS 解析诊断
- **命令**:
  ```bash
  # 从 Pod 侧测试 DNS
  kubectl run dns-test --image=busybox:latest --rm -it --restart=Never -- nslookup <registry-host>
  
  # 从节点侧测试 DNS
  ssh <node-ip> "nslookup <registry-host>"
  
  # 检查 coredns Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-dns
  
  # 检查 resolv.conf
  kubectl debug node/<node-name> -it --image=busybox -- cat /etc/resolv.conf
  ```
- **超时**: 15s
- **预期输出模式**: DNS 解析结果
- **判断规则**:
  - Pod 侧 DNS 解析失败但节点侧成功 → CoreDNS 问题
  - 节点侧 DNS 也解析失败 → 上游 DNS 或网络问题（RC-003）
  - DNS 解析成功但连接超时 → 防火墙/路由问题
- **版本差异**: 无

**Step D2.5**: 代理配置检查
- **命令**:
  ```bash
  # 检查 containerd 代理配置
  ssh <node-ip> "cat /etc/systemd/system/containerd.service.d/proxy.conf 2>/dev/null || echo 'No proxy config'"
  ssh <node-ip> "cat /etc/systemd/system/containerd.service.d/http-proxy.conf 2>/dev/null || echo 'No http-proxy config'"
  
  # 检查环境变量
  ssh <node-ip> "systemctl show containerd --property=Environment"
  
  # 对于 CRI-O
  ssh <node-ip> "cat /etc/systemd/system/crio.service.d/proxy.conf 2>/dev/null || echo 'No CRI-O proxy config'"
  
  # 检查是否需要代理但未配置
  ssh <node-ip> "curl -v --max-time 5 https://registry-1.docker.io/v2/ 2>&1 | head -20"
  ```
- **超时**: 15s
- **预期输出模式**: 代理配置文件内容
- **判断规则**:
  - 公网仓库不可达但内网仓库可达 → 可能需要代理配置（RC-007）
  - 代理配置存在但格式错误 → RC-007
  - NO_PROXY 未包含内网仓库地址 → 内网仓库请求被错误代理
- **版本差异**: 无

**Step D2.6**: TLS 证书诊断
- **命令**:
  ```bash
  # 检查仓库证书链
  ssh <node-ip> "openssl s_client -connect <registry>:443 -showcerts </dev/null 2>&1 | openssl x509 -noout -dates -subject -issuer"
  
  # 检查节点信任的 CA
  ssh <node-ip> "ls -la /etc/docker/certs.d/ 2>/dev/null || echo 'No docker certs dir'"
  ssh <node-ip> "ls -la /etc/containerd/certs.d/ 2>/dev/null || echo 'No containerd certs dir'"
  
  # 检查系统 CA 是否包含自签名证书
  ssh <node-ip> "ls -la /etc/pki/ca-trust/source/anchors/ 2>/dev/null || ls -la /usr/local/share/ca-certificates/ 2>/dev/null"
  ```
- **超时**: 15s
- **预期输出模式**: 证书信息
- **判断规则**:
  - 证书已过期 → 仓库证书问题（联系仓库管理员）
  - Issuer 为自签名但系统不信任 → RC-006（需要添加 CA）
  - 证书链不完整 → 仓库配置问题
- **版本差异**: 无

**Step D2.7**: 镜像 Manifest 检查
- **命令**:
  ```bash
  # 使用 crane 检查 manifest（推荐）
  # 需要先安装 crane: go install github.com/google/go-containerregistry/cmd/crane@latest
  crane manifest <image>:<tag>
  
  # 或使用 skopeo
  skopeo inspect docker://<image>:<tag>
  
  # 使用 curl 直接调用 Registry API
  # Docker Hub 示例
  TOKEN=$(curl -s "https://auth.docker.io/token?service=registry.docker.io&scope=repository:<namespace>/<image>:pull" | jq -r .token)
  curl -s -H "Authorization: Bearer $TOKEN" \
    -H "Accept: application/vnd.docker.distribution.manifest.v2+json" \
    "https://registry-1.docker.io/v2/<namespace>/<image>/manifests/<tag>"
  ```
- **超时**: 30s
- **预期输出模式**: Manifest JSON
- **判断规则**:
  - 404 Not Found → RC-001（镜像/Tag 不存在）
  - 返回 manifest list → 检查是否包含目标架构
  - 返回单个 manifest → 检查 architecture 字段
- **版本差异**: 无

**Step D2.8**: 容器运行时日志检查
- **命令**:
  ```bash
  # containerd 日志
  ssh <node-ip> "journalctl -u containerd --since '30 minutes ago' --no-pager | grep -i 'pull|auth|image|registry' | tail -50"
  
  # CRI-O 日志
  ssh <node-ip> "journalctl -u crio --since '30 minutes ago' --no-pager | grep -i 'pull|auth|image|registry' | tail -50"
  
  # kubelet 镜像相关日志
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -i 'pull|image' | tail -50"
  ```
- **超时**: 15s
- **预期输出模式**: 日志条目
- **判断规则**:
  - 日志包含 `unauthorized` → RC-002
  - 日志包含 `not found` → RC-001
  - 日志包含 `rate limit` 或 `429` → RC-004
  - 日志包含 `timeout` → RC-003 或 RC-010
  - 日志包含 `x509` → RC-006
- **版本差异**: 无

---

### Phase 3: 高级诊断（低风险，可能需审批）

**Step D3.1**: 仓库端日志检查（如有权限）
- **命令**:
  ```bash
  # Harbor 日志
  # 需要 Harbor 管理员权限
  kubectl logs -n harbor -l component=core --tail=100 | grep -i "<image-name>"
  
  # 或通过 Harbor API
  curl -u admin:PASSWORD "https://harbor.example.com/api/v2.0/projects/<project>/repositories/<repo>/artifacts?page_size=10"
  
  # ACR (阿里云) - 通过控制台或 CLI 检查
  aliyun cr GetRepoTags --RepoNamespace <ns> --RepoName <repo> --Region cn-hangzhou
  
  # ECR (AWS) - 检查镜像是否存在
  aws ecr describe-images --repository-name <repo> --image-ids imageTag=<tag> --region <region>
  
  # GCR/Artifact Registry (Google)
  gcloud artifacts docker images list <location>-docker.pkg.dev/<project>/<repo>
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（只读查询）
- **预期输出模式**: 仓库日志或镜像列表
- **判断规则**:
  - 仓库端显示认证失败 → 凭证问题（RC-002）
  - 仓库端显示镜像不存在 → RC-001
  - 仓库端显示限速 → RC-004
  - 仓库端无异常 → 问题在客户端/网络侧
- **版本差异**: 取决于仓库类型

**Step D3.2**: 镜像签名验证（如使用 Cosign/Notation）
- **命令**:
  ```bash
  # 使用 cosign 验证签名
  cosign verify <image>:<tag> --key <public-key>
  
  # 使用 notation 验证
  notation verify <image>@<digest>
  
  # 检查 Sigstore Rekor 透明日志
  rekor-cli search --sha <image-digest>
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（只读验证）
- **预期输出模式**: 签名验证结果
- **判断规则**:
  - 签名验证失败 → RC-012（镜像被篡改或签名配置问题）
  - 签名不存在但策略要求签名 → RC-009（策略阻断）
- **版本差异**: 无

**Step D3.3**: 带宽与拉取速率分析
- **命令**:
  ```bash
  # 测试下载带宽（使用小文件）
  ssh <node-ip> "curl -o /dev/null -w 'Speed: %{speed_download} bytes/sec\n' https://<registry>/v2/"
  
  # 检查网络带宽使用
  ssh <node-ip> "iftop -i eth0 -t -s 10" 2>/dev/null || echo "iftop not installed"
  
  # 检查镜像大小
  crane manifest <image>:<tag> | jq '.config.size, [.layers[].size] | add'
  ```
- **超时**: 60s
- **风险级别**: 🟢 低（只读检测）
- **预期输出模式**: 下载速度和镜像大小
- **判断规则**:
  - 下载速度极低 + 大镜像 → RC-010（大镜像超时）
  - 下载速度正常但仍超时 → 检查 kubelet imagePullProgressDeadline
- **版本差异**: 无

**Step D3.4**: 多架构 Manifest List 检查
- **命令**:
  ```bash
  # 检查镜像支持的架构
  crane manifest <image>:<tag> | jq '.manifests[]? | {platform: .platform, digest: .digest}'
  
  # 或使用 docker manifest inspect（需要 Docker CLI）
  docker manifest inspect <image>:<tag>
  
  # 检查当前节点架构
  kubectl get node <node-name> -o jsonpath='{.status.nodeInfo.architecture}'
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读检查）
- **预期输出模式**: 支持的平台架构列表
- **判断规则**:
  - 不包含 `linux/arm64` 但节点为 ARM → RC-008
  - 不包含 `linux/amd64` 但节点为 x86 → RC-008
  - 仅有 `windows` 架构但 Linux 节点 → RC-008
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 风险级别 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|---------|
| RC-001 | **镜像名称/Tag 错误** — 配置了不存在的镜像名、命名空间、Tag 或 digest，仓库返回 404 | ~25% | 🟢 | D1.2 镜像名异常；D2.2 返回 manifest unknown；D3.1 仓库确认镜像不存在 | pod-fta: BE-image-not-found |
| RC-002 | **私有仓库认证失败** — 用户名/密码错误、token 过期、权限不足，仓库返回 401 | ~20% | 🟡 | D1.1 Events 含 unauthorized；D2.2 返回 401；D3.1 仓库日志显示认证失败 | pod-fta: BE-auth-failed |
| RC-003 | **仓库网络不可达** — 防火墙、安全组、DNS、路由问题导致节点无法访问仓库 | ~12% | 🟡 | D2.3 连接超时；D2.4 DNS 解析失败；D2.5 代理配置问题 | pod-fta: BE-network-unreachable |
| RC-004 | **仓库限速 (429 Rate Limit)** — Docker Hub 或其他仓库实施拉取限速 | ~10% | 🟡 | D1.1 Events 含 429/toomanyrequests；D2.2 返回 429；D3.1 仓库确认限速 | pod-fta: BE-rate-limit |
| RC-005 | **imagePullSecrets 未配置/配置错误** — Pod 或 ServiceAccount 缺少必要的仓库凭证配置 | ~8% | 🟢 | D1.3 无 imagePullSecrets；D1.4 Secret 内容错误；D2.2 认证失败但凭证未使用 | pod-fta: BE-secret-missing |
| RC-006 | **仓库 TLS 证书不受信任** — 私有仓库使用自签名证书但节点未配置信任 | ~6% | 🟡 | D1.1 Events 含 x509；D2.6 证书验证失败；D2.8 日志含 certificate 错误 | pod-fta: BE-tls-error |
| RC-007 | **代理配置缺失/错误** — 需要代理访问公网仓库但 containerd/CRI-O 未配置代理 | ~5% | 🟡 | D2.3 公网仓库不可达；D2.5 代理配置缺失或 NO_PROXY 配置错误 | pod-fta: BE-proxy-miscfg |
| RC-008 | **多架构镜像不兼容** — 镜像不支持目标节点的 CPU 架构（如 ARM64/AMD64） | ~4% | 🟢 | D1.1 Events 含 no matching manifest；D3.4 确认镜像不支持目标架构 | pod-fta: BE-arch-mismatch |
| RC-009 | **镜像安全策略阻断** — Admission Webhook（Gatekeeper/Kyverno/ImagePolicyWebhook）拒绝镜像 | ~3% | 🔴 | D1.1 Events 含 denied/policy/blocked；Admission 日志确认拒绝 | pod-fta: BE-policy-denied |
| RC-010 | **大镜像拉取超时** — 镜像体积过大，在默认超时时间内无法完成拉取 | ~3% | 🟡 | D1.1 Events 含 timeout；D3.3 确认镜像大小异常；网络带宽受限 | pod-fta: BE-pull-timeout |
| RC-011 | **离线环境镜像同步失败** — Air-Gap 环境中内部仓库缺少所需镜像 | ~2% | 🔴 | D3.1 内部仓库确认镜像不存在；Air-Gap 环境确认无外网访问 | pod-fta: BE-airgap-sync-fail |
| RC-012 | **镜像 digest 不匹配 / 被篡改** — 拉取的镜像 hash 与预期不符，可能存在安全风险 | ~2% | ⚫ | D1.1 Events 含 digest mismatch；D3.2 签名验证失败 | pod-fta: BE-image-tampered |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 修正镜像名称/Tag
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认当前配置的镜像名
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}'
  
  # 验证正确的镜像是否存在
  crane manifest <correct-image>:<correct-tag> || skopeo inspect docker://<correct-image>:<correct-tag>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 对于 Deployment
  kubectl set image deployment/<deployment-name> <container-name>=<correct-image>:<correct-tag> -n <namespace>
  
  # 对于直接创建的 Pod（需要删除重建）
  kubectl delete pod <pod-name> -n <namespace>
  # 然后修改 Pod YAML 中的镜像并重新 apply
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l <selector> -w
  # 预期: Pod 状态从 ContainerCreating → Running
  
  kubectl describe pod <new-pod-name> -n <namespace> | grep -A5 "Events:"
  # 预期: 无 ImagePullBackOff/ErrImagePull 事件
  ```
- **回滚命令**:
  ```bash
  kubectl set image deployment/<deployment-name> <container-name>=<previous-image>:<previous-tag> -n <namespace>
  ```

#### REM-002: 创建/更新 imagePullSecrets
- **适用根因**: RC-002, RC-005
- **前置检查**:
  ```bash
  # 确认需要访问的仓库
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}' | sed 's|/.*||'
  
  # 确认现有 Secret（如有）
  kubectl get secrets -n <namespace> -o name | grep -i docker
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 创建新的 imagePullSecret
  kubectl create secret docker-registry <secret-name> \
    --docker-server=<registry-url> \
    --docker-username=<username> \
    --docker-password=<password> \
    --docker-email=<email> \
    -n <namespace>
  
  # 更新 Deployment 使用该 Secret
  kubectl patch deployment <deployment-name> -n <namespace> -p '{"spec":{"template":{"spec":{"imagePullSecrets":[{"name":"<secret-name>"}]}}}}'
  
  # 或直接编辑 Deployment
  kubectl edit deployment <deployment-name> -n <namespace>
  # 添加:
  # spec:
  #   template:
  #     spec:
  #       imagePullSecrets:
  #       - name: <secret-name>
  ```
- **后置验证**:
  ```bash
  # 验证新 Pod 创建成功
  kubectl get pods -n <namespace> -l <selector> -w
  # 预期: Pod 进入 Running 状态
  
  # 验证 Secret 已被挂载
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl delete secret <secret-name> -n <namespace>
  kubectl patch deployment <deployment-name> -n <namespace> -p '{"spec":{"template":{"spec":{"imagePullSecrets":null}}}}'
  ```

#### REM-003: 设置 ServiceAccount 默认 imagePullSecrets
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  # 检查 namespace 中使用最多的 ServiceAccount
  kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.spec.serviceAccountName}{"\n"}{end}' | sort | uniq -c
  
  # 确认 Secret 已存在
  kubectl get secret <secret-name> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 为 ServiceAccount 添加默认 imagePullSecrets
  kubectl patch serviceaccount <sa-name> -n <namespace> -p '{"imagePullSecrets":[{"name":"<secret-name>"}]}'
  
  # 或编辑 ServiceAccount
  kubectl edit sa <sa-name> -n <namespace>
  # 添加:
  # imagePullSecrets:
  # - name: <secret-name>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 验证 SA 配置
  kubectl get sa <sa-name> -n <namespace> -o yaml | grep -A2 imagePullSecrets
  
  # 删除现有 Pod 让其使用新配置重建
  kubectl delete pod <pod-name> -n <namespace>
  # 等待新 Pod 创建并验证状态
  kubectl get pods -n <namespace> -l <selector> -w
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch serviceaccount <sa-name> -n <namespace> --type=json -p='[{"op":"remove","path":"/imagePullSecrets"}]'
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 配置仓库镜像/代理 (Registry Mirror)
- **适用根因**: RC-004, RC-007
- **影响说明**: 修改 containerd 配置需要重启 containerd 服务，可能导致节点上容器短暂中断。建议先 drain 节点或在维护窗口执行。
- **审批提示**: "建议为节点配置仓库镜像/代理以解决 Docker Hub 限速或网络问题。该操作需要修改 containerd 配置并重启服务，节点上容器可能短暂中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前 containerd 配置
  ssh <node-ip> "cat /etc/containerd/config.toml | grep -A10 '\[plugins.*registry\]'"
  
  # 确认镜像仓库可达
  ssh <node-ip> "curl -v https://<mirror-registry>/v2/"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 方式 1: 使用 containerd certs.d 配置（推荐，无需重启）
  ssh <node-ip> "mkdir -p /etc/containerd/certs.d/docker.io"
  ssh <node-ip> "cat > /etc/containerd/certs.d/docker.io/hosts.toml << 'EOF'
  server = \"https://registry-1.docker.io\"
  
  [host.\"https://mirror.ccs.tencentyun.com\"]
    capabilities = [\"pull\", \"resolve\"]
  
  [host.\"https://registry.cn-hangzhou.aliyuncs.com\"]
    capabilities = [\"pull\", \"resolve\"]
  EOF"
  
  # 方式 2: 修改 containerd 主配置（需要重启）
  ssh <node-ip> "cat >> /etc/containerd/config.toml << 'EOF'
  [plugins.\"io.containerd.grpc.v1.cri\".registry.mirrors]
    [plugins.\"io.containerd.grpc.v1.cri\".registry.mirrors.\"docker.io\"]
      endpoint = [\"https://mirror.ccs.tencentyun.com\", \"https://registry-1.docker.io\"]
  EOF"
  
  # 重启 containerd（如使用方式 2）
  ssh <node-ip> "systemctl restart containerd"
  ```
- **后置验证**:
  ```bash
  # 验证配置生效
  ssh <node-ip> "crictl pull docker.io/library/busybox:latest"
  # 预期: 拉取成功
  
  # 验证节点状态
  kubectl get node <node-name>
  # 预期: Ready
  ```
- **回滚命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 删除 certs.d 配置
  ssh <node-ip> "rm -rf /etc/containerd/certs.d/docker.io"  # ⚠️ 删除系统/数据文件
  
  # 或恢复 containerd 配置
  ssh <node-ip> "cp /etc/containerd/config.toml.bak /etc/containerd/config.toml && systemctl restart containerd"
  ```

#### REM-005: 添加私有 CA 证书信任
- **适用根因**: RC-006
- **影响说明**: 添加 CA 证书到系统信任存储需要更新证书存储，可能需要重启 containerd。操作本身风险较低但需要节点访问权限。
- **审批提示**: "建议为节点添加私有仓库的 CA 证书以解决 TLS 握手失败问题。该操作需要修改系统证书存储。是否批准？"
- **前置检查**:
  ```bash
  # 获取仓库证书
  ssh <node-ip> "openssl s_client -connect <registry>:443 -showcerts </dev/null 2>&1 | openssl x509 -outform PEM > /tmp/registry-ca.crt"
  
  # 验证证书
  ssh <node-ip> "openssl x509 -in /tmp/registry-ca.crt -noout -subject -issuer"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 方式 1: 使用 containerd certs.d（推荐，无需重启）
  ssh <node-ip> "mkdir -p /etc/containerd/certs.d/<registry-host>"
  ssh <node-ip> "cp /tmp/registry-ca.crt /etc/containerd/certs.d/<registry-host>/ca.crt"
  ssh <node-ip> "cat > /etc/containerd/certs.d/<registry-host>/hosts.toml << 'EOF'
  server = \"https://<registry-host>\"
  
  [host.\"https://<registry-host>\"]
    ca = \"/etc/containerd/certs.d/<registry-host>/ca.crt\"
  EOF"
  
  # 方式 2: 添加到系统信任存储（需要更新证书存储）
  # RHEL/CentOS
  ssh <node-ip> "cp /tmp/registry-ca.crt /etc/pki/ca-trust/source/anchors/ && update-ca-trust"
  
  # Ubuntu/Debian
  ssh <node-ip> "cp /tmp/registry-ca.crt /usr/local/share/ca-certificates/ && update-ca-certificates"
  
  # 重启 containerd 以加载新证书
  ssh <node-ip> "systemctl restart containerd"
  ```
- **后置验证**:
  ```bash
  # 测试 TLS 连接
  ssh <node-ip> "curl -v https://<registry>/v2/"
  # 预期: 无 SSL 错误
  
  # 测试镜像拉取
  ssh <node-ip> "crictl pull <registry>/<image>:<tag>"
  # 预期: 拉取成功
  ```
- **回滚命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

  ```bash
  # 删除 certs.d 配置
  ssh <node-ip> "rm -rf /etc/containerd/certs.d/<registry-host>"  # ⚠️ 删除系统/数据文件
  
  # 或从系统信任存储移除
  ssh <node-ip> "rm /etc/pki/ca-trust/source/anchors/registry-ca.crt && update-ca-trust"
  ```

#### REM-006: 配置容器运行时代理
- **适用根因**: RC-007
- **影响说明**: 配置代理需要修改 containerd/CRI-O 的 systemd 服务配置并重启服务。在重启期间，节点上的容器操作将短暂中断。
- **审批提示**: "建议为节点配置容器运行时代理以访问公网镜像仓库。该操作需要修改 systemd 配置并重启 containerd，节点上容器可能短暂中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认代理服务器可达
  ssh <node-ip> "curl -x http://<proxy>:<port> -v https://registry-1.docker.io/v2/"
  
  # 确认当前无代理配置
  ssh <node-ip> "systemctl show containerd --property=Environment"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 创建 containerd 代理配置
  ssh <node-ip> "mkdir -p /etc/systemd/system/containerd.service.d"
  ssh <node-ip> "cat > /etc/systemd/system/containerd.service.d/proxy.conf << 'EOF'
  [Service]
  Environment=\"HTTP_PROXY=http://<proxy>:<port>\"
  Environment=\"HTTPS_PROXY=http://<proxy>:<port>\"
  Environment=\"NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,.cluster.local,.svc,<internal-registry>\"
  EOF"
  
  # 重新加载 systemd 并重启 containerd
  ssh <node-ip> "systemctl daemon-reload && systemctl restart containerd"
  
  # 对于 CRI-O
  ssh <node-ip> "mkdir -p /etc/systemd/system/crio.service.d"
  ssh <node-ip> "cat > /etc/systemd/system/crio.service.d/proxy.conf << 'EOF'
  [Service]
  Environment=\"HTTP_PROXY=http://<proxy>:<port>\"
  Environment=\"HTTPS_PROXY=http://<proxy>:<port>\"
  Environment=\"NO_PROXY=localhost,127.0.0.1,10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,.cluster.local,.svc,<internal-registry>\"
  EOF"
  ssh <node-ip> "systemctl daemon-reload && systemctl restart crio"
  ```
- **后置验证**:
  ```bash
  # 验证代理配置生效
  ssh <node-ip> "systemctl show containerd --property=Environment"
  
  # 测试镜像拉取
  ssh <node-ip> "crictl pull docker.io/library/busybox:latest"
  # 预期: 拉取成功
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  ssh <node-ip> "rm /etc/systemd/system/containerd.service.d/proxy.conf && systemctl daemon-reload && systemctl restart containerd"
  ```

#### REM-007: Docker Hub 限速应对方案
- **适用根因**: RC-004
- **影响说明**: 配置镜像缓存或使用认证账号可以绕过 Docker Hub 限速。需要创建 imagePullSecrets 并配置到 ServiceAccount。
- **审批提示**: "建议配置 Docker Hub 认证以提高拉取限额（从匿名 100 次/6h 提升到认证 200 次/6h）。需要创建包含 Docker Hub 凭证的 Secret。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前限速情况
  ssh <node-ip> "curl -s -D - -o /dev/null https://registry-1.docker.io/v2/ | grep -i ratelimit"
  
  # 确认 Docker Hub 账号有效
  curl -u "<username>:<password>" https://hub.docker.com/v2/users/login
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 1: 创建 Docker Hub 认证 Secret
  kubectl create secret docker-registry dockerhub-auth \
    --docker-server=docker.io \
    --docker-username=<dockerhub-username> \
    --docker-password=<dockerhub-password> \
    -n <namespace>
  
  # 配置到 ServiceAccount
  kubectl patch sa default -n <namespace> -p '{"imagePullSecrets":[{"name":"dockerhub-auth"}]}'
  
  # 方案 2: 配置镜像缓存（参见 REM-004）
  
  # 方案 3: 使用 pull-through cache（Harbor）
  # 需要在 Harbor 中配置 Docker Hub 为 registry endpoint
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除并重建 Pod
  kubectl delete pod <pod-name> -n <namespace>
  
  # 验证新 Pod 创建成功
  kubectl get pods -n <namespace> -l <selector> -w
  
  # 检查限速头信息
  ssh <node-ip> "curl -s -D - -o /dev/null https://registry-1.docker.io/v2/ | grep -i ratelimit"
  # 预期: RateLimit-Limit: 200（认证用户）
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch sa default -n <namespace> --type=json -p='[{"op":"remove","path":"/imagePullSecrets"}]'
  kubectl delete secret dockerhub-auth -n <namespace>
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: 紧急镜像手动导入
- **适用根因**: RC-003, RC-011
- **影响说明**: 在无法从仓库拉取镜像的紧急情况下，手动将镜像导入到节点。这是临时应急措施，需要有镜像的离线 tar 包。
- **操作步骤**:
  1. **获取镜像 tar 包**（在有网络的环境）:
     ```bash
     # 使用 docker save
     docker pull <image>:<tag>
     docker save <image>:<tag> -o image.tar
     
     # 或使用 crane
     crane pull <image>:<tag> image.tar
     
     # 或使用 skopeo
     skopeo copy docker://<image>:<tag> docker-archive:image.tar
     ```
  2. **传输到目标节点**:
     ```bash
     scp image.tar <node-ip>:/tmp/
     ```
  3. **导入到容器运行时**:
     ```bash
     # containerd
     ssh <node-ip> "ctr -n k8s.io image import /tmp/image.tar"
     
     # 或使用 crictl（需要转换格式）
     # CRI-O
     ssh <node-ip> "podman load -i /tmp/image.tar"
     ```
  4. **验证导入成功**:
     ```bash
     ssh <node-ip> "crictl images | grep <image>"
     ```
  5. **重建 Pod 使用本地镜像**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 确保 imagePullPolicy 为 IfNotPresent 或 Never
     kubectl delete pod <pod-name> -n <namespace>
     kubectl get pods -n <namespace> -l <selector> -w
     ```
- **安全检查**:
  - 确认镜像来源可信
  - 验证镜像 digest（如有）
  - 记录导入操作用于审计
- **回滚方案**:
  ```bash
  # 删除导入的镜像
  ssh <node-ip> "ctr -n k8s.io image rm <image>:<tag>"
  # 或
  ssh <node-ip> "crictl rmi <image>:<tag>"
  ```

#### REM-009: 禁用/调整镜像安全策略
- **适用根因**: RC-009
- **影响说明**: 禁用或调整镜像安全策略可能降低集群安全性。应仅在紧急情况下临时调整，并尽快修复根本原因（如添加镜像到白名单）。
- **操作步骤**:
  1. **确认阻断策略**:
     ```bash
     # Gatekeeper 示例
     kubectl get constraints --all-namespaces
     kubectl describe K8sAllowedRepos <constraint-name>
     
     # Kyverno 示例
     kubectl get cpol --all-namespaces
     kubectl describe cpol <policy-name>
     ```
  2. **临时豁免（推荐）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     # Gatekeeper: 添加 namespace 到 exemptNamespaces
     kubectl patch constraint <constraint-name> --type=merge -p '{
       "spec": {
         "match": {
           "excludedNamespaces": ["<emergency-namespace>"]
         }
       }
     }'
     
     # Kyverno: 添加 exclude 规则
     kubectl patch cpol <policy-name> --type=merge -p '{
       "spec": {
         "exclude": {
           "namespaces": ["<emergency-namespace>"]
         }
       }
     }'
     ```
  3. **紧急禁用（最后手段）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 禁用 Gatekeeper admission webhook（危险）
     kubectl delete validatingwebhookconfiguration gatekeeper-validating-webhook-configuration
     
     # 禁用 Kyverno admission webhook（危险）
     kubectl delete validatingwebhookconfiguration kyverno-resource-validating-webhook-cfg
     ```
  4. **验证 Pod 可以创建**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete pod <pod-name> -n <namespace>
     kubectl get pods -n <namespace> -l <selector> -w
     ```
- **安全检查**:
  - 记录所有策略变更
  - 设置提醒在问题解决后恢复策略
  - 评估安全影响并通知安全团队
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复 Gatekeeper 策略
  kubectl patch constraint <constraint-name> --type=merge -p '{
    "spec": {
      "match": {
        "excludedNamespaces": null
      }
    }
  }'
  
  # 重新启用 webhook
  # 需要重新 apply webhook 配置
  kubectl apply -f gatekeeper-webhooks.yaml
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: Air-Gap 环境镜像同步方案部署
- **适用根因**: RC-011
- **审批要求**: 需要高级 SRE + 安全团队审批
- **数据备份**: 确保有镜像来源的清单和 digest 验证
- **操作步骤**:
  1. **建立镜像清单**:
     ```bash
     # 收集所有需要的镜像
     kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | sort -u > images.txt
     kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.spec.initContainers[*].image}{"\n"}{end}' | sort -u >> images.txt
     ```
  2. **在联网环境拉取并打包镜像**:
     ```bash
     # 使用 skopeo 批量同步到本地目录
     while read image; do
       skopeo copy docker://${image} dir:./images/${image//\//_}
     done < images.txt
     
     # 或使用 crane
     while read image; do
       crane pull ${image} ./images/${image//\//_}.tar
     done < images.txt
     ```
  3. **传输到 Air-Gap 环境**:
     ```bash
     # 打包所有镜像
     tar -czf all-images.tar.gz ./images/
     
     # 通过安全介质传输到内网
     ```
  4. **导入到内部 Harbor**:
     ```bash
     # 解压镜像
     tar -xzf all-images.tar.gz
     
     # 使用 skopeo 推送到 Harbor
     while read image; do
       src_dir=./images/${image//\//_}
       dest=harbor.internal.com/library/${image#*/}
       skopeo copy dir:${src_dir} docker://${dest}
     done < images.txt
     ```
  5. **配置集群使用内部仓库**:
     ```bash
     # 更新 Deployment 的镜像前缀
     kubectl set image deployment/<name> <container>=harbor.internal.com/library/<image>:<tag> -n <namespace>
     
     # 或配置 containerd 镜像重写
     ```
  6. **验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete pod <pod-name> -n <namespace>
     kubectl get pods -n <namespace> -l <selector> -w
     ```
- **回滚方案**:
  - 保留原始镜像配置的备份
  - 如需回滚，恢复原始镜像路径（前提是网络恢复）

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 确认 Pod 状态恢复正常
kubectl get pods -n <namespace> -l <selector>
# 预期: STATUS 列显示 Running 或 Completed（对于 Job）

# V2: 确认无 ImagePullBackOff/ErrImagePull 事件
kubectl describe pod <pod-name> -n <namespace> | grep -A10 "Events:"
# 预期: 无 Warning 类型的镜像拉取事件

# V3: 确认镜像已成功拉取
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[*].imageID}'
# 预期: 输出镜像 ID（sha256:xxx）

# V4: 确认容器已启动
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[*].state}'
# 预期: running 状态

# V5: 检查节点上的镜像缓存
NODE=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}')
kubectl debug node/${NODE} -it --image=busybox -- crictl images | grep "<image-keyword>"
# 预期: 镜像存在于本地缓存
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pod 状态 | `kubectl get pods -n <namespace> -l <selector>` | 所有 Pod 保持 Running | 任何 Pod 再次进入 ImagePullBackOff |
| 镜像拉取错误 | `kubelet_image_pull_operations_failed_total` | 保持稳定或为 0 | 指标持续增加 |
| 镜像拉取耗时 | `container_runtime_pull_duration_seconds` | 正常范围（<60s） | P99 > 120s |
| 仓库响应时间 | `curl -o /dev/null -w '%{time_total}' https://<registry>/v2/` | <2s | >10s |
| Secret 有效性 | 检查 imagePullSecrets 是否仍有效 | 持续有效 | 云托管凭证即将过期 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 受影响的 Pod 均已恢复 Running 状态
- [ ] Pod Events 中无新的 ImagePullBackOff/ErrImagePull 警告
- [ ] 镜像已成功拉取并缓存到节点
- [ ] 容器进程正常运行（可执行 `kubectl exec` 验证）
- [ ] 如涉及认证修复，imagePullSecrets 已正确配置
- [ ] 如涉及网络/代理修复，其他镜像拉取也正常
- [ ] 相同配置的新 Pod 可以正常创建

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 新 Pod 创建 | `kubectl get events --field-selector reason=Failed --all-namespaces` | 每小时 | 如有新的镜像拉取失败 → 重新诊断 |
| 仓库凭证有效性 | 检查 Secret 中的 token 过期时间 | 每日 | 临近过期 → 主动刷新凭证 |
| Docker Hub 限速状态 | 检查 RateLimit-Remaining 头 | 每 4 小时 | 限额接近耗尽 → 考虑扩展方案 |
| 镜像策略变更 | `kubectl get constraints,cpol --all-namespaces` | 每日 | 策略更新可能导致新的阻断 |
| 证书有效期 | `openssl x509 -in <cert> -noout -enddate` | 每日 | 证书临近过期 → 主动轮转 |
| 仓库健康状态 | Harbor/ACR 等仓库的健康检查 | 每小时 | 仓库异常 → 联系仓库管理员 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **影响扩大** | 初始分级为 P2 但受影响 Pod 数量超过 10 个 | 诊断过程中发现更多失败 Pod |
| **未知根因** | 完成 Phase 1-3 所有诊断但无法匹配任何已知根因 | 所有诊断步骤均无明确异常 |
| **安全事件** | 发现镜像被篡改或 digest 不匹配 | D3.2 签名验证失败 |
| **仓库不可用** | 仓库完全不响应且非网络问题 | D2.3 确认网络通但仓库 HTTP 无响应 |

### 8.2 升级消息模板

```
# 🟢 低风险：只读/信息收集，通常无副作用
【{severity}】镜像拉取问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {affected_pod_count} 个 Pod 无法拉取镜像，持续 {duration}
- 关键错误: {error_type} (如: unauthorized / manifest unknown / timeout)
- 影响范围:
  - 受影响 Pod: {affected_pod_list}
  - 涉及镜像: {image_list}
  - 涉及仓库: {registry_list}
- 已完成诊断:
  - Phase 1 kubectl 诊断: {phase1_summary}
  - Phase 2 节点级诊断: {phase2_summary}
  - Phase 3 仓库级诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-IMAGE-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及依据
3. **可能的根因假设**: 基于已有证据提出的假设及置信度
4. **关键资源快照**:
   ```bash
   # Pod 描述
   kubectl describe pod <pod-name> -n <namespace> > pod-describe.txt
   
   # Pod Events
   kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> > pod-events.txt
   
   # imagePullSecrets 配置（脱敏）
   kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}' > pull-secrets.txt
   
   # 节点镜像缓存
   kubectl debug node/<node> -- crictl images > node-images.txt
   
   # containerd 日志
   ssh <node-ip> "journalctl -u containerd --since '1 hour ago' --no-pager | grep -i pull" > containerd-logs.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| 容器运行时 | containerd 1.6+, CRI-O 1.28+ | containerd 1.7+, CRI-O 1.29+ | containerd 1.7+, CRI-O 1.30+ | containerd 1.7+, CRI-O 1.31+ | containerd 2.0+, CRI-O 1.32+ |
| kubectl debug node/ | GA | GA | GA | GA | GA |
| ImageVolume (OCI artifact) | alpha | alpha | alpha | beta | beta |
| Sidecar Containers | alpha | beta | beta | GA | GA |
| RuntimeClass | GA | GA | GA | GA | GA |
| Image credential providers | GA | GA | GA | GA | GA |
| imagePullPolicy: Always 缓存优化 | 基础 | 改进 | 改进 | 增强 | 增强 |
| CRI image pull 超时配置 | 固定 | 固定 | 可配置 (alpha) | 可配置 (beta) | 可配置 (GA) |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug node/<name>` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `crictl pull` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `ctr -n k8s.io image` | 支持 | 支持 | 支持 | 支持 | containerd 2.0 有变化 |
| `kubectl get events --field-selector` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `--image-pull-progress-deadline` (kubelet) | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Pod | v1 (core) | v1 | v1 | v1 | v1 |
| Secret | v1 (core) | v1 | v1 | v1 | v1 |
| ServiceAccount | v1 (core) | v1 | v1 | v1 | v1 |
| Event | events.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 仓库特定差异

| 仓库 | 认证方式 | 特殊配置 | Rate Limit |
|------|---------|---------|------------|
| **Docker Hub** | docker-registry Secret | 镜像前缀 docker.io/ 可省略 | 匿名 100/6h, 认证 200/6h, 付费无限 |
| **Harbor** | docker-registry Secret | 支持 robot account, OIDC | 取决于部署配置 |
| **ACR (阿里云)** | docker-registry Secret / 实例 RAM 角色 | 支持 VPC 端点 | 企业版无限，标准版有限 |
| **ECR (AWS)** | ecr-credential-provider / docker-registry Secret | Token 12h 过期，需自动刷新 | 无限制 |
| **GCR / Artifact Registry** | gcr-credential-provider / docker-registry Secret | 支持 Workload Identity | 配额制 |
| **GHCR (GitHub)** | docker-registry Secret (PAT) | 支持 GITHUB_TOKEN | 取决于账户类型 |

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 DNS 解析失败误判为认证问题** | 错误信息包含 "connection refused" 或类似网络错误 | 仓库域名无法解析，根本不是认证问题 | 先执行 D2.4 DNS 诊断再判断认证问题；D2.3 应在 D1.4 之前 |
| **将限速误判为网络不通** | 镜像拉取超时或连接被重置 | Docker Hub 429 限速，但错误信息不够明确 | 检查 HTTP 响应头 RateLimit-Remaining；使用 curl -v 获取完整响应 |
| **将 imagePullPolicy 问题误判为镜像不存在** | Pod 报告 ErrImagePull | imagePullPolicy=Never 但节点无本地镜像 | D1.5 中检查 imagePullPolicy；D1.6 确认本地缓存 |
| **将多架构问题误判为镜像损坏** | "manifest unknown" 或 "no matching manifest" | 镜像存在但不支持目标架构 | 使用 D3.4 检查 manifest list；确认节点架构 |
| **将代理配置问题误判为 TLS 错误** | TLS 握手失败或证书验证错误 | 代理服务器返回了自己的证书，而非仓库证书 | D2.5 确认代理配置；检查是否需要将仓库加入 NO_PROXY |
| **将临时网络抖动误判为持久问题** | 偶发的 timeout 错误 | 网络临时不稳定，重试后可成功 | 等待 kubelet 重试（默认会 backoff 重试）；观察是否自动恢复 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| 镜像仓库排障深度指南 | `故障诊断/27-image-registry-troubleshooting.md` | 超出本 Skill 覆盖的深度仓库问题 |
| 容器镜像管理 | `容器运行时/` | 镜像构建、推送、安全扫描全流程 |
| Docker 基础 | `容器运行时/` | containerd 与 Docker 架构差异 |
| 网络故障排查 | `SKILL-NET-001` | 仓库网络不通的深度诊断 |
| 证书管理 | `SKILL-SEC-001` | TLS 证书问题的详细诊断 |
| Pod 调度 | `SKILL-POD-002` | 镜像拉取后 Pod 仍 Pending 的问题 |
| 供应链安全 | `安全/` | 镜像签名、验证、SBOM |
| Air-Gap 部署 | `故障诊断/` | 离线环境完整解决方案 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、10 个修复操作 | 镜像拉取问题占 Pod 异常工单 20-30%，优先级较高 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **OCI 工件拉取**: 使用 ImageVolume 挂载 OCI artifact 的问题诊断
2. **Helm Chart 镜像问题**: Helm 部署时镜像配置错误的快速定位
3. **跨云仓库同步**: 多云环境中镜像同步的问题诊断
4. **仓库高可用问题**: Harbor/Registry HA 集群故障诊断
5. **GPU 容器镜像**: NVIDIA Container Toolkit 相关的镜像问题
6. **Windows 容器镜像**: Windows 节点特有的镜像拉取问题


<!-- risk-assessed -->
