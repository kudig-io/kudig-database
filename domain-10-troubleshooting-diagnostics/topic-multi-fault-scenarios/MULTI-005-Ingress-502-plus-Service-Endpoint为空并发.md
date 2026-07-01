---
scenario_id: "MULTI-005"
type: "multi-fault"
skills: ['13-ingress-gateway-failure', '05-service-connectivity']
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
title: "Ingress 502 + Service Endpoint为空并发"
category: uncategorized
tags: ["uncategorized", "visibility/public"]
---

# Ingress 502 + Service Endpoint为空并发

## 关联Skill
- [[13-ingress-gateway-failure]]
- [[05-service-connectivity]]

## 场景描述
外部用户报告服务返回502，同时Ingress后端Service的Endpoint列表为空，Pod处于Pending状态。

## 根因分析
资源配额超限导致新Pod无法调度（Pending），Service Endpoint为空，Ingress无法路由到可用后端。

## 诊断流程
1. 检查Ingress: kubectl get ingress -n <ns>
2. 检查Service Endpoint: kubectl get endpoints <svc> -n <ns>
3. 检查Pod状态: kubectl get pods -n <ns>
4. 检查ResourceQuota: kubectl describe resourcequota -n <ns>
5. 检查事件: kubectl get events -n <ns> --field-selector reason=FailedScheduling

## 修复方案
1. 调整ResourceQuota: kubectl patch resourcequota <q> -n <ns>
2. 增加节点或清理资源
3. 等待Pod调度成功后验证Endpoint
4. 验证Ingress: curl -H Host:<host> http://<ingress-ip>/
5. 监控配额使用率并设置告警

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）
