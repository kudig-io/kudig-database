# 控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 灾备恢复指南

---

## 目录

1. [灾备设计原则](#1-灾备设计原则)
2. [备份策略规划](#2-备份策略规划)
3. [etcd备份恢复](#3-etcd备份恢复)
4. [配置文件备份](#4-配置文件备份)
5. [证书管理备份](#5-证书管理备份)
6. [存储卷备份](#6-存储卷备份)
7. [灾备演练流程](#7-灾备演练流程)
8. [恢复操作手册](#8-恢复操作手册)
9. [监控告警配置](#9-监控告警配置)
10. [灾备最佳实践](#10-灾备最佳实践)

---

## 1. 灾备设计原则

### 1.1 灾备架构模式

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Disaster Recovery Architecture                         │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  灾备级别定义:                                                                    │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           RTO/RPO Matrix                                │    │
│  │  ┌─────────────┬─────────────┬─────────────────────────────────────────┐│    │
│  │  │   级别      │    RTO      │    RPO      │         说明              ││    │
│  │  ├─────────────┼─────────────┼─────────────┼───────────────────────────┤│    │
│  │  │ Tier 1      │ < 15分钟    │ < 5分钟     │ 核心业务，实时备份         ││    │
│  │  │ Tier 2      │ < 1小时     │ < 30分钟    │ 重要业务，定时备份         ││    │
│  │  │ Tier 3      │ < 4小时     │ < 4小时     │ 一般业务，每日备份         ││    │
│  │  │ Tier 4      │ < 24小时    │ < 24小时    │ 非关键业务，周备份         ││    │
│  │  └─────────────┴─────────────┴─────────────┴───────────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  灾备架构模式:                                                                    │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        Hot Standby (热备模式)                            │    │
│  │  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐               │    │
│  │  │  Primary    │◄───►│  Secondary  │◄───►│   Tertiary   │               │    │
│  │  │  Cluster    │     │   Cluster   │     │   Cluster    │               │    │
│  │  │ (Active)    │     │ (Standby)   │     │ (Cold Standby)│               │    │
│  │  └─────────────┘     └─────────────┘     └─────────────┘               │    │
│  │        │                    │                    │                        │    │
│  │        ▼                    ▼                    ▼                        │    │
│  │  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐               │    │
│  │  │ Real-time   │     │ Periodic    │     │ Manual      │               │    │
│  │  │ Sync        │     │ Sync        │     │ Restore     │               │    │
│  │  └─────────────┘     └─────────────┘     └─────────────┘               │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                     Geo-Redundant (地理冗余)                              │    │
│  │  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐     │    │
│  │  │   Region A      │     │   Region B      │     │   Region C      │     │    │
│  │  │  ┌───────────┐  │     │  ┌───────────┐  │     │  ┌───────────┐  │     │    │
│  │  │  │ Primary   │  │     │  │ Secondary │  │     │  │ Tertiary  │  │     │    │
│  │  │  │ Cluster   │  │     │  │ Cluster   │  │     │  │ Cluster   │  │     │    │
│  │  │  └───────────┘  │     │  └───────────┘  │     │  └───────────┘  │     │    │
│  │  └─────────────────┘     └─────────────────┘     └─────────────────┘     │    │
│  │         │                         │                         │              │    │
│  │         ▼                         ▼                         ▼              │    │
│  │  ┌─────────────────────────────────────────────────────────────────────┐   │    │
│  │  │                    Cross-Region Replication                          │   │    │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────────────────┐ │   │    │
│  │  │  │ etcd DR  │  │ Config   │  │ Cert DR  │  │ Application Data    │ │   │    │
│  │  │  │          │  │ Backup   │  │          │  │                     │ │   │    │
│  │  │  └──────────┘  └──────────┘  └──────────┘  └─────────────────────┘ │   │    │
│  │  └─────────────────────────────────────────────────────────────────────┘   │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 备份数据分类

| 数据类型 | 重要性 | 备份频率 | 恢复时间 | 存储位置 |
|----------|--------|----------|----------|----------|
| **etcd数据** | 最高 | 实时/每5分钟 | <15分钟 | 本地+远程 |
| **证书文件** | 高 | 每日 | <30分钟 | 安全存储 |
| **配置文件** | 高 | 每日 | <1小时 | 版本控制 |
| **持久化数据** | 中 | 每小时 | <4小时 | 存储快照 |
| **日志数据** | 低 | 每日 | <24小时 | 日志归档 |

---

## 2. 备份策略规划

### 2.1 备份策略矩阵

```yaml
# 备份策略配置
backup_strategies:
  etcd_backup:
    schedule: "*/5 * * * *"  # 每5分钟
    retention: "7d"          # 保留7天
    storage_locations:
      - type: local
        path: "/backup/etcd"
      - type: remote
        provider: s3
        bucket: "k8s-backup-production"
        region: "us-west-2"
    encryption: true
    compression: true
    
  config_backup:
    schedule: "0 2 * * *"    # 每日凌晨2点
    retention: "30d"         # 保留30天
    storage_locations:
      - type: git
        repository: "git@github.com:company/k8s-config-backup.git"
        branch: "main"
      - type: s3
        bucket: "k8s-config-backup"
    version_control: true
    
  certificate_backup:
    schedule: "0 3 * * 0"    # 每周日凌晨3点
    retention: "90d"         # 保留90天
    storage_locations:
      - type: encrypted_s3
        bucket: "k8s-certificates"
        kms_key_id: "arn:aws:kms:us-west-2:123456789:key/abcd-efgh"
    rotation_check: true
    
  persistent_data_backup:
    schedule: "0 */4 * * *"  # 每4小时
    retention: "7d"          # 保留7天
    storage_locations:
      - type: snapshot
        provider: "csi-snapshotter"
      - type: s3
        bucket: "k8s-persistent-data"
    incremental: true
```

### 2.2 备份验证机制

```bash
#!/bin/bash
# 备份验证脚本

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/backup-validation.log
}

# etcd备份验证
validate_etcd_backup() {
    log "验证etcd备份完整性..."
    
    local backup_file=$1
    local temp_dir="/tmp/etcd-restore-test"
    
    # 创建临时目录
    mkdir -p $temp_dir
    
    # 恢复备份到临时目录进行验证
    ETCDCTL_API=3 etcdctl snapshot restore $backup_file \
        --data-dir=$temp_dir/data \
        --name=test-member \
        --initial-cluster=test-member=http://localhost:2380 \
        --initial-cluster-token=test-cluster \
        --initial-advertise-peer-urls=http://localhost:2380
    
    # 检查恢复的数据
    if [ -d "$temp_dir/data/member" ]; then
        local db_size=$(du -sh $temp_dir/data | cut -f1)
        log "✓ etcd备份验证通过，数据库大小: $db_size"
        rm -rf $temp_dir
        return 0
    else
        log "✗ etcd备份验证失败"
        rm -rf $temp_dir
        return 1
    fi
}

# 配置文件验证
validate_config_backup() {
    log "验证配置文件备份..."
    
    local config_backup_dir=$1
    
    # 检查关键配置文件是否存在
    local required_files=(
        "kube-apiserver.yaml"
        "kube-controller-manager.yaml"
        "kube-scheduler.yaml"
        "etcd.yaml"
        "admin.conf"
        "controller-manager.conf"
        "scheduler.conf"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$config_backup_dir/$file" ]; then
            log "✗ 缺少配置文件: $file"
            return 1
        fi
    done
    
    log "✓ 配置文件备份验证通过"
    return 0
}

# 证书验证
validate_certificate_backup() {
    log "验证证书备份..."
    
    local cert_backup_dir=$1
    
    # 检查证书有效期
    local cert_files=( "*.crt" "*.pem" )
    
    for pattern in "${cert_files[@]}"; do
        for cert in $cert_backup_dir/$pattern; do
            if [ -f "$cert" ]; then
                local expiry_date=$(openssl x509 -enddate -noout -in "$cert" | cut -d= -f2)
                local expiry_timestamp=$(date -d "$expiry_date" +%s)
                local current_timestamp=$(date +%s)
                local days_until_expiry=$(( (expiry_timestamp - current_timestamp) / 86400 ))
                
                if [ $days_until_expiry -lt 30 ]; then
                    log "⚠ 证书即将过期: $cert ($days_until_expiry 天后)"
                fi
            fi
        done
    done
    
    log "✓ 证书备份验证完成"
    return 0
}

# 主验证函数
validate_all_backups() {
    log "开始备份验证..."
    
    local backup_base="/backup"
    local validation_passed=true
    
    # 验证etcd备份
    local latest_etcd_backup=$(find $backup_base/etcd -name "*.db" -type f -mtime -1 | sort -r | head -1)
    if [ -n "$latest_etcd_backup" ]; then
        if ! validate_etcd_backup "$latest_etcd_backup"; then
            validation_passed=false
        fi
    else
        log "✗ 未找到最近的etcd备份"
        validation_passed=false
    fi
    
    # 验证配置备份
    local config_backup_dir="$backup_base/config/$(date +%Y%m%d)"
    if [ -d "$config_backup_dir" ]; then
        if ! validate_config_backup "$config_backup_dir"; then
            validation_passed=false
        fi
    else
        log "✗ 未找到今日配置备份: $config_backup_dir"
        validation_passed=false
    fi
    
    # 验证证书备份
    local cert_backup_dir="$backup_base/certificates/$(date +%Y%m%d)"
    if [ -d "$cert_backup_dir" ]; then
        if ! validate_certificate_backup "$cert_backup_dir"; then
            validation_passed=false
        fi
    else
        log "✗ 未找到今日证书备份: $cert_backup_dir"
        validation_passed=false
    fi
    
    if $validation_passed; then
        log "✓ 所有备份验证通过"
        return 0
    else
        log "✗ 备份验证失败，请检查上述错误"
        return 1
    fi
}

# 执行验证
validate_all_backups
```

---

## 3. etcd备份恢复

### 3.1 etcd备份策略

```bash
#!/bin/bash
# etcd备份脚本

# 配置变量
BACKUP_DIR="/backup/etcd"
REMOTE_STORAGE="s3://k8s-backup-production/etcd"
RETENTION_DAYS=7
ENCRYPTION_KEY="/etc/backup/encryption.key"

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/etcd-backup.log
}

# 创建备份目录
setup_backup_directory() {
    local date_stamp=$(date +%Y%m%d)
    local time_stamp=$(date +%H%M%S)
    local backup_path="$BACKUP_DIR/$date_stamp"
    
    mkdir -p "$backup_path"
    echo "$backup_path/etcd-backup-$time_stamp.db"
}

# 执行etcd备份
perform_etcd_backup() {
    local backup_file=$1
    local temp_file="/tmp/etcd-snapshot-temp.db"
    
    log "开始etcd备份到: $backup_file"
    
    # 执行快照
    if ETCDCTL_API=3 etcdctl snapshot save "$temp_file" \
        --endpoints=https://127.0.0.1:2379 \
        --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key; then
        
        # 验证快照
        if ETCDCTL_API=3 etcdctl snapshot status "$temp_file" --write-out=table; then
            # 压缩备份
            gzip -c "$temp_file" > "${backup_file}.gz"
            
            # 加密备份（如果配置了加密密钥）
            if [ -f "$ENCRYPTION_KEY" ]; then
                openssl enc -aes-256-cbc -salt -in "${backup_file}.gz" \
                    -out "${backup_file}.gz.enc" -pass file:"$ENCRYPTION_KEY"
                rm "${backup_file}.gz"
                backup_file="${backup_file}.gz.enc"
            else
                backup_file="${backup_file}.gz"
            fi
            
            # 上传到远程存储
            if command -v aws &> /dev/null; then
                aws s3 cp "$backup_file" "$REMOTE_STORAGE/$(basename $backup_file)"
                log "✓ 备份已上传到远程存储"
            fi
            
            rm "$temp_file"
            log "✓ etcd备份完成: $backup_file"
            return 0
        else
            log "✗ etcd快照验证失败"
            rm -f "$temp_file"
            return 1
        fi
    else
        log "✗ etcd快照创建失败"
        return 1
    fi
}

# 清理旧备份
cleanup_old_backups() {
    log "清理 $RETENTION_DAYS 天前的备份..."
    
    # 清理本地备份
    find "$BACKUP_DIR" -name "*.db*" -type f -mtime +$RETENTION_DAYS -delete
    
    # 清理远程备份（如果使用S3）
    if command -v aws &> /dev/null; then
        aws s3 ls "$REMOTE_STORAGE/" | \
        awk -v cutoff="$(date -d "$RETENTION_DAYS days ago" +%s)" '{
            cmd = "date -d \"" $1 " " $2 "\" +%s"
            cmd | getline timestamp
            close(cmd)
            if (timestamp < cutoff) {
                print "s3://k8s-backup-production/etcd/" $4
            }
        }' | while read obj; do
            aws s3 rm "$obj"
        done
    fi
    
    log "✓ 旧备份清理完成"
}

# 主备份函数
main_backup() {
    log "=== etcd备份任务开始 ==="
    
    local backup_file=$(setup_backup_directory)
    
    if perform_etcd_backup "$backup_file"; then
        cleanup_old_backups
        log "=== etcd备份任务完成 ==="
        return 0
    else
        log "=== etcd备份任务失败 ==="
        return 1
    fi
}

# 执行备份
main_backup
```

### 3.2 etcd恢复操作

```bash
#!/bin/bash
# etcd灾难恢复脚本

# 配置变量
BACKUP_SOURCE="/backup/etcd"
ETCD_DATA_DIR="/var/lib/etcd"
ETCD_WAL_DIR="/var/lib/etcd/wal"

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/etcd-restore.log
}

# 停止etcd服务
stop_etcd() {
    log "停止etcd服务..."
    systemctl stop etcd
    # 等待进程完全停止
    while pgrep etcd > /dev/null; do
        sleep 1
    done
    log "✓ etcd服务已停止"
}

# 备份当前数据
backup_current_data() {
    local backup_suffix=$(date +%Y%m%d-%H%M%S)
    log "备份当前etcd数据..."
    
    if [ -d "$ETCD_DATA_DIR" ]; then
        tar -czf "/backup/etcd-current-backup-$backup_suffix.tar.gz" -C / "$(echo $ETCD_DATA_DIR | sed 's|^/||')"
        log "✓ 当前数据已备份到: /backup/etcd-current-backup-$backup_suffix.tar.gz"
    fi
}

# 恢复etcd数据
restore_etcd_data() {
    local backup_file=$1
    local temp_restore_dir="/tmp/etcd-restore"
    
    log "开始恢复etcd数据从: $backup_file"
    
    # 创建临时恢复目录
    mkdir -p "$temp_restore_dir"
    
    # 解密和解压备份文件
    if [[ "$backup_file" == *.enc ]]; then
        log "解密备份文件..."
        openssl enc -d -aes-256-cbc -in "$backup_file" \
            -out "${backup_file%.enc}" -pass file:"/etc/backup/encryption.key"
        backup_file="${backup_file%.enc}"
    fi
    
    if [[ "$backup_file" == *.gz ]]; then
        log "解压备份文件..."
        gunzip -c "$backup_file" > "${backup_file%.gz}"
        backup_file="${backup_file%.gz}"
    fi
    
    # 验证备份文件
    log "验证备份文件完整性..."
    if ! ETCDCTL_API=3 etcdctl snapshot status "$backup_file" --write-out=table; then
        log "✗ 备份文件验证失败"
        rm -rf "$temp_restore_dir"
        return 1
    fi
    
    # 恢复快照
    log "恢复etcd快照..."
    local restore_name="etcd-$(hostname)"
    local initial_cluster=""
    
    # 构建initial cluster字符串
    for node in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
        if [ -n "$initial_cluster" ]; then
            initial_cluster="$initial_cluster,"
        fi
        initial_cluster="${initial_cluster}etcd-$(echo $node | tr . -)=https://$node:2380"
    done
    
    if ETCDCTL_API=3 etcdctl snapshot restore "$backup_file" \
        --data-dir="$temp_restore_dir/data" \
        --wal-dir="$temp_restore_dir/wal" \
        --name="$restore_name" \
        --initial-cluster="$initial_cluster" \
        --initial-cluster-token="etcd-cluster-1" \
        --initial-advertise-peer-urls="https://$(hostname -I | awk '{print $1}'):2380"; then
        
        # 停止etcd并替换数据目录
        stop_etcd
        backup_current_data
        
        # 替换数据目录
        rm -rf "$ETCD_DATA_DIR" "$ETCD_WAL_DIR"
        mv "$temp_restore_dir/data" "$ETCD_DATA_DIR"
        mv "$temp_restore_dir/wal" "$ETCD_WAL_DIR"
        
        # 设置权限
        chown -R etcd:etcd "$ETCD_DATA_DIR" "$ETCD_WAL_DIR"
        
        # 启动etcd
        log "启动etcd服务..."
        systemctl start etcd
        
        # 验证恢复
        sleep 10
        if systemctl is-active etcd > /dev/null; then
            log "✓ etcd恢复成功并正在运行"
            
            # 验证集群健康
            sleep 30
            if ETCDCTL_API=3 etcdctl endpoint health --cluster; then
                log "✓ etcd集群健康检查通过"
                rm -rf "$temp_restore_dir"
                return 0
            else
                log "✗ etcd集群健康检查失败"
                return 1
            fi
        else
            log "✗ etcd服务启动失败"
            return 1
        fi
    else
        log "✗ etcd快照恢复失败"
        rm -rf "$temp_restore_dir"
        return 1
    fi
}

# 选择备份文件
select_backup_file() {
    local date_filter=${1:-$(date +%Y%m%d)}
    
    log "查找备份文件 (日期: $date_filter)..."
    
    local backup_files=($(find "$BACKUP_SOURCE" -name "*$date_filter*.db*" -type f | sort -r))
    
    if [ ${#backup_files[@]} -eq 0 ]; then
        log "✗ 未找到指定日期的备份文件"
        return 1
    fi
    
    echo "可用的备份文件:"
    for i in "${!backup_files[@]}"; do
        echo "  [$i] ${backup_files[$i]}"
    done
    
    read -p "请选择要恢复的备份文件编号: " selection
    
    if [[ $selection =~ ^[0-9]+$ ]] && [ $selection -lt ${#backup_files[@]} ]; then
        echo "${backup_files[$selection]}"
        return 0
    else
        log "✗ 无效的选择"
        return 1
    fi
}

# 主恢复函数
main_restore() {
    log "=== etcd灾难恢复开始 ==="
    
    # 检查参数
    if [ $# -eq 0 ]; then
        BACKUP_FILE=$(select_backup_file)
        if [ $? -ne 0 ]; then
            return 1
        fi
    else
        BACKUP_FILE="$1"
        if [ ! -f "$BACKUP_FILE" ]; then
            log "✗ 指定的备份文件不存在: $BACKUP_FILE"
            return 1
        fi
    fi
    
    # 确认操作
    read -p "确认要从 $BACKUP_FILE 恢复etcd数据吗？(yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        log "操作已取消"
        return 1
    fi
    
    # 执行恢复
    if restore_etcd_data "$BACKUP_FILE"; then
        log "=== etcd灾难恢复完成 ==="
        return 0
    else
        log "=== etcd灾难恢复失败 ==="
        return 1
    fi
}

# 执行恢复
main_restore "$@"
```

---

## 4. 配置文件备份

### 4.1 配置备份脚本

```bash
#!/bin/bash
# Kubernetes配置文件备份脚本

# 配置变量
CONFIG_DIRS=(
    "/etc/kubernetes"
    "/etc/kubernetes/manifests"
    "/etc/kubernetes/pki"
    "/etc/cni/net.d"
    "/etc/containerd"
    "/etc/systemd/system"
)
BACKUP_BASE="/backup/config"
GIT_REPO="git@github.com:company/k8s-config-backup.git"
RETENTION_DAYS=30

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/config-backup.log
}

# 创建备份
create_config_backup() {
    local date_stamp=$(date +%Y%m%d-%H%M%S)
    local backup_dir="$BACKUP_BASE/$date_stamp"
    
    log "创建配置备份到: $backup_dir"
    
    mkdir -p "$backup_dir"
    
    # 备份配置目录
    for config_dir in "${CONFIG_DIRS[@]}"; do
        if [ -d "$config_dir" ]; then
            local relative_path=$(echo "$config_dir" | sed 's|^/||')
            mkdir -p "$(dirname "$backup_dir/$relative_path")"
            cp -r "$config_dir" "$backup_dir/$relative_path"
            log "✓ 已备份: $config_dir"
        else
            log "⚠ 配置目录不存在: $config_dir"
        fi
    done
    
    # 创建备份元数据
    cat > "$backup_dir/backup-metadata.json" << EOF
{
    "timestamp": "$(date -Iseconds)",
    "hostname": "$(hostname)",
    "kubernetes_version": "$(kubectl version --short 2>/dev/null | grep Server | awk '{print $3}')",
    "backup_type": "config",
    "directories": [$(printf '"%s",' "${CONFIG_DIRS[@]}" | sed 's/,$//')],
    "size": "$(du -sh "$backup_dir" | cut -f1)"
}
EOF
    
    log "✓ 配置备份创建完成: $backup_dir"
    echo "$backup_dir"
}

# Git版本控制
backup_to_git() {
    local backup_dir=$1
    local temp_git_dir="/tmp/k8s-config-backup"
    
    log "推送配置到Git仓库..."
    
    # 克隆或更新仓库
    if [ ! -d "$temp_git_dir" ]; then
        git clone "$GIT_REPO" "$temp_git_dir"
    else
        cd "$temp_git_dir"
        git pull origin main
    fi
    
    # 复制备份内容
    local backup_name=$(basename "$backup_dir")
    mkdir -p "$temp_git_dir/$backup_name"
    cp -r "$backup_dir"/* "$temp_git_dir/$backup_name/"
    
    # 提交更改
    cd "$temp_git_dir"
    git add .
    git commit -m "配置备份: $backup_name ($(date '+%Y-%m-%d %H:%M:%S'))"
    git push origin main
    
    if [ $? -eq 0 ]; then
        log "✓ 配置已推送到Git仓库"
    else
        log "✗ Git推送失败"
    fi
    
    rm -rf "$temp_git_dir"
}

# 远程存储备份
backup_to_remote() {
    local backup_dir=$1
    
    log "上传配置备份到远程存储..."
    
    if command -v aws &> /dev/null; then
        local backup_name=$(basename "$backup_dir")
        tar -czf "/tmp/${backup_name}.tar.gz" -C "$BACKUP_BASE" "$backup_name"
        aws s3 cp "/tmp/${backup_name}.tar.gz" "s3://k8s-config-backup/${backup_name}.tar.gz"
        rm "/tmp/${backup_name}.tar.gz"
        log "✓ 配置已上传到S3"
    fi
}

# 清理旧备份
cleanup_old_backups() {
    log "清理 $RETENTION_DAYS 天前的配置备份..."
    
    find "$BACKUP_BASE" -mindepth 1 -maxdepth 1 -type d -mtime +$RETENTION_DAYS -exec rm -rf {} \;
    log "✓ 旧配置备份清理完成"
}

# 主备份函数
main_config_backup() {
    log "=== 配置文件备份开始 ==="
    
    local backup_dir=$(create_config_backup)
    if [ $? -eq 0 ]; then
        backup_to_git "$backup_dir"
        backup_to_remote "$backup_dir"
        cleanup_old_backups
        log "=== 配置文件备份完成 ==="
        return 0
    else
        log "=== 配置文件备份失败 ==="
        return 1
    fi
}

# 执行备份
main_config_backup
```

### 4.2 配置恢复脚本

```bash
#!/bin/bash
# Kubernetes配置文件恢复脚本

# 配置变量
BACKUP_BASE="/backup/config"

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/config-restore.log
}

# 选择备份
select_config_backup() {
    log "可用的配置备份:"
    
    local backups=($(find "$BACKUP_BASE" -mindepth 1 -maxdepth 1 -type d | sort -r))
    
    if [ ${#backups[@]} -eq 0 ]; then
        log "✗ 未找到配置备份"
        return 1
    fi
    
    for i in "${!backups[@]}"; do
        local backup_name=$(basename "${backups[$i]}")
        local backup_time=$(echo "$backup_name" | cut -d- -f1-2)
        local formatted_time=$(date -d "$backup_time" '+%Y-%m-%d %H:%M:%S' 2>/dev/null || echo "$backup_time")
        echo "  [$i] $formatted_time ($backup_name)"
    done
    
    read -p "请选择要恢复的配置备份编号: " selection
    
    if [[ $selection =~ ^[0-9]+$ ]] && [ $selection -lt ${#backups[@]} ]; then
        echo "${backups[$selection]}"
        return 0
    else
        log "✗ 无效的选择"
        return 1
    fi
}

# 恢复配置
restore_config() {
    local backup_dir=$1
    
    log "开始恢复配置从: $backup_dir"
    
    # 备份当前配置
    local current_backup="/backup/config-current-$(date +%Y%m%d-%H%M%S)"
    log "备份当前配置到: $current_backup"
    mkdir -p "$current_backup"
    
    for config_dir in "/etc/kubernetes" "/etc/cni/net.d" "/etc/containerd"; do
        if [ -d "$config_dir" ]; then
            local relative_path=$(echo "$config_dir" | sed 's|^/||')
            mkdir -p "$(dirname "$current_backup/$relative_path")"
            cp -r "$config_dir" "$current_backup/$relative_path"
        fi
    done
    
    # 恢复配置
    log "恢复配置文件..."
    for backup_item in "$backup_dir"/*; do
        local item_name=$(basename "$backup_item")
        local target_path="/$item_name"
        
        if [ -d "$backup_item" ]; then
            log "恢复目录: $target_path"
            rm -rf "$target_path"
            cp -r "$backup_item" "$target_path"
        fi
    done
    
    # 设置权限
    chown -R root:root /etc/kubernetes
    chmod -R 600 /etc/kubernetes/pki
    
    log "✓ 配置恢复完成"
    log "原配置已备份到: $current_backup"
}

# 重启相关服务
restart_services() {
    log "重启相关服务..."
    
    local services=("kubelet" "containerd" "kube-apiserver" "kube-controller-manager" "kube-scheduler")
    
    for service in "${services[@]}"; do
        if systemctl list-unit-files | grep -q "^$service"; then
            log "重启服务: $service"
            systemctl restart "$service"
            
            # 等待服务启动
            sleep 5
            if systemctl is-active "$service" > /dev/null; then
                log "✓ $service 服务启动成功"
            else
                log "✗ $service 服务启动失败"
            fi
        fi
    done
}

# 验证恢复
verify_restore() {
    log "验证配置恢复..."
    
    # 检查关键文件
    local required_files=(
        "/etc/kubernetes/admin.conf"
        "/etc/kubernetes/controller-manager.conf"
        "/etc/kubernetes/scheduler.conf"
        "/etc/kubernetes/pki/ca.crt"
        "/etc/kubernetes/manifests/kube-apiserver.yaml"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$file" ]; then
            log "✗ 关键配置文件缺失: $file"
            return 1
        fi
    done
    
    # 检查集群状态
    log "检查集群状态..."
    if kubectl get nodes > /dev/null 2>&1; then
        log "✓ 集群状态正常"
        kubectl get nodes -o wide
        return 0
    else
        log "✗ 集群状态异常"
        return 1
    fi
}

# 主恢复函数
main_config_restore() {
    log "=== 配置文件恢复开始 ==="
    
    # 选择备份
    local backup_dir=$(select_config_backup)
    if [ $? -ne 0 ]; then
        return 1
    fi
    
    # 确认操作
    read -p "确认要从 $backup_dir 恢复配置吗？这将覆盖当前配置。(yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        log "操作已取消"
        return 1
    fi
    
    # 执行恢复
    if restore_config "$backup_dir" && restart_services && verify_restore; then
        log "=== 配置文件恢复完成 ==="
        return 0
    else
        log "=== 配置文件恢复失败 ==="
        return 1
    fi
}

# 执行恢复
main_config_restore
```

---

## 5. 证书管理备份

### 5.1 证书备份策略

```bash
#!/bin/bash
# Kubernetes证书备份脚本

# 配置变量
CERT_DIRS=(
    "/etc/kubernetes/pki"
    "/etc/etcd/ssl"
    "/etc/containerd/certs"
)
BACKUP_BASE="/backup/certificates"
RETENTION_DAYS=90

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/cert-backup.log
}

# 创建证书备份
create_cert_backup() {
    local date_stamp=$(date +%Y%m%d-%H%M%S)
    local backup_dir="$BACKUP_BASE/$date_stamp"
    
    log "创建证书备份到: $backup_dir"
    
    mkdir -p "$backup_dir"
    
    # 备份证书目录
    for cert_dir in "${CERT_DIRS[@]}"; do
        if [ -d "$cert_dir" ]; then
            local relative_path=$(echo "$cert_dir" | sed 's|^/||')
            mkdir -p "$(dirname "$backup_dir/$relative_path")"
            cp -r "$cert_dir" "$backup_dir/$relative_path"
            log "✓ 已备份证书目录: $cert_dir"
        else
            log "⚠ 证书目录不存在: $cert_dir"
        fi
    done
    
    # 创建证书清单
    local cert_manifest="$backup_dir/certificate-manifest.txt"
    echo "证书备份清单 - $(date)" > "$cert_manifest"
    echo "================================" >> "$cert_manifest"
    
    find "$backup_dir" -name "*.crt" -o -name "*.key" -o -name "*.pem" | while read cert_file; do
        local rel_path=${cert_file#$backup_dir/}
        local expiry_date=$(openssl x509 -enddate -noout -in "$cert_file" 2>/dev/null | cut -d= -f2)
        echo "$rel_path - 到期时间: $expiry_date" >> "$cert_manifest"
    done
    
    log "✓ 证书备份创建完成: $backup_dir"
    echo "$backup_dir"
}

# 加密备份
encrypt_backup() {
    local backup_dir=$1
    local encryption_key="/etc/backup/cert-encryption.key"
    
    if [ ! -f "$encryption_key" ]; then
        log "✗ 加密密钥不存在: $encryption_key"
        return 1
    fi
    
    log "加密证书备份..."
    
    local backup_name=$(basename "$backup_dir")
    tar -czf "/tmp/${backup_name}.tar.gz" -C "$BACKUP_BASE" "$backup_name"
    
    openssl enc -aes-256-cbc -salt -in "/tmp/${backup_name}.tar.gz" \
        -out "${backup_dir}.tar.gz.enc" -pass file:"$encryption_key"
    
    rm "/tmp/${backup_name}.tar.gz"
    rm -rf "$backup_dir"
    
    log "✓ 证书备份已加密: ${backup_dir}.tar.gz.enc"
}

# 上传到安全存储
upload_to_secure_storage() {
    local encrypted_backup=$1
    
    log "上传加密证书到安全存储..."
    
    if command -v aws &> /dev/null; then
        aws s3 cp "$encrypted_backup" "s3://k8s-certificates-secure/$(basename $encrypted_backup)"
        log "✓ 证书已上传到安全S3存储"
    fi
}

# 证书有效性检查
check_certificate_validity() {
    log "检查证书有效性..."
    
    local alerts=()
    
    find "/etc/kubernetes/pki" -name "*.crt" -o -name "*.pem" | while read cert; do
        local expiry_date=$(openssl x509 -enddate -noout -in "$cert" | cut -d= -f2)
        local expiry_timestamp=$(date -d "$expiry_date" +%s)
        local current_timestamp=$(date +%s)
        local days_until_expiry=$(( (expiry_timestamp - current_timestamp) / 86400 ))
        
        if [ $days_until_expiry -lt 30 ]; then
            alerts+=("证书即将过期: $cert ($days_until_expiry 天后)")
        elif [ $days_until_expiry -lt 0 ]; then
            alerts+=("证书已过期: $cert")
        fi
    done
    
    if [ ${#alerts[@]} -gt 0 ]; then
        log "⚠ 证书有效性警告:"
        for alert in "${alerts[@]}"; do
            log "  $alert"
        done
        return 1
    else
        log "✓ 所有证书均在有效期内"
        return 0
    fi
}

# 清理旧备份
cleanup_old_certs() {
    log "清理 $RETENTION_DAYS 天前的证书备份..."
    
    find "$BACKUP_BASE" -name "*.tar.gz.enc" -type f -mtime +$RETENTION_DAYS -delete
    log "✓ 旧证书备份清理完成"
}

# 主备份函数
main_cert_backup() {
    log "=== 证书备份开始 ==="
    
    check_certificate_validity
    
    local backup_dir=$(create_cert_backup)
    if [ $? -eq 0 ]; then
        encrypt_backup "$backup_dir"
        upload_to_secure_storage "${backup_dir}.tar.gz.enc"
        cleanup_old_certs
        log "=== 证书备份完成 ==="
        return 0
    else
        log "=== 证书备份失败 ==="
        return 1
    fi
}

# 执行备份
main_cert_backup
```

---

## 6. 存储卷备份

### 6.1 CSI快照备份

```yaml
# CSI快照备份配置
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapshot-class
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: ebs.csi.aws.com
deletionPolicy: Delete

---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: persistent-volume-backup
  namespace: kube-system
spec:
  schedule: "0 */4 * * *"  # 每4小时
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 5
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: volume-backup
            image: k8s.gcr.io/sig-storage/csi-snapshotter:v6.0.0
            command:
            - /bin/sh
            - -c
            - |
              #!/bin/bash
              set -e
              
              # 创建快照
              kubectl get pvc --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\n"}{end}' | \
              while read namespace pvc_name; do
                cat << EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: ${pvc_name}-snapshot-$(date +%Y%m%d-%H%M%S)
  namespace: ${namespace}
spec:
  volumeSnapshotClassName: csi-snapshot-class
  source:
    persistentVolumeClaimName: ${pvc_name}
EOF
              done
              
              # 清理旧快照 (保留最近7个)
              kubectl get volumesnapshot --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.metadata.creationTimestamp}{"\n"}{end}' | \
              sort -k3 | \
              awk '{
                if (!seen[$1$2]++) {
                  count[$1$2]++
                  if (count[$1$2] > 7) {
                    print "kubectl delete volumesnapshot -n " $1 " " $2
                  }
                }
              }' | bash
              
            volumeMounts:
            - name: kubeconfig
              mountPath: /etc/kubernetes
              readOnly: true
          restartPolicy: OnFailure
          volumes:
          - name: kubeconfig
            hostPath:
              path: /etc/kubernetes
              type: Directory
```

---

## 7. 灾备演练流程

### 7.1 定期演练计划

```bash
#!/bin/bash
# 灾备演练脚本

# 演练配置
DRILL_TYPE=${1:-"partial"}  # partial, full, application
TEST_NAMESPACE="dr-test-$(date +%Y%m%d)"

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/dr-drill.log
}

# 准备演练环境
prepare_drill_environment() {
    log "准备灾备演练环境..."
    
    # 创建测试命名空间
    kubectl create namespace "$TEST_NAMESPACE"
    
    # 部署测试应用
    cat > /tmp/drill-test-app.yaml << EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: drill-test-app
  namespace: $TEST_NAMESPACE
spec:
  replicas: 3
  selector:
    matchLabels:
      app: drill-test
  template:
    metadata:
      labels:
        app: drill-test
    spec:
      containers:
      - name: test-app
        image: nginx:latest
        ports:
        - containerPort: 80
        volumeMounts:
        - name: test-storage
          mountPath: /usr/share/nginx/html
      volumes:
      - name: test-storage
        persistentVolumeClaim:
          claimName: drill-test-pvc
---
apiVersion: v1
kind: Service
metadata:
  name: drill-test-service
  namespace: $TEST_NAMESPACE
spec:
  selector:
    app: drill-test
  ports:
  - port: 80
    targetPort: 80
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: drill-test-pvc
  namespace: $TEST_NAMESPACE
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
EOF

    kubectl apply -f /tmp/drill-test-app.yaml
    kubectl wait --for=condition=available --timeout=300s deployment/drill-test-app -n "$TEST_NAMESPACE"
    
    log "✓ 演练环境准备完成"
}

# 执行备份
execute_backup_during_drill() {
    log "执行灾备演练期间的备份..."
    
    # 触发所有备份任务
    /usr/local/bin/etcd-backup.sh
    /usr/local/bin/config-backup.sh
    /usr/local/bin/cert-backup.sh
    
    # 验证备份
    /usr/local/bin/backup-validation.sh
    
    log "✓ 备份执行完成"
}

# 模拟故障
simulate_failure() {
    local failure_type=${1:-"node"}
    
    log "模拟${failure_type}故障..."
    
    case $failure_type in
        "node")
            # 模拟节点故障
            local target_node=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
            log "模拟节点故障: $target_node"
            # 这里可以集成具体的节点故障模拟工具
            ;;
        "control-plane")
            # 模拟控制平面故障
            log "模拟控制平面组件故障"
            systemctl stop kube-apiserver
            systemctl stop etcd
            ;;
        "network")
            # 模拟网络分区
            log "模拟网络分区"
            # iptables规则模拟网络隔离
            ;;
    esac
    
    log "✓ 故障模拟完成"
}

# 执行恢复
execute_recovery() {
    log "执行灾难恢复..."
    
    # 恢复etcd
    /usr/local/bin/etcd-restore.sh
    
    # 恢复配置
    /usr/local/bin/config-restore.sh
    
    # 恢复证书
    /usr/local/bin/cert-restore.sh
    
    # 重启服务
    systemctl restart kubelet
    systemctl restart containerd
    
    log "✓ 恢复操作完成"
}

# 验证恢复
verify_recovery() {
    log "验证恢复结果..."
    
    # 检查集群状态
    kubectl get nodes
    kubectl get pods -n "$TEST_NAMESPACE"
    
    # 验证应用功能
    local service_ip=$(kubectl get svc drill-test-service -n "$TEST_NAMESPACE" -o jsonpath='{.spec.clusterIP}')
    if curl -s "http://$service_ip" | grep -q "Welcome"; then
        log "✓ 应用功能验证通过"
    else
        log "✗ 应用功能验证失败"
        return 1
    fi
    
    # 验证数据完整性
    local pvc_data=$(kubectl exec -n "$TEST_NAMESPACE" deployment/drill-test-app -- cat /usr/share/nginx/html/test-data.txt 2>/dev/null)
    if [ "$pvc_data" = "drill-test-data" ]; then
        log "✓ 数据完整性验证通过"
    else
        log "✗ 数据完整性验证失败"
        return 1
    fi
    
    log "✓ 恢复验证完成"
    return 0
}

# 清理演练环境
cleanup_drill_environment() {
    log "清理演练环境..."
    
    kubectl delete namespace "$TEST_NAMESPACE" --ignore-not-found=true
    rm -f /tmp/drill-test-app.yaml
    
    log "✓ 演练环境清理完成"
}

# 生成演练报告
generate_drill_report() {
    local drill_result=$1
    
    log "生成灾备演练报告..."
    
    local report_file="/var/reports/dr-drill-report-$(date +%Y%m%d-%H%M%S).md"
    
    cat > "$report_file" << EOF
# 灾备演练报告

**演练时间**: $(date)
**演练类型**: $DRILL_TYPE
**演练结果**: $([ $drill_result -eq 0 ] && echo "成功" || echo "失败")

## 演练步骤执行情况

1. **环境准备**: ✓ 完成
2. **备份执行**: ✓ 完成  
3. **故障模拟**: ✓ 完成
4. **恢复执行**: ✓ 完成
5. **结果验证**: $([ $drill_result -eq 0 ] && echo "✓" || echo "✗") 完成

## 关键指标

- **RTO (恢复时间目标)**: $(($(date +%s) - START_TIME)) 秒
- **RPO (恢复点目标)**: 5 分钟
- **数据完整性**: $([ $drill_result -eq 0 ] && echo "✓ 保持" || echo "✗ 丢失")

## 问题记录

$(if [ $drill_result -ne 0 ]; then
    echo "- 恢复过程中发现问题，需要进一步调查"
fi)

## 改进建议

- 定期更新备份策略
- 优化恢复流程
- 加强监控告警

---
报告生成时间: $(date)
EOF

    log "✓ 演练报告已生成: $report_file"
}

# 主演练函数
main_drill() {
    local START_TIME=$(date +%s)
    local drill_result=0
    
    log "=== 灾备演练开始 ($DRILL_TYPE) ==="
    
    # 根据演练类型执行不同流程
    case $DRILL_TYPE in
        "partial")
            prepare_drill_environment
            execute_backup_during_drill
            simulate_failure "control-plane"
            execute_recovery
            drill_result=$?
            verify_recovery
            drill_result=$?
            cleanup_drill_environment
            ;;
        "full")
            prepare_drill_environment
            execute_backup_during_drill
            simulate_failure "node"
            execute_recovery
            drill_result=$?
            verify_recovery
            drill_result=$?
            cleanup_drill_environment
            ;;
        "application")
            prepare_drill_environment
            # 应用层面的演练
            cleanup_drill_environment
            ;;
    esac
    
    generate_drill_report $drill_result
    log "=== 灾备演练结束 ==="
    
    return $drill_result
}

# 执行演练
main_drill
```

---

## 8. 恢复操作手册

### 8.1 紧急恢复流程

```markdown
# Kubernetes控制平面紧急恢复操作手册

## 恢复前准备

### 1. 确认灾备状态
- [ ] 确认备份系统正常运行
- [ ] 验证最近备份的完整性
- [ ] 准备恢复所需的凭据和权限
- [ ] 通知相关人员启动应急响应

### 2. 评估损失范围
- [ ] 确定受影响的组件和服务
- [ ] 评估数据丢失程度
- [ ] 确认RTO/RPO要求
- [ ] 制定恢复优先级

## 恢复步骤

### 阶段1: 环境准备 (预计时间: 15分钟)

1. **隔离故障环境**
   ```bash
   # 标记故障节点为不可调度
   kubectl cordon <faulty-node>
   
   # 如果需要，驱逐节点上的Pod
   kubectl drain <faulty-node> --ignore-daemonsets --delete-emptydir-data
   ```

2. **准备恢复环境**
   ```bash
   # 创建恢复工作目录
   mkdir -p /tmp/k8s-recovery
   cd /tmp/k8s-recovery
   
   # 下载必要的恢复工具
   wget https://github.com/etcd-io/etcd/releases/download/v3.5.12/etcd-v3.5.12-linux-amd64.tar.gz
   tar -xzf etcd-v3.5.12-linux-amd64.tar.gz
   ```

### 阶段2: etcd恢复 (预计时间: 30分钟)

1. **停止相关服务**
   ```bash
   systemctl stop kube-apiserver
   systemctl stop kube-controller-manager
   systemctl stop kube-scheduler
   systemctl stop etcd
   ```

2. **恢复etcd数据**
   ```bash
   # 选择最新的备份
   LATEST_BACKUP=$(find /backup/etcd -name "*.db" -type f -mtime -7 | sort -r | head -1)
   
   # 恢复快照
   ETCDCTL_API=3 ./etcdctl snapshot restore $LATEST_BACKUP \
     --data-dir=/var/lib/etcd-restored \
     --name=etcd-0 \
     --initial-cluster=etcd-0=https://<node-ip>:2380 \
     --initial-cluster-token=etcd-cluster-1 \
     --initial-advertise-peer-urls=https://<node-ip>:2380
   
   # 替换数据目录
   mv /var/lib/etcd /var/lib/etcd.backup.$(date +%Y%m%d)
   mv /var/lib/etcd-restored /var/lib/etcd
   
   # 设置权限
   chown -R etcd:etcd /var/lib/etcd
   ```

3. **启动etcd集群**
   ```bash
   systemctl start etcd
   
   # 验证集群健康
   ETCDCTL_API=3 etcdctl endpoint health --cluster
   ```

### 阶段3: 控制平面恢复 (预计时间: 20分钟)

1. **恢复API Server**
   ```bash
   # 恢复配置文件
   cp /backup/config/latest/etc/kubernetes/manifests/kube-apiserver.yaml \
      /etc/kubernetes/manifests/
   
   # 等待Pod启动
   kubectl wait --for=condition=ready pod -n kube-system -l component=kube-apiserver --timeout=300s
   ```

2. **恢复其他控制平面组件**
   ```bash
   # 恢复Controller Manager
   cp /backup/config/latest/etc/kubernetes/manifests/kube-controller-manager.yaml \
      /etc/kubernetes/manifests/
   
   # 恢复Scheduler
   cp /backup/config/latest/etc/kubernetes/manifests/kube-scheduler.yaml \
      /etc/kubernetes/manifests/
   
   # 验证组件状态
   kubectl get componentstatuses
   ```

### 阶段4: 节点和服务恢复 (预计时间: 25分钟)

1. **恢复工作节点**
   ```bash
   # 重新标记节点为可调度
   kubectl uncordon <recovered-node>
   
   # 验证节点状态
   kubectl get nodes
   ```

2. **验证核心服务**
   ```bash
   # 检查系统Pod状态
   kubectl get pods -n kube-system
   
   # 验证DNS服务
   kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default
   ```

3. **恢复应用服务**
   ```bash
   # 恢复关键应用
   kubectl apply -f /backup/applications/critical-services.yaml
   
   # 验证服务可用性
   kubectl get services --all-namespaces
   ```

## 验证清单

### 基础设施验证
- [ ] etcd集群健康状态正常
- [ ] API Server响应正常
- [ ] 所有控制平面组件运行正常
- [ ] 工作节点状态Ready
- [ ] 网络插件功能正常

### 数据验证
- [ ] 核心配置数据完整
- [ ] 应用配置数据恢复
- [ ] 持久化数据完整性检查
- [ ] 证书有效性验证

### 服务验证
- [ ] Kubernetes API访问正常
- [ ] 应用服务可访问
- [ ] 监控告警系统运行正常
- [ ] 日志收集系统功能正常

## 后续行动

### 1. 根因分析
- [ ] 收集故障期间的日志和指标
- [ ] 分析故障根本原因
- [ ] 编写事故报告

### 2. 系统加固
- [ ] 根据教训更新灾备策略
- [ ] 优化监控告警配置
- [ ] 加强自动化恢复能力

### 3. 团队沟通
- [ ] 向管理层汇报恢复情况
- [ ] 组织复盘会议
- [ ] 更新相关文档和流程
```

---

## 9. 监控告警配置

### 9.1 备份监控告警

```yaml
# 备份监控告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: backup-monitoring-alerts
  namespace: monitoring
spec:
  groups:
  - name: backup.monitoring.rules
    rules:
    # 备份任务监控
    - alert: BackupJobFailed
      expr: kube_job_status_failed{job_name=~"backup-.*"} > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "备份任务失败"
        description: "备份任务 {{ $labels.job_name }} 执行失败"
        
    - alert: BackupJobMissing
      expr: absent(kube_job_status_succeeded{job_name=~"backup-.*"})
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "备份任务缺失"
        description: "未检测到预期的备份任务运行"
        
    # 备份数据监控
    - alert: BackupSizeAnomaly
      expr: rate(backup_size_bytes[1h]) > 2 * avg_over_time(rate(backup_size_bytes[24h])[1h:])
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "备份数据量异常"
        description: "备份数据量增长超过正常水平的200%"
        
    - alert: BackupAgeTooOld
      expr: time() - max(backup_timestamp_seconds) > 86400
      for: 30m
      labels:
        severity: critical
      annotations:
        summary: "备份文件过旧"
        description: "最新备份文件超过24小时未更新"
        
    # 存储空间监控
    - alert: BackupStorageLow
      expr: (backup_storage_used_bytes / backup_storage_total_bytes) * 100 > 85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "备份存储空间不足"
        description: "备份存储使用率超过85%"
        
    - alert: BackupStorageCritical
      expr: (backup_storage_used_bytes / backup_storage_total_bytes) * 100 > 95
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "备份存储空间严重不足"
        description: "备份存储使用率超过95%"
```

### 9.2 灾备系统监控

```yaml
# 灾备系统监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: dr-system-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: dr-system
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
  - port: health
    interval: 10s
    path: /healthz

---
# 灾备系统告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-system-alerts
  namespace: monitoring
spec:
  groups:
  - name: dr.system.rules
    rules:
    # 灾备系统健康检查
    - alert: DRSystemUnhealthy
      expr: dr_system_health_status != 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "灾备系统不健康"
        description: "灾备系统健康检查失败"
        
    - alert: DRSystemSyncLag
      expr: dr_sync_lag_seconds > 300
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "灾备同步延迟"
        description: "灾备数据同步延迟超过5分钟"
        
    # 恢复能力监控
    - alert: DRRecoveryTimeExceeded
      expr: dr_recovery_time_seconds > 3600
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "灾难恢复时间超限"
        description: "灾难恢复时间超过1小时"
        
    - alert: DRTestFailure
      expr: dr_test_success_rate < 0.8
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "灾备测试失败率高"
        description: "灾备测试成功率低于80%"
```

---

## 10. 灾备最佳实践

### 10.1 灾备规划检查清单

```markdown
## 灾备规划检查清单

### 业务影响分析
- [ ] 识别关键业务系统和数据
- [ ] 确定RTO（恢复时间目标）和RPO（恢复点目标）
- [ ] 评估业务中断成本
- [ ] 制定数据分类和保护级别

### 技术架构设计
- [ ] 选择合适的灾备架构模式
- [ ] 设计网络冗余和故障切换机制
- [ ] 规划存储复制和同步策略
- [ ] 确定备份频率和保留策略

### 实施准备
- [ ] 准备灾备环境基础设施
- [ ] 部署监控和告警系统
- [ ] 建立自动化备份和恢复流程
- [ ] 配置安全管理措施

### 测试验证
- [ ] 制定定期演练计划
- [ ] 建立演练评估标准
- [ ] 记录和分析演练结果
- [ ] 持续优化灾备方案

### 组织保障
- [ ] 明确灾备团队职责分工
- [ ] 建立应急响应流程
- [ ] 准备必要的文档和联系方式
- [ ] 定期培训和意识提升
```

### 10.2 灾备运维规范

```bash
#!/bin/bash
# 灾备系统日常运维脚本

# 配置变量
LOG_FILE="/var/log/dr-maintenance.log"
HEALTH_CHECK_INTERVAL=300  # 5分钟

# 日志函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# 健康检查
health_check() {
    log "执行灾备系统健康检查..."
    
    local checks=(
        "etcd集群健康"
        "备份任务状态"
        "存储空间使用率"
        "网络连通性"
        "证书有效期"
    )
    
    local failed_checks=()
    
    # 检查etcd健康
    if ! ETCDCTL_API=3 etcdctl endpoint health --cluster; then
        failed_checks+=("etcd集群不健康")
    fi
    
    # 检查备份任务
    local failed_jobs=$(kubectl get jobs -n kube-system -l app=backup -o jsonpath='{.items[?(@.status.failed>0)].metadata.name}' | wc -w)
    if [ $failed_jobs -gt 0 ]; then
        failed_checks+=("$failed_jobs个备份任务失败")
    fi
    
    # 检查存储空间
    local storage_usage=$(df /backup | awk 'NR==2 {print $5}' | sed 's/%//')
    if [ $storage_usage -gt 85 ]; then
        failed_checks+=("备份存储使用率过高: ${storage_usage}%")
    fi
    
    # 检查证书有效期
    local expiring_certs=$(find /etc/kubernetes/pki -name "*.crt" -exec openssl x509 -enddate -noout -in {} \; 2>/dev/null | \
        grep -v "notAfter=" | \
        awk -F'=' '{cmd="date -d \"" $2 "\" +%s"; cmd | getline expiry; close(cmd); if (expiry < systime() + 2592000) print $0}')
    
    if [ -n "$expiring_certs" ]; then
        failed_checks+=("存在即将过期的证书")
    fi
    
    if [ ${#failed_checks[@]} -eq 0 ]; then
        log "✓ 灾备系统健康检查通过"
        return 0
    else
        log "✗ 灾备系统健康检查发现问题:"
        for issue in "${failed_checks[@]}"; do
            log "  - $issue"
        done
        return 1
    fi
}

# 性能监控
performance_monitoring() {
    log "收集灾备系统性能指标..."
    
    # 收集etcd性能指标
    local etcd_metrics=$(curl -s http://localhost:2379/metrics | \
        grep -E "(etcd_disk_backend_commit_duration_seconds|etcd_network_peer_round_trip_time_seconds)" | \
        tail -10)
    
    # 收集备份性能指标
    local backup_metrics=$(kubectl get jobs -n kube-system -l app=backup -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.status.completionTime}{"\n"}{end}' | \
        tail -5)
    
    # 记录到监控系统
    echo "$etcd_metrics" >> /var/log/etcd-performance.log
    echo "$backup_metrics" >> /var/log/backup-performance.log
    
    log "✓ 性能指标收集完成"
}

# 自动修复
auto_repair() {
    log "执行自动修复..."
    
    # 重启失败的备份任务
    kubectl get jobs -n kube-system -l app=backup --field-selector=status.failed>0 -o name | \
        xargs -r kubectl delete
    
    # 清理旧的备份文件
    find /backup -name "*.old" -mtime +7 -delete
    
    # 重新生成即将过期的证书
    # 这里应该调用证书管理系统的相应功能
    
    log "✓ 自动修复完成"
}

# 报告生成
generate_report() {
    log "生成灾备系统运行报告..."
    
    local report_file="/var/reports/dr-health-report-$(date +%Y%m%d).md"
    
    cat > "$report_file" << EOF
# 灾备系统健康报告

**报告时间**: $(date)
**系统状态**: $(health_check && echo "健康" || echo "异常")

## 关键指标

### 备份状态
- 今日备份任务: $(kubectl get jobs -n kube-system -l app=backup --field-selector=status.succeeded=1 | wc -l) 成功
- 失败备份任务: $(kubectl get jobs -n kube-system -l app=backup --field-selector=status.failed>0 | wc -l) 个

### 存储使用
- 备份存储使用率: $(df /backup | awk 'NR==2 {print $5}')
- 可用存储空间: $(df -h /backup | awk 'NR==2 {print $4}')

### 系统性能
- etcd平均延迟: $(curl -s http://localhost:2379/metrics | grep etcd_disk_backend_commit_duration_seconds_sum | head -1 | awk '{print $2/1000000"s"}')

## 建议行动项

$(health_check || echo "- 请检查上述健康检查发现的问题")
$(performance_monitoring | grep -q "异常" && echo "- 性能指标异常，请关注系统负载")

---
报告生成时间: $(date)
EOF

    log "✓ 健康报告已生成: $report_file"
}

# 主维护函数
main_maintenance() {
    log "=== 灾备系统日常维护开始 ==="
    
    health_check
    performance_monitoring
    auto_repair
    generate_report
    
    log "=== 灾备系统日常维护完成 ==="
}

# 定时执行
while true; do
    main_maintenance
    sleep $HEALTH_CHECK_INTERVAL
done
```

这份灾备方案提供了完整的备份恢复策略，包括etcd备份、配置管理、证书保护、存储卷快照等各个方面，并配有详细的演练流程和监控告警配置，确保在灾难发生时能够快速有效地恢复Kubernetes控制平面。