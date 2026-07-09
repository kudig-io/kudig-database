---
title: 06 - Pod安全标准详解
description: '# 06 - Pod安全标准详解'
summary: 'pod-security.kubernetes.io/enforce: restricted'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- apiserver
- istio
- networkpolicy
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Pod安全标准详解 是什么
- 如何 Pod安全标准详解
- Kubernetes 7 security 最佳实践
trigger_keywords:
- Pod安全标准详解
- security
prerequisites:
- kubectl-basics
- rbac-basics
- service-mesh-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/topic-fta/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 06 - Pod安全标准详解

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/concepts/security/pod-security-standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)

<!-- chunk: Pod Securityod Security Standards]] (PSS) 级别 -->
## Pod Security Standards (PSS) 级别

| 级别 | 限制程度 | 适用场景 | 特点 | 安全性 |
|-----|---------|---------|------|-------|
| **Privileged** | 无限制 | 系统组件、基础设施 | 不受任何限制 | 低 |
| **Baseline** | 基础限制 | 一般工作负载 | 防止已知提权 | 中 |
| **Restricted** | 严格限制 | 安全敏感工作负载 | 最佳安全实践 | 高 |

<!-- chunk: PSS级别详细对比 -->
## PSS级别详细对比

| 控制项 | Privileged | Baseline | Restricted |
|-------|-----------|----------|------------|
| 特权容器 | ✅ | ❌ | ❌ |
| hostNetwork | ✅ | ❌ | ❌ |
| hostPID | ✅ | ❌ | ❌ |
| hostIPC | ✅ | ❌ | ❌ |
| hostPath卷 | ✅ | ✅ | ❌ |
| 任意Capabilities | ✅ | 部分 | 仅NET_BIND_SERVICE |
| 特权提升 | ✅ | ✅ | ❌ |
| 非root运行 | 可选 | 可选 | 必须 |
| Seccomp | 可选 | 推荐 | 必须 |
| 只读根文件系统 | 可选 | 可选 | 推荐 |

<!-- chunk: PSS执行模式 -->
## PSS执行模式

| 模式 | 效果 | 说明 |
|-----|------|------|
| enforce | 拒绝违规Pod | 生产环境使用 |
| audit | 记录审计日志 | 监控违规情况 |
| warn | 返回警告信息 | 迁移过渡期使用 |

<!-- chunk: 命名空间标签配置 -->
## 命名空间标签配置

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    # 强制执行restricted级别
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.32
    
    # 审计baseline级别
    pod-security.kubernetes.io/audit: baseline
    pod-security.kubernetes.io/audit-version: v1.32
    
    # 警告baseline级别
    pod-security.kubernetes.io/warn: baseline
    pod-security.kubernetes.io/warn-version: v1.32
```

<!-- chunk: Baseline级别限制 -->
## Baseline级别限制

| 控制项 | 允许值 | 字段路径 |
|-------|-------|---------|
| HostProcess | false | `securityContext.windowsOptions.hostProcess` |
| Host Namespaces | false | `hostNetwork`, `hostPID`, `hostIPC` |
| Privileged | false | `securityContext.privileged` |
| Capabilities | 特定列表 | `securityContext.capabilities.add` |
| HostPath Volumes | 无限制 | `volumes[*].hostPath` |
| Host Ports | 无限制 | `containers[*].ports[*].hostPort` |
| AppArmor | 运行时默认或自定义 | `metadata.annotations` |
| SELinux | 仅允许特定类型 | `securityContext.seLinuxOptions.type` |
| /proc Mount Type | 默认 | `securityContext.procMount` |
| Seccomp | RuntimeDefault或Localhost | `securityContext.seccompProfile.type` |
| Sysctls | 安全子集 | `securityContext.sysctls[*].name` |

<!-- chunk: Restricted级别限制 -->
## Restricted级别限制

| 控制项 | 要求 | 字段路径 |
|-------|-----|---------|
| Volume Types | 仅允许安全类型 | `volumes[*]` |
| Privilege Escalation | 禁止 | `securityContext.allowPrivilegeEscalation` |
| Running as Non-root | 必须 | `securityContext.runAsNonRoot` |
| Running as Non-root user | 必须 | `securityContext.runAsUser` (非0) |
| Seccomp | RuntimeDefault或Localhost | `securityContext.seccompProfile.type` |
| Capabilities | 仅允许NET_BIND_SERVICE | `securityContext.capabilities` |

<!-- chunk: Baseline级别允许的Capabilities -->
## Baseline级别允许的Capabilities

```
AUDIT_WRITE
CHOWN
DAC_OVERRIDE
FOWNER
FSETID
KILL
MKNOD
NET_BIND_SERVICE
SETFCAP
SETGID
SETPCAP
SETUID
SYS_CHROOT
```

<!-- chunk: Restricted级别合规Pod示例 -->
## Restricted级别合规Pod示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: restricted-pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: myapp:v1.0
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
    resources:
      limits:
        cpu: "1"
        memory: 512Mi
      requests:
        cpu: 100m
        memory: 128Mi
  volumes:
  - name: data
    emptyDir: {}
  - name: config
    configMap:
      name: app-config
```

<!-- chunk: Restricted允许的Volume类型 -->
## Restricted允许的Volume类型

```
configMap
csi
downwardAPI
emptyDir
ephemeral
persistentVolumeClaim
projected
secret
```

<!-- chunk: 豁免配置 -->
## 豁免配置

```yaml
# API Server配置
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: baseline
      enforce-version: latest
      audit: restricted
      audit-version: latest
      warn: restricted
      warn-version: latest
    exemptions:
      usernames:
      - system:serviceaccount:kube-system:*
      runtimeClasses:
      - gvisor
      namespaces:
      - kube-system
      - istio-system
```

<!-- chunk: PSA迁移步骤 -->
## PSA迁移步骤

| 步骤 | 操作 | 说明 |
|-----|------|------|
| 1 | 评估现状 | 使用dry-run检查违规 |
| 2 | 设置warn/audit | 收集违规信息 |
| 3 | 修复工作负载 | 更新不合规Pod |
| 4 | 启用enforce | 强制执行 |
| 5 | 持续监控 | 监控审计日志 |

<!-- chunk: 检查违规命令 -->
## 检查违规命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查命名空间违规情况
kubectl label --dry-run=server --overwrite ns \
  <namespace> pod-security.kubernetes.io/enforce=restricted

# 查看所有命名空间PSA标签
kubectl get ns -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels}{"\n"}{end}'

# 审计日志查询
kubectl logs -n kube-system -l component=kube-apiserver | grep "pod-security"
```
<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 | 影响 |
|------|---------|------|
| v1.22 | PodSecurity Admission Alpha | 功能预览 |
| v1.23 | PodSecurity Admission Beta | 可生产使用 |
| v1.25 | PodSecurity Admission GA, PSP移除 | 必须迁移 |
| v1.28 | 用户命名空间支持改进 | 增强隔离 |
| v1.29 | AppArmor注解改为字段 | 配置简化 |
| v1.30 | AppArmor字段GA | 生产可用 |

<!-- chunk: 安全加固建议 -->
## 安全加固建议

| 建议 | 说明 | 优先级 |
|-----|------|-------|
| **使用Restricted** | 尽可能使用restricted级别 | P0 |
| **非root运行** | 所有容器以非root运行 | P0 |
| **禁止特权提升** | allowPrivilegeEscalation: false | P0 |
| **只读根文件系统** | readOnlyRootFilesystem: true | P1 |
| **删除所有Capabilities** | drop: ALL | P1 |
| **启用Seccomp** | RuntimeDefault或自定义 | P1 |
| **资源限制** | 设置requests和limits | P1 |
| **网络策略** | 配合NetworkPolicy | P2 |

<!-- chunk: ACK Pod安全配置 -->
## ACK Pod安全配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ACK集群默认启用PodSecurity
# 查看集群PSA配置
kubectl get ns --show-labels | grep pod-security

# 批量设置命名空间安全级别
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | grep -v kube-); do
  kubectl label ns $ns \
    pod-security.kubernetes.io/enforce=baseline \
    pod-security.kubernetes.io/warn=restricted \
    --overwrite
done
```
---

**Pod安全原则**: 最小权限 + 非root运行 + 禁止特权提升 + 启用Seccomp + 持续审计监控

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 安全 MOC
- [[安全/README.md|Security Domain]]
- [[安全/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## See Also

- 04-audit-logging-compliance
- 05-policy-validation-tools
- 07-rbac-matrix-configuration
- 08-security-best-practices

- [[安全/README.md|返回目录]]

## Related

- [[生态参考/topic-index/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
