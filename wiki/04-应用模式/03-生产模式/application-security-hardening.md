---
title: 应用安全加固生产模式
description: 生产级应用安全：PSS/PSA、SecurityContext、NetworkPolicy、mTLS 与镜像签名准入实践
summary: 生产级应用安全：PSS/PSA、SecurityContext、NetworkPolicy、mTLS 与镜像签名准入实践，含安全基线清单与合规检查。
category: application-patterns
tags:
- security
- pss
- networkpolicy
- mtls
- image-signing
- hardening
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 18min
intent_queries:
- K8s 应用安全加固怎么做
- PSS PSA 如何配置
trigger_keywords:
- PSS
- PSA
- SecurityContext
- NetworkPolicy
- mTLS
- 镜像签名
- 安全加固
prerequisites:
- kubectl-basics
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
> 安全策略变更可能导致 Pod 无法调度或启动。变更前在非生产验证，使用 audit/warn 模式渐进推进。

# 应用安全加固生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考`

容器逃逸和供应链攻击是云原生环境的主要安全威胁。本文涵盖五大应用安全加固维度：Pod Security Standards、SecurityContext、NetworkPolicy、mTLS 和镜像签名准入，提供从基线到强化的渐进式落地路径。

---

## 1. Pod Security Standards (PSS/PSA)

### 1.1 三级安全标准

| 级别 | 限制强度 | 典型场景 | 允许特权 |
|---|---|---|---|
| **privileged** | 无限制 | 系统组件（CSI/CNI） | 全部（root、特权模式、hostPath） |
| **baseline** | 防最明显提权 | 一般工作负载 | 禁 hostPID/hostIPC、禁 privileged |
| **restricted** | 严格遵循 hardening | 生产应用 | 禁 root、强制 seccomp、弃全部Capabilities |

### 1.2 Namespace 级别标签

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    # enforce: 违反则拒绝创建（硬性门控）
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    # audit: 违反则记录审计日志（观察用）
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
    # warn: 违反则向 kubectl 输出警告
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: latest
```

### 1.3 渐进式推进策略

> ⚠️ **不要直接 enforce restricted**——已有工作负载可能不兼容，导致 Pod 创建失败。推荐三阶段推进：

```
阶段 1 (观察): audit + warn = restricted, enforce = privileged
  → 收集告警和审计日志，识别不兼容的工作负载
  → 持续 2-4 周

阶段 2 (修复): 逐个修复告警的工作负载（改 SecurityContext）
  → 非 root 运行、加 seccompProfile、弃 capabilities
  → 直到 audit 日志清零

阶段 3 (强制): enforce = baseline → 逐步 enforce = restricted
  → 新 namespace 直接 restricted
  → 老 namespace 先 baseline 再 restricted
```

---

## 2. SecurityContext 生产基线

### 2.1 Restricted 级别安全上下文模板

```yaml
spec:
  securityContext:               # Pod 级
    runAsNonRoot: true           # 禁止 root 用户
    runAsUser: 1000              # 非 0 UID
    runAsGroup: 3000             # 非 0 GID
    fsGroup: 2000                # 挂载卷的 GID
    seccompProfile:              # 系统调用过滤
      type: RuntimeDefault
  containers:
    - name: app
      securityContext:           # 容器级
        allowPrivilegeEscalation: false   # 禁 sudo/setuid 提权
        readOnlyRootFilesystem: true      # 根文件系统只读
        runAsNonRoot: true
        capabilities:
          drop:
            - ALL                # 弃所有 Linux capabilities
          # 仅按需 add，如: add: ["NET_BIND_SERVICE"]
```

### 2.2 readOnlyRootFilesystem 适配

只读根文件系统是 restricted 的要求之一，但部分应用需要写临时文件：

```yaml
containers:
  - name: app
    securityContext:
      readOnlyRootFilesystem: true
    volumeMounts:
      - name: tmp                # 挂载 emptyDir 供临时写入
        mountPath: /tmp
      - name: cache
        mountPath: /app/cache
volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: { sizeLimit: 500Mi }
```

---

## 3. NetworkPolicy 零信任

### 3.1 默认拒绝 + 按需放行

```yaml
# Step 1: 默认拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}                # 匹配 namespace 内所有 Pod
  policyTypes:
    - Ingress
---
# Step 2: 仅允许特定来源访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-allow-frontend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend      # 仅 frontend 可访问 api
      ports:
        - protocol: TCP
          port: 8080
```

### 3.2 出站流量管控

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-egress-controlled
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
    - Egress
  egress:
    # 允许访问 DNS
    - to:
        - namespaceSelector: {}
      ports:
        - protocol: UDP
          port: 53
    # 允许访问数据库
    - to:
        - podSelector:
            matchLabels:
              app: database
      ports:
        - protocol: TCP
          port: 5432
    # 允许访问外部 API（指定 IP）
    - to:
        - ipBlock:
            cidr: 203.0.113.0/24
      ports:
        - protocol: TCP
          port: 443
```

> ⚠️ **NetworkPolicy 前提**: 需 CNI 支持（Calico/Cilium 支持完整；Flannel 默认不支持）。部署前确认 CNI 的 NetworkPolicy 支持。Cilium 还支持 L7 策略（按 HTTP path/method 限制）。

---

## 4. mTLS 服务间加密

### 4.1 方案对比

| 方案 | 加密粒度 | 性能开销 | 运维复杂度 | 适用 |
|---|---|---|---|---|
| **Service Mesh (Istio/Linkerd)** | 自动 sidecar 注入 | 中 | 高 | 大规模微服务 |
| **应用级 mTLS** | 应用代码实现 | 低 | 中 | 小规模、性能敏感 |
| **Cilium mTLS** | eBPF 层（无 sidecar） | 低 | 中 | Cilium 环境 |

### 4.2 Istio PeerAuthentication（网格内 mTLS）

```yaml
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata:
  name: default
  namespace: production
spec:
  mtls:
    mode: STRICT            # 强制 mTLS，拒绝明文
```

> 💡 `PERMISSIVE` 模式允许明文和 mTLS 并存（迁移期用），`STRICT` 模式强制全部加密（生产目标）。

---

## 5. 镜像签名与准入控制

### 5.1 镜像签名流程

```
CI/CD 流水线:
  构建 → 签名 (cosign) → 推送签名到仓库 → 部署时准入校验

  cosign sign --key cosign.key registry.example.com/app:v1.2.3
```

### 5.2 Kyverno 准入策略（仅允许已签名镜像）

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce    # 违反则拒绝部署
  rules:
    - name: check-signature
      match:
        resources:
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
                      <cosign-public-key>
                      -----END PUBLIC KEY-----
```

### 5.3 镜像来源限制

```yaml
# Kyverno: 仅允许受信任仓库的镜像
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: allowed-registries
spec:
  validationFailureAction: Enforce
  rules:
    - name: validate-registry
      match:
        resources:
          kinds: [Pod]
      validate:
        message: "镜像必须来自受信任仓库"
        pattern:
          spec:
            containers:
              - image: "registry.example.com/* | docker.io/library/*"
```

---

## 6. 生产安全检查清单

| # | 检查项 | 验证命令 | 合格标准 |
|---|---|---|---|
| 1 | PSS enforce 已配置 | `kubectl get ns --show-labels \| grep enforce` | 核心 namespace ≥ baseline |
| 2 | Pod 非 root 运行 | `kubectl get pods -o json \| jq '.items[].spec.securityContext.runAsNonRoot'` | true |
| 3 | readOnlyRootFilesystem | 检查 SecurityContext | 核心 Pod 为 true |
| 4 | capabilities 已 drop ALL | 检查 SecurityContext | 核心 Pod 命中 |
| 5 | seccompProfile 已设 | 检查 SecurityContext | RuntimeDefault |
| 6 | NetworkPolicy 默认拒绝 | `kubectl get networkpolicy` | 核心 namespace 有 deny-all |
| 7 | mTLS 已启用 | Istio PeerAuthentication | STRICT 模式 |
| 8 | 镜像签名准入已启用 | Kyverno policy 状态 | Enforce 模式 |
| 9 | 无 latest tag 镜像 | `kubectl get pods -o json \| jq` 检查 image | 全部使用具体 tag/digest |

---

## 7. 排障速查

| 症状 | 可能根因 | 诊断 | 修复 |
|---|---|---|---|
| Pod 创建被拒绝 | PSS enforce 阻止（如 root 运行） | `kubectl describe pod` 看 events | 修改 SecurityContext 符合 PSS 级别 |
| Pod 无法访问其他服务 | NetworkPolicy 拒绝 | 检查 ingress/egress 规则 | 添加放行规则 |
| mTLS 切 STRICT 后服务不通 | 有 Pod 未注入 sidecar / 不支持 mTLS | Istio 检查 PERMISSIVE 日志 | 回退 PERMISSIVE + 修复注入 |
| 镜像部署被 Kyverno 拒绝 | 未签名 / 仓库不在白名单 | Kyverno 策略报告 | 签名镜像或更新白名单 |
| 应用因 readOnlyRootFS 崩溃 | 需写文件但无 emptyDir | 检查 volumeMounts | 加 emptyDir 挂载到写入路径 |

---

## 8. 跨域协作

- **Pod 可用性（探针与 PDB）**: 见 [[pod-availability-lifecycle|Pod 可用性生产模式]]
- **安全合规深入**: 见 `安全/99-production-readiness-operations-guide.md`
- **供应链安全 Runbook**: 见 `安全/05-supply-chain/14-supply-chain-security-runbook.md`
- **运行时安全加固**: 见 `容器运行时/03-containerd-cri-o/06-runtime-security-hardening.md`
- **安全运营 Runbook**: 见 `生产运维/08-security-operations-runbook.md`


<!-- risk-assessed -->
