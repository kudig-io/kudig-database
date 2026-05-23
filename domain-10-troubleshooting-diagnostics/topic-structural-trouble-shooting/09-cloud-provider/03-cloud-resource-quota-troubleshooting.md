---
title: 云资源配额与 API 限流故障排查指南 [topic-structural-trouble-shooting]
description: 'title: 云资源配额与 API 限流故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- controller-manager
- prometheus
- grafana
- docker
- hpa
- job
- cronjob
- ingress
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 云资源配额与 API 限流故障排查指南 是什么
- 如何 云资源配额与 API 限流故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 云资源配额与 API 限流故障排查指南 故障排查
- 云资源配额与 API 限流故障排查指南 排障步骤
trigger_keywords:
- 云资源配额与
- API
- 限流故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- backup-basics
created: "2026-05-23"
---

title: 云资源配额与 API 限流故障排查指南
description: '# 云资源配额与 API 限流故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- controller-manager
- [[Prometheus|prometheus]]
- grafana
- hpa
- job
- [[CronJob|cronjob]]
- [[Ingress|ingress]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 云资源配额与 API 限流故障排查指南 是什么
- 如何 云资源配额与 API 限流故障排查指南
- 云资源配额与 API 限流故障排查指南 故障排查
- 云资源配额与 API 限流故障排查指南 排障步骤
trigger_keywords:
- 云资源配额与
- API
- 限流故障排查指南
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

# 云资源配额与 API 限流故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: 中级

---

## 0. 10 分钟快速诊断

1. **CCM 日志扫描**：`kubectl logs -n kube-system -l app=cloud-controller-manager --tail=200 | grep -iE "quota|rate|limit|throttle"`。
2. **云 CLI 配额检查**：使用对应云厂商 CLI（`aws ec2 describe-account-attributes`、`az vm list-usage`）查看当前配额使用率。
3. **事件检查**：`kubectl get events --field-selector reason=FailedScheduling` 查看是否因配额不足导致节点无法创建。
4. **Service 状态**：`kubectl get svc -A | grep Pending`，确认 LoadBalancer 是否因配额卡住。
5. **节点组状态**：检查 Cluster Autoscaler 日志中的 `InsufficientInstanceCapacity` 或配额相关错误。
6. **快速缓解**：
   - 立即申请临时配额提升（多数云厂商支持紧急配额申请）。
   - 释放闲置资源（未绑定的 EIP、过期的快照、闲置的磁盘）。
7. **证据留存**：保存 CCM 日志、云厂商配额页面截图、受影响的资源列表、配额提升工单号。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 计算资源配额耗尽

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 节点扩容失败 | `InsufficientInstanceCapacity` | Cluster Autoscaler | CA Pod 日志 |
| 配额超限 | `Your quota allows for 0 more running instance(s)` | 云厂商 API | CCM 日志/云控制台 |
| vCPU 限制 | `vCPU limit exceeded` | 云厂商 API | CCM 日志 |
| 竞价实例中断 | `Spot instance termination notice` | 云厂商元数据 | 实例元数据服务 |

#### 1.1.2 网络资源配额耗尽

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| EIP 分配失败 | `Elastic IP address limit exceeded` | 云厂商 API | CCM 日志 |
| 负载均衡器创建失败 | `LoadBalancer limit exceeded` | 云厂商 API | Service Events |
| 安全组规则超限 | `Security group rule limit exceeded` | 云厂商 API | CCM 日志 |
| NAT Gateway 限制 | `NAT Gateway limit exceeded per AZ` | 云厂商 API | 云控制台 |

#### 1.1.3 存储资源配额耗尽

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 云盘创建失败 | `Volume limit exceeded` | CSI 驱动日志 | CSI provisioner 日志 |
| 快照配额耗尽 | `Snapshot limit exceeded` | CSI snapshotter | snapshotter 日志 |
| 存储容量限制 | `Total storage capacity limit exceeded` | 云厂商 API | 云控制台 |
| IOPS/吞吐限制 | `Volume throughput exceeds limit` | 应用日志 | `iostat` / 云监控 |

#### 1.1.4 API 限流与调用异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| API 调用被限流 | `Rate exceeded` / `Throttling` / `429 Too Many Requests` | 云厂商 API | CCM/CSI 日志 |
| API 调用超时 | `RequestTimeout` / `Client.RequestLimitExceeded` | 云厂商 API | 控制器日志 |
| 凭证失效 | `InvalidAccessKeyId` / `SignatureDoesNotMatch` | 云厂商 API | CCM 日志 |
| 区域服务不可用 | `ServiceUnavailableInRegion` | 云厂商 API | 云厂商状态页 |

#### 1.1.5 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大促期间无法扩容** | HPA 触发但节点未增加，应用负载持续升高 | vCPU/实例数配额已达上限 | 提前申请临时配额提升 |
| **批量创建 LoadBalancer 失败** | 新发布的微服务无法暴露外网访问 | 账户 LB 数量配额耗尽 | 清理闲置 LB，申请配额提升 |
| **定时快照任务大面积失败** | 备份 CronJob 持续报错 | 单区域快照数量达到上限 | 调整保留策略，跨区域归档 |
| **Cluster Autoscaler 频繁报错** | CA 日志中大量 `ThrottlingException` | CA 调用云 API 频率过高被限流 | 调大 CA 扫描间隔，申请 API 限流提升 |

### 1.2 报错查看方式汇总

```bash
# Cloud Controller Manager 日志筛查
kubectl logs -n kube-system deployment/cloud-controller-manager --tail=500 | \
  grep -iE "quota|rate|limit|throttle|exceeded|capacity"

# Cluster Autoscaler 日志筛查
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=500 | \
  grep -iE "InsufficientInstanceCapacity|quota|limit|throttle|failed"

# CSI Provisioner 日志筛查
kubectl logs -n kube-system deployment/csi-provisioner --tail=500 | \
  grep -iE "quota|limit|exceeded"

# 查看所有 Pending 状态的 Service（LB 创建失败）
kubectl get svc --all-namespaces -o json | \
  jq -r '.items[] | select(.status.loadBalancer.ingress == null and .spec.type == "LoadBalancer") | \
  "\(.metadata.namespace)/\(.metadata.name)"'

# 查看 FailedScheduling 事件
kubectl get events --field-selector reason=FailedScheduling --sort-by='.lastTimestamp'
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

云资源配额管理是一个多层级体系：

```
┌─────────────────────────────────────────────┐
│          组织/账户级配额 (Organization)        │
│  总 vCPU | 总内存 | 总存储 | 总网络资源        │
├─────────────────────────────────────────────┤
│          区域级配额 (Region)                   │
│  区域 vCPU | 区域实例数 | 区域 EIP | 区域 LB   │
├─────────────────────────────────────────────┤
│          可用区级配额 (Availability Zone)       │
│  AZ 实例容量 | AZ 网络资源 | AZ GPU             │
├─────────────────────────────────────────────┤
│          API 级限流 (API Rate Limit)           │
│  每秒请求数 (QPS) | 并发请求数 | 令牌桶速率     │
└─────────────────────────────────────────────┘
```

**关键概念**：
- **On-Demand Quota**：标准实例配额，适用于大多数场景
- **Spot/Preemptible Quota**：竞价实例独立配额，通常更高
- **API Rate Limit**：云厂商对 API 调用频率的限制，通常按账户+区域维度计算
- **Burst Limit**：部分云厂商允许短时突破基线限流，但持续超限会被惩罚性限流

### 2.2 排查逻辑决策树

```
云资源配额/API 问题
    ├── 资源创建失败
    │   ├── 计算资源（实例/节点）
    │   │   ├── 配额超限？──► 查看账户/区域配额 → 申请提升或释放资源
    │   │   ├── AZ 容量不足？──► 切换到其他 AZ 或区域
    │   │   └── 实例类型限制？──► 使用替代实例类型
    │   ├── 网络资源（LB/EIP/安全组）
    │   │   ├── 配额超限？──► 清理闲置资源 → 申请提升
    │   │   └── 安全组规则冲突？──► 合并规则或清理过期规则
    │   └── 存储资源（磁盘/快照）
    │       ├── 数量配额超限？──► 清理旧快照/磁盘
    │       └── 容量配额超限？──► 扩容配额或归档冷数据
    ├── API 调用失败
    │   ├── 限流 (429/Throttling)
    │   │   ├── 控制器请求频率过高？──► 调大扫描间隔，降低并发
    │   │   ├── 多个控制器共享限流配额？──► 分散到不同账户或使用不同 region
    │   │   └── 需要更高基线？──► 向云厂商申请提升 API 限流
    │   ├── 认证失败
    │   │   ├── 凭证过期？──► 轮换 AccessKey/Token
    │   │   ├── IAM 权限不足？──► 添加必要权限策略
    │   │   └── 实例角色脱落？──► 重新绑定 IAM Role
    │   └── 区域服务不可用
    │       └── 云厂商区域问题？──► 切换到备用区域
    └── 资源被回收/中断
        ├── Spot 实例被回收
        │   └── 部署 Spot 中断处理程序 + 使用 Spot 容量再平衡
        └── 预留实例到期
            └── 续费预留实例或切换到按需实例
```

### 2.3 详细诊断命令

#### 多云配额统一诊断

```bash
#!/bin/bash
# 多云配额统一诊断脚本
# 根据当前环境自动检测云厂商并执行对应检查

echo "=== 云资源配额诊断 ==="

# 检测云厂商
if curl -s http://169.254.169.254/latest/meta-data/ami-id >/dev/null 2>&1; then
  CLOUD="aws"
elif curl -s -H Metadata:true "http://169.254.169.254/metadata/instance/compute/az?api-version=2021-02-01&format=text" >/dev/null 2>&1; then
  CLOUD="azure"
elif curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/id >/dev/null 2>&1; then
  CLOUD="gcp"
elif curl -s http://100.100.100.200/latest/meta-data/instance-id >/dev/null 2>&1; then
  CLOUD="aliyun"
else
  echo "⚠ 无法检测云厂商，请手动指定"
  exit 1
fi

echo "检测到云厂商: $CLOUD"
echo ""

case $CLOUD in
  aws)
    echo "=== AWS 配额诊断 ==="
    echo "1. EC2 实例配额:"
    aws ec2 describe-account-attributes --attribute-names max-instances 2>/dev/null || \
      echo "  ⚠ 无法获取配额信息（可能需要更高权限）"
    
    echo ""
    echo "2. 当前区域实例使用:"
    aws ec2 describe-instances --query 'Reservations[*].Instances[*].{Type:InstanceType,State:State.Name}' --output table 2>/dev/null | \
      grep -E "Running|Pending" | wc -l | xargs -I {} echo "  运行中/_pending 实例数: {}"
    
    echo ""
    echo "3. EIP 配额与使用:"
    aws ec2 describe-addresses --query 'Addresses[*].{IP:PublicIp}' --output table 2>/dev/null
    
    echo ""
    echo "4. 当前区域 vCPU 使用:"
    aws ec2 describe-instances --filters "Name=instance-state-name,Values=running" \
      --query 'Reservations[*].Instances[*].{Type:InstanceType,VCPU:CpuOptions.CoreCount}' --output table 2>/dev/null
    ;;
    
  azure)
    echo "=== Azure 配额诊断 ==="
    LOCATION=$(az account list-locations --query "[?isDefault].name" -o tsv 2>/dev/null || echo "eastus")
    
    echo "1. 区域计算配额 ($LOCATION):"
    az vm list-usage --location $LOCATION -o table 2>/dev/null | head -20
    
    echo ""
    echo "2. 网络配额:"
    az network list-usages --location $LOCATION -o table 2>/dev/null | head -10
    ;;
    
  gcp)
    echo "=== GCP 配额诊断 ==="
    PROJECT=$(gcloud config get-value project 2>/dev/null)
    REGION=$(gcloud config get-value compute/region 2>/dev/null || echo "us-central1")
    
    echo "1. 区域计算配额 ($REGION):"
    gcloud compute regions describe $REGION --format="table(quotas.metric:label=Metric, quotas.limit:label=Limit, quotas.usage:label=Usage)" 2>/dev/null | head -15
    ;;
    
  aliyun)
    echo "=== 阿里云配额诊断 ==="
    echo "1. ECS 实例配额:"
    aliyun ecs DescribeAccountAttribute --AttributeName max-instances 2>/dev/null || \
      echo "  ⚠ 无法获取配额信息"
    
    echo ""
    echo "2. 当前区域实例列表:"
    aliyun ecs DescribeInstances --RegionId cn-hangzhou --PageSize 10 2>/dev/null | \
      jq -r '.Instances.Instance[] | "  \(.InstanceId): \(.InstanceType) (\(.Status))"' 2>/dev/null || \
      echo "  未获取到实例信息"
    ;;
esac
```

#### Kubernetes 控制器限流诊断

```bash
#!/bin/bash
# Kubernetes 控制器 API 限流诊断

echo "=== 控制器 API 限流诊断 ==="

# 1. Cluster Autoscaler 限流检查
echo "1. Cluster Autoscaler 限流情况:"
CA_POD=$(kubectl get pods -n kube-system -l app=cluster-autoscaler -o name 2>/dev/null | head -1)
if [ -n "$CA_POD" ]; then
  kubectl logs -n kube-system $CA_POD --tail=200 2>/dev/null | \
    grep -iE "throttl|rate.*exceed|429|limit" | tail -10
else
  echo "  ⚠ Cluster Autoscaler 未部署"
fi

# 2. Cloud Controller Manager 限流检查
echo ""
echo "2. Cloud Controller Manager 限流情况:"
CCM_POD=$(kubectl get pods -n kube-system -l app=cloud-controller-manager -o name 2>/dev/null | head -1)
if [ -n "$CCM_POD" ]; then
  kubectl logs -n kube-system $CCM_POD --tail=200 2>/dev/null | \
    grep -iE "throttl|rate.*exceed|429|limit" | tail -10
else
  echo "  ⚠ Cloud Controller Manager 未部署"
fi

# 3. CSI 驱动限流检查
echo ""
echo "3. CSI 驱动限流情况:"
for pod in $(kubectl get pods -n kube-system -o name | grep -E "csi|driver" | head -3); do
  echo "=== $pod ==="
  kubectl logs -n kube-system $pod --tail=100 2>/dev/null | \
    grep -iE "throttl|rate.*exceed|429|limit" | tail -3
done

# 4. 事件中的配额相关错误
echo ""
echo "4. 近期配额/限流相关事件:"
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | \
  grep -iE "quota|limit|exceed|throttl|capacity|Insufficient" | tail -15
```

---

## 3. 解决方案与风险控制

### 3.1 配额管理解决方案

#### 方案一：AWS 配额监控与自动告警

```bash
#!/bin/bash
# AWS 配额监控脚本（建议作为 CronJob 运行）

REPORT_FILE="/var/log/kubernetes/aws-quota-report-$(date +%Y%m%d-%H%M%S).json"

echo "=== AWS 配额监控报告 ===" | tee $REPORT_FILE

# 获取 Service Quotas
aws service-quotas list-service-quotas --service-code ec2 --query 'Quotas[*].{Name:QuotaName,Value:Value,Usage:Value}' --output json 2>/dev/null | \
  jq '[.[] | select(.Value > 0)]' > $REPORT_FILE

# 获取 Trusted Advisor 配额告警（需要 Business/Enterprise 支持计划）
aws support describe-trusted-advisor-checks --language zh 2>/dev/null | \
  jq '.checks[] | select(.name | contains("quota") or contains("limit"))' > /tmp/ta-quota-checks.json

echo "配额报告已保存: $REPORT_FILE"
```

#### 方案二：资源清理自动化脚本

```bash
#!/bin/bash
# 云资源自动清理脚本（用于释放配额）
# ⚠️ 请在使用前仔细审查，避免误删生产资源

DRY_RUN=${1:-"true"}  # 默认 dry-run 模式

echo "=== 云资源自动清理 ==="
echo "执行模式: $([ "$DRY_RUN" = "true" ] && echo "模拟运行 (dry-run)" || echo "实际执行")"
echo ""

# 1. 未绑定的 EIP
echo "1. 未绑定的弹性 IP:"
UNASSIGNED_EIPS=$(aws ec2 describe-addresses --query 'Addresses[?AssociationId==null].AllocationId' --output text 2>/dev/null)
if [ -n "$UNASSIGNED_EIPS" ]; then
  for eip in $UNASSIGNED_EIPS; do
    echo "  发现未绑定 EIP: $eip"
    if [ "$DRY_RUN" = "false" ]; then
      aws ec2 release-address --allocation-id $eip && echo "    ✓ 已释放" || echo "    ✗ 释放失败"
    fi
  done
else
  echo "  未发现未绑定的 EIP"
fi

# 2. 已终止但未删除的卷
echo ""
echo "2. 可用状态（未挂载）的磁盘:"
AVAILABLE_VOLS=$(aws ec2 describe-volumes --filters "Name=status,Values=available" --query 'Volumes[*].VolumeId' --output text 2>/dev/null)
if [ -n "$AVAILABLE_VOLS" ]; then
  for vol in $AVAILABLE_VOLS; do
    echo "  发现可用卷: $vol"
    if [ "$DRY_RUN" = "false" ]; then
      aws ec2 delete-volume --volume-id $vol && echo "    ✓ 已删除" || echo "    ✗ 删除失败"
    fi
  done
else
  echo "  未发现可用状态的磁盘"
fi

# 3. 过期的快照（超过 30 天且非近期创建）
echo ""
echo "3. 超过 30 天的旧快照:"
OLD_SNAPSHOTS=$(aws ec2 describe-snapshots --owner-ids self --query "Snapshots[?StartTime<='$(date -u -d '30 days ago' +%Y-%m-%dT%H:%M:%SZ)'].SnapshotId" --output text 2>/dev/null)
if [ -n "$OLD_SNAPSHOTS" ]; then
  for snap in $OLD_SNAPSHOTS; do
    echo "  发现旧快照: $snap"
    if [ "$DRY_RUN" = "false" ]; then
      aws ec2 delete-snapshot --snapshot-id $snap && echo "    ✓ 已删除" || echo "    ✗ 删除失败"
    fi
  done
else
  echo "  未发现超过 30 天的旧快照"
fi

echo ""
echo "清理检查完成"
[ "$DRY_RUN" = "true" ] && echo "本次为模拟运行，未实际删除资源。如需实际执行请传入参数 'false'"
```

#### 方案三：Cluster Autoscaler 限流缓解配置

```yaml
# Cluster Autoscaler 配置优化（减少 API 调用频率）
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  # 通过环境变量传递给 CA
  # 在 CA Deployment 的 spec.template.spec.containers[].env 中引用
  scan-interval: "60s"           # 默认 10s，调大以减少扫描频率
  scale-down-delay-after-add: "10m"
  scale-down-delay-after-failure: "3m"
  scale-down-unneeded-time: "10m"
  max-node-provision-time: "15m"
  node-autoprovisioning-enabled: "false"  # 禁用 Node AutoProvisioning 以减少 API 调用
---
# Cluster Autoscaler Deployment 片段
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.30.0
        command:
        - ./cluster-autoscaler
        - --cloud-provider=aws
        - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/my-cluster
        - --scan-interval=60s          # 调大扫描间隔
        - --scale-down-delay-after-add=10m
        - --scale-down-delay-after-failure=3m
        - --max-nodes-total=100        # 设置节点上限防止过度扩容
        - --skip-nodes-with-system-pods=false
        env:
        - name: AWS_REGION
          value: "us-east-1"
```

### 3.2 API 限流缓解方案

```yaml
# 为 CCM 和 CSI 驱动配置指数退避和请求限流
apiVersion: apps/v1
kind: Deployment
metadata:
  name: aws-cloud-controller-manager
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: aws-cloud-controller-manager
        image: registry.k8s.io/provider-aws/cloud-controller-manager:v1.30.0
        args:
        - --cloud-provider=aws
        - --cluster-name=my-cluster
        - --configure-cloud-routes=true
        - --cloud-config=/etc/kubernetes/cloud-config
        - --route-reconciliation-period=60s   # 调大路由同步间隔
        - --node-status-update-frequency=2m   # 调大节点状态更新间隔
        env:
        # 配置 AWS SDK 的退避策略（通过环境变量）
        - name: AWS_MAX_ATTEMPTS
          value: "10"
        - name: AWS_RETRY_MODE
          value: "adaptive"  # adaptive 模式会自动降低请求速率
```

### 3.3 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 申请配额提升 | ⭐ 低 | 通常无负面影响，可能需要审批时间 | 无需回滚，配额提升不可逆 |
| 自动清理闲置资源 | ⭐⭐ 中 | 可能误删仍需使用的资源 | 从备份恢复或重新创建 |
| 调整 CA 扫描间隔 | ⭐ 低 | 扩容/缩容响应变慢 | 恢复原始扫描间隔 |
| 修改 CCM 同步频率 | ⭐ 低 | 路由/节点状态同步延迟增加 | 恢复原始频率 |
| 切换至 Spot 实例 | ⭐⭐ 中 | 实例可能被中断，工作负载需可容忍 | 切换回 On-Demand 实例 |
| 跨区域资源迁移 | ⭐⭐ 中 | 数据迁移成本，网络延迟变化 | 迁移回原区域 |

### 3.4 验证与监控

#### 配额监控告警规则

```yaml
# Prometheus 配额监控告警
groups:
- name: cloud-quota
  rules:
  - alert: ClusterAutoscalerThrottling
    expr: |
      increase(cluster_autoscaler_errors_total{reason="cloudProviderError"}[5m]) > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Cluster Autoscaler 遇到云提供商错误"
      description: "Cluster Autoscaler 在过去 5 分钟内遇到 {{ $value }} 次云提供商错误，可能是配额或限流问题"

  - alert: PendingLoadBalancers
    expr: |
      count(kube_service_status_load_balancer_ingress == 0 and kube_service_spec_type == 2) > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "存在 Pending 状态的 LoadBalancer"
      description: "有 {{ $value }} 个 LoadBalancer 类型 Service 未分配 ingress IP，可能是配额耗尽"

  - alert: HighCloudAPIErrorRate
    expr: |
      sum(rate(cloudprovider_aws_api_request_errors[5m])) by (service) > 0.1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "云 API 错误率过高"
      description: "AWS API {{ $labels.service }} 错误率超过 0.1/s，可能是限流或权限问题"
```

#### 配额使用率检查脚本

```bash
#!/bin/bash
# 配额使用率检查脚本（用于监控集成）

THRESHOLD=${1:-80}  # 告警阈值百分比

echo "=== 配额使用率检查 (阈值: ${THRESHOLD}%) ==="

# AWS 示例：检查 vCPU 使用率
if command -v aws &>/dev/null; then
  REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region 2>/dev/null || echo "us-east-1")
  
  # 获取运行中实例的 vCPU 总数
  VCPU_USAGE=$(aws ec2 describe-instances \
    --filters "Name=instance-state-name,Values=running" \
    --query 'sum(Reservations[*].Instances[*].CpuOptions.CoreCount)' \
    --output text 2>/dev/null || echo "0")
  
  # 获取 vCPU 配额
  VCPU_LIMIT=$(aws service-quotas get-service-quota \
    --service-code ec2 \
    --quota-code L-1216C47A \
    --query 'Quota.Value' \
    --output text 2>/dev/null || echo "unknown")
  
  if [ "$VCPU_LIMIT" != "unknown" ] && [ "$VCPU_LIMIT" != "None" ]; then
    USAGE_PCT=$(echo "scale=2; $VCPU_USAGE / $VCPU_LIMIT * 100" | bc 2>/dev/null || echo "0")
    echo "vCPU 使用: $VCPU_USAGE / $VCPU_LIMIT (${USAGE_PCT}%)"
    
    if (( $(echo "$USAGE_PCT > $THRESHOLD" | bc -l) )); then
      echo "⚠️ 警告: vCPU 使用率超过 ${THRESHOLD}%"
      exit 1
    fi
  fi
fi

echo "✓ 配额检查通过"
```

### 3.5 最佳实践

1. **配额基线规划**：在集群设计阶段，根据预期节点数、Service 数、PVC 数计算所需配额，提前申请 2-3 倍余量
2. **分层账户策略**：使用独立的账户/项目用于开发、测试、生产环境，避免测试环境耗尽生产配额
3. **Spot 实例优先**：对可容忍中断的工作负载优先使用 Spot/Preemptible 实例，其配额通常更高且成本更低
4. **自动化监控**：将配额使用率纳入 Prometheus/Grafana 监控，设置 70%/85%/95% 三级告警
5. **定期清理策略**：建立自动化策略清理未绑定的 EIP、过期的快照、可用的孤儿磁盘
6. **API 调用优化**：调大控制器（CA、CCM、CSI）的同步间隔，使用 adaptive retry 模式减少限流触发
7. **预留容量**：对核心业务使用 Reserved Instances / Savings Plans + 适当的 On-Demand 配额，确保扩容能力

### 典型问题案例

#### 案例一：大促前夜无法扩容导致服务降级

**问题描述**：电商大促前压力测试触发 HPA，但节点数未增加，Pod 处于 Pending 状态。

**根本原因**：账户的 On-Demand vCPU 配额为 384，当前已使用 380，仅剩 4 vCPU 余量。

**解决方案**：
1. 紧急向云厂商提交配额提升工单（AWS Support Case）
2. 临时释放开发环境的闲置实例
3. 将部分非核心服务迁移到 Spot 实例以释放 On-Demand 配额
4. 事后建立配额监控告警，设置 70% 预警阈值

#### 案例二：定时快照导致 API 限流

**问题描述**：每晚 02:00 的批量快照任务导致 CSI 驱动报 `Rate exceeded`，快照大面积失败。

**根本原因**：所有快照任务集中在同一时刻触发，短时间内大量 `CreateSnapshot` API 调用触发云厂商限流。

**解决方案**：
1. 将快照任务分散到 02:00-04:00 的 2 小时窗口内
2. 在 Velero/Snapshot 控制器中配置 `--snapshot-per-batch` 限制并发数
3. 向云厂商申请提升 `CreateSnapshot` API 的限流阈值

#### 案例三：IAM Role 权限边界导致 CCM 部分功能失效

**问题描述**：LoadBalancer 创建成功，但节点加入目标组失败。

**根本原因**：IAM Role 附加了 Permissions Boundary，CCM 需要的 `elasticloadbalancing:RegisterTargets` 权限在边界内被显式拒绝。

**解决方案**：
1. 检查 IAM Role 的 Permissions Boundary 策略
2. 在边界策略中添加必要的 ELB 权限
3. 使用 IAM Policy Simulator 验证 CCM 权限

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[hot|hot]]
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-cloud-provider/01-cloud-provider-integration-troubleshooting|01-cloud-provider-integration-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-cloud-provider/02-multi-cloud-networking-troubleshooting|02-multi-cloud-networking-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-cloud-provider/01-cloud-provider-integration-troubleshooting|01-cloud-provider-integration-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-cloud-provider/02-multi-cloud-networking-troubleshooting|02-multi-cloud-networking-troubleshooting]]
