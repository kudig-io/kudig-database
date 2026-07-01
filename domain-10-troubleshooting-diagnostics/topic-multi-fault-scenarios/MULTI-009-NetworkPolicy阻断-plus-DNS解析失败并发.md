---
scenario_id: "MULTI-009"
type: "multi-fault"
skills: ['20-networkpolicy-connectivity', '04-dns-resolution-failure']
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
title: "NetworkPolicy阻断 + DNS解析失败并发"
category: uncategorized
tags: ["uncategorized", "visibility/public"]
---

# NetworkPolicy阻断 + DNS解析失败并发

## 关联Skill
- [[20-networkpolicy-connectivity]]
- [[04-dns-resolution-failure]]

## 场景描述
新应用上线后无法访问外部API，同时集群内部DNS解析间歇性超时。

## 根因分析
新部署的NetworkPolicy默认拒绝所有出口流量，同时CoreDNS因网络策略被限制无法访问上游DNS。

## 诊断流程
1. 检查NetworkPolicy: kubectl get networkpolicy -n <ns>
2. 检查CoreDNS连通性: kubectl exec <pod> -n <ns> -- nc -zv coredns.kube-system.svc.cluster.local 53
3. 检查DNS解析: kubectl exec <pod> -n <ns> -- nslookup <external-domain>
4. 检查CoreDNS配置: kubectl get configmap coredns -n kube-system -o yaml
5. 测试策略放通: kubectl run test --rm -it --image=busybox -- wget -O- <external-api>

## 修复方案
1. 修改NetworkPolicy添加允许的出口规则（DNS端口53、外部API地址）
2. 添加Egress策略放通CoreDNS和外部依赖
3. 验证DNS解析和外部访问
4. 使用命名空间级策略替代全局默认拒绝
5. 部署前进行NetworkPolicy影响评估

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）
