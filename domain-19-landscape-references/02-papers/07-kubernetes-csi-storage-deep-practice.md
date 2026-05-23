---
title: Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface Deep Practice Guide)
description: '# Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface Deep Practice Guide)'
category: papers
tags:
- k8s
- papers
- research
- kubelet
- controller-manager
- prometheus
- statefulset
- job
- rbac
- rag
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- 架构师
- 技术决策者
- 研究员
estimated_read_time: 5min
intent_queries:
- Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface Deep Practice Guide) 是什么
- 如何 Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface Deep Practice Guide)
- Kubernetes 19 papers 最佳实践
trigger_keywords:
- Kubernetes
- 容器存储接口
- CSI
- 深度实践指南
- Container
- Storage
- Interface
- Deep
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md
  label: '故障树: csi'
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface Deep Practice Guide)

> **作者**: 存储架构专家 | **版本**: v1.5 | **更新时间**: 2026-03-03
> **适用场景**: 企业级存储架构设计 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文档深入探讨了Kubernetes容器存储接口(CSI)的架构设计、实现原理和最佳实践，基于多种存储后端的生产实践经验，提供从基础概念到高级特性的完整技术指南，帮助企业构建高效、可靠的存储解决方案。

<!-- chunk: 1. CSI 架构与原理 -->## 1. CSI 架构与原理

#<!-- chunk: 1.1 CSI 核心组件 -->## 1.1 CSI 核心组件

```yaml
CSI架构组件:
  CSI Driver:
    - Node Plugin: 运行在每个节点上，负责卷挂载/卸载
    - Controller Plugin: 负责卷生命周期管理
    - Identity Service: 提供驱动标识信息
  
  Kubernetes组件:
    - kubelet: 发起卷操作请求
    - kube-controller-manager: 运行卷控制器
    - csi-provisioner: 动态存储卷创建
    - csi-attacher: 卷挂载管理
    - csi-resizer: 卷大小调整
```

#<!-- chunk: 1.2 CSI接口详解 -->## 1.2 CSI接口详解

```go
// CSI核心接口定义
type IdentityServer interface {
    GetPluginInfo(context.Context, *GetPluginInfoRequest) (*GetPluginInfoResponse, error)
    GetPluginCapabilities(context.Context, *GetPluginCapabilitiesRequest) (*GetPluginCapabilitiesResponse, error)
    Probe(context.Context, *ProbeRequest) (*ProbeResponse, error)
}

type ControllerServer interface {
    CreateVolume(context.Context, *CreateVolumeRequest) (*CreateVolumeResponse, error)
    DeleteVolume(context.Context, *DeleteVolumeRequest) (*DeleteVolumeResponse, error)
    ControllerPublishVolume(context.Context, *ControllerPublishVolumeRequest) (*ControllerPublishVolumeResponse, error)
    ControllerUnpublishVolume(context.Context, *ControllerUnpublishVolumeRequest) (*ControllerUnpublishVolumeResponse, error)
    ValidateVolumeCapabilities(context.Context, *ValidateVolumeCapabilitiesRequest) (*ValidateVolumeCapabilitiesResponse, error)
    ListVolumes(context.Context, *ListVolumesRequest) (*ListVolumesResponse, error)
    GetCapacity(context.Context, *GetCapacityRequest) (*GetCapacityResponse, error)
    ControllerGetCapabilities(context.Context, *ControllerGetCapabilitiesRequest) (*ControllerGetCapabilitiesResponse, error)
    CreateSnapshot(context.Context, *CreateSnapshotRequest) (*CreateSnapshotResponse, error)
    DeleteSnapshot(context.Context, *DeleteSnapshotRequest) (*DeleteSnapshotResponse, error)
    ListSnapshots(context.Context, *ListSnapshotsRequest) (*ListSnapshotsResponse, error)
    ControllerExpandVolume(context.Context, *ControllerExpandVolumeRequest) (*ControllerExpandVolumeResponse, error)
}

type NodeServer interface {
    NodeStageVolume(context.Context, *NodeStageVolumeRequest) (*NodeStageVolumeResponse, error)
    NodeUnstageVolume(context.Context, *NodeUnstageVolumeRequest) (*NodeUnstageVolumeResponse, error)
    NodePublishVolume(context.Context, *NodePublishVolumeRequest) (*NodePublishVolumeResponse, error)
    NodeUnpublishVolume(context.Context, *NodeUnpublishVolumeRequest) (*NodeUnpublishVolumeResponse, error)
    NodeGetVolumeStats(context.Context, *NodeGetVolumeStatsRequest) (*NodeGetVolumeStatsResponse, error)
    NodeExpandVolume(context.Context, *NodeExpandVolumeRequest) (*NodeExpandVolumeResponse, error)
    NodeGetCapabilities(context.Context, *NodeGetCapabilitiesRequest) (*NodeGetCapabilitiesResponse, error)
    NodeGetInfo(context.Context, *NodeGetInfoRequest) (*NodeGetInfoResponse, error)
}
```

<!-- chunk: 2. 存储驱动开发实践 -->## 2. 存储驱动开发实践

#<!-- chunk: 2.1 自定义CSI驱动开发 -->## 2.1 自定义CSI驱动开发

```go
// 自定义CSI驱动示例
package main

import (
    "context"
    "fmt"
    "net"
    "os"
    
    "google.golang.org/grpc"
    "github.com/container-storage-interface/spec/lib/go/csi"
    "k8s.io/klog/v2"
)

const (
    driverName = "custom-csi-driver"
    vendorVersion = "1.0.0"
)

type Driver struct {
    name    string
    nodeID  string
    addr    string
    server  *grpc.Server
    controller *Controller
    node      *Node
}

func NewDriver(nodeID, endpoint string) *Driver {
    return &Driver{
        name:   driverName,
        nodeID: nodeID,
        addr:   endpoint,
    }
}

func (d *Driver) Run() {
    s := strings.Split(d.addr, "://")
    if len(s) < 2 {
        klog.Fatalf("Invalid endpoint: %v", d.addr)
    }

    endpoint := s[1]
    switch s[0] {
    case "unix":
        if err := os.Remove(endpoint); err != nil && !os.IsNotExist(err) {
            klog.Fatalf("Failed to remove endpoint socket: %v", err)
        }
    case "tcp":
        // TCP连接处理
    default:
        klog.Fatalf("Unsupported protocol: %s", s[0])
    }

    lis, err := net.Listen(s[0], endpoint)
    if err != nil {
        klog.Fatalf("Failed to listen: %v", err)
    }

    d.server = grpc.NewServer(grpc.UnaryInterceptor(logGRPC))
    csi.RegisterIdentityServer(d.server, d)
    csi.RegisterControllerServer(d.server, d.controller)
    csi.RegisterNodeServer(d.server, d.node)

    klog.Infof("Listening for connections on address: %s", d.addr)
    d.server.Serve(lis)
}

// Identity Server 实现
func (d *Driver) GetPluginInfo(ctx context.Context, req *csi.GetPluginInfoRequest) (*csi.GetPluginInfoResponse, error) {
    return &csi.GetPluginInfoResponse{
        Name:          d.name,
        VendorVersion: vendorVersion,
    }, nil
}

func (d *Driver) GetPluginCapabilities(ctx context.Context, req *csi.GetPluginCapabilitiesRequest) (*csi.GetPluginCapabilitiesResponse, error) {
    return &csi.GetPluginCapabilitiesResponse{
        Capabilities: []*csi.PluginCapability{
            {
                Type: &csi.PluginCapability_Service_{
                    Service: &csi.PluginCapability_Service{
                        Type: csi.PluginCapability_Service_CONTROLLER_SERVICE,
                    },
                },
            },
            {
                Type: &csi.PluginCapability_VolumeExpansion_{
                    VolumeExpansion: &csi.PluginCapability_VolumeExpansion{
                        Type: csi.PluginCapability_VolumeExpansion_ONLINE,
                    },
                },
            },
        },
    }, nil
}

func (d *Driver) Probe(ctx context.Context, req *csi.ProbeRequest) (*csi.ProbeResponse, error) {
    return &csi.ProbeResponse{}, nil
}
```

#<!-- chunk: 2.2 存储类配置优化 -->## 2.2 存储类配置优化

```yaml
# 高性能存储类配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: high-performance-ssd
provisioner: custom-csi-driver
parameters:
  # 性能参数
  iops: "3000"
  throughput: "125"
  # 优化参数
  fsType: "ext4"
  encryption: "true"
  replication: "3"
  # 缓存策略
  cachePolicy: "writeback"
  blockSize: "4k"
  # 网络参数
  mountOptions: "discard,noatime"
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
reclaimPolicy: Retain
```

<!-- chunk: 3. 高级存储特性 -->## 3. 高级存储特性

#<!-- chunk: 3.1 快照与克隆 -->## 3.1 快照与克隆

```yaml
# 存储快照定义
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: database-snapshot
  namespace: production
spec:
  volumeSnapshotClassName: custom-snapshot-class
  source:
    persistentVolumeClaimName: database-pvc

---
# 快照类配置
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: custom-snapshot-class
driver: custom-csi-driver
deletionPolicy: Delete
parameters:
  # 快照保留策略
  retentionDays: "30"
  # 压缩策略
  compression: "true"
  # 加密选项
  encryption: "aes-256"
```

```go
// 快照管理器实现
type SnapshotManager struct {
    client    snapshotter.Snapshotter
    recorder  record.EventRecorder
    metrics   *SnapshotMetrics
}

func (sm *SnapshotManager) CreateSnapshot(req *csi.CreateSnapshotRequest) (*csi.CreateSnapshotResponse, error) {
    // 验证请求参数
    if err := sm.validateSnapshotRequest(req); err != nil {
        return nil, status.Error(codes.InvalidArgument, err.Error())
    }

    // 检查快照是否存在
    existing, err := sm.getExistingSnapshot(req.GetName())
    if err != nil {
        return nil, status.Errorf(codes.Internal, "failed to check existing snapshot: %v", err)
    }

    if existing != nil {
        // 返回现有快照
        return &csi.CreateSnapshotResponse{
            Snapshot: existing,
        }, nil
    }

    // 创建新快照
    snapshot, err := sm.createBackendSnapshot(req)
    if err != nil {
        return nil, status.Errorf(codes.Internal, "failed to create backend snapshot: %v", err)
    }

    // 记录快照元数据
    if err := sm.recordSnapshotMetadata(snapshot); err != nil {
        klog.Warningf("Failed to record snapshot metadata: %v", err)
    }

    return &csi.CreateSnapshotResponse{
        Snapshot: snapshot,
    }, nil
}

func (sm *SnapshotManager) validateSnapshotRequest(req *csi.CreateSnapshotRequest) error {
    if req.GetName() == "" {
        return fmt.Errorf("snapshot name cannot be empty")
    }

    if req.GetSourceVolumeId() == "" {
        return fmt.Errorf("source volume ID cannot be empty")
    }

    // 验证参数
    for key, value := range req.Parameters {
        if !isValidParameter(key, value) {
            return fmt.Errorf("invalid parameter: %s=%s", key, value)
        }
    }

    return nil
}
```

#<!-- chunk: 3.2 卷克隆与复制 -->## 3.2 卷克隆与复制

```yaml
# 卷克隆配置
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cloned-volume
  namespace: production
spec:
  dataSource:
    name: database-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
  storageClassName: high-performance-ssd
```

<!-- chunk: 4. 性能优化策略 -->## 4. 性能优化策略

#<!-- chunk: 4.1 存储性能基准测试 -->## 4.1 存储性能基准测试

```bash
#!/bin/bash
# storage-performance-test.sh

# 存储性能测试脚本
TEST_SIZE="10G"
TEST_FILE="/mnt/storage/testfile"
RESULTS_DIR="/tmp/storage-test-results"

mkdir -p $RESULTS_DIR

echo "=== 存储性能测试开始 ==="

# 1. 顺序写入测试
echo "1. 顺序写入测试..."
dd if=/dev/zero of=$TEST_FILE bs=1M count=$((TEST_SIZE%1024)) oflag=direct 2>&1 | tee $RESULTS_DIR/write_sequential.txt

# 2. 顺序读取测试
echo "2. 顺序读取测试..."
dd if=$TEST_FILE of=/dev/null bs=1M iflag=direct 2>&1 | tee $RESULTS_DIR/read_sequential.txt

# 3. 随机写入测试
echo "3. 随机写入测试..."
fio --name=randwrite --rw=randwrite --bs=4k --size=$TEST_SIZE --numjobs=4 --runtime=60 --time_based --direct=1 --group_reporting --output=$RESULTS_DIR/randwrite.json

# 4. 随机读取测试
echo "4. 随机读取测试..."
fio --name=randread --rw=randread --bs=4k --size=$TEST_SIZE --numjobs=4 --runtime=60 --time_based --direct=1 --group_reporting --output=$RESULTS_DIR/randread.json

# 5. 混合读写测试
echo "5. 混合读写测试..."
fio --name=mixed --rw=rw --rwmixread=70 --bs=4k --size=$TEST_SIZE --numjobs=2 --runtime=60 --time_based --direct=1 --group_reporting --output=$RESULTS_DIR/mixed.json

echo "=== 存储性能测试完成 ==="
echo "结果保存在: $RESULTS_DIR"
```

#<!-- chunk: 4.2 存储缓存策略 -->## 4.2 存储缓存策略

```yaml
# 缓存优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: storage-cache-config
  namespace: kube-system
data:
  cache-config.yaml: |
    # 缓存策略配置
    cacheStrategies:
      - name: read-through
        description: "读取时缓存"
        ttl: 300s
        maxSize: 100MB
      
      - name: write-back
        description: "写入时异步更新"
        flushInterval: 30s
        batchSize: 100
      
      - name: write-through
        description: "写入时立即更新"
        syncWrites: true
    
    # 缓存分区配置
    cachePartitions:
      - name: metadata-cache
        size: 10GB
        evictionPolicy: LRU
        ttl: 600s
      
      - name: data-cache
        size: 50GB
        evictionPolicy: LFU
        ttl: 1800s
```

<!-- chunk: 5. 监控与问题排除 -->## 5. 监控与问题排除

#<!-- chunk: 5.1 存储性能监控 -->## 5.1 存储性能监控

```yaml
# 存储监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: csi-driver-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: custom-csi-driver
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'csi_(.*)'
      targetLabel: __name__
---
apiVersion: v1
kind: Service
metadata:
  name: csi-driver-metrics
  namespace: kube-system
  labels:
    app: custom-csi-driver
spec:
  ports:
  - name: metrics
    port: 8080
    protocol: TCP
    targetPort: 8080
  selector:
    app: custom-csi-driver
```

#<!-- chunk: 5.2 关键监控指标 -->## 5.2 关键监控指标

```prometheus
# CSI存储监控指标
# 卷操作指标
csi_operations_total{driver="custom-csi-driver", operation="create_volume", result="success"}
csi_operations_duration_seconds_sum{driver="custom-csi-driver", operation="create_volume"}
csi_operations_duration_seconds_count{driver="custom-csi-driver", operation="create_volume"}

# 存储性能指标
csi_storage_capacity_bytes{driver="custom-csi-driver", node="node-1", type="available"}
csi_storage_used_bytes{driver="custom-csi-driver", node="node-1"}

# 错误指标
csi_operations_errors_total{driver="custom-csi-driver", operation="create_volume"}

# 性能指标
csi_volume_read_bytes_total
csi_volume_write_bytes_total
csi_volume_read_ops_total
csi_volume_write_ops_total
```

#<!-- chunk: 5.3 问题排除工具 -->## 5.3 问题排除工具

```bash
#!/bin/bash
# csi-troubleshooting.sh

# CSI故障诊断脚本
echo "=== CSI故障诊断开始 ==="

# 1. 检查CSI驱动状态
echo "1. 检查CSI驱动状态:"
kubectl get csidrivers
kubectl get csinodes
kubectl get csistoragecapacities -A

# 2. 检查CSI控制器状态
echo "2. 检查CSI控制器状态:"
kubectl get pods -n kube-system -l app=csi-controller -o wide

# 3. 检查节点CSI插件状态
echo "3. 检查节点CSI插件状态:"
kubectl get pods -n kube-system -l app=csi-node -o wide

# 4. 检查存储类配置
echo "4. 检查存储类配置:"
kubectl get storageclasses -o wide

# 5. 检查PVC状态
echo "5. 检查PVC状态:"
kubectl get pvc --all-namespaces -o wide

# 6. 检查事件日志
echo "6. 检查相关事件:"
kubectl get events --sort-by='.lastTimestamp' --field-selector involvedObject.kind=PersistentVolumeClaim -A

# 7. 检查控制器日志
echo "7. 检查控制器日志:"
CONTROLLER_POD=$(kubectl get pods -n kube-system -l app=csi-controller -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ ! -z "$CONTROLLER_POD" ]; then
    kubectl logs -n kube-system $CONTROLLER_POD --since=1h
fi

# 8. 检查节点插件日志
echo "8. 检查节点插件日志:"
NODE_POD=$(kubectl get pods -n kube-system -l app=csi-node -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ ! -z "$NODE_POD" ]; then
    kubectl logs -n kube-system $NODE_POD --since=1h
fi

echo "=== CSI故障诊断完成 ==="
```

<!-- chunk: 6. 安全与合规 -->## 6. 安全与合规

#<!-- chunk: 6.1 存储加密配置 -->## 6.1 存储加密配置

```yaml
# 存储加密配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: storage-encryption-config
  namespace: kube-system
data:
  encryption-config.yaml: |
    # 加密配置
    encryptionProviders:
      - name: aes-256-gcm
        keySize: 256
        blockSize: 16
        mode: gcm
        keyRotation:
          interval: 30d
          algorithm: sha256
      
      - name: aes-128-gcm
        keySize: 128
        blockSize: 16
        mode: gcm
        keyRotation:
          interval: 60d
          algorithm: sha256
    
    # 密钥管理配置
    keyManagement:
      provider: kms
      region: us-west-2
      keyAlias: alias/storage-encryption-key
      rotationPeriod: 365d
```

#<!-- chunk: 6.2 访问控制策略 -->## 6.2 访问控制策略

```yaml
# 存储访问控制
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: storage-admin
rules:
- apiGroups: ["storage.k8s.io"]
  resources: ["storageclasses", "csidrivers", "csinodes"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["persistentvolumes", "persistentvolumeclaims"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["snapshot.storage.k8s.io"]
  resources: ["volumesnapshots", "volumesnapshotclasses", "volumesnapshotcontents"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: storage-admin-binding
subjects:
- kind: Group
  name: system:storage-admins
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: storage-admin
  apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 7. 最佳实践总结 -->## 7. 最佳实践总结

#<!-- chunk: 7.1 存储设计原则 -->## 7.1 存储设计原则

```markdown
<!-- chunk: 💾 存储设计最佳实践 -->## 💾 存储设计最佳实践

#<!-- chunk: 1. 性能导向设计 -->## 1. 性能导向设计
- 根据应用特性选择合适的存储类型
- 合理配置IOPS和吞吐量参数
- 优化缓存策略和预读设置

#<!-- chunk: 2. 高可用性保障 -->## 2. 高可用性保障
- 实施多副本存储策略
- 配置跨可用区部署
- 建立完善的备份恢复机制

#<!-- chunk: 3. 安全合规保障 -->## 3. 安全合规保障
- 实施端到端加密
- 配置访问控制策略
- 建立审计日志机制

#<!-- chunk: 4. 成本效益优化 -->## 4. 成本效益优化
- 合理选择存储层级
- 实施自动伸缩策略
- 优化存储利用率
```

#<!-- chunk: 7.2 实施检查清单 -->## 7.2 实施检查清单

```yaml
CSI实施检查清单:
  设计阶段:
    ☐ 存储需求分析完成
    ☐ 性能基准测试完成
    ☐ 安全合规要求确认
    ☐ 备份恢复策略制定
  
  部署阶段:
    ☐ CSI驱动安装验证
    ☐ 存储类配置测试
    ☐ 卷生命周期验证
    ☐ 快照功能测试
  
  运维阶段:
    ☐ 监控告警配置完成
    ☐ 故障处理流程建立
    ☐ 性能调优持续进行
    ☐ 安全补丁定期更新
```

<!-- chunk: N. CSI存储 2026更新 -->## N. CSI存储 2026更新

#<!-- chunk: N.1 ReadWriteOncePod (GA) -->## N.1 ReadWriteOncePod (GA)
- 第三种访问模式：确保PVC只能被单个Pod挂载
- 对比RWO/RWX/RWOP三种模式
- 适用场景：StatefulSet强一致性数据库

```yaml
# ReadWriteOncePod PVC示例
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: database-pvc
  namespace: production
spec:
  accessModes:
  - ReadWriteOncePod
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 100Gi
```

#<!-- chunk: N.2 Volume Group Snapshots -->## N.2 Volume Group Snapshots
- VolumeGroupSnapshot/VolumeGroupSnapshotContent资源
- 多Volume一致性快照（数据库WAL+数据文件同时快照）
- 对比传统单Volume快照的局限

```yaml
# VolumeGroupSnapshot示例
apiVersion: groupsnapshot.storage.k8s.io/v1alpha1
kind: VolumeGroupSnapshot
metadata:
  name: db-group-snapshot
  namespace: production
spec:
  volumeGroupSnapshotClassName: csi-hostpath-groupsnapclass
  source:
    selector:
      matchLabels:
        app: postgres
        snapshot-group: db-data
```

<!-- chunk: 8. 未来发展趋势 -->## 8. 未来发展趋势

#<!-- chunk: 8.1 存储技术演进 -->## 8.1 存储技术演进

```yaml
未来存储技术趋势:
  1. 智能化存储管理
     - AI驱动的存储优化
     - 自动化容量规划
     - 预测性故障检测
  
  2. 新型存储介质
     - NVMe-oF网络存储
     - 持久内存应用
     - 量子存储技术
  
  3. 边缘存储架构
     - 分布式边缘存储
     - 5G网络存储整合
     - 实时数据处理能力
```

---
*本文档基于企业级存储架构实践经验编写，持续更新最新技术和最佳实践。*
*最近更新：2026-03-03，新增CSI存储2026更新章节（ReadWriteOncePod GA、Volume Group Snapshots）。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-19-papers KUDIG Database — Global MOC
- [[domain-19-landscape-references/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers...]]
- index.md|Domain-19 论文与参考 — 开源项目索引]]
- Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framew...
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Op...
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Imp...
- deployment.md|Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Archit...]]
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- Kubernetes 成本治理与 FinOps 实践 (Kubernetes Cost Governance and F...
- Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro...
- Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and ...
- Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)

## See Also

- 05-kubernetes-gitops-complete-practice-guide
- 06-kubernetes-cost-governance-finops-practice
- 08-kubernetes-network-policies-security-micro-segmentation
- 09-kubernetes-service-mesh-istio-integration

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
