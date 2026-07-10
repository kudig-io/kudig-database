---
title: VMware vSphere 企业级灾备与业务连续性
description: '**作者**: 企业级灾备架构师 | **版本**: v2.0 | **更新时间**: 2026-05-18'
summary: '**作者**: 企业级灾备架构师 | **版本**: v2.0 | **更新时间**: 2026-05-18'
category: disaster-recovery
tags:
- k8s
- disaster-recovery
- backup
- ha
- prometheus
- redis
- mysql
- postgresql
- job
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- VMware vSphere 企业级灾备与业务连续性 是什么
- 如何 VMware vSphere 企业级灾备与业务连续性
- Kubernetes 30 disaster recovery business continuity 最佳实践
trigger_keywords:
- VMware
- vSphere
- 企业级灾备与业务连续性
- disaster
- recovery
- business
- continuity
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
- redis-basics
- mysql-basics
- backup-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# VMware vSphere 企业级灾备与业务连续性

> **作者**: 企业级灾备架构师 | **版本**: v2.0 | **更新时间**: 2026-05-18
> **适用场景**: VMware vSphere 虚拟化环境灾备设计 | **复杂度**: ⭐⭐⭐⭐⭐

---

<!-- chunk: 概述 -->## 概述

VMware vSphere 是业界最成熟的企业级虚拟化平台，承载着全球大量企业关键业务工作负载。在灾难恢复领域，vSphere 提供了从高可用（HA）、容错（FT）到站点恢复管理（SRM）的完整灾备技术栈。本文档从生产环境运维专家角度，深入探讨 vSphere 的企业级灾备架构设计、业务连续性策略制定以及运维管理最佳实践。

## RPO 与 RTO 定义

在设计 vSphere 灾备方案之前，必须明确定义两大核心指标：

- **RPO（Recovery Point Objective，恢复点目标）**：指灾难发生后，系统可以容忍丢失的最大数据量，以时间维度衡量。例如 RPO = 15 分钟意味着最多允许丢失灾难发生前 15 分钟的数据。RPO 直接决定了备份频率和复制模式（同步 vs 异步）。
- **RTO（Recovery Time Objective，恢复时间目标）**：指灾难发生后，系统从不可用状态恢复到正常服务水平所需的最大时间。例如 RTO = 4 小时意味着灾难发生后 4 小时内必须恢复业务运行。RTO 直接影响恢复流程的自动化程度和备用资源的就绪状态。

不同业务系统应根据其关键性等级设定差异化的 RPO/RTO 目标：

```yaml
rpo_rto_targets:
  tier_1_critical:
    description: "核心交易系统"
    rpo: "15 分钟"
    rto: "1 小时"
    technology: "同步复制 + 自动故障切换"
    
  tier_2_important:
    description: "重要业务系统（CRM、ERP）"
    rpo: "1 小时"
    rto: "4 小时"
    technology: "异步复制 + 半自动故障切换"
    
  tier_3_standard:
    description: "一般业务系统（OA、邮件）"
    rpo: "4 小时"
    rto: "8 小时"
    technology: "定期备份 + 手动恢复"
    
  tier_4_archive:
    description: "归档系统"
    rpo: "24 小时"
    rto: "24 小时"
    technology: "离线备份 + 按需恢复"
```

---

<!-- chunk: 架构设计 -->## 架构设计

## 双站点灾备架构

企业级 vSphere 灾备通常采用主备站点（Active-Standby）或双活站点（Active-Active）架构。以下是典型的主备站点灾备架构，涵盖计算、存储、网络和管理四个层面。

```mermaid
graph TB
    subgraph "主数据中心 (Primary Site)"
        VC1[vCenter Server<br/>主站点管理]
        ESXI1[ESXi Host 1<br/>计算节点]
        ESXI2[ESXi Host 2<br/>计算节点]
        ESXI3[ESXi Host 3<br/>计算节点]
        VM1[ERP 生产虚拟机]
        VM2[CRM 应用虚拟机]
        VM3[数据库虚拟机]
        PRIMARY_STORAGE[主存储阵列<br/>vSAN / SAN]
        PRIMARY_NETWORK[主网络交换<br/>vSwitch / DVS]
    end
    
    subgraph "灾备数据中心 (DR Site)"
        VC2[vCenter Server<br/>灾备站点管理]
        ESXI4[ESXi Host 4<br/>计算节点]
        ESXI5[ESXi Host 5<br/>计算节点]
        ESXI6[ESXi Host 6<br/>计算节点]
        DR_VM1[ERP 灾备虚拟机]
        DR_VM2[CRM 灾备虚拟机]
        DR_VM3[数据库灾备虚拟机]
        DR_STORAGE[灾备存储阵列<br/>vSAN / SAN]
        DR_NETWORK[灾备网络交换<br/>vSwitch / DVS]
    end
    
    subgraph "灾备管理层"
        SRM[Site Recovery Manager<br/>站点恢复管理器]
        VR[VMware Replication<br/>虚拟机复制]
        PLANNER[DR Planning<br/>灾备计划编排]
    end
    
    subgraph "存储复制层"
        ARRAY_REPL[存储阵列复制<br/>同步/异步]
        VSAN_REPL[vSAN 数据复制<br/>跨站点拉伸集群]
    end
    
    subgraph "网络互联层"
        VPN[IPsec VPN 隧道]
        DEDICATED[专线连接<br/>10Gbps+"]
        DNS_FAILOVER[DNS 故障转移<br/>GSLB]
    end
    
    subgraph "监控与告警"
        VROPS[vRealize Operations<br/>性能监控]
        VRLI[vRealize Log Insight<br/>日志分析]
        ALERTING[告警引擎<br/>多通道通知]
    end
    
    VC1 --> ESXI1 & ESXI2 & ESXI3
    ESXI1 --> VM1
    ESXI2 --> VM2
    ESXI3 --> VM3
    VM1 & VM2 & VM3 --> PRIMARY_STORAGE
    
    VC2 --> ESXI4 & ESXI5 & ESXI6
    ESXI4 --> DR_VM1
    ESXI5 --> DR_VM2
    ESXI6 --> DR_VM3
    DR_VM1 & DR_VM2 & DR_VM3 --> DR_STORAGE
    
    SRM --> VC1 & VC2
    VR --> VM1 & VM2 & VM3
    
    ARRAY_REPL --> PRIMARY_STORAGE & DR_STORAGE
    VSAN_REPL --> PRIMARY_STORAGE & DR_STORAGE
    
    VPN --> PRIMARY_NETWORK & DR_NETWORK
    DEDICATED --> PRIMARY_NETWORK & DR_NETWORK
    
    VROPS --> VC1 & VC2
    VRLI --> VROPS
    ALERTING --> VROPS
```

## 架构选型对比

| 架构类型 | RPO | RTO | 成本 | 复杂度 | 适用场景 |
|:---|:---|:---|:---|:---|:---|
| 主备站点（Active-Standby） | 分钟~小时 | 小时级 | 中等 | 中等 | 大多数企业灾备 |
| 双活站点（Active-Active） | 接近零 | 秒级 | 高 | 高 | 金融核心交易 |
| vSAN 拉伸集群 | 接近零 | 秒级 | 高 | 中高 | 同城双活 |
| 云灾备（DRaaS） | 小时级 | 小时级 | 按需付费 | 低 | 中小企业 |

---

<!-- chunk: 核心配置 -->## 核心配置

## vCenter Server 高可用配置

```yaml
# vCenter Server 高可用部署配置
vcenter_ha:
  deployment:
    type: "embedded"  # embedded 或 external
    size: "large"     # tiny/small/medium/large/xlarge
    platform: "vcsa"  # vCenter Server Appliance
    
  active_node:
    hostname: "vcenter-active.company.com"
    ip_address: "192.168.10.10"
    network:
      netmask: "255.255.255.0"
      gateway: "192.168.10.1"
      dns_servers:
        - "192.168.10.53"
        - "192.168.10.54"
      ntp_servers:
        - "ntp.company.com"
        - "time.google.com"
    
  passive_node:
    hostname: "vcenter-passive.company.com"
    ip_address: "192.168.10.11"
    
  witness_node:
    hostname: "vcenter-witness.company.com"
    ip_address: "192.168.10.12"
    
  database:
    type: "embedded"    # PostgreSQL
    backup_schedule: "0 2 * * *"
    backup_retention_days: 14
    backup_location: "nfs://backup-nas/vcenter-backups"
    
  sso:
    domain: "vsphere.local"
    site: "CompanyPrimary"
    password_policy:
      min_length: 12
      complexity: "high"
      max_age_days: 90
      
  networking:
    management_network:
      portgroup: "Management-Network"
      vlan: 10
      mtu: 1500
    backup_network:
      portgroup: "Backup-Network"
      vlan: 30
      mtu: 9000  # Jumbo frames
      
  security:
    tls_min_version: "1.2"
    certificate:
      type: "custom"  # vmware / custom / third-party-ca
      key_size: 4096
      algorithm: "RSA"
      validity_days: 730
    session_timeout:
      web_client: 30    # 分钟
      api: 20            # 分钟
      shell: 10          # 分钟
```

## ESXi 主机配置

```bash
#!/bin/bash
# ESXi 主机初始化配置脚本

# 1. 网络配置
echo "=== 配置网络 ==="
esxcli network vswitch standard add --vswitch-name=vSwitch0
esxcli network vswitch standard portgroup add --portgroup-name="Management Network" --vswitch-name=vSwitch0
esxcli network vswitch standard portgroup add --portgroup-name="VM Network" --vswitch-name=vSwitch0
esxcli network vswitch standard portgroup add --portgroup-name="Storage Network" --vswitch-name=vSwitch0
esxcli network vswitch standard portgroup add --portgroup-name="vMotion Network" --vswitch-name=vSwitch0

# 配置 VLAN
esxcli network vswitch standard portgroup set --portgroup-name="VM Network" --vlan-id 20
esxcli network vswitch standard portgroup set --portgroup-name="Storage Network" --vlan-id 30
esxcli network vswitch standard portgroup set --portgroup-name="vMotion Network" --vlan-id 40

# 启用巨帧
esxcli network vswitch standard set --mtu 9000 --vswitch-name=vSwitch0

# 2. 存储配置
echo "=== 配置存储 ==="
esxcli storage core adapter rescan --all
esxcli storage core path list
esxcli storage vmfs extent list

# 配置多路径策略（Round Robin）
esxcli storage nmp satp rule add --satp=VMW_SATP_ALUA --psp=VMW_PSP_RR --type=VENDOR --vendor=DELL --model="PowerVault"
esxcli storage nmp device set --device=naa.6005076300810186d00000000000001 --psp=VMW_PSP_RR

# 3. 安全加固
echo "=== 安全加固 ==="
esxcli system settings advanced set -o /UserVars/ESXiShellInteractiveTimeOut -i 600
esxcli system settings advanced set -o /UserVars/ESXiShellTimeOut -i 3600
esxcli system settings advanced set -o /Net/GuestIPHack -i 0
esxcli system settings advanced set -o /Security/PasswordQualityControl -s "retry=3 min=disabled,disabled,disabled,12,12"

# 禁用不必要的服务
esxcli system settings advanced set -o /UserVars/SuppressShellWarning -i 1
esxcli network firewall set --default-action=false --enabled=true
esxcli network firewall ruleset set --ruleset-id=sshServer --allowed-all=false
esxcli network firewall ruleset set --ruleset-id=sshServer --allowed-ip=192.168.10.0/24

# 4. NTP 同步
echo "=== 配置 NTP ==="
echo "server ntp.company.com iburst" >> /etc/ntp.conf
echo "server time.google.com iburst" >> /etc/ntp.conf
/etc/init.d/ntpd restart
esxcli system time get

# 5. SNMP 监控
echo "=== 配置 SNMP ==="
esxcli system snmp set --enable=true
esxcli system snmp set --communities=public
esxcli system snmp set --targets=192.168.10.100@162/public
esxcli system snmp set --port=161
```

## Site Recovery Manager 配置

```yaml
# SRM 恢复计划配置
srm_recovery_plan:
  name: "Critical-Business-Applications"
  description: "核心业务应用灾难恢复计划"
  priority: 1
  
  protected_site:
    vcenter: "vcenter-primary.company.com"
    username: "administrator@vsphere.local"
    storage_arrays:
      - name: "Primary-Storage-Array"
        type: "Dell EMC PowerMax"
        ip: "192.168.20.10"
        replication_type: "synchronous"
        
  recovery_site:
    vcenter: "vcenter-dr.company.com"
    username: "administrator@vsphere.local"
    storage_arrays:
      - name: "DR-Storage-Array"
        type: "Dell EMC PowerMax"
        ip: "192.168.30.10"
        replication_type: "synchronous"
  
  recovery_groups:
    - group_name: "Group-1-Database"
      priority: 1
      pre_power_on_steps:
        - "验证存储复制完整性"
        - "挂载灾备存储卷"
      vms:
        - name: "ERP-Database-Primary"
          protection_group: "PG-Critical-Apps"
          recovery_test_network: "vlan-100-test"
          ip_customization:
            adapter: "Network adapter 1"
            ip: "192.168.30.50"
            subnet_mask: "255.255.255.0"
            gateway: "192.168.30.1"
            dns: ["192.168.30.53"]
            
        - name: "CRM-Database-Primary"
          protection_group: "PG-Critical-Apps"
          ip_customization:
            adapter: "Network adapter 1"
            ip: "192.168.30.51"
            subnet_mask: "255.255.255.0"
            gateway: "192.168.30.1"
            
    - group_name: "Group-2-Application"
      priority: 2
      depends_on: "Group-1-Database"
      vms:
        - name: "ERP-App-Server"
          protection_group: "PG-Critical-Apps"
          ip_customization:
            ip: "192.168.30.60"
            subnet_mask: "255.255.255.0"
            gateway: "192.168.30.1"
            
        - name: "CRM-App-Server"
          protection_group: "PG-Critical-Apps"
          ip_customization:
            ip: "192.168.30.61"
            subnet_mask: "255.255.255.0"
            gateway: "192.168.30.1"
            
    - group_name: "Group-3-Web"
      priority: 3
      depends_on: "Group-2-Application"
      vms:
        - name: "Web-Frontend-01"
        - name: "Web-Frontend-02"
  
  post_power_on_steps:
    - "验证数据库连接"
    - "验证应用健康检查"
    - "更新 DNS 记录"
    - "通知运维团队"
    
  rpo: "15 分钟"
  rto: "60 分钟"
```

---

<!-- chunk: 备份策略 -->## 备份策略

## 备份分层设计

企业级 vSphere 环境需要根据虚拟机的重要性制定差异化的备份策略，在数据安全性和存储成本之间取得平衡。

```yaml
# 备份策略分层配置
backup_tier_policy:
  tier_1_gold:
    description: "核心生产数据库虚拟机"
    backup_method: "VMware snapshot + VDDK 增量"
    schedule:
      full_backup: "每日 22:00"
      incremental: "每 4 小时"
      snapshot: "每小时（存储阵列快照）"
    retention:
      hourly_snapshots: 24
      daily_full: 14
      weekly_full: 8
      monthly_full: 12
    replication:
      target: "灾备数据中心"
      mode: "synchronous"
      rpo: "0（同步复制）"
    application_aware: true
    pre_script: "/opt/scripts/quiesce-db.sh"
    post_script: "/opt/scripts/unquiesce-db.sh"
    
  tier_2_silver:
    description: "应用服务器和中间件"
    backup_method: "VMware snapshot + 增量"
    schedule:
      full_backup: "每周日 22:00"
      incremental: "每日 22:00"
    retention:
      daily_incremental: 14
      weekly_full: 8
      monthly_full: 6
    replication:
      target: "灾备数据中心"
      mode: "asynchronous"
      rpo: "1 小时"
    application_aware: false
    
  tier_3_bronze:
    description: "开发/测试环境虚拟机"
    backup_method: "VMware snapshot 全量"
    schedule:
      full_backup: "每周日 02:00"
    retention:
      weekly_full: 4
      monthly_full: 3
    replication: null
```

## vSphere 存储复制配置

```bash
#!/bin/bash
# 存储阵列复制配置脚本

configure_primary_array() {
    echo "配置主存储阵列复制..."
    
    ssh storage-admin@192.168.20.10 "
        # 创建复制一致性组
        create consistency-group rg-critical-apps
        add volume ERP-DB-VOL to rg-critical-apps
        add volume CRM-DB-VOL to rg-critical-apps
        add volume SHARED-VOL to rg-critical-apps
        
        # 配置同步复制策略
        set replication-policy synchronous
        set replication-mode active-passive
        set rpo-threshold 900          # 15分钟RPO告警阈值
        set compression enabled
        set encryption AES-256
        
        # 激活复制
        activate consistency-group rg-critical-apps
    "
}

verify_replication_health() {
    echo "验证复制健康状态..."
    
    ssh storage-admin@192.168.20.10 "
        show replication-status rg-critical-apps
        show replication-lag rg-critical-apps
        show replication-health rg-critical-apps
        show consistency-group rg-critical-apps detail
    "
}

# 验证灾备端
verify_dr_array() {
    echo "验证灾备存储阵列..."
    
    ssh storage-admin@192.168.30.10 "
        show replication-group rg-critical-apps-dr
        show replication-mode
        show available-recovery-points
    "
}
```

---

<!-- chunk: 恢复流程 -->## 恢复流程

## 步骤一：灾难确认与决策

```yaml
disaster_confirmation:
  step_1:
    action: "监控系统自动检测到主站点异常"
    tools: ["vRealize Operations", "Ping 监控", "SNMP Trap"]
    auto_detection:
      - "ESXi 主机全部不可达"
      - "vCenter 连接超时超过 5 分钟"
      - "存储阵列复制中断"
      - "网络链路完全断开"
    
  step_2:
    action: "值班运维人员确认灾难范围"
    verification:
      - "尝试 VPN 连接主站点"
      - "联系主站点现场人员"
      - "检查运营商网络状态"
      - "确认业务影响范围"
    timeout: "15 分钟"
    
  step_3:
    action: "灾备决策委员会决策"
    participants:
      - "IT 总监"
      - "运维经理"
      - "业务部门负责人"
    decision_criteria:
      - "主站点恢复时间超过 RTO"
      - "数据完整性确认"
      - "业务影响评估结果"
    escalation:
      - "L1 (15分钟): 值班运维评估"
      - "L2 (30分钟): 运维经理参与"
      - "L3 (1小时): IT总监决策启动灾备切换"
```

## 步骤二：灾备切换执行

```bash
#!/bin/bash
# SRM 自动化灾备切换脚本

echo "=== 开始灾备切换 ==="
echo "切换时间: $(date)"

# 1. 验证灾备站点就绪
echo "[Step 1/7] 验证灾备站点就绪..."
srm-check-readiness --site dr \
  --vcenter vcenter-dr.company.com \
  --check-storage \
  --check-network \
  --check-compute

# 2. 确认复制数据一致性
echo "[Step 2/7] 确认复制数据一致性..."
srm-verify-replication --plan "Critical-Business-Applications" \
  --max-lag-seconds 60 \
  --require-consistent

# 3. 暂停主站点（如果可达）
echo "[Step 3/7] 暂停主站点..."
timeout 30 bash -c "
  govc vm.power -off -force /Datacenter/vm/ERP-Database-Primary 2>/dev/null
  govc vm.power -off -force /Datacenter/vm/CRM-App-Server 2>/dev/null
" || echo "主站点不可达，跳过关闭步骤"

# 4. 执行存储故障切换
echo "[Step 4/7] 执行存储故障切换..."
ssh storage-admin@192.168.30.10 "
    failover consistency-group rg-critical-apps
    verify recovery-point latest
"

# 5. 执行 SRM 恢复计划
echo "[Step 5/7] 执行 SRM 恢复计划..."
srm-run-recovery --plan "Critical-Business-Applications" \
  --mode unplanned \
  --skip-test-network \
  --wait-for-completion \
  --timeout 3600

# 6. 验证虚拟机启动
echo "[Step 6/7] 验证虚拟机状态..."
for vm in "ERP-Database-Primary" "CRM-Database-Primary" "ERP-App-Server" "CRM-App-Server"; do
    echo "检查虚拟机: $vm"
    govc vm.info -u administrator@vsphere.local:${DR_PASSWORD}@vcenter-dr.company.com /DR-Datacenter/vm/$vm
    govc vm.power -on /DR-Datacenter/vm/$vm 2>/dev/null
done

# 7. 更新 DNS 记录
echo "[Step 7/7] 更新 DNS 记录..."
python3 /opt/scripts/update-dns-failover.py --target-site dr

echo "=== 灾备切换完成 ==="
echo "完成时间: $(date)"
echo "请执行应用层验证..."
```

## 步骤三：恢复后验证

```yaml
post_recovery_validation:
  infrastructure_validation:
    - name: "虚拟机电源状态检查"
      command: "govc vm.info -json | jq '.VirtualMachines[].Runtime.PowerState'"
      expected: "poweredOn"
      
    - name: "网络连通性检查"
      command: "ping -c 3 192.168.30.50"
      expected: "0% packet loss"
      
    - name: "存储挂载检查"
      command: "govc datastore.info"
      expected: "accessible"
      
  application_validation:
    - name: "数据库连接验证"
      command: "sqlcmd -S 192.168.30.50 -Q 'SELECT @@VERSION'"
      expected: "返回版本信息"
      
    - name: "应用健康检查"
      command: "curl -s http://192.168.30.60/health"
      expected: "HTTP 200"
      
    - name: "业务功能验证"
      command: "python3 /opt/scripts/business-smoke-test.py"
      expected: "所有测试通过"
```

---

<!-- chunk: 容灾演练方案 -->## 容灾演练方案

## 年度演练计划

```yaml
dr_drill_schedule:
  q1_tabletop:
    type: "桌面推演"
    scope: "灾备流程验证（无实际切换）"
    participants: ["运维团队", "网络团队", "安全团队"]
    duration: "4 小时"
    deliverables:
      - "灾备流程审查报告"
      - "改进建议清单"
      - "RPO/RTO 可行性评估"
    
  q2_partial_failover:
    type: "部分故障切换演练"
    scope: "非核心业务虚拟机故障切换"
    participants: ["运维团队", "应用团队"]
    duration: "8 小时"
    steps:
      - "选择非核心虚拟机组"
      - "在灾备站点执行测试故障切换"
      - "验证数据一致性"
      - "执行清理操作"
    deliverables:
      - "故障切换测试报告"
      - "数据一致性验证报告"
      - "RTO 实际测量值"
    
  q3_full_drill:
    type: "全量灾备演练"
    scope: "核心业务完整故障切换与回切"
    participants: ["全部IT团队", "业务部门", "管理层"]
    duration: "2 天"
    steps:
      - "提前通知所有相关方"
      - "主站点维护模式"
      - "执行完整故障切换"
      - "灾备站点运行业务（2小时）"
      - "执行问题回切"
      - "验证数据完整性"
    deliverables:
      - "完整演练报告"
      - "RTO/RPO 达标评估"
      - "问题清单与整改计划"
    
  q4_annual_audit:
    type: "年度灾备审计"
    scope: "灾备体系全面评估"
    participants: ["IT管理层", "审计部门", "第三方顾问"]
    deliverables:
      - "灾备能力成熟度评估"
      - "合规性审计报告"
      - "下年度改进计划"
```

---

<!-- chunk: 监控告警 -->## 监控告警

## vRealize Operations 监控配置

```yaml
# vRealize Operations Manager 灾备监控配置
management_packs:
  - name: "vSphere Management Pack"
    version: "8.6"
    collectors:
      - name: "vCenter Primary Collector"
        type: "vcenter"
        endpoint: "vcenter-primary.company.com"
        credentials:
          username: "administrator@vsphere.local"
          password: "${VCENTER_PASSWORD}"
        collection_interval: "5m"
        
      - name: "vCenter DR Collector"
        type: "vcenter"
        endpoint: "vcenter-dr.company.com"
        credentials:
          username: "administrator@vsphere.local"
          password: "${DR_VCENTER_PASSWORD}"
        collection_interval: "5m"

  - name: "SRM Management Pack"
    version: "8.4"
    collectors:
      - name: "SRM Status Collector"
        endpoint: "srm-primary.company.com"
        monitor:
          - replication_health
          - recovery_plan_status
          - rpo_compliance
```

## 关键告警规则

```yaml
# vSphere 灾备告警规则
alert_rules:
  - name: "HostUnreachable"
    expression: "esxi_host_status == 'unreachable'"
    duration: "5m"
    severity: "critical"
    channels: ["email", "slack", "sms"]
    recipients: ["noc@company.com", "infra-team@company.com"]
    runbook: "https://wiki.company.com/runbooks/host-unreachable"
    
  - name: "DatastoreSpaceCritical"
    expression: "datastore_usage_percent > 90"
    duration: "10m"
    severity: "critical"
    channels: ["email", "slack"]
    recipients: ["storage-team@company.com"]
    runbook: "https://wiki.company.com/runbooks/datastore-full"
    
  - name: "ReplicationLagExceeded"
    expression: "replication_lag_seconds > 900"  # 超过 RPO
    duration: "5m"
    severity: "critical"
    channels: ["email", "slack", "pagerduty"]
    recipients: ["dr-team@company.com"]
    runbook: "https://wiki.company.com/runbooks/replication-lag"
    
  - name: "VMHighCPUReady"
    expression: "vm_cpu_ready_percent > 10"
    duration: "15m"
    severity: "warning"
    channels: ["slack"]
    recipients: ["performance-team@company.com"]
    
  - name: "VCSAServerDown"
    expression: "vcenter_service_status == 'down'"
    duration: "3m"
    severity: "critical"
    channels: ["email", "sms", "pagerduty"]
    recipients: ["platform-team@company.com"]
    
  - name: "SnapshotTooLarge"
    expression: "vm_snapshot_size_gb > 100"
    duration: "0m"
    severity: "warning"
    channels: ["slack"]
    recipients: ["storage-team@company.com"]
    message: "虚拟机快照过大可能影响备份和存储性能"
```

## Prometheus 集成监控

```yaml
# Prometheus vSphere Exporter 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: vsphere-alerts
  namespace: monitoring
data:
  vsphere-alerts.yml: |
    groups:
      - name: vsphere.dr
        rules:
          - alert: VSpereReplicationRPOViolation
            expr: vsphere_replication_rpo_seconds > vsphere_replication_rpo_target_seconds
            for: 5m
            labels:
              severity: critical
              team: disaster-recovery
            annotations:
              summary: "vSphere 复制 RPO 违规"
              description: "虚拟机 {{ $labels.vm_name }} 复制延迟 {{ $value }}秒，超过 RPO 目标"
              
          - alert: VSphereBackupJobFailed
            expr: increase(vsphere_backup_failed_total[24h]) > 0
            for: 5m
            labels:
              severity: critical
            annotations:
              summary: "vSphere 备份作业失败"
              
          - alert: VSphereDatastoreCapacityLow
            expr: vsphere_datastore_free_space_percent < 15
            for: 10m
            labels:
              severity: warning
            annotations:
              summary: "数据存储 {{ $labels.datastore }} 剩余空间不足 15%"
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

## 部署最佳实践

**硬件规划**：vCenter Server 最小 4 核 16GB 内存（推荐 8 核 32GB），ESXi 主机推荐双路 CPU、128GB 以上内存。主备站点硬件规格应对等，确保灾备站点能承载全部关键负载。

**网络设计**：管理网络、虚拟机网络、存储网络、vMotion 网络应物理或 VLAN 隔离。站点间至少 10Gbps 专线连接，启用巨帧（MTU 9000）以优化存储复制性能。

**存储配置**：使用 RAID 10 或 RAID 6 配置存储多路径。启用存储 DRS 实现负载均衡。定期验证存储复制链路健康状态。

## 灾备策略最佳实践

1. **3-2-1 备份原则**：至少 3 份数据副本、2 种不同存储介质、1 份异地保存
2. **自动化优先**：所有恢复流程尽可能脚本化，减少人为操作失误
3. **定期演练**：每季度至少一次部分故障切换演练，年度一次全量演练
4. **文档先行**：维护完整的灾备操作手册、联系人清单和系统架构图
5. **持续验证**：备份完成后自动验证数据可恢复性，而非等到灾难发生时才发现备份损坏

## 安全最佳实践

- 最小权限原则分配 vSphere 权限，定期审查用户角色
- 启用多因素认证（MFA），会话超时设置不超过 30 分钟
- 所有站点间通信使用 TLS 1.2+ 加密
- 存储复制启用 AES-256 加密
- 日志保留 90 天以上，接入 SIEM 系统

---

<!-- chunk: 故障排查 -->## 故障排查

## 常见问题诊断

```bash
#!/bin/bash
# vSphere 灾备故障排查脚本

echo "=== vSphere 灾备故障诊断 ==="

# 1. 检查 vCenter 服务状态
echo "[1] vCenter 服务状态"
ssh vcsa@vcenter.company.com "
    service-control --status --all
    echo '---'
    vmware-vpxd -v 2>/dev/null
    df -h /storage/seat
"

# 2. 检查 ESXi 主机连通性
echo "[2] ESXi 主机连通性"
for host in esxi01 esxi02 esxi03; do
    ping -c 2 -W 2 $host.company.com && echo "$host: OK" || echo "$host: FAILED"
done

# 3. 检查存储复制状态
echo "[3] 存储复制状态"
ssh storage-admin@192.168.20.10 "
    show replication-status --all
    show replication-errors --last 24h
"

# 4. 检查 SRM 恢复计划状态
echo "[4] SRM 恢复计划状态"
srm-getconfig -s vcenter-dr.company.com
srm-list-plans --detail

# 5. 检查网络连通性（站点间）
echo "[5] 站点间网络连通性"
ping -c 5 -s 8972 192.168.30.10  # Jumbo frame test
traceroute -n 192.168.30.10

# 6. 性能瓶颈分析
echo "[6] 性能瓶颈分析"
govc metric.sample -n 10 -t host/*/cpu/usage.average
govc metric.sample -n 10 -t host/*/mem/usage.average
govc metric.sample -n 10 -t host/*/storage/latency.write.average
govc metric.sample -n 10 -t host/*/net/throughput.contention.average
```

## 故障排查手册

| 问题现象 | 可能原因 | 排查步骤 | 解决方案 |
|:---|:---|:---|:---|
| 复制延迟持续增长 | 网络带宽不足 | 检查站点间带宽利用率 | 增加带宽或调整复制计划 |
| SRM 故障切换失败 | 存储卷未正确挂载 | 检查灾备端存储卷状态 | 手动挂载后重试 |
| 虚拟机恢复后无法启动 | IP 地址冲突 | 检查灾备网络 IP 分配 | SRM IP 自定义规则修正 |
| 备份作业超时 | 存储性能下降 | 检查存储 IOPS 和延迟 | 优化备份窗口或存储扩容 |
| vMotion 失败 | 网络配置不匹配 | 检查 vMotion 网络配置 | 确保 vMotion 网络互通 |
| 快照合并卡住 | 快照链过长 | 检查快照数量和大小 | 定期删除旧快照 |

---

**文档版本**: v2.0  
**最后更新**: 2026-05-18  
**适用版本**: vSphere 7.0 / 8.0+

---

<!-- chunk: VMware Site Recovery Manager 深度实践 -->## VMware Site Recovery Manager 深度实践

## SRM 恢复计划详解

VMware Site Recovery Manager（SRM）是 vSphere 生态中最核心的灾备自动化工具。它通过与 vCenter Server 和存储阵列复制紧密集成，提供一键式的灾难恢复能力。SRM 的核心概念包括保护组（Protection Group）、恢复计划（Recovery Plan）和恢复步骤（Recovery Steps）。

保护组定义了一组需要一起保护的虚拟机和它们的存储复制对。一个恢复计划可以包含一个或多个保护组，并定义了这些虚拟机在灾备站点的启动顺序、网络配置和自定义操作。SRM 支持两种故障切换模式：计划内迁移（Planned Migration）用于维护窗口期间的有计划切换，无计划故障切换（Unplanned Failover）用于真正的灾难场景。

恢复计划的核心设计原则是分层恢复。第一层恢复关键数据库，等待数据库完全启动后，第二层启动应用服务器并验证数据库连接，第三层启动前端 Web 服务并验证端到端功能。每层之间可以插入自定义脚本，执行健康检查、DNS 更新或通知等操作。

```yaml
# SRM 恢复计划完整配置
srm_recovery_plan_detailed:
  name: "Enterprise-Critical-DR-Plan"
  version: "2.0"
  last_tested: "2026-04-15"
  
  protection_groups:
    - name: "PG-Database"
      type: "Array-Based Replication"
      storage_array: "Dell EMC PowerMax"
      replication_mode: "synchronous"
      rpo: "0"
      vms:
        - "MySQL-Primary"
        - "MySQL-Replica-01"
        - "MySQL-Replica-02"
        - "Redis-Cluster"
        - "MongoDB-Primary"
        
    - name: "PG-Application"
      type: "Array-Based Replication"
      storage_array: "Dell EMC PowerMax"
      replication_mode: "synchronous"
      rpo: "0"
      vms:
        - "API-Gateway"
        - "Order-Service"
        - "User-Service"
        - "Payment-Service"
        
    - name: "PG-Web"
      type: "vSphere Replication"
      replication_mode: "asynchronous"
      rpo: "15 分钟"
      vms:
        - "Nginx-Frontend-01"
        - "Nginx-Frontend-02"
        - "CDN-Proxy"
        
  recovery_steps:
    pre_failover:
      - name: "验证灾备站点就绪"
        type: "powerShell"
        script: "C:\\Scripts\\Verify-DRReadiness.ps1"
        timeout_minutes: 5
        
      - name: "确认存储复制一致性"
        type: "powerShell"
        script: "C:\\Scripts\\Verify-ReplicationConsistency.ps1"
        timeout_minutes: 3
        
    group_1_database:
      priority: 1
      wait_after_minutes: 5
      vms:
        - name: "MySQL-Primary"
          startup_action: "PowerOn"
          ip_customization:
            adapter: "Network adapter 1"
            ip: "192.168.30.50"
            subnet: "255.255.255.0"
            gateway: "192.168.30.1"
            dns: ["192.168.30.53", "192.168.30.54"]
          post_startup_script: "C:\\Scripts\\Verify-MySQLHealth.ps1"
          
        - name: "Redis-Cluster"
          startup_action: "PowerOn"
          ip_customization:
            ip: "192.168.30.60"
            
      post_group_script: "C:\\Scripts\\Verify-DatabaseLayer.ps1"
      
    group_2_application:
      priority: 2
      depends_on: "group_1_database"
      wait_after_minutes: 3
      vms:
        - name: "API-Gateway"
          startup_action: "PowerOn"
          ip_customization:
            ip: "192.168.30.70"
            
        - name: "Order-Service"
          startup_action: "PowerOn"
          ip_customization:
            ip: "192.168.30.71"
            
      post_group_script: "C:\\Scripts\\Verify-ApplicationLayer.ps1"
      
    group_3_web:
      priority: 3
      depends_on: "group_2_application"
      vms:
        - name: "Nginx-Frontend-01"
          startup_action: "PowerOn"
          ip_customization:
            ip: "192.168.30.80"
            
      post_group_script: "C:\\Scripts\\Verify-WebLayer.ps1"
      
    post_failover:
      - name: "更新 DNS 记录"
        type: "powerShell"
        script: "C:\\Scripts\\Update-DNSFailover.ps1"
        
      - name: "更新负载均衡器"
        type: "powerShell"
        script: "C:\\Scripts\\Update-LoadBalancer.ps1"
        
      - name: "发送通知"
        type: "email"
        recipients: ["dr-team@company.com", "management@company.com"]
        subject: "灾备切换完成通知"
```

## SRM 测试恢复

SRM 的测试恢复（Test Recovery）功能允许在不影响生产的情况下验证恢复计划的有效性。测试恢复会在灾备站点创建一个隔离的网络环境（使用 vApp 或 VLAN 隔离），启动恢复的虚拟机，执行所有恢复步骤，然后自动清理。

建议企业每月执行一次测试恢复，验证恢复计划的完整性和时效性。每次测试恢复后应生成测试报告，记录 RTO 实际测量值、发现的问题和改进建议。

```yaml
# SRM 测试恢复配置
srm_test_recovery:
  schedule: "每月第一个周六 03:00"
  network_isolation:
    method: "vApp Network Isolation"
    test_network: "DR-Test-VLAN-100"
    dhcp: true
    
  automation:
    auto_cleanup: true
    cleanup_timeout_minutes: 30
    generate_report: true
    report_recipients: ["dr-team@company.com"]
    
  success_criteria:
    - "所有虚拟机成功启动"
    - "RTO 实际值 < RTO 目标值"
    - "应用健康检查通过"
    - "数据完整性验证通过"
```

---

<!-- chunk: vSAN 拉伸集群配置 -->## vSAN 拉伸集群配置

## 同城双活 vSAN 架构

vSAN 拉伸集群（Stretched Cluster）是 vSphere 实现同城双活的核心技术。通过将 vSAN 集群横跨两个数据中心，配合见证主机（Witness Host）在第三个位置，实现数据同步复制和自动故障切换。当任一数据中心发生问题时，另一个数据中心的虚拟机无需手动干预即可继续运行。

vSAN 拉伸集群要求两个数据中心之间的网络延迟小于 5 毫秒（推荐小于 2 毫秒），带宽至少 10Gbps。见证主机不需要高性能，可以部署在第三个位置的轻量级虚拟机上，但必须与两个数据中心的网络都可达。

```yaml
# vSAN 拉伸集群配置
vsan_stretched_cluster:
  cluster_name: "vsan-stretched-cluster"
  
  preferred_fault_domain:
    name: "beijing-dc"
    hosts:
      - esxi-01.beijing
      - esxi-02.beijing
      - esxi-03.beijing
    capacity: "60% 流量"
    
  secondary_fault_domain:
    name: "shanghai-dc"
    hosts:
      - esxi-04.shanghai
      - esxi-05.shanghai
      - esxi-06.shanghai
    capacity: "40% 流量"
    
  witness:
    host: "witness.guangzhou"
    ip: "192.168.50.10"
    vsan_network: "192.168.50.0/24"
    
  storage_policy:
    failures_to_tolerate: "RAID-1 (Mirroring)"
    fault_domains_to_tolerate: 1
    object_space_reservation: "100%"
    checksum: "Enabled"
    
  network_requirements:
    inter_site_bandwidth: "10Gbps+"
    max_latency_ms: 5
    mtu: 9000
    vsan_traffic:
      preferred_to_secondary: "10.0.0.0/24"
      witness_network: "10.0.1.0/24"
```

## vSAN 运维脚本

```bash
#!/bin/bash
# vSAN 拉伸集群运维脚本

echo "=== vSAN 拉伸集群健康检查 ==="

# 1. 检查 vSAN 集群状态
echo "[1] vSAN 集群状态"
govc cluster.info -json vsan-stretched-cluster | jq '.Cluster[].Runtime'

# 2. 检查故障域
echo "[2] 故障域状态"
govc object.collect -s /Datacenter/host/vsan-stretched-cluster cluster.configuration.vsanFaultDomainsConfig

# 3. 检查见证主机连通性
echo "[3] 见证主机连通性"
ping -c 3 -W 2 192.168.50.10

# 4. 检查 vSAN 磁盘健康
echo "[4] vSAN 磁盘健康"
govc host.vsan.disk.list

# 5. 检查组件同步状态
echo "[5] 组件同步状态"
govc object.collect -s /Datacenter/host/vsan-stretched-cluster cluster.configuration.vsanConfig

# 6. 性能监控
echo "[6] vSAN 性能指标"
govc metric.sample -n 10 -t cluster/*/vsan/perf.*
```

---

<!-- chunk: vSphere 安全加固 -->## vSphere 安全加固

## 安全基线配置

企业级 vSphere 环境应遵循 VMware 安全加固指南（Security Hardening Guide），从管理平面、控制平面和数据平面三个维度进行安全加固。

```yaml
# vSphere 安全加固配置
vsphere_security_hardening:
  management_plane:
    vcenter:
      tls_min_version: "1.2"
      certificate:
        key_size: 4096
        algorithm: "RSA"
        validity_days: 365
        
    sso:
      password_policy:
        min_length: 15
        complexity: "upper+lower+digit+special"
        max_age_days: 90
        history: 5
        
      lockout_policy:
        max_attempts: 3
        unlock_time_minutes: 30
        
      session_timeout:
        web_client: 30
        api: 20
        shell: 10
        
  control_plane:
    esxi:
      shell:
        enabled: false
        timeout_seconds: 600
        
      firewall:
        default_action: "deny"
        allowed_services:
          - "22 (SSH - 限制 IP)"
          - "443 (vSphere Client)"
          - "902 (vCenter Agent)"
          
      advanced_settings:
        "/UserVars/ESXiShellInteractiveTimeOut": 600
        "/Security/PasswordQualityControl": "retry=3 min=disabled,disabled,disabled,14,14"
        "/Net/GuestIPHack": 0
        
  data_plane:
    vm_security:
      - "启用 vTPM（虚拟可信平台模块）"
      - "启用 Secure Boot"
      - "禁用不必要的虚拟设备（软驱、串口）"
      - "启用 VM Encryption（存储加密）"
```

## 审计与合规

```yaml
# vSphere 审计配置
vsphere_audit:
  logging:
    syslog_server: "siem.company.com"
    syslog_port: 514
    syslog_protocol: "TLS"
    log_retention_days: 365
    
  events:
    critical_events:
      - "UserLoginSessionEvent"
      - "TaskEvent (VM.PowerOff, VM.Delete)"
      - "PermissionUpdatedEvent"
      - "RoleAddedEvent"
      - "EntityModifiedEvent"
      
  compliance:
    reports:
      - name: "vSphere 安全基线检查"
        schedule: "每月"
        tool: "vSphere Security Configuration Guide"
        
      - name: "CIS Benchmark 检查"
        schedule: "每季度"
        tool: "CIS VMware vSphere Benchmark"
```

---

<!-- chunk: 容量规划与性能优化 -->## 容量规划与性能优化

## vSphere 容量管理

企业级 vSphere 环境需要建立系统性的容量管理流程，定期预测资源需求，避免资源瓶颈影响业务。

```yaml
# vSphere 容量管理策略
capacity_management:
  monitoring:
    metrics:
      - name: "CPU 利用率"
        warning: "> 70%"
        critical: "> 85%"
        
      - name: "内存利用率"
        warning: "> 75%"
        critical: "> 90%"
        
      - name: "存储利用率"
        warning: "> 80%"
        critical: "> 90%"
        
      - name: "网络带宽"
        warning: "> 70%"
        critical: "> 85%"
        
  forecasting:
    method: "线性回归 + 季节性调整"
    growth_rate_source: "过去 6 个月历史数据"
    forecast_horizon: "12 个月"
    buffer: "20% 预留"
    
  recommendations:
    - "预留 20% 缓冲空间应对突发增长"
    - "每季度审查容量预测报告"
    - "关注 vMotion 和 Storage vMotion 对性能的影响"
    - "监控虚拟机密度（每主机 VM 数）避免过度整合"
```

---

**文档版本**: v2.0  
**最后更新**: 2026-05-18  
**适用版本**: vSphere 7.0 / 8.0+

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-30-disaster-recovery-business-continuity KUDIG Database — Global MOC
- [[可靠性/README.md|Domain 09: 企业级灾备与业务连续性 (Enterprise [[Kubernetes 灾难恢复最佳实践|Disaster Recovery]] & Busin...]]
- index.md|Domain-30 灾备与业务连续性 — 开源项目索引]]
- Veeam Backup & Replication 企业级备份恢复解决方案
- 企业级容灾架构与混沌工程深度实践
- Commvault 企业级灾备与业务连续性深度实践
- Rubrik 企业级灾备与业务连续性深度实践
- Kubernetes 备份与恢复深度实践
- 混沌工程平台实践：LitmusChaos 与 Chaos Mesh
- 应用级灾备架构：多区域部署与故障转移
- Velero 企业级备份恢复实践指南

## See Also

- 09-application-level-disaster-recovery
- 99-velero-backup-recovery-guide
- 02-veeam-enterprise-backup
- 03-enterprise-disaster-recovery-chaos-engineering

## Related

- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]


<!-- risk-assessed -->
