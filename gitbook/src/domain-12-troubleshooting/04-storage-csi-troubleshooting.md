# 04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes-csi.github.io](https://kubernetes-csi.github.io/)

---

## 目录

1. [概述与诊断框架](#1-概述与诊断框架)
2. [卷创建故障排查](#2-卷创建故障排查)
3. [卷挂载故障排查](#3-卷挂载故障排查)
4. [卷读写异常诊断](#4-卷读写异常诊断)
5. [卷扩容问题处理](#5-卷扩容问题处理)
6. [快照功能故障](#6-快照功能故障)
7. [CSI组件异常处理](#7-csi组件异常处理)
8. [存储类配置优化](#8-存储类配置优化)
9. [监控告警配置](#9-监控告警配置)
10. [最佳实践](#10-最佳实践)

---

## 1. 概述与诊断框架

### 1.1 CSI存储概述

CSI (Container Storage Interface) 是Kubernetes的标准存储接口，允许存储供应商开发自己的存储插件。主要组件包括：

- **CSI Driver**: 存储供应商提供的驱动程序
- **External Provisioner**: 卷供应控制器
- **External Attacher**: 卷挂载控制器
- **External Resizer**: 卷扩容控制器
- **External Snapshotter**: 快照控制器

### 1.2 常见存储故障类型

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **卷创建失败** | PVC Pending、Provisioner报错 | 新应用无法部署 | P1 - 高 |
| **卷挂载失败** | Pod卡在ContainerCreating、MountVolume失败 | 应用启动失败 | P0 - 紧急 |
| **卷读写异常** | IO错误、数据损坏、性能差 | 应用数据异常 | P0 - 紧急 |
| **卷扩容失败** | resize PVC失败、空间不足 | 存储容量受限 | P2 - 中 |
| **快照失败** | VolumeSnapshot创建失败 | 备份恢复受阻 | P2 - 中 |
| **CSI组件异常** | CSI Driver Pod Crash、Controller失败 | 存储功能整体异常 | P0 - 紧急 |

### 1.3 CSI 架构关键组件

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CSI 存储故障诊断架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                          应用Pod层面                                   │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │  │
│  │  │   Pod A     │  │   Pod B     │  │   Pod C     │                   │  │
│  │  │             │  │             │  │             │                   │  │
│  │  │ volumeMounts:│  │ volumeMounts:│  │ volumeMounts:│                   │  │
│  │  │ - name: data │  │ - name: log  │  │ - name: cache│                   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                   │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│           │         │         │                                            │
│           ▼         ▼         ▼                                            │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    VolumeAttachment对象                                │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │  │
│  │  │ Attach-Detach│  │ Attach-Detach│  │ Attach-Detach│                   │  │
│  │  │ Controller   │  │ Controller   │  │ Controller   │                   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                   │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CSI Controller组件                                  │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │ Provisioner │  │ Attacher    │  │ Resizer     │  │ Snapshotter │  │  │
│  │  │ (创建卷)     │  │ (挂载卷)    │  │ (扩容卷)    │  │ (快照)      │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CSI gRPC接口 (Unix Socket)                          │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Controller│    │    Node     │    │  Identity   │                   │
│  │    Plugin   │    │   Plugin    │    │   Service   │                   │
│  │             │    │             │    │             │                   │
│  │ - CreateVol │    │ - NodeStage │    │ - Probe     │                   │
│  │ - DeleteVol │    │ - NodePub   │    │ - GetPlugin │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    存储后端 (云盘/NFS/Ceph等)                          │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   云硬盘     │  │   NFS存储    │  │   Ceph RBD  │  │   本地盘     │  │  │
│  │  │ (EBS/EBS)   │  │             │  │             │  │             │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. PVC 创建失败故障排查 (PVC Provisioning Failure)
### 2.1 故障诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      PVC 创建失败诊断流程                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   PVC一直处于Pending状态                                                    │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查PVC状态和事件                             │                 │
│   │ kubectl describe pvc <pvc-name>                      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── StorageClass不存在 ──▶ 创建StorageClass                      │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查CSI Provisioner状态                       │                 │
│   │ kubectl get pods -n kube-system -l app=csi-provisioner│                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── Provisioner异常 ──▶ 检查Provisioner日志                      │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查存储后端连接                              │                 │
│   │ 检查云服务商API密钥、网络连通性                        │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 后端连接失败 ──▶ 检查凭证/网络                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查资源配额                                  │                 │
│   │ kubectl describe quota -n <namespace>                │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 详细诊断命令

```bash
# ========== 1. PVC状态检查 ==========

# 查看PVC状态
kubectl get pvc -A -o wide

# 详细检查PVC
kubectl describe pvc <pvc-name> -n <namespace>

# 查看PVC相关事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<pvc-name>

# ========== 2. StorageClass检查 ==========

# 查看StorageClass
kubectl get storageclass
kubectl describe storageclass <sc-name>

# 检查默认StorageClass
kubectl get storageclass | grep default

# ========== 3. CSI Provisioner检查 ==========

# 检查CSI控制器Pod
kubectl get pods -n kube-system -l app=csi-controller

# 查看Provisioner日志
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=100

# 检查CSI驱动状态
kubectl get csidrivers

# ========== 4. 存储后端验证 ==========

# AWS EBS示例 - 检查凭证
aws sts get-caller-identity

# 检查可用区
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.failure-domain\.beta\.kubernetes\.io/zone}{"\n"}{end}'

# ========== 5. 配额和限制检查 ==========

# 检查ResourceQuota
kubectl describe quota -n <namespace>

# 检查LimitRange
kubectl get limitrange -n <namespace>
```

### 2.3 常见错误及解决方案

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `no persistent volumes available` | 无匹配的PV/StorageClass配置错误 | 检查StorageClass参数配置 |
| `failed to provision volume` | CSI驱动故障/后端API错误 | 检查CSI驱动日志和后端连接 |
| `exceeded quota` | 超出命名空间配额限制 | 调整ResourceQuota或清理资源 |
| `invalid capacity range` | 请求容量超出限制 | 调整PVC容量或StorageClass限制 |
| `volume node affinity conflict` | 节点拓扑不匹配 | 检查allowedTopologies配置 |

---

## 3. 卷挂载失败故障排查 (Volume Mount Failure)

### 3.1 挂载失败诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      卷挂载失败诊断流程                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Pod卡在ContainerCreating状态                                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查Pod事件和状态                             │                 │
│   │ kubectl describe pod <pod-name>                      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── PVC未绑定 ──▶ 转PVC创建故障排查                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查CSI Node组件                              │                 │
│   │ kubectl get pods -n kube-system -l app=csi-node      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── Node组件异常 ──▶ 检查Node组件日志                            │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查节点挂载点                                │                 │
│   │ ssh <node> && mount \| grep <volume-id>              │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 挂载点不存在 ──▶ 检查挂载过程                                │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查文件系统和权限                            │                 │
│   │ ls -la /var/lib/kubelet/pods/*/volumes/*/*           │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 详细诊断命令

```bash
# ========== 1. Pod挂载状态检查 ==========

# 检查Pod状态
kubectl get pods -A --field-selector=status.phase=Pending

# 详细检查Pod
kubectl describe pod <pod-name> -n <namespace>

# 查看挂载相关事件
kubectl get events -n <namespace> | grep -i "attach\|mount\|volume"

# ========== 2. 节点级别检查 ==========

# 获取Pod所在节点
NODE=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}')

# 登录节点检查挂载
ssh $NODE

# 检查kubelet挂载点
ls -la /var/lib/kubelet/pods/*/volumes/kubernetes.io~csi/

# 检查实际挂载
mount | grep <volume-id>
lsblk | grep <volume-id>

# ========== 3. CSI Node组件检查 ==========

# 检查CSI Node Pod
kubectl get pods -n kube-system -o wide | grep csi-node

# 查看Node组件日志
kubectl logs -n kube-system <csi-node-pod> --all-containers

# 检查节点注册状态
kubectl get csinodes

# ========== 4. 文件系统检查 ==========

# 检查文件系统类型
blkid /dev/<device-path>

# 检查文件系统完整性
fsck -n /dev/<device-path>

# 检查目录权限
ls -ld /var/lib/kubelet/plugins/kubernetes.io/csi/
```

### 3.3 常见挂载错误

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `MountVolume.SetUp failed` | CSI Node插件故障 | 重启CSI Node组件 |
| `timeout expired waiting for volume` | 挂载超时 | 检查存储后端性能 |
| `permission denied` | 文件系统权限问题 | 检查挂载点权限设置 |
| `device or resource busy` | 设备被占用 | 检查是否有残留挂载 |
| `unknown filesystem type` | 文件系统不支持 | 格式化为支持的文件系统 |

---

## 4. CSI 组件故障排查 (CSI Component Issues)

### 4.1 CSI Controller故障

```bash
# ========== Controller组件检查 ==========

# 检查Controller Pod状态
kubectl get pods -n kube-system -l app=csi-controller

# 检查各个CSI容器
kubectl get pods -n kube-system -l app=csi-controller -o jsonpath='{.items[0].spec.containers[*].name}'

# 查看Controller日志
for container in $(kubectl get pods -n kube-system -l app=csi-controller -o jsonpath='{.items[0].spec.containers[*].name}'); do
  echo "=== Container: $container ==="
  kubectl logs -n kube-system -l app=csi-controller -c $container --tail=50
done

# 检查Controller指标
kubectl port-forward -n kube-system svc/csi-controller 8080:8080
curl localhost:8080/metrics | grep csi
```

### 4.2 CSI Node故障

```bash
# ========== Node组件检查 ==========

# 检查所有Node组件
kubectl get daemonset -n kube-system | grep csi

# 检查Node组件Pod分布
kubectl get pods -n kube-system -l app=csi-node -o wide

# 检查Node组件健康状态
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  echo "=== Node: $node ==="
  kubectl get pods -n kube-system -l app=csi-node -o wide | grep $node
done

# Node组件日志分析
kubectl logs -n kube-system -l app=csi-node --tail=100 --all-containers
```

---

## 5. 存储性能问题排查 (Storage Performance Issues)

### 5.1 性能监控指标

```bash
# ========== 1. 存储性能指标收集 ==========

# Pod级别IO统计
kubectl exec -it <pod> -- iostat -x 1 5

# 节点级别存储性能
ssh <node> iotop -ao

# CSI指标收集
kubectl port-forward -n kube-system svc/csi-controller 8080:8080
curl localhost:8080/metrics | grep -E "(csi_operations_seconds|storage_operations)"

# ========== 2. 存储后端性能 ==========

# AWS EBS性能检查
aws cloudwatch get-metric-statistics \
  --namespace AWS/EBS \
  --metric-name VolumeReadOps \
  --dimensions Name=VolumeId,Value=vol-xxxxxxxxx \
  --start-time 2024-01-01T00:00:00Z \
  --end-time 2024-01-02T00:00:00Z \
  --period 300 \
  --statistics Average

# ========== 3. 应用性能影响 ==========

# 检查应用IO等待时间
kubectl exec -it <pod> -- dstat -clmndst

# 检查文件系统缓存命中率
kubectl exec -it <pod> -- cat /proc/vmstat | grep -E "pgsteal|pgactivate"
```

### 5.2 性能优化建议

| 优化方向 | 具体措施 | 适用场景 |
|---------|---------|---------|
| **存储类型选择** | 选择合适的卷类型(SSD/SATA) | 性能敏感应用 |
| **文件系统调优** | 调整挂载参数(noatime等) | 高频读写场景 |
| **预热缓存** | 提前加载热点数据 | 数据库应用 |
| **批量操作** | 合并小IO操作 | 日志写入场景 |
| **监控告警** | 设置性能阈值告警 | 主动发现问题 |

---

## 6. 生产环境应急处理 (Production Emergency Response)

### 6.1 存储故障紧急诊断脚本

```bash
#!/bin/bash
# csi-storage-emergency-check.sh

echo "=== CSI 存储紧急诊断 ==="
echo "时间: $(date)"
echo ""

# 1. PVC状态检查
echo "1. 异常PVC统计:"
kubectl get pvc --all-namespaces | grep -v Bound | wc -l

# 2. CSI组件状态
echo -e "\n2. CSI组件状态:"
kubectl get pods -n kube-system | grep -E "(csi|storage)" | grep -v Running

# 3. 存储后端连接检查
echo -e "\n3. 存储后端检查:"
# 这里可以添加具体的存储后端健康检查命令

# 4. 节点存储压力
echo -e "\n4. 节点存储使用:"
kubectl describe nodes | grep -A 5 "VolumesInUse"

# 5. 最近存储相关事件
echo -e "\n5. 最近存储事件:"
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | grep -i "volume\|storage" | tail -10

echo -e "\n=== 诊断完成 ==="
```

### 6.2 故障处理优先级

| 故障类型 | 响应时间 | 处理步骤 |
|---------|---------|---------|
| **卷挂载失败** | 30分钟内 | 1. 检查CSI Node组件 2. 验证挂载点 3. 重启相关组件 |
| **数据读写异常** | 15分钟内 | 1. 立即隔离故障卷 2. 检查存储后端 3. 数据恢复 |
| **PVC创建失败** | 1小时内 | 1. 检查Provisioner 2. 验证后端连接 3. 调整配置 |
| **存储性能下降** | 2小时内 | 1. 性能分析 2. 资源扩容 3. 参数调优 |

---

## 7. 预防措施与最佳实践 (Prevention & Best Practices)

### 7.1 监控告警配置

```yaml
# Prometheus存储监控告警
groups:
- name: storage.rules
  rules:
  # PVC挂起时间过长
  - alert: PVCPendingTooLong
    expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 在命名空间 {{ $labels.namespace }} 中挂起超过10分钟"
  
  # 存储使用率过高
  - alert: StorageSpaceUsageHigh
    expr: kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "卷 {{ $labels.persistentvolumeclaim }} 使用率超过85%"
  
  # CSI组件不可用
  - alert: CSIComponentDown
    expr: up{job=~"csi.*"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "CSI组件 {{ $labels.job }} 不可用"
```

### 7.2 运维检查清单

- [ ] 定期检查CSI组件健康状态
- [ ] 监控存储后端API调用成功率
- [ ] 验证PVC创建和挂载成功率
- [ ] 检查存储性能指标基线
- [ ] 测试存储故障恢复流程
- [ ] 保持CSI驱动版本更新
- [ ] 定期清理未使用的卷和快照

---