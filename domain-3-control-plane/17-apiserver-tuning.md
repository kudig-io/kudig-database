# API Server 性能调优

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/)

## API Server架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     API Server 请求处理架构                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                     客户端请求流程                                    │  │
│   │                                                                      │  │
│   │   Client Request                                                     │  │
│   │        │                                                             │  │
│   │        ▼                                                             │  │
│   │   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐          │  │
│   │   │    TLS      │────▶│   认证      │────▶│   授权      │          │  │
│   │   │  Termination│     │ AuthN      │     │  AuthZ     │          │  │
│   │   └─────────────┘     └─────────────┘     └─────────────┘          │  │
│   │                                                 │                    │  │
│   │                                                 ▼                    │  │
│   │   ┌─────────────────────────────────────────────────────────────┐  │  │
│   │   │              API Priority and Fairness (APF)                 │  │  │
│   │   │  ┌─────────────────────────────────────────────────────┐   │  │  │
│   │   │  │  FlowSchema匹配 → PriorityLevel → 队列分配 → 执行   │   │  │  │
│   │   │  └─────────────────────────────────────────────────────┘   │  │  │
│   │   └─────────────────────────────────────────────────────────────┘  │  │
│   │                                                 │                    │  │
│   │                                                 ▼                    │  │
│   │   ┌─────────────────────────────────────────────────────────────┐  │  │
│   │   │                    Admission Controllers                     │  │  │
│   │   │  ┌────────────┐  ┌────────────┐  ┌────────────────────┐    │  │  │
│   │   │  │  Mutating  │─▶│ Validating │─▶│ ResourceQuota等    │    │  │  │
│   │   │  │  Webhook   │  │  Webhook   │  │                    │    │  │  │
│   │   │  └────────────┘  └────────────┘  └────────────────────┘    │  │  │
│   │   └─────────────────────────────────────────────────────────────┘  │  │
│   │                                                 │                    │  │
│   │                                                 ▼                    │  │
│   │   ┌─────────────────────────────────────────────────────────────┐  │  │
│   │   │                       etcd 存储                              │  │  │
│   │   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐     │  │  │
│   │   │  │ Watch Cache │  │   Codec     │  │ etcd Client     │     │  │  │
│   │   │  │             │  │ 编解码      │  │ gRPC连接         │     │  │  │
│   │   │  └─────────────┘  └─────────────┘  └─────────────────┘     │  │  │
│   │   └─────────────────────────────────────────────────────────────┘  │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                     并发控制模型                                      │  │
│   │                                                                      │  │
│   │   ┌─────────────────────────────────────────────────────────────┐  │  │
│   │   │ max-requests-inflight (非mutating)                          │  │  │
│   │   │ ├── GET /api/v1/pods                                        │  │  │
│   │   │ ├── LIST /api/v1/namespaces                                 │  │  │
│   │   │ └── WATCH /api/v1/events                                    │  │  │
│   │   └─────────────────────────────────────────────────────────────┘  │  │
│   │   ┌─────────────────────────────────────────────────────────────┐  │  │
│   │   │ max-mutating-requests-inflight (mutating)                   │  │  │
│   │   │ ├── POST /api/v1/pods                                       │  │  │
│   │   │ ├── PUT /api/v1/deployments/xxx                            │  │  │
│   │   │ ├── PATCH /api/v1/services/xxx                             │  │  │
│   │   │ └── DELETE /api/v1/pods/xxx                                │  │  │
│   │   └─────────────────────────────────────────────────────────────┘  │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## API Server关键参数详解

### 并发控制参数

| 参数 | 默认值 | 小集群 | 中集群 | 大集群 | 说明 |
|------|--------|-------|-------|-------|------|
| **--max-requests-inflight** | 400 | 400 | 800 | 1600 | 非mutating请求并发数 |
| **--max-mutating-requests-inflight** | 200 | 200 | 400 | 800 | mutating请求并发数 |
| **--request-timeout** | 60s | 60s | 60s | 60s | 请求超时时间 |
| **--min-request-timeout** | 1800 | 1800 | 1800 | 1800 | Watch最小超时 |

### Watch缓存参数

| 参数 | 默认值 | 推荐值 | 说明 | 调优场景 |
|------|--------|-------|------|---------|
| **--default-watch-cache-size** | 100 | 500-1000 | 默认Watch缓存大小 | 提高Watch性能 |
| **--watch-cache-sizes** | 自动 | 按资源配置 | 特定资源缓存大小 | 热点资源优化 |

### etcd相关参数

| 参数 | 默认值 | 推荐值 | 说明 |
|------|--------|-------|------|
| **--etcd-servers** | - | 多节点 | etcd服务器地址 |
| **--etcd-compaction-interval** | 5m | 5m | 压缩间隔 |
| **--etcd-count-metric-poll-period** | 1m | 1m | 计数指标周期 |
| **--etcd-servers-overrides** | - | 按需配置 | etcd分库配置 |
| **--storage-media-type** | application/vnd.kubernetes.protobuf | protobuf | 存储格式 |

### 审计日志参数

| 参数 | 默认值 | 推荐值 | 说明 |
|------|--------|-------|------|
| **--audit-log-path** | - | /var/log/audit.log | 审计日志路径 |
| **--audit-log-maxage** | 0 | 7 | 保留天数 |
| **--audit-log-maxsize** | 0 | 100 | 单文件大小(MB) |
| **--audit-log-maxbackup** | 0 | 10 | 备份文件数 |
| **--audit-log-format** | json | json | 日志格式 |
| **--audit-policy-file** | - | 配置文件 | 审计策略 |

## 大规模集群配置

### 完整配置示例

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
  labels:
    component: kube-apiserver
    tier: control-plane
spec:
  hostNetwork: true
  priorityClassName: system-node-critical
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.30.0
    command:
    - kube-apiserver
    
    # ==================== 基础配置 ====================
    - --advertise-address=$(POD_IP)
    - --allow-privileged=true
    - --enable-admission-plugins=NodeRestriction,ResourceQuota,LimitRanger,PodSecurity,ValidatingAdmissionPolicy
    - --disable-admission-plugins=PodSecurityPolicy
    
    # ==================== 认证授权 ====================
    - --authorization-mode=Node,RBAC
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --enable-bootstrap-token-auth=true
    - --kubelet-certificate-authority=/etc/kubernetes/pki/ca.crt
    - --kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt
    - --kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key
    
    # ==================== TLS配置 ====================
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    - --tls-cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
    
    # ==================== 并发控制(大集群优化) ====================
    - --max-requests-inflight=1600
    - --max-mutating-requests-inflight=800
    - --request-timeout=60s
    - --min-request-timeout=1800
    
    # ==================== Watch缓存(大集群优化) ====================
    - --default-watch-cache-size=1000
    - --watch-cache-sizes=pods#5000,nodes#1000,services#1000,endpoints#2000,configmaps#1000,secrets#1000
    
    # ==================== etcd配置 ====================
    - --etcd-servers=https://etcd-0.etcd:2379,https://etcd-1.etcd:2379,https://etcd-2.etcd:2379
    - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
    - --etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
    - --etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
    - --etcd-compaction-interval=5m
    - --etcd-count-metric-poll-period=1m
    - --storage-media-type=application/vnd.kubernetes.protobuf
    
    # ==================== 审计配置 ====================
    - --audit-policy-file=/etc/kubernetes/audit/policy.yaml
    - --audit-log-path=/var/log/kubernetes/audit/audit.log
    - --audit-log-maxage=7
    - --audit-log-maxsize=100
    - --audit-log-maxbackup=10
    - --audit-log-format=json
    
    # ==================== API优先级和公平性 ====================
    - --enable-priority-and-fairness=true
    
    # ==================== 性能优化 ====================
    - --profiling=false
    - --goaway-chance=0
    - --delete-collection-workers=4
    - --enable-aggregator-routing=true
    
    # ==================== 安全配置 ====================
    - --anonymous-auth=false
    - --service-account-issuer=https://kubernetes.default.svc
    - --service-account-key-file=/etc/kubernetes/pki/sa.pub
    - --service-account-signing-key-file=/etc/kubernetes/pki/sa.key
    
    # ==================== 特性门控 ====================
    - --feature-gates=APIPriorityAndFairness=true,ServerSideApply=true
    
    resources:
      requests:
        cpu: 2000m
        memory: 4Gi
      limits:
        cpu: 4000m
        memory: 8Gi
        
    env:
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
          
    volumeMounts:
    - name: k8s-certs
      mountPath: /etc/kubernetes/pki
      readOnly: true
    - name: etcd-certs
      mountPath: /etc/kubernetes/pki/etcd
      readOnly: true
    - name: audit-config
      mountPath: /etc/kubernetes/audit
      readOnly: true
    - name: audit-logs
      mountPath: /var/log/kubernetes/audit
      
    livenessProbe:
      httpGet:
        path: /livez
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 10
      periodSeconds: 10
      timeoutSeconds: 15
      failureThreshold: 8
      
    readinessProbe:
      httpGet:
        path: /readyz
        port: 6443
        scheme: HTTPS
      periodSeconds: 1
      timeoutSeconds: 15
      
  volumes:
  - name: k8s-certs
    hostPath:
      path: /etc/kubernetes/pki
      type: DirectoryOrCreate
  - name: etcd-certs
    hostPath:
      path: /etc/kubernetes/pki/etcd
      type: DirectoryOrCreate
  - name: audit-config
    hostPath:
      path: /etc/kubernetes/audit
      type: DirectoryOrCreate
  - name: audit-logs
    hostPath:
      path: /var/log/kubernetes/audit
      type: DirectoryOrCreate
```

## API Priority and Fairness (APF) 配置

### APF架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     API Priority and Fairness 架构                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   请求 ──▶ FlowSchema匹配 ──▶ PriorityLevel分配 ──▶ 队列处理 ──▶ 执行      │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                     FlowSchema (请求分类)                            │  │
│   │  ┌─────────────────────────────────────────────────────────────┐   │  │
│   │  │ 匹配规则:                                                     │   │  │
│   │  │ • subjects: User/Group/ServiceAccount                        │   │  │
│   │  │ • resourceRules: apiGroups/resources/verbs/namespaces       │   │  │
│   │  │ • nonResourceRules: urls (如 /healthz)                       │   │  │
│   │  │ • matchingPrecedence: 优先级(数字越小越优先)                  │   │  │
│   │  └─────────────────────────────────────────────────────────────┘   │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                        │                                    │
│                                        ▼                                    │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                PriorityLevelConfiguration (优先级配置)               │  │
│   │  ┌─────────────────────────────────────────────────────────────┐   │  │
│   │  │ type: Limited                                                │   │  │
│   │  │ limited:                                                     │   │  │
│   │  │   nominalConcurrencyShares: 并发份额                         │   │  │
│   │  │   lendablePercent: 可借出百分比                              │   │  │
│   │  │   borrowingLimitPercent: 借入限制                            │   │  │
│   │  │   limitResponse:                                             │   │  │
│   │  │     type: Queue/Reject                                       │   │  │
│   │  │     queuing: queues/handSize/queueLengthLimit               │   │  │
│   │  └─────────────────────────────────────────────────────────────┘   │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                        │                                    │
│                                        ▼                                    │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                          队列处理                                    │  │
│   │  ┌─────────────────────────────────────────────────────────────┐   │  │
│   │  │ 公平队列算法(Shuffle Sharding + Fair Queuing)                │   │  │
│   │  │ • 每个请求根据flow计算hash                                   │   │  │
│   │  │ • hash决定进入哪些队列(handSize个)                           │   │  │
│   │  │ • 选择最短队列入队                                           │   │  │
│   │  │ • 虚拟时间调度保证公平性                                     │   │  │
│   │  └─────────────────────────────────────────────────────────────┘   │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### APF内置优先级

| 优先级 | 并发份额 | 用途 | 说明 |
|--------|----------|------|------|
| **exempt** | 无限制 | system:masters | 豁免限流 |
| **system** | 30 | 系统组件 | kube-system服务账户 |
| **leader-election** | 10 | 选举请求 | 控制器选举 |
| **workload-high** | 40 | 高优先工作负载 | 重要应用 |
| **workload-low** | 100 | 普通工作负载 | 一般应用 |
| **global-default** | 20 | 默认 | 未匹配请求 |
| **catch-all** | 5 | 兜底 | 最低优先级 |

### 自定义APF配置

```yaml
# apf-custom-config.yaml
# 高优先级配置
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: critical-workload
spec:
  type: Limited
  limited:
    # 并发份额(相对值,所有Limited类型份额总和决定实际并发)
    nominalConcurrencyShares: 200
    
    # 限流响应策略
    limitResponse:
      type: Queue                    # Queue或Reject
      queuing:
        queues: 128                  # 队列数量
        handSize: 8                  # 每个请求hash到的队列数
        queueLengthLimit: 100        # 每个队列最大长度
        
    # 借用配置(v1.29+)
    lendablePercent: 0               # 不允许借出
    borrowingLimitPercent: 25        # 最多借入25%

---
# FlowSchema - 匹配关键服务请求
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: critical-service-requests
spec:
  # 关联的优先级
  priorityLevelConfiguration:
    name: critical-workload
    
  # 匹配优先级(数字越小越优先)
  matchingPrecedence: 500
  
  # 区分方法
  distinguisherMethod:
    type: ByUser                     # ByUser或ByNamespace
    
  # 匹配规则
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: critical-app
        namespace: production
    - kind: ServiceAccount
      serviceAccount:
        name: payment-service
        namespace: production
    resourceRules:
    - verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
      apiGroups: ["", "apps", "batch"]
      resources: ["*"]
      namespaces: ["production"]
      clusterScope: false

---
# 低优先级配置 - 用于批处理任务
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: batch-workload
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 30
    limitResponse:
      type: Queue
      queuing:
        queues: 32
        handSize: 4
        queueLengthLimit: 50
    lendablePercent: 50              # 空闲时可借出50%
    borrowingLimitPercent: 0         # 不借入

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: batch-job-requests
spec:
  priorityLevelConfiguration:
    name: batch-workload
  matchingPrecedence: 9000
  distinguisherMethod:
    type: ByNamespace
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: batch-processor
        namespace: batch-jobs
    resourceRules:
    - verbs: ["*"]
      apiGroups: ["batch"]
      resources: ["jobs", "cronjobs"]
      namespaces: ["*"]
```

### APF监控和调试

```bash
#!/bin/bash
# apf-debug.sh - APF调试脚本

echo "====== APF状态检查 ======"

# 1. 查看所有PriorityLevel
echo "=== 1. PriorityLevel列表 ==="
kubectl get prioritylevelconfiguration

# 2. 查看所有FlowSchema
echo -e "\n=== 2. FlowSchema列表 ==="
kubectl get flowschema

# 3. 查看APF详细状态
echo -e "\n=== 3. APF队列状态 ==="
kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels 2>/dev/null | head -50

echo -e "\n=== 4. APF队列详情 ==="
kubectl get --raw /debug/api_priority_and_fairness/dump_queues 2>/dev/null | head -50

# 4. 查看APF相关指标
echo -e "\n=== 5. APF关键指标 ==="
kubectl get --raw /metrics 2>/dev/null | grep -E "apiserver_flowcontrol|priority_level" | head -30

# 5. 检查请求是否被限流
echo -e "\n=== 6. 限流请求统计 ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_flowcontrol_rejected_requests_total"

# 6. 检查队列等待时间
echo -e "\n=== 7. 队列等待时间 ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_flowcontrol_request_queue_length_after_enqueue"
```

## 审计策略配置

### 完整审计策略

```yaml
# /etc/kubernetes/audit/policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
# 省略审计的请求
omitStages:
- "RequestReceived"

rules:
# ==================== 不记录的请求(减少日志量) ====================
# kube-proxy的watch请求
- level: None
  users: ["system:kube-proxy"]
  verbs: ["watch"]
  resources:
  - group: ""
    resources: ["endpoints", "services", "services/status"]

# kubelet的node状态查询
- level: None
  users: ["kubelet", "system:node-*"]
  verbs: ["get"]
  resources:
  - group: ""
    resources: ["nodes", "nodes/status"]

# 节点获取自己的secret(用于拉取镜像)
- level: None
  userGroups: ["system:nodes"]
  verbs: ["get"]
  resources:
  - group: ""
    resources: ["secrets"]

# 控制器的leader选举
- level: None
  users:
  - system:kube-controller-manager
  - system:kube-scheduler
  verbs: ["get", "update"]
  namespaces: ["kube-system"]
  resources:
  - group: "coordination.k8s.io"
    resources: ["leases"]

# 健康检查端点
- level: None
  nonResourceURLs:
  - "/healthz*"
  - "/readyz*"
  - "/livez*"
  - "/version"

# ==================== Metadata级别(记录元数据) ====================
# 敏感资源只记录元数据
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets", "configmaps", "serviceaccounts/token"]
  omitStages:
  - "RequestReceived"

# ==================== Request级别(记录请求体) ====================
# 重要资源变更
- level: Request
  verbs: ["create", "update", "patch", "delete"]
  resources:
  - group: ""
    resources: ["pods", "services", "persistentvolumeclaims"]
  - group: "apps"
    resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  - group: "batch"
    resources: ["jobs", "cronjobs"]
  - group: "networking.k8s.io"
    resources: ["ingresses", "networkpolicies"]
  omitStages:
  - "RequestReceived"

# ==================== RequestResponse级别(完整记录) ====================
# 安全相关完整记录
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  - group: "authentication.k8s.io"
  - group: "authorization.k8s.io"
  omitStages:
  - "RequestReceived"

# 命名空间操作完整记录
- level: RequestResponse
  verbs: ["create", "delete"]
  resources:
  - group: ""
    resources: ["namespaces"]
  omitStages:
  - "RequestReceived"

# ==================== 默认规则 ====================
# 其他已知API的Metadata级别
- level: Metadata
  resources:
  - group: ""                        # core API
  - group: "admissionregistration.k8s.io"
  - group: "apiextensions.k8s.io"
  - group: "apiregistration.k8s.io"
  - group: "apps"
  - group: "autoscaling"
  - group: "batch"
  - group: "certificates.k8s.io"
  - group: "coordination.k8s.io"
  - group: "extensions"
  - group: "networking.k8s.io"
  - group: "policy"
  - group: "storage.k8s.io"
  omitStages:
  - "RequestReceived"

# 兜底: 所有其他请求记录元数据
- level: Metadata
  omitStages:
  - "RequestReceived"
```

### 审计日志分析

```bash
#!/bin/bash
# audit-log-analysis.sh - 审计日志分析脚本

AUDIT_LOG=${1:-"/var/log/kubernetes/audit/audit.log"}

echo "====== 审计日志分析 ======"
echo "日志文件: $AUDIT_LOG"
echo ""

# 1. 请求方法统计
echo "=== 1. 请求方法分布 ==="
cat "$AUDIT_LOG" | jq -r '.verb' | sort | uniq -c | sort -rn | head -10

# 2. 用户请求统计
echo -e "\n=== 2. Top10请求用户 ==="
cat "$AUDIT_LOG" | jq -r '.user.username' | sort | uniq -c | sort -rn | head -10

# 3. 资源类型统计
echo -e "\n=== 3. Top10请求资源 ==="
cat "$AUDIT_LOG" | jq -r '.objectRef.resource // "unknown"' | sort | uniq -c | sort -rn | head -10

# 4. 响应状态统计
echo -e "\n=== 4. 响应状态分布 ==="
cat "$AUDIT_LOG" | jq -r '.responseStatus.code // "unknown"' | sort | uniq -c | sort -rn

# 5. 错误请求
echo -e "\n=== 5. 最近错误请求 ==="
cat "$AUDIT_LOG" | jq -r 'select(.responseStatus.code >= 400) | "\(.requestReceivedTimestamp) \(.verb) \(.objectRef.resource)/\(.objectRef.name) -> \(.responseStatus.code)"' | tail -20

# 6. 删除操作
echo -e "\n=== 6. 最近删除操作 ==="
cat "$AUDIT_LOG" | jq -r 'select(.verb == "delete") | "\(.requestReceivedTimestamp) \(.user.username) deleted \(.objectRef.resource)/\(.objectRef.name)"' | tail -10

# 7. RBAC变更
echo -e "\n=== 7. RBAC变更 ==="
cat "$AUDIT_LOG" | jq -r 'select(.objectRef.apiGroup == "rbac.authorization.k8s.io") | "\(.requestReceivedTimestamp) \(.verb) \(.objectRef.resource)/\(.objectRef.name) by \(.user.username)"' | tail -10
```

## etcd分库配置

### 大规模集群etcd分离

```yaml
# etcd分库策略
# 主etcd: 存储大部分资源
# 事件etcd: 独立存储events(写入量大)
# Pod etcd: 可选,超大规模集群

# API Server配置
command:
- kube-apiserver
# 主etcd
- --etcd-servers=https://etcd-main-0:2379,https://etcd-main-1:2379,https://etcd-main-2:2379
# 事件独立etcd
- --etcd-servers-overrides=/events#https://etcd-events-0:2379,https://etcd-events-1:2379,https://etcd-events-2:2379
# 超大规模可进一步分离
# - --etcd-servers-overrides=/events#https://etcd-events:2379;/pods#https://etcd-pods:2379
```

### etcd分库架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          etcd 分库架构                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                       API Server                                     │  │
│   │                          │                                           │  │
│   │    ┌────────────────────┬┴────────────────────┐                     │  │
│   │    │                    │                      │                     │  │
│   │    ▼                    ▼                      ▼                     │  │
│   │ ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │  │
│   │ │  Main etcd   │  │ Events etcd  │  │  Pods etcd   │               │  │
│   │ │              │  │              │  │  (可选)      │               │  │
│   │ │ • pods       │  │ • events     │  │ • pods       │               │  │
│   │ │ • services   │  │              │  │              │               │  │
│   │ │ • secrets    │  │              │  │              │               │  │
│   │ │ • configmaps │  │              │  │              │               │  │
│   │ │ • deployments│  │              │  │              │               │  │
│   │ │ • 其他资源   │  │              │  │              │               │  │
│   │ └──────────────┘  └──────────────┘  └──────────────┘               │  │
│   │        │                  │                  │                      │  │
│   │        ▼                  ▼                  ▼                      │  │
│   │ ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │  │
│   │ │ 3-5节点集群  │  │ 3节点集群    │  │ 3-5节点集群  │               │  │
│   │ │ NVMe SSD     │  │ SSD          │  │ NVMe SSD     │               │  │
│   │ │ 高IOPS      │  │ 中等IOPS     │  │ 高IOPS      │               │  │
│   │ └──────────────┘  └──────────────┘  └──────────────┘               │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│   分库收益:                                                                 │
│   • Events写入不影响主集群性能                                              │
│   • 可独立扩展和维护                                                        │
│   • 故障隔离                                                                │
│   • 超大规模支持更多节点                                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

## API Server诊断命令

### 健康检查

```bash
#!/bin/bash
# apiserver-health-check.sh

echo "====== API Server健康检查 ======"

# 1. 基本健康
echo "=== 1. 基本健康状态 ==="
echo "healthz: $(kubectl get --raw /healthz 2>/dev/null || echo 'FAILED')"
echo "livez: $(kubectl get --raw /livez 2>/dev/null || echo 'FAILED')"
echo "readyz: $(kubectl get --raw /readyz 2>/dev/null || echo 'FAILED')"

# 2. 详细健康检查
echo -e "\n=== 2. 详细就绪检查 ==="
kubectl get --raw /readyz?verbose 2>/dev/null | grep -E "^\[|check" | head -30

# 3. etcd健康
echo -e "\n=== 3. etcd健康 ==="
kubectl get --raw /healthz/etcd 2>/dev/null || echo "etcd检查失败"

# 4. API版本
echo -e "\n=== 4. API版本 ==="
kubectl version --short 2>/dev/null || kubectl version -o json | jq -r '.serverVersion.gitVersion'
```

### 性能指标查询

```bash
#!/bin/bash
# apiserver-metrics.sh

echo "====== API Server性能指标 ======"

# 1. 请求延迟
echo "=== 1. 请求延迟(P99) ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_request_duration_seconds" | grep "_bucket" | head -10

# 2. 当前并发请求
echo -e "\n=== 2. 当前并发请求 ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_current_inflight_requests"

# 3. 请求总数
echo -e "\n=== 3. 请求总数(Top10 verb/resource) ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_request_total" | sort -t'=' -k3 -rn | head -10

# 4. Watch连接数
echo -e "\n=== 4. Watch事件 ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_watch_events_total" | head -10

# 5. etcd请求延迟
echo -e "\n=== 5. etcd请求延迟 ==="
kubectl get --raw /metrics 2>/dev/null | grep "etcd_request_duration_seconds" | head -10

# 6. 存储对象数
echo -e "\n=== 6. 存储对象数(Top10) ==="
kubectl get --raw /metrics 2>/dev/null | grep "apiserver_storage_objects" | sort -t'=' -k2 -rn | head -10

# 7. 认证延迟
echo -e "\n=== 7. 认证延迟 ==="
kubectl get --raw /metrics 2>/dev/null | grep "authentication_duration_seconds" | head -5

# 8. 授权延迟
echo -e "\n=== 8. 授权延迟 ==="
kubectl get --raw /metrics 2>/dev/null | grep "authorization_duration_seconds" | head -5
```

## 性能监控告警

### Prometheus告警规则

```yaml
# apiserver-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: apiserver-alerts
  namespace: monitoring
spec:
  groups:
  - name: apiserver.availability
    interval: 30s
    rules:
    # API Server不可用
    - alert: APIServerDown
      expr: up{job="kubernetes-apiservers"} == 0
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "API Server不可用"
        
    # API Server错误率高
    - alert: APIServerErrorRateHigh
      expr: |
        sum(rate(apiserver_request_total{code=~"5.."}[5m])) 
        / sum(rate(apiserver_request_total[5m])) > 0.01
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "API Server错误率超过1%"
        description: "当前错误率: {{ $value | humanizePercentage }}"

  - name: apiserver.latency
    rules:
    # 请求延迟高
    - alert: APIServerLatencyHigh
      expr: |
        histogram_quantile(0.99, 
          sum(rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m])) 
          by (verb, resource, le)
        ) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "API Server {{ $labels.verb }} {{ $labels.resource }} P99延迟>1s"
        description: "当前P99延迟: {{ $value | humanizeDuration }}"
        
    # etcd延迟高
    - alert: EtcdRequestLatencyHigh
      expr: |
        histogram_quantile(0.99, 
          sum(rate(etcd_request_duration_seconds_bucket[5m])) 
          by (operation, le)
        ) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "etcd {{ $labels.operation }} 请求P99延迟>100ms"

  - name: apiserver.capacity
    rules:
    # 并发请求接近限制
    - alert: APIServerTooManyRequests
      expr: |
        sum(apiserver_current_inflight_requests{request_kind="readOnly"}) 
        / scalar(max(apiserver_current_inflight_requests{request_kind="readOnly"}) + 1) > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "API Server并发请求接近限制"
        
    # 请求被限流
    - alert: APIServerRequestsThrottled
      expr: sum(rate(apiserver_dropped_requests_total[5m])) > 0
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "API Server请求被限流"
        
    # Watch连接数过多
    - alert: APIServerTooManyWatches
      expr: sum(apiserver_registered_watchers) > 50000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "API Server Watch连接数过多"
        description: "当前Watch连接数: {{ $value }}"

  - name: apiserver.storage
    rules:
    # etcd存储对象过多
    - alert: APIServerStorageObjectsHigh
      expr: sum(apiserver_storage_objects) > 100000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "API Server存储对象数过多"
        description: "当前对象数: {{ $value }}"
```

### Grafana Dashboard查询

```json
{
  "panels": [
    {
      "title": "API请求延迟(P99)",
      "targets": [
        {
          "expr": "histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb!=\"WATCH\"}[5m])) by (verb, le))",
          "legendFormat": "{{ verb }}"
        }
      ]
    },
    {
      "title": "API请求QPS",
      "targets": [
        {
          "expr": "sum(rate(apiserver_request_total[5m])) by (verb)",
          "legendFormat": "{{ verb }}"
        }
      ]
    },
    {
      "title": "并发请求数",
      "targets": [
        {
          "expr": "sum(apiserver_current_inflight_requests) by (request_kind)",
          "legendFormat": "{{ request_kind }}"
        }
      ]
    },
    {
      "title": "etcd请求延迟",
      "targets": [
        {
          "expr": "histogram_quantile(0.99, sum(rate(etcd_request_duration_seconds_bucket[5m])) by (operation, le))",
          "legendFormat": "{{ operation }}"
        }
      ]
    },
    {
      "title": "APF队列长度",
      "targets": [
        {
          "expr": "sum(apiserver_flowcontrol_request_queue_length_after_enqueue) by (priority_level)",
          "legendFormat": "{{ priority_level }}"
        }
      ]
    }
  ]
}
```

## 常见问题与解决方案

| 问题 | 现象 | 原因 | 解决方案 |
|------|------|------|----------|
| **请求超时** | 504/408错误增多 | 并发过高/后端慢 | 增加并发限制/优化etcd |
| **请求被拒** | 429 Too Many Requests | APF限流 | 调整APF配置/增加优先级 |
| **Watch断开** | Watch频繁重连 | 缓存不足/网络问题 | 增加watch-cache-sizes |
| **审计日志大** | 磁盘空间不足 | 审计级别过高 | 调整审计策略/增加清理 |
| **etcd慢** | 请求延迟高 | 磁盘IO/数据量大 | etcd分库/升级存储 |
| **OOM** | API Server重启 | 内存不足 | 增加内存限制/优化缓存 |
| **证书问题** | TLS握手失败 | 证书过期/不匹配 | 更新证书 |
| **选举抖动** | Leader频繁切换 | 网络延迟/负载高 | 优化网络/增加超时 |

## ACK API Server配置

```bash
# 查看托管API Server状态
kubectl get cs

# ACK Pro版特有功能
# 1. 控制台配置API Server参数
# 集群信息 -> 集群配置 -> API Server参数

# 2. 审计日志配置
# 运维管理 -> 审计日志 -> 开启

# 3. API Server访问控制
# 集群信息 -> 连接信息 -> 公网/私网访问

# 4. 查看API Server事件
kubectl get events -n kube-system --field-selector source=kube-apiserver
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| v1.25 | APF成为稳定版 | 默认启用 |
| v1.26 | Watch缓存优化 | 性能提升 |
| v1.27 | 审计日志增强 | 更多字段 |
| v1.28 | etcd优化 | 大集群支持 |
| v1.29 | APF借用机制GA | 更灵活限流 |
| v1.30 | 请求优先级增强 | 更细粒度控制 |

---

**API Server调优原则**: 监控先行 → 定位瓶颈 → 针对性调优 → 验证效果 → 持续优化

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
