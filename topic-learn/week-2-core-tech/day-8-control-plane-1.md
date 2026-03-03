# Day 8: 控制平面 - etcd + API Server

> **学习时间**: 4-5 小时 | **主题**: K8s 数据存储与 API 网关

---

## 今日目标

- [ ] 深入理解 etcd 的架构和 Raft 协议
- [ ] 掌握 API Server 的请求处理链
- [ ] 能够直接操作 etcd 查看 K8s 数据

---

## 理论学习 (2h)

### 必读文档

1. **etcd 深入**
   - 文件: `../../domain-3-control-plane/11-etcd-deep-dive.md`
   - 重点: Raft 协议、数据存储格式、备份恢复

2. **API Server 深入**
   - 文件: `../../domain-3-control-plane/12-apiserver-deep-dive.md`
   - 重点: 请求处理链、认证、授权、准入控制

### 补充阅读

3. **分布式共识**
   - 文件: `../../domain-2-design-principles/07-distributed-consensus-etcd.md`
   - 重点: Raft 算法理解

---

## 实践任务 (2.5h)

### 任务 1: etcd 操作实践 (1h)

```bash
# 获取 etcd Pod 名称
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')

# 进入 etcd 容器
kubectl exec -it -n kube-system $ETCD_POD -- sh

# 设置 etcdctl 环境变量
export ETCDCTL_API=3
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

# 查看集群成员
etcdctl member list

# 查看集群健康状态
etcdctl endpoint health

# 查看 K8s 数据 (所有 key)
etcdctl get / --prefix --keys-only | head -50

# 查看特定资源
etcdctl get /registry/deployments/default --prefix --keys-only
etcdctl get /registry/pods/kube-system --prefix --keys-only

# 查看某个 Pod 的完整数据
etcdctl get /registry/pods/kube-system/<pod-name>
```

### 任务 2: API Server 请求追踪 (45min)

```bash
# 使用 verbose 模式查看完整请求
kubectl get pods -v=8

# 观察请求路径
# - URL: /api/v1/namespaces/default/pods
# - 认证信息
# - 响应状态

# 查看 API Server 日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=100

# 使用 curl 直接调用 API
# 获取 token
TOKEN=$(kubectl create token default)

# 调用 API (通过 kubectl proxy)
kubectl proxy --port=8001 &
curl http://localhost:8001/api/v1/namespaces
curl http://localhost:8001/api/v1/namespaces/default/pods

# 关闭 proxy
kill %1
```

### 任务 3: 准入控制实验 (45min)

```bash
# 查看启用的准入控制器
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep admission

# 创建带有资源限制的 LimitRange
cat > limitrange.yaml << 'EOF'
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: default
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
EOF

kubectl apply -f limitrange.yaml

# 创建一个没有资源限制的 Pod，观察自动添加
kubectl run test-limit --image=nginx:alpine
kubectl get pod test-limit -o yaml | grep -A10 resources

# 清理
kubectl delete pod test-limit
kubectl delete limitrange default-limits
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **etcd 的 Raft 协议如何保证数据一致性？**
   - Leader 选举
   - 日志复制
   - 多数派确认

2. **API Server 收到请求后经过哪些步骤处理？**
   - 认证 (Authentication)
   - 授权 (Authorization)
   - 准入控制 (Admission Control)
   - 持久化到 etcd

3. **为什么只有 API Server 能直接访问 etcd？**

---

## 今日检验

- [ ] 能够使用 etcdctl 查看 K8s 数据
- [ ] 理解 API Server 请求处理链
- [ ] 知道准入控制器的作用
- [ ] 能够解释 Raft 协议的基本原理

---

## 核心概念总结

| 组件 | 关键点 | 生产注意事项 |
|------|--------|--------------|
| etcd | Raft 共识、键值存储 | 定期备份、奇数节点 |
| API Server | 认证授权准入 | 高可用部署、审计日志 |

---

## 明日预告

Day 9 将学习 Scheduler 和 Controller Manager，理解 K8s 如何实现自动化管理。
