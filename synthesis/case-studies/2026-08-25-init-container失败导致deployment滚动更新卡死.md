---
category: "synthesis"
tags: ["synthesis"]
date: "2026-08-25"
title: "Init Container失败导致Deployment滚动更新卡死"
skill: "08-deployment-rollout-failure"
severity: "P1"
created: "2026-05-23"
updated: "2026-05-23"
---

# Init Container失败导致Deployment滚动更新卡死

**日期**: 2026-08-25  
**关联Skill**: [[08-deployment-rollout-failure]]  
**严重级别**: P1

## 场景描述
Deployment滚动更新进度卡在50%，新Pod无法就绪。检查发现Init Container在验证数据库连接时失败。

## 时间线
15:00 执行Deployment更新
15:05 滚动更新卡在50%，新Pod状态Init:Error
15:10 检查Init Container日志：数据库连接超时
15:15 确认数据库服务正常，但Init Container使用旧的数据库连接配置
15:20 发现ConfigMap未更新，Init Container读取的配置仍指向旧数据库地址
15:30 更新ConfigMap并重新部署
15:35 滚动更新恢复正常
15:45 建立配置变更检查清单：更新Deployment前确认关联ConfigMap/Secret已更新

## 根因分析
Deployment更新时只更新了应用镜像，未同步更新关联ConfigMap。Init Container使用旧配置连接数据库，导致初始化失败，新Pod无法就绪。

## 影响评估
滚动更新中断约35分钟，新旧版本混合运行期间出现数据不一致。

## 教训与预防
1. 建立配置变更检查清单
2. 使用Helm或Kustomize统一管理资源和配置
3. Init Container应有明确的超时和重试策略
## Related

- [[synthesis/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[synthesis/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]
