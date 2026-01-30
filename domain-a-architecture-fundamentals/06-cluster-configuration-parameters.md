# 06 - 集群配置参数完全参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/reference/command-line-tools-reference](https://kubernetes.io/docs/reference/command-line-tools-reference/)

---

## 目录

- [kube-apiserver 参数](#kube-apiserver-参数)
- [etcd 参数](#etcd-参数)
- [kube-scheduler 参数](#kube-scheduler-参数)
- [kube-controller-manager 参数](#kube-controller-manager-参数)
- [kubelet 参数](#kubelet-参数)
- [kube-proxy 参数](#kube-proxy-参数)
- [Feature Gates 完整参考](#feature-gates-完整参考)
- [生产配置示例](#生产配置示例)
- [云厂商特定配置](#云厂商特定配置)

---

## kube-apiserver 参数

### 1.1 核心网络与存储参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--etcd-servers` | 无 | `https://etcd1:2379,https://etcd2:2379,https://etcd3:2379` | 稳定 | etcd端点列表，逗号分隔 | **必须配置**；生产至少3节点 |
| `--etcd-cafile` | 无 | `/etc/kubernetes/pki/etcd/ca.crt` | 稳定 | etcd CA证书 | **安全必需**；验证etcd服务端 |
| `--etcd-certfile` | 无 | `/etc/kubernetes/pki/apiserver-etcd-client.crt` | 稳定 | etcd客户端证书 | **安全必需**；mTLS认证 |
| `--etcd-keyfile` | 无 | `/etc/kubernetes/pki/apiserver-etcd-client.key` | 稳定 | etcd客户端私钥 | **安全必需** |
| `--etcd-compaction-interval` | 5m | 5m | 稳定 | etcd压缩间隔 | 减少存储碎片 |
| `--etcd-count-metric-poll-period` | 0 | 1m | 稳定 | etcd对象计数指标周期 | 监控etcd存储 |
| `--storage-backend` | etcd3 | etcd3 | 稳定 | 存储后端类型 | 仅支持etcd3 |
| `--storage-media-type` | application/vnd.kubernetes.protobuf | protobuf | 稳定 | 存储序列化格式 | protobuf性能更优 |

### 1.2 Service与网络配置

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--service-cluster-ip-range` | 10.0.0.0/24 | `10.96.0.0/12` (1M IP) | 稳定 | Service ClusterIP CIDR | **不可变更**；规划足够空间 |
| `--service-node-port-range` | 30000-32767 | 30000-32767 或扩展 | 稳定 | NodePort范围 | 避免与主机端口冲突 |
| `--kubernetes-service-node-port` | 0 | 0 | 稳定 | kubernetes服务NodePort | 0表示不暴露 |
| `--bind-address` | 0.0.0.0 | 0.0.0.0 或特定IP | 稳定 | API监听地址 | 多网卡时指定管理网 |
| `--secure-port` | 6443 | 6443 | 稳定 | HTTPS端口 | 标准端口，LB健康检查 |
| `--advertise-address` | 首个非localhost IP | 明确指定 | 稳定 | 对外广播地址 | 多网卡时**必须指定** |

### 1.3 认证参数 (Authentication)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--client-ca-file` | 无 | `/etc/kubernetes/pki/ca.crt` | 稳定 | 客户端CA证书 | **安全必需**；验证客户端证书 |
| `--tls-cert-file` | 无 | `/etc/kubernetes/pki/apiserver.crt` | 稳定 | API Server证书 | **安全必需** |
| `--tls-private-key-file` | 无 | `/etc/kubernetes/pki/apiserver.key` | 稳定 | API Server私钥 | **安全必需** |
| `--tls-cipher-suites` | 默认 | TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,... | 稳定 | TLS加密套件 | 禁用弱加密算法 |
| `--tls-min-version` | VersionTLS12 | VersionTLS12 | 稳定 | 最低TLS版本 | 禁用TLS1.0/1.1 |
| `--anonymous-auth` | true | **false** | 稳定 | 允许匿名请求 | **生产必须禁用** |
| `--enable-bootstrap-token-auth` | false | true | 稳定 | 启用Bootstrap Token | 节点自动加入集群 |
| `--service-account-issuer` | 无 | `https://kubernetes.default.svc` | v1.21+ GA | SA Token颁发者 | **必须配置**；OIDC验证 |
| `--service-account-key-file` | 无 | `/etc/kubernetes/pki/sa.pub` | 稳定 | SA验证公钥 | 验证SA Token签名 |
| `--service-account-signing-key-file` | 无 | `/etc/kubernetes/pki/sa.key` | 稳定 | SA签名私钥 | 签发SA Token |
| `--service-account-extend-token-expiration` | true | true | 稳定 | 延长Token有效期 | 兼容旧Token |
| `--oidc-issuer-url` | 无 | OIDC提供商URL | 稳定 | OIDC颁发者 | 企业SSO集成 |
| `--oidc-client-id` | 无 | OAuth客户端ID | 稳定 | OIDC客户端 | 配合OIDC使用 |
| `--oidc-username-claim` | sub | email 或 preferred_username | 稳定 | 用户名声明 | 映射OIDC用户 |
| `--oidc-groups-claim` | 无 | groups | 稳定 | 组声明 | 映射OIDC组 |
| `--authentication-token-webhook-config-file` | 无 | 配置文件路径 | 稳定 | Webhook Token认证 | 自定义认证后端 |

### 1.4 授权参数 (Authorization)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--authorization-mode` | AlwaysAllow | **Node,RBAC** | 稳定 | 授权模式链 | **生产必须**；顺序很重要 |
| `--authorization-webhook-config-file` | 无 | 配置文件路径 | 稳定 | Webhook授权配置 | 自定义授权后端 |
| `--authorization-webhook-cache-authorized-ttl` | 5m | 5m | 稳定 | 授权缓存TTL | 减少Webhook调用 |
| `--authorization-webhook-cache-unauthorized-ttl` | 30s | 30s | 稳定 | 拒绝缓存TTL | 平衡安全与性能 |

### 1.5 准入控制参数 (Admission)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--enable-admission-plugins` | 默认列表 | 见下文完整列表 | v1.30 CEL GA | 启用的准入插件 | 安全策略强制执行 |
| `--disable-admission-plugins` | 无 | 按需禁用 | 稳定 | 禁用的准入插件 | 谨慎禁用安全插件 |
| `--admission-control-config-file` | 无 | 配置文件路径 | 稳定 | 准入插件配置 | 精细化控制 |

**v1.32 推荐准入插件列表**:

```bash
--enable-admission-plugins=\
NodeRestriction,\
PodSecurity,\
LimitRanger,\
ServiceAccount,\
DefaultStorageClass,\
DefaultTolerationSeconds,\
MutatingAdmissionWebhook,\
ValidatingAdmissionWebhook,\
ValidatingAdmissionPolicy,\
ResourceQuota,\
Priority,\
RuntimeClass
```

| 准入插件 | 类型 | 默认启用 | 版本 | 作用 |
|:---|:---|:---|:---|:---|
| `NodeRestriction` | Validating | 是 | 稳定 | 限制kubelet只能修改自身节点和Pod |
| `PodSecurity` | Validating | 是 | v1.25 GA | Pod安全标准强制执行 |
| `LimitRanger` | Mutating+Validating | 是 | 稳定 | 默认资源限制与验证 |
| `ServiceAccount` | Mutating | 是 | 稳定 | 自动挂载SA Token |
| `DefaultStorageClass` | Mutating | 是 | 稳定 | 默认StorageClass |
| `MutatingAdmissionWebhook` | Mutating | 是 | 稳定 | 动态Webhook修改 |
| `ValidatingAdmissionWebhook` | Validating | 是 | 稳定 | 动态Webhook验证 |
| `ValidatingAdmissionPolicy` | Validating | 否 | v1.30 GA | CEL原生策略验证 |
| `ResourceQuota` | Validating | 是 | 稳定 | 资源配额强制 |
| `Priority` | Mutating | 是 | 稳定 | Pod优先级注入 |
| `RuntimeClass` | Mutating | 是 | 稳定 | 运行时类资源注入 |
| `CertificateApproval` | Validating | 是 | 稳定 | CSR审批权限控制 |
| `CertificateSigning` | Validating | 是 | 稳定 | CSR签名权限控制 |
| `CertificateSubjectRestriction` | Validating | 是 | 稳定 | 限制证书主体 |

### 1.6 API优先级与公平性 (APF)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--enable-priority-and-fairness` | true | true | v1.29 GA | 启用APF限流 | 替代max-requests-inflight |
| `--max-requests-inflight` | 400 | 800-1600 | 稳定 | 非APF时最大并发读请求 | APF启用时作为上限 |
| `--max-mutating-requests-inflight` | 200 | 400-800 | 稳定 | 非APF时最大并发写请求 | APF启用时作为上限 |

**APF配置示例** (FlowSchema + PriorityLevelConfiguration):

```yaml
# PriorityLevelConfiguration - 定义优先级队列
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: workload-high
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 100      # 相对并发份额
    lendablePercent: 50                 # 可借出百分比
    limitResponse:
      type: Queue
      queuing:
        queues: 64                      # 队列数量
        handSize: 8                     # 每请求随机选择队列数
        queueLengthLimit: 100           # 队列长度限制
---
# FlowSchema - 定义请求分类规则
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: workload-controller-high
spec:
  priorityLevelConfiguration:
    name: workload-high
  matchingPrecedence: 500              # 匹配优先级，数字越小越优先
  distinguisherMethod:
    type: ByUser                       # 按用户区分请求
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: deployment-controller
        namespace: kube-system
    resourceRules:
    - apiGroups: ["apps"]
      resources: ["deployments", "replicasets"]
      verbs: ["*"]
```

### 1.7 审计日志参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--audit-log-path` | 无 | `/var/log/kubernetes/audit.log` | 稳定 | 审计日志路径 | **生产必须启用** |
| `--audit-log-maxage` | 0 | 30 | 稳定 | 日志保留天数 | 合规要求 |
| `--audit-log-maxbackup` | 0 | 10 | 稳定 | 日志备份数量 | 存储管理 |
| `--audit-log-maxsize` | 0 | 100 | 稳定 | 单文件大小MB | 防止磁盘满 |
| `--audit-log-compress` | false | true | 稳定 | 压缩轮转日志 | 节省存储 |
| `--audit-policy-file` | 无 | `/etc/kubernetes/audit-policy.yaml` | 稳定 | 审计策略文件 | **必须配置** |
| `--audit-log-format` | json | json | 稳定 | 日志格式 | json便于解析 |
| `--audit-log-batch-max-size` | 1 | 100 | 稳定 | 批量写入大小 | 减少IO |
| `--audit-log-batch-max-wait` | 0 | 1s | 稳定 | 批量等待时间 | 平衡延迟与吞吐 |
| `--audit-log-truncate-enabled` | false | true | 稳定 | 截断大型响应 | 防止日志过大 |
| `--audit-log-truncate-max-event-size` | 102400 | 102400 | 稳定 | 事件最大字节 | 100KB |
| `--audit-log-truncate-max-batch-size` | 10485760 | 10485760 | 稳定 | 批次最大字节 | 10MB |
| `--audit-webhook-config-file` | 无 | Webhook配置路径 | 稳定 | 审计Webhook | 实时发送到外部系统 |
| `--audit-webhook-batch-max-size` | 400 | 400 | 稳定 | Webhook批次大小 | Webhook吞吐控制 |

**生产审计策略示例**:

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
# 不审计的请求
omitStages:
  - "RequestReceived"
rules:
  # 不审计健康检查
  - level: None
    users: ["system:kube-probe"]
    
  # 不审计只读端点
  - level: None
    nonResourceURLs:
      - "/healthz*"
      - "/livez*"
      - "/readyz*"
      - "/metrics"
      - "/version"
      
  # 不审计kubelet和node的自我更新
  - level: None
    users: ["system:node:*", "system:serviceaccount:kube-system:*"]
    verbs: ["get", "list", "watch"]
    
  # Secret访问记录Metadata（不记录内容）
  - level: Metadata
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
        
  # 认证相关记录RequestResponse
  - level: RequestResponse
    resources:
      - group: "authentication.k8s.io"
      - group: "authorization.k8s.io"
        
  # Pod执行/端口转发记录Request
  - level: Request
    resources:
      - group: ""
        resources: ["pods/exec", "pods/portforward", "pods/attach", "pods/log"]
        
  # 其他写操作记录Request
  - level: Request
    verbs: ["create", "update", "patch", "delete", "deletecollection"]
    
  # 默认记录Metadata
  - level: Metadata
```

### 1.8 安全与加密参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--encryption-provider-config` | 无 | 配置文件路径 | 稳定 | etcd静态加密配置 | **敏感数据必须加密** |
| `--encryption-provider-config-automatic-reload` | false | true | v1.29+ | 自动重载加密配置 | 密钥轮换无需重启 |
| `--profiling` | true | **false** | 稳定 | 启用pprof端点 | **生产必须禁用** |
| `--contention-profiling` | false | false | 稳定 | 启用锁竞争分析 | 调试用，生产禁用 |
| `--goaway-chance` | 0 | 0 | 稳定 | HTTP/2 GOAWAY概率 | 客户端重连，LB均衡 |
| `--request-timeout` | 60s | 60s | 稳定 | 请求超时 | 复杂操作可延长 |
| `--min-request-timeout` | 1800 | 1800 | 稳定 | 最小超时（watch） | 长连接支持 |
| `--shutdown-delay-duration` | 0s | 30s | 稳定 | 关闭延迟 | 优雅关闭，等待LB移除 |
| `--shutdown-send-retry-after` | false | true | 稳定 | 关闭时返回Retry-After | 通知客户端重试 |
| `--strict-transport-security-directives` | 无 | max-age=31536000 | 稳定 | HSTS头 | 强制HTTPS |
| `--cors-allowed-origins` | 无 | 按需配置 | 稳定 | CORS允许源 | 仅API网关使用时配置 |

**etcd加密配置示例**:

```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
      - secrets
      - configmaps
    providers:
      # AES-GCM加密（推荐）
      - aescbc:
          keys:
            - name: key1
              secret: <base64-encoded-32-byte-key>
      # KMS v2 加密（企业推荐，支持AWS KMS/Vault）
      # - kms:
      #     apiVersion: v2
      #     name: aws-kms
      #     endpoint: unix:///var/run/kmsplugin/socket.sock
      #     timeout: 3s
      # identity作为fallback，用于读取未加密数据
      - identity: {}
```

### 1.9 性能与缓存参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--watch-cache` | true | true | 稳定 | 启用Watch缓存 | **必须启用** |
| `--watch-cache-sizes` | 自动 | 按需调整 | v1.28优化 | 每类资源缓存大小 | 大集群优化 |
| `--default-watch-cache-size` | 100 | 100 | 稳定 | 默认缓存大小 | 未指定资源使用 |
| `--event-ttl` | 1h | 1h | 稳定 | Event保留时间 | 减少etcd存储 |
| `--delete-collection-workers` | 1 | 2-4 | 稳定 | 批量删除并发数 | 加速大规模删除 |
| `--enable-garbage-collector` | true | true | 稳定 | 启用级联删除GC | 自动清理依赖资源 |
| `--http2-max-streams-per-connection` | 1000 | 1000 | 稳定 | HTTP/2流数上限 | 连接复用 |

**Watch缓存大小配置示例**:

```bash
# 针对大集群优化特定资源的缓存
--watch-cache-sizes=\
pods#5000,\
nodes#1000,\
services#1000,\
endpoints#2000,\
configmaps#1000,\
secrets#1000,\
deployments.apps#1000,\
replicasets.apps#2000
```

---

## etcd 参数

> etcd版本: v3.5.x (K8s v1.25-v1.32推荐)

### 2.1 集群配置参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--name` | default | `etcd-0`, `etcd-1`, `etcd-2` | 稳定 | 节点名称，集群唯一 | 必须与initial-cluster对应 |
| `--data-dir` | `${name}.etcd` | `/var/lib/etcd` | 稳定 | 数据目录 | **必须使用SSD**；独占磁盘 |
| `--wal-dir` | 无 | `/var/lib/etcd/wal` (可选独立) | 稳定 | WAL目录 | 高IOPS场景单独SSD |
| `--initial-cluster` | 无 | `etcd-0=https://10.0.1.1:2380,...` | 稳定 | 初始集群成员 | 首次启动必须配置 |
| `--initial-cluster-state` | new | new/existing | 稳定 | 集群初始状态 | 新建用new，加入用existing |
| `--initial-cluster-token` | etcd-cluster | `production-etcd-cluster` | 稳定 | 集群Token | 区分不同集群 |
| `--initial-advertise-peer-urls` | 无 | `https://10.0.1.1:2380` | 稳定 | 对外广播的Peer URL | 其他节点连接地址 |

### 2.2 网络监听参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--listen-peer-urls` | `http://localhost:2380` | `https://0.0.0.0:2380` | 稳定 | Peer监听地址 | 集群内部通信 |
| `--listen-client-urls` | `http://localhost:2379` | `https://0.0.0.0:2379` | 稳定 | Client监听地址 | API Server连接 |
| `--advertise-client-urls` | 无 | `https://10.0.1.1:2379` | 稳定 | 对外广播Client URL | API Server配置使用 |
| `--listen-metrics-urls` | 无 | `http://0.0.0.0:2381` | 稳定 | Metrics端点 | Prometheus采集 |
| `--enable-grpc-gateway` | true | true | 稳定 | 启用gRPC网关 | 便于调试和监控 |

### 2.3 安全与TLS参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--cert-file` | 无 | `/etc/etcd/pki/server.crt` | 稳定 | Server证书 | **生产必须**；Client连接加密 |
| `--key-file` | 无 | `/etc/etcd/pki/server.key` | 稳定 | Server私钥 | **生产必须** |
| `--client-cert-auth` | false | **true** | 稳定 | 强制Client证书认证 | **生产必须启用** |
| `--trusted-ca-file` | 无 | `/etc/etcd/pki/ca.crt` | 稳定 | 客户端CA证书 | 验证Client证书 |
| `--peer-cert-file` | 无 | `/etc/etcd/pki/peer.crt` | 稳定 | Peer证书 | **生产必须**；集群通信加密 |
| `--peer-key-file` | 无 | `/etc/etcd/pki/peer.key` | 稳定 | Peer私钥 | **生产必须** |
| `--peer-client-cert-auth` | false | **true** | 稳定 | 强制Peer证书认证 | **生产必须启用** |
| `--peer-trusted-ca-file` | 无 | `/etc/etcd/pki/ca.crt` | 稳定 | Peer CA证书 | 验证Peer证书 |
| `--cipher-suites` | 默认 | TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,... | v3.4+ | TLS加密套件 | 禁用弱加密 |
| `--auto-tls` | false | **false** | 稳定 | 自动生成TLS | **生产禁止**；仅测试用 |
| `--peer-auto-tls` | false | **false** | 稳定 | 自动生成Peer TLS | **生产禁止** |

### 2.4 性能调优参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--quota-backend-bytes` | 2GB | **8GB** (最大建议) | 稳定 | 存储配额 | 防止无限增长；超过触发alarm |
| `--max-txn-ops` | 128 | 256-512 | 稳定 | 单事务最大操作数 | 大批量写入需增加 |
| `--max-request-bytes` | 1.5MB | **10MB** | 稳定 | 单请求最大字节 | 大ConfigMap/Secret需要 |
| `--grpc-keepalive-min-time` | 5s | 5s | 稳定 | gRPC保活最小间隔 | 防止过频连接 |
| `--grpc-keepalive-interval` | 2h | 2h | 稳定 | gRPC保活间隔 | 长连接维持 |
| `--grpc-keepalive-timeout` | 20s | 20s | 稳定 | gRPC保活超时 | 死连接检测 |

### 2.5 Raft共识参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--heartbeat-interval` | 100ms | 100-200ms | 稳定 | Leader心跳间隔 | 跨机房延迟高时增加 |
| `--election-timeout` | 1000ms | 1000-5000ms | 稳定 | 选举超时 | **必须 >= 5x心跳** |
| `--snapshot-count` | 100000 | **10000** | 稳定 | 快照触发事务数 | 小值=更频繁快照=快恢复 |
| `--max-snapshots` | 5 | 5 | 稳定 | 保留快照数 | 历史恢复点 |
| `--max-wals` | 5 | 5 | 稳定 | 保留WAL数 | 历史恢复点 |

**Raft参数关系说明**:
```
election-timeout >= 5 × heartbeat-interval
跨机房场景: heartbeat=200ms, election=2000ms
同机房场景: heartbeat=100ms, election=1000ms
```

### 2.6 压缩与碎片整理参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--auto-compaction-mode` | periodic | **revision** | 稳定 | 压缩模式 | revision按版本更精确 |
| `--auto-compaction-retention` | 0 | **1000** (revision) 或 **1h** (periodic) | 稳定 | 压缩保留 | 减少存储增长 |

**压缩模式说明**:

| 模式 | retention值 | 效果 | 适用场景 |
|:---|:---|:---|:---|
| `periodic` | 1h | 保留最近1小时历史 | 简单场景，按时间清理 |
| `revision` | 1000 | 保留最近1000个版本 | **推荐**；精确控制历史深度 |

### 2.7 备份与恢复

**自动备份脚本** (生产必备):

```bash
#!/bin/bash
# etcd-backup.sh - 定时备份脚本
set -e

BACKUP_DIR="/backup/etcd"
ENDPOINTS="https://127.0.0.1:2379"
CACERT="/etc/etcd/pki/ca.crt"
CERT="/etc/etcd/pki/server.crt"
KEY="/etc/etcd/pki/server.key"
RETENTION_DAYS=7

# 创建备份
BACKUP_FILE="${BACKUP_DIR}/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db"
etcdctl snapshot save "${BACKUP_FILE}" \
  --endpoints="${ENDPOINTS}" \
  --cacert="${CACERT}" \
  --cert="${CERT}" \
  --key="${KEY}"

# 验证备份
etcdctl snapshot status "${BACKUP_FILE}" --write-out=table

# 清理旧备份
find "${BACKUP_DIR}" -name "etcd-snapshot-*.db" -mtime +${RETENTION_DAYS} -delete

echo "Backup completed: ${BACKUP_FILE}"
```

**恢复流程**:

```bash
# 1. 停止所有etcd节点
systemctl stop etcd

# 2. 清理旧数据目录 (所有节点)
rm -rf /var/lib/etcd/*

# 3. 恢复快照 (每个节点执行，注意参数不同)
# 节点1
etcdctl snapshot restore /backup/etcd-snapshot.db \
  --name etcd-0 \
  --initial-cluster "etcd-0=https://10.0.1.1:2380,etcd-1=https://10.0.1.2:2380,etcd-2=https://10.0.1.3:2380" \
  --initial-cluster-token production-etcd-cluster \
  --initial-advertise-peer-urls https://10.0.1.1:2380 \
  --data-dir /var/lib/etcd

# 4. 启动etcd (所有节点)
systemctl start etcd

# 5. 验证集群状态
etcdctl endpoint health --cluster
etcdctl member list
```

### 2.8 监控指标

| 指标 | 告警阈值 | 说明 |
|:---|:---|:---|
| `etcd_server_has_leader` | = 0 | 无Leader，集群不可用 |
| `etcd_server_leader_changes_seen_total` | > 3/hour | Leader频繁切换 |
| `etcd_mvcc_db_total_size_in_bytes` | > 6GB (quota 8GB) | 接近存储配额 |
| `etcd_disk_wal_fsync_duration_seconds` | P99 > 10ms | 磁盘IO性能差 |
| `etcd_disk_backend_commit_duration_seconds` | P99 > 25ms | 后端提交慢 |
| `etcd_network_peer_round_trip_time_seconds` | P99 > 50ms | 网络延迟高 |
| `etcd_server_proposals_failed_total` | 持续增长 | 提案失败 |
| `etcd_server_proposals_pending` | > 5 | 积压提案 |

**Prometheus告警规则示例**:

```yaml
groups:
- name: etcd
  rules:
  - alert: EtcdNoLeader
    expr: etcd_server_has_leader == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "etcd cluster has no leader"
      
  - alert: EtcdHighDiskLatency
    expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.01
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd WAL fsync latency is high"
      
  - alert: EtcdDatabaseSizeWarning
    expr: etcd_mvcc_db_total_size_in_bytes > 6442450944  # 6GB
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd database size exceeds 6GB"
      
  - alert: EtcdFrequentLeaderChanges
    expr: increase(etcd_server_leader_changes_seen_total[1h]) > 3
    labels:
      severity: warning
    annotations:
      summary: "etcd leader changed more than 3 times in 1 hour"
```

### 2.9 生产集群配置示例

```yaml
# /etc/etcd/etcd.conf.yaml (etcd v3.5+)
name: etcd-0
data-dir: /var/lib/etcd
wal-dir: /var/lib/etcd/wal

# 集群配置
initial-cluster: "etcd-0=https://10.0.1.1:2380,etcd-1=https://10.0.1.2:2380,etcd-2=https://10.0.1.3:2380"
initial-cluster-state: new
initial-cluster-token: production-etcd-cluster
initial-advertise-peer-urls: https://10.0.1.1:2380

# 监听地址
listen-peer-urls: https://0.0.0.0:2380
listen-client-urls: https://0.0.0.0:2379
advertise-client-urls: https://10.0.1.1:2379
listen-metrics-urls: http://0.0.0.0:2381

# TLS配置
client-transport-security:
  cert-file: /etc/etcd/pki/server.crt
  key-file: /etc/etcd/pki/server.key
  client-cert-auth: true
  trusted-ca-file: /etc/etcd/pki/ca.crt

peer-transport-security:
  cert-file: /etc/etcd/pki/peer.crt
  key-file: /etc/etcd/pki/peer.key
  client-cert-auth: true
  trusted-ca-file: /etc/etcd/pki/ca.crt

# 性能调优
quota-backend-bytes: 8589934592  # 8GB
max-txn-ops: 512
max-request-bytes: 10485760      # 10MB
snapshot-count: 10000

# Raft参数
heartbeat-interval: 100
election-timeout: 1000

# 压缩配置
auto-compaction-mode: revision
auto-compaction-retention: "1000"

# 日志
log-level: info
logger: zap
log-outputs: [stderr]
```

---

## kube-scheduler 参数

### 3.1 基础配置参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--config` | 无 | `/etc/kubernetes/scheduler-config.yaml` | v1.25框架GA | 调度配置文件 | **推荐**；替代命令行参数 |
| `--authentication-kubeconfig` | 无 | kubeconfig路径 | 稳定 | 认证配置 | webhook认证用 |
| `--authorization-kubeconfig` | 无 | kubeconfig路径 | 稳定 | 授权配置 | webhook授权用 |
| `--bind-address` | 0.0.0.0 | 127.0.0.1 | 稳定 | 监听地址 | 限制访问 |
| `--secure-port` | 10259 | 10259 | 稳定 | HTTPS端口 | 指标和健康检查 |
| `--profiling` | true | **false** | 稳定 | 启用pprof | **生产禁用** |
| `--contention-profiling` | false | false | 稳定 | 锁竞争分析 | 调试用 |

### 3.2 Leader选举参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--leader-elect` | true | true | 稳定 | 启用选举 | **HA必须** |
| `--leader-elect-lease-duration` | 15s | 15s | 稳定 | 租约时长 | 影响故障切换时间 |
| `--leader-elect-renew-deadline` | 10s | 10s | 稳定 | 续租期限 | 必须 < lease-duration |
| `--leader-elect-retry-period` | 2s | 2s | 稳定 | 重试间隔 | 影响选举速度 |
| `--leader-elect-resource-lock` | leases | leases | 稳定 | 锁资源类型 | leases最高效 |
| `--leader-elect-resource-name` | kube-scheduler | kube-scheduler | 稳定 | 锁资源名 | 多scheduler时区分 |
| `--leader-elect-resource-namespace` | kube-system | kube-system | 稳定 | 锁命名空间 | 通常不变 |

### 3.3 API通信参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--kube-api-qps` | 50 | **100-200** | 稳定 | API请求QPS | 大集群需增加 |
| `--kube-api-burst` | 100 | **200-400** | 稳定 | API请求突发 | 大集群需增加 |

### 3.4 调度框架配置 (KubeSchedulerConfiguration)

```yaml
# /etc/kubernetes/scheduler-config.yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
clientConnection:
  kubeconfig: /etc/kubernetes/scheduler.conf
  qps: 100
  burst: 200

leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
  resourceLock: leases
  resourceName: kube-scheduler
  resourceNamespace: kube-system

profiles:
- schedulerName: default-scheduler
  # 调度插件配置
  plugins:
    # 预过滤阶段
    preFilter:
      enabled:
      - name: NodeResourcesFit
      - name: NodePorts
      - name: PodTopologySpread
      - name: InterPodAffinity
      - name: VolumeBinding
      - name: NodeAffinity
    # 过滤阶段
    filter:
      enabled:
      - name: NodeUnschedulable
      - name: NodeName
      - name: TaintToleration
      - name: NodeAffinity
      - name: NodePorts
      - name: NodeResourcesFit
      - name: VolumeRestrictions
      - name: EBSLimits
      - name: GCEPDLimits
      - name: NodeVolumeLimits
      - name: AzureDiskLimits
      - name: VolumeBinding
      - name: VolumeZone
      - name: PodTopologySpread
      - name: InterPodAffinity
    # 评分阶段
    score:
      enabled:
      - name: NodeResourcesBalancedAllocation
        weight: 1
      - name: ImageLocality
        weight: 1
      - name: InterPodAffinity
        weight: 1
      - name: NodeResourcesFit
        weight: 1
      - name: NodeAffinity
        weight: 1
      - name: PodTopologySpread
        weight: 2
      - name: TaintToleration
        weight: 1
  pluginConfig:
  # NodeResourcesFit配置
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: LeastAllocated  # 或 MostAllocated, RequestedToCapacityRatio
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
  # PodTopologySpread配置
  - name: PodTopologySpread
    args:
      defaultConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
      defaultingType: List
```

### 3.5 调度插件说明

| 插件名 | 阶段 | 版本 | 作用 |
|:---|:---|:---|:---|
| `NodeResourcesFit` | PreFilter/Filter/Score | 稳定 | 资源请求检查与评分 |
| `NodePorts` | PreFilter/Filter | 稳定 | 端口冲突检查 |
| `NodeAffinity` | Filter/Score | 稳定 | 节点亲和性 |
| `InterPodAffinity` | PreFilter/Filter/Score | 稳定 | Pod间亲和/反亲和 |
| `PodTopologySpread` | PreFilter/Filter/Score | v1.19 GA | 拓扑分布约束 |
| `TaintToleration` | Filter/Score | 稳定 | 污点容忍检查 |
| `VolumeBinding` | PreFilter/Filter/Reserve | 稳定 | 存储卷绑定 |
| `NodeUnschedulable` | Filter | 稳定 | 排除不可调度节点 |
| `NodeResourcesBalancedAllocation` | Score | 稳定 | 资源均衡评分 |
| `ImageLocality` | Score | 稳定 | 镜像本地性评分 |

### 3.6 多调度器配置

```yaml
# 第二调度器配置
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
- schedulerName: high-priority-scheduler
  plugins:
    score:
      enabled:
      - name: NodeResourcesFit
        weight: 2  # 优先资源匹配
      disabled:
      - name: ImageLocality  # 禁用镜像本地性
```

**使用方式**:
```yaml
apiVersion: v1
kind: Pod
spec:
  schedulerName: high-priority-scheduler  # 指定调度器
  containers:
  - name: app
    image: nginx
```

---

## kube-controller-manager 参数

### 4.1 基础配置参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--bind-address` | 0.0.0.0 | 127.0.0.1 | 稳定 | 监听地址 | 限制访问 |
| `--secure-port` | 10257 | 10257 | 稳定 | HTTPS端口 | 指标/健康检查 |
| `--profiling` | true | **false** | 稳定 | 启用pprof | **生产禁用** |
| `--contention-profiling` | false | false | 稳定 | 锁竞争分析 | 调试用 |
| `--use-service-account-credentials` | false | **true** | 稳定 | 各控制器独立SA | **安全最佳实践** |

### 4.2 Leader选举参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--leader-elect` | true | true | 稳定 | 启用选举 | **HA必须** |
| `--leader-elect-lease-duration` | 15s | 15s | 稳定 | 租约时长 | 影响故障切换 |
| `--leader-elect-renew-deadline` | 10s | 10s | 稳定 | 续租期限 | 必须 < lease-duration |
| `--leader-elect-retry-period` | 2s | 2s | 稳定 | 重试间隔 | 选举速度 |
| `--leader-elect-resource-lock` | leases | leases | 稳定 | 锁类型 | leases最高效 |

### 4.3 控制器启用参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--controllers` | * | * 或自定义列表 | 稳定 | 启用的控制器 | 可按需禁用 |

**核心控制器列表**:

| 控制器 | 默认启用 | 作用 | 禁用影响 |
|:---|:---|:---|:---|
| `attachdetach` | 是 | 卷挂载/卸载 | 存储功能失效 |
| `bootstrapsigner` | 是 | Bootstrap Token签名 | 节点无法自动加入 |
| `clusterrole-aggregation` | 是 | ClusterRole聚合 | RBAC聚合失效 |
| `cronjob` | 是 | CronJob调度 | 定时任务失效 |
| `csrapproving` | 是 | CSR自动审批 | 证书轮换失败 |
| `csrsigning` | 是 | CSR签名 | 证书签发失败 |
| `daemonset` | 是 | DaemonSet管理 | DS不工作 |
| `deployment` | 是 | Deployment管理 | **核心**；滚动更新失效 |
| `disruption` | 是 | PDB控制 | PDB无效 |
| `endpoint` | 是 | Endpoint更新 | **核心**；服务发现失效 |
| `endpointslice` | 是 | EndpointSlice | 大规模Service支持 |
| `garbagecollector` | 是 | 级联删除GC | 孤儿资源无法清理 |
| `horizontalpodautoscaling` | 是 | HPA | 自动扩缩失效 |
| `job` | 是 | Job管理 | 批处理失效 |
| `namespace` | 是 | NS终结器 | NS无法删除 |
| `nodeipam` | 是 | 节点CIDR分配 | Pod IP分配失败 |
| `nodelifecycle` | 是 | 节点状态/驱逐 | **核心**；故障检测失效 |
| `persistentvolume-binder` | 是 | PV/PVC绑定 | 存储绑定失效 |
| `persistentvolume-expander` | 是 | PVC扩容 | 卷扩容失效 |
| `podgc` | 是 | 终止Pod GC | Pod对象堆积 |
| `pv-protection` | 是 | PV保护终结器 | PV可能误删 |
| `pvc-protection` | 是 | PVC保护终结器 | PVC可能误删 |
| `replicaset` | 是 | ReplicaSet管理 | **核心**；副本管理失效 |
| `replicationcontroller` | 是 | RC管理 | 旧RC不工作 |
| `resourcequota` | 是 | 配额执行 | 配额无效 |
| `root-ca-cert-publisher` | 是 | CA证书发布 | 服务间TLS验证问题 |
| `serviceaccount` | 是 | SA管理 | **核心**；SA创建失败 |
| `serviceaccount-token` | 是 | SA Token创建 | Token生成失败 |
| `statefulset` | 是 | StatefulSet管理 | STS不工作 |
| `tokencleaner` | 是 | 过期Token清理 | Token堆积 |
| `ttl-after-finished` | 是 | Job TTL清理 | 完成Job堆积 |

### 4.4 并发控制参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--concurrent-deployment-syncs` | 5 | **10-20** | 稳定 | Deployment并发 | 大量Deployment时增加 |
| `--concurrent-replicaset-syncs` | 5 | **10-20** | 稳定 | ReplicaSet并发 | 大量RS时增加 |
| `--concurrent-statefulset-syncs` | 5 | 10 | 稳定 | StatefulSet并发 | 大量STS时增加 |
| `--concurrent-daemonset-syncs` | 2 | 5 | 稳定 | DaemonSet并发 | 大量DS时增加 |
| `--concurrent-job-syncs` | 5 | 10 | 稳定 | Job并发 | 大量Job时增加 |
| `--concurrent-service-syncs` | 5 | **10-20** | 稳定 | Service并发 | 大量Service时增加 |
| `--concurrent-endpoint-syncs` | 5 | **10-20** | 稳定 | Endpoint并发 | 大量Endpoint时增加 |
| `--concurrent-service-endpoint-syncs` | 5 | 10 | 稳定 | ServiceEndpoint并发 | EndpointSlice |
| `--concurrent-namespace-syncs` | 10 | 20 | 稳定 | Namespace并发 | 大量NS时增加 |
| `--concurrent-rc-syncs` | 5 | 5 | 稳定 | RC并发 | 旧资源 |
| `--concurrent-gc-syncs` | 20 | **30-50** | 稳定 | GC并发 | 频繁删除时增加 |
| `--concurrent-horizontal-pod-autoscaler-syncs` | 5 | 10 | 稳定 | HPA并发 | 大量HPA时增加 |

### 4.5 节点生命周期参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--node-monitor-period` | 5s | 5s | 稳定 | 节点状态检查周期 | 检测速度 |
| `--node-monitor-grace-period` | 40s | 40s | 稳定 | 节点宽限期 | 影响NotReady判定 |
| `--node-startup-grace-period` | 1m | 1m | 稳定 | 节点启动宽限期 | 新节点容忍时间 |
| `--pod-eviction-timeout` | 5m | 5m | v1.26弃用 | Pod驱逐超时 | 使用--default-not-ready-toleration-seconds替代 |
| `--unhealthy-zone-threshold` | 0.55 | 0.55 | 稳定 | 不健康Zone阈值 | 大规模驱逐保护 |
| `--large-cluster-size-threshold` | 50 | 50 | 稳定 | 大集群阈值 | 影响驱逐速率 |
| `--secondary-node-eviction-rate` | 0.01 | 0.01 | 稳定 | 不健康Zone驱逐速率 | 每秒驱逐节点数 |

### 4.6 GC与资源管理参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--terminated-pod-gc-threshold` | 12500 | 12500 | 稳定 | 终止Pod GC阈值 | 控制API对象数量 |
| `--enable-garbage-collector` | true | true | 稳定 | 启用级联GC | 自动清理依赖资源 |
| `--enable-hostpath-provisioner` | false | false | 稳定 | HostPath动态供给 | 仅测试环境 |

### 4.7 API通信参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--kube-api-qps` | 20 | **50-100** | 稳定 | API请求QPS | 大集群需增加 |
| `--kube-api-burst` | 30 | **100-200** | 稳定 | API请求突发 | 大集群需增加 |

### 4.8 证书签名参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--cluster-signing-cert-file` | 无 | `/etc/kubernetes/pki/ca.crt` | 稳定 | 集群CA证书 | CSR签名用 |
| `--cluster-signing-key-file` | 无 | `/etc/kubernetes/pki/ca.key` | 稳定 | 集群CA私钥 | CSR签名用 |
| `--cluster-signing-duration` | 8760h (1年) | 8760h | 稳定 | 签名证书有效期 | kubelet证书轮换 |
| `--experimental-cluster-signing-duration` | 弃用 | - | v1.25弃用 | 使用--cluster-signing-duration | - |

### 4.9 Service Account参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--service-account-private-key-file` | 无 | `/etc/kubernetes/pki/sa.key` | 稳定 | SA签名私钥 | Token签发 |
| `--root-ca-file` | 无 | `/etc/kubernetes/pki/ca.crt` | 稳定 | 根CA证书 | 注入Pod SA |

---

## kubelet 参数

### 5.1 基础配置参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--config` | 无 | `/var/lib/kubelet/config.yaml` | 推荐 | kubelet配置文件 | **推荐**；替代命令行参数 |
| `--kubeconfig` | 无 | `/etc/kubernetes/kubelet.conf` | 稳定 | API Server连接配置 | **必须配置** |
| `--bootstrap-kubeconfig` | 无 | `/etc/kubernetes/bootstrap-kubelet.conf` | 稳定 | TLS Bootstrap配置 | 节点自动加入 |
| `--hostname-override` | 系统主机名 | 明确指定 | 稳定 | 覆盖主机名 | 云环境可能需要 |
| `--node-ip` | 自动检测 | 明确指定 | 稳定 | 节点IP | 多网卡时**必须指定** |

### 5.2 容器运行时参数 (CRI)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--container-runtime-endpoint` | 无 | `unix:///run/containerd/containerd.sock` | v1.24+ **必须** | CRI端点 | v1.24移除dockershim |
| `--image-service-endpoint` | 同container-runtime | 通常相同 | 稳定 | 镜像服务端点 | 特殊场景分离 |
| `--pod-infra-container-image` | registry.k8s.io/pause:3.9 | 按版本 | 稳定 | Pause容器镜像 | 离线环境需预拉取 |
| `--runtime-request-timeout` | 2m | 2m | 稳定 | CRI请求超时 | 慢存储可能需增加 |

**containerd CRI端点**:
- containerd: `unix:///run/containerd/containerd.sock`
- CRI-O: `unix:///var/run/crio/crio.sock`

### 5.3 Pod与容器限制参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--max-pods` | 110 | 110-250 | 稳定 | 节点最大Pod数 | 需配合CIDR规划 |
| `--pod-max-pids` | -1 | **4096** | 稳定 | Pod最大PID数 | **防止PID耗尽** |
| `--pods-per-core` | 0 | 0 | 稳定 | 每核Pod数限制 | 0=不限制 |
| `--max-open-files` | 1000000 | 1000000 | 稳定 | 最大打开文件数 | 高并发可能需增加 |

### 5.4 资源预留参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--system-reserved` | 无 | `cpu=500m,memory=1Gi` | 稳定 | 系统进程预留 | **保护系统进程** |
| `--kube-reserved` | 无 | `cpu=500m,memory=1Gi` | 稳定 | K8S组件预留 | **保护K8S组件** |
| `--eviction-hard` | 见下文 | 自定义 | 稳定 | 硬驱逐阈值 | 立即驱逐 |
| `--eviction-soft` | 无 | 自定义 | 稳定 | 软驱逐阈值 | 优雅驱逐 |
| `--eviction-soft-grace-period` | 无 | 按需配置 | 稳定 | 软驱逐宽限期 | 配合软驱逐使用 |
| `--eviction-pressure-transition-period` | 5m | 5m | 稳定 | 压力状态转换周期 | 防止状态抖动 |
| `--eviction-minimum-reclaim` | 无 | `memory.available=500Mi` | 稳定 | 最小回收量 | 防止频繁驱逐 |
| `--enforce-node-allocatable` | pods | `pods,kube-reserved,system-reserved` | 稳定 | 强制可分配 | **配合预留使用** |
| `--reserved-cpus` | 无 | 0-1 (可选) | 稳定 | 预留特定CPU | 高性能场景 |
| `--reserved-memory` | 无 | NUMA配置 | v1.21+ | 预留特定内存 | NUMA感知 |

**资源预留计算公式**:
```
Allocatable = Capacity - kube-reserved - system-reserved - eviction-threshold
```

**推荐预留值** (按节点规格):

| 节点内存 | system-reserved | kube-reserved | eviction-hard |
|:---|:---|:---|:---|
| 4GB | cpu=100m,memory=256Mi | cpu=100m,memory=256Mi | memory.available=256Mi |
| 8GB | cpu=200m,memory=512Mi | cpu=200m,memory=512Mi | memory.available=512Mi |
| 16GB | cpu=500m,memory=1Gi | cpu=500m,memory=1Gi | memory.available=1Gi |
| 32GB+ | cpu=1,memory=2Gi | cpu=1,memory=2Gi | memory.available=1.5Gi |

### 5.5 驱逐阈值配置

| 驱逐信号 | 默认硬阈值 | 推荐生产值 | 说明 |
|:---|:---|:---|:---|
| `memory.available` | 100Mi | **500Mi-1Gi** | 可用内存 |
| `nodefs.available` | 10% | **15%** | 节点文件系统可用 |
| `nodefs.inodesFree` | 5% | **10%** | 节点inode可用 |
| `imagefs.available` | 15% | 15% | 镜像文件系统可用 |
| `pid.available` | 无 | **1000** | 可用PID数 |

**完整驱逐配置示例**:
```yaml
# kubelet配置文件中
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "15%"
  pid.available: "1000"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "2m"
  nodefs.available: "2m"
evictionMinimumReclaim:
  memory.available: "500Mi"
  nodefs.available: "1Gi"
evictionPressureTransitionPeriod: 5m
```

### 5.6 镜像管理参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--image-gc-high-threshold` | 85 | **80** | 稳定 | 镜像GC高水位% | 触发GC |
| `--image-gc-low-threshold` | 80 | **70** | 稳定 | 镜像GC低水位% | GC目标 |
| `--serialize-image-pulls` | true | **false** | 稳定 | 串行拉取镜像 | 并行提速但增加负载 |
| `--image-pull-progress-deadline` | 1m | 2m | 稳定 | 拉取进度超时 | 大镜像需增加 |
| `--registry-qps` | 5 | **10-20** | 稳定 | 镜像仓库QPS | 大量Pod启动时增加 |
| `--registry-burst` | 10 | **20-40** | 稳定 | 镜像仓库突发 | 大量Pod启动时增加 |

### 5.7 节点状态更新参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--node-status-update-frequency` | 10s | 10s | 稳定 | 状态更新频率 | 影响控制平面负载 |
| `--node-status-report-frequency` | 5m | 5m | 稳定 | 无变化时报告频率 | Lease续租 |
| `--node-labels` | 无 | 按需添加 | 稳定 | 节点标签 | 调度约束 |

### 5.8 安全参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--rotate-certificates` | true | true | 稳定 | 证书自动轮换 | **安全必需** |
| `--rotate-server-certificates` | false | true | 稳定 | 服务端证书轮换 | 增强安全 |
| `--protect-kernel-defaults` | false | **true** | 稳定 | 保护内核默认值 | **安全加固** |
| `--read-only-port` | 10255 | **0** | 稳定 | 只读端口 | **生产必须禁用** |
| `--anonymous-auth` | true | **false** | 稳定 | 匿名认证 | **生产禁用** |
| `--authorization-mode` | AlwaysAllow | **Webhook** | 稳定 | 授权模式 | **生产必须** |
| `--authentication-token-webhook` | false | true | 稳定 | Token Webhook认证 | 配合API Server |
| `--make-iptables-util-chains` | true | true | 稳定 | 创建iptables链 | kube-proxy依赖 |
| `--allowed-unsafe-sysctls` | 无 | 按需配置 | 稳定 | 允许的unsafe sysctl | 网络调优可能需要 |

### 5.9 cgroup参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--cgroup-driver` | cgroupfs | **systemd** | 稳定 | cgroup驱动 | v1.22+推荐systemd |
| `--cgroup-root` | 无 | / | 稳定 | cgroup根路径 | 容器运行时一致 |
| `--runtime-cgroups` | 无 | /system.slice/containerd.service | 稳定 | 运行时cgroup | systemd场景 |
| `--kubelet-cgroups` | 无 | /system.slice/kubelet.service | 稳定 | kubelet cgroup | systemd场景 |
| `--cpu-manager-policy` | none | **static** | 稳定 | CPU管理策略 | 独占CPU场景 |
| `--cpu-manager-policy-options` | 无 | full-pcpus-only=true | v1.22+ | CPU策略选项 | NUMA优化 |
| `--cpu-manager-reconcile-period` | 10s | 10s | 稳定 | CPU管理器协调周期 | - |
| `--memory-manager-policy` | None | **Static** | v1.22 Beta | 内存管理策略 | NUMA感知 |
| `--topology-manager-policy` | none | **best-effort** 或 **restricted** | v1.18 Beta | 拓扑管理策略 | GPU/NUMA场景 |
| `--topology-manager-scope` | container | **pod** | v1.22+ | 拓扑管理范围 | Pod级别对齐 |

**拓扑管理器策略说明**:

| 策略 | 说明 | 适用场景 |
|:---|:---|:---|
| `none` | 不进行拓扑对齐 | 默认，通用场景 |
| `best-effort` | 尽力对齐，不阻止调度 | 推荐，平衡性能与调度成功率 |
| `restricted` | 严格对齐，不满足则拒绝 | 高性能计算、AI训练 |
| `single-numa-node` | 单NUMA节点对齐 | 极端性能要求 |

### 5.10 优雅关闭参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--shutdown-grace-period` | 0s | **30s** | v1.21 GA | 关闭宽限期 | **生产推荐** |
| `--shutdown-grace-period-critical-pods` | 0s | **10s** | v1.21 GA | 关键Pod关闭宽限期 | 优先关闭非关键Pod |

### 5.11 kubelet配置文件示例

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 认证授权
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2m0s
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt
authorization:
  mode: Webhook
  webhook:
    cacheAuthorizedTTL: 5m0s
    cacheUnauthorizedTTL: 30s

# 容器运行时
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd

# 资源管理
maxPods: 110
podPidsLimit: 4096
systemReserved:
  cpu: 500m
  memory: 1Gi
kubeReserved:
  cpu: 500m
  memory: 1Gi
enforceNodeAllocatable:
- pods
- kube-reserved
- system-reserved

# 驱逐配置
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "15%"
  pid.available: "1000"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "2m"
  nodefs.available: "2m"
evictionPressureTransitionPeriod: 5m

# 镜像管理
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
serializeImagePulls: false
registryPullQPS: 10
registryBurst: 20

# 日志与监控
logging:
  format: json
  verbosity: 2
clusterDomain: cluster.local
clusterDNS:
- 10.96.0.10

# 安全配置
rotateCertificates: true
serverTLSBootstrap: true
protectKernelDefaults: true
readOnlyPort: 0

# 优雅关闭
shutdownGracePeriod: 30s
shutdownGracePeriodCriticalPods: 10s

# CPU/Memory管理 (高性能场景)
cpuManagerPolicy: static
cpuManagerReconcilePeriod: 10s
memoryManagerPolicy: Static
topologyManagerPolicy: best-effort
topologyManagerScope: pod
```

---

## kube-proxy 参数

### 6.1 基础配置参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--config` | 无 | `/var/lib/kube-proxy/config.conf` | 推荐 | 配置文件 | **推荐**；替代命令行 |
| `--hostname-override` | 系统主机名 | 与kubelet一致 | 稳定 | 覆盖主机名 | **必须与kubelet一致** |
| `--bind-address` | 0.0.0.0 | 0.0.0.0 | 稳定 | 监听地址 | - |
| `--healthz-bind-address` | 0.0.0.0:10256 | 0.0.0.0:10256 | 稳定 | 健康检查地址 | - |
| `--metrics-bind-address` | 127.0.0.1:10249 | **127.0.0.1:10249** | 稳定 | 指标端点 | **限制访问** |
| `--profiling` | false | false | 稳定 | 启用pprof | 调试用 |

### 6.2 代理模式参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--proxy-mode` | iptables | **ipvs** | v1.26 nftables Alpha | 代理模式 | **大集群推荐IPVS** |
| `--cluster-cidr` | 无 | Pod CIDR | 稳定 | 集群CIDR | 影响SNAT行为 |
| `--masquerade-all` | false | 按CNI要求 | 稳定 | 全部SNAT | 某些CNI需要 |
| `--detect-local-mode` | LocalModeClusterCIDR | ClusterCIDR | 稳定 | 本地流量检测 | 影响SNAT判断 |

**代理模式对比**:

| 模式 | 版本状态 | 性能 | Service数量支持 | 适用场景 |
|:---|:---|:---|:---|:---|
| `iptables` | 稳定 | 中等 | <1000 | 小规模集群 |
| `ipvs` | 稳定 | **高** | >10000 | **生产推荐** |
| `nftables` | v1.31 Beta | 高 | 大规模 | 未来替代iptables |
| `kernelspace` | Windows | - | - | Windows节点 |

### 6.3 IPVS模式参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--ipvs-scheduler` | rr | 按需选择 | 稳定 | IPVS调度算法 | 见下文 |
| `--ipvs-min-sync-period` | 0s | **1s** | 稳定 | 最小同步周期 | 减少同步频率 |
| `--ipvs-sync-period` | 30s | 30s | 稳定 | 同步周期 | 规则更新频率 |
| `--ipvs-strict-arp` | false | **true** (Calico/MetalLB) | 稳定 | 严格ARP | LoadBalancer需要 |
| `--ipvs-exclude-cidrs` | 无 | 按需配置 | 稳定 | 排除CIDR | 特殊网络段 |
| `--ipvs-tcp-timeout` | 0 (900s) | 900s | 稳定 | TCP超时 | 连接保持 |
| `--ipvs-tcpfin-timeout` | 0 (120s) | 120s | 稳定 | TCP FIN超时 | 连接回收 |
| `--ipvs-udp-timeout` | 0 (300s) | 300s | 稳定 | UDP超时 | UDP连接 |

**IPVS调度算法**:

| 算法 | 全称 | 说明 | 适用场景 |
|:---|:---|:---|:---|
| `rr` | Round Robin | 轮询 | **默认**；通用场景 |
| `lc` | Least Connection | 最少连接 | 长连接服务 |
| `dh` | Destination Hashing | 目标哈希 | 缓存服务 |
| `sh` | Source Hashing | 源哈希 | 会话保持 |
| `sed` | Shortest Expected Delay | 最短期望延迟 | 动态负载 |
| `nq` | Never Queue | 从不排队 | 低延迟 |

### 6.4 iptables模式参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--iptables-min-sync-period` | 1s | 1s | 稳定 | 最小同步周期 | 减少CPU |
| `--iptables-sync-period` | 30s | 30s | 稳定 | 同步周期 | 规则更新 |
| `--iptables-masquerade-bit` | 14 | 14 | 稳定 | SNAT标记位 | 通常不变 |
| `--iptables-localhost-nodeports` | true | true | 稳定 | localhost NodePort | 本地访问 |

### 6.5 nftables模式参数 (v1.31+)

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--proxy-mode=nftables` | - | 测试中 | v1.31 Beta | nftables模式 | 新内核推荐 |
| `--nftables-min-sync-period` | 1s | 1s | v1.31 | 最小同步周期 | - |
| `--nftables-sync-period` | 30s | 30s | v1.31 | 同步周期 | - |

### 6.6 Conntrack参数

| 参数 | 默认值 | 推荐生产值 | 版本变更 | 说明 | 安全/性能影响 |
|:---|:---|:---|:---|:---|:---|
| `--conntrack-max-per-core` | 32768 | **65536** | 稳定 | 每核conntrack数 | 高流量需增加 |
| `--conntrack-min` | 131072 | **262144** | 稳定 | 最小conntrack数 | 高流量需增加 |
| `--conntrack-tcp-timeout-established` | 86400s | 86400s | 稳定 | TCP已建立超时 | 长连接 |
| `--conntrack-tcp-timeout-close-wait` | 1h | 1h | 稳定 | CLOSE_WAIT超时 | 连接回收 |

### 6.7 kube-proxy配置文件示例

```yaml
# /var/lib/kube-proxy/config.conf
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration

# 客户端连接
clientConnection:
  kubeconfig: /var/lib/kube-proxy/kubeconfig.conf
  qps: 50
  burst: 100

# 代理模式
mode: ipvs
clusterCIDR: 10.244.0.0/16

# IPVS配置
ipvs:
  scheduler: rr
  minSyncPeriod: 1s
  syncPeriod: 30s
  strictARP: true
  tcpTimeout: 900s
  tcpFinTimeout: 120s
  udpTimeout: 300s
  excludeCIDRs: []

# iptables配置 (仅iptables模式)
iptables:
  minSyncPeriod: 1s
  syncPeriod: 30s
  masqueradeAll: false
  masqueradeBit: 14
  localhostNodePorts: true

# conntrack配置
conntrack:
  maxPerCore: 65536
  min: 262144
  tcpEstablishedTimeout: 86400s
  tcpCloseWaitTimeout: 1h

# 网络
bindAddress: 0.0.0.0
healthzBindAddress: 0.0.0.0:10256
metricsBindAddress: 127.0.0.1:10249

# 检测模式
detectLocalMode: ClusterCIDR
detectLocal:
  bridgeInterface: ""
  interfaceNamePrefix: ""

# NodePort地址
nodePortAddresses: []
```

---

## Feature Gates 完整参考

### 7.1 Feature Gates版本演进表

| Feature Gate | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | 说明 |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| `PodSecurity` | GA | GA | GA | GA | GA | GA | GA | GA | Pod安全标准 |
| `ServerSideApply` | GA | GA | GA | GA | GA | GA | GA | GA | 服务端应用 |
| `EphemeralContainers` | GA | GA | GA | GA | GA | GA | GA | GA | 临时调试容器 |
| `GracefulNodeShutdown` | Beta | GA | GA | GA | GA | GA | GA | GA | 优雅节点关闭 |
| `ProbeTerminationGracePeriod` | Beta | GA | GA | GA | GA | GA | GA | GA | 探针终止宽限期 |
| `JobTrackingWithFinalizers` | Beta | GA | GA | GA | GA | GA | GA | GA | Job跟踪终结器 |
| `MinDomainsInPodTopologySpread` | Beta | Beta | GA | GA | GA | GA | GA | GA | 拓扑最小域数 |
| `NodeSwap` | Alpha | Beta | Beta | Beta | Beta | Beta | GA | GA | 节点Swap支持 |
| `ValidatingAdmissionPolicy` | Alpha | Beta | Beta | Beta | Beta | GA | GA | GA | CEL验证策略 |
| `SidecarContainers` | - | Alpha | Alpha | Beta | Beta | Beta | GA | GA | 原生Sidecar容器 |
| `InPlacePodVerticalScaling` | - | Alpha | Alpha | Alpha | Alpha | Beta | Beta | GA | 就地Pod垂直扩缩 |
| `UserNamespacesSupport` | Alpha | Alpha | Alpha | Beta | Beta | Beta | GA | GA | 用户命名空间 |
| `DynamicResourceAllocation` | Alpha | Alpha | Alpha | Alpha | Beta | Beta | Beta | GA | 动态资源分配(GPU) |
| `PodSchedulingReadiness` | - | Alpha | Beta | GA | GA | GA | GA | GA | Pod调度就绪 |
| `KMSv2` | Alpha | Beta | Beta | GA | GA | GA | GA | GA | KMS v2加密 |
| `CELValidatingAdmission` | - | Alpha | Beta | Beta | Beta | GA | GA | GA | CEL准入验证 |
| `APIServerTracing` | Alpha | Beta | Beta | Beta | Beta | GA | GA | GA | API Server链路追踪 |
| `ServiceTrafficDistribution` | - | - | - | Alpha | Alpha | Beta | Beta | GA | Service流量分布 |
| `RetryGenerateName` | - | - | - | - | Alpha | Beta | GA | GA | 重试GenerateName |
| `RecoverVolumeExpansionFailure` | Alpha | Alpha | Alpha | Alpha | Alpha | Beta | GA | GA | 卷扩展失败恢复 |

### 7.2 生产推荐Feature Gates配置

```yaml
# kube-apiserver
--feature-gates=\
ValidatingAdmissionPolicy=true,\
APIServerTracing=true,\
DynamicResourceAllocation=true

# kube-scheduler  
--feature-gates=\
PodSchedulingReadiness=true

# kube-controller-manager
--feature-gates=\
JobTrackingWithFinalizers=true

# kubelet
--feature-gates=\
GracefulNodeShutdown=true,\
SidecarContainers=true,\
InPlacePodVerticalScaling=true,\
UserNamespacesSupport=true
```

### 7.3 版本状态说明

| 状态 | 默认值 | 说明 |
|:---|:---|:---|
| Alpha | false | 实验性，可能有bug，随时移除 |
| Beta | true (通常) | 功能完整，API可能变化 |
| GA | true (锁定) | 稳定，不可禁用 |
| Deprecated | - | 已弃用，将在未来版本移除 |

---

## 生产配置示例

### 8.1 kubeconfig 结构与认证方式

```yaml
apiVersion: v1
kind: Config
current-context: production
preferences: {}

clusters:
- cluster:
    certificate-authority-data: <base64>  # 或 certificate-authority: /path/to/ca.crt
    server: https://apiserver.example.com:6443
    # 可选：跳过TLS验证（仅测试！）
    # insecure-skip-tls-verify: true
  name: production-cluster

contexts:
- context:
    cluster: production-cluster
    user: admin
    namespace: default
  name: production

users:
- name: admin
  user:
    # 方式1: 证书认证
    client-certificate-data: <base64>
    client-key-data: <base64>
    
    # 方式2: Token认证
    # token: <bearer-token>
    
    # 方式3: exec认证（推荐云环境）
    # exec:
    #   apiVersion: client.authentication.k8s.io/v1beta1
    #   command: aws
    #   args: ["eks", "get-token", "--cluster-name", "my-cluster"]
    #   env:
    #   - name: AWS_PROFILE
    #     value: production
```

**多集群kubeconfig示例**:

```yaml
apiVersion: v1
kind: Config
current-context: prod-cluster-1

clusters:
- cluster:
    server: https://prod-1.example.com:6443
    certificate-authority-data: <base64>
  name: prod-cluster-1
- cluster:
    server: https://prod-2.example.com:6443
    certificate-authority-data: <base64>
  name: prod-cluster-2
- cluster:
    server: https://staging.example.com:6443
    certificate-authority-data: <base64>
  name: staging

contexts:
- context:
    cluster: prod-cluster-1
    user: admin
  name: prod-1
- context:
    cluster: prod-cluster-2
    user: admin
  name: prod-2
- context:
    cluster: staging
    user: developer
  name: staging

users:
- name: admin
  user:
    client-certificate-data: <base64>
    client-key-data: <base64>
- name: developer
  user:
    token: <bearer-token>
```

### 8.2 kubeadm集群配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
clusterName: production

# 控制平面端点（HA场景）
controlPlaneEndpoint: "api.k8s.example.com:6443"

# 网络配置
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"

# etcd配置
etcd:
  external:
    endpoints:
    - https://etcd-0.example.com:2379
    - https://etcd-1.example.com:2379
    - https://etcd-2.example.com:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

# API Server配置
apiServer:
  certSANs:
  - "api.k8s.example.com"
  - "10.0.0.100"
  - "kubernetes"
  - "kubernetes.default"
  extraArgs:
    authorization-mode: "Node,RBAC"
    enable-admission-plugins: "NodeRestriction,PodSecurity,LimitRanger,ServiceAccount,ResourceQuota"
    audit-log-path: "/var/log/kubernetes/audit.log"
    audit-policy-file: "/etc/kubernetes/audit-policy.yaml"
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    encryption-provider-config: "/etc/kubernetes/encryption-config.yaml"
    profiling: "false"
    anonymous-auth: "false"
    service-account-issuer: "https://kubernetes.default.svc"
    service-account-signing-key-file: "/etc/kubernetes/pki/sa.key"
  extraVolumes:
  - name: audit-config
    hostPath: /etc/kubernetes/audit-policy.yaml
    mountPath: /etc/kubernetes/audit-policy.yaml
    readOnly: true
  - name: audit-log
    hostPath: /var/log/kubernetes
    mountPath: /var/log/kubernetes
    pathType: DirectoryOrCreate
  - name: encryption-config
    hostPath: /etc/kubernetes/encryption-config.yaml
    mountPath: /etc/kubernetes/encryption-config.yaml
    readOnly: true

# Controller Manager配置
controllerManager:
  extraArgs:
    profiling: "false"
    terminated-pod-gc-threshold: "12500"
    node-monitor-grace-period: "40s"
    pod-eviction-timeout: "5m0s"
    use-service-account-credentials: "true"
    concurrent-deployment-syncs: "10"
    concurrent-replicaset-syncs: "10"
    concurrent-gc-syncs: "30"
    kube-api-qps: "50"
    kube-api-burst: "100"

# Scheduler配置
scheduler:
  extraArgs:
    profiling: "false"
    kube-api-qps: "100"
    kube-api-burst: "200"

---
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: "10.0.0.101"
  bindPort: 6443
nodeRegistration:
  criSocket: unix:///run/containerd/containerd.sock
  taints:
  - key: node-role.kubernetes.io/control-plane
    effect: NoSchedule

---
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: systemd
maxPods: 110
podPidsLimit: 4096
systemReserved:
  cpu: 500m
  memory: 1Gi
kubeReserved:
  cpu: 500m
  memory: 1Gi
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "15%"
  pid.available: "1000"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "2m"
  nodefs.available: "2m"
rotateCertificates: true
serverTLSBootstrap: true
protectKernelDefaults: true
readOnlyPort: 0
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
authorization:
  mode: Webhook

---
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: ipvs
ipvs:
  scheduler: rr
  strictARP: true
conntrack:
  maxPerCore: 65536
  min: 262144
```

### 8.3 集群规模配置参考

| 集群规模 | 节点数 | API Server | Controller Manager | Scheduler | etcd |
|:---|:---|:---|:---|:---|:---|
| **小型** | <100 | 默认配置 | 默认配置 | 默认配置 | 3节点, 2GB quota |
| **中型** | 100-500 | max-requests-inflight=800, max-mutating=400 | concurrent-*=10, api-qps=50 | api-qps=100 | 3节点, 4GB quota |
| **大型** | 500-2000 | max-requests-inflight=1600, max-mutating=800 | concurrent-*=20, api-qps=100 | api-qps=200 | 5节点, 8GB quota |
| **超大型** | >2000 | APF限流，多API Server | concurrent-*=50, api-qps=200 | api-qps=400 | 专用etcd集群 |

---

## 云厂商特定配置

### 9.1 阿里云ACK配置

| 配置项 | ACK托管版 | ACK专有版 | 说明 |
|:---|:---|:---|:---|
| **控制平面参数** | 控制台配置 | 手动配置 | 托管版通过控制台调整 |
| **kubelet配置** | 节点池配置 | 手动配置 | 通过节点池模板管理 |
| **kube-proxy模式** | 创建时选择 | 创建时选择 | **推荐IPVS** |
| **审计日志** | 自动集成SLS | 手动配置 | 托管版自动接入日志服务 |
| **证书轮换** | 自动 | 手动 | 托管版自动管理证书 |
| **CNI** | Terway/Flannel | Terway/Flannel | Terway推荐ENI多IP模式 |
| **Ingress** | ALB/Nginx | ALB/Nginx | ALB支持原生ALB Ingress |

**ACK节点池kubelet配置**:

```yaml
# 通过ACK控制台或API配置节点池
kubeletConfiguration:
  maxPods: 110
  cgroupDriver: systemd
  evictionHard:
    memory.available: "500Mi"
    nodefs.available: "15%"
  systemReserved:
    cpu: "500m"
    memory: "1Gi"
  kubeReserved:
    cpu: "500m"
    memory: "1Gi"
```

### 9.2 AWS EKS配置

| 配置项 | 托管节点组 | 自管节点 | Fargate |
|:---|:---|:---|:---|
| **kubelet配置** | 启动模板 | 手动配置 | N/A |
| **kube-proxy** | 托管 | 托管 | 托管 |
| **CNI** | VPC CNI | VPC CNI | VPC CNI |
| **Ingress** | ALB Controller | ALB Controller | ALB Controller |

**EKS kubeconfig (aws-auth)**:

```yaml
# aws-auth ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: aws-auth
  namespace: kube-system
data:
  mapRoles: |
    - rolearn: arn:aws:iam::ACCOUNT_ID:role/NodeInstanceRole
      username: system:node:{{EC2PrivateDNSName}}
      groups:
        - system:bootstrappers
        - system:nodes
    - rolearn: arn:aws:iam::ACCOUNT_ID:role/AdminRole
      username: admin
      groups:
        - system:masters
```

### 9.3 Azure AKS配置

| 配置项 | 托管 | 说明 |
|:---|:---|:---|
| **控制平面** | 完全托管 | 无法直接配置 |
| **kubelet配置** | 节点池配置 | 通过CLI/API配置 |
| **kube-proxy** | 托管 | 可选IPVS |
| **CNI** | Azure CNI/Kubenet | Azure CNI推荐 |

**AKS节点池kubelet配置**:

```bash
# Azure CLI
az aks nodepool add \
  --resource-group myResourceGroup \
  --cluster-name myAKSCluster \
  --name mynodepool \
  --kubelet-config ./kubelet-config.json

# kubelet-config.json
{
  "cpuManagerPolicy": "static",
  "topologyManagerPolicy": "best-effort",
  "allowedUnsafeSysctls": ["net.core.somaxconn"],
  "containerLogMaxSizeMB": 100,
  "containerLogMaxFiles": 5
}
```

### 9.4 GCP GKE配置

| 配置项 | Autopilot | Standard |
|:---|:---|:---|
| **控制平面** | 完全托管 | 托管 |
| **节点配置** | 自动 | 节点池配置 |
| **kube-proxy** | 托管 | 可配置 |
| **CNI** | GKE网络 | GKE网络/Calico |

---

## 配置检查与验证

### 10.1 配置检查命令

```bash
# 查看API Server参数
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep -A 100 'command:'

# 查看Controller Manager参数
kubectl get pods -n kube-system -l component=kube-controller-manager -o yaml | grep -A 50 'command:'

# 查看Scheduler参数
kubectl get pods -n kube-system -l component=kube-scheduler -o yaml | grep -A 30 'command:'

# 查看kubelet配置
kubectl get configmap -n kube-system kubelet-config-1.32 -o yaml
kubectl proxy &
curl -s http://localhost:8001/api/v1/nodes/<node>/proxy/configz | jq

# 查看kube-proxy配置
kubectl get configmap -n kube-system kube-proxy -o yaml

# 查看Feature Gates状态
kubectl get --raw /metrics | grep kubernetes_feature_enabled

# 检查etcd健康
kubectl exec -n kube-system etcd-master -- etcdctl endpoint health --cluster

# 检查集群信息
kubectl cluster-info
kubectl get componentstatuses  # 已弃用但仍可用
```

### 10.2 配置验证清单

| 检查项 | 命令 | 期望结果 |
|:---|:---|:---|
| API Server认证 | `kubectl auth can-i --list` | 返回权限列表 |
| RBAC启用 | `kubectl api-versions \| grep rbac` | 包含rbac.authorization.k8s.io/v1 |
| 审计日志 | `ls -la /var/log/kubernetes/audit.log` | 文件存在且有内容 |
| etcd加密 | `etcdctl get /registry/secrets/... --print-value-only` | 输出加密内容 |
| kubelet证书轮换 | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` | 有效期内 |
| kube-proxy模式 | `kubectl get cm kube-proxy -n kube-system -o yaml \| grep mode` | ipvs |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)