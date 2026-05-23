---
scenario_id: "MULTI-006"
type: "multi-fault"
skills: ['09-rbac-quota-failure', '10-image-pull-failure']
created: "2026-05-23"
updated: "2026-05-23"
title: "RBAC权限不足 + 镜像拉取失败并发"
category: uncategorized
tags: ["uncategorized", "visibility/public"]
---

# RBAC权限不足 + 镜像拉取失败并发

## 关联Skill
- [[09-rbac-quota-failure]]
- [[10-image-pull-failure]]

## 场景描述
CI/CD流水线部署失败，Pod报告ImagePullBackOff，同时服务账户无权限创建ConfigMap。

## 根因分析
ServiceAccount缺少imagePullSecret导致镜像拉取失败，同时缺少create configmap权限导致部署流程中断。

## 诊断流程
1. 检查Pod事件: kubectl describe pod <pod> -n <ns> | grep -A10 Events
2. 检查SA权限: kubectl auth can-i create configmaps --as system:serviceaccount:<ns>:<sa> -n <ns>
3. 检查SA的imagePullSecret: kubectl get sa <sa> -n <ns> -o yaml
4. 检查Secret: kubectl get secret -n <ns> | grep docker
5. 检查Role: kubectl get role -n <ns> -o yaml

## 修复方案
1. 创建docker-registry Secret并绑定SA
2. 创建Role并绑定SA: kubectl create role <role> --verb=create --resource=configmaps -n <ns> && kubectl create rolebinding <rb> --role=<role> --serviceaccount=<ns>:<sa> -n <ns>
3. 重新部署验证
4. 使用最小权限原则定期审计RBAC

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）
