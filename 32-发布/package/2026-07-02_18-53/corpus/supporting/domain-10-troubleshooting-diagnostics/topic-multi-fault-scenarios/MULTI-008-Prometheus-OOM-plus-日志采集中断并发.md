---
title: Prometheus OOM + 日志采集中断并发
summary: Prometheus OOM + 日志采集中断并发：监控系统Prometheus因高基数指标OOM，同时日志采集代理Fluentd缓冲溢出导致日志丢失。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-008
type: multi-fault
skills:
- 15-monitoring-alerting-failure
- 16-logging-pipeline-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Prometheus OOM + 日志采集中断并发

## 关联Skill
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-skills/13-monitoring-alerting-failure]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-10-troubleshooting-diagnostics/topic-skills/14-logging-pipeline-failure]]

## 场景描述
监控系统Prometheus因高基数指标OOM，同时日志采集代理Fluentd缓冲溢出导致日志丢失。

## 根因分析
应用暴露的高基数标签导致Prometheus内存激增，同时日志量突增超过后端处理能力。

## 诊断流程
1. 检查Prometheus: kubectl get pods -n monitoring -l app=prometheus
2. 检查Prometheus日志: kubectl logs prometheus-k8s-0 -n monitoring --tail=50
3. 检查高基数指标: curl -s http://prometheus:9090/api/v1/label/__name__/values | wc -l
4. 检查Fluentd: kubectl get pods -n logging
5. 检查Fluentd缓冲: kubectl exec <fluentd-pod> -n logging -- ls -la /var/log/fluentd-buffer

## 修复方案
1. 限制Prometheus采集目标或增加内存限制
2. 优化应用标签减少高基数指标
3. 增加日志后端处理能力或扩容
4. 清理Fluentd缓冲并调整flush策略
5. 配置指标基数告警和日志延迟告警

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
