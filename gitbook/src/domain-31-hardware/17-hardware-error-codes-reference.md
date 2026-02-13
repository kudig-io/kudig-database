# 硬件错误码速查大全

## 概述

本文档汇总服务器硬件运维中常见的错误码、状态码和诊断信息，包括 MCE、SMART、IPMI/SEL、NVMe、BIOS 蜂鸣码等，为快速故障定位提供参考。

## MCE (Machine Check Exception) 错误码

### MCE 错误结构

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                           MCE 错误报告结构                                                   │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                             │
│  MCE 错误记录包含以下关键字段:                                                               │
│                                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────────────┐ │
│  │  CPU x BANK y                                                                          │ │
│  │  │    │                                                                                │ │
│  │  │    └─── Bank号: 指示错误来源功能单元                                                │ │
│  │  └─────── CPU核心号                                                                    │ │
│  │                                                                                        │ │
│  │  MISC xxxxxxxxxxxxxxxx    ─── 附加信息                                                 │ │
│  │  ADDR xxxxxxxxxxxxxxxx    ─── 故障地址                                                 │ │
│  │  STATUS xxxxxxxxxxxxxxxx  ─── 错误状态码                                               │ │
│  │  MCGSTATUS xxxxxxxx       ─── 全局状态                                                 │ │
│  │                                                                                        │ │
│  │  TIME xxxxx               ─── 时间戳                                                   │ │
│  │  SOCKETID x               ─── 物理CPU槽位                                              │ │
│  │  APICID x                 ─── APIC ID                                                  │ │
│  └───────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                             │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

### Intel MCE Bank 定义

```yaml
Intel_Xeon_MCE_Bank:
  Bank_0:
    名称: Data Cache Unit (DCU)
    功能单元: L1 数据缓存
    常见错误:
      - 数据缓存奇偶校验错误
      - 数据缓存 ECC 错误
      - DTLB 奇偶校验错误
    
  Bank_1:
    名称: Instruction Fetch Unit (IFU)
    功能单元: L1 指令缓存
    常见错误:
      - 指令缓存奇偶校验错误
      - 指令 TLB 错误
      - 预取错误
    
  Bank_2:
    名称: L2 Cache
    功能单元: 二级缓存
    常见错误:
      - L2 缓存 ECC 错误
      - L2 标签错误
    
  Bank_3:
    名称: System Bus Interface
    功能单元: 数据总线单元
    常见错误:
      - 总线奇偶校验错误
      - 总线超时
    
  Bank_4:
    名称: Memory Controller (Intel Internal)
    功能单元: 集成内存控制器
    常见错误:
      - 内存 ECC 错误
      - 内存通道错误
      - 内存链路错误
    
  Bank_5-7:
    名称: 内存控制器通道
    功能单元: 各内存通道
    常见错误:
      - 特定通道 ECC 错误
      - DIMM 故障
    
  Bank_8:
    名称: QPI/UPI Link
    功能单元: 处理器互联
    常见错误:
      - QPI/UPI 协议错误
      - 链路层 CRC 错误
    
  Bank_9+:
    名称: 平台特定
    功能单元: PCIe、IIO等
    说明: 具体定义因CPU型号而异
```

### AMD MCE Bank 定义

```yaml
AMD_EPYC_MCE_Bank:
  Bank_0:
    名称: Load-Store Unit (LS)
    功能单元: 加载存储单元
    常见错误:
      - L1 Data Cache 错误
      - Store Queue 错误
    
  Bank_1:
    名称: Instruction Fetch (IF)
    功能单元: 指令取指单元
    常见错误:
      - L1 Instruction Cache 错误
      - BP 错误
    
  Bank_2:
    名称: L2 Cache Unit (L2)
    功能单元: L2 缓存
    常见错误:
      - L2 Cache Data/Tag 错误
    
  Bank_3:
    名称: Decode Unit (DE)
    功能单元: 解码单元
    常见错误:
      - 微码 ROM 错误
    
  Bank_4:
    名称: Reserved
    
  Bank_5:
    名称: Execution Unit (EX)
    功能单元: 执行单元
    常见错误:
      - 浮点单元错误
      - 整数单元错误
    
  Bank_6:
    名称: Floating Point (FP)
    功能单元: 浮点单元
    
  Bank_7-14:
    名称: Unified Memory Controller (UMC)
    功能单元: 内存控制器
    常见错误:
      - ECC 错误
      - 内存训练失败
    
  Bank_15-22:
    名称: Coherent Slave (CS)
    功能单元: 一致性从站
    
  Bank_23-30:
    名称: Parameter Block (PB)
    功能单元: 参数块
```

### MCE STATUS 字段解析

```bash
#!/bin/bash
# MCE STATUS 字段解析脚本

decode_mce_status() {
    local status=$1
    
    echo "=== MCE STATUS 解析: $status ==="
    
    # 检查各标志位
    local val=$(( 16#${status#0x} ))
    
    # Bit 63: VAL (Valid)
    if (( (val >> 63) & 1 )); then
        echo "[VAL] 记录有效"
    fi
    
    # Bit 62: OVER (Overflow)
    if (( (val >> 62) & 1 )); then
        echo "[OVER] 有错误溢出(丢失)"
    fi
    
    # Bit 61: UC (Uncorrected)
    if (( (val >> 61) & 1 )); then
        echo "[UC] *** 不可纠正错误 ***"
    else
        echo "[CE] 可纠正错误"
    fi
    
    # Bit 60: EN (Error Enabled)
    if (( (val >> 60) & 1 )); then
        echo "[EN] 错误报告已启用"
    fi
    
    # Bit 59: MISCV (MISC Valid)
    if (( (val >> 59) & 1 )); then
        echo "[MISCV] MISC字段有效"
    fi
    
    # Bit 58: ADDRV (ADDR Valid)
    if (( (val >> 58) & 1 )); then
        echo "[ADDRV] 地址字段有效"
    fi
    
    # Bit 57: PCC (Processor Context Corrupt)
    if (( (val >> 57) & 1 )); then
        echo "[PCC] *** 处理器上下文已损坏 ***"
    fi
    
    # Bit 56: S (Signaling)
    if (( (val >> 56) & 1 )); then
        echo "[S] 信号错误"
    fi
    
    # Bit 55: AR (Action Required)
    if (( (val >> 55) & 1 )); then
        echo "[AR] *** 需要立即处理 ***"
    fi
    
    # MCA Error Code (Bits 15:0)
    local error_code=$((val & 0xFFFF))
    echo -e "\nMCA错误码: 0x$(printf '%04X' $error_code)"
    decode_mca_error_code $error_code
}

decode_mca_error_code() {
    local code=$1
    
    # 错误类型判断
    case $((code >> 8)) in
        0x00)
            echo "错误类型: 无错误"
            ;;
        0x01)
            echo "错误类型: 不可恢复检查点错误"
            ;;
        0x04)
            echo "错误类型: 数据读取错误"
            ;;
        0x05)
            echo "错误类型: 数据写入错误"
            ;;
        0x08|0x09|0x0A|0x0B)
            echo "错误类型: 通用缓存层次错误"
            decode_cache_error $code
            ;;
        0x0C|0x0D|0x0E|0x0F)
            echo "错误类型: TLB错误"
            decode_tlb_error $code
            ;;
        0x10|0x11|0x12|0x13|0x14|0x15|0x16|0x17)
            echo "错误类型: 内存控制器错误"
            decode_memory_error $code
            ;;
        *)
            echo "错误类型: 总线/互联错误或其他"
            ;;
    esac
}

decode_cache_error() {
    local code=$1
    local ll=$((code & 0x3))       # Cache Level
    local tt=$(((code >> 2) & 0x3))  # Transaction Type
    local rrrr=$(((code >> 4) & 0xF)) # Request Type
    
    echo "  缓存级别: L${ll}"
    case $tt in
        0) echo "  事务类型: 指令" ;;
        1) echo "  事务类型: 数据" ;;
        2) echo "  事务类型: 通用" ;;
    esac
}

decode_memory_error() {
    local code=$1
    echo "  内存错误子类型: $(printf '0x%02X' $((code & 0xFF)))"
    
    case $((code & 0xF)) in
        0x0) echo "  错误操作: 通用" ;;
        0x1) echo "  错误操作: 读取" ;;
        0x2) echo "  错误操作: 写入" ;;
        0x3) echo "  错误操作: 地址/命令" ;;
        0x4) echo "  错误操作: Scrub" ;;
    esac
}

# 示例使用
# decode_mce_status "0xBE0000000000011A"
```

### 常见 MCE 错误快速参考

| STATUS 特征 | 错误类型 | 严重程度 | 处理建议 |
|------------|---------|---------|---------|
| `UC=1, PCC=1` | 不可纠正且上下文损坏 | 致命 | 立即更换 CPU |
| `UC=1, PCC=0, AR=1` | 不可纠正但可恢复 | 严重 | 检查并计划更换 |
| `UC=0, OVER=0` | 单次可纠正错误 | 轻微 | 监控频率 |
| `UC=0, OVER=1` | 可纠正错误溢出 | 中等 | 增加监控粒度 |
| `Bank 4-7, UC=1` | 内存控制器 UCE | 严重 | 检查 DIMM |
| `Bank 0-2, UC=1` | 缓存严重错误 | 严重 | CPU 可能故障 |

## S.M.A.R.T. 属性码

### 关键 SMART 属性

```yaml
SMART_关键属性:
  ID_001_Raw_Read_Error_Rate:
    名称: 原始读取错误率
    重要性: 厂商相关
    说明: |
      不同厂商定义不同:
      - Seagate: 高值正常(内部统计)
      - WD: 低值为好
    阈值: 厂商定义
    
  ID_003_Spin_Up_Time:
    名称: 启动时间
    重要性: 中
    说明: 电机启动所需时间
    异常: 时间显著增加可能表示电机老化
    
  ID_005_Reallocated_Sector_Count:
    名称: 重映射扇区计数
    重要性: 关键
    说明: 已被替换的坏扇区数量
    阈值:
      安全: 0
      警告: 1-50
      危险: >100
    处理: 任何非零值都需关注
    
  ID_007_Seek_Error_Rate:
    名称: 寻道错误率
    重要性: 高
    说明: 磁头定位错误率
    异常: 急剧增加表示机械问题
    
  ID_009_Power_On_Hours:
    名称: 通电时间
    重要性: 参考
    说明: 总工作小时数
    企业盘预期寿命: 50000-70000小时
    
  ID_010_Spin_Retry_Count:
    名称: 启动重试次数
    重要性: 高
    说明: 电机启动失败重试次数
    阈值:
      安全: 0
      危险: 任何非零值
    
  ID_012_Power_Cycle_Count:
    名称: 通电循环次数
    重要性: 参考
    说明: 开关机次数
    
  ID_187_Reported_Uncorrect:
    名称: 报告的不可纠正错误
    重要性: 关键
    说明: UNC错误数量
    阈值:
      安全: 0
      危险: 任何非零值
    处理: 非零值立即备份更换
    
  ID_188_Command_Timeout:
    名称: 命令超时
    重要性: 高
    说明: 命令执行超时次数
    异常: 频繁增加表示固件或硬件问题
    
  ID_194_Temperature_Celsius:
    名称: 温度
    重要性: 中
    说明: 当前温度
    阈值:
      正常: <45°C
      警告: 45-55°C
      危险: >55°C
    
  ID_196_Reallocated_Event_Count:
    名称: 重映射事件计数
    重要性: 高
    说明: 发生重映射的次数
    与ID_005区别: 计算次数而非扇区数
    
  ID_197_Current_Pending_Sector:
    名称: 当前等待重映射扇区
    重要性: 关键
    说明: 等待被重映射的不稳定扇区
    阈值:
      安全: 0
      危险: 任何非零值
    说明: 非零表示有扇区读取不稳定
    
  ID_198_Offline_Uncorrectable:
    名称: 离线不可纠正扇区
    重要性: 关键
    说明: 离线扫描发现的不可修复扇区
    阈值:
      安全: 0
      危险: 任何非零值
    
  ID_199_UDMA_CRC_Error_Count:
    名称: UDMA CRC错误计数
    重要性: 中
    说明: 接口传输错误
    异常原因: 数据线/接口问题而非磁盘本身
    
  ID_200_Write_Error_Rate:
    名称: 写入错误率
    重要性: 高
    说明: 写入时发生的错误
```

### SSD 特有属性

```yaml
SSD_SMART_属性:
  ID_170_Available_Reserved_Space:
    名称: 可用保留空间
    重要性: 关键
    说明: 剩余备用块百分比
    阈值:
      健康: >20%
      警告: 10-20%
      危险: <10%
    
  ID_171_SSD_Program_Fail_Count:
    名称: 编程失败计数
    重要性: 高
    说明: NAND 编程操作失败次数
    
  ID_172_SSD_Erase_Fail_Count:
    名称: 擦除失败计数
    重要性: 高
    说明: NAND 擦除操作失败次数
    
  ID_173_SSD_Wear_Leveling_Count:
    名称: 磨损均衡计数
    重要性: 参考
    说明: 平均擦除次数
    
  ID_174_Unexpected_Power_Loss:
    名称: 意外断电计数
    重要性: 中
    说明: 非正常断电次数
    对于无PLP SSD: 可能导致数据丢失
    
  ID_177_Wear_Range_Delta:
    名称: 磨损差值
    重要性: 中
    说明: 最大与最小擦除次数差
    异常: 过大表示磨损均衡不佳
    
  ID_180_Unused_Reserved_Block_Count:
    名称: 未使用保留块计数
    重要性: 高
    说明: 可用于替换的坏块数量
    
  ID_181_Program_Fail_Count_Total:
    名称: 总编程失败计数
    重要性: 高
    
  ID_182_Erase_Fail_Count_Total:
    名称: 总擦除失败计数
    重要性: 高
    
  ID_231_SSD_Life_Left:
    名称: SSD剩余寿命
    重要性: 关键
    说明: 剩余寿命百分比
    阈值:
      健康: >20%
      警告: 10-20%
      危险: <10%
    
  ID_232_Endurance_Remaining:
    名称: 剩余耐久度
    重要性: 关键
    说明: 基于写入量的剩余寿命
    
  ID_233_Media_Wearout_Indicator:
    名称: 介质磨损指示器
    重要性: 关键
    说明: 100为新盘，0为寿命耗尽
    
  ID_241_Total_LBAs_Written:
    名称: 总写入LBA数
    重要性: 参考
    说明: 用于计算总写入量(TBW)
    
  ID_242_Total_LBAs_Read:
    名称: 总读取LBA数
    重要性: 参考
```

### SMART 状态快速判断

```bash
#!/bin/bash
# SMART 快速健康评估脚本

evaluate_smart() {
    local device=$1
    local status="HEALTHY"
    local issues=""
    
    echo "=== $device SMART 健康评估 ==="
    
    # 获取关键属性
    local smart_output=$(smartctl -A "$device" 2>/dev/null)
    
    # 检查整体状态
    if ! smartctl -H "$device" 2>/dev/null | grep -q "PASSED"; then
        status="CRITICAL"
        issues+="[SMART整体状态: FAILED] "
    fi
    
    # 检查重映射扇区 (ID 5)
    local reallocated=$(echo "$smart_output" | grep "Reallocated_Sector_Ct" | awk '{print $10}')
    if [ -n "$reallocated" ] && [ "$reallocated" -gt 0 ]; then
        if [ "$reallocated" -gt 100 ]; then
            status="CRITICAL"
            issues+="[重映射扇区: $reallocated (危险)] "
        else
            [ "$status" != "CRITICAL" ] && status="WARNING"
            issues+="[重映射扇区: $reallocated] "
        fi
    fi
    
    # 检查等待重映射 (ID 197)
    local pending=$(echo "$smart_output" | grep "Current_Pending_Sector" | awk '{print $10}')
    if [ -n "$pending" ] && [ "$pending" -gt 0 ]; then
        status="CRITICAL"
        issues+="[等待重映射: $pending] "
    fi
    
    # 检查不可纠正扇区 (ID 198)
    local offline_unc=$(echo "$smart_output" | grep "Offline_Uncorrectable" | awk '{print $10}')
    if [ -n "$offline_unc" ] && [ "$offline_unc" -gt 0 ]; then
        status="CRITICAL"
        issues+="[离线不可纠正: $offline_unc] "
    fi
    
    # 检查报告的UNC错误 (ID 187)
    local reported_unc=$(echo "$smart_output" | grep "Reported_Uncorrect" | awk '{print $10}')
    if [ -n "$reported_unc" ] && [ "$reported_unc" -gt 0 ]; then
        status="CRITICAL"
        issues+="[报告UNC: $reported_unc] "
    fi
    
    # 检查温度 (ID 194)
    local temp=$(echo "$smart_output" | grep "Temperature_Celsius" | awk '{print $10}')
    if [ -n "$temp" ] && [ "$temp" -gt 55 ]; then
        [ "$status" == "HEALTHY" ] && status="WARNING"
        issues+="[温度: ${temp}°C] "
    fi
    
    # 输出结果
    echo "状态: $status"
    [ -n "$issues" ] && echo "问题: $issues"
    
    case $status in
        HEALTHY)
            echo "建议: 继续正常监控"
            ;;
        WARNING)
            echo "建议: 增加监控频率，计划更换"
            ;;
        CRITICAL)
            echo "建议: 立即备份数据，尽快更换"
            ;;
    esac
}

# 检查所有磁盘
for disk in /dev/sd? /dev/nvme?n?; do
    [ -b "$disk" ] && evaluate_smart "$disk"
    echo ""
done
```

## NVMe 错误码

### NVMe SMART 日志字段

```yaml
NVMe_SMART_日志:
  critical_warning:
    描述: 关键警告位图
    位定义:
      Bit_0: 备用空间低于阈值
      Bit_1: 温度超过阈值
      Bit_2: 可靠性降低
      Bit_3: 进入只读模式
      Bit_4: 易失性内存备份设备故障
      Bit_5: 持久内存只读
    正常值: 0
    任何非零: 需要立即关注
    
  temperature:
    描述: 复合温度(Kelvin)
    转换: Celsius = Kelvin - 273
    阈值:
      正常: <70°C
      警告: 70-80°C
      危险: >80°C
    
  available_spare:
    描述: 可用备用空间百分比
    阈值:
      健康: >20%
      警告: 10-20%
      危险: <10%
    
  available_spare_threshold:
    描述: 备用空间阈值
    说明: 低于此值时 critical_warning Bit 0 置位
    
  percentage_used:
    描述: 寿命消耗百分比
    说明: 可能超过100%(超出预期寿命)
    阈值:
      正常: <80%
      警告: 80-100%
      关注: >100%
    
  data_units_read:
    描述: 读取数据量(单位: 1000*512B)
    用途: 计算总读取量
    
  data_units_written:
    描述: 写入数据量(单位: 1000*512B)
    用途: 计算TBW，评估寿命
    
  host_read_commands:
    描述: 主机读命令数
    
  host_write_commands:
    描述: 主机写命令数
    
  controller_busy_time:
    描述: 控制器忙碌时间(分钟)
    
  power_cycles:
    描述: 电源循环次数
    
  power_on_hours:
    描述: 通电小时数
    
  unsafe_shutdowns:
    描述: 不安全关机次数
    说明: 非正常断电计数
    
  media_errors:
    描述: 介质和数据完整性错误
    重要性: 关键
    正常值: 0
    任何非零: 可能表示闪存问题
    
  num_err_log_entries:
    描述: 错误日志条目数
    用途: 检查是否有错误记录
```

### NVMe 命令状态码

```yaml
NVMe_Status_Code:
  Generic_Command_Status:
    0x00: 成功完成
    0x01: 无效命令操作码
    0x02: 无效字段
    0x03: 命令ID冲突
    0x04: 数据传输错误
    0x05: 命令因电源丢失中止
    0x06: 内部错误
    0x07: 命令中止(请求)
    0x08: 命令中止(SQ删除)
    0x09: 命令中止(融合失败)
    0x0A: 命令中止(缺少融合)
    0x0B: 无效命名空间或格式
    0x0C: 命令序列错误
    0x0D: 无效SGL段描述符
    0x0E: 无效SGL描述符数量
    0x0F: 数据SGL长度无效
    0x10: 元数据SGL长度无效
    0x11: SGL描述符类型无效
    0x12: 无效CMB使用
    0x13: 无效PRP偏移
    0x14: 原子写入单元超出
    0x15: 操作被拒绝
    0x16: SGL偏移类型无效
    0x18: 主机标识符格式不一致
    0x19: 保持活动定时器过期
    0x1A: 保持活动超时无效
    0x1C: 中止命令超出限制
    0x20: 命名空间未就绪
    0x21: 格式化进行中
    0x22: 无效队列标识符
    
  Command_Specific_Status:
    0x80: 完成队列无效
    0x81: 无效队列ID
    0x82: 无效队列大小
    0x83: 中止命令限制超出
    0x86: 无效中断向量
    0x87: 无效日志页
    0x88: 无效格式
    0x89: 固件激活需要重置
    0x8A: 无效队列删除
    0x8B: 功能ID不可保存
    0x8C: 功能未更改
    0x8D: 功能未保存
    0x8E: 固件激活需要子系统重置
    0x90: 流ID资源耗尽
    0x91: 流ID打开限制超出
    0x92: 没有打开的流ID
    
  Media_and_Data_Integrity_Errors:
    0x80: 写入错误
    0x81: 未恢复读取错误
    0x82: 端到端保护错误
    0x83: 比较失败
    0x84: 访问被拒绝
    0x85: 释放未分配
    0x86: 端到端参考标签检查失败
    0x87: 端到端应用标签检查失败
```

## IPMI SEL 事件类型

### SEL 事件类型码

```yaml
IPMI_SEL_Event_Type:
  Type_0x01: 阈值传感器
  Type_0x02-0x0C: 离散传感器
  Type_0x6F: 传感器特定
  Type_0x70-0x7F: OEM定义
  
IPMI_Sensor_Type:
  0x01_Temperature:
    事件:
      Upper_Non_Critical: 温度超上限(非关键)
      Upper_Critical: 温度超上限(关键)
      Upper_Non_Recoverable: 温度不可恢复
      Lower_Non_Critical: 温度低于下限(非关键)
      
  0x02_Voltage:
    事件:
      Upper_Critical: 电压过高(关键)
      Lower_Critical: 电压过低(关键)
      
  0x04_Fan:
    事件:
      Lower_Non_Critical: 转速过低(警告)
      Lower_Critical: 转速过低(关键)
      Lower_Non_Recoverable: 风扇失效
      
  0x05_Physical_Security:
    事件:
      Chassis_Intrusion: 机箱入侵
      
  0x07_Processor:
    事件:
      IERR: 内部错误
      Thermal_Trip: 热保护触发
      FRB1_BIST_Failure: BIST失败
      FRB2_Hang: 挂起
      FRB3_Failure: 启动失败
      Config_Error: 配置错误
      Uncorrectable_Error: 不可纠正错误
      Correctable_Error: 可纠正错误
      
  0x0C_Memory:
    事件:
      Correctable_ECC: ECC可纠正错误
      Uncorrectable_ECC: ECC不可纠正错误
      Parity: 奇偶校验错误
      Memory_Scrub_Failed: 内存清洗失败
      Memory_Device_Disabled: 内存设备禁用
      Correctable_Threshold_Reached: 可纠正阈值达到
      Spare_Available: 备用可用
      Memory_Presence: 内存存在检测
      Configuration_Error: 配置错误
      
  0x0D_Drive_Slot:
    事件:
      Drive_Presence: 驱动器存在
      Drive_Fault: 驱动器故障
      Predictive_Failure: 预测性故障
      Hot_Spare: 热备盘
      Rebuild_In_Progress: 重建中
      Rebuild_Aborted: 重建中止
      
  0x13_Critical_Interrupt:
    事件:
      NMI: 不可屏蔽中断
      Bus_Timeout: 总线超时
      IO_Channel_Check: I/O通道检查
      Software_NMI: 软件NMI
      PCI_PERR: PCI奇偶校验错误
      PCI_SERR: PCI系统错误
      EISA_Fail_Safe: EISA失败保护
      Bus_Correctable_Error: 总线可纠正错误
      Bus_Uncorrectable_Error: 总线不可纠正错误
      Fatal_NMI: 致命NMI
      
  0x20_OS_Boot:
    事件:
      A_Boot_Completed: A启动完成
      C_Boot_Completed: C启动完成
      PXE_Boot_Server_Not_Found: PXE服务器未找到
      OS_Boot_Failed: OS启动失败
```

### SEL 日志解析脚本

```bash
#!/bin/bash
# IPMI SEL 日志分析脚本

analyze_sel() {
    echo "=== IPMI SEL 事件分析 ==="
    
    # 获取SEL信息
    echo "--- SEL 存储信息 ---"
    ipmitool sel info
    
    # 获取所有事件
    echo -e "\n--- 所有事件 ---"
    ipmitool sel elist
    
    # 关键事件筛选
    echo -e "\n--- 关键事件 ---"
    ipmitool sel elist | grep -iE "critical|error|fail|fault|assert"
    
    # 按传感器类型分类
    echo -e "\n--- CPU相关事件 ---"
    ipmitool sel elist | grep -i "cpu\|processor"
    
    echo -e "\n--- 内存相关事件 ---"
    ipmitool sel elist | grep -i "memory\|dimm"
    
    echo -e "\n--- 磁盘相关事件 ---"
    ipmitool sel elist | grep -i "disk\|drive\|hdd\|ssd"
    
    echo -e "\n--- 电源相关事件 ---"
    ipmitool sel elist | grep -i "power\|psu\|volt"
    
    echo -e "\n--- 温度相关事件 ---"
    ipmitool sel elist | grep -i "temp\|thermal"
    
    echo -e "\n--- 风扇相关事件 ---"
    ipmitool sel elist | grep -i "fan"
}

# 清理SEL (谨慎使用)
clear_sel() {
    read -p "确定要清空SEL日志吗? (yes/no): " confirm
    if [ "$confirm" == "yes" ]; then
        ipmitool sel clear
        echo "SEL已清空"
    fi
}

# 执行分析
analyze_sel
```

## BIOS 蜂鸣码

### AMI BIOS 蜂鸣码

| 蜂鸣模式 | 含义 | 可能原因 | 处理方法 |
|---------|------|---------|---------|
| 1短 | DRAM刷新失败 | 内存问题 | 重新插拔内存/更换 |
| 2短 | 内存奇偶校验错误 | 内存故障 | 更换内存 |
| 3短 | 基本64K内存错误 | 内存/主板问题 | 检测内存和主板 |
| 4短 | 系统定时器失败 | 主板问题 | 检查/更换主板 |
| 5短 | 处理器错误 | CPU问题 | 检查CPU安装/更换 |
| 6短 | 键盘控制器错误 | 键盘/主板问题 | 检查键盘/主板 |
| 7短 | 虚拟模式异常 | CPU问题 | CPU可能故障 |
| 8短 | 显卡内存错误 | 显卡问题 | 检查/更换显卡 |
| 9短 | ROM校验失败 | BIOS损坏 | 刷新BIOS |
| 10短 | CMOS读写错误 | CMOS电池/芯片 | 更换电池/检查主板 |
| 11短 | 缓存错误 | CPU缓存问题 | 检查/更换CPU |
| 1长1短 | 主板问题 | 主板故障 | 检查/更换主板 |
| 1长2短 | 显卡错误 | 显卡/插槽问题 | 重新安装显卡 |
| 1长3短 | 内存错误 | 内存问题 | 检测/更换内存 |
| 1长8短 | 显示测试失败 | 显卡问题 | 检查显卡 |
| 连续短响 | 电源问题 | PSU故障 | 检查电源 |
| 连续长响 | 内存未检测到 | 内存未安装 | 安装内存 |

### Phoenix/Award BIOS 蜂鸣码

| 蜂鸣模式 | 含义 | 处理方法 |
|---------|------|---------|
| 1-1-3 | CMOS读/写失败 | 检查电池/主板 |
| 1-1-4 | BIOS ROM校验失败 | 刷新BIOS |
| 1-2-1 | 定时器失败 | 检查主板 |
| 1-2-2 | DMA失败 | 检查主板 |
| 1-2-3 | DMA页寄存器失败 | 检查主板 |
| 1-3-1 | 内存刷新失败 | 检查内存 |
| 1-3-3 | 基本64K内存失败 | 检查内存 |
| 1-3-4 | 基本64K内存奇偶失败 | 检查内存 |
| 1-4-1 | 地址线失败 | 检查主板 |
| 1-4-2 | 内存奇偶错误 | 检查内存 |
| 2-x-x | 基本64K内存失败 | 检查内存 |
| 3-1-1 | 从属DMA寄存器失败 | 检查主板 |
| 3-1-2 | 主DMA寄存器失败 | 检查主板 |
| 3-1-3 | 主中断掩码寄存器失败 | 检查主板 |
| 3-1-4 | 从中断掩码寄存器失败 | 检查主板 |
| 3-2-4 | 键盘控制器失败 | 检查键盘/主板 |
| 3-3-4 | 显示内存失败 | 检查显卡 |
| 3-4-1 | 显示初始化失败 | 检查显卡 |
| 4-2-1 | 定时器中断失败 | 检查主板 |
| 4-2-2 | 关机失败 | 检查主板 |
| 4-2-3 | A20门失败 | 检查主板 |
| 4-2-4 | 保护模式中断失败 | 检查CPU/主板 |
| 4-3-1 | 内存超过64K失败 | 检查内存 |
| 4-3-3 | 定时器2失败 | 检查主板 |
| 4-4-1 | 串口失败 | 检查主板 |
| 4-4-2 | 并口失败 | 检查主板 |
| 4-4-3 | 协处理器失败 | 检查CPU |

## 服务器厂商专用诊断码

### Dell PowerEdge LCD 错误码

```yaml
Dell_LCD_Error_Code:
  E1000:
    描述: 电源故障
    原因: PSU失效或功率不足
    处理: 检查/更换电源
    
  E1114:
    描述: 环境温度超出范围
    原因: 机房温度过高
    处理: 检查机房空调
    
  E1116:
    描述: 内存温度超出范围
    原因: 内存过热
    处理: 检查通风/内存
    
  E1210:
    描述: 主板电压错误
    原因: 主板电压调节问题
    处理: 联系支持
    
  E1211:
    描述: CPU VRM电压错误
    原因: CPU供电问题
    处理: 检查VRM模块
    
  E1216:
    描述: 3.3V稳压器故障
    原因: 主板问题
    处理: 更换主板
    
  E1229:
    描述: CPU VCORE故障
    原因: CPU供电异常
    处理: 检查CPU/主板
    
  E122A-E122C:
    描述: CPU VRM故障
    原因: VRM模块问题
    处理: 更换VRM
    
  E1310:
    描述: 风扇缺失
    原因: 风扇未安装
    处理: 安装风扇
    
  E1311:
    描述: 风扇故障
    原因: 风扇失效
    处理: 更换风扇
    
  E1410:
    描述: 系统板温度过高
    原因: 系统散热问题
    处理: 检查散热
    
  E1414:
    描述: 芯片组温度过高
    原因: 芯片组散热问题
    处理: 检查散热
    
  E1418:
    描述: VRM温度过高
    原因: VRM散热问题
    处理: 检查VRM散热
    
  E1422:
    描述: iDRAC温度过高
    原因: iDRAC散热问题
    处理: 检查系统散热
    
  E1618:
    描述: Power Supply冗余丢失
    原因: 一个PSU失效
    处理: 更换故障PSU
    
  E161C:
    描述: Power Supply输入丢失
    原因: 电源输入问题
    处理: 检查电源线
    
  E1620:
    描述: Power Supply输入范围
    原因: 电压不在范围内
    处理: 检查供电
    
  E1710:
    描述: I/O通道检查
    原因: PCIe错误
    处理: 检查PCIe设备
    
  E1714:
    描述: PCI PERR
    原因: PCI奇偶错误
    处理: 检查PCIe设备
    
  E1A14:
    描述: DIMM温度过高
    原因: 内存过热
    处理: 检查散热/更换内存
    
  E1A1D:
    描述: DIMM ECC错误
    原因: 内存ECC错误
    处理: 更换故障DIMM
    
  E2010:
    描述: 内存初始化失败
    原因: 内存问题
    处理: 检查/更换内存
    
  E2011:
    描述: 内存配置错误
    原因: 内存配置不正确
    处理: 检查内存配置
    
  E2012:
    描述: 内存训练失败
    原因: 内存兼容/故障
    处理: 更换内存
    
  E201C:
    描述: 内存备用触发
    原因: 内存错误达阈值
    处理: 更换故障DIMM
    
  E201D:
    描述: 内存镜像触发
    原因: 镜像内存问题
    处理: 检查内存
    
  E2113:
    描述: CPU温度过高
    原因: CPU散热问题
    处理: 检查散热器/硅脂
```

### HPE iLO 状态码

```yaml
HPE_iLO_Status:
  Health_Status:
    OK: 正常
    Degraded: 降级(有警告)
    Critical: 关键(需立即处理)
    
  Component_Status:
    Fans:
      OK: 所有风扇正常
      Degraded: 部分风扇异常
      Critical: 风扇严重故障
      
    Power:
      OK: 电源正常
      Degraded: 冗余丢失
      Critical: 电源故障
      
    Temperature:
      OK: 温度正常
      Degraded: 温度警告
      Critical: 温度过高
      
    Memory:
      OK: 内存正常
      Degraded: 有ECC错误
      Critical: 内存故障
      
    Processors:
      OK: CPU正常
      Degraded: 有错误记录
      Critical: CPU故障
      
    Storage:
      OK: 存储正常
      Degraded: RAID降级/磁盘警告
      Critical: 存储故障
```

## 快速排错检查清单

```yaml
硬件故障快速检查清单:
  系统无法启动:
    检查顺序:
      1. 电源LED是否亮起
      2. PSU指示灯状态
      3. 主板待机LED
      4. POST蜂鸣码/LED
      5. BMC/iLO连接状态
    
  随机重启/宕机:
    检查顺序:
      1. dmesg | grep -i mce
      2. edac-util -s
      3. ipmitool sel elist
      4. sensors (温度)
      5. ipmitool sensor | grep -i power
    
  性能下降:
    检查顺序:
      1. turbostat (CPU频率)
      2. sensors (CPU温度)
      3. iostat -x (磁盘延迟)
      4. smartctl -A (磁盘健康)
      5. ethtool -S (网络错误)
    
  RAID告警:
    检查顺序:
      1. storcli64 /c0 show
      2. storcli64 /c0/eall/sall show
      3. ipmitool sel elist | grep -i drive
      4. smartctl -H /dev/sdX
```

## 参考资源

- [Intel MCE Error Codes](https://www.intel.com/)
- [AMD MCE Documentation](https://www.amd.com/)
- [SMART Attribute Reference](https://www.smartmontools.org/)
- [NVMe Specification](https://nvmexpress.org/)
- [IPMI Specification](https://www.intel.com/ipmi)
- [Dell Error Code Reference](https://www.dell.com/support)
- [HPE iLO User Guide](https://support.hpe.com/)
