---
title: 05 - 网络迁移与流量切换 [migration]
description: 'title: 05 - 网络迁移与流量切换'
summary: 'title: 05 - 网络迁移与流量切换'
category: general
tags:
- migration
- upgrade
- networking
- grafana
- cilium
- flannel
- calico
- helm
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 网络迁移与流量切换 是什么
- 如何 网络迁移与流量切换
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 网络迁移与流量切换
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- monitoring-basics
- cilium-basics
- cni-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 05 - 网络迁移与流量切换
description: '# 05 - 网络迁移与流量切换'
category: migration
tags:
- k8s
- migration
- modernization
- grafana
- [[Cilium|cilium]]
- flannel
- calico
- [[Helm|helm]]
- [[Ingress|ingress]]
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 网络迁移与流量切换 是什么
- 如何 网络迁移与流量切换
trigger_keywords:
- 网络迁移与流量切换
- migration
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 05 - 网络迁移与流量切换

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: CNI, Terway, Ingress, DNS, SLB, NLB, ALB, 灰度切流, NetworkPolicy

---

<!-- chunk: 目录 -->## 目录

1. [CNI 差异与适配](#1-cni-差异与适配)
2. [Service 与负载均衡迁移](#2-service-与负载均衡迁移)
3. [Ingress 迁移](#3-ingress-迁移)
4. [DNS 灰度切流](#4-dns-灰度切流)
5. [NetworkPolicy 迁移](#5-networkpolicy-迁移)
6. [网络连通性验证](#6-网络连通性验证)
7. [流量回滚方案](#7-流量回滚方案)

---

<!-- chunk: 1. CNI 差异与适配 -->## 1. CNI 差异与适配

## 1.1 CNI 对比

| 维度 | Calico (自建常用) | Flannel (自建常用) | Terway (ACK 推荐) |
|------|-----------------|------------------|-------------------|
| **网络模式** | BGP / IPIP / VXLAN | VXLAN 覆盖网络 | ENI 多 IP / IPVLAN |
| **性能** | IPIP 有封装开销 | VXLAN 有封装开销 | 接近裸金属（无封装） |
| **NetworkPolicy** | 支持（Calico NP + K8s NP） | 不支持 | 支持（K8s NetworkPolicy） |
| **Pod IP 分配** | IPAM 从 IP Pool 分配 | 从 Pod CIDR 分配 | 从 vSwitch 分配弹性网卡 IP |
| **Pod IP 可路由** | 仅集群内 | 仅集群内 | VPC 内直接可路由 |
| **对迁移的影响** | 需验证 NetworkPolicy 兼容 | 迁移最简单 | 需额外 Pod vSwitch |

## 1.2 迁移注意事项

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认自建集群 CNI 类型
kubectl --context=source-cluster get pods -n kube-system | grep -iE "(calico|flannel|cilium|weave)"

# 2. 如果自建使用 Calico，检查 NetworkPolicy
kubectl --context=source-cluster get networkpolicies -A
# Calico 特有的 GlobalNetworkPolicy 不直接兼容 ACK
kubectl --context=source-cluster get globalnetworkpolicies 2>/dev/null

# 3. 如果自建使用 Calico BGP，确认是否有 BGP Peer 配置
kubectl --context=source-cluster get bgppeers 2>/dev/null
# BGP 特有配置不需要迁移到 ACK Terway

# 4. Terway 模式下 Pod IP 在 VPC 内可路由
# 这意味着在迁移期间，如果通过 VPN/CEN 互联
# ACK Pod 可以直接访问自建集群的 Service ClusterIP（需路由打通）
```
---

<!-- chunk: 2. Service 与负载均衡迁移 -->## 2. Service 与负载均衡迁移

## 2.1 负载均衡方案对比

| 自建方案 | ACK 替代方案 | 适用场景 | 注解前缀 |
|---------|------------|---------|---------|
| MetalLB | SLB (CLB) | 四层 TCP/UDP | `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-*` |
| kube-vip | SLB (CLB) | 四层 HA VIP | 同上 |
| 外部 Nginx/HAProxy | NLB (网络型) | 四层高性能 | `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-*` |
| 无（NodePort 直连） | SLB / NLB | 统一入口 | 同上 |

## 2.2 SLB/NLB/ALB 选型

| 类型 | 协议 | 性能 | 适用场景 | 成本 |
|------|------|------|---------|------|
| **CLB (传统型)** | TCP/UDP/HTTP/HTTPS | 中 | 通用四层/七层 | 低 |
| **NLB (网络型)** | TCP/UDP/TLS | 极高 (千万级并发) | 高性能四层 | 中 |
| **ALB (应用型)** | HTTP/HTTPS/gRPC/WebSocket | 高 | 七层智能路由 | 中 |

## 2.3 Service 迁移示例

```yaml
# --- 内部服务: ClusterIP（无需改动） ---
apiVersion: v1
kind: Service
metadata:
  name: backend-api
spec:
  type: ClusterIP            # 直接迁移，无需改动
  selector:
    app: backend
  ports:
  - port: 8080
    targetPort: 8080

---
# --- 外部 TCP 服务: 使用 NLB ---
apiVersion: v1
kind: Service
metadata:
  name: tcp-gateway
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    # 使用 NLB
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-class: "nlb"
    # NLB 规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-zone-maps: '[{"zoneId":"cn-hangzhou-h","vSwitchId":"<vsw-a>"},{"zoneId":"cn-hangzhou-i","vSwitchId":"<vsw-b>"}]'
spec:
  type: LoadBalancer
  selector:
    app: tcp-gateway
  ports:
  - port: 9090
    targetPort: 9090
    protocol: TCP

---
# --- HTTP 服务: 使用 CLB ---
apiVersion: v1
kind: Service
metadata:
  name: web-frontend
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    # HTTPS 卸载
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "<ssl-cert-id>"
    # 健康检查
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
spec:
  type: LoadBalancer
  selector:
    app: web-frontend
  ports:
  - port: 443
    targetPort: 8080
```

---

<!-- chunk: 3. Ingress 迁移 -->## 3. Ingress 迁移

## 3.1 Ingress Controller 选型

| 自建方案 | ACK 推荐 | 迁移难度 | 说明 |
|---------|---------|---------|------|
| nginx-ingress | nginx-ingress (ACK Addon) | 低 | 注解高度兼容 |
| nginx-ingress | ALB Ingress | 中 | 需改注解，但功能更强 |
| Traefik | nginx-ingress / ALB | 中 | 需改 IngressRoute → Ingress |
| Higress | Higress (ACK MSE) | 低 | 直接使用 MSE 托管版 |
| 自建 HAProxy | ALB Ingress / nginx-ingress | 中 | 需改配置方式 |

## 3.2 nginx-ingress 迁移（最常见）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 自建集群和 ACK 都使用 nginx-ingress 时，Ingress 资源几乎无需改动

# 确认 ACK nginx-ingress 已安装
kubectl --context=ack-cluster get pods -n kube-system -l app.kubernetes.io/name=ingress-nginx
kubectl --context=ack-cluster get svc -n kube-system -l app.kubernetes.io/name=ingress-nginx

# 获取 ACK Ingress Controller 外部 IP
export ACK_INGRESS_IP=$(kubectl --context=ack-cluster get svc -n kube-system \
  -l app.kubernetes.io/name=ingress-nginx \
  -o jsonpath='{.items[0].status.loadBalancer.ingress[0].ip}')
echo "ACK Ingress IP: $ACK_INGRESS_IP"

# 测试 Ingress 路由（通过 Host 头直接测试）
curl -H "Host: api.example.com" http://$ACK_INGRESS_IP/health
# 预期: 返回应用健康检查响应
```
## 3.3 TLS 证书迁移

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 导出自建集群的 TLS Secret
kubectl --context=source-cluster get secret tls-cert -n production -o yaml | kubectl neat > tls-secret.yaml

# 应用到 ACK（或使用阿里云 SSL 证书服务）
kubectl --context=ack-cluster apply -f tls-secret.yaml

# 推荐: 在 ACK 使用 cert-manager 自动管理证书
helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager --create-namespace \
  --set installCRDs=true

# 创建 Let's Encrypt ClusterIssuer
kubectl --context=ack-cluster apply -f - <<EOF
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod
    solvers:
    - http01:
        ingress:
          class: nginx
EOF
```
---

<!-- chunk: 4. DNS 灰度切流 -->## 4. DNS 灰度切流

## 4.1 切流策略

```
灰度切流时间线（推荐）:

Day 0: 准备
  ├── 确认所有服务在 ACK 健康运行
  ├── 确认监控告警已配置
  └── 记录源集群当前 DNS 配置

Day 1: 10% 切流
  ├── 设置 DNS 权重: 源集群 90% / ACK 10%
  ├── 观察 24h
  └── 检查: 错误率、RT、日志

Day 3: 30% 切流
  ├── 调整 DNS 权重: 源集群 70% / ACK 30%
  ├── 观察 24h
  └── 检查: 高并发接口、边缘 case

Day 5: 50% 切流
  ├── 调整 DNS 权重: 源集群 50% / ACK 50%
  ├── 观察 24h（覆盖一次高峰期）
  └── 检查: 资源水位、自动扩缩

Day 7: 100% 切流
  ├── DNS 全部指向 ACK
  ├── 源集群保留运行（不接流量）
  └── 稳定观察 7 天

Day 14: 退役源集群
  └── 参考 08-validation-cutover-decommission.md
```

## 4.2 阿里云 DNS 权重配置

```bash
# 使用阿里云 DNS（云解析）进行权重路由

# 1. 查看当前 DNS 记录
aliyun alidns DescribeDomainRecords --DomainName example.com \
  --output cols=RecordId,RR,Type,Value,Weight --rows DomainRecords.Record[]

# 2. 添加 ACK Ingress IP 的 DNS 记录（权重 10%）
aliyun alidns AddDomainRecord \
  --DomainName example.com \
  --RR api \
  --Type A \
  --Value $ACK_INGRESS_IP \
  --Weight 1

# 3. 修改源集群记录权重为 90%
aliyun alidns UpdateDomainRecord \
  --RecordId <source-record-id> \
  --RR api \
  --Type A \
  --Value <source-ingress-ip> \
  --Weight 9

# 4. 逐步调整权重
# 30% ACK:
aliyun alidns SetDomainRecordWeight --RecordId <ack-record-id> --Weight 3
aliyun alidns SetDomainRecordWeight --RecordId <source-record-id> --Weight 7

# 50% ACK:
aliyun alidns SetDomainRecordWeight --RecordId <ack-record-id> --Weight 5
aliyun alidns SetDomainRecordWeight --RecordId <source-record-id> --Weight 5

# 100% ACK:
aliyun alidns SetDomainRecordWeight --RecordId <ack-record-id> --Weight 10
aliyun alidns SetDomainRecordWeight --RecordId <source-record-id> --Weight 0
# 或直接删除源记录
# aliyun alidns DeleteDomainRecord --RecordId <source-record-id>
```

## 4.3 基于 GSLB/GTM 的智能切流

```bash
# 阿里云 GTM (全局流量管理) — 更精细的流量控制

# 1. 创建 GTM 实例
# 通过控制台: DNS → 全局流量管理 → 创建实例

# 2. 配置地址池
#    地址池 A: 源集群 Ingress IP
#    地址池 B: ACK Ingress IP

# 3. 配置访问策略
#    默认策略: 地址池 A (源集群)
#    灰度策略: 地址池 B (ACK) — 按比例/按地域/按运营商

# 4. 配置健康检查
#    检查 URL: /health
#    检查间隔: 10s
#    失败阈值: 3 次

# GTM 优势:
# - 支持按比例/地域/运营商精细切流
# - 自动故障切换（健康检查失败自动摘除）
# - 实时流量统计和监控
```

## 4.4 切流监控看板

```bash
# 在 Grafana 创建切流监控 Dashboard，核心指标:

# 1. 双集群请求量对比
# PromQL (源集群):
# sum(rate(nginx_ingress_controller_requests[5m])) by (host)
# PromQL (ACK):
# sum(rate(nginx_ingress_controller_requests[5m])) by (host)

# 2. 双集群错误率对比
# sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) /
# sum(rate(nginx_ingress_controller_requests[5m]))

# 3. 双集群 RT 对比 (P99)
# histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le))

# 4. ACK 资源水位
# sum(node_namespace_pod_container:container_cpu_usage_seconds_total:sum_irate) /
# sum(kube_pod_container_resource_requests{resource="cpu"})
```

---

<!-- chunk: 5. NetworkPolicy 迁移 -->## 5. NetworkPolicy 迁移

## 5.1 Calico NetworkPolicy → K8s NetworkPolicy

```yaml
# Calico 特有的 GlobalNetworkPolicy（不兼容 ACK）
# apiVersion: projectcalico.org/v3
# kind: GlobalNetworkPolicy
# 需转换为标准 K8s NetworkPolicy

# 转换前（Calico）:
# apiVersion: projectcalico.org/v3
# kind: NetworkPolicy
# metadata:
#   name: allow-frontend
#   namespace: production
# spec:
#   selector: app == 'backend'
#   ingress:
#   - action: Allow
#     source:
#       selector: app == 'frontend'
#   egress:
#   - action: Allow

# 转换后（标准 K8s NetworkPolicy，ACK Terway 支持）:
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
  egress:
  - {}                        # 允许所有出站
```

## 5.2 批量转换脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出所有 Calico NetworkPolicy 并转换
kubectl --context=source-cluster get networkpolicies -A -o yaml > source-netpol.yaml

# 标准 K8s NetworkPolicy 直接迁移
kubectl --context=ack-cluster apply -f source-netpol.yaml

# Calico 特有的 GlobalNetworkPolicy 需手动转换
kubectl --context=source-cluster get globalnetworkpolicies -o yaml 2>/dev/null > calico-gnp.yaml
# 需要逐条手动转换为 K8s NetworkPolicy（每个 Namespace 一份）
```
---

<!-- chunk: 6. 网络连通性验证 -->## 6. 网络连通性验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# verify-network.sh
# ACK 网络全面验证

ACK_CONTEXT="ack-cluster"

echo "=== 1. Pod 间网络连通 ==="
# 创建测试 Pod（不同节点）
kubectl --context=$ACK_CONTEXT run net-test-1 --image=busybox:1.36 --restart=Never -- sleep 3600
kubectl --context=$ACK_CONTEXT run net-test-2 --image=busybox:1.36 --restart=Never -- sleep 3600

# 等待就绪
kubectl --context=$ACK_CONTEXT wait --for=condition=Ready pod/net-test-1 pod/net-test-2 --timeout=60s

POD2_IP=$(kubectl --context=$ACK_CONTEXT get pod net-test-2 -o jsonpath='{.status.podIP}')
kubectl --context=$ACK_CONTEXT exec net-test-1 -- ping -c 3 $POD2_IP
echo "Pod 间连通: OK"

echo "=== 2. Service 解析 ==="
kubectl --context=$ACK_CONTEXT exec net-test-1 -- nslookup kubernetes.default.svc.cluster.local
echo "DNS 解析: OK"

echo "=== 3. 外网访问 ==="
kubectl --context=$ACK_CONTEXT exec net-test-1 -- wget -qO- --timeout=5 http://www.aliyun.com > /dev/null 2>&1 && echo "外网访问: OK" || echo "外网访问: FAIL"

echo "=== 4. Ingress 外部访问 ==="
INGRESS_IP=$(kubectl --context=$ACK_CONTEXT get svc -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].status.loadBalancer.ingress[0].ip}')
curl -s -o /dev/null -w "%{http_code}" http://$INGRESS_IP/ && echo " Ingress 响应: OK"

echo "=== 5. 与源集群互通（如已配置 VPN/CEN）==="
kubectl --context=$ACK_CONTEXT exec net-test-1 -- ping -c 3 <source-cluster-pod-cidr-test-ip> 2>/dev/null && echo "跨集群: OK" || echo "跨集群: N/A 或 FAIL"

# 清理
kubectl --context=$ACK_CONTEXT delete pod net-test-1 net-test-2
```
---

<!-- chunk: 7. 流量回滚方案 -->## 7. 流量回滚方案

## 7.1 紧急回滚 SOP

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# emergency-rollback.sh
# 紧急将流量切回源集群

echo "!!! 紧急流量回滚 !!!"
echo "时间: $(date '+%Y-%m-%d %H:%M:%S')"

# Step 1: DNS 全量切回源集群
echo ">>> Step 1: DNS 回切"
aliyun alidns UpdateDomainRecord \
  --RecordId <source-record-id> \
  --RR api \
  --Type A \
  --Value <source-ingress-ip> \
  --Weight 10

aliyun alidns UpdateDomainRecord \
  --RecordId <ack-record-id> \
  --RR api \
  --Type A \
  --Value $ACK_INGRESS_IP \
  --Weight 0

echo "DNS 已切回源集群（生效时间取决于 TTL，建议 TTL 设为 60s）"

# Step 2: 验证源集群健康
echo ">>> Step 2: 验证源集群"
kubectl --context=source-cluster get pods -A | grep -v Running | grep -v Completed | grep -v kube-system

# Step 3: 确认流量恢复
echo ">>> Step 3: 等待 DNS 生效后确认"
echo "请在 60s 后检查源集群 Ingress 访问日志"
echo "kubectl --context=source-cluster logs -n ingress-nginx deploy/ingress-nginx-controller --tail=10"

echo ""
echo "回滚执行完成，请持续观察源集群状态"
```
## 7.2 DNS TTL 建议

| 阶段 | TTL 设置 | 说明 |
|------|---------|------|
| 迁移准备期 | 降低到 60s | 切流前 24h 调低，确保快速生效 |
| 灰度切流期 | 保持 60s | 确保回滚可在 1-2min 内生效 |
| 100% 稳定后 7 天 | 保持 60s | 保留快速回滚能力 |
| 源集群退役后 | 恢复 600s | 正常 TTL |

---

<!-- chunk: 检查清单 -->## 检查清单

- [ ] CNI 差异已分析，NetworkPolicy 已转换
- [ ] 所有 Service 类型已适配（NodePort → LoadBalancer）
- [ ] SLB/NLB 已分配外部 IP
- [ ] Ingress 已迁移，通过 ACK Ingress IP 可访问
- [ ] TLS 证书已迁移或由 cert-manager 管理
- [ ] DNS TTL 已降低到 60s
- [ ] DNS 灰度权重已配置
- [ ] 网络连通性验证通过
- [ ] 切流监控看板已就绪
- [ ] 紧急回滚脚本已准备并测试

---

**上一步**: ← [04-存储与数据迁移](./04-storage-data-migration.md)
**下一步**: → [06-有状态服务迁移](./06-stateful-services-migration.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration MOC
- [[发布变更/迁移方案/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[发布变更/迁移方案/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[发布变更/迁移方案/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[发布变更/迁移方案/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[发布变更/迁移方案/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[发布变更/迁移方案/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- [[发布变更/迁移方案/07-observability-security-migration.md|07 - 可观测性与安全迁移]]
- [[发布变更/迁移方案/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]]
- [[发布变更/迁移方案/09-migration-toolchain.md|09 - 迁移工具链参考]]
- [[发布变更/迁移方案/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 03-application-workload-migration
- 04-storage-data-migration
- 06-stateful-services-migration
- 07-observability-security-migration

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
