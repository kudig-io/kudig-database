---
title: StatefulSet PVC未绑定 + 节点NotReady并发
summary: StatefulSet PVC未绑定 + 节点NotReady并发：数据库StatefulSet无法启动，PVC未绑定，同时承载PV的存储节点进入NotReady状态。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-007
type: multi-fault
skills:
- 21-statefulset-failure
- 01-node-notready
- 08-pvc-storage-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# StatefulSet PVC未绑定 + 节点NotReady并发

## 关联Skill
- [[23-statefulset-failure]]
- [[01-node-notready]]
- [[08-pvc-storage-failure]]

## 场景描述
数据库StatefulSet无法启动，PVC未绑定，同时承载PV的存储节点进入NotReady状态。

## 根因分析
存储节点问题导致PV不可访问，StatefulSet的PVC无法绑定，数据库服务完全中断。

## 诊断流程
1. 检查StatefulSet: kubectl get statefulset -n <ns>
2. 检查PVC: kubectl get pvc -n <ns>
3. 检查PV: kubectl get pv
4. 检查节点: kubectl get nodes
5. 检查StorageClass: kubectl get storageclass

## 修复方案
1. 恢复存储节点或迁移PV数据
2. 检查并修复StorageClass动态供应配置
3. 手动创建PV匹配Pending的PVC
4. 验证StatefulSet恢复: kubectl get pods -l app=<app> -n <ns>
5. 配置存储高可用和节点监控

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）

## 时间线还原

| 时间 | 事件 | 操作 |
|------|------|------|
| 08:00 | 节点故障 NotReady，StatefulSet Pod 被驱逐 | 🟢 `kubectl get nodes -o wide` |
| 08:02 | Pod 重新调度但 PVC 无法绑定(云盘在旧节点) | 🟢 `kubectl describe pvc ${PVC} -n ${NS}` |
| 08:05 | Multi-Attach 错误: 云盘仍 attach 在旧节点 | 🟢 `kubectl get volumeattachment -o wide` |
| 08:10 | 确认根因: 旧节点未完全下线，云盘未 detach | 🟢 `kubectl get nodes ${OLD_NODE} -o jsonpath='{.spec.taints}'` |
| 08:15 | 强制删除旧 Pod + 等待云盘 detach | 🟡 `kubectl delete pod ${POD} -n ${NS} --force --grace-period=0` |
| 08:20 | PVC 绑定成功，Pod 启动 | 🟢 `kubectl get pods -n ${NS} -w` |

## 故障关联图

```
节点故障(触发因素)
    ├── 节点NotReady → Pod被驱逐
    │       └── 云盘未detach(旧节点未完全下线)
    │               └── Multi-Attach错误
    │                       └── PVC无法绑定到新节点
    └── StatefulSet有序部署阻塞 → 后续Pod不更新
```

## 关键教训

1. **云盘 detach 延迟**: 节点故障后云盘 detach 需要时间(6min 默认)
2. **StatefulSet 有序性**: 一个 Pod 卡住会阻塞整个更新
3. **存储高可用**: 单节点故障不应影响数据可用性

## 面试要点

1. **Q: 节点故障后 StatefulSet Pod 无法重新调度的处理？**
   A: 等待云盘自动 detach(6min) → 或强制删除旧 Pod → 检查 VolumeAttachment → 确认 PVC 绑定 → Pod 重新调度

2. **Q: Multi-Attach 错误的根因和解决？**
   A: 云盘仍 attach 在旧节点 → 等待自动 detach → 或手动 detach → 确认 RWO 访问模式限制

3. **Q: 如何加速节点故障后的 Pod 恢复？**
   A: 配置 pod-eviction-timeout → 使用分布式存储(无 attach 限制) → 配置 node-problem-detector 快速检测

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
