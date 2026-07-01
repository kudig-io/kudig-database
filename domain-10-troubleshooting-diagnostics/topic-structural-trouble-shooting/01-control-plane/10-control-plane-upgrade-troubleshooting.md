---
title: 控制平面升级迁移问题处理指南 [topic-structural-trouble-shooting]
description: 'title: 控制平面升级迁移问题处理指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- upgrade
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- helm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 控制平面升级迁移问题处理指南 是什么
- 如何 控制平面升级迁移问题处理指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 控制平面升级迁移问题处理指南 故障排查
- 控制平面升级迁移问题处理指南 排障步骤
trigger_keywords:
- 控制平面升级迁移问题处理指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- etcd-basics
created: "2026-05-23"
---

title: 控制平面升级迁移问题处理指南
description: '# 控制平面升级迁移问题处理指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
- [[Prometheus|prometheus]]
- helm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 控制平面升级迁移问题处理指南 是什么
- 如何 控制平面升级迁移问题处理指南
- 控制平面升级迁移问题处理指南 故障排查
- 控制平面升级迁移问题处理指南 排障步骤
trigger_keywords:
- 控制平面升级迁移问题处理指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 控制平面升级迁移问题处理指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 生产环境升级保障

## 问题现象与影响分析

### 典型升级问题现象

| 问题现象 | 典型报错 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| 升级过程中控制平面不可用 | `connection refused` | ⭐⭐⭐ 高 | P0 |
| API 版本不兼容 | `UnsupportedMediaType` | ⭐⭐⭐ 高 | P0 |
| 组件启动失败 | `CrashLoopBackOff` | ⭐⭐⭐ 高 | P0 |
| etcd 数据格式不兼容 | `etcdserver: mvcc: database space exceeded` | ⭐⭐⭐ 高 | P0 |
| 升级后功能异常 | `feature gate disabled` | ⭐⭐ 中 | P1 |
| 回滚失败 | `rollback not supported` | ⭐⭐⭐ 高 | P0 |
| 证书过期导致升级失败 | `certificate has expired` | ⭐⭐⭐ 高 | P0 |

### 升级前状态检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 升级前状态检查脚本

echo "=== Kubernetes 升级前状态检查 ==="

# 1. 集群版本检查
echo "1. 当前集群版本:"
kubectl version --short

# 2. 节点状态检查
echo "2. 节点状态检查:"
kubectl get nodes -o wide

# 3. 控制平面组件状态
echo "3. 控制平面组件状态:"
kubectl get pods -n kube-system -l tier=control-plane

# 4. etcd 集群健康检查
echo "4. etcd 集群健康检查:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health

# 5. 证书有效期检查
echo "5. 证书有效期检查:"
for cert in /etc/kubernetes/pki/*.crt; do
  if [ -f "$cert" ]; then
    expiry_date=$(openssl x509 -in "$cert" -noout -enddate | cut -d= -f2)
    days_left=$((($(date -d "$expiry_date" +%s) - $(date +%s)) / 86400))
    echo "  $cert: ${days_left} 天后过期"
  fi
done

# 6. 存储空间检查
echo "6. 存储空间检查:"
df -h /var/lib/etcd /var/lib/kubelet

# 7. 备份状态检查
echo "7. 备份状态检查:"
ls -la /var/backups/kubernetes/ 2>/dev/null || echo "  备份目录不存在"
```

## 排查方法与步骤

### 诊断原理说明

Kubernetes 升级过程涉及多个关键环节的风险：

1. **版本兼容性**：API 版本、功能开关、配置格式的变化
2. **数据迁移**：etcd 数据结构升级、存储格式转换
3. **证书管理**：证书轮换、CA 信任链维护
4. **组件协调**：控制平面组件间的版本依赖关系
5. **回滚机制**：升级失败后的恢复能力

### 升级问题诊断决策树

```
升级问题发生
    ├── 版本兼容性检查
    │   ├── API 版本支持
    │   ├── 功能开关配置
    │   ├── 配置文件格式
    │   └── 第三方组件兼容性
    ├── 数据迁移检查
    │   ├── etcd 版本兼容性
    │   ├── 数据格式转换
    │   ├── 存储空间充足性
    │   └── 备份完整性
    ├── 证书状态检查
    │   ├── 证书有效期
    │   ├── CA 信任链
    │   ├── 证书轮换机制
    │   └── 客户端证书同步
    └── 组件启动检查
        ├── Pod 启动状态
        ├── 容器日志分析
        ├── 健康检查状态
        └── 资源依赖关系
```

### 详细诊断命令

#### 1. 版本兼容性诊断

```bash
#!/bin/bash
# 版本兼容性诊断脚本

echo "=== 版本兼容性诊断 ==="

TARGET_VERSION="v1.32.0"
CURRENT_VERSION=$(kubectl version --short | grep Server | awk '{print $3}')

echo "当前版本: $CURRENT_VERSION"
echo "目标版本: $TARGET_VERSION"

# 1. 检查跳版本升级
echo "1. 版本跳跃检查:"
CURRENT_MINOR=$(echo $CURRENT_VERSION | cut -d. -f2)
TARGET_MINOR=$(echo $TARGET_VERSION | cut -d. -f2)
VERSION_JUMP=$((TARGET_MINOR - CURRENT_MINOR))

if [ $VERSION_JUMP -gt 1 ]; then
  echo "⚠ 警告: 跨版本升级 ($CURRENT_VERSION -> $TARGET_VERSION)"
  echo "建议逐版本升级以降低风险"
fi

# 2. 检查废弃的 API 版本
echo "2. 废弃 API 版本检查:"
DEPRECATED_APIS=$(kubectl api-versions | grep -E "(extensions/v1beta1|apps/v1beta1|apps/v1beta2)")
if [ -n "$DEPRECATED_APIS" ]; then
  echo "发现废弃的 API 版本:"
  echo "$DEPRECATED_APIS"
fi

# 3. 检查功能开关状态
echo "3. 功能开关兼容性检查:"
kubectl get --raw /metrics | grep feature | head -10

# 4. 第三方组件兼容性
echo "4. 第三方组件版本检查:"
helm list --all-namespaces 2>/dev/null || echo "Helm 未安装或无法访问"
```

#### 2. 数据迁移问题诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 数据迁移问题诊断脚本

echo "=== 数据迁移问题诊断 ==="

# 1. etcd 版本兼容性检查
echo "1. etcd 版本兼容性检查:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
ETCD_VERSION=$(kubectl exec -n kube-system $ETCD_POD -- etcd --version | head -1)
echo "当前 etcd 版本: $ETCD_VERSION"

# 2. etcd 数据库大小检查
echo "2. etcd 数据库大小检查:"
ETCD_SIZE=$(kubectl exec -n kube-system $ETCD_POD -- du -sh /var/lib/etcd/member/snap/db | cut -f1)
echo "etcd 数据库大小: $ETCD_SIZE"

# 检查是否接近配额限制
QUOTA_BYTES=$(kubectl exec -n kube-system $ETCD_POD -- ps aux | grep etcd | grep quota-backend-bytes | sed -E 's/.*quota-backend-bytes=([0-9]+).*/\1/')
if [ -n "$QUOTA_BYTES" ]; then
  CURRENT_BYTES=$(kubectl exec -n kube-system $ETCD_POD -- du -b /var/lib/etcd/member/snap/db | cut -f1)
  USAGE_PERCENT=$((CURRENT_BYTES * 100 / QUOTA_BYTES))
  echo "etcd 使用率: ${USAGE_PERCENT}%"
  if [ $USAGE_PERCENT -gt 80 ]; then
    echo "⚠ 警告: etcd 使用率过高，建议升级前清理数据"
  fi
fi

# 3. 数据备份完整性检查
echo "3. 数据备份检查:"
BACKUP_DIR="/var/backups/kubernetes"
if [ -d "$BACKUP_DIR" ]; then
  LATEST_BACKUP=$(ls -t $BACKUP_DIR/etcd-* 2>/dev/null | head -1)
  if [ -n "$LATEST_BACKUP" ]; then
    BACKUP_SIZE=$(du -h "$LATEST_BACKUP" | cut -f1)
    echo "最新备份: $LATEST_BACKUP ($BACKUP_SIZE)"
    
    # 验证备份完整性
    echo "验证备份完整性..."
    # 这里可以添加具体的备份验证逻辑
  else
    echo "❌ 未找到有效的 etcd 备份"
  fi
else
  echo "❌ 备份目录不存在: $BACKUP_DIR"
fi

# 4. 存储空间检查
echo "4. 存储空间检查:"
STORAGE_USED=$(df -h /var/lib/etcd | tail -1 | awk '{print $5}' | sed 's/%//')
if [ $STORAGE_USED -gt 80 ]; then
  echo "⚠ 警告: /var/lib/etcd 存储使用率 ${STORAGE_USED}%"
fi
```

#### 3. 证书问题诊断

```bash
#!/bin/bash
# 证书问题诊断脚本

echo "=== 证书问题诊断 ==="

# 1. 证书有效期检查
echo "1. 证书有效期检查:"
CERT_DIR="/etc/kubernetes/pki"
for cert_file in $CERT_DIR/*.crt; do
  if [ -f "$cert_file" ]; then
    subject=$(openssl x509 -in "$cert_file" -noout -subject | cut -d'=' -f2-)
    expiry_date=$(openssl x509 -in "$cert_file" -noout -enddate | cut -d'=' -f2)
    days_left=$((($(date -d "$expiry_date" +%s) - $(date +%s)) / 86400))
    
    echo "证书: $(basename $cert_file)"
    echo "  主题: $subject"
    echo "  过期时间: $expiry_date"
    echo "  剩余天数: $days_left"
    
    if [ $days_left -lt 30 ]; then
      echo "  ❌ 警告: 证书即将过期"
    elif [ $days_left -lt 0 ]; then
      echo "  ❌ 错误: 证书已过期"
    else
      echo "  ✓ 正常"
    fi
    echo ""
  fi
done

# 2. CA 证书一致性检查
echo "2. CA 证书一致性检查:"
CA_HASH=$(openssl x509 -in $CERT_DIR/ca.crt -noout -pubkey | openssl md5)
echo "CA 证书指纹: $CA_HASH"

# 检查各个组件使用的 CA 是否一致
for component in apiserver etcd front-proxy; do
  if [ -f "$CERT_DIR/$component-ca.crt" ]; then
    component_hash=$(openssl x509 -in $CERT_DIR/$component-ca.crt -noout -pubkey | openssl md5)
    if [ "$component_hash" != "$CA_HASH" ]; then
      echo "❌ $component-ca.crt 与主 CA 不一致"
    else
      echo "✓ $component-ca.crt 与主 CA 一致"
    fi
  fi
done

# 3. 证书SAN检查
echo "3. API Server 证书 SAN 检查:"
openssl x509 -in $CERT_DIR/apiserver.crt -noout -text | grep -A5 "Subject Alternative Name"

# 4. 证书轮换状态检查
echo "4. 证书轮换状态检查:"
CERTIFICATE_EVENTS=$(kubectl get events -n kube-system --field-selector involvedObject.kind=CertificateSigningRequest --sort-by=.lastTimestamp | tail -10)
if [ -n "$CERTIFICATE_EVENTS" ]; then
  echo "近期证书相关事件:"
  echo "$CERTIFICATE_EVENTS"
fi
```

## 解决方案与风险控制

### 版本兼容性问题解决

#### 方案一：逐版本升级策略

```bash
#!/bin/bash
# 逐版本升级脚本

CURRENT_VERSION="v1.30.0"
TARGET_VERSION="v1.32.0"

echo "=== 逐版本升级策略 ==="
echo "从 $CURRENT_VERSION 升级到 $TARGET_VERSION"

# 1. 确定升级路径
VERSION_PATH=("v1.30.0" "v1.31.0" "v1.32.0")

# 2. 逐版本升级函数
upgrade_to_version() {
  local target_version=$1
  echo "开始升级到 $target_version"
  
  # 备份当前状态
  echo "创建升级前备份..."
  mkdir -p /var/backups/kubernetes/pre-${target_version}-$(date +%Y%m%d_%H%M%S)
  
  # 执行升级
  echo "执行 kubeadm upgrade..."
  kubeadm upgrade plan $target_version
  kubeadm upgrade apply $target_version --yes
  
  # 验证升级结果
  echo "验证升级结果..."
  kubectl version --short
  kubectl get nodes
  
  # 等待组件稳定
  echo "等待组件稳定..."
  sleep 60
}

# 3. 执行逐版本升级
for version in "${VERSION_PATH[@]}"; do
  if "$version" > "$CURRENT_VERSION"; then
    upgrade_to_version $version
  fi
done

echo "升级完成！"
```

#### 方案二：API 版本迁移工具

```bash
#!/bin/bash
# API 版本迁移检查和修复工具

echo "=== API 版本迁移检查 ==="

# 1. 检查使用废弃 API 的资源
echo "1. 检查废弃 API 使用情况:"

# 检查 extensions/v1beta1
echo "检查 extensions/v1beta1 资源:"
kubectl get ingresses.extensions --all-namespaces 2>/dev/null && echo "发现 extensions/v1beta1 ingress 资源" || echo "未发现 extensions/v1beta1 ingress 资源"

# 检查 apps/v1beta1 和 apps/v1beta2
echo "检查 apps/v1beta1/v1beta2 资源:"
kubectl get deployments.apps --all-namespaces -o jsonpath='{range .items[*]}{.apiVersion}{" "}{.metadata.namespace}{" "}{.metadata.name}{"\n"}{end}' | grep -E "(v1beta1|v1beta2)" && echo "发现旧版本 deployment" || echo "未发现旧版本 deployment"

# 2. 自动迁移脚本
migrate_deprecated_resources() {
  echo "开始迁移废弃资源..."
  
  # 迁移 ingress 资源
  kubectl get ingresses.extensions --all-namespaces -o yaml | \
    sed 's/apiVersion: extensions\/v1beta1/apiVersion: networking.k8s.io\/v1/g' | \
    sed 's/kind: Ingress/kind: Ingress/g' > /tmp/migrated-ingresses.yaml
  
  # 迁移 deployment 资源
  kubectl get deployments.apps --all-namespaces -o yaml | \
    sed 's/apiVersion: apps\/v1beta1/apiVersion: apps\/v1/g' | \
    sed 's/apiVersion: apps\/v1beta2/apiVersion: apps\/v1/g' > /tmp/migrated-deployments.yaml
  
  echo "资源迁移文件已生成:"
  echo "  /tmp/migrated-ingresses.yaml"
  echo "  /tmp/migrated-deployments.yaml"
  echo "请审核后手动应用这些配置"
}

# 询问是否执行迁移
read -p "是否执行废弃资源迁移？(y/N): " -n 1 -r
echo
if $REPLY =~ ^[Yy]$; then
  migrate_deprecated_resources
fi
```

### 数据迁移问题解决

#### 方案一：etcd 数据清理和压缩

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# etcd 数据清理和压缩脚本

echo "=== etcd 数据清理和压缩 ==="

ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)

# 1. 检查 etcd 状态
echo "1. 检查 etcd 状态:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint status -w table

# 2. 执行数据压缩
echo "2. 执行数据压缩:"
REVISION=$(kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint status --write-out="json" | jq '.[0].Status.header.revision')
echo "当前修订版本: $REVISION"

kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt compact $REVISION

# 3. 执行碎片整理
echo "3. 执行碎片整理:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt defrag

# 4. 清理告警
echo "4. 清理 etcd 告警:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt alarm disarm

# 5. 验证清理结果
echo "5. 验证清理结果:"
kubectl exec -n kube-system $ETCD_POD -- du -sh /var/lib/etcd/member/snap/db
```

#### 方案二：完整的备份恢复流程

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 完整的 etcd 备份恢复流程

BACKUP_DIR="/var/backups/kubernetes"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== etcd 备份恢复流程 ==="

# 1. 创建完整备份
create_backup() {
  echo "1. 创建完整备份..."
  
  mkdir -p $BACKUP_DIR
  
  # etcd 备份
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
  kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt snapshot save /tmp/etcd-snapshot-${TIMESTAMP}.db
  
  # 复制备份到本地
  kubectl cp kube-system/$ETCD_POD:/tmp/etcd-snapshot-${TIMESTAMP}.db $BACKUP_DIR/etcd-snapshot-${TIMESTAMP}.db
  
  # 配置文件备份
  cp -r /etc/kubernetes $BACKUP_DIR/kubernetes-config-${TIMESTAMP}
  
  echo "备份完成: $BACKUP_DIR/etcd-snapshot-${TIMESTAMP}.db"
}

# 2. 验证备份完整性
verify_backup() {
  local backup_file=$1
  echo "2. 验证备份完整性: $backup_file"
  
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
  kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --write-out=table snapshot status /tmp/etcd-snapshot-${TIMESTAMP}.db
}

# 3. 恢复流程
restore_from_backup() {
  local backup_file=$1
  echo "3. 从备份恢复: $backup_file"
  
  # 停止 etcd
  systemctl stop etcd
  
  # 清理现有数据
  rm -rf /var/lib/etcd/member  # ⚠️ 删除系统/数据文件
  
  # 恢复数据
  ETCDCTL_API=3 etcdctl snapshot restore $backup_file \
    --data-dir=/var/lib/etcd \
    --initial-cluster=$(hostname)=https://$(hostname -i):2380 \
    --initial-cluster-token=etcd-cluster-1 \
    --initial-advertise-peer-urls=https://$(hostname -i):2380
  
  # 启动 etcd
  systemctl start etcd
  
  # 验证恢复
  echo "验证恢复状态..."
  sleep 30
  ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health
}

# 执行备份
create_backup

# 验证备份
LATEST_BACKUP=$(ls -t $BACKUP_DIR/etcd-snapshot-* 2>/dev/null | head -1)
if [ -n "$LATEST_BACKUP" ]; then
  verify_backup "$LATEST_BACKUP"
else
  echo "❌ 未找到备份文件"
  exit 1
fi

echo "备份验证完成，可用于恢复操作"
```

### 证书问题解决

#### 方案一：证书续期和轮换

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
#!/bin/bash
# 证书续期和轮换脚本

echo "=== 证书续期和轮换 ==="

# 1. 检查证书状态
echo "1. 检查证书状态:"
CERT_DIR="/etc/kubernetes/pki"
for cert in $CERT_DIR/*.crt; do
  if [ -f "$cert" ]; then
    days_left=$((($(openssl x509 -in "$cert" -noout -enddate | cut -d= -f2 | xargs -I{} date -d {} +%s) - $(date +%s)) / 86400))
    if [ $days_left -lt 30 ]; then
      echo "需要续期: $cert (${days_left} 天后过期)"
    fi
  fi
done

# 2. 自动续期函数
renew_certificates() {
  echo "2. 开始证书续期..."
  
  # 备份当前证书
  BACKUP_DIR="/var/backups/certificates/$(date +%Y%m%d_%H%M%S)"
  mkdir -p $BACKUP_DIR
  cp -r $CERT_DIR $BACKUP_DIR/
  
  # 使用 kubeadm 续期证书
  echo "使用 kubeadm 续期证书..."
  kubeadm certs renew all
  
  # 重启控制平面组件
  echo "重启控制平面组件..."
  systemctl restart kubelet
  
  # 等待组件重启完成
  sleep 60
  
  # 验证新证书
  echo "验证新证书..."
  for cert in $CERT_DIR/*.crt; do
    if [ -f "$cert" ]; then
      new_expiry=$(openssl x509 -in "$cert" -noout -enddate | cut -d= -f2)
      echo "  $(basename $cert): $new_expiry"
    fi
  done
}

# 3. 手动生成证书（当 kubeadm 不可用时）
generate_certificates_manual() {
  echo "3. 手动生成证书..."
  
  cd $CERT_DIR
  
  # 生成 CA 证书（如果需要）
  if [ ! -f "ca.crt" ] || [ ! -f "ca.key" ]; then
    echo "生成新的 CA 证书..."
    openssl genrsa -out ca.key 2048
    openssl req -x509 -new -nodes -key ca.key -subj "/CN=kubernetes" -days 3650 -out ca.crt
  fi
  
  # 生成 API Server 证书
  cat > apiserver.cnf << EOF
[req]
req_extensions = v3_req
distinguished_name = req_distinguished_name
[req_distinguished_name]
[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names
[alt_names]
DNS.1 = kubernetes
DNS.2 = kubernetes.default
DNS.3 = kubernetes.default.svc
DNS.4 = kubernetes.default.svc.cluster.local
IP.1 = 10.96.0.1
IP.2 = 127.0.0.1
IP.3 = $(hostname -i)
EOF
  
  openssl genrsa -out apiserver.key 2048
  openssl req -new -key apiserver.key -subj "/CN=kube-apiserver" -out apiserver.csr -config apiserver.cnf
  openssl x509 -req -in apiserver.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out apiserver.crt -days 365 -extensions v3_req -extfile apiserver.cnf
}

# 执行续期
read -p "是否执行证书续期？这将重启控制平面组件 (y/N): " -n 1 -r
echo
if $REPLY =~ ^[Yy]$; then
  renew_certificates
fi
```

#### 方案二：证书分发同步

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
#!/bin/bash
# 证书分发同步脚本

CONTROL_PLANE_NODES=("control-plane-01" "control-plane-02" "control-plane-03")
CERT_DIR="/etc/kubernetes/pki"

echo "=== 证书分发同步 ==="

# 1. 同步证书到其他控制平面节点
sync_certificates() {
  echo "1. 同步证书到控制平面节点..."
  
  for node in "${CONTROL_PLANE_NODES[@]:1}"; do  # 跳过第一个节点（主节点）
    echo "同步到节点: $node"
    
    # 创建远程目录
    ssh $node "sudo mkdir -p $CERT_DIR"
    
    # 同步证书文件
    rsync -avz --rsync-path="sudo rsync" $CERT_DIR/ $node:$CERT_DIR/
    
    # 验证同步结果
    ssh $node "sudo openssl x509 -in $CERT_DIR/ca.crt -noout -subject"
  done
}

# 2. 重启远程节点组件
restart_remote_components() {
  echo "2. 重启远程节点组件..."
  
  for node in "${CONTROL_PLANE_NODES[@]:1}"; do
    echo "重启节点 $node 上的组件..."
    ssh $node "sudo systemctl restart kubelet"
  done
}

# 3. 验证证书一致性
verify_certificate_consistency() {
  echo "3. 验证证书一致性..."
  
  PRIMARY_HASH=$(openssl x509 -in $CERT_DIR/ca.crt -noout -pubkey | openssl md5)
  
  for node in "${CONTROL_PLANE_NODES[@]}"; do
    REMOTE_HASH=$(ssh $node "sudo openssl x509 -in $CERT_DIR/ca.crt -noout -pubkey | openssl md5")
    if [ "$PRIMARY_HASH" = "$REMOTE_HASH" ]; then
      echo "✓ 节点 $node 证书一致"
    else
      echo "❌ 节点 $node 证书不一致"
    fi
  done
}

# 执行同步
sync_certificates
restart_remote_components
verify_certificate_consistency

echo "证书同步完成！"
```

## ⚠️ 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 逐版本升级 | ⭐⭐ 中 | 时间较长但风险较低 | 可随时停止升级 |
| etcd 数据压缩 | ⭐⭐ 中 | 短暂性能影响 | 监控集群状态 |
| 证书续期 | ⭐⭐⭐ 高 | 需要重启控制平面 | 使用备份证书恢复 |
| 跨版本升级 | ⭐⭐⭐ 高 | 可能导致不兼容 | 只能通过备份恢复 |

## 📊 升级验证与监控

### 升级后验证脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 升级后验证脚本

echo "=== Kubernetes 升级后验证 ==="

# 1. 版本验证
echo "1. 版本验证:"
kubectl version --short

# 2. 组件状态验证
echo "2. 组件状态验证:"
kubectl get pods -n kube-system -l tier=control-plane

# 3. 功能验证
echo "3. 核心功能验证:"
# 创建测试 deployment
kubectl create deployment test-upgrade --image=nginx:alpine
kubectl rollout status deployment/test-upgrade
kubectl delete deployment test-upgrade

# 4. 网络功能验证
echo "4. 网络功能验证:"
kubectl run test-pod --image=busybox --command -- sleep 3600
kubectl wait --for=condition=Ready pod/test-pod
kubectl exec test-pod -- nslookup kubernetes.default
kubectl delete pod test-pod

# 5. 存储功能验证
echo "5. 存储功能验证:"
kubectl apply -f - << EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: test-pvc
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
EOF

kubectl get pvc test-pvc
kubectl delete pvc test-pvc

echo "升级验证完成！"
```

### 升级监控告警配置

```yaml
# Prometheus 升级监控告警
groups:
- name: kubernetes.upgrade
  rules:
  - alert: UpgradeInProgress
    expr: kube_pod_labels{label_k8s_app_kubernetes_io_component="upgrade"} == 1
    for: 1m
    labels:
      severity: info
    annotations:
      summary: "Kubernetes 升级进行中"
      description: "检测到升级相关组件正在运行"

  - alert: ComponentUpgradeFailed
    expr: kube_pod_status_phase{phase="Failed"} == 1 and kube_pod_labels{label_component=~"kube-apiserver|kube-controller-manager|kube-scheduler"} == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "控制平面组件升级失败"
      description: "控制平面组件 {{ $labels.pod }} 升级失败"

  - alert: EtcdUpgradeIssue
    expr: etcd_server_has_leader == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "etcd 升级问题"
      description: "etcd 集群失去 leader，可能是升级过程中出现问题"

  - alert: CertificateExpiringDuringUpgrade
    expr: kube_cert_expiration_timestamp_seconds - time() < 86400 * 30
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "证书在升级期间即将过期"
      description: "证书将在30天内过期，建议在升级前处理"
```

## 📚 升级最佳实践

### 升级前准备清单

```yaml
# 升级前准备清单
preUpgradeChecklist:
  backups:
    - etcd 数据库备份
    - 配置文件备份
    - 证书备份
    - 应用配置备份
  
  compatibility:
    - 版本兼容性检查
    - API 版本检查
    - 第三方组件兼容性
    - 自定义资源定义检查
  
  resources:
    - 存储空间充足
    - 内存/CPU 资源预留
    - 网络带宽评估
    - 备用节点准备
  
  testing:
    - 测试环境验证
    - 升级流程演练
    - 回滚方案测试
    - 性能基准测试
```

### 升级执行计划模板

```bash
#!/bin/bash
# 升级执行计划模板

UPGRADE_VERSION="v1.32.0"
MAINTENANCE_WINDOW="2026-02-07 02:00:00"

echo "=== Kubernetes 升级执行计划 ==="
echo "目标版本: $UPGRADE_VERSION"
echo "维护窗口: $MAINTENANCE_WINDOW"

# 1. 升级前准备
echo "阶段1: 升级前准备 (预计 30 分钟)"
echo "  ▢ 执行备份脚本"
echo "  ▢ 检查集群状态"
echo "  ▢ 通知相关人员"
echo "  ▢ 准备回滚方案"

# 2. 控制平面升级
echo "阶段2: 控制平面升级 (预计 60 分钟)"
echo "  ▢ 升级第一个控制平面节点"
echo "  ▢ 验证控制平面功能"
echo "  ▢ 逐个升级其余控制平面节点"
echo "  ▢ 验证高可用状态"

# 3. 工作节点升级
echo "阶段3: 工作节点升级 (预计 120 分钟)"
echo "  ▢ 逐批升级工作节点"
echo "  ▢ 验证应用运行状态"
echo "  ▢ 监控性能指标"

# 4. 升级后验证
echo "阶段4: 升级后验证 (预计 30 分钟)"
echo "  ▢ 执行功能测试"
echo "  ▢ 验证监控告警"
echo "  ▢ 更新文档记录"
echo "  ▢ 通知升级完成"

echo ""
echo "风险评估:"
echo "  • 高风险操作: 控制平面升级"
echo "  • 中等风险操作: 工作节点升级"
echo "  • 低风险操作: 验证和监控"

echo ""
echo "回滚方案:"
echo "  1. 如遇严重问题，立即停止升级"
echo "  2. 使用备份恢复 etcd 数据"
echo "  3. 降级控制平面组件"
echo "  4. 恢复工作节点到原版本"
```

## 🔄 典型升级问题案例

### 案例一：跨版本升级导致 API 不兼容

**问题描述**：从 v1.28 直接升级到 v1.32，大量应用出现 API 不兼容错误。

**根本原因**：跳过了中间版本，某些 API 版本在目标版本中已被移除。

**解决方案**：
1. 立即暂停升级流程
2. 逐版本回退到稳定状态
3. 在每个版本中修复废弃 API 的使用
4. 重新执行逐版本升级策略

### 案例二：etcd 版本不兼容导致数据丢失

**问题描述**：升级 etcd 版本后，集群无法启动，数据似乎丢失。

**根本原因**：新版本 etcd 无法读取旧版本的数据格式，且没有正确执行数据迁移。

**解决方案**：
1. 使用最近的 etcd 备份恢复集群
2. 在兼容的环境中执行数据格式转换
3. 逐步升级 etcd 版本
4. 建立完善的备份和验证机制

## 📞 升级支持

**升级咨询服务**：
- Kubernetes 官方升级文档：https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/
- 版本发布说明：https://github.com/kubernetes/kubernetes/releases
- 社区支持论坛：https://discuss.kubernetes.io/

**紧急支持**：
- CNCF 认证 Kubernetes 服务商
- 企业级升级支持服务
- 24/7 技术支持热线

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/08-control-plane-performance-troubleshooting.md|08-control-plane-performance-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/09-control-plane-ha-troubleshooting.md|09-control-plane-ha-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting.md|01-apiserver-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/02-etcd-troubleshooting.md|02-etcd-troubleshooting]]

```