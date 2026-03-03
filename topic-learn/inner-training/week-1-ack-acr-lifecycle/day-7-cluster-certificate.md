# Day 7: K8S 集群证书

> **学习时间**: 4-5 小时 | **主题**: 理解集群证书管理与更新机制

---

## 今日目标

- [ ] 理解 K8S 集群中的证书体系
- [ ] 掌握 ACK 集群证书的管理方式
- [ ] 能够检查证书过期时间和状态
- [ ] 了解证书轮换和 kubeconfig 更新流程

---

## 理论学习 (2h)

### 必读文档

1. **证书管理**
   - 文件: `../../../domain-7-security/10-certificate-management.md`
   - 重点: K8S 证书体系、CA 证书链、组件证书

2. **安全架构**
   - 文件: `../../../domain-1-architecture-fundamentals/14-security-architecture.md`
   - 重点: 认证机制、证书在安全架构中的角色

3. **证书排障**
   - 文件: `../../../domain-12-troubleshooting/13-certificate-troubleshooting.md`
   - 重点: 常见证书问题和排查方法

### 阅读要点

- K8S 证书体系包括: CA 根证书、API Server 证书、kubelet 证书、etcd 证书
- ACK 托管版: 管控面证书由阿里云管理，用户只需关注 kubeconfig 和自定义证书
- kubeconfig 有效期: 默认 3 年，需要定期更新
- 证书过期影响: kubectl 无法连接、组件间通信失败、集群不可用

---

## 实践任务 (2.5h)

### 任务 1: 检查集群证书状态 (45min)

```bash
# 查看 kubeconfig 中的证书过期时间
# 提取 kubeconfig 中的 client-certificate-data
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -text -noout | grep -A2 "Validity"

# 检查集群 CA 证书
kubectl get configmap -n kube-system cluster-info -o yaml

# 查看 kube-system 中的证书相关 Secret
kubectl get secrets -n kube-system | grep -i cert

# 检查 API Server 证书 (通过 OpenSSL)
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
echo | openssl s_client -connect ${APISERVER#https://} 2>/dev/null | openssl x509 -noout -dates

# 检查 webhook 证书
kubectl get validatingwebhookconfigurations -o yaml | grep -A2 caBundle
```

### 任务 2: kubeconfig 管理 (45min)

```bash
# 1. 通过 API 获取新的 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config

# 2. 获取临时 kubeconfig (有效期较短)
aliyun cs GET /k8s/<cluster_id>/user_config \
  --TemporaryDurationMinutes 480

# 3. 获取私网 kubeconfig (通过内网访问)
aliyun cs GET /k8s/<cluster_id>/user_config \
  --PrivateIpAddress true

# 4. 撤销已颁发的 kubeconfig
aliyun cs POST /clusters/<cluster_id>/ravelokens/revoke

# 5. 管理多集群 kubeconfig
# 查看当前 context
kubectl config get-contexts

# 切换 context
kubectl config use-context <context-name>

# 合并多个 kubeconfig
KUBECONFIG=~/.kube/config:~/cluster-2.yaml kubectl config view --flatten > merged-config.yaml
```

### 任务 3: 证书轮换操作 (30min)

```bash
# ACK 托管版证书轮换 (通过 API)
# 触发证书轮换
aliyun cs POST /clusters/<cluster_id>/certrenew

# 查看轮换进度
aliyun cs GET /clusters/<cluster_id>/logs

# 轮换完成后，需要重新获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config

# 验证新证书
kubectl cluster-info
kubectl get nodes
```

### 任务 4: 证书过期场景模拟与排查 (30min)

```bash
# 场景: kubectl 连接失败，疑似证书过期

# 排查步骤:
# 1. 检查错误信息
kubectl get nodes 2>&1
# 常见错误: "x509: certificate has expired or is not yet valid"
# 常见错误: "Unable to connect to the server"

# 2. 检查 kubeconfig 证书有效期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates

# 3. 如果 kubeconfig 过期，重新获取
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config

# 4. 如果是集群内部证书问题
# 检查 kube-system 组件日志
# 联系 ACK 团队处理管控面证书问题

# 5. 验证修复
kubectl cluster-info
kubectl get nodes
kubectl get pods -n kube-system
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **K8S 集群中有哪些类型的证书？各自的作用是什么？**
   - 提示: CA 证书、API Server 证书、kubelet 证书、kubeconfig 客户端证书

2. **当用户报告 kubectl 连接失败时，如何判断是证书问题？**
   - 提示: 错误信息中包含 x509、certificate expired

3. **ACK 托管版和专有版在证书管理上有什么区别？**
   - 提示: 托管版管控面证书自动管理，专有版需要手动处理

---

## 今日检验

- [ ] 能检查 kubeconfig 中的证书过期时间
- [ ] 能通过 API 获取和更新 kubeconfig
- [ ] 理解证书轮换的操作流程
- [ ] 能排查证书过期导致的连接问题

---

## 核心概念总结

| 证书类型 | 用途 | 有效期 | 管理方式 |
|----------|------|--------|---------|
| CA 根证书 | 签发所有组件证书 | 10 年 | 阿里云管理 (托管版) |
| API Server 证书 | API 服务端认证 | 1 年 | 自动轮换 |
| kubeconfig | 用户访问凭证 | 3 年 | 手动获取更新 |
| kubelet 证书 | 节点身份认证 | 1 年 | 自动轮换 |

---

## 本周总结

恭喜完成 Week 1 的全部学习! 本周你应该已经掌握:
- ACK/ACR 服务架构和管控概念
- SDK/API 调用方式
- 控制台核心操作
- 集群完整生命周期: 创建 -> 升级 -> 证书管理 -> 删除

请完成 [checkpoint.md](./checkpoint.md) 自测和 [P1 实操项目](../projects/p1-ack-cluster-lifecycle.md)。
