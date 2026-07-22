---
title: Init Containers
summary: Init Containers 在应用容器启动前运行，用于执行初始化任务如配置生成、依赖检查等。
category: concepts
tags:
- pod
- init
- startup
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Init Containers

## 概述

Init Containers 是 Pod 中一类特殊的容器，它们在**应用容器之前按顺序执行**，且必须全部成功退出（exit 0）后才会启动应用容器。Init 容器用于把"启动前的准备"从应用代码中解耦：等待依赖（数据库、外部 API）就绪、生成配置文件、下载/解密密钥、初始化数据目录、注册服务发现、迁移数据库 schema 等。它们可以拥有与应用不同的镜像、安全上下文和资源视图，便于使用专门工具（如 wait-for-it、jq、vault cli）完成初始化逻辑。

## 架构与工作原理

```
Pod 启动顺序（严格串行）：
  ┌──────────────────────────────────────────────┐
  │ 1. initContainers[0]   → 必须成功 exit 0       │
  │ 2. initContainers[1]   → 必须成功 exit 0       │
  │ 3. initContainers[2]   → 必须成功 exit 0       │
  │ 4. containers[*]       → 并行启动并常驻         │
  └──────────────────────────────────────────────┘
        │ 共享 Pod 的 volumes / network namespace
```

**关键行为**：
- **串行执行**：多个 init 容器按声明顺序逐个运行，前一个成功才运行下一个。
- **必须成功**：任意 init 容器失败，整个 Pod 按 `restartPolicy` 重启（Always/OnFailure 重新运行 init；Never 则 Pod Failed）。
- **资源/init 阶段不计入应用 limit**：init 容器按最高资源请求作为调度依据（取所有 init 中最大的）。
- **共享卷**：init 容器可通过 emptyDir 把生成的配置传递给应用容器。
- **重启行为**：容器运行时重启 Pod（如 kubelet 重启）会**重新执行所有 init**——这是与"sidecar init"（1.28+ 的 `restartPolicy: Always` init）的区别。

**Init vs Sidecar（1.29 GA）**：
传统 init 容器是"一次性"的，跑完即退出。1.28+ 引入 `restartPolicy: Always` 的 init 容器（Sidecar），它常驻运行、与应用容器并行，但**保证在应用容器之前启动就绪**，解决了旧 sidecar 启动顺序不确定的问题。

## 关键组件与特性

| 特性 | 说明 |
|------|------|
| `spec.initContainers[]` | init 容器列表，按序执行 |
| 资源计算 | 调度取所有 init 与应用 requests 的较大者 |
| 共享卷 | emptyDir / configMap 桥接 init 与 app |
| restartPolicy | Always/OnFailure 触发重新执行 init |
| 1.28+ sidecar init | `restartPolicy: Always` 的 init，常驻并先启动 |
| 失败重试 | 按 backoff 重试，最终 CrashLoopBackOff |

## 配置示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata: {name: webapp, namespace: production}
spec:
  replicas: 3
  selector: {matchLabels: {app: webapp}}
  template:
    metadata: {labels: {app: webapp}}
    spec:
      initContainers:
      # 1. 等待数据库可达
      - name: wait-db
        image: busybox:1.36
        env:
        - {name: DB_HOST, value: db.production.svc}
        command: ['sh', '-c', 'until nc -z $DB_HOST 5432; do echo waiting db; sleep 2; done']
      # 2. 从 Vault 拉取配置写入共享卷
      - name: fetch-config
        image: vault:1.13
        env:
        - {name: VAULT_ADDR, value: http://vault:8200}
        command:
        - sh
        - -c
        - |
          vault login -no-store $VAULT_TOKEN
          vault kv get -field=config secret/webapp > /config/app.yaml
        volumeMounts:
        - {name: cfg, mountPath: /config}
      # 3. 数据库迁移
      - name: migrate
        image: webapp:migrator-v1.2
        command: ['./migrate', 'up']
        envFrom:
        - secretRef: {name: db-creds}
      containers:
      - name: webapp
        image: webapp:v1.2.0
        volumeMounts:
        - {name: cfg, mountPath: /etc/webapp, readOnly: true}
        ports: [{containerPort: 8080}]
      volumes:
      - name: cfg
        emptyDir: {}
---
# 1.29+ Sidecar 模式 init（常驻，先于应用就绪）
spec:
  initContainers:
  - name: envoy-proxy
    image: envoyproxy/envoy:v1.29
    restartPolicy: Always        # ← 关键：声明为 sidecar
    # ... 必须进入 Ready 后应用容器才启动
```

## 常用操作与命令

```bash
# 查看 init 容器状态
kubectl describe pod webapp-xxx | grep -A20 Init
kubectl get pod webapp-xxx -o jsonpath='{.status.initContainerStatuses}'

# 查看 init 容器日志（指定 -c）
kubectl logs webapp-xxx -c wait-db
kubectl logs webapp-xxx -c fetch-config
kubectl logs webapp-xxx --all-containers --init-containers

# 查看历史日志（init 崩溃重启后看上一次）
kubectl logs webapp-xxx -c migrate --previous

# 排查 Pod 卡在 Init 阶段
kubectl get pod webapp-xxx        # STATUS: Init:0/2 或 Init:Error
kubectl get events --field-selector involvedObject.name=webapp-xxx

# 调试：手动复现 init 逻辑
kubectl run debug --image=busybox -it --rm --restart=Never -- \
  sh -c 'until nc -z db.production 5432; do sleep 2; done'
```

## 最佳实践

1. **init 容器只做"准备"**：不要把业务逻辑塞进 init，保持幂等、快速、可重试。
2. **依赖等待用工具而非 sleep**：`nc -z` / `wait-for-it` 主动探测，避免固定 sleep 不可靠。
3. **敏感操作放 init**：DB 迁移、密钥解密只执行一次，避免应用容器每次重启重复。
4. **共享卷传递产物**：init 生成配置写 emptyDir，app 挂载只读，避免配置注入到环境变量。
5. **init 容器设资源 limit**：避免迁移脚本吃光节点内存影响调度。
6. **1.29+ 用 sidecar init**：envoy/log-shipper 等常驻 sidecar 用 `restartPolicy: Always` 的 init，保证启动顺序。
7. **失败要明确退出码**：init 失败应 `exit 1` 并打印可读错误，便于排查。

## 常见陷阱

- **Pod 一直 Init:CrashLoopBackOff**：init 容器命令写错、依赖服务不可达、镜像 tag 不存在。
- **init 阻塞应用启动**：等待依赖的 init 用了无超时循环，依赖挂了 Pod 永远起不来。
- **资源计算放大**：所有 init 取最大 request 作为调度请求，多个大内存 init 叠加导致难调度。
- **Pod 重启重新执行 init**：kubelet 重启会重新跑所有 init，若 init 有副作用（如重复迁移）需幂等。
- **sidecar 启动顺序**：1.27 以前的 sidecar 作为普通容器，应用可能先于 sidecar 启动导致连接失败；升级到 1.29+ 用 sidecar init。
- **init 容器看不到应用环境变量**：envFrom/env 在每个容器独立声明，init 需要的变量要重复定义。
- **v1.29 前 sidecar 优雅停止问题**：sidecar 在应用容器退出后不会自动退出，导致 Pod 卡在 Terminating。

## 源码实现分析

### kubelet Init Container 执行流程

```go
// k8s.io/kubernetes/pkg/kubelet/kuberuntime/kuberuntime_manager.go
// kubelet 按顺序执行 init containers，每个必须成功退出
func (m *kubeGenericRuntimeManager) computePodActions(ctx context.Context, pod *v1.Pod) {
    // 1. 找到下一个未完成的 init container
    for i, initContainer := range pod.Spec.InitContainers {
        status := findContainerStatus(initContainer)
        if status == nil {
            // 未启动：启动这个 init container
            return startContainer(initContainer)
        }
        if status.State.Terminated != nil && status.State.Terminated.ExitCode != 0 {
            // 失败：重启这个 init container（CrashLoopBackOff）
            return startContainer(initContainer)
        }
        if status.State.Running != nil {
            // 正在运行：等待其完成
            return wait
        }
        // 成功完成：继续下一个
    }
    // 2. 所有 init 完成后，启动主容器
    startMainContainers()
}
```

### Init Container 执行流程

```
┌───────────────────────────────────────────────────────────┐
│          Init Container 执行流程                      │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Pod 创建                                                │
│    │                                                      │
│    ▼                                                      │
│  init-1: wait-for-db (nc -z db 5432)                    │
│    │  exit 0 → 继续                                     │
│    ▼                                                      │
│  init-2: db-migrate (flyway migrate)                    │
│    │  exit 0 → 继续                                     │
│    ▼                                                      │
│  init-3: config-gen (生成配置文件到 emptyDir)          │
│    │  exit 0 → 继续                                     │
│    ▼                                                      │
│  主容器启动 (app)                                       │
│                                                           │
│  关键规则:                                               │
│  • 严格顺序执行，前一个成功才启动下一个             │
│  • 任何一个失败 → Pod 重启时重新执行所有 init      │
│  • init 必须幂等（重复执行不产生副作用）            │
│  • 资源调度：取所有 init 中最大的 requests          │
└───────────────────────────────────────────────────────────┘
```

### 生产配置示例（🟡 部署到集群）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-init
spec:
  initContainers:
  # 等待数据库就绪（带超时）
  - name: wait-for-db
    image: busybox:1.36
    command: ['sh', '-c',
      'for i in $(seq 1 30); do nc -z postgres-svc 5432 && exit 0; sleep 2; done; exit 1']
    resources:
      limits:
        cpu: 50m
        memory: 32Mi
  # 数据库迁移（幂等）
  - name: db-migrate
    image: my-app:1.0
    command: ['./migrate', '--idempotent']
    resources:
      limits:
        cpu: 500m
        memory: 256Mi
  containers:
  - name: app
    image: my-app:1.0
    readinessProbe:
      httpGet:
        path: /health
        port: 8080
```

## 面试要点

1. **Init Container 的执行规则？**
   - 严格顺序执行，前一个成功才启动下一个
   - 任何一个失败，Pod 重启时重新执行所有 init
   - 必须幂等，因为可能被重复执行

2. **Init Container 与主容器的资源计算区别？**
   - 调度时：取所有 init 中最大的 requests（因为顺序执行）
   - 运行时：init 和主容器不同时运行
   - limits：每个容器独立设置

3. **1.29 sidecar init 与普通 init 的区别？**
   - 普通 init：运行完退出，然后下一个
   - sidecar init：restartPolicy=Always，启动后不退出
   - 用途：envoy/log-collector 等常驻 sidecar

4. **init 容器等待依赖的最佳实践？**
   - 用 nc -z / curl 主动探测，不用 sleep
   - 必须设超时（for 循环 + 计数器）
   - 失败时打印可读错误信息

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/pods.md|Pod]] — init 容器的宿主
- [[概念/sidecar-containers.md|Sidecar Containers]]
- [[概念/ephemeral-containers.md|Ephemeral Containers]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
