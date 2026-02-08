# BIOS与固件故障排查

## 概述

BIOS/UEFI固件是服务器启动和硬件初始化的核心。本文档详细解析BIOS故障、POST错误、固件更新问题的诊断方法。

## BIOS/UEFI故障排查

### POST错误诊断

```yaml
POST错误类型:
  无显示输出:
    可能原因:
      - CPU故障
      - 内存故障
      - 显卡/VGA故障
      - 主板故障
    排查顺序:
      1. 检查电源和连接
      2. 观察主板LED
      3. 听蜂鸣代码
      4. 最小化配置测试
      
  卡在POST:
    可能原因:
      - 硬件检测失败
      - 配置冲突
      - 固件问题
    排查方法:
      - 查看POST代码
      - 检查BMC日志
      - 清除CMOS
      
  POST后无法启动OS:
    可能原因:
      - 启动设备问题
      - 启动顺序错误
      - 安全启动冲突
```

### POST代码参考

```
POST代码常见含义 (厂商可能有差异):

代码   含义                    处理
----   ----                    ----
00     系统关机                 检查电源
01     CPU初始化               检查CPU
02     CMOS/RTC检测            清除CMOS
03     BIOS完整性检查          重刷BIOS
04     内存初始化              检查内存
15     内存安装检测            重装内存
19     启动设备检测            检查存储
30     POST结束，准备启动      正常
AE     操作系统启动            正常

Dell服务器错误代码示例:
E1000   电源故障
E1210   CPU故障
E1310   内存故障
E2010   系统板故障
```

### BIOS设置问题

```bash
#!/bin/bash
# BIOS配置检查脚本

echo "=== BIOS信息检查 ==="

# BIOS版本
echo -e "\n--- BIOS版本 ---"
dmidecode -t bios | grep -E "Vendor|Version|Release"

# 系统信息
echo -e "\n--- 系统信息 ---"
dmidecode -t system | grep -E "Manufacturer|Product|Serial"

# 启动模式
echo -e "\n--- 启动模式 ---"
if [ -d /sys/firmware/efi ]; then
    echo "启动模式: UEFI"
    ls /sys/firmware/efi/efivars/ | head -5
else
    echo "启动模式: Legacy BIOS"
fi

# 安全启动状态
echo -e "\n--- 安全启动 ---"
if [ -f /sys/firmware/efi/efivars/SecureBoot-* ]; then
    sb_status=$(od -An -t u1 /sys/firmware/efi/efivars/SecureBoot-* 2>/dev/null | tail -1)
    if [ "$sb_status" == " 1" ]; then
        echo "Secure Boot: 已启用"
    else
        echo "Secure Boot: 已禁用"
    fi
else
    echo "Secure Boot: 不适用"
fi

# CPU微码
echo -e "\n--- CPU微码 ---"
cat /proc/cpuinfo | grep -m1 microcode

# ACPI信息
echo -e "\n--- ACPI表 ---"
ls /sys/firmware/acpi/tables/ | head -10
```

## 固件更新故障

### 固件更新风险

```yaml
固件更新注意事项:
  准备工作:
    - 备份当前BIOS配置
    - 确认电源稳定
    - 记录当前固件版本
    - 阅读发行说明
    
  更新风险:
    断电风险:
      影响: 可能导致系统无法启动
      预防: 使用UPS，避免高峰期
      
    兼容性问题:
      影响: 新固件可能不兼容现有配置
      预防: 验证测试环境
      
    功能回退:
      影响: 部分功能可能变化
      预防: 了解变更内容
      
  恢复方法:
    双BIOS:
      - 部分主板支持备份BIOS
      - 自动切换到备份
      
    BIOS恢复U盘:
      - 准备恢复介质
      - 按特定键恢复
      
    SPI编程器:
      - 物理刷写BIOS芯片
      - 需要专业工具
```

### BMC固件更新

```bash
#!/bin/bash
# BMC固件更新示例

# 检查当前BMC版本
check_bmc_version() {
    echo "=== 当前BMC版本 ==="
    ipmitool mc info | grep -E "Firmware|Manufacturer|Product"
}

# 使用ipmitool更新 (厂商命令可能不同)
update_bmc_firmware() {
    local firmware_file=$1
    
    echo "警告: 即将更新BMC固件"
    echo "固件文件: $firmware_file"
    echo "更新过程中请勿断电!"
    
    read -p "确认继续? (yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        echo "已取消"
        return 1
    fi
    
    # 实际命令因厂商而异
    # Dell: racadm update -f $firmware_file
    # HPE: iLO命令行或Web界面
    # 浪潮: ipmitool hpm upgrade $firmware_file
}

# 使用Redfish更新
update_via_redfish() {
    local bmc_ip=$1
    local user=$2
    local pass=$3
    local image_url=$4
    
    curl -sk -u "$user:$pass" \
        -X POST \
        -H "Content-Type: application/json" \
        -d "{\"ImageURI\":\"$image_url\"}" \
        "https://$bmc_ip/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate"
}

check_bmc_version
```

## 常见BIOS问题

### 问题速查表

| 问题 | 症状 | 可能原因 | 解决方案 |
|------|------|----------|----------|
| 无法启动 | 黑屏/无POST | 硬件/BIOS损坏 | 清CMOS/重刷BIOS |
| 卡在POST | 停在特定代码 | 硬件检测失败 | 最小化配置 |
| 启动慢 | POST时间长 | 配置/硬件扫描 | 优化BIOS设置 |
| 时间错误 | CMOS时间重置 | 电池耗尽 | 更换CMOS电池 |
| 配置丢失 | 设置重置 | 电池/CMOS | 更换电池/检查跳线 |

### CMOS重置

```yaml
CMOS重置方法:
  方法1 - 跳线重置:
    步骤:
      1. 关机断电
      2. 找到CMOS跳线 (通常标注CLR_CMOS)
      3. 短接跳线5-10秒
      4. 恢复跳线
      5. 重新开机
      
  方法2 - 取出电池:
    步骤:
      1. 关机断电
      2. 取出CMOS电池
      3. 等待30秒以上
      4. 装回电池
      5. 重新开机
      
  方法3 - BIOS选项:
    步骤:
      1. 进入BIOS设置
      2. 选择Load Default Settings
      3. 保存退出
      
重置后操作:
  - 重新设置日期时间
  - 配置启动顺序
  - 设置必要的BIOS选项
  - 验证硬件识别正确
```

## 固件管理最佳实践

```yaml
固件管理策略:
  版本控制:
    - 记录所有固件版本
    - 保存固件镜像备份
    - 记录更新历史
    
  更新策略:
    - 非紧急不更新
    - 测试环境先验证
    - 批量更新分批进行
    - 避免生产高峰期
    
  监控告警:
    - 监控固件安全公告
    - 设置版本一致性检查
    
  回滚准备:
    - 保留上一版本固件
    - 准备恢复介质
    - 记录回滚步骤
```

## 参考资源

- [UEFI Forum Specifications](https://uefi.org/specifications)
- [DMTF Redfish Documentation](https://www.dmtf.org/redfish)
- [各服务器厂商BIOS更新指南]
