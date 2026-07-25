---
title: JDK Flight Recorder on K8s
description: 'JFR 基础、K8s 环境下 JFR 触发、持续性 JFR、JFR 与 async-profiler 结合、JFR 可视化及生产环境低开销采集策略'
summary: 'JFR 基础、K8s 环境下 JFR 触发、持续性 JFR、JFR 与 async-profiler 结合、JFR 可视化及生产环境低开销采集策略'
category: troubleshooting-diagnostics
tags:
- jfr
- jdk-flight-recorder
- jvm-profiling
- async-profiler
- performance
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- JDK Flight Recorder 是什么
- 如何在 K8s 中使用 JFR
- JFR 持续性采集如何配置
trigger_keywords:
- JFR
- JDK Flight Recorder
- jcmd
- StartFlightRecording
- async-profiler
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# JDK Flight Recorder on K8s

## 概述

JDK Flight Recorder (JFR) 是 JVM 内置的低开销性能分析工具，通过事件驱动模型采集 CPU、内存、I/O、线程、GC 等运行时数据。在 Kubernetes 环境中，JFR 是 Java 微服务性能诊断的首选工具——无需重启 JVM、开销 < 1%、可直接在容器内触发。

```
JFR 与其他工具对比:

工具                开销     精度     持续采集    K8s 适配
─────────────────────────────────────────────────────────
JFR                 < 1%     高       支持        原生
async-profiler       1-3%    高       支持        需注入
Arthas               2-5%    中       有限        需注入
JVisualVM            5-10%   中       支持        需端口转发
YourKit              3-8%    高       支持        需 License
```

## 1. JFR 基础

### 1.1 核心概念

```
JFR 数据模型:

Recording (录制)
  ├── Event (事件)
  │     ├── jdk.GCHeapSummary          # GC 堆摘要
  │     ├── jdk.CPULoad                # CPU 负载
  │     ├── jdk.ThreadSleep            # 线程睡眠
  │     ├── jdk.SocketRead             # Socket 读取
  │     ├── jdk.FileRead               # 文件读取
  │     ├── jdk.JavaMonitorEnter       # 锁竞争
  │     ├── jdk.Exception              # 异常
  │     ├── jdk.ExecuteVMOperation     # VM 操作
  │     └── jdk.Compilation            # JIT 编译
  ├── Setting (配置)
  │     ├── Event Threshold            # 事件阈值
  │     ├── Stack Trace                # 是否采集栈
  │     └── Period                     # 采集周期
  └── Metadata (元数据)
        ├── JVM 版本
        ├── 启动时间
        └── 采集时间范围
```

### 1.2 内置 Profile

| Profile | 事件数 | 开销 | 适用场景 |
|---------|--------|------|---------|
| `default` | ~50 | < 0.5% | 日常监控 |
| `profile` | ~120 | 1-2% | 深度分析 |

```bash
# 查看 profile 包含的事件
jcmd <pid> JFR.configure
jcmd <pid> VM.flags | grep StartFlightRecording
```

## 2. K8s 环境下 JFR 触发

### 2.1 通过 jcmd 触发

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方法 1: kubectl exec + jcmd（最常用）
# 获取目标 Pod 的 Java 进程 PID
kubectl exec -it pod/my-java-app -c my-java-app -- jps -l

# 触发 60 秒的 JFR 录制
kubectl exec -it pod/my-java-app -c my-java-app -- \
  jcmd 1 JFR.start \
    name=diagnostic \
    duration=60s \
    filename=/tmp/recording.jfr \
    settings=profile

# 等待录制完成
sleep 65

# 拷贝 JFR 文件到本地
kubectl cp my-namespace/my-java-app:/tmp/recording.jfr ./recording.jfr

# 方法 2: 使用 jcmd 命令行参数
kubectl exec -it pod/my-java-app -c my-java-app -- \
  jcmd 1 JFR.start \
    name=quick-diag \
    duration=30s \
    filename=/tmp/quick.jfr \
    settings=default \
    gc=high \
    method-profiling=high
```
### 2.2 通过 K8s Job 触发

```yaml
# 使用 Job 对目标 Pod 触发 JFR（避免 exec 权限问题）
apiVersion: batch/v1
kind: Job
metadata:
  name: jfr-trigger
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: jfr-trigger-sa
      containers:
      - name: jfr
        image: eclipse-temurin:21-jdk
        command:
        - /bin/bash
        - -c
        - |
          # 获取目标 Pod IP
          TARGET_IP=$(kubectl get pod my-java-app -o jsonpath='{.status.podIP}')
          
          # 通过 JMX 触发 JFR（需要 JMX 端口开放）
          jcmd $TARGET_IP:9091 JFR.start \
            name=job-triggered \
            duration=120s \
            filename=/tmp/job-recording.jfr \
            settings=profile
        volumeMounts:
        - name: output
          mountPath: /tmp
      volumes:
      - name: output
        emptyDir: {}
      restartPolicy: Never
```

### 2.3 通过 JMX Remote 触发

```yaml
# 在 Deployment 中启用 JMX 远程访问
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-java-app
spec:
  template:
    spec:
      containers:
      - name: my-java-app
        image: my-java-app:latest
        env:
        - name: JAVA_OPTS
          value: >-
            -Dcom.sun.management.jmxremote
            -Dcom.sun.management.jmxremote.port=9091
            -Dcom.sun.management.jmxremote.rmi.port=9091
            -Dcom.sun.management.jmxremote.authenticate=false
            -Dcom.sun.management.jmxremote.ssl=false
            -Dcom.sun.management.jmxremote.local.only=false
            -Djava.rmi.server.hostname=$(POD_IP)
        - name: POD_IP
          valueFrom:
            fieldRef:
              fieldPath: status.podIP
        ports:
        - containerPort: 9091
          name: jmx
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 从外部通过 JMX 触发 JFR
# 端口转发
kubectl port-forward pod/my-java-app 9091:9091

# 使用 jcmd 连接
jcmd localhost:9091 JFR.start \
  name=remote-recording \
  duration=60s \
  filename=/tmp/remote.jfr \
  settings=profile
```
## 3. 持续性 JFR

### 3.1 JVM 启动参数配置

```yaml
# 通过 Deployment 环境变量启用持续性 JFR
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-java-app
spec:
  template:
    spec:
      containers:
      - name: my-java-app
        image: my-java-app:latest
        env:
        - name: JAVA_OPTS
          value: >-
            -XX:StartFlightRecording=name=continuous,settings=default,disk=true,maxage=6h,maxsize=100MB
            -XX:FlightRecorderOptions=repository=/tmp/jfr-repository,maxage=6h,maxsize=500MB
        volumeMounts:
        - name: jfr-data
          mountPath: /tmp/jfr-repository
      volumes:
      - name: jfr-data
        emptyDir:
          sizeLimit: 1Gi
```

### 3.2 持续性 JFR 参数详解

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# -XX:StartFlightRecording 参数:
#   name=<name>           录制名称
#   settings=<profile>    使用的 profile（default/profile/自定义）
#   disk=<bool>           是否写入磁盘（默认 true）
#   maxage=<duration>     保留最大时长（如 6h, 1d）
#   maxsize=<size>        保留最大大小（如 100MB, 1GB）
#   dumponexit=<bool>     JVM 退出时是否 dump（默认 false）
#   filename=<path>       直接写入指定文件

# -XX:FlightRecorderOptions 参数:
#   repository=<path>     JFR 临时文件目录
#   maxage=<duration>     仓库文件保留时长
#   maxsize=<size>        仓库最大大小
#   dumponexit=<bool>     退出时是否 dump
#   globalbuffersize=<n>  全局缓冲区大小
#   numglobalbuffers=<n>  全局缓冲区数量
#   old-object-queue-size=<n>  老年代对象队列大小

# 动态调整持续性 JFR（无需重启）
kubectl exec -it pod/my-java-app -- \
  jcmd 1 JFR.configure \
    repository=/tmp/jfr-repository \
    maxage=12h \
    maxsize=1GB
```
### 3.3 从持续性 JFR 中提取录制

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 当需要分析时，从持续性录制中提取一个时间窗口
kubectl exec -it pod/my-java-app -- \
  jcmd 1 JFR.start \
    name=extract \
    settings=profile \
    duration=60s \
    filename=/tmp/extract.jfr \
    maxage=1h \
    maxsize=50MB

# 或者 dump 当前录制
kubectl exec -it pod/my-java-app -- \
  jcmd 1 JFR.dump \
    name=continuous \
    filename=/tmp/dump.jfr

# 拷贝到本地
kubectl cp my-namespace/my-java-app:/tmp/extract.jfr ./extract.jfr
```
## 4. JFR 与 async-profiler 结合

### 4.1 为什么需要结合

```
JFR 的优势:
  ✅ JVM 内置，无需额外安装
  ✅ 开销极低（< 1%）
  ✅ 丰富的事件类型（GC/IO/锁/异常/编译）
  ✅ 持续性采集原生支持

JFR 的局限:
  ❌ CPU 火焰图精度不如 async-profiler
  ❌ 无法采集 Native 栈帧
  ❌ Wall-clock 分析能力有限

async-profiler 的优势:
  ✅ CPU 火焰图精度极高（基于 perf_events）
  ✅ 支持 Wall-clock/alloc/lock 分析
  ✅ 可采集 Native/C++ 栈帧
  ✅ 支持 eBPF 模式（无需 perf_events 权限）

结合策略:
  JFR 负责: 持续性监控、事件采集、异常/IO/GC 分析
  async-profiler 负责: 深度 CPU 分析、Wall-clock 分析、内存分配热点
```

### 4.2 在 K8s 中同时使用

```yaml
# 在 Pod 中同时启用 JFR 和 async-profiler
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-java-app
spec:
  template:
    spec:
      containers:
      - name: my-java-app
        image: my-java-app:latest
        env:
        - name: JAVA_OPTS
          value: >-
            -XX:StartFlightRecording=name=continuous,settings=default,disk=true,maxage=6h
            -XX:FlightRecorderOptions=repository=/tmp/jfr-repository
            -agentpath:/opt/async-profiler/lib/libasyncProfiler.so=start,event=cpu,interval=10ms,file=/tmp/profile.html
        securityContext:
          # async-profiler 需要 SYS_PTRACE 权限（或使用 eBPF 模式）
          capabilities:
            add: ["SYS_PTRACE"]
        volumeMounts:
        - name: async-profiler
          mountPath: /opt/async-profiler
      initContainers:
      - name: setup-profiler
        image: async-profiler/async-profiler:latest
        command: ['cp', '-r', '/opt/async-profiler', '/shared/']
        volumeMounts:
        - name: async-profiler
          mountPath: /shared/async-profiler
      volumes:
      - name: async-profiler
        emptyDir: {}
```

### 4.3 联合分析工作流

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 通过 JFR 获取全局视图
kubectl exec -it pod/my-java-app -- \
  jcmd 1 JFR.start name=overview duration=300s filename=/tmp/overview.jfr settings=profile

# Step 2: 通过 async-profiler 获取精确的 CPU 火焰图
kubectl exec -it pod/my-java-app -- \
  bash -c "asprof -d 30 -f /tmp/cpu-flamegraph.html -o html 1"

# Step 3: 通过 async-profiler 获取内存分配热点
kubectl exec -it pod/my-java-app -- \
  bash -c "asprof -d 30 -e alloc -f /tmp/alloc-flamegraph.html -o html 1"

# Step 4: 通过 async-profiler 获取锁竞争热点
kubectl exec -it pod/my-java-app -- \
  bash -c "asprof -d 30 -e lock -f /tmp/lock-flamegraph.html -o html 1"

# Step 5: 拷贝所有结果到本地
kubectl cp my-namespace/my-java-app:/tmp/overview.jfr ./overview.jfr
kubectl cp my-namespace/my-java-app:/tmp/cpu-flamegraph.html ./cpu-flamegraph.html
kubectl cp my-namespace/my-java-app:/tmp/alloc-flamegraph.html ./alloc-flamegraph.html
kubectl cp my-namespace/my-java-app:/tmp/lock-flamegraph.html ./lock-flamegraph.html
```
## 5. JFR 可视化

### 5.1 JDK Mission Control (JMC)

```bash
# 安装 JDK Mission Control
# macOS
brew install --cask mission-control

# Linux
# 下载: https://www.oracle.com/java/technologies/jdk-mission-control.html

# 启动 JMC
jmc

# 在 JMC 中打开 JFR 文件:
# File → Open File → 选择 .jfr 文件

# JMC 主要视图:
# - Overview: 概览（CPU/内存/线程/GC）
# - Code: 热点方法（Hot Methods）
# - Memory: 内存分配和 GC 分析
# - Threads: 线程状态和锁竞争
# - I/O: 文件和 Socket I/O
# - System: 系统环境信息
```

### 5.2 Flame Graph 生成

```bash
# 方法 1: 使用 JMC 内置的火焰图
# 在 JMC 中打开 JFR → 选择 "Method Profiling" → 右键 "Open Flame View"

# 方法 2: 使用 jfr-flame-graph 工具
# 安装
go install github.com/chrishantha/jfr-flame-graph@latest

# 生成火焰图
jfr-flame-graph -f recording.jfr -o flamegraph.html

# 方法 3: 使用 jfr2flame 工具（Python）
pip install jfr2flame
jfr2flame recording.jfr --output flamegraph.html

# 方法 4: 使用 async-profiler 的 jfr2flame
# 将 JFR 栈数据转换为火焰图
java -jar jfr-converter.jar recording.jfr > stacks.txt
flamegraph.pl --title="JFR CPU Flame Graph" stacks.txt > flamegraph.svg
```

### 5.3 命令行分析

```bash
# 使用 jfr 命令行工具（JDK 12+）快速分析
# 打印所有事件摘要
jfr summary recording.jfr

# 打印特定类型的事件
jfr print --events jdk.CPULoad recording.jfr
jfr print --events jdk.GCHeapSummary recording.jfr
jfr print --events jdk.JavaMonitorEnter recording.jfr

# 按时间范围过滤
jfr print --startTime "2026-07-02T10:00:00" --endTime "2026-07-02T10:05:00" recording.jfr

# 导出为 JSON（便于脚本处理）
jfr to-json recording.jfr > recording.json

# 导出为 CSV
jfr to-csv recording.jfr > recording.csv
```

## 6. 生产环境低开销采集策略

### 6.1 分层采集架构

```
生产环境 JFR 分层策略:

Layer 1: 持续性采集（始终开启）
  -XX:StartFlightRecording=name=continuous,settings=default,disk=true,maxage=6h,maxsize=100MB
  开销: < 0.5%
  用途: 异常回溯、GC 趋势、线程死锁检测

Layer 2: 触发式采集（按需开启）
  jcmd 1 JFR.start name=diagnostic duration=60s settings=profile
  开销: 1-2%
  用途: 性能热点分析、方法级 profiling

Layer 3: 深度采集（紧急情况）
  jcmd 1 JFR.start name=deep duration=30s settings=profile gc=high allocation=high
  开销: 2-5%
  用途: 内存泄漏、GC 风暴、锁竞争根因分析
```

### 6.2 自定义 JFR Event 配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建自定义 JFR 配置文件（.jfc）
# custom-profile.jfc 示例:
cat > /tmp/custom-profile.jfc << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<configuration version="2.0" label="Custom Profile" description="Low-overhead custom profile">
  
  <!-- GC 事件（低频采集）-->
  <event name="jdk.GCHeapSummary">
    <setting name="enabled">true</setting>
    <setting name="period">10000 ms</setting>
  </event>
  
  <!-- CPU 负载（每 5 秒）-->
  <event name="jdk.CPULoad">
    <setting name="enabled">true</setting>
    <setting name="period">5000 ms</setting>
  </event>
  
  <!-- 方法采样（低频，降低开销）-->
  <event name="jdk.ExecutionSample">
    <setting name="enabled">true</setting>
    <setting name="period">50 ms</setting>
  </event>
  
  <!-- 锁竞争（仅采集 > 10ms）-->
  <event name="jdk.JavaMonitorEnter">
    <setting name="enabled">true</setting>
    <setting name="threshold">10 ms</setting>
    <setting name="stackTrace">true</setting>
  </event>
  
  <!-- 异常（仅采集未捕获异常）-->
  <event name="jdk.JavaExceptionThrow">
    <setting name="enabled">true</setting>
    <setting name="stackTrace">true</setting>
  </event>
  
  <!-- 线程阻塞（仅采集 > 20ms）-->
  <event name="jdk.ThreadSleep">
    <setting name="enabled">true</setting>
    <setting name="threshold">20 ms</setting>
  </event>
  
  <!-- I/O 事件（仅采集 > 100ms）-->
  <event name="jdk.SocketRead">
    <setting name="enabled">true</setting>
    <setting name="threshold">100 ms</setting>
  </event>
  
  <event name="jdk.FileRead">
    <setting name="enabled">true</setting>
    <setting name="threshold">100 ms</setting>
  </event>
</configuration>
EOF

# 使用自定义配置
kubectl cp /tmp/custom-profile.jfr my-namespace/my-java-app:/tmp/custom-profile.jfc
kubectl exec -it pod/my-java-app -- \
  jcmd 1 JFR.start \
    name=custom \
    duration=300s \
    filename=/tmp/custom.jfr \
    settings=/tmp/custom-profile.jfc
```
### 6.3 JFR 采集最佳实践

```
生产环境 JFR 最佳实践:

1. 始终开启持续性 JFR
   - 使用 default profile，开销 < 0.5%
   - 保留最近 6 小时数据
   - 限制存储空间 < 500MB

2. 诊断时使用 profile profile
   - 默认 60 秒采集
   - 关注热点方法和锁竞争
   - 结合 async-profiler 火焰图

3. 避免以下操作:
   - ❌ 在生产环境使用 gc=high 长时间采集
   - ❌ 将 JFR 文件写入慢速存储（NFS/云盘）
   - ❌ 同时运行多个 JFR 录制
   - ❌ 在 GC 风暴期间启动新的 JFR 录制

4. 容器资源限制:
   - JFR 内存开销: ~50MB（全局缓冲区）
   - JFR 磁盘开销: 默认 < 500MB
   - 确保容器 limits 包含 JFR 开销

5. 安全注意事项:
   - JFR 文件可能包含敏感数据（环境变量、系统属性）
   - 不要将 JFR 文件上传到公共平台
   - 使用 jfr 命令行过滤敏感事件
```

---

## Related

- [[19-故障诊断/05-JVM调优/99-jvm-gc-container-tuning-guide|JVM GC 容器调优]]
- [[19-故障诊断/05-JVM调优/99-java-performance-resource-sizing-guide|Java 性能资源配比]]
- [[17-系统基础/05-速查卡/perf-bpftrace-cheat-sheet|perf/bpftrace 速查卡]]

## See Also

- [Oracle JFR Documentation](https://docs.oracle.com/en/java/javase/21/jfapi/)
- [async-profiler GitHub](https://github.com/async-profiler/async-profiler)
- [JDK Mission Control](https://www.oracle.com/java/technologies/jdk-mission-control.html)


<!-- risk-assessed -->
