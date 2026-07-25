---
title: NetworkPolicy默认拒绝导致CI/CD流水线全中断
summary: NetworkPolicy默认拒绝导致CI/CD流水线全中断：安全团队部署了默认拒绝的NetworkPolicy后，CI/CD流水线全部失败，无法从镜像仓库拉取镜像和推送构建产物。
category: synthesis
tags:
- synthesis
tier: core
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-05-10'
skill: 20-networkpolicy-connectivity
severity: P1
last_updated: 2026-05-23
---



# NetworkPolicy默认拒绝导致CI/CD流水线全中断

**日期**: 2026-05-10  
**关联Skill**: [[20-networkpolicy-connectivity]]  
**严重级别**: P1

## 场景描述
安全团队部署了默认拒绝的NetworkPolicy后，CI/CD流水线全部失败，无法从镜像仓库拉取镜像和推送构建产物。

## 时间线
10:00 安全团队部署默认deny-all NetworkPolicy
10:05 CI/CD流水线开始失败
10:15 检查Pod状态：ImagePullBackOff和ErrImagePull
10:20 检查NetworkPolicy：发现命名空间级别默认拒绝所有入口和出口
10:30 确认CI/CD Pod需要访问外部镜像仓库和Git仓库
10:45 添加Egress规则允许CI/CD访问必要的外部服务
11:00 流水线恢复正常
11:30 制定NetworkPolicy变更流程：必须包含影响评估和回滚方案

## 根因分析
安全团队未进行影响评估直接部署默认拒绝策略，未考虑CI/CD服务对外部资源的依赖。

## 影响评估
所有CI/CD流水线中断约1小时，延迟了3个生产版本的发布。

## 教训与预防
1. NetworkPolicy变更必须经过影响评估
2. 部署前应使用网络策略模拟工具验证
3. 始终保留快速回滚路径
## Related

- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[concepts/case-studies/2026-07-15--admission-webhook超时导致所有api操作失败.md|2026-07-15--admission-webhook超时导致所有api操作失败]]
