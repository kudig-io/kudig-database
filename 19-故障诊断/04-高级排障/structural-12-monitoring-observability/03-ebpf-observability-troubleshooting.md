---
title: eBPF 可观测性故障排查指南
description: '# eBPF 可观测性故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- kubelet
- scheduler
- prometheus
- cilium
- containerd
- daemonset
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- eBPF 可观测性故障排查指南 是什么
- 如何 eBPF 可观测性故障排查指南
- eBPF 可观测性故障排查指南 故障排查
- eBPF 可观测性故障排查指南 排障步骤
trigger_keywords:
- eBPF
- 可观测性故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- ebpf-basics
- cilium-basics
tier: supporting
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# eBPF 可观测性故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Cilium v1.14+ / Pixie v0.14+ | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **eBPF 程序加载状态**：`bpftool prog show` 或 `cilium status`，确认 eBPF 程序已加载到内核。
2. **内核版本兼容性**：`uname -r`，确认内核版本 >= 5.4（部分功能需 >= 5.10）。
3. **BTF 可用性**：`ls /sys/kernel/btf/vmlinux`，确认内核暴露 BTF 信息。
4. **Cilium/Pixie Pod 状态**：`kubectl get pods -n kube-system -l k8s-app=cilium` 或 `olm/px-operator`。
5. **Hubble/Tetragon 可见性**：`hubble status` 或查看 Tetragon Pod 日志中的 `tetragon` 事件。
6. **快速缓解**：
   - eBPF 加载失败：检查内核配置是否启用 `CONFIG_BPF`、`CONFIG_BPF_SYSCALL`。
   - Hubble 无流量数据：确认 Cilium 的 `hubble.listenAddress` 配置和 relay 连接。
   - Tetragon 事件丢失：增大 ringbuf 大小或调整事件过滤条件。
7. **证据留存**：保存 `bpftool` 输出、内核配置 `/boot/config-$(uname -r)`、Cilium/Pixie 组件日志。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 eBPF 程序加载失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| eBPF 验证失败 | `BPF program load failed: permission denied` | Cilium Agent | Agent Pod 日志 |
| 内核不支持 BTF | `BTF is required but not available` | Cilium Agent | Agent Pod 日志 |
| 内核版本过低 | `kernel version is too old` | eBPF Loader | 组件日志 |
| eBPF map 创建失败 | `cannot allocate memory` | eBPF Loader | 组件日志 |
| 缺少内核头文件 | `kernel headers not found` | eBPF Compiler | 编译日志 |

#### 1.1.2 Cilium Hubble 观测异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Hubble Relay 连接失败 | `failed to connect to hubble relay` | Hubble CLI | `hubble status` |
| 流量数据缺失 | `no flows observed` | Hubble UI/CLI | Hubble observe |
| DNS 监控空白 | `no dns flows` | Hubble UI | Hubble observe --protocol dns |
| L7 协议解析失败 | `l7 parser not available` | Cilium Agent | Agent 日志 |
| Hubble UI 空白 | `no data available` | Hubble UI | 浏览器控制台 |

#### 1.1.3 Tetragon 安全追踪异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 事件丢失 | `events were lost` | Tetragon Agent | Agent Pod 日志 |
| 进程执行未捕获 | `execve event missing` | Tetragon User | `tetra getevents` |
| 网络连接未记录 | `connect event not observed` | Tetragon User | `tetra getevents` |
| 策略未触发 | `tracing policy not matched` | Tetragon Agent | Agent 日志 |
| 内核事件缓冲区满 | `perf_event ring buffer full` | Tetragon Agent | Agent 日志 |

#### 1.1.4 Pixie 数据采集异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| PEM 未就绪 | `pem pod not ready` | Pixie Operator | Operator 日志 |
| 脚本执行失败 | `script execution failed` | Pixie API | Vizier 查询日志 |
| 数据表为空 | `table contains no data` | Pixie UI | Pixie Live UI |
| 代理连接断开 | `agent disconnected` | Pixie Cloud | Cloud Connector 日志 |

#### 1.1.5 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **内核升级后 Cilium 无法启动** | 节点 NotReady，Cilium Pod CrashLoopBackOff | 新内核缺少 BTF 或内核配置变化 | 启用 BTF 编译或回滚内核 |
| **大规模集群 Hubble 数据延迟** | Hubble UI 显示的流量延迟 5 分钟以上 | Hubble Relay 单实例瓶颈 | 部署 Hubble Relay HA + 负载均衡 |
| **Tetragon 高 CPU 消耗** | Tetragon Agent CPU 使用率持续 >50% | 追踪策略过于宽泛，捕获了过多事件 | 精细化事件过滤，减少 syscall 捕获 |
| **eBPF map 内存泄漏** | 节点内存持续增长，重启 Cilium 后恢复 | eBPF map 中的连接跟踪条目未清理 | 调小 CT map 超时或重启 Agent |

### 1.2 报错查看方式汇总

```bash
# eBPF 程序状态（需节点 root 权限）
bpftool prog show
bpftool map show
bpftool net list

# Cilium 状态
cilium status
cilium sysdump  # 收集完整诊断信息

# Hubble 状态与观测
hubble status
hubble observe --server hubble-relay.kube-system.svc.cluster.local:80

# Tetragon 状态
tetra status
tetra getevents --processes

# Pixie 状态
px status
px collect-logs

# 内核配置检查
grep -E "CONFIG_BPF|CONFIG_BPF_SYSCALL|CONFIG_DEBUG_INFO_BTF" /boot/config-$(uname -r)

# 内核 BTF
ls -la /sys/kernel/btf/
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

eBPF（extended Berkeley Packet Filter）允许在内核中安全执行沙箱程序。可观测性场景中的 eBPF 架构：

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户空间 (User Space)                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │ Cilium Agent │  │ Hubble Relay│  │ Tetragon    │             │
│  │ Pixie PEM    │  │ CLI/UI      │  │ CLI         │             │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘             │
│         │                │                │                    │
│         │  ringbuf/perf  │    gRPC        │  protobuf          │
│         │  buffer        │                │                    │
├─────────┼────────────────┼────────────────┼────────────────────┤
│         ▼                ▼                ▼                    │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │                    eBPF 虚拟机 (eBPF VM)                   │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │ │
│  │  │ kprobe/trace│  │ sockops     │  │ XDP/TC      │       │ │
│  │  │ point 程序   │  │ socket 程序  │  │ 网络程序     │       │ │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘       │ │
│  │         │                │                │              │ │
│  │         ▼                ▼                ▼              │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │              eBPF Maps (Key-Value Store)             │ │ │
│  │  │  - 连接跟踪表 (CT)                                    │ │ │
│  │  │  - 端点信息 (Endpoints)                               │ │ │
│  │  │  - 策略规则 (Policies)                                │ │ │
│  │  │  - 计数器/直方图 (Metrics)                            │ │ │
│  │  └─────────────────────────────────────────────────────┘ │ │
│  └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                         内核空间 (Kernel Space)                  │
│  Syscalls | Network Stack | Scheduler | File System | ...       │
└─────────────────────────────────────────────────────────────────┘
```

**关键概念**：
- **BTF (BPF Type Format)**：内核类型信息，使 eBPF 程序可移植到不同内核版本
- **Verifier**：内核组件，在加载 eBPF 程序前验证其安全性
- **Ring Buffer / Perf Buffer**：内核向用户空间传递事件的机制
- **CO-RE (Compile Once, Run Everywhere)**：依赖 BTF 的 eBPF 程序可移植技术

### 2.2 排查逻辑决策树

```
eBPF 可观测性问题
    ├── eBPF 程序加载失败
    │   ├── 内核配置不支持？
    │   │   ├── CONFIG_BPF 未启用？──► 启用并重新编译内核
    │   │   ├── CONFIG_DEBUG_INFO_BTF 缺失？──► 启用 BTF 或使用运行时编译
    │   │   └── 内核版本过低？──► 升级到 5.4+（建议 5.10+）
    │   ├── 验证器拒绝？
    │   │   ├── 程序逻辑存在无限循环？──► 修改 eBPF 代码
    │   │   ├── 内存访问越界？──► 修复指针访问
    │   │   └── 内核 API 不兼容？──► 升级 Cilium/Pixie 版本
    │   └── 资源不足？
    │       ├── eBPF map 内存限制？──► 调大内核 `max_locked_memory`
    │       └── eBPF 指令数超限？──► 简化程序逻辑
    ├── 数据采集异常
    │   ├── 无流量/事件数据？
    │   │   ├── eBPF 程序未附加到正确 hook？──► 检查 attach point
    │   │   ├── 事件被过滤器排除？──► 检查过滤配置
    │   │   └── ring buffer 满导致丢事件？──► 增大 buffer 或降低采样率
    │   └── 数据不完整/错误？
    │       ├── 多 CPU 数据竞争？──► 使用 per-CPU map
    │       └── 内核结构体偏移错误？──► 更新 BTF 或使用 CO-RE
    └── 组件性能问题
        ├── 高 CPU 消耗？
        │   ├── 事件触发频率过高？──► 增加过滤条件
        │   └── 用户空间处理慢？──► 优化数据序列化/批处理
        ├── 高内存消耗？
        │   ├── map 条目累积？──► 配置 TTL/超时清理
        │   └── ring buffer 过大？──► 调小 buffer size
        └── 数据延迟高？
            └── 批处理/聚合配置不当？──► 调小批处理窗口
```

### 2.3 详细诊断命令

#### eBPF 基础诊断

```bash
#!/bin/bash
# eBPF 基础诊断脚本（需在节点上以 root 运行）

echo "=== eBPF 基础诊断 ==="

# 1. 内核版本
echo "1. 内核版本: $(uname -r)"

# 2. 内核 eBPF 配置
echo ""
echo "2. 内核 eBPF 配置:"
if [ -f /boot/config-$(uname -r) ]; then
  grep -E "CONFIG_BPF|CONFIG_BPF_SYSCALL|CONFIG_HAVE_EBPF_JIT|CONFIG_BPF_JIT|CONFIG_DEBUG_INFO_BTF" /boot/config-$(uname -r) | \
    sed 's/^/  /'
else
  echo "  未找到内核配置文件"
fi

# 3. BTF 可用性
echo ""
echo "3. BTF 可用性:"
if [ -f /sys/kernel/btf/vmlinux ]; then
  echo "  ✓ /sys/kernel/btf/vmlinux 存在"
  ls -lh /sys/kernel/btf/vmlinux | sed 's/^/  /'
else
  echo "  ✗ BTF 不可用"
fi

# 4. bpftool 检查
echo ""
echo "4. 已加载的 eBPF 程序:"
if command -v bpftool &>/dev/null; then
  bpftool prog show 2>/dev/null | sed 's/^/  /' | head -20
else
  echo "  bpftool 未安装"
fi

# 5. eBPF map 检查
echo ""
echo "5. 已加载的 eBPF Maps:"
if command -v bpftool &>/dev/null; then
  bpftool map show 2>/dev/null | wc -l | xargs -I {} echo "  已加载 {} 个 map"
else
  echo "  bpftool 未安装"
fi

# 6. eBPF JIT 状态
echo ""
echo "6. eBPF JIT 状态:"
cat /proc/sys/net/core/bpf_jit_enable 2>/dev/null | xargs -I {} echo "  bpf_jit_enable={}" || echo "  无法读取"

# 7. eBPF 资源限制
echo ""
echo "7. eBPF 资源限制:"
cat /proc/sys/kernel/bpf_stats_enabled 2>/dev/null | xargs -I {} echo "  bpf_stats_enabled={}" || echo "  无法读取"
ulimit -l 2>/dev/null | xargs -I {} echo "  max locked memory={} KB"
```

#### Cilium Hubble 深度诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Cilium Hubble 深度诊断脚本

echo "=== Cilium Hubble 深度诊断 ==="

# 1. Cilium Agent 状态
echo "1. Cilium Agent 状态:"
cilium status --verbose 2>/dev/null || echo "  cilium CLI 不可用，使用 kubectl:"
kubectl get pods -n kube-system -l k8s-app=cilium -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), ready=\(.status.containerStatuses[0].ready)"
'

# 2. Hubble Relay 状态
echo ""
echo "2. Hubble Relay 状态:"
kubectl get pods -n kube-system -l k8s-app=hubble-relay -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase)"
'

# 3. Hubble 配置检查
echo ""
echo "3. Hubble 配置:"
CILIUM_CONFIG=$(kubectl get configmap cilium-config -n kube-system -o json 2>/dev/null)
echo "  enable-hubble: $(echo $CILIUM_CONFIG | jq -r '.data["enable-hubble"] // "not set"')"
echo "  hubble-listen-address: $(echo $CILIUM_CONFIG | jq -r '.data["hubble-listen-address"] // "not set"')"

# 4. Hubble Relay 日志
echo ""
echo "4. Hubble Relay 错误日志:"
kubectl logs -n kube-system -l k8s-app=hubble-relay --tail=100 2>/dev/null | \
  grep -iE "error|fail|timeout|refused" | tail -10

# 5. Cilium Agent Hubble 日志
echo ""
echo "5. Cilium Agent Hubble 相关日志:"
kubectl logs -n kube-system -l k8s-app=cilium --tail=200 2>/dev/null | \
  grep -iE "hubble.*error|hubble.*fail|observer" | tail -10

# 6. 流数据测试
echo ""
echo "6. Hubble 流观测测试 (10 秒):"
if command -v hubble &>/dev/null; then
  timeout 10 hubble observe --server hubble-relay.kube-system.svc.cluster.local:80 2>/dev/null | wc -l | xargs -I {} echo "  捕获 {} 条流"
else
  echo "  hubble CLI 未安装"
fi
```
#### Tetragon 深度诊断

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Tetragon 深度诊断脚本

echo "=== Tetragon 深度诊断 ==="

# 1. Tetragon Pod 状态
echo "1. Tetragon Pod 状态:"
kubectl get pods -n kube-system -l app.kubernetes.io/name=tetragon -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), restarts=\(.status.containerStatuses[0].restartCount)"
'

# 2. Tetragon Agent 日志
echo ""
echo "2. Tetragon Agent 错误日志:"
kubectl logs -n kube-system -l app.kubernetes.io/name=tetragon -c tetragon --tail=200 2>/dev/null | \
  grep -iE "error|fail|lost|dropped" | tail -10

# 3. TracingPolicy 状态
echo ""
echo "3. TracingPolicy 状态:"
kubectl get tracingpolicies -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.name): sensors=\(.spec? | keys | join(","))"
'

# 4. eBPF map 统计（从 Tetragon Pod 内）
echo ""
echo "4. Tetragon eBPF map 统计:"
TETRAGON_POD=$(kubectl get pods -n kube-system -l app.kubernetes.io/name=tetragon -o jsonpath='{.items[0].metadata.name}')
if [ -n "$TETRAGON_POD" ]; then
  kubectl exec -n kube-system $TETRAGON_POD -c tetragon -- \
    bpftool map show 2>/dev/null | head -10 || echo "  bpftool 在 Tetragon 容器中不可用"
fi

# 5. 事件丢失检查
echo ""
echo "5. 事件丢失检查:"
kubectl logs -n kube-system -l app.kubernetes.io/name=tetragon -c export-stdout --tail=100 2>/dev/null | \
  grep -i "lost" | tail -5 || echo "  未发现事件丢失记录"
```
---

## 3. 解决方案与风险控制

### 3.1 eBPF 基础环境修复

#### 方案一：内核配置启用 BTF

```bash
#!/bin/bash
# 检查并报告内核 eBPF 配置
# 如需修改内核配置，需要重新编译内核或使用支持 BTF 的发行版内核

echo "=== 内核 eBPF 兼容性报告 ==="

REQUIRED_CONFIGS=(
  "CONFIG_BPF=y"
  "CONFIG_BPF_SYSCALL=y"
  "CONFIG_HAVE_EBPF_JIT=y"
  "CONFIG_BPF_JIT=y"
  "CONFIG_BPF_EVENTS=y"
  "CONFIG_DEBUG_INFO=y"
  "CONFIG_DEBUG_INFO_BTF=y"
)

KERNEL_CONFIG="/boot/config-$(uname -r)"
if [ ! -f "$KERNEL_CONFIG" ]; then
  echo "✗ 未找到内核配置文件 $KERNEL_CONFIG"
  exit 1
fi

for cfg in "${REQUIRED_CONFIGS[@]}"; do
  KEY=$(echo $cfg | cut -d'=' -f1)
  EXPECTED=$(echo $cfg | cut -d'=' -f2)
  ACTUAL=$(grep "^$KEY=" $KERNEL_CONFIG 2>/dev/null | cut -d'=' -f2)
  
  if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo "✓ $KEY=$ACTUAL"
  else
    echo "✗ $KEY=$ACTUAL (期望: $EXPECTED)"
  fi
done

echo ""
echo "建议:"
echo "  对于 Ubuntu/Debian: 安装 linux-image-<version>-dbgsym 获取 BTF"
echo "  对于 RHEL/CentOS: 确保 kernel-debuginfo 包已安装"
echo "  对于容器优化 OS: 确认 OS 镜像已启用 BTF"
```

#### 方案二：Cilium 无 BTF 模式（降级方案）

```yaml
# Cilium 在无 BTF 环境下的配置（性能会降低）
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  # 禁用 BTF 依赖的功能
  bpf-map-dynamic-size-ratio: "0.0025"
  # 使用编译时头文件而非 BTF
  bpf-root: "/var/lib/cilium/bpf"
  # 如内核不支持 BTF，Cilium 会尝试运行时编译
  install-iptables-rules: "true"
  # 禁用需要 BTF 的高级功能
  hubble-enable: "true"
  hubble-listen-address: ":4244"
  # 注意：部分功能（如 sockops L7 解析）需要 BTF
```

### 3.2 Cilium Hubble 优化配置

```yaml
# Hubble Relay HA 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: hubble-relay
  namespace: kube-system
spec:
  replicas: 2
  template:
    spec:
      containers:
      - name: hubble-relay
        image: quay.io/cilium/hubble-relay:v1.15.0
        command:
        - hubble-relay
        args:
        - serve
        - --listen-address=tcp://0.0.0.0:4245
        - --dial-timeout=5s
        - --retry-timeout=30s
        - --sort-buffer-len-max=10000
        - --sort-buffer-drain-timeout=1s
        resources:
          limits:
            cpu: "2"
            memory: "1Gi"
          requests:
            cpu: "500m"
            memory: "256Mi"
        livenessProbe:
          tcpSocket:
            port: grpc
          initialDelaySeconds: 5
          periodSeconds: 5
        readinessProbe:
          tcpSocket:
            port: grpc
          initialDelaySeconds: 5
          periodSeconds: 5
---
# Hubble UI Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: hubble-ui
  namespace: kube-system
  annotations:
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
spec:
  rules:
  - host: hubble.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: hubble-ui
            port:
              number: 80
```

### 3.3 Tetragon 性能优化

```yaml
# Tetragon 高性能且低资源消耗配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: tetragon-config
  namespace: kube-system
data:
  # 启用进程缓存减少重复事件
  enable-process-cred: "false"
  enable-process-ns: "false"
  
  # 事件过滤配置
  export-filename: /var/run/cilium/tetragon/tetragon.log
  export-allowlist: |
    {"namespace":["default","production"],"event_set":["PROCESS_EXEC","PROCESS_EXIT","TCP_CONNECT"]}
  export-denylist: |
    {"binary_regex":["^/usr/bin/(ls|cat|grep)$"]}
  
  # Ring buffer 配置
  rb-size: "65535"
  rb-size-total: "262144"
  
  # 进程缓存大小
  process-cache-size: "65536"
  data-cache-size: "1024"
---
# Tetragon TracingPolicy 示例（精细化监控）
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: monitor-suspicious-executions
spec:
  kprobes:
  - call: "__x64_sys_execve"
    syscall: true
    args:
    - index: 0
      type: "string"
    selectors:
    - matchArgs:
      - index: 0
        operator: "Prefix"
        values:
        - "/tmp/"
        - "/dev/shm/"
      matchActions:
      - action: "Post"
```

### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 升级内核以支持 BTF | ⭐⭐⭐ 高 | 可能影响节点稳定性 | 使用备用内核启动 |
| 部署/更新 Cilium eBPF 程序 | ⭐⭐ 中 | 网络策略短暂失效 | 回滚 Cilium DaemonSet |
| 修改 Tetragon TracingPolicy | ⭐ 低 | 影响事件采集范围 | 删除或恢复原始策略 |
| 调整 eBPF map 大小 | ⭐⭐ 中 | 可能导致 Agent 重启 | 恢复原始 ConfigMap |
| 启用/禁用 Hubble | ⭐ 低 | 影响可观测性数据 | 恢复原始 cilium-config |
| 修改 ring buffer 大小 | ⭐ 低 | 影响事件吞吐和内存 | 恢复原始配置 |

### 3.5 验证与监控

#### eBPF 健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# eBPF 可观测性健康检查脚本

echo "=== eBPF 可观测性健康检查 ==="

# 1. Cilium 组件健康
echo "1. Cilium 组件:"
for comp in cilium hubble-relay hubble-ui; do
  READY=$(kubectl get pods -n kube-system -l k8s-app=$comp -o json 2>/dev/null | jq '[.items[].status.containerStatuses[]?.ready] | all')
  if [ "$READY" = "true" ]; then
    echo "  ✓ $comp"
  else
    echo "  ✗ $comp 有 Pod 未就绪"
  fi
done

# 2. Tetragon 健康
echo ""
echo "2. Tetragon 组件:"
kubectl get pods -n kube-system -l app.kubernetes.io/name=tetragon -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase)"
'

# 3. eBPF 程序数量检查
echo ""
echo "3. 各节点 eBPF 程序数量:"
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  COUNT=$(kubectl debug node/$node -it --image=busybox -- chroot /host bpftool prog show 2>/dev/null | wc -l || echo "0")
  echo "  $node: $COUNT 个 eBPF 程序"
done

# 4. 事件流测试
echo ""
echo "4. Hubble 事件流测试 (5 秒):"
if command -v hubble &>/dev/null; then
  timeout 5 hubble observe 2>/dev/null | wc -l | xargs -I {} echo "  捕获 {} 条流"
else
  echo "  hubble CLI 未安装"
fi
```
#### Prometheus 监控告警

```yaml
# eBPF 可观测性监控告警
groups:
- name: ebpf-observability
  rules:
  - alert: CiliumAgentNotReady
    expr: |
      cilium_agent_health_status{status="unknown"} == 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Cilium Agent 未就绪"
      description: "节点 {{ $labels.node }} 上的 Cilium Agent 健康状态未知"

  - alert: HubbleRelayDown
    expr: |
      up{job="hubble-relay"} == 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Hubble Relay 不可用"
      description: "Hubble Relay 已宕机"

  - alert: TetragonEventsLost
    expr: |
      rate(tetragon_errors_total{type="event_lost"}[5m]) > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Tetragon 事件丢失"
      description: "Tetragon 正在丢失事件"

  - alert: CiliumBPFMapPressure
    expr: |
      cilium_bpf_map_pressure > 0.9
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Cilium BPF Map 压力过高"
      description: "BPF Map {{ $labels.map_name }} 压力超过 90%"

  - alert: EBPFProgramLoadFailure
    expr: |
      increase(cilium_bpf_syscall_duration_seconds_count{outcome="fail"}[5m]) > 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "eBPF 程序加载失败"
      description: "Cilium 加载 eBPF 程序失败"
```

### 3.6 最佳实践

1. **内核版本统一**：集群内所有节点使用相同内核版本，避免 eBPF 程序兼容性问题
2. **BTF 优先**：部署前确认节点内核支持 BTF，如不支持优先升级内核而非使用运行时编译
3. **事件过滤**：Tetragon 策略应尽量精确，避免捕获过多无关事件消耗 CPU
4. **资源预留**：为 Cilium Agent 预留足够的 CPU（建议 500m-1000m）和内存（建议 512Mi-1Gi）
5. **Ring Buffer 调优**：高事件率场景增大 ring buffer，低事件率场景减小以节省内存
6. **Map 大小监控**：监控 Cilium BPF map 的压力指标，接近满时及时扩容或清理
7. **安全基线**：使用 Tetragon 建立进程执行基线，通过 TracingPolicy 检测异常执行

### 典型问题案例

#### 案例一：内核升级后 Cilium 无法启动

**问题描述**：Ubuntu 节点从 5.4 升级到 5.15 后，Cilium Pod 进入 CrashLoopBackOff。

**根本原因**：新内核编译时未启用 `CONFIG_DEBUG_INFO_BTF`，Cilium 的 CO-RE eBPF 程序无法加载。

**解决方案**：
1. 安装 `linux-image-$(uname -r)-dbgsym` 包提供 BTF 信息
2. 或重新编译内核启用 `CONFIG_DEBUG_INFO_BTF`
3. 或降级回支持 BTF 的内核版本

#### 案例二：Hubble 观测不到跨节点流量

**问题描述**：Hubble UI 只能看到本节点 Pod 的流量，跨节点流量完全缺失。

**根本原因**：Cilium 的 `cluster-pool` IPAM 模式下，跨节点隧道（VXLAN）流量未被 Hubble observer 捕获。

**解决方案**：
1. 确认 Cilium 配置中 `enable-hubble: "true"` 且 `hubble-listen-address: ":4244"`
2. 检查 Hubble Relay 是否连接到所有节点的 Hubble observer
3. 在 Cilium ConfigMap 中启用 `hubble-event-buffer-capacity: "65535"`

#### 案例三：Tetragon 高 CPU 导致节点响应缓慢

**问题描述**：部署 Tetragon 后，节点 `sys` CPU 使用率持续 >30%。

**根本原因**：TracingPolicy 捕获了所有进程的 `execve` 和 `exit`，在容器高密度节点上事件率过高。

**解决方案**：
1. 在 TracingPolicy 中添加 namespace 过滤，仅监控生产 namespace
2. 排除系统二进制文件（如 `/usr/bin/kubelet`、`/usr/bin/containerd`）
3. 将 `rb-size` 调大并启用事件批处理

## Related

- [[21-生态参考/03-领域索引/observability-index|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
