---
title: Pod Disruption Budget阻止节点维护排空
summary: Pod Disruption Budget阻止节点维护排空：执行节点维护时，kubectl drain命令卡住无法完成。检查发现Pod Disruption
  Budget配置过于严格。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-10-15'
skill: 01-node-notready
severity: P2
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Disruption Budget阻止节点维护排空

**日期**: 2026-10-15  
**关联Skill**: [[01-node-notready]]  
**严重级别**: P2

## 场景描述
执行节点维护时，kubectl drain命令卡住无法完成。检查发现Pod Disruption Budget配置过于严格。

## 时间线
02:00 执行节点维护，kubectl drain <node>
02:05 drain命令卡住，提示evicting pod xxxx (disruption budget)
02:10 检查PDB：minAvailable=3，但当前只有3个副本
02:15 确认根因：PDB不允许任何Pod被驱逐，drain无法完成
02:20 临时放宽PDB：kubectl patch pdb <pdb> -p '{"spec":{"minAvailable":2}}'
02:25 drain成功完成
02:30 恢复PDB配置
02:45 建立维护窗口PDB调整流程

## 根因分析
PDB的minAvailable等于当前副本数，意味着不允许任何Pod被驱逐。节点维护时必须驱逐Pod，PDB阻止了这一操作。

## 影响评估
节点维护延迟约30分钟，维护窗口超时。

## 教训与预防
1. PDB配置应留有余量（如minAvailable=replicas-1）
2. 维护窗口期间临时放宽PDB
3. 建立维护前PDB检查清单
## Related

- [[22-概念/14-案例研究/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[22-概念/14-案例研究/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]


<!-- risk-assessed -->
