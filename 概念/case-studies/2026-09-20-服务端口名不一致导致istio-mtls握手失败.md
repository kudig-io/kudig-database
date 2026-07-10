---
title: 服务端口名不一致导致Istio mTLS握手失败
summary: 服务端口名不一致导致Istio mTLS握手失败：启用Istio严格mTLS后，部分服务间调用返回503。检查发现Service端口名与DestinationRule端口名不一致。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-09-20'
skill: 13-ingress-gateway-failure
severity: P1
last_updated: 2026-05-23
---



# 服务端口名不一致导致Istio mTLS握手失败

**日期**: 2026-09-20  
**关联Skill**: [[13-ingress-gateway-failure]]  
**严重级别**: P1

## 场景描述
启用Istio严格mTLS后，部分服务间调用返回503。检查发现Service端口名与DestinationRule端口名不一致。

## 时间线
09:00 启用严格mTLS后部分服务返回503
09:15 检查Istio代理日志：TLS handshake error
09:25 检查DestinationRule：端口名配置为http-api
09:30 检查Service：端口名配置为http
09:35 确认根因：Istio使用端口名匹配策略，名称不一致导致mTLS策略未生效
09:45 统一端口命名：Service和DestinationRule均使用http-api
10:00 mTLS握手成功，503错误消失
10:15 制定端口命名规范

## 根因分析
Istio的DestinationRule和Service使用端口名进行策略匹配。Service端口名为http，DestinationRule配置为http-api，名称不一致导致mTLS策略未正确应用。

## 影响评估
约15%的服务间调用失败，影响用户下单流程。

## 教训与预防
1. 使用Istio时必须统一端口命名
2. 建立端口命名规范（建议遵循Istio协议选择标准）
3. 启用mTLS前使用istioctl authn tls-check验证
## Related

- [[概念/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[概念/case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]
