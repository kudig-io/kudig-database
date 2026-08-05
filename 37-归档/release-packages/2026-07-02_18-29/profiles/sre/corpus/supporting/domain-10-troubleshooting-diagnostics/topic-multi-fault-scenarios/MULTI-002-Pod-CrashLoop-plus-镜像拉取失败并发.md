---
title: Pod CrashLoop + 镜像拉取失败并发
summary: Pod CrashLoop + 镜像拉取失败并发：新部署的服务所有Pod处于ImagePullBackOff，部分已存在的Pod反复CrashLoopBackOff。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-002
type: multi-fault
skills:
- 02-pod-crashloop-oomkilled
- 10-image-pull-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod CrashLoop + 镜像拉取失败并发

## 关联Skill
- [[02-pod-crashloop-oomkilled]]
- [[11-image-pull-failure]]

## 场景描述
新部署的服务所有Pod处于ImagePullBackOff，部分已存在的Pod反复CrashLoopBackOff。

## 根因分析
镜像仓库认证Secret过期导致新Pod无法拉取镜像，而已存在的Pod因内存泄漏导致OOMKilled后反复重启。

## 诊断流程
1. 检查Pod状态: kubectl get pods -n <ns>
2. 检查镜像拉取事件: kubectl describe pod <pod> -n <ns> | grep -A5 Events
3. 检查Secret: kubectl get secret <secret> -n <ns> -o yaml
4. 检查内存使用: kubectl top pod -n <ns>
5. 检查之前Pod的退出原因: kubectl get pod <pod> -n <ns> -o jsonpath={.status.containerStatuses[0].lastState}

## 修复方案
1. 更新镜像拉取Secret: kubectl create secret docker-registry <secret> --docker-server=<reg> --docker-username=<user> --docker-password=<pass> -n <ns>
2. 手动删除卡住的Pod让控制器重建
3. 增加内存限制: kubectl patch deployment <d> -p spec.template.spec.containers[0].resources.limits.memory=<new>
4. 检查应用内存泄漏并修复代码
5. 验证: kubectl get pods -n <ns> -w

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
