---
title: HPA不扩容 + 节点资源压力并发
summary: HPA不扩容 + 节点资源压力并发：业务高峰期HPA未触发扩容，同时多个节点因磁盘压力进入DiskPressure状态，Pod被驱逐。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-004
type: multi-fault
skills:
- 12-autoscaling-failure
- 19-node-resource-pressure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HPA不扩容 + 节点资源压力并发

## 关联Skill
- [[13-autoscaling-failure]]
- [[20-node-resource-pressure]]

## 场景描述
业务高峰期HPA未触发扩容，同时多个节点因磁盘压力进入DiskPressure状态，Pod被驱逐。

## 根因分析
metrics-server问题导致HPA无法获取CPU指标，同时节点日志堆积导致磁盘压力触发Pod驱逐。

## 诊断流程
1. 检查HPA状态: kubectl describe hpa <hpa> -n <ns>
2. 检查metrics-server: kubectl get pods -n kube-system | grep metrics-server
3. 检查节点状态: kubectl describe node <node> | grep -A10 Conditions
4. 检查节点磁盘: kubectl get node <node> -o json | jq .status.conditions
5. 检查Pod驱逐事件: kubectl get events --field-selector reason=Evicted

## 修复方案
1. 重启metrics-server: kubectl rollout restart deployment metrics-server -n kube-system
2. 清理节点磁盘: ssh <node> crictl rmi --prune && journalctl --vacuum-time=1d
3. 临时手动扩容: kubectl scale deployment <d> --replicas=<n> -n <ns>
4. 调整Pod资源限制避免过度调度
5. 配置日志轮转和磁盘告警

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
| 14:00 | 流量激增，HPA 应触发扩容 | 🟢 `kubectl get hpa -n prod -o wide` |
| 14:02 | HPA 显示 `FailedGetResourceMetric` | 🟢 `kubectl describe hpa ${HPA} -n prod` |
| 14:03 | 节点 CPU >95%，Memory Pressure | 🟢 `kubectl top nodes` |
| 14:05 | metrics-server OOMKilled | 🟢 `kubectl get pods -n kube-system -l k8s-app=metrics-server` |
| 14:10 | 确认根因: 节点资源压力导致 metrics-server 被驱逐 | 🟢 `kubectl describe node ${NODE} \| grep -A5 Conditions` |
| 14:15 | 修复 metrics-server + 扩容节点 | 🟡 `kubectl rollout restart deployment metrics-server -n kube-system` |
| 14:20 | HPA 正常扩容，服务恢复 | 🟢 `kubectl get hpa -n prod -w` |

## 故障关联图

```
流量激增(触发因素)
    ├── 节点资源压力增大
    │       └── metrics-server OOMKilled
    │               └── HPA无法获取指标
    │                       └── 扩容失败
    └── 现有Pod过载 → 响应变慢 → 用户体验下降
```

## 关键教训

1. **metrics-server 单点**: 未配置多副本和反亲和
2. **资源预留不足**: 节点未预留系统组件资源
3. **HPA 容错**: 未配置备用指标源(Prometheus Adapter)

## 面试要点

1. **Q: HPA 和节点资源压力同时出现的处理优先级？**
   A: 先恢复 metrics-server(让 HPA 能工作) → 再扩容节点(解决资源压力) → 最后验证 HPA 正常扩容

2. **Q: 如何避免 metrics-server 单点故障？**
   A: 多副本 + 反亲和 + 资源预留 + 备用指标源(Prometheus Adapter) + 监控自监控

3. **Q: 节点资源压力的紧急处理？**
   A: 识别压力源(Disk/Memory/PID) → 清理无用资源 → 驱逐低优先级 Pod → 扩容节点 → 配置资源预留

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
