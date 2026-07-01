---
category: "synthesis"
tags: ["synthesis"]
date: "2026-04-05"
title: "StorageClass动态供应失败导致所有有状态应用中断"
skill: "07-pvc-storage-failure"
severity: "P0"
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# StorageClass动态供应失败导致所有有状态应用中断

**日期**: 2026-04-05  
**关联Skill**: [[07-pvc-storage-failure]]  
**严重级别**: P0

## 场景描述
数据库集群全部Pod处于Pending状态，事件显示PVC未绑定。问题发生在所有使用动态供应的命名空间。

## 时间线
01:00 数据库告警全部Pod Pending
01:05 检查PVC状态：全部Pending
01:10 检查StorageClass：存在但Provisioner Pod已崩溃
01:15 检查Provisioner日志：连接存储后端超时
01:20 发现存储后端（云盘服务）维护窗口导致API不可用
01:30 存储后端恢复，但Provisioner未自动重连
01:45 手动重启Provisioner Pod
02:00 PVC开始逐个绑定
02:30 所有数据库Pod恢复Running

## 根因分析
云盘服务计划维护期间API不可用，CSI Provisioner连接失败后进入崩溃循环。维护结束后Provisioner未自动恢复，导致所有动态PVC供应中断。

## 影响评估
所有有状态应用（数据库、缓存、消息队列）中断约1.5小时，数据面未丢失但服务完全不可用。

## 教训与预防
1. 存储后端维护前需提前通知并制定预案
2. CSI组件应配置健康检查和自动重启
3. 关键有状态应用应使用预创建PV而非动态供应
## Related

- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[concepts/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]
