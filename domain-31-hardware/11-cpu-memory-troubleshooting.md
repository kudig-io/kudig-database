# CPU与内存故障排查

## 概述

CPU和内存是服务器核心计算组件，其故障往往导致系统不稳定或完全无法工作。本文档详细解析CPU和内存故障的诊断方法、排查技巧及修复方案。

## CPU故障排查

### CPU故障类型

```yaml
CPU故障分类:
  硬件故障:
    物理损坏:
      - 针脚弯曲/断裂
      - 核心烧毁
      - 封装损坏
    原因: 安装不当、过热、电压异常
    
    功能失效:
      - 单核心故障
      - 缓存故障
      - 内存控制器故障
    原因: 老化、制造缺陷
    
  热故障:
    过温保护:
      - CPU降频 (Throttling)
      - 紧急关机
    原因: 散热器松动、硅脂干涸、风扇故障
    
  逻辑故障:
    MCE (Machine Check Exception):
      - 可纠正错误 (CE)
      - 不可纠正错误 (UCE)
    原因: 内部错误、外部干扰
```

### CPU故障诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        CPU故障诊断流程                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     Step 1: 症状收集                                 │   │
│  │  ┌────────────────────────────────────────────────────────────────┐ │   │
│  │  │ • 系统无法启动？                                               │ │   │
│  │  │ • 频繁死机/蓝屏？                                             │ │   │
│  │  │ • 性能下降？                                                   │ │   │
│  │  │ • 温度异常？                                                   │ │   │
│  │  │ • 错误日志？                                                   │ │   │
│  │  └────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     Step 2: 温度检查                                 │   │
│  │  ┌────────────────────────────────────────────────────────────────┐ │   │
│  │  │ 检查命令:                                                      │ │   │
│  │  │   sensors                                                      │ │   │
│  │  │   ipmitool sensor | grep -i cpu                               │ │   │
│  │  │   cat /sys/class/thermal/thermal_zone*/temp                   │ │   │
│  │  │                                                                │ │   │
│  │  │ 正常范围: 30-80°C (负载)                                       │ │   │
│  │  │ 告警阈值: >85°C                                                │ │   │
│  │  │ 临界阈值: >95°C                                                │ │   │
│  │  └────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     Step 3: MCE检查                                  │   │
│  │  ┌────────────────────────────────────────────────────────────────┐ │   │
│  │  │ 检查命令:                                                      │ │   │
│  │  │   mcelog --client                                              │ │   │
│  │  │   dmesg | grep -i mce                                          │ │   │
│  │  │   journalctl | grep -i 'machine check'                         │ │   │
│  │  │                                                                │ │   │
│  │  │ 关注字段:                                                      │ │   │
│  │  │   - MCE地址                                                    │ │   │
│  │  │   - 错误类型 (cache/TLB/bus等)                                 │ │   │
│  │  │   - 发生频率                                                   │ │   │
│  │  └────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     Step 4: 频率状态检查                             │   │
│  │  ┌────────────────────────────────────────────────────────────────┐ │   │
│  │  │ 检查命令:                                                      │ │   │
│  │  │   cat /proc/cpuinfo | grep MHz                                 │ │   │
│  │  │   turbostat                                                    │ │   │
│  │  │   cpupower frequency-info                                      │ │   │
│  │  │                                                                │ │   │
│  │  │ 关注:                                                          │ │   │
│  │  │   - 是否达到标称频率                                           │ │   │
│  │  │   - 是否触发降频                                               │ │   │
│  │  │   - Turbo是否正常                                              │ │   │
│  │  └────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### MCE错误分析

```bash
#!/bin/bash
# MCE错误分析脚本

# 检查mcelog服务
check_mcelog() {
    if ! command -v mcelog &> /dev/null; then
        echo "mcelog未安装，请先安装: yum install mcelog"
        return 1
    fi
    
    # 启动mcelog守护进程
    systemctl status mcelog || systemctl start mcelog
}

# 分析MCE错误
analyze_mce() {
    echo "=== MCE错误分析 ==="
    
    # 获取MCE统计
    mcelog --client 2>/dev/null || echo "无mcelog客户端数据"
    
    # 从dmesg获取MCE信息
    echo -e "\n=== dmesg中的MCE记录 ==="
    dmesg | grep -i "machine check" | tail -20
    
    # 分类统计
    echo -e "\n=== MCE错误类型统计 ==="
    dmesg | grep -i mce | grep -oP 'BANK \d+' | sort | uniq -c
    
    # 检查是否有不可纠正错误
    if dmesg | grep -qi "uncorrected"; then
        echo -e "\n[警告] 检测到不可纠正的MCE错误!"
        dmesg | grep -i uncorrected | tail -5
    fi
}

# MCE类型解读
decode_mce() {
    local bank=$1
    
    case $bank in
        0)
            echo "Bank 0: 数据缓存单元错误"
            ;;
        1)
            echo "Bank 1: 指令缓存单元错误"
            ;;
        2)
            echo "Bank 2: 总线单元错误"
            ;;
        3)
            echo "Bank 3: 加载存储单元错误"
            ;;
        4)
            echo "Bank 4: 内存控制器错误"
            ;;
        *)
            echo "Bank $bank: 参考CPU文档"
            ;;
    esac
}

# 主程序
check_mcelog
analyze_mce
```

### CPU降频排查

```yaml
CPU降频原因排查:
  热节流 (Thermal Throttling):
    症状:
      - CPU频率低于基频
      - 高温告警
      - 风扇高速运转
      
    检查命令:
      # 查看是否触发热节流
      grep -r . /sys/devices/system/cpu/cpu*/thermal_throttle/
      
    解决方案:
      - 检查散热器安装
      - 更换硅脂
      - 清理灰尘
      - 改善机房温度
      
  功率限制 (Power Limiting):
    症状:
      - 高负载下频率受限
      - 功耗达到TDP上限
      
    检查命令:
      # 查看功率限制
      turbostat --Summary --quiet --show PkgWatt
      
    解决方案:
      - 调整BIOS功率配置
      - 升级电源供电
      
  BIOS配置:
    症状:
      - Turbo Boost未启用
      - 节能模式锁频
      
    检查要点:
      - Intel Turbo Boost: 应启用
      - C-States: 根据场景配置
      - Power Profile: 设置为最大性能
```

## 内存故障排查

### 内存故障类型

```yaml
内存故障分类:
  硬件故障:
    DIMM失效:
      表现: 系统不识别、容量减少
      原因: 芯片损坏、金手指氧化
      
    颗粒故障:
      表现: 持续性单比特/多比特错误
      原因: 颗粒老化、制造缺陷
      
  软性错误:
    单比特错误 (SBE/CE):
      表现: ECC自动纠正
      原因: 宇宙射线、电气干扰
      可接受: 偶发性，非固定位置
      
    多比特错误 (MBE/UCE):
      表现: 系统崩溃、数据损坏
      原因: 严重故障
      处理: 立即更换
      
  配置问题:
    频率不匹配:
      表现: 降频运行或不稳定
    通道不平衡:
      表现: 带宽下降
    兼容性问题:
      表现: POST失败、间歇故障
```

### 内存诊断命令

```bash
#!/bin/bash
# 内存诊断脚本

echo "=== 内存硬件信息 ==="
dmidecode -t memory | grep -E "Size:|Type:|Speed:|Manufacturer:|Part Number:|Locator:"

echo -e "\n=== 内存使用情况 ==="
free -h

echo -e "\n=== EDAC错误统计 ==="
if [ -d /sys/devices/system/edac/mc ]; then
    for mc in /sys/devices/system/edac/mc/mc*; do
        if [ -d "$mc" ]; then
            echo "--- $(basename $mc) ---"
            echo "可纠正错误: $(cat $mc/ce_count 2>/dev/null || echo 'N/A')"
            echo "不可纠正错误: $(cat $mc/ue_count 2>/dev/null || echo 'N/A')"
            
            # 检查各DIMM
            for dimm in $mc/dimm*; do
                if [ -d "$dimm" ]; then
                    echo "  $(basename $dimm): CE=$(cat $dimm/dimm_ce_count 2>/dev/null)"
                fi
            done
        fi
    done
else
    echo "EDAC模块未加载，尝试: modprobe edac_core"
fi

echo -e "\n=== 内存相关内核日志 ==="
dmesg | grep -iE "memory|ecc|edac|dimm|mce" | tail -20

echo -e "\n=== IPMI SEL中的内存事件 ==="
ipmitool sel elist 2>/dev/null | grep -i memory | tail -10
```

### 内存错误定位

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        内存错误定位流程                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Step 1: 确认错误类型                                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  # 查看EDAC错误计数                                                  │   │
│  │  edac-util -s                                                        │   │
│  │                                                                       │   │
│  │  输出示例:                                                           │   │
│  │  mc0: 0 Uncorrected Errors with no DIMM info                        │   │
│  │  mc0: 15 Corrected Errors with no DIMM info                         │   │
│  │  mc0: csrow0: 15 Corrected Errors                                   │   │
│  │  mc0: csrow0: ch0: 15 Corrected Errors                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Step 2: 定位故障DIMM                                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  # 查看详细位置                                                      │   │
│  │  edac-util -v                                                        │   │
│  │                                                                       │   │
│  │  # 或通过BMC查看                                                     │   │
│  │  ipmitool sel elist | grep -i dimm                                   │   │
│  │                                                                       │   │
│  │  # 对应物理位置                                                      │   │
│  │  dmidecode -t memory | grep -A 10 "Memory Device"                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Step 3: 验证故障                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  方法1: 交换测试                                                     │   │
│  │    - 交换可疑DIMM位置                                                │   │
│  │    - 错误是否跟随DIMM移动                                            │   │
│  │                                                                       │   │
│  │  方法2: 压力测试                                                     │   │
│  │    - 运行memtest86+                                                  │   │
│  │    - 运行stress-ng内存测试                                           │   │
│  │                                                                       │   │
│  │  方法3: 隔离测试                                                     │   │
│  │    - 只保留可疑DIMM运行                                              │   │
│  │    - 验证问题复现                                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Step 4: 决策处理                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  错误判定标准:                                                       │   │
│  │                                                                       │   │
│  │  CE错误:                                                             │   │
│  │    - 偶发(<1次/天): 监控观察                                        │   │
│  │    - 频繁(>10次/天): 计划更换                                       │   │
│  │    - 快速增长: 尽快更换                                              │   │
│  │                                                                       │   │
│  │  UCE错误:                                                            │   │
│  │    - 任何UCE: 立即更换                                               │   │
│  │                                                                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 内存压力测试

```bash
#!/bin/bash
# 内存压力测试脚本

# memtester测试
run_memtester() {
    local mem_size="${1:-1G}"
    local loops="${2:-1}"
    
    echo "运行memtester: ${mem_size}, ${loops}次循环"
    memtester "$mem_size" "$loops"
}

# stress-ng内存测试
run_stress_ng() {
    local duration="${1:-300}"
    
    echo "运行stress-ng内存测试: ${duration}秒"
    stress-ng --vm 2 --vm-bytes 75% --timeout "${duration}s" --metrics
}

# 监控ECC错误
monitor_ecc() {
    local duration="${1:-60}"
    local interval="${2:-5}"
    
    echo "监控ECC错误: ${duration}秒，间隔${interval}秒"
    
    start_time=$(date +%s)
    end_time=$((start_time + duration))
    
    while [ $(date +%s) -lt $end_time ]; do
        echo "--- $(date) ---"
        edac-util -s 2>/dev/null || cat /sys/devices/system/edac/mc/mc*/ce_count 2>/dev/null
        sleep $interval
    done
}

# 综合测试
full_memory_test() {
    echo "=== 开始内存全面测试 ==="
    
    # 记录初始错误计数
    echo "初始ECC计数:"
    edac-util -s 2>/dev/null
    
    # 启动监控
    monitor_ecc 600 10 &
    monitor_pid=$!
    
    # 运行压力测试
    run_stress_ng 600
    
    # 停止监控
    kill $monitor_pid 2>/dev/null
    
    # 最终错误计数
    echo "最终ECC计数:"
    edac-util -s 2>/dev/null
    
    echo "=== 测试完成 ==="
}

# 执行选定测试
case "${1:-full}" in
    memtester)
        run_memtester "$2" "$3"
        ;;
    stress)
        run_stress_ng "$2"
        ;;
    monitor)
        monitor_ecc "$2" "$3"
        ;;
    full)
        full_memory_test
        ;;
    *)
        echo "用法: $0 {memtester|stress|monitor|full} [参数]"
        ;;
esac
```

### 常见内存问题与解决

```yaml
内存问题速查:
  问题: 系统不识别全部内存
    可能原因:
      - DIMM未插紧
      - 金手指氧化
      - DIMM故障
      - 内存槽故障
      - BIOS设置问题
    排查步骤:
      1. 检查BIOS内存显示
      2. 重新插拔DIMM
      3. 清洁金手指
      4. 交换槽位测试
      5. 单条测试隔离
      
  问题: 频繁ECC可纠正错误
    可能原因:
      - 内存颗粒退化
      - 电压不稳
      - 环境因素
    排查步骤:
      1. 确认错误位置
      2. 检查环境温度
      3. 监控错误趋势
      4. 计划更换故障DIMM
      
  问题: 系统频繁崩溃
    可能原因:
      - 内存UCE错误
      - 内存控制器故障
      - 内存配置问题
    排查步骤:
      1. 检查系统日志MCE
      2. 运行memtest86+
      3. 检查DIMM配置
      4. 测试最小化配置
      
  问题: 内存性能下降
    可能原因:
      - 通道不平衡
      - 频率降级
      - 配置不当
    排查步骤:
      1. 检查通道填充
      2. 确认运行频率
      3. 验证配置对称性
```

## 联合故障排查

### CPU与内存交互问题

```yaml
CPU-内存交互故障:
  内存控制器故障:
    症状:
      - 特定通道所有DIMM失效
      - 内存带宽异常低
      - 特定NUMA节点问题
    诊断:
      - 检查通道错误分布
      - 测试不同通道DIMM
      - BIOS内存控制器日志
    处理:
      - 可能需要更换CPU
      
  NUMA配置问题:
    症状:
      - 内存访问延迟高
      - 特定核心性能差
    诊断:
      numactl --hardware
      numastat
    处理:
      - 检查NUMA配置
      - 优化进程绑定
      
  CPU-内存电压问题:
    症状:
      - 高负载下不稳定
      - 温度正常但出错
    诊断:
      - 检查VCCIO/VCCSA电压
      - 监控供电模块
    处理:
      - 调整BIOS电压设置
      - 更换供电模块
```

## 参考资源

- [Intel MCE解码指南](https://www.intel.com/)
- [AMD MCE说明文档](https://www.amd.com/)
- [Linux EDAC文档](https://www.kernel.org/doc/html/latest/admin-guide/ras.html)
- [memtest86+](https://www.memtest.org/)
- [mcelog工具](https://mcelog.org/)
