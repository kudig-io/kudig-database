# Week 2 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. RBAC 中 Role 和 ClusterRole 的区别是什么？RoleBinding 和 ClusterRoleBinding 呢？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
- Role 仅限 Namespace 范围，ClusterRole 是集群范围
- RoleBinding 绑定到 Namespace，ClusterRoleBinding 绑定到整个集群
- ClusterRole 可以通过 RoleBinding 绑定到特定 Namespace

---

### 2. ACK 的两层权限模型是什么？各自控制什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
- RAM 权限: 控制云平台操作 (控制台访问、API 调用)
- RBAC 权限: 控制集群内 K8S 资源操作
- 两层独立但互补

---

### 3. K8S 审计日志有哪四个级别？各自记录什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
- None: 不记录
- Metadata: 请求元数据 (用户、时间、资源)
- Request: 元数据 + 请求体
- RequestResponse: 元数据 + 请求体 + 响应体

---

### 4. ResourceQuota 和 LimitRange 的区别是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
- ResourceQuota: 限制 Namespace 下所有资源的总量
- LimitRange: 限制单个 Pod/Container 的资源范围，设置默认值
- 两者配合使用效果最佳

---

### 5. Pod Security Standards (PSS) 的三个级别分别是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
- Privileged: 无限制，适合系统组件
- Baseline: 基本安全限制，防止已知的提权方式
- Restricted: 最严格限制，遵循所有安全最佳实践

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何检查某个 ServiceAccount 是否有权限创建 Deployment？
**你的回答:**
```
```
**参考答案:** `kubectl auth can-i create deployments --as=system:serviceaccount:<ns>:<sa-name> -n <namespace>`

---

### 7. 如何查看当前 Namespace 的资源配额使用情况？
**你的回答:**
```
```
**参考答案:** `kubectl describe resourcequota -n <namespace>`

---

### 8. 如何检查集群中是否有特权容器在运行？
**你的回答:**
```
```
**参考答案:** `kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].securityContext.privileged}{"\n"}{end}' | grep true`

---

### 9. 如何查看集群节点的 CPU 和内存使用率？
**你的回答:**
```
```
**参考答案:** `kubectl top nodes`

---

### 10. 如何为 Namespace 启用 PSS baseline 级别？
**你的回答:**
```
```
**参考答案:** `kubectl label namespace <ns> pod-security.kubernetes.io/enforce=baseline`

---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 用户报告无法在某个 Namespace 创建 Pod，但在其他 Namespace 可以，排查思路？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
1. 检查该 Namespace 的 ResourceQuota 是否已用满
2. 检查 LimitRange 是否限制了 Pod 资源
3. 检查 PSS 是否拒绝了 Pod 的安全配置
4. 检查 RBAC 权限是否针对该 Namespace 限制
5. 查看 kubectl describe 中的 Events

---

### 12. 如何设计一个安全的多团队集群权限方案？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
1. 每个团队独立 Namespace
2. RAM 用户映射到 ACK 角色
3. RBAC 限制为 Namespace 级别的 Role
4. ResourceQuota 限制各团队资源
5. NetworkPolicy 隔离团队间网络
6. PSS baseline 级别强制执行

---

### 13. 收到安全漏洞公告 (CVE)，处理流程是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
1. 评估影响范围和严重等级
2. 检查集群版本是否受影响
3. 实施临时缓解措施
4. 规划升级修复时间窗口
5. 执行修复并验证
6. 更新安全基线文档

---

### 14. 如何通过审计日志排查"某个 Deployment 被误删"的问题？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**
1. 在 SLS 查询: verb=delete AND resource=deployments AND name=<deployment>
2. 确认操作者: user.username 字段
3. 确认操作时间: requestReceivedTimestamp
4. 确认来源 IP: sourceIPs
5. 形成审计报告并制定防范措施

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

---

## 下周计划调整

```
需要加强的领域:

下周额外复习:
```
