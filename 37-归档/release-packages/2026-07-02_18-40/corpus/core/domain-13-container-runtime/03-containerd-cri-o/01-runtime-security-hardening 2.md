---
title: 容器运行时安全加固指南
description: 面向 containerd/CRI-O 的容器运行时安全加固实践，覆盖 seccomp、AppArmor/SELinux、特权容器限制、User Namespaces、Falco/Tetragon 与镜像签名强制校验
category: container-runtime
tags:
- production
- best-practices
- playbook
- container-runtime
- security
- seccomp
- apparmor
- selinux
- user-namespaces
- falco
- tetragon
- supply-chain
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
- 如何加固 containerd/CRI-O 运行时安全
- seccomp AppArmor SELinux UserNamespaces Falco Tetragon 最佳实践
trigger_keywords:
- 运行时安全
- seccomp
- AppArmor
- SELinux
- User Namespaces
- Falco
- Tetragon
- privileged 限制
prerequisites:
- kubectl-basics
- containerd-basics
- linux-basics
- security-basics
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

本指南面向使用 containerd/CRI-O 作为 Kubernetes 容器运行时的生产环境，系统梳理运行时安全加固的六大控制面：seccomp、AppArmor/SELinux、特权容器限制、User Namespaces、运行时威胁检测（Falco/Tetragon）以及镜像签名强制校验。目标是在不影响业务可用性的前提下，最小化容器逃逸与横向移动风险。

> **适用范围**: containerd / CRI-O v1.28-v1.33 | **维护状态**: 持续更新 | **风险等级**: 高 — 涉及安全策略与容器启动

---

## 1. 适用场景与范围

- 新建集群需要落地默认安全基线。
- 存量集群从宽松策略迁移到受限策略。
- 多租户 / 不可信负载场景需要加强隔离。
- 安全审计、合规检查（CIS、等保、PCI-DSS）需要可验证的运行时控制。
- 需要部署运行时威胁检测并建立响应流程。

---

## 2. 前置条件与工具

- 集群已启用 Pod Security Admission（PSA）或 Kyverno / OPA Gatekeeper。
- 节点 OS 支持 AppArmor 或 SELinux（Alibaba Cloud Linux / Ubuntu / RHEL / Bottlerocket）。
- 已安装 `crictl`、`ctr` 或 `podman` 用于运行时调试。
- 已部署 Falco 或 Tetragon DaemonSet。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认节点内核与安全模块
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kernelVersion}{"\t"}{.status.nodeInfo.osImage}{"\n"}{end}'

# 确认 containerd 运行状态
sudo systemctl status containerd --no-pager
```
---

## 3. 核心概念/架构

```
运行时安全纵深防御:
┌─────────────────────────────────────────────────────────┐
│  第一层：准入控制（Kyverno/OPA/PSA）禁止高危配置          │
│  第二层：seccomp/AppArmor/SELinux 限制系统调用与文件访问  │
│  第三层：User Namespaces 隔离容器 root 与宿主机 root      │
│  第四层：运行时检测（Falco/Tetragon）发现异常行为          │
│  第五层：镜像签名强制校验，防止未签名镜像运行              │
└─────────────────────────────────────────────────────────┘
```

---

## 4. 标准操作流程

### 4.1 启用默认 seccomp Profile

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前 Pod seccomp 配置
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.securityContext.seccompProfile.type}'

# 推荐：在 PodSecurity 中强制 RuntimeDefault
kubectl label --overwrite ns production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/enforce-version=v1.30
```
在 Deployment 中显式使用 RuntimeDefault：

```yaml
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
    image: registry.example.com/app:v1.0.0
    securityContext:
      allowPrivilegeEscalation: false
      runAsNonRoot: true
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
```

### 4.2 AppArmor 配置（Ubuntu/Debian）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点 AppArmor 状态
sudo aa-status

# 自定义 profile（示例：限制敏感路径）
sudo tee /etc/apparmor.d/k8s-default <<'EOF'
#include <tunables/global>
profile k8s-default flags=(attach_disconnected,mediate_deleted) {
  #include <abstractions/base>
  deny /proc/sys/** w,
  deny /sys/** w,
  file,
  capability,
  network,
}
EOF
sudo apparmor_parser -r /etc/apparmor.d/k8s-default

# 在 Pod 中引用
kubectl annotate pod secure-app -n production \
  container.apparmor.security.beta.kubernetes.io/app=localhost/k8s-default
```
### 4.3 SELinux 配置（RHEL/CentOS/Alibaba Cloud Linux）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 SELinux 启用
getenforce

# 使用 selinuxOptions 在 Pod 中指定类型
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: selinux-app
  namespace: production
spec:
  securityContext:
    seLinuxOptions:
      level: "s0:c123,c456"
  containers:
  - name: app
    image: registry.example.com/app:v1.0.0
EOF
```
### 4.4 禁止特权容器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Kyverno 策略强制禁止 privileged
kubectl apply -f - <<EOF
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged
spec:
  validationFailureAction: Enforce
  rules:
  - name: privileged-containers
    match:
      any:
      - resources:
          kinds: [Pod]
    validate:
      message: "生产环境禁止特权容器"
      pattern:
        spec:
          containers:
          - securityContext:
              allowPrivilegeEscalation: "false"
              privileged: "false"
EOF
```
### 4.5 启用 User Namespaces（v1.30+ Beta）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 kubelet 启用 UserNamespacesSupport 特性门
ps aux | grep kubelet | grep feature-gates

# 在 Pod 中启用
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: userns-app
  namespace: production
spec:
  hostUsers: false
  containers:
  - name: app
    image: registry.example.com/app:v1.0.0
EOF
```
### 4.6 部署 Falco 运行时检测

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Helm 部署 Falco
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm upgrade --install falco falcosecurity/falco -n falco --create-namespace \
  --set falcosidekick.enabled=true \
  --set falcosidekick.config.slack.webhookurl="https://hooks.slack.com/..."

# 查看告警
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=100
```
### 4.7 部署 Tetragon eBPF 检测

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Helm 部署 Tetragon
helm repo add cilium https://helm.cilium.io
helm upgrade --install tetragon cilium/tetragon -n kube-system

# 查看事件
kubectl exec -n kube-system ds/tetragon -c tetragon -- tetra getevents
```
### 4.8 镜像签名强制校验

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Kyverno verifyImages 策略强制 Cosign 签名
kubectl apply -f - <<EOF
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  background: false
  rules:
  - name: verify-cosign
    match:
      any:
      - resources:
          kinds: [Pod]
          namespaces: ["production"]
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              ...
              -----END PUBLIC KEY-----
EOF
```
---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| seccomp 配置 | `kubectl get pod <pod> -o jsonpath='{.spec.securityContext.seccompProfile.type}'` | RuntimeDefault 或自定义 Profile |
| AppArmor 加载 | `sudo aa-status` | 自定义 profile 已加载 |
| SELinux 状态 | `kubectl get pod <pod> -o jsonpath='{.spec.securityContext.seLinuxOptions}'` | 已配置 selinuxOptions |
| 特权容器扫描 | `kubectl get pods -A -o json \| jq '.items[].spec.containers[] \| select(.securityContext.privileged==true)'` | 生产命名空间无特权容器 |
| Kyverno 策略 | `kubectl get clusterpolicy` | disallow-privileged / verify-image-signature 存在 |
| Falco 运行 | `kubectl get ds -n falco` | 全节点运行 |
| Tetragon 运行 | `kubectl get ds -n kube-system tetragon` | 全节点运行 |
| 镜像签名验证 | `cosign verify --key cosign.pub registry.example.com/app:v1.0.0` | 验证通过 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 无法创建，事件提示 `exceeded pod-security` | PSA enforce 策略拦截 | `kubectl describe pod` | 调整 Pod securityContext 或提升命名空间 PSA 豁免 |
| seccomp 阻止必要 syscall | RuntimeDefault 过于严格 | `kubectl describe pod` / `dmesg` | 切换为自定义 seccomp profile |
| AppArmor profile 未生效 | 节点未加载 profile | `sudo aa-status` | 重新加载 profile |
| Falco 无告警输出 | 规则未加载或输出通道阻塞 | `kubectl logs -n falco` | 检查 falco.yaml 输出配置 |
| 镜像拉取失败 `signature verification failed` | Kyverno 策略拒绝未签名镜像 | `kubectl get policyreport -A` | 确认镜像已签名，或临时放行白名单 registry |
| User Namespaces 启动失败 | 内核或 kubelet 未启用 | `kubectl describe pod` | 启用 UserNamespacesSupport 特性门 |
| 特权容器被拦截 | Kyverno/PSA 策略生效 | `kubectl get events` | 移除 privileged: true 或申请豁免 |

---

## 7. 风险与注意事项

- **seccomp 误拦截**：从 `Unconfined` 迁移到 `RuntimeDefault` 前，务必在 staging 环境验证应用正常启动。
- **User Namespaces 兼容性**：v1.30 之前为 Alpha，部分存储、网络插件可能不支持，生产环境启用前需充分测试。
- **性能影响**：Falco/Tetragon 会消耗一定 CPU/内存，建议在节点规格选择时预留 5-10% 开销。
- **策略豁免**：对于必须运行特权容器的场景，应通过命名空间标签或豁免规则单独审批，禁止全局关闭策略。
- **镜像签名密钥管理**：Cosign 公钥应通过 GitOps 或 KMS 管理，禁止明文存放在仓库中。

---

## 8. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-13-container-runtime/04-production-readiness-operations-guide|容器运行时生产就绪运维指南]]
- [[02-containerd-production-operations|containerd 生产运维指南]]
- [[05-kata-containers-secure-container|Kata Containers 安全容器]]
- [[06-gvisor-sandbox-runtime|gVisor 沙箱运行时]]
- [[domain-05-security-compliance/README.md|安全合规域]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-05-security-compliance/01-supply-chain/01-supply-chain-security-overview|供应链安全概述]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/03-runtime-security/07-falco-runtime-security-guide|Falco 运行时安全指南]]

---

*本指南聚焦容器运行时安全加固，实际执行前请结合组织安全策略、合规要求与应用兼容性进行裁剪与演练。*


<!-- risk-assessed -->
