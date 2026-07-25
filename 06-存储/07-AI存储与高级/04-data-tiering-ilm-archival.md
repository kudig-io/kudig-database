---
title: "数据分层与生命周期管理"
description: "K8s 环境中数据分层模型、ILM 策略实现与 AI 数据生命周期管理"
summary: "覆盖 Hot/Warm/Cold/Archive 分层模型、StorageClass 分层策略、对象存储生命周期规则、AI 数据分层与成本优化实践"
category: 存储
tags:
- storage
- data-tiering
- ilm
- lifecycle
- cost-optimization
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 20min
intent_queries:
- "K8s 中如何实现数据分层存储"
- "AI 训练数据生命周期管理策略"
- "存储成本优化与数据归档方案"
trigger_keywords:
- 数据分层
- ILM
- 生命周期
- 归档
- 冷数据
- 成本优化
prerequisites:
- kubectl-basics
- storage-basics
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

# 数据分层与生命周期管理

## 概述

数据分层（Data Tiering）和信息生命周期管理（Information Lifecycle Management, ILM）是存储成本优化的核心策略。在 AI/ML 工作负载中，数据从采集、标注、训练、验证到归档，经历完整的生命周期，每个阶段对存储性能和成本的要求截然不同。将正确的数据放在正确层级的存储上，可以在不牺牲训练性能的前提下将存储成本降低 60%-80%。

本文覆盖从分层模型设计到 Kubernetes 环境中的具体实现，包括 StorageClass 分层策略、对象存储生命周期规则、AI 数据特有的版本化管理，以及跨层数据迁移的自动化工具链。

## 架构与核心概念

### 数据分层模型

```
┌─────────────────────────────────────────────────────────┐
│  Hot Layer (热数据层)                                     │
│  NVMe SSD / 高性能并行文件系统                              │
│  当前训练数据集、活跃 Checkpoint                            │
│  延迟 < 1ms, IOPS > 100K                                 │
├─────────────────────────────────────────────────────────┤
│  Warm Layer (温数据层)                                    │
│  SSD 云盘 / 标准对象存储                                   │
│  近期模型版本、验证数据集                                   │
│  延迟 < 10ms, 按需访问                                    │
├─────────────────────────────────────────────────────────┤
│  Cold Layer (冷数据层)                                    │
│  HDD / 低频对象存储                                       │
│  历史训练日志、过期模型版本                                  │
│  延迟 < 100ms, 偶尔访问                                   │
├─────────────────────────────────────────────────────────┤
│  Archive Layer (归档层)                                   │
│  磁带 / 深度归档对象存储                                    │
│  合规数据、历史实验记录                                     │
│  延迟 > 1h, 极少访问                                      │
└─────────────────────────────────────────────────────────┘
```

### 分层存储成本对比

| 存储层级 | 典型介质 | 成本 ($/TB/月) | 读取延迟 | 适用 AI 数据 |
|---------|---------|---------------|---------|-------------|
| Hot | NVMe SSD | $150-300 | < 1ms | 当前 epoch 训练数据 |
| Warm | SSD 云盘/S3 Standard | $40-100 | 5-50ms | 近期模型、验证集 |
| Cold | HDD/S3 Infrequent | $10-25 | 100ms-1s | 历史日志、旧模型 |
| Archive | 磁带/S3 Glacier | $1-5 | 1-12h | 合规数据、实验记录 |

### K8s 中的 ILM 实现路径

Kubernetes 本身不提供原生的数据分层机制，ILM 需要通过以下组合实现：

1. **StorageClass 分层**：为不同层级创建独立的 StorageClass
2. **对象存储生命周期规则**：S3/MinIO Lifecycle Policy 自动转换存储类别
3. **CronJob 数据迁移**：定时任务将数据在层级间迁移
4. **CSI 卷快照 + 归档**：快照后删除高性能卷，保留低成本快照
5. **第三方工具**：如 Komiser、Cloud Custodian 进行策略执行

## 生产部署

### StorageClass 分层策略

🟡 中风险：创建 StorageClass 影响后续所有 PVC 的供给行为

```yaml
# Hot 层：高性能 NVMe（当前训练数据）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: tier-hot-nvme
  labels:
    storage-tier: hot
provisioner: ebs.csi.aws.com
parameters:
  type: io2
  iopsPerGB: "50"
  encrypted: "true"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
---
# Warm 层：标准 SSD（近期模型/验证数据）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: tier-warm-ssd
  labels:
    storage-tier: warm
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  throughput: "125"
  encrypted: "true"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
---
# Cold 层：低成本 HDD（历史数据）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: tier-cold-hdd
  labels:
    storage-tier: cold
provisioner: ebs.csi.aws.com
parameters:
  type: sc1
  encrypted: "true"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Retain
```

### 对象存储生命周期规则

🟡 中风险：生命周期规则会自动转换或删除数据

```yaml
# MinIO/S3 生命周期配置（通过 mc 命令或 YAML）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: ilm-policy-applier
  namespace: ai-platform
spec:
  schedule: "0 2 * * 0"  # 每周日凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: ilm-apply
              image: minio/mc:latest
              command:
                - /bin/sh
                - -c
                - |
                  mc alias set minio https://minio.ai-platform.svc:9000 $ACCESS_KEY $SECRET_KEY
                  
                  # 训练数据集：90天未访问转冷存储，365天归档
                  mc ilm rule add minio/training-datasets \
                    --transition-days 90 --storage-class COLD \
                    --noncurrent-expire-days 365
                  
                  # Checkpoint：保留最近 7 天，之后转温存储
                  mc ilm rule add minio/checkpoints \
                    --transition-days 7 --storage-class WARM \
                    --expire-days 180
                  
                  # 模型 artifact：版本化保留
                  mc ilm rule add minio/model-artifacts \
                    --noncurrent-transition-days 30 --storage-class COLD \
                    --noncurrent-expire-days 730
                  
                  # 实验日志：30天后归档
                  mc ilm rule add minio/experiment-logs \
                    --transition-days 30 --storage-class ARCHIVE \
                    --expire-days 2555
              env:
                - name: ACCESS_KEY
                  valueFrom:
                    secretKeyRef:
                      name: minio-credentials
                      key: access-key
                - name: SECRET_KEY
                  valueFrom:
                    secretKeyRef:
                      name: minio-credentials
                      key: secret-key
          restartPolicy: OnFailure
```

### AI 数据分层 CronJob

🟡 中风险：数据迁移操作可能影响正在运行的训练任务

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: ai-data-tiering-job
  namespace: ai-training
spec:
  schedule: "0 3 * * *"  # 每天凌晨 3 点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      activeDeadlineSeconds: 7200
      template:
        spec:
          containers:
            - name: data-tiering
              image: ai-platform/data-tiering-tool:1.2.0
              command:
                - python
                - /app/tier_data.py
                - --config=/etc/tiering/config.yaml
              volumeMounts:
                - name: tiering-config
                  mountPath: /etc/tiering
                - name: hot-storage
                  mountPath: /data/hot
                - name: warm-storage
                  mountPath: /data/warm
              resources:
                requests:
                  cpu: "2"
                  memory: 4Gi
                limits:
                  cpu: "4"
                  memory: 8Gi
          volumes:
            - name: tiering-config
              configMap:
                name: data-tiering-config
            - name: hot-storage
              persistentVolumeClaim:
                claimName: training-data-hot
            - name: warm-storage
              persistentVolumeClaim:
                claimName: training-data-warm
          restartPolicy: OnFailure
```

## 运维操作

### 数据分层状态审计

🟢 低风险/只读：查看各层存储使用情况

```bash
# 查看各 StorageClass 的 PVC 分布
kubectl get pvc --all-namespaces -o json | \
  jq -r '.items[] | [.metadata.namespace, .metadata.name, .spec.storageClassName, .status.capacity.storage] | @tsv' | \
  sort -k3

# 查看 PV 按 StorageClass 分组的容量
kubectl get pv -o json | \
  jq -r '.items[] | [.spec.storageClassName, .spec.capacity.storage, .status.phase] | @tsv' | \
  awk '{tier[$1]+=$2} END {for (t in tier) print t, tier[t]}'

# MinIO 各桶容量与对象数
kubectl exec -n ai-platform minio-pool-0-0 -- \
  mc du local/ --json | jq '{bucket: .key, size: .size, objects: .objects}'

# 查看生命周期规则状态
kubectl exec -n ai-platform minio-pool-0-0 -- \
  mc ilm rule list local/training-datasets --json
```

### 手动数据降级操作

🔴 高风险：数据迁移后原位置数据将被删除

```bash
# 将过期 Checkpoint 从热存储迁移到对象存储
# 步骤 1: 确认目标数据
kubectl exec -n ai-training data-mgmt-pod -- \
  find /data/hot/checkpoints -mtime +7 -name "*.pt" -size +1G

# 步骤 2: 迁移到 MinIO
kubectl exec -n ai-training data-mgmt-pod -- \
  mc mirror /data/hot/checkpoints/expired/ minio/checkpoints/archive/ --overwrite

# 步骤 3: 验证迁移完整性
kubectl exec -n ai-training data-mgmt-pod -- \
  mc diff /data/hot/checkpoints/expired/ minio/checkpoints/archive/

# 步骤 4: 删除本地热存储数据（确认迁移成功后）
kubectl exec -n ai-training data-mgmt-pod -- \
  rm -rf /data/hot/checkpoints/expired/
```

## 故障排查

### 生命周期规则未生效

🟢 低风险/只读：诊断 ILM 规则执行状态

```bash
# 检查 MinIO ILM 规则状态
kubectl exec -n ai-platform minio-pool-0-0 -- \
  mc ilm rule list local/training-datasets --json | jq '.config.Rules[]'

# 查看 ILM 执行日志
kubectl logs -n ai-platform minio-pool-0-0 --tail=200 | grep -i "lifecycle\|transition\|expire"

# 检查对象是否满足转换条件
kubectl exec -n ai-platform minio-pool-0-0 -- \
  mc stat local/training-datasets/old-dataset/train.tar --json | jq '{lastModified, size, tier}'
```

### 数据迁移失败

| 故障现象 | 可能原因 | 排查方法 | 修复措施 |
|---------|---------|---------|---------|
| CronJob 超时 | 数据量过大/网络带宽不足 | 检查 Job 日志和网络流量 | 增加 activeDeadlineSeconds |
| 迁移后数据不一致 | 并发写入 | 对比源和目标 checksum | 使用 --overwrite 重新同步 |
| 分层后训练报错 | 数据路径变更未通知 | 检查训练配置中的数据路径 | 使用符号链接或统一数据目录 |
| 对象存储转换失败 | 存储类别未配置 | `mc admin tier list` | 添加目标存储类别 |
| PVC 扩容失败 | StorageClass 不支持 | `kubectl describe pvc` | 更换支持扩容的 StorageClass |

### 容量告警处理

```bash
# 🟢 低风险/只读：快速定位容量消耗大户
kubectl get pvc --all-namespaces -o json | \
  jq -r '.items[] | select(.status.phase=="Bound") | 
  [.metadata.namespace, .metadata.name, .status.capacity.storage] | @tsv' | \
  sort -k3 -h -r | head -20

# 查看 PV 实际使用率（需要节点访问）
kubectl exec -n ai-training training-pod-0 -- df -h /data
```

## 最佳实践

1. **标签驱动分层**：为 PVC 添加 `data-tier`、`data-category`、`retention-days` 标签，实现策略自动化
2. **渐进式降级**：数据从 Hot → Warm → Cold → Archive 逐级降级，每级设置合理的停留时间
3. **AI 数据版本化**：训练数据集使用语义化版本（v1.0.0），模型 artifact 关联训练实验 ID，参考 [[15-AI基础设施/01-基础设施/06-ai-data-pipeline.md|AI 数据管线]]
4. **成本可视化**：定期生成各 Namespace/团队的存储成本报告，参考 [[22-概念/08-可靠性与运维/finops-greenops-practices.md|FinOps 实践]]
5. **回取策略**：冷数据回取（restore）需要明确 SLA，归档层回取可能需要数小时
6. **合规保留**：涉及用户数据的训练集需满足数据保留法规，归档层设置不可删除的 Object Lock
7. **自动化优先**：所有分层策略通过 CronJob/Operator 自动执行，避免人工操作遗漏
8. **监控覆盖**：为每层存储设置容量水位告警（70%/85%/95%），参考 [[06-存储/01-K8s存储/12-storage-monitoring-alerting.md|存储监控告警]]
9. **灾备考虑**：分层策略需与 [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|灾备架构]] 协调，确保各层数据均有备份

## Related

- [[06-存储/07-AI存储与高级/01-minio-object-storage-ai.md|MinIO 对象存储 for AI/ML]]
- [[06-存储/01-K8s存储/04-storageclass-dynamic-provisioning.md|StorageClass 动态供给]]
- [[15-AI基础设施/01-基础设施/06-ai-data-pipeline.md|AI 数据管线]]
- [[22-概念/08-可靠性与运维/finops-greenops-practices.md|FinOps 与 GreenOps 实践]]
- [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|多区域灾备架构]]
