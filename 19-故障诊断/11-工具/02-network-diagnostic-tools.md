---
title: 网络诊断工具实战指南
description: 面向阿里云/专有云 K8s 的网络诊断工具指南，涵盖 ping、netshoot、ksniff、cilium-cli 的使用与典型网络故障排查场景。
summary: 面向阿里云/专有云 K8s 的网络诊断工具指南，涵盖 ping、netshoot、ksniff、cilium-cli 的使用与典型网络故障排查场景。
category: troubleshooting
tags:
- k8s
- network
- diagnostics
- netshoot
- ksniff
- cilium-cli
- ping
- dns
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- K8s 网络诊断工具
- netshoot ksniff 使用
- 阿里云 K8s 网络故障排查
trigger_keywords:
- 网络诊断
- netshoot
- ksniff
- cilium-cli
- ping
- dns
prerequisites:
- kubectl-basics
- network-basics
- linux-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络诊断工具实战指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解常用网络诊断工具的使用方法与典型故障排查场景。

## 目录

1. [网络故障排查思路](#网络故障排查思路)
2. [ping / traceroute](#ping--traceroute)
3. [netshoot 全能诊断容器](#netshoot-全能诊断容器)
4. [ksniff 抓包](#ksniff-抓包)
5. [cilium-cli：Cilium 网络诊断](#cilium-clicilium-网络诊断)
6. [DNS 诊断](#dns-诊断)
7. [典型场景排查](#典型场景排查)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 网络故障排查思路

### 1.1 分层排查法

```
应用层 → Service → Endpoint → Pod → CNI → 节点网络 → 云网络
```

### 1.2 常用检查命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Service 与 Endpoint
kubectl get svc -n production
kubectl get endpoints -n production

# 查看 Pod IP 与状态
kubectl get pod -n production -o wide

# 查看 NetworkPolicy
kubectl get networkpolicies -n production
```
---

## 2. ping / traceroute

### 2.1 Pod 内 ping 测试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启动临时 Pod 测试网络连通性
kubectl run ping-test --rm -it --image=busybox --restart=Never -- \
  ping -c 4 <target-ip>
```
### 2.2 traceroute 路径追踪

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run trace-test --rm -it --image=nicolaka/netshoot --restart=Never -- \
  traceroute <target-ip>
```
---

## 3. netshoot 全能诊断容器

### 3.1 进入 netshoot 容器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 以 netshoot 镜像启动调试 Pod
kubectl run netshoot --rm -it --image=nicolaka/netshoot --restart=Never -- bash
```
### 3.2 netshoot 内置工具

| 工具 | 用途 |
|:---|:---|
| tcpdump | 抓包 |
| tshark | 协议分析 |
| nmap | 端口扫描 |
| ss | socket 统计 |
| dig / nslookup | DNS 查询 |
| curl / wget | HTTP 测试 |
| iperf | 带宽测试 |
| mtr | 综合路由测试 |

### 3.3 测试 Service 连通性

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在 netshoot 中测试 Service
kubectl run netshoot --rm -it --image=nicolaka/netshoot --restart=Never -- \
  curl -v http://order-service.production.svc.cluster.local:8080/health
```
---

## 4. ksniff 抓包

### 4.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install sniff
```
### 4.2 抓取 Pod 流量

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 抓取指定 Pod 的所有流量
kubectl sniff <pod-name> -n <namespace>

# 抓取指定容器与端口的流量
kubectl sniff <pod-name> -n <namespace> -c <container> -p -o /tmp/capture.pcap
```
### 4.3 分析抓包文件

```bash
# 本地使用 Wireshark 分析
tshark -r /tmp/capture.pcap -Y "http"
```

---

## 5. cilium-cli：Cilium 网络诊断

### 5.1 安装 cilium-cli

```bash
curl -L --remote-name-all https://github.com/cilium/cilium-cli/releases/latest/download/cilium-linux-amd64.tar.gz
tar xzvf cilium-linux-amd64.tar.gz
sudo mv cilium /usr/local/bin/
```

### 5.2 查看 Cilium 状态

```bash
# 查看 Cilium 组件状态
cilium status

# 查看 Cilium Pod
cilium status --all-controllers
```

### 5.3 连通性测试

```bash
# Cilium 内置连通性测试
cilium connectivity test

# 查看节点间连通性
cilium-health status
```

### 5.4 查看 BPF 映射

```bash
# 查看 endpoint 列表
cilium endpoint list

# 查看指定 endpoint 的策略
cilium endpoint get <endpoint-id>
```

---

## 6. DNS 诊断

### 6.1 CoreDNS 状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CoreDNS Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
```
### 6.2 dig 测试 DNS

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试集群内部 DNS
kubectl run dig-test --rm -it --image=nicolaka/netshoot --restart=Never -- \
  dig order-service.production.svc.cluster.local

# 测试外部 DNS
kubectl run dig-test --rm -it --image=nicolaka/netshoot --restart=Never -- \
  dig @<core-dns-ip> example.com
```
---

## 7. 典型场景排查

### 7.1 Service 无法访问

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Service 与 Endpoint
kubectl get svc, endpoints -n production

# 2. 检查 Pod 是否 Ready
kubectl get pod -n production -l app=order-service

# 3. 检查 NetworkPolicy
kubectl get networkpolicies -n production

# 4. 使用 netshoot 测试
kubectl run netshoot --rm -it --image=nicolaka/netshoot --restart=Never -n production -- \
  curl -v http://order-service:8080/health

# 5. 抓包分析
kubectl sniff order-service-xxx -n production -o /tmp/service.pcap
```
### 7.2 Pod 跨节点不通

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 CNI Pod
kubectl get pods -n kube-system -l k8s-app=calico-node

# 2. 检查节点路由
kubectl node-shell <node-name> -- ip route

# 3. 使用 cilium connectivity test
cilium connectivity test
```
---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| netshoot 可用 | 常用诊断镜像 | 镜像仓库 |
| ksniff 安装 | 抓包工具 | `kubectl krew list` |
| cilium-cli 安装 | Cilium 环境 | `cilium version` |
| DNS 监控 | CoreDNS 告警 | PrometheusRule |
| 网络策略审计 | 避免过度限制 | 定期检查 |

---

## Terway 与 Calico 排查差异

阿里云 ACK 常见 CNI 包括 Terway（独占 ENI/IPAM）与 Calico（BGP/VXLAN）。不同 CNI 的排查重点不同。

| 维度 | Terway | Calico |
|:---|:---|:---|
| IP 分配 | 依赖阿里云 VPC / ENI | 依赖 Calico IPAM |
| Pod IP | 与 VPC 同网段 | 独立 Calico 网段 |
| 网络策略 | 支持 NetworkPolicy | 全面支持 NetworkPolicy |
| 排查工具 | aliyun CLI 查 ENI | calicoctl / cilium-cli |

### 网络抓包合规

生产环境抓包涉及敏感流量，必须遵循：

1. 获得用户或业务方授权
2. 限定抓包时间窗口与目标范围
3. 对 pcap 文件加密存储并设定保留期限
4. 抓包操作记录到审计日志

### 常见问题速查

| 现象 | 优先工具 | 关键检查 |
|:---|:---|:---|
| DNS 解析慢 | netshoot | CoreDNS 副本数、缓存配置 |
| 跨节点不通 | cilium-cli / calicoctl | CNI 隧道、路由、iptables |
| Service 无响应 | netshoot | Endpoint、kube-proxy、IPVS |
| 外部访问丢包 | ksniff | MTU、NAT、安全组 |

## 网络诊断流程

面对网络问题，建议按以下顺序排查：

1. **Pod 内自测**：nslookup、ping、nc、curl。
2. **Service 层检查**：Endpoint、kube-proxy、IPVS/iptables。
3. **CNI 层检查**：Cilium/Calico 路由、NetworkPolicy。
4. **节点层检查**：路由表、iptables、安全组、ENI。
5. **抓包分析**：ksniff 或节点 tcpdump。

### 常用命令组合

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Service 后端
kubectl get endpoints <svc> -n production

# 检查 Pod 内 DNS
kubectl exec -it <pod> -n production -- nslookup kubernetes.default

# 检查节点路由
kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- ip route

# Pod 间连通性测试
kubectl run tmp --rm -i --tty --image=nicolaka/netshoot --restart=Never -- /bin/bash
```
## 典型工单场景与处理

**场景**：Pod 无法访问外部 HTTPS 服务。

处理步骤：
1. 在 Pod 内执行 curl -v 查看具体错误。
2. 检查 DNS 解析是否正常。
3. 检查 egress NetworkPolicy 是否放行。
4. 检查节点安全组与 NAT 网关配置。
5. 必要时使用 ksniff 抓包分析 TLS 握手。

## 网络诊断最佳实践

1. **从 Pod 内部开始**：先确认容器内 DNS、路由、连通性是否正常。
2. **逐步向外排查**：Pod → Service → 节点 → CNI → 外部网络。
3. **结合监控**：查看网络丢包、连接数、带宽指标。
4. **保留证据**：抓包文件命名规范并加密保存。
5. **避免长时间抓包**：防止占用过多磁盘与带宽。

### 常见网络错误码速查

| 错误 | 可能原因 | 排查方向 |
|:---|:---|:---|
| Connection refused | 目标端口未监听 | Service/Endpoint/Pod |
| Connection timed out | 网络不通或防火墙 | NetworkPolicy/安全组/路由 |
| DNS NXDOMAIN | DNS 解析失败 | CoreDNS/配置 |
| TLS handshake failed | 证书或 SNI 问题 | Ingress/Secret |
| 502 Bad Gateway | 后端无响应 | 上游 Pod/健康检查 |

### 阿里云 ACK 网络组件

- **Terway**：ENI 模式、IPAM、NetworkPolicy
- **Flannel**：VXLAN Overlay
- **Calico**：BGP/VXLAN，支持 eBPF 模式
- **kube-proxy**：iptables / IPVS 模式

## 网络问题排查决策树

```
Pod 无法访问外部
  │
  ├─ DNS 解析失败 → 检查 CoreDNS / resolv.conf
  │
  ├─ 路由不可达 → 检查节点路由表 / CNI
  │
  ├─ 被 NetworkPolicy 拦截 → 检查 policy 规则
  │
  ├─ 安全组 / NAT 配置问题 → 检查阿里云安全组
  │
  └─ 外部服务问题 → 使用 netshoot 在节点测试
```

### 阿里云 ACK 网络诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Terway 分配的 Pod IP
kubectl get pod <pod> -o jsonpath='{.status.podIP}'

# 查看节点 ENI 与 IP
kubectl exec -n kube-system <terway-pod> -- terway-cli show

# 检查安全组规则
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId <sg-id>
```
## 网络诊断实战：Service 不可达

以 `production/order-service` 无法访问为例：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Service 与 Endpoint
kubectl get svc order-service -n production
kubectl get endpoints order-service -n production

# 2. 进入同命名空间 Pod 测试
kubectl exec -it <pod> -n production -- nc -zv order-service 80

# 3. 使用 netshoot 深入排查
kubectl debug pod/<pod> -n production -it --image=nicolaka/netshoot -- /bin/bash
# 在 netshoot 中执行：nslookup order-service、curl -v order-service、ip route

# 4. 检查 NetworkPolicy
kubectl get networkpolicies -n production

# 5. 必要时抓包
kubectl sniff -n production <pod> -f "tcp port 80" -o /tmp/order-service.pcap
```
### 网络诊断工具选型

| 工具 | 适用场景 |
|:---|:---|
| ping | 基础连通性 |
| netshoot | 综合网络排查 |
| ksniff | Pod 级抓包 |
| cilium-cli | Cilium CNI 深度诊断 |
| calicoctl | Calico CNI 深度诊断 |

## 网络抓包文件管理

抓包文件可能包含敏感信息，需规范管理：

1. 命名规范：`namespace-pod-port-timestamp.pcap`
2. 存储位置：集中存储于加密的对象存储或取证服务器。
3. 保留期限：一般问题 7 天，安全事件按合规要求保留。
4. 访问权限：仅授权人员可下载与分析。

### 抓包后分析

```bash
# 使用 tshark 统计 Top 慢请求
tshark -r /tmp/order-service.pcap -q -z io,stat,1

# 使用 Wireshark 过滤特定 HTTP 请求
# 显示过滤器：http.request.method == "POST"
```

## Related

- [[19-故障诊断/11-工具/README.md|Domain-12 故障排查工具套件使用说明]]
- [[19-故障诊断/03-基础设施排障/01-network-connectivity-troubleshooting.md|网络连通性故障诊断]]

## See Also

- [[19-故障诊断/11-工具/01-kubectl-plugins-guide.md|kubectl 插件指南]]
- [[19-故障诊断/11-工具/03-ebpf-diagnostic-tools.md|eBPF 诊断工具]]


<!-- risk-assessed -->
