# Day 14: K8S 集群配额 & License

> **学习时间**: 4-5 小时 | **主题**: 资源配额管理与许可证相关

---

## 今日目标

- [ ] 掌握 ResourceQuota 和 LimitRange 配置
- [ ] 理解 ACK 集群配额限制
- [ ] 了解 License 与集群规模的关系
- [ ] 能够设计合理的资源配额方案

---

## 理论学习 (2h)

### 必读文档

1. **资源管理**
   - 文件: `../../../domain-4-workloads/23-resource-management.md`
   - 重点: requests/limits、QoS 等级

2. **配额排障**
   - 文件: `../../../domain-12-troubleshooting/24-quota-limitrange-troubleshooting.md`
   - 重点: 配额相关的常见问题

---

## 实践任务 (2.5h)

### 任务 1: ResourceQuota 配置 (45min)

```bash
kubectl create namespace quota-test

# 创建资源配额
cat > resource-quota.yaml << 'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: quota-test
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    pods: "20"
    services: "10"
    persistentvolumeclaims: "5"
    configmaps: "20"
    secrets: "20"
EOF
kubectl apply -f resource-quota.yaml

# 查看配额使用情况
kubectl describe resourcequota team-quota -n quota-test
```

### 任务 2: LimitRange 配置 (45min)

```bash
# 创建默认限制
cat > limit-range.yaml << 'EOF'
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: quota-test
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 4Gi
    min:
      cpu: 50m
      memory: 64Mi
  - type: Pod
    max:
      cpu: "4"
      memory: 8Gi
EOF
kubectl apply -f limit-range.yaml

# 测试: 创建不带 resources 的 Pod
kubectl run test-limit --image=nginx:alpine -n quota-test
kubectl get pod test-limit -n quota-test -o yaml | grep -A 10 resources

# 测试: 创建超出限制的 Pod
cat > over-limit-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: over-limit
  namespace: quota-test
spec:
  containers:
  - name: test
    image: nginx:alpine
    resources:
      requests:
        cpu: "3"
        memory: 8Gi
EOF
kubectl apply -f over-limit-pod.yaml  # 应该被拒绝
```

### 任务 3: ACK 集群配额检查 (30min)

```bash
# ACK 集群级配额
# 1. 节点数量限制 (根据集群规格)
# 2. Pod 数量限制 (与节点数和 CIDR 相关)
# 3. Service 数量限制 (与 Service CIDR 相关)
# 4. 每节点最大 Pod 数 (默认 110)

# 查看节点 Pod 容量
kubectl get nodes -o custom-columns='NAME:.metadata.name,CAPACITY:.status.capacity.pods,ALLOCATED:.status.allocatable.pods'

# 查看当前 Pod 数量
kubectl get pods -A --no-headers | wc -l

# 查看 Service CIDR 容量
kubectl cluster-info dump | grep service-cluster-ip-range
```

### 任务 4: 多团队配额方案设计 (30min)

```bash
# 场景: 3 个团队共享一个集群
# 集群总资源: 12C 48G
# 分配方案:
# - 开发团队: 4C 16G
# - 测试团队: 4C 16G
# - 生产团队: 4C 16G (预留)

# 为各团队创建 Namespace + Quota
for team in dev test prod; do
  kubectl create namespace team-$team
  kubectl create resourcequota ${team}-quota -n team-$team \
    --hard=requests.cpu=4,requests.memory=16Gi,limits.cpu=4,limits.memory=16Gi,pods=50
done

# 验证各团队配额
for team in dev test prod; do
  echo "=== team-$team ==="
  kubectl describe resourcequota -n team-$team
done

# 清理
kubectl delete namespace quota-test team-dev team-test team-prod
```

---

## 费曼复述 (0.5h)

1. **ResourceQuota 和 LimitRange 的区别是什么？**
2. **为什么多团队场景必须配置资源配额？**
3. **ACK 集群有哪些级别的配额限制？**

---

## 今日检验

- [ ] 能创建 ResourceQuota 和 LimitRange
- [ ] 理解 requests 和 limits 的关系
- [ ] 了解 ACK 集群级配额限制
- [ ] 能设计多团队资源配额方案

---

## 核心概念总结

| 资源 | 作用 | 生产建议 |
|------|------|---------|
| ResourceQuota | 限制 Namespace 资源总量 | 每个业务 Namespace 必配 |
| LimitRange | 限制单个 Pod/Container 资源 | 设置合理的默认值和上限 |
| QoS | 资源驱逐优先级 | Guaranteed > Burstable > BestEffort |

---

## 本周总结

恭喜完成 Week 2 的全部学习! 本周你应该已经掌握:
- RBAC 权限模型与配置
- RAM 账号与 ACK 集成
- 安全漏洞识别与风险防范
- 审计日志配置与分析
- 监控体系搭建与告警配置
- 资源配额管理

请完成 [checkpoint.md](./checkpoint.md) 自测和 [P2 实操项目](../projects/p2-security-monitoring-setup.md)。
