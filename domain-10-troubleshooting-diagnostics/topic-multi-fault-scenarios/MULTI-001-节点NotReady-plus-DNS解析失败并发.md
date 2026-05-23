---
scenario_id: "MULTI-001"
type: "multi-fault"
skills: ['01-node-notready', '04-dns-resolution-failure']
created: "2026-05-23"
updated: "2026-05-23"
---

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
