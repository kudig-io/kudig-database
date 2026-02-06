# 生产环境部署最佳实践 (Production Deployment Best Practices)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 企业级部署指南

---

## 目录

1. [企业级架构设计](#1-企业级架构设计)
2. [生产环境硬件规格](#2-生产环境硬件规格)
3. [网络架构优化](#3-网络架构优化)
4. [存储架构设计](#4-存储架构设计)
5. [安全合规部署](#5-安全合规部署)
6. [监控告警体系](#6-监控告警体系)
7. [备份灾备策略](#7-备份灾备策略)
8. [成本优化方案](#8-成本优化方案)
9. [多云混合部署](#9-多云混合部署)
10. [自动化运维实践](#10-自动化运维实践)

---

## 1. 企业级架构设计

### 1.1 分层架构模式

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Enterprise Kubernetes Architecture                        │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────── Edge Layer ────────────────────────────────┐   │
│  │                                                                             │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │   │
│  │  │   Edge Nodes    │  │   Edge Cache    │  │   Local Registry│            │   │
│  │  │ (10-50 nodes)   │  │   (Redis/NGINX) │  │   (Harbor)      │            │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘            │   │
│  │        │                        │                   │                      │   │
│  └────────┼────────────────────────┼───────────────────┼──────────────────────┘   │
│           │                        │                   │                          │
│           ▼                        ▼                   ▼                          │
│  ┌─────────────────────────────── Regional Layer ─────────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │  │
│  │  │ Regional Master │  │ Regional Worker │  │ Regional Ingress│            │  │
│  │  │ (3-5 nodes)     │  │ (50-200 nodes)  │  │ (2-4 nodes)     │            │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘            │  │
│  │        │                        │                   │                      │  │
│  └────────┼────────────────────────┼───────────────────┼──────────────────────┘  │
│           │                        │                   │                          │
│           ▼                        ▼                   ▼                          │
│  ┌─────────────────────────────── Global Layer ───────────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │  │
│  │  │ Global Control  │  │ Global Monitor  │  │ Global Security │            │  │
│  │  │ Plane (3 nodes) │  │ (Prometheus)    │  │ (Falco/Kyverno) │            │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘            │  │
│  │                                                                             │  │
│  └─────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 生产环境规模分级

| 规模等级 | 节点数量 | Pod数量 | 用户规模 | 推荐架构 |
|----------|----------|---------|----------|----------|
| **小型** | 10-50节点 | 500-2000 Pod | 10-50用户 | 单集群，标准HA |
| **中型** | 50-200节点 | 2000-8000 Pod | 50-200用户 | 多区域，联邦管理 |
| **大型** | 200-1000节点 | 8000-40000 Pod | 200-1000用户 | 多集群，服务网格 |
| **超大型** | 1000+节点 | 40000+ Pod | 1000+用户 | 多云，分布式控制平面 |

### 1.3 部署前置检查清单

```yaml
# 生产环境部署前置检查清单
production_precheck:
  business_requirements:
    - [ ] 明确业务SLA要求 (可用性、性能、成本)
    - [ ] 确定合规性要求 (GDPR、SOX、HIPAA等)
    - [ ] 评估团队技能水平和培训需求
    - [ ] 制定变更管理流程
    - [ ] 建立应急预案和联系人清单
    
  technical_assessment:
    - [ ] 网络带宽和延迟测试
    - [ ] 存储IOPS和容量评估
    - [ ] 计算资源需求分析
    - [ ] 安全扫描和漏洞评估
    - [ ] 灾备演练计划制定
    
  infrastructure_readiness:
    - [ ] 硬件设备验收测试
    - [ ] 网络连通性验证
    - [ ] 存储系统性能基准
    - [ ] 备份系统功能验证
    - [ ] 监控告警系统联调
```

---

## 2. 生产环境硬件规格

### 2.1 控制平面节点规格

| 组件 | CPU | 内存 | 存储 | 网络 | 说明 |
|------|-----|------|------|------|------|
| **etcd节点** | 8-16核 | 32-64GB | 1TB NVMe | 25GbE | 关键业务建议16核64GB |
| **API Server节点** | 8-16核 | 32-64GB | 500GB SSD | 25GbE | 高并发场景需要更多CPU |
| **Controller节点** | 4-8核 | 16-32GB | 200GB SSD | 10GbE | 可与API Server共节点 |
| **Scheduler节点** | 4-8核 | 16-32GB | 200GB SSD | 10GbE | 可与Controller共节点 |
| **负载均衡器** | 4-8核 | 8-16GB | 100GB | 40GbE | 建议硬件负载均衡器 |

### 2.2 工作节点规格推荐

```yaml
# 工作节点规格配置
worker_node_profiles:
  compute_optimized:
    description: "计算密集型应用"
    cpu: "32核"
    memory: "128GB"
    storage: "500GB NVMe + 2TB HDD"
    network: "25GbE"
    use_cases: ["AI/ML训练", "大数据处理", "科学计算"]
    
  memory_optimized:
    description: "内存密集型应用"
    cpu: "16核"
    memory: "256GB"
    storage: "1TB NVMe"
    network: "10GbE"
    use_cases: ["数据库", "缓存", "内存计算"]
    
  storage_optimized:
    description: "存储密集型应用"
    cpu: "16核"
    memory: "64GB"
    storage: "10TB HDD + 1TB NVMe"
    network: "10GbE"
    use_cases: ["对象存储", "数据湖", "日志分析"]
    
  network_optimized:
    description: "网络密集型应用"
    cpu: "16核"
    memory: "64GB"
    storage: "500GB NVMe"
    network: "100GbE"
    use_cases: ["CDN", "视频处理", "实时通信"]
```

### 2.3 硬件冗余配置

```bash
#!/bin/bash
# 硬件健康检查脚本

# 1. CPU健康检查
check_cpu_health() {
    echo "Checking CPU health..."
    
    # 检查CPU温度
    for cpu_temp in /sys/class/hwmon/hwmon*/temp*_input; do
        temp=$(cat $cpu_temp)
        temp_c=$(echo "$temp/1000" | bc)
        if [ $temp_c -gt 70 ]; then
            echo "WARNING: CPU temperature ${temp_c}°C exceeds threshold"
        fi
    done
    
    # 检查CPU频率稳定性
    base_freq=$(lscpu | grep "CPU max MHz" | awk '{print $4}')
    current_freq=$(cat /proc/cpuinfo | grep "cpu MHz" | head -1 | awk '{print $4}')
    
    freq_ratio=$(echo "$current_freq/$base_freq" | bc -l)
    if (( $(echo "$freq_ratio < 0.9" | bc -l) )); then
        echo "WARNING: CPU frequency throttling detected"
    fi
}

# 2. 内存健康检查
check_memory_health() {
    echo "Checking memory health..."
    
    # 检查内存错误
    if command -v edac-util &> /dev/null; then
        edac-util --summary
    fi
    
    # 检查内存使用率
    mem_usage=$(free | grep Mem | awk '{printf("%.2f", $3/$2 * 100.0)}')
    if (( $(echo "$mem_usage > 90" | bc -l) )); then
        echo "WARNING: Memory usage ${mem_usage}% exceeds threshold"
    fi
}

# 3. 存储健康检查
check_storage_health() {
    echo "Checking storage health..."
    
    # 检查SMART状态
    for disk in /dev/sd*; do
        if [ -b "$disk" ]; then
            smartctl -H $disk | grep "test result" | grep -q "PASSED" || echo "ERROR: SMART test failed for $disk"
        fi
    done
    
    # 检查磁盘使用率
    df -h | awk '$5 > 90 {print "WARNING: Disk usage critical on " $6 ": " $5}'
}

# 4. 网络健康检查
check_network_health() {
    echo "Checking network health..."
    
    # 检查网络接口状态
    for interface in $(ip link show | grep UP | awk -F': ' '{print $2}' | grep -E '^(eth|ens)'); do
        rx_errors=$(cat /sys/class/net/$interface/statistics/rx_errors)
        tx_errors=$(cat /sys/class/net/$interface/statistics/tx_errors)
        
        if [ $rx_errors -gt 100 ] || [ $tx_errors -gt 100 ]; then
            echo "WARNING: Network errors on $interface: RX=$rx_errors, TX=$tx_errors"
        fi
    done
}

# 执行所有检查
check_cpu_health
check_memory_health
check_storage_health
check_network_health

echo "Hardware health check completed"
```

---

## 3. 网络架构优化

### 3.1 生产网络拓扑设计

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Production Network Topology                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Internet (Public IPs)                                                          │
│          │                                                                       │
│          ▼                                                                       │
│  ┌─────────────────┐                                                            │
│  │   Border Router │                                                            │
│  │   (防火墙/NAT)  │                                                            │
│  └─────────────────┘                                                            │
│          │                                                                       │
│          ▼                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    Core Switch Layer (100GbE)                           │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐       │    │
│  │  │ Core-1  │  │ Core-2  │  │ Core-3  │  │ Core-4  │  │ Core-5  │       │    │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘       │    │
│  └─────────────────────┬─────────────────────────────────────────────────────┘    │
│                        │                                                         │
│          ┌─────────────┼─────────────┬───────────────┬───────────────┐           │
│          │             │             │               │               │           │
│          ▼             ▼             ▼               ▼               ▼           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  ToR-AZ1    │ │  ToR-AZ2    │ │  ToR-AZ3    │ │  ToR-DMZ    │ │  ToR-MGMT   │ │
│  │ (接入交换机) │ │ (接入交换机) │ │ (接入交换机) │ │ (DMZ区域)   │ │ (管理网络)  │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
│          │             │             │               │               │           │
│    ┌─────┼─────┐ ┌─────┼─────┐ ┌─────┼─────┐   ┌─────┼─────┐   ┌─────┼─────┐     │
│    │     │     │ │     │     │ │     │     │   │     │     │   │     │     │     │
│    ▼     ▼     ▼ ▼     ▼     ▼ ▼     ▼     ▼   ▼     ▼     ▼   ▼     ▼     ▼     │
│ ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐  │
│ │CP-1││CP-2││CP-3││WN-1││WN-2││WN-3││WN-4││WN-5││DMZ ││MGMT││STOR││NET ││SEC │  │
│ └────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 网络性能优化配置

```bash
#!/bin/bash
# 生产环境网络优化脚本

# 1. 系统网络参数调优
optimize_system_network() {
    cat >> /etc/sysctl.conf << EOF
# TCP性能优化
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr
net.ipv4.tcp_slow_start_after_idle = 0
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 60
net.ipv4.tcp_keepalive_probes = 3

# 网络连接优化
net.core.netdev_max_backlog = 5000
net.core.somaxconn = 65535
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.tcp_tw_reuse = 1

# 网络安全
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.all.rp_filter = 1
EOF

    sysctl -p
}

# 2. 网卡中断优化
optimize_network_interrupts() {
    # 启用IRQ平衡
    systemctl enable irqbalance
    systemctl start irqbalance
    
    # 为特定网卡设置CPU亲和性
    for iface in eth0 eth1; do
        if [ -d "/sys/class/net/$iface" ]; then
            echo "Optimizing interrupts for $iface"
            # 设置接收队列中断亲和性
            for irq in $(grep $iface /proc/interrupts | awk '{print $1}' | sed 's/://'); do
                echo 2 > /proc/irq/$irq/smp_affinity
            done
        fi
    done
}

# 3. 网络服务质量(QoS)配置
configure_qos() {
    # 安装tc工具
    yum install -y iproute-tc
    
    # 为Kubernetes流量设置优先级
    tc qdisc add dev eth0 root handle 1: htb default 30
    tc class add dev eth0 parent 1: classid 1:1 htb rate 1000mbit
    tc class add dev eth0 parent 1:1 classid 1:10 htb rate 700mbit ceil 1000mbit prio 1  # 控制平面流量
    tc class add dev eth0 parent 1:1 classid 1:20 htb rate 200mbit ceil 300mbit prio 2  # 应用流量
    tc class add dev eth0 parent 1:1 classid 1:30 htb rate 100mbit ceil 200mbit prio 3  # 管理流量
    
    # 设置过滤规则
    tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dport 6443 0xffff flowid 1:10  # API Server
    tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dport 2379 0xffff flowid 1:10  # etcd
    tc filter add dev eth0 protocol ip parent 1:0 prio 2 u32 match ip protocol 6 0xff flowid 1:20      # TCP应用流量
}

# 4. 网络监控配置
setup_network_monitoring() {
    # 安装网络监控工具
    yum install -y iperf3 tcpdump nmap
    
    # 创建网络性能测试脚本
    cat > /usr/local/bin/network-benchmark.sh << 'EOF'
#!/bin/bash
# 网络性能基准测试

TEST_DURATION=60
TARGET_HOST=$1

echo "Testing network performance to $TARGET_HOST"

# 带宽测试
echo "Bandwidth test:"
iperf3 -c $TARGET_HOST -t $TEST_DURATION -P 4

# 延迟测试
echo "Latency test:"
ping -c 10 $TARGET_HOST

# MTU测试
echo "MTU test:"
ping -M do -s 1472 $TARGET_HOST

# 连接数测试
echo "Connection test:"
for i in {1..100}; do
    nc -z $TARGET_HOST 6443 &
done
wait
EOF

    chmod +x /usr/local/bin/network-benchmark.sh
}

# 执行优化
optimize_system_network
optimize_network_interrupts
configure_qos
setup_network_monitoring

echo "Network optimization completed"
```

### 3.3 多网络平面设计

```yaml
# 多网络平面配置
network_planes:
  control_plane:
    description: "控制平面专用网络"
    cidr: "10.10.0.0/16"
    vlan: 100
    security: "highest"
    isolation: "complete"
    
  data_plane:
    description: "应用数据网络"
    cidr: "10.20.0.0/16"
    vlan: 200
    security: "high"
    isolation: "namespace"
    
  pod_network:
    description: "Pod间通信网络"
    cidr: "10.30.0.0/16"
    vlan: 300
    security: "medium"
    isolation: "network-policy"
    
  service_network:
    description: "Service网络"
    cidr: "10.40.0.0/16"
    vlan: 400
    security: "high"
    isolation: "service-account"
    
  management:
    description: "管理网络"
    cidr: "10.50.0.0/16"
    vlan: 500
    security: "highest"
    isolation: "physical"
```

---

## 4. 存储架构设计

### 4.1 存储分层架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Storage Tier Architecture                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Application Layer                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    Container Storage Interface (CSI)                      │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Orchestration Layer                                                            │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Storage Classes                                │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │  Fast SSD   │ │  Balanced   │ │  Archive    │ │  Shared     │       │    │
│  │  │ (NVMe)      │ │ (SATA SSD)  │ │ (HDD)       │ │ (NFS/Ceph)  │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Infrastructure Layer                                                           │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Storage Backends                               │    │
│  │                                                                          │    │
│  │  Tier 1: High Performance                                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │    │
│  │  │   Local     │ │   Rook/Ceph │ │   Longhorn  │                        │    │
│  │  │   NVMe      │ │   (Flash)   │ │   (Replicated)│                       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘                        │    │
│  │                                                                          │    │
│  │  Tier 2: Balanced Performance                                           │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │    │
│  │  │   Local     │ │   Rook/Ceph │ │   NFS       │                        │    │
│  │  │   SATA SSD  │ │   (Hybrid)  │ │   (External)│                       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘                        │    │
│  │                                                                          │    │
│  │  Tier 3: Cost Optimized                                                 │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │    │
│  │  │   Local     │ │   Object    │ │   Archive   │                        │    │
│  │  │   HDD       │ │   Storage   │ │   Storage   │                        │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘                        │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 存储性能基准测试

```bash
#!/bin/bash
# 存储性能基准测试脚本

STORAGE_PATH="/mnt/storage-test"
TEST_SIZE="10G"
BLOCK_SIZES=("4k" "64k" "1M" "4M")

# 1. FIO性能测试
run_fio_tests() {
    echo "Running FIO performance tests..."
    
    mkdir -p $STORAGE_PATH
    
    for bs in "${BLOCK_SIZES[@]}"; do
        echo "Testing block size: $bs"
        
        # 顺序读写测试
        fio --name=seq-read-$bs \
            --directory=$STORAGE_PATH \
            --size=$TEST_SIZE \
            --bs=$bs \
            --rw=read \
            --ioengine=libaio \
            --direct=1 \
            --numjobs=4 \
            --group_reporting \
            --output=/tmp/fio-seq-read-$bs.json
        
        fio --name=seq-write-$bs \
            --directory=$STORAGE_PATH \
            --size=$TEST_SIZE \
            --bs=$bs \
            --rw=write \
            --ioengine=libaio \
            --direct=1 \
            --numjobs=4 \
            --group_reporting \
            --output=/tmp/fio-seq-write-$bs.json
            
        # 随机读写测试
        fio --name=rand-read-$bs \
            --directory=$STORAGE_PATH \
            --size=$TEST_SIZE \
            --bs=$bs \
            --rw=randread \
            --ioengine=libaio \
            --direct=1 \
            --numjobs=4 \
            --group_reporting \
            --output=/tmp/fio-rand-read-$bs.json
            
        fio --name=rand-write-$bs \
            --directory=$STORAGE_PATH \
            --size=$TEST_SIZE \
            --bs=$bs \
            --rw=randwrite \
            --ioengine=libaio \
            --direct=1 \
            --numjobs=4 \
            --group_reporting \
            --output=/tmp/fio-rand-write-$bs.json
    done
}

# 2. 文件系统测试
run_filesystem_tests() {
    echo "Running filesystem tests..."
    
    # 创建测试文件
    dd if=/dev/zero of=$STORAGE_PATH/test-file bs=1M count=1000
    
    # 测试创建大量小文件
    time for i in {1..10000}; do
        echo "test" > $STORAGE_PATH/small-file-$i.txt
    done
    
    # 测试大文件复制
    time cp $STORAGE_PATH/test-file $STORAGE_PATH/test-file-copy
    
    # 清理测试文件
    rm -rf $STORAGE_PATH/*
}

# 3. 数据库性能测试
run_database_tests() {
    echo "Running database-like tests..."
    
    # 模拟数据库随机IO模式
    fio --name=db-simulate \
        --directory=$STORAGE_PATH \
        --size=5G \
        --bs=8k \
        --rw=randrw \
        --rwmixread=70 \
        --ioengine=libaio \
        --direct=1 \
        --numjobs=8 \
        --iodepth=32 \
        --group_reporting \
        --output=/tmp/fio-db-simulation.json
}

# 4. 结果分析
analyze_results() {
    echo "Analyzing test results..."
    
    # 汇总性能数据
    echo "=== Storage Performance Summary ==="
    echo "Block Size | Seq Read | Seq Write | Rand Read | Rand Write"
    echo "-----------|----------|-----------|-----------|------------"
    
    for bs in "${BLOCK_SIZES[@]}"; do
        seq_read_iops=$(jq '.jobs[0].read.iops' /tmp/fio-seq-read-$bs.json 2>/dev/null || echo "N/A")
        seq_write_iops=$(jq '.jobs[0].write.iops' /tmp/fio-seq-write-$bs.json 2>/dev/null || echo "N/A")
        rand_read_iops=$(jq '.jobs[0].read.iops' /tmp/fio-rand-read-$bs.json 2>/dev/null || echo "N/A")
        rand_write_iops=$(jq '.jobs[0].write.iops' /tmp/fio-rand-write-$bs.json 2>/dev/null || echo "N/A")
        
        printf "%-10s | %-8s | %-9s | %-9s | %-10s\n" \
            $bs "${seq_read_iops}" "${seq_write_iops}" "${rand_read_iops}" "${rand_write_iops}"
    done
    
    # 生成HTML报告
    cat > /tmp/storage-benchmark-report.html << EOF
<!DOCTYPE html>
<html>
<head>
    <title>Storage Performance Benchmark Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }
        th { background-color: #f2f2f2; }
        .good { color: green; }
        .warning { color: orange; }
        .critical { color: red; }
    </style>
</head>
<body>
    <h1>Storage Performance Benchmark Report</h1>
    <h2>Test Environment</h2>
    <ul>
        <li>Test Path: $STORAGE_PATH</li>
        <li>Test Size: $TEST_SIZE</li>
        <li>Date: $(date)</li>
    </ul>
    
    <h2>Performance Results</h2>
    <table>
        <tr>
            <th>Block Size</th>
            <th>Seq Read (IOPS)</th>
            <th>Seq Write (IOPS)</th>
            <th>Rand Read (IOPS)</th>
            <th>Rand Write (IOPS)</th>
        </tr>
EOF

    for bs in "${BLOCK_SIZES[@]}"; do
        seq_read_iops=$(jq '.jobs[0].read.iops' /tmp/fio-seq-read-$bs.json 2>/dev/null || echo "0")
        seq_write_iops=$(jq '.jobs[0].write.iops' /tmp/fio-seq-write-$bs.json 2>/dev/null || echo "0")
        rand_read_iops=$(jq '.jobs[0].read.iops' /tmp/fio-rand-read-$bs.json 2>/dev/null || echo "0")
        rand_write_iops=$(jq '.jobs[0].write.iops' /tmp/fio-rand-write-$bs.json 2>/dev/null || echo "0")
        
        cat >> /tmp/storage-benchmark-report.html << EOF
        <tr>
            <td>$bs</td>
            <td class="$([ $seq_read_iops -gt 10000 ] && echo "good" || echo "warning")">$seq_read_iops</td>
            <td class="$([ $seq_write_iops -gt 5000 ] && echo "good" || echo "warning")">$seq_write_iops</td>
            <td class="$([ $rand_read_iops -gt 5000 ] && echo "good" || echo "warning")">$rand_read_iops</td>
            <td class="$([ $rand_write_iops -gt 2000 ] && echo "good" || echo "warning")">$rand_write_iops</td>
        </tr>
EOF
    done

    cat >> /tmp/storage-benchmark-report.html << EOF
    </table>
</body>
</html>
EOF
}

# 执行测试
run_fio_tests
run_filesystem_tests
run_database_tests
analyze_results

echo "Storage benchmark completed. Report saved to /tmp/storage-benchmark-report.html"
```

### 4.3 存储容量规划

```yaml
# 存储容量规划模板
storage_capacity_planning:
  etcd_storage:
    purpose: "集群状态存储"
    size_calculation: |
      基础容量: 1GB
      每个Pod元数据: 2KB
      每个Node元数据: 10KB
      预留空间: 200%
      公式: (1GB + (Pod数 × 2KB + Node数 × 10KB)) × 2
    example: |
      5000 Pods, 100 Nodes
      = (1GB + (5000×2KB + 100×10KB)) × 2
      = (1GB + 10MB + 1MB) × 2
      = ~2.02GB × 2 = 4.04GB
      建议配置: 10GB SSD
      
  container_images:
    purpose: "容器镜像存储"
    size_calculation: |
      平均镜像大小: 500MB
      镜像副本数: 3
      缓存保留: 2倍空间
      公式: 平均镜像大小 × 镜像数量 × 副本数 × 2
    example: |
      100个不同镜像
      = 500MB × 100 × 3 × 2
      = 300GB
      建议配置: 500GB高速存储
      
  application_data:
    purpose: "应用持久化数据"
    size_calculation: |
      当前数据量: X GB
      增长率: Y%每年
      保留周期: Z个月
      公式: 当前数据量 × (1 + 增长率)^(保留周期/12) × 1.5(预留空间)
    example: |
      当前1TB数据，年增长50%，保留12个月
      = 1TB × (1 + 0.5)^(12/12) × 1.5
      = 1.5TB × 1.5 = 2.25TB
      建议配置: 3TB可用存储
      
  logs_and_monitoring:
    purpose: "日志和监控数据"
    size_calculation: |
      每Pod日志: 100MB/天
      监控数据: 1GB/天
      保留天数: 30天
      压缩比: 3:1
      公式: ((Pod数 × 100MB + 1GB) × 保留天数) ÷ 3
    example: |
      1000个Pod
      = ((1000 × 100MB + 1GB) × 30) ÷ 3
      = (100GB + 1GB) × 30 ÷ 3
      = 101GB × 10 = 1010GB
      建议配置: 1.2TB存储
```

---

## 5. 安全合规部署

### 5.1 企业级安全架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Enterprise Security Architecture                         │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌────────────────────────────── Perimeter Security ─────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │  │
│  │  │   Firewall  │  │   WAF       │  │   IDS/IPS   │  │   DDoS      │      │  │
│  │  │   (边界)    │  │   (应用层)  │  │   (入侵检测)│  │   (防护)    │      │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘      │  │
│  │                                                                             │  │
│  └─────────────────────────────────────┬─────────────────────────────────────┘  │
│                                        │                                        │
│  ┌────────────────────────────── Network Security ───────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │  │
│  │  │   Network   │  │   Service   │  │   Pod       │  │   CNI       │      │  │
│  │  │   Policies  │  │   Mesh      │  │   Security  │  │   Plugins   │      │  │
│  │  │   (Calico)  │  │   (Istio)   │  │   (Kyverno) │  │   (Cilium)  │      │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘      │  │
│  │                                                                             │  │
│  └─────────────────────────────────────┬─────────────────────────────────────┘  │
│                                        │                                        │
│  ┌────────────────────────────── Platform Security ──────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │  │
│  │  │   RBAC      │  │   Secrets   │  │   Audit     │  │   Encryption│      │  │
│  │  │   (权限)    │  │   (密钥)    │  │   (审计)    │  │   (加密)    │      │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘      │  │
│  │                                                                             │  │
│  └─────────────────────────────────────┬─────────────────────────────────────┘  │
│                                        │                                        │
│  ┌────────────────────────────── Data Security ──────────────────────────────┐  │
│  │                                                                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │  │
│  │  │   etcd      │  │   Storage   │  │   Backup    │  │   Key       │      │  │
│  │  │   Encryption│  │   Encryption│  │   Encryption│  │   Management│      │  │
│  │  │   (静态)    │  │   (传输)    │  │   (备份)    │  │   (HashiCorp)│     │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘      │  │
│  │                                                                             │  │
│  └─────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 合规性配置基线

```yaml
# 合规性安全配置基线
compliance_baseline:
  cis_controls:
    # CIS Kubernetes Benchmark配置
    control_1_authentication:
      description: "身份认证强化"
      requirements:
        - anonymous_auth: false
        - webhook_authentication: true
        - oidc_integration: true
        - certificate_rotation: "720h"
      implementation:
        api_server_args:
          - "--anonymous-auth=false"
          - "--authentication-token-webhook=true"
          - "--authentication-token-webhook-cache-ttl=3m"
          - "--oidc-issuer-url=https://dex.example.com"
          - "--oidc-client-id=kubernetes"
          - "--oidc-username-claim=email"
          
    control_2_authorization:
      description: "授权控制强化"
      requirements:
        - rbac_enabled: true
        - node_authorization: true
        - abac_disabled: true
      implementation:
        api_server_args:
          - "--authorization-mode=Node,RBAC"
          - "--enable-admission-plugins=NodeRestriction"
          
    control_3_network_security:
      description: "网络安全强化"
      requirements:
        - network_policies_enforced: true
        - pod_security_standards: "restricted"
        - service_mesh_mtls: true
      implementation:
        network_policy_example: |
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
            
    control_4_secrets_management:
      description: "密钥管理强化"
      requirements:
        - secrets_encryption: true
        - etcd_encryption: true
        - external_secrets_store: true
      implementation:
        encryption_config: |
          apiVersion: apiserver.config.k8s.io/v1
          kind: EncryptionConfiguration
          resources:
            - resources:
              - secrets
              - configmaps
              providers:
              - aescbc:
                  keys:
                  - name: key1
                    secret: <base64-encoded-key>
              - identity: {}

  gdpr_compliance:
    data_protection:
      personal_data_encryption: true
      data_minimization: true
      right_to_erasure: "72h"
      data_portability: "JSON format"
      
    privacy_by_design:
      default_deny_networking: true
      anonymized_logging: true
      audit_trail_retention: "3 years"
      
  hipaa_compliance:
    security_rules:
      access_control: "Role-based access"
      audit_controls: "Comprehensive logging"
      integrity: "Data checksums and signatures"
      transmission_security: "End-to-end encryption"
      
    technical_safeguards:
      access_logs: "90-day retention"
      encryption_at_rest: "AES-256"
      encryption_in_transit: "TLS 1.3"
      backup_encryption: "Customer-managed keys"

  soc2_compliance:
    trust_services_criteria:
      security: "Access controls and monitoring"
      availability: "99.9% uptime SLA"
      processing_integrity: "Data validation and error handling"
      confidentiality: "Encryption and data classification"
      privacy: "Personal data protection"
```

### 5.3 安全监控和告警

```yaml
# 安全监控告警配置
security_monitoring:
  falco_rules:
    # 容器安全监控规则
    suspicious_process:
      rule: "Suspicious Process Creation"
      desc: "Detect suspicious process creation in containers"
      condition: >
        spawned_process and container
        and (proc.name in (sudo, su, chown, chmod, wget, curl, nc, netcat, nmap, tcpdump))
      output: >
        Suspicious process %(proc.name) spawned in container %(container.id)
        (user=%(user.name) command=%(proc.cmdline) parent=%(proc.pname))
      priority: "WARNING"
      tags: ["process", "mitre_execution"]
      
    fileless_malware:
      rule: "Fileless Malware Detection"
      desc: "Detect fileless malware execution"
      condition: >
        spawned_process and proc.name in (sh, bash, python, perl)
        and proc.args contains "exec" and proc.args contains "/dev/shm"
      output: >
        Potential fileless malware detected: %(proc.cmdline)
      priority: "CRITICAL"
      tags: ["malware", "fileless"]
      
    privilege_escalation:
      rule: "Privilege Escalation Attempt"
      desc: "Detect container breakout attempts"
      condition: >
        spawned_process and container
        and (proc.name = "chroot" or proc.args contains "CAP_SYS_ADMIN")
      output: >
        Privilege escalation attempt in container %(container.id): %(proc.cmdline)
      priority: "CRITICAL"
      tags: ["privilege", "container_escape"]
  
  prometheus_alerts:
    # 安全相关告警规则
    api_abuse_detection:
      alert: "APIAbuseDetected"
      expr: |
        sum(rate(apiserver_request_total{verb=~"POST|PUT|DELETE"}[5m])) 
        > 100
      for: "10m"
      labels:
        severity: "warning"
      annotations:
        summary: "High rate of mutating API requests"
        description: "{{ $value }} mutating requests per second detected"
        
    unauthorized_access:
      alert: "UnauthorizedAccessAttempt"
      expr: |
        sum(rate(apiserver_request_total{code="401"}[5m])) 
        > 10
      for: "5m"
      labels:
        severity: "warning"
      annotations:
        summary: "Multiple unauthorized access attempts"
        description: "{{ $value }} 401 errors detected in the last 5 minutes"
        
    certificate_expiration:
      alert: "CertificateExpiringSoon"
      expr: |
        kube_certificate_expiration_seconds < 86400 * 30
      for: "1h"
      labels:
        severity: "warning"
      annotations:
        summary: "Kubernetes certificate expiring soon"
        description: "Certificate expires in {{ $value | humanizeDuration }}"
```

---

## 6. 监控告警体系

### 6.1 企业级监控架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Enterprise Monitoring Architecture                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Data Collection Layer                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Metrics Collection                            │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   Node      │ │   Pod       │ │   Service   │ │   Control   │       │    │
│  │  │   Exporter  │ │   Exporter  │ │   Monitor   │ │   Plane     │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Processing Layer                                                               │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Data Processing                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │  Prometheus │ │   Grafana   │ │  Alert-     │ │   Log       │       │    │
│  │  │   (Metrics) │ │  (Visual)   │ │  Manager    │ │   Processor │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Storage Layer                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Data Storage                                  │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   Long-term │ │   Short-    │ │   Log       │ │   Tracing   │       │    │
│  │  │   Storage   │ │   Term      │ │   Storage   │ │   Storage   │       │    │
│  │  │   (Thanos)  │ │   Memory    │ │   (Loki)    │ │   (Tempo)   │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Notification Layer                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        Alert Distribution                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   Slack     │ │   Email     │ │   PagerDuty │ │   Webhook   │       │    │
│  │  │   (Teams)   │ │   (SMS)     │ │   (Opsgenie)│ │   (Custom)  │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 核心监控指标体系

```yaml
# 企业级监控指标体系
monitoring_metrics:
  slos_sli:
    availability_slo:
      target: "99.9%"
      measurement: "uptime_percentage"
      calculation: |
        (total_time - downtime) / total_time × 100%
        
    latency_slo:
      target: "P95 < 100ms"
      measurement: "request_duration"
      calculation: |
        95th percentile of API request latency
        
    reliability_slo:
      target: "99.5%"
      measurement: "success_rate"
      calculation: |
        successful_requests / total_requests × 100%
  
  control_plane_metrics:
    api_server:
      request_rate: "apiserver_request_total"
      error_rate: "apiserver_request_total{code=~'5..'}"
      latency_p95: "histogram_quantile(0.95, apiserver_request_duration_seconds_bucket)"
      inflight_requests: "apiserver_current_inflight_requests"
      
    etcd:
      db_size: "etcd_debugging_mvcc_db_total_size_in_bytes"
      commit_latency: "histogram_quantile(0.95, etcd_disk_backend_commit_duration_seconds_bucket)"
      wal_fsync: "histogram_quantile(0.95, etcd_disk_wal_fsync_duration_seconds_bucket)"
      has_leader: "etcd_server_has_leader"
      
    scheduler:
      scheduling_attempts: "scheduler_schedule_attempts_total"
      scheduling_latency: "histogram_quantile(0.95, scheduler_scheduling_attempt_duration_seconds_bucket)"
      pending_pods: "scheduler_pending_pods"
      
    controller_manager:
      workqueue_depth: "workqueue_depth"
      workqueue_latency: "histogram_quantile(0.95, workqueue_queue_duration_seconds_bucket)"
      reconcile_errors: "controller_runtime_reconcile_errors_total"
  
  node_metrics:
    resource_utilization:
      cpu_usage: "sum(rate(node_cpu_seconds_total{mode!='idle'}[5m])) by (instance)"
      memory_usage: "node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes"
      disk_io: "rate(node_disk_io_time_seconds_total[5m])"
      network_throughput: "rate(node_network_receive_bytes_total[5m])"
      
    health_status:
      node_ready: "kube_node_status_condition{condition='Ready',status='true'}"
      node_pressure: "kube_node_status_condition{condition=~'DiskPressure|MemoryPressure|PIDPressure'}"
      kubelet_status: "kubelet_running_pods"
  
  application_metrics:
    business_metrics:
      transaction_rate: "app_transactions_total"
      error_rate: "app_errors_total"
      user_sessions: "active_user_sessions"
      response_time: "histogram_quantile(0.95, app_response_time_seconds_bucket)"
      
    infrastructure_metrics:
      pod_restart_count: "kube_pod_container_status_restarts_total"
      container_cpu_usage: "rate(container_cpu_usage_seconds_total[5m])"
      container_memory_usage: "container_memory_usage_bytes"
      network_errors: "rate(container_network_receive_packets_dropped_total[5m])"

  cost_metrics:
    resource_cost:
      cpu_cost: "sum(rate(container_cpu_usage_seconds_total[1h])) * hourly_cpu_cost"
      memory_cost: "sum(avg_over_time(container_memory_usage_bytes[1h])/1024/1024/1024) * hourly_memory_cost"
      storage_cost: "sum(kube_persistentvolumeclaim_resource_requests_storage_bytes) * monthly_storage_cost"
      network_cost: "sum(rate(container_network_receive_bytes_total[1h])) * hourly_network_cost"
```

### 6.3 智能告警策略

```yaml
# 智能告警策略配置
intelligent_alerting:
  alert_suppression:
    # 告警抑制规则
    maintenance_window:
      active: true
      schedule: "02:00-04:00 daily"
      suppress_alerts:
        - "NodeNotReady"
        - "PodCrashLooping"
        - "HighCPUUsage"
        
    cascading_failure:
      detection: "multiple related alerts within 5 minutes"
      suppression: "suppress child alerts when parent alert is active"
      example:
        parent_alert: "APIServerDown"
        suppressed_alerts: 
          - "ControllerManagerDown"
          - "SchedulerDown"
          - "EtcdInsufficientMembers"
          
    flapping_detection:
      threshold: "10 state changes in 1 hour"
      action: "inhibit alert and send notification to investigate"
      
  dynamic_thresholds:
    # 动态阈值设置
    baseline_calculation:
      method: "statistical_analysis"
      window: "7 days"
      algorithm: "holt_winters_forecast"
      
    adaptive_alerting:
      cpu_threshold:
        normal: "static_threshold: 80%"
        peak_hours: "dynamic_threshold: baseline + 2σ"
        off_peak: "dynamic_threshold: baseline + 1.5σ"
        
      memory_threshold:
        normal: "static_threshold: 85%"
        growth_trend: "predictive_threshold: forecast + 10%"
        
  notification_routing:
    # 智能通知路由
    escalation_policy:
      level_1:
        recipients: ["oncall-team"]
        channels: ["slack", "sms"]
        response_time: "15 minutes"
        
      level_2:
        recipients: ["team-leads"]
        channels: ["email", "phone"]
        response_time: "1 hour"
        trigger: "level_1 timeout or critical severity"
        
      level_3:
        recipients: ["management"]
        channels: ["email", "phone"]
        response_time: "4 hours"
        trigger: "level_2 timeout or business impact"
        
    routing_rules:
      business_hours:
        time: "09:00-18:00 weekdays"
        priority_notifications: true
        
      off_hours:
        time: "outside business hours"
        critical_only_notifications: true
        auto_ticket_creation: true
```

---

## 7. 备份灾备策略

### 7.1 企业级备份架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Enterprise Backup Architecture                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Primary Site                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Live Production                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   etcd      │ │   Config    │ │   App Data  │ │   Container │       │    │
│  │  │   Cluster   │ │   Files     │ │   Volumes   │ │   Images    │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  │        │              │              │              │                   │    │
│  └────────┼──────────────┼──────────────┼──────────────┼───────────────────┘    │
│           │              │              │              │                        │
│           ▼              ▼              ▼              ▼                        │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        Backup Processing                              │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   etcd      │ │   Velero    │ │   Rsync     │ │   Skopeo    │       │    │
│  │  │   Snapshot  │ │   (Cluster) │ │   (Files)   │ │   (Images)  │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│           │              │              │              │                        │
│           ▼              ▼              ▼              ▼                        │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                      Local Backup Storage                              │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   etcd      │ │   Cluster   │ │   File      │ │   Image     │       │    │
│  │  │   Snapshots │ │   Backups   │ │   Backups   │ │   Registry  │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  Secondary Site (Same Region)                                                   │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                      Warm Standby System                              │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   etcd      │ │   Config    │ │   Data      │ │   Registry  │       │    │
│  │  │   Replica   │ │   Mirror    │ │   Replica   │ │   Mirror    │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  DR Site (Different Region)                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                      Cold DR System                                   │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   Offsite   │ │   Offsite   │ │   Offsite   │ │   Offsite   │       │    │
│  │  │   etcd      │ │   Cluster   │ │   Data      │ │   Images    │       │    │
│  │  │   Archive   │ │   Archive   │ │   Archive   │ │   Archive   │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 备份策略配置

```yaml
# 企业级备份策略
backup_strategies:
  rto_rpo_targets:
    critical_systems:
      rto: "15 minutes"
      rpo: "5 minutes"
      backup_frequency: "continuous"
      
    important_systems:
      rto: "2 hours"
      rpo: "30 minutes"
      backup_frequency: "hourly"
      
    standard_systems:
      rto: "24 hours"
      rpo: "24 hours"
      backup_frequency: "daily"
      
    archival_systems:
      rto: "7 days"
      rpo: "30 days"
      backup_frequency: "monthly"

  etcd_backup:
    configuration:
      snapshot_interval: "30m"
      retention_days: "30"
      storage_locations:
        - local: "/backup/etcd"
        - remote: "s3://backup-bucket/etcd"
        - dr_site: "gs://dr-backup/etcd"
        
    automation_script: |
      #!/bin/bash
      # etcd备份自动化脚本
      
      BACKUP_DIR="/backup/etcd"
      TIMESTAMP=$(date +%Y%m%d_%H%M%S)
      SNAPSHOT_NAME="etcd-snapshot-${TIMESTAMP}.db"
      
      # 创建快照
      ETCDCTL_API=3 etcdctl snapshot save ${BACKUP_DIR}/${SNAPSHOT_NAME} \
        --endpoints=https://127.0.0.1:2379 \
        --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key
      
      # 验证快照
      ETCDCTL_API=3 etcdctl snapshot status ${BACKUP_DIR}/${SNAPSHOT_NAME}
      
      # 上传到远程存储
      aws s3 cp ${BACKUP_DIR}/${SNAPSHOT_NAME} s3://backup-bucket/etcd/
      gsutil cp ${BACKUP_DIR}/${SNAPSHOT_NAME} gs://dr-backup/etcd/
      
      # 清理旧备份
      find ${BACKUP_DIR} -name "etcd-snapshot-*.db" -mtime +30 -delete
      
      # 记录备份日志
      echo "$(date): etcd backup completed - ${SNAPSHOT_NAME}" >> /var/log/backup.log

  cluster_backup:
    velero_configuration:
      schedule: "0 */6 * * *"  # 每6小时
      ttl: "168h"  # 7天保留
      included_namespaces:
        - production
        - staging
        - monitoring
      excluded_resources:
        - events
        - nodes
        - events.events.k8s.io
        
    backup_policy: |
      apiVersion: velero.io/v1
      kind: Schedule
      metadata:
        name: daily-backup
        namespace: velero
      spec:
        schedule: "0 2 * * *"
        template:
          includedNamespaces:
          - '*'
          excludedNamespaces:
          - kube-system
          - velero
          snapshotVolumes: true
          storageLocation: default
          ttl: 168h0m0s

  data_volume_backup:
    strategy: "incremental_with_full_weekly"
    tools:
      - restic for file-level backups
      - rclone for object storage sync
      - rsync for direct file sync
      
    script_example: |
      #!/bin/bash
      # 数据卷增量备份脚本
      
      SOURCE_DIR="/data/application"
      BACKUP_DIR="/backup/data"
      REMOTE_STORAGE="s3://backup-bucket/application"
      
      # 周日执行完整备份
      if [ $(date +%u) -eq 7 ]; then
        rsync -av --delete ${SOURCE_DIR}/ ${BACKUP_DIR}/full_$(date +%Y%m%d)/
        rclone sync ${BACKUP_DIR}/full_$(date +%Y%m%d)/ ${REMOTE_STORAGE}/full/
      else
        # 工作日执行增量备份
        rsync -av --delete --link-dest=${BACKUP_DIR}/latest/ \
          ${SOURCE_DIR}/ ${BACKUP_DIR}/incremental_$(date +%Y%m%d_%H%M)/
        rclone sync ${BACKUP_DIR}/incremental_$(date +%Y%m%d_%H%M)/ \
          ${REMOTE_STORAGE}/incremental/
      fi
      
      # 更新最新链接
      rm -f ${BACKUP_DIR}/latest
      ln -s ${BACKUP_DIR}/incremental_$(date +%Y%m%d_%H%M) ${BACKUP_DIR}/latest

  container_registry_backup:
    strategy: "registry_sync_with_metadata"
    configuration:
      primary_registry: "harbor.internal"
      backup_registry: "harbor.backup"
      sync_frequency: "hourly"
      retention_policy: "90 days"
      
    backup_script: |
      #!/bin/bash
      # 容器镜像备份脚本
      
      PRIMARY_REGISTRY="harbor.internal"
      BACKUP_REGISTRY="harbor.backup"
      REPOSITORIES=("nginx" "redis" "postgresql" "custom-app")
      
      for repo in "${REPOSITORIES[@]}"; do
        # 获取最新标签
        tags=$(curl -s https://${PRIMARY_REGISTRY}/v2/${repo}/tags/list | jq -r '.tags[]')
        
        for tag in $tags; do
          # 同步镜像
          skopeo copy \
            --src-tls-verify=false \
            --dest-tls-verify=false \
            docker://${PRIMARY_REGISTRY}/${repo}:${tag} \
            docker://${BACKUP_REGISTRY}/${repo}:${tag}
        done
      done
      
      # 清理过期镜像
      find /backup/registry -name "*.tar" -mtime +90 -delete
```

### 7.3 灾难恢复演练

```bash
#!/bin/bash
# 灾难恢复演练脚本

DR_TEST_ENV="dr-test-cluster"
PRODUCTION_ENV="production-cluster"
TEST_DURATION="2h"

# 1. 演练前准备
prepare_dr_test() {
    echo "Preparing DR test environment..."
    
    # 创建测试命名空间
    kubectl create namespace dr-test --dry-run=client -o yaml | kubectl apply -f -
    
    # 部署测试应用
    cat > /tmp/dr-test-app.yaml <