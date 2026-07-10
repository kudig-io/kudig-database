---
title: 节点安全 — Node Authorization / NodeRestriction / Pod Security
description: 'title: 节点安全机制'
summary: 'title: 节点安全机制'
category: general
tags:
- reference
- security
- etcd
- apiserver
- kubelet
- rbac
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点安全 — Node Authorization / NodeRestriction / Pod Security 是什么
- 如何 节点安全 — Node Authorization / NodeRestriction / Pod Security
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点安全
- Node
- Authorization
- NodeRestriction
- Pod
- Security
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点安全机制
description: '# 节点安全 — Node Authorization / NodeRestriction / Pod Security'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- rbac
- webhook
last_updated: 2026-05-18
difficulty: expert
reading_level: expert
audience:
- Kubernetes 安全工程师
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Node Authorization node restriction
- kubelet security configuration best practices
- Pod Security Standards PSS baseline privileged
- Node Authorizer RBAC node isolation
- kubelet authentication webhook authorization
trigger_keywords:
- Node Authorization
- NodeRestriction
- Pod Security
- PSS
- PSP
- security
- node isolation
- system:nodes
- RBAC
- webhook authentication
- privileged
- baseline
- restricted
- seccomp
- capabilities
related_domains:
- 安全
- 集群基础
related_topics:
- 16-security
- cluster-create/03-certs
- node-create/06-certificate
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 节点安全 — Node Authorization / NodeRestriction / Pod Security

## 概述

节点安全是 Kubernetes 集群安全体系中至关重要的一环。每个节点上运行着 kubelet，它拥有创建/删除 Pod、读写 Secret 和 ConfigMap、上报节点状态等敏感权限。如果 kubelet 的权限没有被正确限制，攻击者一旦控制了某个节点，就可以利用 kubelet 的权限在整个集群中横向移动，造成灾难性的安全后果。

Kubernetes 从 v1.7 开始引入了 Node Authorizer 和 NodeRestriction Admission Plugin，专门用于限制 kubelet 的权限范围，确保每个 kubelet 只能操作与自身节点相关的资源。这一机制被称为**节点隔离（Node Isolation）**，是 Kubernetes 安全模型的重要基石。

本文档从源码层面深入分析 Node Authorizer、NodeRestriction、Pod Security 三大安全机制的工作原理，并提供生产环境的安全加固建议。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Node Authorizer | `pkg/kube-apiserver/authorizer/node/` | 节点授权器 |
| NodeRestriction | `plugin/pkg/admission/noderestriction/` | 节点准入插件 |
| PodSecurity | `pkg/apis/policy/types.go` | Pod 安全标准 |
| kubelet 认证 | `pkg/kubelet/server/auth.go` | kubelet API 认证 |
| RBAC 引擎 | `pkg/kube-apiserver/authorizer/rbac/` | RBAC 授权 |

---

## 一、Node Authorization

### 1.1 设计背景

在 Node Authorizer 出现之前，kubelet 使用 RBAC 获取权限。这意味着 kubelet 拥有对所有 Node、Pod、Secret 等资源的全局访问权限，而不是仅限于自身节点上的资源。这种设计存在严重的安全隐患：

- 被入侵的 kubelet 可以读取其他节点上的 Secret
- 被入侵的 kubelet 可以修改其他节点的状态
- 被入侵的 kubelet 可以删除其他节点上的 Pod

Node Authorizer 的出现彻底解决了这个问题。它基于证书中的 `system:nodes` 组和 CN（Common Name）中的节点名称，精确限制每个 kubelet 只能访问与自身节点相关的资源。

### 1.2 启用 Node Authorizer

```bash
# API Server 启动参数
--authorization-mode=Node,RBAC

# 注意顺序: Node 授权器在 RBAC 之前检查
# 如果 Node 授权器拒绝了请求，不会继续检查 RBAC
```

### 1.3 Node Authorizer 授权规则

Node Authorizer 允许 kubelet 执行以下操作：

| 资源 | 允许的操作 | 限制条件 |
|------|-----------|---------|
| Node（自身） | 读取、更新状态 | 只能操作 CN 中指定的节点名 |
| Pod（自身节点上） | 读取、创建、删除、更新状态 | 只能操作绑定到自身节点的 Pod |
| Secret | 读取 | 只能读取自身节点上 Pod 使用的 Secret |
| ConfigMap | 读取 | 只能读取自身节点上 Pod 使用的 ConfigMap |
| PV/PVC | 读取 | 只能读取自身节点上 Pod 使用的卷 |
| ServiceAccount Token | 读取 | 只能读取自身节点上 Pod 关联的 Token |

```go
// pkg/kube-apiserver/authorizer/node/node_authorizer.go
type NodeAuthorizer struct {
    // nodeIndex 建立了节点到 Pod 到 Secret/ConfigMap 的索引
    // 用于快速判断 kubelet 请求的资源是否属于自身节点
}

func (r *NodeAuthorizer) Authorize(ctx context.Context, attrs authorizer.Attributes) (authorized authorizer.Decision, reason string, err error) {
    // 1. 检查请求者是否属于 system:nodes 组
    // 2. 从证书 CN 中提取节点名称
    // 3. 检查请求的资源是否与该节点相关
    // 4. 仅允许上述表格中的操作
}
```

### 1.4 证书身份映射

kubelet 的证书中包含了身份信息，Node Authorizer 根据这些信息判断权限：

```bash
# 查看 kubelet 证书身份
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -subject
# 输出: subject=O=system:nodes, CN=system:node:<node-name>

# O (Organization) = system:nodes  → 节点组
# CN (Common Name) = system:node:<node-name>  → 节点标识
```

---

## 二、NodeRestriction Admission Plugin

### 2.1 设计背景

Node Authorizer 限制了 kubelet 可以访问的资源范围，但没有限制 kubelet 对 Node 对象的修改行为。例如，kubelet 可以修改自身 Node 对象的任何字段，包括标签和注解。这意味着被入侵的 kubelet 可以通过修改节点标签来影响调度决策，或者通过修改注解来伪造状态信息。

NodeRestriction Admission Plugin 进一步限制了 kubelet 对 Node 对象和 Pod 对象的修改行为。

### 2.2 启用 NodeRestriction

```bash
# API Server 启动参数
--enable-admission-plugins=NodeRestriction
```

### 2.3 NodeRestriction 限制规则

NodeRestriction 对 kubelet 的操作施加了以下额外限制：

#### Node 对象修改限制

kubelet 只能修改自身 Node 对象的以下字段：

```yaml
# 允许修改的字段:
status.conditions: [...]       # 节点健康状态
status.addresses: [...]        # 节点地址
status.capacity: {...}         # 节点资源容量
status.allocatable: {...}      # 节点可分配资源
status.nodeInfo: {...}         # 节点系统信息
status.images: [...]           # 节点镜像列表
status.volumesInUse: [...]     # 正在使用的卷
status.volumesAttached: [...]  # 已挂载的卷
status.daemonEndpoints: {...}  # kubelet 端点

# 禁止修改的字段:
spec: {...}                    # 不能修改节点规格
metadata.labels: {...}         # 不能修改/添加/删除标签（kubelet 不应设置标签）
metadata.annotations: {...}    # 不能修改注解（部分例外）
```

#### Pod 对象修改限制

kubelet 只能修改绑定到自身节点的 Pod 的状态字段：

```go
// plugin/pkg/admission/noderestriction/admission.go
func (a *Plugin) Admit(ctx context.Context, attr admission.Attributes, ...) error {
    // 1. 验证请求者是否为 kubelet (system:nodes 组)
    // 2. 验证 kubelet 只能修改自身节点上的 Pod
    // 3. 验证只能修改 status 子资源
    // 4. 验证只能修改 status.conditions 和 status.containerStatuses
}
```

---

## 三、kubelet 安全配置

### 3.1 认证配置

```yaml
# /var/lib/kubelet/config.yaml
authentication:
  anonymous:
    enabled: false      # 禁止匿名访问 kubelet API
  webhook:
    enabled: true       # 通过 API Server 认证 (推荐)
    cacheTTL: 2h0m0s    # 认证结果缓存时间
  bootstrap:
    enabled: true       # 启用 Bootstrap Token 认证

authorization:
  mode: Webhook        # 通过 API Server 授权 (推荐)
  # mode: AlwaysAllow  # 不推荐：允许所有请求
```

**为什么必须禁用匿名访问**：kubelet 的 10250 端口暴露了 Pod 日志、执行命令、端口转发等敏感 API。如果允许匿名访问，攻击者可以直接读取 Pod 中的 Secret 或在容器中执行任意命令。

### 3.2 TLS 配置

```yaml
# /var/lib/kubelet/config.yaml
serverTLSBootstrap: true        # 通过 API Server 签发 kubelet 服务端证书
rotateCertificates: true        # 自动轮换客户端证书
# tlsCertFile: /path/to/cert   # 手动指定证书 (不推荐)
# tlsPrivateKeyFile: /path/to/key
```

### 3.3 保护 kubelet 端口

```bash
# kubelet 默认端口:
# 10250: HTTPS API (认证+授权)
# 10255: HTTP readonly API (已废弃，v1.28 移除)
# 10248: 健康检查端点 (localhost only)

# 检查 kubelet 端口是否对外暴露
ss -tlnp | grep kubelet

# 确保防火墙规则限制 10250 端口访问
iptables -A INPUT -p tcp --dport 10250 -s <trusted-cidr> -j ACCEPT
iptables -A INPUT -p tcp --dport 10250 -j DROP
```

---

## 四、Pod Security Standards

### 4.1 Pod Security 替代 PSP


Kubernetes v1.25 移除了 PodSecurityPolicy (PSP)，替换为 Pod Security Standards (PSS)。PSS 通过 Namespace 标签来强制执行 Pod 安全策略，无需创建额外的 API 对象。

### 4.2 三个安全级别

| 级别 | 说明 | 允许的权限 |
|------|------|-----------|
| **Privileged** | 不限制 | 所有权限，包括特权容器、主机命名空间共享 |
| **Baseline** | 最小限制 | 禁止明显的提权（hostNetwork、hostPID、privileged 等） |
| **Restricted** | 严格限制 | 进一步限制 capabilities、runAsNonRoot、seccomp 等 |

### 4.3 配置示例

```yaml
# 命名空间级别强制执行
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted      # 强制：违规 Pod 被拒绝
    pod-security.kubernetes.io/audit: restricted        # 审计：记录违规事件
    pod-security.kubernetes.io/warn: restricted         # 警告：用户看到警告
    pod-security.kubernetes.io/enforce-version: v1.28   # 策略版本
```

### 4.4 各级别详细对比

```yaml
# --- Privileged: 无限制 ---
# 允许所有操作，适用于系统组件
spec:
  containers:
  - name: app
    securityContext:
      privileged: true          # 允许特权模式
    hostNetwork: true           # 允许共享主机网络
    hostPID: true               # 允许共享主机 PID 命名空间

# --- Baseline: 基线安全 ---
# 禁止明显提权
spec:
  containers:
  - name: app
    securityContext:
      # privileged: false       # 禁止特权模式
      capabilities:
        drop: ["ALL"]           # 必须丢弃所有 capabilities
    # hostNetwork: false        # 禁止共享主机网络
    # hostPID: false            # 禁止共享主机 PID
    # hostIPC: false            # 禁止共享主机 IPC

# --- Restricted: 严格限制 ---
# 进一步强化安全
spec:
  containers:
  - name: app
    securityContext:
      allowPrivilegeEscalation: false
      runAsNonRoot: true
      readOnlyRootFilesystem: true
      seccompProfile:
        type: RuntimeDefault
      capabilities:
        drop: ["ALL"]
```

---

## 五、安全加固最佳实践

### 5.1 节点级别加固

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 内核安全参数
sysctl -w kernel.dmesg_restrict=1
sysctl -w kernel.kptr_restrict=2
sysctl -w net.ipv4.conf.all.send_redirects=0
sysctl -w net.ipv4.conf.default.accept_redirects=0

# 2. 文件系统权限
chmod 700 /etc/kubernetes/pki/
chmod 600 /etc/kubernetes/pki/*.key
chmod 600 /var/lib/kubelet/config.yaml

# 3. 禁用不必要的服务
systemctl disable avahi-daemon
systemctl disable cups

# 4. 配置 auditd 审计
auditctl -w /etc/kubernetes/pki/ -p wa -k k8s-pki
auditctl -w /var/lib/kubelet/ -p wa -k kubelet-data
```
### 5.2 网络隔离

```bash
# 限制 kubelet API 访问
iptables -A INPUT -p tcp --dport 10250 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 10250 -j DROP

# 限制 etcd 访问 (仅控制面节点)
iptables -A INPUT -p tcp --dport 2379 -s <control-plane-cidr> -j ACCEPT
iptables -A INPUT -p tcp --dport 2379 -j DROP
```

### 5.3 定期安全扫描

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kube-bench 进行 CIS 合规检查
kube-bench run --targets master,node

# 使用 kubectl who-can 检查 RBAC 权限
kubectl who-can list pods --namespace default

# 检查 kubelet 匿名访问
curl -k https://<node>:10250/pods
# 应该返回 401 Unauthorized
```
---

## 六、常见错误与排查

### 6.1 常见错误

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| `nodes is forbidden: User "system:node:xxx" cannot get node` | Node Authorizer 未启用 | 检查 `--authorization-mode` | 添加 `Node` 到 `--authorization-mode` |
| kubelet 认证失败 | 证书过期或 CN 不匹配 | `openssl x509 -in <cert> -noout -subject -dates` | 续期证书，确保 CN 格式为 `system:node:<name>` |
| `nodes "xxx" is forbidden: node xxx cannot modify node yyy` | NodeRestriction 阻止 | 检查 kubelet `--hostname-override` | 确保 hostname 与证书 CN 匹配 |
| Pod Security 违规 | Namespace 标签配置了 restricted | `kubectl get ns <ns> -o yaml` | 调整 Pod 安全上下文或 Namespace 标签 |
| kubelet 401 Unauthorized | 匿名访问被拒绝 | `curl -k https://<node>:10250/healthz` | 使用有效证书访问 |
| RBAC 权限不足 | ClusterRole 未绑定 | `kubectl auth can-i --as=system:node:xxx list pods` | 检查 ClusterRoleBinding |

### 6.2 权限调试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 kubelet 是否有特定权限
kubectl auth can-i list pods --as=system:node:node-1
kubectl auth can-i get secrets --as=system:node:node-1

# 检查 Node Authorizer 决策
# API Server 审计日志中搜索:
# authorization.k8s.io/decision

# 查看 kubelet 使用的证书身份
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem \
  -noout -subject -issuer -dates
```
---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `NodeAuthorizer.Authorize` | `pkg/kube-apiserver/authorizer/node/node_authorizer.go` | Node 授权主入口 |
| `NodeAuthorizer.rulesFor` | `pkg/kube-apiserver/authorizer/node/node_authorizer.go` | 查询节点允许的规则 |
| `Plugin.Admit` | `plugin/pkg/admission/noderestriction/admission.go` | NodeRestriction 准入逻辑 |
| `PodSecurityAdmission` | `pkg/security/podsecurity/` | Pod Security 准入插件 |
| `kubeletAuthentication` | `pkg/kubelet/server/auth.go` | kubelet 认证配置 |
| `csrapproving controller` | `pkg/controller/certificates/approval/sarapproval.go` | CSR 自动审批 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]


<!-- risk-assessed -->
