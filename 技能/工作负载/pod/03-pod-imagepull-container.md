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

## 2. ImagePullBackOff 诊断

### 2.1 快速诊断

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

### 2.2 错误信息决策表

| 错误信息 | 根因 | 修复 |
|---------|------|------|
| `repository does not exist` | 镜像名称/路径错误 | 核实镜像地址拼写 |
| `manifest unknown` / `not found` | Tag 不存在 | 核实 Tag 是否已推送 |
| `unauthorized` / `pull access denied` | 认证失败 | 检查 imagePullSecret |
| `timeout` / `connection refused` | 网络不通 | 检查节点到 Registry 的连通性 |
| `too many requests` / `rate limit` | Registry 限流 | 使用镜像缓存/切换仓库 |
| `x509: certificate signed by unknown authority` | TLS 证书问题 | 配置 CA 证书或使用受信仓库 |
| `no space left on device` | 节点磁盘满 | 清理镜像/扩容磁盘 |

### 2.3 深度诊断

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 Secret 内容
kubectl get secret <secret> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | python3 -m json.tool

# 在节点上测试拉取（需要节点访问权限）
# crictl pull <image>

# 检查节点磁盘空间
kubectl describe node <node> | grep -A 5 "Conditions" | grep DiskPressure
```

### 2.4 修复方案

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

## 3. ContainerCreating 卡住诊断

### 3.1 诊断流程

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

### 3.2 常见卡住原因

| 事件关键词 | 根因 | 修复 |
|-----------|------|------|
| `FailedCreatePodSandBox` / `NetworkPluginNotReady` | CNI 未就绪 | 检查 CNI DaemonSet 状态 |
| `MountVolume.SetUp failed` | 存储挂载失败 | 检查 PVC/CSI driver |
| `Multi-Attach error for volume` | 卷未从旧节点释放 | 删除残留 VolumeAttachment |
| `Unable to attach or mount volumes` | CSI 超时 | 检查 CSI node plugin 日志 |
| `context deadline exceeded` | 操作超时 | 检查节点负载/网络 |

---

## 4. Init 容器错误诊断

### 4.1 诊断流程

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Init 容器状态
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.initContainerStatuses[*]}{"name: "}{.name}{"\n  state: "}{.state}{"\n  restarts: "}{.restartCount}{"\n"}{end}'

# 查看 Init 容器日志
kubectl logs <pod> -n <namespace> -c <init-container-name>

# 查看 Init 容器配置
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.initContainers[*]}{"name: "}{.name}{"\n  image: "}{.image}{"\n  command: "}{.command}{"\n"}{end}'
```

### 4.2 常见 Init 容器问题

| 问题 | 症状 | 修复 |
|------|------|------|
| 等待依赖服务 | Init 容器 `nslookup` 或 `wget` 失败 | 确认依赖 Service 已就绪 |
| 数据库迁移失败 | Init 容器 exit code 1 | 检查迁移脚本和数据库连接 |
| 配置渲染错误 | Init 容器日志有模板错误 | 检查 ConfigMap/Secret 内容 |
| 权限不足 | `Permission denied` | 调整 SecurityContext/FSGroup |

---

## 5. 根因分类

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

## 6. 生产案例

### 案例: Docker Hub 限流导致批量 ImagePullBackOff

**现象**: 凌晨批量扩容后，新 Pod 全部 ImagePullBackOff

**诊断**: Events 显示 `toomanyrequests: You have reached your pull rate limit`

**根因**: Docker Hub 匿名拉取限制 100 次/6h，扩容触发限流

**修复**:
1. 🟢 短期：等待限流窗口重置
2. 🟡 长期：配置 Docker Hub 认证 + 内部镜像缓存代理

---

## 7. 监控告警配置

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

## 8. 版本差异（基于 code/ 源码实证）

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

## 9. 快速分级（P0-P3）

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 镜像仓库整体不可用，集群大面积 ImagePullBackOff | 立即 | 检查 registry 与网络，启用镜像缓存/mirror |
| **P1** | 关键服务新版本全部拉取失败无法发布 | ≤15min | 回滚镜像 tag 或修复凭证 |
| **P2** | 部分节点拉取失败，服务仍可用 | ≤1h | 定位节点/凭证差异 |
| **P3** | 单 Pod 镜像拉取慢或偶发超时 | ≤1d | 观察网络/镜像大小 |

---

## 10. 证据三元组

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

## 11. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `kubectl get pod` | 容器状态由 Waiting → Running |
| 短期监控 | 拉取告警 | 5min 内无新增 ImagePullBackOff |
| 解决标准 | 镜像成功拉取并缓存 | 节点 `crictl images` 可见目标镜像 |
| 回归检测 | 下一次发布 | 同镜像仓库拉取正常 |

---

## 12. 升级协议

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

- [[技能/工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[技能/工作负载/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff 诊断]]
- [[技能/工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]
- [[技能/工作负载/pod/04-pod-sop-runbook.md|Pod SOP/Runbook]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]
