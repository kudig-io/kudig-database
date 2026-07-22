---
title: dns failure Remediation Playbook
summary: dns failure Remediation Playbook：kubectl rollout restart deployment coredns
  -n kube-system kubectl rollout status deployment coredns -n kube-system
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-dns-failure
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DNS 解析问题修复手册

## 修复步骤

### 修复 1：重启 [[CoreDNS|CoreDNS]]

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment coredns -n kube-system
kubectl rollout status deployment coredns -n kube-system
```
### 修复 2：修正 CoreDNS ConfigMap

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get configmap coredns -n kube-system -o yaml
# 检查 Corefile 配置，修正后：
kubectl apply -f coredns-configmap-fixed.yaml
kubectl rollout restart deployment coredns -n kube-system
```
### 修复 3：扩大 CoreDNS 资源

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment coredns -n kube-system -p '{"spec":{"template":{"spec":{"containers":[{"name":"coredns","resources":{"limits":{"memory":"256Mi","cpu":"500m"}}}]}}}}'
```
### 修复 4：修正 Pod DNS 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pod <pod> --type merge -p '{"spec":{"dnsPolicy":"ClusterFirst","dnsConfig":{"nameservers":["10.96.0.10"],"searches":["default.svc.cluster.local","svc.cluster.local","cluster.local"]}}}'
```
### 修复 5：修复 NodeLocal DNSCache 异常

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认

``` bash
# 🟢 低风险：检查 NodeLocal DNS 状态
kubectl get pods -n kube-system -l k8s-app=node-local-dns -o wide
kubectl logs -n kube-system -l k8s-app=node-local-dns --tail=50

# 🟡 中风险：重启 NodeLocal DNS
kubectl rollout restart daemonset node-local-dns -n kube-system
```

### 修复 6：修复 Pod /etc/resolv.conf 配置异常

``` bash
# 🟢 低风险：检查 Pod DNS 配置
kubectl exec <pod> -- cat /etc/resolv.conf

# 🟡 中风险：通过 dnsConfig 修正（需重建 Pod）
# 在 Deployment spec 中添加:
# dnsPolicy: ClusterFirst
# dnsConfig:
#   nameservers: ["10.96.0.10"]
#   searches: ["default.svc.cluster.local", "svc.cluster.local", "cluster.local"]
#   options: [{name: ndots, value: "5"}, {name: timeout, value: "2"}, {name: attempts, value: "2"}]
```

## 验证方法

``` bash
# 🟢 低风险：验证 DNS 解析恢复正常
# 1. 集群内域名解析测试
kubectl run dns-test --rm -it --image=busybox -- nslookup kubernetes.default.svc.cluster.local

# 2. 外部域名解析测试
kubectl run dns-test --rm -it --image=busybox -- nslookup www.example.com

# 3. 检查 CoreDNS 指标
kubectl exec -n kube-system deploy/coredns -- wget -qO- http://localhost:9153/metrics | grep coredns_dns_request

# 4. 确认 CoreDNS 副本全部就绪
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
```

## 回滚方案

| 修复操作 | 回滚方法 | 风险 |
|----------|----------|------|
| 重启 CoreDNS | 无需回滚（滚动重启自动完成） | 🟢 |
| 修正 ConfigMap | `kubectl apply -f coredns-configmap-backup.yaml` | 🟡 |
| 扩大资源 | `kubectl patch` 恢复原始值 | 🟢 |
| 修改 Pod DNS | 删除 dnsConfig 并重建 Pod | 🟡 |
| 重启 NodeLocal DNS | 无需回滚 | 🟢 |

## 升级决策点

- **P0（立即升级）**：集群 DNS 完全不可用，所有服务间调用失败
- **P1（30分钟内升级）**：DNS 间歇性失败，影响部分业务，有临时 workaround（IP直连）
- **P2（下一工作日）**：仅外部域名解析异常，集群内服务不受影响

## 生产注意事项

1. 修改 CoreDNS ConfigMap 前务必备份：`kubectl get cm coredns -n kube-system -o yaml > coredns-backup.yaml`
2. CoreDNS 重启期间会有短暂 DNS 解析中断（通常 <5s），避免在业务高峰期操作
3. ndots 参数影响解析性能，生产环境建议设置为 2-5，避免过多无效 search 查询
4. 大规模集群建议部署 NodeLocal DNSCache 减轻 CoreDNS 压力
5. 监控 `coredns_dns_request_duration_seconds` P99 延迟，超过 100ms 需关注

## 面试要点

1. **Q: Kubernetes DNS 解析的完整链路是什么？**
   A: Pod 发起 DNS 请求 → /etc/resolv.conf 中的 nameserver（CoreDNS ClusterIP 或 NodeLocal DNS）→ CoreDNS 根据 Corefile 规则处理：cluster.local 域走 kubernetes 插件查询 apiserver，外部域名走 forward 插件转发到上游 DNS。

2. **Q: CoreDNS 性能瓶颈如何识别和优化？**
   A: 识别：监控 `coredns_dns_request_duration_seconds` P99、`coredns_cache_misses_total`、Pod CPU/内存使用率。优化：① 启用 NodeLocal DNSCache；② 增加 CoreDNS 副本数；③ 调整 cache TTL；④ 使用 autopath 插件减少无效 search 查询；⑤ 设置合理的 ndots 值。

3. **Q: Pod DNS 解析超时但 CoreDNS 正常，如何排查？**
   A: ① 检查 Pod 的 /etc/resolv.conf 中 nameserver 是否正确；② 检查 NetworkPolicy 是否阻断 53/UDP；③ 检查节点 iptables 规则中 CoreDNS Service 的 DNAT 是否存在；④ 检查 conntrack 表是否溢出（`dmesg | grep conntrack`）；⑤ 检查是否有 DNS 查询风暴导致 UDP 丢包。

## 参见

- [[remediation-playbook]] — remediation 领域核心页面

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
