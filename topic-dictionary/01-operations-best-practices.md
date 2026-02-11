# 01 - Kubernetes 生产环境运维最佳实践字典

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler | **质量等级**: ⭐⭐⭐⭐⭐ 专家级

> **生产环境实战经验总结**: 基于万级节点集群运维经验，涵盖从故障处理到性能优化的全方位最佳实践

---

## 知识地图

**本文定位**: 这是一份面向生产环境的 Kubernetes 运维实战手册，涵盖集群配置、高可用、安全、监控、灾备、自动化、成本优化和多集群管理的完整知识体系。

**面向读者**:
- **初学者**: 了解生产环境运维的核心概念和为什么需要这些配置
- **中级工程师**: 掌握配置细节、调优参数和实施步骤
- **资深专家**: 深入架构设计、故障预防和大规模集群管理

**前置知识要求**:
- 基础: 了解 Kubernetes 核心概念（Pod、Deployment、Service）
- 进阶: 熟悉 YAML 配置语法和 kubectl 基本操作
- 专家: 了解分布式系统原理和 Linux 系统管理

**关联文件**:
- [05-concept-reference.md](05-concept-reference.md) - 核心概念参考（不熟悉术语时查阅）
- [06-cli-commands.md](06-cli-commands.md) - 运维命令速查
- [02-failure-patterns-analysis.md](02-failure-patterns-analysis.md) - 故障模式分析
- [03-performance-tuning-expert.md](03-performance-tuning-expert.md) - 性能调优详解
- [15-sli-slo-sla-engineering.md](15-sli-slo-sla-engineering.md) - SLI/SLO/SLA 工程

---

## 目录

- [知识地图](#知识地图)
- [1. 生产环境配置标准](#1-生产环境配置标准)
- [2. 高可用架构模式](#2-高可用架构模式)
- [3. 安全加固指南](#3-安全加固指南)
- [4. 监控告警最佳实践](#4-监控告警最佳实践)
- [5. 灾备恢复方案](#5-灾备恢复方案)
- [6. 自动化运维策略](#6-自动化运维策略)
- [7. 成本优化实践](#7-成本优化实践)
- [8. 多集群管理规范](#8-多集群管理规范)
- [9. 生产环境故障应急响应](#9-生产环境故障应急响应)
- [10. 生产环境安全最佳实践](#10-生产环境安全最佳实践)
- [11. 成本优化与资源管理](#11-成本优化与资源管理)
- [12. 变更管理与发布策略](#12-变更管理与发布策略)
- [关联阅读](#关联阅读)

---

## 1. 生产环境配置标准

### 概念解析

**一句话定义**: 生产环境配置标准是一组经过验证的 Kubernetes 集群参数和应用部署模板，确保系统在真实业务负载下稳定、安全、高效运行。

**类比**: 就像建造一栋大楼需要遵循建筑规范一样，生产环境配置标准就是 Kubernetes 集群的「建筑规范」——它告诉你每个参数应该设置多大、每个组件应该如何配置，才能保证系统不会在关键时刻「倒塌」。

**核心要点**:
- **API Server 并发控制**: 限制同时处理的请求数量，防止控制平面过载
- **etcd 存储配额**: 限制集群状态数据的大小，防止存储空间耗尽导致集群瘫痪
- **资源请求与限制**: 为每个容器设置 CPU/内存的下限（requests）和上限（limits），这是 K8s 调度和稳定性的基石
- **健康检查三件套**: livenessProbe（存活探针）、readinessProbe（就绪探针）、startupProbe（启动探针）缺一不可
- **网络策略**: 默认拒绝所有流量，按需开放，实现「零信任」网络

### 原理深入

**工作机制**:
- 当 Pod 被创建时，Kubernetes 调度器根据 `resources.requests` 找到有足够空闲资源的节点
- kubelet 根据 `resources.limits` 使用 Linux cgroups 限制容器的资源使用上限
- 如果容器内存使用超过 limits，会被 OOM Killer 杀死；如果 CPU 超限，会被 CFS 调度器限流
- 健康检查由 kubelet 定期执行：livenessProbe 失败会重启容器，readinessProbe 失败会从 Service 端点移除

**架构关系**:
```
用户请求 → API Server（并发限制）→ etcd（存储配额）
                ↓
           调度器（根据 requests 选节点）
                ↓
           kubelet（执行 limits、健康检查）
                ↓
           容器运行时（cgroups 资源隔离）
```

**关键参数解读**:
| 参数 | 作用 | 设置过小的后果 | 设置过大的后果 |
|-----|------|--------------|--------------|
| `requests.cpu` | 调度依据 | Pod 无法被调度 | 节点资源浪费 |
| `limits.memory` | 内存上限 | 应用 OOM | 挤占其他 Pod |
| `--max-pods` | 单节点 Pod 上限 | 需要更多节点 | 网络/存储压力大 |
| `--quota-backend-bytes` | etcd 存储上限 | 无法创建新资源 | 磁盘空间浪费 |

### 渐进式示例

**Level 1 - 基础用法（入门级 Deployment）**:
```yaml
# 最小化的生产 Deployment —— 适合第一次部署应用
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app          # 应用名称
  namespace: default     # 命名空间
spec:
  replicas: 2            # 运行 2 个副本，一个挂了另一个还能服务
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-app
        image: nginx:1.24  # 使用固定版本号，不要用 latest
        ports:
        - containerPort: 80
        resources:
          requests:         # 最少需要多少资源
            cpu: "100m"     # 0.1 核 CPU
            memory: "128Mi" # 128MB 内存
          limits:           # 最多允许用多少资源
            cpu: "500m"     # 0.5 核 CPU
            memory: "256Mi" # 256MB 内存
```

**Level 2 - 进阶配置（添加健康检查和安全设置）**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
  labels:
    app: my-app
    tier: backend
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1   # 滚动更新时最多 1 个 Pod 不可用
      maxSurge: 1          # 最多多创建 1 个 Pod
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-app
        image: registry.example.com/my-app:v1.0
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "250m"
            memory: "512Mi"
          limits:
            cpu: "1000m"
            memory: "1Gi"
        # 存活探针：检测应用是否还活着
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 30   # 启动后等 30 秒再检查
          periodSeconds: 10         # 每 10 秒检查一次
        # 就绪探针：检测应用是否准备好接收流量
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
        # 安全上下文：不使用 root 运行
        securityContext:
          runAsNonRoot: true
          runAsUser: 1000
          readOnlyRootFilesystem: true
      volumes:
      - name: tmp
        emptyDir: {}
```

**Level 3 - 生产最佳实践（企业级完整配置）**:

### 1.1 集群配置基线

| 配置项 | 推荐值 | 说明 | 风险等级 |
|-------|--------|------|---------|
| **API Server并发限制** | `--max-requests-inflight=400` | 控制并发请求数量 | 中 |
| | `--max-mutating-requests-inflight=200` | 写操作并发限制 | 中 |
| **etcd存储配额** | `--quota-backend-bytes=8GB` | 存储空间限制 | 高 |
| **事件保留时间** | `--event-ttl=1h` | 减少etcd存储压力 | 低 |
| **节点最大Pod数** | `--max-pods=110` | 标准环境配置 | 中 |
| | `--max-pods=500` | AWS云环境配置 | 高 |
| **镜像垃圾回收** | `--image-gc-high-threshold=85` | 高水位触发GC | 中 |
| | `--image-gc-low-threshold=80` | 低水位停止GC | 中 |

### 1.2 资源配置标准模板

```yaml
# ========== 生产环境Deployment标准配置 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-app-standard
  namespace: production
  labels:
    app: production-app
    tier: backend
    version: v1.0
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
  selector:
    matchLabels:
      app: production-app
  template:
    metadata:
      labels:
        app: production-app
        version: v1.0
      annotations:
        # 注入构建信息
        build.timestamp: "2026-02-05T10:30:00Z"
        build.commit: "a1b2c3d4"
    spec:
      # 优先级设置
      priorityClassName: high-priority
      
      # 节点选择策略
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - production-app
              topologyKey: kubernetes.io/hostname
      
      # 容忍污点
      tolerations:
      - key: dedicated
        operator: Equal
        value: production
        effect: NoSchedule
        
      containers:
      - name: app
        image: registry.example.com/app:v1.0
        imagePullPolicy: Always
        
        # 核心资源配置
        resources:
          requests:
            cpu: "250m"
            memory: "512Mi"
          limits:
            cpu: "1000m"
            memory: "1Gi"
            
        # 健康检查配置
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
          
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
          
        # 启动探针（K8s 1.18+）
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 30
          
        # 环境变量配置
        env:
        - name: LOG_LEVEL
          value: "INFO"
        - name: JAVA_OPTS
          value: "-Xmx768m -Xms512m -XX:+UseG1GC"
        - name: GOMEMLIMIT
          value: "800MiB"
          
        # 安全上下文
        securityContext:
          runAsNonRoot: true
          runAsUser: 1000
          readOnlyRootFilesystem: true
          allowPrivilegeEscalation: false
          
        # 挂载卷
        volumeMounts:
        - name: tmp-volume
          mountPath: /tmp
        - name: logs-volume
          mountPath: /var/log/app
          
      volumes:
      - name: tmp-volume
        emptyDir: {}
      - name: logs-volume
        persistentVolumeClaim:
          claimName: app-logs-pvc
```

### 1.3 网络策略标准

```yaml
# ========== 默认拒绝网络策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress

---
# ========== 允许DNS查询策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-access
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

---
# ========== 应用间通信策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: app-communication-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - protocol: TCP
      port: 8080
```

### 常见误区与最佳实践

**常见误区**:
1. **不设置 resources.requests/limits**: 不设置资源限制会导致「吵闹的邻居」问题——一个 Pod 占满所有资源，其他 Pod 全部受影响
2. **使用 `latest` 镜像标签**: 生产环境应始终使用固定版本标签（如 `v1.2.3`），`latest` 会导致部署不可预测
3. **livenessProbe 和 readinessProbe 使用相同端点**: 存活探针检查的是「进程是否还活着」，就绪探针检查的是「能否接收请求」，两者目的不同
4. **limits 设置为 requests 的 10 倍以上**: 过大的差距会导致节点超卖严重，当所有 Pod 同时用满 limits 时会触发 OOM
5. **忽略 NetworkPolicy**: 默认情况下所有 Pod 可以互相通信，在多租户环境下这是严重的安全隐患

**最佳实践**:
- **requests 和 limits 的比例**: 建议 limits 为 requests 的 2-4 倍，CPU 可以适当放宽（因为 CPU 是可压缩资源）
- **健康检查三件套**: startupProbe 保护慢启动应用 → readinessProbe 控制流量 → livenessProbe 保证自愈
- **先部署 NetworkPolicy 再部署应用**: 养成「默认拒绝、按需放开」的网络策略习惯
- **使用 PodDisruptionBudget**: 配合 Deployment 使用 PDB，确保滚动更新和节点维护时有最小可用副本数

**故障排查**:
```bash
# 查看 Pod 为什么没有被调度
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 Events

# 查看节点资源使用情况
kubectl top nodes

# 查看 Pod 资源使用情况
kubectl top pods -n <namespace> --sort-by=memory

# 检查 NetworkPolicy 是否生效
kubectl get networkpolicy -n <namespace> -o yaml
```

---

## 2. 高可用架构模式

### 概念解析

**一句话定义**: 高可用（HA）架构是指通过冗余设计消除单点故障，确保系统在部分组件失效时仍能持续提供服务的架构模式。

**类比**: 就像医院不会只有一台急救设备一样，高可用架构就是让 Kubernetes 集群的每个关键组件都有「备份」。当主设备坏了，备用设备能立刻顶上，用户完全感知不到。

**核心要点**:
- **控制平面 HA**: 至少 3 个 master 节点（etcd 需要奇数个节点来维持 Raft 共识）
- **应用层 HA**: 多副本 + 跨可用区分布（一个机房断电，其他机房的副本继续服务）
- **拓扑分布约束**: 使用 `topologySpreadConstraints` 确保 Pod 均匀分布在不同故障域
- **Pod 反亲和性**: 避免同一应用的多个副本调度到同一节点

### 原理深入

**工作机制**:
- **etcd Raft 共识**: 3 节点的 etcd 集群可以容忍 1 个节点故障（需要 (n/2)+1 个节点存活）；5 节点可容忍 2 个
- **API Server 负载均衡**: 多个 API Server 实例通过外部负载均衡器（如 HAProxy、云 LB）对外提供统一入口
- **调度器/控制器 Leader 选举**: 同一时刻只有一个 scheduler/controller-manager 活跃，其他实例处于待命状态

**架构关系**:
```
外部 LB（VIP）
  ├── API Server #1（Master-1，AZ-a）
  ├── API Server #2（Master-2，AZ-b）
  └── API Server #3（Master-3，AZ-c）
         ↕
  etcd 集群（3 或 5 节点，Raft 共识）

Worker 节点（跨 AZ 分布）
  ├── AZ-a: App Pod #1, #2
  ├── AZ-b: App Pod #3, #4
  └── AZ-c: App Pod #5, #6
```

**关键参数**:
| 参数 | 作用 | 推荐值 |
|-----|------|--------|
| `replicas` | 应用副本数 | 至少 3（跨 3 个 AZ）|
| `topologyKey` | 拓扑分布键 | `topology.kubernetes.io/zone` |
| `maxSkew` | 最大倾斜度 | 1（尽可能均匀）|
| `whenUnsatisfiable` | 不满足时策略 | `DoNotSchedule`（生产）或 `ScheduleAnyway`（开发）|

### 渐进式示例

**Level 1 - 基础用法（简单多副本）**:
```yaml
# 最简单的高可用：3 个副本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3  # 3 个副本，一个挂了还有 2 个
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-app
        image: my-app:v1.0
```

**Level 2 - 进阶配置（跨节点分布 + PDB）**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      # 反亲和性：不要把同一应用的 Pod 放在同一节点
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: ["my-app"]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: my-app
        image: my-app:v1.0
        resources:
          requests:
            cpu: "250m"
            memory: "512Mi"
---
# PodDisruptionBudget：确保至少 2 个副本可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: my-app
```

**Level 3 - 生产最佳实践（跨可用区 + 拓扑约束）**:

### 2.1 控制平面高可用

```yaml
# ========== 生产环境控制平面配置 ==========
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
metadata:
  name: production-cluster
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
etcd:
  local:
    extraArgs:
      listen-client-urls: "https://0.0.0.0:2379"
      advertise-client-urls: "https://ETCD_IP:2379"
      initial-cluster-token: "etcd-cluster-1"
      initial-cluster-state: "new"
      auto-compaction-mode: "periodic"
      auto-compaction-retention: "1"
    serverCertSANs:
    - "etcd01.example.com"
    - "etcd02.example.com"
    - "etcd03.example.com"
apiServer:
  certSANs:
  - "k8s-api.example.com"
  - "10.0.0.100"  # Load Balancer VIP
  extraArgs:
    authorization-mode: "Node,RBAC"
    enable-bootstrap-token-auth: "true"
    encryption-provider-config: "/etc/kubernetes/encryption-config.yaml"
controllerManager:
  extraArgs:
    cluster-signing-cert-file: "/etc/kubernetes/pki/ca.crt"
    cluster-signing-key-file: "/etc/kubernetes/pki/ca.key"
scheduler:
  extraArgs:
    bind-address: "0.0.0.0"
```

### 2.2 应用层面高可用

```yaml
# ========== 多区域部署策略 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: multi-region-app
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: multi-region-app
  template:
    metadata:
      labels:
        app: multi-region-app
    spec:
      affinity:
        # 跨可用区分布
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - multi-region-app
            topologyKey: topology.kubernetes.io/zone
            
        # 节点亲和性
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/region
                operator: In
                values:
                - us-west-1
                - us-east-1
                
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: multi-region-app
```

### 常见误区与最佳实践

**常见误区**:
1. **只有 2 个 master 节点**: etcd 使用 Raft 共识协议需要多数派存活，2 节点集群挂 1 个就无法选举 leader，还不如 1 个节点可靠
2. **多副本但全部在同一节点**: 节点宕机时所有副本同时丢失，必须配合反亲和性或拓扑约束
3. **使用 `preferredDuringScheduling` 代替 `required`**: 在生产环境中，跨 AZ 分布应使用 `required`（硬约束），否则调度器可能将所有 Pod 放在同一 AZ
4. **没有配置 PodDisruptionBudget**: 节点维护（drain）时可能同时驱逐所有 Pod，导致服务中断

**最佳实践**:
- **控制平面**: 3 或 5 个 master 节点，分布在不同可用区，前端配置负载均衡器
- **应用层**: replicas >= 3，配合 `topologySpreadConstraints` 实现跨 AZ 均匀分布
- **PDB**: 始终为关键服务配置 `PodDisruptionBudget`，`minAvailable` 设为 (replicas - 1)
- **etcd 备份**: 即使有 HA，也要定期备份 etcd，因为数据损坏可能同步到所有节点

**故障排查**:
```bash
# 检查 Pod 的拓扑分布情况
kubectl get pods -l app=my-app -o wide

# 查看 Pod 在哪些节点/AZ
kubectl get pods -l app=my-app -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}'

# 检查 PDB 状态
kubectl get pdb -n <namespace>

# 查看 etcd 集群健康状态
etcdctl --endpoints=https://127.0.0.1:2379 --cert=... --key=... --cacert=... endpoint health
```

---

## 3. 安全加固指南

### 概念解析

**一句话定义**: 安全加固是通过最小权限原则、网络隔离、运行时保护等手段，减少 Kubernetes 集群和应用的攻击面，防止未授权访问和数据泄露。

**类比**: 就像银行不会把金库大门敞开一样，安全加固就是给 Kubernetes 集群加上「多重锁」——从门禁（RBAC）、监控摄像（审计日志）、到防弹玻璃（容器隔离），每一层都是为了保护核心资产。

**核心要点**:
- **Pod 安全上下文**: 以非 root 用户运行、只读文件系统、禁止提权——这三项是安全基线
- **RBAC**: 基于角色的访问控制，每个用户/服务账户只拥有完成工作所需的最小权限
- **NetworkPolicy**: 网络层面的「防火墙」，控制 Pod 之间和 Pod 与外部的通信
- **Secret 管理**: 敏感信息（密码、密钥）不要硬编码，使用 Secret 或外部密钥管理系统
- **镜像安全**: 只使用受信任的镜像仓库，定期扫描漏洞

### 原理深入

**工作机制**:
- **SecurityContext** 映射到 Linux 内核能力：`runAsNonRoot` 对应 Linux UID 检查，`capabilities.drop` 对应 Linux capabilities，`readOnlyRootFilesystem` 对应 mount 的只读标志
- **RBAC** 由 API Server 在每次请求时执行：认证（你是谁）→ 授权（你能做什么）→ 准入控制（请求是否合规）
- **NetworkPolicy** 由 CNI 插件（Calico/Cilium）实现，翻译为 iptables/eBPF 规则

**架构关系**:
```
用户/ServiceAccount
  → API Server [认证 → RBAC 授权 → 准入控制]
     → kubelet [SecurityContext → Linux cgroups/namespaces]
        → 容器 [capabilities/seccomp/AppArmor]

Pod ←→ NetworkPolicy（CNI 插件实现）←→ Pod
```

### 渐进式示例

**Level 1 - 基础用法（最小安全配置）**:
```yaml
# 安全基线：非 root + 只读文件系统
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  containers:
  - name: app
    image: nginx:1.24
    securityContext:
      runAsNonRoot: true     # 不以 root 运行
      runAsUser: 1000        # 使用普通用户
      readOnlyRootFilesystem: true  # 只读根文件系统
    volumeMounts:
    - name: tmp
      mountPath: /tmp        # 需要写入的目录单独挂载
  volumes:
  - name: tmp
    emptyDir: {}
```

**Level 2 - 进阶配置（RBAC + 最小权限）**:
```yaml
# 为开发者创建只读权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: developer-readonly
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "services"]
  verbs: ["get", "list", "watch"]  # 只允许查看，不允许修改
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-readonly-binding
  namespace: production
subjects:
- kind: User
  name: dev@example.com
roleRef:
  kind: Role
  name: developer-readonly
  apiGroup: rbac.authorization.k8s.io
```

**Level 3 - 生产最佳实践（完整安全加固）**:

### 3.1 Pod安全标准

```yaml
# ========== 生产环境Pod安全配置 ==========
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
  namespace: production
spec:
  # 服务账户
  serviceAccountName: app-service-account
  
  # 安全上下文
  securityContext:
    runAsNonRoot: true
    runAsUser: 10001
    fsGroup: 2000
    supplementalGroups: [3000]
    
  containers:
  - name: app
    image: registry.example.com/secure-app:v1.0
    securityContext:
      # 容器安全设置
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 10001
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE  # 如需绑定低端口
        
    # 只读挂载重要目录
    volumeMounts:
    - name: tmpfs
      mountPath: /tmp
    - name: app-config
      mountPath: /config
      readOnly: true
      
  volumes:
  - name: tmpfs
    emptyDir:
      medium: Memory
  - name: app-config
    configMap:
      name: app-config
```

### 3.2 网络安全策略

```yaml
# ========== 生产网络安全策略 ==========
apiVersion: security.k8s.io/v1
kind: PodSecurityPolicy
metadata:
  name: production-psp
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
  - ALL
  volumes:
  - configMap
  - emptyDir
  - projected
  - secret
  - downwardAPI
  - persistentVolumeClaim
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  supplementalGroups:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  fsGroup:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  readOnlyRootFilesystem: true

---
# ========== RBAC最小权限原则 ==========
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-developer-role
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-developer-binding
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: app-developer-role
  apiGroup: rbac.authorization.k8s.io
```

### 常见误区与最佳实践

**常见误区**:
1. **以 root 运行容器**: 默认情况下容器以 root 运行，攻击者一旦突破容器即获得宿主机 root 权限
2. **给 ServiceAccount 过多权限**: 很多人图方便给 `default` ServiceAccount 赋予 `cluster-admin` 权限，这等于取消了所有安全防护
3. **忽略 NetworkPolicy**: 没有 NetworkPolicy 意味着任何 Pod 可以访问任何 Pod，一个被攻破的容器可以横向移动到整个集群
4. **Secret 存在 Git 仓库中**: 将密码、密钥等写入代码仓库是最常见的安全事故来源

**最佳实践**:
- **默认拒绝 + 白名单模式**: 先部署 `default-deny-all` NetworkPolicy，再逐个开放需要的通信路径
- **使用 Pod Security Standards**: K8s 1.25+ 使用 Pod Security Admission 替代已弃用的 PSP
- **定期轮转证书和密钥**: 建议 90 天轮转一次 TLS 证书，使用 cert-manager 自动化
- **启用审计日志**: 记录所有 API Server 请求，便于安全事件溯源

**故障排查**:
```bash
# 查看 Pod 的安全上下文
kubectl get pod <pod-name> -o jsonpath='{.spec.securityContext}' | jq .

# 查看当前用户权限
kubectl auth can-i --list

# 检查 RBAC 绑定
kubectl get rolebindings,clusterrolebindings -A | grep <user-or-sa>

# 测试网络策略是否生效
kubectl exec -it test-pod -- curl -v <target-service>:8080
```

---

## 4. 监控告警最佳实践

### 概念解析

**一句话定义**: 监控告警是通过持续采集集群和应用的运行指标，设定阈值规则，在异常发生时自动通知运维人员的系统。

**类比**: 就像汽车的仪表盘和警告灯一样——仪表盘（监控）让你随时看到时速、油量、水温，警告灯（告警）在出问题时主动提醒你。没有监控的集群就像闭着眼睛开车。

**核心要点**:
- **四大黄金信号**: 延迟（Latency）、流量（Traffic）、错误率（Errors）、饱和度（Saturation）——监控这四项就能覆盖 90% 的问题
- **分层监控**: 基础设施层（节点 CPU/内存/磁盘） → 平台层（API Server/etcd/kubelet） → 应用层（业务指标）
- **告警分级**: Critical（立即响应）→ Warning（计划处理）→ Info（知晓即可），避免告警疲劳
- **ServiceMonitor**: Prometheus Operator 的资源类型，声明式地配置指标采集目标

### 原理深入

**工作机制**:
- Prometheus 通过 pull 模式定期（默认 30s）从目标端点的 `/metrics` 路径抓取指标
- 告警规则在 Prometheus 内部以固定间隔评估 PromQL 表达式，满足条件持续 `for` 时间后触发
- Alertmanager 负责告警去重、分组、路由和通知（Slack/邮件/PagerDuty）
- ServiceMonitor 被 Prometheus Operator 监听，自动转化为 Prometheus 的 scrape 配置

**架构关系**:
```
应用 Pod (/metrics) ←── Prometheus（抓取 + 评估规则）
                              ↓ 触发告警
                        Alertmanager（去重 + 分组 + 路由）
                              ↓
                    Slack / PagerDuty / 邮件 / 企业微信
```

### 渐进式示例

**Level 1 - 基础用法（一条简单告警规则）**:
```yaml
# 最基本的告警：Pod 挂了
groups:
- name: basic.rules
  rules:
  - alert: PodDown
    expr: up == 0         # 目标不可达
    for: 2m               # 持续 2 分钟
    labels:
      severity: critical
    annotations:
      summary: "{{ $labels.instance }} 不可用"
```

**Level 2 - 进阶配置（多维度监控）**:
```yaml
groups:
- name: app.rules
  rules:
  # 错误率监控
  - alert: HighErrorRate
    expr: rate(http_requests_total{status=~"5.."}[5m]) / rate(http_requests_total[5m]) > 0.05
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "{{ $labels.service }} 5xx 错误率超过 5%"
  # 延迟监控
  - alert: HighLatency
    expr: histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "{{ $labels.service }} P99 延迟超过 1 秒"
```

**Level 3 - 生产最佳实践（完整监控体系）**:

### 4.1 核心监控指标

```yaml
# ========== Prometheus核心告警规则 ==========
groups:
- name: kubernetes.system.rules
  rules:
  # API Server监控
  - alert: APIServerDown
    expr: up{job="apiserver"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "API Server实例 {{ $labels.instance }} 不可用"
      description: "API Server已经宕机超过2分钟，请立即处理"

  - alert: APIServerLatencyHigh
    expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "API Server响应延迟过高"
      description: "99th百分位响应时间超过1秒"

  # etcd监控
  - alert: EtcdNoLeader
    expr: etcd_server_has_leader == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "etcd集群无领导者"
      description: "etcd集群已失去领导者超过1分钟"

  - alert: EtcdHighFsyncDuration
    expr: histogram_quantile(0.99, etcd_disk_backend_commit_duration_seconds_bucket) > 0.5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd磁盘同步延迟高"
      description: "99th百分位fsync延迟超过500ms"

  # 节点监控
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} 不可用"
      description: "节点已处于NotReady状态超过5分钟"

  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "节点 {{ $labels.node }} 内存压力大"
      description: "节点内存使用率达到警告阈值"

  # Pod监控
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[15m]) > 0.2
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"
      description: "Pod重启频率超过每分钟0.2次"

  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="true"} == 0
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 未就绪"
      description: "Pod长时间未进入Ready状态"
```

### 4.2 应用监控配置

```yaml
# ========== ServiceMonitor配置 ==========
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-monitor
  namespace: monitoring
  labels:
    team: sre
spec:
  selector:
    matchLabels:
      app: production-app
  namespaceSelector:
    matchNames:
    - production
  endpoints:
  - port: http-metrics
    interval: 30s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
    - sourceLabels: [__meta_kubernetes_service_name]
      targetLabel: service
    
  # 自定义指标采集
  - port: http-app
    interval: 60s
    path: /actuator/prometheus
    params:
      include: ["jvm.memory.used", "http.server.requests"]
```

### 常见误区与最佳实践

**常见误区**:
1. **告警阈值照抄网上配置**: 每个集群负载特征不同，阈值必须根据自身基线调整
2. **所有告警都设为 Critical**: 导致告警疲劳，运维人员对真正的紧急问题麻木
3. **只监控基础设施，忽略业务指标**: CPU/内存正常不代表业务正常，必须同时监控四大黄金信号
4. **不设 `for` 持续时间**: 瞬间波动会产生大量无意义告警

**最佳实践**:
- **告警金字塔**: Critical（<5 条）→ Warning（<20 条）→ Info（不限），确保 Critical 告警一定有人响应
- **告警要有 Runbook**: 每条告警都应附带处理文档链接，告诉值班人员该怎么做
- **使用多窗口告警**: 短窗口（5m）检测突发故障，长窗口（1h）检测趋势劣化
- **定期审查告警**: 每月清理不再有效的告警规则，保持告警系统「干净」

**故障排查**:
```bash
# 查看 Prometheus 是否正常抓取目标
kubectl port-forward -n monitoring svc/prometheus 9090:9090
# 浏览器访问 http://localhost:9090/targets

# 查看当前活跃告警
kubectl exec -n monitoring prometheus-0 -- promtool query instant http://localhost:9090 'ALERTS{alertstate="firing"}'

# 检查 Alertmanager 路由配置
kubectl get secret -n monitoring alertmanager-config -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d
```

---

## 5. 灾备恢复方案

### 概念解析

**一句话定义**: 灾备恢复（DR）是一套确保在集群或数据中心发生灾难性故障时，能够在可接受的时间内恢复服务和数据的策略与流程。

**类比**: 就像我们会备份手机里的照片到云端一样，灾备恢复就是给整个 Kubernetes 集群做「云端备份」。即使手机丢了（集群挂了），你的照片（数据和配置）还在，可以在新手机（新集群）上恢复。

**核心要点**:
- **RPO（恢复点目标）**: 最多能接受丢失多长时间的数据？决定了备份频率
- **RTO（恢复时间目标）**: 从故障到恢复服务最多能接受多长时间？决定了恢复方案的复杂度
- **etcd 是集群的「大脑」**: 所有集群状态都存储在 etcd 中，etcd 备份是灾备的核心
- **Velero**: Kubernetes 生态中最流行的备份恢复工具，支持资源定义和持久卷的备份

### 原理深入

**工作机制**:
- **etcd 快照备份**: `etcdctl snapshot save` 将 etcd 的 BoltDB 数据库导出为快照文件，包含集群所有资源定义
- **Velero 备份流程**: 调用 API Server 导出资源定义（JSON/YAML） → 调用 CSI/云提供商 API 创建卷快照 → 上传到对象存储（S3/GCS/MinIO）
- **恢复流程**: 从对象存储下载备份 → 重新创建资源定义 → 恢复持久卷数据

**关键参数**:
| 参数 | 说明 | 推荐值 |
|-----|------|--------|
| 备份频率 | etcd 快照频率 | 每小时 1 次（关键环境每 15 分钟）|
| 备份保留 | 保留多长时间的备份 | 至少 7 天，关键数据 30 天 |
| 备份验证 | 多久验证一次备份可恢复 | 每周至少 1 次 |
| 跨区域复制 | 备份是否复制到其他区域 | 生产环境必须开启 |

### 渐进式示例

**Level 1 - 基础用法（手动 etcd 快照）**:
```bash
# 最简单的 etcd 备份：一条命令
ETCDCTL_API=3 etcdctl snapshot save /tmp/etcd-backup.db \
  --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt

# 验证备份文件
etcdctl snapshot status /tmp/etcd-backup.db --write-out=table
```

**Level 2 - 进阶配置（Velero 定时备份）**:
```yaml
# 安装 Velero 后创建定时备份
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  template:
    includedNamespaces: ["production"]
    snapshotVolumes: true
    ttl: 168h  # 保留 7 天
```

**Level 3 - 生产最佳实践（完整灾备方案）**:

### 5.1 etcd备份策略

```bash
#!/bin/bash
# ========== etcd备份脚本 ==========
set -euo pipefail

BACKUP_DIR="/backup/etcd"
DATE=$(date +%Y%m%d_%H%M%S)
ETCDCTL_API=3

# 创建备份目录
mkdir -p ${BACKUP_DIR}/${DATE}

# 执行快照备份
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  snapshot save ${BACKUP_DIR}/${DATE}/etcd-snapshot.db

# 验证备份完整性
etcdctl --write-out=table snapshot status ${BACKUP_DIR}/${DATE}/etcd-snapshot.db

# 压缩备份文件
tar -czf ${BACKUP_DIR}/${DATE}.tar.gz -C ${BACKUP_DIR} ${DATE}

# 清理旧备份（保留最近7天）
find ${BACKUP_DIR} -name "*.tar.gz" -mtime +7 -delete
find ${BACKUP_DIR} -mindepth 1 -maxdepth 1 -type d -empty -delete

echo "etcd backup completed: ${BACKUP_DIR}/${DATE}.tar.gz"
```

### 5.2 应用数据备份

```yaml
# ========== Velero备份配置 ==========
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  template:
    includedNamespaces:
    - production
    - staging
    excludedNamespaces:
    - kube-system
    - monitoring
    includedResources:
    - deployments
    - services
    - configmaps
    - secrets
    - persistentvolumeclaims
    labelSelector:
      matchLabels:
        backup: enabled
    snapshotVolumes: true
    ttl: 168h  # 保留7天

---
# ========== 灾难恢复演练配置 ==========
apiVersion: velero.io/v1
kind: Restore
metadata:
  name: dr-test-restore
  namespace: velero
spec:
  backupName: daily-backup-20260205020000
  includedNamespaces:
  - production-dr-test
  restorePVs: true
  preserveNodePorts: true
```

### 常见误区与最佳实践

**常见误区**:
1. **只备份 etcd 不备份应用数据**: etcd 只存储集群资源定义，数据库等持久卷中的数据需要单独备份
2. **从不测试恢复流程**: 没有经过验证的备份等于没有备份，定期执行恢复演练至关重要
3. **备份和源数据在同一个区域/存储**: 整个区域故障时备份也会丢失，必须跨区域复制

**最佳实践**:
- **3-2-1 备份原则**: 3 份副本、2 种存储介质、1 份异地备份
- **自动化备份 + 自动化验证**: 使用 CronJob 定时备份，配合自动化恢复测试
- **备份加密**: 备份文件包含敏感信息（Secret），传输和存储时必须加密
- **分级备份策略**: 关键命名空间每小时备份，普通命名空间每天备份

**故障排查**:
```bash
# 查看 Velero 备份状态
velero backup get
velero backup describe <backup-name> --details

# 查看备份日志
velero backup logs <backup-name>

# 测试恢复（恢复到不同命名空间）
velero restore create --from-backup <backup-name> --namespace-mappings production:dr-test
```

---

## 6. 自动化运维策略

### 概念解析

**一句话定义**: 自动化运维是通过 GitOps、自动扩缩容等技术手段，将人工操作转化为代码和策略驱动的自动化流程，减少人为错误并提高效率。

**类比**: 就像从手动挡汽车升级到自动挡一样——GitOps 就是「自动变速箱」（代码变更自动部署），HPA 就是「自适应巡航」（负载变化自动伸缩），让运维从「手忙脚乱」变成「从容应对」。

**核心要点**:
- **GitOps**: 以 Git 仓库为单一事实来源，所有变更通过 Git commit 触发，实现「代码即基础设施」
- **HPA（水平 Pod 自动扩缩器）**: 根据 CPU/内存/自定义指标自动调整 Pod 副本数
- **VPA（垂直 Pod 自动扩缩器）**: 自动调整单个 Pod 的 CPU/内存 requests 和 limits
- **ArgoCD**: 最流行的 GitOps 工具，持续监听 Git 仓库变化并自动同步到集群

### 原理深入

**工作机制**:
- **ArgoCD 同步循环**: 每 3 分钟对比 Git 仓库中的期望状态和集群中的实际状态，发现差异时自动或手动同步
- **HPA 控制循环**: 每 15 秒查询 Metrics Server 获取当前 Pod 的资源使用率，根据目标利用率计算期望副本数，通过 Deployment 的 scale 子资源调整
- **VPA 推荐器**: 分析 Pod 历史资源使用数据，计算最优的 requests/limits 值，可自动应用或仅推荐

**关键参数**:
| 参数 | 说明 | 推荐值 |
|-----|------|--------|
| HPA `averageUtilization` | 目标利用率 | CPU 70%，内存 80% |
| HPA `stabilizationWindowSeconds` | 缩容稳定窗口 | 300s（防止频繁缩容）|
| HPA `scaleUp.periodSeconds` | 扩容周期 | 60s |
| ArgoCD `selfHeal` | 自动修复漂移 | 生产环境开启 |

### 渐进式示例

**Level 1 - 基础用法（简单 HPA）**:
```yaml
# 最简单的自动扩缩容：根据 CPU 使用率
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2    # 最少 2 个副本
  maxReplicas: 10   # 最多 10 个副本
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70  # CPU 超过 70% 就扩容
```

**Level 2 - 进阶配置（HPA + 缩容保护）**:
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容前等 5 分钟，避免波动
      policies:
      - type: Percent
        value: 10           # 每次最多缩 10%
        periodSeconds: 60
    scaleUp:
      policies:
      - type: Percent
        value: 50            # 每次最多扩 50%
        periodSeconds: 60
```

**Level 3 - 生产最佳实践（GitOps + 自动扩缩容）**:

### 6.1 GitOps流水线

```yaml
# ========== ArgoCD应用配置 ==========
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: production-app
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/company/production-app.git
    targetRevision: HEAD
    path: k8s/overlays/production
    helm:
      valueFiles:
      - values-production.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
    syncOptions:
    - CreateNamespace=true
    - PruneLast=true

---
# ========== 多环境配置管理 ==========
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-environment-config
  namespace: production
data:
  # 生产环境特定配置
  DATABASE_URL: "postgresql://prod-db:5432/app"
  LOG_LEVEL: "WARN"
  CACHE_TTL: "300"
  ENABLE_DEBUG: "false"
  MAX_CONNECTIONS: "100"
  
  # 安全配置
  TLS_MIN_VERSION: "TLS1.2"
  HSTS_MAX_AGE: "31536000"
  CORS_ALLOWED_ORIGINS: "https://app.example.com"
```

### 6.2 自动扩缩容配置

```yaml
# ========== HPA高级配置 ==========
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "100"
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      selectPolicy: Max

---
# ========== VPA配置 ==========
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  updatePolicy:
    updateMode: "Auto"
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 256Mi
      maxAllowed:
        cpu: 2000m
        memory: 4Gi
      controlledResources: ["cpu", "memory"]
```

### 常见误区与最佳实践

**常见误区**:
1. **HPA 和 VPA 同时管理同一资源**: HPA 调副本数，VPA 调资源请求，两者同时作用于 CPU 会冲突。VPA 应设为 `updateMode: "Off"` 仅做推荐
2. **ArgoCD selfHeal 开启但不理解后果**: 手动在集群中做的临时修改会被 ArgoCD 自动回滚，要临时变更应先暂停同步
3. **HPA minReplicas 设为 1**: 高可用要求至少 2 个副本，minReplicas 应 >= 2
4. **不设置缩容稳定窗口**: 流量波动时 Pod 频繁创建销毁，影响服务稳定性

**最佳实践**:
- **GitOps 单一事实来源**: 禁止 `kubectl apply/edit` 直接修改生产集群，所有变更通过 Git PR
- **HPA + Cluster Autoscaler 联动**: HPA 扩 Pod，Cluster Autoscaler 扩节点，形成完整弹性链
- **配合 PDB 使用**: 自动扩缩容过程中 PDB 保证最小可用副本
- **监控扩缩容事件**: 创建告警规则监控 HPA 是否频繁触碰 maxReplicas（说明需要调整上限）

**故障排查**:
```bash
# 查看 HPA 状态和指标
kubectl get hpa -n <namespace> -o wide
kubectl describe hpa <hpa-name> -n <namespace>

# 查看 ArgoCD 同步状态
argocd app get <app-name>
argocd app diff <app-name>

# 查看 VPA 推荐值
kubectl get vpa -n <namespace> -o yaml
```

---

## 7. 成本优化实践

### 概念解析

**一句话定义**: 成本优化是在保证服务质量的前提下，通过合理配置资源、使用弹性计费、清理浪费等手段，最大限度降低 Kubernetes 集群运行成本。

**类比**: 就像家庭理财一样——你不会让每个房间都开着暖气（资源浪费），也不会在用电高峰期洗衣服（错峰调度），更不会买了会员却不用（闲置资源）。成本优化就是让集群的每一分钱都花在刀刃上。

**核心要点**:
- **资源 Right-sizing**: requests/limits 与实际使用量匹配，避免过度申请
- **Spot/抢占实例**: 利用云厂商的闲置资源，成本降低 50-80%，适合可中断的工作负载
- **自动扩缩容**: 根据负载动态调整节点和 Pod 数量，避免为峰值永久买单
- **存储生命周期**: 冷数据迁移到低成本存储，定期清理未使用的 PV
- **成本可观测性**: 按命名空间/团队/项目分摊成本，让团队对资源使用负责

### 原理深入

**工作机制**:
- **Requests 决定成本**: 云厂商节点费用固定，Pod 的 requests 决定了节点利用率——如果 requests 远大于实际使用，你在为空气买单
- **Spot 实例机制**: 云厂商在闲置容量上提供大幅折扣，但可能随时回收（2 分钟通知），适合无状态应用和批处理任务
- **Cluster Autoscaler**: 当 Pod 因资源不足无法调度时自动添加节点，当节点利用率长期低于阈值时自动移除

### 渐进式示例

**Level 1 - 基础用法（合理设置资源请求）**:
```yaml
# 根据实际使用量设置 resources（不要盲目设大值）
# 先用 kubectl top 观察实际使用，再设置
containers:
- name: app
  resources:
    requests:
      cpu: "100m"     # 实际使用 80m，requests 设为 100m（留 20% 余量）
      memory: "256Mi"  # 实际使用 200Mi，requests 设为 256Mi
    limits:
      cpu: "500m"     # 允许突发到 0.5 核
      memory: "512Mi"  # 内存上限 512Mi
```

**Level 2 - 进阶配置（Spot 实例 + 容忍）**:
```yaml
# 应用配置容忍 Spot 节点
spec:
  tolerations:
  - key: "spot-instance"
    operator: "Equal"
    value: "true"
    effect: "NoSchedule"
  affinity:
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 80
        preference:
          matchExpressions:
          - key: "node.kubernetes.io/lifecycle"
            operator: In
            values: ["spot"]  # 优先使用 Spot 节点（便宜）
      - weight: 20
        preference:
          matchExpressions:
          - key: "node.kubernetes.io/lifecycle"
            operator: In
            values: ["on-demand"]  # Spot 不够时用按需节点
```

**Level 3 - 生产最佳实践（完整成本优化方案）**:

### 7.1 资源优化策略

```yaml
# ========== 成本优化资源配置 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cost-optimized-app
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      # Spot实例容忍
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: NoSchedule
        
      # 节点亲和性 - 优先使用成本较低的实例
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - t3.medium
                - t3.large
          - weight: 50
            preference:
              matchExpressions:
              - key: cloud.google.com/gke-preemptible
                operator: In
                values:
                - "true"
                
      containers:
      - name: app
        image: app:v1.0
        resources:
          requests:
            # 基于实际使用量精确配置
            cpu: "150m"
            memory: "384Mi"
          limits:
            # 合理的上限，避免浪费
            cpu: "500m"
            memory: "768Mi"
            
        # 应用层优化
        env:
        - name: JAVA_OPTS
          value: "-Xmx640m -Xms384m -XX:MaxRAMPercentage=80.0"
        - name: GOMEMLIMIT
          value: "680MiB"
```

### 7.2 成本监控告警

```yaml
# ========== 成本监控告警规则 ==========
groups:
- name: cost.monitoring.rules
  rules:
  - alert: HighResourceUtilizationCost
    expr: avg(rate(container_cpu_usage_seconds_total[1h])) by (namespace) * 100 > 80
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "命名空间 {{ $labels.namespace }} CPU使用率过高"
      description: "平均CPU使用率超过80%，可能存在资源配置过度"

  - alert: MemoryOverProvisioned
    expr: (kube_pod_container_resource_limits_memory_bytes - container_memory_working_set_bytes) / kube_pod_container_resource_limits_memory_bytes * 100 > 50
    for: 6h
    labels:
      severity: info
    annotations:
      summary: "内存过度配置"
      description: "Pod内存预留量超过实际使用量50%以上"

  - alert: UnusedPersistentVolumes
    expr: kube_persistentvolume_status_phase{phase="Available"} == 1
    for: 24h
    labels:
      severity: warning
    annotations:
      summary: "存在未使用的持久卷"
      description: "检测到闲置的PV，建议清理以降低成本"
```

### 常见误区与最佳实践

**常见误区**:
1. **Spot 实例运行有状态服务**: 数据库、消息队列等有状态服务不适合 Spot 实例，节点回收会导致数据丢失
2. **所有 Pod limits 设为 requests 的 10 倍**: 过度超卖导致节点 OOM，应根据应用特性设置合理的 limits/requests 比
3. **删除「看起来没用」的 PV**: 有些 PV 可能被暂停的 StatefulSet 使用，删除前必须确认无引用
4. **只看 CPU/内存，忽略网络和存储成本**: 跨 AZ 数据传输和高 IOPS 存储往往是隐形成本大头

**最佳实践**:
- **使用 VPA 推荐值调整 requests**: VPA `updateMode: "Off"` 模式下只提供建议值，手动审核后再应用
- **标签化成本归属**: 使用 `cost-center`、`team`、`project` 标签标记 Pod，配合 Kubecost 等工具按维度分析
- **Spot + On-Demand 混合**: 关键服务用 On-Demand，可中断任务用 Spot，比例建议 30:70
- **定期清理**: 每月清理未使用的 PV、过期的 CronJob、废弃的 ConfigMap/Secret

**故障排查**:
```bash
# 查看集群资源利用率
kubectl top nodes
kubectl top pods -A --sort-by=cpu | head -20

# 查找资源浪费（requests 远大于实际使用）
kubectl top pods -A --no-headers | awk '{print $1, $2, $3, $4}'

# 查找未绑定的 PV（可能是浪费）
kubectl get pv | grep Available

# 查看命名空间资源配额使用情况
kubectl describe resourcequota -n <namespace>
```

---

## 8. 多集群管理规范

### 概念解析

**一句话定义**: 多集群管理是通过统一的控制平面和标准化流程，同时管理多个 Kubernetes 集群，实现跨集群的服务发现、负载均衡和统一运维。

**类比**: 就像一家连锁餐厅管理多个分店一样——总部（管理集群）统一制定菜单和标准（策略和配置），每个分店（业务集群）根据当地情况运营，但总部能看到所有分店的经营状况（统一监控）。

**核心要点**:
- **Cluster API**: 用 Kubernetes 管理 Kubernetes，声明式地创建、升级、删除集群
- **多集群服务发现**: 通过 ServiceExport/ServiceImport 让服务跨集群可达
- **统一监控**: Thanos/Cortex 聚合多个集群的 Prometheus 数据，提供全局视图
- **策略一致性**: 使用 Kyverno/OPA 确保所有集群遵循相同的安全和配置策略

### 原理深入

**工作机制**:
- **Cluster API**: 管理集群运行 Cluster API 控制器，通过 Infrastructure Provider（AWS/Azure/GCP）调用云 API 创建节点和网络资源
- **ServiceExport/Import**: MCS（Multi-Cluster Services）API 标准，集群 A 导出服务后，集群 B 可以通过相同名称访问该服务
- **Thanos**: Prometheus Sidecar 将数据上传到对象存储，Thanos Query 组件跨集群查询并去重

### 渐进式示例

**Level 1 - 基础用法（多 kubeconfig 管理）**:
```bash
# 最简单的多集群管理：使用 kubeconfig 切换上下文
# 查看所有集群上下文
kubectl config get-contexts

# 切换到指定集群
kubectl config use-context production-cluster

# 在指定集群执行命令（不切换上下文）
kubectl --context=staging-cluster get pods
```

**Level 2 - 进阶配置（跨集群服务发现）**:
```yaml
# 集群 A：导出服务
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: my-service
  namespace: production
---
# 集群 B：导入服务（可以像本地服务一样访问）
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceImport
metadata:
  name: my-service
  namespace: production
spec:
  type: ClusterSetIP
  ports:
  - port: 80
    protocol: TCP
```

**Level 3 - 生产最佳实践（Cluster API + 统一监控）**:

### 8.1 集群联邦配置

```yaml
# ========== Cluster API配置 ==========
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: production-cluster-us-west
  namespace: capi-system
spec:
  clusterNetwork:
    services:
      cidrBlocks: ["10.128.0.0/12"]
    pods:
      cidrBlocks: ["10.0.0.0/8"]
    serviceDomain: "cluster.local"
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: AWSCluster
    name: production-cluster-us-west
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: production-cluster-us-west-control-plane

---
# ========== 多集群服务发现 ==========
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: global-service
  namespace: production
spec: {}

---
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceImport
metadata:
  name: global-service
  namespace: production
spec:
  type: ClusterSetIP
  ports:
  - name: http
    protocol: TCP
    port: 80
```

### 8.2 统一监控配置

```yaml
# ========== Thanos多集群监控 ==========
apiVersion: v1
kind: Service
metadata:
  name: thanos-sidecar
  namespace: monitoring
  labels:
    app: thanos-sidecar
spec:
  ports:
  - name: grpc
    port: 10901
    targetPort: 10901
  - name: http
    port: 10902
    targetPort: 10902
  clusterIP: None

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: thanos-query
  namespace: monitoring
spec:
  serviceName: thanos-query
  replicas: 2
  selector:
    matchLabels:
      app: thanos-query
  template:
    metadata:
      labels:
        app: thanos-query
    spec:
      containers:
      - name: thanos-query
        image: quay.io/thanos/thanos:v0.32.0
        args:
        - query
        - --grpc-address=0.0.0.0:10901
        - --http-address=0.0.0.0:10902
        - --store=dnssrv+_grpc._tcp.thanos-sidecar.monitoring.svc.cluster.local
        - --query.replica-label=replica
        ports:
        - name: grpc
          containerPort: 10901
        - name: http
          containerPort: 10902
```

### 常见误区与最佳实践

**常见误区**:
1. **管理集群和业务集群混用**: 管理集群（运行 Cluster API/ArgoCD）应独立于业务集群，避免业务故障影响管理能力
2. **多集群网络直接打通**: 全互联网络增加攻击面，应按需建立跨集群通信通道
3. **各集群配置不一致**: 手动维护多集群导致「配置漂移」，应使用 GitOps 统一管理

**最佳实践**:
- **Hub-Spoke 模型**: 一个管理集群（Hub）管理多个业务集群（Spoke），职责清晰
- **统一策略下发**: 使用 Kyverno/OPA Gatekeeper 在所有集群强制执行安全策略
- **跨集群可观测性**: Thanos/Cortex 聚合全局指标，Loki 聚合全局日志
- **集群标准化**: 使用 Cluster API + GitOps 确保新集群与现有集群配置一致

**故障排查**:
```bash
# 查看 Cluster API 管理的集群状态
kubectl get clusters -A
kubectl describe cluster <cluster-name>

# 检查跨集群服务发现
kubectl get serviceexports -A
kubectl get serviceimports -A

# Thanos 查询跨集群指标
thanos query --store=<sidecar-grpc-address>
```

---

## 9. 生产环境故障应急响应

### 概念解析

**一句话定义**: 故障应急响应是一套标准化流程，用于在生产环境发生故障时快速定位问题、恢复服务、通知相关人员、并事后复盘改进。

**类比**: 就像消防队的应急预案一样——火灾（故障）发生时，不是所有人都去灭火，而是有人指挥（Incident Commander）、有人灭火（工程师排查）、有人疏散人群（通知客户）、事后有人调查火因（复盘）。

**核心要点**:
- **分级响应**: P0（核心中断，5 分钟响应）到 P3（优化建议，次日处理），避免小问题占用大量人力
- **Incident Commander**: 每次 P0/P1 故障指定一个「指挥官」，协调所有资源，统一决策
- **时间线记录**: 从故障发现到恢复的每一步操作都记录时间和操作人，复盘时还原现场
- **故障复盘（Postmortem）**: 不追责，只找根因和改进措施，建立「无责文化」

### 原理深入

**工作机制**:
- **故障检测**: 监控系统自动告警 → 值班人员确认 → 触发应急流程
- **故障升级**: P2 超过 2 小时未解决 → 升级为 P1 → P1 超过 30 分钟升级为 P0
- **War Room**: P0/P1 故障时开启专用通信频道，所有相关人员实时协作
- **自动化恢复**: 预置的 Runbook 脚本可以自动执行常见故障的恢复操作

### 渐进式示例

**Level 1 - 基础用法（简单的故障检查步骤）**:
```bash
# 故障发生时的第一步：快速检查集群健康
# 1. 检查节点状态
kubectl get nodes

# 2. 检查异常 Pod
kubectl get pods -A | grep -v Running | grep -v Completed

# 3. 查看最近事件
kubectl get events -A --sort-by='.lastTimestamp' | tail -20

# 4. 检查系统组件
kubectl get pods -n kube-system
```

**Level 2 - 进阶配置（结构化故障记录）**:
```bash
# 故障发生时创建结构化记录
INCIDENT_ID="INC-$(date +%Y%m%d-%H%M%S)"
mkdir -p /tmp/incidents/${INCIDENT_ID}

# 收集集群快照
kubectl cluster-info dump --output-directory=/tmp/incidents/${INCIDENT_ID}/
kubectl get events -A --sort-by='.lastTimestamp' > /tmp/incidents/${INCIDENT_ID}/events.txt
kubectl top nodes > /tmp/incidents/${INCIDENT_ID}/node-resources.txt
kubectl top pods -A > /tmp/incidents/${INCIDENT_ID}/pod-resources.txt

echo "故障快照已保存到: /tmp/incidents/${INCIDENT_ID}/"
```

**Level 3 - 生产最佳实践（完整应急响应体系）**:

### 9.1 故障分级响应机制

| 故障等级 | 响应时间 | 通知范围 | 处理流程 | 记录要求 |
|---------|---------|---------|---------|---------|
| **P0 - 核心服务中断** | 5分钟内响应 | 全体技术团队+管理层 | 立即组建应急小组，启动应急预案 | 详细故障时间线记录 |
| **P1 - 重要功能异常** | 30分钟内响应 | 相关技术团队 | 指定负责人处理，定期同步进展 | 故障分析报告必填 |
| **P2 - 一般性问题** | 2小时内响应 | 对应模块负责人 | 按正常流程处理，纳入周报 | 问题跟踪记录 |
| **P3 - 优化建议类** | 下一工作日处理 | 相关人员 | 纳入改进计划 | 需求池管理 |

### 9.2 应急响应标准操作程序(SOP)

```bash
#!/bin/bash
# ========== 生产环境应急响应脚本 ==========
set -euo pipefail

INCIDENT_ID=$(date +%Y%m%d_%H%M%S)_${RANDOM}
INCIDENT_DIR="/var/incidents/${INCIDENT_ID}"
mkdir -p ${INCIDENT_DIR}

log_incident() {
    local severity=$1
    local component=$2
    local description=$3
    
    echo "$(date '+%Y-%m-%d %H:%M:%S') [${severity}] ${component}: ${description}" | \
        tee -a ${INCIDENT_DIR}/incident.log
    
    # 发送告警通知
    case ${severity} in
        "P0")
            # 紧急通知所有相关人员
            send_emergency_alert "${description}"
            ;;
        "P1")
            # 通知相关技术团队
            send_team_alert "${component}" "${description}"
            ;;
    esac
}

# 故障诊断函数
diagnose_cluster_health() {
    echo "=== 集群健康状态诊断 ===" > ${INCIDENT_DIR}/diagnosis.txt
    
    # 检查控制平面状态
    kubectl get componentstatuses >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查节点状态
    kubectl get nodes -o wide >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查关键系统Pod状态
    kubectl get pods -n kube-system >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查事件日志
    kubectl get events --sort-by='.lastTimestamp' -A | tail -20 >> ${INCIDENT_DIR}/diagnosis.txt
}

# 自动化恢复尝试
attempt_auto_recovery() {
    local component=$1
    
    case ${component} in
        "coredns")
            echo "尝试重启CoreDNS..."
            kubectl rollout restart deployment coredns -n kube-system
            ;;
        "kube-proxy")
            echo "尝试重启kube-proxy DaemonSet..."
            kubectl delete pods -n kube-system -l k8s-app=kube-proxy
            ;;
        *)
            echo "组件${component}暂无自动恢复策略"
            return 1
            ;;
    esac
}

# 使用示例
# log_incident "P0" "API Server" "API Server响应超时，影响集群管理"
# diagnose_cluster_health
# attempt_auto_recovery "coredns"
```

### 9.3 故障复盘与改进

```yaml
# ========== 故障复盘模板 ==========
apiVersion: incident.review/v1
kind: PostMortemReport
metadata:
  name: incident-${INCIDENT_ID}
spec:
  incidentDetails:
    startTime: "2026-02-05T14:30:00Z"
    endTime: "2026-02-05T15:45:00Z"
    duration: "1h15m"
    severity: "P0"
    affectedServices:
    - name: user-api-service
      impact: "50%请求失败"
    - name: order-processing
      impact: "完全不可用"
  
  timeline:
  - time: "14:30"
    event: "监控系统告警：API Server响应时间超过阈值"
    actor: "Prometheus Alertmanager"
  - time: "14:32"
    event: "值班工程师确认问题并通知SRE团队"
    actor: "on-call engineer"
  - time: "14:35"
    event: "启动应急响应流程，创建故障工单"
    actor: "incident commander"
  - time: "14:40"
    event: "初步诊断发现etcd集群出现网络分区"
    actor: "SRE team"
  - time: "15:10"
    event: "执行etcd集群恢复操作"
    actor: "database specialist"
  - time: "15:30"
    event: "服务恢复正常，开始验证"
    actor: "QA team"
  - time: "15:45"
    event: "确认服务稳定，关闭故障工单"
    actor: "incident commander"
  
  rootCauseAnalysis:
    primaryCause: "etcd集群网络分区导致脑裂"
    contributingFactors:
    - 网络设备固件bug
    - 缺乏网络健康检查机制
    - 故障转移测试不充分
    
  correctiveActions:
  - immediate:
    - 修复网络设备固件
    - 增加etcd健康检查频率
    - 完善故障转移测试流程
  - longTerm:
    - 部署网络监控系统
    - 建立多地域etcd集群
    - 完善灾难恢复预案
  
  lessonsLearned:
  - 网络基础设施的可靠性直接影响集群稳定性
  - 需要建立更完善的监控告警体系
  - 定期进行故障演练的重要性
```

### 常见误区与最佳实践

**常见误区**:
1. **故障时先追责再解决问题**: 应急阶段唯一目标是恢复服务，追责和复盘在事后进行
2. **所有人都去排查同一个问题**: 缺乏 Incident Commander 导致混乱，多人重复操作甚至互相干扰
3. **不记录操作时间线**: 事后复盘时无法还原现场，也无法计算 MTTR
4. **复盘只写报告不落实改进**: 没有具体的 Action Item 和 Owner，同样的故障会再次发生

**最佳实践**:
- **值班制度**: 7x24 On-Call 轮班，每人每次不超过一周，避免疲劳
- **Runbook 标准化**: 每个常见故障场景都有标准化的处理步骤文档
- **定期演练**: 每季度至少一次故障演练（Chaos Engineering），验证应急流程的有效性
- **Postmortem 模板化**: 使用统一模板记录时间线、根因、影响范围、改进措施

**故障排查**:
```bash
# 快速故障诊断一键脚本
echo "=== 节点状态 ==="
kubectl get nodes -o wide | grep -v " Ready"

echo "=== 异常 Pod ==="
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

echo "=== 资源压力 ==="
kubectl describe nodes | grep -A 5 "Conditions:" | grep -E "True|False"

echo "=== 最近告警事件 ==="
kubectl get events -A --field-selector=type=Warning --sort-by='.lastTimestamp' | tail -10
```

---

## 10. 生产环境安全最佳实践

### 概念解析

**一句话定义**: 生产环境安全最佳实践是一套覆盖身份认证、权限控制、网络隔离、运行时防护和合规审计的综合安全框架，保护集群和应用免受攻击。

**类比**: 就像一座城堡的防御体系——外围有护城河（NetworkPolicy），城门有守卫验证身份（RBAC），城内有巡逻队检查异常行为（运行时安全），档案室有访客登记册（审计日志），每层防御都不可缺少。

**核心要点**:
- **零信任原则**: 不信任任何请求，每次访问都要验证身份和权限
- **纵深防御**: 从网络、容器、应用、数据多层设置安全屏障
- **合规自动化**: 使用工具自动检查 CIS Benchmark、GDPR 等合规要求
- **持续安全监控**: 实时检测异常行为（提权、异常网络连接、文件篡改）

### 原理深入

**工作机制**:
- **Pod Security Admission**: K8s 内置的准入控制器，在 Pod 创建时检查其安全配置是否符合策略（privileged/baseline/restricted）
- **审计日志**: API Server 记录所有请求的元数据和响应，可配置不同详细级别（None/Metadata/Request/RequestResponse）
- **OPA/Kyverno**: 策略引擎作为准入 Webhook，在资源创建/修改时执行自定义规则

### 渐进式示例

**Level 1 - 基础用法（命名空间级安全标签）**:
```yaml
# 给命名空间设置 Pod 安全标准（K8s 1.25+）
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    # restricted 是最严格的安全级别
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/audit: restricted
```

**Level 2 - 进阶配置（审计日志策略）**:
```yaml
# API Server 审计日志策略
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 忽略健康检查等高频请求
- level: None
  resources:
  - group: ""
    resources: ["events"]
# 记录敏感操作的完整内容
- level: RequestResponse
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
  - group: "rbac.authorization.k8s.io"
    resources: ["*"]
# 其他操作只记录元数据
- level: Metadata
  omitStages: ["RequestReceived"]
```

**Level 3 - 生产最佳实践（完整零信任安全）**:

### 10.1 零信任安全实施框架

```yaml
# ========== 生产环境零信任安全配置 ==========
apiVersion: security.k8s.io/v1
kind: PodSecurityPolicy
metadata:
  name: production-restricted
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
  - ALL
  volumes:
  - configMap
  - emptyDir
  - projected
  - secret
  - downwardAPI
  - persistentVolumeClaim
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  supplementalGroups:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  fsGroup:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  readOnlyRootFilesystem: true

---
# ========== 网络策略实施 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 9090
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

---
# ========== RBAC最小权限配置 ==========
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-developer-role
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-developer-binding
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: app-developer-role
  apiGroup: rbac.authorization.k8s.io
```

### 10.2 安全监控与告警策略

| 安全维度 | 监控指标 | 告警阈值 | 响应动作 | 处理时效 |
|---------|---------|---------|---------|---------|
| **身份认证** | 异常登录尝试、令牌泄露 | >5次失败登录/小时 | 立即锁定账户 | 5分钟 |
| **权限变更** | RBAC规则修改、ServiceAccount变更 | 任何未授权变更 | 安全审计、回滚变更 | 30分钟 |
| **网络访问** | 异常端口访问、外部连接 | 连接到黑名单IP | 阻断流量、安全调查 | 15分钟 |
| **镜像安全** | 漏洞扫描结果、基线不符合 | Critical/High漏洞 | 阻断部署、紧急修复 | 1小时 |
| **运行时安全** | 异常系统调用、文件修改 | 违反安全策略 | 隔离容器、告警通知 | 10分钟 |

### 10.3 合规性自动化检查

```bash
#!/bin/bash
# ========== Kubernetes安全合规检查脚本 ==========
set -euo pipefail

COMPLIANCE_REPORT="/var/reports/compliance-$(date +%Y%m%d).txt"
echo "Kubernetes安全合规检查报告 - $(date)" > ${COMPLIANCE_REPORT}

# CIS基准检查
check_cis_benchmark() {
    echo "=== CIS Kubernetes Benchmark 检查 ===" >> ${COMPLIANCE_REPORT}
    
    # 检查API Server配置
    if kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | \
       grep -q "anonymous-auth=false"; then
        echo "✅ API Server匿名认证已禁用" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ API Server匿名认证未禁用" >> ${COMPLIANCE_REPORT}
    fi
    
    # 检查etcd加密
    if kubectl get pod -n kube-system -l component=etcd -o jsonpath='{.items[*].spec.containers[*].command}' | \
       grep -q "auto-tls=true"; then
        echo "✅ etcd自动TLS已启用" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ etcd自动TLS未启用" >> ${COMPLIANCE_REPORT}
    fi
    
    # 检查Pod安全策略
    psp_count=$(kubectl get psp --no-headers | wc -l)
    if [ ${psp_count} -gt 0 ]; then
        echo "✅ 已配置${psp_count}个Pod安全策略" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ 未配置Pod安全策略" >> ${COMPLIANCE_REPORT}
    fi
}

# GDPR合规检查
check_gdpr_compliance() {
    echo -e "\n=== GDPR合规检查 ===" >> ${COMPLIANCE_REPORT}
    
    # 检查数据加密
    secrets_encrypted=$(kubectl get secrets -A --no-headers | wc -l)
    echo "🔒 加密Secret数量: ${secrets_encrypted}" >> ${COMPLIANCE_REPORT}
    
    # 检查日志保留策略
    log_retention_days=$(kubectl get cm -n kube-system kube-proxy -o jsonpath='{.data.config\.yaml}' | \
                        grep -o "log-flush-frequency=[0-9]*" | cut -d'=' -f2 || echo "未配置")
    echo "📝 日志刷新频率: ${log_retention_days}s" >> ${COMPLIANCE_REPORT}
}

check_cis_benchmark
check_gdpr_compliance

echo -e "\n合规检查完成，详情请查看: ${COMPLIANCE_REPORT}"
```

### 常见误区与最佳实践

**常见误区**:
1. **使用默认 ServiceAccount**: 默认 SA 可能被挂载到所有 Pod，应为每个应用创建专用 SA 并设置 `automountServiceAccountToken: false`
2. **安全扫描只在 CI 阶段做**: 运行时也可能引入新漏洞，需要持续扫描
3. **审计日志级别全设为 RequestResponse**: 会产生海量日志，存储和性能都受影响

**最佳实践**:
- **最小权限 ServiceAccount**: 每个应用一个 SA，只绑定必要的 Role
- **镜像签名验证**: 使用 Sigstore/Cosign 签名镜像，在准入控制时验证
- **网络策略 + 服务网格**: NetworkPolicy 做粗粒度隔离，Istio mTLS 做细粒度认证
- **定期渗透测试**: 每季度对集群进行安全评估，使用 kube-bench 检查 CIS 基准

**故障排查**:
```bash
# 检查 Pod Security Admission 是否阻止了 Pod 创建
kubectl get events -n <namespace> | grep "Forbidden"

# 检查 RBAC 权限
kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa>

# 查看审计日志
# 通常在 /var/log/kubernetes/audit/ 或通过日志系统查询

# 使用 kube-bench 检查 CIS 基准
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml
kubectl logs job/kube-bench
```

---

## 11. 成本优化与资源管理

### 概念解析

**一句话定义**: 成本优化与资源管理是通过配额（Quota）和限制范围（LimitRange）从命名空间级别控制资源使用总量和单个 Pod 的资源范围，防止资源滥用和意外超支。

**类比**: 就像公司的预算管理一样——ResourceQuota 是部门年度预算（总额不能超），LimitRange 是单笔报销上限（每笔不能太多也不能太少）。两者结合确保资源使用既有总量控制，又有单项合理性。

**核心要点**:
- **ResourceQuota**: 命名空间级别的资源总量限制（CPU、内存、存储、对象数量）
- **LimitRange**: 限制单个容器/Pod 的资源请求和上限范围，并设置默认值
- **资源画像**: 基于历史数据分析资源使用模式，为优化提供数据支撑
- **Chargeback/Showback**: 将资源成本分摊到业务团队，推动自主优化

### 原理深入

**工作机制**:
- **ResourceQuota** 由 API Server 的准入控制器执行：每次创建/更新资源时，检查命名空间当前总用量 + 新增量是否超过配额
- **LimitRange** 作为准入 Webhook：如果 Pod 未设置 resources，自动注入 LimitRange 定义的默认值；如果超出范围则拒绝创建
- **两者配合**: LimitRange 确保每个 Pod 「合理」，ResourceQuota 确保命名空间总量「可控」

### 渐进式示例

**Level 1 - 基础用法（简单配额）**:
```yaml
# 为命名空间设置资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: team-a
spec:
  hard:
    requests.cpu: "10"        # 最多请求 10 核 CPU
    requests.memory: "20Gi"   # 最多请求 20GB 内存
    pods: "50"                # 最多 50 个 Pod
```

**Level 2 - 进阶配置（配额 + 默认值）**:
```yaml
# LimitRange：为容器设置默认资源和范围
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: team-a
spec:
  limits:
  - type: Container
    default:          # 如果 Pod 没设 limits，自动用这个
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:   # 如果 Pod 没设 requests，自动用这个
      cpu: "100m"
      memory: "128Mi"
    max:              # 单个容器最大值
      cpu: "4"
      memory: "8Gi"
    min:              # 单个容器最小值
      cpu: "10m"
      memory: "16Mi"
```

**Level 3 - 生产最佳实践（完整资源管理方案）**:

### 11.1 资源配额与限制管理

```yaml
# ========== 生产环境资源配额配置 ==========
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    # 计算资源配额
    requests.cpu: "100"
    requests.memory: "200Gi"
    limits.cpu: "200"
    limits.memory: "400Gi"
    
    # 存储资源配额
    requests.storage: "10Ti"
    persistentvolumeclaims: "1000"
    
    # 对象数量配额
    pods: "10000"
    services: "500"
    secrets: "1000"
    configmaps: "1000"

---
# ========== LimitRange配置 ==========
apiVersion: v1
kind: LimitRange
metadata:
  name: production-limits
  namespace: production
spec:
  limits:
  - type: Container
    default:
      cpu: "1"
      memory: "1Gi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "8"
      memory: "16Gi"
    min:
      cpu: "10m"
      memory: "16Mi"
  - type: Pod
    max:
      cpu: "16"
      memory: "32Gi"
```

### 11.2 成本监控与优化策略

| 优化维度 | 监控指标 | 优化策略 | 预期收益 | 实施复杂度 |
|---------|---------|---------|---------|-----------|
| **节点资源** | CPU/内存利用率、节点空闲率 | 水平扩缩容、节点池优化 | 20-40%成本节约 | ⭐⭐ |
| **存储成本** | PVC使用率、快照保留 | 生命周期管理、冷热数据分离 | 30-50%存储节约 | ⭐⭐⭐ |
| **网络费用** | 流量使用、跨区域传输 | CDN优化、就近部署 | 25-35%网络节约 | ⭐⭐ |
| **Spot实例** | 按需/竞价实例比例 | 智能调度策略 | 50-80%计算节约 | ⭐⭐⭐⭐ |
| **镜像缓存** | 镜像拉取次数、缓存命中率 | 镜像预热、本地缓存 | 15-25%拉取节约 | ⭐⭐ |

### 11.3 成本优化自动化脚本

```bash
#!/bin/bash
# ========== Kubernetes成本优化分析脚本 ==========
set -euo pipefail

COST_ANALYSIS_DIR="/var/cost-analysis/$(date +%Y%m%d)"
mkdir -p ${COST_ANALYSIS_DIR}

analyze_cluster_costs() {
    echo "=== 集群成本分析报告 ===" > ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 节点成本分析
    echo "节点成本分布:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.cpu}{"\t"}{.status.capacity.memory}{"\n"}{end}' | \
    while read node cpu mem; do
        # 基于实例类型的估算成本（示例价格）
        case ${node} in
            *m5.large*) hourly_cost=0.096 ;;
            *m5.xlarge*) hourly_cost=0.192 ;;
            *m5.2xlarge*) hourly_cost=0.384 ;;
            *) hourly_cost=0.200 ;;  # 默认价格
        esac
        monthly_cost=$(echo "${hourly_cost} * 730" | bc -l)
        echo "${node}: $${monthly_cost}/月 (${cpu}vCPU, ${mem})" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    done
    
    # Pod资源使用分析
    echo -e "\nPod资源使用效率:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl top pods -A --no-headers | \
    awk '{
        cpu_req=$3+0; mem_req=$4+0;
        cpu_util=$5+0; mem_util=$6+0;
        cpu_efficiency = (cpu_util/cpu_req)*100;
        mem_efficiency = (mem_util/mem_req)*100;
        if(cpu_efficiency < 30 || mem_efficiency < 30) {
            print $1"/"$2": CPU效率="cpu_efficiency"% Memory效率="mem_efficiency"%"
        }
    }' >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 存储成本分析
    echo -e "\n存储成本分析:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get pvc -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.spec.resources.requests.storage}{"\n"}{end}' | \
    while read ns pvc size; do
        # 基于存储类型的估算成本
        storage_cost=$(echo "${size%Gi} * 0.10" | bc -l)  # $0.10/GiB/月
        echo "${ns}/${pvc}: ${size} ($${storage_cost}/月)" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    done
}

generate_optimization_recommendations() {
    echo -e "\n=== 优化建议 ===" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 低效Pod推荐
    echo "建议优化的Pod:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl top pods -A --no-headers | \
    awk '$5 < 30 || $6 < 30 {print $1"/"$2" - 资源使用率低"}' >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 节点优化建议
    echo -e "\n节点优化建议:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.cpu}{"\n"}{end}' | \
    while read node allocatable; do
        pod_count=$(kubectl get pods --field-selector spec.nodeName=${node} --no-headers | wc -l)
        pods_per_core=$(echo "${pod_count}/${allocatable}" | bc -l)
        if (( $(echo "${pods_per_core} < 2" | bc -l) )); then
            echo "${node}: CPU利用率低，考虑缩小实例规格" >> ${COST_ANALYSIS_DIR}/cost-report.txt
        fi
    done
}

analyze_cluster_costs
generate_optimization_recommendations

echo "成本分析报告已生成: ${COST_ANALYSIS_DIR}/cost-report.txt"
```

### 常见误区与最佳实践

**常见误区**:
1. **ResourceQuota 设置后不告知开发团队**: 开发者不知道配额存在，Pod 创建失败时一脸懵，应在命名空间 README 中说明配额限制
2. **LimitRange 默认值设太大**: 默认值应偏保守，让开发者主动申请更多资源，而不是默认给很多
3. **只设 ResourceQuota 不设 LimitRange**: 开发者可以创建一个巨大的 Pod 吃掉整个配额，LimitRange 防止单个 Pod 过大

**最佳实践**:
- **每个命名空间都设配额**: 没有配额的命名空间是「无底洞」
- **成本可视化**: 使用 Kubecost/OpenCost 让每个团队看到自己的资源消耗和成本
- **定期审查**: 每月分析 requests vs 实际使用，推动 Right-sizing
- **分级存储**: 热数据用 SSD，冷数据用 HDD/对象存储，降低存储成本

**故障排查**:
```bash
# 查看配额使用情况
kubectl describe resourcequota -n <namespace>

# Pod 创建失败时检查是否超配额
kubectl get events -n <namespace> | grep "exceeded quota"

# 查看 LimitRange
kubectl describe limitrange -n <namespace>

# 分析命名空间实际资源使用
kubectl top pods -n <namespace> --sort-by=memory
```

---

## 12. 变更管理与发布策略

### 概念解析

**一句话定义**: 变更管理与发布策略是一套控制生产环境变更风险的流程和技术，包括变更审批、渐进式发布、自动化回滚等，确保每次发布都是安全、可控、可回滚的。

**类比**: 就像药物上市前要经过临床试验一样——先在小范围（金丝雀发布）验证安全性，确认没问题后逐步扩大范围（灰度发布），最终全面上市（全量发布）。一旦发现副作用（故障），立即召回（回滚）。

**核心要点**:
- **蓝绿部署**: 维护新旧两套完整环境，切换流量实现零停机发布
- **金丝雀发布**: 先将少量流量（如 5%）导向新版本，观察无异常后逐步增加
- **滚动更新**: Kubernetes 原生支持，逐个替换 Pod 实例
- **变更审批**: 任何生产变更都需要经过审批流程，包括代码审查、安全扫描、性能测试
- **自动回滚**: 基于指标监控（错误率、延迟）自动触发回滚

### 原理深入

**工作机制**:
- **Rolling Update**: Deployment 控制器创建新 ReplicaSet → 逐步增加新 RS 副本数 → 同时减少旧 RS 副本数 → 通过 `maxSurge`/`maxUnavailable` 控制速度
- **金丝雀（Argo Rollouts）**: 创建新 RS → 分配少量流量 → 等待分析完成 → 逐步增加流量 → 失败时自动回滚到旧 RS
- **蓝绿（Argo Rollouts）**: 创建完整的新 RS（绿色） → 预览环境验证 → 一次性切换所有流量 → 保留旧 RS 一段时间

**关键发布策略对比**:
| 策略 | 零停机 | 资源开销 | 回滚速度 | 风险控制 | 适用场景 |
|-----|-------|---------|---------|---------|---------|
| 滚动更新 | 是 | 低（+1 Pod） | 慢（逐个回滚） | 中 | 大多数无状态应用 |
| 蓝绿 | 是 | 高（2 倍资源） | 极快（秒级切换） | 高 | 核心服务、数据库迁移 |
| 金丝雀 | 是 | 中（+少量 Pod） | 快 | 极高 | 核心 API、用户体验敏感 |

### 渐进式示例

**Level 1 - 基础用法（Kubernetes 原生滚动更新）**:
```yaml
# 最简单的发布：滚动更新
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 更新时最多 1 个不可用
      maxSurge: 1         # 最多多创建 1 个新 Pod
  template:
    spec:
      containers:
      - name: my-app
        image: my-app:v2.0  # 更新镜像版本即触发滚动更新
```

**Level 2 - 进阶配置（金丝雀发布）**:
```yaml
# 使用 Argo Rollouts 实现金丝雀发布
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  replicas: 5
  strategy:
    canary:
      steps:
      - setWeight: 10        # 第 1 步：10% 流量到新版本
      - pause: {duration: 5m} # 等 5 分钟观察
      - setWeight: 30        # 第 2 步：增加到 30%
      - pause: {duration: 5m}
      - setWeight: 60        # 第 3 步：增加到 60%
      - pause: {duration: 10m}
      # 全部通过后自动全量发布
  template:
    spec:
      containers:
      - name: my-app
        image: my-app:v2.0
```

**Level 3 - 生产最佳实践（完整变更管理流程）**:

### 12.1 GitOps流水线最佳实践

```yaml
# ========== ArgoCD应用配置模板 ==========
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: production-app-template
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/company/production-app.git
    targetRevision: HEAD
    path: k8s/overlays/production
    helm:
      valueFiles:
      - values-production.yaml
      parameters:
      - name: image.tag
        value: ${ARGOCD_APP_REVISION}
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
      allowEmpty: false
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
    syncOptions:
    - CreateNamespace=true
    - PruneLast=true
    - RespectIgnoreDifferences=true
    - ApplyOutOfSyncOnly=true

---
# ========== 多环境配置管理 ==========
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-environment-config
  namespace: production
data:
  # 生产环境特定配置
  DATABASE_URL: "postgresql://prod-db.cluster.local:5432/app"
  REDIS_URL: "redis://prod-redis.cluster.local:6379"
  LOG_LEVEL: "WARN"
  CACHE_TTL: "300"
  ENABLE_DEBUG: "false"
  MAX_CONNECTIONS: "100"
  
  # 安全配置
  TLS_MIN_VERSION: "TLS1.3"
  HSTS_MAX_AGE: "31536000"
  CORS_ALLOWED_ORIGINS: "https://app.example.com"
  SECURITY_HEADERS: |
    Strict-Transport-Security: max-age=31536000; includeSubDomains
    X-Content-Type-Options: nosniff
    X-Frame-Options: DENY
    Content-Security-Policy: default-src 'self'
```

### 12.2 渐进式发布策略

| 发布策略 | 实施方式 | 风险控制 | 监控指标 | 回滚机制 |
|---------|---------|---------|---------|---------|
| **蓝绿部署** | 维护两套完整环境 | 零停机时间 | 健康检查、性能指标 | 一键切换回旧环境 |
| **金丝雀发布** | 逐步增加新版本流量 | 限制影响范围 | 错误率、延迟指标 | 自动回滚到稳定版本 |
| **滚动更新** | 逐个替换Pod实例 | 原地升级 | 就绪探针、存活探针 | 失败时暂停并回滚 |
| **功能开关** | 代码层面控制功能 | 精确控制范围 | 业务指标、用户反馈 | 动态开启/关闭功能 |

### 12.3 变更审批与审计流程

```yaml
# ========== 变更管理流程配置 ==========
apiVersion: changemanagement.example.com/v1
kind: ChangeRequest
metadata:
  name: cr-20260205-001
spec:
  changeType: "Production Deployment"
  priority: "High"
  affectedSystems:
  - name: "user-service"
    environment: "production"
    criticality: "Business Critical"
  
  approvalWorkflow:
    reviewers:
    - role: "SRE Team Lead"
      required: true
    - role: "Security Officer"
      required: true
    - role: "Product Owner"
      required: false
    
    approvalConditions:
    - type: "Automated Tests"
      status: "Passed"
      required: true
    - type: "Security Scan"
      status: "Clean"
      required: true
    - type: "Performance Test"
      status: "Within Threshold"
      required: true
  
  rollbackPlan:
    triggerConditions:
    - metric: "error_rate"
      threshold: "5%"
      duration: "5m"
    - metric: "response_time"
      threshold: "2s"
      duration: "10m"
    - metric: "business_impact"
      threshold: "significant_degradation"
      duration: "immediate"
    
    rollbackActions:
    - action: "argo_rollout_undo"
      target: "user-service"
      timeout: "300s"
    - action: "notification_slack"
      target: "#production-alerts"
      message: "Automatic rollback triggered for user-service"
```

### 常见误区与最佳实践

**常见误区**:
1. **直接全量发布到生产环境**: 无论多「小」的变更都应使用渐进式发布，因为任何变更都可能引发意外
2. **回滚计划只存在于脑海中**: 每次发布前必须有书面的回滚步骤和触发条件
3. **金丝雀发布只看错误率**: 还应监控延迟、资源使用、业务指标（如订单量、转化率）
4. **周五下午发布**: 经典翻车时间——如果出问题，整个周末都在修

**最佳实践**:
- **变更窗口**: 定义允许发布的时间窗口，避免在业务高峰期发布
- **发布检查清单**: 每次发布前过一遍 Checklist（镜像版本、配置变更、数据库迁移、回滚方案）
- **自动化回滚门禁**: 配置基于指标的自动回滚（错误率 > 5% 持续 5 分钟 → 自动回滚）
- **Feature Flags**: 将代码发布和功能开启解耦，先发布代码，再通过开关逐步开启功能

**故障排查**:
```bash
# 查看 Deployment 滚动更新状态
kubectl rollout status deployment/<name> -n <namespace>

# 查看发布历史
kubectl rollout history deployment/<name> -n <namespace>

# 手动回滚到上一个版本
kubectl rollout undo deployment/<name> -n <namespace>

# 回滚到指定版本
kubectl rollout undo deployment/<name> --to-revision=<number> -n <namespace>

# 查看 Argo Rollouts 状态
kubectl argo rollouts get rollout <name> -n <namespace>
```

---

## 关联阅读

| 主题 | 文件 | 说明 |
|-----|------|------|
| 核心概念 | [05-concept-reference.md](05-concept-reference.md) | 不熟悉术语时查阅 |
| CLI 命令 | [06-cli-commands.md](06-cli-commands.md) | 运维命令速查 |
| 故障分析 | [02-failure-patterns-analysis.md](02-failure-patterns-analysis.md) | 故障模式和根因分析方法论 |
| 性能调优 | [03-performance-tuning-expert.md](03-performance-tuning-expert.md) | CPU/内存/网络/存储调优详解 |
| SRE 成熟度 | [04-sre-maturity-model.md](04-sre-maturity-model.md) | 运维成熟度评估和提升路径 |
| 安全专题 | [09-cloud-native-security.md](09-cloud-native-security.md) | 零信任架构深度指南 |
| 多云运维 | [10-multi-cloud-operations.md](10-multi-cloud-operations.md) | 多云架构设计和管理 |
| 事故管理 | [12-incident-management-runbooks.md](12-incident-management-runbooks.md) | 标准化 Runbook 集合 |
| 容量规划 | [13-capacity-planning-forecasting.md](13-capacity-planning-forecasting.md) | 容量预测模型和扩容策略 |
| SLI/SLO | [15-sli-slo-sla-engineering.md](15-sli-slo-sla-engineering.md) | 服务质量目标工程 |
| 故障排查 | [16-production-troubleshooting-playbook.md](16-production-troubleshooting-playbook.md) | 生产故障排查手册 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-02 | 版本: v1.25-v1.32 | 质量等级: ⭐⭐⭐⭐⭐ 专家级