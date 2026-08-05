---
title: Secret未更新导致Rolling Update新旧版本配置不一致
summary: Secret未更新导致Rolling Update新旧版本配置不一致：Secret更新后，Deployment执行Rolling Update，但旧版本Pod仍使用旧Secret，导致新旧Pod行为不一致，用户请求出现随机认证失败。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-10-25'
skill: 14-configmap-secret-failure
severity: P1
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Secret未更新导致Rolling Update新旧版本配置不一致

**日期**: 2026-10-25  
**关联Skill**: [[15-configmap-secret-failure]]  
**严重级别**: P1

## 场景描述
Secret更新后，Deployment执行Rolling Update，但旧版本Pod仍使用旧Secret，导致新旧Pod行为不一致，用户请求出现随机认证失败。

## 时间线
11:00 更新数据库密码Secret
11:05 执行Deployment Rolling Update
11:10 用户报告部分请求认证失败（约50%）
11:20 检查Pod Secret挂载：新Pod使用新密码，旧Pod仍使用旧密码
11:30 确认根因：Rolling Update期间新旧版本共存，旧Pod未重启导致仍使用旧Secret
11:40 强制重启所有旧Pod：kubectl rollout restart deployment/<d>
11:50 所有Pod使用新Secret，问题消失
12:00 制定Secret变更流程：更新Secret后必须强制滚动更新

## 根因分析
Secret更新不会自动触发Pod重启。Rolling Update仅更新镜像，旧Pod继续使用已挂载的旧Secret（通过subPath挂载的Secret不会自动更新）。

## 影响评估
约50%的用户请求随机失败，持续约40分钟，影响用户信任。

## 教训与预防
1. 更新Secret后必须执行kubectl rollout restart
2. 避免使用subPath挂载Secret（阻止自动更新）
3. 考虑使用reloader等工具自动触发Secret变更后的重启
## Related

- [[22-概念/14-case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[22-概念/14-case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]


<!-- risk-assessed -->
