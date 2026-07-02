---
title: CoreDNS HPA配置错误导致DNS雪崩
summary: CoreDNS HPA配置错误导致DNS雪崩：业务团队报告服务间调用间歇性失败，错误日志显示DNS解析超时。问题呈现波浪式：正常5分钟后异常2分钟循环。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-02-12'
skill: 04-dns-resolution-failure
severity: P1
last_updated: 2026-05-23
---



# CoreDNS HPA配置错误导致DNS雪崩

**日期**: 2026-02-12  
**关联Skill**: [[04-dns-resolution-failure]]  
**严重级别**: P1

## 场景描述
业务团队报告服务间调用间歇性失败，错误日志显示DNS解析超时。问题呈现波浪式：正常5分钟后异常2分钟循环。

## 时间线
09:00 业务团队报告服务调用超时
09:15 检查发现CoreDNS Pod CPU使用率持续100%
09:20 发现CoreDNS HPA配置：minReplicas=2, maxReplicas=2（实际上限和下限相同）
09:25 CoreDNS因高负载处理不过来，部分DNS请求超时
09:30 业务Pod因DNS超时而重试，形成放大效应
09:45 手动扩容CoreDNS至5副本，问题缓解
10:00 修正HPA配置，设置合理的min/max和targetCPU

## 根因分析
CoreDNS HPA的minReplicas和maxReplicas被误设为相同值（2），导致HPA无法扩容。在高负载下CoreDNS CPU满载，DNS请求排队超时。

## 影响评估
约30%的服务间调用受到影响，部分用户请求延迟增加至5秒以上。

## 教训与预防
1. HPA配置必须设置maxReplicas > minReplicas
2. CoreDNS副本数应根据集群规模配置（建议每1000 Pod 1个副本）
3. DNS解析延迟应纳入SLO监控
## Related

- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[concepts/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]
