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
- [[12-autoscaling-failure]]
- [[19-node-resource-pressure]]

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

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
