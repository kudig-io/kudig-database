# 网络硬件故障排查

## 概述

网络硬件故障会导致业务中断和性能下降。本文档详细解析网卡、交换机、光模块及布线问题的诊断方法和排查技巧。

## 网卡故障排查

### 网卡诊断流程

```bash
#!/bin/bash
# 网卡诊断脚本

diagnose_nic() {
    local nic=$1
    
    echo "=== 网卡诊断: $nic ==="
    
    # 基本信息
    echo -e "\n--- 基本信息 ---"
    ethtool -i "$nic"
    
    # 链路状态
    echo -e "\n--- 链路状态 ---"
    ethtool "$nic" | grep -E "Speed|Duplex|Link detected|Auto-negotiation"
    
    # 统计信息
    echo -e "\n--- 统计信息 ---"
    ethtool -S "$nic" | grep -E "rx_.*error|tx_.*error|rx_.*drop|tx_.*drop|crc|collision" | head -20
    
    # 固件信息
    echo -e "\n--- 固件版本 ---"
    ethtool -i "$nic" | grep -E "firmware-version|driver"
    
    # EEPROM信息 (光模块)
    echo -e "\n--- 光模块信息 ---"
    ethtool -m "$nic" 2>/dev/null | head -20
}

# 遍历所有网卡
for nic in $(ls /sys/class/net | grep -v lo); do
    if ethtool "$nic" &>/dev/null; then
        diagnose_nic "$nic"
        echo -e "\n"
    fi
done
```

### 常见网卡问题

```yaml
网卡问题速查:
  链路不通:
    症状: Link detected: no
    排查:
      - 检查物理连接
      - 检查交换机端口
      - 更换线缆/光模块
      - 检查SFP兼容性
      
  速率协商失败:
    症状: 速率低于预期
    排查:
      ethtool eth0
      # 检查Auto-negotiation
    解决:
      # 强制速率
      ethtool -s eth0 speed 25000 duplex full autoneg off
      
  大量错误包:
    症状: rx_errors/tx_errors增长
    排查:
      ethtool -S eth0 | grep error
    可能原因:
      - 线缆问题
      - 光模块衰减
      - 电磁干扰
      - 驱动bug
      
  性能下降:
    症状: 带宽低于预期
    排查:
      # 检查中断分布
      cat /proc/interrupts | grep eth
      # 检查队列配置
      ethtool -l eth0
    优化:
      # 增加队列数
      ethtool -L eth0 combined 8
```

## 交换机故障排查

### 交换机健康检查

```yaml
交换机检查项:
  物理层:
    - 端口LED状态
    - 风扇运行状态
    - 电源状态
    - 温度指示
    
  链路层:
    - 端口up/down状态
    - 错误计数器
    - 丢包统计
    - MAC地址表
    
  网络层:
    - VLAN配置
    - 路由表
    - ARP表
    - BGP/OSPF邻居

常见命令 (Cisco风格):
  # 查看端口状态
  show interface status
  show interface brief
  
  # 查看端口统计
  show interface eth1/1 counters
  show interface eth1/1 counters errors
  
  # 查看MAC表
  show mac address-table
  
  # 查看日志
  show logging
```

## 光模块与光纤故障

### 光模块诊断

```bash
#!/bin/bash
# 光模块诊断脚本

check_sfp() {
    local nic=$1
    
    echo "=== 光模块诊断: $nic ==="
    
    # SFP信息
    sfp_info=$(ethtool -m "$nic" 2>/dev/null)
    
    if [ -z "$sfp_info" ]; then
        echo "无光模块或不支持DDM"
        return
    fi
    
    echo "$sfp_info"
    
    # 提取关键参数
    local tx_power=$(echo "$sfp_info" | grep "Laser output power" | awk '{print $4}')
    local rx_power=$(echo "$sfp_info" | grep "Receiver signal average optical power" | awk '{print $6}')
    local temp=$(echo "$sfp_info" | grep "Module temperature" | awk '{print $3}')
    
    echo -e "\n--- 关键参数 ---"
    echo "发射功率: $tx_power dBm"
    echo "接收功率: $rx_power dBm"
    echo "模块温度: $temp °C"
    
    # 功率判断
    # 正常范围参考: TX -6~+3 dBm, RX -14~+3 dBm
    echo -e "\n--- 功率评估 ---"
    if [[ -n "$rx_power" ]]; then
        rx_val=$(echo "$rx_power" | sed 's/[^0-9.-]//g')
        if (( $(echo "$rx_val < -14" | bc -l) )); then
            echo "[警告] 接收功率过低，检查光纤或远端发射"
        elif (( $(echo "$rx_val > 3" | bc -l) )); then
            echo "[警告] 接收功率过高，可能需要衰减器"
        else
            echo "接收功率正常"
        fi
    fi
}

for nic in $(ls /sys/class/net | grep -E "^eth|^ens|^enp"); do
    check_sfp "$nic"
done
```

### 光纤问题排查

```yaml
光纤问题诊断:
  高误码率:
    可能原因:
      - 光纤弯曲过度
      - 连接器脏污
      - 光功率不足
      - 光纤断裂
    排查步骤:
      1. 检查光功率
      2. 清洁连接器
      3. 检查弯曲半径
      4. OTDR测试
      
  间歇性断连:
    可能原因:
      - 连接器松动
      - 光纤疲劳
      - 温度变化
      - 振动
    排查步骤:
      1. 重新插拔连接器
      2. 检查固定是否牢靠
      3. 监控错误日志
      
  链路完全中断:
    可能原因:
      - 光纤断裂
      - 光模块故障
      - 端口故障
    排查步骤:
      1. 检查两端链路灯
      2. 交换光模块测试
      3. 更换光纤测试
      4. 更换端口测试
```

## 网络性能问题

### 网络延迟诊断

```bash
#!/bin/bash
# 网络延迟诊断

echo "=== 网络延迟诊断 ==="

# 基本ping测试
echo -e "\n--- Ping测试 ---"
ping -c 10 -i 0.2 ${1:-8.8.8.8} | tail -5

# MTR路径分析
echo -e "\n--- 路径分析 ---"
mtr -r -c 10 ${1:-8.8.8.8}

# 检查网卡中断合并
echo -e "\n--- 中断合并设置 ---"
for nic in $(ls /sys/class/net | grep -v lo); do
    echo "$nic:"
    ethtool -c "$nic" 2>/dev/null | grep -E "rx-usecs|tx-usecs|adaptive"
done

# 检查Ring Buffer
echo -e "\n--- Ring Buffer ---"
for nic in $(ls /sys/class/net | grep -v lo); do
    echo "$nic:"
    ethtool -g "$nic" 2>/dev/null | grep -A2 "Current"
done
```

### 性能优化参数

```yaml
网卡性能调优:
  中断合并:
    场景: 降低CPU负载
    命令: ethtool -C eth0 rx-usecs 50 tx-usecs 50
    
  Ring Buffer:
    场景: 减少丢包
    命令: ethtool -G eth0 rx 4096 tx 4096
    
  多队列:
    场景: 多核负载均衡
    命令: ethtool -L eth0 combined 16
    
  RSS/RPS:
    场景: 接收端扩展
    配置: /sys/class/net/eth0/queues/rx-*/rps_cpus
    
  TSO/GSO:
    场景: 大包卸载
    检查: ethtool -k eth0 | grep -E "tso|gso"
```

## 网络问题速查表

| 症状 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| 链路不通 | 物理连接 | ethtool eth0 | 检查线缆/光模块 |
| 速率低 | 协商问题 | ethtool eth0 | 强制速率 |
| 高丢包 | 队列满/错误 | ethtool -S eth0 | 调整队列/检查硬件 |
| 高延迟 | 中断/配置 | ethtool -c eth0 | 调整中断合并 |
| CRC错误 | 线缆/光模块 | ethtool -S eth0 | 更换线缆 |

## 参考资源

- [Linux Network Documentation](https://www.kernel.org/doc/html/latest/networking/)
- [ethtool Manual](https://man7.org/linux/man-pages/man8/ethtool.8.html)
- [Intel Network Adapter Diagnostics](https://www.intel.com/)
