---
title: 'Day 16: 安全体系 - Pod 安全 + 密钥管理'
description: 'title: Day 16: 安全体系 - Pod 安全 + 密钥管理'
summary: 'title: Day 16: 安全体系 - Pod 安全 + 密钥管理'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- docker
- opa
- rbac
- operator
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 16: 安全体系 - Pod 安全 + 密钥管理 是什么'
- '如何 Day 16: 安全体系 - Pod 安全 + 密钥管理'
trigger_keywords:
- Day
- '16:'
- 安全体系
- Pod
- 安全
- 密钥管理
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 16: 安全体系 - Pod 安全 + 密钥管理
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] Pod Securityod Security Standards]]
  - K8s Secret 管理最佳实践
  - [[Kyverno|Kyverno]] 策略引擎
  - Pod 安全上下文配置
trigger_keywords:
  - Pod Security
  - PSS
  - Secret
  - SecurityContext
  - Kyverno
  - 密钥管理
  - Pod 安全标准
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
  - security-engineer
estimated_read_time: 240min
related_domains:
  - 安全
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - 故障诊断/topic-fta/04-fta-core-principles
---

# Day 16: 安全体系 - Pod 安全 + 密钥管理

> **学习时间**: 4-5 小时 | **主题**: Pod 安全标准与 Secret 管理

---

## 概述

Pod 安全和密钥管理是 Kubernetes 安全体系的两大核心支柱。Pod 安全标准（Pod Security Standards, PSS）定义了不同安全级别下的 Pod 安全策略，从无限制的 Privileged 到最严格的 Restricted，帮助你在安全性和功能性之间找到合适的平衡点。Secret 管理则涉及敏感信息（如数据库密码、API Key、TLS 证书）的安全存储、分发和使用，是防止数据泄露的关键环节。

本课程将系统性地介绍 Pod Security Standards 的三个安全级别，通过实践掌握 SecurityContext 的配置方法，深入理解 Secret 的创建、使用和安全最佳实践，并了解如何使用 Kyverno 策略引擎实现策略即代码（Policy as Code）。

**学习目标**：
- 理解 Pod Security Standards (Restricted/Baseline/Privileged) 三个级别
- 掌握 Secret 管理最佳实践
- 了解 Kyverno/OPA 策略引擎

**前置条件**：
- 已完成 Day 15 的 RBAC 学习
- 了解 Pod 基本概念和 YAML 配置
- 有 kubectl 操作集群的能力

---

## 核心概念

### Pod Security Standards (PSS)


Pod Security Standards 是 Kubernetes 官方定义的 Pod 安全策略框架，在 K8s 1.25 中成为稳定特性，替代了已废弃的 PodSecurityPolicy（PSP）。PSS 定义了三个安全级别：

#### 三个安全级别详解

| 级别 | 安全限制 | 允许的 capability | host 访问 | 适用场景 |
|------|---------|------------------|----------|---------|
| **Privileged** | 无限制 | 所有 | 允许 hostPID/hostNetwork/hostIPC | 系统组件、CNI 插件 |
| **Baseline** | 基本限制 | NET_RAW | 禁止 hostPID/hostNetwork | 中间件、通用应用 |
| **Restricted** | 严格限制 | 无（必须 drop ALL） | 禁止所有 host 访问 | 业务应用、前端服务 |

#### PSS vs PSP 对比

| 特性 | PSP (已废弃) | PSS (推荐) |
|------|-------------|-----------|
| 实现方式 | Admission Controller | Pod Security Admission |
| 配置方式 | ClusterPolicy 资源 | Namespace 标签 |
| 灵活性 | 高（精细控制） | 中（三个预设级别） |
| 维护方 | Kubernetes 社区（已废弃） | Kubernetes 社区（稳定） |
| 替代方案（需更精细控制） | - | Kyverno / OPA Gatekeeper |

### SecurityContext 配置

SecurityContext 分为 Pod 级别和容器级别，容器级别可以覆盖 Pod 级别的设置。

#### Pod 级别 SecurityContext

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `runAsNonRoot` | 禁止以 root 运行 | true |
| `runAsUser` | 运行用户 UID | 1000 |
| `fsGroup` | 文件系统组 GID | 2000 |
| `seccompProfile` | seccomp 配置 | RuntimeDefault |
| `sysctls` | 内核参数 | 仅允许安全的 sysctls |

#### 容器级别 SecurityContext

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `allowPrivilegeEscalation` | 允许权限提升 | false |
| `readOnlyRootFilesystem` | 只读根文件系统 | true |
| `capabilities.drop` | 删除 Linux capabilities | ALL |
| `capabilities.add` | 添加 Linux capabilities | 仅必要的 |

### Secret 管理

Kubernetes Secret 用于存储敏感数据，但默认情况下 Secret 只做了 Base64 编码，并非真正的加密。

#### Secret 类型

| 类型 | 用途 | 示例 |
|------|------|------|
| `Opaque` | 通用键值对 | 数据库密码、API Key |
| `kubernetes.io/tls` | TLS 证书 | HTTPS 证书 |
| `kubernetes.io/dockerconfigjson` | 镜像拉取凭证 | ACR/ECR 登录凭证 |
| `kubernetes.io/basic-auth` | 基本认证 | 用户名/密码 |
| `kubernetes.io/ssh-auth` | SSH 认证 | SSH 私钥 |
| `kubernetes.io/service-account-token` | SA Token | ServiceAccount 令牌 |

#### Secret 安全最佳实践

1. **启用 etcd 加密**: 配置 EncryptionConfiguration 对 etcd 中的 Secret 加密
2. **最小权限访问**: 使用 RBAC 限制 Secret 的读取权限
3. **使用外部 Secret 管理工具**: 如 External Secrets Operator、Vault
4. **禁用自动挂载 Token**: `automountServiceAccountToken: false`
5. **审计 Secret 访问**: 通过审计日志追踪 Secret 读取操作

---

## 实战演练

### 任务 1: Pod SecurityContext 配置 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 创建安全的 Pod
cat > secure-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
  namespace: default
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 2000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: nginx:alpine
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
      mountPath: /var/cache/nginx
    - name: run
      mountPath: /var/run
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
    livenessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 10
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 5
      periodSeconds: 5
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
  - name: run
    emptyDir: {}
EOF

kubectl apply -f secure-pod.yaml

# 预期输出:
# pod/secure-pod created

# Step 2: 验证安全配置
kubectl exec secure-pod -- id

# 预期输出:
# uid=1000 gid=2000(groups=2000)

# Step 3: 验证只读文件系统
kubectl exec secure-pod -- sh -c 'touch /test 2>&1 || true'

# 预期输出:
# touch: /test: Read-only file system

# Step 4: 验证 capabilities
kubectl exec secure-pod -- cat /proc/1/status | grep Cap

# Step 5: 测试在 PSS restricted 命名空间中创建
kubectl create namespace secure-ns
kubectl label namespace secure-ns pod-security.kubernetes.io/enforce=restricted

kubectl apply -f secure-pod.yaml -n secure-ns

# 预期输出:
# pod/secure-pod created (配置合规，创建成功)

# 测试不合规的 Pod
kubectl run insecure --image=nginx -n secure-ns

# 预期输出:
# Error from server (Forbidden): pods "insecure" is forbidden: violates PodSecurity "restricted:latest": 
#   allowPrivilegeEscalation != false (container "insecure" must set securityContext.allowPrivilegeEscalation=false),
#   unrestricted capabilities (pod and container "insecure" must set securityContext.capabilities.drop=["ALL"]),
#   runAsNonRoot != true (pod or container "insecure" must set securityContext.runAsNonRoot=true),
#   seccompProfile (pod or container "insecure" must set securityContext.seccompProfile.type to "RuntimeDefault" or "Localhost")
```
### 任务 2: Secret 管理 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 创建 Secret
kubectl create secret generic app-secret \
  --from-literal=username=admin \
  --from-literal=password='S3cur3P@ssw0rd!' \
  --from-literal=api-key='ak-1234567890abcdef'

# 预期输出:
# secret/app-secret created

# Step 2: 查看 Secret (Base64 编码)
kubectl get secret app-secret -o yaml

# 预期输出:
# apiVersion: v1
# kind: Secret
# data:
#   api-key: YWstMTIzNDU2Nzg5MGFiY2RlZg==
#   password: UzNjdXIzUEBzc3cwcmQh
#   username: YWRtaW4=
# type: Opaque

# Step 3: 解码 Secret
kubectl get secret app-secret -o jsonpath='{.data.password}' | base64 -d
# 预期输出: S3cur3P@ssw0rd!

# Step 4: 在 Pod 中使用 Secret（环境变量方式）
cat > secret-env-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: secret-env-test
spec:
  automountServiceAccountToken: false
  containers:
  - name: app
    image: nginx:alpine
    env:
    - name: DB_USER
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: username
    - name: DB_PASS
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: password
    - name: API_KEY
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: api-key
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
EOF

kubectl apply -f secret-env-pod.yaml

# Step 5: 验证环境变量
kubectl exec secret-env-test -- env | grep -E 'DB_|API_'

# 预期输出:
# DB_USER=admin
# DB_PASS=S3cur3P@ssw0rd!
# API_KEY=ak-1234567890abcdef

# Step 6: 使用 Secret 卷挂载
cat > secret-volume-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: secret-volume-test
spec:
  automountServiceAccountToken: false
  containers:
  - name: app
    image: nginx:alpine
    volumeMounts:
    - name: secret-volume
      mountPath: /etc/secrets
      readOnly: true
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
  volumes:
  - name: secret-volume
    secret:
      secretName: app-secret
      defaultMode: 0400
EOF

kubectl apply -f secret-volume-pod.yaml

# Step 7: 验证文件挂载
kubectl exec secret-volume-test -- ls -la /etc/secrets/

# 预期输出:
# total 0
# drwxrwxrwt    3 root     root           120 May 18 10:30 .
# drwxr-xr-x    1 root     root            30 May 18 10:30 ..
# drwxr-xr-x    2 root     root            80 May 18 10:30 ..2026_05_18_10_30_00.123456789
# lrwxrwxrwx    1 root     root            31 May 18 10:30 ..data -> ..2026_05_18_10_30_00.123456789
# -r--------    1 root     root            16 May 18 10:30 api-key
# -r--------    1 root     root            16 May 18 10:30 password
# -r--------    1 root     root             5 May 18 10:30 username

kubectl exec secret-volume-test -- cat /etc/secrets/password
# 预期输出: S3cur3P@ssw0rd!
```
### 任务 3: Kyverno 策略实践 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 安装 Kyverno
kubectl create -f https://github.com/kyverno/kyverno/releases/download/v1.11.0/install.yaml

# Step 2: 等待就绪
kubectl wait --namespace kyverno --for=condition=ready pod -l app.kubernetes.io/name=kyverno --timeout=300s

# 预期输出:
# pod/kyverno-admission-controller-xxx condition met
# pod/kyverno-background-controller-xxx condition met
# pod/kyverno-cleanup-controller-xxx condition met
# pod/kyverno-reports-controller-xxx condition met

# Step 3: 创建策略 - 禁止使用 latest 标签
cat > disallow-latest.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
  annotations:
    policies.kyverno.io/title: Disallow Latest Tag
    policies.kyverno.io/category: Best Practices
    policies.kyverno.io/severity: medium
    policies.kyverno.io/description: >-
      The ':latest' tag is mutable and can lead to unexpected errors.
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: validate-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "Using 'latest' tag is not allowed. Use a specific version tag."
      pattern:
        spec:
          containers:
          - image: "!*:latest"
          =(initContainers):
          - image: "!*:latest"
EOF

kubectl apply -f disallow-latest.yaml

# 预期输出:
# clusterpolicy.kyverno.io/disallow-latest-tag created

# Step 4: 测试策略 - 应该被拒绝
kubectl run test-latest --image=nginx:latest

# 预期输出:
# Error from server: admission webhook "validate.kyverno.svc-fail" denied the request:
# resource Pod/default/test-latest was blocked due to the following policies
# disallow-latest-tag:
#   validate-image-tag: 'Using ''latest'' tag is not allowed. Use a specific version tag.'

# Step 5: 测试策略 - 应该成功
kubectl run test-versioned --image=nginx:1.25-alpine

# 预期输出:
# pod/test-versioned created

# Step 6: 创建策略 - 要求资源限制
cat > require-resources.yaml << 'EOF'
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
  annotations:
    policies.kyverno.io/title: Require Resource Limits
    policies.kyverno.io/category: Best Practices
    policies.kyverno.io/severity: medium
spec:
  validationFailureAction: Audit
  background: true
  rules:
  - name: validate-resources
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "CPU and memory resource requests and limits are required."
      pattern:
        spec:
          containers:
          - resources:
              requests:
                cpu: "?*"
                memory: "?*"
              limits:
                cpu: "?*"
                memory: "?*"
EOF

kubectl apply -f require-resources.yaml
```
---

## 配置参考

### Restricted 级别 Pod 完整 YAML 模板

```yaml
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
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        runAsGroup: 2000
        fsGroup: 2000
        seccompProfile:
          type: RuntimeDefault
      automountServiceAccountToken: false
      containers:
      - name: app
        image: my-registry.cn-hangzhou.aliyuncs.com/app:v1.2.3
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop:
            - ALL
        ports:
        - containerPort: 8080
        env:
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: app-secret
              key: password
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 512Mi
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 15
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: cache
          mountPath: /var/cache/app
      volumes:
      - name: tmp
        emptyDir: {}
      - name: cache
        emptyDir:
          sizeLimit: 500Mi
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: secure-app
```

### PSS 级别与安全参数对应表

| 安全参数 | Privileged | Baseline | Restricted |
|----------|-----------|----------|-----------|
| `privileged` | 允许 | 禁止 | 禁止 |
| `hostPID` | 允许 | 禁止 | 禁止 |
| `hostIPC` | 允许 | 禁止 | 禁止 |
| `hostNetwork` | 允许 | 禁止 | 禁止 |
| `hostPorts` | 允许 | 禁止 | 禁止 |
| `capabilities.add` | 允许 | 仅 NET_RAW | 禁止（必须 drop ALL） |
| `runAsNonRoot` | 不要求 | 不要求 | 必须 true |
| `runAsUser` | 不限制 | 不限制 | 必须 > 0 |
| `readOnlyRootFilesystem` | 不要求 | 不要求 | 推荐 true |
| `allowPrivilegeEscalation` | 允许 | 允许 | 禁止 |
| `seccompProfile` | 不要求 | 不要求 | RuntimeDefault 或 Localhost |

---

## 常见问题

### Q1: 为什么容器应该以非 root 用户运行？

**A**: 以 root 运行的容器在发生容器逃逸时，攻击者将获得宿主机的 root 权限。通过设置 `runAsNonRoot: true` 和 `runAsUser: 1000`，即使容器被突破，攻击者只能获得普通用户权限，攻击面大大缩小。生产环境中所有应用容器都应以非 root 运行。

### Q2: Secret 和 ConfigMap 的区别是什么？

**A**: 
- **ConfigMap**: 存储非敏感配置数据，如配置文件、命令行参数，无需加密
- **Secret**: 存储敏感数据，支持 Base64 编码，可以启用 etcd 加密，支持更严格的 RBAC 控制
- **核心区别**: Secret 支持 etcd 加密存储、独立的 RBAC 权限控制、自动轮换机制

### Q3: readOnlyRootFilesystem 设为 true 后应用无法写入怎么办？

**A**: 使用 emptyDir 卷挂载需要写入的目录：
```yaml
volumeMounts:
- name: tmp
  mountPath: /tmp
- name: logs
  mountPath: /var/log/app
- name: cache
  mountPath: /var/cache/app
volumes:
- name: tmp
  emptyDir: {}
- name: logs
  emptyDir: {}
- name: cache
  emptyDir:
    sizeLimit: 500Mi
```

### Q4: Kyverno 和 OPA Gatekeeper 怎么选？

**A**:
- **Kyverno**: Kubernetes 原生设计，使用 YAML 策略（无需学习新语言），上手简单，推荐大多数场景
- **OPA Gatekeeper**: 使用 Rego 语言编写策略，更灵活但学习曲线陡峭，适合需要复杂策略逻辑的场景
- **建议**: 如果策略需求是常见的安全/最佳实践检查，选 Kyverno；如果需要非常复杂的条件判断，选 OPA

### Q5: 如何在生产环境启用 etcd Secret 加密？

**A**: 配置 EncryptionConfiguration：
```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  providers:
  - aescbc:
      keys:
      - name: key1
        secret: <base64-encoded-32-byte-key>
  - identity: {}
```
然后配置 API Server 使用此配置文件。ACK 托管版集群可以在控制台一键启用 Secret 加密。

---

## 要点总结

- **PSS 三个级别**: Privileged（无限制）→ Baseline（基本限制）→ Restricted（最严格）
- **SecurityContext** 分 Pod 级别和容器级别，容器级别可覆盖 Pod 级别
- **Restricted 模式** 要求：非 root、drop ALL capabilities、readOnlyRootFilesystem、seccompProfile
- **Secret** 默认仅 Base64 编码，生产环境必须启用 etcd 加密或使用外部 Secret 管理
- **Kyverno** 用 YAML 编写策略，比 OPA Gatekeeper 更易上手
- **策略即代码**: 将安全策略定义为代码，纳入 GitOps 流程管理

---

## 延伸阅读

- [Pod Security Standards 官方文档](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [Kyverno 官方文档](https://kyverno.io/docs/)
- [Secret 最佳实践](https://kubernetes.io/docs/concepts/configuration/secret/#good-practices)
- [文件: `../../安全/06-pod-security-standards.md`](../../安全/06-pod-security-standards.md)
- [文件: `../../安全/11-secret-management-tools.md`](../../安全/11-secret-management-tools.md)
- [文件: `../../安全/14-policy-engines-opa-kyverno.md`](../../安全/14-policy-engines-opa-kyverno.md)

```

<!-- risk-assessed -->
