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
- 07-pvc-storage-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# StatefulSet PVC未绑定 + 节点NotReady并发

## 关联Skill
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-10-troubleshooting-diagnostics/topic-skills/07-statefulset-failure]]
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

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
