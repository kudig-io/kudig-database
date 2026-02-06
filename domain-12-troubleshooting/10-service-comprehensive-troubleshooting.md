# 10 - Service 全面故障排查 (Service Comprehensive Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐ | **参考**: [Kubernetes Service官方文档](https://kubernetes.io/docs/concepts/services-networking/service/)

## 🎯 本文档价值

本文档专注于生产环境Service故障的系统性诊断和处理，提供：
- **完整的故障分类体系**：从网络连通性到负载均衡的全流程覆盖
- **实战诊断方法**：基于真实生产案例的故障分析技巧
- **自动化工具集**：可直接使用的诊断脚本和检查清单
- **预防性最佳实践**：避免常见Service问题的配置建议

---

## 相关文档交叉引用

### 🔗 关联故障排查文档
- **[03-CNI网络故障排查](./03-networking-cni-troubleshooting.md)** - 底层网络连通性问题
- **[25-网络连通性故障排查](./25-network-connectivity-troubleshooting.md)** - 网络层面诊断
- **[26-DNS故障排查](./26-dns-troubleshooting.md)** - 服务发现相关问题
- **[15-Ingress故障排查](./15-ingress-troubleshooting.md)** - 外部访问相关问题

### 📚 扩展学习资料
- **[Kubernetes网络模型](../domain-5-networking/01-networking-architecture-overview.md)** - 理解Service网络原理
- **[负载均衡器集成](../domain-5-networking/08-loadbalancer-cloud-provider-integration.md)** - 云提供商负载均衡配置

---

## 1. Service 故障诊断流程 (Troubleshooting Flow)

### 1.1 排查流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Service 故障排查流程                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────────────┐                                                     │
│   │ Service访问失败    │                                                     │
│   └─────────┬─────────┘                                                     │
│             │                                                               │
│             ▼                                                               │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ Step 1: 检查Service是否存在                          │                  │
│   │ kubectl get svc <name> -n <ns>                      │                  │
│   └─────────────────────────────────────────────────────┘                  │
│             │                                                               │
│             ├─── Service不存在 ──▶ 创建Service                              │
│             │                                                               │
│             ▼                                                               │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ Step 2: 检查Endpoints                                │                  │
│   │ kubectl get endpoints <name> -n <ns>                │                  │
│   └─────────────────────────────────────────────────────┘                  │
│             │                                                               │
│             ├─── Endpoints为空 ──▶ 检查selector/Pod状态                     │
│             │                                                               │
│             ▼                                                               │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ Step 3: 测试Pod直连                                  │                  │
│   │ kubectl exec -- curl <pod-ip>:<port>                │                  │
│   └─────────────────────────────────────────────────────┘                  │
│             │                                                               │
│             ├─── Pod直连失败 ──▶ 检查Pod应用/端口                           │
│             │                                                               │
│             ▼                                                               │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ Step 4: 检查kube-proxy                               │                  │
│   │ kubectl get pods -n kube-system -l k8s-app=kube-proxy│                  │
│   └─────────────────────────────────────────────────────┘                  │
│             │                                                               │
│             ├─── kube-proxy异常 ──▶ 重启kube-proxy                          │
│             │                                                               │
│             ▼                                                               │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ Step 5: 检查iptables/ipvs规则                        │                  │
│   │ iptables -t nat -L | grep <svc-name>                │                  │
│   └─────────────────────────────────────────────────────┘                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 常见问题速查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|:---|:---|:---|:---|
| Service不存在 | 未创建/命名空间错误 | `kubectl get svc -A` | 创建Service |
| Endpoints为空 | selector不匹配/Pod未Ready | `kubectl get ep` | 检查selector |
| ClusterIP不通 | kube-proxy故障/iptables | `iptables -t nat -L` | 检查kube-proxy |
| NodePort不通 | 防火墙/端口冲突 | `ss -tlnp` | 检查防火墙 |
| LoadBalancer Pending | 云provider未配置 | `kubectl describe svc` | 配置LB provider |
| 跨命名空间不通 | NetworkPolicy | `kubectl get netpol` | 调整策略 |

### 1.3 生产环境典型场景

#### 场景1：微服务架构中Service间调用失败
```yaml
# ❌ 问题现象
# 微服务A无法通过Service名称访问微服务B

# 🔍 诊断步骤
# 1. 检查DNS解析
kubectl exec -it <pod-name> -n <namespace> -- nslookup <service-name>.<namespace>.svc.cluster.local

# 2. 检查网络连通性
kubectl exec -it <pod-name> -n <namespace> -- ping <service-cluster-ip>

# 3. 检查Endpoints状态
kubectl get endpoints <service-name> -n <namespace>

# ✅ 解决方案
# 确保Service selector与Pod标签匹配
# 检查NetworkPolicy是否阻止了流量
# 验证Pod处于Running和Ready状态
```

#### 场景2：LoadBalancer服务长时间处于Pending状态
```bash
# ❌ 问题现象
# Service类型为LoadBalancer但EXTERNAL-IP一直显示<pending>

# 🔍 诊断步骤
# 1. 检查云提供商集成状态
kubectl describe service <service-name> -n <namespace> | grep -A10 "Events"

# 2. 检查云控制器管理器日志
kubectl logs -n kube-system -l k8s-app=kube-controller-manager

# 3. 验证云凭据配置
kubectl get secrets -n kube-system | grep cloud

# ✅ 解决方案
# 确保云提供商凭据正确配置
# 检查云配额和权限
# 验证网络ACL和安全组配置
```

#### 场景3：NodePort服务外部访问异常
```bash
# ❌ 问题现象
# NodePort服务从外部无法访问

# 🔍 诊断步骤
# 1. 检查节点防火墙规则
sudo iptables -L -n | grep <nodeport>

# 2. 检查节点网络配置
ip route show | grep <node-ip>

# 3. 验证服务端口监听
ss -tlnp | grep <nodeport>

# ✅ 解决方案
# 配置防火墙规则允许NodePort流量
# 检查网络路由配置
# 确保节点安全组开放相应端口
```

---

## 2. Endpoints 问题排查 (Endpoints Troubleshooting)

### 2.1 Endpoints 为空的原因

```bash
# === 检查Endpoints ===
kubectl get endpoints <svc-name> -n <namespace>
kubectl describe endpoints <svc-name> -n <namespace>

# === Endpoints为空的常见原因 ===
# 1. selector不匹配
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.selector}'
kubectl get pods -n <namespace> --show-labels

# 2. Pod未Ready
kubectl get pods -n <namespace> -l <label-selector>

# 3. Pod端口与Service端口不匹配
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.ports}'
kubectl get pods -n <namespace> -o jsonpath='{.items[0].spec.containers[0].ports}'
```

### 2.2 Selector 问题

```bash
# === 获取Service selector ===
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.selector}'
# 输出: {"app":"nginx"}

# === 查找匹配的Pod ===
kubectl get pods -n <namespace> -l app=nginx

# === 如果没有匹配的Pod ===
# 1. 检查Pod标签
kubectl get pods -n <namespace> --show-labels

# 2. 修复selector或Pod标签
kubectl label pod <pod-name> app=nginx --overwrite
```

### 2.3 Pod未Ready

```bash
# === 检查Pod Ready状态 ===
kubectl get pods -n <namespace> -l <label> -o custom-columns=NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status

# === Pod未Ready的原因 ===
# 1. 容器未启动
kubectl get pods -n <namespace> -o wide

# 2. ReadinessProbe失败
kubectl describe pod <pod-name> -n <namespace> | grep -A10 "Readiness"

# 3. 容器CrashLoopBackOff
kubectl logs <pod-name> -n <namespace>
```

---

## 3. ClusterIP Service 排查 (ClusterIP Troubleshooting)

### 3.1 连通性测试

```bash
# === 从集群内测试 ===
kubectl run test --rm -it --image=busybox --restart=Never -- wget -qO- http://<svc-name>.<namespace>.svc.cluster.local:<port>

# === 测试DNS解析 ===
kubectl run test --rm -it --image=busybox --restart=Never -- nslookup <svc-name>.<namespace>.svc.cluster.local

# === 直接测试ClusterIP ===
kubectl run test --rm -it --image=busybox --restart=Never -- wget -qO- http://<cluster-ip>:<port>

# === 测试Pod直连 ===
kubectl run test --rm -it --image=busybox --restart=Never -- wget -qO- http://<pod-ip>:<target-port>
```

### 3.2 kube-proxy 排查

```bash
# === 检查kube-proxy Pod ===
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# === 查看kube-proxy日志 ===
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# === 检查kube-proxy模式 ===
kubectl get configmap kube-proxy -n kube-system -o yaml | grep mode

# === iptables模式检查规则 ===
iptables -t nat -L KUBE-SERVICES | grep <svc-name>
iptables -t nat -L KUBE-SVC-<hash> -n

# === ipvs模式检查规则 ===
ipvsadm -Ln | grep <cluster-ip>
ipvsadm -Ln -t <cluster-ip>:<port>
```

### 3.3 iptables 规则分析

```bash
# === 查看Service链 ===
iptables -t nat -L KUBE-SERVICES -n | head -20

# === 查看特定Service的规则 ===
# 找到Service对应的链
iptables -t nat -L KUBE-SERVICES -n | grep <svc-cluster-ip>

# 查看链中的规则
iptables -t nat -L KUBE-SVC-XXXX -n

# === 常见问题 ===
# 规则缺失 → kube-proxy未同步
# 目标Pod IP不存在 → Endpoints过期
```

---

## 4. NodePort Service 排查 (NodePort Troubleshooting)

### 4.1 基本检查

```bash
# === 获取NodePort ===
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.ports[0].nodePort}'

# === 测试NodePort ===
curl http://<node-ip>:<node-port>

# === 检查端口监听 ===
# 在节点上
ss -tlnp | grep <node-port>
netstat -tlnp | grep <node-port>
```

### 4.2 NodePort 不通原因

| 原因 | 检查方法 | 解决方案 |
|:---|:---|:---|
| **防火墙阻断** | `iptables -L INPUT` | 开放端口 |
| **云安全组** | 云控制台检查 | 添加规则 |
| **端口冲突** | `ss -tlnp \| grep <port>` | 更换端口 |
| **kube-proxy故障** | `kubectl get pods -n kube-system` | 重启kube-proxy |
| **节点网络问题** | `ping <node-ip>` | 检查网络 |

### 4.3 防火墙配置

```bash
# === iptables ===
iptables -A INPUT -p tcp --dport <node-port> -j ACCEPT

# === firewalld ===
firewall-cmd --permanent --add-port=<node-port>/tcp
firewall-cmd --reload

# === 云安全组 (以阿里云为例) ===
# 控制台 → 安全组 → 添加入方向规则
# 协议: TCP
# 端口范围: 30000-32767 (或具体端口)
# 源: 0.0.0.0/0 或 限定IP
```

---

## 5. LoadBalancer Service 排查 (LoadBalancer Troubleshooting)

### 5.1 Pending 状态排查

```bash
# === 检查LB状态 ===
kubectl get svc <svc-name> -n <namespace>
kubectl describe svc <svc-name> -n <namespace>

# === 查看Events ===
kubectl describe svc <svc-name> -n <namespace> | grep -A10 Events

# === Pending的原因 ===
# 1. 云provider未配置
# 2. 云账号权限不足
# 3. 配额不足
# 4. 注解配置错误
```

### 5.2 云平台LB问题

| 云平台 | 常见问题 | 检查方法 |
|:---|:---|:---|
| **阿里云** | SLB配额不足 | 控制台检查配额 |
| **AWS** | ELB未创建 | AWS控制台 |
| **GCP** | 权限不足 | IAM检查 |
| **Azure** | 资源组问题 | Azure门户 |

### 5.3 阿里云SLB问题排查

```bash
# === 检查cloud-controller-manager ===
kubectl get pods -n kube-system -l app=cloud-controller-manager
kubectl logs -n kube-system -l app=cloud-controller-manager --tail=100

# === 检查Service注解 ===
kubectl get svc <svc-name> -n <namespace> -o yaml | grep annotations -A20

# === 常用阿里云注解 ===
# service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
# service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
```

---

## 6. Headless Service 排查 (Headless Troubleshooting)

### 6.1 Headless Service 特点

```bash
# === Headless Service (ClusterIP: None) ===
kubectl get svc <svc-name> -n <namespace>
# CLUSTER-IP应该显示None

# === DNS解析返回Pod IP列表 ===
kubectl run test --rm -it --image=busybox --restart=Never -- nslookup <svc-name>
# 应返回所有Pod的IP
```

### 6.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| DNS不返回IP | Pod未Ready | 检查Pod状态 |
| 只返回部分IP | 部分Pod不健康 | 检查Pod健康 |
| 返回旧IP | DNS缓存 | 等待TTL过期 |

---

## 7. ExternalName Service 排查 (ExternalName Troubleshooting)

### 7.1 DNS解析测试

```bash
# === 检查Service配置 ===
kubectl get svc <svc-name> -n <namespace> -o yaml

# === 测试DNS解析 ===
kubectl run test --rm -it --image=busybox --restart=Never -- nslookup <svc-name>.<namespace>.svc.cluster.local
# 应返回CNAME记录指向externalName
```

### 7.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| NXDOMAIN | externalName拼写错误 | 检查externalName |
| 解析到内网IP | DNS返回内网地址 | 检查外部DNS |
| 连接超时 | 网络不通 | 检查出口网络 |

---

## 8. Session Affinity 问题 (Session Affinity Issues)

### 8.1 检查Session Affinity配置

```bash
# === 查看配置 ===
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.sessionAffinity}'
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.sessionAffinityConfig}'

# === 测试会话保持 ===
for i in {1..10}; do curl http://<svc-ip>:<port> 2>/dev/null | grep hostname; done
```

### 8.2 配置Session Affinity

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时
```

---

## 9. NetworkPolicy 对Service的影响 (NetworkPolicy Impact)

### 9.1 检查NetworkPolicy

```bash
# === 查看命名空间的NetworkPolicy ===
kubectl get networkpolicy -n <namespace>

# === 查看策略详情 ===
kubectl describe networkpolicy <policy-name> -n <namespace>

# === 检查是否阻断Service流量 ===
# NetworkPolicy默认是增量的，确保允许必要的ingress/egress
```

### 9.2 允许Service流量示例

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-service-access
  namespace: my-namespace
spec:
  podSelector:
    matchLabels:
      app: my-app
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector: {}  # 允许所有命名空间
    ports:
    - protocol: TCP
      port: 8080
```

---

## 10. 诊断命令速查 (Quick Reference)

```bash
# === Service基本信息 ===
kubectl get svc -n <namespace>
kubectl describe svc <svc-name> -n <namespace>

# === Endpoints ===
kubectl get endpoints <svc-name> -n <namespace>

# === DNS测试 ===
kubectl run test --rm -it --image=busybox --restart=Never -- nslookup <svc-name>

# === 连通性测试 ===
kubectl run test --rm -it --image=nicolaka/netshoot --restart=Never -- curl -v http://<svc-name>:<port>

# === kube-proxy状态 ===
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50

# === iptables规则 ===
iptables -t nat -L KUBE-SERVICES -n | grep <svc-name>
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
