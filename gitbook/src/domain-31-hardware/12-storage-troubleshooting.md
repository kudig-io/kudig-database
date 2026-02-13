# 存储设备故障排查

## 概述

存储设备故障直接影响数据安全和业务连续性。本文档详细解析HDD、SSD和RAID控制器的故障诊断、排查流程及数据恢复方法。

## HDD故障排查

### HDD故障类型

```yaml
HDD故障分类:
  机械故障:
    磁头故障:
      症状: 异响、读写错误、无法识别
      严重性: 高，可能导致盘片损伤
      
    主轴电机故障:
      症状: 无法起转、转速异常
      严重性: 高，硬盘无法工作
      
    伺服系统故障:
      症状: 寻道异常、定位错误
      严重性: 中，影响读写速度
      
  电子故障:
    PCB损坏:
      症状: 无法供电、无法识别
      原因: 电涌、静电、过热
      
    固件损坏:
      症状: 识别异常、容量错误
      
  逻辑故障:
    坏扇区:
      症状: 读写错误、性能下降
      处理: 重映射或标记
      
    S.M.A.R.T.预警:
      症状: 健康指标恶化
      处理: 预防性更换
```

### S.M.A.R.T.诊断

```bash
#!/bin/bash
# HDD S.M.A.R.T.诊断脚本

check_smart() {
    local device=$1
    
    echo "=== ${device} S.M.A.R.T.诊断 ==="
    
    # 检查SMART支持
    if ! smartctl -i "$device" | grep -q "SMART support is: Available"; then
        echo "设备不支持S.M.A.R.T."
        return 1
    fi
    
    # 整体健康状态
    echo -e "\n--- 健康状态 ---"
    smartctl -H "$device"
    
    # 关键属性
    echo -e "\n--- 关键属性 ---"
    smartctl -A "$device" | grep -E "Reallocated_Sector|Current_Pending|Offline_Uncorrectable|Reported_Uncorrect|Spin_Retry|Power_On_Hours|Temperature"
    
    # 错误日志
    echo -e "\n--- 错误日志 ---"
    smartctl -l error "$device" | head -30
    
    # 自检日志
    echo -e "\n--- 自检日志 ---"
    smartctl -l selftest "$device"
}

# 关键属性解读
analyze_smart() {
    local device=$1
    
    echo "=== S.M.A.R.T.属性分析 ==="
    
    # 获取关键属性值
    local reallocated=$(smartctl -A "$device" | grep "Reallocated_Sector_Ct" | awk '{print $10}')
    local pending=$(smartctl -A "$device" | grep "Current_Pending_Sector" | awk '{print $10}')
    local uncorrectable=$(smartctl -A "$device" | grep "Offline_Uncorrectable" | awk '{print $10}')
    local power_hours=$(smartctl -A "$device" | grep "Power_On_Hours" | awk '{print $10}')
    local temp=$(smartctl -A "$device" | grep "Temperature_Celsius" | awk '{print $10}')
    
    echo "重映射扇区: ${reallocated:-0}"
    echo "等待重映射: ${pending:-0}"
    echo "离线不可纠正: ${uncorrectable:-0}"
    echo "通电时间: ${power_hours:-0} 小时"
    echo "温度: ${temp:-N/A} °C"
    
    # 风险评估
    local risk="LOW"
    local reasons=""
    
    if [ "${reallocated:-0}" -gt 100 ]; then
        risk="HIGH"
        reasons+="重映射扇区过多; "
    elif [ "${reallocated:-0}" -gt 0 ]; then
        risk="MEDIUM"
        reasons+="存在重映射扇区; "
    fi
    
    if [ "${pending:-0}" -gt 0 ]; then
        risk="HIGH"
        reasons+="存在等待重映射扇区; "
    fi
    
    if [ "${uncorrectable:-0}" -gt 0 ]; then
        risk="HIGH"
        reasons+="存在不可纠正扇区; "
    fi
    
    echo -e "\n风险等级: $risk"
    [ -n "$reasons" ] && echo "原因: $reasons"
    
    case $risk in
        HIGH)
            echo "建议: 立即备份数据，准备更换硬盘"
            ;;
        MEDIUM)
            echo "建议: 密切监控，计划性更换"
            ;;
        LOW)
            echo "建议: 正常监控"
            ;;
    esac
}

# 启动自检
run_selftest() {
    local device=$1
    local type=${2:-short}
    
    echo "启动${type}自检..."
    smartctl -t "$type" "$device"
    
    echo "自检已启动，可使用 smartctl -l selftest $device 查看结果"
}

# 主程序
for dev in /dev/sd[a-z]; do
    [ -b "$dev" ] && check_smart "$dev"
done
```

## SSD故障排查

### SSD故障类型

```yaml
SSD故障分类:
  闪存故障:
    写入耗尽:
      症状: DWPD用尽、只读模式
      监控: Percentage_Used、TBW
      
    数据保持失效:
      症状: 长期断电后数据丢失
      原因: 电荷泄漏
      
    坏块累积:
      症状: 备用空间耗尽
      监控: Available_Spare
      
  控制器故障:
    固件问题:
      症状: 挂起、不响应
      处理: 固件更新
      
    硬件失效:
      症状: 完全无法识别
      
  掉电问题:
    数据丢失:
      原因: 无PLP的SSD异常断电
      预防: 使用企业级SSD
```

### NVMe诊断

```bash
#!/bin/bash
# NVMe SSD诊断脚本

diagnose_nvme() {
    local device=$1
    
    echo "=== NVMe设备诊断: $device ==="
    
    # 设备信息
    echo -e "\n--- 设备信息 ---"
    nvme id-ctrl "$device" | grep -E "sn|mn|fr|tnvmcap"
    
    # SMART信息
    echo -e "\n--- SMART日志 ---"
    nvme smart-log "$device"
    
    # 错误日志
    echo -e "\n--- 错误日志 ---"
    nvme error-log "$device" | head -20
}

analyze_nvme_health() {
    local device=$1
    
    echo "=== NVMe健康分析 ==="
    
    # 获取SMART数据
    local smart_output=$(nvme smart-log "$device" -o json)
    
    # 解析关键指标
    local critical_warning=$(echo "$smart_output" | jq -r '.critical_warning')
    local avail_spare=$(echo "$smart_output" | jq -r '.avail_spare')
    local spare_thresh=$(echo "$smart_output" | jq -r '.spare_thresh')
    local percent_used=$(echo "$smart_output" | jq -r '.percent_used')
    local media_errors=$(echo "$smart_output" | jq -r '.media_errors')
    local temp=$(echo "$smart_output" | jq -r '.temperature')
    
    # 温度转换(Kelvin to Celsius)
    temp=$((temp - 273))
    
    echo "关键警告标志: $critical_warning"
    echo "可用备用空间: ${avail_spare}%"
    echo "备用空间阈值: ${spare_thresh}%"
    echo "寿命消耗: ${percent_used}%"
    echo "介质错误: $media_errors"
    echo "温度: ${temp}°C"
    
    # 健康评估
    local status="HEALTHY"
    local issues=""
    
    if [ "$critical_warning" != "0" ]; then
        status="CRITICAL"
        issues+="存在关键警告; "
    fi
    
    if [ "$avail_spare" -lt "$spare_thresh" ]; then
        status="CRITICAL"
        issues+="备用空间低于阈值; "
    fi
    
    if [ "$percent_used" -gt 90 ]; then
        status="WARNING"
        issues+="寿命消耗超过90%; "
    fi
    
    if [ "$media_errors" -gt 0 ]; then
        status="WARNING"
        issues+="存在介质错误; "
    fi
    
    if [ "$temp" -gt 70 ]; then
        status="WARNING"
        issues+="温度过高; "
    fi
    
    echo -e "\n健康状态: $status"
    [ -n "$issues" ] && echo "问题: $issues"
}

# 遍历所有NVMe设备
for nvme_dev in /dev/nvme?; do
    [ -c "$nvme_dev" ] && diagnose_nvme "$nvme_dev"
done
```

## RAID故障排查

### RAID状态诊断

```yaml
RAID状态:
  Optimal/Online:
    含义: 正常运行
    动作: 无需处理
    
  Degraded:
    含义: 存在故障盘，冗余已生效
    动作: 尽快更换故障盘
    风险: 再坏一盘可能丢数据
    
  Rebuilding:
    含义: 正在重建
    动作: 监控进度，避免中断
    注意: 重建期间性能下降
    
  Failed:
    含义: 阵列失效
    动作: 评估数据恢复可能
    
  Offline:
    含义: 阵列离线
    原因: 缺盘、控制器问题
```

### RAID诊断命令

```bash
#!/bin/bash
# RAID诊断脚本

# 软RAID (mdadm)
check_mdraid() {
    echo "=== 软RAID状态 ==="
    cat /proc/mdstat
    
    for md in /dev/md*; do
        if [ -b "$md" ]; then
            echo -e "\n--- $md 详细信息 ---"
            mdadm --detail "$md"
        fi
    done
}

# LSI/Broadcom MegaRAID (新版storcli)
check_storcli() {
    if ! command -v storcli64 &> /dev/null; then
        echo "storcli未安装"
        return 1
    fi
    
    echo "=== MegaRAID控制器状态 ==="
    storcli64 /c0 show
    
    echo -e "\n=== 虚拟磁盘状态 ==="
    storcli64 /c0/vall show
    
    echo -e "\n=== 物理磁盘状态 ==="
    storcli64 /c0/eall/sall show
    
    # 检查告警
    echo -e "\n=== 告警信息 ==="
    storcli64 /c0/eall/sall show all | grep -E "State|Error|Predictive"
}

# LSI MegaRAID (旧版megacli)
check_megacli() {
    if ! command -v megacli &> /dev/null; then
        echo "megacli未安装"
        return 1
    fi
    
    echo "=== MegaRAID适配器信息 ==="
    megacli -AdpAllInfo -aALL
    
    echo -e "\n=== 逻辑磁盘信息 ==="
    megacli -LDInfo -Lall -aALL
    
    echo -e "\n=== 物理磁盘信息 ==="
    megacli -PDList -aALL | grep -E "Slot|Device Id|Inquiry|Firmware|State|Media Error|Predictive"
    
    # BBU状态
    echo -e "\n=== BBU状态 ==="
    megacli -AdpBbuCmd -GetBbuStatus -aALL
}

# HP Smart Array
check_ssacli() {
    if ! command -v ssacli &> /dev/null; then
        echo "ssacli未安装"
        return 1
    fi
    
    echo "=== HP Smart Array状态 ==="
    ssacli ctrl all show config
    
    echo -e "\n=== 物理磁盘状态 ==="
    ssacli ctrl slot=0 pd all show status
    
    echo -e "\n=== 逻辑磁盘状态 ==="
    ssacli ctrl slot=0 ld all show status
}

# 检测并运行相应工具
detect_and_check() {
    # 软RAID
    if [ -f /proc/mdstat ]; then
        check_mdraid
    fi
    
    # 硬件RAID
    if lspci | grep -qi "RAID\|MegaRAID\|Smart Array"; then
        check_storcli 2>/dev/null || check_megacli 2>/dev/null || check_ssacli 2>/dev/null
    fi
}

detect_and_check
```

### RAID故障处理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        RAID故障处理流程                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  RAID降级 (Degraded)                                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  Step 1: 确认故障盘                                                  │   │
│  │    storcli64 /c0/eall/sall show | grep -i "failed\|offline"         │   │
│  │                                                                       │   │
│  │  Step 2: 记录故障盘位置                                              │   │
│  │    - Enclosure ID                                                    │   │
│  │    - Slot ID                                                         │   │
│  │    - 序列号                                                          │   │
│  │                                                                       │   │
│  │  Step 3: 检查热备盘                                                  │   │
│  │    storcli64 /c0/eall/sall show | grep -i "hotspare"                │   │
│  │    - 如有热备，应自动启动重建                                        │   │
│  │                                                                       │   │
│  │  Step 4: 准备更换                                                    │   │
│  │    - 确认备件型号兼容                                                │   │
│  │    - 容量不小于原盘                                                  │   │
│  │    - 安排维护窗口                                                    │   │
│  │                                                                       │   │
│  │  Step 5: 更换故障盘                                                  │   │
│  │    - 物理更换硬盘                                                    │   │
│  │    - 新盘应自动重建                                                  │   │
│  │    - 手动触发: storcli64 /c0/eX/sY start rebuild                    │   │
│  │                                                                       │   │
│  │  Step 6: 监控重建进度                                                │   │
│  │    storcli64 /c0/eall/sall show rebuild                             │   │
│  │    - RAID 5/6 重建时间可能较长                                       │   │
│  │    - 避免重建期间大量I/O                                             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  RAID失效 (Failed)                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  紧急处理:                                                           │   │
│  │  1. 停止所有写入操作                                                 │   │
│  │  2. 记录当前状态和日志                                               │   │
│  │  3. 不要尝试初始化或重建                                             │   │
│  │  4. 联系专业数据恢复服务                                             │   │
│  │                                                                       │   │
│  │  可能的恢复方案:                                                     │   │
│  │  - Import Foreign Config (如果盘配置丢失)                            │   │
│  │  - Force Online (如果盘状态被误标)                                   │   │
│  │  - 专业数据恢复                                                      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 存储性能问题排查

### 性能诊断

```bash
#!/bin/bash
# 存储性能诊断脚本

echo "=== 磁盘I/O统计 ==="
iostat -xz 1 5

echo -e "\n=== 每设备详细统计 ==="
for dev in /dev/sd[a-z] /dev/nvme?n?; do
    if [ -b "$dev" ]; then
        echo "--- $dev ---"
        iostat -x "$dev" 1 3 | tail -4
    fi
done

echo -e "\n=== I/O延迟分析 ==="
# 使用blktrace (如已安装)
if command -v blktrace &> /dev/null; then
    echo "运行blktrace 5秒..."
    timeout 5 blktrace -d /dev/sda -o - | blkparse -i - 2>/dev/null | tail -20
fi

echo -e "\n=== 队列深度 ==="
for dev in /sys/block/sd*/queue /sys/block/nvme*/queue; do
    if [ -d "$dev" ]; then
        echo "$(dirname $dev): nr_requests=$(cat $dev/nr_requests)"
    fi
done

echo -e "\n=== 调度器设置 ==="
for dev in /sys/block/sd*/queue/scheduler /sys/block/nvme*/queue/scheduler; do
    [ -f "$dev" ] && echo "$(dirname $(dirname $dev)): $(cat $dev)"
done
```

### 性能问题速查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| 高延迟 | 队列满 | iostat -x | 增加队列深度 |
| 低IOPS | 磁盘瓶颈 | iostat | 升级SSD |
| 带宽低 | 接口瓶颈 | dmesg | 检查PCIe/SATA速率 |
| 不稳定 | 坏扇区 | smartctl | 更换磁盘 |
| RAID降速 | 重建中 | storcli | 等待重建完成 |

## 数据恢复指南

### 紧急恢复步骤

```yaml
数据恢复原则:
  第一原则: 不要写入
    - 停止所有写入操作
    - 不要尝试修复文件系统
    - 创建磁盘镜像
    
  第二原则: 保护现场
    - 记录所有状态信息
    - 拍照记录物理状态
    - 保存日志
    
  第三原则: 专业评估
    - 评估数据价值
    - 决定DIY还是专业恢复
    - 重要数据找专业服务

恢复工具:
  Linux工具:
    ddrescue: 磁盘镜像
    testdisk: 分区恢复
    photorec: 文件恢复
    extundelete: ext文件恢复
    
  商业工具:
    R-Studio: 全能恢复
    DiskGenius: Windows/Linux
    
  专业服务:
    - 物理故障需要无尘室
    - 费用较高但成功率高
```

## 参考资源

- [smartmontools](https://www.smartmontools.org/)
- [NVMe CLI](https://github.com/linux-nvme/nvme-cli)
- [Broadcom storcli](https://www.broadcom.com/)
- [Linux RAID Wiki](https://raid.wiki.kernel.org/)
