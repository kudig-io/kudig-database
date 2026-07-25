---
title: Kubernetes 命令输出 → 根因解析语料库
description: '# Kubernetes 命令输出 → 根因解析语料库'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- kubelet
- scheduler
- envoy
- cilium
- flannel
- calico
- coredns
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Kubernetes 命令输出 → 根因解析语料库 是什么
- 如何 Kubernetes 命令输出 → 根因解析语料库
- Kubernetes 命令输出 → 根因解析语料库 故障排查
- Kubernetes 命令输出 → 根因解析语料库 排障步骤
trigger_keywords:
- Kubernetes
- 命令输出
- 根因解析语料库
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- cilium-basics
- cni-basics
- redis-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 命令输出 → 根因解析语料库

> **文档类型**: Agent 诊断语料 | **适用版本**: K8s 1.28-1.33 | **条目数**: 100+ | **最后更新**: 2026-05
> **使用场景**: Agent 从 kubectl 原始输出中提取关键信息，判断根因，给出修复建议

---

## 目录

- [1. kubectl describe pod 输出解析](#1-kubectl-describe-pod-输出解析)
- [2. kubectl logs 输出解析](#2-kubectl-logs-输出解析)
- [3. kubectl get events 输出解析](#3-kubectl-get-events-输出解析)
- [4. kubectl top node/pod 输出解析](#4-kubectl-top-nodepod-输出解析)
- [5. kubectl exec/attach/cp 失败解析](#5-kubectl-execattachcp-失败解析)
- [6. kubectl describe node 输出解析](#6-kubectl-describe-node-输出解析)
- [7. kubectl get 资源状态解析](#7-kubectl-get-资源状态解析)
- [8. kubectl describe 各类资源输出解析](#8-kubectl-describe-各类资源输出解析)

---

## 1. kubectl describe pod 输出解析

### 1.1 Conditions 段 —— 调度失败

```yaml
output_pattern:
  - id: "pod-001"
    command: "kubectl describe pod <pod-name>"
    section: "Conditions"
    indicator: |
      Type: PodScheduled
      Status: False
      Reason: Unschedulable
      Message: "0/3 nodes are available: 1 Insufficient memory, 2 node(s) had taint {node.kubernetes.io/not-ready:}, that the pod didn't tolerate."
    diagnosis: "Pod 无法调度，节点资源不足（内存）+ 节点 NotReady + 有污点未容忍"
    severity: P1
    urgency: high
    possible_causes:
      - cause: "节点内存不足"
        indicators: ["Insufficient memory", "memory available < request"]
        next_step: "检查节点内存使用 kubectl top node / describe node"
      - cause: "节点 NotReady"
        indicators: ["node(s) had taint", "node.kubernetes.io/not-ready"]
        next_step: "检查节点状态 kubectl get node / describe node"
      - cause: "Pod 未容忍污点"
        indicators: ["didn't tolerate", "taint"]
        next_step: "检查 Pod spec tolerations / 节点污点"
    k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
    related_sop: "SOP-POD-PENDING"
```

```yaml
output_pattern:
  - id: "pod-002"
    command: "kubectl describe pod <pod-name>"
    section: "Conditions"
    indicator: |
      Type: PodScheduled
      Status: False
      Reason: Unschedulable
      Message: "0/3 nodes are available: 3 Insufficient cpu."
    diagnosis: "Pod 调度失败，CPU 资源不足"
    severity: P1
    possible_causes:
      - cause: "节点 CPU 不足"
        indicators: ["Insufficient cpu"]
        next_step: "检查节点 CPU 使用率，降低 Pod requests 或扩容节点"
```

```yaml
output_pattern:
  - id: "pod-003"
    command: "kubectl describe pod <pod-name>"
    section: "Conditions"
    indicator: |
      Type: PodScheduled
      Status: False
      Reason: Unschedulable
      Message: "0/3 nodes are available: 1 node(s) had no available volume zone."
    diagnosis: "Pod 调度失败，PVC 指定的可用区没有节点"
    severity: P1
    possible_causes:
      - cause: "PVC 的 StorageClass 指定了特定可用区"
        indicators: ["no available volume zone"]
        next_step: "检查 PVC 的 volumeBindingMode 是否为 WaitForFirstConsumer，或修改 StorageClass 的 allowedTopologies"
```

```yaml
output_pattern:
  - id: "pod-004"
    command: "kubectl describe pod <pod-name>"
    section: "Conditions"
    indicator: |
      Type: PodScheduled
      Status: False
      Reason: Unschedulable
      Message: "0/3 nodes are available: 1 node(s) had volume node affinity conflict."
    diagnosis: "Pod 调度失败，Pod 与 PV 的节点亲和性冲突"
    severity: P1
    possible_causes:
      - cause: "Pod 调度到了 PV 所在节点以外的节点"
        indicators: ["volume node affinity conflict"]
        next_step: "使用 WaitForFirstConsumer 或手动指定节点"
```

### 1.2 Containers 段 —— 镜像拉取失败

```yaml
output_pattern:
  - id: "pod-005"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: ImagePullBackOff
      Message: "rpc error: code = Unknown desc = failed to pull image registry.example.com/nginx:1.25: http request failed StatusCode=403"
    diagnosis: "镜像拉取失败，HTTP 403 认证/权限被拒绝"
    severity: P0
    possible_causes:
      - cause: "镜像仓库凭证错误或失效"
        indicators: ["StatusCode=403", "ImagePullBackOff"]
        next_step: "检查 Secret imagePullSecrets 是否正确，kubectl get secret -n <ns>"
      - cause: "镜像仓库网络不通"
        indicators: ["StatusCode=403" without auth error]
        next_step: "在节点上手动拉取测试: crictl pull <image>"
```

```yaml
output_pattern:
  - id: "pod-006"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: ImagePullBackOff
      Message: "rpc error: code = Unknown desc = failed to pull image registry.example.com/nginx:1.25: http request failed StatusCode=404"
    diagnosis: "镜像拉取失败，镜像不存在（404）"
    severity: P0
    possible_causes:
      - cause: "镜像 tag 不存在或拼写错误"
        indicators: ["StatusCode=404"]
        next_step: "确认镜像 tag 是否正确，检查 registry 中的镜像列表"
```

```yaml
output_pattern:
  - id: "pod-007"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: ImagePullBackOff
      Message: "rpc error: code = Unknown desc = failed to pull image registry.example.com/nginx:1.25: dial tcp: i/o timeout"
    diagnosis: "镜像拉取失败，网络超时"
    severity: P1
    possible_causes:
      - cause: "节点到镜像仓库网络不通"
        indicators: ["i/o timeout", "dial tcp"]
        next_step: "在节点上测试网络: curl -v <image-registry>"
      - cause: "镜像仓库访问限流"
        indicators: ["timeout" after retries]
        next_step: "配置 mirror 或使用内网 registry"
```

```yaml
output_pattern:
  - id: "pod-008"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: ErrImagePull
      Message: "rpc error: code = Unknown desc = failed to pull image registry.example.com/nginx:1.25: x509: certificate has expired"
    diagnosis: "镜像拉取失败，证书过期"
    severity: P0
    possible_causes:
      - cause: "私有镜像仓库证书过期"
        indicators: ["x509: certificate has expired"]
        next_step: "更新 registry 证书，或在 kubelet 配置 --registry-pull-qps 和跳过证书验证"
```

```yaml
output_pattern:
  - id: "pod-009"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: ErrImagePull
      Message: 'spec.containers[0].image: Invalid value "nginx:1.2x": Invalid reference format'
    diagnosis: "镜像名称格式错误"
    severity: P1
    possible_causes:
      - cause: "镜像 tag 含有非法字符"
        indicators: ["Invalid reference format"]
        next_step: "修正镜像名称，使用合法的 tag"
```

### 1.3 Containers 段 —— CrashLoopBackOff

```yaml
output_pattern:
  - id: "pod-010"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: CrashLoopBackOff
      Last State: Terminated
        Exit Code: 1
    diagnosis: "容器启动后立即退出（Exit Code 1），典型应用错误"
    severity: P1
    possible_causes:
      - cause: "应用启动命令或参数错误"
        indicators: ["Exit Code: 1", "CrashLoopBackOff"]
        next_step: "查看上一轮日志: kubectl logs <pod> --previous"
      - cause: "配置文件缺失或权限错误"
        indicators: ["Exit Code: 1" + no logs]
        next_step: "检查应用代码的初始化逻辑，确认 ConfigMap/Secret 挂载正确"
```

```yaml
output_pattern:
  - id: "pod-011"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Running
      Ready: False
      Restart Count: 5
      Last State: Terminated
        Exit Code: 137
        Signal: 9
    diagnosis: "容器被 SIGKILL（137 = 128+9），OOMKilled 或被手动 kill"
    severity: P0
    possible_causes:
      - cause: "OOMKilled（内存超限）"
        indicators: ["Exit Code: 137", "Signal: 9"]
        next_step: "检查 kubectl top pod / kubectl describe node 确认内存使用"
      - cause: "节点资源压力被 kubelet 驱逐"
        indicators: ["Exit Code: 137" + 节点 NotReady"]
        next_step: "检查节点是否有其他 Pod 竞争资源"
```

```yaml
output_pattern:
  - id: "pod-012"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      State: Waiting
      Reason: CrashLoopBackOff
      Last State: Terminated
        Exit Code: 127
    diagnosis: "容器启动失败，命令或依赖可执行文件不存在"
    severity: P1
    possible_causes:
      - cause: "ENTRYPOINT / CMD 路径错误或文件无执行权限"
        indicators: ["Exit Code: 127"]
        next_step: "检查 Dockerfile 的 CMD/ENTRYPOINT 是否使用绝对路径"
```

### 1.4 Containers 段 —— OOMKilled

```yaml
output_pattern:
  - id: "pod-013"
    command: "kubectl describe pod <pod-name>"
    section: "Containers"
    indicator: |
      Last State: Terminated
        Reason: OOMKilled
        Exit Code: 137
        Finished: 2026-05-18T10:30:00Z
    diagnosis: "容器内存超限被 OOM Killer 杀死"
    severity: P0
    possible_causes:
      - cause: "Pod 内存 limit 设置过低"
        indicators: ["OOMKilled"]
        next_step: "查看 kubectl top pod 确认实际使用内存，调高 limits.memory 或优化应用内存使用"
      - cause: "应用内存泄漏"
        indicators: ["OOMKilled" + restart count 持续上升]
        next_step: "分析堆内存 dump，定位内存泄漏"
```

### 1.5 Events 段 —— 调度相关

```yaml
output_pattern:
  - id: "pod-014"
    command: "kubectl describe pod <pod-name>"
    section: "Events"
    indicator: |
      Type     Reason                  Age                   From                Message
      Normal   Scheduled               2m                    default-scheduler   Successfully assigned default/nginx to node-1
      Warning  FailedScheduling        1m                    default-scheduler   0/3 nodes available: 3 Insufficient memory.
    diagnosis: "Pod 首次调度成功，但曾因内存不足失败过（历史事件）"
    severity: P2
    possible_causes:
      - cause: "历史调度失败，当前已恢复"
        indicators: ["Successfully assigned" + 历史 "Insufficient memory"]
        next_step: "确认 Pod 当前 Running 状态，监控内存使用趋势"
```

```yaml
output_pattern:
  - id: "pod-015"
    command: "kubectl describe pod <pod-name>"
    section: "Events"
    indicator: |
      Warning   FailedScheduling        5s    default-scheduler   skip scheduling because of unfinished predicates.
    diagnosis: "调度器未完成谓词计算，典型于节点池扩缩容或缓存同步延迟"
    severity: P1
    possible_causes:
      - cause: "调度器缓存与实际节点状态不同步"
        indicators: ["unfinished predicates"]
        next_step: "等待 30s 重试，或检查 kube-scheduler 日志"
```

### 1.6 完整 Status 解析

```yaml
output_pattern:
  - id: "pod-016"
    command: "kubectl get pod <pod-name>"
    status_line: "nginx-7d9f6b8c5-xk2p4   0/1     Pending   0          5m"
    status_fields:
      ready: "0/1"
      restarts: "0"
      status: "Pending"
      age: "5m"
    diagnosis: "Pod 卡在 Pending，未被调度，5分钟内无变化"
    severity: P1
    possible_causes:
      - cause: "调度失败未触发 Events"
        indicators: ["Pending + no events visible"]
        next_step: "kubectl describe pod 查看 Conditions 和 Events"
      - cause: "调度成功但容器未启动"
        indicators: ["Pending + ImagePullBackOff in describe"]
        next_step: "按镜像拉取失败处理"
```

```yaml
output_pattern:
  - id: "pod-017"
    command: "kubectl get pod <pod-name>"
    status_line: "nginx-7d9f6b8c5-xk2p4   0/1     Terminating   0          10m"
    diagnosis: "Pod 处于 Terminating 状态，10分钟内未完成删除"
    severity: P1
    possible_causes:
      - cause: "finalizers 阻塞删除"
        indicators: ["Terminating + kubectl get pod shows deletionTimestamp set"]
        next_step: "检查是否有 finalizers，kubectl get pod <pod> -o yaml | grep finalizers"
      - cause: "容器 graceful shutdown 超时"
        indicators: ["Terminating + SIGTERM 未被处理"]
        next_step: "检查应用是否正确处理 SIGTERM，增加 terminationGracePeriodSeconds"
      - cause: "存储卷/PVC 阻塞"
        indicators: ["Terminating + PVC still bound"]
        next_step: "强制删除: kubectl delete pod <pod> --grace-period=0 --force"
```

### 1.7 PodDisruptionBudget 相关

```yaml
output_pattern:
  - id: "pod-018"
    command: "kubectl describe pod <pod-name>"
    section: "Events"
    indicator: |
      Warning   FailedCreate        3s    job-controller   Cannot create pod: pod is blocked by PDB
    diagnosis: "Job 控制器无法创建 Pod，因为 PDB 保护了太多 Pod"
    severity: P2
    possible_causes:
      - cause: "PDB 的 minAvailable 过低导致无法驱逐"
        indicators: ["blocked by PDB"]
        next_step: "调整 PDB 的 minAvailable 或使用 maxUnavailable"
```

---

## 2. kubectl logs 输出解析

### 2.1 应用层错误

```yaml
output_pattern:
  - id: "log-001"
    command: "kubectl logs <pod-name>"
    pattern: "panic: runtime error: index out of range"
    raw_output: |
      panic: runtime error: index out of range
      goroutine 1234 [running]
      main.processHTTP()
    diagnosis: "Go 应用崩溃（panic），索引越界"
    severity: P0
    possible_causes:
      - cause: "代码逻辑缺陷，访问了越界 slice/index"
        indicators: ["panic", "index out of range"]
        next_step: "查看完整堆栈定位 panic 位置，分析请求参数"
```

```yaml
output_pattern:
  - id: "log-002"
    command: "kubectl logs <pod-name>"
    pattern: "fatal: not a git repository"
    diagnosis: "应用启动失败，需要 git 仓库但当前目录不是"
    severity: P1
    possible_causes:
      - cause: "WORKDIR 设置错误或 Dockerfile 构建上下文错误"
        indicators: ["fatal: not a git repository"]
        next_step: "检查 Dockerfile 和 WORKDIR 配置"
```

```yaml
output_pattern:
  - id: "log-003"
    command: "kubectl logs <pod-name>"
    pattern: "connect: connection refused"
    diagnosis: "应用无法连接到下游服务（数据库/Redis/API）"
    severity: P1
    possible_causes:
      - cause: "下游服务未启动或端口错误"
        indicators: ["connection refused"]
        next_step: "检查下游服务状态和 Service 配置"
```

```yaml
output_pattern:
  - id: "log-004"
    command: "kubectl logs <pod-name>"
    pattern: "context deadline exceeded"
    diagnosis: "请求超时（gRPC/HTTP client 超时）"
    severity: P1
    possible_causes:
      - cause: "下游服务响应慢或网络问题"
        indicators: ["context deadline exceeded"]
        next_step: "检查下游服务延迟，增加 timeout 配置"
```

```yaml
output_pattern:
  - id: "log-005"
    command: "kubectl logs <pod-name>"
    pattern: "dial tcp: lookup registry.example.com: no such host"
    diagnosis: "DNS 解析失败，无法访问镜像仓库或外部服务"
    severity: P0
    possible_causes:
      - cause: "DNS 配置错误或 CoreDNS 异常"
        indicators: ["no such host"]
        next_step: "检查集群 DNS 状态: kubectl exec <pod> -- nslookup registry.example.com"
```

```yaml
output_pattern:
  - id: "log-006"
    command: "kubectl logs <pod-name>"
    pattern: "x509: certificate signed by unknown authority"
    diagnosis: "TLS 证书验证失败，不受信任的 CA"
    severity: P0
    possible_causes:
      - cause: "私有 CA 证书未被信任"
        indicators: ["x509: certificate signed by unknown authority"]
        next_step: "将私有 CA 证书挂载到 Pod 的信任存储，或配置 skipVerify"
```

```yaml
output_pattern:
  - id: "log-007"
    command: "kubectl logs <pod-name>"
    pattern: "OOMKilled"
    raw_output: |
      kubectl exec nginx-pod -- dmesg | grep -i memory
      [  123.456] Memory cgroup out of memory: Killed process 456 (nginx) total-vm:2048576kB
    diagnosis: "容器内存超限被 OOM Killer 杀死"
    severity: P0
    possible_causes:
      - cause: "Pod 内存 limit 不足"
        indicators: ["OOMKilled in dmesg"]
        next_step: "调高 memory limit，或优化应用内存使用"
```

### 2.2 Java/JVM 层错误

```yaml
output_pattern:
  - id: "log-008"
    command: "kubectl logs <pod-name>"
    pattern: "java.lang.OutOfMemoryError: Java heap space"
    diagnosis: "JVM 堆内存溢出"
    severity: P0
    possible_causes:
      - cause: "JVM堆内存设置过小或内存泄漏"
        indicators: ["OutOfMemoryError: Java heap space"]
        next_step: "增加 -Xmx 参数，配置 JVM heap size >= 2 * 峰值使用"
```

```yaml
output_pattern:
  - id: "log-009"
    command: "kubectl logs <pod-name>"
    pattern: "java.lang.OutOfMemoryError: GC overhead limit exceeded"
    diagnosis: "GC 开销超限，内存使用率过高且回收效率低"
    severity: P0
    possible_causes:
      - cause: "内存持续增长，GC 无法跟上分配速度"
        indicators: ["GC overhead limit exceeded"]
        next_step: "分析堆 dump，排查内存泄漏"
```

```yaml
output_pattern:
  - id: "log-010"
    command: "kubectl logs <pod-name>"
    pattern: "java.net.SocketTimeoutException: Accept timeout"
    diagnosis: "Java 服务端 accept 超时，线程池满或连接队列积压"
    severity: P1
    possible_causes:
      - cause: "Tomcat/Jetty 连接队列积压"
        indicators: ["Accept timeout"]
        next_step: "调高 acceptCount / maxThreads 配置"
```

### 2.3 Python 层错误

```yaml
output_pattern:
  - id: "log-011"
    command: "kubectl logs <pod-name>"
    pattern: "ModuleNotFoundError: No module named 'psycopg2'"
    diagnosis: "Python 依赖包缺失"
    severity: P1
    possible_causes:
      - cause: "requirements.txt 未正确安装"
        indicators: ["ModuleNotFoundError"]
        next_step: "检查 requirements.txt 和 Dockerfile pip install 命令"
```

```yaml
output_pattern:
  - id: "log-012"
    command: "kubectl logs <pod-name>"
    pattern: "django.db.utils.OperationalError: could not connect to server"
    diagnosis: "Django 无法连接 PostgreSQL 数据库"
    severity: P0
    possible_causes:
      - cause: "数据库地址/端口错误或数据库服务异常"
        indicators: ["could not connect to server"]
        next_step: "检查 DATABASE_URL 环境变量和数据库服务状态"
```

### 2.4 [[23-实体/04-网络/envoy|Envoy]] 层错误

```yaml
output_pattern:
  - id: "log-013"
    command: "kubectl logs <pod-name> -c nginx"
    pattern: "upstream timed out (110: Connection timed out)"
    diagnosis: "NGINX upstream 连接超时"
    severity: P1
    possible_causes:
      - cause: "后端 Pod 未就绪或网络不通"
        indicators: ["upstream timed out"]
        next_step: "检查 backend Pod 状态和 Service Endpoints"
```

```yaml
output_pattern:
  - id: "log-014"
    command: "kubectl logs <pod-name> -c envoy"
    pattern: "no healthy upstream"
    diagnosis: "Envoy 找不到健康的上游节点"
    severity: P1
    possible_causes:
      - cause: "所有 upstream Pod 均不健康或不在 Endpoints 中"
        indicators: ["no healthy upstream"]
        next_step: "检查 upstream Pod 状态和 health check 配置"
```

```yaml
output_pattern:
  - id: "log-015"
    command: "kubectl logs <pod-name> -c nginx"
    pattern: " connect() failed (111: Connection refused) while connecting to upstream"
    diagnosis: "NGINX 连接 upstream 失败（Connection refused）"
    severity: P1
    possible_causes:
      - cause: "upstream 服务端口未监听或 Pod 未就绪"
        indicators: ["Connection refused"]
        next_step: "检查 upstream Pod 的端口监听状态"
```

---

## 3. kubectl get events 输出解析

### 3.1 Pod 相关事件分类

```yaml
output_pattern:
  - id: "event-001"
    command: "kubectl get events --sort-by='.lastTimestamp' | grep -i pod"
    pattern: |
      LAST SEEN   TYPE      REASON              KIND      MESSAGE
      2m          Warning   Evicted             Pod       nginx-5f9d8c-xk2p4 was evicted
    diagnosis: "Pod 被驱逐（Evicted）"
    severity: P0
    possible_causes:
      - cause: "节点资源不足，kubelet 主动驱逐低优先级 Pod"
        indicators: ["Evicted" + low priority + resource pressure]
        next_step: "kubectl describe node 检查资源使用，确认是否有 eviction 阈值触发"
      - cause: "Pod QoS 等级低（BestEffort）被优先驱逐"
        indicators: ["Evicted" + BestEffort pod]
        next_step: "提高 Pod QoS 等级（Guaranteed/Burstable）"
```

```yaml
output_pattern:
  - id: "event-002"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      5m          Normal    Scheduled            Pod       nginx-7d9f6b8c5-xk2p4 Successfully scheduled
      5m          Normal    Pulling              Pod       nginx-7d9f6b8c5-xk2p4 Pulling image "nginx:1.25"
      4m          Normal    Pulled               Pod       nginx-7d9f6b8c5-xk2p4 Successfully pulled image
      4m          Normal    Created              Pod       nginx-7d9f6b8c5-xk2p4 Created container
      4m          Normal    Started              Pod       nginx-7d9f6b8c5-xk2p4 Started container
    diagnosis: "Pod 正常创建流程，无异常"
    severity: P0
    status: "normal_lifecycle"
    note: "这是正常生命周期事件序列，不代表有问题"
```

```yaml
output_pattern:
  - id: "event-003"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      3m          Warning   UnHealthy            Pod       nginx-7d9f6b8c5-xk2p4 Type:Ready Subject:Pod Name:nginx-xxx
    diagnosis: "Pod 就绪探针检查失败，容器可能被重启"
    severity: P1
    possible_causes:
      - cause: "应用响应探针时异常"
        indicators: ["UnHealthy" + "Type:Ready"]
        next_step: "检查应用健康检查端点，确认探针配置正确"
```

```yaml
output_pattern:
  - id: "event-004"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      1m          Warning   FailedCreate         Pod       Error creating: pods "nginx-xxx" already exists
    diagnosis: "Pod 创建冲突，可能由 ReplicaSet controller 并发问题导致"
    severity: P2
    possible_causes:
      - cause: "ReplicaSet 多次重试导致重名 Pod 存在"
        indicators: ["already exists"]
        next_step: "删除已存在的 Pod 让 ReplicaSet 重新管理"
```

### 3.2 节点相关事件

```yaml
output_pattern:
  - id: "event-005"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      10m         Warning   Registered           Node      Node node-1 has not been ready for 5m: Kubelet stopped posting node status.
    diagnosis: "节点 NotReady，Kubelet 停止上报状态已 5 分钟"
    severity: P0
    possible_causes:
      - cause: "节点上 kubelet 进程崩溃或网络中断"
        indicators: ["Kubelet stopped posting node status"]
        next_step: "SSH 到节点检查 kubelet 状态: systemctl status kubelet / journalctl -u kubelet"
      - cause: "节点网络分区"
        indicators: ["node not ready" + 跨所有 pod"]
        next_step: "检查节点网络连通性和安全组/防火墙配置"
```

```yaml
output_pattern:
  - id: "event-006"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      20m         Normal    NodeHasNoDiskPressure Node      Node node-1 status is now: NodeHasNoDiskPressure
    diagnosis: "节点磁盘压力正常（周期性正常事件）"
    severity: P0
    status: "normal"
    note: "这是正常心跳事件，NodeHasNoDiskPressure 表示节点无磁盘压力，不需要处理"
```

```yaml
output_pattern:
  - id: "event-007"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      5m          Warning   FailedScheduling     Pod       0/3 nodes available: 3 node(s) had taints that the pod didn't tolerate.
    diagnosis: "Pod 调度失败，节点有污点但 Pod 未设置对应容忍"
    severity: P1
    possible_causes:
      - cause: "Pod 调度到了有特殊污点的节点"
        indicators: ["didn't tolerate"]
        next_step: "检查节点污点: kubectl describe node | grep Taints，添加对应 tolerations"
```

### 3.3 存储相关事件

```yaml
output_pattern:
  - id: "event-008"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      2m          Warning   FailedBinding        PersistentVolumeClaim nginx-pvc   no node found to satisfy condition
    diagnosis: "PVC 绑定失败，拓扑约束无法满足（StorageClass 的 allowedTopologies）"
    severity: P1
    possible_causes:
      - cause: "PVC 的 StorageClass 指定了特定可用区，但集群节点不在该区"
        indicators: ["no node found to satisfy condition"]
        next_step: "调整 StorageClass 的 allowedTopologies 或使用 volumeBindingMode: WaitForFirstConsumer"
```

```yaml
output_pattern:
  - id: "event-009"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      3m          Warning   FailedAttachVolume   Pod       AttachVolume.Attach failed for volume "pvc-xxx"
    diagnosis: "云盘/存储卷挂载失败（云厂商 CSI）"
    severity: P1
    possible_causes:
      - cause: "云盘已在其他节点挂载（阿里云 CSP 限制）"
        indicators: ["AttachVolume.Attach failed"]
        next_step: "在云控制台检查云盘状态，解除已挂载节点的绑定"
      - cause: "CSI driver 问题"
        indicators: ["Attach failed" + no specific reason]
        next_step: "检查 CSI driver pod 日志和云厂商文档"
```

### 3.4 网络相关事件

```yaml
output_pattern:
  - id: "event-010"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      1m          Warning   FailedCreatePodSandBox Pod      Failed to create pod sandbox: rpc error: code = Unknown desc = network type cni.type not supported
    diagnosis: "CNI 插件类型不支持，通常是 CNI 配置错误"
    severity: P0
    possible_causes:
      - cause: "CNI 配置中指定了不存在的插件类型"
        indicators: ["network type cni.type not supported"]
        next_step: "检查 /etc/cni/net.d/ 中的 CNI 配置，确认插件名称正确"
```

```yaml
output_pattern:
  - id: "event-011"
    command: "kubectl get events --sort-by='.lastTimestamp'"
    pattern: |
      30s         Warning   FailedCreatePodSandBox Pod      Failed to create pod sandbox: cni plugin not initialized
    diagnosis: "CNI 插件未初始化，kubelet 与 CNI 插件通信异常"
    severity: P0
    possible_causes:
      - cause: "CNI 插件进程异常退出"
        indicators: ["cni plugin not initialized"]
        next_step: "检查节点上 CNI 插件进程（flanneld/cilium-agent/calico-node）状态"
```

---

## 4. kubectl top node/pod 输出解析

### 4.1 Node 资源使用率阈值判断

```yaml
output_pattern:
  - id: "top-001"
    command: "kubectl top node"
    raw_output: |
      NAME        CPU(c)   CPU%   MEMORY   MEMORY%
      node-1      3950m    98%    12Gi     95%
    threshold:
      cpu_percent: 90
      memory_percent: 90
    diagnosis: "节点 CPU 使用率 98%，内存使用率 95%，严重资源不足"
    severity: P0
    possible_causes:
      - cause: "节点上运行了过多 Pod"
        indicators: ["CPU% 98%"]
        next_step: "kubectl get pods -o wide --all-namespaces | grep node-1 | wc -l"
      - cause: "单 Pod CPU 密集型"
        indicators: ["CPU% 98% + MEMORY% normal"]
        next_step: "kubectl top pod --sort-by=cpu | head -5 定位高 CPU Pod"
```

```yaml
output_pattern:
  - id: "top-002"
    command: "kubectl top node"
    raw_output: |
      NAME        CPU(c)   CPU%   MEMORY   MEMORY%
      node-2      500m     12%    28Gi     95%
    diagnosis: "节点内存使用率 95% 但 CPU 正常，内存资源紧张"
    severity: P1
    possible_causes:
      - cause: "内存泄漏或缓存占满"
        indicators: ["MEMORY% 95% + CPU% normal"]
        next_step: "kubectl top pod --sort-by=memory | head -5 定位高内存 Pod"
      - cause: "Pod memory limit 设置过大导致内存预留"
        indicators: ["MEMORY% 95% + many Pods"]
        next_step: "检查 Pod 的 memory requests 是否合理"
```

```yaml
output_pattern:
  - id: "top-003"
    command: "kubectl top node"
    raw_output: |
      NAME        CPU(c)   CPU%   MEMORY   MEMORY%
      node-3      200m     5%     2Gi      16%
    threshold:
      cpu_percent: 90
      memory_percent: 90
    diagnosis: "节点资源充足，无压力（CPU 5%, Memory 16%）"
    severity: P0
    status: "normal"
    note: "如节点出现异常但资源充足，问题可能在网络/存储/应用层"
```

### 4.2 Pod 资源使用率阈值判断

```yaml
output_pattern:
  - id: "top-004"
    command: "kubectl top pod -n <namespace>"
    raw_output: |
      NAME                    CPU(c)   CPU%   MEMORY
      nginx-7d9f6b8c5-xk2p4   980m     98%    256Mi
      redis-master            50m      5%     500Mi
    threshold:
      cpu_percent: 90
      memory_percent: 90
    diagnosis: "Pod CPU 使用率 98%，触发 CPU throttle"
    severity: P1
    possible_causes:
      - cause: "Pod CPU limit 过低导致节流"
        indicators: ["CPU% 98% + throttling"]
        next_step: "检查 Dockerfile/limits，CPU throttle 时延: kubectl describe pod 查看 CPU Throttling"
```

```yaml
output_pattern:
  - id: "top-005"
    command: "kubectl top pod -n <namespace>"
    raw_output: |
      NAME                    CPU(c)   CPU%   MEMORY
      java-app-0              2000m    40%    8Gi     95%
    diagnosis: "Java 进程内存使用率接近 limit（95%），存在 OOM 风险"
    severity: P0
    possible_causes:
      - cause: "JVM heap 设置 + 元空间/直接内存 + 容器 overhead 接近 limit"
        indicators: ["MEMORY 95% + Java"]
        next_step: "增加 container memory limit，或降低 JVM heap size (-Xmx)"
```

### 4.3 无 metrics 数据时的解读

```yaml
output_pattern:
  - id: "top-006"
    command: "kubectl top node"
    raw_output: "error: metrics not available yet"
    diagnosis: "metrics-server 未启动或未部署，top 命令无法获取数据"
    severity: P1
    possible_causes:
      - cause: "metrics-server deployment 未运行"
        indicators: ["metrics not available yet"]
        next_step: "kubectl get pods -n kube-system | grep metrics-server"
      - cause: "kubelet 没有正确配置 metrics 端点"
        indicators: ["metrics not available" + APIService unavailable"]
        next_step: "检查 kubelet 配置 --authentication-webhook 和 metrics 端口"
```

---

## 5. kubectl exec/attach/cp 失败解析

### 5.1 认证/权限类

```yaml
output_pattern:
  - id: "exec-001"
    command: "kubectl exec -it <pod> -- /bin/bash"
    error: |
      error: unable to upgrade connection: not authorized: unauthorized
    diagnosis: "RBAC 权限不足，无法 exec 到容器"
    severity: P0
    possible_causes:
      - cause: "ServiceAccount 没有 exec/create 权限"
        indicators: ["not authorized: unauthorized"]
        next_step: "检查 RBAC RoleBinding，确保有 pod/exec 权限"
```

```yaml
output_pattern:
  - id: "exec-002"
    command: "kubectl exec <pod> -- ls"
    error: |
      error: unable to upgrade connection: Forbidden: not allowed to do this operation
    diagnosis: "Pod Security Policy / SecurityContext 禁止 exec"
    severity: P1
    possible_causes:
      - cause: "PSP（Pod Security Policy）禁止 exec"
        indicators: ["Forbidden: not allowed"]
        next_step: "检查 Pod 安全策略配置，或使用其他方式（kubectl logs）替代"
```

### 5.2 网络/连接类

```yaml
output_pattern:
  - id: "exec-003"
    command: "kubectl exec -it <pod> -- /bin/bash"
    error: |
      error: unable to upgrade connection: Forbidden
    diagnosis: "API Server 与 Pod 之间的隧道中断（网络问题或 kubelet 异常）"
    severity: P0
    possible_causes:
      - cause: "节点网络分区或 kubelet 异常"
        indicators: ["unable to upgrade connection"]
        next_step: "检查节点状态 kubectl get node，SSH 到节点检查 kubelet"
```

```yaml
output_pattern:
  - id: "exec-004"
    command: "kubectl cp <file> <pod>:/path"
    error: |
      error: unable to upgrade connection: connection refused
    diagnosis: "Pod 内部的容器无法访问（容器未启动或端口不监听）"
    severity: P1
    possible_causes:
      - cause: "容器还未启动完成"
        indicators: ["connection refused"]
        next_step: "确认 Pod 状态 Running 后重试"
```

### 5.3 容器运行时不兼容

```yaml
output_pattern:
  - id: "exec-005"
    command: "kubectl exec <pod> -- ls"
    error: |
      Error from server (NotAcceptable): unable to convert to vty: no container runtime detected
    diagnosis: "容器运行时不兼容（k3s 使用 containerd 但 exec 协议不匹配）"
    severity: P1
    possible_causes:
      - cause: "容器运行时版本不支持 exec"
        indicators: ["no container runtime detected"]
        next_step: "使用 crictl exec 代替 kubectl exec，或升级 containerd 版本"
```

---

## 6. kubectl describe node 输出解析

### 6.1 Conditions 判断节点健康

```yaml
output_pattern:
  - id: "node-001"
    command: "kubectl describe node <node-name>"
    section: "Conditions"
    indicator: |
      Type             Status
      MemoryPressure   True
      PIDPressure      False
      DiskPressure     False
      NetworkUnavailable False
      Ready            True
    diagnosis: "节点内存压力大（MemoryPressure=True），Ready 仍为 True"
    severity: P1
    possible_causes:
      - cause: "节点内存使用率 > 90%，接近 eviction 阈值"
        indicators: ["MemoryPressure=True"]
        next_step: "kubectl top node 确认内存使用率，开始调度时排除此节点（cordon）"
```

```yaml
output_pattern:
  - id: "node-002"
    command: "kubectl describe node <node-name>"
    section: "Conditions"
    indicator: |
      Type             Status
      Ready            False
      MemoryPressure   False
      DiskPressure     True
    diagnosis: "节点 NotReady（Ready=False）且 DiskPressure=True，磁盘空间不足"
    severity: P0
    possible_causes:
      - cause: "节点磁盘使用率 > 85%，kubelet 开始拒绝新 Pod 调度"
        indicators: ["Ready=False", "DiskPressure=True"]
        next_step: "SSH 到节点，清理磁盘空间（删除旧镜像、日志、临时文件）"
```

```yaml
output_pattern:
  - id: "node-003"
    command: "kubectl describe node <node-name>"
    section: "Conditions"
    indicator: |
      Type             Status
      Ready            Unknown
      MemoryPressure   Unknown
      DiskPressure     Unknown
    diagnosis: "节点状态完全未知，Kubelet 未上报状态"
    severity: P0
    possible_causes:
      - cause: "Kubelet 进程崩溃或节点网络中断"
        indicators: ["Ready=Unknown"]
        next_step: "SSH 到节点检查 kubelet 状态: systemctl status kubelet"
```

### 6.2 Allocatable 资源判断

```yaml
output_pattern:
  - id: "node-004"
    command: "kubectl describe node <node-name>"
    section: "Allocatable"
    indicator: |
      cpu:                8
      memory:             16Gi
      pods:               110
      nvidia.com/gpu:     2
    diagnosis: "节点可分配资源清单，含 GPU 资源（nvidia.com/gpu: 2）"
    severity: P0
    note: "Allocatable 中的资源类型决定了 Pod 调度范围，GPU 节点需检查 nvidia.com/gpu 是否正确上报"
```

### 6.3 污点与标签

```yaml
output_pattern:
  - id: "node-005"
    command: "kubectl describe node <node-name>"
    section: "Taints"
    indicator: |
      node.kubernetes.io/not-ready:NoSchedule 2026-05-18T08:00:00Z
    diagnosis: "节点有 not-ready 污点，K8s 自动对新 Pod 停止调度"
    severity: P1
    possible_causes:
      - cause: "节点之前 NotReady，污点未清除"
        indicators: ["not-ready:NoSchedule"]
        next_step: "确认节点恢复后: kubectl taint nodes <node-name> node.kubernetes.io/not-ready-"
```

---

## 7. kubectl get 资源状态解析

### 7.1 Deployment 状态

```yaml
output_pattern:
  - id: "get-001"
    command: "kubectl get deploy -n <namespace>"
    status: |
      NAME     READY   UP-TO-DATE   AVAILABLE   AGE
      nginx    2/3     3            2           10m
    diagnosis: "Deployment 期望 3 个副本但只有 2 个就绪（2/3），存在 1 个 Pod 未就绪"
    severity: P1
    possible_causes:
      - cause: "Pod 调度失败或启动中"
        indicators: ["READY 2/3"]
        next_step: "kubectl get pods -n <namespace> -l app=nginx 查看具体 pod 状态"
      - cause: "ImagePullBackOff / CrashLoopBackOff"
        indicators: ["UP-TO-DATE 3 but AVAILABLE 2"]
        next_step: "kubectl describe deploy 查看 Events"
```

```yaml
output_pattern:
  - id: "get-002"
    command: "kubectl get deploy -n <namespace>"
    status: |
      NAME     READY   UP-TO-DATE   AVAILABLE   AGE
      api      0/2     2            0           5m
    diagnosis: "Deployment 所有 Pod 都未就绪，滚动更新卡住"
    severity: P0
    possible_causes:
      - cause: "所有 Pod 启动失败"
        indicators: ["READY 0/2"]
        next_step: "kubectl get pods -n <namespace> -l app=api 确认 Pod 状态"
```

### 7.2 Service/Endpoints 状态

```yaml
output_pattern:
  - id: "get-003"
    command: "kubectl get svc -n <namespace>"
    status: |
      NAME     TYPE        CLUSTER-IP     PORT(S)   SELECTOR
      nginx     ClusterIP   10.96.0.123    80/TCP    app=nginx
    endpoint_check: |
      kubectl get endpoints -n <namespace> nginx
      NAME    ENDPOINTS                      AGE
      nginx   10.244.1.15:80,10.244.2.23:80  5m
    diagnosis: "Service 有正确的 Endpoints（两个 Pod IP:80）"
    severity: P0
    status: "normal"
    note: "如 Endpoints 为空，则 Service 后端无 Pod"
```

```yaml
output_pattern:
  - id: "get-004"
    command: "kubectl get endpoints -n <namespace> <svc-name>"
    status: |
      NAME     ENDPOINTS   AGE
      nginx    <none>      5m
    diagnosis: "Service 没有 Endpoints，后端 Pod 均未就绪或 selector 不匹配"
    severity: P1
    possible_causes:
      - cause: "Pod 均未就绪（0 Ready）"
        indicators: ["ENDPOINTS <none>"]
        next_step: "kubectl get pods -n <namespace> 查看 Pod 状态"
      - cause: "Service selector 与 Pod labels 不匹配"
        indicators: ["ENDPOINTS <none> + pods running"]
        next_step: "检查 Service selector 和 Pod labels 是否一致"
```

### 7.3 PVC 状态

```yaml
output_pattern:
  - id: "get-005"
    command: "kubectl get pvc -n <namespace>"
    status: |
      NAME      STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
      data-pvc   Pending                          gp2            5d
    diagnosis: "PVC 处于 Pending 状态超过 5 天，存储卷未成功动态供给"
    severity: P1
    possible_causes:
      - cause: "StorageClass 不存在或 Provisioner 异常"
        indicators: ["STATUS Pending"]
        next_step: "kubectl get sc 确认 StorageClass 存在，检查 Provisioner pod"
      - cause: "集群配额不足（storage limit reached）"
        indicators: ["STATUS Pending + no events"]
        next_step: "检查集群的 storage quota"
```

### 7.4 HPA 状态

```yaml
output_pattern:
  - id: "get-006"
    command: "kubectl get hpa -n <namespace>"
    status: |
      NAME     REFERENCE           TARGETS     MINPODS   MAXPODS   REPLICAS
      nginx    Deployment/nginx    85%/80%     2         5         3
    diagnosis: "HPA 检测到 CPU 85%（超过 80% 阈值），正在扩容（当前 3 pods，目标 5）"
    severity: P1
    status: "scaling"
    possible_causes:
      - cause: "正常业务高峰触发扩容"
        indicators: ["TARGETS 85%/80%"]
        next_step: "监控扩容进程 kubectl get hpa -w"
      - cause: "配置了错误的扩缩容指标"
        indicators: ["TARGETS 85% 但业务无明显增长"]
        next_step: "检查 HPA 配置的指标类型和目标值"
```

---

## 8. kubectl describe 各类资源输出解析

### 8.1 Ingress 状态

```yaml
output_pattern:
  - id: "ingress-001"
    command: "kubectl describe ingress -n <namespace>"
    section: "Annotations"
    indicator: |
      kubernetes.io/ingress.class: nginx
      nginx.ingress.kubernetes.io/ssl-redirect: "true"
    status_check: "kubectl get ingress -n <namespace>"
    diagnosis: "Ingress 注解正常，使用 nginx ingress class"
    severity: P0
    note: "如访问返回 503，检查 backend Service/Endpoints 是否存在"
```

### 8.2 CronJob 状态

```yaml
output_pattern:
  - id: "cronjob-001"
    command: "kubectl describe cronjob -n <namespace>"
    section: "Events"
    indicator: |
      Warning   FailedSchedule   CronJob   Missing schedule, will never run.
    diagnosis: "CronJob 配置错误，缺少 schedule 字段"
    severity: P0
    possible_causes:
      - cause: "cronjob.spec.schedule 未设置"
        indicators: ["Missing schedule"]
        next_step: "修正 CronJob YAML，补全 schedule 字段（cron 表达式）"
```

### 8.3 PDB (PodDisruptionBudget) 状态

```yaml
output_pattern:
  - id: "pdb-001"
    command: "kubectl describe pdb -n <namespace>"
    section: "Status"
    indicator: |
      Allowed disruptions: 2
      Current   healthy: 5
      Desired   healthy: 5
      Total:    7
    diagnosis: "PDB 允许 2 个 Pod 中断，当前有 5 个健康的 Pod"
    severity: P0
    status: "normal"
    note: "如 Allowed disruptions 为 0，说明 PDB 保护了所有 Pod，驱逐将被阻止"
```

---

## 附录：命令输出 → 根因 快速索引表

| 症状关键词 | 命令 | 指示器 | 初步诊断 |
|-----------|------|--------|---------|
| `OOMKilled` | `kubectl describe pod` | `Reason: OOMKilled` | 容器内存超限 |
| `CrashLoopBackOff` | `kubectl describe pod` | `Reason: CrashLoopBackOff` | 应用启动失败 |
| `ImagePullBackOff` | `kubectl describe pod` | `Reason: ImagePullBackOff` | 镜像拉取失败 |
| `Evicted` | `kubectl get events` | `Evicted` | Pod 被驱逐 |
| `Terminating` | `kubectl get pod` | `status: Terminating` | Pod 删除卡住 |
| `Pending` | `kubectl get pod` | `status: Pending` | 调度失败 |
| `Unschedulable` | `kubectl describe pod` | `Reason: Unschedulable` | 调度失败（资源不足） |
| `Connection refused` | `kubectl logs` | `connect: connection refused` | 后端服务不可达 |
| `no such host` | `kubectl logs` | `no such host` | DNS 解析失败 |
| `x509` | `kubectl logs` | `x509: certificate` | 证书验证失败 |
| `panic` | `kubectl logs` | `panic:` | Go 应用崩溃 |
| `OutOfMemoryError` | `kubectl logs` | `OutOfMemoryError` | JVM 内存溢出 |
| `NotReady` | `kubectl describe node` | `Ready: False` | 节点不可用 |
| `MemoryPressure` | `kubectl describe node` | `MemoryPressure: True` | 节点内存压力大 |
| `<none>` (endpoints) | `kubectl get endpoints` | `ENDPOINTS <none>` | Service 无后端 |
| `metrics not available` | `kubectl top node` | `metrics not available yet` | metrics-server 异常 |
| `not authorized` | `kubectl exec` | `not authorized` | RBAC 权限不足 |
| `unable to upgrade` | `kubectl exec` | `unable to upgrade connection` | API Server 与 Pod 隧道中断 |

---

## 9. kubectl rollout 输出解析

### 9.1 rollout status —— 滚动更新状态

```yaml
output_pattern:
  - id: "rollout-001"
    command: "kubectl rollout status deployment/nginx -n <namespace>"
    raw_output: |
      Waiting for rollout to finish: 1 out of 3 new replicas have been updated...
      Waiting for rollout to finish: 1 out of 3 new replicas have been updated...
      error: deployment "nginx" exceeded its progress deadline
    diagnosis: "滚动更新卡住，超出 progressDeadlineSeconds（默认 600s）未完成"
    severity: P1
    possible_causes:
      - cause: "新版本 Pod 启动失败（镜像拉取/运行时错误）"
        indicators: ["new replicas have been updated" + 超时"]
        next_step: "kubectl get pods -n <namespace> -l app=nginx --show-labels 查看新 Pod 状态"
      - cause: "maxSurge=0 且有 PDB 保护导致无法推进"
        indicators: ["new replicas have been updated" + 无后续进展"]
        next_step: "kubectl get pdb -n <namespace> 检查 PodDisruptionBudget"
    expected_output: "deployment \"nginx\" successfully rolled out"  # 正常完成输出
```

```yaml
output_pattern:
  - id: "rollout-002"
    command: "kubectl rollout status daemonset/monitoring-agent -n <namespace>"
    raw_output: |
      Waiting for daemon set "monitoring-agent" to roll out: 0 out of 5 new pods created
    diagnosis: "DaemonSet 滚动更新无法创建新 Pod（节点资源不足或污点）"
    severity: P1
    possible_causes:
      - cause: "所有目标节点不可用"
        indicators: ["0 out of 5 new pods created"]
        next_step: "kubectl get nodes -l <nodeSelector> 检查节点可用性"
```

### 9.2 rollout history —— 版本历史

```yaml
output_pattern:
  - id: "rollout-003"
    command: "kubectl rollout history deployment/api -n <namespace>"
    raw_output: |
      deployment.apps/api
      REVISION  CHANGE-CAUSE
      3         kubectl apply --filename=api-deployment.yaml
      2         kubectl apply --filename=api-deployment.yaml
      1         <none>
    diagnosis: "Deployment 有 3 个历史版本，当前版本为 revision 3"
    severity: P0
    status: "normal"
    note: "revision 数量受 spec.revisionHistoryLimit 控制（默认 10）"
```

```yaml
output_pattern:
  - id: "rollout-004"
    command: "kubectl rollout history deployment/api --revision=5"
    raw_output: |
      error: unable to find specified revision 5
    diagnosis: "请求查看不存在的 revision（历史版本已被 prune）"
    severity: P2
    possible_causes:
      - cause: "revisionHistoryLimit 设置过低，历史版本被清理"
        indicators: ["unable to find specified revision"]
        next_step: "增加 revisionHistoryLimit 或确认是否手动执行了 prune"
```

### 9.3 rollout undo —— 回滚

```yaml
output_pattern:
  - id: "rollout-005"
    command: "kubectl rollout undo deployment/api -n <namespace>"
    raw_output: |
      deployment.apps/api rolled back
    diagnosis: "回滚成功（回退到上一个 revision）"
    severity: P0
    status: "normal"
    note: "回滚后可立即通过 rollout history 确认当前 revision 变化"
```

```yaml
output_pattern:
  - id: "rollout-006"
    command: "kubectl rollout undo deployment/api -n <namespace> --to-revision=2"
    raw_output: |
      error: cannot specify revision when undoing to a previous rollout
    diagnosis: "rollback 到指定 revision 时出现错误（K8s 1.28 后 undo 不支持 --to-revision）"
    severity: P2
    possible_causes:
      - cause: "K8s 1.28+ 废弃了 --to-revision 参数"
        indicators: ["cannot specify revision"]
        next_step: "使用 kubectl rollout history 查看各 revision，再通过 apply 或 set image 回退到指定版本"
```

---

## 10. kubectl apply/diff 输出解析

### 10.1 kubectl apply —— YAML 应用错误

```yaml
output_pattern:
  - id: "apply-001"
    command: "kubectl apply -f deployment.yaml"
    error: |
      error: error validating "deployment.yaml": error validating data:
      [ValidationError(Deployment.spec): unknown field 'replics' ...
    diagnosis: "YAML 字段名错误（'replics' 应为 'replicas'）"
    severity: P1
    possible_causes:
      - cause: "手动编写 YAML 时拼写错误"
        indicators: ["unknown field 'replics'"]
        next_step: "使用 kubectl apply --validate=true -f deployment.yaml --dry-run=client 查看具体错误行"
```

```yaml
output_pattern:
  - id: "apply-002"
    command: "kubectl apply -f deployment.yaml"
    error: |
      error: error validating "deployment.yaml":
      [ValidationError(Deployment.spec): missing required field 'selector' ...
    diagnosis: "Deployment 缺少必需的 spec.selector 字段"
    severity: P0
    possible_causes:
      - cause: "YAML 不完整或 selector 字段被注释掉"
        indicators: ["missing required field 'selector'"]
        next_step: "补充 spec.selector.matchLabels 配置"
```

```yaml
output_pattern:
  - id: "apply-003"
    command: "kubectl apply -f pvc.yaml"
    error: |
      error: PersistentVolumeClaim "data-pvc" is forbidden:
      unable to validate splitted objects: Field ... is immutable
    diagnosis: "PVC 字段不可变更（已存在的字段无法修改）"
    severity: P1
    possible_causes:
      - cause: "已存在的 PVC 的 storageClassName 或 accessModes 不可更改"
        indicators: ["is forbidden" + "immutable"]
        next_step: "删除旧 PVC 重新创建（会丢失数据），或使用 patch 只修改可变字段"
```

```yaml
output_pattern:
  - id: "apply-004"
    command: "kubectl apply -f sa.yaml"
    error: |
      error: ServiceAccount "default" is forbidden: User "system:anonymous"
      cannot update resource "serviceaccounts" in this namespace
    diagnosis: "匿名用户无权更新资源（RBAC 禁止）"
    severity: P0
    possible_causes:
      - cause: "当前 kubeconfig 没有正确认证信息"
        indicators: ["system:anonymous" + "cannot update"]
        next_step: "更新 kubeconfig 凭证或切换到有权限的 context"
```

### 10.2 kubectl diff —— 配置差异

```yaml
output_pattern:
  - id: "diff-001"
    command: "kubectl diff -f deployment-update.yaml"
    raw_output: |
      - spec.replicas: 3
      + spec.replicas: 5
      - image: nginx:1.24
      + image: nginx:1.25
    diagnosis: "配置变更预览：副本从 3 增加到 5，镜像从 1.24 升级到 1.25"
    severity: P0
    status: "normal"
    note: "diff 输出中 - 表示当前值（将删除），+ 表示新值（将应用）"
```

```yaml
output_pattern:
  - id: "diff-002"
    command: "kubectl diff -f deployment.yaml"
    raw_output: (无输出)
    diagnosis: "无差异（当前集群配置与 YAML 完全一致）"
    severity: P0
    status: "normal"
    note: "无输出表示 apply 后不会产生任何变更"
```

```yaml
output_pattern:
  - id: "diff-003"
    command: "kubectl diff -f deployment.yaml"
    error: |
      Error: Error from server (NotFound): deployments.apps "nginx" not found
    diagnosis: "资源不存在，无法 diff（--prune 或部分字段 apply 时会遇到）"
    severity: P1
    possible_causes:
      - cause: "diff 针对不存在的资源"
        indicators: ["not found"]
        next_step: "使用 kubectl apply -f 代替 diff（apply 会自动创建）"
```

---

## 11. kubectl get --watch 输出解析

### 11.1 watch 事件类型判断

```yaml
output_pattern:
  - id: "watch-001"
    command: "kubectl get pods -n <namespace> --watch"
    raw_output: |
      NAME      READY   STATUS    AGE
      nginx     1/1     Running   10s
      nginx     1/1     Running   10s, 1/1     Terminating   10s
      nginx     0/1     Terminating   11s
      nginx     0/1     Terminating   12s, 1/1     Running   0s
    diagnosis: "Pod 经历了 Terminating → Running 的重启，容器被重建"
    severity: P1
    possible_causes:
      - cause: "进程崩溃导致容器重启（livenessProbe 失败或 OOM）"
        indicators: ["Terminating" + "Running" 快速切换"]
        next_step: "kubectl describe pod nginx 查看重启原因和 Events"
    expected_output: "Pod 保持 Running 且 READY=1/1 不变"  # 正常状态
```

```yaml
output_pattern:
  - id: "watch-002"
    command: "kubectl get nodes -l <node-name> --watch"
    raw_output: |
      NAME      STATUS   ROLES    AGE
      node-1    Ready    worker   100d
      node-1    NotReady   worker   100d
    diagnosis: "节点从 Ready 变为 NotReady，kubelet 上报心跳中断"
    severity: P0
    possible_causes:
      - cause: "节点上 kubelet 进程崩溃或网络中断"
        indicators: ["NotReady"]
        next_step: "SSH 到节点检查 kubelet 状态: systemctl status kubelet"
    expected_output: "STATUS 始终保持 Ready，不出现 NotReady"  # 正常状态
```

### 11.2 watch 异常事件识别

```yaml
output_pattern:
  - id: "watch-003"
    command: "kubectl get pods -n <namespace> --watch"
    raw_output: |
      NAME      READY   STATUS    AGE
      api       2/3     Running   5m
      api       2/3     Running   5m, 1/3     NotReady   5m
      api       1/3     Running   6m
    diagnosis: "Deployment 有 Pod 从 Ready 变为 NotReady，副本数从 2/3 降到 1/3"
    severity: P1
    possible_causes:
      - cause: "某个 Pod 探针失败导致 Ready 下降"
        indicators: ["NotReady"]
        next_step: "kubectl describe pod api-<pod-id> 查看探针失败原因"
    expected_output: "所有 Pod 的 READY 列保持稳定，不出现突然下降"  # 正常状态
```

```yaml
output_pattern:
  - id: "watch-004"
    command: "kubectl get deployments -n <namespace> --watch"
    raw_output: |
      NAME    READY   UP-TO-DATE   AVAILABLE   AGE
      api     3/3     3            3           10m
      api     2/3     3            2           10m
    diagnosis: "Deployment 副本数从 3 降到 2，UP-TO-DATE 仍为 3 表示新版本 ReplicaSet 已就绪但不可用 Pod 减少"
    severity: P1
    possible_causes:
      - cause: "Pod 被驱逐或重启导致可用数下降"
        indicators: ["AVAILABLE 2"]
        next_step: "kubectl get pods -n <namespace> -l app=api 查看 Pod 状态"
```

---

## 12. kubectl api-resources 输出解析

### 12.1 资源类型速查

```yaml
output_pattern:
  - id: "api-001"
    command: "kubectl api-resources"
    raw_output: |
      NAME                                  SHORTNAMES   APIVERSION                    NAMESPACED   KIND
      pods                                  po           v1                             true         Pod
      services                              svc          v1                             true         Service
      deployments                           deploy       apps/v1                        true         Deployment
      replicasets                           rs           apps/v1                        true         Deployment
      statefulsets                          sts          apps/v1                        true         StatefulSet
      daemonsets                            ds           apps/v1                        true         DaemonSet
      jobs                                  job          batch/v1                       true         Job
      cronjobs                              cj           batch/v1                       true         CronJob
      configmaps                            cm           v1                             true         ConfigMap
      secrets                               sv           v1                             true         Secret
      persistentvolumeclaims                pvc          v1                             true         PersistentVolumeClaim
      persistentvolumes                     pv           v1                             false        PersistentVolume
      nodes                                 no           v1                             false        Node
      namespaces                            ns           v1                             false        Namespace
      clusterroles                          cr           rbac.authorization.k8s.io/v1   false        ClusterRole
      customresourcedefinitions             crd          apiextensions.k8s.io/v1       false        CustomResourceDefinition
    diagnosis: "api-resources 输出展示集群支持的所有资源类型"
    severity: P0
    status: "normal"
    note: "NAMESPACED=true 表示该资源是命名空间作用域，false 表示集群作用域"
```

```yaml
output_pattern:
  - id: "api-002"
    command: "kubectl api-resources --namespaced=true"
    raw_output: |
      # 仅显示命名空间作用域资源（Deployment/Pod/Service 等）
    diagnosis: "过滤后只显示命名空间作用域资源"
    severity: P0
    status: "normal"
```

```yaml
output_pattern:
  - id: "api-003"
    command: "kubectl api-resources -o wide"
    raw_output: |
      NAME      SHORTNAMES   APIVERSION   NAMESPACED   KIND   VERBS
      pods      po           v1           true         Pod    [create delete deletecollection get list patch update watch]
    diagnosis: "输出包含每个资源支持的 verbs（操作方法）"
    severity: P0
    status: "normal"
    note: "如某资源的 verbs 中缺少 get/update，说明当前 RBAC 可能无法执行该操作"
```

---

## 13. kubectl explain 输出解析

### 13.1 字段层级查询

```yaml
output_pattern:
  - id: "explain-001"
    command: "kubectl explain deployment.spec"
    raw_output: |
      RESOURCE: spec <Object>
      DESCRIPTION:
      Specification of the desired behavior of the Deployment.
      ...
      FIELDS:
        replicas     <integer>  --  Number of desired instances ...
        selector     <Object>   --  Label selector ...
        strategy     <Object>   --  The deployment strategy ...
    diagnosis: "字段层级查询成功，spec 下有 replicas/selector/strategy 等字段"
    severity: P0
    status: "normal"
```

```yaml
output_pattern:
  - id: "explain-002"
    command: "kubectl explain deployment.spec.strategy"
    raw_output: |
      RESOURCE: spec.strategy <Object>
      DESCRIPTION:
      The deployment strategy ...
      FIELDS:
        type         <string>  --  Type of deployment ...
        rollingUpdate <Object> --  Rolling update config ...
    diagnosis: "嵌套字段查询成功，可看到 strategy.type 和 strategy.rollingUpdate"
    severity: P0
    status: "normal"
```

```yaml
output_pattern:
  - id: "explain-003"
    command: "kubectl explain deployment.spec.strategy.rollingUpdate"
    raw_output: |
      RESOURCE: spec.strategy.rollingUpdate <Object>
      DESCRIPTION:
      Rolling update config ...
      FIELDS:
        maxSurge       <string>   --  Maximum number of pods that can be scheduled ...
        maxUnavailable  <string>   --  Maximum number of pods that can be unavailable ...
    diagnosis: "最深层级查询，显示 maxSurge 和 maxUnavailable 的类型和说明"
    severity: P0
    status: "normal"
```

---

## 14. kubectl cp 上传/下载失败解析

### 14.1 cp 常见错误

```yaml
output_pattern:
  - id: "cp-001"
    command: "kubectl cp <file> <pod>:/path -n <namespace>"
    error: |
      error: unable to upgrade connection: container not running
    diagnosis: "目标容器未运行（Pod 未进入 Running 状态或容器已终止）"
    severity: P1
    possible_causes:
      - cause: "Pod 还处于 Creating/BackOff 状态"
        indicators: ["container not running"]
        next_step: "等待 Pod 进入 Running 状态后再尝试 cp"
    expected_output: "无错误，文件成功复制到容器内"  # 正常
```

```yaml
output_pattern:
  - id: "cp-002"
    command: "kubectl cp <pod>:/path/to/file ./local -n <namespace>"
    error: |
      error: cp /path/to/file is not a tar archive
    diagnosis: "从容器复制出的文件不是 tar 格式（cp 默认使用 tar 传输）"
    severity: P1
    possible_causes:
      - cause: "直接用 cp 复制二进制文件（应加 -O 选项）"
        indicators: ["not a tar archive"]
        next_step: "使用 kubectl exec <pod> -- tar cf - /path/to/file | tar xf - 解压"
```

```yaml
output_pattern:
  - id: "cp-003"
    command: "kubectl cp config.yaml default/mypod:/etc/config/config.yaml"
    error: |
      Error from server (NotFound): pods "mypod" not found
    diagnosis: "Pod 名称或命名空间错误（使用了简化名而非完整名）"
    severity: P1
    possible_causes:
      - cause: "未指定正确的 namespace 或 pod 全名"
        indicators: ["not found"]
        next_step: "使用完整路径: kubectl cp config.yaml namespace/pod-name:/path"
```

```yaml
output_pattern:
  - id: "cp-004"
    command: "kubectl cp big-file.tar <pod>:/data/ -n <namespace>"
    error: |
      error: write tar://archive: write tcp <ip>: write: broken pipe
    diagnosis: "传输大文件时连接中断（网络不稳定或超时）"
    severity: P1
    possible_causes:
      - cause: "文件过大或网络不稳定"
        indicators: ["broken pipe"]
        next_step: "分割文件后分批传输，或使用持久化存储（PVC）代替 cp"
```

---

## 元数据

```yaml
---
id: CMD-OUTPUT-CORPUS-001
domain: structural-trouble-shooting
type: command-output-interpretation
tags: [kubectl, diagnostics, agent-corpus, k8s-1.28-1.33]
intent_queries:
  - "kubectl describe pod 输出怎么解读"
  - "kubectl logs 怎么判断根因"
  - "kubectl top 资源使用率多少算危险"
  - "kubectl exec 失败怎么排查"
  - "kubectl get events 怎么分类"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - 故障诊断/topic-structural-trouble-shooting/00-configuration-first-methodology.md
  - 故障诊断/FTA故障树/list/pod-fta.md
  - 故障诊断/05-pod-pending-diagnosis.md
---
```

<!-- risk-assessed -->
