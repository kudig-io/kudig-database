---
title: service connectivity Remediation Playbook
summary: service connectivity Remediation Playbook：kubectl get svc <svc> -o jsonpath='{.spec.selector}'
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-service-connectivity
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Service|Service]] 连通性问题修复手册

## 修复步骤

### 修复 1：修正 Selector 标签

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前 selector
kubectl get svc <svc> -o jsonpath='{.spec.selector}'

# 查看后端 Pod 标签
kubectl get pods -l app=<correct-app> --show-labels

# 修正 Service selector
kubectl patch svc <svc> -p '{"spec":{"selector":{"app":"<correct-label>"}}}'
```
### 修复 2：重启 kube-proxy

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart daemonset kube-proxy -n kube-system
```
### 修复 3：删除并重建 Service

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get svc <svc> -o yaml > svc-backup.yaml
kubectl delete svc <svc>
kubectl apply -f svc-backup.yaml
```
### 修复 4：修正 Endpoints 手动绑定（无 Selector 场景）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认

``` bash
# 🟡 中风险：手动创建 Endpoints 绑定外部服务
kubectl apply -f - <<EOF
apiVersion: v1
kind: Endpoints
metadata:
  name: <svc>
  namespace: <ns>
subsets:
- addresses:
  - ip: <backend-ip>
  ports:
  - port: <port>
    protocol: TCP
EOF
```

### 修复 5：修正 kube-proxy iptables/ipvs 规则异常

``` bash
# 🟢 低风险：检查当前规则
# iptables 模式
iptables-save | grep <svc-cluster-ip>
# ipvs 模式
ipvsadm -Ln | grep <svc-cluster-ip>

# 🟡 中风险：强制刷新 kube-proxy 规则
kubectl rollout restart daemonset kube-proxy -n kube-system
```

## 验证方法

``` bash
# 🟢 低风险：验证 Service 连通性
# 1. 确认 Endpoints 已填充
kubectl get endpoints <svc> -n <ns>

# 2. 集群内 Pod 测试
kubectl run test --rm -it --image=busybox -- wget -qO- http://<svc>.<ns>.svc.cluster.local:<port>/health

# 3. 检查 Service 事件
kubectl describe svc <svc> -n <ns> | grep -A10 Events

# 4. 验证 DNS 解析
kubectl run test --rm -it --image=busybox -- nslookup <svc>.<ns>.svc.cluster.local
```

## 回滚方案

| 修复操作 | 回滚方法 | 风险 |
|----------|----------|------|
| 修正 Selector | `kubectl patch svc <svc> -p '{"spec":{"selector":<original>}}'` | 🟡 |
| 重启 kube-proxy | 无需回滚（滚动重启自动完成） | 🟢 |
| 删除重建 Service | 使用 svc-backup.yaml 重新 apply | 🟡 |
| 手动 Endpoints | `kubectl delete endpoints <svc> -n <ns>` | 🟡 |

## 升级决策点

- **P0（立即升级）**：核心业务 Service 完全不可用，所有请求失败，影响营收
- **P1（30分钟内升级）**：部分流量异常，有临时 workaround（如直接 Pod IP 访问）
- **P2（下一工作日）**：非关键 Service，影响范围有限，可安排变更窗口处理

## 生产注意事项

1. 修改 Selector 前务必备份原始 Service YAML：`kubectl get svc <svc> -o yaml > backup.yaml`
2. 删除重建 Service 会导致 ClusterIP 变化（除非指定），依赖 ClusterIP 的客户端需同步更新
3. kube-proxy 重启期间新 Service 规则不会下发，避免在业务高峰期操作
4. 无 Selector 的 Service（ExternalName/手动 Endpoints）需特别关注 Endpoints 同步
5. 多集群环境下确认操作的目标集群 context：`kubectl config current-context`

## 面试要点

1. **Q: Service 的 ClusterIP 是如何实现流量转发的？**
   A: kube-proxy 监听 apiserver 的 Service/Endpoints 变化，在 iptables 模式下写入 DNAT 规则将 ClusterIP 流量随机转发到后端 Pod IP；ipvs 模式使用内核级负载均衡，支持 rr/lc/dh 等多种算法，性能优于 iptables。

2. **Q: Endpoints 为空但 Pod 正在运行，可能原因有哪些？**
   A: ① Service selector 与 Pod label 不匹配；② Pod readinessProbe 未通过；③ Pod 的 targetPort 与 Service port 不一致；④ Pod 处于 Terminating 状态；⑤ EndpointSlice 控制器异常。

3. **Q: 如何排查 Service 的 DNS 解析失败？**
   A: 按层排查：① `nslookup <svc>.<ns>.svc.cluster.local` 确认 DNS 记录；② 检查 CoreDNS Pod 状态和日志；③ 验证 Pod 的 /etc/resolv.conf 中 nameserver 指向 CoreDNS ClusterIP；④ 检查 NetworkPolicy 是否阻断了 53/UDP 流量。

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
