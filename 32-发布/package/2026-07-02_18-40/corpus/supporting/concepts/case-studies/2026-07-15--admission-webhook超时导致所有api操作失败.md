---
title: ' admission webhook超时导致所有API操作失败'
summary: ' admission webhook超时导致所有API操作失败：所有kubectl命令返回Internal error occurred: failed
  calling webhook，无法创建、更新或删除任何资源。'
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-07-15'
skill: 11-control-plane-failure
severity: P0
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




#  admission webhook超时导致所有API操作失败

**日期**: 2026-07-15  
**关联Skill**: [[12-control-plane-failure]]  
**严重级别**: P0

## 场景描述
所有kubectl命令返回Internal error occurred: failed calling webhook，无法创建、更新或删除任何资源。

## 时间线
11:00 所有kubectl操作失败，错误包含failed calling webhook
11:05 检查webhook配置：kubectl get validatingwebhookconfiguration
11:10 发现自定义admission webhook的service后端已不存在
11:15 webhook配置了failurePolicy: Fail，导致所有请求被阻断
11:20 临时删除问题webhook配置恢复服务
11:30 检查webhook部署历史：namespace被误删除导致service消失
11:45 重新部署webhook并验证
12:00 制定webhook管理规范：必须配置failurePolicy: Ignore或确保高可用

## 根因分析
自定义admission webhook的namespace被误删除，webhook service消失。failurePolicy配置为Fail，导致apiserver无法调用webhook而拒绝所有请求。

## 影响评估
整个集群API完全不可用约20分钟，所有自动化流程（CI/CD、自动扩缩容、监控告警）中断。

## 教训与预防
1. 自定义webhook必须配置failurePolicy: Ignore或Failover
2. webhook服务必须部署在高可用命名空间
3. 删除namespace前必须检查是否有关联webhook
## Related

- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[concepts/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]


<!-- risk-assessed -->
