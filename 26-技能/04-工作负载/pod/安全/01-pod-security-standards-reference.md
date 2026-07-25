---
title: Pod Security Standards 参考配置
description: Pod Security Standards (PSS) Restricted/Baseline/Privileged 模式配置
summary: 使用 Pod Security Admission 实现 PSS restricted/baseline 级别强制执行，包括 Namespace 级别标签配置
category: manifests-patterns
tags:
- k8s
- manifests
- security
- pod-security
- pss
- admission
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- Pod Security Standards 如何配置
- PSS restricted 模式
- Pod Security Admission
trigger_keywords:
- pod-security
- pss
- restricted
- baseline
- privileged
prerequisites:
- k8s-namespace-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Pod Security Standards 参考配置

## 1. PSS 三级模型

| 级别 | 限制程度 | 适用场景 |
|------|----------|----------|
| **Privileged** | 无限制 | 系统组件（kube-proxy、CNI） |
| **Baseline** | 防止已知提权 | 一般应用最低要求 |
| **Restricted** | 严格安全加固 | 生产应用推荐 |

## 2. Namespace 级别强制执行

通过 Namespace 标签配置 Pod Security Admission：

```yaml
# Restricted 级别（生产推荐）
apiVersion: v1
kind: Namespace
metadata:
  name: production-apps
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: latest
---
# Baseline 级别（开发环境）
apiVersion: v1
kind: Namespace
metadata:
  name: dev-apps
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# Privileged（仅系统命名空间）
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    pod-security.kubernetes.io/enforce: privileged
```

### 2.1 模式说明

| 模式 | 行为 |
|------|------|
| `enforce` | 拒绝不合规的 Pod |
| `audit` | 记录审计事件（不拒绝） |
| `warn` | 向用户返回警告（不拒绝） |

## 3. Restricted 级别符合性 Pod

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
  namespace: production-apps
spec:
  replicas: 3
  selector:
    matchLabels:
      app: secure-app
  template:
    metadata:
      labels:
        app: secure-app
    spec:
      securityContext:
        runAsNonRoot: true         # 禁止 root 运行
        runAsUser: 10001           # 非 0 UID
        runAsGroup: 10001
        fsGroup: 10001
        seccompProfile:
          type: RuntimeDefault      # 默认 seccomp profile
      containers:
        - name: app
          image: registry.example.com/app:v1.0.0
          securityContext:
            allowPrivilegeEscalation: false  # 禁止提权
            readOnlyRootFilesystem: true     # 只读根文件系统
            runAsNonRoot: true
            runAsUser: 10001
            capabilities:
              drop:
                - ALL               # 删除所有 Linux capabilities
              add:
                - NET_BIND_SERVICE  # 仅添加必需的
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          volumeMounts:
            - name: tmp
              mountPath: /tmp       # 可写临时目录
            - name: cache
              mountPath: /app/cache
      volumes:
        - name: tmp
          emptyDir: {}
        - name: cache
          emptyDir: {}
```

## 4. AdmissionConfiguration（集群级配置）

```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
  - name: PodSecurity
    configuration:
      apiVersion: pod-security.admission.config.k8s.io/v1
      kind: PodSecurityConfiguration
      defaults:
        enforce: "baseline"           # 集群默认 enforce 级别
        enforce-version: "latest"
        audit: "restricted"           # 集群默认 audit 级别
        audit-version: "latest"
        warn: "restricted"
        warn-version: "latest"
      exemptions:
        usernames: []
        runtimeClasses: []
        namespaces:
          - kube-system               # 豁免系统命名空间
```

## 5. 常见违规及修复

| 违规 | 修复 |
|------|------|
| `runAsUser: 0` | 改为非 0 UID |
| `privileged: true` | 删除或设为 false |
| 未设置 `seccompProfile` | 添加 `type: RuntimeDefault` |
| 未 drop capabilities | `drop: [ALL]` |
| `allowPrivilegeEscalation: true` | 设为 false |
| hostPath 挂载 | 使用 PVC 或 emptyDir |

## 6. 特权工作负载豁免

```yaml
# 需要特权的系统组件（如 CNI、存储插件）
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    pod-security.kubernetes.io/enforce: privileged
---
# 特权 DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: kube-system
spec:
  template:
    spec:
      hostNetwork: true
      hostPID: true
      containers:
        - name: exporter
          image: prom/node-exporter:v1.7.0
          securityContext:
            runAsUser: 0
            privileged: false
          args:
            - --path.rootfs=/host/root
          volumeMounts:
            - name: rootfs
              mountPath: /host/root
              readOnly: true
      volumes:
        - name: rootfs
          hostPath:
            path: /
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 生产环境使用 `restricted` | 最高安全标准 |
| 渐进式迁移 | 先 `warn` 再 `enforce` |
| CI 中验证 | kubeval/kube-score 检查 PSS 合规 |
| 使用 non-root 镜像 | Dockerfile 中 `USER 10001` |
| 仅必要时豁免 | 记录每个特权工作负载的原因 |

## Related

- [[03-清单模式/06-安全模式/02-networkpolicy-default-deny|NetworkPolicy Default Deny]]
- [[03-清单模式/01-YAML参考/23-pod-security-standards|PSS 完整参考]]

## See Also

- [Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [Pod Security Admission](https://kubernetes.io/docs/concepts/security/pod-security-admission/)

<!-- risk-assessed -->
