# 23 - Pod Security Standards (PSS/PSA) YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

## 目录

- [概述](#概述)
- [三种安全级别](#三种安全级别)
- [命名空间标签配置](#命名空间标签配置)
- [安全控制项详解](#安全控制项详解)
- [PSP 到 PSS 迁移指南](#psp-到-pss-迁移指南)
- [内部实现机制](#内部实现机制)
- [YAML 配置示例](#yaml-配置示例)
- [版本兼容性](#版本兼容性)
- [最佳实践](#最佳实践)
- [常见问题 FAQ](#常见问题-faq)
- [生产案例](#生产案例)

---

## 概述

### PSS/PSA 是什么

**Pod Security Standards (PSS)** 是 Kubernetes 定义的一套预定义的安全策略标准,替代了已废弃的 Pod Security Policy (PSP)。

**Pod Security Admission (PSA)** 是内置的准入控制器,用于强制执行 PSS 定义的安全标准。

### 关键特性

| 特性 | PSP (已废弃) | PSS/PSA (推荐) |
|------|-------------|----------------|
| **引入版本** | v1.3 (Beta) | v1.22 (Alpha), v1.23 (Beta), v1.25 (GA) |
| **废弃状态** | v1.21 废弃, v1.25 移除 | v1.25+ GA 稳定 |
| **配置方式** | 独立 CRD 资源 | Namespace 标签 |
| **策略定义** | 自定义 YAML | 三种预定义级别 |
| **权限控制** | 需要 RBAC 绑定 | 直接标签控制 |
| **学习曲线** | 复杂 | 简单直观 |
| **审计能力** | 有限 | 内置 audit/warn 模式 |

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    API Server                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │         PodSecurity Admission Plugin                   │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  读取 Namespace 标签                              │  │  │
│  │  │  pod-security.kubernetes.io/enforce=baseline    │  │  │
│  │  │  pod-security.kubernetes.io/audit=restricted    │  │  │
│  │  │  pod-security.kubernetes.io/warn=restricted     │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  │              ↓                                          │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  应用 PSS 策略                                     │  │  │
│  │  │  - Privileged: 无限制                             │  │  │
│  │  │  - Baseline: 禁止已知权限提升                      │  │  │
│  │  │  - Restricted: 严格限制,遵循安全最佳实践           │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  │              ↓                                          │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  三种执行模式                                      │  │  │
│  │  │  - enforce: 拒绝违规 Pod                          │  │  │
│  │  │  - audit: 记录审计事件                             │  │  │
│  │  │  - warn: 返回警告信息                              │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
         ↓                    ↓                    ↓
   允许创建 Pod         记录审计日志          返回警告给用户
```

---

## 三种安全级别

### 1. Privileged (特权级)

**适用场景**: 系统级组件、可信工作负载

**限制**: 无限制,允许所有已知的权限提升

```yaml
# 允许的场景
- 系统守护进程 (kube-proxy, CNI 插件)
- 节点监控代理 (node-exporter, cAdvisor)
- 存储驱动 (CSI driver)
- 特权调试容器
```

### 2. Baseline (基线级)

**适用场景**: 一般业务应用,平衡安全性和兼容性

**限制**: 禁止已知的权限提升路径,同时保持易用性

| 控制项 | 限制说明 |
|--------|----------|
| **HostProcess** | 禁止 Windows HostProcess 容器 |
| **Host Namespaces** | 禁止 `hostNetwork`, `hostPID`, `hostIPC` |
| **Privileged Containers** | 禁止 `privileged: true` |
| **Capabilities** | 只能添加 `AUDIT_WRITE`, `CHOWN`, `DAC_OVERRIDE`, `FOWNER`, `FSETID`, `KILL`, `MKNOD`, `NET_BIND_SERVICE`, `SETFCAP`, `SETGID`, `SETPCAP`, `SETUID`, `SYS_CHROOT` |
| **HostPath Volumes** | 禁止 `hostPath` 类型卷 |
| **Host Ports** | 禁止 `hostPort` (v1.32+ 支持配置) |
| **AppArmor** | 不强制,但禁止 `unconfined` |
| **SELinux** | 禁止自定义 SELinux 用户和角色选项 |
| **proc Mount** | 禁止 `procMount: Unmasked` |
| **Seccomp** | v1.19+ 必须设置 (可选 `RuntimeDefault` 或自定义) |
| **Sysctls** | 禁止除安全 sysctls 外的所有配置 |

### 3. Restricted (受限级)

**适用场景**: 高安全要求的应用

**限制**: 遵循 Pod 安全加固最佳实践,严格限制

在 Baseline 基础上,额外增加:

| 控制项 | 限制说明 |
|--------|----------|
| **Volume Types** | 只允许: `configMap`, `downwardAPI`, `emptyDir`, `persistentVolumeClaim`, `projected`, `secret` |
| **Privilege Escalation** | 必须设置 `allowPrivilegeEscalation: false` |
| **runAsNonRoot** | 必须设置 `runAsNonRoot: true` |
| **runAsUser** | 必须以非 root 运行 (UID ≠ 0) |
| **Seccomp** | 必须设置为 `RuntimeDefault` 或 `Localhost` |
| **Capabilities** | 必须删除所有 capabilities (`drop: ["ALL"]`),可添加 `NET_BIND_SERVICE` |

---

## 命名空间标签配置

### 标签语法

```yaml
# 完整标签格式
pod-security.kubernetes.io/<MODE>: <LEVEL>
pod-security.kubernetes.io/<MODE>-version: <VERSION>

# MODE: enforce | audit | warn
# LEVEL: privileged | baseline | restricted
# VERSION: v1.25, v1.26, ..., latest (可选)
```

### 三种执行模式

| 模式 | 行为 | 适用场景 |
|------|------|----------|
| **enforce** | 拒绝违规 Pod 创建 | 生产环境强制执行 |
| **audit** | 允许创建,但记录到审计日志 | 观察模式,分析影响 |
| **warn** | 允许创建,返回警告信息给用户 | 过渡期提醒用户 |

### 基础配置示例

#### 1. Baseline 强制执行

```yaml
# namespace-baseline.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  labels:
    # 强制执行 Baseline 标准
    pod-security.kubernetes.io/enforce: baseline
    # 版本锁定到 v1.28
    pod-security.kubernetes.io/enforce-version: v1.28
    
    # 同时审计 Restricted 标准
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: v1.28
    
    # 警告 Restricted 标准
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: v1.28
```

#### 2. 渐进式策略 (推荐生产环境)

```yaml
# namespace-progressive.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production-app
  labels:
    # 当前强制执行 Baseline
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.28
    
    # 审计更严格的 Restricted 标准
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
    
    # 警告用户 Restricted 标准要求
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: latest
```

#### 3. 系统命名空间 (Privileged)

```yaml
# namespace-system.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    # 系统组件需要特权模式
    pod-security.kubernetes.io/enforce: privileged
    pod-security.kubernetes.io/audit: privileged
    pod-security.kubernetes.io/warn: privileged
```

### 版本锁定策略

```yaml
# 1. 锁定到特定版本 (推荐生产环境)
pod-security.kubernetes.io/enforce: baseline
pod-security.kubernetes.io/enforce-version: v1.28  # 明确版本

# 2. 使用 latest (自动更新)
pod-security.kubernetes.io/enforce: baseline
pod-security.kubernetes.io/enforce-version: latest  # 跟随集群版本

# 3. 不指定版本 (默认使用 latest)
pod-security.kubernetes.io/enforce: baseline  # 等同于 latest
```

**版本选择建议**:
- **生产环境**: 锁定到具体版本,避免集群升级导致的意外中断
- **测试环境**: 使用 `latest` 及早发现兼容性问题
- **混合策略**: `enforce` 锁定版本,`audit`/`warn` 使用 `latest`

---

## 安全控制项详解

### 完整控制项对照表

| 控制项 | Privileged | Baseline | Restricted | 检查字段 |
|--------|-----------|----------|------------|----------|
| **HostProcess** | ✅ 允许 | ❌ 禁止 | ❌ 禁止 | `securityContext.windowsOptions.hostProcess` |
| **Host Namespaces** | ✅ 允许 | ❌ 禁止 | ❌ 禁止 | `hostNetwork`, `hostPID`, `hostIPC` |
| **Privileged Containers** | ✅ 允许 | ❌ 禁止 | ❌ 禁止 | `securityContext.privileged` |
| **Capabilities** | ✅ 无限制 | ⚠️ 限制列表 | ⚠️ 必须 DROP ALL | `securityContext.capabilities.add/drop` |
| **HostPath Volumes** | ✅ 允许 | ❌ 禁止 | ❌ 禁止 | `volumes[*].hostPath` |
| **Host Ports** | ✅ 允许 | ❌ 禁止 | ❌ 禁止 | `containers[*].ports[*].hostPort` |
| **AppArmor** | ✅ 无限制 | ⚠️ 禁止 unconfined | ⚠️ 禁止 unconfined | `metadata.annotations` |
| **SELinux** | ✅ 无限制 | ⚠️ 限制选项 | ⚠️ 限制选项 | `securityContext.seLinuxOptions` |
| **proc Mount** | ✅ 允许 | ❌ 禁止 Unmasked | ❌ 禁止 Unmasked | `securityContext.procMount` |
| **Seccomp** | ✅ 可选 | ⚠️ 必须设置 | ⚠️ RuntimeDefault/Localhost | `securityContext.seccompProfile` |
| **Sysctls** | ✅ 无限制 | ⚠️ 只允许安全 sysctls | ⚠️ 只允许安全 sysctls | `securityContext.sysctls` |
| **Volume Types** | ✅ 无限制 | ✅ 无限制 | ⚠️ 限制类型 | `volumes[*].type` |
| **Privilege Escalation** | ✅ 可选 | ✅ 可选 | ⚠️ 必须设置 false | `securityContext.allowPrivilegeEscalation` |
| **runAsNonRoot** | ✅ 可选 | ✅ 可选 | ⚠️ 必须 true | `securityContext.runAsNonRoot` |
| **runAsUser** | ✅ 无限制 | ✅ 无限制 | ⚠️ 必须 ≠ 0 | `securityContext.runAsUser` |

### 关键控制项详解

#### 1. Capabilities 控制

```yaml
# Baseline 允许的 Capabilities
allowedCapabilities:
  - AUDIT_WRITE      # 写审计日志
  - CHOWN            # 修改文件所有者
  - DAC_OVERRIDE     # 忽略文件读写权限检查
  - FOWNER           # 忽略文件所有权操作权限检查
  - FSETID           # 确保文件修改时不清除 setuid/setgid 位
  - KILL             # 发送信号给进程
  - MKNOD            # 创建特殊文件
  - NET_BIND_SERVICE # 绑定 <1024 端口
  - SETFCAP          # 设置文件 capabilities
  - SETGID           # 修改进程 GID
  - SETPCAP          # 修改进程 capabilities
  - SETUID           # 修改进程 UID
  - SYS_CHROOT       # 使用 chroot()

# Restricted 推荐配置
securityContext:
  capabilities:
    drop: ["ALL"]                      # 删除所有 capabilities
    add: ["NET_BIND_SERVICE"]          # 可选:添加绑定特权端口能力
```

#### 2. Seccomp 配置

```yaml
# Baseline: 必须设置 seccomp (v1.19+)
securityContext:
  seccompProfile:
    type: RuntimeDefault              # 使用容器运行时默认配置

# Restricted: 更严格的限制
securityContext:
  seccompProfile:
    type: RuntimeDefault              # 或
    # type: Localhost
    # localhostProfile: my-profile.json  # 自定义配置文件
```

#### 3. Volume Types 限制 (Restricted)

```yaml
# Restricted 允许的卷类型
volumes:
  - name: config
    configMap:                         # ✅ 允许
      name: my-config
  - name: secret
    secret:                            # ✅ 允许
      secretName: my-secret
  - name: data
    emptyDir: {}                       # ✅ 允许
  - name: downward
    downwardAPI:                       # ✅ 允许
      items:
        - path: "labels"
          fieldRef:
            fieldPath: metadata.labels
  - name: pvc
    persistentVolumeClaim:             # ✅ 允许
      claimName: my-pvc
  - name: projected
    projected:                         # ✅ 允许
      sources:
        - serviceAccountToken:
            path: token

# ❌ Restricted 禁止的卷类型
  - name: host
    hostPath:                          # ❌ 禁止
      path: /var/log
  - name: nfs
    nfs:                               # ❌ 禁止
      server: nfs-server
      path: /share
  - name: cephfs
    cephfs:                            # ❌ 禁止
      monitors: [...]
```

#### 4. 安全 Sysctls

```yaml
# Baseline/Restricted 允许的安全 sysctls
securityContext:
  sysctls:
    # 网络命名空间内的安全 sysctls
    - name: kernel.shm_rmid_forced     # ✅ 允许
      value: "1"
    - name: net.ipv4.ip_local_port_range  # ✅ 允许
      value: "1024 65535"
    - name: net.ipv4.tcp_syncookies    # ✅ 允许
      value: "1"
    - name: net.ipv4.ping_group_range  # ✅ 允许
      value: "0 2147483647"
    - name: net.ipv4.ip_unprivileged_port_start  # ✅ 允许 (v1.22+)
      value: "1024"

# ❌ 禁止的不安全 sysctls
    - name: kernel.shm_max             # ❌ 禁止 (影响节点)
      value: "68719476736"
    - name: kernel.sem                 # ❌ 禁止 (影响节点)
      value: "250 32000 32 1024"
```

---

## PSP 到 PSS 迁移指南

### 迁移流程

```
┌─────────────────────────────────────────────────────────────┐
│                    迁移四阶段                                │
└─────────────────────────────────────────────────────────────┘

第一阶段: 评估分析
  ├─ 审计现有 PSP 策略
  ├─ 映射到 PSS 级别
  └─ 识别不兼容工作负载

第二阶段: 准备环境
  ├─ 启用 PodSecurity Admission (v1.23+)
  ├─ 为命名空间添加 warn/audit 标签
  └─ 收集审计日志分析影响

第三阶段: 逐步强制执行
  ├─ 非生产环境先启用 enforce
  ├─ 修复违规工作负载
  └─ 生产环境渐进式 enforce

第四阶段: 清理收尾
  ├─ 删除 PSP 资源和 RBAC
  ├─ 升级到 v1.25+ (移除 PSP)
  └─ 监控和持续优化
```

### PSP 到 PSS 映射表

| PSP 配置项 | PSS Baseline | PSS Restricted | 说明 |
|-----------|-------------|----------------|------|
| `privileged: false` | ✅ 满足 | ✅ 满足 | 禁止特权容器 |
| `allowPrivilegeEscalation: false` | ⚠️ 可选 | ✅ 必须 | 禁止权限提升 |
| `hostNetwork: false` | ✅ 满足 | ✅ 满足 | 禁止 Host 网络 |
| `hostPID: false` | ✅ 满足 | ✅ 满足 | 禁止 Host PID |
| `hostIPC: false` | ✅ 满足 | ✅ 满足 | 禁止 Host IPC |
| `runAsUser.rule: MustRunAsNonRoot` | ⚠️ 可选 | ✅ 必须 | 必须非 root 运行 |
| `seLinux` 限制 | ✅ 满足 | ✅ 满足 | SELinux 选项限制 |
| `supplementalGroups` | ⚠️ 无限制 | ⚠️ 无限制 | PSS 不控制 |
| `fsGroup` | ⚠️ 无限制 | ⚠️ 无限制 | PSS 不控制 |
| `readOnlyRootFilesystem` | ⚠️ 可选 | ⚠️ 可选 | PSS 不强制 |
| `volumes` 白名单 | ⚠️ 无限制 | ✅ 严格限制 | 只允许 6 种卷类型 |
| `allowedCapabilities` | ⚠️ 限制列表 | ⚠️ 必须 DROP ALL | Capabilities 限制 |

### 迁移实战示例

#### 原 PSP 策略

```yaml
# legacy-psp.yaml (已废弃)
apiVersion: policy/v1beta1
kind: PodSecurityPolicy
metadata:
  name: restricted-psp
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
    - ALL
  volumes:
    - 'configMap'
    - 'emptyDir'
    - 'projected'
    - 'secret'
    - 'downwardAPI'
    - 'persistentVolumeClaim'
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: 'MustRunAsNonRoot'
  seLinux:
    rule: 'RunAsAny'
  supplementalGroups:
    rule: 'RunAsAny'
  fsGroup:
    rule: 'RunAsAny'
  readOnlyRootFilesystem: false
```

#### 迁移到 PSS

```yaml
# 1. 命名空间配置 (替代 PSP)
apiVersion: v1
kind: Namespace
metadata:
  name: restricted-app
  labels:
    # 对应 PSP 的限制策略 -> Restricted 级别
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

---
# 2. 工作负载适配
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: restricted-app
spec:
  securityContext:
    # PSP runAsUser.rule: MustRunAsNonRoot
    runAsNonRoot: true
    runAsUser: 1000
    # PSP fsGroup 配置
    fsGroup: 3000
    # PSP seLinux 配置
    seLinuxOptions:
      level: "s0:c123,c456"
    # PSS Restricted 要求
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: nginx:1.27
    securityContext:
      # PSP allowPrivilegeEscalation: false
      allowPrivilegeEscalation: false
      # PSP requiredDropCapabilities
      capabilities:
        drop: ["ALL"]
        add: ["NET_BIND_SERVICE"]
      # PSP readOnlyRootFilesystem
      readOnlyRootFilesystem: true
    volumeMounts:
    - name: cache
      mountPath: /var/cache/nginx
    - name: run
      mountPath: /var/run
  volumes:
  # PSP volumes 白名单
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
```

### 迁移工具脚本

```bash
#!/bin/bash
# psp-to-pss-migration.sh - PSP 迁移辅助脚本

# 1. 列出所有 PSP
echo "=== 现有 PSP 策略 ==="
kubectl get psp

# 2. 为所有命名空间添加 audit/warn 标签 (观察模式)
echo -e "\n=== 添加 PSS 审计标签 ==="
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  # 跳过系统命名空间
  if [[ "$ns" =~ ^(kube-|default).*$ ]]; then
    echo "跳过系统命名空间: $ns"
    continue
  fi
  
  echo "处理命名空间: $ns"
  kubectl label ns "$ns" \
    pod-security.kubernetes.io/audit=baseline \
    pod-security.kubernetes.io/warn=baseline \
    --overwrite
done

# 3. 收集违规 Pod 信息 (审计日志)
echo -e "\n=== 检查审计日志 (需要等待一段时间) ==="
# 在 API Server 上执行
# grep "pod-security" /var/log/kubernetes/audit/audit.log | jq .

# 4. 生成违规报告
echo -e "\n=== 违规 Pod 报告 ==="
kubectl get pods -A -o json | jq -r '
  .items[] |
  select(
    .spec.hostNetwork == true or
    .spec.hostPID == true or
    .spec.hostIPC == true or
    (.spec.containers[].securityContext.privileged == true) or
    (.spec.securityContext.runAsUser == 0)
  ) |
  "\(.metadata.namespace)/\(.metadata.name)"
'

# 5. 测试强制执行 (在测试命名空间)
echo -e "\n=== 测试命名空间强制执行 ==="
kubectl label ns test-namespace \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/enforce-version=v1.28 \
  --overwrite

echo "迁移准备完成,请查看审计日志分析影响范围"
```

### 不兼容场景处理

#### 场景 1: 需要 hostPath 的监控 Agent

```yaml
# 问题: Restricted 禁止 hostPath
# 解决方案: 使用 Baseline 或为特定 Namespace 设置 Privileged

apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
  labels:
    # 监控组件需要 hostPath,使用 Baseline
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/audit: baseline
    pod-security.kubernetes.io/warn: baseline
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      # Baseline 允许 hostPath
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
      containers:
      - name: exporter
        image: prom/node-exporter:v1.8.0
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: sys
          mountPath: /host/sys
          readOnly: true
```

#### 场景 2: 遗留应用以 root 运行

```yaml
# 问题: Restricted 要求 runAsNonRoot: true
# 解决方案 A: 修改镜像,使用非 root 用户

# Dockerfile 修改
FROM nginx:1.27
# 创建非 root 用户
RUN groupadd -r appuser && useradd -r -g appuser -u 1000 appuser && \
    chown -R appuser:appuser /var/cache/nginx /var/run /var/log/nginx
USER 1000
EXPOSE 8080
CMD ["nginx", "-g", "daemon off;"]

---
# 解决方案 B: 使用 Baseline (允许 root)
apiVersion: v1
kind: Namespace
metadata:
  name: legacy-apps
  labels:
    pod-security.kubernetes.io/enforce: baseline  # 不强制 runAsNonRoot
    pod-security.kubernetes.io/audit: restricted  # 审计 Restricted 违规
    pod-security.kubernetes.io/warn: restricted   # 警告用户
```

---

## 内部实现机制

### PodSecurity Admission Plugin 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    API Server                                │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │          Admission Chain                                │ │
│  │  MutatingWebhook → ValidatingWebhook → PodSecurity     │ │
│  └────────────────────────────────────────────────────────┘ │
│                           ↓                                  │
│  ┌────────────────────────────────────────────────────────┐ │
│  │   PodSecurity Admission Plugin (内置)                   │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │ 1. 提取 Namespace 标签                            │  │ │
│  │  │    - enforce/audit/warn                           │  │ │
│  │  │    - level: privileged/baseline/restricted       │  │ │
│  │  │    - version: v1.28, latest                      │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │              ↓                                          │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │ 2. 加载对应版本的 PSS 策略定义                     │  │ │
│  │  │    - 从内置策略库读取                              │  │ │
│  │  │    - 缓存策略避免重复解析                          │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │              ↓                                          │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │ 3. 验证 Pod Spec                                  │  │ │
│  │  │    - 遍历所有控制项 (15+ 检查)                     │  │ │
│  │  │    - 检查 Pod 和 Container SecurityContext        │  │ │
│  │  │    - 检查 Volumes、Capabilities 等                │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │              ↓                                          │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │ 4. 应用执行模式                                    │  │ │
│  │  │    - enforce: 拒绝 + 返回错误                     │  │ │
│  │  │    - audit: 写入审计事件                           │  │ │
│  │  │    - warn: 添加 Warning Header                    │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
         ↓                    ↓                    ↓
   etcd 存储 Pod        审计日志文件          客户端收到警告
```

### 启用 PodSecurity Admission

#### 1. 检查是否已启用 (v1.23+ 默认启用)

```bash
# 查看 API Server 启动参数
kubectl get pod -n kube-system kube-apiserver-* -o yaml | grep enable-admission-plugins

# 输出示例
# - --enable-admission-plugins=NodeRestriction,PodSecurity
```

#### 2. 手动启用 (v1.22)

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    # 添加 PodSecurity 到准入插件列表
    - --enable-admission-plugins=NodeRestriction,PodSecurity
    # 禁用旧的 PodSecurityPolicy (v1.25 已移除)
    # - --enable-admission-plugins=NodeRestriction,PodSecurityPolicy
```

### 豁免配置 (Exemptions)

某些特殊场景需要豁免 PSS 检查:

```yaml
# /etc/kubernetes/admission-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "baseline"                # 全局默认强制级别
      enforce-version: "v1.28"
      audit: "restricted"
      audit-version: "v1.28"
      warn: "restricted"
      warn-version: "v1.28"
    exemptions:
      usernames:                         # 豁免用户
        - "system:serviceaccount:kube-system:kube-proxy"
      runtimeClasses:                    # 豁免运行时类
        - "sandbox"
      namespaces:                        # 豁免命名空间
        - "kube-system"
        - "kube-public"
        - "kube-node-lease"
```

#### 配置 API Server 使用豁免配置

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    - --admission-control-config-file=/etc/kubernetes/admission-config.yaml
    volumeMounts:
    - name: admission-config
      mountPath: /etc/kubernetes/admission-config.yaml
      readOnly: true
  volumes:
  - name: admission-config
    hostPath:
      path: /etc/kubernetes/admission-config.yaml
      type: File
```

### 审计日志分析

```bash
# 1. 查看 PodSecurity 审计事件
cat /var/log/kubernetes/audit/audit.log | grep "pod-security.kubernetes.io/audit" | jq .

# 2. 统计违规 Pod
cat /var/log/kubernetes/audit/audit.log | \
  jq -r 'select(.annotations["pod-security.kubernetes.io/audit-violations"] != null) |
         "\(.objectRef.namespace)/\(.objectRef.name): \(.annotations["pod-security.kubernetes.io/audit-violations"])"' | \
  sort | uniq -c | sort -rn

# 3. 查看特定命名空间的违规
kubectl get events -n my-namespace --field-selector reason=FailedCreate | grep "pod-security"
```

---

## YAML 配置示例

### 示例 1: Baseline 级别最小配置

```yaml
# baseline-minimal.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: baseline-app
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.28

---
apiVersion: v1
kind: Pod
metadata:
  name: nginx
  namespace: baseline-app
spec:
  containers:
  - name: nginx
    image: nginx:1.27
    # Baseline 不强制 securityContext
    ports:
    - containerPort: 80
```

### 示例 2: Restricted 级别完整配置

```yaml
# restricted-full.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: restricted-app
  labels:
    # 强制执行 Restricted
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

---
apiVersion: v1
kind: Pod
metadata:
  name: secure-web
  namespace: restricted-app
spec:
  # Pod 级别 SecurityContext
  securityContext:
    # 必须: 非 root 运行
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 3000
    fsGroup: 2000
    # 必须: Seccomp 配置
    seccompProfile:
      type: RuntimeDefault
    # 可选: SELinux 配置
    seLinuxOptions:
      level: "s0:c123,c456"
  
  containers:
  - name: web
    image: nginx:1.27-alpine
    
    # Container 级别 SecurityContext
    securityContext:
      # 必须: 禁止权限提升
      allowPrivilegeEscalation: false
      # 必须: 删除所有 Capabilities
      capabilities:
        drop: ["ALL"]
        add: ["NET_BIND_SERVICE"]  # 可选: 绑定 <1024 端口
      # 推荐: 只读根文件系统
      readOnlyRootFilesystem: true
      # 必须: 非 root 运行 (如果未在 Pod 级别设置)
      runAsNonRoot: true
      runAsUser: 1000
    
    ports:
    - containerPort: 8080
    
    volumeMounts:
    # 只读配置
    - name: config
      mountPath: /etc/nginx/conf.d
      readOnly: true
    # 可写临时目录 (emptyDir)
    - name: cache
      mountPath: /var/cache/nginx
    - name: run
      mountPath: /var/run
    
    resources:
      requests:
        memory: "64Mi"
        cpu: "100m"
      limits:
        memory: "128Mi"
        cpu: "200m"
  
  volumes:
  # Restricted 允许的卷类型
  - name: config
    configMap:
      name: nginx-config
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
```

### 示例 3: 多容器 Pod (Sidecar 模式)

```yaml
# restricted-multi-container.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecar
  namespace: restricted-app
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 3000
    seccompProfile:
      type: RuntimeDefault
  
  # Init 容器
  initContainers:
  - name: init-setup
    image: busybox:1.36
    command: ['sh', '-c', 'echo "Setup complete" > /work-dir/init.txt']
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop: ["ALL"]
      runAsNonRoot: true
      runAsUser: 1000
    volumeMounts:
    - name: workdir
      mountPath: /work-dir
  
  # 主容器
  containers:
  - name: app
    image: myapp:1.0
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop: ["ALL"]
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 1000
    volumeMounts:
    - name: workdir
      mountPath: /app/data
      readOnly: true
    - name: tmp
      mountPath: /tmp
  
  # Sidecar 容器 (日志收集)
  - name: log-collector
    image: fluent/fluent-bit:3.0
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop: ["ALL"]
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 1000
    volumeMounts:
    - name: log
      mountPath: /var/log
      readOnly: true
    - name: fluent-config
      mountPath: /fluent-bit/etc
      readOnly: true
  
  volumes:
  - name: workdir
    emptyDir: {}
  - name: tmp
    emptyDir: {}
  - name: log
    emptyDir: {}
  - name: fluent-config
    configMap:
      name: fluent-bit-config
```

### 示例 4: Deployment 生产配置

```yaml
# restricted-deployment-production.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
data:
  app.conf: |
    server {
      listen 8080;
      location / {
        return 200 "Secure App\n";
      }
    }

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
  namespace: production
  labels:
    app: secure-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: secure-app
  template:
    metadata:
      labels:
        app: secure-app
      annotations:
        # 可选: 强制使用 AppArmor
        container.apparmor.security.beta.kubernetes.io/app: runtime/default
    spec:
      # Pod 级别安全配置
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        runAsGroup: 3000
        fsGroup: 2000
        seccompProfile:
          type: RuntimeDefault
        # 可选: 设置 Supplemental Groups
        supplementalGroups: [4000]
      
      # 服务账号 (遵循最小权限原则)
      serviceAccountName: secure-app-sa
      automountServiceAccountToken: false  # 如果不需要访问 API Server
      
      containers:
      - name: app
        image: nginx:1.27-alpine
        imagePullPolicy: IfNotPresent
        
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
            add: ["NET_BIND_SERVICE"]
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000
        
        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        
        # 存活探针
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 10
        
        # 就绪探针
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        
        # 资源限制 (推荐配置)
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "256Mi"
            cpu: "200m"
        
        volumeMounts:
        - name: config
          mountPath: /etc/nginx/conf.d
          readOnly: true
        - name: cache
          mountPath: /var/cache/nginx
        - name: run
          mountPath: /var/run
        - name: tmp
          mountPath: /tmp
      
      volumes:
      - name: config
        configMap:
          name: app-config
      - name: cache
        emptyDir:
          sizeLimit: 100Mi
      - name: run
        emptyDir:
          sizeLimit: 10Mi
      - name: tmp
        emptyDir:
          sizeLimit: 50Mi
      
      # Pod 反亲和性 (高可用)
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: secure-app
              topologyKey: kubernetes.io/hostname

---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: secure-app-sa
  namespace: production
automountServiceAccountToken: false

---
apiVersion: v1
kind: Service
metadata:
  name: secure-app
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: secure-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
```

### 示例 5: StatefulSet 持久化存储

```yaml
# restricted-statefulset.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: secure-db
  namespace: restricted-app
spec:
  serviceName: "db"
  replicas: 3
  selector:
    matchLabels:
      app: db
  template:
    metadata:
      labels:
        app: db
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 999  # MySQL 用户
        fsGroup: 999
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: mysql
        image: mysql:8.0
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
            add: ["NET_BIND_SERVICE", "CHOWN", "SETUID", "SETGID"]
          runAsNonRoot: true
          runAsUser: 999
        
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-secret
              key: password
        
        ports:
        - name: mysql
          containerPort: 3306
        
        volumeMounts:
        # Restricted 允许 PVC
        - name: data
          mountPath: /var/lib/mysql
        - name: config
          mountPath: /etc/mysql/conf.d
          readOnly: true
        - name: tmp
          mountPath: /tmp
        
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "1000m"
      
      volumes:
      - name: config
        configMap:
          name: mysql-config
      - name: tmp
        emptyDir: {}
  
  # PVC 模板
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 10Gi
```

### 示例 6: 特权系统组件 (Privileged)

```yaml
# privileged-system-component.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: system-monitoring
  labels:
    # 系统组件需要特权模式
    pod-security.kubernetes.io/enforce: privileged
    pod-security.kubernetes.io/audit: privileged
    pod-security.kubernetes.io/warn: privileged

---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: system-monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      # Privileged 允许 hostNetwork/hostPID
      hostNetwork: true
      hostPID: true
      
      containers:
      - name: exporter
        image: prom/node-exporter:v1.8.0
        
        # Privileged 允许特权容器
        securityContext:
          privileged: true
        
        args:
        - --path.procfs=/host/proc
        - --path.sysfs=/host/sys
        - --path.rootfs=/host/root
        - --collector.filesystem.mount-points-exclude=^/(sys|proc|dev|host|etc)($$|/)
        
        # Privileged 允许 hostPort
        ports:
        - name: metrics
          containerPort: 9100
          hostPort: 9100
        
        volumeMounts:
        # Privileged 允许 hostPath
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: sys
          mountPath: /host/sys
          readOnly: true
        - name: root
          mountPath: /host/root
          readOnly: true
          mountPropagation: HostToContainer
        
        resources:
          requests:
            memory: "100Mi"
            cpu: "50m"
          limits:
            memory: "200Mi"
            cpu: "100m"
      
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
      - name: root
        hostPath:
          path: /
      
      tolerations:
      - effect: NoSchedule
        operator: Exists
```

### 示例 7: Job 批处理任务

```yaml
# restricted-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processor
  namespace: restricted-app
spec:
  ttlSecondsAfterFinished: 3600  # 完成后 1 小时清理
  backoffLimit: 3
  template:
    metadata:
      labels:
        app: data-processor
    spec:
      restartPolicy: OnFailure
      
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 3000
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: processor
        image: python:3.12-alpine
        command: ["python", "/app/process.py"]
        
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000
        
        env:
        - name: DATA_SOURCE
          valueFrom:
            configMapKeyRef:
              name: job-config
              key: source
        
        volumeMounts:
        - name: script
          mountPath: /app
          readOnly: true
        - name: work
          mountPath: /work
        - name: tmp
          mountPath: /tmp
        
        resources:
          requests:
            memory: "256Mi"
            cpu: "200m"
          limits:
            memory: "512Mi"
            cpu: "500m"
      
      volumes:
      - name: script
        configMap:
          name: processor-script
      - name: work
        emptyDir: {}
      - name: tmp
        emptyDir: {}
```

---

## 版本兼容性

### PSS/PSA 演进时间线

| 版本 | 状态 | 重要变化 |
|------|------|----------|
| **v1.21** | - | PSP 宣布废弃 |
| **v1.22** | Alpha | 引入 PSS/PSA (默认禁用) |
| **v1.23** | Beta | PSA 默认启用,支持 warn/audit 模式 |
| **v1.24** | Beta | 优化性能,增强审计能力 |
| **v1.25** | GA | PSS/PSA 稳定版,PSP 完全移除 |
| **v1.26** | GA | 支持更细粒度的豁免配置 |
| **v1.27** | GA | 增强 Capabilities 检查 |
| **v1.28** | GA | 支持自定义 Seccomp 配置文件 |
| **v1.29** | GA | 优化 hostPort 检查逻辑 |
| **v1.30** | GA | 增强 Windows 容器支持 |
| **v1.31** | GA | 支持更多卷类型白名单配置 |
| **v1.32** | GA | 性能优化,降低准入延迟 |

### 版本特性对照表

| 特性 | v1.23 | v1.25 | v1.28 | v1.32 |
|------|-------|-------|-------|-------|
| **PSA 默认启用** | ✅ | ✅ | ✅ | ✅ |
| **PSP 支持** | ⚠️ 废弃 | ❌ 移除 | ❌ 移除 | ❌ 移除 |
| **三种安全级别** | ✅ | ✅ | ✅ | ✅ |
| **Namespace 标签** | ✅ | ✅ | ✅ | ✅ |
| **版本锁定** | ✅ | ✅ | ✅ | ✅ |
| **豁免配置** | ⚠️ 基础 | ✅ 完整 | ✅ 增强 | ✅ 增强 |
| **审计日志** | ✅ | ✅ | ✅ | ✅ |
| **Seccomp 强制** | ✅ | ✅ | ✅ 自定义 | ✅ 增强 |
| **hostPort 检查** | ✅ 基础 | ✅ 基础 | ✅ 基础 | ✅ 优化 |
| **Windows 支持** | ⚠️ 实验 | ✅ | ✅ | ✅ 增强 |

### 跨版本升级注意事项

#### 从 v1.24 升级到 v1.25+

```yaml
# 升级前检查清单
1. 确认所有 PSP 已迁移到 PSS
   kubectl get psp  # 应该为空或即将删除

2. 为所有命名空间添加 PSS 标签
   kubectl get ns --show-labels | grep pod-security

3. 测试升级后行为
   # 在测试集群先升级,验证工作负载正常

4. 更新 CI/CD 管道
   # 移除 PSP 相关的 RBAC 配置
```

#### 从 v1.25 升级到 v1.28+

```yaml
# 利用新特性
1. 使用自定义 Seccomp 配置
   securityContext:
     seccompProfile:
       type: Localhost
       localhostProfile: profiles/audit.json

2. 更新豁免配置
   # /etc/kubernetes/admission-config.yaml
   exemptions:
     runtimeClasses: ["kata", "gvisor"]  # 新增运行时类豁免
```

---

## 最佳实践

### 1. 命名空间策略规划

```yaml
# 推荐的命名空间分层策略

# Tier 1: 系统命名空间 (Privileged)
- kube-system
- kube-public
- kube-node-lease
- monitoring (需要 hostPath)
- logging (需要 hostPath)

# Tier 2: 中间件命名空间 (Baseline)
- databases
- message-queues
- caching
- storage

# Tier 3: 业务应用命名空间 (Restricted)
- production
- staging
- development

# Tier 4: 多租户命名空间 (Restricted)
- tenant-a
- tenant-b
```

#### 实施方案

```yaml
# tier1-system.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    pod-security.kubernetes.io/enforce: privileged

---
# tier2-middleware.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: databases
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted    # 审计更严格级别
    pod-security.kubernetes.io/warn: restricted

---
# tier3-business.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 2. 渐进式策略实施

```yaml
# 阶段 1: 观察模式 (第 1-2 周)
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  labels:
    # 不强制执行,只审计和警告
    pod-security.kubernetes.io/audit: baseline
    pod-security.kubernetes.io/warn: baseline

# 收集审计日志,识别违规工作负载

---
# 阶段 2: 部分强制 (第 3-4 周)
metadata:
  labels:
    # 强制执行 Baseline
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.28
    # 审计 Restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

# 修复违规工作负载

---
# 阶段 3: 完全强制 (第 5+ 周)
metadata:
  labels:
    # 强制执行 Restricted
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 3. 安全配置模板

#### 通用 SecurityContext 模板

```yaml
# security-context-template.yaml
# 推荐所有工作负载使用此模板

# Pod 级别
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  runAsGroup: 3000
  fsGroup: 2000
  seccompProfile:
    type: RuntimeDefault

# Container 级别
containerSecurityContext:
  allowPrivilegeEscalation: false
  capabilities:
    drop: ["ALL"]
    # 可选: 根据需要添加
    # add: ["NET_BIND_SERVICE"]
  readOnlyRootFilesystem: true
  runAsNonRoot: true
  runAsUser: 1000
```

#### Kustomize 集成

```yaml
# kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 统一为所有 Pod 注入安全配置
patches:
- target:
    kind: Pod
  patch: |-
    - op: add
      path: /spec/securityContext
      value:
        runAsNonRoot: true
        runAsUser: 1000
        seccompProfile:
          type: RuntimeDefault

- target:
    kind: Pod
  patch: |-
    - op: add
      path: /spec/containers/0/securityContext
      value:
        allowPrivilegeEscalation: false
        capabilities:
          drop: ["ALL"]
        readOnlyRootFilesystem: true

resources:
- deployment.yaml
- service.yaml
```

### 4. 镜像构建最佳实践

```dockerfile
# Dockerfile - 遵循 Restricted 标准

FROM node:20-alpine

# 创建非 root 用户
RUN addgroup -g 1000 appuser && \
    adduser -D -u 1000 -G appuser appuser

# 设置工作目录并修改权限
WORKDIR /app
COPY --chown=appuser:appuser package*.json ./
RUN npm ci --only=production

COPY --chown=appuser:appuser . .

# 切换到非 root 用户
USER 1000

# 暴露非特权端口
EXPOSE 8080

CMD ["node", "server.js"]
```

### 5. 监控和告警

```yaml
# prometheus-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pss-violations
spec:
  groups:
  - name: pod-security
    interval: 30s
    rules:
    # 告警: PSS 违规次数过多
    - alert: HighPSSViolationRate
      expr: |
        rate(apiserver_admission_webhook_rejection_count{
          name="PodSecurity",
          type="enforce"
        }[5m]) > 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "PSS 违规率过高"
        description: "命名空间 {{ $labels.namespace }} 在过去 10 分钟内有 {{ $value }} 次/秒的 PSS 违规"

    # 告警: Privileged 命名空间过多
    - alert: TooManyPrivilegedNamespaces
      expr: |
        count(kube_namespace_labels{
          label_pod_security_kubernetes_io_enforce="privileged"
        }) > 5
      for: 1h
      labels:
        severity: info
      annotations:
        summary: "特权命名空间过多"
        description: "集群中有 {{ $value }} 个命名空间使用 Privileged 模式"
```

### 6. CI/CD 集成

```yaml
# .gitlab-ci.yml
stages:
  - validate
  - test
  - deploy

# PSS 验证阶段
validate:pss:
  stage: validate
  image: bitnami/kubectl:latest
  script:
    # 使用 dry-run 验证 PSS 合规性
    - |
      kubectl apply -f manifests/ --dry-run=server \
        --namespace=production 2>&1 | tee validation.log
    # 检查是否有 PSS 警告
    - |
      if grep -i "pod-security" validation.log; then
        echo "警告: 检测到 PSS 违规"
        exit 1
      fi
  artifacts:
    paths:
      - validation.log
    when: on_failure

# OPA 策略测试
test:opa:
  stage: test
  image: openpolicyagent/conftest:latest
  script:
    # 使用 OPA 强制执行 PSS Restricted
    - conftest test manifests/ -p policy/

# policy/pss-restricted.rego
package main

deny[msg] {
  input.kind == "Pod"
  not input.spec.securityContext.runAsNonRoot
  msg = "必须设置 runAsNonRoot: true"
}

deny[msg] {
  input.kind == "Pod"
  container := input.spec.containers[_]
  not container.securityContext.allowPrivilegeEscalation == false
  msg = sprintf("容器 %v 必须设置 allowPrivilegeEscalation: false", [container.name])
}
```

---

## 常见问题 FAQ

### Q1: PSS 和 PSP 的主要区别是什么?

**A**: 

| 维度 | PSP (已废弃) | PSS/PSA (推荐) |
|------|-------------|----------------|
| **复杂度** | 高 (需要 RBAC 绑定) | 低 (Namespace 标签) |
| **灵活性** | 高 (完全自定义) | 中 (预定义 3 个级别) |
| **易用性** | 低 (学习曲线陡峭) | 高 (开箱即用) |
| **维护成本** | 高 (复杂配置) | 低 (简单标签) |
| **审计能力** | 有限 | 内置 audit/warn 模式 |

### Q2: 如何判断应该使用哪个安全级别?

**A**:

```yaml
# 决策树
命名空间类型:
  系统组件 (kube-system, monitoring):
    → Privileged
    原因: 需要 hostPath, hostNetwork 等特权功能

  中间件 (数据库, 消息队列):
    → Baseline
    原因: 可能需要特定 capabilities,但不需要完全特权

  业务应用:
    内部可信应用:
      → Baseline (推荐) 或 Restricted
    外部多租户应用:
      → Restricted (强制)

# 实施建议
1. 默认使用 Baseline (平衡安全性和兼容性)
2. 高安全要求场景使用 Restricted
3. 仅系统组件使用 Privileged
```

### Q3: Restricted 级别下如何运行需要 hostPath 的应用?

**A**:

```yaml
# 解决方案 1: 调整命名空间级别 (推荐)
apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
  labels:
    pod-security.kubernetes.io/enforce: baseline  # hostPath 需要 Baseline

---
# 解决方案 2: 使用 PVC 替代 hostPath
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-storage
spec:
  capacity:
    storage: 10Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/data
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - node-1

---
# Pod 使用 PVC (Restricted 允许)
apiVersion: v1
kind: Pod
metadata:
  name: app
  namespace: restricted-app
spec:
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: local-storage-pvc  # ✅ Restricted 允许
```

### Q4: 如何为现有集群添加 PSS 而不中断业务?

**A**:

```yaml
# 四步渐进式实施

# 步骤 1: 添加 audit/warn 标签 (第 1-2 周)
kubectl label ns production \
  pod-security.kubernetes.io/audit=baseline \
  pod-security.kubernetes.io/warn=baseline

# 观察审计日志,识别违规 Pod
kubectl get events -n production | grep pod-security

# 步骤 2: 修复违规工作负载 (第 3-4 周)
# 根据审计日志逐步修复

# 步骤 3: 启用 enforce (第 5 周)
kubectl label ns production \
  pod-security.kubernetes.io/enforce=baseline \
  --overwrite

# 步骤 4: 提升到 Restricted (第 6+ 周)
kubectl label ns production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/enforce-version=v1.28 \
  --overwrite

# 关键: 保持 audit 和 warn 始终比 enforce 更严格
# 例如: enforce=baseline, audit/warn=restricted
```

### Q5: Pod 被 PSS 拒绝,如何调试?

**A**:

```bash
# 1. 查看拒绝原因
kubectl apply -f pod.yaml 2>&1 | grep "violates PodSecurity"

# 输出示例:
# Error from server (Forbidden): error when creating "pod.yaml": 
# pods "test" is forbidden: violates PodSecurity "restricted:v1.28": 
# allowPrivilegeEscalation != false (container "app" must set 
# securityContext.allowPrivilegeEscalation=false), 
# unrestricted capabilities (container "app" must set 
# securityContext.capabilities.drop=["ALL"])

# 2. 检查命名空间标签
kubectl get ns production -o jsonpath='{.metadata.labels}' | jq .

# 3. 使用 dry-run 测试
kubectl apply -f pod.yaml --dry-run=server -v=8

# 4. 查看审计日志
cat /var/log/kubernetes/audit/audit.log | \
  grep "pod-security" | \
  jq '.annotations["pod-security.kubernetes.io/enforce-policy"]'
```

### Q6: 多个标签冲突时的行为?

**A**:

```yaml
# 场景: 命名空间同时设置多个模式
apiVersion: v1
kind: Namespace
metadata:
  name: test
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

# 行为:
# 1. enforce: baseline → 违反 Baseline 的 Pod 会被拒绝
# 2. audit: restricted → 违反 Restricted 的 Pod 会记录审计日志 (即使被允许创建)
# 3. warn: restricted → 违反 Restricted 的 Pod 会返回警告 (即使被允许创建)

# 示例: 创建一个违反 Restricted 但符合 Baseline 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: test-pod
  namespace: test
spec:
  containers:
  - name: app
    image: nginx:1.27
    # 符合 Baseline (无 hostPath, 无 privileged)
    # 但违反 Restricted (缺少 runAsNonRoot, allowPrivilegeEscalation=false)

# 结果:
# ✅ Pod 创建成功 (因为 enforce=baseline)
# ⚠️ 返回警告信息 (因为 warn=restricted)
# 📋 记录审计日志 (因为 audit=restricted)
```

### Q7: 如何豁免特定 Pod?

**A**:

```yaml
# 方法 1: 命名空间级别豁免 (推荐)
# 为特殊命名空间设置 Privileged
apiVersion: v1
kind: Namespace
metadata:
  name: special-system
  labels:
    pod-security.kubernetes.io/enforce: privileged

---
# 方法 2: 全局豁免配置
# /etc/kubernetes/admission-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    exemptions:
      usernames:
        - "system:serviceaccount:kube-system:*"  # 豁免所有 kube-system SA
      runtimeClasses:
        - "kata-containers"                      # 豁免特定运行时类
      namespaces:
        - "kube-system"
        - "kube-public"

---
# 方法 3: 使用 RuntimeClass 豁免
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: privileged-runtime
handler: runc
# 在豁免配置中引用此 RuntimeClass

---
apiVersion: v1
kind: Pod
metadata:
  name: special-pod
spec:
  runtimeClassName: privileged-runtime  # 使用豁免的运行时类
  # 此 Pod 将绕过 PSS 检查
```

### Q8: PSS 是否影响性能?

**A**:

```yaml
# 性能影响评估

影响因素:
1. 准入延迟: +2-5ms (轻微)
   - PSS 是内置轻量级检查
   - 相比 Webhook 准入控制器快 10-100 倍

2. API Server 负载: 可忽略 (<1% CPU)
   - 策略检查是内存操作
   - 无需外部调用

3. 审计日志: 轻微磁盘 I/O
   - 仅 audit 模式产生
   - 可配置日志轮转

# 性能优化
1. 使用版本锁定避免动态解析
   pod-security.kubernetes.io/enforce-version: v1.28  # 推荐

2. 减少不必要的 audit/warn 标签
   # 生产环境
   enforce: restricted
   audit: restricted      # 同级别,减少重复检查
   warn: restricted

3. 合理配置豁免
   # 豁免系统命名空间,减少检查次数
   exemptions:
     namespaces: ["kube-system", "kube-public"]

# 基准测试结果 (1000 Pods 创建)
无 PSS:              平均 50ms
PSS Baseline:        平均 53ms (+6%)
PSS Restricted:      平均 55ms (+10%)
```

---

## 生产案例

### 案例 1: 金融行业多租户平台

**背景**: 
- 50+ 租户命名空间
- 严格合规要求 (PCI DSS)
- 需要审计追踪

**实施方案**:

```yaml
# 1. 全局默认策略 (API Server 配置)
# /etc/kubernetes/admission-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "restricted"              # 全局默认 Restricted
      enforce-version: "v1.28"
      audit: "restricted"
      audit-version: "latest"
      warn: "restricted"
      warn-version: "latest"
    exemptions:
      namespaces:
        - "kube-system"
        - "kube-public"
        - "kube-node-lease"
        - "monitoring"
        - "logging"

---
# 2. 租户命名空间模板
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-{{ .TenantID }}
  labels:
    tenant: "{{ .TenantID }}"
    # 强制 Restricted (继承全局默认)
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28
  annotations:
    owner: "{{ .TenantEmail }}"

---
# 3. ResourceQuota 配合使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-{{ .TenantID }}
spec:
  hard:
    requests.cpu: "10"
    requests.memory: "20Gi"
    persistentvolumeclaims: "10"
    pods: "50"

---
# 4. NetworkPolicy 隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-{{ .TenantID }}
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: "{{ .TenantID }}"  # 只允许同租户通信
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          tenant: "{{ .TenantID }}"
  - to:  # 允许访问 DNS
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
```

**效果**:
- ✅ 100% 租户命名空间强制 Restricted
- ✅ 完整审计日志支持合规检查
- ✅ 6 个月 0 安全事故

### 案例 2: 电商平台 PSP 迁移

**背景**:
- Kubernetes v1.24 → v1.26 升级
- 150+ 微服务工作负载
- 零停机迁移要求

**迁移时间线**:

```yaml
# 第 1 周: 分析评估
1. 导出所有 PSP 策略
   kubectl get psp -o yaml > psp-backup.yaml

2. 分析工作负载分布
   kubectl get pods -A -o json | \
     jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"' | \
     wc -l
   # 结果: 1200+ Pods

3. 映射到 PSS 级别
   - kube-system (5 PSPs) → Privileged
   - middleware (8 PSPs) → Baseline
   - business (2 PSPs) → Restricted

---
# 第 2-3 周: 渐进式标签添加
# 脚本: add-pss-labels.sh
#!/bin/bash
declare -A NS_LEVEL=(
  ["kube-system"]="privileged"
  ["monitoring"]="privileged"
  ["logging"]="privileged"
  ["mysql"]="baseline"
  ["redis"]="baseline"
  ["kafka"]="baseline"
)

for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  level=${NS_LEVEL[$ns]:-"restricted"}  # 默认 Restricted
  
  echo "配置命名空间 $ns → $level"
  kubectl label ns "$ns" \
    pod-security.kubernetes.io/audit="$level" \
    pod-security.kubernetes.io/warn="$level" \
    --overwrite
done

---
# 第 4-5 周: 修复违规工作负载
# 常见问题和修复

# 问题 1: 80+ Pods 缺少 securityContext
# 解决: 使用 Kustomize 批量注入
# kustomization.yaml
patches:
- target:
    kind: Deployment
  patch: |-
    - op: add
      path: /spec/template/spec/securityContext
      value:
        runAsNonRoot: true
        runAsUser: 1000
        seccompProfile:
          type: RuntimeDefault

# 问题 2: 12 个服务使用 hostPath
# 解决: 迁移到 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data
spec:
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 10Gi

# 问题 3: 5 个遗留服务以 root 运行
# 解决: 重构镜像
# 修改 Dockerfile,添加非 root 用户

---
# 第 6 周: 启用 enforce (金丝雀)
# 先在 10% 命名空间启用
kubectl label ns production-canary \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/enforce-version=v1.26 \
  --overwrite

# 监控 24 小时,无问题后扩展

---
# 第 7-8 周: 全面 enforce
for ns in $(kubectl get ns -l tier=business -o name); do
  kubectl label "$ns" \
    pod-security.kubernetes.io/enforce=restricted \
    pod-security.kubernetes.io/enforce-version=v1.26 \
    --overwrite
  sleep 60  # 间隔 1 分钟,避免突发影响
done

---
# 第 9 周: 升级到 v1.26
# PSP 自动失效,PSS 完全接管

---
# 第 10 周: 清理 PSP 资源
kubectl delete psp --all
kubectl delete clusterrole psp:*
kubectl delete clusterrolebinding psp:*
```

**结果**:
- ✅ 零停机完成迁移
- ✅ 所有业务应用达到 Restricted 级别
- ✅ 安全评分提升 40%

### 案例 3: AI/ML 平台 GPU 工作负载

**背景**:
- GPU 节点需要特殊设备访问
- 用户提交的训练任务需要隔离
- 部分任务需要 hostPath 访问数据集

**解决方案**:

```yaml
# 1. 分层命名空间设计
# Tier 1: 系统组件 (GPU Device Plugin)
apiVersion: v1
kind: Namespace
metadata:
  name: gpu-system
  labels:
    pod-security.kubernetes.io/enforce: privileged  # GPU 插件需要

---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nvidia-device-plugin
  namespace: gpu-system
spec:
  selector:
    matchLabels:
      name: nvidia-device-plugin
  template:
    metadata:
      labels:
        name: nvidia-device-plugin
    spec:
      # Privileged 允许设备访问
      hostNetwork: true
      containers:
      - name: nvidia-device-plugin
        image: nvcr.io/nvidia/k8s-device-plugin:v0.15.0
        securityContext:
          privileged: true  # 需要访问 GPU 设备
        volumeMounts:
        - name: device-plugin
          mountPath: /var/lib/kubelet/device-plugins
      volumes:
      - name: device-plugin
        hostPath:
          path: /var/lib/kubelet/device-plugins

---
# Tier 2: 数据集存储 (Baseline)
apiVersion: v1
kind: Namespace
metadata:
  name: ml-datasets
  labels:
    pod-security.kubernetes.io/enforce: baseline  # 允许 hostPath

---
# 使用 HostPath PV 提供数据集访问
apiVersion: v1
kind: PersistentVolume
metadata:
  name: imagenet-dataset
spec:
  capacity:
    storage: 1Ti
  accessModes:
  - ReadOnlyMany
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-datasets
  local:
    path: /data/imagenet
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: node-role.kubernetes.io/ml-storage
          operator: Exists

---
# Tier 3: 用户训练任务 (Restricted)
apiVersion: v1
kind: Namespace
metadata:
  name: ml-training
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.28

---
# 用户训练 Job
apiVersion: batch/v1
kind: Job
metadata:
  name: train-resnet
  namespace: ml-training
spec:
  template:
    spec:
      # Restricted 要求
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 100
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: trainer
        image: pytorch/pytorch:2.1.0-cuda12.1
        
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
          runAsNonRoot: true
          runAsUser: 1000
        
        # GPU 资源请求 (不需要特权)
        resources:
          limits:
            nvidia.com/gpu: 1
        
        volumeMounts:
        # 使用 PVC 访问数据集 (Restricted 允许)
        - name: dataset
          mountPath: /data
          readOnly: true
        - name: output
          mountPath: /output
        - name: tmp
          mountPath: /tmp
      
      volumes:
      - name: dataset
        persistentVolumeClaim:
          claimName: imagenet-pvc
      - name: output
        persistentVolumeClaim:
          claimName: training-output
      - name: tmp
        emptyDir: {}
      
      restartPolicy: OnFailure
```

**架构图**:

```
┌─────────────────────────────────────────────────────────────┐
│                    ML Platform                               │
├─────────────────────────────────────────────────────────────┤
│  gpu-system (Privileged)                                    │
│    └─ nvidia-device-plugin DaemonSet                        │
│       └─ 管理 GPU 设备,需要 privileged                       │
├─────────────────────────────────────────────────────────────┤
│  ml-datasets (Baseline)                                     │
│    └─ HostPath PV → Local Storage                          │
│       └─ 提供大规模数据集访问                                │
├─────────────────────────────────────────────────────────────┤
│  ml-training (Restricted)                                   │
│    └─ User Training Jobs                                    │
│       ├─ 通过 PVC 访问数据集 (无 hostPath)                   │
│       ├─ GPU 资源请求 (nvidia.com/gpu)                      │
│       └─ 完全隔离,非 root 运行                               │
└─────────────────────────────────────────────────────────────┘
```

**效果**:
- ✅ GPU 工作负载正常运行
- ✅ 用户任务达到 Restricted 级别
- ✅ 数据集访问性能无损失

---

## 相关文档

- [Kubernetes 官方文档: Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [Pod Security Admission](https://kubernetes.io/docs/concepts/security/pod-security-admission/)
- [PSP 迁移指南](https://kubernetes.io/docs/tasks/configure-pod-container/migrate-from-psp/)
- [22 - PodSecurityPolicy (PSP) YAML 配置参考](./22-pod-security-policy.md) (已废弃,仅供参考)
- [21 - RBAC 权限控制 YAML 配置参考](./21-rbac-authorization.md)
- [24 - ServiceAccount 配置参考](./24-service-account.md)

---

## 总结

### PSS/PSA 核心要点

1. **三种安全级别**:
   - Privileged: 无限制,仅系统组件
   - Baseline: 禁止已知权限提升,推荐默认
   - Restricted: 严格限制,高安全要求

2. **三种执行模式**:
   - enforce: 拒绝违规 Pod
   - audit: 记录审计日志
   - warn: 返回警告信息

3. **配置方式**: Namespace 标签 (简单直观)

4. **迁移策略**: 渐进式,先 audit/warn,后 enforce

5. **生产建议**:
   - 默认使用 Baseline
   - 业务应用优先 Restricted
   - 版本锁定避免意外

**最佳实践**: 
- ✅ 分层命名空间 (Privileged/Baseline/Restricted)
- ✅ 渐进式策略 (audit → warn → enforce)
- ✅ 统一 SecurityContext 模板
- ✅ CI/CD 集成验证
- ✅ 监控告警 PSS 违规

**关键提醒**: 
- ⚠️ PSP 在 v1.25 已完全移除
- ⚠️ PSS 无法完全替代 OPA/Gatekeeper (自定义策略需求)
- ⚠️ Restricted 级别可能需要修改现有镜像

---

**文档版本**: v1.0  
**最后更新**: 2026-02  
**适用版本**: Kubernetes v1.25 - v1.32
