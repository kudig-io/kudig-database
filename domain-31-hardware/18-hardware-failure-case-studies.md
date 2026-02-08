# 硬件故障实战案例库

## 概述

本文档收录数据中心和 Kubernetes 集群环境中的真实硬件故障案例，每个案例包含完整的故障现象、诊断过程、解决方案和事后复盘，为一线运维人员提供实战参考。

## 案例分类索引

| 分类 | 案例编号 | 故障类型 | 严重程度 |
|------|---------|---------|---------|
| CPU/处理器 | CASE-001 | CPU MCE 导致 K8s Node 随机 NotReady | Critical |
| CPU/处理器 | CASE-002 | CPU 降频导致应用性能下降 50% | High |
| 内存 | CASE-003 | 内存 ECC 错误导致 Pod 随机 OOMKilled | Critical |
| 内存 | CASE-004 | NUMA 配置不当导致数据库性能异常 | High |
| 存储 | CASE-005 | NVMe SSD 静默故障导致 etcd 数据损坏 | Critical |
| 存储 | CASE-006 | RAID 卡电池故障导致写入性能骤降 | High |
| 存储 | CASE-007 | HDD 扇区错误导致 PVC 挂载失败 | Medium |
| 网络 | CASE-008 | 网卡固件 Bug 导致 K8s 网络间歇中断 | Critical |
| 网络 | CASE-009 | 光模块故障导致丢包率飙升 | High |
| 电源 | CASE-010 | PSU 故障导致多节点突然宕机 | Critical |
| 散热 | CASE-011 | 数据中心温度异常导致集群大面积告警 | High |

---

## CASE-001: CPU MCE 导致 K8s Node 随机 NotReady

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-03-15 14:30 |
| 影响范围 | 生产集群 node-worker-07 |
| 业务影响 | 该节点 Pod 被驱逐，服务短暂中断 |
| 解决时长 | 4 小时 |

### 故障现象

```yaml
初始告警:
  - 告警内容: "K8s node node-worker-07 is NotReady"
  - 告警时间: 14:30
  - 告警级别: Critical

现象描述:
  - Node 状态在 Ready/NotReady 之间反复切换
  - kubelet 日志显示心跳超时
  - 该节点上的 Pod 被驱逐后重新调度
  - 约 10-30 分钟会发生一次
```

### 诊断过程

```bash
# Step 1: 检查节点状态
kubectl describe node node-worker-07
# 发现 Conditions 中 Ready 状态为 Unknown
# LastHeartbeatTime 停滞

# Step 2: SSH 登录节点检查 kubelet
ssh node-worker-07
systemctl status kubelet
# 服务运行正常，但 journal 中有超时日志

# Step 3: 检查系统日志
dmesg | tail -100
# !!!发现关键信息!!!
# [12345.678901] mce: [Hardware Error]: CPU 12 BANK 4
# [12345.678902] mce: [Hardware Error]: RIP 10:<ffffffffa1234567>
# [12345.678903] mce: [Hardware Error]: TSC 1234567890
# [12345.678904] mce: [Hardware Error]: PROCESSOR 0:306e4 TIME 1234567890
# [12345.678905] mce: [Hardware Error]: Machine check events logged

# Step 4: 详细 MCE 分析
mcelog --client
# CPU 12 BANK 4 STATUS bc00000000800136
# MISC 9001064500000086 ADDR 7f8c12345000
# 错误类型: 内存控制器错误

# Step 5: 检查 IPMI SEL
ipmitool sel elist | grep -i cpu
# 发现多条 CPU Machine Check 记录
# 3月15日 14:25:30 | CPU 2 | Machine Check Exception | Asserted

# Step 6: 确认故障 CPU
dmidecode -t processor
# 确认 CPU 2 (物理插槽) 对应逻辑 CPU 12-23
```

### MCE 错误解析

```yaml
MCE_Status_解析:
  STATUS: bc00000000800136
  解析:
    VAL(63): 1 - 记录有效
    OVER(62): 0 - 无溢出
    UC(61): 1 - 不可纠正错误
    EN(60): 1 - 错误报告启用
    MISCV(59): 1 - MISC有效
    ADDRV(58): 1 - 地址有效
    PCC(57): 0 - 上下文未损坏
    
  BANK: 4 - 内存控制器
  错误码: 0x0136 - 内存读取错误
  
  初步判断: CPU 内存控制器问题，可能是 CPU 或 DIMM 故障
```

### 解决方案

```yaml
临时措施:
  1. 隔离节点:
     kubectl cordon node-worker-07
  2. 迁移工作负载:
     kubectl drain node-worker-07 --ignore-daemonsets --delete-emptydir-data
  3. 安排维护窗口

根因定位:
  - 交叉测试: 将该 CPU 插槽的 DIMM 与其他槽位交换
  - 观察 MCE 是否跟随 DIMM 移动
  - 结果: MCE 未跟随 DIMM，确认是 CPU 内存控制器问题

最终处理:
  - 申请更换 CPU 2
  - 更换后运行 48 小时压力测试无异常
  - kubectl uncordon node-worker-07
```

### 复盘总结

```yaml
根本原因:
  - CPU 内存控制器存在间歇性故障
  - 触发条件: 高负载内存访问时更容易触发

预防措施:
  - 部署 mcelog 监控，设置告警阈值
  - 配置 Prometheus 采集 node_edac 指标
  - 添加告警规则: MCE 错误数 > 0 即告警

经验教训:
  - Node NotReady 不一定是软件问题
  - 要善于查看 dmesg 中的硬件错误
  - MCE Bank 4 通常指向内存控制器
```

---

## CASE-002: CPU 降频导致应用性能下降 50%

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-04-22 09:00 |
| 影响范围 | 生产集群 3 个计算节点 |
| 业务影响 | API 响应时间从 50ms 增加到 200ms |
| 解决时长 | 2 小时 |

### 故障现象

```yaml
告警内容:
  - "API 响应延迟超过 SLO"
  - "Pod CPU 使用率异常高"

用户反馈:
  - 应用响应变慢
  - 同样的请求量，CPU 占用率翻倍

初步观察:
  - top 显示 CPU 使用率高但系统不卡
  - 进程运行时间明显变长
```

### 诊断过程

```bash
# Step 1: 检查 CPU 频率
cat /proc/cpuinfo | grep MHz
# 发现: cpu MHz 只有 1200.000
# 标称频率应该是 3000 MHz

# Step 2: 使用 turbostat 确认
turbostat --Summary --quiet
# Avg_MHz   Busy%   Bzy_MHz   TSC_MHz
# 1199      85.3    1200      3000
# 确认: CPU 被锁定在 1.2GHz

# Step 3: 检查降频原因
# 检查温度
sensors
# Package id 0:  +92.0°C (high = +85.0°C, crit = +105.0°C)
# Core 0:        +91.0°C
# !!! 温度过高 !!!

# Step 4: 检查热节流
cat /sys/devices/system/cpu/cpu0/thermal_throttle/package_throttle_count
# 12345 - 触发了大量热节流

# Step 5: 物理检查
# 发现: 服务器风扇运行正常，但进风口堵塞
# 原因: 数据中心最近调整了机柜布局，热通道封闭不当
```

### 解决方案

```yaml
临时措施:
  - 打开机柜门增加通风
  - 临时调度 workload 到其他节点

根因处理:
  - 重新调整热通道封闭
  - 清理服务器进风口积灰
  - 检查并更换干涸的导热硅脂

效果验证:
  - CPU 温度降至 65°C
  - CPU 频率恢复到 3.0GHz
  - 应用性能恢复正常
```

---

## CASE-003: 内存 ECC 错误导致 Pod 随机 OOMKilled

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-05-10 持续一周 |
| 影响范围 | node-worker-12 上的所有 Pod |
| 业务影响 | 应用随机崩溃，重启后短时间内再次崩溃 |
| 解决时长 | 3 天（定位困难） |

### 故障现象

```yaml
表面现象:
  - Pod 被 OOMKilled
  - 但 kubectl top 显示内存使用未满
  - 应用日志无异常，突然被 kill

迷惑点:
  - 同一个 Pod 调度到其他节点正常
  - 不同的应用调度到该节点都会出问题
  - 内存 limit 设置足够大
```

### 诊断过程

```bash
# Step 1: 常规 OOM 排查 (走了弯路)
kubectl describe pod xxx
# Reason: OOMKilled
# 看起来像是内存不足

# Step 2: 检查节点内存
ssh node-worker-12
free -h
#               total   used   free
# Mem:           256G   180G   76G
# 内存充足，为什么 OOM？

# Step 3: 检查 cgroup 限制
cat /sys/fs/cgroup/memory/kubepods/pod-xxx/memory.limit_in_bytes
# 8589934592 (8G)
cat /sys/fs/cgroup/memory/kubepods/pod-xxx/memory.usage_in_bytes
# 4294967296 (4G)
# 使用量远小于限制，不应该 OOM

# Step 4: 深入分析 - 检查 meminfo
cat /proc/meminfo | grep -i hardware
# HardwareCorrupted:  4194304 kB
# !!! 发现 4GB 内存被标记为损坏 !!!

# Step 5: 检查 EDAC
edac-util -s
# mc0: csrow2: 8192 Corrected Errors
# mc0: csrow2: ch1: 8192 Corrected Errors
# !!! DIMM slot 2 channel 1 有大量 ECC 错误 !!!

# Step 6: 查看详细 DIMM 信息
edac-util -v
# mc0: csrow2: ch1: 8192 Corrected Errors
# DIMM location: CPU0_DIMM_A2

# Step 7: 物理位置确认
dmidecode -t memory | grep -A 16 "CPU0_DIMM_A2"
# Size: 32 GB
# Manufacturer: Samsung
# Serial Number: 123ABC
```

### 根因分析

```yaml
根本原因:
  - DIMM CPU0_DIMM_A2 存在硬件缺陷
  - 大量 ECC 可纠正错误累积
  - 内核将故障内存页标记为 HardwareCorrupted
  - 实际可用内存小于显示的 free 值
  - Pod 被分配到已损坏的内存页时触发 OOM

为什么难以定位:
  - 表面看是 OOM，实际是硬件问题
  - free 命令不显示 HardwareCorrupted
  - 需要检查 /proc/meminfo 才能发现
```

### 解决方案

```bash
# 临时措施: 隔离节点
kubectl cordon node-worker-12

# 根因处理: 更换故障 DIMM
# 1. 关机
# 2. 更换 CPU0_DIMM_A2
# 3. 开机，运行 memtest86+ 验证
# 4. 确认 HardwareCorrupted 为 0

# 恢复服务
kubectl uncordon node-worker-12
```

### 监控改进

```yaml
新增监控规则:
  - name: NodeMemoryCorrupted
    expr: node_memory_HardwareCorrupted_bytes > 0
    severity: critical
    annotation: "节点存在损坏内存，可能导致 Pod 异常 OOMKilled"

  - name: NodeEDACErrors
    expr: increase(node_edac_correctable_errors_total[1h]) > 100
    severity: warning
    annotation: "内存 ECC 错误增加，建议检查 DIMM"
```

---

## CASE-005: NVMe SSD 静默故障导致 etcd 数据损坏

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-06-05 03:00 (凌晨) |
| 影响范围 | K8s Master 节点 etcd |
| 业务影响 | 集群 API 不可用 2 小时 |
| 解决时长 | 4 小时 |

### 故障现象

```yaml
凌晨告警:
  - "etcd cluster is unhealthy"
  - "kube-apiserver connection refused"

现象:
  - kubectl 命令全部超时
  - etcd 日志显示数据校验失败
  - 其他两个 etcd 成员正常
```

### 诊断过程

```bash
# Step 1: 检查 etcd 状态
etcdctl endpoint status --cluster
# 发现 master-01 的 etcd 无响应

# Step 2: 登录 master-01 检查 etcd 日志
journalctl -u etcd | tail -100
# panic: runtime error: invalid memory address
# etcdserver: snapshot file check failed
# wal: crc mismatch

# Step 3: 检查 NVMe 状态
nvme smart-log /dev/nvme0
# critical_warning: 0x02  # !!! Bit 1: 温度超阈值 
# temperature: 358       # 85°C
# media_errors: 12       # !!! 有介质错误 !!!
# percentage_used: 5%

# Step 4: 检查详细错误日志
nvme error-log /dev/nvme0
# Entry 0: Status 0x0281 - Media Error
# Entry 1: Status 0x0281 - Media Error

# Step 5: 检查内核日志
dmesg | grep nvme
# nvme0n1: I/O error, dev nvme0n1, sector 12345678
# blk_update_request: critical target error

# 结论: NVMe SSD 发生静默故障，导致部分数据损坏
```

### 解决方案

```yaml
紧急恢复:
  1. 从健康节点恢复 etcd:
     # 在 master-02 上备份
     etcdctl snapshot save /backup/etcd-snapshot.db
     
  2. 停止故障节点 etcd:
     systemctl stop etcd
     
  3. 清理故障数据:
     rm -rf /var/lib/etcd/member
     
  4. 从备份恢复:
     etcdctl snapshot restore /backup/etcd-snapshot.db \
       --data-dir=/var/lib/etcd
       
  5. 重启 etcd:
     systemctl start etcd

后续处理:
  - 更换故障 NVMe SSD
  - 重建 etcd 成员
```

### 复盘总结

```yaml
根本原因:
  - NVMe SSD 闪存颗粒退化
  - 发生静默数据损坏(写入成功但数据错误)
  - etcd WAL 数据 CRC 校验失败

教训:
  - NVMe 的 media_errors 非零必须重视
  - etcd 必须使用企业级 NVMe (带 PLP)
  - 定期检查 NVMe SMART 日志

预防措施:
  - 添加 NVMe media_errors 监控
  - etcd 数据目录使用单独的高质量 NVMe
  - 增加 etcd 备份频率
```

---

## CASE-006: RAID 卡电池故障导致写入性能骤降

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-07-18 10:00 |
| 影响范围 | 数据库服务器 |
| 业务影响 | 数据库写入延迟从 2ms 增加到 50ms |
| 解决时长 | 1.5 小时 |

### 故障现象

```yaml
监控告警:
  - "Database write latency > 30ms"
  - "Disk await time high"

用户反馈:
  - 数据库插入操作极慢
  - 批量导入任务超时
```

### 诊断过程

```bash
# Step 1: 检查磁盘延迟
iostat -x 1 5
# Device   r/s   w/s  await  svctm
# sda      10    500  52.3   15.2
# await 过高

# Step 2: 检查 RAID 状态
storcli64 /c0 show
# Controller Status: Optimal
# BBU Status: Failed  # !!! BBU 故障 !!!

# Step 3: 检查写缓存状态
storcli64 /c0 show all | grep -i cache
# Current Cache Policy: WriteThrough  # !!! 写穿透模式 !!!
# Default Cache Policy: WriteBack

# Step 4: BBU 详细状态
storcli64 /c0/bbu show all
# Battery State: Failed
# Voltage: 0 mV
# Temperature: 0 C
# Learn Cycle Status: Failed

# 结论: BBU 故障导致写缓存被禁用
```

### 解决方案

```yaml
临时措施:
  - 评估数据丢失风险后，强制启用写缓存 (生产环境慎用)
  - storcli64 /c0 set writepolicy=wb force

根因处理:
  - 申请更换 BBU 电池
  - 更换后执行 Learn Cycle
  - 验证写缓存自动启用

验证:
  - iostat await 恢复到 2ms
  - 数据库性能恢复正常
```

---

## CASE-008: 网卡固件 Bug 导致 K8s 网络间歇中断

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-08-05 ~ 2025-08-10 |
| 影响范围 | 多个节点网络间歇性中断 |
| 业务影响 | Service 调用随机失败 |
| 解决时长 | 5 天 |

### 故障现象

```yaml
告警:
  - "K8s node NetworkUnavailable"
  - "Service endpoint check failed"

现象:
  - 网络中断持续几秒到几分钟
  - 自动恢复后一段时间再次中断
  - 多个节点出现，但不同时
```

### 诊断过程

```bash
# Step 1: 检查网卡状态
ethtool eth0
# Link detected: yes (正常时)
# Link detected: no  (故障时)

# Step 2: 检查网卡统计
ethtool -S eth0 | grep -i error
# rx_errors: 12345
# tx_errors: 678
# rx_crc_errors: 12000  # CRC 错误很多

# Step 3: 检查驱动日志
dmesg | grep -i eth0
# eth0: Detected Hardware Unit Hang
# eth0: Reset adapter
# !!! 网卡重置 !!!

# Step 4: 检查固件版本
ethtool -i eth0
# driver: ixgbe
# version: 5.1.0
# firmware-version: 0x800007F5

# Step 5: 查询厂商 Bug 数据库
# 发现: 该固件版本存在已知 Bug
# Bug: 高负载下可能触发 TX Hang
```

### 解决方案

```yaml
解决方案:
  - 升级网卡固件到最新版本
  - 升级 ixgbe 驱动到 5.10.2
  
升级过程:
  1. 下载最新固件和驱动
  2. 逐节点滚动升级:
     - kubectl cordon node
     - 升级固件/驱动
     - 重启验证
     - kubectl uncordon node

验证:
  - 监控 2 周无网络中断
  - dmesg 无 Unit Hang 日志
```

---

## CASE-010: PSU 故障导致多节点突然宕机

### 故障概述

| 项目 | 内容 |
|------|------|
| 故障时间 | 2025-09-12 15:30 |
| 影响范围 | 同一机柜 4 台服务器 |
| 业务影响 | 多个服务不可用 |
| 解决时长 | 30 分钟 |

### 故障现象

```yaml
告警:
  - 4 台服务器同时失联
  - "K8s node NotReady" x 4
  - 机房人员报告: 服务器全部关机

特征:
  - 同一机柜的服务器
  - 突然断电，无预警日志
```

### 诊断过程

```yaml
物理检查:
  - 服务器电源 LED 熄灭
  - 机柜 PDU 显示正常
  - PDU 输出正常
  
进一步检查:
  - 检查供电线路
  - 发现: PDU 到服务器的 C13-C14 电源线损坏
  - 原因: 线材老化导致接触不良
  
确认:
  - 更换电源线后服务器正常启动
```

### 复盘总结

```yaml
根本原因:
  - 电源线老化，接触不良
  - 某次振动导致完全断开

预防措施:
  - 定期检查电源线连接
  - 使用锁定式电源接头
  - PDU 输出监控告警
  
改进:
  - 部署 sPDU 监控每路输出
  - 服务器双电源双 PDU 冗余
```

---

## 故障排查检查清单

### 通用检查流程

```yaml
硬件故障排查通用流程:
  1_信息收集:
    - 故障现象描述
    - 故障发生时间
    - 影响范围
    - 最近变更
    
  2_日志检查:
    - dmesg (内核硬件日志)
    - journalctl (系统日志)
    - ipmitool sel elist (BMC事件)
    - 应用日志
    
  3_硬件状态检查:
    - sensors (温度)
    - mcelog (CPU错误)
    - edac-util (内存错误)
    - smartctl (磁盘健康)
    - ethtool (网卡状态)
    
  4_性能数据:
    - top/htop (CPU/内存)
    - iostat (磁盘IO)
    - sar (历史数据)
    
  5_物理检查:
    - LED 状态
    - 风扇运转
    - 温度感知
    - 线缆连接
```

### K8s 环境专项检查

```yaml
K8s_硬件故障检查:
  Node_NotReady:
    - kubectl describe node
    - kubelet 日志
    - dmesg 硬件错误
    - 网络连通性
    
  Pod_OOMKilled:
    - cgroup 内存限制
    - /proc/meminfo HardwareCorrupted
    - EDAC 错误
    
  etcd_问题:
    - 磁盘延迟 (fio/ioping)
    - NVMe SMART
    - etcd 日志
    
  性能下降:
    - CPU 频率 (turbostat)
    - CPU 温度
    - 磁盘 IOPS/延迟
```

## 参考资源

- [16-kubernetes-hardware-troubleshooting.md](./16-kubernetes-hardware-troubleshooting.md)
- [17-hardware-error-codes-reference.md](./17-hardware-error-codes-reference.md)
- [10-hardware-troubleshooting-methodology.md](./10-hardware-troubleshooting-methodology.md)
