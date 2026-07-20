---
title: "存储混沌工程"
description: "Kubernetes 存储故障注入、混沌实验设计与数据一致性验证实践"
summary: "覆盖磁盘故障/网络分区/延迟注入、LitmusChaos 存储实验、Chaos Mesh IOChaos、PV/PVC 故障场景、CSI 驱动容错测试与演练剧本"
category: 存储
tags:
- storage
- chaos-engineering
- fault-injection
- resilience
- testing
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
- "如何对 K8s 存储进行混沌工程测试"
- "存储故障注入有哪些方法和工具"
- "如何验证存储在故障下的数据一致性"
trigger_keywords:
- 混沌工程
- 故障注入
- LitmusChaos
- Chaos Mesh
- IOChaos
- 存储演练
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

# 存储混沌工程

## 概述

存储混沌工程是通过主动注入存储层故障来验证系统韧性的实践方法。与网络或计算层混沌实验不同，存储混沌实验的风险更高——错误的故障注入可能导致不可逆的数据丢失。然而，不经过故障验证的存储系统，其 RTO/RPO 承诺只是纸面数字。

本文建立一套安全的存储混沌工程实践框架，覆盖故障注入方法、实验设计原则、主流工具（LitmusChaos、Chaos Mesh）的存储实验配置、数据一致性验证方法，以及完整的演练剧本。特别关注 AI 训练场景——训练任务中断后 Checkpoint 恢复的正确性验证是 AI 平台韧性的核心指标。

## 架构与核心概念

### 存储故障分类

```
Storage Fault Taxonomy:

1. 磁盘/卷故障
   ├── 磁盘完全失效 (I/O error)
   ├── 磁盘性能退化 (高延迟)
   ├── 磁盘容量满 (ENOSPC)
   └── 文件系统损坏 (corruption)

2. 网络存储故障
   ├── 存储网络分区 (不可达)
   ├── 网络延迟注入 (高 RTT)
   ├── 带宽限制 (throttling)
   └── 连接中断 (reset)

3. CSI/控制面故障
   ├── CSI 驱动 Pod 崩溃
   ├── 卷挂载/卸载失败
   ├── 快照操作超时
   └── 扩容操作失败

4. 数据一致性故障
   ├── 写入中断 (power loss 模拟)
   ├── 部分写入 (torn write)
   ├── 副本不一致
   └── 元数据损坏
```

### 混沌实验安全等级

| 等级 | 环境 | 数据风险 | 审批要求 | 示例 |
|------|------|---------|---------|------|
| L1 安全 | 开发/测试 | 无 | 无需 | 只读延迟注入 |
| L2 低风险 | 预生产 | 可恢复 | 团队 Lead | CSI Pod 重启 |
| L3 中风险 | 生产(非核心) | 可能丢失 | SRE Manager | 卷 detach |
| L4 高风险 | 生产(核心) | 高概率丢失 | VP/Director | 磁盘格式化 |

### 实验设计原则

1. **假设驱动**：每次实验验证一个明确的韧性假设
2. **爆炸半径控制**：从最小影响范围开始，逐步扩大
3. **可观测性先行**：实验前确保监控和日志覆盖
4. **自动回滚**：设置超时自动恢复机制
5. **数据备份**：实验前确认备份有效且可恢复
6. **生产豁免**：核心数据路径的破坏性实验仅在维护窗口执行

## 生产部署

### Chaos Mesh IOChaos 配置

🔴 高风险：I/O 故障注入会直接影响目标 Pod 的数据读写

```yaml
# 模拟存储高延迟（验证训练任务对 I/O 延迟的容忍度）
apiVersion: chaos-mesh.org/v1alpha1
kind: IOChaos
metadata:
  name: storage-latency-injection
  namespace: ai-training
spec:
  action: latency
  mode: one  # 只影响一个 Pod（控制爆炸半径）
  selector:
    namespaces:
      - ai-training
    labelSelectors:
      app: training-worker
      chaos-target: "true"  # 只影响标记的 Pod
  volumePath: /data
  delay: "500ms"  # 注入 500ms I/O 延迟
  percent: 50  # 50% 的 I/O 操作受影响
  duration: "5m"
  scheduler:
    cron: "@every 30m"  # 每 30 分钟执行一次
---
# 模拟 I/O 错误（验证应用对读写错误的处理）
apiVersion: chaos-mesh.org/v1alpha1
kind: IOChaos
metadata:
  name: storage-io-error
  namespace: ai-training
spec:
  action: fault
  mode: fixed
  value: "1"  # 只影响 1 个 Pod
  selector:
    namespaces:
      - ai-training
    labelSelectors:
      app: training-worker
      chaos-target: "true"
  volumePath: /data
  errno: 5  # EIO (I/O error)
  percent: 10  # 10% 的 I/O 返回错误
  duration: "2m"
---
# 模拟磁盘满（验证容量告警和应用降级）
apiVersion: chaos-mesh.org/v1alpha1
kind: IOChaos
metadata:
  name: storage-space-full
  namespace: ai-training
spec:
  action: attrOverride
  mode: one
  selector:
    namespaces:
      - ai-training
    labelSelectors:
      app: checkpoint-writer
  volumePath: /checkpoints
  attr:
    size: 0  # 报告磁盘已满
  percent: 100
  duration: "3m"
```

### LitmusChaos 存储实验

🔴 高风险：CSI 卷操作可能导致数据暂时不可用

```yaml
# LitmusChaos: CSI 卷 detach/attach 实验
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: csi-volume-detach
  namespace: litmus
spec:
  appinfo:
    appns: ai-training
    applabel: app=training-worker
    appkind: statefulset
  engineState: active
  chaosServiceAccount: litmus-sa
  experiments:
    - name: csi-volume-detach
      spec:
        probe:
          - name: check-data-integrity
            type: cmdProbe
            cmdProbe/commands:
              command: "md5sum /data/training-data/checksum.md5"
              comparator:
                type: string
                criteria: ==
                value: "<expected-checksum>"
            runProperties:
              probeTimeout: 30
              interval: 10
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: "60"
            - name: VOLUME_NAME
              value: "training-data-pvc"
            - name: APP_NAMESPACE
              value: "ai-training"
---
# LitmusChaos: Pod 存储网络分区
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: storage-network-partition
  namespace: litmus
spec:
  appinfo:
    appns: ai-training
    applabel: app=minio-client
    appkind: deployment
  engineState: active
  chaosServiceAccount: litmus-sa
  experiments:
    - name: pod-network-partition
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: "120"
            - name: NETWORK_INTERFACE
              value: "eth0"
            - name: DESTINATION_HOSTS
              value: "minio.ai-platform.svc.cluster.local"
            - name: DESTINATION_PORTS
              value: "9000"
```

### PV/PVC 故障场景

🔴 高风险：直接操作 PV 可能导致数据丢失

```yaml
# 模拟 PVC 绑定失败（测试调度器行为）
apiVersion: chaos-mesh.org/v1alpha1
kind: KernelChaos
metadata:
  name: block-device-fault
  namespace: ai-training
spec:
  mode: one
  selector:
    namespaces:
      - ai-training
    labelSelectors:
      chaos-target: "true"
  failKernRequest:
    callchain:
      - funcname: "bio_alloc"
    probability: 20  # 20% 概率触发
    times: 10
  duration: "3m"
```

## 运维操作

### 数据一致性验证

🟢 低风险/只读：验证数据完整性

```bash
# 实验前：生成数据校验和
kubectl exec -n ai-training training-worker-0 -- \
  find /data -type f -exec md5sum {} \; | sort > /tmp/pre-chaos-checksum.md5

# 实验后：验证数据完整性
kubectl exec -n ai-training training-worker-0 -- \
  find /data -type f -exec md5sum {} \; | sort > /tmp/post-chaos-checksum.md5

# 对比校验和
diff /tmp/pre-chaos-checksum.md5 /tmp/post-chaos-checksum.md5

# 验证文件系统一致性（只读检查）
kubectl exec -n ai-training training-worker-0 -- \
  xfs_repair -n /dev/nvme0n1p1 2>&1 || echo "需要修复"

# 验证应用层数据一致性
kubectl exec -n ai-training training-worker-0 -- \
  python -c "
import torch
ckpt = torch.load('/checkpoints/latest.pt', map_location='cpu')
print(f'Checkpoint epoch: {ckpt[\"epoch\"]}')
print(f'Model params checksum: {hash(str(ckpt[\"model_state_dict\"]))}')
"
```

### CSI 驱动容错测试

🟡 中风险：重启 CSI 组件会暂时影响卷操作

```bash
# 测试 CSI Controller 故障恢复
# 🟡 中风险：短暂影响卷供给能力
kubectl delete pod -n kube-system -l app=ebs-csi-controller

# 观察 CSI 恢复时间
kubectl get pods -n kube-system -l app=ebs-csi-controller -w

# 测试 CSI Node Plugin 故障
# 🟡 中风险：影响目标节点的卷挂载
kubectl delete pod -n kube-system -l app=ebs-csi-node --field-selector spec.nodeName=node-01

# 验证已挂载卷不受影响
kubectl exec -n ai-training training-worker-0 -- ls /data/

# 测试新 PVC 创建在 CSI 恢复后是否正常
kubectl apply -f test-pvc.yaml
kubectl get pvc test-pvc -n ai-training -w
```

### 演练剧本模板

```markdown
# 存储混沌演练剧本

## 演练信息
- 日期: YYYY-MM-DD
- 环境: [生产/预生产/测试]
- 安全等级: [L1-L4]
- 参与人员: [SRE/开发/观察员]

## 演练目标
- 假设: [例] MinIO 单节点故障不影响训练数据读取
- 验证指标: [例] 训练任务无中断，GPU 利用率不下降

## 前置检查
- [ ] 备份已验证可恢复
- [ ] 监控面板已打开
- [ ] 回滚方案已确认
- [ ] 相关团队已通知

## 实验步骤
1. 记录基线指标 (GPU util, I/O latency, throughput)
2. 注入故障: [具体命令]
3. 观察系统响应 (等待 N 秒)
4. 记录影响指标
5. 恢复故障: [具体命令]
6. 验证数据一致性
7. 确认系统恢复正常

## 结果记录
- 故障检测时间: ___s
- 服务影响时间: ___s
- 数据丢失: [是/否]
- 自动恢复: [是/否]

## 改进项
- [ ] ...
```

## 故障排查

### 混沌实验失控处理

🔴 高风险：紧急停止实验并恢复

```bash
# 紧急停止所有 Chaos Mesh 实验
kubectl delete ioChaos --all -n ai-training
kubectl delete networkChaos --all -n ai-training
kubectl delete podChaos --all -n ai-training

# 紧急停止 LitmusChaos 实验
kubectl patch chaosengine csi-volume-detach -n litmus \
  --type merge -p '{"spec":{"engineState":"stop"}}'

# 检查是否有残留影响
kubectl get pods -n ai-training -o wide
kubectl get pvc -n ai-training
kubectl get events -n ai-training --sort-by='.lastTimestamp' | tail -20

# 如果 CSI 驱动异常，强制重启
kubectl rollout restart daemonset ebs-csi-node -n kube-system
```

### 实验后数据恢复

| 场景 | 恢复方法 | 预计时间 | 数据风险 |
|------|---------|---------|---------|
| I/O 延迟注入 | 自动恢复（duration 到期） | 即时 | 无 |
| 网络分区 | 删除 NetworkChaos CR | 即时 | 无 |
| CSI Pod 崩溃 | 自动重启（DaemonSet） | 30-60s | 无 |
| 卷 detach | 重新 attach（CSI 自动） | 1-5min | 低 |
| 数据损坏 | 从快照/备份恢复 | 10-60min | 中 |
| 文件系统损坏 | fsck 修复 | 5-30min | 高 |

## 最佳实践

1. **渐进式演练**：从 L1（只读延迟）开始，逐步升级到 L3（卷操作），永远不在未验证的环境执行 L4
2. **标记目标 Pod**：使用 `chaos-target: "true"` 标签精确控制实验范围，避免影响非目标工作负载
3. **AI Checkpoint 验证**：每次存储混沌实验后验证最新 Checkpoint 可正常加载和恢复训练
4. **自动化集成**：将存储混沌实验纳入 CI/CD 流水线（预生产环境），参考 [[可靠性/混沌工程/08-chaos-engineering-platforms.md|混沌工程平台]]
5. **Game Day 制度化**：每季度执行一次存储 Game Day，全团队参与，参考 [[可靠性/灾难恢复/17-disaster-recovery-drills.md|灾备演练]]
6. **监控联动**：实验期间自动触发告警，验证告警链路完整性，参考 [[存储/K8s存储/12-storage-monitoring-alerting.md|存储监控告警]]
7. **文档化发现**：每次实验产出改进项，更新 Runbook 和架构设计
8. **生产保护**：生产环境实验必须有自动回滚机制和人工终止开关
9. **与备份联动**：实验前确认 [[存储/AI存储与高级/09-velero-production-deep-dive.md|Velero 备份]] 有效，作为最后安全网

## Related

- [[可靠性/混沌工程/08-chaos-engineering-platforms.md|混沌工程平台]]
- [[可靠性/灾难恢复/17-disaster-recovery-drills.md|灾备演练]]
- [[存储/AI存储与高级/09-velero-production-deep-dive.md|Velero 生产深度指南]]
- [[存储/K8s存储/12-storage-monitoring-alerting.md|存储监控告警]]
- [[概念/chaos-engineering-platforms.md|混沌工程平台概念]]
