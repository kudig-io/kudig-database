---
title: 安全上下文
description: SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心...
summary: SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心...
category: dictionary
tags:
- k8s
- glossary
- security
- security-context
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全上下文 是什么
- SecurityContext 详解
trigger_keywords:
- 安全上下文
- SecurityContext
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全上下文

> **英文名**: SecurityContext

## 概述

SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心机制。

## 核心概念/原理

### 关键配置

```yaml
securityContext:
  runAsNonRoot: true           # 禁止以 root 用户运行
  runAsUser: 1000              # 指定 UID
  runAsGroup: 3000             # 指定 GID
  fsGroup: 2000                # 挂载卷的文件组
  readOnlyRootFilesystem: true # 只读根文件系统
  allowPrivilegeEscalation: false # 禁止提权
  capabilities:
    drop: ["ALL"]              # 删除所有 Linux 能力
  seccompProfile:
    type: RuntimeDefault       # 使用默认 seccomp 配置
```

### Pod 级 vs 容器级

- **Pod SecurityContext**：应用于 Pod 中所有容器和卷。
- **Container SecurityContext**：仅应用于特定容器，可覆盖 Pod 级设置。

## 关键机制或特性

- `allowPrivilegeEscalation: false` 阻止 setuid/setgid 二进制提权。
- `capabilities.drop: ALL` 移除所有 Linux 能力，按需添加。
- `seccompProfile` 限制容器可以执行的系统调用。
- `AppArmor` / `SELinux` 提供额外的 MAC（强制访问控制）层。

## 使用场景与最佳实践

- 所有生产容器都应配置 SecurityContext。
- 始终设置 `runAsNonRoot: true` 和 `readOnlyRootFilesystem: true`。
- 使用 `capabilities.drop: ALL` 并根据需要添加最小能力。
- 配合 Pod Security Standards 的 Restricted 级别强制执行。

## 架构深度解析

### SecurityContext 作用域与生效链路

```
┌──────────────────────────────────────────────────────────────┐
│  SecurityContext 三级作用域                                    │
│  ├─ Pod 级（pod.spec.securityContext）：                       │
│  │   fsGroup / seLinuxOptions / sysctls / runAsUser /         │
│  │   runAsGroup / supplementalGroups                          │
│  ├─ 容器级（container.securityContext）：                      │
│  │   privileged / capabilities / allowPrivilegeEscalation /   │
│  │   readOnlyRootFilesystem / runAsUser / seccompProfile /    │
│  │   procMount / windowsOptions                               │
│  └─ 默认值来源（优先级从低到高）：                             │
│      PodSecurityPolicy/PSS → Namespace 默认值（PSA）→          │
│      Pod/容器显式声明                                          │
│                                                                 │
│  生效链路：                                                     │
│  Pod spec → kubelet → OCI 运行时（containerd/runc）            │
│   → Linux capabilities / UID/GID / namespace / seccomp         │
│   → 进程实际权限                                                │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 安全上下文 | `pkg/securitycontext/` | 默认值与校验 |
| 运行时转换 | `pkg/kubelet/kuberuntime/security_context_linux.go` | 转换为 OCI spec |
| capabilities | `pkg/kubelet/kuberuntime/` | 能力集计算（+/-） |
| 校验 | `pkg/apis/core/validation/` | 字段合法性校验 |

### 流程步骤

1. 准入阶段（PSA/自定义策略）校验或注入安全上下文字段。
2. kubelet 合并 Pod 级与容器级配置，计算最终生效值。
3. 转换为 OCI 运行时 spec（runc 可识别的 JSON）。
4. 运行时应用：设置 UID/GID、capabilities、seccomp profile、只读根文件系统。
5. 容器进程以最终安全上下文启动并持续生效。

## 生产案例

### 案例 1：capabilities 误配导致应用能力异常

| 时间 | 事件 |
| --- | --- |
| T+0 | 安全加固：为 Web 容器添加 `drop: [ALL]` + 仅保留 NET_BIND_SERVICE |
| T+30min | 服务启动后绑定 80 端口失败，大量容器 CrashLoop |
| T+2h | 定位：应用需要 CAP_NET_RAW 做健康探测（ICMP），被 drop 后探测失败 |
| T+4h | 补充 `add: [NET_RAW]`，服务恢复 |
| T+1w | 形成"最小能力集评审"规范：按应用实际 syscall 需求定制 |

- **根因分析**：`drop: [ALL]` 是最安全但最激进的做法，未评估应用实际依赖的 capabilities 就全量 drop 会破坏运行。
- **修复命令**：
```bash
# 1. 查看容器实际能力（只读）
kubectl exec -it <pod> -- sh -c 'cat /proc/1/status | grep Cap'
# 2. 用 capsh 解析能力位（本地验证，🟢 低风险）
capsh --decode=$(kubectl exec <pod> -- sh -c 'grep CapEff /proc/1/status | awk "{print \$2}"' 2>/dev/null)
# 3. 修正安全上下文（🟡 中风险）
kubectl patch deployment web -n prod --type=strategic -p '{"spec":{"template":{"spec":{"containers":[{"name":"web","securityContext":{"capabilities":{"drop":["ALL"],"add":["NET_BIND_SERVICE","NET_RAW"]}}}]}}}}'
```

### 案例 2：runAsNonRoot 与镜像默认用户冲突

| 时间 | 事件 |
| --- | --- |
| T+0 | 平台强制 `runAsNonRoot: true`（安全基线） |
| T+1h | 大量镜像以 root 启动的旧应用全部启动失败 |
| T+3h | 定位：镜像 Dockerfile 未声明非 root 用户，容器内 UID 0 被拒绝 |
| T+1d | 修复：分批改造镜像（USER 指令）+ 临时豁免清单 |
| T+2w | 全部镜像完成非 root 化，基线全量生效 |

- **根因分析**：runAsNonRoot 校验的是进程 UID≠0；镜像默认 root 是常见历史债，强制启用前需先完成镜像画像盘点。
- **修复命令**：
```bash
# 1. 扫描以 root 运行的容器（只读）
kubectl get pods -A -o json | jq -r '.items[] | select(.status.containerStatuses[]?.state.running != null) | .metadata.namespace + "/" + .metadata.name' | head -20
# 2. 改造镜像：Dockerfile 增加非 root 用户（🟡 中风险）
# FROM ... 
# RUN useradd -r -u 10001 appuser
# USER 10001
# 3. 豁免清单（临时，Kyverno 策略）
kubectl label ns legacy pod-security.kubernetes.io/enforce-version=v1.30 --overwrite
```

## 对比评测

| 维度 | SecurityContext | PSA（策略强制） | seccomp | capabilities |
| --- | --- | --- | --- | --- |
| 定位 | 声明运行权限 | 校验声明 | 系统调用过滤 | 内核能力控制 |
| 层级 | Pod/容器 | 命名空间 | 容器 | 容器 |
| 强制方式 | 声明 | 准入 | 运行时 | 运行时 |
| 组合 | 基础 | 管控 | 纵深 | 纵深 |

**选型建议**：SecurityContext 是"声明"，PSA 是"校验"，seccomp/capabilities 是"纵深"。生产组合：PSA restricted 基线 + 最小 capabilities + 默认 seccomp RuntimeDefault。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 启动失败：Operation not permitted | capabilities 不足/privileged 缺失 | 检查 CapEff 与错误栈，补最小能力 |
| 容器以 root 运行 | 镜像默认 root / 未声明 | `kubectl exec -- id`；改造镜像 |
| seccomp 拒绝 syscall | 默认 profile 过严 | 改用 RuntimeDefault/自定义 profile |
| fsGroup 不生效 | 卷类型不支持 | 检查 CSI 驱动支持 fsGroup |
| 只读根文件系统报错 | 应用写根目录 | 挂载临时卷或调整目录权限 |

## 生产部署清单

- [ ] 全部容器声明 `runAsNonRoot: true` + 固定 runAsUser
- [ ] capabilities 最小集（drop ALL + 白名单 add）评审
- [ ] seccomp 默认 RuntimeDefault，敏感应用定制 profile
- [ ] 只读根文件系统 + 临时目录挂载
- [ ] 与 PSA restricted 基线对齐，豁免走审批

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 特权容器在关键路径运行 | 评估最小化改造，短期加 PSS 豁免记录 |
| P1 | 镜像默认 root 且无改造计划 | 制定非 root 化排期 |
| P2 | 无 capabilities 评审规范 | 建立最小能力集规范 |

## 面试要点

1. **Q：Pod 级与容器级 SecurityContext 的关系与覆盖规则？**
   A：Pod 级字段（fsGroup/runAsUser 等）作用于整 Pod 与卷权限；容器级字段（privileged/capabilities 等）仅作用于该容器；同名字段（如 runAsUser）容器级覆盖 Pod 级。实际生效值在 kubelet 合并后转为 OCI spec 执行。
2. **Q：如何在不破坏应用的前提下收紧 capabilities？**
   A：先盘点应用实际需要的系统调用与能力（`/proc/<pid>/status` 的 CapEff + 运行期审计日志）；采用"drop ALL + 白名单 add"；对无法确定的先运行观察（audit 模式），再逐步收紧。禁止不加评估直接全量 drop。
3. **Q：runAsNonRoot 与镜像用户的关系？**
   A：runAsNonRoot 是准入/运行时校验：进程 UID 必须非 0。若镜像默认 root（未写 USER 指令），容器会启动失败。因此镜像构建时声明非 root 用户（USER 10001）是前提，运行时再叠加 runAsUser 固定 UID 与 runAsNonRoot 校验。

## 运维要点

- 基线：PSA restricted + runAsNonRoot + capabilities 最小集作为默认模板。
- 豁免管理：特权/特殊应用豁免清单季度复核。
- 监控：Falco 检测能力滥用（如容器内提权尝试）。
- 审计：安全上下文变更纳入发布评审。
- 排障入口：启动失败先看错误类型（EPERM=能力/权限，Exec format=平台），再查 CapEff。

## 参考链接

- [SecurityContext - Official Documentation](https://kubernetes.io/docs/tasks/configure-pod-container/security-context/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
