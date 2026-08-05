---
title: 容器运行时安全加固指南
description: Kubernetes 容器运行时（containerd/CRI-O）安全加固手册，覆盖 seccomp、AppArmor/SELinux、特权限制、User Namespaces、Falco/Tetragon 与镜像签名强制校验。
summary: 面向 SRE 与安全工程师的容器运行时安全加固指南，包含 seccomp、AppArmor/SELinux、特权容器限制、User Namespaces、Falco/Tetragon 运行时检测与镜像签名强制的可执行步骤。
category: container-runtime
tags:
- production
- best-practices
- playbook
- container-runtime
- containerd
- security
- seccomp
- apparmor
- selinux
- user-namespaces
- falco
- tetragon
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 安全工程师
estimated_read_time: 25min
intent_queries:
- 容器运行时安全加固指南是什么
- 如何配置 seccomp、AppArmor、SELinux
- 如何限制特权容器
- 什么是 User Namespaces
- Falco 与 Tetragon 如何检测运行时威胁
trigger_keywords:
- runtime security
- seccomp
- AppArmor
- SELinux
- privileged
- user namespaces
- Falco
- Tetragon
- 容器运行时安全
prerequisites:
- kubectl-basics
- containerd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 容器运行时安全加固指南

> **适用场景**：在 Kubernetes 生产环境中加固容器运行时安全，降低容器逃逸、权限提升、异常系统调用等风险。
> **目标读者**：SRE、平台工程师、安全工程师。

容器运行时是 Kubernetes 的最后一道隔离边界。即使镜像签名、NetworkPolicy、RBAC 都已就位，运行时的错误配置（如特权容器、宽松的 seccomp、root 用户）仍可能导致节点级入侵。本指南覆盖 seccomp、AppArmor/SELinux、特权限制、User Namespaces、Falco/Tetragon 运行时检测与镜像签名强制校验。

---

## 1. 适用场景与范围

本指南适用于：

- 新集群建设时需要建立运行时安全基线。
- 已有集群需要收敛特权容器、收紧 seccomp/AppArmor/SELinux 策略。
- 多租户或不可信负载场景需要启用 User Namespaces 或 gVisor/Kata。
- 需要部署运行时威胁检测（Falco/Tetragon）并配置告警。

覆盖范围：containerd/CRI-O 运行时参数、Pod Security Standards、seccomp 自定义 profile、AppArmor/SELinux、User Namespaces（alpha/beta）、Falco/Tetragon、镜像签名准入。

---

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认运行时版本与 K8s 版本兼容
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'

# 确认 Kernel 支持 seccomp、AppArmor 或 SELinux
# Ubuntu/Debian 检查 AppArmor
aa-status
# RHEL/CentOS/Rocky 检查 SELinux
getenforce

# 安装 Falco 或 Tetragon CLI
helm repo add falcosecurity https://falcosecurity.github.io/charts/
helm repo add cilium https://helm.cilium.io/
helm repo update
```
---

## 3. 核心概念与架构

### 3.1 运行时安全分层

```
┌─────────────────────────────────────────────┐
│  应用层：只读根文件系统、非 root 用户、distroless 镜像  │
├─────────────────────────────────────────────┤
│  Kubernetes 层：Pod Security Standards / Kyverno    │
├─────────────────────────────────────────────┤
│  容器运行时层：seccomp / AppArmor / SELinux        │
├─────────────────────────────────────────────┤
│  内核层：User Namespaces、Capabilities、cgroups    │
├─────────────────────────────────────────────┤
│  威胁检测层：Falco / Tetragon / auditd              │
└─────────────────────────────────────────────┘
```

### 3.2 关键原则

1. **默认拒绝**：默认禁止 privileged、hostNetwork、hostPID、hostIPC、hostPath。
2. **最小能力**：容器只保留必需的 Linux capabilities，禁止 `CAP_SYS_ADMIN` 等高危能力。
3. **不可变根文件系统**：生产业务容器启用 `readOnlyRootFilesystem: true`。
4. **非 root 用户**：业务容器以非 root UID/GID 运行。
5. **可观测性**：所有运行时异常行为必须进入 SIEM/SOAR。

---

## 4. 标准操作流程

### 4.1 配置 Pod Security Standards

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 为生产命名空间设置 restricted 级别
kubectl label --overwrite ns production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 2. 为系统组件命名空间设置 privileged 并审计
kubectl label --overwrite ns kube-system \
  pod-security.kubernetes.io/enforce=privileged \
  pod-security.kubernetes.io/audit=restricted

# 3. 验证
kubectl auth can-i use podsecuritypolicies --namespace production
kubectl get events -n production --field-selector reason=FailedCreate | grep pod-security
```
### 4.2 启用 seccomp RuntimeDefault

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. containerd 默认启用 RuntimeDefault（containerd v1.6+）
# 检查 containerd 配置
sudo containerd config dump | grep -i seccomp

# 2. 在 Pod 中显式指定 seccompProfile
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: production
spec:
  securityContext:
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: registry.example.com/app:v1.2.3
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 1000
      capabilities:
        drop:
        - ALL
EOF

# 3. 自定义 seccomp profile（需要同步到所有节点 /var/lib/kubelet/seccomp）
# 示例：仅允许少量系统调用
cat <<EOF | sudo tee /var/lib/kubelet/seccomp/custom-restricted.json
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "architectures": ["SCMP_ARCH_X86_64", "SCMP_ARCH_X86"],
  "syscalls": [{
    "names": ["exit", "exit_group", "read", "write", "open", "close"],
    "action": "SCMP_ACT_ALLOW"
  }]
}
EOF
```
### 4.3 AppArmor 与 SELinux 配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# AppArmor（Ubuntu/Debian）
# 1. 创建 profile
sudo tee /etc/apparmor.d/k8s-deny-write <<'EOF'
#include <tunables/global>
profile k8s-deny-write flags=(attach_disconnected) {
  #include <abstractions/base>
  deny /** w,
}
EOF
sudo apparmor_parser -r /etc/apparmor.d/k8s-deny-write

# 2. 在 Pod 中引用
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: apparmor-demo
  annotations:
    container.apparmor.security.beta.kubernetes.io/app: localhost/k8s-deny-write
spec:
  containers:
  - name: app
    image: registry.example.com/app:v1.2.3
EOF

# SELinux（RHEL/CentOS/Rocky）
# 1. 确保 SELinux 为 Enforcing
sudo setenforce 1

# 2. 在 Pod securityContext 中指定 SELinux 上下文
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: selinux-demo
spec:
  securityContext:
    seLinuxOptions:
      level: "s0:c123,c456"
      role: "spc_t"
      type: "spc_t"
      user: "system_u"
  containers:
  - name: app
    image: registry.example.com/app:v1.2.3
EOF
```
### 4.4 启用 User Namespaces（v1.30+ beta，需 feature gate）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认集群启用 UserNamespacesSupport
kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | grep -i UserNamespacesSupport

# 2. 在 Pod 中启用
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: userns-demo
spec:
  hostUsers: false
  containers:
  - name: app
    image: registry.example.com/app:v1.2.3
EOF

# 3. 验证容器内部 root 映射为外部非特权用户
kubectl exec userns-demo -- cat /proc/self/uid_map
```
### 4.5 部署 Falco 运行时检测

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 安装 Falco
helm upgrade --install falco falcosecurity/falco \
  -n falco --create-namespace \
  --set falcosidekick.enabled=true \
  --set falcosidekick.config.slack.webhookurl="<webhook-url>"

# 2. 检查规则加载
kubectl logs -n falco -l app.kubernetes.io/name=falco | grep "Loaded rules"

# 3. 查看高危事件
kubectl logs -n falco -l app.kubernetes.io/name=falco | grep "priority=Critical"
```
### 4.6 部署 Tetragon（Cilium）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 安装 Tetragon
helm upgrade --install tetragon cilium/tetragon -n kube-system

# 2. 查看策略与事件
kubectl get tracingpolicies
kubectl logs -n kube-system -l app.kubernetes.io/name=tetragon
```
### 4.7 强制镜像签名准入（Kyverno）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 参见 [[08-安全/05-供应链/14-supply-chain-security-runbook.md|供应链安全运维 Runbook]]
# 最小示例：禁止未签名镜像
cat <<EOF | kubectl apply -f -
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-image-signature
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
EOF
```
---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|--------|---------|---------|
| PSA 级别 | `kubectl get ns production --show-labels` | enforce=restricted |
| 无特权容器 | `kubectl get pods -A -o json \| jq '.items[].spec.containers[] \| select(.securityContext.privileged==true)'` | 无输出 |
| seccomp 配置 | `kubectl get pod <pod> -o jsonpath='{.spec.securityContext.seccompProfile.type}'` | RuntimeDefault 或 Localhost |
| AppArmor 加载 | `sudo aa-status \| grep k8s-` | profile 处于 enforce 状态 |
| SELinux 模式 | `getenforce` | Enforcing |
| User Namespaces | `kubectl get pod <pod> -o jsonpath='{.spec.hostUsers}'` | false |
| Falco 运行 | `kubectl get pods -n falco` | 全节点 Running |
| 镜像签名验证 | `cosign verify --key cosign.pub <image>:<tag>` | Verification OK |

---

## 6. 常见故障与 Remediation

### 6.1 业务 Pod 被 PSA/Seccomp 拦截

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看事件
kubectl describe pod <pod> -n <ns>

# 2. 临时调整为 audit 模式排查
kubectl label --overwrite ns <ns> pod-security.kubernetes.io/enforce=baseline

# 3. 修复 Pod securityContext 后恢复 restricted
kubectl label --overwrite ns <ns> pod-security.kubernetes.io/enforce=restricted
```
### 6.2 seccomp 自定义 profile 阻止必要 syscall

```bash
# 1. 查看容器内被阻止的 syscall
dmesg | grep -i seccomp

# 2. 在 audit 模式下收集调用日志，补充到 profile
sudo journalctl -k | grep -i seccomp

# 3. 更新 profile 后滚动重启 Pod
```

### 6.3 Falco 误报过多

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看规则文件
kubectl get configmap falco-rules -n falco -o yaml

# 2. 添加例外或调整优先级
# 修改 rules 后 reload
kubectl rollout restart ds/falco -n falco
```
---

## 7. 风险与注意事项

1. **自定义 seccomp 兼容性差**：不同应用需要的 syscall 集合差异大，建议先在 audit 模式运行 2 周再 enforce。
2. **User Namespaces 限制**：v1.30 之前为 alpha，且不支持 hostNetwork、hostPath 等，升级前需验证功能集。
3. **AppArmor 节点级依赖**：自定义 profile 必须同步到所有节点，建议使用 DaemonSet 分发。
4. **SELinux 标签冲突**：某些 CSI 驱动或 HostPath 挂载需要特殊类型，需提前测试。
5. **运行时检测的资源开销**：Falco/Tetragon 会消耗 CPU/内存，并可能产生大量事件，必须配置过滤与采样。

---

## 8. 相关 Runbook / 推荐阅读

- [[14-容器运行时/00-总览/01-production-readiness-operations-guide.md|容器运行时 生产就绪运维指南]]
- [[14-容器运行时/03-containerd-CRI-O/02-containerd-production-operations.md|containerd 生产运维指南]]
- [[14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container.md|Kata Containers 安全容器]]
- [[14-容器运行时/03-containerd-CRI-O/06-gvisor-sandbox-runtime.md|gVisor 沙箱运行时]]
- [[08-安全/05-供应链/14-supply-chain-security-runbook.md|供应链安全运维 Runbook]]
- [[08-安全/00-总览/01-production-readiness-operations-guide.md|安全与合规 生产就绪运维指南]]

---

*本指南应与 Pod Security Standards、NetworkPolicy、镜像签名策略联合使用，形成纵深防御体系。建议每次运行时版本升级后重新验证所有安全 profile 与策略。*


<!-- risk-assessed -->
