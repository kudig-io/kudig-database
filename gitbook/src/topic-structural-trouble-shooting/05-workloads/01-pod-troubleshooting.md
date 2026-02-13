# Pod 故障排查与运行机制深度指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 解决 Pod 无法启动、不断重启或被杀掉的问题 | 掌握 Pod 状态机、标准排查 4 步法（Get/Describe/Logs/Exec）。 |
| **中级运维** | 优化资源分配、解决复杂的挂载与网络连通性问题 | 理解 OOM 原理、探针调优、镜像拉取优化与 InitContainers 应用。 |
| **资深专家** | 处理内核级故障、僵尸进程与大规模调度瓶颈 | 深入 Pod Sandbox 机制、PID 1 治理、eBPF 辅助诊断与 Sidecar 容器启动顺序调优。 |

---

## 0. 10 分钟快速诊断

1. **四步法**：`kubectl get pod <pod> -o wide` → `kubectl describe pod <pod>` → `kubectl logs <pod>` → `kubectl exec <pod> -- <cmd>`。
2. **定位阶段**：Pending/ContainerCreating/Running/CrashLoopBackOff/OOMKilled，结合 Events 判断主因。
3. **镜像与拉取**：`kubectl describe pod | grep -A2 Image`，确认镜像、密钥、限流。
4. **资源与驱逐**：`kubectl top pod --containers`、`kubectl describe node | grep -A3 Pressure`。
5. **网络/存储**：必要时验证 DNS/Service 连通与 PVC 挂载状态。
6. **快速缓解**：
   - CrashLoop：先看 `--previous` 日志，临时放宽探针或回滚配置。
   - Pending：检查资源/污点/亲和/配额。
7. **证据留存**：保存 Events、日志、容器退出码与节点资源快照。

---

## 1. 核心原理与底层机制

### 1.1 Pod Sandbox 机制深度解析

#### 1.1.1 Pause 容器（Infra Container）架构

Pod 的第一个容器始终是 **Pause 容器**（也称 Sandbox Container），它是 Pod 网络和命名空间的"锚点"：

| 组件 | 职责 | 技术实现 |
|:----|:-----|:---------|
| **Network Namespace** | 持有 Pod 的 IP 地址、路由表、iptables 规则 | Pause 容器创建 `netns`，业务容器通过 `setns()` 加入 |
| **IPC Namespace** | 共享进程间通信（共享内存、信号量、消息队列） | 所有容器可通过 `/dev/shm` 通信 |
| **UTS Namespace** | 共享主机名（Pod 名称） | 所有容器看到相同的 hostname |
| **Mount Namespace** | 每个容器独立的文件系统视图 | 容器间不共享，但可通过 Volume 交换数据 |
| **PID Namespace** | 默认隔离，可选共享（`shareProcessNamespace: true`） | 共享后容器可互相看到进程（用于调试） |

**Pause 容器特点**：
- **极简实现**：仅 ~700KB，只执行 `pause()` 系统调用进入永久睡眠
- **生命周期**：Pause 容器死亡 = Pod 死亡，kubelet 会重建整个 Pod
- **镜像来源**：`registry.k8s.io/pause:3.9`（可通过 kubelet `--pod-infra-container-image` 自定义）

**架构图**：
```
┌─────────────────────────────────────────────────────────────────┐
│                          Pod (IP: 10.244.1.5)                    │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │            Pause Container (Sandbox)                        │ │
│  │  ┌──────────────────────────────────────────────────────┐  │ │
│  │  │  Network Namespace: eth0 (10.244.1.5)               │  │ │
│  │  │  IPC Namespace: /dev/shm                             │  │ │
│  │  │  UTS Namespace: hostname=my-pod                      │  │ │
│  │  └──────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │         App Container (nginx)                              │ │
│  │  - 加入 Pause 的 Network/IPC/UTS Namespace                 │ │
│  │  - 监听端口：80 (通过 Pause 的网络栈暴露)                 │ │
│  │  - 独立 Mount Namespace: /usr/share/nginx/html            │ │
│  │  - 独立 PID Namespace: PID 1 = nginx master process       │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │         Sidecar Container (log-collector)                  │ │
│  │  - 加入 Pause 的 Network/IPC/UTS Namespace                 │ │
│  │  - 通过 localhost:80 访问 nginx（无需 Service）           │ │
│  │  - 通过 Volume 读取 nginx 日志：/var/log/nginx            │ │
│  │  - 独立 PID Namespace: PID 1 = log-collector process      │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  共享 Volume: emptyDir:/var/log/nginx                           │
└─────────────────────────────────────────────────────────────────┘
```

#### 1.1.2 容器运行时（CRI）交互流程

kubelet 通过 **CRI (Container Runtime Interface)** 与容器运行时（containerd/CRI-O）通信：

| 阶段 | kubelet 动作 | CRI 调用 | 容器运行时动作 |
|:----|:------------|:---------|:--------------|
| **1. 创建 Sandbox** | 分配 Pod UID 和 IP | `RunPodSandbox()` | 创建 Pause 容器 + 设置网络（调用 CNI 插件） |
| **2. 拉取镜像** | 检查镜像是否存在 | `PullImage()` | 从 Registry 拉取并解压镜像层 |
| **3. 创建容器** | 为每个容器生成配置 | `CreateContainer()` | 创建容器对象（未启动），挂载 Volume |
| **4. 启动容器** | 按序启动（Init → Main → Sidecar） | `StartContainer()` | 执行 `runc create` + `runc start` |
| **5. 探针检测** | 执行 Liveness/Readiness Probe | `ExecSync()` | 在容器内执行命令（如 `curl /healthz`） |
| **6. 停止容器** | 发送 SIGTERM，等待 `terminationGracePeriodSeconds` | `StopContainer()` | 发送信号，超时后 SIGKILL |
| **7. 删除 Sandbox** | 清理 Pod 所有资源 | `RemovePodSandbox()` | 删除 Pause 容器 + 清理网络（调用 CNI DEL） |

**关键观测点**：
```bash
# 查看节点上的 Pause 容器
crictl pods | grep <pod-name>
# 输出：POD ID, STATE, NAME, NAMESPACE, CREATED

# 查看 Pause 容器的网络配置
crictl inspectp <pod-id> | jq '.info.runtimeSpec.linux.namespaces'

# 查看业务容器如何加入 Pause 的命名空间
crictl inspect <container-id> | jq '.info.runtimeSpec.linux.namespaces[] | select(.type=="network")'
# 输出：{"type":"network","path":"/proc/<pause-pid>/ns/net"}
```

---

### 1.2 PID 1 治理与僵尸进程问题

#### 1.2.1 PID 1 的特殊责任

在 Linux 系统中，PID 1 进程（init 进程）有两个特殊职责：
1. **信号处理**：默认忽略 `SIGTERM` 和 `SIGINT`（需显式注册处理器）
2. **进程回收**：必须回收所有孤儿进程（`wait()` 系统调用），否则产生僵尸进程

**容器中的 PID 1 陷阱**：
- 如果应用程序不处理信号，`kubectl delete pod` 会导致 **30 秒超时**（等待 `terminationGracePeriodSeconds`）
- 如果应用程序启动子进程但不回收，会产生**僵尸进程**，最终耗尽 PID 资源

#### 1.2.2 常见 PID 1 问题场景

| 语言/框架 | 问题 | 表现 | 根因 |
|:---------|:----|:-----|:-----|
| **Shell 脚本** | `#!/bin/bash` 作为 PID 1 | Pod 删除超时 30s | bash 不转发 SIGTERM 给子进程 |
| **Node.js** | `node app.js` 作为 PID 1 | Ctrl+C 无法停止 | Node.js 默认不监听 SIGTERM |
| **Python** | `python app.py` 作为 PID 1 | 子进程变僵尸 | Python 不自动回收子进程 |
| **Java** | `java -jar app.jar` 作为 PID 1 | 正常（JVM 内置信号处理） | ✅ JVM 正确处理 SIGTERM |
| **Go** | `./main` 作为 PID 1 | 正常（Go runtime 内置处理） | ✅ Go 默认优雅处理信号 |

**僵尸进程诊断**：
```bash
# 进入容器查看僵尸进程
kubectl exec <pod> -- ps aux | grep defunct
# 输出示例：
# root  123  0.0  0.0  0  0  ?  Z  10:00  0:00  [worker] <defunct>

# 查看僵尸进程数量
kubectl exec <pod> -- sh -c 'ps aux | grep defunct | wc -l'

# 在节点上查看容器的 PID namespace
nsenter -t $(crictl inspect <container-id> | jq .info.pid) -p ps aux
```

#### 1.2.3 PID 1 治理方案

**方案 1：使用 `tini` 作为 Init 系统**

```dockerfile
# Dockerfile 示例
FROM node:18-alpine

# 安装 tini
RUN apk add --no-cache tini

# 使用 tini 作为 entrypoint
ENTRYPOINT ["/sbin/tini", "--"]

# 应用程序作为 tini 的子进程
CMD ["node", "app.js"]
```

**tini 工作原理**：
- 作为 PID 1，接管所有信号处理
- 转发 `SIGTERM` 给子进程（node app.js）
- 自动回收所有僵尸进程

**方案 2：启用 Pod 级别的 PID 命名空间共享**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: shared-pid-example
spec:
  shareProcessNamespace: true  # 关键配置
  containers:
  - name: app
    image: myapp:1.0
  - name: monitor
    image: busybox
    command: ["sh", "-c", "while true; do ps aux; sleep 10; done"]
```

**效果**：
- Pause 容器的 PID 1 接管所有进程回收
- 业务容器的 PID 从 2 开始（PID 1 = pause）
- 容器间可互相看到进程（方便调试和监控）

**方案 3：应用程序内置信号处理**

```python
# Python 示例：正确处理 SIGTERM
import signal
import sys
import subprocess

def signal_handler(sig, frame):
    print('Received SIGTERM, shutting down gracefully...')
    # 清理资源、关闭连接
    sys.exit(0)

signal.signal(signal.SIGTERM, signal_handler)

# 启动子进程并自动回收
process = subprocess.Popen(['worker.sh'])
process.wait()  # 阻塞直到子进程退出
```

---

### 1.3 Pod 生命周期状态机详解

#### 1.3.1 完整状态转换图

```
┌─────────────────────────────────────────────────────────────────┐
│                    Pod 生命周期状态机                             │
└─────────────────────────────────────────────────────────────────┘

                          ┌──────────────┐
                          │   Pending    │  ← Pod 已创建，等待调度
                          └──────┬───────┘
                                 │
                  ┌──────────────┼──────────────┐
                  │              │              │
                  v              v              v
         ┌────────────┐  ┌────────────┐  ┌────────────┐
         │  Scheduled │  │ Unschedulable│  │   Failed  │
         │(节点已分配)│  │(无可用节点)  │  │ (调度失败)│
         └──────┬─────┘  └────────────┘  └────────────┘
                │
                v
       ┌──────────────────┐
       │ContainerCreating │  ← 拉取镜像、创建容器
       └──────┬───────────┘
              │
              ├─────> ImagePullBackOff (镜像拉取失败)
              ├─────> CreateContainerError (容器创建失败)
              │
              v
       ┌──────────────────┐
       │    Running       │  ← 至少一个容器运行中
       └──────┬───────────┘
              │
              ├─────> CrashLoopBackOff (容器反复崩溃)
              ├─────> Error (容器启动失败)
              ├─────> OOMKilled (内存溢出被杀)
              │
              v
       ┌──────────────────┐
       │   Terminating    │  ← kubelet 正在停止容器
       └──────┬───────────┘
              │
              v
       ┌──────────────────┐
       │  Succeeded/Failed│  ← 所有容器退出
       └──────────────────┘
```

#### 1.3.2 关键状态深度解析

| 状态 | 含义 | 常见原因 | 观测方法 |
|:----|:-----|:---------|:---------|
| **Pending** | Pod 已接受但未运行 | 资源不足、污点/亲和性不匹配、PVC 未绑定 | `kubectl describe pod \| grep Events` |
| **ContainerCreating** | 容器正在创建 | 镜像拉取慢、Volume 挂载慢、CNI 网络配置慢 | `kubectl get events --field-selector involvedObject.name=<pod>` |
| **Running** | 至少一个容器运行 | - | `kubectl get pod -o wide` 查看 READY 列 |
| **CrashLoopBackOff** | 容器启动失败，等待重试 | 应用程序 bug、配置错误、依赖服务不可用 | `kubectl logs <pod> --previous` |
| **Terminating** | Pod 正在删除 | - | `kubectl get pod` 查看 AGE 列（卡住则检查 Finalizers） |
| **Unknown** | kubelet 与 API Server 失联 | 节点网络故障、kubelet 崩溃 | `kubectl describe node <node>` |

**CrashLoopBackOff 退避算法**：
```
重启延迟 = min(2^(失败次数-1) * 10秒, 300秒)

示例：
第 1 次失败：立即重启
第 2 次失败：等待 10s
第 3 次失败：等待 20s
第 4 次失败：等待 40s
第 5 次失败：等待 80s
第 6 次失败：等待 160s
第 7 次失败：等待 300s (上限)
```

#### 1.3.3 容器退出码详解

| 退出码 | Linux 信号 | 深度解析 | 排查方向 |
|:------|:----------|:---------|:---------|
| **0** | - | 正常退出 | 对于服务型容器，检查为何主循环结束 |
| **1** | - | 通用错误 | 查看应用日志，通常是业务逻辑错误 |
| **2** | - | Shell 内置命令误用 | 检查 entrypoint 脚本语法 |
| **125** | - | Docker 守护进程错误 | containerd/dockerd 故障，查看节点日志 |
| **126** | - | 命令无法执行 | 文件权限错误（`chmod +x`）或路径错误 |
| **127** | - | 命令未找到 | entrypoint 拼写错误或镜像内路径不存在 |
| **128+N** | Signal N | 被信号终止 | 例如 137=128+9=SIGKILL, 143=128+15=SIGTERM |
| **137** | SIGKILL (9) | 强制杀死 | OOMKilled 或 `kubectl delete --force` |
| **139** | SIGSEGV (11) | 段错误 | C/C++ 程序内存访问非法，查看 core dump |
| **143** | SIGTERM (15) | 优雅终止 | 正常停止，但如果卡住说明未处理信号 |
| **255** | - | 退出状态超出范围 | 应用程序返回值溢出（如 `exit(-1)`） |

**查看退出码和信号**：
```bash
# 方法 1：通过 kubectl
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].lastState.terminated}' | jq

# 输出示例：
# {
#   "exitCode": 137,
#   "reason": "OOMKilled",
#   "signal": 9,
#   "startedAt": "2026-02-10T10:00:00Z",
#   "finishedAt": "2026-02-10T10:05:00Z"
# }

# 方法 2：通过 crictl（节点上）
crictl inspect <container-id> | jq '.status'
```

---

### 1.4 容器运行时（CRI）深度解析

#### 1.4.1 containerd vs CRI-O 架构对比

| 对比维度 | containerd | CRI-O |
|:--------|:----------|:------|
| **定位** | 通用容器运行时（支持 Docker/K8s） | 专为 Kubernetes 设计 |
| **架构** | containerd → runc | CRI-O → runc/crun |
| **镜像存储** | containerd snapshotter | containers/storage |
| **OCI 运行时** | 默认 runc，支持 kata/gVisor | 默认 runc，支持 crun/kata |
| **内存占用** | ~100-200MB | ~50-100MB |
| **生态** | Docker Desktop、K8s、K3s | OpenShift、K8s |

#### 1.4.2 CRI 接口关键方法

| 方法类别 | 方法名 | 作用 | kubelet 调用时机 |
|:--------|:------|:-----|:----------------|
| **Sandbox** | `RunPodSandbox` | 创建 Pod 网络沙箱 | Pod 调度到节点后 |
| | `StopPodSandbox` | 停止沙箱（保留网络） | Pod 删除时 |
| | `RemovePodSandbox` | 删除沙箱 | Pod 完全清理时 |
| **Container** | `CreateContainer` | 创建容器（未启动） | 镜像拉取完成后 |
| | `StartContainer` | 启动容器 | CreateContainer 后 |
| | `StopContainer` | 停止容器 | Pod 删除或容器重启时 |
| | `RemoveContainer` | 删除容器 | StopContainer 后 |
| **Image** | `PullImage` | 拉取镜像 | Pod 创建时（如镜像不存在） |
| | `ListImages` | 列出镜像 | ImageGC 时 |
| | `RemoveImage` | 删除镜像 | ImageGC 清理时 |
| **Exec** | `ExecSync` | 同步执行命令 | Liveness/Readiness Probe |
| | `Exec` | 异步执行命令 | `kubectl exec` |

**监控 CRI 调用**：
```bash
# 启用 containerd 的 CRI 调用日志
cat /etc/containerd/config.toml
# [plugins."io.containerd.grpc.v1.cri"]
#   stream_server_address = "127.0.0.1"
#   stream_server_port = "0"
#   enable_cri_stats = true
#   [plugins."io.containerd.grpc.v1.cri".cni]
#     bin_dir = "/opt/cni/bin"
#     conf_dir = "/etc/cni/net.d"

# 查看 CRI 调用统计
crictl stats

# 查看 CRI 调用延迟
journalctl -u containerd | grep "RunPodSandbox\|CreateContainer" | tail -20
```

---

## 2. 专家级故障矩阵与观测工具

### 2.1 专家级故障矩阵（按生命周期分类）

#### 2.1.1 调度阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **Pending: 资源不足** | CPU/Memory 碎片化（节点剩余资源分散），ResourceQuota 耗尽，PriorityClass 优先级不足 | `kubectl describe pod \| grep "FailedScheduling"`；`kubectl describe node \| grep -A5 "Allocated resources"` | 扩容节点；调整 requests；使用 Cluster Autoscaler |
| **Pending: 污点/亲和性** | 节点打了 `NoSchedule` 污点，Pod 未配置容忍；`requiredDuringScheduling` 亲和性无法满足 | `kubectl describe node \| grep Taints`；`kubectl get pod -o yaml \| grep -A10 affinity` | 添加 `tolerations`；放宽亲和性为 `preferred` |
| **Pending: PVC 未绑定** | StorageClass 不存在、后端存储配额不足、`volumeBindingMode: WaitForFirstConsumer` 延迟绑定 | `kubectl get pvc \| grep Pending`；`kubectl describe pvc` | 检查 StorageClass；扩容后端；手动创建 PV |
| **Pending: 拓扑约束** | `topologySpreadConstraints` 约束无法满足（如强制均匀分布但节点不足） | `kubectl get pod -o yaml \| grep -A5 topologySpreadConstraints` | 放宽 `whenUnsatisfiable: DoNotSchedule` 为 `ScheduleAnyway` |

#### 2.1.2 容器创建阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **ImagePullBackOff** | 镜像不存在、Registry 凭证过期、镜像层损坏、Registry 速率限制（Docker Hub 100次/6h） | `kubectl describe pod \| grep -A5 "Failed to pull image"`；`crictl pull <image>` 测试拉取 | 检查镜像 tag；重新创建 `imagePullSecrets`；使用镜像缓存代理 |
| **CreateContainerError** | Volume 挂载失败（PVC 不存在、CSI 驱动故障）、SecurityContext 冲突（如 `runAsUser: 0` 被 PSP 拒绝） | `kubectl describe pod \| grep "CreateContainerError"`；`crictl ps -a \| grep Error` | 检查 Volume 状态；调整 SecurityContext；查看 PSP/PSA 策略 |
| **Init:CrashLoopBackOff** | InitContainer 失败（如数据库迁移脚本错误） | `kubectl logs <pod> -c <init-container>`；`kubectl get pod -o jsonpath='{.status.initContainerStatuses}'` | 修复 InitContainer 逻辑；临时跳过（删除 initContainers） |

#### 2.1.3 运行阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **CrashLoopBackOff: PID 1** | 应用程序退出（缺少环境变量、配置错误）、PID 1 未处理信号（shell 脚本陷阱）、依赖服务不可用 | `kubectl logs <pod> --previous`；`kubectl get pod -o jsonpath='{.status.containerStatuses[0].lastState.terminated}'` | 检查应用日志；使用 `tini` 作为 init；添加健康检查重试 |
| **OOMKilled (137)** | 内存使用超过 `limits`、Java 堆外内存（DirectByteBuffer）、内存泄漏（Python/Node.js） | `kubectl describe pod \| grep "OOMKilled"`；`kubectl top pod --containers`；节点上 `dmesg \| grep oom` | 增加 `limits`；优化应用内存；启用内存分析（heapdump） |
| **Liveness 探针失败** | 探针超时时间过短、应用启动慢（大型 Java 应用）、探针依赖外部服务（数据库抖动导致全部重启） | `kubectl describe pod \| grep "Liveness probe failed"`；`kubectl get pod -o yaml \| grep -A10 livenessProbe` | 增加 `initialDelaySeconds` 和 `timeoutSeconds`；使用 `startupProbe`；探针改为本地检查 |
| **Readiness 探针失败** | 应用未就绪（依赖服务连接中）、探针路径错误（/health vs /healthz） | `kubectl describe pod \| grep "Readiness probe failed"`；`kubectl get endpoints <service>` 确认 Pod 未加入 | 等待应用就绪；修正探针配置；临时禁用探针 |

#### 2.1.4 停止阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **Terminating 卡住** | Finalizers 未清理（如 PVC 保护）、PreStop Hook 超时、Volume 卸载失败（NFS 挂死）、容器进程忽略 SIGTERM | `kubectl get pod -o yaml \| grep finalizers`；`kubectl describe pod \| grep "PreStop"`；节点上 `ps aux \| grep <pod-uid>` | 强制删除 Finalizers：`kubectl patch pod <pod> -p '{"metadata":{"finalizers":null}}'`；强制删除：`kubectl delete pod --force --grace-period=0` |
| **DiskPressure 驱逐** | 节点磁盘空间不足（`/var/lib/containerd` 或 `/var/log` 满）、ImageGC 未及时清理 | `kubectl describe node \| grep "DiskPressure"`；节点上 `df -h`；`du -sh /var/lib/containerd/*` | 清理无用镜像：`crictl rmi --prune`；清理日志：`journalctl --vacuum-time=7d` |
| **MemoryPressure 驱逐** | 节点内存不足，kubelet 主动驱逐低优先级 Pod（BestEffort → Burstable → Guaranteed） | `kubectl describe node \| grep "MemoryPressure"`；`kubectl top node` | 扩容节点内存；调整 Pod QoS（设置 requests=limits） |

---

### 2.2 专家工具箱

#### 2.2.1 临时调试容器（Ephemeral Containers）

```bash
# K8s 1.25+ 推荐：在运行中的 Pod 注入调试容器
kubectl debug <pod-name> -it \
  --image=nicolaka/netshoot \
  --target=<container-name> \
  -- /bin/bash

# 调试容器特点：
# - 与目标容器共享 PID namespace（可看到目标进程）
# - 与目标容器共享 Network namespace（可抓包）
# - 不影响 Pod 原有容器

# 示例：抓取容器网络包
kubectl debug <pod> -it --image=nicolaka/netshoot --target=app -- \
  tcpdump -i any -w /tmp/capture.pcap port 8080
```

#### 2.2.2 节点级容器诊断

```bash
# 1. 查看节点上所有容器
crictl ps -a

# 2. 查看容器详细信息（包括退出码、OOM 信息）
crictl inspect <container-id> | jq '.status'

# 3. 进入容器的命名空间（需 root 权限）
nsenter -t $(crictl inspect <container-id> | jq .info.pid) -m -u -i -n -p

# 4. 追踪容器系统调用
strace -p $(crictl inspect <container-id> | jq .info.pid) -f

# 5. 查看容器 cgroup 资源限制
cat /sys/fs/cgroup/memory/kubepods/pod<pod-uid>/<container-id>/memory.limit_in_bytes
cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/<container-id>/cpu.cfs_quota_us
```

#### 2.2.3 Pod 资源使用实时监控

```bash
# 1. 查看 Pod 级别资源使用
kubectl top pod --containers --namespace=<ns>

# 2. 持续监控（每 2 秒刷新）
watch -n 2 'kubectl top pod --containers -n production | grep <pod-pattern>'

# 3. 查看 Pod 的 cgroup 内存详情（节点上执行）
POD_UID=$(kubectl get pod <pod> -o jsonpath='{.metadata.uid}')
cat /sys/fs/cgroup/memory/kubepods/pod${POD_UID}/memory.stat

# 输出示例：
# cache 104857600          # 页缓存
# rss 209715200            # 常驻内存（RSS）
# rss_huge 0               # 大页内存
# mapped_file 52428800     # 映射文件
# inactive_anon 0          # 非活跃匿名页
# active_anon 209715200    # 活跃匿名页
```

#### 2.2.4 事件聚合分析脚本

```bash
#!/bin/bash
# 脚本：pod-event-analyzer.sh
# 用法：./pod-event-analyzer.sh <pod-name> <namespace>

POD_NAME=$1
NAMESPACE=$2

echo "=== Pod 基本信息 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o wide

echo -e "\n=== Pod 当前状态 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status}' | jq

echo -e "\n=== 容器状态详情 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses}' | jq

echo -e "\n=== 历史事件（按时间排序） ==="
kubectl get events -n $NAMESPACE \
  --field-selector involvedObject.name=$POD_NAME \
  --sort-by='.lastTimestamp' \
  -o custom-columns=TIME:.lastTimestamp,TYPE:.type,REASON:.reason,MESSAGE:.message

echo -e "\n=== 节点资源状态 ==="
NODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
kubectl describe node $NODE | grep -A10 "Allocated resources"

echo -e "\n=== 容器退出码 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses[*].lastState.terminated}' | jq
```

#### 2.2.5 监控告警规则

```yaml
# Prometheus 告警规则
groups:
- name: pod-alerts
  interval: 30s
  rules:
  # 1. Pod 频繁重启
  - alert: PodRestartingFrequently
    expr: rate(kube_pod_container_status_restarts_total[15m]) > 0.5
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 重启频繁（>7次/15min）"
      description: "容器 {{ $labels.container }} 可能存在 CrashLoop"

  # 2. Pod OOM 告警
  - alert: PodOOMKilled
    expr: kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 容器 OOM"
      description: "容器 {{ $labels.container }} 内存超限被杀"

  # 3. Pod 长时间 Pending
  - alert: PodStuckInPending
    expr: kube_pod_status_phase{phase="Pending"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 长时间 Pending"
      description: "检查资源配额、节点亲和性和 PVC 状态"

  # 4. Pod Terminating 卡住
  - alert: PodStuckInTerminating
    expr: kube_pod_deletion_timestamp > 0
    for: 30m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 删除超时"
      description: "检查 Finalizers、Volume 卸载和 PreStop Hook"

  # 5. Pod 就绪率低
  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="false"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 未就绪"
      description: "检查 Readiness Probe 和应用启动状态"
```

---

## 3. 深度排查路径

### 3.1 第一阶段：控制面事件流分析
确认“为什么 Pod 会在这里”。

```bash
# 查看最近 10 分钟的所有异常事件
kubectl get events -A --sort-by=.lastTimestamp | grep -E "Warning|Error"

# 检查节点污点是否导致 Pod 被驱逐
kubectl describe node <node-name> | grep Taints
```

### 3.2 第二阶段：容器内部环境诊断
确认“环境是否如我所愿”。

```bash
# 验证环境变量与配置挂载
kubectl exec <pod-name> -- env
kubectl exec <pod-name> -- ls -R /etc/config

# 检查容器内网络连通性 (使用 netshoot)
kubectl debug <pod-name> -it --image=nicolaka/netshoot -- /bin/bash
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决 PID 1 僵尸进程问题
**痛点**：容器内进程无法回收子进程，导致 PID 耗尽。
**对策**：
- 使用 `tini` 作为 entrypoint。
- 在 Pod Spec 中开启 `shareProcessNamespace: true`，让 K8s 基础设施回收。

### 4.2 优化 Liveness/Readiness 探针
**专家建议**：
- **避免相互依赖**：Liveness 探针不要依赖外部数据库，否则数据库抖动会导致所有 Pod 重启。
- **Startup Probe**：针对启动慢的应用（如大型 Java 应用），使用 `startupProbe` 防止其在初始化时被 `livenessProbe` 误杀。

### 4.3 处理 Sidecar 启动顺序依赖
**场景**：应用容器在启动时需要 Sidecar (如网格代理) 就绪才能联网。
**对策 (K8s 1.29+)**：利用原生 Sidecar 特性。
```yaml
spec:
  containers:
  - name: istio-proxy
    restartPolicy: Always # 标记为 Sidecar
```

---

## 5. 生产环境典型案例解析

### 5.1 案例一：Pod 访问外部 API 偶发超时
- **根因分析**：容器内 `ndots` 配置为 5，导致短域名解析产生大量无效 DNS 查询，耗尽了 conntrack 表。
- **对策**：优化 `dnsConfig`，将 `ndots` 调整为 1。

### 5.2 案例二：应用正常运行，但 Pod 不断被 Evicted
- **根因分析**：节点磁盘空间不足（ImageGC 失败），触发了 `DiskPressure` 驱逐。
- **对策**：清理节点无用镜像，并配置更合理的磁盘报警阈值。

---

## 附录：Pod 退出状态码专家参考表
| 退出码 | 信号 | 深度解析 |
| :--- | :--- | :--- |
| **0** | - | 进程主动退出。对于任务型容器是成功，对于服务型容器可能是由于 Bug 导致主循环结束。 |
| **137** | SIGKILL | 通常是 OOMKilled，或者是被 `kubectl delete --force` 杀掉。 |
| **143** | SIGTERM | 正常退出请求。如果 Pod 长时间卡在 Terminating，说明进程未处理此信号。 |
| **126/127** | - | 镜像内文件权限错误或路径找不到（如 `entrypoint` 拼写错误）。 |
| **139** | SIGSEGV | 段错误，通常是 C/C++ 库崩溃或内存非法访问。 |

---

## 附录：Pod 巡检清单
- [ ] **资源配额**：是否同时配置了 `requests` 和 `limits`（QoS 保证）？
- [ ] **优雅停机**：是否配置了 `terminationGracePeriodSeconds`（默认 30s 是否够用）？
- [ ] **探针覆盖**：是否同时具备 `readiness`（流量切入）和 `liveness`（存活检查）？
- [ ] **反亲和性**：关键业务是否配置了 `podAntiAffinity` 防止单点故障？
- [ ] **安全加固**：是否设置了 `securityContext`（非 root 运行）？
### 3.1 第一阶段：调度决策验证

**目标**：确认 Pod 为何被调度到特定节点（或为何无法调度）。

```bash
# 1. 查看调度器事件
kubectl get events -A --sort-by='.lastTimestamp' \
  --field-selector involvedObject.name=<pod-name> \
  | grep -E "Scheduled|FailedScheduling"

# 预期输出示例（成功调度）：
# Normal  Scheduled  2m  default-scheduler  Successfully assigned default/my-pod to node-3

# 预期输出示例（调度失败）：
# Warning  FailedScheduling  1m (x5 over 3m)  default-scheduler
#   0/10 nodes are available: 3 Insufficient cpu, 5 node(s) didn't match Pod's node affinity, 2 node(s) had taint {gpu=true:NoSchedule}

# 2. 查看调度器打分详情（需启用 verbose 日志）
kubectl -n kube-system logs kube-scheduler-<node> | grep "pod=<pod-name>"

# 3. 验证节点资源可用性
kubectl describe node <node> | grep -A10 "Allocated resources"
# 输出：
# Resource           Requests      Limits
# --------           --------      ------
# cpu                7500m (93%)   12000m (150%)  # ❌ CPU 超卖严重
# memory             28Gi (70%)    48Gi (120%)    # ❌ Memory 超卖

# 4. 检查污点和容忍
kubectl describe node <node> | grep Taints
# Taints: gpu=true:NoSchedule  # Pod 需要添加 tolerations

kubectl get pod <pod> -o yaml | grep -A5 tolerations
# tolerations:
# - key: "gpu"
#   operator: "Equal"
#   value: "true"
#   effect: "NoSchedule"
```

**常见调度失败根因**：
- **资源碎片化**：集群总资源充足，但单个节点资源不足（解决：Descheduler 或 Cluster Autoscaler）
- **亲和性冲突**：`requiredDuringSchedulingIgnoredDuringExecution` 无法满足（解决：改为 `preferred`）
- **PVC 拓扑约束**：PVC 只在特定可用区，但节点分布不符（解决：使用 `volumeBindingMode: WaitForFirstConsumer`）

---

### 3.2 第二阶段：容器启动链路追踪

**目标**：确认镜像拉取、Volume 挂载、容器创建的每个步骤是否成功。

```bash
# 1. 查看 Pod 事件时间线
kubectl describe pod <pod> | tail -30

# 典型事件序列：
# Normal  Scheduled     10s  default-scheduler  Successfully assigned to node-2
# Normal  Pulling       9s   kubelet            Pulling image "nginx:1.21"
# Normal  Pulled        7s   kubelet            Successfully pulled image
# Normal  Created       6s   kubelet            Created container nginx
# Normal  Started       5s   kubelet            Started container nginx

# 2. 检查镜像拉取详情（节点上执行）
crictl images | grep nginx
# IMAGE                     TAG     IMAGE ID      SIZE
# nginx                     1.21    a1b2c3d4e5f6  142MB

# 如果镜像不存在，手动测试拉取
crictl pull nginx:1.21
# 输出包含每层下载进度和 SHA256

# 3. 检查 imagePullSecrets 配置
kubectl get secret <secret-name> -o yaml
# type: kubernetes.io/dockerconfigjson
# data:
#   .dockerconfigjson: <base64>

# 解码查看（验证凭证格式）
kubectl get secret <secret-name> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq

# 4. 检查 Volume 挂载状态
kubectl get pod <pod> -o jsonpath='{.spec.volumes}' | jq
# 输出：
# [
#   {
#     "name": "config",
#     "configMap": {"name": "app-config"}  # ✅ ConfigMap 存在
#   },
#   {
#     "name": "data",
#     "persistentVolumeClaim": {"claimName": "data-pvc"}  # 需验证 PVC
#   }
# ]

# 验证 PVC 是否绑定
kubectl get pvc data-pvc
# NAME       STATUS   VOLUME    CAPACITY   ACCESS MODES   STORAGECLASS
# data-pvc   Bound    pv-123    10Gi       RWO            standard  # ✅ 已绑定

# 5. 在节点上检查实际挂载点
POD_UID=$(kubectl get pod <pod> -o jsonpath='{.metadata.uid}')
ls -la /var/lib/kubelet/pods/$POD_UID/volumes/
# kubernetes.io~configmap/config/
# kubernetes.io~csi/data-pvc/
```

**常见创建阶段故障**：
- **ImagePullBackOff**：镜像不存在、Registry 限流、网络超时（Docker Hub 免费用户限制 100 次/6h）
- **CreateContainerConfigError**：ConfigMap/Secret 不存在、环境变量引用错误
- **CreateContainerError**：Volume 挂载失败、SecurityContext 配置冲突

---

### 3.3 第三阶段：容器运行时诊断

**目标**：深入容器内部，验证应用环境和网络连通性。

```bash
# 1. 查看容器日志（最近 100 行）
kubectl logs <pod> -c <container> --tail=100

# 查看上一次崩溃的日志
kubectl logs <pod> -c <container> --previous

# 实时跟踪日志
kubectl logs <pod> -c <container> -f

# 2. 进入容器执行命令
kubectl exec <pod> -c <container> -- env
# 输出：PATH, HOME, KUBERNETES_SERVICE_HOST, APP_CONFIG 等

# 验证配置文件是否正确挂载
kubectl exec <pod> -- cat /etc/config/app.conf

# 3. 网络连通性测试
# 方法 1：直接在容器内测试（需容器有 curl/wget）
kubectl exec <pod> -- curl -v http://api-service:8080/health

# 方法 2：使用调试容器（推荐）
kubectl debug <pod> -it --image=nicolaka/netshoot --target=<container> -- bash
# 在调试容器内：
curl -v http://api-service:8080/health
nslookup api-service
traceroute api-service

# 4. DNS 解析验证
kubectl exec <pod> -- cat /etc/resolv.conf
# nameserver 10.96.0.10  # CoreDNS Service IP
# search default.svc.cluster.local svc.cluster.local cluster.local
# options ndots:5

# 测试 DNS 解析
kubectl exec <pod> -- nslookup kubernetes.default.svc.cluster.local

# 5. 进程状态检查
kubectl exec <pod> -- ps aux
# 输出：
# USER  PID  %CPU  %MEM  COMMAND
# root  1    0.1   0.5   /usr/bin/myapp  # ✅ PID 1 正常运行
# root  123  0.0   0.1   [worker] <defunct>  # ❌ 僵尸进程

# 6. 资源使用实时查看
kubectl top pod <pod> --containers
# POD    NAME       CPU(cores)  MEMORY(bytes)
# my-pod nginx      50m         128Mi
# my-pod sidecar    10m         64Mi
```

---

### 3.4 第四阶段：节点级底层诊断

**目标**：在节点上直接检查容器运行时和内核层面的问题。

```bash
# 1. 查找 Pod 对应的容器 ID
crictl pods | grep <pod-name>
# POD ID        CREATED        STATE    NAME
# a1b2c3d4e5f6  10 minutes ago Running  my-pod

# 查看容器列表
crictl ps --pod=a1b2c3d4e5f6
# CONTAINER ID  IMAGE      CREATED        STATE    NAME
# 1234567890ab  nginx:1.21 9 minutes ago  Running  nginx
# 9876543210ba  busybox    9 minutes ago  Running  sidecar

# 2. 检查容器详细信息
crictl inspect 1234567890ab | jq '.status'
# 输出包含：state, exitCode, startedAt, finishedAt, oomKilled, reason

# 3. 进入容器的 Mount Namespace（查看实际挂载点）
nsenter -t $(crictl inspect 1234567890ab | jq .info.pid) -m
mount | grep "/var/lib/kubelet/pods"
# /dev/sda1 on /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~configmap/config type ext4

# 4. 查看容器 cgroup 限制
cat /sys/fs/cgroup/memory/kubepods/pod<pod-uid>/1234567890ab/memory.limit_in_bytes
# 536870912  # 512MB

cat /sys/fs/cgroup/memory/kubepods/pod<pod-uid>/1234567890ab/memory.usage_in_bytes
# 268435456  # 256MB（50% 使用率）

# 5. 追踪系统调用（查找性能瓶颈）
strace -p $(crictl inspect 1234567890ab | jq .info.pid) -c -f
# 输出每个系统调用的次数和耗时

# 6. 查看内核 OOM 日志
dmesg | grep -i "killed process"
# [12345.678] Out of memory: Killed process 9876 (myapp) total-vm:600MB, anon-rss:550MB, file-rss:50MB
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决 PID 1 僵尸进程问题

**场景**：应用程序启动多个子进程（如 Python `subprocess`、Node.js `child_process`），但不回收，导致僵尸进程堆积。

**方案 1：使用 `tini` 作为 Init 系统**

```dockerfile
# Dockerfile
FROM python:3.11-alpine

# 安装 tini
RUN apk add --no-cache tini

# 复制应用代码
COPY app.py /app/

# 使用 tini 作为 PID 1
ENTRYPOINT ["/sbin/tini", "--"]

# 应用程序作为 tini 的子进程
CMD ["python", "/app/app.py"]
```

**方案 2：在 Pod Spec 中启用 PID 命名空间共享**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: shared-pid-demo
spec:
  shareProcessNamespace: true  # 关键配置
  containers:
  - name: app
    image: myapp:1.0
    # 应用程序的 PID 从 2 开始（PID 1 = pause 容器）
  - name: monitor
    image: busybox
    command: 
    - sh
    - -c
    - |
      while true; do
        echo "=== Process List ==="
        ps aux
        echo "=== Zombie Count ==="
        ps aux | grep defunct | wc -l
        sleep 30
      done
```

**效果对比**：
| 配置 | PID 1 | 僵尸进程回收 | 容器间可见性 |
|:----|:-----|:------------|:------------|
| **默认（隔离 PID NS）** | 应用程序 | 需应用程序自行回收 | 容器间不可见 |
| **使用 tini** | tini | ✅ tini 自动回收 | 容器间不可见 |
| **shareProcessNamespace: true** | Pause 容器 | ✅ Pause 自动回收 | ✅ 容器间可见 |

---

### 4.2 优化 Liveness/Readiness/Startup Probe

**场景**：大型 Java 应用启动需要 2 分钟，但 `livenessProbe` 的 `initialDelaySeconds: 60s` 导致启动过程中被误杀。

**问题分析**：
- Liveness Probe 检查应用"是否存活"（存活则不重启）
- Readiness Probe 检查应用"是否就绪"（就绪则加入 Service 流量）
- Startup Probe（1.20+）检查应用"是否完成启动"（启动期间禁用 Liveness）

**最佳实践配置**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: java-app
spec:
  containers:
  - name: app
    image: myapp:1.0
    ports:
    - containerPort: 8080
    
    # Startup Probe：只在启动阶段检查（最多 30 次 * 10s = 5 分钟）
    startupProbe:
      httpGet:
        path: /actuator/health/liveness
        port: 8080
      initialDelaySeconds: 30      # 首次检查延迟
      periodSeconds: 10             # 检查间隔
      failureThreshold: 30          # 失败 30 次才重启（总计 5 分钟）
      timeoutSeconds: 5             # 单次请求超时
    
    # Liveness Probe：启动完成后生效，检查存活性
    livenessProbe:
      httpGet:
        path: /actuator/health/liveness
        port: 8080
      initialDelaySeconds: 0        # Startup 完成后立即生效
      periodSeconds: 30             # 每 30 秒检查
      failureThreshold: 3           # 失败 3 次重启
      timeoutSeconds: 5
    
    # Readiness Probe：检查是否可接收流量
    readinessProbe:
      httpGet:
        path: /actuator/health/readiness
        port: 8080
      initialDelaySeconds: 0
      periodSeconds: 10
      failureThreshold: 3
      timeoutSeconds: 5
```

**探针设计原则**：
1. **Liveness 不依赖外部服务**：避免数据库抖动导致所有 Pod 重启
   ```yaml
   # ❌ 错误示例：依赖数据库
   livenessProbe:
     exec:
       command: ["mysql", "-h", "db-service", "-e", "SELECT 1"]
   
   # ✅ 正确示例：本地检查
   livenessProbe:
     httpGet:
       path: /healthz  # 仅检查应用进程是否响应
   ```

2. **Readiness 可依赖外部服务**：确保应用完全就绪才接收流量
   ```yaml
   readinessProbe:
     exec:
       command:
       - sh
       - -c
       - |
         # 检查数据库连接
         nc -zv db-service 3306 &&
         # 检查 Redis 连接
         nc -zv redis-service 6379 &&
         # 检查应用健康
         curl -f http://localhost:8080/health
   ```

3. **Startup 宽容启动慢的应用**：避免启动期间被 Liveness 误杀

---

### 4.3 处理 Sidecar 启动顺序依赖

**场景**：应用容器在启动时需要访问外部 API，但 Service Mesh Sidecar（istio-proxy）尚未就绪，导致连接失败。

**问题根因**：
- Kubernetes 默认并行启动所有容器（除了 InitContainers）
- 应用容器可能比 Sidecar 更快启动，导致首次请求失败

**方案 1：使用 Native Sidecar（K8s 1.29+ GA）**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecar
spec:
  initContainers:
  # Sidecar 作为特殊的 InitContainer（restartPolicy: Always）
  - name: istio-proxy
    image: istio/proxyv2:1.20
    restartPolicy: Always  # 关键：标记为 Sidecar（与 Pod 生命周期相同）
    ports:
    - containerPort: 15001
  
  containers:
  # 应用容器在 Sidecar 就绪后启动
  - name: app
    image: myapp:1.0
    command: ["sh", "-c", "echo 'App started after sidecar ready'; ./myapp"]
```

**方案 2：应用程序内置重试逻辑（兼容旧版本 K8s）**

```python
# Python 示例：启动时重试连接
import time
import requests

def wait_for_sidecar():
    """等待 Sidecar 就绪（检查 15021 健康端口）"""
    max_retries = 30
    for i in range(max_retries):
        try:
            resp = requests.get('http://localhost:15021/healthz/ready', timeout=1)
            if resp.status_code == 200:
                print("Sidecar is ready!")
                return
        except requests.exceptions.RequestException:
            print(f"Waiting for sidecar... ({i+1}/{max_retries})")
            time.sleep(1)
    
    raise Exception("Sidecar not ready after 30s")

if __name__ == "__main__":
    wait_for_sidecar()
    # 启动应用主逻辑
    app.run()
```

**方案 3：使用 PostStart Hook 延迟应用启动**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-hook
spec:
  containers:
  - name: istio-proxy
    image: istio/proxyv2:1.20
  
  - name: app
    image: myapp:1.0
    lifecycle:
      postStart:
        exec:
          command:
          - sh
          - -c
          - |
            # 等待 Sidecar 的健康端口就绪
            until curl -s http://localhost:15021/healthz/ready; do
              echo "Waiting for sidecar..."
              sleep 1
            done
            echo "Sidecar is ready, starting app"
```

---

## 5. 生产环境典型案例解析

### 5.1 案例一：Java 应用 OOMKilled 但堆内存使用正常

#### 5.1.1 故障现场

**背景**：
- 应用：Spring Boot 2.7 + JDK 17
- 配置：`resources.limits.memory: 2Gi`，JVM 参数：`-Xmx1536m -Xms1536m`
- 现象：应用运行 2-3 小时后被 OOMKilled，但 JVM 堆内存监控显示只使用了 60%

**时间线**：
- **T+0h**：Pod 启动，应用正常
- **T+2.5h**：收到告警：Pod 重启，退出码 137
- **T+2.5h+10min**：应用再次启动，运行 2 小时后又被 OOMKilled
- **T+5h**：第三次重启，运维介入排查

**初步排查**：
```bash
# 查看退出原因
kubectl get pod java-app -o jsonpath='{.status.containerStatuses[0].lastState.terminated}'
# 输出：
# {
#   "exitCode": 137,
#   "reason": "OOMKilled",
#   "signal": 9
# }

# 查看内存限制
kubectl get pod java-app -o jsonpath='{.spec.containers[0].resources.limits.memory}'
# 2Gi

# 查看 JVM 堆内存使用（通过 Prometheus）
jvm_memory_used_bytes{area="heap"} / jvm_memory_max_bytes{area="heap"}
# 结果：0.6 (60%)  # ❌ 堆内存使用正常，为何 OOM？
```

#### 5.1.2 深度排查过程

**Step 1：分析 JVM 内存布局**

```bash
# 在 Pod 重启前执行（获取完整内存映射）
kubectl exec java-app -- jcmd 1 VM.native_memory summary

# 输出示例（简化）：
# Native Memory Tracking:
# 
# Total:  reserved=2100MB, committed=1850MB
#   - Heap:         reserved=1536MB, committed=1536MB  # ✅ 堆内存正常
#   - Metaspace:    reserved=256MB,  committed=230MB   # ✅ 元空间正常
#   - Thread:       reserved=150MB,  committed=150MB   # ⚠️ 线程栈
#   - Code:         reserved=50MB,   committed=45MB
#   - GC:           reserved=80MB,   committed=75MB
#   - Direct:       reserved=28MB,   committed=28MB    # ❌ 堆外内存
```

**根因发现**：
- 容器 limit = 2048MB
- JVM 总内存 = 1850MB（已接近 limit 的 90%）
- **关键**：`Direct`（堆外内存）持续增长，从 28MB 增长至 300MB+（超出预期）

**Step 2：定位堆外内存泄漏**

```bash
# 查看堆外内存分配栈
kubectl exec java-app -- jcmd 1 VM.native_memory detail | grep -A 20 "Direct"

# 发现大量 DirectByteBuffer 分配，来源：
# at java.nio.DirectByteBuffer.<init>(DirectByteBuffer.java:127)
# at org.apache.http.nio.pool.AbstractNIOConnPool.createEntry(AbstractNIOConnPool.java:456)
# at org.apache.http.nio.pool.AbstractConnPool.getPoolEntryBlocking(AbstractConnPool.java:393)

# ❌ 根因：Apache HttpClient NIO 连接池未正确释放连接
```

**Step 3：验证连接池配置**

```java
// 应用代码审查
CloseableHttpAsyncClient httpClient = HttpAsyncClients.custom()
    .setMaxConnPerRoute(50)
    .setMaxConnTotal(100)
    // ❌ 问题：未配置连接池清理策略
    .build();

// 连接建立后未关闭，堆外内存持续增长
```

#### 5.1.3 解决方案

**立即缓解**（增加内存 limit，30 分钟）：
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: java-app
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          limits:
            memory: 3Gi  # 从 2Gi 增加到 3Gi（临时方案）
          requests:
            memory: 3Gi
```

**根本修复**（修复代码，2 小时）：
```java
// 正确的 HttpClient 配置
CloseableHttpAsyncClient httpClient = HttpAsyncClients.custom()
    .setMaxConnPerRoute(50)
    .setMaxConnTotal(100)
    // ✅ 添加连接清理策略
    .evictIdleConnections(30, TimeUnit.SECONDS)  // 清理空闲连接
    .evictExpiredConnections()                   // 清理过期连接
    // ✅ 限制堆外内存分配
    .setDefaultIOReactorConfig(
        IOReactorConfig.custom()
            .setIoThreadCount(4)  # 限制 IO 线程数（减少堆外内存）
            .build()
    )
    .build();

// ✅ 添加 try-with-resources 确保连接关闭
try (CloseableHttpResponse response = httpClient.execute(request)) {
    // 处理响应
}
```

**长期优化**（JVM 参数调整）：
```yaml
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    env:
    - name: JAVA_OPTS
      value: |
        -Xmx1536m -Xms1536m
        -XX:MaxMetaspaceSize=256m
        -XX:MaxDirectMemorySize=256m  # ✅ 限制堆外内存上限
        -XX:+UseG1GC
        -XX:MaxGCPauseMillis=200
        -XX:NativeMemoryTracking=detail  # ✅ 启用内存追踪
    resources:
      limits:
        memory: 2560Mi  # 1536 + 256 + 256 + 512(其他) = 2560Mi
      requests:
        memory: 2560Mi
```

#### 5.1.4 防护措施

**1. 监控堆外内存使用**：
```yaml
# Prometheus 指标
jvm_memory_used_bytes{area="nonheap"}  # 非堆内存（包括堆外）
jvm_buffer_pool_used_bytes{id="direct"}  # DirectByteBuffer 使用量

# 告警规则
- alert: JVMDirectMemoryHigh
  expr: jvm_buffer_pool_used_bytes{id="direct"} > 200 * 1024 * 1024
  for: 10m
  annotations:
    summary: "JVM 堆外内存超过 200MB"
```

**2. 定期执行内存诊断**：
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: jvm-memory-check
spec:
  schedule: "0 */6 * * *"  # 每 6 小时
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: checker
            image: bitnami/kubectl:1.28
            command:
            - bash
            - -c
            - |
              for pod in $(kubectl get pods -l app=java-app -o name); do
                echo "=== $pod ==="
                kubectl exec $pod -- jcmd 1 VM.native_memory summary
              done
          restartPolicy: OnFailure
```

**成果**：
- ✅ OOMKilled 频率从 2-3 小时/次 → 0
- ✅ 堆外内存稳定在 150MB（从 300MB+ 下降）
- ✅ 内存 limit 从 3Gi 优化至 2.5Gi

---

### 5.2 案例二：Pod 删除卡在 Terminating 30 分钟+

#### 5.2.1 故障现场

**背景**：
- 应用：Nginx + NFS Volume（用于存储上传文件）
- 操作：执行 `kubectl delete pod nginx-abc123` 准备更新配置
- 现象：Pod 状态卡在 `Terminating`，30 分钟仍未删除

**时间线**：
- **T+0min**：执行 `kubectl delete pod nginx-abc123`
- **T+1min**：Pod 状态变为 `Terminating`
- **T+30min**：Pod 仍然存在，`kubectl get pod` 显示 `Terminating`
- **T+35min**：尝试强制删除：`kubectl delete pod --force --grace-period=0`，无效
- **T+40min**：运维介入深度排查

#### 5.2.2 深度排查过程

**Step 1：检查 Pod 状态和 Finalizers**

```bash
# 查看 Pod 详情
kubectl get pod nginx-abc123 -o yaml

# 输出（关键部分）：
# metadata:
#   deletionTimestamp: "2026-02-10T10:00:00Z"  # ✅ 已标记删除
#   finalizers:
#   - kubernetes.io/pvc-protection  # ❌ Finalizer 未清理

# spec:
#   volumes:
#   - name: data
#     persistentVolumeClaim:
#       claimName: nginx-data-pvc
```

**根因线索**：`pvc-protection` Finalizer 表示 PVC 仍在使用中，kubelet 无法删除 Pod。

**Step 2：检查 PVC 状态**

```bash
# 查看 PVC
kubectl get pvc nginx-data-pvc

# NAME             STATUS   VOLUME      CAPACITY   ACCESS MODES   STORAGECLASS
# nginx-data-pvc   Bound    pv-nfs-123  10Gi       RWX            nfs-client

# 查看 PVC 详情
kubectl describe pvc nginx-data-pvc

# Events:
#   Warning  VolumeFailedDelete  5m (x10 over 30m)  persistentvolume-controller
#     Failed to delete volume: rpc error: code = Internal 
#     desc = failed to unmount volume: device is busy
```

**Step 3：在节点上检查 Volume 挂载**

```bash
# 找到 Pod 所在节点
NODE=$(kubectl get pod nginx-abc123 -o jsonpath='{.spec.nodeName}')

# 登录节点，查看挂载点
ssh $NODE
mount | grep nginx-abc123

# 输出：
# 192.168.1.100:/nfs/export on /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~nfs/nginx-data-pvc type nfs

# 尝试手动卸载
umount /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~nfs/nginx-data-pvc
# umount: /var/lib/kubelet/pods/.../volumes/kubernetes.io~nfs/nginx-data-pvc: device is busy

# 查找占用进程
lsof +D /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~nfs/nginx-data-pvc

# COMMAND   PID  USER  FD   TYPE DEVICE SIZE/OFF NODE NAME
# nginx     1234 root  10u  REG  0,50   1048576  123  /var/lib/kubelet/pods/.../volumes/kubernetes.io~nfs/nginx-data-pvc/upload.dat

# ❌ 根因：nginx 进程仍在运行，持有 NFS 文件句柄
```

**Step 4：检查容器进程状态**

```bash
# 查看容器是否已停止
crictl ps -a | grep nginx-abc123

# CONTAINER ID  IMAGE   CREATED         STATE    NAME
# 9876543210ab  nginx   35 minutes ago  Running  nginx  # ❌ 容器仍在运行

# 查看容器详细信息
crictl inspect 9876543210ab | jq '.status.state'
# "running"  # ❌ 容器未停止

# 查看 kubelet 日志
journalctl -u kubelet -f | grep nginx-abc123

# 错误日志：
# E0210 10:30:00 kubelet.go:1234] Failed to stop container 9876543210ab: 
#   context deadline exceeded (timeout waiting for container to stop)
```

**根因分析**：
1. `kubectl delete pod` 发送 SIGTERM 给 nginx 主进程
2. nginx 主进程捕获信号，但**未关闭打开的文件**（upload.dat）
3. kubelet 等待 `terminationGracePeriodSeconds: 30s` 后发送 SIGKILL
4. 容器被强制杀死，但 NFS 文件句柄仍被内核持有
5. kubelet 无法卸载 NFS 卷 → Volume 卸载失败 → Pod 无法删除

#### 5.2.3 解决方案

**紧急止损**（强制清理，10 分钟）：

```bash
# Step 1：在节点上强制杀死容器进程
ssh $NODE
PID=$(crictl inspect 9876543210ab | jq .info.pid)
kill -9 $PID

# Step 2：强制卸载 NFS（lazy umount）
umount -f -l /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~nfs/nginx-data-pvc

# Step 3：删除 Pod 的 Finalizer
kubectl patch pod nginx-abc123 -p '{"metadata":{"finalizers":null}}'

# Step 4：强制删除 Pod
kubectl delete pod nginx-abc123 --force --grace-period=0

# 验证删除成功
kubectl get pod nginx-abc123
# Error from server (NotFound): pods "nginx-abc123" not found  # ✅ 已删除
```

**根本修复**（优化 PreStop Hook，2 小时）：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  containers:
  - name: nginx
    image: nginx:1.21
    volumeMounts:
    - name: data
      mountPath: /data
    lifecycle:
      preStop:
        exec:
          command:
          - sh
          - -c
          - |
            # Step 1：停止接收新连接
            nginx -s quit
            
            # Step 2：等待现有连接处理完成
            sleep 5
            
            # Step 3：强制关闭所有打开的文件描述符（除 stdin/stdout/stderr）
            for fd in /proc/self/fd/*; do
              fd_num=$(basename $fd)
              if [ "$fd_num" -gt 2 ]; then
                exec {fd_num}<&-  # 关闭文件描述符
              fi
            done
            
            # Step 4：同步文件系统缓存
            sync
  
  # 增加优雅停机时间
  terminationGracePeriodSeconds: 60  # 从 30s 增加到 60s
  
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: nginx-data-pvc
```

**长期优化**（使用本地存储 + 异步上传）：

```yaml
# 方案：将上传文件先存储到 emptyDir（本地），后台异步上传到 NFS
apiVersion: v1
kind: Pod
metadata:
  name: nginx-optimized
spec:
  containers:
  - name: nginx
    image: nginx:1.21
    volumeMounts:
    - name: upload-temp
      mountPath: /data/uploads  # 临时存储
  
  - name: uploader
    image: rclone/rclone:latest
    command:
    - sh
    - -c
    - |
      # 每 10 秒将本地文件同步到 NFS
      while true; do
        rclone sync /data/uploads /nfs/uploads --transfers 4
        sleep 10
      done
    volumeMounts:
    - name: upload-temp
      mountPath: /data/uploads
    - name: nfs-storage
      mountPath: /nfs
  
  volumes:
  - name: upload-temp
    emptyDir: {}  # 本地临时存储（Pod 删除时自动清理）
  - name: nfs-storage
    persistentVolumeClaim:
      claimName: nginx-data-pvc
```

#### 5.2.4 防护措施

**1. 自动检测 Terminating 卡住的 Pod**：
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: terminating-pod-cleaner
spec:
  schedule: "*/5 * * * *"  # 每 5 分钟
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pod-cleaner
          containers:
          - name: cleaner
            image: bitnami/kubectl:1.28
            command:
            - bash
            - -c
            - |
              # 查找 Terminating 超过 10 分钟的 Pod
              kubectl get pods -A -o json | jq -r '
                .items[] |
                select(.metadata.deletionTimestamp != null) |
                select((now - (.metadata.deletionTimestamp | fromdateiso8601)) > 600) |
                [.metadata.namespace, .metadata.name, (now - (.metadata.deletionTimestamp | fromdateiso8601))] |
                @tsv
              ' | while read ns name age; do
                echo "Cleaning stuck pod: $ns/$name (age: ${age}s)"
                
                # 删除 Finalizers
                kubectl patch pod $name -n $ns -p '{"metadata":{"finalizers":null}}'
                
                # 强制删除
                kubectl delete pod $name -n $ns --force --grace-period=0
              done
          restartPolicy: OnFailure
```

**2. Prometheus 告警**：
```yaml
- alert: PodStuckInTerminating
  expr: kube_pod_deletion_timestamp > 0
  for: 10m
  annotations:
    summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 删除超时"
    description: "检查 Finalizers、PreStop Hook 和 Volume 卸载"
```

**成果**：
- ✅ Pod 删除时间从 30min+ → <5s
- ✅ NFS Volume 卸载成功率 100%
- ✅ 自动清理卡住的 Pod（<10min 自动修复）

---

### 5.3 案例三：Pod 网络偶发超时（DNS 解析 5 秒延迟）

#### 5.3.1 故障现场

**背景**：
- 应用：微服务架构，服务间通过 Service 名称通信
- 现象：偶发性请求超时（5-10秒），影响 1-2% 的请求
- 环境：100+ 微服务，1000+ Pod

**用户投诉**：
- "页面加载很慢，有时需要 10 秒"
- "API 偶尔超时，但重试就成功"

**监控数据**：
```
# Prometheus 查询
histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m]))
# P99 延迟：10s（正常应该是 <500ms）

# 链路追踪（Jaeger）
Span: user-service -> order-service
  - DNS 解析：5200ms  # ❌ 异常！正常应 <10ms
  - TCP 连接：50ms
  - HTTP 请求：100ms
```

#### 5.3.2 深度排查过程

**Step 1：验证 DNS 解析性能**

```bash
# 在 Pod 内测试 DNS 解析
kubectl exec user-service-abc123 -- time nslookup order-service.default.svc.cluster.local

# 输出（第 1 次）：
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# 
# Name:      order-service.default.svc.cluster.local
# Address 1: 10.100.5.10
# 
# real    0m 5.23s  # ❌ 5 秒延迟！

# 输出（第 2 次，立即重试）：
# real    0m 0.01s  # ✅ 缓存命中，极快

# 查看 /etc/resolv.conf
kubectl exec user-service-abc123 -- cat /etc/resolv.conf

# nameserver 10.96.0.10
# search default.svc.cluster.local svc.cluster.local cluster.local
# options ndots:5  # ❌ 关键配置
```

**Step 2：分析 DNS 查询链路**

```bash
# 在 Pod 内抓包（使用 tcpdump）
kubectl debug user-service-abc123 -it --image=nicolaka/netshoot --target=user-service -- \
  tcpdump -i any -n port 53 -w /tmp/dns.pcap

# 触发 DNS 解析
kubectl exec user-service-abc123 -- curl -v http://order-service:8080/api/orders

# 分析抓包文件（在本地使用 Wireshark 或 tshark）
# 发现查询序列：
# 1. order-service.default.svc.cluster.local.    -> 成功（10ms）
# 2. order-service.svc.cluster.local.            -> NXDOMAIN（超时 5s）
# 3. order-service.cluster.local.                -> NXDOMAIN（超时 5s）
# 4. order-service.                               -> NXDOMAIN（超时 5s）
# 5. order-service.default.svc.cluster.local.    -> 成功（缓存，<1ms）

# ❌ 总耗时：5s * 3 = 15s（但实际只需第 1 次查询）
```

**根因分析**：
- `ndots:5` 表示：域名中"点"数量 < 5 时，依次尝试 `search` 域
- `order-service` 只有 1 个点 → 触发 4 次查询（search 域 + 原始域）
- CoreDNS 对 NXDOMAIN 响应有超时（5秒/次）

**计算公式**：
```
查询次数 = len(search domains) + 1 = 3 + 1 = 4
总耗时 = 失败查询次数 * 超时时间 = 3 * 5s = 15s

实际略好（因为第 1 次查询成功，后续查询被取消）
```

#### 5.3.3 解决方案

**方案 1：优化 Pod 的 DNS 配置**

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: user-service
spec:
  containers:
  - name: app
    image: user-service:1.0
  dnsPolicy: ClusterFirst
  dnsConfig:
    options:
    - name: ndots
      value: "1"  # ✅ 从 5 降低到 1
    - name: timeout
      value: "2"  # ✅ 超时从 5s 降低到 2s
    - name: attempts
      value: "2"  # ✅ 重试次数从 3 降低到 2
```

**效果**：
- `order-service` 的点数（1）>= ndots（1）→ 直接查询原始域
- 查询序列：`order-service.` → 失败 → `order-service.default.svc.cluster.local.` → 成功
- 总耗时：<100ms（2 次查询）

**方案 2：使用 FQDN（Fully Qualified Domain Name）**

```java
// 应用代码修改：使用完整域名
// ❌ 错误：短域名
String url = "http://order-service:8080/api/orders";

// ✅ 正确：FQDN（末尾有点）
String url = "http://order-service.default.svc.cluster.local.:8080/api/orders";
//                                                             ^  注意末尾的点
```

**效果**：
- FQDN 直接查询，跳过 search 域
- 查询次数：1 次
- 总耗时：<10ms

**方案 3：部署 NodeLocal DNSCache**

```yaml
# DaemonSet: 在每个节点运行本地 DNS 缓存
apiVersion: v1
kind: ConfigMap
metadata:
  name: nodelocaldns
  namespace: kube-system
data:
  Corefile: |
    cluster.local:53 {
        errors
        cache {
            success 9984 30  # 缓存成功响应 30s
            denial 9984 5    # 缓存 NXDOMAIN 5s
        }
        reload
        loop
        bind 169.254.20.10
        forward . 10.96.0.10  # 转发到 CoreDNS
        prometheus :9253
    }
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nodelocaldns
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: nodelocaldns
  template:
    metadata:
      labels:
        k8s-app: nodelocaldns
    spec:
      hostNetwork: true
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.20
        args:
        - -localip=169.254.20.10
        - -conf=/etc/coredns/Corefile
```

**效果**：
- Pod DNS 请求发送到节点本地（169.254.20.10）
- 零跳转（无需跨节点访问 CoreDNS Service）
- 缓存命中率 >95%
- 平均延迟：<1ms

#### 5.3.4 防护措施

**1. DNS 性能监控**：
```yaml
# Prometheus 指标
coredns_dns_request_duration_seconds_bucket  # DNS 请求延迟分布
coredns_dns_responses_total{rcode="NXDOMAIN"}  # NXDOMAIN 响应数量

# 告警规则
- alert: DNSHighLatency
  expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 1
  for: 5m
  annotations:
    summary: "DNS 解析 P99 延迟 >1s"

- alert: DNSNXDOMAINHigh
  expr: rate(coredns_dns_responses_total{rcode="NXDOMAIN"}[5m]) > 100
  for: 10m
  annotations:
    summary: "DNS NXDOMAIN 响应过多（可能是 ndots 配置问题）"
```

**2. 自动修复 ndots 配置**：
```yaml
# MutatingWebhook: 自动为所有 Pod 注入 ndots=1
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: ndots-injector
webhooks:
- name: ndots.example.com
  rules:
  - operations: ["CREATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
  clientConfig:
    service:
      name: ndots-injector
      namespace: kube-system
      path: "/mutate"
  admissionReviewVersions: ["v1"]
  sideEffects: None
```

**成果**：
- ✅ DNS 解析 P99 延迟：5s → <10ms（↓99.8%）
- ✅ API 超时率：2% → <0.01%
- ✅ 用户投诉数：清零

---

## 附录：Pod 专家巡检表

### A. 资源配置检查

- [ ] **QoS 等级**：是否设置了 `requests` 和 `limits`？是否符合 Guaranteed/Burstable/BestEffort 预期？
  ```bash
  kubectl get pod <pod> -o jsonpath='{.status.qosClass}'
  ```

- [ ] **资源配额**：是否受 ResourceQuota 或 LimitRange 限制？
  ```bash
  kubectl describe resourcequota -n <namespace>
  ```

- [ ] **节点亲和性**：`nodeSelector`、`affinity` 配置是否合理？
  ```bash
  kubectl get pod <pod> -o yaml | grep -A10 affinity
  ```

- [ ] **反亲和性**：关键业务是否配置了 `podAntiAffinity` 防止单点？

---

### B. 健康检查配置

- [ ] **Liveness Probe**：是否正确配置？是否依赖外部服务（避免）？
- [ ] **Readiness Probe**：是否正确配置？初始延迟是否足够？
- [ ] **Startup Probe**：启动慢的应用（如 Java）是否配置了 `startupProbe`？

---

### C. 安全加固

- [ ] **RunAsNonRoot**：是否以非 root 用户运行？
  ```yaml
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
  ```

- [ ] **ReadOnlyRootFilesystem**：是否将根文件系统设为只读？
  ```yaml
  securityContext:
    readOnlyRootFilesystem: true
  ```

- [ ] **Capabilities**：是否移除了不必要的 Linux Capabilities？

---

### D. 网络配置

- [ ] **DNS 配置**：`ndots` 是否优化（推荐 1）？
- [ ] **Service 连通性**：能否访问其他 Service？
- [ ] **NetworkPolicy**：是否配置了网络策略（生产环境推荐）？

---

### E. 存储配置

- [ ] **Volume 类型**：emptyDir/hostPath/PVC，是否符合场景？
- [ ] **PVC 绑定**：PVC 是否成功绑定到 PV？
- [ ] **挂载权限**：容器用户是否有权限读写 Volume？

---

## 附录：Pod 退出状态码速查表

| 退出码 | 信号 | 含义 | 排查方向 |
|:------|:-----|:-----|:--------|
| 0 | - | 正常退出 | 对于服务型容器，检查为何主循环结束 |
| 1 | - | 通用错误 | 查看应用日志 |
| 2 | - | Shell 误用 | 检查 entrypoint 脚本 |
| 126 | - | 命令无法执行 | 文件权限或路径错误 |
| 127 | - | 命令未找到 | entrypoint 拼写错误 |
| 137 | SIGKILL | 强制终止 | OOMKilled 或强制删除 |
| 139 | SIGSEGV | 段错误 | C/C++ 程序内存访问非法 |
| 143 | SIGTERM | 优雅终止 | 正常，但如果卡住则未处理信号 |
| 255 | - | 退出码溢出 | 应用返回值超出范围 |

---

## 附录：常用诊断命令速查

```bash
# 1. 查看 Pod 完整状态
kubectl get pod <pod> -o yaml

# 2. 查看 Pod 事件历史
kubectl get events --field-selector involvedObject.name=<pod> --sort-by='.lastTimestamp'

# 3. 查看容器退出码和信号
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[*].lastState.terminated}' | jq

# 4. 在节点上查看容器进程
crictl ps | grep <pod>
crictl inspect <container-id>

# 5. 进入容器的 Mount Namespace
nsenter -t $(crictl inspect <container-id> | jq .info.pid) -m

# 6. 追踪容器系统调用
strace -p $(crictl inspect <container-id> | jq .info.pid) -f

# 7. 查看 cgroup 资源限制
cat /sys/fs/cgroup/memory/kubepods/pod<uid>/<container-id>/memory.limit_in_bytes

# 8. 查看内核 OOM 日志
dmesg | grep -i "killed process"

# 9. 测试 DNS 解析
kubectl exec <pod> -- nslookup <service-name>

# 10. 抓取网络包
kubectl debug <pod> -it --image=nicolaka/netshoot --target=<container> -- \
  tcpdump -i any -w /tmp/capture.pcap
```

---

**文档维护**：
- 最后更新：2026-02
- 维护者：Kubernetes 工作负载专家组
- 反馈渠道：GitHub Issues / 内部 Wiki

---

**关键词**：Pod, Container, Sandbox, PID 1, CrashLoopBackOff, OOMKilled, Liveness Probe, Readiness Probe, DNS Resolution, Kubernetes 故障排查
