# Day 22: Service 基础

> **学习时间**: 4-5 小时 | **主题**: Service 类型与配置实践

---

## 今日目标

- [ ] 理解 Service 的作用与核心机制 (kube-proxy / iptables / IPVS)
- [ ] 掌握 ClusterIP / NodePort / LoadBalancer 三种类型
- [ ] 能通过 ACK 控制台和 kubectl 创建 Service
- [ ] 了解 ACK 中 SLB (负载均衡) 与 LoadBalancer Service 的集成

---

## 理论学习 (2h)

### 必读文档

1. **K8S Service 基础**
   - 文件: `../../../domain-06-service-networking/01-service-overview.md`
   - 重点: Service 类型、selector 与 Endpoints

2. **ACK 网络管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/260-ack-networking.md`
   - 重点: ACK 中 Service 与 SLB 的自动关联

3. **kube-proxy 模式**
   - 文件: `../../../domain-06-service-networking/02-kube-proxy.md`
   - 重点: iptables 模式 vs IPVS 模式

### 阅读要点

- **ClusterIP**: 集群内部访问，默认类型
- **NodePort**: 通过节点端口暴露 (30000-32767)
- **LoadBalancer**: 在 ACK 中自动创建 SLB 实例
- **Headless Service**: `clusterIP: None`，用于 StatefulSet
- **ExternalName**: CNAME 映射到外部域名
- ACK 中可通过 annotation 指定 SLB 规格、带宽、计费方式

---

## 实践任务 (2.5h)

### 任务 1: ClusterIP Service (40min)

```bash
# 创建 Deployment
kubectl create deployment web --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24 --replicas=3

# 创建 ClusterIP Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-clusterip
spec:
  type: ClusterIP
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
    protocol: TCP
EOF

# 验证 Service
kubectl get svc web-clusterip
kubectl describe svc web-clusterip
kubectl get endpoints web-clusterip

# 集群内访问测试
kubectl run curl-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- http://web-clusterip
```

### 任务 2: NodePort Service (30min)

```bash
# 创建 NodePort Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
spec:
  type: NodePort
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
    nodePort: 30080
EOF

# 查看分配的 NodePort
kubectl get svc web-nodeport

# 通过节点 IP + NodePort 访问
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="InternalIP")].address}')
echo "访问地址: http://${NODE_IP}:30080"
```

### 任务 3: LoadBalancer Service (ACK + SLB) (40min)

```bash
# 创建 LoadBalancer Service (ACK 自动创建 SLB)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-lb
  annotations:
    # 指定 SLB 规格 (可选)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
    # 指定为内网 SLB (可选)
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
EOF

# 等待外部 IP 分配
kubectl get svc web-lb -w

# 查看关联的 SLB 信息
kubectl describe svc web-lb | grep "LoadBalancer Ingress"

# 通过 SLB 外部 IP 访问
EXTERNAL_IP=$(kubectl get svc web-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
echo "外部访问: http://${EXTERNAL_IP}"
```

### 任务 4: Headless Service 与 DNS (30min)

```bash
# 创建 Headless Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-headless
spec:
  clusterIP: None
  selector:
    app: web
  ports:
  - port: 80
EOF

# DNS 解析对比
kubectl run dns-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- sh -c '
  echo "=== ClusterIP Service ==="
  nslookup web-clusterip
  echo "=== Headless Service ==="
  nslookup web-headless
'

# 清理
kubectl delete svc web-clusterip web-nodeport web-lb web-headless
kubectl delete deploy web
```

---

## 费曼复述 (0.5h)

1. **Service 的 selector 如何关联到后端 Pod？Endpoints 对象的作用是什么？**
2. **LoadBalancer 类型在 ACK 中创建时，背后发生了什么？**
3. **Headless Service 的 DNS 解析与普通 ClusterIP Service 有什么区别？**

---

## 今日检验

- [ ] 能创建 ClusterIP / NodePort / LoadBalancer 三种 Service
- [ ] 理解 ACK 中 Service 与 SLB 的自动集成
- [ ] 能通过 DNS 和 IP 方式验证 Service 连通性
- [ ] 了解 Headless Service 的用途

---

## 核心概念总结

| Service 类型 | 访问范围 | ACK 集成 | 适用场景 |
|-------------|---------|---------|---------|
| ClusterIP | 集群内 | 无 | 内部微服务通信 |
| NodePort | 节点IP:端口 | 无 | 测试/开发环境 |
| LoadBalancer | 外部 IP | 自动创建 SLB | 生产环境暴露服务 |
| Headless | DNS 直接解析到 Pod | 无 | StatefulSet / 服务发现 |

---

## 明日预告

Day 23 将学习 Ingress 路由规则与 ALB/Nginx Ingress Controller 配置。
