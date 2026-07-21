---
title: 配置管理知识词典
description: 涵盖 Kubernetes 配置管理全领域的完整术语体系，包括 ConfigMap、Secret、探针、资源管理、准入控制、补丁策略等
summary: 配置管理领域词典，覆盖 ConfigMap、Secret、Probe、Resource Quota、Server-Side Apply、Webhook 等核心概念
category: dictionary
tags:
- dictionary
- configuration
- configmap
- secret
- probe
- resource-management
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- 开发工程师
- 平台工程师
- SRE
---

# 配置管理知识词典（Configuration）

> 本词典覆盖 Kubernetes 配置管理领域的核心术语、技术组件及工程实践，是开发工程师和平台工程师管理应用配置、资源限制、健康检查的权威参考。

## 领域概述

配置管理是 Kubernetes 工作负载运行的基础，解决的核心问题包括：

- **配置与镜像解耦**：同一镜像在不同环境使用不同配置
- **敏感信息管理**：密码、证书、Token 的安全存储与分发
- **健康检查**：自动检测并恢复异常容器
- **资源治理**：防止资源滥用，保障集群稳定性
- **变更管理**：声明式配置更新、服务端应用、冲突解决

## 核心术语定义

### 配置数据管理

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| ConfigMap | 存储非敏感配置数据的 API 对象 | 最大 1MiB，支持环境变量/卷挂载 |
| Secret | 存储敏感数据的 API 对象 | Base64 编码（非加密），可集成 KMS |
| Downward API | 将 Pod 元数据暴露给容器 | 支持 labels/annotations/resources |
| Env | 容器环境变量配置 | 支持 ConfigMap/Secret 引用 |
| Helm Values | Helm Chart 的参数化配置 | values.yaml + --set 覆盖 |
| Kustomization | Kustomize 的声明式配置定制 | 无模板、层叠覆盖 |
| KCL | KusionStack 配置语言，强类型配置 | 静态检查、继承、合并 |

### 健康检查与探针

| 术语 | 定义 | 失败后果 |
|------|------|----------|
| Liveness Probe | 检测容器是否存活 | 失败 → 重启容器 |
| Readiness Probe | 检测容器是否就绪接收流量 | 失败 → 从 Service 摘除 |
| Startup Probe | 检测应用是否启动完成 | 失败 → 重启（保护慢启动应用） |
| Probe Handler | 探针检测方式 | exec/httpGet/tcpSocket/grpc |
| Graceful Shutdown | 优雅停机，处理完存量请求再退出 | preStop + SIGTERM + terminationGracePeriod |

### 资源管理

| 术语 | 定义 | 作用 |
|------|------|------|
| Resource Requests | 容器最小资源保证 | 调度依据，QoS 分类 |
| Resource Limits | 容器资源使用上限 | 超过 CPU 限流，超过 Memory OOMKill |
| LimitRange | 命名空间级默认资源限制 | 自动注入 requests/limits |
| ResourceQuota | 命名空间级资源配额 | 限制总资源/Pod 数量 |
| PriorityClass | Pod 优先级定义 | 调度抢占、驱逐顺序 |
| Taint/Toleration | 节点污点与 Pod 容忍 | 控制 Pod 调度到特定节点 |

### 变更管理与准入控制

| 术语 | 定义 | 典型场景 |
|------|------|----------|
| Server-Side Apply (SSA) | 服务端声明式资源管理 | 多控制器协作、冲突检测 |
| Strategic Merge Patch | K8s 原生智能合并补丁 | 按 key 合并而非替换 |
| Validating Webhook | 准入控制：验证资源合法性 | 策略检查、规范强制 |
| Mutating Webhook | 准入控制：修改资源内容 | 自动注入 sidecar、默认值 |

## 技术组件索引

### 配置数据类

- [[系统基础/知识字典/configuration/configmap.md|ConfigMap（配置数据）]]
- [[系统基础/知识字典/configuration/configmaps.md|ConfigMaps（配置管理实践）]]
- [[系统基础/知识字典/configuration/secrets.md|Secrets（敏感信息）]]
- [[系统基础/知识字典/configuration/downward-api.md|Downward API（元数据暴露）]]
- [[系统基础/知识字典/configuration/env.md|Env（环境变量）]]
- [[系统基础/知识字典/configuration/helm-values.md|Helm Values（参数化配置）]]
- [[系统基础/知识字典/configuration/kustomization.md|Kustomization（声明式定制）]]
- [[系统基础/知识字典/configuration/kcl.md|KCL（配置语言）]]

### 健康检查类

- [[系统基础/知识字典/configuration/probe.md|Probe（探针总论）]]
- [[系统基础/知识字典/configuration/liveness-probe.md|Liveness Probe（存活探针）]]
- [[系统基础/知识字典/configuration/readiness-probe.md|Readiness Probe（就绪探针）]]
- [[系统基础/知识字典/configuration/startup-probe.md|Startup Probe（启动探针）]]
- [[系统基础/知识字典/configuration/liveness-readiness-and-startup-probes.md|三种探针综合实践]]
- [[系统基础/知识字典/configuration/graceful-shutdown.md|Graceful Shutdown（优雅停机）]]

### 资源管理类

- [[系统基础/知识字典/configuration/resource-management-for-pods-and-containers.md|Pod/Container 资源管理]]
- [[系统基础/知识字典/configuration/resource-management-for-windows-nodes.md|Windows 节点资源管理]]
- [[系统基础/知识字典/configuration/limit-range.md|LimitRange（默认限制）]]
- [[系统基础/知识字典/configuration/resource-quota.md|ResourceQuota（资源配额）]]
- [[系统基础/知识字典/configuration/priority-class.md|PriorityClass（优先级）]]
- [[系统基础/知识字典/configuration/taint-toleration.md|Taint/Toleration（污点容忍）]]

### 变更管理类

- [[系统基础/知识字典/configuration/server-side-apply.md|Server-Side Apply]]
- [[系统基础/知识字典/configuration/strategic-merge-patch.md|Strategic Merge Patch]]
- [[系统基础/知识字典/configuration/validating-webhook.md|Validating Webhook]]
- [[系统基础/知识字典/configuration/organizing-cluster-access-using-kubeconfig-files.md|Kubeconfig 管理]]
- [[系统基础/知识字典/configuration/schemahero.md|SchemaHero（数据库 Schema 管理）]]

## 配置管理架构模式

### 配置分层策略

```
配置分层模型:

┌─────────────────────────────────────┐
│  L4: 环境特定覆盖 (Helm values-prod.yaml)  │
├─────────────────────────────────────┤
│  L3: 集群特定配置 (Kustomize overlay)     │
├─────────────────────────────────────┤
│  L2: 应用默认配置 (Helm values.yaml)       │
├─────────────────────────────────────┤
│  L1: 基础配置模板 (Helm Chart/Kustomize base) │
└─────────────────────────────────────┘

优先级: L4 > L3 > L2 > L1
```

### 探针配置最佳实践

```yaml
# 完整的探针配置示例
apiVersion: v1
kind: Pod
spec:
  terminationGracePeriodSeconds: 60  # 优雅停机窗口
  containers:
  - name: app
    # 启动探针：保护慢启动应用
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      failureThreshold: 30      # 最多等待 30*10=300s
      periodSeconds: 10
    # 存活探针：检测死锁/无响应
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 0    # startupProbe 完成后才开始
      periodSeconds: 15
      timeoutSeconds: 5
      failureThreshold: 3
    # 就绪探针：控制流量接入
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 0
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 3
      successThreshold: 1
    # 优雅停机
    lifecycle:
      preStop:
        exec:
          command: ["sh", "-c", "sleep 10"]  # 等待 Service 摘除
```

## 生产最佳实践

### ConfigMap/Secret 管理

1. **不可变配置**：生产环境使用 `immutable: true`，避免意外修改
2. **版本化配置**：配置变更创建新 ConfigMap（名称加 hash），而非就地修改
3. **Secret 加密**：启用 etcd 静态加密 + RBAC 限制 Secret 访问
4. **外部密钥管理**：生产环境使用 External Secrets Operator + Vault/KMS

### 资源管理

1. **必须设置 requests**：所有容器必须声明 CPU/Memory requests
2. **Limits 策略**：CPU 可不设 limits（避免限流），Memory 必须设 limits
3. **QoS 等级**：关键服务使用 Guaranteed（requests = limits）
4. **配额保护**：每个 Namespace 设置 ResourceQuota 防止资源耗尽

### 准入控制

1. **策略即代码**：使用 OPA/Gatekeeper 或 Kyverno 实现策略强制
2. **Webhook 高可用**：准入 Webhook 必须多副本 + 超时配置
3. **失败策略**：非关键 Webhook 使用 `failurePolicy: Ignore`
4. **命名空间排除**：排除 kube-system 避免影响核心组件

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Pod CrashLoopBackOff | Liveness 探针失败/应用崩溃 | `kubectl logs --previous`、检查探针配置 |
| Pod Pending | 资源不足/配额超限 | `kubectl describe pod`、检查 ResourceQuota |
| ConfigMap 更新未生效 | 卷挂载有延迟/环境变量不更新 | 重启 Pod 或使用 reloader |
| Secret 解码失败 | Base64 编码错误/字符集问题 | `echo $SECRET | base64 -d` 验证 |
| Webhook 拒绝请求 | 策略检查不通过 | 检查 Webhook 日志、调整策略规则 |
| OOMKilled | Memory limit 过低/内存泄漏 | 检查 `kubectl top pod`、调整 limits |

## 学习路径

```
基础: ConfigMap/Secret → 环境变量/卷挂载
进阶: 探针配置 → 资源管理 → 优雅停机
高级: Server-Side Apply → 准入控制 → 策略引擎
专家: 配置即代码 (KCL/CUE) → GitOps 配置管理
```

## 参考链接

- https://kubernetes.io/docs/concepts/configuration/
- https://kubernetes.io/docs/tasks/configure-pod-container/
- https://helm.sh/
- https://kubectl.docs.kubernetes.io/guides/config_management/
- https://kcl-lang.io/

## Related

- [[系统基础/知识字典/workloads/deployment.md|Deployment 工作负载]]
- [[系统基础/知识字典/scheduling/affinity.md|调度亲和性]]
- [[系统基础/知识字典/security/rbac.md|RBAC 权限控制]]
- [[系统基础/知识字典/operations/helm.md|Helm 包管理]]

## 深度技术解析

### Server-Side Apply (SSA) 工作原理

SSA 是 K8s 1.22+ 的声明式资源管理新范式：

```
SSA vs 传统 Client-Side Apply:

Client-Side Apply (kubectl apply):
  1. 客户端获取当前对象
  2. 客户端计算 diff
  3. 客户端发送 PATCH
  问题: 多控制器冲突、last-applied-configuration annotation 膨胀

Server-Side Apply (kubectl apply --server-side):
  1. 客户端发送完整期望状态
  2. 服务端记录每个字段的管理者 (fieldManager)
  3. 服务端检测冲突 (同一字段被多个管理者修改)
  4. 冲突时返回 409，客户端决定 force 或放弃
  优势: 精确字段级所有权、无 annotation 膨胀
```

```yaml
# SSA 示例：多控制器协作
# Controller A 管理 replicas
kubectl apply --server-side --field-manager=controller-a -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
EOF

# Controller B 管理 image
kubectl apply --server-side --field-manager=controller-b -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  template:
    spec:
      containers:
      - name: app
        image: my-app:v2
EOF
# 两个控制器互不干扰，各管各的字段
```

### OPA/Gatekeeper 策略引擎

```yaml
# 策略示例：强制所有 Pod 设置资源限制
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredResources
metadata:
  name: must-have-limits
spec:
  match:
    kinds:
    - apiGroups: [""]
      kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
  parameters:
    limits: ["cpu", "memory"]
---
# 对应的 Rego 策略
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredresources
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredResources
      validation:
        openAPIV3Schema:
          type: object
          properties:
            limits:
              type: array
              items:
                type: string
  targets:
  - target: admission.k8s.gatekeeper.sh
    rego: |
      package k8srequiredresources
      violation[{"msg": msg}] {
        container := input.review.object.spec.containers[_]
        not container.resources.limits
        msg := sprintf("Container %v must have resource limits", [container.name])
      }
```

### 优雅停机完整流程

```
Pod 终止流程 (terminationGracePeriodSeconds=60):

1. kubectl delete pod / 滚动更新触发
   │
2. Pod 状态设为 Terminating
   │
3. 并行执行:
   ├── a) 从 Service Endpoints 摘除 (Readiness 失败)
   ├── b) 执行 preStop Hook (sleep 10 等待摘除生效)
   │
4. preStop 完成后，发送 SIGTERM
   │
5. 应用处理完存量请求，清理资源
   │
6. 应用退出 (或等待 terminationGracePeriodSeconds)
   │
7. 超时未退出 → 发送 SIGKILL 强制终止
   │
8. Pod 从 API Server 删除

关键时间点:
- preStop: 10s (等待 Service 摘除传播)
- SIGTERM → 退出: 应用自定义 (建议 < 45s)
- 总窗口: 60s (terminationGracePeriodSeconds)
```

## 生产案例研究

### 案例：配置管理混乱导致的故障

**背景：** 某公司生产环境 ConfigMap 被误修改，导致 200+ Pod 重启。

**根因：**
- ConfigMap 未设置 `immutable: true`
- 开发人员直接 `kubectl edit configmap` 修改生产配置
- 卷挂载的 ConfigMap 自动更新触发应用重载

**修复与预防：**
1. 所有生产 ConfigMap 设置 `immutable: true`
2. 配置变更走 GitOps 流程（ArgoCD 同步）
3. RBAC 限制生产 Namespace 的 ConfigMap 写权限
4. 使用 OPA 策略强制 immutable 字段

## 常用运维命令速查

```bash
# === ConfigMap/Secret ===
# 创建 ConfigMap
kubectl create configmap app-config --from-file=config.yaml
# 查看 Secret 解码
kubectl get secret my-secret -o jsonpath='{.data.password}' | base64 -d
# 检查 ConfigMap 是否 immutable
kubectl get configmap -o jsonpath='{.items[*].immutable}'

# === 资源管理 ===
# 查看 Namespace 配额使用
kubectl describe resourcequota -n my-namespace
# 查看 LimitRange
kubectl get limitrange -n my-namespace -o yaml
# 查看 Pod QoS 等级
kubectl get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.qosClass}{"\n"}{end}'

# === 探针调试 ===
# 查看探针配置
kubectl get pod my-pod -o jsonpath='{.spec.containers[0].livenessProbe}'
# 查看探针事件
kubectl describe pod my-pod | grep -A5 "Liveness\|Readiness"
# 手动测试探针端点
kubectl exec my-pod -- curl -s http://localhost:8080/healthz

# === Server-Side Apply ===
# 查看字段管理者
kubectl get deployment my-app -o jsonpath='{.metadata.managedFields}'
# 强制应用 (解决冲突)
kubectl apply --server-side --force-conflicts -f deployment.yaml

# === 准入控制 ===
# 查看 Webhook 配置
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations
# 查看 Gatekeeper 约束
kubectl get constraints
```

## 常见问题 FAQ

**Q1: ConfigMap 更新后 Pod 会自动重载吗？**

A: 取决于挂载方式：
- 环境变量引用：**不会**自动更新，必须重启 Pod
- 卷挂载：**会**自动更新（有 1-2min 延迟），但应用是否重载取决于应用自身
- subPath 挂载：**不会**自动更新
建议使用 reloader 或版本化 ConfigMap 实现可控更新。

**Q2: requests 和 limits 怎么设置？**

A: 经验法则：
- CPU requests: 设置为 P50 使用量，limits 可不设（避免 CPU throttling）
- Memory requests: 设置为 P95 使用量，limits 设置为 requests 的 1.5-2x
- 关键服务：requests = limits (Guaranteed QoS)
- 必须先压测获取真实数据，不要拍脑袋

**Q3: Liveness 探针失败就重启，会不会太激进？**

A: 是的，Liveness 探针要谨慎设计：
- 只检测“不可恢复”的故障（死锁、无响应）
- 不要检测依赖服务（DB 不可用不应重启应用）
- failureThreshold 设大一点（3-5）
- 考虑用 Readiness 替代（摘流量而非重启）

**Q4: Secret 真的安全吗？**

A: 默认不安全。Secret 只是 Base64 编码，不是加密。安全措施：
1. 启用 etcd 静态加密 (EncryptionConfiguration)
2. RBAC 严格限制 Secret 访问权限
3. 生产环境用 External Secrets + Vault/KMS
4. 避免 Secret 进入 Git（使用 Sealed Secrets 或 SOPS）

**Q5: Strategic Merge Patch 和 JSON Merge Patch 有什么区别？**

A: 
- JSON Merge Patch: 数组整体替换
- Strategic Merge Patch: K8s 原生，数组按 key 合并（如 containers 按 name 合并）
- JSON Patch: 精确的 add/remove/replace 操作
SSA 是最新推荐方式，替代所有 Patch 策略。

## 配置反模式警示

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| 环境变量引用 ConfigMap | 更新不生效，需重启 | 卷挂载 + reloader |
| Secret 明文存 Git | 泄露风险 | Sealed Secrets / SOPS / External Secrets |
| Liveness 检测依赖服务 | 依赖故障导致级联重启 | 只检测自身健康 |
| 不设置 requests | 调度不可预测，QoS BestEffort | 必须设置 requests |
| CPU limits 过低 | CPU throttling，延迟飙升 | 可不设 CPU limits |
| 直接 kubectl edit 生产 | 无审计、无回滚 | GitOps 流程变更 |
| 单个巨大 ConfigMap | 超过 1MiB 限制、变更影响大 | 拆分多个小 ConfigMap |
| preStop 缺失 | 流量丢失（Service 未摘除就终止） | preStop sleep + 优雅停机 |

## 配置管理工具对比

| 工具 | 定位 | 优势 | 劣势 |
|------|------|------|------|
| Helm | 包管理 + 模板 | 生态成熟、Chart 仓库 | 模板复杂、调试困难 |
| Kustomize | 声明式覆盖 | 无模板、K8s 原生 | 复杂场景能力有限 |
| KCL | 配置语言 | 强类型、继承、验证 | 学习曲线、生态较新 |
| CUE | 配置语言 | 类型安全、可组合 | 学习曲线陡峭 |
| Jsonnet | 配置语言 | 函数式、灵活 | 语法复杂 |
| Crossplane | IaC 配置 | 云资源抽象 | 重量级、学习成本高 |

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| SSA | Server-Side Apply | 服务端声明式应用 |
| SMP | Strategic Merge Patch | 战略合并补丁 |
| QoS | Quality of Service | 服务质量等级 |
| OPA | Open Policy Agent | 开放策略引擎 |
| KMS | Key Management Service | 密钥管理服务 |
| RBAC | Role-Based Access Control | 基于角色的访问控制 |
| CRD | Custom Resource Definition | 自定义资源定义 |
| HPA | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩 |
| VPA | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩 |
