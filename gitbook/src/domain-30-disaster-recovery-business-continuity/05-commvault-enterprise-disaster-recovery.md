# Commvault 企业级灾备与业务连续性深度实践

> **作者**: 灾备架构师 | **版本**: v1.0 | **更新时间**: 2026-02-07
> **场景**: 企业级数据保护和灾难恢复解决方案 | **复杂度**: ⭐⭐⭐⭐

## 🎯 摘要

本文档全面探讨了Commvault企业级部署架构、灾备策略实施和业务连续性管理实践。基于大规模生产环境经验，提供从备份架构设计到灾难恢复演练的完整技术指导，帮助企业构建统一、可靠的数据保护平台，实现RTO/RPO目标，确保关键业务系统在各种灾难场景下的快速恢复能力。

## 1. Commvault 企业架构

### 1.1 核心组件架构

```mermaid
graph TB
    subgraph "Commvault 基础设施层"
        A[CommServe 服务器]
        B[MediaAgents]
        C[数据库服务器]
        D[索引服务器]
        E[Web 控制台]
    end
    
    subgraph "数据保护层"
        F[文件系统备份]
        G[数据库备份]
        H[虚拟机备份]
        I[应用程序备份]
        J[云存储备份]
    end
    
    subgraph "灾备管理"
        K[备份策略]
        L[恢复点目标]
        M[恢复时间目标]
        N[灾难恢复计划]
        O[业务影响分析]
    end
    
    subgraph "存储管理层"
        P[磁带库]
        Q[磁盘阵列]
        R[对象存储]
        S[云存储]
        T[重复数据删除]
    end
    
    subgraph "监控与报告"
        U[备份监控]
        V[性能报告]
        W[容量规划]
        X[合规报告]
        Y[审计日志]
    end
    
    subgraph "安全与合规"
        Z[数据加密]
        AA[访问控制]
        AB[审计追踪]
        AC[合规检查]
        AD[密钥管理]
    end
    
    A --> B
    B --> C
    C --> D
    D --> E
    
    F --> G
    G --> H
    H --> I
    I --> J
    
    K --> L
    L --> M
    M --> N
    N --> O
    
    P --> Q
    Q --> R
    R --> S
    S --> T
    
    U --> V
    V --> W
    W --> X
    X --> Y
    
    Z --> AA
    AA --> AB
    AB --> AC
    AC --> AD
```

### 1.2 企业级部署架构

```yaml
commvault_enterprise_deployment:
  commserve_configuration:
    production_commserve:
      hostname: "commserve-prod.company.com"
      ip_address: "192.168.1.100"
      operating_system: "Windows Server 2019"
      cpu_cores: 16
      memory_gb: 32
      storage_gb: 1000
      database:
        type: "Microsoft SQL Server 2019"
        edition: "Enterprise"
        collation: "SQL_Latin1_General_CP1_CI_AS"
        backup_retention_days: 30
      
      network_configuration:
        management_interface:
          ip: "192.168.1.100"
          subnet_mask: "255.255.255.0"
          gateway: "192.168.1.1"
          dns_servers:
            - "192.168.1.10"
            - "192.168.1.11"
        
        backup_interface:
          ip: "10.0.1.100"
          subnet_mask: "255.255.255.0"
          mtu: 9000  # Jumbo Frames for better performance
    
    high_availability:
      cluster_type: "Windows Failover Cluster"
      nodes:
        - hostname: "commserve-node1"
          ip: "192.168.1.101"
        - hostname: "commserve-node2"
          ip: "192.168.1.102"
      
      shared_storage:
        type: "SAN"
        lun_id: "LUN001"
        size_gb: 2000
        filesystem: "NTFS"
  
  mediaagent_configuration:
    primary_mediaagents:
      - hostname: "ma-prod-01"
        ip_address: "192.168.1.110"
        operating_system: "Windows Server 2019"
        cpu_cores: 12
        memory_gb: 24
        network_interfaces:
          - name: "Management"
            ip: "192.168.1.110"
          - name: "Backup"
            ip: "10.0.1.110"
            mtu: 9000
        
        storage_libraries:
          - library_name: "Tape Library A"
            type: "IBM TS4500"
            drive_count: 8
            slot_count: 2000
          
          - library_name: "Disk Library"
            type: "Dell EMC PowerVault"
            capacity_tb: 500
            raid_level: "RAID 6"
    
    secondary_mediaagents:
      - hostname: "ma-dr-01"
        ip_address: "192.168.2.110"
        location: "异地数据中心"
        network_bandwidth_mbps: 1000
        purpose: "灾难恢复站点"
  
  storage_policies:
    tiered_storage:
      primary_storage:
        type: "Disk"
        retention_days: 30
        deduplication_ratio: "20:1"
        encryption: "AES-256"
      
      secondary_storage:
        type: "Tape"
        retention_weeks: 52
        compression: "Hardware"
        encryption: "AES-256"
      
      tertiary_storage:
        type: "Cloud"
        provider: "Amazon S3 Glacier"
        retention_years: 7
        transfer_protocol: "HTTPS"
        encryption: "AES-256"
  
  security_configuration:
    authentication:
      method: "Active Directory"
      domain: "company.com"
      service_account: "svc-commvault"
    
    authorization:
      admin_groups:
        - "Commvault Admins"
        - "Backup Operators"
      user_groups:
        - "Department A Users"
        - "Department B Users"
    
    encryption:
      in_transit:
        protocol: "TLS 1.3"
        certificate_validity_days: 365
      at_rest:
        method: "Hardware Encryption"
        key_length: 256
        key_rotation_days: 90
    
    auditing:
      log_retention_days: 180
      alert_thresholds:
        failed_logins: 5
        unauthorized_access: 1
        policy_changes: 1
```

## 2. 高级备份策略

### 2.1 分层备份配置

```powershell
# Commvault PowerShell 脚本 - 分层备份策略配置

# 1. 创建存储策略
New-CVStoragePolicy -Name "Tiered-Backup-Policy" -Description "企业级分层备份策略" `
    -RetentionRules @{
        "Daily" = @{ RetentionDays = 30; BackupType = "Full" }
        "Weekly" = @{ RetentionWeeks = 12; BackupType = "Full" }
        "Monthly" = @{ RetentionMonths = 12; BackupType = "Full" }
        "Yearly" = @{ RetentionYears = 7; BackupType = "Full" }
    } `
    -DeduplicationEnabled $true `
    -GlobalDeduplication $true `
    -EncryptionEnabled $true

# 2. 配置主存储层（磁盘）
Add-CVStoragePool -StoragePolicy "Tiered-Backup-Policy" `
    -PoolName "Primary-Disk-Pool" `
    -MediaType "Disk" `
    -Path "\\storage-array\backup-pool" `
    -BlockSizeKB 1024 `
    -DeduplicationRatio 20 `
    -RetentionDays 30

# 3. 配置二级存储层（磁带）
Add-CVStoragePool -StoragePolicy "Tiered-Backup-Policy" `
    -PoolName "Secondary-Tape-Pool" `
    -MediaType "Tape" `
    -LibraryName "IBM-TS4500-Library" `
    -DriveCount 8 `
    -SlotCount 2000 `
    -RetentionWeeks 52

# 4. 配置三级存储层（云）
Add-CVStoragePool -StoragePolicy "Tiered-Backup-Policy" `
    -PoolName "Tertiary-Cloud-Pool" `
    -MediaType "Cloud" `
    -CloudProvider "Amazon S3" `
    -BucketName "company-backup-archive" `
    -Region "us-west-2" `
    -RetentionYears 7

# 5. 创建备份集
New-CVBackupSet -ClientGroup "Production-Servers" `
    -BackupSetName "Critical-Systems-Backup" `
    -StoragePolicy "Tiered-Backup-Policy" `
    -SubclientPolicy @{
        "Database-Servers" = @{
            Schedule = "每天 23:00"
            Type = "Full"
            Throttle = "Medium"
        }
        "File-Servers" = @{
            Schedule = "每周日 22:00"
            Type = "Incremental"
            Throttle = "Low"
        }
        "Virtual-Machines" = @{
            Schedule = "每4小时"
            Type = "SnapShot"
            Throttle = "High"
        }
    }
```

### 2.2 应用程序一致性备份

```xml
<!-- 应用程序一致性备份配置 -->
<ApplicationConsistentBackup>
    <Applications>
        <!-- Microsoft SQL Server 配置 -->
        <Application name="SQL Server">
            <PreScript>
                <Command>powershell.exe -File "C:\Scripts\PreBackup-SQL.ps1"</Command>
                <TimeoutMinutes>30</TimeoutMinutes>
                <RunAsUser>DOMAIN\sqlservice</RunAsUser>
            </PreScript>
            
            <PostScript>
                <Command>powershell.exe -File "C:\Scripts\PostBackup-SQL.ps1"</Command>
                <TimeoutMinutes>15</TimeoutMinutes>
            </PostScript>
            
            <VSSConfiguration>
                <WriterName>SqlServerWriter</WriterName>
                <ComponentSelection>All</ComponentSelection>
                <TransactionLogBackup>Enabled</TransactionLogBackup>
                <LogTruncation>AfterBackup</LogTruncation>
            </VSSConfiguration>
        </Application>
        
        <!-- Oracle 数据库配置 -->
        <Application name="Oracle">
            <PreScript>
                <Command>rman target / @C:\Scripts\PreBackup-Oracle.sql</Command>
                <TimeoutMinutes>45</TimeoutMinutes>
            </PreScript>
            
            <ArchiveLogMode>ARCHIVELOG</ArchiveLogMode>
            <ControlFileAutobackup>Enabled</ControlFileAutobackup>
            <BackupValidation>Enabled</BackupValidation>
        </Application>
        
        <!-- Exchange Server 配置 -->
        <Application name="Exchange">
            <VSSConfiguration>
                <WriterName>Microsoft Exchange Writer</WriterName>
                <GranularRecovery>Enabled</GranularRecovery>
                <MailboxRecovery>Enabled</MailboxRecovery>
            </VSSConfiguration>
        </Application>
        
        <!-- SharePoint 配置 -->
        <Application name="SharePoint">
            <PreScript>
                <Command>stsadm -o quiesceservice -allowupdates 0</Command>
                <TimeoutMinutes>10</TimeoutMinutes>
            </PreScript>
            
            <PostScript>
                <Command>stsadm -o quiesceservice -allowupdates 1</Command>
                <TimeoutMinutes>10</TimeoutMinutes>
            </PostScript>
        </Application>
    </Applications>
    
    <!-- 虚拟机应用程序一致性 -->
    <VirtualMachineBackup>
        <VMware>
            <GuestQuiescing>Enabled</GuestQuiescing>
            <FileSystemQuiescing>Enabled</FileSystemQuiescing>
            <ApplicationQuiescing>
                <SQLServer>Enabled</SQLServer>
                <Exchange>Enabled</Exchange>
                <ActiveDirectory>Enabled</ActiveDirectory>
            </ApplicationQuiescing>
        </VMware>
        
        <HyperV>
            <ChildIntegrationService>Enabled</ChildIntegrationService>
            <BackupIntegration>Enabled</BackupIntegration>
            <GuestVSSProvider>Enabled</GuestVSSProvider>
        </HyperV>
    </VirtualMachineBackup>
</ApplicationConsistentBackup>
```

## 3. 灾难恢复策略

### 3.1 多站点灾备架构

```yaml
disaster_recovery_architecture:
  primary_site:
    location: "北京数据中心"
    commserve: "commserve-beijing"
    mediaagents:
      - "ma-beijing-01"
      - "ma-beijing-02"
    storage:
      local_disk_tb: 500
      tape_library: "IBM-TS4500-Local"
    network_bandwidth_gbps: 10
    rpo_hours: 4
    rto_hours: 2
  
  secondary_site:
    location: "上海数据中心"
    commserve: "commserve-shanghai"
    mediaagents:
      - "ma-shanghai-01"
      - "ma-shanghai-02"
    storage:
      local_disk_tb: 300
      tape_library: "IBM-TS4500-DR"
    network_bandwidth_gbps: 1
    rpo_hours: 24
    rto_hours: 8
    synchronization_schedule: "每4小时增量同步"
  
  tertiary_site:
    location: "广州异地备份中心"
    storage_type: "云存储"
    provider: "阿里云 OSS"
    bucket_name: "company-dr-archive"
    rpo_days: 7
    rto_days: 3
    data_sync_schedule: "每日同步"
  
  failover_scenarios:
    site_failure:
      detection_time_minutes: 30
      failover_procedure:
        - 启动备用 CommServe
        - 激活远程 MediaAgents
        - 重定向备份流量
        - 验证数据完整性
        - 通知相关人员
    
    regional_disaster:
      scope: "整个区域电力中断"
      recovery_steps:
        - 切换到第三站点
        - 从云存储恢复关键数据
        - 重建核心业务系统
        - 逐步恢复其他服务
```

### 3.2 自动化故障转移配置

```powershell
# Commvault 自动化故障转移脚本

param(
    [Parameter(Mandatory=$true)]
    [string]$PrimarySite,
    
    [Parameter(Mandatory=$true)]
    [string]$SecondarySite,
    
    [Parameter(Mandatory=$false)]
    [int]$HealthCheckInterval = 300  # 5分钟检查间隔
)

class DisasterRecoveryOrchestrator {
    [string]$PrimaryCommServe
    [string]$SecondaryCommServe
    [hashtable]$SiteStatus
    [bool]$FailoverInProgress
    
    DisasterRecoveryOrchestrator($primary, $secondary) {
        $this.PrimaryCommServe = $primary
        $this.SecondaryCommServe = $secondary
        $this.SiteStatus = @{}
        $this.FailoverInProgress = $false
    }
    
    [bool] CheckSiteHealth($site) {
        try {
            $response = Invoke-RestMethod -Uri "https://$site/HealthCheck" -Method Get -TimeoutSec 30
            return $response.Status -eq "Healthy"
        }
        catch {
            Write-Warning "无法连接到站点 $site : $($_.Exception.Message)"
            return $false
        }
    }
    
    [void] PerformFailover() {
        if ($this.FailoverInProgress) {
            Write-Warning "故障转移已在进行中"
            return
        }
        
        $this.FailoverInProgress = $true
        Write-Host "开始执行故障转移..." -ForegroundColor Yellow
        
        try {
            # 1. 停止主站点服务
            Write-Host "停止主站点备份作业..." -ForegroundColor Cyan
            Stop-CVBackupJobs -CommServe $this.PrimaryCommServe
            
            # 2. 激活备用站点
            Write-Host "激活备用站点..." -ForegroundColor Cyan
            Enable-CVDRSite -CommServe $this.SecondaryCommServe
            
            # 3. 重定向客户端
            Write-Host "重定向备份客户端..." -ForegroundColor Cyan
            $clients = Get-CVClients -CommServe $this.PrimaryCommServe
            foreach ($client in $clients) {
                Move-CVClient -ClientName $client.Name -TargetCommServe $this.SecondaryCommServe
            }
            
            # 4. 启动备份作业
            Write-Host "启动备用站点备份作业..." -ForegroundColor Cyan
            Start-CVBackupJobs -CommServe $this.SecondaryCommServe
            
            # 5. 验证恢复
            Write-Host "验证故障转移状态..." -ForegroundColor Cyan
            $validationResult = $this.ValidateFailover()
            
            if ($validationResult) {
                Write-Host "故障转移成功完成！" -ForegroundColor Green
            } else {
                Write-Error "故障转移验证失败，请手动检查"
            }
        }
        catch {
            Write-Error "故障转移过程中发生错误: $($_.Exception.Message)"
            $this.InitiateRollback()
        }
        finally {
            $this.FailoverInProgress = $false
        }
    }
    
    [bool] ValidateFailover() {
        $maxWaitTime = 1800  # 30分钟超时
        $startTime = Get-Date
        
        do {
            Start-Sleep -Seconds 60
            $backupStatus = Get-CVBackupStatus -CommServe $this.SecondaryCommServe
            
            if ($backupStatus.RunningJobs -gt 0 -and $backupStatus.FailedJobs -eq 0) {
                return $true
            }
            
            if ((Get-Date) - $startTime).TotalSeconds -gt $maxWaitTime {
                break
            }
        } while ($true)
        
        return $false
    }
    
    [void] InitiateRollback() {
        Write-Warning "开始回滚操作..."
        # 回滚逻辑实现
    }
    
    [void] MonitorSites() {
        while ($true) {
            $primaryHealthy = $this.CheckSiteHealth($this.PrimaryCommServe)
            $secondaryHealthy = $this.CheckSiteHealth($this.SecondaryCommServe)
            
            $this.SiteStatus.Primary = $primaryHealthy
            $this.SiteStatus.Secondary = $secondaryHealthy
            
            if (-not $primaryHealthy -and $secondaryHealthy -and -not $this.FailoverInProgress) {
                Write-Host "检测到主站点故障，准备执行故障转移..." -ForegroundColor Red
                $this.PerformFailover()
            }
            
            Start-Sleep -Seconds $HealthCheckInterval
        }
    }
}

# 主程序执行
$orchestrator = [DisasterRecoveryOrchestrator]::new($PrimarySite, $SecondarySite)

# 启动监控
Write-Host "启动灾备监控服务..." -ForegroundColor Green
$orchestrator.MonitorSites()
```

## 4. 性能优化与容量规划

### 4.1 备份性能调优

```bash
#!/bin/bash
# commvault_performance_optimization.sh

# 1. 系统级性能优化
optimize_system_performance() {
    echo "=== 系统性能优化 ==="
    
    # 调整TCP参数
    echo "优化网络TCP参数..."
    cat >> /etc/sysctl.conf << EOF
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr
EOF
    
    sysctl -p
    
    # 调整文件系统参数
    echo "优化文件系统参数..."
    tune2fs -o journal_data_writeback /dev/sdb1
    
    # 调整IO调度器
    echo "设置IO调度器为deadline..."
    echo deadline > /sys/block/sdb/queue/scheduler
}

# 2. Commvault特定优化
optimize_commvault_settings() {
    echo "=== Commvault 参数优化 ==="
    
    # 数据库优化
    cat > /opt/commvault/optimize_db.sql << 'EOF'
-- SQL Server 性能优化
USE Commvault;
GO

-- 创建性能索引
CREATE INDEX IX_JobHistory_StartTime ON JobHistory(StartTime);
CREATE INDEX IX_JobHistory_ClientId ON JobHistory(ClientId);
CREATE INDEX IX_BackupInfo_BackupTime ON BackupInfo(BackupTime);

-- 更新统计信息
UPDATE STATISTICS JobHistory;
UPDATE STATISTICS BackupInfo;

-- 配置内存优化
EXEC sp_configure 'max server memory (MB)', 24576;
EXEC sp_configure 'min server memory (MB)', 4096;
RECONFIGURE;

-- 启用即时文件初始化
EXEC xp_cmdshell 'sc config SQLSERVERAGENT binpath= "C:\Program Files\Microsoft SQL Server\MSSQL15.MSSQLSERVER\MSSQL\Binn\SQLAGENT.EXE" -sSQLSERVERAGENT -i"C:\Program Files\Microsoft SQL Server\MSSQL15.MSSQLSERVER\MSSQL" -d"C:\Program Files\Microsoft SQL Server\MSSQL15.MSSQLSERVER\MSSQL\DATA\master.mdf" -l"C:\Program Files\Microsoft SQL Server\MSSQL15.MSSQLSERVER\MSSQL\DATA\mastlog.ldf" -T2704';
EOF
    
    # 执行数据库优化
    sqlcmd -S localhost -E -i /opt/commvault/optimize_db.sql
}

# 3. 存储性能优化
optimize_storage_performance() {
    echo "=== 存储性能优化 ==="
    
    # 磁盘队列深度优化
    echo "优化磁盘队列深度..."
    cat > /etc/udev/rules.d/99-commvault-storage.rules << 'EOF'
ACTION=="add", SUBSYSTEM=="block", KERNEL=="sd*", ATTR{queue/rotational}=="0", ATTR{queue/scheduler}="noop", ATTR{queue/nr_requests}="1024", ATTR{queue/read_ahead_kb}="4096"
EOF
    
    # 重启udev服务
    udevadm control --reload-rules
    udevadm trigger
    
    # 创建优化的挂载选项
    echo "优化存储挂载选项..."
    sed -i '/backup-storage/d' /etc/fstab
    echo "/dev/sdb1 /backup ext4 defaults,noatime,nobarrier,data=writeback 0 2" >> /etc/fstab
    mount -o remount /backup
}

# 4. 网络性能优化
optimize_network_performance() {
    echo "=== 网络性能优化 ==="
    
    # 配置巨帧
    echo "启用巨帧支持..."
    for interface in eth0 eth1; do
        if ip link show $interface >/dev/null 2>&1; then
            ip link set $interface mtu 9000
            ethtool -K $interface gso on tso on gro on
        fi
    done
    
    # 优化网络缓冲区
    echo "优化网络缓冲区..."
    cat >> /etc/sysctl.conf << EOF
net.core.netdev_max_backlog = 5000
net.core.rmem_default = 262144
net.core.wmem_default = 262144
net.core.optmem_max = 20480
EOF
    
    sysctl -p
}

# 5. 监控和基准测试
performance_benchmarking() {
    echo "=== 性能基准测试 ==="
    
    # 创建测试脚本
    cat > /opt/commvault/benchmark.sh << 'EOF'
#!/bin/bash

TEST_DIR="/backup/benchmark"
TEST_SIZE="10G"
RESULTS_FILE="/var/log/commvault/benchmark_results.txt"

mkdir -p $TEST_DIR
mkdir -p /var/log/commvault

echo "开始性能基准测试..." | tee -a $RESULTS_FILE
echo "测试时间: $(date)" | tee -a $RESULTS_FILE

# 磁盘IO测试
echo "=== 磁盘IO性能测试 ===" | tee -a $RESULTS_FILE
dd if=/dev/zero of=$TEST_DIR/testfile bs=1M count=10240 oflag=direct 2>&1 | tee -a $RESULTS_FILE

# 网络吞吐量测试
echo "=== 网络吞吐量测试 ===" | tee -a $RESULTS_FILE
iperf3 -c backup-server -t 60 -P 4 2>&1 | tee -a $RESULTS_FILE

# 备份性能测试
echo "=== 备份性能测试 ===" | tee -a $RESULTS_FILE
# 这里可以集成Commvault的备份性能测试命令

# 清理测试文件
rm -f $TEST_DIR/testfile
EOF
    
    chmod +x /opt/commvault/benchmark.sh
    
    # 运行基准测试
    /opt/commvault/benchmark.sh
}

# 主执行函数
main() {
    echo "开始Commvault性能优化..."
    
    optimize_system_performance
    optimize_commvault_settings
    optimize_storage_performance
    optimize_network_performance
    performance_benchmarking
    
    echo "性能优化完成！"
    echo "请重启Commvault服务以使更改生效"
}

main
```

### 4.2 容量规划工具

```python
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Commvault 容量规划和预测工具
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.linear_model import LinearRegression
import json
import argparse

class CommvaultCapacityPlanner:
    def __init__(self):
        self.current_data = {}
        self.forecast_data = {}
        self.growth_rate = 0.15  # 默认年增长率15%
        
    def load_current_inventory(self, inventory_file):
        """加载当前备份环境清单"""
        try:
            with open(inventory_file, 'r', encoding='utf-8') as f:
                self.current_data = json.load(f)
            print(f"成功加载清单文件: {inventory_file}")
        except FileNotFoundError:
            print(f"错误: 找不到清单文件 {inventory_file}")
            return False
        except json.JSONDecodeError:
            print(f"错误: 清单文件格式不正确")
            return False
        return True
    
    def calculate_current_capacity(self):
        """计算当前容量使用情况"""
        total_protected_data = 0
        total_backup_size = 0
        total_deduplicated_size = 0
        
        for client in self.current_data.get('clients', []):
            client_size = client.get('data_size_gb', 0)
            total_protected_data += client_size
            
            # 计算备份大小（考虑压缩和去重）
            compression_ratio = client.get('compression_ratio', 2.0)
            deduplication_ratio = client.get('deduplication_ratio', 10.0)
            
            backup_size = client_size / compression_ratio
            deduplicated_size = backup_size / deduplication_ratio
            
            total_backup_size += backup_size
            total_deduplicated_size += deduplicated_size
        
        capacity_metrics = {
            'protected_data_tb': round(total_protected_data / 1024, 2),
            'raw_backup_tb': round(total_backup_size / 1024, 2),
            'deduplicated_backup_tb': round(total_deduplicated_size / 1024, 2),
            'effective_compression_ratio': round(total_protected_data / total_deduplicated_size, 2) if total_deduplicated_size > 0 else 0,
            'total_clients': len(self.current_data.get('clients', []))
        }
        
        return capacity_metrics
    
    def forecast_growth(self, months_ahead=36):
        """预测未来容量增长"""
        current_metrics = self.calculate_current_capacity()
        historical_data = []
        
        # 生成历史数据点（假设过去2年的月度数据）
        base_size = current_metrics['deduplicated_backup_tb']
        current_date = datetime.now()
        
        for i in range(24, -1, -1):  # 过去24个月到当前
            date = current_date - timedelta(days=i*30)
            months_back = 24 - i
            
            # 模拟历史增长（带一些随机波动）
            growth_factor = (1 + self.growth_rate/12) ** months_back
            size = base_size / growth_factor * (0.95 + 0.1*np.random.random())
            
            historical_data.append({
                'date': date.strftime('%Y-%m'),
                'months_ago': months_back,
                'size_tb': round(size, 2)
            })
        
        # 预测未来数据
        forecast_data = []
        for i in range(1, months_ahead + 1):
            date = current_date + timedelta(days=i*30)
            growth_factor = (1 + self.growth_rate/12) ** i
            size = base_size * growth_factor * (0.98 + 0.04*np.random.random())
            
            forecast_data.append({
                'date': date.strftime('%Y-%m'),
                'months_ahead': i,
                'size_tb': round(size, 2),
                'growth_rate': f"{self.growth_rate*100:.1f}%"
            })
        
        self.forecast_data = {
            'historical': historical_data,
            'forecast': forecast_data,
            'current_metrics': current_metrics
        }
        
        return self.forecast_data
    
    def generate_recommendations(self):
        """生成容量规划建议"""
        if not self.forecast_data:
            self.forecast_growth()
        
        current = self.forecast_data['current_metrics']
        forecast = self.forecast_data['forecast']
        
        recommendations = {
            'immediate_needs': {},
            'short_term_planning': {},
            'long_term_strategy': {}
        }
        
        # 1年后的预测容量
        one_year_forecast = next((item for item in forecast if item['months_ahead'] == 12), None)
        three_year_forecast = next((item for item in forecast if item['months_ahead'] == 36), None)
        
        if one_year_forecast:
            # 立即需求
            current_capacity = current['deduplicated_backup_tb']
            projected_capacity_1y = one_year_forecast['size_tb']
            growth_needed_1y = projected_capacity_1y - current_capacity
            
            recommendations['immediate_needs'] = {
                'additional_capacity_tb': round(growth_needed_1y, 2),
                'recommended_action': '扩展当前存储池',
                'timeline': '3-6个月内'
            }
        
        if three_year_forecast:
            # 长期规划
            projected_capacity_3y = three_year_forecast['size_tb']
            total_growth_3y = projected_capacity_3y - current['deduplicated_backup_tb']
            
            recommendations['long_term_strategy'] = {
                'total_growth_needed_tb': round(total_growth_3y, 2),
                'annual_growth_tb': round(total_growth_3y/3, 2),
                'recommended_approach': '渐进式扩展 + 云存储归档',
                'investment_timeline': '分阶段实施'
            }
        
        # 技术建议
        recommendations['technical_considerations'] = {
            'storage_tiering': '建议采用三层存储架构（热/温/冷）',
            'cloud_integration': '考虑将长期归档迁移到云端',
            'performance_scaling': '提前规划网络带宽和处理能力',
            'monitoring_alerts': '建立容量预警机制'
        }
        
        return recommendations
    
    def create_visualization(self, output_file='capacity_forecast.png'):
        """创建容量预测可视化图表"""
        if not self.forecast_data:
            self.forecast_growth()
        
        # 准备数据
        historical_df = pd.DataFrame(self.forecast_data['historical'])
        forecast_df = pd.DataFrame(self.forecast_data['forecast'])
        
        # 创建图表
        plt.figure(figsize=(12, 8))
        sns.set_style("whitegrid")
        
        # 历史数据
        plt.plot(historical_df['months_ago'], historical_df['size_tb'], 
                marker='o', linewidth=2, label='历史数据', color='#2E86AB')
        
        # 预测数据
        months_future = [item['months_ahead'] for item in self.forecast_data['forecast']]
        sizes_future = [item['size_tb'] for item in self.forecast_data['forecast']]
        plt.plot(months_future, sizes_future, 
                marker='s', linewidth=2, label='预测数据', color='#A23B72')
        
        # 当前点标记
        current_months = 0
        current_size = self.forecast_data['current_metrics']['deduplicated_backup_tb']
        plt.scatter([current_months], [current_size], 
                   s=100, color='#F18F01', zorder=5, label='当前状态')
        
        # 关键里程碑
        milestones = [12, 24, 36]  # 1年, 2年, 3年
        for milestone in milestones:
            if milestone <= len(sizes_future):
                size_at_milestone = sizes_future[milestone-1]
                plt.axvline(x=milestone, color='gray', linestyle='--', alpha=0.7)
                plt.annotate(f'{milestone}个月\n{size_at_milestone:.1f}TB', 
                           xy=(milestone, size_at_milestone),
                           xytext=(5, 10), textcoords='offset points',
                           bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.7))
        
        plt.xlabel('月份')
        plt.ylabel('容量 (TB)')
        plt.title('Commvault 容量增长预测')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # 保存图表
        plt.tight_layout()
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        plt.show()
        
        print(f"容量预测图表已保存到: {output_file}")
    
    def generate_report(self, output_file='capacity_planning_report.json'):
        """生成完整的容量规划报告"""
        if not self.forecast_data:
            self.forecast_growth()
        
        report = {
            'generated_at': datetime.now().isoformat(),
            'current_capacity': self.forecast_data['current_metrics'],
            'forecast_data': self.forecast_data,
            'recommendations': self.generate_recommendations(),
            'assumptions': {
                'annual_growth_rate': f"{self.growth_rate*100}%",
                'data_retention_period': '默认保留策略',
                'compression_assumption': '基于历史平均压缩比',
                'reporting_period': '月度分析'
            }
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        
        print(f"容量规划报告已保存到: {output_file}")
        return report

def main():
    parser = argparse.ArgumentParser(description='Commvault 容量规划工具')
    parser.add_argument('--inventory', '-i', required=True, 
                       help='当前环境清单文件路径')
    parser.add_argument('--growth-rate', '-g', type=float, default=0.15,
                       help='年增长率 (默认: 0.15 = 15%)')
    parser.add_argument('--months', '-m', type=int, default=36,
                       help='预测月数 (默认: 36个月)')
    parser.add_argument('--output', '-o', default='capacity_report',
                       help='输出文件前缀')
    
    args = parser.parse_args()
    
    # 创建规划器实例
    planner = CommvaultCapacityPlanner()
    planner.growth_rate = args.growth_rate
    
    # 加载数据
    if not planner.load_current_inventory(args.inventory):
        return 1
    
    # 执行分析
    print("开始容量规划分析...")
    forecast_data = planner.forecast_growth(args.months)
    recommendations = planner.generate_recommendations()
    
    # 生成输出
    planner.create_visualization(f"{args.output}_forecast.png")
    planner.generate_report(f"{args.output}_report.json")
    
    # 打印摘要
    print("\n=== 容量规划摘要 ===")
    current = forecast_data['current_metrics']
    print(f"当前保护数据: {current['protected_data_tb']} TB")
    print(f"当前备份容量: {current['deduplicated_backup_tb']} TB")
    print(f"有效压缩比: {current['effective_compression_ratio']}:1")
    
    one_year = next((item for item in forecast_data['forecast'] if item['months_ahead'] == 12), None)
    if one_year:
        print(f"1年后预计容量: {one_year['size_tb']} TB")
    
    print(f"\n主要建议:")
    recs = recommendations
    if 'immediate_needs' in recs:
        print(f"- 立即需求: {recs['immediate_needs'].get('additional_capacity_tb', 0)} TB")
    if 'long_term_strategy' in recs:
        print(f"- 长期规划: {recs['long_term_strategy'].get('total_growth_needed_tb', 0)} TB 总增长")
    
    return 0

if __name__ == "__main__":
    exit(main())
```

## 5. 监控与告警系统

### 5.1 综合监控仪表板

```json
{
  "dashboard": {
    "name": "Commvault 企业级监控仪表板",
    "refresh_interval": "30s",
    "timezone": "Asia/Shanghai",
    "panels": [
      {
        "title": "备份作业状态概览",
        "type": "stat",
        "datasource": "Commvault",
        "targets": [
          {
            "query": "SELECT COUNT(*) as total_jobs FROM JobHistory WHERE StartTime >= DATEADD(day, -1, GETDATE())",
            "legendFormat": "总作业数"
          },
          {
            "query": "SELECT COUNT(*) as successful_jobs FROM JobHistory WHERE Status = 'Completed' AND StartTime >= DATEADD(day, -1, GETDATE())",
            "legendFormat": "成功作业"
          },
          {
            "query": "SELECT COUNT(*) as failed_jobs FROM JobHistory WHERE Status IN ('Failed', 'Error') AND StartTime >= DATEADD(day, -1, GETDATE())",
            "legendFormat": "失败作业"
          }
        ],
        "thresholds": {
          "failed_jobs": {
            "warning": 5,
            "critical": 10
          }
        }
      },
      {
        "title": "存储容量使用情况",
        "type": "gauge",
        "targets": [
          {
            "query": "SELECT (UsedSpaceGB/TotalSpaceGB)*100 as utilization FROM StoragePools",
            "legendFormat": "{{pool_name}}"
          }
        ],
        "thresholds": {
          "utilization": {
            "normal": 0,
            "warning": 80,
            "critical": 95
          }
        }
      },
      {
        "title": "备份性能趋势",
        "type": "graph",
        "targets": [
          {
            "query": "SELECT AVG(DurationMinutes) as avg_duration, DATE(Date) as day FROM JobHistory WHERE JobType = 'Backup' GROUP BY DATE(Date) ORDER BY Date DESC LIMIT 30",
            "legendFormat": "平均备份时长(分钟)"
          },
          {
            "query": "SELECT AVG(DataSizeGB) as avg_data_size, DATE(Date) as day FROM JobHistory WHERE JobType = 'Backup' GROUP BY DATE(Date) ORDER BY Date DESC LIMIT 30",
            "legendFormat": "平均数据量(GB)"
          }
        ]
      },
      {
        "title": "客户端保护状态",
        "type": "table",
        "targets": [
          {
            "query": "SELECT ClientName, LastBackupTime, BackupStatus, DaysSinceLastBackup FROM Clients ORDER BY DaysSinceLastBackup DESC",
            "legendFormat": "客户端保护状态"
          }
        ],
        "thresholds": {
          "DaysSinceLastBackup": {
            "warning": 2,
            "critical": 7
          }
        }
      }
    ]
  }
}
```

### 5.2 智能告警规则

```yaml
# commvault_alerting_rules.yaml
alerting_rules:
  backup_job_failures:
    name: "备份作业失败告警"
    description: "监控备份作业失败情况"
    severity: "high"
    frequency: "5m"
    conditions:
      - metric: "failed_backup_jobs"
        operator: ">"
        threshold: 3
        duration: "15m"
    actions:
      - type: "email"
        recipients:
          - "backup-admin@company.com"
          - "noc@company.com"
      - type: "sms"
        recipients:
          - "+86-138-0000-0001"
      - type: "webhook"
        url: "https://monitoring.company.com/webhook/commvault"
    
  storage_capacity_warning:
    name: "存储容量警告"
    description: "监控存储池容量使用情况"
    severity: "warning"
    frequency: "1h"
    conditions:
      - metric: "storage_utilization"
        operator: ">"
        threshold: 85
        duration: "1h"
      - label_filters:
          pool_type: "disk"
    actions:
      - type: "email"
        recipients:
          - "storage-admin@company.com"
      - type: "ticket"
        system: "ServiceNow"
        priority: "3"
    
  ransomware_detection:
    name: "勒索软件检测"
    description: "检测异常文件修改模式"
    severity: "critical"
    frequency: "1m"
    conditions:
      - metric: "file_modification_rate"
        operator: ">"
        threshold: 1000  # 每分钟文件修改次数
        duration: "5m"
      - metric: "unusual_file_extensions"
        operator: ">"
        threshold: 50
        duration: "10m"
    actions:
      - type: "immediate_shutdown"
        target: "affected_clients"
      - type: "isolation"
        target: "network_segments"
      - type: "notification"
        recipients:
          - "security-team@company.com"
          - "management@company.com"
    
  compliance_violation:
    name: "合规性违规告警"
    description: "监控备份保留策略合规性"
    severity: "medium"
    frequency: "1d"
    conditions:
      - metric: "expired_backups_not_deleted"
        operator: ">"
        threshold: 0
        duration: "1d"
      - metric: "missing_required_backups"
        operator: ">"
        threshold: 0
        duration: "1d"
    actions:
      - type: "email"
        recipients:
          - "compliance@company.com"
          - "audit@company.com"
      - type: "report_generation"
        template: "compliance_violation_report"
    
  performance_degradation:
    name: "性能下降告警"
    description: "监控备份性能指标"
    severity: "warning"
    frequency: "10m"
    conditions:
      - metric: "average_backup_duration"
        operator: ">"
        threshold: 1.5  # 相比基线增加50%
        duration: "30m"
      - metric: "throughput_mb_per_second"
        operator: "<"
        threshold: 50  # MB/s
        duration: "15m"
    actions:
      - type: "email"
        recipients:
          - "performance-team@company.com"
      - type: "auto_scaling"
        target: "media_agents"
        action: "scale_up"
```

## 6. 合规性与审计

### 6.1 数据保护合规框架

```xml
<!-- 数据保护合规性配置 -->
<DataProtectionCompliance>
    <Regulations>
        <!-- GDPR 合规配置 -->
        <GDPR>
            <DataSubjectRights>
                <RightToAccess>Enabled</RightToAccess>
                <RightToErasure>Enabled</RightToErasure>
                <RightToDataPortability>Enabled</RightToDataPortability>
                <RightToObject>Enabled</RightToObject>
            </DataSubjectRights>
            
            <RetentionPolicies>
                <PurposeBasedRetention>Enabled</PurposeBasedRetention>
                <MaximumRetentionPeriod>2555</MaximumRetentionPeriod> <!-- 7 years -->
                <RegularReviewInterval>90</RegularReviewInterval> <!-- 90 days -->
            </RetentionPolicies>
            
            <DataProcessing>
                <ConsentManagement>Enabled</ConsentManagement>
                <PrivacyByDefault>Enabled</PrivacyByDefault>
                <PrivacyByDesign>Enabled</PrivacyByDesign>
            </DataProcessing>
        </GDPR>
        
        <!-- 网络安全法合规 -->
        <CyberSecurityLaw>
            <DataLocalization>Required</DataLocalization>
            <SecurityAssessment>Mandatory</SecurityAssessment>
            <IncidentReporting>Within24Hours</IncidentReporting>
            <CrossBorderTransfer>Restricted</CrossBorderTransfer>
        </CyberSecurityLaw>
        
        <!-- 等保2.0合规 -->
        <LevelProtection2>
            <SecurityLevel>三级</SecurityLevel>
            <TechnicalRequirements>
                <IdentityAuthentication>MultiFactor</IdentityAuthentication>
                <AccessControl>FineGrained</AccessControl>
                <SecurityAudit>FullCoverage</SecurityAudit>
                <IntrusionPrevention>RealTime</IntrusionPrevention>
                <MaliciousCodePrevention>MultiLayer</MaliciousCodePrevention>
            </TechnicalRequirements>
            
            <ManagementRequirements>
                <SecurityManagementSystem>Established</SecurityManagementSystem>
                <PersonnelSecurity>AwarenessTraining</PersonnelSecurity>
                <SystemConstruction>SecureDevelopment</SystemConstruction>
                <SystemOperation>MaintenancePlan</SystemOperation>
            </ManagementRequirements>
        </LevelProtection2>
    </Regulations>
    
    <AuditTrail>
        <Logging>
            <UserActivities>Full</UserActivities>
            <SystemEvents>Full</SystemEvents>
            <DataAccess>Full</DataAccess>
            <PolicyChanges>Full</PolicyChanges>
        </Logging>
        
        <LogRetention>
            <SecurityLogs>180days</SecurityLogs>
            <OperationalLogs>90days</OperationalLogs>
            <AuditLogs>365days</AuditLogs>
        </LogRetention>
        
        <Reporting>
            <ScheduledReports>
                <Daily>BackupStatus,DiskUsage</Daily>
                <Weekly>PerformanceMetrics,ComplianceStatus</Weekly>
                <Monthly>CapacityPlanning,SecurityAssessment</Monthly>
                <Quarterly>RiskAnalysis,RegulatoryCompliance</Quarterly>
            </ScheduledReports>
            
            <AdhocReports>
                <IncidentInvestigation>OnDemand</IncidentInvestigation>
                <ComplianceAudit>OnRequest</ComplianceAudit>
                <ManagementReview>Monthly</ManagementReview>
            </AdhocReports>
        </Reporting>
    </AuditTrail>
</DataProtectionCompliance>
```

### 6.2 自动化合规检查脚本

```python
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动化合规性检查和报告生成工具
"""

import json
import sqlite3
from datetime import datetime, timedelta
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

class ComplianceChecker:
    def __init__(self, commvault_db_path):
        self.db_path = commvault_db_path
        self.compliance_rules = self.load_compliance_rules()
        self.violations = []
        
    def load_compliance_rules(self):
        """加载合规性规则"""
        return {
            'backup_frequency': {
                'critical_systems': {'max_gap_hours': 24},
                'important_systems': {'max_gap_hours': 168},  # 1周
                'standard_systems': {'max_gap_hours': 336}    # 2周
            },
            'retention_compliance': {
                'minimum_retention_days': 30,
                'maximum_retention_years': 7
            },
            'security_compliance': {
                'encryption_required': True,
                'access_logging': True,
                'regular_audits': True
            }
        }
    
    def check_backup_frequency_compliance(self):
        """检查备份频率合规性"""
        conn = sqlite3.connect(self.db_path)
        
        query = """
        SELECT 
            c.ClientName,
            c.ClientGroupName,
            MAX(j.EndTime) as LastBackupTime,
            CASE 
                WHEN c.ClientGroupName LIKE '%Critical%' THEN 'critical'
                WHEN c.ClientGroupName LIKE '%Important%' THEN 'important'
                ELSE 'standard'
            END as system_type
        FROM Clients c
        LEFT JOIN JobHistory j ON c.ClientId = j.ClientId 
            AND j.JobType = 'Backup' 
            AND j.Status = 'Completed'
        GROUP BY c.ClientId
        """
        
        df = pd.read_sql_query(query, conn)
        conn.close()
        
        current_time = datetime.now()
        violations = []
        
        for _, row in df.iterrows():
            if pd.isna(row['LastBackupTime']):
                violations.append({
                    'type': 'missing_backup',
                    'client': row['ClientName'],
                    'severity': 'critical',
                    'description': '从未执行过备份'
                })
                continue
            
            last_backup = datetime.strptime(row['LastBackupTime'], '%Y-%m-%d %H:%M:%S')
            gap_hours = (current_time - last_backup).total_seconds() / 3600
            
            system_type = row['system_type']
            max_gap = self.compliance_rules['backup_frequency'][f'{system_type}_systems']['max_gap_hours']
            
            if gap_hours > max_gap:
                violations.append({
                    'type': 'backup_gap',
                    'client': row['ClientName'],
                    'system_type': system_type,
                    'gap_hours': round(gap_hours, 2),
                    'max_allowed_hours': max_gap,
                    'severity': 'high' if system_type == 'critical' else 'medium',
                    'description': f'备份间隔超过规定时间 {gap_hours:.1f}小时 > {max_gap}小时'
                })
        
        self.violations.extend(violations)
        return violations
    
    def check_retention_compliance(self):
        """检查数据保留合规性"""
        conn = sqlite3.connect(self.db_path)
        
        query = """
        SELECT 
            bp.BackupSetName,
            sp.StoragePolicyName,
            sp.RetentionDays,
            COUNT(*) as backup_count
        FROM BackupInfo bp
        JOIN StoragePolicies sp ON bp.StoragePolicyId = sp.StoragePolicyId
        WHERE bp.BackupTime >= datetime('now', '-2 years')
        GROUP BY bp.BackupSetId
        """
        
        df = pd.read_sql_query(query, conn)
        conn.close()
        
        violations = []
        
        for _, row in df.iterrows():
            retention_days = row['RetentionDays']
            
            # 检查最小保留期
            if retention_days < self.compliance_rules['retention_compliance']['minimum_retention_days']:
                violations.append({
                    'type': 'insufficient_retention',
                    'backup_set': row['BackupSetName'],
                    'current_retention': retention_days,
                    'minimum_required': self.compliance_rules['retention_compliance']['minimum_retention_days'],
                    'severity': 'medium',
                    'description': f'保留期不足: {retention_days}天 < {self.compliance_rules["retention_compliance"]["minimum_retention_days"]}天'
                })
            
            # 检查最大保留期
            if retention_days > (self.compliance_rules['retention_compliance']['maximum_retention_years'] * 365):
                violations.append({
                    'type': 'excessive_retention',
                    'backup_set': row['BackupSetName'],
                    'current_retention': retention_days,
                    'maximum_allowed': self.compliance_rules['retention_compliance']['maximum_retention_years'] * 365,
                    'severity': 'low',
                    'description': f'保留期过长: {retention_days}天 > {self.compliance_rules["retention_compliance"]["maximum_retention_years"] * 365}天'
                })
        
        self.violations.extend(violations)
        return violations
    
    def check_security_compliance(self):
        """检查安全合规性"""
        conn = sqlite3.connect(self.db_path)
        
        # 检查加密状态
        encryption_query = """
        SELECT 
            COUNT(*) as total_backups,
            SUM(CASE WHEN IsEncrypted = 1 THEN 1 ELSE 0 END) as encrypted_backups
        FROM BackupInfo
        WHERE BackupTime >= datetime('now', '-1 month')
        """
        
        enc_df = pd.read_sql_query(encryption_query, conn)
        
        # 检查访问日志
        audit_query = """
        SELECT COUNT(*) as audit_entries
        FROM AuditLog
        WHERE EventTime >= datetime('now', '-24 hours')
        """
        
        audit_df = pd.read_sql_query(audit_query, conn)
        conn.close()
        
        violations = []
        
        # 加密合规检查
        if self.compliance_rules['security_compliance']['encryption_required']:
            encryption_rate = (enc_df.iloc[0]['encrypted_backups'] / enc_df.iloc[0]['total_backups']) * 100
            if encryption_rate < 95:  # 要求95%以上的备份加密
                violations.append({
                    'type': 'encryption_non_compliance',
                    'encryption_rate': round(encryption_rate, 2),
                    'required_rate': 95,
                    'severity': 'high',
                    'description': f'加密率不足: {encryption_rate:.1f}% < 95%'
                })
        
        # 审计日志检查
        if self.compliance_rules['security_compliance']['access_logging']:
            daily_audit_entries = audit_df.iloc[0]['audit_entries']
            if daily_audit_entries < 1000:  # 要求每日至少1000条审计记录
                violations.append({
                    'type': 'insufficient_audit_logging',
                    'daily_entries': daily_audit_entries,
                    'minimum_required': 1000,
                    'severity': 'medium',
                    'description': f'审计日志不足: {daily_audit_entries}条 < 1000条'
                })
        
        self.violations.extend(violations)
        return violations
    
    def generate_compliance_report(self):
        """生成合规性报告"""
        # 执行所有合规检查
        self.check_backup_frequency_compliance()
        self.check_retention_compliance()
        self.check_security_compliance()
        
        # 生成报告数据
        report_data = {
            'generated_at': datetime.now().isoformat(),
            'total_violations': len(self.violations),
            'violations_by_severity': {
                'critical': len([v for v in self.violations if v['severity'] == 'critical']),
                'high': len([v for v in self.violations if v['severity'] == 'high']),
                'medium': len([v for v in self.violations if v['severity'] == 'medium']),
                'low': len([v for v in self.violations if v['severity'] == 'low'])
            },
            'violations_by_type': {},
            'detailed_violations': self.violations
        }
        
        # 按类型统计违规
        for violation in self.violations:
            v_type = violation['type']
            if v_type not in report_data['violations_by_type']:
                report_data['violations_by_type'][v_type] = 0
            report_data['violations_by_type'][v_type] += 1
        
        return report_data
    
    def create_visual_dashboard(self, report_data):
        """创建可视化仪表板"""
        # 创建违规严重程度饼图
        plt.figure(figsize=(12, 5))
        
        # 子图1: 严重程度分布
        plt.subplot(1, 2, 1)
        severity_counts = [
            report_data['violations_by_severity']['critical'],
            report_data['violations_by_severity']['high'],
            report_data['violations_by_severity']['medium'],
            report_data['violations_by_severity']['low']
        ]
        severity_labels = ['严重', '高', '中', '低']
        colors = ['#FF6B6B', '#FFE66D', '#4ECDC4', '#45B7D1']
        
        plt.pie(severity_counts, labels=severity_labels, colors=colors, autopct='%1.1f%%')
        plt.title('违规严重程度分布')
        
        # 子图2: 违规类型分布
        plt.subplot(1, 2, 2)
        type_counts = list(report_data['violations_by_type'].values())
        type_labels = list(report_data['violations_by_type'].keys())
        
        bars = plt.bar(range(len(type_counts)), type_counts, color=colors[:len(type_counts)])
        plt.xticks(range(len(type_labels)), type_labels, rotation=45, ha='right')
        plt.ylabel('违规数量')
        plt.title('违规类型分布')
        
        # 添加数值标签
        for bar, count in zip(bars, type_counts):
            plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1, 
                    str(count), ha='center', va='bottom')
        
        plt.tight_layout()
        plt.savefig('compliance_dashboard.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def send_email_report(self, report_data, recipients):
        """发送邮件报告"""
        # 创建邮件
        msg = MIMEMultipart()
        msg['From'] = 'compliance@company.com'
        msg['To'] = ', '.join(recipients)
        msg['Subject'] = f'Commvault 合规性报告 - {datetime.now().strftime("%Y-%m-%d")}'
        
        # 邮件正文
        body = f"""
        <html>
        <body>
        <h2>Commvault 合规性检查报告</h2>
        <p><strong>报告生成时间:</strong> {report_data['generated_at']}</p>
        <p><strong>总违规数量:</strong> {report_data['total_violations']}</p>
        
        <h3>违规严重程度统计</h3>
        <ul>
        <li>严重: {report_data['violations_by_severity']['critical']}</li>
        <li>高: {report_data['violations_by_severity']['high']}</li>
        <li>中: {report_data['violations_by_severity']['medium']}</li>
        <li>低: {report_data['violations_by_severity']['low']}</li>
        </ul>
        
        <h3>主要违规类型</h3>
        <ul>
        """
        
        for v_type, count in report_data['violations_by_type'].items():
            body += f"<li>{v_type}: {count}</li>"
        
        body += """
        </ul>
        <p>详细报告请查看附件。</p>
        <p>此邮件由系统自动发送，请勿回复。</p>
        </body>
        </html>
        """
        
        msg.attach(MIMEText(body, 'html'))
        
        # 附加详细报告
        report_json = json.dumps(report_data, indent=2, ensure_ascii=False)
        attachment = MIMEApplication(report_json.encode('utf-8'))
        attachment.add_header('Content-Disposition', 'attachment', filename='detailed_compliance_report.json')
        msg.attach(attachment)
        
        # 发送邮件
        try:
            server = smtplib.SMTP('smtp.company.com', 587)
            server.starttls()
            server.login('compliance@company.com', 'password')
            server.send_message(msg)
            server.quit()
            print("合规性报告邮件发送成功")
        except Exception as e:
            print(f"发送邮件失败: {e}")

def main():
    # 初始化合规检查器
    checker = ComplianceChecker('/opt/commvault/database/CommServ.db')
    
    # 生成报告
    report = checker.generate_compliance_report()
    
    # 创建可视化
    checker.create_visual_dashboard(report)
    
    # 发送报告
    recipients = ['compliance@company.com', 'audit@company.com', 'management@company.com']
    checker.send_email_report(report, recipients)
    
    # 保存报告到文件
    with open('compliance_report.json', 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
    
    print("合规性检查完成，报告已生成")

if __name__ == "__main__":
    main()
```

---
*本文档基于企业级Commvault实践经验编写，并持续更新最新的技术和最佳实践。*