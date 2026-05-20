---
title: containerd 灾难恢复与业务连续性
description: '## 1. 灾难恢复概述'
category: cncf-landscape
tags:
- k8s
- containerd
- disaster-recovery
- backup
- restore
- failover
- business-continuity
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 运维工程师
- SRE
- 架构师
estimated_read_time: 12min
intent_queries:
- containerd 灾难恢复 步骤
- containerd 数据备份 如何备份
- containerd 恢复 操作步骤
- containerd 容灾 方案
trigger_keywords:
- containerd 灾难恢复
- containerd 备份
- containerd 恢复
- containerd 容灾
---

# containerd 灾难恢复与业务连续性

> **版本**: v1.0 | **适用版本**: containerd 1.6+ / 2.0 | **最后更新**: 2026-05

---

## 1. 灾难恢复概述

### 1.1 灾难场景分类

| 场景 | RTO | RPO | 影响范围 |
|------|-----|-----|----------|
| **containerd 进程崩溃** | 5 min | 0 | 单节点容器中断 |
| **节点故障** | 15 min | 0 | 该节点所有容器中断 |
| **数据目录损坏** | 30 min | 1 hour | 单节点，需恢复镜像和容器 |
| **多节点故障** | 1 hour | 1 hour | 集群级别影响 |
| **区域级故障** | 4 hours | 4 hours | 整个集群不可用 |

### 1.2 灾难恢复架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 灾难恢复架构                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Primary Site                          DR Site                                  │
│  ┌─────────────────────┐            ┌─────────────────────┐                    │
│  │   Kubernetes Cluster │            │   Kubernetes Cluster │                    │
│  │   ┌───────────────┐ │            │   ┌───────────────┐ │                    │
│  │   │   containerd  │ │            │   │   containerd  │ │                    │
│  │   │   Nodes (3)   │ │            │   │   Nodes (1)   │ │                    │
│  │   └───────┬───────┘ │            │   └───────┬───────┘ │                    │
│  │           │         │            │           │         │                    │
│  │           ▼         │            │           ▼         │                    │
│  │   ┌───────────────┐│◄───────────►│   (镜像同步)        │                    │
│  │   │  Data Backup  ││   Rsync     │   ┌───────────────┐│                    │
│  │   │  /var/lib/    ││   Mirror    │   │  Mirror       ││                    │
│  │   │  containerd   ││            │   │  /var/lib/    ││                    │
│  │   └───────────────┘│            │   │  containerd   ││                    │
│  └─────────────────────┘            └─────────────────────┘                    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 备份类型

| 备份类型 | 频率 | 内容 | 恢复时间 |
|----------|------|------|----------|
| **配置备份** | 每次变更 | config.toml, certs | < 5 min |
| **元数据备份** | 每日 | containers, images metadata | < 10 min |
| **完整数据备份** | 每周 | /var/lib/containerd | > 1 hour |
| **增量镜像备份** | 每日 | 业务镜像 | < 30 min |

---

## 2. 备份策略

### 2.1 配置备份

```bash
#!/bin/bash
# backup-containerd-config.sh

BACKUP_DIR="/backup/containerd/config"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# 备份配置文件
cp /etc/containerd/config.toml $BACKUP_DIR/config_$DATE.toml

# 备份证书目录
tar -czf $BACKUP_DIR/certs_$DATE.tar.gz /etc/containerd/certs.d/

# 备份 kubelet 配置（CRI 端点）
cp /var/lib/kubelet/config.yaml $BACKUP_DIR/kubelet-config_$DATE.yaml
cp /var/lib/kubelet/kubeadm-flags.env $BACKUP_DIR/kubeadm-flags_$DATE.env

# 计算配置哈希（用于完整性验证）
sha256sum /etc/containerd/config.toml > $BACKUP_DIR/config_$DATE.sha256

# 清理 30 天前的备份
find $BACKUP_DIR -type f -mtime +30 -delete

echo "[$DATE] Configuration backup completed"
```

### 2.2 元数据备份

```bash
#!/bin/bash
# backup-containerd-metadata.sh

BACKUP_DIR="/backup/containerd/metadata"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# 获取 containerd 状态
crictl info > $BACKUP_DIR/crictl-info_$DATE.json

# 导出容器列表
crictl ps -a > $BACKUP_DIR/containers_$DATE.txt

# 导出镜像列表
crictl images > $BACKUP_DIR/images_$DATE.txt

# 导出 Pod Sandbox 列表
crictl pods > $BACKUP_DIR/pods_$DATE.txt

# 备份 containerd 元数据
cp -r /var/lib/containerd/io.containerd.metadata.v1.bolt/ $BACKUP_DIR/bolt-meta_$DATE/

# 创建备份清单
cat > $BACKUP_DIR/manifest_$DATE.txt << EOF
backup_date: $DATE
containerd_version: $(containerd --version | awk '{print $3}')
total_containers: $(crictl ps -a | wc -l)
total_images: $(crictl images | wc -l)
total_pods: $(crictl pods | wc -l)
EOF

# 上传到远程存储
aws s3 sync $BACKUP_DIR s3://my-bucket/containerd-metadata/$DATE/ --storage-class STANDARD_IA

echo "[$DATE] Metadata backup completed"
```

### 2.3 完整数据备份 (Rsync 方式)

```bash
#!/bin/bash
# backup-containerd-full.sh

set -e

REMOTE_HOST="dr-site.example.com"
REMOTE_USER="backup"
REMOTE_DIR="/backup/containerd/data"
DATE=$(date +%Y%m%d)

echo "[$DATE] Starting full containerd backup..."

# 1. 暂停容器写入 (可选，使用快照一致性)
# crictl保湿所有容器
crictl保湿 --all  # 暂停所有容器

# 2. 执行 Rsync 备份
rsync -avz --progress \
  --exclude='io.containerd.runtime.v2.task/' \
  --exclude='*.tmp' \
  --exclude='shm/' \
  /var/lib/containerd/ \
  $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/$DATE/

# 3. 恢复容器写入
crictl保湿 --all --undo  # 恢复所有容器

# 4. 验证备份
ssh $REMOTE_USER@$REMOTE_HOST "ls -la $REMOTE_DIR/$DATE/ | head -20"

# 5. 记录备份元数据
ssh $REMOTE_USER@$REMOTE_HOST "echo '$(date),$(du -sh /var/lib/containerd | cut -f1)' >> $REMOTE_DIR/backup-log.txt"

echo "[$DATE] Full backup completed"
```

### 2.4 增量镜像备份

```bash
#!/bin/bash
# backup-containerd-images.sh

set -e

REMOTE_HOST="dr-site.example.com"
REMOTE_DIR="/backup/containerd/images"

# 获取所有镜像
IMAGES=$(crictl images -o json | jq -r '.images[].repoTags[]' 2>/dev/null || crictl images | awk 'NR>1 {print $1":"$2}')

# 增量备份：只备份不存在的镜像
for image in $IMAGES; do
    # 检查远程是否已存在
    if ! ssh $REMOTE_USER@$REMOTE_HOST "test -e $REMOTE_DIR/$(echo $image | tr '/:' '_').tar"; then
        echo "Backing up $image..."
        docker save $image -o /tmp/$(echo $image | tr '/:' '_').tar
        rsync -avz /tmp/$(echo $image | tr '/:' '_').tar $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/
        rm /tmp/$(echo $image | tr '/:' '_').tar
    fi
done

echo "Image backup completed"
```

---

## 3. 恢复流程

### 3.1 快速恢复 (配置 + 元数据)

```bash
#!/bin/bash
# restore-containerd-quick.sh

set -e

BACKUP_DATE="20260519_120000"
BACKUP_DIR="/backup/containerd/config"

echo "Starting quick restore from $BACKUP_DATE..."

# 1. 停止 containerd
systemctl stop containerd

# 2. 恢复配置
cp $BACKUP_DIR/config_$BACKUP_DATE.toml /etc/containerd/config.toml
sha256sum -c $BACKUP_DIR/config_$BACKUP_DATE.sha256

# 3. 恢复证书
tar -xzf $BACKUP_DIR/certs_$BACKUP_DATE.tar.gz -C /

# 4. 恢复 kubelet 配置
cp $BACKUP_DIR/kubelet-config_$BACKUP_DATE.yaml /var/lib/kubelet/config.yaml
cp $BACKUP_DIR/kubeadm-flags_$BACKUP_DATE.env /var/lib/kubelet/kubeadm-flags.env

# 5. 启动 containerd
systemctl start containerd

# 6. 验证
crictl info | grep -i version
kubectl get nodes

echo "Quick restore completed"
```

### 3.2 完整恢复 (包含镜像)

```bash
#!/bin/bash
# restore-containerd-full.sh

set -e

REMOTE_HOST="dr-site.example.com"
REMOTE_USER="backup"
REMOTE_DIR="/backup/containerd/data"
BACKUP_DATE=$(date +%Y%m%d)

echo "Starting full restore from DR site..."

# 1. 封锁节点
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# 2. 停止 containerd
systemctl stop containerd

# 3. 备份当前数据（以防万一）
tar -czf /tmp/containerd-current-$(date +%Y%m%d%H%M%S).tar.gz /var/lib/containerd/ 2>/dev/null || true

# 4. 从远程恢复数据
rsync -avz --delete \
  $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/$BACKUP_DATE/ \
  /var/lib/containerd/

# 5. 恢复证书
tar -xzf $BACKUP_DIR/certs_*.tar.gz -C /

# 6. 启动 containerd
systemctl start containerd

# 7. 验证
crictl images
crictl ps -a

# 8. 解锁节点
kubectl uncordon <node-name>

# 9. 恢复镜像
for image_tar in $(ssh $REMOTE_USER@$REMOTE_HOST "ls $REMOTE_DIR/images/*.tar" 2>/dev/null); do
    crictl load -i $image_tar
done

echo "Full restore completed"
```

### 3.3 跨集群迁移

```bash
#!/bin/bash
# migrate-containerd.sh

set -e

SOURCE_NODE="source-node"
TARGET_NODE="target-node"
IMAGES_FILE="/tmp/images-list.txt"

echo "Starting containerd migration..."

# 1. 导出镜像列表
crictl images -o json | jq -r '.images[].repoTags[]' > $IMAGES_FILE

# 2. 在目标节点安装 containerd
ssh $TARGET_NODE "apt-get install containerd.io"

# 3. 复制配置
scp /etc/containerd/config.toml $TARGET_NODE:/etc/containerd/config.toml

# 4. 迁移镜像
while read image; do
    echo "Migrating $image..."
    docker pull $image
    docker save $image | ssh $TARGET_NODE "ctr -n k8s.io images import -"
done < $IMAGES_FILE

# 5. 验证
ssh $TARGET_NODE "crictl images"

echo "Migration completed"
```

---

## 4. 容灾方案

### 4.1 主从同步

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         主从同步架构                                             │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Primary Node                      DR Node                                       │
│  ┌──────────────────┐            ┌──────────────────┐                          │
│  │   containerd     │            │   containerd     │                          │
│  │                  │   Rsync     │                  │                          │
│  │  /var/lib/       │ ──────────► │  /var/lib/       │                          │
│  │  containerd/     │   实时增量   │  containerd/     │                          │
│  └──────────────────┘            └──────────────────┘                          │
│           │                               │                                     │
│           ▼                               ▼                                     │
│  ┌──────────────────┐            ┌──────────────────┐                          │
│  │   kubelet        │            │   kubelet        │                          │
│  └──────────────────┘            └──────────────────┘                          │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Rsync 实时同步配置

```bash
# 在 DR 节点设置 rsyncd 服务
# /etc/rsyncd.conf
uid = root
gid = root
max connections = 10
timeout = 300
exclude = ["shm/", "run/", "tmp/"]

[containerd-data]
    path = /var/lib/containerd
    comment = containerd data directory
    auth users = backup
    secrets file = /etc/rsyncd.secrets
```

```bash
# 主节点 cron 任务（每分钟同步）
# */1 * * * * rsync -avz --delete --exclude='shm/' --exclude='run/' root@dr-node:/var/lib/containerd/ /var/lib/containerd/
```

### 4.3 镜像预热

```bash
#!/bin/bash
# prewarm-images.sh

DR_NODE="dr-node.example.com"

echo "Starting image prewarm..."

# 获取所有镜像列表
IMAGES=$(crictl images -o json | jq -r '.images[].repoTags[]')

# 预热到 DR 节点
for image in $IMAGES; do
    echo "Prewarming $image..."
    ssh $DR_NODE "crictl pull $image"
done

echo "Image prewarm completed"
```

---

## 5. 灾难恢复演练

### 5.1 演练计划

| 阶段 | 时间 | 内容 |
|------|------|------|
| **准备** | D-7 | 制定演练方案，准备测试环境 |
| **执行** | D-0 | 执行恢复流程，记录时间 |
| **评估** | D+1 | 分析问题，更新文档 |
| **改进** | D+7 | 修复发现的问题 |

### 5.2 演练脚本

```bash
#!/bin/bash
# disaster-recovery-drill.sh

set -e

TEST_NODE="drill-node"
DR_BACKUP_DIR="/backup/containerd/test"
DATE=$(date +%Y%m%d_%H%M%S)

echo "=== Containerd Disaster Recovery Drill ==="
echo "Date: $DATE"
echo ""

# 记录开始时间
START_TIME=$(date +%s)

# 1. 创建测试数据
echo "1. Creating test workload..."
kubectl run test-app --image=busybox -- sleep 300
kubectl get pods -l run=test-app

# 2. 执行备份
echo "2. Executing backup..."
./backup-containerd-config.sh
./backup-containerd-metadata.sh

# 3. 模拟故障
echo "3. Simulating failure..."
kubectl cordon $TEST_NODE
kubectl drain $TEST_NODE --ignore-daemonsets --delete-emptydir-data --force
systemctl stop containerd
rm -rf /var/lib/containerd/*

# 4. 执行恢复
echo "4. Executing restore..."
./restore-containerd-quick.sh

# 5. 验证恢复
echo "5. Verifying restore..."
crictl images | wc -l
crictl ps -a | wc -l

# 6. 恢复节点
echo "6. Restoring node to service..."
kubectl uncordon $TEST_NODE

# 记录结束时间
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo ""
echo "=== Drill Completed ==="
echo "Duration: $DURATION seconds"
echo "RTO achieved: < 15 minutes (target)"
```

### 5.3 演练报告模板

```markdown
# Containerd 灾难恢复演练报告

## 基本信息
- **演练日期**: YYYY-MM-DD
- **演练节点**: xxx
- **参与人员**: xxx
- **演练目标**: 验证 RTO < 15 分钟

## 执行过程

### 1. 备份阶段
- 配置备份: X 分钟
- 元数据备份: X 分钟
- 镜像备份: X 分钟
- **子总计**: X 分钟

### 2. 故障模拟
- 停止服务: X 秒
- 数据清除: X 秒

### 3. 恢复阶段
- 配置恢复: X 分钟
- 服务启动: X 分钟
- 镜像恢复: X 分钟
- 验证检查: X 分钟
- **子总计**: X 分钟

## 结果评估

| 指标 | 目标值 | 实际值 | 状态 |
|------|--------|--------|------|
| RTO | < 15 min | X min | ✅/❌ |
| RPO | < 1 hour | X min | ✅/❌ |
| 数据完整性 | 100% | X% | ✅/❌ |
| 服务可用性 | 100% | X% | ✅/❌ |

## 问题记录

| # | 问题描述 | 严重程度 | 解决方案 |
|---|----------|----------|----------|
| 1 | | | |

## 改进建议

1.
2.
3.
```

---

## 6. 多区域容灾

### 6.1 区域级故障切换

```yaml
# Failover 配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: containerd-dr-config
  namespace: default
data:
  dr-enabled: "true"
  primary-region: "us-east-1"
  dr-region: "us-west-2"
  heartbeat-interval: "30s"
  failover-timeout: "120s"
---
apiVersion: v1
kind: Service
metadata:
  name: containerd-dr-monitor
spec:
  selector:
    app: containerd-dr
  clusterIP: None
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: containerd-dr-controller
spec:
  replicas: 1
  selector:
    matchLabels:
      app: containerd-dr
  template:
    spec:
      containers:
      - name: dr-controller
        image: my-dr-controller:v1
        env:
        - name: DR_CONFIG
          valueFrom:
            configMapKeyRef:
              name: containerd-dr-config
              key: dr-enabled
```

### 6.2 健康检查

```bash
#!/bin/bash
# containerd-health-check.sh

set -e

PRIMARY_SITE="primary.example.com"
DR_SITE="dr.example.com"

check_containerd() {
    local host=$1
    local result
    
    result=$(ssh $host "systemctl is-active containerd" 2>/dev/null)
    if [ "$result" = "active" ]; then
        # 检查 CRI 可用性
        crictl info --timeout 5s >/dev/null 2>&1
        return $?
    fi
    return 1
}

# 检查主站点
if check_containerd $PRIMARY_SITE; then
    echo "Primary site: OK"
else
    echo "Primary site: FAILED"
    # 触发 failover
    /usr/local/bin/trigger-failover.sh
fi

# 检查 DR 站点
if check_containerd $DR_SITE; then
    echo "DR site: OK"
else
    echo "DR site: WARNING - not ready"
fi
```

---

## 7. 业务连续性保障

### 7.1 自动故障切换

```bash
#!/bin/bash
# trigger-failover.sh

set -e

DR_NODE="dr-node.example.com"
SITE_NAME="us-west-2"

echo "Triggering failover to $SITE_NAME..."

# 1. 停止主站点写入
ssh $PRIMARY_SITE "systemctl stop containerd"

# 2. 提升 DR 站点
ssh $DR_NODE "systemctl restart containerd"

# 3. 更新 DNS/负载均衡
aws route53 change-resource-record-sets \
    --hosted-zone-id Z1234567890 \
    --change-batch file://failover-record.json

# 4. 验证
sleep 30
kubectl get nodes --context=$SITE_NAME

# 5. 发送通知
curl -X POST https://notify.example.com/alerts \
    -d '{"severity": "critical", "message": "Failover to DR site completed"}'

echo "Failover completed"
```

### 7.2 回切流程

```bash
#!/bin/bash
# failback.sh

set -e

PRIMARY_SITE="primary.example.com"
DR_SITE="dr.example.com"

echo "Starting failback to primary site..."

# 1. 同步 DR 到主站点（增量）
rsync -avz $DR_SITE:/var/lib/containerd/ $PRIMARY_SITE:/var/lib/containerd/

# 2. 停止 DR 站点
ssh $DR_SITE "systemctl stop containerd"

# 3. 启动主站点
ssh $PRIMARY_SITE "systemctl start containerd"

# 4. 验证
kubectl get nodes --context=primary

# 5. 更新 DNS
aws route53 change-resource-record-sets \
    --hosted-zone-id Z1234567890 \
    --change-batch file://primary-record.json

echo "Failback completed"
```

---

## 8. 恢复时间目标 (RTO) 优化

### 8.1 RTO 分解

| 阶段 | 目标时间 | 优化措施 |
|------|----------|----------|
| **检测** | < 30s | 监控告警自动化 |
| **决策** | < 1min | 预设恢复策略 |
| **恢复配置** | < 3min | 配置备份自动化 |
| **恢复数据** | < 10min | 增量备份 + 快速存储 |
| **验证** | < 1min | 健康检查脚本 |
| **总目标** | **< 15min** | |

### 8.2 加速技术

```bash
# 使用 LVM 快照加速恢复
# 创建快照
lvcreate -L 50G -s -n containerd-snap /dev/vg00/lv-containerd

# 恢复时
umount /var/lib/containerd
lvconvert --merge /dev/vg00/containerd-snap
mount /dev/vg00/lv-containerd /var/lib/containerd

# 使用 Restic 增量备份
restic -r s3:/bucket/containerd init
restic -r s3:/bucket/containerd backup /var/lib/containerd
restic -r s3:/bucket/containerd restore latest --target /var/lib/containerd
```

---

**维护者**: Kudig Team | **许可证**: MIT