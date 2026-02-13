# Service 与 Ingress 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 10 分钟快速诊断

1. **资源链路快照**：`kubectl get svc,ep,endpointslices -n <ns> <svc> -o wide`，确认后端 Ready。
2. **Service 类型核对**：`kubectl get svc <svc> -o jsonpath='{.spec.type}'`，ClusterIP/NodePort/LB 的路径不同。
3. **数据面规则**：
   - iptables：`iptables -t nat -L -n | grep <cluster-ip>`
   - IPVS：`ipvsadm -Ln -t <cluster-ip>:<port>`
4. **Ingress 控制器健康**：`kubectl get pods -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx`，查看日志是否报 404/502/证书错误。
5. **Host/TLS 校验**：`curl -v http://<ingress-ip> -H "Host: <hostname>"`；TLS 证书检查 `kubectl get secret <tls-secret> -o yaml`。
6. **LB 健康检查**：云环境查看 LB 实例状态与安全组放通端口；确保健康检查路径与应用一致。
7. **快速缓解**：
   - Endpoints 为空：修复后端 Pod/Readiness 探针。
   - 规则缺失：重启 kube-proxy 或 Ingress Controller。
   - TLS 失败：更新 Secret 并触发热更新，必要时回滚证书。
8. **证据留存**：保存 Service/Ingress 描述、控制器日志、规则快照与 curl 输出。

---

## 1. 问题现象与影响分析

### 1.1 Service 常见问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| ClusterIP 无法访问 | `connection refused/timeout` | 客户端 | curl/telnet |
| NodePort 无法访问 | `connection refused` | 外部 | curl |
| LoadBalancer 无 External IP | `<pending>` | kubectl | `kubectl get svc` |
| Endpoints 为空 | 无后端 Pod | kubectl | `kubectl get endpoints` |
| 端口映射错误 | 连接错误端口 | 应用 | 连接测试 |

### 1.2 Ingress 常见问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Ingress Controller 异常 | `CrashLoopBackOff` | kubectl | `kubectl get pods` |
| 路由不生效 | `404 Not Found` | HTTP 响应 | curl |
| TLS 证书错误 | `SSL certificate error` | 浏览器/curl | curl -v |
| 后端不可达 | `502 Bad Gateway` | HTTP 响应 | curl |
| 地址未分配 | `ADDRESS` 为空 | kubectl | `kubectl get ingress` |

### 1.3 报错查看方式汇总

```bash
# Service 相关
kubectl get svc -A
kubectl describe svc <service-name>
kubectl get endpoints <service-name>
kubectl get endpointslices -l kubernetes.io/service-name=<service-name>

# Ingress 相关
kubectl get ingress -A
kubectl describe ingress <ingress-name>

# Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=200

# 测试 Service
kubectl run test --rm -it --image=busybox -- wget -qO- http://<cluster-ip>:<port>

# 测试 Ingress
curl -v http://<ingress-ip> -H "Host: <hostname>"
```

### 1.4 影响面分析

| 问题类型 | 影响范围 | 影响描述 |
|----------|----------|----------|
| Service 不可用 | 服务间调用 | 微服务调用失败 |
| Ingress 不可用 | 外部访问 | 用户无法访问应用 |
| LoadBalancer 问题 | 外部流量 | 云负载均衡失效 |
| TLS 证书问题 | HTTPS 访问 | 安全连接失败 |

---

## 2. 排查方法与步骤

### 2.1 排查原理与架构深度剖析

#### 2.1.1 Service 代理模式详解

Kubernetes Service 通过 kube-proxy 实现流量转发，支持三种代理模式：

**模式对比**

| 模式 | 实现方式 | 性能 | 可观测性 | 维护状态 | 适用场景 |
|------|---------|------|---------|---------|---------|
| **userspace** | 用户态代理 | 差（用户态-内核态切换） | 好（日志详细） | 已废弃 | - |
| **iptables** | Netfilter 规则链 | 中（内核态处理） | 差（无连接跟踪日志） | 默认模式 | 中小规模集群 |
| **IPVS** | Linux 虚拟服务器 | 优（LVS 内核模块） | 中（ipvsadm 查询） | 推荐 | 大规模集群（>1000 Service） |

---

**iptables 模式架构**

```
┌────────────────────────────────────────────────────────────┐
│                         Client Pod                          │
│                     (10.244.1.5)                            │
└───────────────────────────┬────────────────────────────────┘
                            │ 访问 ClusterIP:Port
                            │ (10.96.100.10:80)
                            v
┌────────────────────────────────────────────────────────────┐
│                      iptables 规则链                        │
│                                                              │
│  PREROUTING (nat)                                           │
│    └─> KUBE-SERVICES                                        │
│         └─> KUBE-SVC-XXXXX  (匹配 10.96.100.10:80)         │
│              ├─> KUBE-SEP-AAAAA (33% 概率) ──> 10.244.2.10:8080
│              ├─> KUBE-SEP-BBBBB (50% 概率) ──> 10.244.3.15:8080
│              └─> KUBE-SEP-CCCCC (100% 概率)──> 10.244.1.20:8080
│                                                              │
│  POSTROUTING (nat)                                          │
│    └─> KUBE-POSTROUTING  (SNAT 源地址伪装)                 │
└────────────────────────────────────────────────────────────┘
```

**关键规则示例**

```bash
# 查看 Service 入口规则
iptables -t nat -L KUBE-SERVICES -n | grep 10.96.100.10
# -A KUBE-SERVICES -d 10.96.100.10/32 -p tcp -m tcp --dport 80 -j KUBE-SVC-XXXXX

# 查看负载均衡规则（随机概率）
iptables -t nat -L KUBE-SVC-XXXXX -n
# -A KUBE-SVC-XXXXX -m statistic --mode random --probability 0.33333 -j KUBE-SEP-AAAAA
# -A KUBE-SVC-XXXXX -m statistic --mode random --probability 0.50000 -j KUBE-SEP-BBBBB
# -A KUBE-SVC-XXXXX -j KUBE-SEP-CCCCC  # 最后一个 100% 兜底

# 查看 Endpoint 规则
iptables -t nat -L KUBE-SEP-AAAAA -n
# -A KUBE-SEP-AAAAA -p tcp -m tcp -j DNAT --to-destination 10.244.2.10:8080
```

**性能瓶颈**

- **规则数量爆炸**：Service 数量 × Endpoints 数量 = O(N×M) 条规则
  - 1000 个 Service × 10 个 Pod = 10000+ 条规则
  - 规则匹配时间复杂度 O(N)，延迟随规则增长线性增加
- **无连接会话亲和**：默认随机选择 Endpoint，同一客户端请求可能路由到不同后端
- **更新延迟**：大规模集群中，刷新全量规则需要数秒

---

**IPVS 模式架构**

```
┌────────────────────────────────────────────────────────────┐
│                         Client Pod                          │
│                     (10.244.1.5)                            │
└───────────────────────────┬────────────────────────────────┘
                            │ 访问 ClusterIP:Port
                            │ (10.96.100.10:80)
                            v
┌────────────────────────────────────────────────────────────┐
│                    IPVS Virtual Server                      │
│                                                              │
│  Virtual Service: 10.96.100.10:80                           │
│    Algorithm: rr (Round Robin)                              │
│    Scheduler: wrr / lc / lblc / dh 等                       │
│                                                              │
│    Real Servers:                                            │
│      -> 10.244.2.10:8080   (Weight: 1)                      │
│      -> 10.244.3.15:8080   (Weight: 1)                      │
│      -> 10.244.1.20:8080   (Weight: 1)                      │
│                                                              │
│  Connection Table (连接跟踪)                                │
│    10.244.1.5:52345 -> 10.244.2.10:8080 [ESTABLISHED]       │
└────────────────────────────────────────────────────────────┘
```

**IPVS 负载均衡算法**

| 算法 | 缩写 | 策略 | 适用场景 |
|------|------|------|---------|
| Round Robin | `rr` | 轮询 | 后端性能均等 |
| Weighted RR | `wrr` | 加权轮询 | 后端性能差异大 |
| Least Connection | `lc` | 最少连接 | 长连接服务 |
| Destination Hashing | `dh` | 目标地址哈希 | 会话保持 |
| Source Hashing | `sh` | 源地址哈希 | 客户端会话保持 |

**IPVS 优势**

- **哈希表查找**：O(1) 时间复杂度，性能不随规则数增长
- **连接保持**：内置会话亲和性（`sh` 算法）
- **灵活调度**：10+ 种负载均衡算法
- **更好可观测**：`ipvsadm` 工具查看连接统计

**启用 IPVS 模式**

```yaml
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    mode: "ipvs"  # 切换为 IPVS
    ipvs:
      scheduler: "rr"  # 调度算法
      syncPeriod: 30s
      minSyncPeriod: 5s
    # IPVS 回退策略：IPVS 不可用时自动降级为 iptables
    # mode: "" 则自动检测

# 重启 kube-proxy
kubectl rollout restart daemonset -n kube-system kube-proxy

# 验证 IPVS 规则
ipvsadm -Ln
# IP Virtual Server version 1.2.1 (size=4096)
# Prot LocalAddress:Port Scheduler Flags
#   -> RemoteAddress:Port           Forward Weight ActiveConn InActConn
# TCP  10.96.100.10:80 rr
#   -> 10.244.2.10:8080             Masq    1      0          0
#   -> 10.244.3.15:8080             Masq    1      0          0
```

---

#### 2.1.2 Service 类型深度解析

**ClusterIP（集群内访问）**

```yaml
apiVersion: v1
kind: Service
metadata:
  name: backend
spec:
  type: ClusterIP
  selector:
    app: backend
  ports:
  - port: 80          # Service 端口
    targetPort: 8080  # Pod 端口
    protocol: TCP
```

**流量路径**：
```
Pod A (10.244.1.5) 
  └─> ClusterIP (10.96.100.10:80) 
       └─> iptables/IPVS DNAT 
            └─> Pod B (10.244.2.10:8080)
```

**注意事项**：
- ClusterIP 仅在集群内可达（不可从节点外访问）
- DNS 自动生成记录：`<service>.<namespace>.svc.cluster.local`

---

**NodePort（节点端口暴露）**

```yaml
apiVersion: v1
kind: Service
metadata:
  name: frontend
spec:
  type: NodePort
  selector:
    app: frontend
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080  # 节点端口（30000-32767）
```

**流量路径**：
```
外部客户端 
  └─> NodeIP:30080 
       └─> iptables DNAT 
            ├─> 本节点 Pod (10.244.2.10:8080) 
            └─> 其他节点 Pod (10.244.3.15:8080)  # 额外跳转开销
```

**注意事项**：
- 访问任意节点的 NodePort 均可到达服务
- **externalTrafficPolicy: Local** 避免跨节点跳转（仅路由到本地 Pod）
- 端口范围有限，不适合大量服务

---

**LoadBalancer（云负载均衡）**

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
  externalTrafficPolicy: Local  # 保留源 IP
```

**流量路径（云环境）**：
```
外部客户端 
  └─> 云 LB (1.2.3.4:80) 
       └─> NodePort (NodeIP:32345) 
            └─> iptables/IPVS 
                 └─> Pod (10.244.2.10:8080)
```

**云 LB 健康检查**：
- **默认行为**：检查 NodePort 可达性（不考虑 Pod 健康状态）
- **externalTrafficPolicy: Local**：
  - 仅路由到本地 Pod（避免跨节点）
  - 健康检查失败时，LB 自动摘除节点
  - **保留真实源 IP**（无 SNAT）

**注意事项**：
- 依赖云厂商实现（裸机需要 MetalLB）
- External IP 分配需要时间（通常 1-3 分钟）
- 成本较高（每个 LB 实例收费）

---

#### 2.1.3 Ingress Controller 架构深度剖析

**Ingress 架构概览**

```
┌──────────────────────────────────────────────────────────┐
│                   External Client                         │
│                 (user.browser.com)                        │
└─────────────────────────┬────────────────────────────────┘
                          │ HTTP/HTTPS
                          │ Host: api.example.com
                          v
┌──────────────────────────────────────────────────────────┐
│                Cloud LoadBalancer                         │
│                 (1.2.3.4:80/443)                          │
└─────────────────────────┬────────────────────────────────┘
                          │
                          v
┌──────────────────────────────────────────────────────────┐
│           Ingress Controller (Nginx/Traefik)             │
│                                                            │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Config Watcher (监听 Ingress 资源)              │   │
│  │    └─> 动态生成 Nginx 配置                       │   │
│  │         └─> nginx.conf                            │   │
│  │              server {                             │   │
│  │                listen 80;                         │   │
│  │                server_name api.example.com;       │   │
│  │                location / {                       │   │
│  │                  proxy_pass http://backend-svc;   │   │
│  │                }                                  │   │
│  │              }                                    │   │
│  └──────────────────────────────────────────────────┘   │
│                                                            │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Reverse Proxy (Nginx Worker)                    │   │
│  │    ├─> TLS 终止                                  │   │
│  │    ├─> 路径路由 (/api -> backend-svc)           │   │
│  │    ├─> 负载均衡 (Upstream)                       │   │
│  │    └─> Header 注入 (X-Forwarded-For)            │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────┬────────────────────────────────┘
                          │ HTTP (ClusterIP)
                          │ backend-svc:80
                          v
┌──────────────────────────────────────────────────────────┐
│                   Backend Service                         │
│  ClusterIP: 10.96.100.10:80                              │
│    └─> Endpoints:                                        │
│         ├─> Pod A (10.244.2.10:8080)                     │
│         ├─> Pod B (10.244.3.15:8080)                     │
│         └─> Pod C (10.244.1.20:8080)                     │
└──────────────────────────────────────────────────────────┘
```

---

**Nginx Ingress Controller 核心机制**

1. **配置同步流程**

```
Kubernetes API Server
  └─> Ingress/Service/Endpoint 变更
       └─> Controller Watch 机制
            └─> 计算配置差异
                 └─> 生成新 nginx.conf
                      └─> 执行配置测试 (nginx -t)
                           └─> 热重载 (nginx -s reload)
                                └─> Worker 平滑更新（毫秒级）
```

2. **Upstream 动态更新**

```nginx
# 传统静态配置（需重启）
upstream backend {
    server 10.244.2.10:8080;
    server 10.244.3.15:8080;
}

# Nginx Plus / OpenResty 动态更新（无需重启）
upstream backend {
    zone backend 64k;
    # Endpoints 通过共享内存动态更新
}
```

Nginx Ingress Controller 使用 `lua-resty-core` 实现动态 Upstream：
- **balancer_by_lua**：Lua 脚本实时查询 Endpoints
- **无需重载**：Endpoints 变更立即生效（<100ms）
- **连接复用**：Keepalive 连接池提升性能

3. **关键 Annotations**

| Annotation | 功能 | 示例值 | 影响 |
|-----------|------|--------|------|
| `nginx.ingress.kubernetes.io/rewrite-target` | URL 重写 | `/$2` | 路径转换 |
| `nginx.ingress.kubernetes.io/ssl-redirect` | 强制 HTTPS | `"true"` | 自动跳转 |
| `nginx.ingress.kubernetes.io/proxy-body-size` | 请求体大小限制 | `"50m"` | 上传限制 |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | 连接超时 | `"10"` | 后端连接 |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | 读超时 | `"60"` | 慢接口 |
| `nginx.ingress.kubernetes.io/rate-limit` | 限流 | `"100"` | QPS 限制 |
| `nginx.ingress.kubernetes.io/whitelist-source-range` | IP 白名单 | `"10.0.0.0/8"` | 访问控制 |

4. **TLS 证书管理**

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: tls-secret
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-cert>
  tls.key: <base64-encoded-key>
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: secure-ingress
spec:
  tls:
  - hosts:
    - api.example.com
    secretName: tls-secret  # 引用 Secret
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 80
```

**证书更新流程**：
```
kubectl create secret tls tls-secret --cert=new.crt --key=new.key --dry-run=client -o yaml | kubectl apply -f -
  └─> Secret 更新
       └─> Controller 检测变更
            └─> 重新加载证书（nginx -s reload）
                 └─> 新连接使用新证书（旧连接继续）
```

---

**Ingress 路径匹配规则**

| pathType | 行为 | 示例 | 匹配 | 不匹配 |
|----------|------|------|------|--------|
| `Exact` | 精确匹配 | `/foo` | `/foo` | `/foo/`, `/foobar` |
| `Prefix` | 前缀匹配（按元素） | `/foo` | `/foo`, `/foo/`, `/foo/bar` | `/foobar` |
| `ImplementationSpecific` | 实现定义 | `/foo` | 取决于 Controller | - |

**Nginx 路径匹配优先级**：
1. Exact 精确匹配
2. Prefix 最长前缀匹配
3. 正则匹配（通过 annotations）

```yaml
# 复杂路由示例
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: complex-routing
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /$2
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      # 精确匹配 /api
      - path: /api
        pathType: Exact
        backend:
          service:
            name: api-v1
            port:
              number: 80
      # 前缀匹配 /api/v2/*
      - path: /api/v2(/|$)(.*)
        pathType: Prefix
        backend:
          service:
            name: api-v2
            port:
              number: 80
      # 默认后端
      - path: /
        pathType: Prefix
        backend:
          service:
            name: default-backend
            port:
              number: 80
```

---

#### 2.1.4 服务网格 vs. Ingress

| 维度 | Ingress | Service Mesh (Istio/Linkerd) |
|------|---------|------------------------------|
| **流量入口** | 南北向（集群外→集群内） | 东西向（集群内 Pod→Pod） |
| **协议支持** | HTTP/HTTPS/gRPC | HTTP/TCP/gRPC/任意协议 |
| **负载均衡** | L7（应用层） | L4+L7（传输层+应用层） |
| **TLS 终止** | Ingress Controller | Sidecar Proxy（mTLS） |
| **可观测** | 基础指标（QPS/延迟） | 分布式追踪/细粒度指标 |
| **流量控制** | 路径路由/限流 | 金丝雀/蓝绿/熔断/重试 |
| **复杂度** | 低 | 高（Sidecar 注入/控制平面） |
| **适用场景** | 外部流量入口 | 微服务间通信治理 |

### 2.2 Service 排查步骤

#### 2.2.1 检查 Service 配置

```bash
# 查看 Service 详情
kubectl get svc <service-name> -o yaml

# 检查 selector 是否匹配 Pod
kubectl get svc <service-name> -o jsonpath='{.spec.selector}'
kubectl get pods -l <selector-key>=<selector-value>

# 检查端口配置
kubectl get svc <service-name> -o jsonpath='{.spec.ports}'

# 检查 Service 类型
kubectl get svc <service-name> -o jsonpath='{.spec.type}'
```

#### 2.2.2 检查 Endpoints

```bash
# 查看 Endpoints
kubectl get endpoints <service-name> -o yaml

# 查看 EndpointSlices
kubectl get endpointslices -l kubernetes.io/service-name=<service-name> -o yaml

# 如果 Endpoints 为空，检查 Pod 状态
kubectl get pods -l <selector> -o wide

# 检查 Pod 是否 Ready
kubectl get pods -l <selector> -o jsonpath='{.items[*].status.conditions}'
```

#### 2.2.3 测试连通性

```bash
# 测试 ClusterIP
kubectl run test --rm -it --image=busybox -- sh
# 在 Pod 内
wget -qO- http://<cluster-ip>:<port>
nc -zv <cluster-ip> <port>

# 测试 NodePort
curl http://<node-ip>:<node-port>

# 检查 iptables/IPVS 规则
iptables -t nat -L -n | grep <cluster-ip>
ipvsadm -Ln -t <cluster-ip>:<port>
```

### 2.3 Ingress 排查步骤

#### 2.3.1 检查 Ingress Controller

```bash
# 检查 Controller Pod 状态
kubectl get pods -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 查看 Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=200

# 检查 Controller Service
kubectl get svc -n ingress-nginx
```

#### 2.3.2 检查 Ingress 配置

```bash
# 查看 Ingress 详情
kubectl get ingress <ingress-name> -o yaml

# 检查 rules 配置
kubectl get ingress <ingress-name> -o jsonpath='{.spec.rules}'

# 检查 TLS 配置
kubectl get ingress <ingress-name> -o jsonpath='{.spec.tls}'

# 验证 Secret 存在
kubectl get secret <tls-secret-name>
```

#### 2.3.3 测试 Ingress

```bash
# 获取 Ingress IP
kubectl get ingress <ingress-name> -o jsonpath='{.status.loadBalancer.ingress[0].ip}'

# 测试 HTTP
curl -v http://<ingress-ip> -H "Host: <hostname>"

# 测试 HTTPS
curl -vk https://<ingress-ip> -H "Host: <hostname>"

# 检查证书
openssl s_client -connect <ingress-ip>:443 -servername <hostname>
```

---

## 3. 解决方案与风险控制

### 3.1 Service Endpoints 为空

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查 selector 匹配
SERVICE_SELECTOR=$(kubectl get svc <service-name> -o jsonpath='{.spec.selector}')
echo "Service selector: $SERVICE_SELECTOR"

# 步骤 2：查找匹配的 Pod
kubectl get pods -l <selector-key>=<selector-value>

# 步骤 3：如果 Pod 存在但不在 Endpoints 中
# 检查 Pod 是否 Ready
kubectl get pods -l <selector> -o wide
kubectl describe pod <pod-name> | grep -A5 Conditions

# 步骤 4：如果 Pod 未 Ready
# 检查探针配置
kubectl get pod <pod-name> -o yaml | grep -A10 readinessProbe

# 步骤 5：修复 Pod 或 Service selector
# 修改 Service selector
kubectl patch svc <service-name> -p '{"spec":{"selector":{"app":"correct-label"}}}'

# 或修改 Pod labels
kubectl label pod <pod-name> app=correct-label

# 步骤 6：验证 Endpoints
kubectl get endpoints <service-name>
```

#### 3.1.2 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 修改 selector 会立即影响流量路由
2. 确保新 selector 匹配正确的 Pod
3. 检查 readinessProbe 是否过于严格
4. Pod 未 Ready 不会加入 Endpoints
```

### 3.2 LoadBalancer External IP Pending

#### 3.2.1 解决步骤

```bash
# 步骤 1：检查 Service 状态
kubectl get svc <service-name>
kubectl describe svc <service-name>

# 步骤 2：检查云平台配额
# 阿里云
aliyun slb DescribeLoadBalancers
# AWS
aws elbv2 describe-load-balancers

# 步骤 3：检查是否有 Cloud Controller Manager
kubectl get pods -n kube-system | grep cloud-controller

# 步骤 4：查看 Cloud Controller 日志
kubectl logs -n kube-system -l app=cloud-controller-manager --tail=100

# 步骤 5：检查节点标签（某些云需要）
kubectl get nodes --show-labels | grep -E "node.kubernetes.io|topology"

# 步骤 6：如果是裸金属环境，使用 MetalLB
# 部署 MetalLB
kubectl apply -f https://raw.githubusercontent.com/metallb/metallb/main/config/manifests/metallb-native.yaml

# 步骤 7：验证 External IP 分配
kubectl get svc <service-name>
```

#### 3.2.2 安全生产风险提示

```
⚠️  安全生产风险提示：
1. LoadBalancer 创建可能需要几分钟
2. 云平台配额限制可能阻止创建
3. 裸金属环境需要额外的负载均衡方案
4. External IP 分配后检查安全组配置
```

### 3.3 Ingress 502 Bad Gateway

#### 3.3.1 解决步骤

```bash
# 步骤 1：检查后端 Service
kubectl get svc <backend-service>
kubectl get endpoints <backend-service>

# 步骤 2：检查后端 Pod 健康
kubectl get pods -l <backend-selector>
kubectl describe pod <backend-pod>

# 步骤 3：检查 Ingress Controller 到后端的连通性
kubectl exec -n ingress-nginx <nginx-pod> -- curl -v http://<service-cluster-ip>:<port>

# 步骤 4：查看 Nginx 配置
kubectl exec -n ingress-nginx <nginx-pod> -- cat /etc/nginx/nginx.conf | grep -A20 "location"

# 步骤 5：检查 Ingress annotations
kubectl get ingress <ingress-name> -o yaml | grep -A10 annotations

# 步骤 6：调整超时配置
kubectl annotate ingress <ingress-name> nginx.ingress.kubernetes.io/proxy-connect-timeout="30"
kubectl annotate ingress <ingress-name> nginx.ingress.kubernetes.io/proxy-read-timeout="60"

# 步骤 7：验证修复
curl -v http://<ingress-ip> -H "Host: <hostname>"
```

#### 3.3.2 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 502 通常表示后端不可达
2. 检查后端 Service 和 Pod 是第一步
3. 超时配置影响慢请求处理
4. Ingress annotations 立即生效
```

### 3.4 TLS 证书配置

#### 3.4.1 解决步骤

```bash
# 步骤 1：检查 TLS Secret 存在
kubectl get secret <tls-secret-name>

# 步骤 2：验证证书内容
kubectl get secret <tls-secret-name> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -text

# 步骤 3：检查证书有效期
kubectl get secret <tls-secret-name> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates

# 步骤 4：创建新的 TLS Secret
kubectl create secret tls <tls-secret-name> \
  --cert=path/to/tls.crt \
  --key=path/to/tls.key

# 步骤 5：更新 Ingress 引用
kubectl patch ingress <ingress-name> -p '{
  "spec": {
    "tls": [{
      "hosts": ["example.com"],
      "secretName": "<tls-secret-name>"
    }]
  }
}'

# 步骤 6：验证 TLS
curl -vk https://<ingress-ip> -H "Host: <hostname>"
openssl s_client -connect <ingress-ip>:443 -servername <hostname>
```

#### 3.4.2 安全生产风险提示

```
⚠️  安全生产风险提示：
1. TLS Secret 包含私钥，需要保护
2. 证书更新会立即生效
3. 证书域名必须匹配 Ingress host
4. 定期检查证书有效期
5. 考虑使用 cert-manager 自动管理
```

---

## 附录

### A. Service 类型对比

| 类型 | 访问范围 | External IP | 端口 |
|------|----------|-------------|------|
| ClusterIP | 集群内 | 无 | 任意 |
| NodePort | 集群外 | 节点 IP | 30000-32767 |
| LoadBalancer | 集群外 | 云 LB IP | 任意 |
| ExternalName | DNS 别名 | 无 | - |

### B. 常用 Ingress Annotations

| Annotation | 说明 | 示例值 |
|------------|------|--------|
| `nginx.ingress.kubernetes.io/rewrite-target` | 路径重写 | `/` |
| `nginx.ingress.kubernetes.io/ssl-redirect` | HTTPS 重定向 | `"true"` |
| `nginx.ingress.kubernetes.io/proxy-body-size` | 请求体大小 | `"50m"` |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | 连接超时 | `"30"` |

### C. 排查清单

- [ ] Service selector 匹配 Pod labels
- [ ] Pod 状态为 Ready
- [ ] Endpoints 不为空
- [ ] 端口配置正确
- [ ] kube-proxy 规则存在
- [ ] Ingress Controller 正常
- [ ] TLS Secret 有效
- [ ] 网络策略未阻止流量

---

## 4. 生产环境典型案例

### 案例 1：IPVS 模式下 Service 不可用导致业务大面积故障

**故障现场**

- **现象**：某微服务集群升级 kube-proxy 至 IPVS 模式后，30% 的 Service 调用失败（`connection refused`）
- **影响范围**：200+ 个微服务，涉及 50+ 个 Service
- **业务影响**：
  - 订单服务调用支付服务失败，交易成功率降至 70%
  - 前端 API Gateway 频繁返回 502/504
  - 用户投诉量激增，客服系统瘫痪

**排查过程**

```bash
# 1. 确认故障范围
kubectl run test --rm -it --image=busybox -- sh
# 在 Pod 内测试
wget -qO- http://payment-service.default.svc.cluster.local
# wget: can't connect to remote host (10.96.100.10): Connection refused
# ❌ Service 不可达

# 2. 检查 Service 和 Endpoints 正常
kubectl get svc payment-service
# NAME              TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)   AGE
# payment-service   ClusterIP   10.96.100.10   <none>        80/TCP    30d

kubectl get endpoints payment-service
# NAME              ENDPOINTS                               AGE
# payment-service   10.244.2.10:8080,10.244.3.15:8080,...   30d
# ✅ Endpoints 正常

# 3. 检查 Pod 健康状态
kubectl get pods -l app=payment -o wide
# NAME                       READY   STATUS    RESTARTS   IP
# payment-7d8f4b6c9-abc12    1/1     Running   0          10.244.2.10
# payment-7d8f4b6c9-def34    1/1     Running   0          10.244.3.15
# ✅ Pod 正常运行

# 4. 检查 kube-proxy 模式
kubectl logs -n kube-system kube-proxy-xxxxx | head -20
# I0101 10:00:00.000000   1 server_others.go:258] Using ipvs Proxier.
# ✅ 已切换为 IPVS 模式

# 5. 检查 IPVS 规则
ipvsadm -Ln | grep 10.96.100.10
# （无输出）
# ❌ IPVS 规则缺失！

# 6. 检查节点内核模块
lsmod | grep ip_vs
# （无输出）
# ❌ IPVS 内核模块未加载！

# 7. 检查节点系统日志
dmesg | grep -i ipvs
# [FAILED] modprobe: FATAL: Module ip_vs not found
# ❌ 内核不支持 IPVS

# 8. 根因分析：
# - kube-proxy 切换为 IPVS 模式，但节点内核未加载 IPVS 模块
# - kube-proxy 未回退到 iptables 模式（配置未设置回退）
# - 30% 节点因内核版本过低不支持 IPVS（CentOS 7.4，kernel 3.10.0-693）
```

**应急措施**

```bash
# 方案 A：紧急回退到 iptables 模式（10 分钟恢复）
kubectl edit cm -n kube-system kube-proxy
# 修改 mode: "iptables"

# 滚动重启 kube-proxy（逐节点重启）
kubectl rollout restart daemonset -n kube-system kube-proxy

# 验证规则生成
iptables -t nat -L KUBE-SERVICES -n | grep 10.96.100.10
# -A KUBE-SERVICES -d 10.96.100.10/32 -p tcp -m tcp --dport 80 -j KUBE-SVC-XXXXX
# ✅ iptables 规则恢复

# 业务验证
kubectl run test --rm -it --image=busybox -- wget -qO- http://payment-service
# HTTP/1.1 200 OK
# ✅ 服务恢复

# 方案 B：升级内核（长期方案）
# 在不支持 IPVS 的节点上：
yum install -y kernel-lt  # 升级至 5.x 内核
grub2-set-default 0       # 设置默认启动内核
reboot                    # 重启节点

# 重启后验证
uname -r
# 5.4.200-1.el7.elrepo.x86_64
# ✅ 内核升级成功

modprobe ip_vs ip_vs_rr ip_vs_wrr ip_vs_sh
lsmod | grep ip_vs
# ip_vs_sh               12688  0
# ip_vs_wrr              12697  0
# ip_vs_rr               12600  0
# ip_vs                 145458  6 ip_vs_rr,ip_vs_sh,ip_vs_wrr
# ✅ IPVS 模块加载成功
```

**长期优化**

```yaml
# 1. kube-proxy 配置优化（支持自动回退）
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    mode: ""  # ← 自动检测（IPVS 不可用时降级为 iptables）
    ipvs:
      scheduler: "rr"
      syncPeriod: 30s
      minSyncPeriod: 5s
      # 严格 ARP 模式（避免 ARP 缓存问题）
      strictARP: true
    # 连接跟踪配置
    conntrack:
      maxPerCore: 32768
      min: 131072
      tcpEstablishedTimeout: 86400s
      tcpCloseWaitTimeout: 3600s

# 2. 节点初始化脚本（确保 IPVS 模块加载）
cat <<EOF > /etc/modules-load.d/ipvs.conf
# Load IPVS modules at boot
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

# 立即加载模块
modprobe ip_vs ip_vs_rr ip_vs_wrr ip_vs_sh nf_conntrack

# 验证加载
lsmod | grep -E 'ip_vs|nf_conntrack'

# 3. 内核参数优化
cat <<EOF > /etc/sysctl.d/k8s-ipvs.conf
# IPVS 连接跟踪
net.ipv4.vs.conntrack = 1
# 连接跟踪表大小
net.netfilter.nf_conntrack_max = 1048576
# 连接跟踪超时
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 3600
# 转发配置
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1
EOF

sysctl -p /etc/sysctl.d/k8s-ipvs.conf

# 4. 监控 IPVS 规则数量
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kube-proxy-ipvs-alert
  namespace: kube-system
spec:
  groups:
  - name: kube-proxy
    interval: 30s
    rules:
    # IPVS 规则缺失
    - alert: IPVS_RulesMissing
      expr: |
        kube_proxy_sync_proxy_rules_last_timestamp_seconds{mode="ipvs"} 
        and on(instance) (node_systemd_unit_state{name="kube-proxy.service",state="active"} == 1)
        and on(instance) (count by(instance) (node_ipvs_backend_connections_active) == 0)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "节点 {{ \$labels.instance }} IPVS 规则缺失"
        description: "检查 IPVS 内核模块是否加载"
    
    # kube-proxy 规则同步失败
    - alert: KubeProxy_SyncFailed
      expr: increase(kubeproxy_sync_proxy_rules_duration_seconds_count{result="error"}[5m]) > 0
      labels:
        severity: warning
      annotations:
        summary: "kube-proxy 规则同步失败"
EOF
```

**渐进式升级策略**

```bash
# 阶段 1：金丝雀测试（1 周）
# 选择 5% 节点切换 IPVS，验证稳定性
kubectl label node node-1 node-2 proxy-mode=ipvs

# 部署金丝雀 kube-proxy DaemonSet
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kube-proxy-ipvs
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: kube-proxy-ipvs
  template:
    metadata:
      labels:
        k8s-app: kube-proxy-ipvs
    spec:
      nodeSelector:
        proxy-mode: ipvs  # ← 仅在标记节点运行
      # ... kube-proxy 配置（mode: ipvs）
EOF

# 监控金丝雀节点指标
watch -n 5 'ipvsadm -Ln --stats | head -20'

# 阶段 2：灰度发布（2 周）
# 扩展至 50% 节点
kubectl label node node-3 node-4 node-5 proxy-mode=ipvs

# 阶段 3：全量发布
# 确认无问题后，全局切换
kubectl edit cm -n kube-system kube-proxy
# mode: "ipvs"
kubectl rollout restart daemonset -n kube-system kube-proxy
```

**效果评估**

| 指标 | iptables 模式 | IPVS 模式 | 改善 |
|-----|--------------|-----------|------|
| Service 查找延迟 | 15ms（5000 规则） | 0.5ms | ↓ 97% |
| kube-proxy CPU | 2.5 核 | 0.8 核 | ↓ 68% |
| 规则同步时间 | 8s | 0.3s | ↓ 96% |
| 连接会话保持 | 不支持 | 支持（`sh` 算法） | ✅ 新增 |

---

### 案例 2：Ingress TLS 证书过期导致 HTTPS 服务全面中断

**故障现场**

- **现象**：凌晨 00:00 所有 HTTPS 域名返回证书错误（`ERR_CERT_DATE_INVALID`）
- **影响范围**：全站 20+ 域名，100% HTTPS 流量
- **业务影响**：
  - 用户无法访问 Web 应用
  - 移动 APP API 调用失败（证书校验失败）
  - 第三方回调失败（Webhook 签名验证失败）

**排查过程**

```bash
# 1. 用户反馈验证
curl -v https://api.example.com
# * SSL certificate problem: certificate has expired
# ❌ 证书过期

# 2. 检查 Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50
# 2024/01/01 00:00:05 [warn] SSL certificate "default/tls-secret" expired
# 2024/01/01 00:00:05 [error] SSL_do_handshake() failed (SSL: error:1417A0C1)
# ❌ 证书过期告警

# 3. 查看 TLS Secret
kubectl get secret tls-secret -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
# notBefore=Jan  1 00:00:00 2023 GMT
# notAfter=Dec 31 23:59:59 2023 GMT  # ← 昨天过期！
# ❌ 证书已过期

# 4. 检查 cert-manager（自动证书管理）
kubectl get certificate -A
# NAME          READY   SECRET        AGE
# tls-cert      False   tls-secret    365d  # ← READY=False

kubectl describe certificate tls-cert
# Events:
#   Warning  Failed   1h    cert-manager  Failed to renew certificate: acme: authorization failed
# ❌ 证书自动续期失败（Let's Encrypt ACME 挑战失败）

# 5. 分析根因
kubectl logs -n cert-manager -l app=cert-manager --tail=100 | grep -i error
# E0101 00:00:00.000000   1 sync.go:180] cert-manager/controller/challenges: 
#   Challenge did not complete successfully: 
#   Timeout: Failed to fetch http://api.example.com/.well-known/acme-challenge/xxx
# ❌ ACME HTTP-01 挑战失败（防火墙/Ingress 配置问题）

# 6. 根因：
# - Let's Encrypt 续期需要验证域名所有权（HTTP-01 挑战）
# - 挑战路径 /.well-known/acme-challenge/* 被 Ingress 规则覆盖
# - cert-manager 无法完成自动续期
# - 证书到期后未触发告警
```

**应急措施**

```bash
# 方案 A：使用备用证书（5 分钟恢复）
# 从备份恢复上次有效证书（或申请临时证书）
kubectl create secret tls tls-secret-backup \
  --cert=/backup/tls.crt \
  --key=/backup/tls.key \
  --dry-run=client -o yaml | kubectl apply -f -

# 更新 Ingress 引用
kubectl patch ingress api-ingress -p '{
  "spec": {
    "tls": [{
      "hosts": ["api.example.com"],
      "secretName": "tls-secret-backup"
    }]
  }
}'

# Nginx 热更新（无需重启）
kubectl exec -n ingress-nginx <nginx-pod> -- nginx -s reload

# 验证恢复
curl -v https://api.example.com 2>&1 | grep "SSL certificate verify ok"
# * SSL certificate verify ok.
# ✅ 证书恢复

# 方案 B：修复 cert-manager 并重新申请证书
# 1. 修复 Ingress 配置（允许 ACME 挑战）
kubectl annotate ingress api-ingress \
  cert-manager.io/cluster-issuer=letsencrypt-prod

# 2. 配置 HTTP-01 挑战路径（优先级最高）
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: acme-challenge-ingress
  namespace: default
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      # ACME 挑战路径（优先匹配）
      - path: /.well-known/acme-challenge
        pathType: Prefix
        backend:
          service:
            name: cm-acme-http-solver-xxxxx  # cert-manager 自动创建
            port:
              number: 8089
EOF

# 3. 手动触发证书续期
kubectl delete secret tls-secret  # 删除旧证书
kubectl annotate certificate tls-cert cert-manager.io/issue-temporary-certificate="true" --overwrite

# 4. 查看续期进度
kubectl get certificate tls-cert -w
# NAME       READY   SECRET       AGE
# tls-cert   False   tls-secret   1m  # 申请中
# tls-cert   True    tls-secret   2m  # ✅ 续期成功

# 5. 验证新证书
kubectl get secret tls-secret -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
# notAfter=Mar 31 23:59:59 2024 GMT  # ← 新证书有效期 90 天
# ✅ 证书更新成功
```

**长期优化**

```yaml
# 1. 配置证书自动续期（cert-manager）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: ops@example.com
    privateKeySecretRef:
      name: letsencrypt-prod
    solvers:
    # HTTP-01 挑战（推荐）
    - http01:
        ingress:
          class: nginx
    # DNS-01 挑战（适用于通配符证书）
    - dns01:
        cloudflare:
          apiTokenSecretRef:
            name: cloudflare-api-token
            key: api-token
---
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: tls-cert
  namespace: default
spec:
  secretName: tls-secret
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - api.example.com
  - www.example.com
  # 自动续期配置
  renewBefore: 720h  # 提前 30 天续期
  # 私钥配置
  privateKey:
    algorithm: RSA
    size: 2048
    rotationPolicy: Always  # 续期时轮换私钥

# 2. 监控证书有效期
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-expiry-alert
  namespace: default
spec:
  groups:
  - name: certificates
    interval: 1h
    rules:
    # 证书即将过期（<30 天）
    - alert: CertificateExpiringSoon
      expr: |
        certmanager_certificate_expiration_timestamp_seconds - time() < 30*24*3600
      labels:
        severity: warning
      annotations:
        summary: "证书 {{ \$labels.name }} 将在 {{ \$value | humanizeDuration }} 后过期"
        description: "立即检查 cert-manager 续期状态"
    
    # 证书已过期
    - alert: CertificateExpired
      expr: |
        certmanager_certificate_expiration_timestamp_seconds - time() < 0
      labels:
        severity: critical
      annotations:
        summary: "证书 {{ \$labels.name }} 已过期"
        description: "紧急更新证书！"
    
    # 证书续期失败
    - alert: CertificateRenewalFailed
      expr: |
        certmanager_certificate_ready_status{condition="False"} == 1
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "证书 {{ \$labels.name }} 续期失败"
EOF

# 3. 配置 Ingress 注解（自动关联证书）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  annotations:
    # 自动申请证书
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    # ACME 挑战类型
    cert-manager.io/acme-challenge-type: http01
    # 证书续期窗口
    cert-manager.io/renew-before-expiry-duration: "720h"
spec:
  tls:
  - hosts:
    - api.example.com
    secretName: tls-secret  # cert-manager 自动管理
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80

# 4. 定期证书健康检查（CronJob）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cert-health-check
  namespace: default
spec:
  schedule: "0 */6 * * *"  # 每 6 小时检查
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: checker
            image: curlimages/curl:latest
            command:
            - /bin/sh
            - -c
            - |
              for domain in api.example.com www.example.com; do
                echo "Checking $domain..."
                expiry=$(echo | openssl s_client -servername $domain -connect $domain:443 2>/dev/null | \
                         openssl x509 -noout -enddate | cut -d= -f2)
                echo "$domain expires: $expiry"
                
                # 检查是否<15天
                exp_epoch=$(date -d "$expiry" +%s)
                now_epoch=$(date +%s)
                days_left=$(( ($exp_epoch - $now_epoch) / 86400 ))
                
                if [ $days_left -lt 15 ]; then
                  echo "⚠️  WARNING: $domain expires in $days_left days!"
                  # 发送告警（Slack/Email）
                fi
              done
          restartPolicy: OnFailure
```

**事后复盘**

| 维度 | 问题 | 改进措施 |
|-----|------|---------|
| **监控** | 无证书过期监控 | 新增 Prometheus 告警规则（<30 天预警） |
| **自动化** | cert-manager 续期失败未发现 | 监控 `certmanager_certificate_ready_status` |
| **配置** | Ingress 规则覆盖 ACME 挑战路径 | 调整路径优先级，确保 `.well-known` 可达 |
| **备份** | 无证书备份机制 | 每周自动备份证书到外部存储 |
| **演练** | 未进行证书过期演练 | 每季度故障注入测试 |

**故障时间线**

```
00:00:00 - 证书过期，用户开始报错
00:05:00 - 监控告警触发（用户反馈）
00:08:00 - 运维介入，确认证书过期
00:15:00 - 部署备用证书，HTTPS 服务恢复
00:30:00 - 修复 cert-manager 配置
01:00:00 - 新证书申请成功
总中断时间：15 分钟（应急措施）
```

---

### 案例 3：externalTrafficPolicy: Local 导致负载不均与间歇性超时

**故障现场**

- **现象**：LoadBalancer Service 后端 Pod 负载极度不均（部分 Pod CPU 90%，部分 Pod 空闲）
- **影响范围**：Web 服务（100 个 Pod）
- **业务影响**：
  - 30% 请求延迟 >5s（超时）
  - 部分用户报 504 Gateway Timeout
  - 热点 Pod OOMKilled 导致服务抖动

**排查过程**

```bash
# 1. 检查 Pod 负载分布
kubectl top pods -l app=web | sort -k3 -rn | head -10
# NAME              CPU     MEMORY
# web-node1-abc12   2800m   1800Mi  # ← Node1 上的 Pod 负载极高
# web-node1-def34   2650m   1750Mi
# web-node2-ghi56   50m     200Mi   # ← Node2 上的 Pod 几乎空闲
# web-node3-jkl78   45m     195Mi

# 2. 检查 Service 配置
kubectl get svc web -o yaml
# spec:
#   type: LoadBalancer
#   externalTrafficPolicy: Local  # ← 关键配置
#   selector:
#     app: web

# 3. 分析流量分布
# 云 LB 配置了 3 个后端节点（Node1/Node2/Node3）
# 云 LB 健康检查通过的节点：Node1（10 Pod）、Node2（5 Pod）、Node3（0 Pod，被驱逐）

# 4. 查看节点 Pod 分布
kubectl get pods -l app=web -o wide | awk '{print $7}' | sort | uniq -c
#  60 node-1  # ← Node1 有 60 个 Pod
#  30 node-2  # ← Node2 有 30 个 Pod
#  10 node-3  # ← Node3 有 10 个 Pod

# 5. 检查云 LB 后端健康状态
# 云控制台查看：
# - Node1: Healthy (60 Pod)
# - Node2: Healthy (30 Pod)
# - Node3: Unhealthy (10 Pod)  # ← 健康检查失败

# 6. 分析根因：
# - externalTrafficPolicy: Local 导致流量仅路由到本地 Pod
# - 云 LB 按节点级负载均衡（不感知 Pod 数量）
# - Node1/Node2/Node3 按 1:1:1 接收流量
# - 但实际 Pod 分布 60:30:10，导致 Node1 上 Pod 过载
# - Node3 健康检查失败后，流量全部集中到 Node1/Node2（60:30），负载更不均
```

**应急措施**

```bash
# 方案 A：切换回 Cluster 模式（立即生效）
kubectl patch svc web -p '{"spec":{"externalTrafficPolicy":"Cluster"}}'

# 验证流量分布
kubectl top pods -l app=web | sort -k3 -rn | head -10
# NAME              CPU     MEMORY
# web-node1-abc12   800m    600Mi  # ← 负载恢复均衡
# web-node2-def34   750m    580Mi
# web-node3-ghi56   820m    610Mi
# ✅ 各 Pod 负载趋于均衡

# 方案 B：调整 Pod 拓扑分布（长期方案）
kubectl patch deployment web -p '{
  "spec": {
    "template": {
      "spec": {
        "topologySpreadConstraints": [{
          "maxSkew": 1,
          "topologyKey": "kubernetes.io/hostname",
          "whenUnsatisfiable": "DoNotSchedule",
          "labelSelector": {
            "matchLabels": {"app": "web"}
          }
        }]
      }
    }
  }
}'

# 触发 Pod 重新调度（逐步迁移）
kubectl rollout restart deployment web

# 等待调度完成后，恢复 Local 模式
kubectl patch svc web -p '{"spec":{"externalTrafficPolicy":"Local"}}'

# 验证 Pod 均匀分布
kubectl get pods -l app=web -o wide | awk '{print $7}' | sort | uniq -c
#  33 node-1  # ← 接近均匀
#  33 node-2
#  34 node-3
```

**长期优化**

```yaml
# 1. 使用拓扑感知提示（Topology Aware Hints）
# Kubernetes 1.21+ 特性，自动优化 Local 模式负载分布
apiVersion: v1
kind: Service
metadata:
  name: web
  annotations:
    service.kubernetes.io/topology-aware-hints: "auto"  # ← 启用拓扑提示
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
---
# 2. 强制 Pod 均匀分布
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
spec:
  replicas: 100
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      # 拓扑分布约束
      topologySpreadConstraints:
      # 按节点均匀分布
      - maxSkew: 2  # 允许最大偏差 2 个 Pod
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway  # 软约束（尽力而为）
        labelSelector:
          matchLabels:
            app: web
      # 按可用区均匀分布
      - maxSkew: 5
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule  # 硬约束
        labelSelector:
          matchLabels:
            app: web
      containers:
      - name: web
        image: nginx:1.21
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 2000m
            memory: 2Gi

# 3. 配置 HPA 自动扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 50
  maxReplicas: 200
  metrics:
  # CPU 目标 60%
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60
  # 自定义指标：每 Pod QPS
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"  # 每 Pod 1000 QPS
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50  # 每次扩容 50%
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 5  # 每次缩容 5 个 Pod
        periodSeconds: 60

# 4. 监控负载分布
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: service-load-balance-alert
  namespace: default
spec:
  groups:
  - name: service-lb
    interval: 30s
    rules:
    # Pod 负载不均（标准差过大）
    - alert: PodLoadImbalance
      expr: |
        stddev(rate(container_cpu_usage_seconds_total{pod=~"web-.*"}[5m])) 
        / avg(rate(container_cpu_usage_seconds_total{pod=~"web-.*"}[5m])) > 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Pod 负载分布不均（变异系数 >0.5）"
        description: "检查 externalTrafficPolicy 和拓扑分布"
    
    # 节点级流量不均
    - alert: NodeTrafficImbalance
      expr: |
        max(rate(node_network_receive_bytes_total{device="eth0"}[5m])) 
        / min(rate(node_network_receive_bytes_total{device="eth0"}[5m])) > 3
      for: 10m
      annotations:
        summary: "节点流量分布不均（最大/最小 >3 倍）"
EOF
```

**externalTrafficPolicy 对比**

| 模式 | 流量路由 | 源 IP | 负载均衡 | 跨节点跳转 | 适用场景 |
|-----|---------|-------|---------|-----------|---------|
| **Cluster**（默认） | 任意节点→任意 Pod | SNAT（丢失源 IP） | 均衡（Pod 级） | 是（额外跳转） | 通用场景 |
| **Local** | 本节点→本节点 Pod | 保留真实源 IP | 不均（节点级） | 否 | 需要源 IP 的场景 |

**效果评估**

| 指标 | 优化前（Local+不均分布） | 优化后（Local+均匀分布） | 改善 |
|-----|------------------------|------------------------|------|
| Pod CPU 标准差 | 45%（严重不均） | 8%（基本均衡） | ↓ 82% |
| P99 延迟 | 5200ms | 180ms | ↓ 97% |
| 超时错误率 | 30% | <0.1% | ↓ 99.7% |
| 保留源 IP | ✅ | ✅ | - |
