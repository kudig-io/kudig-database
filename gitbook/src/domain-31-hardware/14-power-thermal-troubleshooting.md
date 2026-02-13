# 电源与散热故障排查

## 概述

电源和散热系统是服务器稳定运行的基础保障。本文档详细解析电源供电、散热系统的故障诊断方法及排查技巧。

## 电源故障排查

### 电源故障类型

```yaml
电源故障分类:
  PSU硬件故障:
    完全失效:
      症状: 无输出、LED灭
      原因: 电源模块损坏
      
    输出异常:
      症状: 电压不稳、功率不足
      原因: 电容老化、负载过重
      
    保护触发:
      症状: 间歇性断电
      原因: 过流/过温保护
      
  供电环境问题:
    输入电压异常:
      症状: UPS告警、电源保护
      原因: 电网波动、接线问题
      
    负载不均衡:
      症状: 单电源过载
      原因: 配置错误、线缆问题
```

### 电源诊断命令

```bash
#!/bin/bash
# 电源诊断脚本

echo "=== 电源状态诊断 ==="

# IPMI电源传感器
echo -e "\n--- 电源传感器 ---"
ipmitool sensor | grep -iE "power|volt|current|psu"

# 电源状态
echo -e "\n--- PSU状态 ---"
ipmitool sdr type "Power Supply"

# 系统功耗
echo -e "\n--- 系统功耗 ---"
ipmitool dcmi power reading

# 电源冗余状态
echo -e "\n--- 冗余状态 ---"
ipmitool sel elist | grep -i power | tail -10

# Redfish API (如支持)
check_redfish_power() {
    local bmc_ip=$1
    local user=$2
    local pass=$3
    
    curl -sk -u "$user:$pass" \
        "https://$bmc_ip/redfish/v1/Chassis/1/Power" \
        | jq '.PowerSupplies[] | {Name, Status, PowerOutputWatts}'
}

# 电压检查
check_voltages() {
    echo -e "\n--- 电压检查 ---"
    ipmitool sensor | grep -i volt | while read line; do
        name=$(echo "$line" | awk -F'|' '{print $1}')
        value=$(echo "$line" | awk -F'|' '{print $2}')
        status=$(echo "$line" | awk -F'|' '{print $4}')
        
        echo "$name: $value ($status)"
        
        if [[ "$status" =~ (cr|nr) ]]; then
            echo "  [警告] 电压异常!"
        fi
    done
}

check_voltages
```

### 电源问题速查

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|------|----------|----------|----------|
| 无法开机 | PSU故障/接线 | 检查PSU LED | 更换PSU/检查线缆 |
| 随机重启 | 电压不稳/过载 | 监控电压 | 检查负载/更换PSU |
| PSU报警 | 过温/过流 | IPMI日志 | 改善散热/减少负载 |
| 单PSU故障 | 冗余保护 | PSU状态 | 更换故障PSU |
| 功耗过高 | 配置/负载 | 功耗监控 | 优化配置 |

## 散热故障排查

### 散热系统诊断

```bash
#!/bin/bash
# 散热系统诊断脚本

echo "=== 散热系统诊断 ==="

# 温度传感器
echo -e "\n--- 温度传感器 ---"
ipmitool sensor | grep -iE "temp|thermal"

# 风扇状态
echo -e "\n--- 风扇状态 ---"
ipmitool sensor | grep -i fan
ipmitool sdr type "Fan"

# Linux温度监控
echo -e "\n--- 系统温度 ---"
if command -v sensors &> /dev/null; then
    sensors
fi

# 热区信息
echo -e "\n--- 热区信息 ---"
for zone in /sys/class/thermal/thermal_zone*; do
    if [ -d "$zone" ]; then
        type=$(cat "$zone/type" 2>/dev/null)
        temp=$(cat "$zone/temp" 2>/dev/null)
        temp_c=$((temp/1000))
        echo "$type: ${temp_c}°C"
    fi
done

# CPU温度详细
echo -e "\n--- CPU温度 ---"
if [ -f /sys/class/hwmon/hwmon*/temp*_input ]; then
    for f in /sys/class/hwmon/hwmon*/temp*_input; do
        label=$(cat "${f%_input}_label" 2>/dev/null || echo "Temp")
        value=$(cat "$f" 2>/dev/null)
        echo "$label: $((value/1000))°C"
    done
fi
```

### 温度阈值参考

```yaml
服务器温度参考:
  CPU温度:
    正常: 30-70°C
    警告: 70-85°C
    临界: >85°C
    关机: >100°C
    
  内存温度:
    正常: 30-65°C
    警告: 65-80°C
    临界: >80°C
    
  系统入口温度:
    推荐: 18-27°C (ASHRAE A1)
    允许: 15-32°C
    
  硬盘温度:
    HDD: 25-45°C
    SSD: 25-55°C
    NVMe: 30-70°C
    
风扇转速参考:
  正常范围: 3000-10000 RPM
  低速告警: <2000 RPM
  高速告警: 持续最高转速
```

### 散热问题排查

```yaml
散热问题诊断:
  高温问题:
    排查步骤:
      1. 确认温度读数
      2. 检查风扇运行
      3. 检查气流通道
      4. 检查散热器安装
      5. 检查硅脂状态
      
    常见原因:
      - 风扇故障
      - 灰尘堵塞
      - 散热器松动
      - 硅脂干涸
      - 环境温度过高
      - 负载过重
      
  风扇故障:
    症状:
      - 转速为0
      - 转速异常低
      - 异常噪音
      
    处理:
      - 清理灰尘
      - 检查连接
      - 更换风扇
      
  气流问题:
    检查点:
      - 机柜门是否关闭
      - 盲板是否安装
      - 线缆是否阻塞
      - 冷热通道是否隔离
```

### 热管理脚本

```python
# 温度监控与告警脚本
import subprocess
import json
from datetime import datetime

class ThermalMonitor:
    def __init__(self):
        self.thresholds = {
            'cpu': {'warning': 75, 'critical': 85},
            'memory': {'warning': 70, 'critical': 80},
            'system': {'warning': 35, 'critical': 40},
        }
        
    def get_temperatures(self):
        temps = {}
        
        # 从IPMI获取温度
        result = subprocess.run(
            ['ipmitool', 'sensor'],
            capture_output=True, text=True
        )
        
        for line in result.stdout.split('\n'):
            if 'Temp' in line or 'temp' in line:
                parts = line.split('|')
                if len(parts) >= 2:
                    name = parts[0].strip()
                    value = parts[1].strip()
                    try:
                        temps[name] = float(value.split()[0])
                    except:
                        pass
                        
        return temps
    
    def check_thresholds(self, temps):
        alerts = []
        
        for name, value in temps.items():
            for category, limits in self.thresholds.items():
                if category.lower() in name.lower():
                    if value >= limits['critical']:
                        alerts.append({
                            'level': 'CRITICAL',
                            'sensor': name,
                            'value': value,
                            'threshold': limits['critical']
                        })
                    elif value >= limits['warning']:
                        alerts.append({
                            'level': 'WARNING',
                            'sensor': name,
                            'value': value,
                            'threshold': limits['warning']
                        })
                        
        return alerts
    
    def get_fan_status(self):
        fans = []
        
        result = subprocess.run(
            ['ipmitool', 'sensor'],
            capture_output=True, text=True
        )
        
        for line in result.stdout.split('\n'):
            if 'Fan' in line or 'fan' in line:
                parts = line.split('|')
                if len(parts) >= 4:
                    fans.append({
                        'name': parts[0].strip(),
                        'rpm': parts[1].strip(),
                        'status': parts[3].strip()
                    })
                    
        return fans
    
    def generate_report(self):
        temps = self.get_temperatures()
        fans = self.get_fan_status()
        alerts = self.check_thresholds(temps)
        
        report = {
            'timestamp': datetime.now().isoformat(),
            'temperatures': temps,
            'fans': fans,
            'alerts': alerts,
            'status': 'CRITICAL' if any(a['level'] == 'CRITICAL' for a in alerts)
                      else 'WARNING' if alerts else 'OK'
        }
        
        return report

if __name__ == '__main__':
    monitor = ThermalMonitor()
    report = monitor.generate_report()
    print(json.dumps(report, indent=2))
```

## 电源散热最佳实践

### 预防性维护

```yaml
定期检查:
  每日:
    - 温度监控告警
    - 风扇状态监控
    - 功耗监控
    
  每周:
    - PSU冗余状态
    - 温度趋势分析
    
  每月:
    - 清理灰尘
    - 检查气流通道
    
  每年:
    - 更换高负载PSU
    - 检查散热器硅脂
    
机房环境:
  温度控制: 18-27°C
  湿度控制: 40-60%
  气流管理: 冷热通道隔离
  清洁度: 定期清洁
```

## 参考资源

- [ASHRAE Data Center Guidelines](https://www.ashrae.org/)
- [IPMI Specification](https://www.intel.com/ipmi)
- [Linux Hardware Monitoring](https://www.kernel.org/doc/html/latest/hwmon/)
