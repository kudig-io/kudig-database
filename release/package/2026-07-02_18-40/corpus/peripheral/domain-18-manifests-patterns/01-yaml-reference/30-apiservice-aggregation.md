---
title: 30 - APIService YAML 配置参考
description: '# 30 - APIService YAML 配置参考'
summary: '1. [APIService 基础概念](#1-apiservice-基础概念)'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- etcd
- apiserver
- kubelet
- rbac
- crd
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- APIService YAML 配置参考 是什么
- 如何 APIService YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- APIService
- YAML
- 配置参考
- yaml
- manifests
prerequisites:
- kubectl-basics
- etcd-basics
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
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md
  label: '故障树: service'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 30 - APIService YAML 配置参考

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02

**本文档全面覆盖 APIService(API 聚合层)的 YAML 配置**,包括完整字段说明、本地 vs 远程 APIService、内部原理、与 Metrics Server 集成、自定义 API Server 开发等。

---

<!-- chunk: 📋 目录 -->## 📋 目录

1. [APIService 基础概念](#1-apiservice-基础概念)
2. [完整字段说明](#2-完整字段说明)
3. [本地 vs 远程 APIService](#3-本地-vs-远程-apiservice)
4. [内部原理](#4-内部原理)
5. [生产案例](#5-生产案例)
6. [故障排查](#6-故障排查)

---

<!-- chunk: 1. APIService 基础概念 -->## 1. APIService 基础概念

## 1.1 什么是 APIService

APIService 是 Kubernetes 的**聚合层(Aggregation Layer)**机制,允许扩展 API Server 的功能:

- **动态 API 扩展**: 将自定义 API 路由到独立的 API Server(不依赖 kube-apiserver 重启)
- **灵活存储**: 自定义 API Server 可以使用自己的存储后端(不限于 [[etcd|etcd]])
- **复杂业务逻辑**: 支持复杂的计算、聚合、外部系统集成
- **透明代理**: 客户端无需感知后端 API Server,统一通过 kube-apiserver 访问

## 1.2 APIService vs CRD

| 特性 | APIService (聚合 API) | CRD |
|------|----------------------|-----|
| **实现复杂度** | 高(需要独立 API Server) | 低(仅需 YAML 定义) |
| **存储** | 自定义(任意数据库) | etcd(固定) |
| **业务逻辑** | 完全自定义(任意代码) | 受限于 OpenAPI Schema + CEL |
| **性能** | 中(额外网络跳转) | 高(直接由 kube-apiserver 处理) |
| **适用场景** | 计算型/聚合型资源 | 配置型资源 |
| **典型案例** | Metrics Server, Custom Metrics API | 自定义 CRD(如 Database, Pipeline) |

## 1.3 架构图

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│ kubectl / 客户端                                                │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ kube-apiserver                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ API Aggregation Layer (kube-aggregator)                  │  │
│  │  - 检查请求路径(如 /apis/metrics.k8s.io/v1beta1/nodes)   │  │
│  │  - 匹配 APIService 路由规则                              │  │
│  │  - 代理请求到后端 Service                                │  │
│  └────────────┬─────────────────────────────────────────────┘  │
└───────────────┼────────────────────────────────────────────────┘
                │ TLS 连接(使用 caBundle 验证)
                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 后端 Service (如 metrics-server.kube-system.svc)               │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 自定义 API Server Pod (如 metrics-server)                      │
│  - 实现 API 逻辑(如聚合节点/Pod 指标)                          │
│  - 自定义存储(内存、外部 DB、外部 API)                         │
└─────────────────────────────────────────────────────────────────┘
```
---

<!-- chunk: 2. 完整字段说明 -->## 2. 完整字段说明

## 2.1 基础结构 YAML

```yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  # APIService 名称格式: <version>.<group>
  name: v1beta1.metrics.k8s.io
  labels:
    # 推荐添加标签标识用途
    k8s-app: metrics-server
    kubernetes.io/cluster-service: "true"
spec:
  # === API 组和版本定义 ===
  
  # API 组名(留空表示核心 API 组)
  group: metrics.k8s.io
  
  # API 版本
  version: v1beta1
  
  # === 优先级配置(用于路由冲突解决) ===
  
  # 组优先级(数值越大优先级越高,范围 1-20000)
  groupPriorityMinimum: 100
  
  # 版本优先级(同一组内版本排序,数值越大优先级越高)
  versionPriority: 100
  
  # === 后端服务配置 ===
  
  service:
    # 服务命名空间
    namespace: kube-system
    
    # 服务名称
    name: metrics-server
    
    # 服务端口(可选,默认 443)
    port: 443
  
  # === TLS 配置 ===
  
  # CA 证书(Base64 编码,用于验证后端服务端证书)
  caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSURKekNDQWcrZ0F3SUJBZ0lVYS...
  
  # 是否跳过 TLS 验证(生产环境禁用,仅用于开发测试)
  insecureSkipTLSVerify: false
```

## 2.2 本地 APIService(API Server 内置)

某些 API 组由 kube-apiserver 直接提供,无需后端 Service:

```yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1.apps
spec:
  group: apps
  version: v1
  groupPriorityMinimum: 17800
  versionPriority: 15
  # service 字段为空,表示本地 APIService
  service: null
```

## 2.3 完整示例 - Metrics Server

```yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io
  labels:
    k8s-app: metrics-server
    kubernetes.io/cluster-service: "true"
spec:
  # API 定义
  group: metrics.k8s.io
  version: v1beta1
  
  # 优先级配置
  groupPriorityMinimum: 100
  versionPriority: 100
  
  # 后端服务
  service:
    namespace: kube-system
    name: metrics-server
    port: 443
  
  # TLS 配置(使用 CA Bundle)
  caBundle: LS0tLS1CRUdJTi0tLS0t...
  insecureSkipTLSVerify: false

---
# 对应的后端 Service
apiVersion: v1
kind: Service
metadata:
  name: metrics-server
  namespace: kube-system
  labels:
    k8s-app: metrics-server
spec:
  selector:
    k8s-app: metrics-server
  ports:
    - name: https
      port: 443
      targetPort: https
      protocol: TCP

---
# Metrics Server Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      k8s-app: metrics-server
  template:
    metadata:
      labels:
        k8s-app: metrics-server
    spec:
      serviceAccountName: metrics-server
      containers:
        - name: metrics-server
          image: registry.k8s.io/metrics-server/metrics-server:v0.7.0
          args:
            # 从 Kubelet 收集指标
            - --kubelet-preferred-address-types=InternalIP,Hostname,ExternalIP
            # 证书配置
            - --cert-dir=/tmp
            - --secure-port=4443
            # TLS 证书(由 Secret 提供)
            - --tls-cert-file=/certs/tls.crt
            - --tls-private-key-file=/certs/tls.key
          ports:
            - name: https
              containerPort: 4443
              protocol: TCP
          volumeMounts:
            - name: certs
              mountPath: /certs
              readOnly: true
      volumes:
        - name: certs
          secret:
            secretName: metrics-server-certs
```

---

<!-- chunk: 3. 本地 vs 远程 APIService -->## 3. 本地 vs 远程 APIService

## 3.1 本地 APIService

**特点**: 由 kube-apiserver 直接提供,无需额外 Pod

```yaml
# 示例: apps/v1 API
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1.apps
spec:
  group: apps
  version: v1
  groupPriorityMinimum: 17800
  versionPriority: 15
  service: null  # 无后端服务

# 查看所有本地 APIService
kubectl get apiservices | grep '<none>'
# 输出:
# v1.                          Local  True   ...
# v1.apps                      Local  True   ...
# v1.batch                     Local  True   ...
```

## 3.2 远程 APIService

**特点**: 代理到独立的 API Server Pod

```yaml
# 示例: metrics.k8s.io/v1beta1
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io
spec:
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 100
  versionPriority: 100
  service:
    namespace: kube-system
    name: metrics-server
    port: 443
  caBundle: LS0tLS1...

# 查看所有远程 APIService
kubectl get apiservices | grep -v '<none>'
# 输出:
# v1beta1.metrics.k8s.io    kube-system/metrics-server  True  ...
```

## 3.3 对比表

| 维度 | 本地 APIService | 远程 APIService |
|------|----------------|----------------|
| **处理进程** | kube-apiserver | 独立 API Server Pod |
| **Service 配置** | `service: null` | 必须指定 Service |
| **TLS 配置** | 不需要 | 需要 caBundle 或 insecureSkipTLSVerify |
| **性能** | 高(无额外网络跳转) | 中(需要代理) |
| **可用性** | 依赖 kube-apiserver | 独立(可独立扩缩容) |
| **典型案例** | apps, batch, core API | metrics.k8s.io, custom.metrics.k8s.io |

---

<!-- chunk: 4. 内部原理 -->## 4. 内部原理

## 4.1 API 聚合层路由

```
# 🟢 低风险：只读/信息收集，通常无副作用
客户端请求 (kubectl top nodes)
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. kube-apiserver 接收请求                                      │
│    GET /apis/metrics.k8s.io/v1beta1/nodes                       │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. kube-aggregator 路由匹配                                     │
│    - 解析请求路径: group=metrics.k8s.io, version=v1beta1        │
│    - 查找 APIService: v1beta1.metrics.k8s.io                    │
│    - 读取后端服务配置: kube-system/metrics-server:443           │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. 建立到后端 Service 的 TLS 连接                               │
│    - DNS 解析: metrics-server.kube-system.svc.cluster.local    │
│    - TLS 握手: 使用 caBundle 验证服务端证书                     │
│    - 请求头注入:                                                │
│      * X-Remote-User: <原始用户名>                             │
│      * X-Remote-Group: <原始用户组>                            │
│      * Impersonate-User: <如果有模拟请求>                      │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 4. metrics-server Pod 处理请求                                  │
│    - 验证客户端证书(检查 X-Remote-User 等请求头)                │
│    - 执行业务逻辑(从 Kubelet Summary API 聚合指标)              │
│    - 返回响应: {nodes: [{name: "node1", usage: {...}}]}        │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 5. kube-apiserver 返回响应给客户端                              │
│    - 透明代理(客户端无感知后端 Service 存在)                    │
└─────────────────────────────────────────────────────────────────┘
```
## 4.2 优先级解析

当多个 APIService 定义相同的 group/version 时(极少见),优先级规则:

```yaml
# 示例: 两个 APIService 都定义 metrics.k8s.io/v1beta1
---
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io.high
spec:
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 200  # 更高优先级
  versionPriority: 100
  service:
    namespace: kube-system
    name: metrics-server-v2

---
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io.low
spec:
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 100  # 较低优先级
  versionPriority: 100
  service:
    namespace: kube-system
    name: metrics-server-v1

# 解析规则:
# 1. groupPriorityMinimum 高者优先(200 > 100)
# 2. 如果相同,则 versionPriority 高者优先
# 3. 如果仍相同,按 metadata.name 字典序排序
```

## 4.3 健康检查机制

APIService 会持续监控后端 Service 的可用性:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 APIService 状态
kubectl get apiservices v1beta1.metrics.k8s.io -o yaml

# 输出:
status:
  conditions:
    # Available=True 表示后端可用
    - type: Available
      status: "True"
      lastTransitionTime: "2026-02-10T10:00:00Z"
      reason: Passed
      message: all checks passed

# 如果后端不可用:
status:
  conditions:
    - type: Available
      status: "False"
      lastTransitionTime: "2026-02-10T10:05:00Z"
      reason: ServiceNotFound
      message: service/metrics-server in "kube-system" is not ready
```
**健康检查流程**:

1. **连接检查**: 每 10 秒尝试连接后端 Service 的 443 端口
2. **证书验证**: 验证后端服务端证书是否与 caBundle 匹配
3. **HTTP 探测**: 发送 GET /healthz 或 GET /readyz 请求
4. **状态更新**: 更新 APIService.status.conditions

---

<!-- chunk: 5. 生产案例 -->## 5. 生产案例

## 5.1 Metrics Server 完整部署

**1. 生成 TLS 证书**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 生成 CA 证书
openssl req -x509 -newkey rsa:4096 -nodes -keyout ca.key -out ca.crt \
  -subj "/CN=metrics-server-ca" -days 3650

# 生成服务端证书
cat > csr.conf <<EOF
[req]
req_extensions = v3_req
distinguished_name = req_distinguished_name
[req_distinguished_name]
[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names
[alt_names]
DNS.1 = metrics-server
DNS.2 = metrics-server.kube-system
DNS.3 = metrics-server.kube-system.svc
DNS.4 = metrics-server.kube-system.svc.cluster.local
EOF

openssl req -newkey rsa:4096 -nodes -keyout server.key -out server.csr \
  -subj "/CN=metrics-server.kube-system.svc" -config csr.conf

openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days 365 -extensions v3_req -extfile csr.conf

# 创建 Secret
kubectl create secret tls metrics-server-certs \
  --cert=server.crt --key=server.key -n kube-system
```
**2. 部署 Metrics Server**

```yaml
# metrics-server-rbac.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: metrics-server
  namespace: kube-system

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: system:metrics-server
rules:
  # 读取节点指标
  - apiGroups: [""]
    resources: ["nodes/stats"]
    verbs: ["get", "list", "watch"]
  # 读取 Pod 指标
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
  # 读取节点信息
  - apiGroups: [""]
    resources: ["nodes"]
    verbs: ["get", "list", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: system:metrics-server
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:metrics-server
subjects:
  - kind: ServiceAccount
    name: metrics-server
    namespace: kube-system

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: metrics-server:system:auth-delegator
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:auth-delegator
subjects:
  - kind: ServiceAccount
    name: metrics-server
    namespace: kube-system

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: metrics-server-auth-reader
  namespace: kube-system
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: extension-apiserver-authentication-reader
subjects:
  - kind: ServiceAccount
    name: metrics-server
    namespace: kube-system

---
# metrics-server-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
  labels:
    k8s-app: metrics-server
spec:
  replicas: 2  # 高可用部署
  selector:
    matchLabels:
      k8s-app: metrics-server
  template:
    metadata:
      labels:
        k8s-app: metrics-server
    spec:
      serviceAccountName: metrics-server
      # 优先级调度(确保关键组件优先调度)
      priorityClassName: system-cluster-critical
      # 节点选择器(推荐部署到稳定节点)
      nodeSelector:
        kubernetes.io/os: linux
      # 反亲和性(避免单点问题)
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    k8s-app: metrics-server
                topologyKey: kubernetes.io/hostname
      containers:
        - name: metrics-server
          image: registry.k8s.io/metrics-server/metrics-server:v0.7.0
          imagePullPolicy: IfNotPresent
          args:
            # Kubelet 连接配置
            - --kubelet-preferred-address-types=InternalIP,Hostname,ExternalIP
            - --kubelet-use-node-status-port
            - --metric-resolution=15s  # 指标收集间隔
            # TLS 配置
            - --cert-dir=/tmp
            - --secure-port=4443
            - --tls-cert-file=/certs/tls.crt
            - --tls-private-key-file=/certs/tls.key
            # 认证配置
            - --authorization-always-allow-paths=/livez,/readyz,/healthz
            - --enable-aggregator-routing=true
          ports:
            - name: https
              containerPort: 4443
              protocol: TCP
          # 存活探测
          livenessProbe:
            httpGet:
              path: /livez
              port: https
              scheme: HTTPS
            initialDelaySeconds: 10
            periodSeconds: 10
            failureThreshold: 3
          # 就绪探测
          readinessProbe:
            httpGet:
              path: /readyz
              port: https
              scheme: HTTPS
            initialDelaySeconds: 20
            periodSeconds: 10
            failureThreshold: 3
          # 资源限制
          resources:
            requests:
              cpu: 100m
              memory: 200Mi
            limits:
              cpu: 1000m
              memory: 1Gi
          # 安全上下文
          securityContext:
            runAsNonRoot: true
            runAsUser: 1000
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            capabilities:
              drop:
                - ALL
          # 挂载证书
          volumeMounts:
            - name: certs
              mountPath: /certs
              readOnly: true
            - name: tmp
              mountPath: /tmp
      volumes:
        - name: certs
          secret:
            secretName: metrics-server-certs
        - name: tmp
          emptyDir: {}

---
# metrics-server-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: metrics-server
  namespace: kube-system
  labels:
    k8s-app: metrics-server
spec:
  selector:
    k8s-app: metrics-server
  ports:
    - name: https
      port: 443
      targetPort: https
      protocol: TCP
  # 会话亲和性(可选)
  sessionAffinity: ClientIP

---
# metrics-server-apiservice.yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io
  labels:
    k8s-app: metrics-server
spec:
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 100
  versionPriority: 100
  service:
    namespace: kube-system
    name: metrics-server
    port: 443
  # 使用 CA 证书验证
  caBundle: |
    LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSURKekNDQWcrZ0F3SUJBZ0lVYS...
    (cat ca.crt | base64 -w0 输出)
  insecureSkipTLSVerify: false
```

**3. 部署和验证**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 应用所有资源
kubectl apply -f metrics-server-rbac.yaml
kubectl apply -f metrics-server-deployment.yaml
kubectl apply -f metrics-server-service.yaml
kubectl apply -f metrics-server-apiservice.yaml

# 检查 APIService 状态
kubectl get apiservices v1beta1.metrics.k8s.io
# 输出:
# NAME                        SERVICE                    AVAILABLE   AGE
# v1beta1.metrics.k8s.io      kube-system/metrics-server True        1m

# 测试节点指标
kubectl top nodes
# 输出:
# NAME       CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node1      250m         12%    2048Mi          25%
# node2      180m         9%     1536Mi          19%

# 测试 Pod 指标
kubectl top pods -n kube-system
```
## 5.2 自定义 API Server - Task 资源示例

**场景**: 创建一个自定义 API Server 管理异步任务

**1. 自定义 API Server 代码(Go 示例)**

```go
// main.go
package main

import (
    "context"
    "encoding/json"
    "fmt"
    "net/http"
    "sync"
    
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apiserver/pkg/server"
)

// Task 资源定义
type Task struct {
    metav1.TypeMeta   `json:",inline"`
    metav1.ObjectMeta `json:"metadata,omitempty"`
    Spec   TaskSpec   `json:"spec"`
    Status TaskStatus `json:"status,omitempty"`
}

type TaskSpec struct {
    Command string   `json:"command"`
    Args    []string `json:"args,omitempty"`
}

type TaskStatus struct {
    Phase      string `json:"phase"`  // Pending, Running, Succeeded, Failed
    StartTime  string `json:"startTime,omitempty"`
    ExitCode   int    `json:"exitCode,omitempty"`
}

// 内存存储(生产环境应使用数据库)
type TaskStore struct {
    mu    sync.RWMutex
    tasks map[string]*Task
}

func NewTaskStore() *TaskStore {
    return &TaskStore{tasks: make(map[string]*Task)}
}

func (s *TaskStore) Create(task *Task) error {
    s.mu.Lock()
    defer s.mu.Unlock()
    s.tasks[task.Name] = task
    return nil
}

func (s *TaskStore) Get(name string) (*Task, error) {
    s.mu.RLock()
    defer s.mu.RUnlock()
    task, ok := s.tasks[name]
    if !ok {
        return nil, fmt.Errorf("task %s not found", name)
    }
    return task, nil
}

func (s *TaskStore) List() []*Task {
    s.mu.RLock()
    defer s.mu.RUnlock()
    result := make([]*Task, 0, len(s.tasks))
    for _, t := range s.tasks {
        result = append(result, t)
    }
    return result
}

// HTTP Handler
func main() {
    store := NewTaskStore()
    
    http.HandleFunc("/apis/tasks.example.com/v1/namespaces/default/tasks", func(w http.ResponseWriter, r *http.Request) {
        switch r.Method {
        case "GET":
            tasks := store.List()
            json.NewEncoder(w).Encode(map[string]interface{}{
                "apiVersion": "tasks.example.com/v1",
                "kind":       "TaskList",
                "items":      tasks,
            })
        case "POST":
            var task Task
            json.NewDecoder(r.Body).Decode(&task)
            task.Status.Phase = "Pending"
            store.Create(&task)
            w.WriteHeader(http.StatusCreated)
            json.NewEncoder(w).Encode(task)
        }
    })
    
    http.HandleFunc("/apis/tasks.example.com/v1/namespaces/default/tasks/", func(w http.ResponseWriter, r *http.Request) {
        name := r.URL.Path[len("/apis/tasks.example.com/v1/namespaces/default/tasks/"):]
        task, err := store.Get(name)
        if err != nil {
            http.Error(w, err.Error(), http.StatusNotFound)
            return
        }
        json.NewEncoder(w).Encode(task)
    })
    
    http.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
        w.Write([]byte("ok"))
    })
    
    fmt.Println("Starting API Server on :8443")
    http.ListenAndServeTLS(":8443", "/certs/tls.crt", "/certs/tls.key", nil)
}
```

**2. 部署配置**

```yaml
# task-apiserver-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: task-apiserver
  namespace: custom-api
spec:
  replicas: 2
  selector:
    matchLabels:
      app: task-apiserver
  template:
    metadata:
      labels:
        app: task-apiserver
    spec:
      containers:
        - name: apiserver
          image: myregistry.com/task-apiserver:v1.0.0
          ports:
            - containerPort: 8443
              name: https
          volumeMounts:
            - name: certs
              mountPath: /certs
              readOnly: true
      volumes:
        - name: certs
          secret:
            secretName: task-apiserver-certs

---
apiVersion: v1
kind: Service
metadata:
  name: task-apiserver
  namespace: custom-api
spec:
  selector:
    app: task-apiserver
  ports:
    - port: 443
      targetPort: 8443

---
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1.tasks.example.com
spec:
  group: tasks.example.com
  version: v1
  groupPriorityMinimum: 1000
  versionPriority: 15
  service:
    namespace: custom-api
    name: task-apiserver
    port: 443
  caBundle: LS0tLS1...
  insecureSkipTLSVerify: false
```

**3. 使用自定义资源**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Task
kubectl create -f - <<EOF
apiVersion: tasks.example.com/v1
kind: Task
metadata:
  name: my-task
  namespace: default
spec:
  command: "echo"
  args: ["Hello", "World"]
EOF

# 查询 Task
kubectl get tasks
kubectl get tasks my-task -o yaml

# API 直接访问
kubectl get --raw /apis/tasks.example.com/v1/namespaces/default/tasks
```
---

<!-- chunk: 6. 故障排查 -->## 6. 故障排查

## 6.1 APIService 不可用

**症状**: `kubectl get apiservices` 显示 `Available=False`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 APIService 状态
kubectl get apiservices v1beta1.metrics.k8s.io -o yaml

# 输出:
status:
  conditions:
    - type: Available
      status: "False"
      reason: ServiceNotFound
      message: service/metrics-server in "kube-system" is not present
```
**排查步骤:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查后端 Service 是否存在
kubectl get svc metrics-server -n kube-system

# 2. 检查 Service Endpoints(是否有 Ready Pod)
kubectl get endpoints metrics-server -n kube-system

# 3. 检查 Pod 状态
kubectl get pods -n kube-system -l k8s-app=metrics-server

# 4. 查看 Pod 日志
kubectl logs -n kube-system -l k8s-app=metrics-server

# 5. 测试连通性(从 kube-apiserver Pod 内部)
kubectl exec -n kube-system kube-apiserver-xxx -- \
  curl -k https://metrics-server.kube-system.svc:443/healthz
```
## 6.2 TLS 证书验证失败

**症状**: `x509: certificate signed by unknown authority`

```bash
# 错误日志(kube-apiserver)
E0210 10:00:00.123456 1 controller.go:116] loading OpenAPI spec for "v1beta1.metrics.k8s.io" failed with: 
  failed to retrieve openAPI spec: Get "https://metrics-server.kube-system.svc:443/openapi/v2": 
  x509: certificate signed by unknown authority
```

**解决方案:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 验证 caBundle 配置
kubectl get apiservices v1beta1.metrics.k8s.io -o jsonpath='{.spec.caBundle}' | base64 -d | openssl x509 -text

# 2. 检查服务端证书
kubectl exec -n kube-system metrics-server-xxx -- \
  openssl s_client -connect localhost:4443 -showcerts

# 3. 确保 CA 证书匹配
# caBundle 应该是签发服务端证书的 CA 证书

# 4. 临时跳过验证(仅用于调试)
kubectl patch apiservice v1beta1.metrics.k8s.io --type=json -p='[
  {"op": "replace", "path": "/spec/insecureSkipTLSVerify", "value": true},
  {"op": "remove", "path": "/spec/caBundle"}
]'
```
## 6.3 请求超时

**症状**: `context deadline exceeded`

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查网络连通性
kubectl run test-curl --image=curlimages/curl --rm -it -- \
  curl -m 5 -k https://metrics-server.kube-system.svc:443/healthz

# 检查 Service 端口配置
kubectl get svc metrics-server -n kube-system -o yaml

# 检查容器端口映射
kubectl get pods -n kube-system -l k8s-app=metrics-server -o jsonpath='{.items[*].spec.containers[*].ports}'

# 查看 kube-apiserver 日志
kubectl logs -n kube-system kube-apiserver-xxx | grep metrics-server
```
## 6.4 认证失败

**症状**: `User "system:anonymous" cannot get path "/apis/metrics.k8s.io/v1beta1"`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 RBAC 配置
kubectl get clusterrolebinding | grep metrics-server

# 验证 ServiceAccount
kubectl get sa metrics-server -n kube-system

# 检查 extension-apiserver-authentication-reader Role
kubectl get role extension-apiserver-authentication-reader -n kube-system -o yaml

# 确保 RoleBinding 存在
kubectl get rolebinding metrics-server-auth-reader -n kube-system
```
## 6.5 调试技巧

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 启用 kube-apiserver 详细日志
# 在 kube-apiserver 启动参数中添加:
--v=6  # 或更高级别

# 2. 直接访问 API(绕过 Aggregation Layer)
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes

# 3. 查看 APIService 优先级
kubectl get apiservices --sort-by=.spec.groupPriorityMinimum

# 4. 监控 APIService 变化
kubectl get apiservices -w

# 5. 查看 kube-aggregator 日志
kubectl logs -n kube-system kube-apiserver-xxx | grep aggregator
```
---

<!-- chunk: 📚 参考资源 -->## 📚 参考资源

- **官方文档**:
  - [Extend [[domain-17-system-foundation/知识字典/fundamentals/the-kubernetes-api.md|the Kubernetes API]] with the aggregation layer](https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/apiserver-aggregation/)
  - [Configure the aggregation layer](https://kubernetes.io/docs/tasks/extend-kubernetes/configure-aggregation-layer/)
  - [Metrics Server](https://github.com/kubernetes-sigs/metrics-server)
- **API Server 开发**:
  - [apiserver-builder](https://github.com/kubernetes-sigs/apiserver-builder-alpha)
  - [sample-apiserver](https://github.com/kubernetes/sample-apiserver)

---

**最佳实践总结**:

1. **TLS 证书管理**: 始终使用 `caBundle` 验证,避免 `insecureSkipTLSVerify: true`(仅限开发测试)
2. **高可用部署**: 后端 API Server 至少部署 2 个副本,配置 PodDisruptionBudget
3. **健康检查**: 实现 `/livez`, `/readyz`, `/healthz` 端点,便于监控
4. **认证授权**: 使用 `X-Remote-User` 请求头获取原始用户信息,配合 RBAC
5. **性能优化**: 合理设置资源限制,避免 API Server Pod OOM
6. **监控告警**: 监控 APIService `Available` 状态,及时发现后端服务问题
7. **版本管理**: 使用独立的 APIService 版本(如 v1, v2),避免直接修改现有版本

---

🚀 **APIService 是 Kubernetes 扩展能力的高级形态,适合构建复杂的平台服务!**

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[domain-18-manifests-patterns/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- Domain-32 YAML 清单 — 开源项目索引
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 08 - Service 全类型 YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考

## See Also

- 28-poddisruptionbudget-reference
- 29-customresourcedefinition
- 31-api-priority-fairness
- 32-lease-event-node


<!-- risk-assessed -->
