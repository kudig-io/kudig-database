---
title: 节点NotReady + DNS解析失败并发
summary: 节点NotReady + DNS解析失败并发：多个节点同时进入NotReady状态，同时工程师报告Pod间DNS解析间歇性失败。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-001
type: multi-fault
skills:
- 01-node-notready
- 04-dns-resolution-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点NotReady + DNS解析失败并发

## 关联Skill
- [[01-node-notready]]
- [[04-dns-resolution-failure]]

## 场景描述
多个节点同时进入NotReady状态，同时工程师报告Pod间DNS解析间歇性失败。

## 根因分析
节点资源压力（磁盘满）导致kubelet停止上报，同时CoreDNS副本被调度到NotReady节点上被驱逐，导致DNS服务可用副本数不足。

## 诊断流程
1. 确认节点状态: kubectl get nodes
2. 确认CoreDNS副本状态: kubectl get pods -n kube-system -l k8s-app=kube-dns
3. 检查节点磁盘: kubectl describe node <node> | grep -A5 Conditions
4. 检查CoreDNS日志: kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
5. 确认DNS解析: kubectl run test --rm -it -- nslookup kubernetes.default

## 修复方案
1. 清理节点磁盘: ssh <node> crictl rmi --prune && journalctl --vacuum-time=1d
2. 重启kubelet: ssh <node> systemctl restart kubelet
3. 扩容CoreDNS副本: kubectl scale deployment coredns -n kube-system --replicas=5
4. 添加反亲和性避免CoreDNS副本集中到同一节点
5. 验证: kubectl get nodes && kubectl run test --rm -it -- nslookup kubernetes.default

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）

## 时间线还原

| 时间 | 事件 | 操作 |
|------|------|------|
| 03:00 | 监控告警: node-3 DiskPressure | 🟢 `kubectl describe node node-3 \| grep Conditions` |
| 03:02 | node-3, node-5 进入 NotReady | 🟢 `kubectl get nodes -w` |
| 03:03 | CoreDNS Pod 被驱逐(2/3副本丢失) | 🟢 `kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide` |
| 03:04 | 业务报告 DNS 解析间歇性失败 | 🟢 `kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default` |
| 03:10 | 确认根因: 日志未轮转导致磁盘满 | 🟢 `ssh node-3 df -h /var/lib/docker` |
| 03:15 | 清理磁盘 + 重启 kubelet | 🟡 `crictl rmi --prune && systemctl restart kubelet` |
| 03:20 | 节点恢复 Ready，CoreDNS 重新调度 | 🟢 `kubectl get nodes && kubectl get pods -n kube-system -l k8s-app=kube-dns` |
| 03:25 | DNS 解析完全恢复 | 🟢 `nslookup kubernetes.default` |

## 故障关联图

```
磁盘未轮转(根因)
    ├── kubelet DiskPressure → 节点 NotReady
    │       └── CoreDNS Pod 被驱逐
    │               └── DNS 可用副本不足
    │                       └── 服务间解析失败
    └── 影响范围: 所有依赖 DNS 的服务
```

## 关键教训

1. **级联效应**: 单一根因(磁盘满)通过级联引发多个看似无关的故障
2. **反亲和性重要性**: CoreDNS 未配置 Pod 反亲和，多副本集中在同一节点
3. **监控盲区**: 磁盘使用率告警未配置，直到 DiskPressure 才发现

## 面试要点

1. **Q: 如何判断多个故障是否有关联？**
   A: 检查时间线是否重叠 → 分析是否共享根因 → 确认影响链是否连贯 → 排除巧合(概率分析)

2. **Q: 并发故障的优先级排序？**
   A: 先恢复影响面最大的(DNS) → 再修复根因(磁盘) → 最后预防复发(监控+反亲和)

3. **Q: 如何避免类似级联故障？**
   A: 关键组件反亲和 + 磁盘监控告警 + 日志轮转 + PDB 保护 + 混沌工程演练

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
