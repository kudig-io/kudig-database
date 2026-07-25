---
title: Kubernetes Network Troubleshooting Guide
description: K8s 网络故障排查 — DNS 解析、Service 连通性、NetworkPolicy 调试、跨节点通信、性能诊断
summary: 生产环境 Kubernetes 网络问题的系统化排查方法论与工具链
category: practice
tags:
- troubleshooting
- dns
- networkpolicy
- connectivity
- debugging
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: networking
---
# Kubernetes 网络故障排查指南

> 系统化排查 K8s 网络问题的方法论、工具链与实战案例。

## 排查方法论

```
问题报告 → 确定范围 → 分层排查 → 定位根因 → 修复验证
                │
    ┌───────────┼───────────┐
    ▼           ▼           ▼
 单 Pod      Service     跨集群
 内部通信    访问异常    通信失败
```

## 分层排查清单

### L1: Pod 内部网络

```bash
# 进入 Pod 网络命名空间
kubectl exec -it pod-name -- sh

# 检查网络接口
ip addr show
# 预期：eth0 有 Pod IP

# 检查路由
ip route
# 预期：default via <node-gateway>

# 检查 DNS 配置
cat /etc/resolv.conf
# 预期：nameserver 10.96.0.10 (CoreDNS ClusterIP)

# DNS 解析测试
nslookup kubernetes.default
nslookup my-service.my-namespace.svc.cluster.local

# 连通性测试
wget -qO- http://my-service:8080/health
curl -v http://my-service:8080/api
```

### L2: Service 层

```bash
# 检查 Service 是否存在
kubectl get svc my-service -n my-namespace -o yaml

# 检查 Endpoints（是否有后端 Pod）
kubectl get endpoints my-service -n my-namespace
# 如果 Endpoints 为空 → 检查 selector 是否匹配 Pod labels

# 检查 kube-proxy 规则（iptables 模式）
kubectl exec -n kube-system ds/kube-proxy -- iptables -t nat -L -n | grep my-service

# 检查 IPVS 规则
kubectl exec -n kube-system ds/kube-proxy -- ipvsadm -Ln | grep -A5 <cluster-ip>

# 从其他 Pod 测试 ClusterIP
kubectl run test --rm -it --image=nicolaka/netshoot -- bash
curl http://<cluster-ip>:<port>
```

### L3: 跨节点通信

```bash
# 确认 Pod 分布
kubectl get pods -o wide -n my-namespace

# 从节点 A 的 Pod ping 节点 B 的 Pod
kubectl exec pod-on-node-a -- ping <pod-ip-on-node-b>

# 检查节点间路由
# 在节点上
ip route | grep <pod-cidr>
# Cilium: cilium-dbg route list
# Calico: calicoctl node status

# 检查隧道接口（VXLAN 模式）
ip link show flannel.1  # Flannel
ip link show vxlan.calico  # Calico VXLAN
ip link show cilium_vxlan  # Cilium

# MTU 问题排查
ping -s 1472 -M do <remote-pod-ip>  # 测试 MTU
# 如果失败，检查 MTU 配置
ip link show eth0 | grep mtu
```

### L4: NetworkPolicy 调试

```bash
# 查看命名空间中的策略
kubectl get networkpolicy -n my-namespace -o yaml

# Cilium: 查看策略是否生效
cilium policy get
cilium endpoint list
cilium endpoint log <endpoint-id>

# 测试策略是否阻断
kubectl exec source-pod -- curl -v http://target-pod:8080
# 如果超时 → 可能被 NetworkPolicy 阻断

# 临时禁用策略测试
kubectl delete networkpolicy <policy-name> -n my-namespace
# 测试后恢复
```

### L5: DNS 问题

```bash
# CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# DNS 查询延迟
kubectl exec test-pod -- nslookup -debug kubernetes.default
kubectl exec test-pod -- dig kubernetes.default.svc.cluster.local

# 检查 ndots 配置（影响外部域名解析速度）
kubectl exec test-pod -- cat /etc/resolv.conf
# 如果 ndots:5 → 外部域名会尝试 5 次搜索域
# 解决：在 Pod spec 中设置 dnsConfig
```

```yaml
# 优化 DNS 配置
spec:
  dnsPolicy: ClusterFirst
  dnsConfig:
    options:
      - name: ndots
        value: "2"  # 减少无效搜索
      - name: single-request-reopen
      - name: timeout
        value: "1"
```

## 诊断工具集

### netshoot（网络诊断瑞士军刀）

```bash
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash

# 常用命令
dig my-service.namespace.svc.cluster.local
tcpdump -i eth0 -nn port 8080
curl -w "@curl-format.txt" http://my-service:8080
mtr --report my-service
ss -tlnp
nmap -sT my-service
```

### Cilium Hubble 流量可视化

```bash
# 安装 Hubble CLI
hubble observe --namespace production --to-service my-service
hubble observe --namespace production --verdict DROPPED
hubble observe --namespace production --type l7

# 实时流量监控
hubble observe -f --namespace production --pod my-pod
```

### 抓包分析

```bash
# 在节点上抓 Pod 流量
# 找到 Pod 的 veth 接口
kubectl exec -n kube-system ds/cilium -- cilium endpoint list | grep <pod-ip>
# 抓包
tcpdump -i <veth-interface> -w /tmp/pod-traffic.pcap

# 或使用 nsenter 进入 Pod 网络命名空间
PID=$(crictl inspect <container-id> | jq .info.pid)
nsenter -t $PID -n tcpdump -i eth0 -w /tmp/pod.pcap
```

## 常见问题速查

| 症状 | 可能原因 | 排查命令 |
|------|----------|----------|
| Pod 无法解析 DNS | CoreDNS 异常/ndots 配置 | `nslookup kubernetes.default` |
| Service 无响应 | Endpoints 为空/selector 不匹配 | `kubectl get endpoints` |
| 跨节点 Pod 不通 | CNI 路由/MTU/防火墙 | `ping <remote-pod-ip>` |
| NetworkPolicy 阻断 | 策略过严/缺少允许规则 | `kubectl get netpol -o yaml` |
| 外部域名解析慢 | ndots:5 导致多次搜索 | 调整 dnsConfig |
| Service 间歇性超时 | kube-proxy 规则同步延迟 | 检查 kube-proxy 日志 |
| Ingress 502 | 后端 Pod 未就绪 | `kubectl get endpoints ingress-svc` |
| 连接数耗尽 | conntrack 表满 | `conntrack -C` / 调整内核参数 |

## 内核参数调优

```bash
# /etc/sysctl.d/99-k8s-network.conf
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
```

## 自动化网络诊断脚本

### 一键网络健康检查

```bash
#!/bin/bash
# 🟢 只读：K8s 网络健康一键检查
echo "=== K8s 网络健康检查 $(date) ==="

# 1. CoreDNS 状态
echo -n "[1/7] CoreDNS: "
DNS_PODS=$(kubectl get pods -n kube-system -l k8s-app=kube-dns --no-headers 2>/dev/null | grep Running | wc -l | tr -d ' ')
echo "$DNS_PODS 个运行中"

# 2. DNS 解析测试
echo -n "[2/7] DNS 解析: "
kubectl run dns-test --rm -it --restart=Never --image=busybox:1.36 --quiet -- \
  nslookup kubernetes.default 2>/dev/null | grep -q "Address" && echo "✅" || echo "❌"

# 3. Service 连通性
echo -n "[3/7] API Server Service: "
kubectl run svc-test --rm -it --restart=Never --image=busybox:1.36 --quiet -- \
  wget -qO- --timeout=5 https://kubernetes.default/healthz 2>/dev/null | grep -q "ok" && echo "✅" || echo "❌"

# 4. 跨节点连通性
echo "[4/7] 跨节点 Pod 连通:"
NODES=$(kubectl get nodes --no-headers -o custom-columns=NAME:.metadata.name | head -2)
NODE_A=$(echo "$NODES" | head -1)
NODE_B=$(echo "$NODES" | tail -1)
POD_A=$(kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_A,status.phase=Running --no-headers | head -1 | awk '{print $2" -n "$1}')
POD_B_IP=$(kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_B,status.phase=Running --no-headers | head -1 | awk '{print $7}')
if [ -n "$POD_B_IP" ]; then
  kubectl exec $POD_A -- ping -c 2 -W 3 $POD_B_IP &>/dev/null && echo "  ✅ $NODE_A → $NODE_B" || echo "  ❌ $NODE_A → $NODE_B"
fi

# 5. NetworkPolicy 状态
echo -n "[5/7] NetworkPolicy 数量: "
kubectl get networkpolicy -A --no-headers 2>/dev/null | wc -l | tr -d ' '

# 6. kube-proxy/CNI 状态
echo -n "[6/7] CNI Pod: "
CNI_NOT_RUNNING=$(kubectl get pods -n kube-system -l k8s-app=cilium --no-headers 2>/dev/null | grep -v Running | wc -l | tr -d ' ')
echo "$CNI_NOT_RUNNING 个异常"

# 7. conntrack 使用率
echo -n "[7/7] conntrack 使用率: "
kubectl exec -n kube-system ds/kube-proxy -- cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null || echo "N/A"

echo ""
echo "=== 检查完成 ==="
```

## Ingress/Gateway 故障排查

### Ingress 排查流程

```bash
# 🟢 只读：Ingress 状态检查
kubectl get ingress -A -o wide
kubectl describe ingress <name> -n <ns>

# 检查 Ingress Controller Pod
kubectl get pods -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50

# 检查后端 Endpoints
kubectl get endpoints <backend-svc> -n <ns>

# 检查 Ingress Class
kubectl get ingressclass

# 测试 Ingress 路径
kubectl run curl-test --rm -it --image=curlimages/curl -- \
  curl -v -H "Host: app.example.com" http://<ingress-ip>/
```

### Ingress 常见问题

| 症状 | 根因 | 修复 |
|------|------|------|
| 502 Bad Gateway | 后端 Pod 未就绪/端口不匹配 | 检查 Endpoints + Service targetPort |
| 404 Not Found | 路径配置错误/Ingress 未生效 | 检查 path/pathType + Ingress Class |
| SSL 证书错误 | Secret 不存在/证书过期 | 检查 TLS Secret + cert-manager |
| 超时 | 后端响应慢/代理超时配置 | 调整 proxy-read-timeout annotation |
| 连接被拒绝 | Ingress Controller 未监听 | 检查 Service type + 节点端口 |

## 网络监控告警

### PrometheusRule — 网络健康

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: network-health-alerts
  namespace: monitoring
spec:
  groups:
    - name: network-health
      rules:
        - alert: CoreDNSDown
          expr: |
            up{job="coredns"} == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "CoreDNS 实例 {{ $labels.instance }} 不可用"

        - alert: DNSLatencyHigh
          expr: |
            histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "DNS P99 延迟 > 100ms"

        - alert: ConntrackTableNearFull
          expr: |
            node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "conntrack 表使用率 > 80%，可能导致新连接失败"

        - alert: PodNetworkErrors
          expr: |
            rate(container_network_receive_errors_total[5m]) > 0
            or rate(container_network_transmit_errors_total[5m]) > 0
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 网络错误"

        - alert: KubeProxySyncSlow
          expr: |
            kubeproxy_sync_proxy_rules_duration_seconds > 5
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "kube-proxy 规则同步 > 5s，Service 更新可能延迟"
```

## 网络性能基准测试

### 带宽与延迟测试

```bash
# 🟢 只读：Pod 间带宽测试 (iperf3)
# 服务端
kubectl run iperf-server --image=networkstatic/iperf3 -- iperf3 -s

# 客户端
kubectl run iperf-client --rm -it --image=networkstatic/iperf3 -- \
  iperf3 -c iperf-server -t 30 -P 4

# 延迟测试
kubectl run ping-test --rm -it --image=busybox:1.36 -- \
  ping -c 100 <target-pod-ip>

# 跨 AZ 延迟
kubectl run az-test --rm -it --image=busybox:1.36 -- \
  ping -c 50 <pod-ip-in-other-az>
```

### 网络性能基线

| 场景 | 带宽 | 延迟 | 说明 |
|------|------|------|------|
| 同节点 Pod-to-Pod | 10+ Gbps | < 0.1ms | veth 直连 |
| 跨节点 (VXLAN) | 1-5 Gbps | 0.2-0.5ms | 封装开销 |
| 跨节点 (路由模式) | 5-10 Gbps | 0.1-0.3ms | 无封装 |
| 跨 AZ | 1-5 Gbps | 0.5-2ms | 物理距离 |
| Service (iptables) | 降 10-20% | +0.1ms | NAT 开销 |
| Service (IPVS) | 降 5-10% | +0.05ms | 更优 |

## Service Mesh 调试

### Istio/Envoy 排查

```bash
# 🟢 只读：检查 Sidecar 注入
kubectl get pods -n <ns> -o jsonpath='{range .items[*]}{.metadata.name}: {range .spec.containers[*]}{.name} {end}{"\n"}{end}'

# 检查 Envoy Sidecar 状态
kubectl exec <pod> -c istio-proxy -- pilot-agent status
kubectl exec <pod> -c istio-proxy -- curl -s localhost:15000/stats | grep cluster_manager

# 检查 VirtualService/DestinationRule
kubectl get virtualservice,destinationrule -n <ns>

# 查看 Envoy 访问日志
kubectl logs <pod> -c istio-proxy --tail=50

# 检查 mTLS 状态
kubectl exec <pod> -c istio-proxy -- curl -s localhost:15000/certs
```

## Related

- [[05-网络/02-网络基础/index.md|网络基础]]
- [[05-网络/05-eBPF/index.md|eBPF 网络]]
- [[19-故障诊断/index.md|故障诊断]]
