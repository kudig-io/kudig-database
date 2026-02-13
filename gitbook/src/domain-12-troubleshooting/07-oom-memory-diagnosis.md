# 07 - OOM和内存问题诊断

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes.io/docs/concepts/configuration/manage-resources-containers](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)

## Kubernetes内存管理架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Kubernetes 内存管理与OOM处理架构                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                         内存层次结构                                 │    │
│  │                                                                      │    │
│  │   节点总内存 (Node Allocatable)                                      │    │
│  │   ┌──────────────────────────────────────────────────────────────┐  │    │
│  │   │                                                               │  │    │
│  │   │  System Reserved    Kube Reserved    Eviction     Allocatable│  │    │
│  │   │  (系统进程)         (K8s组件)       Threshold     (Pod可用)   │  │    │
│  │   │  ┌─────────┐       ┌─────────┐     ┌───────┐    ┌──────────┐│  │    │
│  │   │  │ ~1-2GB  │       │ ~1-2GB  │     │ 100Mi │    │  剩余    ││  │    │
│  │   │  │ sshd    │       │ kubelet │     │ 硬阈值 │    │  容量    ││  │    │
│  │   │  │ systemd │       │ runtime │     │       │    │          ││  │    │
│  │   │  └─────────┘       └─────────┘     └───────┘    └──────────┘│  │    │
│  │   │                                                               │  │    │
│  │   └──────────────────────────────────────────────────────────────┘  │    │
│  └────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                      OOM触发层次                                     │    │
│  │                                                                      │    │
│  │   Layer 1: kubelet驱逐                                               │    │
│  │   ┌──────────────────────────────────────────────────────────────┐  │    │
│  │   │ • 基于evictionSoft/evictionHard阈值                          │  │    │
│  │   │ • 驱逐顺序: BestEffort → Burstable(超用) → Guaranteed        │  │    │
│  │   │ • 优雅终止,有grace period                                    │  │    │
│  │   └──────────────────────────────────────────────────────────────┘  │    │
│  │                              ↓ 驱逐不及时                            │    │
│  │   Layer 2: cgroup OOM Killer                                        │    │
│  │   ┌──────────────────────────────────────────────────────────────┐  │    │
│  │   │ • 容器超过memory.limit_in_bytes                              │  │    │
│  │   │ • 直接kill容器主进程                                          │  │    │
│  │   │ • exitCode=137 (128+9=SIGKILL)                               │  │    │
│  │   │ • reason=OOMKilled                                            │  │    │
│  │   └──────────────────────────────────────────────────────────────┘  │    │
│  │                              ↓ cgroup未限制/系统内存耗尽            │    │
│  │   Layer 3: 系统OOM Killer                                           │    │
│  │   ┌──────────────────────────────────────────────────────────────┐  │    │
│  │   │ • 内核级OOM Killer                                            │  │    │
│  │   │ • 基于oom_score选择进程                                       │  │    │
│  │   │ • 可能kill任意进程(包括kubelet)                              │  │    │
│  │   │ • 日志: dmesg | grep oom                                      │  │    │
│  │   └──────────────────────────────────────────────────────────────┘  │    │
│  └────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                      cgroup v2 内存控制                              │    │
│  │                                                                      │    │
│  │   /sys/fs/cgroup/kubepods.slice/                                    │    │
│  │   ├── kubepods-burstable.slice/                                     │    │
│  │   │   └── kubepods-burstable-pod<uid>.slice/                        │    │
│  │   │       └── cri-containerd-<container-id>.scope/                  │    │
│  │   │           ├── memory.max          # 硬限制(OOM触发点)          │    │
│  │   │           ├── memory.high         # 软限制(触发回收)           │    │
│  │   │           ├── memory.current      # 当前使用量                  │    │
│  │   │           ├── memory.swap.max     # swap限制                    │    │
│  │   │           └── memory.events       # OOM事件计数                 │    │
│  │   ├── kubepods-besteffort.slice/                                    │    │
│  │   └── kubepods-guaranteed.slice/                                    │    │
│  └────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

## OOM类型详解

| 类型 | 触发者 | 触发条件 | 表现 | 日志特征 | 影响范围 |
|-----|-------|---------|------|---------|---------|
| **容器OOM** | cgroup限制 | 容器内存超过limits | 容器重启,OOMKilled | exitCode=137, reason=OOMKilled | 单容器 |
| **Pod驱逐** | kubelet | 节点内存低于阈值 | Pod被驱逐到其他节点 | Evicted状态,MemoryPressure | 多Pod |
| **节点OOM** | 系统内核 | 系统内存耗尽 | 任意进程被kill | dmesg: oom-killer | 可能影响节点 |
| **init容器OOM** | cgroup限制 | init容器超限 | Pod启动失败 | Init:OOMKilled | Pod启动 |

### OOM Score机制

```
┌─────────────────────────────────────────────────────────────────┐
│                     OOM Score 计算逻辑                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   基础分数 = (进程内存使用 / 总内存) * 1000                       │
│                                                                  │
│   调整因素:                                                       │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ QoS Class        │ oom_score_adj │ 被kill优先级          │  │
│   ├──────────────────┼───────────────┼──────────────────────┤  │
│   │ Guaranteed       │ -997          │ 最低 (几乎不会)       │  │
│   │ Burstable        │ 2~999         │ 中等 (按内存超用比)   │  │
│   │ BestEffort       │ 1000          │ 最高 (优先kill)       │  │
│   │ 系统关键进程      │ -999          │ 受保护               │  │
│   │ kubelet          │ -999          │ 受保护               │  │
│   └──────────────────┴───────────────┴──────────────────────┘  │
│                                                                  │
│   Burstable oom_score_adj 计算:                                  │
│   adj = 1000 - 1000 * (requests.memory / limits.memory)         │
│   或: adj = max(2, 1000 - 10 * (requests.memory_% of node))     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## OOM诊断流程

### 第一阶段: 快速定位

```bash
#!/bin/bash
# oom-quick-diagnosis.sh - OOM快速诊断

POD_NAME=${1:-""}
NAMESPACE=${2:-"default"}

echo "====== OOM快速诊断 ======"
echo "时间: $(date)"
echo ""

# 1. 检查集群中所有OOMKilled事件
echo "=== 1. 最近OOMKilled事件 ==="
kubectl get events -A --field-selector reason=OOMKilled --sort-by='.lastTimestamp' | tail -20

# 2. 检查Evicted Pod
echo -e "\n=== 2. 被驱逐的Pod ==="
kubectl get pods -A --field-selector=status.phase=Failed | grep Evicted | head -20

# 3. 检查节点内存状态
echo -e "\n=== 3. 节点内存使用 ==="
kubectl top nodes 2>/dev/null || echo "Metrics Server未安装"

# 4. 检查节点MemoryPressure
echo -e "\n=== 4. 节点MemoryPressure状态 ==="
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.conditions[?(@.type=="MemoryPressure")]}{.status}{"\n"}{end}{end}'

# 5. 如果指定了Pod,检查具体Pod
if [ -n "$POD_NAME" ]; then
    echo -e "\n=== 5. Pod $POD_NAME 状态 ==="
    kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o yaml | grep -A 20 "containerStatuses:"
    
    echo -e "\n=== Pod资源配置 ==="
    kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .spec.containers[*]}Container: {.name}{"\n"}  requests.memory: {.resources.requests.memory}{"\n"}  limits.memory: {.resources.limits.memory}{"\n"}{end}'
    
    echo -e "\n=== Pod当前内存使用 ==="
    kubectl top pod "$POD_NAME" -n "$NAMESPACE" --containers 2>/dev/null
fi

# 6. 找出高内存使用的Pod
echo -e "\n=== 6. 内存使用Top10 Pod ==="
kubectl top pods -A --sort-by=memory 2>/dev/null | head -11
```

### 第二阶段: 详细分析

```bash
#!/bin/bash
# oom-detailed-analysis.sh - OOM详细分析

POD_NAME=$1
NAMESPACE=${2:-"default"}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <pod-name> [namespace]"
    exit 1
fi

echo "====== OOM详细分析: $NAMESPACE/$POD_NAME ======"

# 1. Pod完整状态
echo "=== 1. Pod状态详情 ==="
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq '{
  phase: .status.phase,
  containerStatuses: [.status.containerStatuses[] | {
    name: .name,
    ready: .ready,
    restartCount: .restartCount,
    lastState: .lastState,
    state: .state
  }]
}'

# 2. 检查是否OOMKilled
echo -e "\n=== 2. OOMKilled检查 ==="
OOM_REASON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[*].lastState.terminated.reason}')
if [ "$OOM_REASON" == "OOMKilled" ]; then
    echo "⚠️ 确认: 容器因OOM被kill"
    echo "终止时间: $(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[*].lastState.terminated.finishedAt}')"
    echo "退出码: $(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}')"
else
    echo "最近终止原因: $OOM_REASON"
fi

# 3. 资源配置分析
echo -e "\n=== 3. 资源配置分析 ==="
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq -r '
.spec.containers[] | 
"容器: \(.name)
  requests.memory: \(.resources.requests.memory // "未设置")
  limits.memory: \(.resources.limits.memory // "未设置")
  requests.cpu: \(.resources.requests.cpu // "未设置")
  limits.cpu: \(.resources.limits.cpu // "未设置")
"'

# 4. QoS Class
echo -e "\n=== 4. QoS Class ==="
QOS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.qosClass}')
echo "QoS Class: $QOS"
case $QOS in
    "Guaranteed")
        echo "说明: 最高优先级,limits=requests,最不容易被OOM kill"
        ;;
    "Burstable")
        echo "说明: 中等优先级,有requests但limits!=requests"
        ;;
    "BestEffort")
        echo "说明: 最低优先级,无resource配置,最容易被OOM kill"
        ;;
esac

# 5. 容器内存使用(如果Pod运行中)
echo -e "\n=== 5. 当前内存使用 ==="
kubectl top pod "$POD_NAME" -n "$NAMESPACE" --containers 2>/dev/null || echo "Pod未运行或Metrics不可用"

# 6. Pod事件
echo -e "\n=== 6. Pod相关事件 ==="
kubectl events -n "$NAMESPACE" --for="pod/$POD_NAME" --types=Warning | tail -20

# 7. 节点信息
echo -e "\n=== 7. 所在节点信息 ==="
NODE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.nodeName}')
if [ -n "$NODE" ]; then
    echo "节点: $NODE"
    kubectl top node "$NODE" 2>/dev/null
    kubectl describe node "$NODE" | grep -A 10 "Allocated resources:"
fi
```

### 第三阶段: 节点级分析

```bash
#!/bin/bash
# node-memory-analysis.sh - 节点内存深度分析(需SSH到节点)

echo "====== 节点内存深度分析 ======"
echo "主机名: $(hostname)"
echo "时间: $(date)"
echo ""

# 1. 系统内存概览
echo "=== 1. 系统内存概览 ==="
free -h
echo ""

# 2. 详细内存信息
echo "=== 2. 内存详情 ==="
cat /proc/meminfo | grep -E "MemTotal|MemFree|MemAvailable|Buffers|^Cached|SwapTotal|SwapFree|Slab|SReclaimable|SUnreclaim|Mapped|Shmem"

# 3. kubelet内存阈值
echo -e "\n=== 3. kubelet驱逐阈值 ==="
cat /var/lib/kubelet/config.yaml 2>/dev/null | grep -A 15 "eviction" || echo "无法读取kubelet配置"

# 4. cgroup内存统计
echo -e "\n=== 4. cgroup内存统计 ==="
if [ -d "/sys/fs/cgroup/memory/kubepods" ]; then
    # cgroup v1
    echo "cgroup v1 模式"
    echo "kubepods总使用: $(cat /sys/fs/cgroup/memory/kubepods/memory.usage_in_bytes | numfmt --to=iec)"
    echo "kubepods限制: $(cat /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes | numfmt --to=iec)"
elif [ -d "/sys/fs/cgroup/kubepods.slice" ]; then
    # cgroup v2
    echo "cgroup v2 模式"
    echo "kubepods当前使用: $(cat /sys/fs/cgroup/kubepods.slice/memory.current | numfmt --to=iec)"
    echo "kubepods最大限制: $(cat /sys/fs/cgroup/kubepods.slice/memory.max 2>/dev/null || echo 'max')"
fi

# 5. 进程内存排名
echo -e "\n=== 5. 内存使用Top15进程 ==="
ps aux --sort=-%mem | head -16 | awk '{printf "%-10s %-8s %-6s %-10s %s\n", $1, $2, $4"%", $6/1024"MB", $11}'

# 6. 容器内存使用
echo -e "\n=== 6. 容器内存使用Top10 ==="
for cid in $(crictl ps -q | head -10); do
    name=$(crictl inspect "$cid" 2>/dev/null | jq -r '.status.labels["io.kubernetes.container.name"]')
    pod=$(crictl inspect "$cid" 2>/dev/null | jq -r '.status.labels["io.kubernetes.pod.name"]')
    
    # cgroup路径
    cgroup_path=$(crictl inspect "$cid" 2>/dev/null | jq -r '.info.runtimeSpec.linux.cgroupsPath')
    
    if [ -n "$cgroup_path" ] && [ -f "/sys/fs/cgroup/memory/${cgroup_path}/memory.usage_in_bytes" ]; then
        usage=$(cat "/sys/fs/cgroup/memory/${cgroup_path}/memory.usage_in_bytes" | numfmt --to=iec)
        limit=$(cat "/sys/fs/cgroup/memory/${cgroup_path}/memory.limit_in_bytes" | numfmt --to=iec)
        echo "$pod/$name: $usage / $limit"
    fi
done 2>/dev/null

# 7. OOM历史
echo -e "\n=== 7. 最近OOM事件 ==="
dmesg | grep -i "oom\|killed process\|out of memory" | tail -20

# 8. 内存压力指标
echo -e "\n=== 8. 内存压力指标 ==="
if [ -f "/proc/pressure/memory" ]; then
    echo "PSI内存压力:"
    cat /proc/pressure/memory
fi

echo ""
cat /proc/vmstat | grep -E "pgfault|pgmajfault|pswpin|pswpout|oom_kill"
```

## 容器OOM诊断

### 识别OOMKilled容器

```bash
# 1. 查找所有OOMKilled的Pod
kubectl get pods -A -o json | jq -r '
.items[] | 
select(.status.containerStatuses != null) |
select(.status.containerStatuses[].lastState.terminated.reason == "OOMKilled") |
"\(.metadata.namespace)/\(.metadata.name)"'

# 2. 获取OOMKilled详情
kubectl get pods -A -o json | jq -r '
.items[] | 
select(.status.containerStatuses != null) |
.status.containerStatuses[] | 
select(.lastState.terminated.reason == "OOMKilled") |
"Pod: \(.name)
  Restart Count: \(.restartCount)
  Killed At: \(.lastState.terminated.finishedAt)
  Exit Code: \(.lastState.terminated.exitCode)
"'

# 3. 检查特定Pod的OOM历史
kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "Last State:"

# 4. 查看容器cgroup内存事件
# SSH到节点后
CONTAINER_ID="<container-id>"
# cgroup v1
cat /sys/fs/cgroup/memory/kubepods/*/*/$CONTAINER_ID/memory.oom_control
cat /sys/fs/cgroup/memory/kubepods/*/*/$CONTAINER_ID/memory.failcnt
# cgroup v2
cat /sys/fs/cgroup/kubepods.slice/*/cri-containerd-$CONTAINER_ID.scope/memory.events
```

### 容器内存使用分析

```bash
#!/bin/bash
# container-memory-analysis.sh

CONTAINER_ID=$1
if [ -z "$CONTAINER_ID" ]; then
    echo "用法: $0 <container-id>"
    exit 1
fi

echo "====== 容器内存分析: $CONTAINER_ID ======"

# 1. 获取容器信息
echo "=== 1. 容器基本信息 ==="
crictl inspect "$CONTAINER_ID" 2>/dev/null | jq '{
  name: .status.labels["io.kubernetes.container.name"],
  pod: .status.labels["io.kubernetes.pod.name"],
  namespace: .status.labels["io.kubernetes.pod.namespace"],
  state: .status.state
}'

# 2. 获取cgroup路径
CGROUP_PATH=$(crictl inspect "$CONTAINER_ID" 2>/dev/null | jq -r '.info.runtimeSpec.linux.cgroupsPath')
echo -e "\n=== 2. cgroup路径 ==="
echo "$CGROUP_PATH"

# 3. cgroup内存详情
echo -e "\n=== 3. cgroup内存详情 ==="
if [ -d "/sys/fs/cgroup/memory/$CGROUP_PATH" ]; then
    # cgroup v1
    echo "当前使用: $(cat /sys/fs/cgroup/memory/$CGROUP_PATH/memory.usage_in_bytes | numfmt --to=iec)"
    echo "内存限制: $(cat /sys/fs/cgroup/memory/$CGROUP_PATH/memory.limit_in_bytes | numfmt --to=iec)"
    echo "最大使用: $(cat /sys/fs/cgroup/memory/$CGROUP_PATH/memory.max_usage_in_bytes | numfmt --to=iec)"
    echo "失败计数: $(cat /sys/fs/cgroup/memory/$CGROUP_PATH/memory.failcnt)"
    echo ""
    echo "内存统计:"
    cat /sys/fs/cgroup/memory/$CGROUP_PATH/memory.stat | head -15
elif [ -d "/sys/fs/cgroup/$CGROUP_PATH" ]; then
    # cgroup v2
    echo "当前使用: $(cat /sys/fs/cgroup/$CGROUP_PATH/memory.current | numfmt --to=iec)"
    echo "内存最大: $(cat /sys/fs/cgroup/$CGROUP_PATH/memory.max)"
    echo "内存高水位: $(cat /sys/fs/cgroup/$CGROUP_PATH/memory.high 2>/dev/null || echo 'max')"
    echo ""
    echo "内存事件:"
    cat /sys/fs/cgroup/$CGROUP_PATH/memory.events
fi

# 4. 容器内进程内存
echo -e "\n=== 4. 容器内进程 ==="
PID=$(crictl inspect "$CONTAINER_ID" 2>/dev/null | jq -r '.info.pid')
if [ -n "$PID" ] && [ "$PID" != "null" ]; then
    # 获取容器内所有进程
    for pid in $(pgrep -P $PID); do
        rss=$(cat /proc/$pid/status 2>/dev/null | grep VmRSS | awk '{print $2}')
        comm=$(cat /proc/$pid/comm 2>/dev/null)
        echo "$comm (PID:$pid): ${rss}kB"
    done | sort -t: -k2 -nr | head -10
fi
```

## 节点OOM诊断

### 系统级OOM分析

```bash
#!/bin/bash
# system-oom-analysis.sh

echo "====== 系统级OOM分析 ======"

# 1. 检查dmesg中的OOM事件
echo "=== 1. dmesg OOM事件 ==="
dmesg | grep -i "oom\|killed process\|out of memory" | tail -30

# 2. 解析OOM kill详情
echo -e "\n=== 2. OOM Kill详情解析 ==="
dmesg | grep -A 20 "invoked oom-killer" | tail -30

# 3. 检查journalctl
echo -e "\n=== 3. journalctl OOM记录 ==="
journalctl -k | grep -i "oom\|killed" | tail -20

# 4. 检查哪些进程被OOM kill
echo -e "\n=== 4. 被kill的进程 ==="
dmesg | grep "Killed process" | tail -10

# 5. 当前系统OOM score
echo -e "\n=== 5. 进程OOM Score Top10 ==="
for proc in /proc/[0-9]*; do
    pid=$(basename $proc)
    if [ -f "$proc/oom_score" ] && [ -f "$proc/comm" ]; then
        score=$(cat "$proc/oom_score" 2>/dev/null)
        adj=$(cat "$proc/oom_score_adj" 2>/dev/null)
        comm=$(cat "$proc/comm" 2>/dev/null)
        echo "$score $adj $pid $comm"
    fi
done 2>/dev/null | sort -rn | head -10 | awk '{printf "Score:%-5s Adj:%-5s PID:%-8s %s\n", $1, $2, $3, $4}'

# 6. kubelet和containerd的OOM保护
echo -e "\n=== 6. 关键进程OOM保护 ==="
for proc in kubelet containerd dockerd; do
    pid=$(pgrep -x $proc | head -1)
    if [ -n "$pid" ]; then
        adj=$(cat /proc/$pid/oom_score_adj 2>/dev/null)
        score=$(cat /proc/$pid/oom_score 2>/dev/null)
        echo "$proc (PID:$pid): oom_score=$score, oom_score_adj=$adj"
    fi
done
```

### kubelet驱逐分析

```bash
#!/bin/bash
# kubelet-eviction-analysis.sh

echo "====== kubelet驱逐分析 ======"

# 1. 当前驱逐配置
echo "=== 1. 驱逐配置 ==="
cat /var/lib/kubelet/config.yaml | grep -A 20 "eviction"

# 2. 当前内存状态vs阈值
echo -e "\n=== 2. 内存状态vs阈值 ==="
TOTAL=$(cat /proc/meminfo | grep MemTotal | awk '{print $2}')
AVAIL=$(cat /proc/meminfo | grep MemAvailable | awk '{print $2}')
echo "总内存: $((TOTAL/1024)) MB"
echo "可用内存: $((AVAIL/1024)) MB"
echo "可用比例: $((AVAIL*100/TOTAL))%"

# 解析驱逐阈值
HARD_THRESHOLD=$(cat /var/lib/kubelet/config.yaml | grep -A 5 "evictionHard:" | grep "memory.available" | awk -F'"' '{print $2}')
SOFT_THRESHOLD=$(cat /var/lib/kubelet/config.yaml | grep -A 5 "evictionSoft:" | grep "memory.available" | awk -F'"' '{print $2}')
echo "硬驱逐阈值: $HARD_THRESHOLD"
echo "软驱逐阈值: $SOFT_THRESHOLD"

# 3. kubelet驱逐日志
echo -e "\n=== 3. kubelet驱逐日志 ==="
journalctl -u kubelet --since "1 hour ago" | grep -i "evict" | tail -20

# 4. 被驱逐的Pod
echo -e "\n=== 4. 被驱逐Pod(kubectl) ==="
kubectl get pods -A --field-selector=status.phase=Failed -o json 2>/dev/null | jq -r '
.items[] | 
select(.status.reason == "Evicted") |
"\(.metadata.namespace)/\(.metadata.name): \(.status.message)"' | head -20
```

## 内存配置最佳实践

### QoS等级与资源配置

| QoS等级 | 配置要求 | requests/limits比 | 使用场景 | OOM优先级 |
|--------|---------|-------------------|---------|----------|
| **Guaranteed** | requests=limits(CPU和Memory都设置) | 1:1 | 关键业务,数据库 | 最低(最不容易被kill) |
| **Burstable** | 至少设置一个requests | < 1 | 一般应用,允许突发 | 中等(按超用比例) |
| **BestEffort** | 不设置任何requests/limits | N/A | 开发测试,低优先级任务 | 最高(优先被kill) |

### 内存配置建议

```yaml
# 推荐的内存配置模式
apiVersion: v1
kind: Pod
metadata:
  name: memory-best-practice
spec:
  containers:
  - name: app
    image: app:v1
    resources:
      requests:
        # 基于实际稳态使用量的1.2倍
        memory: "256Mi"
        cpu: "100m"
      limits:
        # requests的1.5-2倍,允许合理突发
        memory: "512Mi"
        cpu: "500m"
        
---
# Guaranteed QoS - 关键服务
apiVersion: v1
kind: Pod
metadata:
  name: critical-service
spec:
  containers:
  - name: app
    image: critical-app:v1
    resources:
      requests:
        memory: "1Gi"
        cpu: "500m"
      limits:
        # requests = limits 实现Guaranteed QoS
        memory: "1Gi"
        cpu: "500m"
        
---
# Burstable QoS - 一般应用
apiVersion: v1
kind: Pod
metadata:
  name: normal-app
spec:
  containers:
  - name: app
    image: normal-app:v1
    resources:
      requests:
        memory: "256Mi"
        cpu: "100m"
      limits:
        memory: "1Gi"    # 允许4倍内存突发
        cpu: "1000m"     # 允许10倍CPU突发
```

### 内存limits设置原则

```
┌─────────────────────────────────────────────────────────────────┐
│                   内存limits设置决策流程                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Step 1: 确定基准内存使用                                        │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ • 监控应用稳态内存使用(至少观察24小时)                     │  │
│   │ • 记录峰值内存使用                                         │  │
│   │ • 考虑启动时内存峰值                                       │  │
│   └──────────────────────────────────────────────────────────┘  │
│                              ↓                                   │
│   Step 2: 设置requests                                           │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ requests = 稳态使用 × 1.2                                  │  │
│   │ (预留20%缓冲)                                              │  │
│   └──────────────────────────────────────────────────────────┘  │
│                              ↓                                   │
│   Step 3: 设置limits                                             │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ 关键服务: limits = requests (Guaranteed)                   │  │
│   │ 一般应用: limits = max(峰值×1.2, requests×1.5)            │  │
│   │ 开发环境: limits = requests × 2                            │  │
│   └──────────────────────────────────────────────────────────┘  │
│                              ↓                                   │
│   Step 4: 验证                                                   │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ • limits/requests比不超过2(避免过度超卖)                   │  │
│   │ • 压测验证limits足够                                       │  │
│   │ • 监控OOM事件                                              │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## JVM应用内存配置

### 容器感知JVM配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: java-app
spec:
  containers:
  - name: java
    image: openjdk:17-slim
    env:
    # JVM容器感知设置(JDK 8u191+/JDK 10+)
    - name: JAVA_OPTS
      value: >-
        -XX:+UseContainerSupport
        -XX:MaxRAMPercentage=75.0
        -XX:InitialRAMPercentage=50.0
        -XX:MinRAMPercentage=25.0
        -XX:+HeapDumpOnOutOfMemoryError
        -XX:HeapDumpPath=/tmp/heapdump.hprof
        -XX:+ExitOnOutOfMemoryError
        -XX:+UseG1GC
        -XX:MaxGCPauseMillis=200
        -Xlog:gc*:file=/tmp/gc.log:time,uptime:filecount=5,filesize=10M
    command: ["java"]
    args:
      - "$(JAVA_OPTS)"
      - "-jar"
      - "/app/app.jar"
    resources:
      requests:
        memory: "1Gi"
        cpu: "500m"
      limits:
        memory: "2Gi"
        cpu: "2000m"
    volumeMounts:
    - name: heap-dumps
      mountPath: /tmp
  volumes:
  - name: heap-dumps
    emptyDir:
      sizeLimit: 5Gi
```

### JVM内存计算

```
┌─────────────────────────────────────────────────────────────────┐
│                      JVM内存组成                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   容器limits.memory 应该 >= JVM总内存需求                         │
│                                                                  │
│   JVM总内存 = Heap + Non-Heap + Native                           │
│                                                                  │
│   ┌────────────────────────────────────────────────────────┐    │
│   │ Heap (堆内存)                                           │    │
│   │ • -Xmx / MaxRAMPercentage控制                           │    │
│   │ • 建议: 容器limits的50-75%                              │    │
│   └────────────────────────────────────────────────────────┘    │
│   ┌────────────────────────────────────────────────────────┐    │
│   │ Non-Heap (非堆)                                         │    │
│   │ • Metaspace: 类元数据 (~256MB-512MB)                    │    │
│   │ • CodeCache: JIT编译代码 (~128MB-256MB)                 │    │
│   │ • Thread Stacks: 线程栈 (线程数 × 1MB)                  │    │
│   └────────────────────────────────────────────────────────┘    │
│   ┌────────────────────────────────────────────────────────┐    │
│   │ Native Memory                                           │    │
│   │ • Direct ByteBuffers: NIO直接内存                       │    │
│   │ • JNI: 本地库内存                                       │    │
│   │ • GC: 垃圾回收器内部使用                                │    │
│   └────────────────────────────────────────────────────────┘    │
│                                                                  │
│   推荐配置示例(2Gi容器):                                         │
│   • MaxRAMPercentage=75.0 → Heap ≈ 1.5Gi                        │
│   • 剩余 0.5Gi 用于 Non-Heap + Native                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### JVM OOM类型区分

| OOM类型 | 错误信息 | 原因 | 解决方案 |
|--------|---------|------|---------|
| **Java Heap** | java.lang.OutOfMemoryError: Java heap space | 堆内存不足 | 增加-Xmx或优化代码 |
| **Metaspace** | java.lang.OutOfMemoryError: Metaspace | 类元数据空间满 | 增加-XX:MaxMetaspaceSize |
| **GC Overhead** | java.lang.OutOfMemoryError: GC overhead limit exceeded | GC占用过多时间 | 优化代码减少对象创建 |
| **Direct Memory** | java.lang.OutOfMemoryError: Direct buffer memory | 直接内存不足 | 增加-XX:MaxDirectMemorySize |
| **Native** | java.lang.OutOfMemoryError: unable to create new native thread | 线程数过多 | 减少线程或增加系统限制 |
| **Container OOM** | 容器被kill,exitCode=137 | 容器内存超限 | 增加limits或减少MaxRAMPercentage |

## Node内存驱逐配置

### kubelet驱逐阈值配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 硬驱逐阈值 - 立即驱逐,无宽限期
evictionHard:
  memory.available: "100Mi"      # 可用内存低于100Mi
  nodefs.available: "10%"        # 根分区可用空间低于10%
  nodefs.inodesFree: "5%"        # 根分区inode低于5%
  imagefs.available: "15%"       # 镜像分区可用空间低于15%
  
# 软驱逐阈值 - 有宽限期
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  
# 软驱逐宽限期
evictionSoftGracePeriod:
  memory.available: "1m30s"      # 持续1分30秒后开始驱逐
  nodefs.available: "1m30s"

# 驱逐后最小回收量
evictionMinimumReclaim:
  memory.available: "500Mi"      # 至少回收500Mi内存
  nodefs.available: "1Gi"
  
# 压力转换周期
evictionPressureTransitionPeriod: 30s

# eviction信号评估周期
evictionMaxPodGracePeriod: 30    # Pod终止最大宽限期

# 系统预留
systemReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "1Gi"

kubeReserved:
  cpu: "500m"  
  memory: "1Gi"
  ephemeral-storage: "1Gi"
```

### 驱逐顺序

```
┌─────────────────────────────────────────────────────────────────┐
│                    kubelet驱逐顺序                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   当节点内存压力时,kubelet按以下顺序驱逐Pod:                      │
│                                                                  │
│   1. 优先级(PriorityClass)                                       │
│      └── 低优先级Pod先被驱逐                                     │
│                                                                  │
│   2. 资源使用超出requests的程度                                   │
│      └── 超出越多,越优先被驱逐                                   │
│                                                                  │
│   3. QoS等级                                                      │
│      ┌─────────────────────────────────────────────────────┐    │
│      │  BestEffort  →  Burstable(超用)  →  Guaranteed     │    │
│      │  (优先驱逐)      (次优先)           (最后驱逐)       │    │
│      └─────────────────────────────────────────────────────┘    │
│                                                                  │
│   具体排序公式:                                                   │
│   score = priority_score + usage_score + qos_score              │
│                                                                  │
│   usage_score = (当前使用 - requests) / requests                │
│   • usage_score越大,越优先驱逐                                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## 内存问题预防

### VPA自动调整

```yaml
# Vertical Pod Autoscaler配置
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
  namespace: default
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  updatePolicy:
    updateMode: "Auto"              # Off/Initial/Recreate/Auto
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: "50m"
        memory: "128Mi"
      maxAllowed:
        cpu: "4"
        memory: "8Gi"
      controlledResources: ["cpu", "memory"]
      controlledValues: RequestsAndLimits  # RequestsOnly/RequestsAndLimits
      
---
# VPA推荐模式(仅生成建议,不自动应用)
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa-recommend
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  updatePolicy:
    updateMode: "Off"               # 仅推荐,不更新
```

### 查看VPA推荐

```bash
# 查看VPA推荐值
kubectl get vpa app-vpa -o jsonpath='{.status.recommendation.containerRecommendations[*]}' | jq

# 输出示例:
# {
#   "containerName": "app",
#   "lowerBound": {"cpu": "50m", "memory": "128Mi"},
#   "target": {"cpu": "100m", "memory": "256Mi"},
#   "upperBound": {"cpu": "200m", "memory": "512Mi"},
#   "uncappedTarget": {"cpu": "100m", "memory": "256Mi"}
# }
```

### LimitRange设置

```yaml
# 命名空间级别默认限制
apiVersion: v1
kind: LimitRange
metadata:
  name: memory-limits
  namespace: default
spec:
  limits:
  # 容器级别
  - type: Container
    default:
      memory: "512Mi"
      cpu: "500m"
    defaultRequest:
      memory: "256Mi"
      cpu: "100m"
    max:
      memory: "4Gi"
      cpu: "4"
    min:
      memory: "64Mi"
      cpu: "50m"
    maxLimitRequestRatio:
      memory: "2"                   # limits/requests最大比值
      cpu: "10"
      
  # Pod级别
  - type: Pod
    max:
      memory: "8Gi"
      cpu: "8"
```

### ResourceQuota设置

```yaml
# 命名空间资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: mem-quota
  namespace: default
spec:
  hard:
    requests.memory: "10Gi"         # 命名空间总requests上限
    limits.memory: "20Gi"           # 命名空间总limits上限
    pods: "50"                      # Pod数量上限
    
  # 按优先级分配
  scopeSelector:
    matchExpressions:
    - operator: In
      scopeName: PriorityClass
      values: ["high"]
```

## 监控告警配置

### Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: memory-alerts
  namespace: monitoring
spec:
  groups:
  - name: container.memory
    interval: 30s
    rules:
    # 容器OOMKilled告警
    - alert: ContainerOOMKilled
      expr: |
        increase(kube_pod_container_status_restarts_total[10m]) > 0
        and on (namespace, pod, container)
        kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
      for: 0m
      labels:
        severity: warning
        category: memory
      annotations:
        summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} OOM重启"
        description: "容器在过去10分钟内因OOM被kill并重启"
        
    # 容器内存使用率高
    - alert: ContainerMemoryUsageHigh
      expr: |
        (container_memory_working_set_bytes / container_spec_memory_limit_bytes) * 100 > 85
        and container_spec_memory_limit_bytes > 0
      for: 5m
      labels:
        severity: warning
        category: memory
      annotations:
        summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} 内存使用率高"
        description: "容器内存使用率超过85%,当前: {{ $value | printf \"%.1f\" }}%,可能发生OOM"
        
    # 容器内存接近限制
    - alert: ContainerMemoryNearLimit
      expr: |
        (container_memory_working_set_bytes / container_spec_memory_limit_bytes) * 100 > 95
        and container_spec_memory_limit_bytes > 0
      for: 2m
      labels:
        severity: critical
        category: memory
      annotations:
        summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} 内存即将耗尽"
        description: "容器内存使用率超过95%,即将OOM"
        
    # 频繁OOM重启
    - alert: ContainerFrequentOOMKill
      expr: |
        increase(kube_pod_container_status_restarts_total[1h]) > 3
        and on (namespace, pod, container)
        kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
      for: 0m
      labels:
        severity: critical
        category: memory
      annotations:
        summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} 频繁OOM"
        description: "容器在过去1小时内OOM重启超过3次"

  - name: node.memory
    rules:
    # 节点内存压力
    - alert: NodeMemoryPressure
      expr: |
        kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 2m
      labels:
        severity: critical
        category: memory
      annotations:
        summary: "节点 {{ $labels.node }} 内存压力"
        description: "节点处于MemoryPressure状态,正在驱逐Pod"
        
    # 节点内存使用率高
    - alert: NodeMemoryHigh
      expr: |
        (1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100 > 85
      for: 5m
      labels:
        severity: warning
        category: memory
      annotations:
        summary: "节点 {{ $labels.instance }} 内存使用率高"
        description: "节点内存使用率超过85%,当前: {{ $value | printf \"%.1f\" }}%"
        
    # 节点内存严重不足
    - alert: NodeMemoryCritical
      expr: |
        (1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100 > 95
      for: 2m
      labels:
        severity: critical
        category: memory
      annotations:
        summary: "节点 {{ $labels.instance }} 内存严重不足"
        description: "节点内存使用率超过95%,可能触发系统OOM"

  - name: pod.eviction
    rules:
    # Pod被驱逐
    - alert: PodEvicted
      expr: |
        increase(kube_pod_status_reason{reason="Evicted"}[10m]) > 0
      for: 0m
      labels:
        severity: warning
        category: memory
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 被驱逐"
        description: "Pod因资源压力被驱逐"
        
    # 大量Pod被驱逐
    - alert: MassivePodEviction
      expr: |
        sum(increase(kube_pod_status_reason{reason="Evicted"}[30m])) > 10
      for: 0m
      labels:
        severity: critical
        category: memory
      annotations:
        summary: "集群大量Pod被驱逐"
        description: "过去30分钟内有超过10个Pod被驱逐,检查节点资源"
```

### Grafana Dashboard查询

```json
{
  "panels": [
    {
      "title": "容器内存使用率",
      "targets": [
        {
          "expr": "(container_memory_working_set_bytes{container!=\"\", container!=\"POD\"} / container_spec_memory_limit_bytes{container!=\"\", container!=\"POD\"}) * 100",
          "legendFormat": "{{ namespace }}/{{ pod }}/{{ container }}"
        }
      ]
    },
    {
      "title": "OOMKilled事件",
      "targets": [
        {
          "expr": "increase(kube_pod_container_status_restarts_total[1h]) and on(namespace,pod,container) kube_pod_container_status_last_terminated_reason{reason=\"OOMKilled\"} == 1",
          "legendFormat": "{{ namespace }}/{{ pod }}"
        }
      ]
    },
    {
      "title": "节点内存使用",
      "targets": [
        {
          "expr": "(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100",
          "legendFormat": "{{ instance }}"
        }
      ]
    },
    {
      "title": "kubepods内存使用",
      "targets": [
        {
          "expr": "sum by(node) (container_memory_working_set_bytes{id=\"/kubepods\"})",
          "legendFormat": "{{ node }}"
        }
      ]
    }
  ]
}
```

## 内存问题排查清单

| 检查项 | 诊断命令 | 期望结果 | 异常处理 |
|-------|---------|---------|---------|
| Pod内存使用 | `kubectl top pod` | 低于limits | 优化应用或增加limits |
| 节点内存使用 | `kubectl top nodes` | 低于85% | 扩容或驱逐Pod |
| OOMKilled事件 | `kubectl get events --field-selector=reason=OOMKilled` | 无结果 | 调整内存配置 |
| 容器重启次数 | `kubectl get pods -o jsonpath='{.items[*].status.containerStatuses[*].restartCount}'` | 0或低 | 检查OOM原因 |
| 节点MemoryPressure | `kubectl describe node \| grep MemoryPressure` | False | 检查节点资源 |
| 被驱逐Pod | `kubectl get pods --field-selector=status.phase=Failed` | 无Evicted | 检查驱逐原因 |
| QoS配置 | `kubectl get pod -o jsonpath='{.status.qosClass}'` | 符合预期 | 调整resource配置 |

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| v1.22 | cgroup v2默认支持 | 新的内存控制接口 |
| v1.24 | Memory QoS (Alpha) | 基于memory.high的软限制 |
| v1.25 | PodOverhead计入驱逐决策 | 更精确的驱逐计算 |
| v1.27 | In-place Pod Resources Update (Alpha) | 不重启更新资源 |
| v1.28 | Memory QoS (Beta) | 更成熟的内存QoS |
| v1.29 | Sidecar Containers GA | 影响OOM优先级 |
| v1.30 | Node Memory Swap (Beta) | swap支持更成熟 |

---

**OOM防治原则**: 合理设置limits → 监控内存使用 → 配置VPA → JVM容器感知 → 设置驱逐阈值 → 分析根因持续优化

---

## 深度解决方案与预防体系

### 4.1 容器OOM问题根本解决策略

#### 4.1.1 内存请求和限制优化

**精准资源配置方法：**

```yaml
# 生产环境推荐配置模板
apiVersion: v1
kind: Pod
metadata:
  name: memory-intensive-app
spec:
  containers:
  - name: app
    image: myapp:v1.0
    resources:
      requests:
        memory: "512Mi"    # 基于实际压测数据
        cpu: "250m"
      limits:
        memory: "1Gi"      # requests的1.5-2倍
        cpu: "1000m"
    # 内存优化配置
    env:
    - name: GOMEMLIMIT
      value: "800MiB"      # Go应用内存软限制
    - name: JAVA_OPTS
      value: "-Xmx768m -XX:MaxRAMPercentage=75.0"
```

**资源配置调优步骤：**

1. **基线测量**
   ```bash
   # 使用metrics-server观察历史峰值
   kubectl top pods -n <namespace> --containers
   
   # 使用Prometheus查询历史数据
   rate(container_memory_working_set_bytes[5m]) > 0
   ```

2. **压力测试**
   ```bash
   # 内存压力测试工具
   kubectl run mem-stress --image=polinux/stress \
     --restart=Never \
     -- -m 1 --vm-bytes 800M --timeout 300s
   ```

3. **动态调整**
   ```bash
   # VPA自动调整建议
   kubectl get vpa <vpa-name> -o jsonpath='{.status.recommendation.containerRecommendations}'
   ```

#### 4.1.2 应用层内存优化

**JVM应用优化：**

```bash
# 容器感知的JVM参数
-XX:+UseContainerSupport \
-XX:MaxRAMPercentage=75.0 \
-XX:InitialRAMPercentage=50.0 \
-XX:MinRAMPercentage=25.0 \
-Xlog:gc*:stdout:time,tags \
-XX:+PrintCommandLineFlags
```

**Go应用优化：**
```go
// 内存限制感知
func init() {
    if limit, err := memlimit.FromCgroup(); err == nil {
        debug.SetMemoryLimit(int64(float64(limit) * 0.9))
    }
}
```

#### 4.1.3 系统级内存管理优化

**节点内存预留配置：**

```yaml
# kubelet配置优化
apiVersion: v1
kind: Node
metadata:
  name: worker-node
spec:
  kubeletConfig:
    systemReserved:
      memory: "2Gi"        # 系统进程预留
    kubeReserved:
      memory: "1Gi"        # K8s组件预留
    evictionHard:
      memory.available: "500Mi"    # 硬驱逐阈值
      nodefs.available: "10%"      # 磁盘阈值
    evictionSoft:
      memory.available: "1Gi"       # 软驱逐阈值
      nodefs.available: "15%"
    evictionSoftGracePeriod:
      memory.available: "1m30s"     # 软驱逐宽限期
```

### 4.2 Pod驱逐问题综合治理

#### 4.2.1 驱逐策略优化

**分层驱逐配置：**

```yaml
# 分QoS驱逐策略
apiVersion: v1
kind: Pod
metadata:
  name: critical-app
  annotations:
    # 关键应用保护
    scheduler.alpha.kubernetes.io/critical-pod: ""
spec:
  priorityClassName: high-priority
  containers:
  - name: app
    resources:
      requests:
        memory: "1Gi"
      limits:
        memory: "2Gi"
```

**驱逐顺序控制：**
```bash
# 驱逐顺序验证
kubectl describe node <node-name> | grep -A 10 "Eviction"
```

#### 4.2.2 集群容量规划

**容量评估脚本：**
```bash
#!/bin/bash
# cluster_capacity_check.sh

echo "=== 集群内存容量分析 ==="

# 节点资源统计
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.memory}{"\t"}{.status.capacity.memory}{"\n"}{end}'

# Pod内存请求汇总
echo -e "\n=== Pod内存请求分布 ==="
kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.spec.containers[*].resources.requests.memory}{"\n"}{end}' | \
  grep -v '^$' | sort | uniq -c | sort -nr

# 驱逐风险评估
echo -e "\n=== 驱逐风险评估 ==="
kubectl top nodes | awk '$4 > 85 {print "高风险节点:", $1, "内存使用率:", $4"%"}'
```

### 4.3 系统OOM防护机制

#### 4.3.1 内核参数调优

**系统级防护配置：**
```bash
# /etc/sysctl.conf 内存相关参数
vm.overcommit_memory=1          # 允许内存超分配
vm.overcommit_ratio=80          # 超分配比例
vm.swappiness=1                 # 降低swap倾向
vm.min_free_kbytes=65536        # 保留最小空闲内存
vm.admin_reserve_kbytes=131072  # 管理员保留内存

# 应用配置
sysctl -p
```

#### 4.3.2 进程保护策略

**关键进程保护：**
```bash
# 保护kubelet进程
echo -999 > /proc/$(pidof kubelet)/oom_score_adj

# 保护系统关键服务
systemctl daemon-reload
systemctl restart kubelet
```

### 4.4 监控告警与自动化响应

#### 4.4.1 完整监控体系

**Prometheus告警规则：**
```yaml
# memory_alerts.yaml
groups:
- name: memory.rules
  rules:
  - alert: ContainerMemoryUsageHigh
    expr: (container_memory_working_set_bytes / container_spec_memory_limit_bytes * 100) > 85
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "容器内存使用率过高 {{ $labels.pod }}"
      
  - alert: NodeMemoryPressure
    expr: node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes * 100 < 15
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "节点内存压力过大 {{ $labels.instance }}"
```

#### 4.4.2 自动化响应机制

**自动扩容脚本：**
```bash
#!/bin/bash
# auto_scale_memory.sh

NAMESPACE="production"
DEPLOYMENT="memory-intensive-app"

# 检查内存使用率
CURRENT_USAGE=$(kubectl top deployment $DEPLOYMENT -n $NAMESPACE --no-headers | awk '{print $3}' | sed 's/%//')

if [ $CURRENT_USAGE -gt 80 ]; then
    # 扩容副本数
    kubectl scale deployment $DEPLOYMENT -n $NAMESPACE --replicas=3
    echo "已触发自动扩容: $DEPLOYMENT"
elif [ $CURRENT_USAGE -lt 30 ]; then
    # 缩容副本数
    kubectl scale deployment $DEPLOYMENT -n $NAMESPACE --replicas=1
    echo "已触发自动缩容: $DEPLOYMENT"
fi
```

### 4.5 故障演练与预案

#### 4.5.1 模拟故障演练

**OOM故障模拟：**
```bash
# 模拟容器内存泄漏
kubectl run oom-test --image=busybox --restart=Never \
  --limits=memory=100Mi \
  -- sh -c "dd if=/dev/zero of=/tmp/test bs=1M count=200"

# 监控驱逐过程
watch -n 1 'kubectl get pods -o wide | grep oom-test'
```

#### 4.5.2 应急响应预案

**分级响应流程：**

1. **Level 1 - 容器OOM (5分钟内响应)**
   - 自动重启容器
   - 记录事件日志
   - 通知应用负责人

2. **Level 2 - Pod驱逐 (10分钟内响应)**
   - 检查节点资源状况
   - 调整资源配置
   - 考虑节点扩容

3. **Level 3 - 节点OOM (30分钟内响应)**
   - 隔离故障节点
   - 迁移关键应用
   - 紧急扩容新节点

---
