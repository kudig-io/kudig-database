# Week 3 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. RBAC 中 Role vs ClusterRole，RoleBinding vs ClusterRoleBinding 的区别和适用场景？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- Role/RoleBinding: namespace 级别权限
- ClusterRole/ClusterRoleBinding: 集群级别权限
- ClusterRole + RoleBinding: 复用 ClusterRole 但限制在 namespace

---

### 2. PromQL 中 rate() 和 irate() 的区别？写出"过去5分钟容器 CPU 使用率"的查询

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- rate(): 使用范围内所有点的平均变化率，更平滑
- irate(): 使用最后两个点的即时变化率，更敏感
- 查询: `rate(container_cpu_usage_seconds_total[5m])`

---

### 3. Node 突然 NotReady，你的完整排查步骤（至少列出 8 步）？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
1. `kubectl describe node`
2. 检查 kubelet 状态
3. 检查容器运行时
4. 检查磁盘空间
5. 检查内存使用
6. 检查网络连通性
7. 检查系统日志
8. 检查证书有效期

---

### 4. FTA 和 FEBM 分别适用于什么场景？两者如何协作？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- FTA: 系统性分析故障可能原因，构建故障树
- FEBM: 收集证据，验证假设，确定根因
- 协作: FTA 提供分析框架，FEBM 提供验证方法

---

### 5. etcd 备份的命令是什么？备份应该多久做一次？为什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- `etcdctl snapshot save backup.db`
- 建议每天至少一次，变更前必须备份
- 原因: etcd 存储所有集群状态，是灾难恢复的关键

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何检查当前用户是否有权限创建 Deployment？

**你的回答:**

```

```

**参考答案:** `kubectl auth can-i create deployments`

---

### 7. 如何查看 Pod 的上一次崩溃日志？

**你的回答:**

```

```

**参考答案:** `kubectl logs <pod-name> --previous`

---

### 8. 如何列出所有 firing 状态的告警？

**你的回答:**

```

```

**参考答案:** 访问 Alertmanager UI 或 `amtool alert --alertmanager.url=http://...`

---

### 9. 如何查看某个 namespace 的所有事件并按时间排序？

**你的回答:**

```

```

**参考答案:** `kubectl get events -n <namespace> --sort-by='.lastTimestamp'`

---

### 10. 如何使用 ServiceAccount 的 Token 进行 API 调用？

**你的回答:**

```

```

**参考答案:** `kubectl create token <sa-name>` 获取 token，然后用 `curl -H "Authorization: Bearer $TOKEN"`

---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 设计一个 RBAC 方案：开发人员只能在 dev namespace 中查看和创建 Deployment/Pod/Service

**你的回答:**

```yaml
(在此写下你的 YAML)




```

---

### 12. 描述如何使用 Prometheus + Grafana + Alertmanager 构建完整的监控告警链路

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- Prometheus 采集指标
- 配置 PrometheusRule 告警规则
- Alertmanager 接收告警、分组、路由
- Grafana 可视化展示
- Webhook 通知到钉钉/企微等

---

### 13. 如果线上出现 Pod OOMKilled，完整的分析和修复流程是什么？

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
1. `kubectl describe pod` 确认 OOMKilled
2. 查看资源 limits 配置
3. 查看实际内存使用 (`kubectl top pod`)
4. 分析是内存泄漏还是 limits 设置不合理
5. 调整 limits 或修复应用
6. 配置告警防止再次发生

---

### 14. 解释 Pod Security Standards 的三个级别，以及如何在生产环境实施

**你的回答:**

```
(在此写下你的答案)




```

**参考要点:**
- Privileged: 无限制，仅限特殊用途
- Baseline: 基本限制，防止已知提权
- Restricted: 严格限制，生产推荐
- 实施: 使用 Pod Security Admission 或 Kyverno/OPA

---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 20 |
| 命令实操 | __ | 10 |
| 场景分析 | __ | 20 |
| **总分** | __ | **50** |

---

## 五、薄弱点记录

```
1. 


2. 


3. 

```
