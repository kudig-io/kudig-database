---
title: Pod ImagePullBackOff 与容器创建失败诊断
description: 针对 Pod 镜像拉取失败（ImagePullBackOff/ErrImagePull）和容器创建阶段异常的完整诊断技能，覆盖认证失败、镜像不存在、网络不通、CNI/存储挂载等问题
summary: 镜像拉取和容器创建是 Pod 启动的关键路径，本技能覆盖 ImagePullBackOff、CreateContainerError、Init:Error 等启动阶段故障
category: skill
tags:
- k8s
- pod
- troubleshooting
- imagepull
- registry
- container
- cni
- volume
- init-container
sources:
- 故障诊断/高级排障/structural-05-workloads/01-pod-troubleshooting.md
- 故障诊断/核心排障/08-pod-comprehensive-troubleshooting.md
- 故障诊断/FTA故障树/list/pod-fta.md
- code/kubernetes-release-1.20/pkg/apis/core/types.go
- code/kubernetes-release-1.34/pkg/apis/core/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- ImagePullBackOff 怎么解决
- 镜像拉取失败怎么排查
- Pod 卡在 ContainerCreating 怎么办
- Init 容器报错怎么处理
- ErrImagePull 什么原因
trigger_keywords:
- ImagePullBackOff
- ErrImagePull
- ContainerCreating
- CreateContainerError
- Init:Error
- 镜像拉取失败
- unauthorized
- manifest unknown
- pull access denied
prerequisites:
- kubectl-basics
- pod-lifecycle
- container-registry-basics
skill_id: SKILL-POD-003
skill_name: Pod ImagePullBackOff 与容器创建失败诊断
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-3 -> IE-3.2/IE-3.3 -> BE-3.5~BE-3.10
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod ImagePullBackOff 与容器创建失败诊断

> **Skill ID**: SKILL-POD-003
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批
> **预计修复时间**: 5-15 分钟
> **FTA 路径**: TE-3 → IE-3.2 (镜像拉取) / IE-3.3 (容器创建)

---

## 1. 概述

Pod 在调度成功后进入容器创建阶段，此阶段的故障主要表现为：

| 状态 | 含义 | 常见原因 |
|------|------|---------|
| **ImagePullBackOff** | 镜像拉取失败，退避重试 | 镜像不存在/凭证错误/网络不通/限流 |
| **ErrImagePull** | 首次拉取即失败 | 同上（ImagePullBackOff 的前置状态） |
| **ContainerCreating** | 容器创建中（卡住） | CNI 配置/存储挂载/镜像层解压 |
| **CreateContainerError** | 容器创建失败 | SecurityContext 冲突/运行时错误 |
| **Init:Error** | Init 容器执行失败 | 初始化逻辑错误/依赖未就绪 |
| **Init:CrashLoopBackOff** | Init 容器反复崩溃 | 同 CrashLoopBackOff 诊断 |

---

## 2. 症状识别

| # | 症状描述（错误消息/事件原文） | 检测方法 | 置信度 | 排除条件 |
|---|---------------------------|---------|:---:|---------|
| S1 | Pod STATUS 为 `ImagePullBackOff` / `ErrImagePull` | `kubectl get pod` STATUS 列 | 0.95 | 节点 DiskPressure 导致拉取失败 → 转 [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力]] |
| S2 | Events 出现 `Failed to pull image ...: not found` / `manifest unknown` | `kubectl describe pod` Events 段 | 0.95 | 镜像刚推送需确认仓库同步延迟 |
| S3 | Events 出现 `pull access denied` / `unauthorized: authentication required` | `kubectl describe pod` Events 段 | 0.95 | imagePullSecrets 存在时需验证 secret 内容有效性 |
| S4 | Events 出现 `dial tcp ...: i/o timeout`（仓库网络不通） | `kubectl describe pod` Events 段 | 0.85 | 仅单节点失败属节点网络问题而非仓库故障 |
| S5 | Pod 长时间 `ContainerCreating` + Events `FailedCreatePodSandBox` | `kubectl describe pod` Events 段（CNI 报错文本） | 0.90 | 网络插件故障 → 转网络技能集/节点 CNI 排查 |
| S6 | Events 出现 `FailedMount: MountVolume.SetUp failed` / `FailedAttachVolume` | `kubectl describe pod` Events 段 | 0.90 | 存储卷问题 → 转存储技能集（csi-storage） |
| S7 | Pod STATUS 为 `CreateContainerConfigError` | `kubectl get pod` STATUS + Events `configmap/secret ... not found` | 0.95 | 引用对象存在时检查 key 级缺失 |
| S8 | Pod STATUS 为 `Init:Error` / `Init:CrashLoopBackOff` | `kubectl get pod` STATUS 列 | 0.90 | 主容器 CrashLoop → 转 [01-pod-crashloop-oomkilled.md](01-pod-crashloop-oomkilled.md) |
| S9 | Events 出现 `x509: certificate signed by unknown authority` | `kubectl describe pod` Events 段（私有仓库 TLS） | 0.90 | 节点时钟偏移也会报 x509，先校验 NTP |

**工单关键词映射**：`ImagePullBackOff`、`ErrImagePull`、`镜像拉不下来`、`ContainerCreating 卡住`、`unauthorized`、`sandbox`、`FailedMount` → 触发本技能。

---

## 3. ImagePullBackOff 诊断

### 3.1 快速诊断

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查镜像名称和 tag
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[*].image}'

# Step 2: 检查 imagePullSecrets
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'
kubectl get secret <secret> -n <namespace> -o jsonpath='{.type}'

# Step 3: 检查事件中的详细错误
kubectl describe pod <pod> -n <namespace> | grep -A 5 "Failed"
```

### 3.2 错误信息决策表

| 错误信息 | 根因 | 修复 |
|---------|------|------|
| `repository does not exist` | 镜像名称/路径错误 | 核实镜像地址拼写 |
| `manifest unknown` / `not found` | Tag 不存在 | 核实 Tag 是否已推送 |
| `unauthorized` / `pull access denied` | 认证失败 | 检查 imagePullSecret |
| `timeout` / `connection refused` | 网络不通 | 检查节点到 Registry 的连通性 |
| `too many requests` / `rate limit` | Registry 限流 | 使用镜像缓存/切换仓库 |
| `x509: certificate signed by unknown authority` | TLS 证书问题 | 配置 CA 证书或使用受信仓库 |
| `no space left on device` | 节点磁盘满 | 清理镜像/扩容磁盘 |

### 3.3 常见错误消息与事件日志速查

> 以下错误消息和事件日志是镜像拉取/容器创建失败场景的高频诊断线索。Agent 在采集 Events 后可直接匹配本表快速路由。

#### 关键 Events（`kubectl describe pod` / `kubectl get events`）

| 事件 Reason | 事件 Message 模式 | 含义 | 检测命令 | 路由 |
|-------------|------------------|------|---------|------|
| `Failed` | `Failed to pull image "<image>": rpc error: code = NotFound desc = manifest for <image> not found` | 镜像 Tag 不存在 | `kubectl get events -n <ns> --field-selector reason=Failed,involvedObject.name=<pod>` | → RC-001 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = Error response from daemon: unauthorized: authentication required` | 认证失败（无凭证或凭证错误） | 同上 | → RC-002 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = Error response from daemon: pull access denied for <repo>, repository does not exist or may require 'docker login'` | 仓库不存在或需登录 | 同上 | → RC-001/RC-002 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = context deadline exceeded` | 拉取超时（网络慢/镜像大） | 同上 | → RC-003 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = Error response from daemon: toomanyrequests: You have reached your pull rate limit` | Docker Hub 限流 | 同上 | → RC-004 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = x509: certificate signed by unknown authority` | TLS 证书不受信任 | 同上 | → 配置 CA/使用受信仓库 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = no such host` | DNS 解析失败（Registry 域名不可达） | 同上 | → RC-003 网络 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Canceled desc = context canceled` | 拉取被取消（Pod 被删除/超时） | 同上 | → 检查 Pod 生命周期 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = write /var/lib/containerd/...: no space left on device` | 节点磁盘空间不足 | 同上 | → 清理镜像/扩容磁盘 |
| `Failed` | `Failed to pull image "<image>": rpc error: code = Unknown desc = unexpected status from HEAD request to ...: 403 Forbidden` | Registry 拒绝访问（IP 封禁/权限） | 同上 | → RC-002/RC-003 |
| `BackOff` | `Back-off pulling image "<image>"` | 拉取失败进入退避（ImagePullBackOff 状态） | `kubectl get events -n <ns> --field-selector reason=BackOff` | → 查上方具体 Failed 事件 |
| `FailedCreatePodSandBox` | `Failed to create pod sandbox: rpc error: code = Unknown desc = failed to setup network for sandbox: plugin type="<cni>" failed (add): ...` | CNI 网络配置失败导致沙箱创建失败 | `kubectl get events -n <ns> --field-selector reason=FailedCreatePodSandBox` | → 转 CNI 排查 |
| `FailedMount` | `MountVolume.SetUp failed for volume "<vol>": rpc error: code = Internal desc = ...` | CSI 卷挂载失败 | `kubectl get events -n <ns> --field-selector reason=FailedMount` | → 转 [[26-技能/06-存储/csi-storage/csi-fta.md\|CSI 存储诊断]] |
| `FailedMount` | `Unable to attach or mount volumes: unmounted volumes=[<vol>], unattached volumes=[...]: timed out waiting for the condition` | 卷挂载超时 | 同上 | → 转 CSI 存储诊断 |
| `MultiAttach` | `Multi-Attach error for volume "<pv>" Volume is already used by pod(s) on node <node>` | 卷未从旧节点释放 | `kubectl get volumeattachment \| grep <pv>` | → 删除残留 VolumeAttachment |

#### 容器创建失败错误消息（CreateContainerError / RunContainerError）

| 事件 Message 模式 | 含义 | 检测命令 | 修复方向 |
|------------------|------|---------|----------|
| `Error: failed to create containerd task: failed to create shim task: OCI runtime create failed: runc create failed: unable to start container process: exec: "<cmd>": executable file not found in $PATH` | 容器入口命令不存在 | `kubectl describe pod` Events | 修正 command/args |
| `Error: failed to create containerd task: OCI runtime create failed: runc create failed: unable to start container process: exec: "<cmd>": permission denied` | 入口文件无执行权限 | 同上 | 修正镜像文件权限 |
| `Error: container has runAsNonRoot and image will run as root` | SecurityContext 冲突（runAsNonRoot=true 但镜像 USER=root） | 同上 | 设置 runAsUser 或修改镜像 |
| `Error: container's runAsUser breaks non-root policy` | PodSecurityPolicy/PSA 拒绝 | 同上 | 调整 SecurityContext |
| `Error: failed to create containerd task: failed to create shim: failed to mount /dev/shm` | 共享内存挂载失败 | 同上 | 检查节点 /dev/shm |
| `CreateContainerConfigError: secret "<name>" not found` | 引用的 Secret 不存在 | `kubectl get secret <name> -n <ns>` | 创建 Secret |
| `CreateContainerConfigError: configmap "<name>" not found` | 引用的 ConfigMap 不存在 | `kubectl get cm <name> -n <ns>` | 创建 ConfigMap |
| `CreateContainerConfigError: references non-existent config key` | ConfigMap/Secret 中缺少指定 key | `kubectl get cm <name> -o yaml` | 修正 key 引用 |

#### 节点级诊断命令（crictl）

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
# 检查节点上镜像拉取状态
ssh <node-ip> "crictl pull <image> 2>&1"   # 手动测试拉取
ssh <node-ip> "crictl images | grep <image-repo>"  # 查看已缓存镜像
ssh <node-ip> "crictl rmi --prune"  # 🟡 清理未使用镜像释放磁盘

# 检查节点磁盘空间（镜像存储目录）
ssh <node-ip> "df -h /var/lib/containerd /var/lib/kubelet"

# 检查 containerd 配置中的 mirror/registry 设置
ssh <node-ip> "cat /etc/containerd/config.toml | grep -A5 'plugins.*registry'"
```

| crictl 错误输出 | 含义 | 修复 |
|--------------|------|------|
| `pulling image failed: rpc error: code = NotFound` | 镜像不存在 | 核实镜像地址 |
| `pulling image failed: rpc error: code = Unauthenticated` | 认证失败 | 配置 registry mirror 凭证 |
| `pulling image failed: rpc error: code = DeadlineExceeded` | 拉取超时 | 检查网络/配置镜像加速 |
| `Image is in use by running container` | 镜像被占用无法删除 | 先停止容器再清理 |

### 3.4 深度诊断

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 Secret 内容
kubectl get secret <secret> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | python3 -m json.tool

# 在节点上测试拉取（需要节点访问权限）
# crictl pull <image>

# 检查节点磁盘空间
kubectl describe node <node> | grep -A 5 "Conditions" | grep DiskPressure
```

### 3.5 修复方案

**创建/更新 imagePullSecret**:
```bash
# 🟡 中风险：会修改集群/资源状态
kubectl create secret docker-registry regcred \
  --docker-server=<registry-url> \
  --docker-username=<username> \
  --docker-password=<password> \
  -n <namespace> \
  --dry-run=client -o yaml | kubectl apply -f -
```

**配置 Pod 使用 Secret**:
```yaml
spec:
  imagePullSecrets:
    - name: regcred
  containers:
    - name: app
      image: registry.example.com/app:v1.0
```

---

## 4. ContainerCreating 卡住诊断

### 4.1 诊断流程

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看事件
kubectl describe pod <pod> -n <namespace> | grep -A 10 "Events:"

# Step 2: 检查 CNI 状态
kubectl get pods -n kube-system -l k8s-app=calico-node  # 或对应 CNI
kubectl logs -n kube-system -l k8s-app=calico-node --tail=20

# Step 3: 检查存储挂载
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

# Step 4: 检查 CSI 驱动
kubectl get pods -n kube-system | grep csi
```

### 4.2 常见卡住原因

| 事件关键词 | 根因 | 修复 |
|-----------|------|------|
| `FailedCreatePodSandBox` / `NetworkPluginNotReady` | CNI 未就绪 | 检查 CNI DaemonSet 状态 |
| `MountVolume.SetUp failed` | 存储挂载失败 | 检查 PVC/CSI driver |
| `Multi-Attach error for volume` | 卷未从旧节点释放 | 删除残留 VolumeAttachment |
| `Unable to attach or mount volumes` | CSI 超时 | 检查 CSI node plugin 日志 |
| `context deadline exceeded` | 操作超时 | 检查节点负载/网络 |

---

## 5. Init 容器错误诊断

### 5.1 诊断流程

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Init 容器状态
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.initContainerStatuses[*]}{"name: "}{.name}{"\n  state: "}{.state}{"\n  restarts: "}{.restartCount}{"\n"}{end}'

# 查看 Init 容器日志
kubectl logs <pod> -n <namespace> -c <init-container-name>

# 查看 Init 容器配置
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.initContainers[*]}{"name: "}{.name}{"\n  image: "}{.image}{"\n  command: "}{.command}{"\n"}{end}'
```

### 5.2 常见 Init 容器问题

| 问题 | 症状 | 修复 |
|------|------|------|
| 等待依赖服务 | Init 容器 `nslookup` 或 `wget` 失败 | 确认依赖 Service 已就绪 |
| 数据库迁移失败 | Init 容器 exit code 1 | 检查迁移脚本和数据库连接 |
| 配置渲染错误 | Init 容器日志有模板错误 | 检查 ConfigMap/Secret 内容 |
| 权限不足 | `Permission denied` | 调整 SecurityContext/FSGroup |

---

## 6. 根因分类

| RC-ID | 根因 | 概率 | 修复方案 | 风险 |
|-------|------|------|---------|------|
| RC-001 | 镜像 Tag 不存在/名称错误 | 30% | 修正镜像地址 | 🟡 |
| RC-002 | imagePullSecret 缺失/过期 | 25% | 创建/更新 Secret | 🟡 |
| RC-003 | 节点到 Registry 网络不通 | 15% | 修复网络/安全组 | 🟡 |
| RC-004 | Registry 限流 | 10% | 使用镜像缓存代理 | 🟢 |
| RC-005 | CNI 插件异常 | 10% | 重启 CNI DaemonSet | 🔴 |
| RC-006 | 存储挂载失败 | 5% | 修复 CSI/PVC | 🟡 |
| RC-007 | Init 容器逻辑错误 | 5% | 修复 Init 容器配置 | 🟡 |

---

## 7. 生产案例

### 案例: Docker Hub 限流导致批量 ImagePullBackOff

**现象**: 凌晨批量扩容后，新 Pod 全部 ImagePullBackOff

**诊断**: Events 显示 `toomanyrequests: You have reached your pull rate limit`

**根因**: Docker Hub 匿名拉取限制 100 次/6h，扩容触发限流

**修复**:
1. 🟢 短期：等待限流窗口重置
2. 🟡 长期：配置 Docker Hub 认证 + 内部镜像缓存代理

---

## 8. 监控告警配置

```yaml
groups:
  - name: pod-image-pull
    rules:
      - alert: ImagePullBackOff
        expr: kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"} == 1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 镜像拉取失败"

      - alert: ContainerCreatingStuck
        expr: kube_pod_container_status_waiting_reason{reason="ContainerCreating"} == 1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 容器创建卡住超过 10 分钟"
```

---

## 9. 版本差异（基于 code/ 源码实证）

> 基于 `code/kubernetes-release-1.20`、`-1.34` 的 `pkg/apis/core/types.go` 比对，影响镜像拉取与容器创建诊断的版本敏感点。

| 特性 / 字段 | ≤ 1.20 | 1.28 | 1.34 | 1.36 | 诊断影响 |
|------------|:-----:|:----:|:----:|:----:|---------|
| `RuntimeClassName`（运行时选择） | 🅱/✅（1.14 beta，1.20 无 gate） | ✅ | ✅ | ✅ | 多运行时（runc/gVisor/Kata）镜像不兼容时需结合 RuntimeClass 诊断 |
| Ephemeral Containers（`kubectl debug`） | 🅰 alpha | ✅ | ✅ | ✅ | 1.25 GA；调试镜像/容器创建问题的临时容器手段 |
| `RecursiveReadOnlyMounts` | ❌ | ❌ | ✅（1.32 alpha → 1.34 GA） | ✅ | 递归只读挂载；挂载异常导致 ContainerCreating 卡住时需关注 |
| `HostnameOverride` | ❌ | ❌ | 🅰 alpha | 🅰 alpha | 1.34+ 可覆盖 hostname |

**CRI（容器运行时接口）相关**：

- **Dockershim 移除**：Kubernetes 1.24 起 kubelet 不再内置 dockershim。1.24+ 集群节点运行时为 containerd/CRI-O，诊断容器创建/镜像拉取时应用 `crictl ps` / `crictl images` 而非 `docker ps`。
- 镜像拉取本身由 kubelet + CRI 运行时完成，`ImagePullBackOff` 错误信息格式在 1.18–1.36 基本一致。

**诊断适配要点**：

- 本技能核心命令（`kubectl describe pod` 看 Events、检查 imagePullSecrets）全版本通用。
- 在 ≤ 1.23 集群上节点可能仍为 dockershim，`crictl` 与 `docker` 命令需根据节点实际运行时选择。

> [存疑：Dockershim 于 1.24 移除属官方公开信息，但本仓库无 1.24 及 kubelet CRI 层源码，"docker/crictl 命令适用性"结论基于运行时生态常识而非本地代码；请以节点 `crictl info` 与 `kubectl get node -o wide` 的 CONTAINER-RUNTIME 列为准]

完整版本矩阵见 [reference/pod-version-differences.md](reference/pod-version-differences.md)。

---

## 10. 快速分级（P0-P3）

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 镜像仓库整体不可用，集群大面积 ImagePullBackOff | 立即 | 检查 registry 与网络，启用镜像缓存/mirror |
| **P1** | 关键服务新版本全部拉取失败无法发布 | ≤15min | 回滚镜像 tag 或修复凭证 |
| **P2** | 部分节点拉取失败，服务仍可用 | ≤1h | 定位节点/凭证差异 |
| **P3** | 单 Pod 镜像拉取慢或偶发超时 | ≤1d | 观察网络/镜像大小 |

---

## 11. 证据三元组

```promql
# 🟢 ImagePullBackOff 判据
kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"} == 1

# 🟢 ErrImagePull 判据
kube_pod_container_status_waiting_reason{reason="ErrImagePull"} == 1
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | Prometheus | waiting_reason 命中 ImagePullBackOff/ErrImagePull |
| Events | `kubectl describe pod` | `Failed to pull image`：`unauthorized` / `not found` / `no such host` / `context deadline exceeded` |

---

## 12. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `kubectl get pod` | 容器状态由 Waiting → Running |
| 短期监控 | 拉取告警 | 5min 内无新增 ImagePullBackOff |
| 解决标准 | 镜像成功拉取并缓存 | 节点 `crictl images` 可见目标镜像 |
| 回归检测 | 下一次发布 | 同镜像仓库拉取正常 |

---

## 13. 升级协议

- 凭证/tag 错误等明确根因 → Agent 提交修复建议，人工审批。
- 镜像仓库整体不可用（集群级）→ 立即升级 P0，联系 registry / 网络团队。
- 升级交接信息包：镜像全名与 tag、`Failed to pull image` 事件全文、imagePullSecrets 配置、节点 `crictl pull` 测试结果。

### 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| ImagePullBackOff 一律归因为网络 | 需区分 unauthorized（凭证）/ not found（tag）/ 网络超时 |
| ContainerCreating 卡住当作镜像问题 | 可能为 CNI/CSI 挂载问题，需查 Events |

---

## 相关链接

- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff 诊断]]
- [[26-技能/04-工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]
- [[26-技能/04-工作负载/pod/04-pod-sop-runbook.md|Pod SOP/Runbook]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
