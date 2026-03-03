# Week 2 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. Deployment 的 maxSurge 和 maxUnavailable 如何影响滚动更新行为？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- maxSurge: 允许临时超过期望副本数的数量
- maxUnavailable: 允许不可用的最大数量
- 两者共同决定更新速度和服务可用性的平衡

---

### 2. StatefulSet 为什么不能像 Deployment 一样随意调度？headless Service 的作用是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- StatefulSet 需要稳定的网络标识和存储
- Pod 名称有序且固定 (app-0, app-1, app-2)
- Headless Service 为每个 Pod 提供 DNS 记录
- DNS 格式: `<pod-name>.<service-name>.<namespace>.svc.cluster.local`

---

### 3. ClusterIP Service 的流量是如何通过 iptables/IPVS 转发到 Pod 的？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- kube-proxy 配置 iptables/IPVS 规则
- 请求到达 Service IP
- DNAT 规则将目标 IP 改为 Pod IP
- 通过概率或轮询选择后端 Pod

---

### 4. PV 的 Reclaim Policy (Retain/Delete/Recycle) 三种的区别和使用场景？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- Retain: 保留数据，需手动清理，适合重要数据
- Delete: 自动删除，适合临时数据
- Recycle: 已弃用，清空数据后重用

---

### 5. 如果应用频繁 OOMKilled，resources.limits.memory 应该如何调整？依据是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- 首先分析应用实际内存使用 (kubectl top pod)
- 检查是否有内存泄漏
- limits 应大于实际峰值使用量
- requests 应等于正常使用量
- 考虑 JVM 等特殊应用的内存管理

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何查看 Deployment 的更新历史？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl rollout history deployment/<name>`

---

### 7. 如何将 Deployment 回滚到指定版本 (如 revision 2)？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl rollout undo deployment/<name> --to-revision=2`

---

### 8. 如何查看 Service 对应的 Endpoints？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl get endpoints <service-name>`

---

### 9. 如何测试 DNS 解析是否正常？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl run dns-test --image=busybox --rm -it -- nslookup <service-name>`

---

### 10. 如何查看 HPA 的当前状态和指标？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl get hpa <name>` 或 `kubectl describe hpa <name>`

---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 设计一个有状态应用 (如 MySQL) 的部署方案，需要考虑哪些因素？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- 使用 StatefulSet 保证稳定标识
- 使用 Headless Service 提供 DNS
- 每个 Pod 独立 PVC 存储数据
- 配置适当的资源限制
- 配置健康检查探针
- 考虑备份和恢复策略

---

### 12. 解释 Ingress 和 Service 的关系，以及何时使用 Ingress？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- Service: L4 负载均衡 (TCP/UDP)
- Ingress: L7 路由 (HTTP/HTTPS)
- Ingress 需要后端 Service
- 使用场景: 需要基于路径/主机名路由、TLS 终止、多服务统一入口

---

### 13. NetworkPolicy 的默认行为是什么？如何实现"只允许前端访问 API"的策略？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- 默认允许所有流量
- 一旦有 Policy 作用于 Pod，则变为默认拒绝
- 需要显式允许需要的流量

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
spec:
  podSelector:
    matchLabels:
      app: api
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
```

---

### 14. 描述 PV/PVC 动态供应的完整流程。

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
1. 用户创建 PVC，指定 StorageClass
2. StorageClass 关联的 Provisioner 监测到 PVC
3. Provisioner 调用存储后端 API 创建存储卷
4. Provisioner 创建对应的 PV
5. PVC 与 PV 绑定
6. Pod 使用 PVC 挂载存储

---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 20 |
| 命令实操 | __ | 10 |
| 场景分析 | __ | 20 |
| **总分** | __ | **50** |

### 评估标准

- **45-50 分**: 优秀，完全掌握本周内容
- **35-44 分**: 良好，基本掌握，部分细节需加强
- **25-34 分**: 及格，核心概念理解，需要复习
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

```
1. 


2. 


3. 

```
