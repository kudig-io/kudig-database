# P5: 毕业综合项目

> **对应周次**: 全部 4 周 | **预计时间**: 6-8 小时 | **难度**: ⭐⭐⭐⭐

---

## 项目目标

综合运用 4 周所学，独立完成一套完整的 ACK 集群运维方案：从集群规划、创建、安全加固、应用部署到监控告警，模拟真实生产环境的运维场景。

## 前置条件

- [ ] 完成 4 周全部教案和 4 次自测
- [ ] 完成 P1-P4 实操项目
- [ ] 准备好测试用的阿里云账号和资源

---

## 项目场景

> 你的团队需要为一个新业务搭建 ACK 集群环境。该业务包含 Web 前端、API 后端、数据库三层架构。
> 要求：高可用部署、权限隔离、监控告警、网络安全、存储持久化。

---

## 实施步骤

### Phase 1: 集群规划与创建 (1.5h)

#### 1.1 网络规划文档

```
填写以下规划表:

| 项目 | CIDR / 配置 |
|------|------------|
| VPC CIDR | |
| 节点 vSwitch-A (可用区) | |
| 节点 vSwitch-B (可用区) | |
| Pod vSwitch (Terway) 或 Pod CIDR (Flannel) | |
| Service CIDR | |
| CNI 方案选择 | Terway / Flannel |
| 选择理由 | |
```

#### 1.2 集群创建

```bash
# 使用 aliyun CLI 创建集群
# 要求:
# - 托管版 ACK
# - K8S 最新稳定版
# - 启用公网访问
# - 安装 Nginx Ingress Controller

# 你的创建命令:
aliyun cs POST /clusters --body '{
  # ... 填写你的配置
}'
```

#### 1.3 节点池设计

```bash
# 要求创建 3 个节点池:
# 1. system-pool: 系统组件专用 (2 节点)
# 2. app-pool: 业务应用 (2-5 节点, 自动伸缩)
# 3. data-pool: 数据库专用 (2 节点, 大内存规格)

# 你的节点池创建命令:
```

---

### Phase 2: 安全加固 (1h)

#### 2.1 RBAC 权限设计

```
设计权限矩阵:

| 角色 | Namespace | 权限 |
|------|-----------|------|
| 运维工程师 | 全集群 | 读写全部资源 |
| 开发工程师 | app-ns | Pod/Deployment/Service 读写 |
| 测试工程师 | app-ns | Pod/Service 只读 + Pod 日志 |
| 安全审计 | 全集群 | 只读 |
```

```bash
# 创建 Role / ClusterRole / Binding
# 你的 YAML:
```

#### 2.2 资源配额

```bash
# 为 app-ns 设置合理的 ResourceQuota 和 LimitRange
# 你的 YAML:
```

#### 2.3 NetworkPolicy (如使用 Terway)

```bash
# 实现: app-ns 的 Pod 只能被同 Namespace 的 Pod 访问
# 数据库 Pod 只接受 API 后端的连接
# 你的 YAML:
```

---

### Phase 3: 应用部署 (2h)

#### 3.1 数据库层

```bash
# 要求:
# - StatefulSet 部署
# - 调度到 data-pool 节点
# - 独立云盘 PVC (至少 40Gi)
# - Headless Service

# 你的部署 YAML:
```

#### 3.2 API 后端

```bash
# 要求:
# - Deployment (3 副本)
# - 调度到 app-pool
# - 反亲和性 (分散到不同节点)
# - 完整探针配置 (startup + liveness + readiness)
# - 通过 ConfigMap 注入配置
# - 通过 Secret 注入数据库密码

# 你的部署 YAML:
```

#### 3.3 Web 前端

```bash
# 要求:
# - Deployment (3 副本)
# - ClusterIP Service
# - Ingress 路由 (域名: app.graduation.local)
# - TLS 证书 (自签名)

# 你的部署 YAML:
```

#### 3.4 架构验证

```bash
# 验证清单:
echo "=== 架构总览 ==="
kubectl get all -n app-ns -o wide

echo "=== 调度验证 ==="
kubectl get pods -n app-ns -o custom-columns='NAME:.metadata.name,NODE:.spec.nodeName,STATUS:.status.phase'

echo "=== 网络验证 ==="
kubectl get svc,ingress -n app-ns

echo "=== 存储验证 ==="
kubectl get pvc -n app-ns

echo "=== 端到端测试 ==="
INGRESS_IP=$(kubectl get svc -n kube-system nginx-ingress-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -k -H "Host: app.graduation.local" https://${INGRESS_IP}/
```

---

### Phase 4: 监控与运维 (1h)

#### 4.1 监控配置

```bash
# 确认 Prometheus 监控可用
# 创建自定义告警规则:
# - Pod 重启告警
# - 节点 CPU > 80% 告警
# - PVC 使用率 > 90% 告警

# 你的 PrometheusRule YAML:
```

#### 4.2 故障演练

```bash
# 演练 1: 模拟 Pod 故障
kubectl delete pod <api-pod-name> -n app-ns
# 观察: 自动恢复、readinessProbe 生效

# 演练 2: 模拟节点故障
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
# 观察: Pod 迁移、Service 自动更新 Endpoints

# 演练 3: 模拟 DNS 故障排查
kubectl exec <pod-name> -n app-ns -- nslookup db-svc
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=10
```

---

### Phase 5: 文档输出 (0.5h)

完成以下文档 (在自己的笔记中记录):

1. **集群架构图**: 画出集群的网络拓扑、节点池架构、应用部署图
2. **运维手册**: 记录日常运维操作 (扩容、升级、故障排查)
3. **经验总结**: 遇到的问题和解决方案

---

## 评分标准

| 评估项 | 满分 | 得分 |
|--------|:----:|:----:|
| 网络规划合理性 | 10 | |
| 集群创建与节点池配置 | 10 | |
| RBAC + 配额配置 | 10 | |
| 应用部署完整性 (三层架构) | 15 | |
| 调度策略正确性 | 10 | |
| 网络暴露 (Service + Ingress + TLS) | 10 | |
| 存储配置 (PVC + 持久化) | 10 | |
| 监控与告警 | 10 | |
| 故障演练与恢复 | 10 | |
| 文档输出质量 | 5 | |
| **合计** | **100** | |

**通过标准**: 80 分及以上

---

## 清理资源

```bash
# 删除应用
kubectl delete namespace app-ns

# 删除集群 (如不再需要)
aliyun cs DELETE /clusters/<cluster_id> --body '{"retain_all_resources": false}'

# 清理 VPC 资源
# aliyun vpc DeleteVSwitch --VSwitchId <vsw-id>
# aliyun vpc DeleteVpc --VpcId <vpc-id>
```

---

## 恭喜毕业！

完成本项目标志着你已具备 ACK 集群的独立运维能力。建议:

1. 将此项目的实操经验整理为团队文档
2. 在实际工作中持续应用所学
3. 关注 ACK 产品更新，持续学习新特性
4. 参与团队知识分享，教是最好的学
