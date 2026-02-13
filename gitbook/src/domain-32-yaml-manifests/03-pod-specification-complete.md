# 03 - Pod 完整规格说明书

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

---

## 📋 目录

- [Pod 概述](#pod-概述)
- [API 信息](#api-信息)
- [完整字段规格表](#完整字段规格表)
- [Container 字段详解](#container-字段详解)
- [Init Containers](#init-containers)
- [Sidecar Containers](#sidecar-containers)
- [Ephemeral Containers](#ephemeral-containers)
- [Volumes 所有类型](#volumes-所有类型)
- [Pod 级 SecurityContext](#pod-级-securitycontext)
- [调度字段](#调度字段)
- [DNS 配置](#dns-配置)
- [配置示例](#配置示例)
- [内部原理](#内部原理)
- [版本兼容性矩阵](#版本兼容性矩阵)
- [最佳实践](#最佳实践)
- [FAQ](#faq)
- [生产案例](#生产案例)

---

## Pod 概述

### 什么是 Pod?

**Pod** 是 Kubernetes 中最小的可部署计算单元,代表集群中运行的一个或多个容器的组合。

**核心特性**:
- 共享网络命名空间(同一 IP 地址和端口空间)
- 共享存储卷
- 共享 IPC 命名空间(可选)
- 共享 PID 命名空间(可选)
- 原子调度单位(Pod 中所有容器调度到同一节点)

**设计用途**:
- 紧密耦合的应用容器组合
- Sidecar 模式(日志收集、代理、监控)
- Init 容器执行预启动任务

---

## API 信息

| 项目 | 值 |
|------|-----|
| **API Group** | core (空字符串) |
| **API Version** | v1 |
| **Kind** | Pod |
| **完整 API 路径** | `/api/v1/namespaces/{namespace}/pods` |
| **Scope** | Namespaced |
| **Short Names** | po |

**kubectl 常用命令**:
```bash
# 创建 Pod
kubectl apply -f pod.yaml

# 查看 Pod
kubectl get pods
kubectl get pod <name> -o yaml

# 查看 Pod 详情
kubectl describe pod <name>

# 查看日志
kubectl logs <pod-name> [-c <container-name>]

# 执行命令
kubectl exec -it <pod-name> [-c <container-name>] -- /bin/bash

# 删除 Pod
kubectl delete pod <name>

# 调试 Pod
kubectl debug <pod-name> -it --image=busybox
```

---

## 完整字段规格表

### spec 顶层字段

| 字段 | 类型 | 必需 | 描述 | 默认值 |
|------|------|------|------|--------|
| `containers` | []Container | ✅ | 主容器列表 | - |
| `initContainers` | []Container | ❌ | 初始化容器列表 | - |
| `ephemeralContainers` | []EphemeralContainer | ❌ | 临时调试容器列表 | - |
| `volumes` | []Volume | ❌ | 可挂载的卷列表 | - |
| `restartPolicy` | string | ❌ | 重启策略(Always/OnFailure/Never) | Always |
| `terminationGracePeriodSeconds` | int64 | ❌ | 优雅终止等待时间(秒) | 30 |
| `activeDeadlineSeconds` | int64 | ❌ | Pod 最大存活时间(秒) | - |
| `dnsPolicy` | string | ❌ | DNS 策略 | ClusterFirst |
| `dnsConfig` | PodDNSConfig | ❌ | 自定义 DNS 配置 | - |
| `nodeSelector` | map[string]string | ❌ | 节点选择器 | - |
| `nodeName` | string | ❌ | 指定调度到的节点名 | - |
| `affinity` | Affinity | ❌ | 亲和性规则 | - |
| `tolerations` | []Toleration | ❌ | 容忍度 | - |
| `schedulerName` | string | ❌ | 调度器名称 | default-scheduler |
| `priority` | int32 | ❌ | 优先级值 | 0 |
| `priorityClassName` | string | ❌ | 优先级类名 | - |
| `serviceAccountName` | string | ❌ | 服务账户名 | default |
| `automountServiceAccountToken` | bool | ❌ | 自动挂载 SA Token | true |
| `hostNetwork` | bool | ❌ | 使用宿主机网络 | false |
| `hostPID` | bool | ❌ | 使用宿主机 PID 命名空间 | false |
| `hostIPC` | bool | ❌ | 使用宿主机 IPC 命名空间 | false |
| `shareProcessNamespace` | bool | ❌ | 容器间共享 PID 命名空间 | false |
| `securityContext` | PodSecurityContext | ❌ | Pod 级安全上下文 | - |
| `imagePullSecrets` | []LocalObjectReference | ❌ | 镜像拉取密钥 | - |
| `hostname` | string | ❌ | Pod 主机名 | - |
| `subdomain` | string | ❌ | Pod 子域名 | - |
| `setHostnameAsFQDN` | bool | ❌ | 设置完整域名为主机名 | false |
| `topologySpreadConstraints` | []TopologySpreadConstraint | ❌ | 拓扑分布约束 | - |
| `overhead` | map[string]Quantity | ❌ | Pod 开销(由 RuntimeClass 设置) | - |
| `readinessGates` | []PodReadinessGate | ❌ | 就绪门控 | - |
| `runtimeClassName` | string | ❌ | 运行时类名 | - |
| `enableServiceLinks` | bool | ❌ | 启用服务环境变量 | true |
| `preemptionPolicy` | string | ❌ | 抢占策略 | PreemptLowerPriority |
| `resourceClaims` | []PodResourceClaim | ❌ | 动态资源声明(v1.26+) | - |
| `schedulingGates` | []PodSchedulingGate | ❌ | 调度门控(v1.27+) | - |

---

## Container 字段详解

### Container 完整字段

| 字段 | 类型 | 必需 | 描述 |
|------|------|------|------|
| `name` | string | ✅ | 容器名称 |
| `image` | string | ✅ | 容器镜像 |
| `imagePullPolicy` | string | ❌ | 镜像拉取策略(Always/IfNotPresent/Never) |
| `command` | []string | ❌ | 容器入口命令(覆盖 ENTRYPOINT) |
| `args` | []string | ❌ | 容器参数(覆盖 CMD) |
| `workingDir` | string | ❌ | 工作目录 |
| `ports` | []ContainerPort | ❌ | 容器端口列表 |
| `env` | []EnvVar | ❌ | 环境变量列表 |
| `envFrom` | []EnvFromSource | ❌ | 从 ConfigMap/Secret 导入环境变量 |
| `resources` | ResourceRequirements | ❌ | 资源请求和限制 |
| `volumeMounts` | []VolumeMount | ❌ | 卷挂载列表 |
| `volumeDevices` | []VolumeDevice | ❌ | 块设备挂载 |
| `livenessProbe` | Probe | ❌ | 存活探针 |
| `readinessProbe` | Probe | ❌ | 就绪探针 |
| `startupProbe` | Probe | ❌ | 启动探针 |
| `lifecycle` | Lifecycle | ❌ | 生命周期钩子 |
| `terminationMessagePath` | string | ❌ | 终止消息文件路径 |
| `terminationMessagePolicy` | string | ❌ | 终止消息策略 |
| `securityContext` | SecurityContext | ❌ | 容器级安全上下文 |
| `stdin` | bool | ❌ | 分配标准输入 |
| `stdinOnce` | bool | ❌ | 标准输入仅一次 |
| `tty` | bool | ❌ | 分配 TTY |
| `resizePolicy` | []ContainerResizePolicy | ❌ | 容器调整大小策略(v1.27+) |
| `restartPolicy` | string | ❌ | 容器重启策略(v1.28+,仅 Sidecar) |

### 1. Image 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: image-demo
spec:
  containers:
  - name: app
    image: nginx:1.25-alpine  # 镜像名称:标签
    imagePullPolicy: IfNotPresent  # Always: 总是拉取 | IfNotPresent: 本地不存在时拉取 | Never: 仅使用本地镜像
  
  # 私有镜像仓库认证
  imagePullSecrets:
  - name: my-registry-secret  # 引用 Secret 对象
```

**ImagePullPolicy 默认值规则**:
- `image: nginx:latest` 或无标签 → `Always`
- `image: nginx:1.25` (非 latest 标签) → `IfNotPresent`

### 2. Command 和 Args

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: command-demo
spec:
  containers:
  - name: app
    image: busybox
    # command 覆盖 Dockerfile ENTRYPOINT
    command: ["/bin/sh"]
    # args 覆盖 Dockerfile CMD
    args: ["-c", "while true; do echo hello; sleep 10; done"]
  
  - name: app2
    image: nginx
    # 等价写法(YAML 数组格式)
    command:
    - /bin/sh
    - -c
    args:
    - |
      echo "Starting nginx..."
      nginx -g 'daemon off;'
```

**Dockerfile vs Kubernetes 映射**:
| Dockerfile | Kubernetes | 说明 |
|------------|------------|------|
| ENTRYPOINT | command | 可执行文件 |
| CMD | args | 参数 |

### 3. Environment Variables

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: env-demo
spec:
  containers:
  - name: app
    image: nginx
    env:
    # 方式1: 直接定义
    - name: ENV_VAR1
      value: "plain-value"
    
    # 方式2: 引用 ConfigMap
    - name: CONFIG_KEY
      valueFrom:
        configMapKeyRef:
          name: my-config
          key: config.key
    
    # 方式3: 引用 Secret
    - name: SECRET_PASSWORD
      valueFrom:
        secretKeyRef:
          name: my-secret
          key: password
    
    # 方式4: 引用字段路径(Downward API)
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    
    - name: NODE_NAME
      valueFrom:
        fieldRef:
          fieldPath: spec.nodeName
    
    # 方式5: 引用资源字段
    - name: CPU_REQUEST
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: requests.cpu
          divisor: "1m"  # 单位: 1m (millicore)
    
    - name: MEMORY_LIMIT
      valueFrom:
        resourceFieldRef:
          resource: limits.memory
          divisor: "1Mi"  # 单位: 1Mi
    
    # 方式6: 批量导入 ConfigMap
    envFrom:
    - configMapRef:
        name: my-config-all  # 所有 key-value 作为环境变量
    
    # 方式7: 批量导入 Secret
    - secretRef:
        name: my-secret-all
    
    # 方式8: 带前缀批量导入
    - prefix: APP_  # 所有环境变量加 APP_ 前缀
      configMapRef:
        name: app-config
```

**Downward API 可用字段**:
| fieldPath | 描述 |
|-----------|------|
| `metadata.name` | Pod 名称 |
| `metadata.namespace` | Pod 命名空间 |
| `metadata.uid` | Pod UID |
| `metadata.labels['<KEY>']` | Pod 标签值 |
| `metadata.annotations['<KEY>']` | Pod 注解值 |
| `spec.nodeName` | 节点名称 |
| `spec.serviceAccountName` | 服务账户名 |
| `status.hostIP` | 节点 IP |
| `status.podIP` | Pod IP |
| `status.podIPs` | Pod IPs (双栈) |

### 4. Ports

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ports-demo
spec:
  containers:
  - name: nginx
    image: nginx
    ports:
    - name: http  # 端口名称(可被 Service 引用)
      containerPort: 80  # 容器监听端口
      protocol: TCP  # TCP | UDP | SCTP
    
    - name: https
      containerPort: 443
      protocol: TCP
      hostPort: 8443  # 宿主机端口(不推荐使用,限制调度)
```

**注意**: `ports` 字段仅是声明性的(文档作用),不配置也不影响网络连通性。

### 5. Resources

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: resources-demo
spec:
  containers:
  - name: app
    image: nginx
    resources:
      # 资源请求(调度和 QoS 依据)
      requests:
        cpu: "250m"       # 0.25 核
        memory: "256Mi"   # 256 MiB
        ephemeral-storage: "2Gi"  # 临时存储
      
      # 资源限制(cgroup 限制)
      limits:
        cpu: "500m"       # 0.5 核
        memory: "512Mi"   # 512 MiB
        ephemeral-storage: "4Gi"
        nvidia.com/gpu: "1"  # GPU(扩展资源)
```

**资源单位**:
- **CPU**: `1` = 1核, `1000m` = 1核, `100m` = 0.1核
- **Memory**: `1Gi` = 1024³字节, `1G` = 1000³字节, `1Mi` = 1024²字节
- **Storage**: 同 Memory

**QoS 类别**:
| QoS 类别 | 条件 | 驱逐优先级 |
|----------|------|-----------|
| **Guaranteed** | 所有容器 requests = limits | 最低(最后驱逐) |
| **Burstable** | 至少一个容器有 requests/limits | 中等 |
| **BestEffort** | 无任何 requests/limits | 最高(最先驱逐) |

### 6. VolumeMounts

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: volumemounts-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: html  # 引用 volumes 中的卷名
      mountPath: /usr/share/nginx/html  # 容器内挂载路径
      readOnly: false  # 只读挂载
    
    - name: config
      mountPath: /etc/nginx/nginx.conf
      subPath: nginx.conf  # 仅挂载卷中的特定文件(不覆盖整个目录)
    
    - name: cache
      mountPath: /var/cache/nginx
      mountPropagation: None  # None | HostToContainer | Bidirectional
    
    - name: secret-vol
      mountPath: /etc/secret
      readOnly: true
  
  volumes:
  - name: html
    emptyDir: {}
  - name: config
    configMap:
      name: nginx-config
  - name: cache
    emptyDir: {}
  - name: secret-vol
    secret:
      secretName: my-secret
```

**mountPropagation 选项**:
- `None`: 不传播挂载事件(默认)
- `HostToContainer`: 宿主机挂载传播到容器
- `Bidirectional`: 双向传播(需要特权容器)

### 7. Probes (探针)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: probes-demo
spec:
  containers:
  - name: app
    image: nginx
    
    # 启动探针(v1.16+): 检测容器是否完成启动
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
        httpHeaders:
        - name: Custom-Header
          value: Awesome
      initialDelaySeconds: 0  # 容器启动后多少秒开始探测
      periodSeconds: 10  # 探测间隔
      timeoutSeconds: 1  # 探测超时
      successThreshold: 1  # 成功阈值
      failureThreshold: 30  # 失败阈值(失败30次后重启容器)
    
    # 存活探针: 检测容器是否健康(不健康则重启)
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
        scheme: HTTP  # HTTP | HTTPS
      initialDelaySeconds: 15
      periodSeconds: 10
      timeoutSeconds: 1
      successThreshold: 1
      failureThreshold: 3
    
    # 就绪探针: 检测容器是否就绪(不就绪则从 Service 摘除)
    readinessProbe:
      # 方式1: httpGet
      httpGet:
        path: /ready
        port: 8080
      
      # 方式2: tcpSocket
      # tcpSocket:
      #   port: 8080
      
      # 方式3: exec
      # exec:
      #   command:
      #   - cat
      #   - /tmp/healthy
      
      # 方式4: grpc (v1.24+)
      # grpc:
      #   port: 9090
      #   service: my-service  # 可选
      
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 1
      successThreshold: 1
      failureThreshold: 3
      
      # v1.29+: terminationGracePeriodSeconds 可覆盖 Pod 级别的配置
      terminationGracePeriodSeconds: 30
```

**探针类型对比**:
| 探针类型 | 失败后动作 | 使用场景 |
|----------|-----------|---------|
| `startupProbe` | 重启容器 | 慢启动应用(避免被 livenessProbe 误杀) |
| `livenessProbe` | 重启容器 | 检测死锁、死循环 |
| `readinessProbe` | 从 Service 摘除 | 检测依赖服务是否就绪 |

**探针检测方式**:
1. **httpGet**: HTTP GET 请求(返回 200-399 为成功)
2. **tcpSocket**: TCP 连接(连接成功则成功)
3. **exec**: 执行命令(退出码 0 为成功)
4. **grpc**: gRPC 健康检查(v1.24+)

### 8. Lifecycle Hooks

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: lifecycle-demo
spec:
  containers:
  - name: app
    image: nginx
    lifecycle:
      # 容器启动后钩子
      postStart:
        exec:
          command:
          - /bin/sh
          - -c
          - |
            echo "Container started at $(date)" > /tmp/start.log
            # 注意: postStart 与容器 ENTRYPOINT 并发执行,不保证顺序
      
      # 容器终止前钩子
      preStop:
        httpGet:
          path: /shutdown
          port: 8080
        # 或使用 exec:
        # exec:
        #   command:
        #   - /bin/sh
        #   - -c
        #   - nginx -s quit; sleep 5
```

**生命周期钩子执行时机**:
- **postStart**: 容器创建后立即执行(与 ENTRYPOINT 并发,不保证先后顺序)
- **preStop**: 容器终止前执行(在发送 SIGTERM 之前)

**钩子失败影响**:
- `postStart` 失败 → 容器被杀死并重启
- `preStop` 失败 → 继续终止容器

### 9. SecurityContext (容器级)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: security-context-demo
spec:
  containers:
  - name: app
    image: nginx
    securityContext:
      # 运行用户
      runAsUser: 1000  # UID
      runAsGroup: 3000  # GID
      runAsNonRoot: true  # 强制非 root 运行(root 则启动失败)
      
      # 只读根文件系统
      readOnlyRootFilesystem: true
      
      # 允许特权提升
      allowPrivilegeEscalation: false
      
      # Capabilities (Linux 能力)
      capabilities:
        add: ["NET_ADMIN", "SYS_TIME"]  # 添加能力
        drop: ["ALL"]  # 删除所有能力(推荐)
      
      # SELinux
      seLinuxOptions:
        level: "s0:c123,c456"
      
      # Seccomp
      seccompProfile:
        type: RuntimeDefault  # RuntimeDefault | Localhost | Unconfined
        # localhostProfile: profiles/audit.json  # type=Localhost 时使用
      
      # AppArmor (通过注解配置,见下文)
      
      # Privileged 特权模式(不推荐)
      privileged: false
    
    volumeMounts:
    - name: cache
      mountPath: /var/cache/nginx  # readOnlyRootFilesystem=true 时需要挂载可写卷
  
  volumes:
  - name: cache
    emptyDir: {}
```

**AppArmor 配置**(通过注解):
```yaml
metadata:
  annotations:
    container.apparmor.security.beta.kubernetes.io/app: runtime/default  # 容器名: app
```

---

## Init Containers

### 概述

**Init Containers** 在主容器启动前按顺序执行,用于执行初始化任务。

**特性**:
- 按顺序执行(一个完成后才启动下一个)
- 全部成功后才启动主容器
- 失败则重启 Pod(根据 restartPolicy)
- 不支持 readinessProbe/livenessProbe
- 支持与主容器不同的镜像和资源配置

### 配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: init-demo
spec:
  # Init 容器列表(按顺序执行)
  initContainers:
  # 第一个 Init 容器: 等待服务可用
  - name: wait-for-db
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      until nslookup mysql.default.svc.cluster.local; do
        echo "Waiting for mysql service..."
        sleep 2
      done
  
  # 第二个 Init 容器: 下载配置文件
  - name: download-config
    image: curlimages/curl:8.5.0
    command:
    - sh
    - -c
    - curl -o /config/app.conf http://config-server/app.conf
    volumeMounts:
    - name: config
      mountPath: /config
  
  # 第三个 Init 容器: 数据库迁移
  - name: db-migrate
    image: myapp/migrate:1.0
    env:
    - name: DB_HOST
      value: mysql.default.svc.cluster.local
    - name: DB_PASSWORD
      valueFrom:
        secretKeyRef:
          name: db-secret
          key: password
  
  # 主容器
  containers:
  - name: app
    image: myapp:1.0
    volumeMounts:
    - name: config
      mountPath: /etc/app
  
  volumes:
  - name: config
    emptyDir: {}
```

### 使用场景

1. **等待依赖服务**: 等待数据库、缓存等服务就绪
2. **下载配置**: 从配置中心下载配置文件
3. **数据库初始化**: 执行数据库迁移、创建表结构
4. **生成证书**: 生成 TLS 证书、密钥
5. **注册服务**: 向服务注册中心注册
6. **权限设置**: 修改卷的权限和所有者

---

## Sidecar Containers

### 概述 (v1.29+)

**Sidecar Containers** 是与主容器同时运行的辅助容器,在 v1.29+ 通过 `restartPolicy: Always` 标识。

**与 Init Containers 的区别**:
- Init: 顺序执行,完成后退出
- Sidecar: 与主容器并行运行,持续存在

**原生 Sidecar 容器特性**(v1.29+):
- 在 `initContainers` 中定义,但设置 `restartPolicy: Always`
- 在主容器启动前启动
- 在主容器终止后才终止
- 影响 Pod 就绪状态

### 配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: sidecar-demo
spec:
  # 原生 Sidecar (v1.29+)
  initContainers:
  - name: log-shipper
    image: fluent/fluent-bit:2.1
    restartPolicy: Always  # 标识为 Sidecar(v1.29+)
    volumeMounts:
    - name: logs
      mountPath: /var/log
    env:
    - name: FLUENT_ELASTICSEARCH_HOST
      value: elasticsearch.logging.svc.cluster.local
  
  # 主容器
  containers:
  - name: app
    image: myapp:1.0
    volumeMounts:
    - name: logs
      mountPath: /app/logs
  
  # 传统 Sidecar(所有版本)
  - name: metrics-exporter
    image: prom/statsd-exporter:v0.26.0
    ports:
    - name: metrics
      containerPort: 9102
  
  - name: envoy-proxy
    image: envoyproxy/envoy:v1.28
    ports:
    - name: proxy
      containerPort: 8080
    volumeMounts:
    - name: envoy-config
      mountPath: /etc/envoy
  
  volumes:
  - name: logs
    emptyDir: {}
  - name: envoy-config
    configMap:
      name: envoy-config
```

### 使用场景

1. **日志收集**: Fluent Bit, Filebeat
2. **服务网格**: Envoy, Linkerd
3. **监控**: Prometheus exporter, StatsD
4. **安全**: 认证代理, 加密代理
5. **配置同步**: 配置热更新

### 版本对比

| 版本 | Sidecar 实现方式 | 限制 |
|------|-----------------|------|
| v1.28- | 在 containers 中定义 | 终止顺序不保证 |
| v1.29+ | initContainers + restartPolicy: Always | 启动/终止顺序可控 |

---

## Ephemeral Containers

### 概述

**Ephemeral Containers**(临时容器)用于故障排查和调试,可动态添加到运行中的 Pod。

**特性**:
- 仅用于调试(不能定义 ports, resources, livenessProbe 等)
- 不会自动重启
- 不能在 Pod 创建时定义(只能通过 API 添加)
- 共享 Pod 的网络和存储命名空间

### 使用方法

```bash
# 方式1: kubectl debug 自动创建临时容器
kubectl debug -it my-pod --image=busybox:1.36 --target=app

# 方式2: 使用不同的镜像
kubectl debug -it my-pod --image=nicolaka/netshoot:latest

# 方式3: 共享进程命名空间
kubectl debug -it my-pod --image=busybox --target=app \
  --share-processes -- sh
```

### 手动添加临时容器(API)

```yaml
# 获取 Pod 配置
kubectl get pod my-pod -o json > /tmp/pod.json

# 编辑添加 ephemeralContainers
{
  "apiVersion": "v1",
  "kind": "Pod",
  "metadata": {...},
  "spec": {
    "ephemeralContainers": [
      {
        "name": "debugger",
        "image": "busybox:1.36",
        "command": ["sh"],
        "stdin": true,
        "tty": true,
        "targetContainerName": "app"  # 共享目标容器的 PID 命名空间
      }
    ]
  }
}

# 应用(使用子资源端点)
kubectl replace --raw /api/v1/namespaces/default/pods/my-pod/ephemeralcontainers \
  -f /tmp/pod.json

# 连接到临时容器
kubectl attach -it my-pod -c debugger
```

### 使用场景

1. **Distroless 镜像调试**: 无 shell 的精简镜像
2. **崩溃容器排查**: 容器不断重启时
3. **网络诊断**: tcpdump, netstat, curl
4. **进程调试**: gdb, strace
5. **文件系统检查**: 查看容器内文件

---

## Volumes 所有类型

### Volume 类型总览

| 类型 | 用途 | 生命周期 | 数据持久化 |
|------|------|---------|-----------|
| `emptyDir` | 临时存储 | Pod 生命周期 | ❌ |
| `hostPath` | 宿主机目录 | 节点生命周期 | ✅ |
| `configMap` | 配置文件 | ConfigMap 对象 | ✅ |
| `secret` | 敏感数据 | Secret 对象 | ✅ |
| `persistentVolumeClaim` | 持久存储 | PVC 对象 | ✅ |
| `projected` | 组合卷 | 依赖源对象 | 部分 |
| `downwardAPI` | Pod 元数据 | Pod 生命周期 | ❌ |
| `nfs` | NFS 共享 | 外部 NFS 服务器 | ✅ |
| `csi` | CSI 驱动 | 依赖驱动实现 | ✅ |
| `cephfs` | CephFS | 外部 Ceph 集群 | ✅ |
| `rbd` | Ceph RBD | 外部 Ceph 集群 | ✅ |
| `glusterfs` | GlusterFS | 外部 GlusterFS 集群 | ✅ |
| `iscsi` | iSCSI | 外部 iSCSI 目标 | ✅ |
| `fc` | Fibre Channel | 外部 FC 存储 | ✅ |
| `azureDisk` | Azure 磁盘 | Azure 云 | ✅ |
| `azureFile` | Azure 文件 | Azure 云 | ✅ |
| `awsElasticBlockStore` | AWS EBS | AWS 云 | ✅ |
| `gcePersistentDisk` | GCE PD | GCP 云 | ✅ |
| `local` | 本地持久卷 | 节点本地存储 | ✅ |
| `ephemeral` | 临时 CSI 卷 | Pod 生命周期 | ❌ |

---

### 1. emptyDir

**临时目录,Pod 删除时数据丢失**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: emptydir-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: cache
      mountPath: /var/cache/nginx
  
  - name: sidecar
    image: busybox
    command: ["/bin/sh", "-c", "tail -f /cache/access.log"]
    volumeMounts:
    - name: cache
      mountPath: /cache
  
  volumes:
  - name: cache
    emptyDir:
      medium: Memory  # 默认为 ""(磁盘), Memory 使用 tmpfs(内存)
      sizeLimit: 1Gi  # 大小限制(v1.22+)
```

**使用场景**:
- 容器间数据共享
- 临时缓存
- 检查点文件

---

### 2. hostPath

**挂载宿主机目录或文件**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: hostpath-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: host-data
      mountPath: /data
    - name: docker-sock
      mountPath: /var/run/docker.sock
  
  volumes:
  - name: host-data
    hostPath:
      path: /data/app  # 宿主机路径
      type: DirectoryOrCreate  # 类型(见下表)
  
  - name: docker-sock
    hostPath:
      path: /var/run/docker.sock
      type: Socket
```

**type 选项**:
| type | 行为 |
|------|------|
| `""` (默认) | 不检查,直接挂载 |
| `DirectoryOrCreate` | 目录不存在则创建(权限 0755) |
| `Directory` | 必须存在的目录 |
| `FileOrCreate` | 文件不存在则创建 |
| `File` | 必须存在的文件 |
| `Socket` | 必须存在的 Unix socket |
| `CharDevice` | 必须存在的字符设备 |
| `BlockDevice` | 必须存在的块设备 |

**⚠️ 安全风险**:
- 不同节点路径可能不同
- 可能访问宿主机敏感文件
- PodSecurityPolicy 应限制使用

---

### 3. configMap

**将 ConfigMap 挂载为文件**。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  app.conf: |
    server {
      listen 80;
      server_name example.com;
    }
  log.level: "info"
---
apiVersion: v1
kind: Pod
metadata:
  name: configmap-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    # 方式1: 挂载整个 ConfigMap
    - name: config-volume
      mountPath: /etc/config
    
    # 方式2: 挂载特定键为文件
    - name: config-file
      mountPath: /etc/nginx/nginx.conf
      subPath: nginx.conf
  
  volumes:
  - name: config-volume
    configMap:
      name: app-config
      defaultMode: 0644  # 文件权限
      optional: false  # ConfigMap 不存在时是否报错
  
  - name: config-file
    configMap:
      name: app-config
      items:  # 选择特定键
      - key: app.conf
        path: nginx.conf
        mode: 0644
```

**自动更新**: ConfigMap 更新后,挂载的文件会自动更新(延迟约 1 分钟,使用 subPath 的除外)。

---

### 4. secret

**将 Secret 挂载为文件**(用法与 ConfigMap 类似)。

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secret
type: Opaque
data:
  username: YWRtaW4=  # base64 编码: admin
  password: cGFzc3dvcmQxMjM=  # base64 编码: password123
---
apiVersion: v1
kind: Pod
metadata:
  name: secret-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: secret-volume
      mountPath: /etc/secret
      readOnly: true  # 推荐只读
  
  volumes:
  - name: secret-volume
    secret:
      secretName: app-secret
      defaultMode: 0400  # 更严格的权限
      optional: false
      items:  # 可选: 选择特定键
      - key: username
        path: my-group/my-username
        mode: 0400
```

**Secret 特殊类型**:
```yaml
# TLS 证书
volumes:
- name: tls-cert
  secret:
    secretName: tls-secret
    items:
    - key: tls.crt
      path: cert.pem
    - key: tls.key
      path: key.pem

# Docker 镜像拉取凭证(不作为卷,用 imagePullSecrets)
imagePullSecrets:
- name: docker-registry-secret
```

---

### 5. persistentVolumeClaim

**挂载 PersistentVolumeClaim**。

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
  storageClassName: standard
---
apiVersion: v1
kind: Pod
metadata:
  name: pvc-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: data
      mountPath: /data
  
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: my-pvc
      readOnly: false
```

---

### 6. projected

**将多个卷源投影到同一目录**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: projected-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: all-in-one
      mountPath: /projected
      readOnly: true
  
  volumes:
  - name: all-in-one
    projected:
      defaultMode: 0644
      sources:
      # 源1: Secret
      - secret:
          name: app-secret
          items:
          - key: username
            path: secrets/username
      
      # 源2: ConfigMap
      - configMap:
          name: app-config
          items:
          - key: app.conf
            path: config/app.conf
      
      # 源3: Downward API
      - downwardAPI:
          items:
          - path: "metadata/labels"
            fieldRef:
              fieldPath: metadata.labels
          - path: "metadata/annotations"
            fieldRef:
              fieldPath: metadata.annotations
      
      # 源4: ServiceAccountToken (v1.20+)
      - serviceAccountToken:
          path: token
          expirationSeconds: 3600
          audience: api
```

**挂载后目录结构**:
```
/projected/
├── secrets/
│   └── username
├── config/
│   └── app.conf
├── metadata/
│   ├── labels
│   └── annotations
└── token
```

---

### 7. downwardAPI

**将 Pod/Container 元数据暴露为文件**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: downwardapi-demo
  labels:
    app: myapp
    tier: frontend
  annotations:
    build: "1234"
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: podinfo
      mountPath: /etc/podinfo
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
  
  volumes:
  - name: podinfo
    downwardAPI:
      defaultMode: 0644
      items:
      # Pod 字段
      - path: "pod-name"
        fieldRef:
          fieldPath: metadata.name
      
      - path: "pod-namespace"
        fieldRef:
          fieldPath: metadata.namespace
      
      - path: "pod-ip"
        fieldRef:
          fieldPath: status.podIP
      
      - path: "labels"
        fieldRef:
          fieldPath: metadata.labels
      
      - path: "annotations"
        fieldRef:
          fieldPath: metadata.annotations
      
      # 容器资源字段
      - path: "cpu-request"
        resourceFieldRef:
          containerName: app
          resource: requests.cpu
          divisor: "1m"
      
      - path: "memory-limit"
        resourceFieldRef:
          containerName: app
          resource: limits.memory
          divisor: "1Mi"
```

---

### 8. nfs

**挂载 NFS 共享**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nfs-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: nfs-data
      mountPath: /data
  
  volumes:
  - name: nfs-data
    nfs:
      server: nfs-server.example.com  # NFS 服务器地址
      path: /exported/path  # 导出路径
      readOnly: false
```

---

### 9. csi

**使用 CSI(Container Storage Interface)驱动**。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: csi-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: csi-volume
      mountPath: /data
  
  volumes:
  - name: csi-volume
    csi:
      driver: csi.example.com  # CSI 驱动名称
      volumeAttributes:
        foo: bar
      fsType: ext4
      readOnly: false
      # 可选: 引用 Secret(用于驱动认证)
      nodePublishSecretRef:
        name: csi-secret
```

**常见 CSI 驱动**:
- AWS EBS CSI: `ebs.csi.aws.com`
- Azure Disk CSI: `disk.csi.azure.com`
- GCE PD CSI: `pd.csi.storage.gke.io`
- Ceph CSI: `rbd.csi.ceph.com`

---

### 10. ephemeral (内联临时卷)

**动态创建临时卷**(v1.23+)。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ephemeral-demo
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: scratch
      mountPath: /scratch
  
  volumes:
  - name: scratch
    ephemeral:
      volumeClaimTemplate:
        spec:
          accessModes: ["ReadWriteOnce"]
          storageClassName: fast
          resources:
            requests:
              storage: 1Gi
```

**特性**: Pod 删除时自动删除 PVC 和 PV。

---

## Pod 级 SecurityContext

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-security-context-demo
spec:
  # Pod 级别安全上下文
  securityContext:
    # 运行用户/组
    runAsUser: 1000  # 所有容器默认 UID
    runAsGroup: 3000  # 所有容器默认 GID
    fsGroup: 2000  # 挂载卷的所属组 GID
    fsGroupChangePolicy: "OnRootMismatch"  # Always | OnRootMismatch(默认)
    
    # 补充组
    supplementalGroups: [4000, 5000]
    
    # 强制非 root
    runAsNonRoot: true
    
    # SELinux
    seLinuxOptions:
      level: "s0:c123,c456"
    
    # Seccomp
    seccompProfile:
      type: RuntimeDefault  # RuntimeDefault | Localhost | Unconfined
    
    # Sysctls (内核参数)
    sysctls:
    - name: net.ipv4.ip_local_port_range
      value: "1024 65535"
    - name: net.core.somaxconn
      value: "1024"
    
    # Windows 特有配置
    windowsOptions:
      gmsaCredentialSpecName: "gmsa-spec"
      runAsUserName: "ContainerUser"
  
  containers:
  - name: app
    image: nginx
    # 容器级 securityContext 会覆盖 Pod 级
    securityContext:
      runAsUser: 2000  # 覆盖 Pod 级的 1000
```

**fsGroup 工作原理**:
- 挂载卷时,将卷的所属组改为 `fsGroup`
- 卷的权限设置为 `g+rwX`(组可读写执行)
- 容器进程加入 `fsGroup` 组

**sysctls 分类**:
- **Safe sysctls**(默认允许):
  - `kernel.shm_rmid_forced`
  - `net.ipv4.ip_local_port_range`
  - `net.ipv4.tcp_syncookies`
  - `net.ipv4.ping_group_range`
- **Unsafe sysctls**(需要管理员明确允许): 其他所有 sysctls

---

## 调度字段

### 1. nodeSelector (简单节点选择)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nodeselector-demo
spec:
  nodeSelector:
    disktype: ssd  # 节点必须有标签 disktype=ssd
    region: us-west
  
  containers:
  - name: app
    image: nginx
```

**给节点打标签**:
```bash
kubectl label nodes node-1 disktype=ssd region=us-west
```

---

### 2. nodeName (直接指定节点)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nodename-demo
spec:
  nodeName: node-1  # 直接调度到 node-1(跳过调度器)
  
  containers:
  - name: app
    image: nginx
```

**⚠️ 注意**: 跳过调度器的资源检查,可能导致调度失败。

---

### 3. affinity (亲和性与反亲和性)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: affinity-demo
spec:
  affinity:
    # 节点亲和性
    nodeAffinity:
      # 硬性要求(必须满足)
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/hostname
            operator: In  # In | NotIn | Exists | DoesNotExist | Gt | Lt
            values:
            - node-1
            - node-2
        - matchExpressions:
          - key: disktype
            operator: Exists
      
      # 软性偏好(尽量满足)
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100  # 权重 1-100
        preference:
          matchExpressions:
          - key: region
            operator: In
            values:
            - us-west
      - weight: 50
        preference:
          matchExpressions:
          - key: zone
            operator: In
            values:
            - zone-a
    
    # Pod 亲和性(倾向于与匹配的 Pod 调度到一起)
    podAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - cache
        topologyKey: kubernetes.io/hostname  # 拓扑域: 相同节点
      
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: database
          topologyKey: topology.kubernetes.io/zone  # 拓扑域: 相同可用区
    
    # Pod 反亲和性(避免与匹配的 Pod 调度到一起)
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - web
        topologyKey: kubernetes.io/hostname  # 避免与 app=web 的 Pod 在同一节点
      
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: web
          topologyKey: topology.kubernetes.io/zone  # 尽量避免在同一可用区
  
  containers:
  - name: app
    image: nginx
```

**topologyKey 常用值**:
- `kubernetes.io/hostname`: 节点级别
- `topology.kubernetes.io/zone`: 可用区级别
- `topology.kubernetes.io/region`: 地域级别
- 自定义标签键

---

### 4. tolerations (容忍度)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tolerations-demo
spec:
  tolerations:
  # 容忍 key=value 的污点
  - key: "key1"
    operator: "Equal"  # Equal | Exists
    value: "value1"
    effect: "NoSchedule"  # NoSchedule | PreferNoSchedule | NoExecute
  
  # 容忍存在 key 的任意值污点
  - key: "key2"
    operator: "Exists"
    effect: "NoSchedule"
  
  # 容忍所有污点
  - operator: "Exists"
  
  # NoExecute 类型可指定容忍时间
  - key: "node.kubernetes.io/unreachable"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300  # 容忍 300 秒后驱逐
  
  containers:
  - name: app
    image: nginx
```

**节点污点操作**:
```bash
# 添加污点
kubectl taint nodes node-1 key1=value1:NoSchedule

# 删除污点
kubectl taint nodes node-1 key1:NoSchedule-

# 查看节点污点
kubectl describe node node-1 | grep Taints
```

**effect 类型**:
- `NoSchedule`: 不调度新 Pod(已存在的不受影响)
- `PreferNoSchedule`: 尽量不调度(软性限制)
- `NoExecute`: 不调度且驱逐已存在的 Pod

---

### 5. topologySpreadConstraints (拓扑分布约束)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: topology-spread-demo
  labels:
    app: web
spec:
  topologySpreadConstraints:
  # 约束1: 在节点维度均匀分布
  - maxSkew: 1  # 最大偏差(各拓扑域的 Pod 数量差值)
    topologyKey: kubernetes.io/hostname  # 拓扑域键
    whenUnsatisfiable: DoNotSchedule  # DoNotSchedule | ScheduleAnyway
    labelSelector:
      matchLabels:
        app: web
    minDomains: 3  # 最少分布到 3 个域(v1.25+)
  
  # 约束2: 在可用区维度均匀分布
  - maxSkew: 2
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: ScheduleAnyway  # 软性约束
    labelSelector:
      matchLabels:
        app: web
  
  containers:
  - name: app
    image: nginx
```

**使用场景**: 高可用部署(避免所有 Pod 集中在少数节点或可用区)。

---

### 6. schedulerName (自定义调度器)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: custom-scheduler-demo
spec:
  schedulerName: my-custom-scheduler  # 使用自定义调度器
  
  containers:
  - name: app
    image: nginx
```

---

### 7. priority 和 priorityClassName (优先级)

```yaml
# 定义 PriorityClass
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 1000000  # 优先级值(越大越高)
globalDefault: false  # 是否作为默认优先级
description: "High priority for critical workloads"
preemptionPolicy: PreemptLowerPriority  # PreemptLowerPriority | Never
---
apiVersion: v1
kind: Pod
metadata:
  name: priority-demo
spec:
  priorityClassName: high-priority  # 引用 PriorityClass
  
  containers:
  - name: app
    image: nginx
```

**抢占机制**: 高优先级 Pod 无法调度时,会驱逐低优先级 Pod。

---

## DNS 配置

### dnsPolicy

| 值 | 行为 |
|---|------|
| `ClusterFirst` (默认) | 使用集群 DNS(kube-dns/CoreDNS) |
| `ClusterFirstWithHostNet` | hostNetwork=true 时使用集群 DNS |
| `Default` | 继承节点的 DNS 配置(/etc/resolv.conf) |
| `None` | 使用 dnsConfig 自定义配置 |

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-policy-demo
spec:
  dnsPolicy: ClusterFirst  # 默认值
  
  containers:
  - name: app
    image: nginx
```

---

### dnsConfig (自定义 DNS)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-config-demo
spec:
  dnsPolicy: None  # 必须设置为 None
  dnsConfig:
    # DNS 服务器
    nameservers:
    - 8.8.8.8
    - 8.8.4.4
    
    # 搜索域
    searches:
    - my.dns.search.suffix
    - example.com
    
    # 选项(对应 /etc/resolv.conf 的 options)
    options:
    - name: ndots
      value: "2"
    - name: timeout
      value: "3"
    - name: attempts
      value: "2"
    - name: edns0  # 无值选项
  
  containers:
  - name: app
    image: nginx
```

**合并行为**(dnsPolicy 非 None 时):
- `nameservers` 追加到集群 DNS 之后
- `searches` 追加到集群搜索域之后
- `options` 与集群选项合并

---

### hostname 和 subdomain

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: hostname-demo
spec:
  hostname: my-pod  # Pod 主机名
  subdomain: my-service  # 子域名(需要同名 Headless Service)
  
  containers:
  - name: app
    image: nginx
```

**完整 FQDN**: `my-pod.my-service.default.svc.cluster.local`

**要求**: 需要创建同名 Headless Service:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  clusterIP: None  # Headless
  selector:
    app: myapp
```

---

## 配置示例

### 最小配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: minimal-pod
spec:
  containers:
  - name: nginx
    image: nginx:1.25-alpine
```

---

### 生产级配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: production-app
  namespace: production
  labels:
    app: web
    tier: frontend
    version: v1.2.3
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
spec:
  # ========== 初始化容器 ==========
  initContainers:
  # 等待数据库就绪
  - name: wait-for-db
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      until nc -z postgres.production.svc.cluster.local 5432; do
        echo "等待数据库就绪..."
        sleep 2
      done
      echo "数据库已就绪!"
  
  # 原生 Sidecar: 日志收集器(v1.29+)
  - name: log-shipper
    image: fluent/fluent-bit:2.1
    restartPolicy: Always  # 标识为 Sidecar
    volumeMounts:
    - name: app-logs
      mountPath: /var/log/app
    env:
    - name: FLUENT_ELASTICSEARCH_HOST
      valueFrom:
        configMapKeyRef:
          name: logging-config
          key: es.host
  
  # ========== 主容器 ==========
  containers:
  # 应用容器
  - name: app
    image: myregistry.com/myapp:1.2.3
    imagePullPolicy: IfNotPresent
    
    # 命令和参数
    command: ["/app/server"]
    args: ["--config", "/etc/app/config.yaml"]
    
    # 端口
    ports:
    - name: http
      containerPort: 8080
      protocol: TCP
    - name: metrics
      containerPort: 9090
      protocol: TCP
    
    # 环境变量
    env:
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    - name: DATABASE_HOST
      value: postgres.production.svc.cluster.local
    - name: DATABASE_USER
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: username
    - name: DATABASE_PASSWORD
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: password
    - name: LOG_LEVEL
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: log.level
    
    # 资源配置
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
        ephemeral-storage: "1Gi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
        ephemeral-storage: "2Gi"
    
    # 卷挂载
    volumeMounts:
    - name: app-config-volume
      mountPath: /etc/app
      readOnly: true
    - name: app-logs
      mountPath: /var/log/app
    - name: cache
      mountPath: /tmp/cache
    - name: data
      mountPath: /data
    
    # 启动探针(慢启动应用)
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 30  # 最多等待 300 秒
    
    # 存活探针
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
        httpHeaders:
        - name: X-Health-Check
          value: "liveness"
      initialDelaySeconds: 30
      periodSeconds: 10
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 3
    
    # 就绪探针
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
        httpHeaders:
        - name: X-Health-Check
          value: "readiness"
      initialDelaySeconds: 10
      periodSeconds: 5
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 3
    
    # 生命周期钩子
    lifecycle:
      postStart:
        exec:
          command:
          - /bin/sh
          - -c
          - echo "容器启动于 $(date)" >> /var/log/app/lifecycle.log
      
      preStop:
        exec:
          command:
          - /bin/sh
          - -c
          - |
            # 优雅停机: 停止接收新请求,等待现有请求完成
            kill -TERM 1
            sleep 15
    
    # 容器级安全配置
    securityContext:
      runAsUser: 1000
      runAsGroup: 3000
      runAsNonRoot: true
      readOnlyRootFilesystem: true
      allowPrivilegeEscalation: false
      capabilities:
        drop:
        - ALL
      seccompProfile:
        type: RuntimeDefault
  
  # Sidecar: Envoy 代理(传统方式)
  - name: envoy
    image: envoyproxy/envoy:v1.28-latest
    ports:
    - name: proxy
      containerPort: 15001
    volumeMounts:
    - name: envoy-config
      mountPath: /etc/envoy
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "200m"
        memory: "256Mi"
  
  # Sidecar: Prometheus exporter
  - name: metrics-exporter
    image: prom/statsd-exporter:v0.26.0
    ports:
    - name: metrics
      containerPort: 9102
    resources:
      requests:
        cpu: "50m"
        memory: "64Mi"
      limits:
        cpu: "100m"
        memory: "128Mi"
  
  # ========== 卷配置 ==========
  volumes:
  # ConfigMap 卷
  - name: app-config-volume
    configMap:
      name: app-config
      defaultMode: 0644
  
  # 日志卷(与 Sidecar 共享)
  - name: app-logs
    emptyDir:
      sizeLimit: 1Gi
  
  # 缓存卷(内存)
  - name: cache
    emptyDir:
      medium: Memory
      sizeLimit: 512Mi
  
  # 持久化数据
  - name: data
    persistentVolumeClaim:
      claimName: app-data-pvc
  
  # Envoy 配置
  - name: envoy-config
    configMap:
      name: envoy-config
  
  # ========== Pod 级配置 ==========
  # 重启策略
  restartPolicy: Always
  
  # 优雅终止时间
  terminationGracePeriodSeconds: 60
  
  # 服务账户
  serviceAccountName: app-sa
  automountServiceAccountToken: true
  
  # 镜像拉取凭证
  imagePullSecrets:
  - name: myregistry-secret
  
  # Pod 级安全配置
  securityContext:
    runAsUser: 1000
    runAsGroup: 3000
    fsGroup: 2000
    fsGroupChangePolicy: OnRootMismatch
    runAsNonRoot: true
    seccompProfile:
      type: RuntimeDefault
  
  # DNS 配置
  dnsPolicy: ClusterFirst
  dnsConfig:
    options:
    - name: ndots
      value: "2"
    - name: timeout
      value: "3"
  
  # ========== 调度配置 ==========
  # 节点选择器
  nodeSelector:
    disktype: ssd
    workload: web
  
  # 亲和性
  affinity:
    # 节点亲和性: 必须在 us-west 或 us-east 区域
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: topology.kubernetes.io/region
            operator: In
            values:
            - us-west
            - us-east
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        preference:
          matchExpressions:
          - key: node-type
            operator: In
            values:
            - high-memory
    
    # Pod 反亲和性: 避免多个副本在同一节点
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - web
        topologyKey: kubernetes.io/hostname
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: web
          topologyKey: topology.kubernetes.io/zone
  
  # 容忍度
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "web"
    effect: "NoSchedule"
  - key: "node.kubernetes.io/unreachable"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300
  
  # 拓扑分布约束
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: kubernetes.io/hostname
    whenUnsatisfiable: DoNotSchedule
    labelSelector:
      matchLabels:
        app: web
  - maxSkew: 2
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: ScheduleAnyway
    labelSelector:
      matchLabels:
        app: web
  
  # 优先级
  priorityClassName: high-priority
  
  # 主机名配置
  hostname: web-app
  subdomain: web-service
  
  # 共享进程命名空间
  shareProcessNamespace: false
  
  # 启用服务环境变量
  enableServiceLinks: false
```

---

## 内部原理

### 1. Pod 生命周期状态机

```
┌─────────┐
│ Pending │ ← Pod 已创建,等待调度或拉取镜像
└────┬────┘
     │
     ↓
┌──────────┐
│ Running  │ ← 至少一个容器正在运行
└────┬─────┘
     │
     ├──→ ┌───────────┐
     │    │ Succeeded │ ← 所有容器成功终止(退出码 0)
     │    └───────────┘
     │
     └──→ ┌────────┐
          │ Failed │ ← 至少一个容器失败终止(非 0 退出码)
          └────────┘
     
     ┌─────────┐
     │ Unknown │ ← 无法获取 Pod 状态(通常是节点失联)
     └─────────┘
```

**Phase 详解**:
| Phase | 描述 | Conditions |
|-------|------|------------|
| `Pending` | 调度中或拉取镜像 | PodScheduled, ContainersReady |
| `Running` | 至少一个容器运行中 | Initialized, Ready |
| `Succeeded` | 所有容器成功终止 | - |
| `Failed` | 至少一个容器失败 | - |
| `Unknown` | 状态未知(节点失联) | - |

**Conditions 字段**:
```yaml
status:
  conditions:
  - type: PodScheduled      # Pod 已调度到节点
    status: "True"
  - type: Initialized       # 所有 Init 容器成功
    status: "True"
  - type: ContainersReady   # 所有容器就绪
    status: "True"
  - type: Ready             # Pod 就绪(可接收流量)
    status: "True"
```

---

### 2. 容器状态

```yaml
status:
  containerStatuses:
  - name: app
    state:
      # 三种互斥状态之一
      waiting:          # 等待中(拉取镜像、等待 Init 容器)
        reason: ContainerCreating
      
      # running:        # 运行中
      #   startedAt: "2026-02-10T10:00:00Z"
      
      # terminated:     # 已终止
      #   exitCode: 0
      #   reason: Completed
      #   startedAt: "2026-02-10T10:00:00Z"
      #   finishedAt: "2026-02-10T10:05:00Z"
    
    ready: true
    restartCount: 0
    image: nginx:1.25-alpine
    imageID: docker-pullable://nginx@sha256:...
    containerID: containerd://abc123...
```

---

### 3. QoS 类别计算

**Guaranteed** (最高 QoS):
```yaml
containers:
- name: app
  resources:
    requests:
      cpu: "500m"
      memory: "512Mi"
    limits:
      cpu: "500m"       # 必须等于 requests
      memory: "512Mi"   # 必须等于 requests
```

**Burstable** (中等 QoS):
```yaml
containers:
- name: app
  resources:
    requests:
      cpu: "250m"
      memory: "256Mi"
    limits:
      cpu: "1000m"      # 可大于 requests
      memory: "1Gi"     # 可大于 requests
```

**BestEffort** (最低 QoS):
```yaml
containers:
- name: app
  # 无 resources 配置
```

**驱逐顺序**(节点资源不足时):
1. BestEffort (最先驱逐)
2. Burstable (超出 requests 的优先驱逐)
3. Guaranteed (最后驱逐)

---

### 4. Downward API 可用字段汇总

**metadata 字段**:
- `metadata.name`: Pod 名称
- `metadata.namespace`: Pod 命名空间
- `metadata.uid`: Pod UID
- `metadata.labels['<KEY>']`: 标签值
- `metadata.annotations['<KEY>']`: 注解值

**spec 字段**:
- `spec.nodeName`: 节点名称
- `spec.serviceAccountName`: 服务账户名

**status 字段**:
- `status.hostIP`: 节点 IP
- `status.podIP`: Pod IP
- `status.podIPs`: Pod IPs (双栈)

**容器资源字段**(仅 resourceFieldRef):
- `requests.cpu`
- `requests.memory`
- `requests.ephemeral-storage`
- `limits.cpu`
- `limits.memory`
- `limits.ephemeral-storage`

---

### 5. Pod 创建流程

```
1. kubectl apply
   │
   ↓
2. API Server 验证并写入 etcd
   │
   ↓
3. Scheduler 监听到新 Pod,执行调度算法
   │  (过滤节点 → 打分 → 选择最优节点)
   │
   ↓
4. Scheduler 更新 Pod.spec.nodeName
   │
   ↓
5. Kubelet 监听到调度到本节点的 Pod
   │
   ↓
6. Kubelet 执行:
   │  a. 创建 Pod 沙箱(Pause 容器)
   │  b. 拉取镜像
   │  c. 启动 Init 容器(顺序执行)
   │  d. 启动主容器和 Sidecar
   │  e. 执行 postStart 钩子
   │
   ↓
7. Kubelet 持续监控容器状态
   │  (执行探针检测)
   │
   ↓
8. Pod 状态更新到 API Server
```

---

### 6. Pod 终止流程

```
1. kubectl delete pod
   │
   ↓
2. API Server 标记 Pod deletionTimestamp
   │
   ↓
3. 并行执行:
   │
   ├─→ Endpoint Controller 从 Service Endpoints 移除 Pod
   │
   ├─→ Kubelet 执行:
   │    a. 执行 preStop 钩子
   │    b. 发送 SIGTERM 信号给容器
   │    c. 等待 terminationGracePeriodSeconds (默认 30 秒)
   │    d. 超时则发送 SIGKILL 强制杀死
   │
   ↓
4. Kubelet 清理 Pod 资源
   │  (删除 Pause 容器、卷挂载等)
   │
   ↓
5. Kubelet 更新 Pod 状态为 Terminated
   │
   ↓
6. API Server 从 etcd 删除 Pod 对象
```

**优雅终止最佳实践**:
1. 设置合理的 `terminationGracePeriodSeconds`(建议 60-120 秒)
2. 实现 `preStop` 钩子停止接收新请求
3. 应用监听 SIGTERM 信号并优雅关闭

---

## 版本兼容性矩阵

| 功能 | 引入版本 | GA 版本 | 说明 |
|------|---------|---------|------|
| **startupProbe** | v1.16 (Alpha) | v1.20 (Stable) | 启动探针 |
| **ephemeralContainers** | v1.16 (Alpha) | v1.25 (Stable) | 临时调试容器 |
| **topologySpreadConstraints** | v1.16 (Alpha) | v1.19 (Stable) | 拓扑分布约束 |
| **minDomains** | v1.25 (Alpha) | v1.30 (Beta) | topologySpreadConstraints 最小域数 |
| **sizeLimit (emptyDir)** | v1.22 (Stable) | v1.22 | emptyDir 大小限制 |
| **Sidecar Containers** | v1.28 (Alpha) | v1.29 (Beta) | initContainers.restartPolicy |
| **Container Resize** | v1.27 (Alpha) | - | 容器资源在线调整 |
| **PodSchedulingGates** | v1.26 (Alpha) | v1.27 (Beta) | 调度门控 |
| **PodResourceClaims** | v1.26 (Alpha) | - | 动态资源声明(DRA) |
| **readinessProbe.terminationGracePeriodSeconds** | v1.29 (Alpha) | - | 探针级优雅终止时间 |
| **grpc probe** | v1.24 (Beta) | v1.27 (Stable) | gRPC 探针 |
| **AppArmor** | v1.4 (Beta) | - | AppArmor 配置 |
| **Seccomp** | v1.19 (Beta) | v1.22 (Stable) | Seccomp 配置 |

**弃用功能**:
| 功能 | 弃用版本 | 移除版本 | 替代方案 |
|------|---------|---------|---------|
| **Pod Security Policy** | v1.21 | v1.25 | Pod Security Standards |
| **云厂商 in-tree 卷插件** | v1.21 | v1.26+ | CSI 驱动 |

---

## 最佳实践

### 1. 资源管理

✅ **推荐**:
- 所有容器设置 `requests` 和 `limits`
- requests = limits (Guaranteed QoS)用于关键服务
- 使用 VPA(Vertical Pod Autoscaler)自动调整资源

❌ **避免**:
- 不设置资源限制(可能耗尽节点资源)
- limits 过大(浪费资源)
- requests 过小(频繁 OOMKilled)

---

### 2. 健康检查

✅ **推荐**:
- 慢启动应用使用 `startupProbe`
- 所有服务实现 `readinessProbe`
- 长时间运行服务实现 `livenessProbe`
- 探针端点轻量级(避免复杂逻辑)

❌ **避免**:
- readinessProbe 检查外部依赖(会导致级联故障)
- livenessProbe 超时过短(误杀健康容器)
- 探针 initialDelaySeconds 过短(容器未启动完成)

---

### 3. 安全配置

✅ **推荐**:
```yaml
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
  capabilities:
    drop: [ALL]
  seccompProfile:
    type: RuntimeDefault
```

❌ **避免**:
- 使用 root 用户运行
- privileged: true
- hostNetwork: true (除非必要)
- hostPath 卷(除非必要)

---

### 4. 高可用

✅ **推荐**:
- 使用 `podAntiAffinity` 分散副本
- 使用 `topologySpreadConstraints` 跨可用区分布
- 设置合理的 `terminationGracePeriodSeconds`
- 实现 `preStop` 钩子优雅停机

❌ **避免**:
- 单副本关键服务
- 所有副本在同一节点/可用区
- 立即强制终止(SIGKILL)

---

### 5. 日志和监控

✅ **推荐**:
- 日志输出到 stdout/stderr
- 使用 Sidecar 收集日志
- 暴露 Prometheus metrics 端口
- 添加 `prometheus.io/scrape` 注解

❌ **避免**:
- 日志写入容器文件系统(占用存储)
- 在应用中直接推送日志(耦合度高)

---

### 6. 镜像管理

✅ **推荐**:
- 使用特定版本标签(如 `1.25.3`)
- 使用 digest 引用(如 `@sha256:...`)
- 定期更新基础镜像(安全补丁)
- 使用私有镜像仓库

❌ **避免**:
- 使用 `latest` 标签(不可预测)
- 镜像过大(拉取慢)
- 在镜像中硬编码配置

---

### 7. 配置管理

✅ **推荐**:
- 使用 ConfigMap/Secret 存储配置
- 敏感数据用 Secret + 加密(如 Sealed Secrets)
- 使用 `subPath` 挂载单个文件(避免覆盖目录)

❌ **避免**:
- 环境变量中明文存储密码
- 配置硬编码在镜像中
- ConfigMap 超过 1MB(影响 etcd 性能)

---

### 8. 调度优化

✅ **推荐**:
- 关键服务使用 PriorityClass
- 使用 nodeSelector/nodeAffinity 调度到合适节点
- 设置 Pod Disruption Budget(PDB)

❌ **避免**:
- 使用 `nodeName` 硬绑定(跳过调度器)
- 过度使用 `requiredDuringScheduling`(可能无法调度)

---

## FAQ

### Q1: Pod 一直处于 Pending 状态?

**原因**:
1. 资源不足(CPU/Memory/PVC)
2. 节点亲和性/污点无法满足
3. ImagePullBackOff(镜像拉取失败)
4. PVC 无法绑定

**排查**:
```bash
kubectl describe pod <pod-name>
kubectl get events --sort-by=.metadata.creationTimestamp
kubectl get nodes -o wide
```

---

### Q2: Pod 频繁重启?

**原因**:
1. OOMKilled(内存超限)
2. livenessProbe 失败
3. 应用崩溃(退出码非 0)
4. 节点资源压力驱逐

**排查**:
```bash
kubectl describe pod <pod-name>
kubectl logs <pod-name> --previous  # 查看上次容器日志
kubectl get pod <pod-name> -o yaml | grep -A 10 "lastState:"
```

---

### Q3: Init 容器与 Sidecar 容器如何选择?

| 场景 | 使用 |
|------|------|
| 等待依赖服务 | Init 容器 |
| 数据库迁移 | Init 容器 |
| 下载配置文件 | Init 容器 |
| 日志收集 | Sidecar 容器 |
| 服务网格代理 | Sidecar 容器 |
| 监控 exporter | Sidecar 容器 |

---

### Q4: 如何实现容器间文件共享?

**方法1**: emptyDir 卷
```yaml
volumes:
- name: shared
  emptyDir: {}
```

**方法2**: PVC 共享(需支持 ReadWriteMany)
```yaml
volumes:
- name: shared
  persistentVolumeClaim:
    claimName: shared-pvc
```

**方法3**: Projected 卷(只读共享 ConfigMap/Secret)
```yaml
volumes:
- name: shared
  projected:
    sources:
    - configMap:
        name: config1
    - secret:
        name: secret1
```

---

### Q5: 如何调试 CrashLoopBackOff 的 Pod?

**方法1**: 查看容器日志
```bash
kubectl logs <pod-name> --previous
```

**方法2**: 使用 Ephemeral Containers
```bash
kubectl debug -it <pod-name> --image=busybox --target=<container-name>
```

**方法3**: 覆盖容器命令(阻止退出)
```yaml
containers:
- name: app
  image: myapp
  command: ["/bin/sh", "-c", "sleep 3600"]  # 保持容器运行
```

**方法4**: 查看事件
```bash
kubectl get events --field-selector involvedObject.name=<pod-name>
```

---

### Q6: Pod 如何访问宿主机服务?

**方法1**: hostNetwork
```yaml
spec:
  hostNetwork: true  # 使用宿主机网络
```

**方法2**: Downward API(获取节点 IP)
```yaml
env:
- name: HOST_IP
  valueFrom:
    fieldRef:
      fieldPath: status.hostIP
```

**方法3**: hostPort
```yaml
containers:
- name: app
  ports:
  - containerPort: 8080
    hostPort: 8080  # 绑定到宿主机端口
```

---

### Q7: 如何限制 Pod 的临时存储使用?

```yaml
containers:
- name: app
  resources:
    requests:
      ephemeral-storage: "1Gi"
    limits:
      ephemeral-storage: "2Gi"

volumes:
- name: cache
  emptyDir:
    sizeLimit: 1Gi  # 限制 emptyDir 大小(v1.22+)
```

---

### Q8: Pod 的 DNS 解析慢?

**优化 dnsConfig**:
```yaml
dnsConfig:
  options:
  - name: ndots
    value: "2"        # 减少 DNS 搜索尝试次数
  - name: timeout
    value: "2"        # 减少超时时间
  - name: attempts
    value: "2"        # 减少重试次数
  - name: single-request-reopen  # TCP 和 UDP 使用不同端口
```

**禁用不必要的服务环境变量**:
```yaml
spec:
  enableServiceLinks: false
```

---

## 生产案例

### 案例 1: 电商网站高可用部署

**需求**:
- 多副本分散到不同节点和可用区
- 使用 SSD 节点
- 与数据库 Pod 亲和
- Prometheus 监控集成
- 优雅滚动更新

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ecommerce-web
  labels:
    app: ecommerce
    tier: web
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
    prometheus.io/path: "/metrics"
spec:
  priorityClassName: high-priority
  
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: disktype
            operator: In
            values: [ssd]
    
    podAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: ecommerce
              tier: database
          topologyKey: topology.kubernetes.io/zone
    
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: ecommerce
            tier: web
        topologyKey: kubernetes.io/hostname
  
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
    labelSelector:
      matchLabels:
        app: ecommerce
        tier: web
  
  containers:
  - name: web
    image: ecommerce/web:2.1.0
    resources:
      requests:
        cpu: "1000m"
        memory: "2Gi"
      limits:
        cpu: "2000m"
        memory: "4Gi"
    
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
    
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 10
      periodSeconds: 5
    
    lifecycle:
      preStop:
        exec:
          command:
          - /bin/sh
          - -c
          - sleep 15  # 等待负载均衡器移除
    
    securityContext:
      runAsNonRoot: true
      readOnlyRootFilesystem: true
      allowPrivilegeEscalation: false
  
  - name: metrics-exporter
    image: prom/statsd-exporter:v0.26.0
    ports:
    - name: metrics
      containerPort: 9090
  
  terminationGracePeriodSeconds: 60
```

---

### 案例 2: 机器学习训练任务

**需求**:
- GPU 资源
- 大内存
- 持久化模型存储
- 训练完成自动退出

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ml-training-job
spec:
  restartPolicy: Never  # 训练完成不重启
  
  nodeSelector:
    accelerator: nvidia-tesla-v100
  
  tolerations:
  - key: nvidia.com/gpu
    operator: Exists
    effect: NoSchedule
  
  containers:
  - name: trainer
    image: ml/pytorch-trainer:1.12-cuda11.8
    command: ["python", "train.py"]
    
    resources:
      requests:
        cpu: "4000m"
        memory: "32Gi"
        nvidia.com/gpu: "2"
      limits:
        cpu: "8000m"
        memory: "64Gi"
        nvidia.com/gpu: "2"
    
    volumeMounts:
    - name: dataset
      mountPath: /data
      readOnly: true
    - name: model-output
      mountPath: /output
    - name: shm
      mountPath: /dev/shm  # 共享内存(PyTorch DataLoader)
    
    env:
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
    - name: CUDA_VISIBLE_DEVICES
      value: "0,1"
  
  volumes:
  - name: dataset
    persistentVolumeClaim:
      claimName: ml-dataset-pvc
  - name: model-output
    persistentVolumeClaim:
      claimName: ml-model-pvc
  - name: shm
    emptyDir:
      medium: Memory
      sizeLimit: 16Gi
```

---

### 案例 3: 多租户 SaaS 平台

**需求**:
- 租户隔离(不同 ServiceAccount)
- 资源配额
- 网络隔离(NetworkPolicy)
- 审计日志

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tenant-app-123
  namespace: tenant-123
  labels:
    tenant: "123"
    app: saas-app
spec:
  serviceAccountName: tenant-123-sa
  automountServiceAccountToken: true
  
  securityContext:
    runAsUser: 10123  # 租户专用 UID
    runAsGroup: 10123
    fsGroup: 10123
    runAsNonRoot: true
    seccompProfile:
      type: RuntimeDefault
  
  containers:
  - name: app
    image: saas-platform/app:3.0
    
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
    
    env:
    - name: TENANT_ID
      value: "123"
    - name: DATABASE_URL
      valueFrom:
        secretKeyRef:
          name: tenant-123-db-secret
          key: url
    
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop: [ALL]
      readOnlyRootFilesystem: true
    
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /app/cache
  
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir:
      sizeLimit: 100Mi
```

---

## 总结

Pod 是 Kubernetes 中最核心的资源对象,理解其完整规格对于构建生产级应用至关重要。本文档涵盖:

✅ **完整字段规格**: 所有 spec 顶层字段和 Container 字段
✅ **实战配置**: 从最小到生产级的完整示例
✅ **深入原理**: 生命周期、QoS、调度机制
✅ **最佳实践**: 资源管理、安全、高可用、监控
✅ **故障排查**: 常见问题的诊断和解决方法
✅ **生产案例**: 电商、机器学习、SaaS 等真实场景

**推荐学习路径**:
1. 从最小配置开始,逐步理解每个字段
2. 实践探针、资源管理、安全配置
3. 掌握调度、亲和性、拓扑分布
4. 学习 Init 容器和 Sidecar 模式
5. 深入理解生命周期和 QoS 机制
6. 应用到生产环境并持续优化

**相关文档**:
- [02 - ConfigMap 配置管理](./02-configmap-complete.md)
- [04 - Service 服务暴露](./04-service-complete.md)
- [05 - Deployment 声明式部署](./05-deployment-complete.md)
