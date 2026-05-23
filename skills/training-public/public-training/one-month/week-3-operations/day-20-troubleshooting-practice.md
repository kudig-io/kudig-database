---
title: 'Day 20: 故障排查实战'
description: 'title: Day 20: 故障排查实战'
category: learning
tags:
- k8s
- training
- hands-on
- docker
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 20: 故障排查实战 是什么'
- '如何 Day 20: 故障排查实战'
- 'Day 20: 故障排查实战 故障排查'
- 'Day 20: 故障排查实战 排障步骤'
trigger_keywords:
- Day
- '20:'
- 故障排查实战
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

---
title: Day 20: 故障排查实战
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] 故障排查实战练习
  - k8s 常见故障注入和排除
  - kubectl 故障排查命令练习
  - ImagePullBackOff CrashLoopBackOff OOMKilled 排查
trigger_keywords:
  - 故障排查
  - 实战
  - ImagePullBackOff
  - CrashLoopBackOff
  - OOMKilled
  - PVC Pending
  - 故障注入
  - 故障演练
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Day 20: 故障排查实战

> **学习时间**: 4-5 小时 | **主题**: 构造并排查常见故障

---

## 今日目标

- [ ] 实战排查 5 类常见故障
- [ ] 熟练使用排障工具链
- [ ] 建立排障肌肉记忆

---

## 理论学习 (2h)

### 必读文档

1. **Pod 综合排障**
   - 文件: `../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md`

2. **[[Service|Service]] 综合排障**
   - 文件: `../../[[domain-10-troubleshooting-diagnostics/10-service-comprehensive-troubleshooting|10-service-comprehensive-troubleshooting]].md`

3. **网络 CNI 排障**
   - 文件: `../../domain-10-troubleshooting-diagnostics/03-networking-cni-troubleshooting.md`

4. **OOM 诊断**
   - 文件: `../../domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md`

---

## 实践任务 (2.5h) - 构造 5 类故障并排查

### 故障 1: ImagePullBackOff (30min)

```bash
# 构造故障
kubectl run image-error --image=nonexistent/image:v1

# 排障流程
kubectl get pod image-error
kubectl describe pod image-error | grep -A10 Events

# 分析:
# - 镜像不存在
# - 仓库认证失败
# - 网络不通

# 验证镜像存在
docker pull nonexistent/image:v1

# 修复
kubectl delete pod image-error
kubectl run image-ok --image=nginx:alpine
```

### 故障 2: OOMKilled (30min)

```bash
# 构造故障
cat > oom-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: oom-test
spec:
  containers:
  - name: stress
    image: polinux/stress
    command: ["stress"]
    args: ["--vm", "1", "--vm-bytes", "500M", "--vm-hang", "1"]
    resources:
      limits:
        memory: 100Mi
EOF

kubectl apply -f oom-pod.yaml

# 排障流程
kubectl get pod oom-test
kubectl describe pod oom-test | grep -A5 "Last State"

# 查看 OOM 事件
kubectl get events | grep OOM

# 分析:
# - 应用实际内存需求 500M
# - limits 设置 100M
# - 触发 OOMKiller

# 修复: 增加内存限制
kubectl delete pod oom-test
```

### 故障 3: Service 访问不通 (30min)

```bash
# 创建 Deployment 但故意设置错误的 label
kubectl create deployment web --image=nginx:alpine

# 创建 Service 但 selector 不匹配
cat > wrong-svc.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: wrong-svc
spec:
  selector:
    app: wrong-label  # 故意错误
  ports:
  - port: 80
EOF

kubectl apply -f wrong-svc.yaml

# 排障流程
# 1. 检查 Service
kubectl get svc wrong-svc
kubectl describe svc wrong-svc

# 2. 检查 Endpoints (应该是空的)
kubectl get endpoints wrong-svc

# 3. 检查 Pod labels
kubectl get pods --show-labels

# 4. 分析: selector 不匹配

# 修复
kubectl delete svc wrong-svc
kubectl expose deployment web --port=80
kubectl get endpoints web
```

### 故障 4: PVC Pending (30min)

```bash
# 构造故障: 请求不存在的 StorageClass
cat > pvc-error.yaml << 'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc-error
spec:
  storageClassName: nonexistent-class
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
EOF

kubectl apply -f pvc-error.yaml

# 排障流程
kubectl get pvc pvc-error
kubectl describe pvc pvc-error

# 检查可用的 StorageClass
kubectl get storageclass

# 分析: StorageClass 不存在

# 修复: 使用正确的 StorageClass
kubectl delete pvc pvc-error
```

### 故障 5: CrashLoopBackOff (30min)

```bash
# 构造故障: 启动命令错误
cat > crash-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: crash-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    command: ["nonexistent-command"]
EOF

kubectl apply -f crash-pod.yaml

# 排障流程
kubectl get pod crash-test
kubectl describe pod crash-test
kubectl logs crash-test --previous

# 分析: 命令不存在导致容器立即退出

# 修复
kubectl delete pod crash-test
```

---

## 费曼复述 (0.5h)

针对每类故障，回答:
1. 症状是什么？
2. 如何定位？
3. 根因是什么？
4. 如何修复？

---

## 今日检验

- [ ] 能够排查 ImagePullBackOff
- [ ] 能够排查 OOMKilled
- [ ] 能够排查 Service 访问问题
- [ ] 能够排查 PVC Pending
- [ ] 能够排查 CrashLoopBackOff

---

## 排障速查表

| 状态 | 可能原因 | 排查命令 |
|------|----------|----------|
| Pending | 资源不足/调度问题 | `kubectl describe pod` |
| ImagePullBackOff | 镜像/认证/网络 | `kubectl describe pod` |
| CrashLoopBackOff | 应用错误 | `kubectl logs --previous` |
| OOMKilled | 内存不足 | `kubectl describe pod` |
| Evicted | 节点资源紧张 | `kubectl describe node` |
