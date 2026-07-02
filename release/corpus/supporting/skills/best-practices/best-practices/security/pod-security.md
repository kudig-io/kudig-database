---
title: Kubernetes Pod安全最佳实践 [security]
description: 生产环境 Kubernetes Pod安全配置的最佳实践指南
summary: 生产环境 Kubernetes Pod安全配置的最佳实践指南
category: best-practices/security
tags:
- kubernetes
- security
- pod-security
- rbac
- pss
- containerd
- cri-o
- docker
- falco
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- DevOps 工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes Pod安全 最佳实践
- 如何 配置 Kubernetes 安全上下文
- Kubernetes 容器安全 加固
trigger_keywords:
- Kubernetes
- Pod安全
- 安全上下文
- 容器安全
prerequisites:
- kubectl-basics
cross_refs:
- type: domain
  path: ../../domain-05-security-compliance/
  label: 安全知识域
- type: domain
  path: ../../domain-05-security-compliance/
  label: 云原生安全知识域
---



# Kubernetes Pod安全最佳实践

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-05 | **作者**: 系统生成 | **质量等级**: ⭐⭐⭐⭐⭐ 专家级

> **生产环境实战经验总结**: 基于大规模集群安全运维经验，涵盖从Pod安全标准到运行时安全的全方位最佳实践

---

## 概述

本指南提供生产环境 Kubernetes Pod安全配置的最佳实践，帮助团队构建安全、合规、可审计的容器化应用。

### 目标读者

- **安全工程师**: 了解Kubernetes安全架构和Pod安全标准
- **SRE**: 掌握安全配置和故障排查
- **DevOps 工程师**: 学习安全上下文和RBAC配置

### 前置知识

- Kubernetes 核心概念（Pod、Deployment、Service）
- 容器安全基础（镜像安全、运行时安全）
- Linux 安全基础（用户权限、文件系统权限）

---

## 问题描述

### 常见问题

**问题1：容器以root用户运行**
- **症状**：容器内进程以root用户运行
- **原因**：未配置安全上下文，镜像默认使用root
- **影响**：容器逃逸风险增加，安全漏洞扩大

**问题2：特权容器**
- **症状**：容器拥有主机权限
- **原因**：配置了privileged: true
- **影响**：容器可访问主机资源，安全风险极高

**问题3：敏感信息泄露**
- **症状**：密码、密钥等敏感信息暴露
- **原因**：环境变量或配置文件包含敏感信息
- **影响**：敏感信息泄露，安全风险

---

## 解决方案

### Pod安全标准

**Pod安全标准（PSS）级别**：

| 级别 | 描述 | 限制内容 | 适用场景 |
|------|------|---------|---------|
| **Privileged** | 无限制 | 无 | 系统组件、可信工作负载 |
| **Baseline** | 最低限制 | 禁止hostNetwork/hostPID/hostIPC | 大多数应用 |
| **Restricted** | 严格限制 | 必须非root、只读根文件系统 | 安全敏感应用 |

**PSS配置示例**：

```yaml
# 命名空间级别的PSS配置
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 安全上下文配置

**Pod级别安全上下文**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 3000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
    supplementalGroups: [4000]
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsUser: 1000
      capabilities:
        drop:
          - ALL
        add:
          - NET_BIND_SERVICE
      seLinuxOptions:
        type: container_t
```

**容器级别安全上下文**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  containers:
  - name: app
    image: myapp:v1.0
    securityContext:
      # 禁止特权提升
      allowPrivilegeEscalation: false
      # 只读根文件系统
      readOnlyRootFilesystem: true
      # 非root用户
      runAsNonRoot: true
      runAsUser: 1000
      # 丢弃所有能力
      capabilities:
        drop:
          - ALL
      # seccomp配置
      seccompProfile:
        type: RuntimeDefault
    # 可写卷挂载
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
```

### RBAC配置

**最小权限原则**：

```yaml
# 创建专用ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
automountServiceAccountToken: false
---
# 创建角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-role
  namespace: production
rules:
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]
---
# 绑定角色
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-role-binding
  namespace: production
subjects:
- kind: ServiceAccount
  name: app-sa
  namespace: production
roleRef:
  kind: Role
  name: app-role
  apiGroup: rbac.authorization.k8s.io
```

---

## 实施步骤

### 前置条件

**硬件要求**：
- 支持安全上下文的容器运行时
- 支持seccomp的内核版本

**软件要求**：
- Kubernetes：v1.25+
- 容器运行时：containerd 1.6+ / CRI-O 1.24+
- 内核版本：≥ 4.14（seccomp支持）

### 步骤1：启用Pod安全标准

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
#!/bin/bash
# 启用Pod安全标准

# 1. 为生产命名空间启用PSS
kubectl label namespace production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 2. 为开发命名空间启用PSS
kubectl label namespace development \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 3. 验证PSS配置
kubectl get namespace -L pod-security.kubernetes.io/enforce
```

### 步骤2：配置安全上下文

```bash
#!/bin/bash
# 配置安全上下文模板

cat <<EOF > secure-pod-template.yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: production
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 3000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
          - ALL
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache
    - name: run
      mountPath: /var/run
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
EOF

echo "安全Pod模板已创建: secure-pod-template.yaml"
```

### 步骤3：配置RBAC

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
#!/bin/bash
# 配置RBAC

# 1. 创建ServiceAccount
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
automountServiceAccountToken: false
EOF

# 2. 创建角色
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-role
  namespace: production
rules:
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list"]
EOF

# 3. 绑定角色
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-role-binding
  namespace: production
subjects:
- kind: ServiceAccount
  name: app-sa
  namespace: production
roleRef:
  kind: Role
  name: app-role
  apiGroup: rbac.authorization.k8s.io
EOF

# 4. 验证RBAC
kubectl auth can-i list configmaps --as=system:serviceaccount:production:app-sa
```

### 步骤4：配置镜像安全

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
#!/bin/bash
# 配置镜像安全

# 1. 创建镜像策略
cat <<EOF | kubectl apply -f -
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: image-policy-webhook
webhooks:
- name: image-policy.example.com
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE"]
    resources: ["pods"]
  clientConfig:
    service:
      namespace: image-policy
      name: image-policy-webhook
      path: /validate
    caBundle: <CA_BUNDLE>
  admissionReviewVersions: ["v1"]
  sideEffects: None
EOF

# 2. 配置镜像仓库白名单
cat <<EOF > image-policy.yaml
allowedRegistries:
- docker.io/library
- gcr.io/my-project
- registry.example.com
blockedRegistries:
- docker.io/untrusted
EOF

echo "镜像安全策略已配置"
```

---

## 验证方法

### 自动化验证脚本

```bash
#!/bin/bash
# Pod安全配置验证脚本

echo "=== Kubernetes Pod安全配置验证 ==="
echo "验证时间: $(date)"
echo ""

# 1. 检查PSS配置
echo "1. Pod安全标准配置:"
kubectl get namespace -L pod-security.kubernetes.io/enforce
echo ""

# 2. 检查特权容器
echo "2. 特权容器检查:"
kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].securityContext.privileged}{"\n"}{end}' | grep -v "false" | grep -v "<nil>"
echo ""

# 3. 检查root用户容器
echo "3. root用户容器检查:"
kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].securityContext.runAsUser}{"\n"}{end}' | grep -E "^0$|<nil>"
echo ""

# 4. 检查ServiceAccount
echo "4. ServiceAccount配置:"
kubectl get serviceaccounts --all-namespaces
echo ""

# 5. 检查RBAC
echo "5. RBAC配置:"
kubectl get roles,rolebindings --all-namespaces
echo ""

echo "=== 验证完成 ==="
```

### 手动验证清单

**Pod安全标准验证**：
- [ ] PSS配置正确
- [ ] 特权容器已禁止
- [ ] root用户容器已限制
- [ ] 安全上下文配置完整

**RBAC验证**：
- [ ] ServiceAccount配置正确
- [ ] 角色权限最小化
- [ ] 角色绑定正确
- [ ] 自动挂载Token已禁用

**镜像安全验证**：
- [ ] 镜像来源可信
- [ ] 镜像扫描通过
- [ ] 镜像签名验证
- [ ] 镜像策略生效

---

## 常见陷阱

### 陷阱1：忽略init容器安全

**问题**：只配置了主容器安全上下文，忽略了init容器。

**后果**：init容器可能以root运行，存在安全风险。

**正确做法**：
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  initContainers:
  - name: init
    image: busybox:1.36
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
      allowPrivilegeEscalation: false
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
```

### 陷阱2：卷挂载权限不当

**问题**：只读根文件系统但未提供可写卷。

**后果**：应用无法写入临时文件，启动失败。

**正确做法**：
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      readOnlyRootFilesystem: true
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache/nginx
    - name: run
      mountPath: /var/run
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
```

### 陷阱3：能力配置错误

**问题**：丢弃了所有能力但应用需要特定能力。

**后果**：应用功能异常，无法正常运行。

**正确做法**：
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      capabilities:
        drop:
          - ALL
        add:
          - NET_BIND_SERVICE  # 如果需要绑定特权端口
```

---

## 相关资源

### 官方文档
- [Pod安全标准](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [安全上下文](https://kubernetes.io/docs/tasks/configure-pod-container/security-context/)
- [RBAC授权](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

### 工具推荐
- [kube-bench](https://github.com/aquasecurity/kube-bench) - CIS基准检查
- [Polaris](https://github.com/FairwindsOps/polaris) - 最佳实践验证
- [Falco](https://falco.org/) - 运行时安全

### 参考案例
- [Kubernetes安全最佳实践](https://kubernetes.io/docs/concepts/security/overview/)
- [NSA/CISA Kubernetes安全加固指南](https://media.defense.gov/2022/Aug/29/2003066362/-1/-1/0/CTR_KUBERNETES_HARDENING_GUIDANCE_1.2_20220829.PDF)

---

## 版本历史

| 版本 | 日期 | 变更说明 | 作者 |
|------|------|----------|------|
| v1.0 | 2026-05 | 初始版本 | 系统生成 |

---

**最佳实践原则**: 具体、可操作、可验证、可维护

---

**文档维护**：定期审查和更新，确保与Kubernetes版本和安全标准保持同步