# Week 1 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. ACK 托管版、专有版、Serverless 三种集群类型的核心区别是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- 托管版: 管控面由阿里云托管，用户管理 Worker 节点
- 专有版: 用户完全管理 Master 和 Worker 节点
- Serverless: 无需管理节点，按 Pod 计费
- 适用场景: 托管版适合大多数生产场景，专有版适合深度定制，Serverless 适合突发流量

---

### 2. ACK 集群创建时，VPC CIDR、Pod CIDR、Service CIDR 三者有什么关系和约束？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- 三者的地址段不能重叠
- VPC CIDR 是底层网络基础
- Pod CIDR 决定 Pod 可分配的 IP 范围
- Service CIDR 决定 Service ClusterIP 范围
- 创建后 Service CIDR 不可修改

---

### 3. ACK 集群中有哪些核心的 kube-system 组件？各自的作用是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- coredns: 集群 DNS 服务
- cloud-controller-manager: 管理云资源 (SLB、路由等)
- terway/flannel: CNI 网络插件
- csi-plugin/csi-provisioner: 存储插件
- metrics-server: 指标采集

---

### 4. ACK 集群升级分哪两个阶段？替换升级的操作流程是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- 阶段一: 管控面升级 (托管版自动完成)
- 阶段二: 节点升级 (原地升级或替换升级)
- 替换升级流程: 扩容新节点 -> cordon 旧节点 -> drain 旧节点 -> 确认 Pod 迁移 -> 移除旧节点

---

### 5. K8S 集群中的证书体系包含哪些类型？kubeconfig 过期后如何处理？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**
- CA 根证书: 签发所有组件证书，有效期 10 年
- API Server 证书: 服务端认证，自动轮换
- kubelet 证书: 节点身份认证，自动轮换
- kubeconfig: 用户访问凭证，默认 3 年
- 过期处理: 通过 API 重新获取 kubeconfig

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何使用 aliyun CLI 查看当前账号下所有 ACK 集群？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `aliyun cs GET /api/v1/clusters`

---

### 7. 如何检查 kubeconfig 中客户端证书的过期时间？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates`

---

### 8. 如何对一个节点执行排水操作 (drain)？需要哪些参数？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data`

---

### 9. 如何查看 ACK 集群中所有 LoadBalancer 类型的 Service？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `kubectl get svc -A | grep LoadBalancer` 或 `kubectl get svc -A -o jsonpath='{range .items[?(@.spec.type=="LoadBalancer")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'`

---

### 10. 如何查看集群节点的 kubelet 版本信息？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'`

---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 用户报告集群创建失败，你的排查步骤是什么？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
1. 查看集群创建日志: `aliyun cs GET /clusters/<cluster_id>/logs`
2. 检查 VPC/vSwitch 配置是否正确
3. 检查安全组规则是否允许节点通信
4. 检查 ECS 实例规格在目标可用区是否有库存
5. 检查 CIDR 是否与已有网络冲突
6. 检查 RAM 权限是否充足

---

### 12. 用户需要将集群从 1.26 版本升级到 1.28 版本，你如何制定升级计划？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
1. 版本不能跨版本升级，需 1.26 -> 1.27 -> 1.28
2. 每次升级前使用 kubent 检查 API 废弃
3. 备份集群资源和关键配置
4. 先升级管控面，再替换升级节点
5. 每步升级后验证集群状态和业务可用性
6. 准备回滚方案

---

### 13. 删除集群时提示"部分资源无法释放"，如何处理？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
1. 查看删除日志确认哪些资源无法释放
2. 检查 SLB 是否被非 K8S 服务引用
3. 检查 ENI 是否有安全组规则关联
4. 手动释放残留的云资源
5. 重试删除操作
6. 如仍失败，联系内部 oncall 处理

---

### 14. 用户反馈 kubectl 命令突然返回 "x509: certificate has expired"，你的排查和处理步骤？

**你的回答:**

```
(在此写下你的答案)



```

**参考要点:**
1. 确认是 kubeconfig 客户端证书过期还是集群证书问题
2. 检查 kubeconfig 证书过期时间
3. 如果 kubeconfig 过期，通过 API 重新获取
4. 如果是集群内部证书，检查 kube-system 组件状态
5. 触发证书轮换: `aliyun cs POST /clusters/<cluster_id>/certrenew`
6. 轮换后重新获取 kubeconfig 并验证

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

记录自测中暴露的薄弱点，下周重点复习:

```
1. 


2. 


3. 

```

---

## 下周计划调整

基于自测结果，调整下周学习重点:

```
需要加强的领域:


下周额外复习:


```
