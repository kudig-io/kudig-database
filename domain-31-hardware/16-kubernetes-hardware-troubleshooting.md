# Kubernetes 运维硬件故障排查专题

## 概述

Kubernetes 集群运维中，硬件故障是导致节点异常、Pod 驱逐、服务中断的重要原因。本文档专注于 K8s 场景下的硬件故障识别、诊断方法和应急处理流程。

## K8s 硬件故障影响矩阵

### 硬件故障与 K8s 症状映射

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                    硬件故障 → Kubernetes 症状映射表                                           │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                             │
│  硬件故障类型          │  Kubernetes 表现                │  系统日志特征                    │
│  ─────────────────────┼─────────────────────────────────┼──────────────────────────────── │
│  CPU MCE错误          │  • Node NotReady                │  • kernel: mce: CPU x           │
│                       │  • kubelet 无响应               │  • Machine check events logged │
│                       │  • 容器进程被kill               │  • BUG: unable to handle kernel │
│                       │                                 │                                  │
│  内存 ECC UCE         │  • Node 突然宕机                │  • EDAC MC: UE on mc            │
│                       │  • 多个 Pod CrashLoopBackOff    │  • kernel panic                  │
│                       │  • OOMKilled (误判)             │  • MCE: Uncorrected error       │
│                       │                                 │                                  │
│  内存 ECC CE (频繁)   │  • 随机 Pod OOMKilled          │  • EDAC MC: CE on mc            │
│                       │  • 应用数据错误                 │  • mcelog: corrected error      │
│                       │  • 性能波动                     │                                  │
│                       │                                 │                                  │
│  磁盘故障             │  • PVC 挂载失败                │  • sd X: I/O error              │
│                       │  • Pod ContainerCreating 卡住   │  • blk_update_request: I/O      │
│                       │  • etcd 延迟告警                │  • EXT4-fs error                │
│                       │  • kubelet 日志写入失败         │  • XFS: metadata I/O error      │
│                       │                                 │                                  │
│  NVMe 故障            │  • nvme driver timeout          │  • nvme: I/O error              │
│                       │  • 高延迟 IOPS 下降             │  • nvme: controller fatal       │
│                       │  • containerd 超时              │  • nvme: reset controller       │
│                       │                                 │                                  │
│  网卡故障             │  • Node NetworkUnavailable      │  • link is not ready            │
│                       │  • Service 不可达               │  • watchdog timeout             │
│                       │  • CNI 报错                     │  • netdev_watchdog: TIMEOUT     │
│                       │                                 │                                  │
│  电源故障             │  • Node 突然消失                │  • (无日志-直接断电)            │
│                       │  • etcd 集群选举                │  • watchdog: BUG: soft lockup   │
│                       │  • 数据不一致                   │                                  │
│                       │                                 │                                  │
│  散热故障             │  • CPU throttling 性能下降      │  • CPU temperature above        │
│                       │  • 调度延迟增加                 │  • thermal_zone: critical       │
│                       │  • Node 降频运行                │  • kernel: CPU freq limited     │
│                       │                                 │                                  │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

## Node NotReady 硬件根因分析

### Node NotReady 诊断流程

```yaml
Node_NotReady_硬件诊断:
  Step_1_确认节点状态:
    commands:
      - kubectl get nodes
      - kubectl describe node <node-name>
    关注点:
      - Conditions 中的 Ready 状态
      - LastHeartbeatTime 最后心跳时间
      - Kubelet 报告的原因

  Step_2_检查kubelet进程:
    commands:
      - ssh node "systemctl status kubelet"
      - ssh node "journalctl -u kubelet --since '30 min ago'"
    硬件相关错误:
      - "failed to get node info" → 可能CPU/内存问题
      - "PLEG is not healthy" → 容器运行时或磁盘问题
      - "NodeStatusUnknown" → 网络或严重硬件故障

  Step_3_硬件快速检查:
    CPU检查:
      - ssh node "dmesg | grep -i mce"
      - ssh node "mcelog --client"
    内存检查:
      - ssh node "edac-util -s"
      - ssh node "dmesg | grep -i edac"
    磁盘检查:
      - ssh node "dmesg | grep -i 'I/O error'"
      - ssh node "smartctl -H /dev/sda"
    网络检查:
      - ssh node "ethtool eth0" (如果能连上)
      - 从其他节点 ping 测试
```

### PLEG 故障与硬件关联

```bash
#!/bin/bash
# PLEG故障硬件排查脚本

# PLEG (Pod Lifecycle Event Generator) 故障常与以下硬件问题关联:
# 1. 磁盘I/O延迟过高
# 2. 内存不足或内存错误
# 3. CPU性能问题

echo "=== PLEG 硬件关联诊断 ==="

# 检查kubelet PLEG状态
check_pleg_status() {
    echo "--- Kubelet PLEG 状态 ---"
    journalctl -u kubelet --since "1 hour ago" | grep -i "pleg\|relist"
    
    # PLEG not healthy 通常与以下硬件问题相关:
    # - 磁盘延迟 > 100ms
    # - 内存压力导致 swap
    # - CPU throttling
}

# 检查容器运行时性能
check_runtime_performance() {
    echo -e "\n--- 容器运行时性能 ---"
    
    # containerd 健康检查
    if systemctl is-active containerd &>/dev/null; then
        crictl info | jq '.status'
        
        # 检查 containerd 日志中的硬件相关错误
        journalctl -u containerd --since "1 hour ago" | grep -iE "timeout|error|i/o"
    fi
}

# 磁盘延迟检查 (PLEG 对磁盘敏感)
check_disk_latency() {
    echo -e "\n--- 磁盘延迟检查 ---"
    
    # containerd 数据目录
    CONTAINERD_ROOT="/var/lib/containerd"
    
    # 使用 ioping 测试延迟
    if command -v ioping &>/dev/null; then
        echo "测试 containerd 目录延迟:"
        ioping -c 10 "$CONTAINERD_ROOT"
    fi
    
    # iostat 检查
    iostat -x 1 5 | grep -E "Device|sd|nvme"
    
    # 高延迟阈值: await > 50ms 可能导致 PLEG 问题
}

# 内存压力检查
check_memory_pressure() {
    echo -e "\n--- 内存压力检查 ---"
    
    # 检查 swap 使用
    free -h
    vmstat 1 5
    
    # 检查 cgroup 内存压力
    if [ -f /sys/fs/cgroup/memory/memory.pressure_level ]; then
        echo "Memory Pressure: $(cat /sys/fs/cgroup/memory/memory.pressure_level)"
    fi
    
    # ECC 错误检查
    edac-util -s 2>/dev/null
}

# CPU 性能检查
check_cpu_throttling() {
    echo -e "\n--- CPU 性能检查 ---"
    
    # 检查 CPU 降频
    for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq; do
        [ -f "$cpu" ] && echo "$cpu: $(cat $cpu)"
    done
    
    # 检查热节流
    grep -r . /sys/devices/system/cpu/cpu*/thermal_throttle/ 2>/dev/null
    
    # MCE 错误
    dmesg | grep -i mce | tail -10
}

# 主诊断流程
main() {
    check_pleg_status
    check_runtime_performance
    check_disk_latency
    check_memory_pressure
    check_cpu_throttling
    
    echo -e "\n=== 诊断完成 ==="
    echo "PLEG 问题常见硬件原因:"
    echo "  1. 磁盘 await > 50ms"
    echo "  2. 内存使用 > 90% 且有 swap 活动"
    echo "  3. CPU throttling 或 MCE 错误"
    echo "  4. ECC 内存错误累积"
}

main
```

## Pod 异常与硬件故障关联

### Pod OOMKilled 硬件误判分析

```yaml
OOMKilled_硬件误判场景:
  场景1_ECC内存错误:
    表现:
      - Pod 频繁 OOMKilled
      - 但 kubectl top pod 显示内存未满
      - 重启后随机再次 OOMKilled
    
    根因分析:
      - 内存 ECC 错误导致可用内存减少
      - 内核将故障页面隔离
      - 实际可用内存小于显示值
    
    诊断命令:
      dmesg | grep -iE "corrected|uncorrected"
      edac-util -s
      cat /proc/meminfo | grep -i hardware
    
    证据特征:
      - HardwareCorrupted 字段 > 0
      - EDAC 错误计数增加
      - mcelog 记录内存错误
      
  场景2_NUMA内存不均衡:
    表现:
      - 特定 Node 上 Pod 容易 OOM
      - 整体内存看起来充足
      - 只有某些 Pod 受影响
    
    根因分析:
      - Pod 被调度到内存不足的 NUMA node
      - 内存绑定策略问题
    
    诊断命令:
      numactl --hardware
      numastat -c
      cat /proc/buddyinfo
      
  场景3_内存碎片化:
    表现:
      - 大内存请求失败
      - 小内存请求正常
      - 内存总量充足
    
    诊断命令:
      cat /proc/buddyinfo
      echo 3 > /proc/sys/vm/drop_caches  # 清理缓存
      cat /proc/pagetypeinfo
```

### Pod CrashLoopBackOff 硬件排查

```bash
#!/bin/bash
# Pod CrashLoop 硬件关联排查

POD_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$POD_NAME" ]; then
    echo "Usage: $0 <pod-name> [namespace]"
    exit 1
fi

echo "=== Pod $POD_NAME 硬件关联排查 ==="

# 获取 Pod 所在节点
NODE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.nodeName}')
echo "Pod 运行节点: $NODE"

# 获取 Pod 事件
echo -e "\n--- Pod 事件 ---"
kubectl describe pod "$POD_NAME" -n "$NAMESPACE" | grep -A 20 "Events:"

# 获取容器退出原因
echo -e "\n--- 容器退出原因 ---"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[*].lastState.terminated}' | jq .

# 检查节点硬件状态
echo -e "\n--- 节点 $NODE 硬件状态 ---"
ssh "$NODE" << 'EOF'
echo "=== 内存状态 ==="
free -h
edac-util -s 2>/dev/null || echo "EDAC not available"

echo -e "\n=== 磁盘状态 ==="
df -h /var/lib/containerd /var/lib/kubelet
iostat -x 1 3 2>/dev/null | tail -10

echo -e "\n=== 最近硬件错误 ==="
dmesg | grep -iE "mce|edac|i/o error|hardware error" | tail -10

echo -e "\n=== CPU 状态 ==="
cat /proc/cpuinfo | grep -E "processor|model name|cpu MHz" | head -10
grep -r . /sys/devices/system/cpu/cpu0/thermal_throttle/ 2>/dev/null
EOF
```

## etcd 硬件故障影响

### etcd 磁盘性能要求

```yaml
etcd_硬件要求:
  磁盘延迟:
    建议值: fsync延迟 < 10ms
    告警阈值: > 25ms
    临界阈值: > 100ms (可能导致选举超时)
    
  检查方法:
    # 使用 fio 测试
    fio --name=fsync_test --filename=/var/lib/etcd/test \
        --rw=write --bs=4k --numjobs=1 --size=100M \
        --sync=1 --direct=1 --runtime=30 --time_based
    
    # 使用 etcd 自带 benchmark
    etcdctl check perf --load="s"
    
  性能下降原因:
    - NVMe/SSD 老化
    - RAID 卡电池故障 (写缓存被禁用)
    - 其他 I/O 争用
    - 磁盘即将故障

etcd_磁盘故障症状:
  日志特征:
    - "apply request took too long"
    - "failed to send out heartbeat"
    - "wal: sync duration exceeded"
    - "etcdserver: request timed out"
    
  集群表现:
    - Leader 频繁切换
    - API Server 响应慢
    - kubectl 命令超时
    - 配置变更不生效
```

### etcd 硬件故障诊断

```bash
#!/bin/bash
# etcd 硬件故障诊断脚本

ETCD_DATA_DIR="${ETCD_DATA_DIR:-/var/lib/etcd}"

echo "=== etcd 硬件故障诊断 ==="

# 检查 etcd 数据目录磁盘
check_etcd_disk() {
    echo "--- etcd 数据目录磁盘状态 ---"
    
    # 获取磁盘设备
    local disk_device=$(df "$ETCD_DATA_DIR" | tail -1 | awk '{print $1}')
    echo "数据目录: $ETCD_DATA_DIR"
    echo "磁盘设备: $disk_device"
    
    # 磁盘空间
    df -h "$ETCD_DATA_DIR"
    
    # SMART 状态
    local base_device=$(echo "$disk_device" | sed 's/[0-9]*$//')
    if [[ "$base_device" == *"nvme"* ]]; then
        nvme smart-log "$base_device" 2>/dev/null | grep -E "critical|temperature|percentage"
    else
        smartctl -H "$base_device" 2>/dev/null
    fi
}

# 磁盘延迟测试
test_disk_latency() {
    echo -e "\n--- 磁盘延迟测试 ---"
    
    # 简单 fsync 测试
    local test_file="$ETCD_DATA_DIR/latency_test_$$"
    
    echo "测试 fsync 延迟 (10次):"
    for i in {1..10}; do
        local start=$(date +%s%N)
        dd if=/dev/zero of="$test_file" bs=4k count=1 conv=fsync 2>/dev/null
        local end=$(date +%s%N)
        local latency=$(( (end - start) / 1000000 ))
        echo "  第${i}次: ${latency}ms"
    done
    rm -f "$test_file"
    
    # ioping 测试
    if command -v ioping &>/dev/null; then
        echo -e "\nioping 测试:"
        ioping -c 20 -i 0.1 "$ETCD_DATA_DIR"
    fi
}

# 检查 etcd 日志中的硬件相关错误
check_etcd_logs() {
    echo -e "\n--- etcd 日志硬件相关错误 ---"
    
    # 查找性能问题日志
    journalctl -u etcd --since "1 hour ago" 2>/dev/null | \
        grep -iE "took too long|sync duration|slow|timeout|disk" | tail -20
    
    # 或检查容器日志
    if kubectl get pods -n kube-system -l component=etcd &>/dev/null; then
        echo "--- etcd Pod 日志 ---"
        kubectl logs -n kube-system -l component=etcd --tail=50 2>/dev/null | \
            grep -iE "took too long|sync duration|slow|timeout"
    fi
}

# etcd 集群状态
check_etcd_cluster() {
    echo -e "\n--- etcd 集群状态 ---"
    
    ETCDCTL_API=3 etcdctl endpoint status --cluster -w table 2>/dev/null
    ETCDCTL_API=3 etcdctl endpoint health --cluster 2>/dev/null
}

# 执行检查
check_etcd_disk
test_disk_latency
check_etcd_logs
check_etcd_cluster

echo -e "\n=== etcd 硬件诊断建议 ==="
echo "1. fsync 延迟应 < 10ms, 超过 25ms 需要关注"
echo "2. 使用 NVMe SSD, 避免与其他 I/O 密集应用共享"
echo "3. RAID 卡需确保 BBU/电容正常 (写缓存生效)"
echo "4. 定期检查 SMART 状态和磁盘健康度"
```

## 硬件故障下的 K8s 应急响应

### Node 硬件故障应急流程

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                    Node 硬件故障应急响应流程                                               │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Phase 1: 快速隔离 (0-5分钟)                              │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  1. 确认节点状态                                                           │ │   │
│  │  │     kubectl get node `<node>` -o wide                                       │ │   │
│  │  │                                                                            │ │   │
│  │  │  2. 立即隔离节点 (阻止新 Pod 调度)                                         │ │   │
│  │  │     kubectl cordon `<node>`                                                 │ │   │
│  │  │                                                                            │ │   │
│  │  │  3. 如果确认硬件故障严重，驱逐工作负载                                     │ │   │
│  │  │     kubectl drain `<node>` --ignore-daemonsets --delete-emptydir-data      │ │   │
│  │  │     --force --grace-period=30                                            │ │   │
│  │  │                                                                            │ │   │
│  │  │  4. 通知相关团队                                                          │ │   │
│  │  │     - 应用负责人                                                          │ │   │
│  │  │     - 硬件运维团队                                                        │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                           │                                             │
│                                           ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Phase 2: 影响评估 (5-15分钟)                             │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  1. 检查受影响的 Pod                                                       │ │   │
│  │  │     kubectl get pods --all-namespaces -o wide | grep `<node>`              │ │   │
│  │  │                                                                            │ │   │
│  │  │  2. 检查重要服务状态                                                       │ │   │
│  │  │     - etcd 集群状态                                                       │ │   │
│  │  │     - StatefulSet 状态                                                    │ │   │
│  │  │     - 存储 PV/PVC 状态                                                    │ │   │
│  │  │                                                                            │ │   │
│  │  │  3. 确认 Pod 是否成功迁移                                                  │ │   │
│  │  │     kubectl get pods -o wide | grep -v `<node>`                            │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                           │                                             │
│                                           ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Phase 3: 硬件诊断 (15-60分钟)                            │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  1. 收集硬件诊断信息                                                       │ │   │
│  │  │     - BMC/IPMI SEL 日志                                                   │ │   │
│  │  │     - dmesg 内核日志                                                      │ │   │
│  │  │     - mcelog/EDAC 错误                                                    │ │   │
│  │  │     - SMART 磁盘状态                                                      │ │   │
│  │  │                                                                            │ │   │
│  │  │  2. 确定故障组件                                                          │ │   │
│  │  │     - CPU/内存/磁盘/网卡/电源/主板                                        │ │   │
│  │  │                                                                            │ │   │
│  │  │  3. 决定处理方式                                                          │ │   │
│  │  │     - 在线修复 (软件/配置问题)                                            │ │   │
│  │  │     - 硬件更换 (需停机)                                                   │ │   │
│  │  │     - 报废替换 (整机故障)                                                 │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                           │                                             │
│                                           ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Phase 4: 恢复与验证 (按需)                               │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  1. 硬件修复后                                                             │ │   │
│  │  │     - 运行硬件诊断测试                                                    │ │   │
│  │  │     - 验证 kubelet 正常启动                                               │ │   │
│  │  │                                                                            │ │   │
│  │  │  2. 恢复节点                                                              │ │   │
│  │  │     kubectl uncordon `<node>`                                               │ │   │
│  │  │                                                                            │ │   │
│  │  │  3. 监控节点状态                                                          │ │   │
│  │  │     - 观察 30 分钟无异常                                                  │ │   │
│  │  │     - 检查工作负载正常运行                                                │ │   │
│  │  │                                                                            │ │   │
│  │  │  4. 事后复盘                                                              │ │   │
│  │  │     - 编写故障报告                                                        │ │   │
│  │  │     - 更新监控告警                                                        │ │   │
│  │  │     - 完善预防措施                                                        │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 批量节点硬件检查脚本

```bash
#!/bin/bash
# K8s 集群节点硬件批量检查脚本

# 获取所有节点
NODES=$(kubectl get nodes -o jsonpath='{.items[*].metadata.name}')

# 检查结果汇总
declare -A RESULTS

check_node_hardware() {
    local node=$1
    echo "=== 检查节点: $node ==="
    
    # 通过 SSH 或 kubectl debug 执行检查
    local check_script='
        echo "--- CPU MCE ---"
        dmesg 2>/dev/null | grep -c -i mce || echo 0
        
        echo "--- Memory ECC ---"
        edac-util -s 2>/dev/null | grep -E "^[a-z]" || echo "No EDAC"
        
        echo "--- Disk Health ---"
        for disk in /dev/sd?; do
            [ -b "$disk" ] && smartctl -H "$disk" 2>/dev/null | grep -E "PASSED|FAILED" || true
        done
        for nvme in /dev/nvme?; do
            [ -c "$nvme" ] && nvme smart-log "$nvme" 2>/dev/null | grep -E "critical_warning|percentage_used" || true
        done
        
        echo "--- Temperature ---"
        sensors 2>/dev/null | grep -E "Core|temp" | head -5 || echo "sensors not available"
        
        echo "--- IPMI SEL (Recent) ---"
        ipmitool sel elist 2>/dev/null | tail -5 || echo "IPMI not available"
    '
    
    # 使用 kubectl debug (K8s 1.25+)
    kubectl debug node/"$node" -it --image=busybox -- sh -c "$check_script" 2>/dev/null
    
    # 或直接 SSH
    # ssh "$node" "$check_script"
}

generate_report() {
    echo ""
    echo "========================================"
    echo "        集群硬件健康报告"
    echo "========================================"
    echo "检查时间: $(date)"
    echo "节点总数: $(echo $NODES | wc -w)"
    echo ""
    
    for node in $NODES; do
        echo "--- $node ---"
        check_node_hardware "$node"
        echo ""
    done
}

# 执行检查
generate_report
```

## K8s 监控集成

### Prometheus 硬件监控指标

```yaml
# Prometheus 硬件监控规则
groups:
  - name: kubernetes_hardware_alerts
    rules:
      # CPU MCE 错误告警
      - alert: NodeMCEError
        expr: increase(node_edac_correctable_errors_total[1h]) > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Node {{ $labels.instance }} MCE errors detected"
          description: "节点 {{ $labels.instance }} 检测到 CPU MCE 错误"
          
      - alert: NodeMCECritical
        expr: increase(node_edac_uncorrectable_errors_total[1h]) > 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Node {{ $labels.instance }} uncorrectable MCE error"
          description: "节点 {{ $labels.instance }} 检测到不可纠正 MCE 错误，需要立即处理"
      
      # 内存 ECC 错误告警
      - alert: NodeMemoryECCWarning
        expr: increase(node_edac_correctable_errors_total{controller="mc*"}[1h]) > 100
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Node {{ $labels.instance }} memory ECC errors"
          description: "节点 {{ $labels.instance }} 内存 ECC 错误增加，建议计划更换 DIMM"
          
      - alert: NodeMemoryECCCritical
        expr: node_edac_uncorrectable_errors_total{controller="mc*"} > 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Node {{ $labels.instance }} memory UCE error"
          description: "节点 {{ $labels.instance }} 内存不可纠正错误，立即隔离节点"
      
      # 磁盘 SMART 告警
      - alert: NodeDiskSMARTWarning
        expr: smartmon_device_smart_healthy == 0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Disk {{ $labels.disk }} SMART unhealthy on {{ $labels.instance }}"
          description: "节点 {{ $labels.instance }} 磁盘 {{ $labels.disk }} SMART 状态异常"
          
      - alert: NodeDiskReallocatedSectors
        expr: smartmon_reallocated_sector_ct_raw_value > 100
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Disk {{ $labels.disk }} has reallocated sectors"
          description: "磁盘 {{ $labels.disk }} 存在重映射扇区，建议备份数据并更换"
      
      # NVMe 寿命告警
      - alert: NodeNVMeLifeWarning
        expr: nvme_percentage_used > 80
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "NVMe {{ $labels.device }} life {{ $value }}% used"
          description: "NVMe {{ $labels.device }} 寿命已使用 {{ $value }}%，建议计划更换"
      
      # CPU 温度告警
      - alert: NodeCPUTempWarning
        expr: node_hwmon_temp_celsius{sensor=~".*core.*"} > 80
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Node {{ $labels.instance }} CPU temperature high"
          description: "节点 {{ $labels.instance }} CPU 温度 {{ $value }}°C 过高"
          
      - alert: NodeCPUTempCritical
        expr: node_hwmon_temp_celsius{sensor=~".*core.*"} > 90
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "Node {{ $labels.instance }} CPU temperature critical"
          description: "节点 {{ $labels.instance }} CPU 温度 {{ $value }}°C 达到临界值"
      
      # 磁盘延迟告警 (影响 etcd)
      - alert: NodeDiskLatencyHigh
        expr: rate(node_disk_write_time_seconds_total[5m]) / rate(node_disk_writes_completed_total[5m]) > 0.1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Node {{ $labels.instance }} disk latency high"
          description: "节点 {{ $labels.instance }} 磁盘写入延迟过高，可能影响 etcd"
```

### Node Problem Detector 配置

```yaml
# Node Problem Detector 硬件问题检测配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-problem-detector-config
  namespace: kube-system
data:
  kernel-monitor.json: |
    {
      "plugin": "kmsg",
      "logPath": "/dev/kmsg",
      "lookback": "5m",
      "bufferSize": 10,
      "source": "kernel-monitor",
      "conditions": [
        {
          "type": "KernelDeadlock",
          "reason": "KernelHasNoDeadlock",
          "message": "kernel has no deadlock"
        },
        {
          "type": "ReadonlyFilesystem",
          "reason": "FilesystemIsNotReadOnly",
          "message": "Filesystem is not read-only"
        },
        {
          "type": "MemoryHardwareError",
          "reason": "MemoryHardwareHealthy",
          "message": "Memory hardware is healthy"
        },
        {
          "type": "CPUHardwareError",
          "reason": "CPUHardwareHealthy",
          "message": "CPU hardware is healthy"
        }
      ],
      "rules": [
        {
          "type": "permanent",
          "condition": "KernelDeadlock",
          "reason": "AUFSUmountHung",
          "pattern": "task aufs_destroy:.*blocked for more than \\d+ seconds\\."
        },
        {
          "type": "permanent",
          "condition": "ReadonlyFilesystem",
          "reason": "FilesystemIsReadOnly",
          "pattern": "Remounting filesystem read-only"
        },
        {
          "type": "permanent",
          "condition": "MemoryHardwareError",
          "reason": "MemoryECCError",
          "pattern": "EDAC.*CE.*|EDAC.*UE.*|Memory failure.*"
        },
        {
          "type": "permanent",
          "condition": "CPUHardwareError",
          "reason": "CPUMCEError",
          "pattern": "mce:.*Hardware Error|Machine check events logged|CPU.*Machine Check"
        },
        {
          "type": "temporary",
          "reason": "IOError",
          "pattern": "Buffer I/O error.*|end_request: I/O error.*|blk_update_request: I/O error"
        },
        {
          "type": "temporary",
          "reason": "NVMeError",
          "pattern": "nvme.*I/O error|nvme.*timeout|nvme.*controller fatal"
        },
        {
          "type": "temporary",
          "reason": "NetworkDriverError",
          "pattern": "NETDEV WATCHDOG.*transmit queue.*timed out|link is not ready"
        }
      ]
    }
```

## 参考资源

### 命令速查表

| 场景 | 命令 |
|------|------|
| 节点隔离 | `kubectl cordon <node>` |
| 驱逐工作负载 | `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` |
| 恢复节点 | `kubectl uncordon <node>` |
| 查看节点事件 | `kubectl describe node <node> \| grep -A 20 Events` |
| 查看节点硬件条件 | `kubectl get node <node> -o jsonpath='{.status.conditions}'` |
| 检查 kubelet 日志 | `journalctl -u kubelet --since "30 min ago"` |
| 检查 containerd 日志 | `journalctl -u containerd --since "30 min ago"` |
| MCE 错误检查 | `dmesg | grep -i mce` |
| 内存 ECC 检查 | `edac-util -s` |
| 磁盘 SMART 检查 | `smartctl -H /dev/sda` |
| NVMe 健康检查 | `nvme smart-log /dev/nvme0` |
| IPMI 事件日志 | `ipmitool sel elist` |

### 相关文档

- [10-hardware-troubleshooting-methodology.md](./10-hardware-troubleshooting-methodology.md) - 硬件故障排查方法论
- [11-cpu-memory-troubleshooting.md](./11-cpu-memory-troubleshooting.md) - CPU与内存故障排查
- [12-storage-troubleshooting.md](./12-storage-troubleshooting.md) - 存储设备故障排查
- [domain-12-troubleshooting](../domain-12-troubleshooting/) - K8s 故障排查专题
