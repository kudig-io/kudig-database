# 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 升级迁移指南

---

## 目录

1. [升级策略规划](#1-升级策略规划)
2. [版本兼容性矩阵](#2-版本兼容性矩阵)
3. [控制平面升级流程](#3-控制平面升级流程)
4. [etcd升级指南](#4-etcd升级指南)
5. [零停机升级方案](#5-零停机升级方案)
6. [回滚策略](#6-回滚策略)
7. [迁移场景处理](#7-迁移场景处理)
8. [自动化升级工具](#8-自动化升级工具)
9. [升级验证清单](#9-升级验证清单)
10. [最佳实践总结](#10-最佳实践总结)

---

## 1. 升级策略规划

### 1.1 升级路径设计

```
升级路径规划:

┌─────────────────────────────────────────────────────────────────────────┐
│                        Upgrade Path Planning                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  小版本升级 (Minor Version): v1.X → v1.Y                               │
│  ├── 支持跳版本升级 (最多±3个版本)                                       │
│  ├── 控制平面优先升级                                                    │
│  ├── 自动处理大部分API变更                                               │
│  └── 通常无需应用改造                                                    │
│                                                                          │
│  大版本升级 (Major Version): v1.X → v2.X                               │
│  ├── 需要逐步升级                                                        │
│  ├── 可能涉及API移除                                                     │
│  ├── 需要应用适配                                                        │
│  └── 详细的迁移计划                                                      │
│                                                                          │
│  组件升级顺序:                                                           │
│  1. etcd集群                                                             │
│  2. API Server                                                          │
│  3. Controller Manager                                                  │
│  4. Scheduler                                                           │
│  5. 工作节点                                                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 升级风险评估

| 风险等级 | 风险类型 | 影响程度 | 缓解措施 |
|----------|----------|----------|----------|
| **高** | API移除 | 应用不可用 | 提前适配，充分测试 |
| **中** | 性能回归 | 用户体验下降 | 性能基准测试 |
| **中** | 配置变更 | 功能异常 | 配置迁移脚本 |
| **低** | Bug修复 | 功能改善 | 监控验证 |

---

## 2. 版本兼容性矩阵

### 2.1 Kubernetes版本兼容性

| 当前版本 | 支持升级到 | 注意事项 |
|----------|------------|----------|
| v1.25 | v1.26-v1.28 | 直接升级 |
| v1.26 | v1.27-v1.29 | 直接升级 |
| v1.27 | v1.28-v1.30 | 直接升级 |
| v1.28 | v1.29-v1.31 | 直接升级 |
| v1.29 | v1.30-v1.32 | 直接升级 |
| v1.30 | v1.31-v1.32 | 直接升级 |

### 2.2 组件版本依赖

```yaml
# 版本兼容性配置示例
version_compatibility:
  kubernetes: "1.30.x"
  etcd: "3.5.12+"        # 支持v1.30
  container_runtime: 
    containerd: "1.7.x"
    cri-o: "1.28.x"
  cni_plugins: "1.3.x"
  dns: "1.10.x"
```

---

## 3. 控制平面升级流程

### 3.1 升级前准备

```bash
#!/bin/bash
# 升级前检查脚本

echo "=== Pre-Upgrade Checklist ==="

# 1. 备份关键数据
backup_critical_data() {
    echo "Backing up etcd data..."
    ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-pre-upgrade-$(date +%Y%m%d).db \
        --endpoints=https://127.0.0.1:2379 \
        --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key
    
    echo "Backing up cluster configuration..."
    mkdir -p /backup/kube-config-$(date +%Y%m%d)
    cp -r /etc/kubernetes/* /backup/kube-config-$(date +%Y%m%d)/
}

# 2. 集群健康检查
check_cluster_health() {
    echo "Checking cluster health..."
    
    # 检查节点状态
    kubectl get nodes
    
    # 检查控制平面组件
    kubectl get pods -n kube-system
    
    # 检查关键应用
    kubectl get deployments --all-namespaces
    
    # 验证API Server响应
    kubectl get --raw=/healthz
}

# 3. 版本兼容性验证
verify_version_compatibility() {
    echo "Verifying version compatibility..."
    
    # 检查当前版本
    kubectl version --short
    
    # 验证目标版本支持
    TARGET_VERSION="v1.31.0"
    echo "Target version: $TARGET_VERSION"
    
    # 检查废弃API
    kubectl api-resources --verbs=list --namespaced -o name | \
        xargs -n 1 kubectl get --show-kind --ignore-not-found --all-namespaces
}

# 4. 资源容量评估
assess_resource_capacity() {
    echo "Assessing resource capacity..."
    
    # 检查节点资源
    kubectl top nodes
    
    # 检查Pod资源请求
    kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].resources.requests}{"\n"}{end}'
}

# 执行检查
backup_critical_data
check_cluster_health
verify_version_compatibility
assess_resource_capacity

echo "=== Pre-Upgrade Checks Complete ==="
```

### 3.2 升级执行步骤

```yaml
# 控制平面升级流程
upgrade_process:
  phase_1_etcd_upgrade:
    - backup_etcd_data: true
    - upgrade_etcd_version: "3.5.12"
    - verify_etcd_health: true
    - rolling_restart_etcd: true
    
  phase_2_control_plane_upgrade:
    - upgrade_api_server: true
    - upgrade_controller_manager: true
    - upgrade_scheduler: true
    - verify_control_plane_health: true
    
  phase_3_validation:
    - run_conformance_tests: true
    - verify_workload_functionality: true
    - performance_benchmarking: true
    - security_scanning: true
    
  phase_4_workload_upgrade:
    - node_drain_and_upgrade: true
    - application_compatibility_testing: true
    - gradual_rollout: true
```

---

## 4. etcd升级指南

### 4.1 etcd升级策略

```bash
#!/bin/bash
# etcd滚动升级脚本

ETCD_VERSION="3.5.12"
BACKUP_DIR="/backup/etcd"

# 1. 创建快照备份
create_backup() {
    echo "Creating etcd backup..."
    ETCDCTL_API=3 etcdctl snapshot save $BACKUP_DIR/etcd-backup-$(date +%Y%m%d-%H%M%S).db \
        --endpoints=https://127.0.0.1:2379 \
        --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key
}

# 2. 逐节点升级
upgrade_etcd_node() {
    local node=$1
    
    echo "Upgrading etcd on node: $node"
    
    # 暂停etcd服务
    ssh $node "systemctl stop etcd"
    
    # 备份数据目录
    ssh $node "cp -r /var/lib/etcd /var/lib/etcd.backup.$(date +%Y%m%d)"
    
    # 升级etcd二进制
    ssh $node "wget https://github.com/etcd-io/etcd/releases/download/v${ETCD_VERSION}/etcd-v${ETCD_VERSION}-linux-amd64.tar.gz"
    ssh $node "tar -xzf etcd-v${ETCD_VERSION}-linux-amd64.tar.gz"
    ssh $node "cp etcd-v${ETCD_VERSION}-linux-amd64/etcd* /usr/local/bin/"
    
    # 启动etcd服务
    ssh $node "systemctl start etcd"
    
    # 验证健康状态
    sleep 30
    ETCDCTL_API=3 etcdctl endpoint health --endpoints=https://$node:2379 \
        --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key
}

# 3. 执行滚动升级
perform_rolling_upgrade() {
    local nodes=("etcd-0" "etcd-1" "etcd-2")
    
    for node in "${nodes[@]}"; do
        upgrade_etcd_node $node
        
        # 验证集群健康
        ETCDCTL_API=3 etcdctl endpoint health --cluster \
            --cacert=/etc/kubernetes/pki/etcd/ca.crt \
            --cert=/etc/kubernetes/pki/etcd/server.crt \
            --key=/etc/kubernetes/pki/etcd/server.key
        
        echo "Waiting for cluster stabilization..."
        sleep 60
    done
}

# 执行升级
create_backup
perform_rolling_upgrade
```

---

## 5. 零停机升级方案

### 5.1 蓝绿部署策略

```
蓝绿升级架构:

┌─────────────────────────────────────────────────────────────────────────┐
│                        Blue-Green Upgrade Architecture                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  当前生产环境 (Blue)                    备用环境 (Green)                │
│  ┌─────────────────┐                   ┌─────────────────┐              │
│  │ Control Plane   │ ◄───────────────► │ Control Plane   │              │
│  │ v1.30          │                   │ v1.31          │              │
│  │                 │                   │                 │              │
│  │ API Server      │                   │ API Server      │              │
│  │ etcd Cluster    │                   │ etcd Cluster    │              │
│  │ Controllers     │                   │ Controllers     │              │
│  └─────────────────┘                   └─────────────────┘              │
│          │                                     │                          │
│          ▼                                     ▼                          │
│  ┌─────────────────┐                   ┌─────────────────┐              │
│  │   Load Balancer │                   │   Load Balancer │              │
│  │   (Active)      │                   │   (Standby)     │              │
│  └─────────────────┘                   └─────────────────┘              │
│          │                                     │                          │
│          ▼                                     ▼                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                        Worker Nodes                             │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │    │
│  │  │ Node-1  │ │ Node-2  │ │ Node-3  │ │ Node-4  │ │ Node-5  │   │    │
│  │  │ v1.30   │ │ v1.30   │ │ v1.30   │ │ v1.30   │ │ v1.30   │   │    │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘   │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  升级流程:                                                               │
│  1. 在Green环境部署新版本控制平面                                        │
│  2. 验证Green环境功能正常                                                │
│  3. 将流量切换到Green环境                                                │
│  4. 逐步升级Worker节点                                                   │
│  5. 如有问题可快速回滚到Blue环境                                         │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 渐进式升级

```yaml
# 渐进式升级配置
progressive_upgrade:
  strategy: "canary"
  canary_percentage: 10
  validation_steps:
    - health_check: true
    - performance_test: true
    - application_test: true
    - security_scan: true
  rollback_conditions:
    - error_rate > 1%
    - latency_increase > 50%
    - resource_usage_spike > 200%
```

---

## 6. 回滚策略

### 6.1 快速回滚机制

```bash
#!/bin/bash
# 快速回滚脚本

ROLLBACK_VERSION="v1.30.0"
BACKUP_TIMESTAMP=$(date +%Y%m%d-%H%M%S)

# 1. 回滚前状态检查
pre_rollback_check() {
    echo "Performing pre-rollback checks..."
    
    # 检查备份可用性
    if [ ! -f "/backup/etcd-pre-upgrade.db" ]; then
        echo "ERROR: No etcd backup found!"
        exit 1
    fi
    
    # 检查回滚版本二进制
    if [ ! -f "/usr/local/bin/kube-apiserver-${ROLLBACK_VERSION}" ]; then
        echo "ERROR: Rollback binaries not available!"
        exit 1
    fi
}

# 2. etcd回滚
rollback_etcd() {
    echo "Rolling back etcd..."
    
    # 停止当前etcd
    systemctl stop etcd
    
    # 恢复备份数据
    ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-pre-upgrade.db \
        --data-dir=/var/lib/etcd-restored-${BACKUP_TIMESTAMP} \
        --name=etcd-0 \
        --initial-cluster=etcd-0=https://localhost:2380 \
        --initial-cluster-token=etcd-cluster-1 \
        --initial-advertise-peer-urls=https://localhost:2380
    
    # 替换数据目录
    mv /var/lib/etcd /var/lib/etcd-failed-${BACKUP_TIMESTAMP}
    mv /var/lib/etcd-restored-${BACKUP_TIMESTAMP} /var/lib/etcd
    
    # 启动etcd
    systemctl start etcd
}

# 3. 控制平面回滚
rollback_control_plane() {
    echo "Rolling back control plane..."
    
    # 回滚API Server
    cp /etc/kubernetes/manifests/kube-apiserver.yaml.backup \
       /etc/kubernetes/manifests/kube-apiserver.yaml
    
    # 回滚Controller Manager
    cp /etc/kubernetes/manifests/kube-controller-manager.yaml.backup \
       /etc/kubernetes/manifests/kube-controller-manager.yaml
    
    # 回滚Scheduler
    cp /etc/kubernetes/manifests/kube-scheduler.yaml.backup \
       /etc/kubernetes/manifests/kube-scheduler.yaml
       
    # 等待组件重启
    sleep 60
}

# 4. 验证回滚成功
verify_rollback() {
    echo "Verifying rollback success..."
    
    # 检查组件状态
    kubectl get componentstatuses
    
    # 验证API Server功能
    kubectl get nodes
    
    # 检查应用状态
    kubectl get deployments --all-namespaces
}

# 执行回滚
pre_rollback_check
rollback_etcd
rollback_control_plane
verify_rollback

echo "Rollback completed successfully!"
```

---

## 7. 迁移场景处理

### 7.1 云厂商迁移

```yaml
# 云厂商迁移配置
cloud_migration:
  source_provider: "aws"
  target_provider: "gcp"
  migration_strategy: "lift-and-shift"
  data_migration:
    etcd_backup_restore: true
    persistent_volumes: true
    load_balancers: true
  network_migration:
    vpc_peering: true
    dns_migration: true
    certificate_migration: true
```

### 7.2 版本跳跃迁移

```bash
#!/bin/bash
# 跨版本迁移脚本

SOURCE_VERSION="v1.25.0"
TARGET_VERSION="v1.31.0"

# 1. 版本兼容性检查
check_version_compatibility() {
    echo "Checking version compatibility from $SOURCE_VERSION to $TARGET_VERSION"
    
    # 检查中间版本要求
    local intermediate_versions=("v1.26.0" "v1.27.0" "v1.28.0" "v1.29.0" "v1.30.0")
    
    for version in "${intermediate_versions[@]}"; do
        echo "Checking compatibility with $version..."
        # 执行兼容性检查逻辑
    done
}

# 2. 分阶段升级
perform_staged_upgrade() {
    local versions=("v1.26.0" "v1.27.0" "v1.28.0" "v1.29.0" "v1.30.0" "v1.31.0")
    
    for version in "${versions[@]}"; do
        echo "Upgrading to $version..."
        # 执行单版本升级
        upgrade_to_version $version
        
        # 验证升级结果
        verify_version $version
        
        # 等待稳定
        sleep 300
    done
}

# 3. API迁移处理
handle_api_migrations() {
    echo "Handling API migrations..."
    
    # 检查废弃API使用情况
    kubectl api-resources --verbs=list --namespaced -o name | \
        xargs -n 1 kubectl get --show-kind --ignore-not-found --all-namespaces > \
        /tmp/current-api-usage.txt
    
    # 生成迁移报告
    echo "API migration report generated at /tmp/api-migration-report.txt"
}
```

---

## 8. 自动化升级工具

### 8.1 kubeadm升级

```bash
# 使用kubeadm进行升级
#!/bin/bash

NEW_VERSION="v1.31.0"

# 1. 升级控制平面
upgrade_control_plane() {
    echo "Upgrading control plane to $NEW_VERSION"
    
    # 升级第一个控制平面节点
    kubeadm upgrade plan $NEW_VERSION
    kubeadm upgrade apply $NEW_VERSION --certificate-renewal=true --dry-run=false
    
    # 升级其他控制平面节点
    for node in control-plane-2 control-plane-3; do
        ssh $node "kubeadm upgrade node"
    done
}

# 2. 升级工作节点
upgrade_worker_nodes() {
    echo "Upgrading worker nodes..."
    
    kubectl get nodes --no-headers | awk '{print $1}' | grep -v control-plane | \
    while read node; do
        echo "Draining node: $node"
        kubectl drain $node --ignore-daemonsets --delete-emptydir-data
        
        echo "Upgrading node: $node"
        ssh $node "apt-mark unhold kubelet kubeadm kubectl"
        ssh $node "apt-get update && apt-get install -y kubelet=$NEW_VERSION-00 kubeadm=$NEW_VERSION-00 kubectl=$NEW_VERSION-00"
        ssh $node "apt-mark hold kubelet kubeadm kubectl"
        ssh $node "systemctl daemon-reload && systemctl restart kubelet"
        
        echo "Uncordoning node: $node"
        kubectl uncordon $node
    done
}

# 3. 验证升级结果
verify_upgrade() {
    echo "Verifying upgrade..."
    kubectl version --short
    kubectl get nodes
    kubectl get componentstatuses
}

upgrade_control_plane
upgrade_worker_nodes
verify_upgrade
```

### 8.2 自定义升级Operator

```yaml
# 升级Operator配置
apiVersion: upgrade.k8s.io/v1
kind: UpgradePlan
metadata:
  name: cluster-upgrade-plan
spec:
  targetVersion: "1.31.0"
  upgradeStrategy:
    type: "RollingUpdate"
    maxUnavailable: 1
  preflightChecks:
    - type: "ClusterHealth"
      timeout: "300s"
    - type: "VersionCompatibility"
      timeout: "60s"
    - type: "ResourceCapacity"
      timeout: "120s"
  phases:
  - name: "Backup"
    steps:
    - name: "etcd-backup"
      action: "etcdctl snapshot save"
    - name: "config-backup"
      action: "cp -r /etc/kubernetes /backup"
  - name: "ControlPlane"
    steps:
    - name: "upgrade-etcd"
      action: "upgrade etcd to 3.5.12"
    - name: "upgrade-apiserver"
      action: "upgrade kube-apiserver to 1.31.0"
    - name: "upgrade-controllers"
      action: "upgrade kube-controller-manager to 1.31.0"
    - name: "upgrade-scheduler"
      action: "upgrade kube-scheduler to 1.31.0"
  - name: "Validation"
    steps:
    - name: "health-check"
      action: "kubectl get componentstatuses"
    - name: "conformance-test"
      action: "sonobuoy run --mode=quick"
```

---

## 9. 升级验证清单

```yaml
# 升级验证清单
upgrade_validation_checklist:
  pre_upgrade:
    - [x] 备份etcd数据
    - [x] 备份集群配置
    - [x] 验证当前集群健康状态
    - [x] 检查版本兼容性
    - [x] 评估资源容量
    - [x] 准备回滚方案
    - [x] 通知相关团队
    
  during_upgrade:
    - [ ] 监控控制平面组件状态
    - [ ] 跟踪API Server响应时间
    - [ ] 监控etcd集群健康
    - [ ] 验证控制器功能
    - [ ] 检查调度器性能
    - [ ] 监控关键应用状态
    
  post_upgrade:
    - [x] 验证集群版本
    - [x] 检查所有组件运行状态
    - [x] 运行端到端测试
    - [x] 验证应用功能
    - [x] 性能基准测试
    - [x] 安全扫描
    - [x] 更新文档
    - [x] 团队培训
```

---

## 10. 最佳实践总结

### 10.1 升级黄金法则

1. **Always Backup First** - 永远先备份
2. **Test in Staging** - 在预发环境充分测试
3. **Upgrade Incrementally** - 增量升级，避免跳跃
4. **Monitor Continuously** - 持续监控升级过程
5. **Have Rollback Ready** - 准备好快速回滚方案
6. **Communicate Early** - 提前沟通升级计划
7. **Verify Thoroughly** - 全面验证升级结果

### 10.2 升级时间窗口建议

| 集群规模 | 推荐升级时间 | 预估耗时 |
|----------|-------------|----------|
| 小型(<50节点) | 业务低峰期 | 1-2小时 |
| 中型(50-200节点) | 维护窗口 | 2-4小时 |
| 大型(200+节点) | 计划性停机 | 4-8小时 |

通过遵循这些升级和迁移策略，可以最大程度降低升级风险，确保Kubernetes集群的平稳演进。