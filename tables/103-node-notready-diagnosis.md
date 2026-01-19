# 87 - 节点NotReady状态诊断

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/architecture/nodes](https://kubernetes.io/docs/concepts/architecture/nodes/)

## NotReady状态架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Node NotReady 状态检测与处理架构                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    ┌─────────────────────────────────────────────────────────────────┐     │
│    │                      API Server                                  │     │
│    │  ┌─────────────────────────────────────────────────────────┐   │     │
│    │  │ Node Controller                                          │   │     │
│    │  │ • 监控节点心跳(Lease/NodeStatus)                         │   │     │
│    │  │ • 标记NotReady状态                                       │   │     │
│    │  │ • 触发Pod驱逐(--pod-eviction-timeout)                   │   │     │
│    │  │ • 更新节点Taint                                          │   │     │
│    │  └─────────────────────────────────────────────────────────┘   │     │
│    └────────────────────────────┬────────────────────────────────────┘     │
│                                 │                                           │
│              监控Lease对象 ←────┴────→ 检查NodeStatus                       │
│                   │                          │                              │
│                   ▼                          ▼                              │
│    ┌─────────────────────┐    ┌─────────────────────────────────────────┐  │
│    │   Lease (kube-node- │    │           NodeStatus                    │  │
│    │   lease namespace)  │    │  ┌─────────────────────────────────┐   │  │
│    │  ┌───────────────┐  │    │  │ Conditions:                     │   │  │
│    │  │ • holderID    │  │    │  │   Ready            (True/False) │   │  │
│    │  │ • renewTime   │  │    │  │   MemoryPressure   (True/False) │   │  │
│    │  │ • leaseDur:40s│  │    │  │   DiskPressure     (True/False) │   │  │
│    │  └───────────────┘  │    │  │   PIDPressure      (True/False) │   │  │
│    └─────────────────────┘    │  │   NetworkUnavailable(True/False)│   │  │
│                               │  └─────────────────────────────────┘   │  │
│                               └─────────────────────────────────────────┘  │
│                                                                             │
│    ┌─────────────────────────── Worker Node ────────────────────────────┐  │
│    │                                                                     │  │
│    │   ┌─────────────────────────────────────────────────────────┐      │  │
│    │   │                        kubelet                           │      │  │
│    │   │  ┌─────────────────────────────────────────────────┐    │      │  │
│    │   │  │ 心跳上报:                                        │    │      │  │
│    │   │  │ • 更新Lease: 每10秒(默认)                        │    │      │  │
│    │   │  │ • 更新NodeStatus: 每5分钟或状态变化时            │    │      │  │
│    │   │  │ • nodeMonitorPeriod: 5s                          │    │      │  │
│    │   │  │ • nodeMonitorGracePeriod: 40s                    │    │      │  │
│    │   │  └─────────────────────────────────────────────────┘    │      │  │
│    │   │                                                          │      │  │
│    │   │  依赖组件:                                               │      │  │
│    │   │  ┌─────────────┐ ┌────────────────┐ ┌────────────────┐  │      │  │
│    │   │  │ Container   │ │    CNI         │ │   System       │  │      │  │
│    │   │  │ Runtime     │ │    Plugin      │ │   Resources    │  │      │  │
│    │   │  │ containerd  │ │  calico/cni    │ │  CPU/Mem/Disk  │  │      │  │
│    │   │  └──────┬──────┘ └───────┬────────┘ └───────┬────────┘  │      │  │
│    │   │         │                │                   │           │      │  │
│    │   │         ▼                ▼                   ▼           │      │  │
│    │   │  ┌─────────────────────────────────────────────────┐    │      │  │
│    │   │  │              故障检测点                           │    │      │  │
│    │   │  │ • 运行时状态    • CNI健康检查    • 资源压力      │    │      │  │
│    │   │  └─────────────────────────────────────────────────┘    │      │  │
│    │   └─────────────────────────────────────────────────────────┘      │  │
│    └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│    ┌─────────────────────── NotReady 触发时间线 ─────────────────────────┐  │
│    │                                                                      │  │
│    │   kubelet停止心跳                                                    │  │
│    │         │                                                            │  │
│    │         ├──── 10s ────→ Lease过期(kubelet未更新)                    │  │
│    │         │                                                            │  │
│    │         ├──── 40s ────→ Node Controller标记Unknown                  │  │
│    │         │                (nodeMonitorGracePeriod)                    │  │
│    │         │                                                            │  │
│    │         ├──── 5m  ────→ 添加NoSchedule Taint                        │  │
│    │         │                (node.kubernetes.io/unreachable)            │  │
│    │         │                                                            │  │
│    │         └──── 5m+ ───→ 开始驱逐Pod                                  │  │
│    │                         (pod-eviction-timeout)                       │  │
│    │                                                                      │  │
│    └──────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Node Condition状态详解

| Condition | 正常值 | 异常含义 | 检测机制 | 影响 | 恢复时间 |
|-----------|-------|---------|---------|------|---------|
| **Ready** | True | kubelet不健康或无法与API Server通信 | Lease更新/NodeStatus | Pod无法调度,触发驱逐 | 心跳恢复后40s |
| **MemoryPressure** | False | 节点内存低于eviction阈值 | kubelet内存监控 | 可能驱逐BestEffort Pod | 内存释放后立即 |
| **DiskPressure** | False | 节点磁盘空间不足 | kubelet磁盘监控 | 可能驱逐Pod,阻止调度 | 磁盘释放后立即 |
| **PIDPressure** | False | 节点进程数接近限制 | kubelet PID监控 | 可能驱逐Pod | PID释放后立即 |
| **NetworkUnavailable** | False | 节点网络未正确配置 | CNI插件报告 | Pod网络故障 | CNI修复后 |

### Condition状态转换

```yaml
# 节点Condition结构示例
status:
  conditions:
  - type: Ready
    status: "True"                              # True/False/Unknown
    lastHeartbeatTime: "2026-01-19T10:00:00Z"  # 最后心跳时间
    lastTransitionTime: "2026-01-18T08:00:00Z" # 状态变更时间
    reason: KubeletReady                        # 状态原因
    message: "kubelet is posting ready status"  # 详细信息
  - type: MemoryPressure
    status: "False"
    lastHeartbeatTime: "2026-01-19T10:00:00Z"
    lastTransitionTime: "2026-01-18T08:00:00Z"
    reason: KubeletHasSufficientMemory
    message: "kubelet has sufficient memory available"
```

## NotReady常见原因分类

### 原因分类矩阵

| 原因类别 | 具体原因 | 发生频率 | 影响范围 | 紧急程度 | 恢复难度 |
|---------|---------|---------|---------|---------|---------|
| **kubelet问题** | kubelet进程崩溃 | 高 | 单节点 | P0 | 低 |
| **kubelet问题** | kubelet配置错误 | 中 | 单节点 | P1 | 中 |
| **kubelet问题** | kubelet版本不兼容 | 低 | 多节点 | P1 | 中 |
| **容器运行时** | containerd故障 | 中 | 单节点 | P0 | 低 |
| **容器运行时** | 运行时OOM | 高 | 单节点 | P1 | 中 |
| **容器运行时** | shim进程泄漏 | 中 | 单节点 | P2 | 中 |
| **网络问题** | CNI插件故障 | 高 | 多节点 | P0 | 中 |
| **网络问题** | 节点与API Server不通 | 中 | 单节点 | P0 | 高 |
| **网络问题** | DNS解析失败 | 中 | 单节点 | P1 | 低 |
| **证书问题** | kubelet证书过期 | 低 | 多节点 | P1 | 中 |
| **证书问题** | CA证书不匹配 | 低 | 多节点 | P0 | 高 |
| **资源耗尽** | 内存耗尽 | 高 | 单节点 | P0 | 中 |
| **资源耗尽** | 磁盘满 | 高 | 单节点 | P1 | 低 |
| **资源耗尽** | inode耗尽 | 中 | 单节点 | P1 | 低 |
| **内核问题** | 内核panic | 低 | 单节点 | P0 | 高 |
| **内核问题** | 内核死锁 | 低 | 单节点 | P0 | 高 |

### 根因分析决策树

```
                          Node NotReady
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
         能SSH到节点?      节点物理存活?    多节点同时NotReady?
              │                │                │
         ┌────┴────┐      ┌────┴────┐      ┌────┴────┐
         │Yes      │No    │Yes      │No    │Yes      │No
         │         │      │         │      │         │
    检查kubelet   检查    检查      检查    检查      继续
    和运行时     网络/    IPMI/     硬件    控制平面   单节点
                云平台   console          /网络     排查
```

## 完整诊断流程

### 第一阶段: 快速状态检查

```bash
#!/bin/bash
# node-notready-quick-check.sh - 快速诊断脚本

NODE_NAME=${1:-""}

echo "====== Node NotReady 快速诊断 ======"
echo "时间: $(date)"
echo "目标节点: ${NODE_NAME:-所有NotReady节点}"
echo ""

# 1. 列出所有NotReady节点
echo "=== 1. NotReady节点列表 ==="
kubectl get nodes -o wide | grep -v " Ready"

# 2. 获取节点详细Condition
if [ -n "$NODE_NAME" ]; then
    echo -e "\n=== 2. 节点 $NODE_NAME Conditions ==="
    kubectl get node "$NODE_NAME" -o jsonpath='{range .status.conditions[*]}{.type}{"\t"}{.status}{"\t"}{.reason}{"\t"}{.message}{"\n"}{end}' | column -t
    
    # 3. 检查Lease
    echo -e "\n=== 3. 节点Lease状态 ==="
    kubectl get lease -n kube-node-lease "$NODE_NAME" -o yaml 2>/dev/null | grep -E "renewTime|holderIdentity|leaseDurationSeconds"
    
    # 4. 最近事件
    echo -e "\n=== 4. 节点相关事件 ==="
    kubectl get events -A --field-selector involvedObject.name="$NODE_NAME" --sort-by='.lastTimestamp' | tail -20
    
    # 5. 节点上的Pod状态
    echo -e "\n=== 5. 节点上的Pod状态 ==="
    kubectl get pods -A -o wide --field-selector spec.nodeName="$NODE_NAME" | grep -v Running | head -20
fi

# 6. 控制平面组件状态(对比)
echo -e "\n=== 6. kube-system组件状态 ==="
kubectl get pods -n kube-system -o wide | grep -E "coredns|calico|cilium|kube-proxy" | head -10
```

### 第二阶段: 节点内部诊断

```bash
#!/bin/bash
# node-internal-diagnosis.sh - 节点内部诊断(需SSH到节点执行)

echo "====== 节点内部诊断 ======"
echo "主机名: $(hostname)"
echo "时间: $(date)"
echo ""

# 1. kubelet状态检查
echo "=== 1. kubelet进程状态 ==="
systemctl status kubelet --no-pager | head -20
echo ""

echo "kubelet进程信息:"
ps aux | grep kubelet | grep -v grep
echo ""

# 检查kubelet是否能正常启动
echo "kubelet启动失败原因(如有):"
systemctl is-failed kubelet && journalctl -u kubelet -n 50 --no-pager | grep -E "error|Error|ERROR|failed|Failed"

# 2. 容器运行时状态
echo -e "\n=== 2. 容器运行时状态 ==="
if systemctl is-active containerd &>/dev/null; then
    echo "containerd状态: $(systemctl is-active containerd)"
    echo ""
    echo "containerd详情:"
    crictl info 2>/dev/null | head -30
    echo ""
    echo "运行中容器数量: $(crictl ps -q 2>/dev/null | wc -l)"
    echo "所有容器数量: $(crictl ps -aq 2>/dev/null | wc -l)"
else
    echo "containerd未运行!"
    systemctl status containerd --no-pager | head -10
fi

# 3. 系统资源状态
echo -e "\n=== 3. 系统资源状态 ==="
echo "--- 内存 ---"
free -h
echo ""
echo "内存详情:"
cat /proc/meminfo | grep -E "MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree"

echo -e "\n--- 磁盘 ---"
df -h | grep -E "Filesystem|/$|/var"
echo ""
echo "inode使用:"
df -i | grep -E "Filesystem|/$|/var"

echo -e "\n--- CPU负载 ---"
uptime
echo ""
echo "CPU使用Top5进程:"
ps aux --sort=-%cpu | head -6

echo -e "\n--- 进程数 ---"
echo "当前进程数: $(ps aux | wc -l)"
echo "系统PID最大值: $(cat /proc/sys/kernel/pid_max)"

# 4. 网络状态
echo -e "\n=== 4. 网络状态 ==="
echo "--- 网络接口 ---"
ip addr show | grep -E "^[0-9]+:|inet "

echo -e "\n--- 默认路由 ---"
ip route show default

echo -e "\n--- API Server连通性 ---"
API_SERVER=$(cat /etc/kubernetes/kubelet.conf 2>/dev/null | grep server | awk '{print $2}')
if [ -n "$API_SERVER" ]; then
    echo "API Server: $API_SERVER"
    curl -sk --connect-timeout 5 "${API_SERVER}/healthz" && echo " (健康)" || echo " (无法连接)"
fi

# 5. 证书状态
echo -e "\n=== 5. 证书状态 ==="
echo "--- kubelet客户端证书 ---"
KUBELET_CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
if [ -f "$KUBELET_CERT" ]; then
    echo "证书路径: $KUBELET_CERT"
    openssl x509 -in "$KUBELET_CERT" -noout -dates 2>/dev/null
    
    # 计算证书剩余天数
    EXPIRY=$(openssl x509 -in "$KUBELET_CERT" -noout -enddate 2>/dev/null | cut -d= -f2)
    if [ -n "$EXPIRY" ]; then
        EXPIRY_EPOCH=$(date -d "$EXPIRY" +%s 2>/dev/null || date -j -f "%b %d %H:%M:%S %Y %Z" "$EXPIRY" +%s 2>/dev/null)
        NOW_EPOCH=$(date +%s)
        DAYS_LEFT=$(( (EXPIRY_EPOCH - NOW_EPOCH) / 86400 ))
        echo "证书剩余天数: $DAYS_LEFT 天"
        if [ $DAYS_LEFT -lt 30 ]; then
            echo "⚠️ 警告: 证书即将过期!"
        fi
    fi
else
    echo "未找到kubelet证书文件"
fi

# 6. CNI状态
echo -e "\n=== 6. CNI插件状态 ==="
echo "--- CNI配置 ---"
ls -la /etc/cni/net.d/ 2>/dev/null || echo "CNI配置目录不存在"

echo -e "\n--- CNI二进制 ---"
ls -la /opt/cni/bin/ 2>/dev/null | head -10 || echo "CNI二进制目录不存在"

# 7. 系统日志检查
echo -e "\n=== 7. 关键系统日志 ==="
echo "--- 最近的内核错误 ---"
dmesg | tail -50 | grep -iE "error|fail|oom|kill|panic" | tail -10

echo -e "\n--- kubelet最近错误 ---"
journalctl -u kubelet --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail" | tail -20

echo -e "\n====== 诊断完成 ======"
```

### 第三阶段: 深度问题分析

```bash
#!/bin/bash
# deep-analysis.sh - 深度问题分析

echo "====== 深度问题分析 ======"

# 1. kubelet详细日志分析
echo "=== 1. kubelet日志模式分析 ==="

echo "--- PLEG相关错误 ---"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep -c "PLEG"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep "PLEG" | tail -5

echo -e "\n--- 节点状态更新失败 ---"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep -c "failed to update node"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep "failed to update node" | tail -5

echo -e "\n--- 证书相关错误 ---"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep -iE "certificate|x509|tls" | tail -5

echo -e "\n--- 容器运行时错误 ---"
journalctl -u kubelet --since "1 hour ago" --no-pager 2>/dev/null | grep -iE "rpc error|context deadline|runtime" | tail -5

# 2. containerd详细分析
echo -e "\n=== 2. containerd状态分析 ==="

echo "--- containerd内存使用 ---"
CONTAINERD_PID=$(pgrep -x containerd)
if [ -n "$CONTAINERD_PID" ]; then
    ps -p $CONTAINERD_PID -o pid,rss,vsz,%mem,%cpu,etime
    echo ""
    echo "containerd打开文件数: $(ls /proc/$CONTAINERD_PID/fd 2>/dev/null | wc -l)"
fi

echo -e "\n--- shim进程统计 ---"
ps aux | grep containerd-shim | grep -v grep | wc -l
echo "shim进程数量: $(ps aux | grep containerd-shim | grep -v grep | wc -l)"

echo -e "\n--- 僵尸容器检查 ---"
crictl ps -a 2>/dev/null | grep -v "Running\|Created" | head -10

# 3. 系统资源深度分析
echo -e "\n=== 3. 系统资源深度分析 ==="

echo "--- 内存分页统计 ---"
cat /proc/vmstat | grep -E "pgfault|pgmajfault|pswpin|pswpout"

echo -e "\n--- OOM历史 ---"
dmesg | grep -i "oom\|killed process" | tail -10

echo -e "\n--- 磁盘IO统计 ---"
iostat -x 1 3 2>/dev/null | tail -20 || echo "iostat不可用"

# 4. 网络深度分析
echo -e "\n=== 4. 网络深度分析 ==="

echo "--- 连接状态统计 ---"
ss -s

echo -e "\n--- TIME_WAIT连接数 ---"
ss -tan | grep TIME-WAIT | wc -l

echo -e "\n--- ESTABLISHED连接数 ---"
ss -tan | grep ESTABLISHED | wc -l

echo -e "\n--- 网络错误统计 ---"
cat /proc/net/dev | head -5

# 5. 文件描述符分析
echo -e "\n=== 5. 文件描述符分析 ==="
echo "系统级别:"
echo "  当前打开文件数: $(cat /proc/sys/fs/file-nr | awk '{print $1}')"
echo "  最大文件数: $(cat /proc/sys/fs/file-max)"

echo -e "\n进程级别(Top5):"
for pid in $(ps aux --sort=-%mem | head -6 | tail -5 | awk '{print $2}'); do
    fd_count=$(ls /proc/$pid/fd 2>/dev/null | wc -l)
    process=$(ps -p $pid -o comm= 2>/dev/null)
    echo "  $process (PID:$pid): $fd_count"
done
```

## kubelet问题诊断与修复

### kubelet进程崩溃

```bash
# 1. 检查kubelet状态
systemctl status kubelet
systemctl is-failed kubelet

# 2. 查看崩溃前日志
journalctl -u kubelet -n 200 --no-pager | less

# 3. 常见崩溃原因
# - 配置文件语法错误
# - 证书问题
# - 容器运行时不可用
# - 内存不足

# 4. 验证kubelet配置
kubelet --config=/var/lib/kubelet/config.yaml --dry-run 2>&1 | head -20

# 5. 重启kubelet
systemctl restart kubelet
systemctl status kubelet

# 6. 如果持续崩溃,检查系统日志
dmesg | grep kubelet
```

### kubelet配置问题

```yaml
# /var/lib/kubelet/config.yaml - 关键配置项
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 心跳相关(影响NotReady检测)
nodeStatusUpdateFrequency: 10s        # NodeStatus更新频率
nodeStatusReportFrequency: 5m         # NodeStatus上报频率(无变化时)

# 资源驱逐阈值
evictionHard:
  memory.available: "100Mi"           # 低于此值开始驱逐
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"

# 容器运行时
containerRuntimeEndpoint: "unix:///run/containerd/containerd.sock"

# 证书轮转
rotateCertificates: true
serverTLSBootstrap: true

# 日志配置
logging:
  format: text
  verbosity: 2                        # 调试时可增加到4-5

# 系统预留资源
systemReserved:
  cpu: "500m"
  memory: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
```

### kubelet证书问题

```bash
# 1. 检查证书有效期
echo "=== kubelet证书状态 ==="
for cert in /var/lib/kubelet/pki/*.pem; do
    echo "文件: $cert"
    openssl x509 -in "$cert" -noout -dates 2>/dev/null || echo "  不是有效证书"
    echo ""
done

# 2. 检查证书与CA匹配
CA_CERT="/etc/kubernetes/pki/ca.crt"
KUBELET_CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
openssl verify -CAfile "$CA_CERT" "$KUBELET_CERT" 2>&1

# 3. kubeadm环境-更新证书
kubeadm certs check-expiration
kubeadm certs renew all

# 4. 手动触发证书轮转
# 删除旧证书后kubelet会自动申请新证书(需配置rotateCertificates: true)
rm /var/lib/kubelet/pki/kubelet-client-current.pem
systemctl restart kubelet

# 5. 验证新证书
sleep 30
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

## 容器运行时诊断

### containerd完整诊断

```bash
#!/bin/bash
# containerd-diagnosis.sh

echo "====== containerd诊断 ======"

# 1. 服务状态
echo "=== 1. containerd服务状态 ==="
systemctl status containerd --no-pager | head -15

# 2. 进程状态
echo -e "\n=== 2. containerd进程 ==="
ps aux | grep containerd | grep -v grep

# 3. Socket可用性
echo -e "\n=== 3. containerd socket ==="
ls -la /run/containerd/containerd.sock
crictl info 2>&1 | head -20

# 4. 命名空间和容器
echo -e "\n=== 4. 容器统计 ==="
echo "k8s.io命名空间:"
ctr -n k8s.io containers list 2>/dev/null | wc -l
echo ""
echo "运行中容器:"
crictl ps 2>/dev/null | wc -l

# 5. 镜像状态
echo -e "\n=== 5. 镜像统计 ==="
crictl images 2>/dev/null | wc -l
echo "总镜像数: $(crictl images 2>/dev/null | tail -n +2 | wc -l)"

# 6. 磁盘使用
echo -e "\n=== 6. containerd磁盘使用 ==="
du -sh /var/lib/containerd/* 2>/dev/null | sort -hr | head -5

# 7. shim进程
echo -e "\n=== 7. shim进程状态 ==="
shim_count=$(ps aux | grep containerd-shim | grep -v grep | wc -l)
echo "shim进程数: $shim_count"
if [ $shim_count -gt 100 ]; then
    echo "⚠️ 警告: shim进程过多,可能存在泄漏"
fi

# 8. 错误日志
echo -e "\n=== 8. containerd错误日志 ==="
journalctl -u containerd --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail|panic" | tail -10
```

### containerd配置检查

```toml
# /etc/containerd/config.toml - 关键配置
version = 2

[grpc]
  address = "/run/containerd/containerd.sock"
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"
    max_concurrent_downloads = 3
    
    [plugins."io.containerd.grpc.v1.cri".containerd]
      snapshotter = "overlayfs"
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true

    [plugins."io.containerd.grpc.v1.cri".registry]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://mirror.gcr.io"]

[debug]
  level = "info"  # 调试时改为debug
```

### 运行时修复操作

```bash
# 1. 重启containerd
systemctl restart containerd
sleep 5
crictl info

# 2. 清理无用资源
# 清理停止的容器
crictl rm $(crictl ps -aq --state exited) 2>/dev/null

# 清理未使用镜像
crictl rmi --prune

# 清理containerd垃圾
ctr -n k8s.io content prune references
ctr -n k8s.io snapshots cleanup

# 3. 重建containerd状态
# 危险操作,仅在严重问题时使用
systemctl stop kubelet
systemctl stop containerd
rm -rf /var/lib/containerd/io.containerd.*/
systemctl start containerd
systemctl start kubelet
```

## 资源耗尽诊断

### 内存问题诊断与修复

```bash
#!/bin/bash
# memory-diagnosis.sh

echo "====== 内存问题诊断 ======"

# 1. 当前内存状态
echo "=== 1. 内存概览 ==="
free -h

# 2. 详细内存信息
echo -e "\n=== 2. 内存详情 ==="
cat /proc/meminfo | grep -E "MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree|Slab|SReclaimable"

# 3. 内存使用Top进程
echo -e "\n=== 3. 内存使用Top10进程 ==="
ps aux --sort=-%mem | head -11 | awk '{printf "%-8s %-8s %-6s %-6s %s\n", $1, $2, $4"%", $6/1024"MB", $11}'

# 4. cgroup内存统计
echo -e "\n=== 4. kubelet管理的cgroup内存 ==="
if [ -d "/sys/fs/cgroup/memory/kubepods" ]; then
    echo "kubepods总内存限制: $(cat /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes 2>/dev/null | numfmt --to=iec)"
    echo "kubepods当前使用: $(cat /sys/fs/cgroup/memory/kubepods/memory.usage_in_bytes 2>/dev/null | numfmt --to=iec)"
fi

# 5. OOM事件
echo -e "\n=== 5. 最近OOM事件 ==="
dmesg | grep -i "oom\|killed process" | tail -10

# 6. 缓存可回收性
echo -e "\n=== 6. 可回收内存 ==="
echo "PageCache: $(cat /proc/meminfo | grep "^Cached:" | awk '{print $2/1024"MB"}')"
echo "SReclaimable: $(cat /proc/meminfo | grep "SReclaimable:" | awk '{print $2/1024"MB"}')"
echo "Buffers: $(cat /proc/meminfo | grep "^Buffers:" | awk '{print $2/1024"MB"}')"
```

### 内存清理操作

```bash
# 1. 安全的缓存清理
sync
echo 1 > /proc/sys/vm/drop_caches  # 仅PageCache
# echo 2 > /proc/sys/vm/drop_caches  # dentries和inodes
# echo 3 > /proc/sys/vm/drop_caches  # 全部(生产环境谨慎)

# 2. 找出并清理泄漏进程
# 检查是否有内存持续增长的进程
watch -n 5 'ps aux --sort=-%mem | head -10'

# 3. 驱逐低优先级Pod释放内存
# 找出BestEffort Pod
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].resources.limits == null) | "\(.metadata.namespace)/\(.metadata.name)"'

# 手动驱逐特定Pod
kubectl delete pod <pod-name> -n <namespace>
```

### 磁盘问题诊断与修复

```bash
#!/bin/bash
# disk-diagnosis.sh

echo "====== 磁盘问题诊断 ======"

# 1. 磁盘空间
echo "=== 1. 磁盘空间使用 ==="
df -h | grep -E "Filesystem|^/dev"

# 2. inode使用
echo -e "\n=== 2. inode使用 ==="
df -i | grep -E "Filesystem|^/dev"

# 3. 大目录分析
echo -e "\n=== 3. /var/lib下大目录 ==="
du -sh /var/lib/* 2>/dev/null | sort -hr | head -10

# 4. 容器相关目录
echo -e "\n=== 4. 容器存储使用 ==="
echo "containerd: $(du -sh /var/lib/containerd 2>/dev/null | awk '{print $1}')"
echo "kubelet: $(du -sh /var/lib/kubelet 2>/dev/null | awk '{print $1}')"

# 5. 日志目录
echo -e "\n=== 5. 日志使用 ==="
du -sh /var/log/* 2>/dev/null | sort -hr | head -10

# 6. 大文件搜索
echo -e "\n=== 6. 大文件(>100MB) ==="
find /var -type f -size +100M 2>/dev/null | head -10

# 7. 容器日志
echo -e "\n=== 7. 容器日志文件 ==="
du -sh /var/log/pods/* 2>/dev/null | sort -hr | head -5
du -sh /var/log/containers/* 2>/dev/null | sort -hr | head -5
```

### 磁盘清理操作

```bash
# 1. 清理容器日志
# 方法1: 直接截断
find /var/log/containers -name "*.log" -size +100M -exec truncate -s 0 {} \;

# 方法2: 使用crictl
for container_id in $(crictl ps -q); do
    log_path=$(crictl inspect $container_id 2>/dev/null | jq -r '.status.logPath')
    if [ -n "$log_path" ] && [ -f "$log_path" ]; then
        size=$(stat -f%z "$log_path" 2>/dev/null || stat -c%s "$log_path" 2>/dev/null)
        if [ "$size" -gt 104857600 ]; then  # 100MB
            truncate -s 0 "$log_path"
            echo "已清理: $log_path"
        fi
    fi
done

# 2. 清理未使用镜像
crictl rmi --prune

# 3. 清理旧日志
journalctl --vacuum-size=1G
journalctl --vacuum-time=7d

# 4. 清理kubelet临时文件
rm -rf /var/lib/kubelet/pods/*/volumes/kubernetes.io~empty-dir/*/

# 5. 清理已退出容器
crictl rm $(crictl ps -a -q --state exited)
```

## 网络问题诊断

### CNI问题诊断

```bash
#!/bin/bash
# cni-diagnosis.sh

echo "====== CNI网络诊断 ======"

# 1. CNI配置检查
echo "=== 1. CNI配置文件 ==="
ls -la /etc/cni/net.d/
echo ""
for conf in /etc/cni/net.d/*; do
    echo "--- $conf ---"
    cat "$conf" 2>/dev/null | head -20
    echo ""
done

# 2. CNI二进制检查
echo -e "\n=== 2. CNI插件 ==="
ls -la /opt/cni/bin/ | head -15

# 3. 网络接口
echo -e "\n=== 3. 网络接口 ==="
ip addr show | grep -E "^[0-9]+:|inet "

# 4. 路由表
echo -e "\n=== 4. 路由表 ==="
ip route show

# 5. iptables规则概览
echo -e "\n=== 5. iptables NAT规则数 ==="
echo "NAT规则数: $(iptables -t nat -L -n 2>/dev/null | wc -l)"
echo "Filter规则数: $(iptables -t filter -L -n 2>/dev/null | wc -l)"

# 6. IPVS(如使用)
echo -e "\n=== 6. IPVS状态(如启用) ==="
ipvsadm -L -n 2>/dev/null | head -20 || echo "IPVS未启用或未安装ipvsadm"

# 7. 测试Pod网络连通性
echo -e "\n=== 7. 基础网络测试 ==="
# 测试到API Server
API_SERVER=$(cat /etc/kubernetes/kubelet.conf 2>/dev/null | grep server | awk '{print $2}')
if [ -n "$API_SERVER" ]; then
    echo "API Server连通性:"
    curl -sk --connect-timeout 5 "${API_SERVER}/healthz"
    echo ""
fi
```

### Calico网络诊断

```bash
#!/bin/bash
# calico-diagnosis.sh

echo "====== Calico网络诊断 ======"

# 1. calicoctl状态
echo "=== 1. Calico节点状态 ==="
calicoctl node status 2>/dev/null || kubectl get pods -n kube-system -l k8s-app=calico-node -o wide

# 2. BGP对等状态
echo -e "\n=== 2. BGP Peers ==="
calicoctl get bgppeer -o wide 2>/dev/null

# 3. IPPool
echo -e "\n=== 3. IP Pool ==="
calicoctl get ippool -o yaml 2>/dev/null | head -30

# 4. 本节点工作负载端点
echo -e "\n=== 4. 本节点Workload Endpoints ==="
calicoctl get workloadEndpoint --node=$(hostname) 2>/dev/null | head -20

# 5. Felix状态
echo -e "\n=== 5. Felix状态 ==="
curl -s http://localhost:9099/readiness 2>/dev/null || echo "Felix健康检查端点不可用"

# 6. BIRD状态
echo -e "\n=== 6. BIRD BGP状态 ==="
birdcl show protocols 2>/dev/null || echo "BIRD不可用"
```

### 网络连通性测试

```bash
#!/bin/bash
# network-connectivity-test.sh

echo "====== 网络连通性测试 ======"

# 从集群外部测试(在Master节点执行)
# 1. DNS测试
echo "=== 1. DNS测试 ==="
kubectl run test-dns --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes 2>/dev/null

# 2. Service访问测试
echo -e "\n=== 2. Service访问测试 ==="
kubectl run test-svc --image=curlimages/curl:8.4.0 --rm -it --restart=Never -- curl -s -o /dev/null -w "%{http_code}" https://kubernetes.default.svc:443/healthz -k 2>/dev/null

# 3. Pod间通信测试
echo -e "\n=== 3. Pod间通信测试 ==="
# 创建测试Pod
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: network-test-1
spec:
  containers:
  - name: test
    image: busybox:1.36
    command: ["sleep", "3600"]
---
apiVersion: v1
kind: Pod
metadata:
  name: network-test-2
spec:
  containers:
  - name: test
    image: busybox:1.36
    command: ["sleep", "3600"]
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            run: network-test-1
        topologyKey: kubernetes.io/hostname
EOF

sleep 30
POD2_IP=$(kubectl get pod network-test-2 -o jsonpath='{.status.podIP}')
kubectl exec network-test-1 -- ping -c 3 $POD2_IP

# 清理
kubectl delete pod network-test-1 network-test-2 --force --grace-period=0
```

## 节点恢复操作

### 标准恢复流程

```bash
#!/bin/bash
# node-recovery.sh

NODE_NAME=$1
if [ -z "$NODE_NAME" ]; then
    echo "用法: $0 <node-name>"
    exit 1
fi

echo "====== 节点 $NODE_NAME 恢复流程 ======"

# 1. 隔离节点防止新Pod调度
echo "=== 1. 隔离节点 ==="
kubectl cordon $NODE_NAME
kubectl get node $NODE_NAME

# 2. SSH到节点进行修复
echo -e "\n请SSH到节点执行以下检查和修复:"
cat << 'REMOTE_COMMANDS'
# 在节点上执行:

# 检查并重启kubelet
systemctl status kubelet
systemctl restart kubelet

# 如果kubelet启动失败,检查日志
journalctl -u kubelet -n 50 --no-pager

# 检查并重启containerd
systemctl status containerd
systemctl restart containerd

# 清理资源(如需要)
crictl rmi --prune
crictl rm $(crictl ps -aq --state exited)

# 检查磁盘和内存
df -h
free -h

REMOTE_COMMANDS

# 3. 等待节点恢复
echo -e "\n=== 3. 等待节点恢复 ==="
echo "等待中..."
for i in {1..12}; do
    STATUS=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
    if [ "$STATUS" == "True" ]; then
        echo "节点已恢复Ready状态!"
        break
    fi
    echo "  等待 $((i*10))秒... (当前状态: $STATUS)"
    sleep 10
done

# 4. 取消隔离
echo -e "\n=== 4. 取消隔离 ==="
kubectl uncordon $NODE_NAME
kubectl get node $NODE_NAME

# 5. 验证节点上的Pod
echo -e "\n=== 5. 节点上的Pod状态 ==="
kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_NAME | head -20
```

### 紧急重启流程

```bash
#!/bin/bash
# emergency-node-reboot.sh

NODE_NAME=$1
if [ -z "$NODE_NAME" ]; then
    echo "用法: $0 <node-name>"
    exit 1
fi

echo "====== 节点 $NODE_NAME 紧急重启流程 ======"
echo "⚠️ 警告: 此操作将驱逐节点上所有Pod并重启节点"
read -p "确认继续? (yes/no): " CONFIRM
if [ "$CONFIRM" != "yes" ]; then
    echo "已取消"
    exit 0
fi

# 1. 驱逐Pod
echo "=== 1. 驱逐节点上的Pod ==="
kubectl drain $NODE_NAME \
    --ignore-daemonsets \
    --delete-emptydir-data \
    --force \
    --grace-period=30 \
    --timeout=5m

# 2. 确认Pod已驱逐
echo -e "\n=== 2. 检查节点上剩余Pod ==="
kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_NAME | grep -v DaemonSet

# 3. 重启节点
echo -e "\n=== 3. 重启节点 ==="
echo "请SSH到节点执行: sudo reboot"
echo "或通过云平台控制台重启"

# 4. 等待节点恢复
echo -e "\n=== 4. 等待节点恢复(最长10分钟) ==="
for i in {1..60}; do
    STATUS=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}' 2>/dev/null)
    if [ "$STATUS" == "True" ]; then
        echo "节点已恢复!"
        break
    fi
    echo "  等待 $((i*10))秒..."
    sleep 10
done

# 5. 取消隔离
echo -e "\n=== 5. 取消节点隔离 ==="
kubectl uncordon $NODE_NAME

# 6. 最终状态
echo -e "\n=== 6. 最终状态 ==="
kubectl get node $NODE_NAME
kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_NAME | head -20
```

### 节点替换流程

```bash
#!/bin/bash
# node-replacement.sh - 彻底无法恢复时的节点替换

OLD_NODE=$1
if [ -z "$OLD_NODE" ]; then
    echo "用法: $0 <old-node-name>"
    exit 1
fi

echo "====== 节点 $OLD_NODE 替换流程 ======"

# 1. 标记节点不可调度
echo "=== 1. 隔离节点 ==="
kubectl cordon $OLD_NODE

# 2. 驱逐所有Pod
echo -e "\n=== 2. 驱逐Pod ==="
kubectl drain $OLD_NODE \
    --ignore-daemonsets \
    --delete-emptydir-data \
    --force \
    --grace-period=60 \
    --timeout=10m || echo "驱逐超时,继续..."

# 3. 删除节点对象
echo -e "\n=== 3. 删除节点对象 ==="
kubectl delete node $OLD_NODE

# 4. 清理etcd中的数据(如需要)
echo -e "\n=== 4. 检查并清理残留资源 ==="
# 检查是否有PV绑定到此节点
kubectl get pv -o json | jq -r '.items[] | select(.spec.nodeAffinity.required.nodeSelectorTerms[].matchExpressions[].values[] == "'$OLD_NODE'") | .metadata.name'

# 5. 提示添加新节点
echo -e "\n=== 5. 添加新节点 ==="
cat << 'NEW_NODE_GUIDE'
添加新节点步骤:
1. 准备新服务器,安装容器运行时和kubelet
2. 获取join命令: kubeadm token create --print-join-command
3. 在新节点执行join命令
4. 验证新节点: kubectl get nodes
NEW_NODE_GUIDE
```

## 监控告警配置

### Prometheus告警规则

```yaml
# node-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: node-notready-alerts
  namespace: monitoring
  labels:
    prometheus: k8s
    role: alert-rules
spec:
  groups:
  - name: node.status
    interval: 30s
    rules:
    # 节点NotReady告警
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 2m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} NotReady"
        description: "节点 {{ $labels.node }} 已处于NotReady状态超过2分钟"
        runbook: "https://wiki.example.com/runbook/node-notready"
        
    # 节点Unknown状态(可能网络分区)
    - alert: NodeUnknown
      expr: kube_node_status_condition{condition="Ready",status="unknown"} == 1
      for: 3m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} 状态Unknown"
        description: "无法获取节点 {{ $labels.node }} 状态,可能存在网络问题"
        
    # 节点内存压力
    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} 内存压力"
        description: "节点内存不足,可能触发Pod驱逐"
        
    # 节点磁盘压力
    - alert: NodeDiskPressure
      expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} 磁盘压力"
        description: "节点磁盘空间不足,可能触发Pod驱逐"
        
    # 节点PID压力
    - alert: NodePIDPressure
      expr: kube_node_status_condition{condition="PIDPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} PID压力"
        description: "节点进程数接近限制"
        
    # 节点网络不可用
    - alert: NodeNetworkUnavailable
      expr: kube_node_status_condition{condition="NetworkUnavailable",status="true"} == 1
      for: 2m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} 网络不可用"
        description: "节点网络配置异常,CNI可能故障"

  - name: node.resource
    rules:
    # 节点CPU使用率高
    - alert: NodeCPUHigh
      expr: |
        100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} CPU使用率高"
        description: "节点CPU使用率超过85%,当前: {{ $value | printf \"%.1f\" }}%"
        
    # 节点内存使用率高
    - alert: NodeMemoryHigh
      expr: |
        (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 > 85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} 内存使用率高"
        description: "节点内存使用率超过85%,当前: {{ $value | printf \"%.1f\" }}%"
        
    # 节点磁盘使用率高
    - alert: NodeDiskHigh
      expr: |
        (1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100 > 85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} 磁盘使用率高"
        description: "节点根分区使用率超过85%,当前: {{ $value | printf \"%.1f\" }}%"

  - name: kubelet.health
    rules:
    # kubelet不可用
    - alert: KubeletDown
      expr: up{job="kubelet"} == 0
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "kubelet {{ $labels.instance }} 不可用"
        description: "kubelet无法访问,节点可能NotReady"
        
    # kubelet PLEG延迟高
    - alert: KubeletPLEGDurationHigh
      expr: |
        histogram_quantile(0.99, sum(rate(kubelet_pleg_relist_duration_seconds_bucket[5m])) by (instance, le)) > 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "kubelet {{ $labels.instance }} PLEG延迟高"
        description: "PLEG relist P99延迟超过3秒,可能影响Pod状态更新"
        
    # kubelet Pod启动延迟
    - alert: KubeletPodStartSLOBreach
      expr: |
        histogram_quantile(0.99, sum(rate(kubelet_pod_start_duration_seconds_bucket[5m])) by (instance, le)) > 60
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "kubelet {{ $labels.instance }} Pod启动慢"
        description: "Pod启动P99延迟超过60秒"
```

### Grafana Dashboard配置

```json
{
  "dashboard": {
    "title": "Node Health Dashboard",
    "panels": [
      {
        "title": "节点状态概览",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(kube_node_status_condition{condition=\"Ready\",status=\"true\"})",
            "legendFormat": "Ready节点"
          },
          {
            "expr": "sum(kube_node_status_condition{condition=\"Ready\",status=\"false\"})",
            "legendFormat": "NotReady节点"
          }
        ]
      },
      {
        "title": "节点Condition状态",
        "type": "table",
        "targets": [
          {
            "expr": "kube_node_status_condition{status=\"true\"} == 1",
            "format": "table"
          }
        ]
      },
      {
        "title": "kubelet心跳延迟",
        "type": "graph",
        "targets": [
          {
            "expr": "time() - kube_node_status_condition{condition=\"Ready\",status=\"true\"} * on(node) group_left() kube_node_created",
            "legendFormat": "{{ node }}"
          }
        ]
      },
      {
        "title": "节点资源使用率",
        "type": "graph",
        "targets": [
          {
            "expr": "100 - (avg by(instance) (rate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "CPU {{ instance }}"
          },
          {
            "expr": "(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100",
            "legendFormat": "Memory {{ instance }}"
          }
        ]
      }
    ]
  }
}
```

## ACK节点诊断

### 阿里云ACK节点诊断

```bash
# ACK节点诊断入口
# 控制台: 集群 -> 节点管理 -> 节点列表 -> 诊断

# 使用aliyun CLI诊断
# 1. 获取集群节点列表
aliyun cs DescribeClusterNodes --ClusterId <cluster-id>

# 2. 节点健康检查
aliyun cs GET /clusters/{ClusterId}/nodes/{NodeName}/repair

# 3. 节点重置(危险操作)
aliyun cs POST /clusters/{ClusterId}/nodes --body '{
  "nodes": ["<node-name>"],
  "action": "repair"
}'

# 4. ACK诊断工具ack-diagnose
# 下载并安装
curl -O https://alibabacloud-china.github.io/diagnose-tools/scripts/installer.sh
bash installer.sh

# 运行诊断
ack-diagnose cluster --cluster-id <cluster-id>
ack-diagnose node --cluster-id <cluster-id> --node-name <node-name>
```

### ACK特有问题排查

```bash
# 1. 检查云监控Agent
systemctl status cloudmonitor
systemctl status aliyun-service

# 2. 检查安全组规则
# 确保节点间10250, 10255, 10256端口互通
# 确保节点到API Server 6443端口连通

# 3. 检查ENI网卡状态(Terway网络)
ip addr show | grep eth
cat /etc/cni/net.d/10-terway.conf

# 4. 检查节点标签和污点
kubectl get node <node-name> -o yaml | grep -A 20 labels
kubectl get node <node-name> -o yaml | grep -A 10 taints

# 5. ACK节点自愈
# 在控制台开启节点自愈功能
# 设置 -> 功能管理 -> 节点自愈
```

## 最佳实践

### 预防措施清单

| 类别 | 措施 | 优先级 | 说明 |
|-----|------|-------|------|
| **监控** | 部署Node Exporter | P0 | 收集节点指标 |
| **监控** | 配置节点告警 | P0 | 及时发现问题 |
| **监控** | 配置心跳监控 | P1 | 监控kubelet心跳 |
| **资源** | 设置资源预留 | P0 | systemReserved/kubeReserved |
| **资源** | 配置驱逐阈值 | P1 | 合理的evictionHard/Soft |
| **资源** | 磁盘监控告警 | P1 | 80%使用率告警 |
| **证书** | 证书过期监控 | P0 | 30天前告警 |
| **证书** | 自动证书轮转 | P1 | rotateCertificates=true |
| **运维** | 定期健康检查 | P1 | 每日自动化检查 |
| **运维** | 文档化故障处理 | P1 | Runbook |
| **运维** | 演练恢复流程 | P2 | 定期演练 |

### 快速诊断检查表

```
□ 节点能SSH登录?
  ├── Yes → 继续检查
  └── No  → 检查网络/云平台/物理机状态

□ kubelet进程运行?
  ├── systemctl status kubelet
  └── 不运行 → 检查日志并重启

□ containerd运行?
  ├── systemctl status containerd
  └── 不运行 → 检查日志并重启

□ 能连接API Server?
  ├── curl -k https://<apiserver>:6443/healthz
  └── 不能 → 检查网络/证书

□ 证书有效?
  ├── openssl x509 -in <cert> -noout -dates
  └── 过期 → 更新证书

□ 资源充足?
  ├── free -h / df -h
  └── 不足 → 清理资源

□ CNI正常?
  ├── 检查CNI Pod状态
  └── 异常 → 重启CNI
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| v1.24 | 移除dockershim | 必须使用containerd或CRI-O |
| v1.25 | Lease API稳定版 | 心跳机制更可靠 |
| v1.26 | 节点日志API | 可远程获取kubelet日志 |
| v1.27 | 改进节点状态上报 | 更精确的状态检测 |
| v1.28 | 增强/readyz端点 | 更详细的健康检查 |
| v1.29 | SidecarContainers GA | 影响Pod生命周期 |
| v1.30 | 节点内存交换支持Beta | 新的内存管理选项 |

---

**NotReady诊断原则**: 分层诊断(kubelet→运行时→资源→网络→证书) → 隔离节点 → 定位根因 → 修复并验证 → 取消隔离

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
