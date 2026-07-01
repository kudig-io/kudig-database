---
title: "[2026-01-22] [P0] CoreDNS 问题导致服务发现中断"
category: case-study
tags: [production, incident, networking, dns, service-discovery]
date: "2026-01-22"
severity: P0
mttr: "22min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-01-22] CoreDNS CrashLoopBackOff 导致全集群服务发现中断

## 工单信息
- **工单编号**: INC-2026-0122-002
- **发现时间**: 2026-01-22 14:15 UTC
- **恢复时间**: 2026-01-22 14:37 UTC
- **影响范围**: 全集群（12 个 namespace, 380+ Pod）
- **业务影响**: 微服务间调用全部失败，用户登录、订单、推荐服务均不可用

## 问题现象
14:15，监控大盘全红，大量 `connection refused`、`no such host` 错误。用户反馈：
- 登录页面报错 "无法连接到认证服务"
- 购物车添加商品后无法提交
- 所有内部 API 调用返回 `dial tcp: lookup auth-service on 10.96.0.10:53: no such host`

## 诊断过程

**14:16** — 检查 DNS 解析：
```bash
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- nslookup kubernetes.default
# ;; connection timed out; no servers could be reached
```

**14:17** — 检查 CoreDNS Pod 状态：
```bash
kubectl get pods -n kube-system -l k8s-app=kube-dns
# NAME                       READY   STATUS             RESTARTS   AGE
# coredns-5c6c9d7f8b-2k9mz   0/1     CrashLoopBackOff   12         23m
# coredns-5c6c9d7f8b-7x3pq   0/1     CrashLoopBackOff   11         23m
```

**14:18** — 查看 CoreDNS 日志：
```bash
kubectl logs -n kube-system coredns-5c6c9d7f8b-2k9mz --previous
# [ERROR] plugin/errors: 2 1234567890.12345.cluster.local. A: plugin/forward: no healthy upstreams
# [ERROR] plugin/loop: Forwarding loop detected in "/etc/resolv.conf"
# [FATAL] plugin/loop: See https://coredns.io/plugins/loop#troubleshooting
```

**14:20** — 检查 CoreDNS ConfigMap：
```bash
kubectl get cm coredns -n kube-system -o yaml
# ...
# forward . /etc/resolv.conf {
#     max_concurrent 1000
# }
# ...
```

**14:22** — 检查节点 resolv.conf 和近期变更：
```bash
# 节点 /etc/resolv.conf 被 DHCP 更新，新插入了 127.0.0.53 (systemd-resolved)
cat /etc/resolv.conf
# nameserver 127.0.0.53
# search ec2.internal

# 变更来源：运维团队 14:00 执行的 "优化 DNS 配置" Ansible Playbook
```

## 根因
运维团队在 14:00 执行节点级 Ansible Playbook，将 `/etc/resolv.conf` 统一指向 `127.0.0.53`（systemd-resolved），期望提升 DNS 缓存命中率。但 CoreDNS 的 `forward` 插件配置了 `forward . /etc/resolv.conf`，读取到 `127.0.0.53` 后，CoreDNS 向自身所在容器内的 127.0.0.53 转发查询，形成转发环路，触发 `loop` 插件保护机制，导致 CoreDNS 启动即退出。

## 修复动作

**14:25** — 回滚节点 resolv.conf：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 在所有节点执行
sudo systemctl disable systemd-resolved
sudo rm -f /etc/resolv.conf
sudo bash -c 'cat > /etc/resolv.conf <<EOF
nameserver 10.0.0.2
search ec2.internal
EOF'
```

**14:30** — 更新 CoreDNS ConfigMap，使用固定上游而非 `/etc/resolv.conf`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl edit cm coredns -n kube-system
# 将 forward . /etc/resolv.conf 改为：
# forward . 10.0.0.2 {
#     max_concurrent 1000
# }
```

**14:32** — 重启 CoreDNS：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment coredns -n kube-system
kubectl get pods -n kube-system -l k8s-app=kube-dns
# NAME                       READY   STATUS    RESTARTS   AGE
# coredns-5c6c9d7f8b-9a2bc   1/1     Running   0          2m
```

**14:35** — 验证 DNS：
```bash
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- nslookup auth-service.prod
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      auth-service.prod
# Address 1: 10.96.123.45 auth-service.prod.svc.cluster.local
```

## 验证
- 14:36 — `curl http://auth-service.prod/health` 返回 200
- 14:37 — 各业务 namespace 的 5xx 率归零，登录、下单恢复正常

## 复盘
- **直接原因**: 节点 `/etc/resolv.conf` 被改为 `127.0.0.53` → CoreDNS forward 形成查询环路 → CrashLoopBackOff → 全集群 DNS 解析失败
- **根本原因**: 节点级 DNS 变更未通知集群团队，CoreDNS 依赖脆弱的 `/etc/resolv.conf` 转发配置
- **改进措施**:
  1. CoreDNS `forward` 插件改为使用 VPC DNS IP（如 AWS `10.0.0.2`），不再读取节点 `/etc/resolv.conf`
  2. 节点级网络变更需走集群 SRE 变更评审（CAB）
  3. 为 CoreDNS 添加 `loop` 告警的专属 PagerDuty 策略，5min 内响应
- **相关 Skill**: [[k8s-network-configuration-guide]]
- **相关 FTA**: [[dns-fta]]
