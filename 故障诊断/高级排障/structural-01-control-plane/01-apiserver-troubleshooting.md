---
title: API Server 故障排查指南
description: '# API Server 故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- coredns
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- API Server 故障排查指南 是什么
- 如何 API Server 故障排查指南
- API Server 故障排查指南 故障排查
- API Server 故障排查指南 排障步骤
trigger_keywords:
- API
- Server
- 故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# API Server 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

本文档面向 Kubernetes 集群管理员及 SRE 工程师，旨在提供一套从基础排查到深度优化的完整体系。

### 🎓 初学者视角
- **核心概念**：API Server 是集群的唯一入口，所有组件（kubelet, scheduler 等）都通过它与 etcd 通信。
- **简单类比**：API Server 就像一个 7x24 小时营业的政务大厅窗口，所有的办事申请（YAML）都必须在这里登记、校验并存入档案库（etcd）。

### 👨‍💻 资深专家视角
- **并发控制**：深度理解 APF (API Priority and Fairness) 如何在多租户高并发场景下保护核心流量。
- **内存管理**：掌握 API Server 在处理大规模 `LIST` 请求时的内存消耗模式及 `Watch` 缓存的调优思路。
- **扩展性排查**：分析 Aggregated API Server (如 Metrics Server) 异常对主 API Server 性能的链式影响。

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 10 分钟快速诊断

1. **确认影响面**：`kubectl version --short && kubectl get --raw /readyz`，若失败同时检查 LB 健康检查与节点安全组端口 6443。
2. **看健康端点**：`curl -k https://$HOST:6443/readyz?verbose`，若等到 `[-]etcd`/`[-]informer-sync` 失败，优先检查 etcd/网络。
3. **看资源与限流**：`kubectl top pod -A | grep kube-apiserver`、`grep -E "429|throttling" /var/log/kube-apiserver.log | tail`，观察 APF 触发与 QPS 峰值。
4. **看 etcd 延迟**：`kubectl exec -n kube-system etcd-<node> -- etcdctl endpoint status --write-out=table`，关注 `db size`、`raft term` 与 `leader` 变更频率。
5. **看请求模式**：`kubectl logs -n kube-system kube-apiserver-<node> | grep "LIST" | head`，确认是否有大表全量 LIST 或 watch 风暴。
6. **快速缓解**：
   - LB / iptables 阶段：切换备用 LB 或移除异常后端。
   - 资源阶段：临时调高 CPU/memory request/limit，必要时水平扩容副本（前提：etcd/LB 配置允许）。
   - 流量阶段：临时调低过载来源（CI 扫描、监控抓取）并开启 APF 保护核心租户。
7. **记录证据**：在处置前后保存 `/readyz?verbose` 输出、pprof（`/debug/pprof/profile`）、关键日志与指标快照，以便后续复盘。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 API Server 完全不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| kubectl 命令超时 | `Unable to connect to the server: dial tcp <IP>:6443: i/o timeout` | kubectl 客户端 | 直接命令行输出 |
| kubectl 连接被拒绝 | `The connection to the server <IP>:6443 was refused` | kubectl 客户端 | 直接命令行输出 |
| 证书验证失败 | `x509: certificate signed by unknown authority` | kubectl 客户端 | 直接命令行输出 |
| 证书过期 | `x509: certificate has expired or is not yet valid` | kubectl 客户端 | 直接命令行输出 |
| 服务端内部错误 | `Internal error occurred: the server is currently unable to handle the request` | API Server | kubectl 输出或 API 响应 |
| 负载均衡器问题 | `502 Bad Gateway` | Load Balancer | 客户端响应 |
| API Server 内存溢出 | `OOMKilled` | Kubernetes | Pod 状态或日志 |
| 请求队列积压 | `context deadline exceeded` | 客户端 | kubectl 命令输出 |

#### 1.1.2 API Server 响应缓慢

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 请求超时 | `context deadline exceeded` | kubectl/客户端 | 命令行输出 |
| 请求延迟高 | `request latency exceeded threshold` | API Server 日志 | `journalctl -u kube-apiserver` |
| 限流触发 | `429 Too Many Requests` | API Server | 客户端响应码 |
| 优先级调度延迟 | `request is being throttled by APF` | API Server 日志 | API Server 日志 |

#### 1.1.3 API Server 间歇性问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 偶发连接失败 | `connection reset by peer` | kubectl/客户端 | 命令行输出 |
| 负载均衡异常 | `no healthy upstream` | 负载均衡器 | LB 日志/健康检查 |
| Leader 切换 | `leadership changed` | API Server 日志 | API Server 日志 |
| etcd 连接波动 | `etcdserver: request timed out` | API Server 日志 | API Server 日志 |

#### 1.1.4 认证授权错误

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 未认证 | `Unauthorized` (401) | API Server | API 响应 |
| 无权限 | `Forbidden` (403) | API Server | API 响应 |
| ServiceAccount 问题 | `no credentials provided` | Pod 内客户端 | Pod 日志 |
| Token 过期 | `token has expired` | API Server | API 响应 |

#### 1.1.5 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大促活动期间** | API Server 响应延迟增加 | 请求量激增超出处理能力 | 水平扩展、限流调优 |
| **证书批量过期** | 多个组件同时认证失败 | 证书管理不当 | 自动化证书轮转 |
| **恶意扫描攻击** | 429 限流频繁触发 | 外部恶意请求 | WAF防护、IP黑名单 |
| **配置变更失误** | API Server 启动失败 | 参数配置错误 | 配置校验、灰度发布 |
| **存储性能下降** | etcd 延迟高 | 存储介质老化 | 存储优化、迁移SSD |
| **网络抖动** | 间歇性连接失败 | 网络不稳定 | 网络质量优化 |

### 1.2 报错查看方式汇总

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 API Server 进程状态（systemd 管理）
systemctl status kube-apiserver

# 查看 API Server 日志（systemd 管理）
journalctl -u kube-apiserver -f --no-pager -l

# 查看 API Server 日志（容器化部署）
kubectl logs -n kube-system kube-apiserver-<node-name> --tail=500

# 查看 API Server Pod 日志（静态 Pod）
crictl logs $(crictl ps -a --name kube-apiserver -q | head -1)

# 查看 API Server 健康状态
curl -k https://localhost:6443/healthz
curl -k https://localhost:6443/livez
curl -k https://localhost:6443/readyz

# 查看详细健康检查
curl -k 'https://localhost:6443/readyz?verbose'

# 查看 API Server 指标
curl -k https://localhost:6443/metrics | grep apiserver_request
```
### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **kubectl 操作** | 完全不可用 | 所有 kubectl 命令无法执行 |
| **API 调用** | 完全不可用 | 所有 Kubernetes API 请求失败 |
| **控制器操作** | 控制循环中断 | Controller Manager、Scheduler 等无法获取/更新资源状态 |
| **准入控制** | 无法工作 | Webhook、ValidatingAdmission 等无法执行 |
| **认证鉴权** | 完全失效 | 无法验证用户身份和权限 |
| **资源 CRUD** | 无法执行 | 无法创建、读取、更新、删除任何 Kubernetes 资源 |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **现有工作负载** | 短期无影响 | 已运行的 Pod 继续运行，但无法扩缩容、更新 |
| **自动扩缩容** | 失效 | HPA/VPA/CA 无法获取指标和调整副本数 |
| **服务发现** | 部分影响 | 新的 Endpoints 无法更新，CoreDNS 无法感知变化 |
| **监控告警** | 可能失效 | 依赖 API 的监控系统无法采集数据 |
| **CI/CD 流程** | 中断 | 自动化部署流程无法执行 |
| **故障自愈** | 失效 | 节点问题后 Pod 无法重新调度 |
| **证书轮转** | 中断 | 证书到期后无法自动更新 |
| **审计日志** | 丢失 | 无法记录 API 操作审计日志 |

#### 1.3.3 影响严重程度评估

##### 业务连续性影响矩阵

| 问题类型 | RTO(恢复时间目标) | RPO(数据丢失目标) | 业务影响等级 | 处理优先级 |
|----------|-------------------|-------------------|--------------|------------|
| **完全不可用** | &lt; 5分钟 | 0 | P0-紧急 | 立即处理 |
| **部分功能受限** | &lt; 30分钟 | 0 | P1-高 | 快速响应 |
| **性能下降** | &lt; 2小时 | 0 | P2-中 | 计划处理 |
| **偶发性问题** | &lt; 24小时 | 0 | P3-低 | 持续观察 |

##### 核心业务依赖关系图

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────────────┐
│                    API Server 问题影响传播链                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   API Server 不可用                                                          │
│         │                                                                    │
│         ├──► kubectl 失效 ──► 运维人员无法操作集群                           │
│         │                                                                    │
│         ├──► Scheduler 失效 ──► 新 Pod 无法调度                              │
│         │                                                                    │
│         ├──► Controller Manager 失效 ──► 控制循环中断                         │
│         │         │                                                          │
│         │         ├──► Deployment 无法管理 ReplicaSet                        │
│         │         ├──► ReplicaSet 无法管理 Pod 副本数                         │
│         │         ├──► Service 的 Endpoints 无法更新                         │
│         │         └──► Node Controller 无法检测节点状态                       │
│         │                                                                    │
│         ├──► kubelet watch 断开 ──► 无法接收新的 Pod 规格                     │
│         │                                                                    │
│         ├──► kube-proxy watch 断开 ──► Service 规则无法更新                   │
│         │                                                                    │
│         └──► 外部集成失效 ──► CI/CD、监控、日志收集等系统受影响               │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```
---

## 2. 排查方法与步骤

### 2.1 排查原理

API Server 是 Kubernetes 集群的核心组件，所有组件都通过 API Server 进行通信。排查 API Server 问题需要从以下层面入手：

#### 2.1.1 进程层面
- **生命周期管理**：理解 systemd/kubelet 如何管理 kube-apiserver 静态 Pod，重启策略与健康探针如何协同
- **启动依赖**：需依赖 etcd 可用、证书存在、配置文件合法，任一缺失都会导致启动失败
- **核心流程**：初始化 → 注册 API 资源 → 启动 Informer 缓存 → 监听端口 → 提供服务

#### 2.1.2 网络层面
- **多层连接校验**：客户端 → LB → API Server → etcd，每一跳都可能产生延迟/证书错误/超时
- **端口绑定与监听**：默认 6443(secure)、8080(insecure,已废弃)、健康端口(默认 6443 复用或独立)
- **TLS 握手**：客户端证书、服务端证书、CA 证书链，任一失效都会导致 `x509` 错误
- **负载均衡器健康检查**：LB 健康探针路径(如 `/healthz`)返回非 200 时会将后端标记为不健康

#### 2.1.3 存储层面
- **etcd 连接池**：API Server 维护与 etcd 的长连接池，连接断开会触发重连与缓存失效
- **Watch 机制**：所有资源变更通过 etcd watch 推送，etcd 延迟直接影响 API 响应速度
- **数据一致性**：API Server 作为 etcd 的唯一客户端，负责数据校验、版本控制(ResourceVersion)与冲突检测

#### 2.1.4 资源层面
- **内存管理**：Informer 缓存(所有资源在内存)、连接池、请求上下文，大集群内存消耗可达数 GB
- **CPU 瓶颈**：序列化/反序列化、准入控制、RBAC 鉴权、复杂 watch 过滤，高 QPS 下 CPU 成为瓶颈
- **文件描述符**：每个 watch 连接消耗一个 fd，大量长连接会耗尽 fd 限制

#### 2.1.5 配置层面
- **启动参数**：超过 200 个可配置参数，常见的如 `--etcd-servers`、`--tls-cert-file`、`--enable-admission-plugins`
- **准入控制器链**：MutatingAdmission → ValidatingAdmission → ResourceQuota，任一环节超时/失败都会拒绝请求
- **APF(API Priority and Fairness)**：请求分类、优先级队列、并发限制，配置不当会导致关键请求被限流

### 2.2 排查逻辑决策树

```
开始排查
    │
    ├─► 检查进程状态
    │       │
    │       ├─► 进程不存在 ──► 检查启动失败原因（配置错误、资源不足）
    │       │
    │       └─► 进程存在 ──► 继续下一步
    │
    ├─► 检查健康端点
    │       │
    │       ├─► /healthz 失败 ──► 检查核心组件连接（etcd）
    │       │
    │       ├─► /livez 失败 ──► 检查死锁和资源耗尽
    │       │
    │       └─► /readyz 失败 ──► 检查依赖组件和初始化状态
    │
    ├─► 检查网络连通性
    │       │
    │       ├─► 端口未监听 ──► 检查绑定配置和端口冲突
    │       │
    │       ├─► 证书错误 ──► 检查证书有效期和配置
    │       │
    │       └─► 连接正常 ──► 继续下一步
    │
    ├─► 检查 etcd 连接
    │       │
    │       ├─► 连接失败 ──► 排查 etcd 状态
    │       │
    │       └─► 连接正常 ──► 继续下一步
    │
    ├─► 检查资源使用
    │       │
    │       ├─► CPU/内存过高 ──► 分析负载来源，考虑扩容
    │       │
    │       ├─► 文件描述符耗尽 ──► 调整 ulimit
    │       │
    │       └─► 资源正常 ──► 继续下一步
    │
    └─► 检查日志错误
            │
            ├─► 认证/授权错误 ──► 检查 RBAC 和证书配置
            │
            ├─► 准入控制错误 ──► 检查 Webhook 配置
            │
            └─► 其他错误 ──► 根据具体错误分析
```

### 2.3 排查步骤和具体命令

#### 🔍 生产环境快速诊断清单

在生产环境中，时间就是金钱。以下是按优先级排序的快速诊断步骤：

**黄金5分钟诊断法**：
1. `kubectl get nodes` - 确认集群基本状态 (30秒)
2. `curl -k https://localhost:6443/healthz` - 检查API Server健康 (30秒)  
3. `systemctl status kube-apiserver` - 检查进程状态 (30秒)
4. `journalctl -u kube-apiserver --since "5 minutes ago"` - 查看近期错误 (1分钟)
5. `ETCDCTL_API=3 etcdctl endpoint health` - 检查etcd连接 (1分钟)

#### 2.3.1 第一步：检查进程状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API Server 进程是否存在
ps aux | grep kube-apiserver | grep -v grep

# 检查进程详细信息
pgrep -a kube-apiserver

# systemd 管理的服务状态
systemctl status kube-apiserver

# 静态 Pod 方式部署检查
ls -la /etc/kubernetes/manifests/kube-apiserver.yaml
crictl ps -a | grep kube-apiserver

# 查看进程启动参数
cat /proc/$(pgrep kube-apiserver)/cmdline | tr '\0' '\n'
```
#### 2.3.2 第二步：检查健康端点

```bash
# 检查整体健康状态
curl -k https://127.0.0.1:6443/healthz
# 预期输出: ok

# 检查存活状态
curl -k https://127.0.0.1:6443/livez
# 预期输出: ok

# 检查就绪状态
curl -k https://127.0.0.1:6443/readyz
# 预期输出: ok

# 详细健康检查（显示每个子组件状态）
curl -k 'https://127.0.0.1:6443/healthz?verbose'
curl -k 'https://127.0.0.1:6443/livez?verbose'
curl -k 'https://127.0.0.1:6443/readyz?verbose'

# 检查特定组件健康状态
curl -k 'https://127.0.0.1:6443/healthz/etcd'
curl -k 'https://127.0.0.1:6443/healthz/poststarthook/start-kube-apiserver-admission-initializer'
```

#### 2.3.3 第三步：检查网络连通性

```bash
# 检查端口监听状态
netstat -tlnp | grep 6443
ss -tlnp | grep 6443

# 检查防火墙规则
iptables -L -n | grep 6443
firewall-cmd --list-all

# 测试本地连接
curl -k -v https://127.0.0.1:6443/healthz

# 测试远程连接
curl -k -v https://<api-server-ip>:6443/healthz

# 检查 TLS 证书信息
openssl s_client -connect 127.0.0.1:6443 -showcerts </dev/null 2>/dev/null | openssl x509 -noout -text

# 检查证书有效期
openssl s_client -connect 127.0.0.1:6443 </dev/null 2>/dev/null | openssl x509 -noout -dates
```

#### 2.3.4 第四步：检查 etcd 连接

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd 端点健康
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 检查 etcd 集群状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status --write-out=table

# 检查 API Server 到 etcd 的网络延迟
ping -c 5 <etcd-ip>

# 查看 API Server 日志中的 etcd 相关错误
journalctl -u kube-apiserver | grep -i etcd | tail -50
```
#### 2.3.5 第五步：检查资源使用

```bash
# 检查 CPU 和内存使用
top -p $(pgrep kube-apiserver) -b -n 1

# 检查进程资源限制
cat /proc/$(pgrep kube-apiserver)/limits

# 检查文件描述符使用
ls /proc/$(pgrep kube-apiserver)/fd | wc -l
cat /proc/$(pgrep kube-apiserver)/limits | grep "Max open files"

# 检查系统整体资源
free -h
df -h
vmstat 1 5

# 检查 goroutine 数量（通过 metrics）
curl -k https://127.0.0.1:6443/metrics | grep go_goroutines

# 检查请求队列长度
curl -k https://127.0.0.1:6443/metrics | grep apiserver_current_inflight_requests
```

#### 2.3.6 第六步：检查日志错误

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 实时查看日志
journalctl -u kube-apiserver -f --no-pager

# 查看最近的错误日志
journalctl -u kube-apiserver -p err --since "1 hour ago"

# 查看启动日志
journalctl -u kube-apiserver -b | head -100

# 静态 Pod 方式查看日志
crictl logs $(crictl ps -q --name kube-apiserver) 2>&1 | tail -500

# 查找常见错误模式
journalctl -u kube-apiserver | grep -iE "(error|failed|unable|timeout)" | tail -50

# 查找认证授权相关错误
journalctl -u kube-apiserver | grep -iE "(unauthorized|forbidden|authentication|authorization)" | tail -50

# 查找证书相关错误
journalctl -u kube-apiserver | grep -iE "(certificate|x509|tls)" | tail -50

# 🔍 高级日志分析技巧
# 提取错误模式和频率统计
journalctl -u kube-apiserver --since "1 hour ago" | \
  grep -i "error\|failed\|warning" | \
  awk '{print $NF}' | \
  sort | uniq -c | sort -nr | head -10

# 分析请求延迟分布
curl -k https://127.0.0.1:6443/metrics | \
  grep apiserver_request_duration_seconds_bucket | \
  awk '{print $1}' | cut -d'{' -f2 | cut -d'}' -f1 | \
  sort | uniq -c | sort -nr
```
#### 2.3.7 第七步：检查配置

```bash
# 查看 API Server 启动配置（静态 Pod）
cat /etc/kubernetes/manifests/kube-apiserver.yaml

# 检查证书文件是否存在
ls -la /etc/kubernetes/pki/

# 检查证书有效期
for cert in /etc/kubernetes/pki/*.crt; do
  echo "=== $cert ==="
  openssl x509 -in $cert -noout -dates 2>/dev/null
done

# 检查 kubeconfig 文件
cat /etc/kubernetes/admin.conf | grep server

# 验证配置语法
kube-apiserver --help | grep -A2 "<flag-name>"
```

### 2.4 排查注意事项

#### 💡 初学者笔记：健康检查端点的区别
- `/healthz`：基础健康检查，通常只检查 API Server 进程本身。
- `/livez`：存活检查，如果失败，kubelet 会重启 API Server。它会检查 etcd 连通性。
- `/readyz`：就绪检查，如果失败，LB 会摘除该节点。它会检查所有 post-start hooks 是否完成。

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **证书文件权限** | 不要随意更改证书文件权限 | 保持原有权限，一般为 600 |
| **日志敏感信息** | 日志可能包含敏感信息 | 不要将日志发送到不安全的渠道 |
| **端口暴露** | 6443 端口是敏感端口 | 确保只有授权的网络可以访问 |
| **kubeconfig 安全** | kubeconfig 包含认证信息 | 不要泄露 kubeconfig 内容 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **高可用场景** | 多 API Server 实例 | 检查所有实例状态，注意负载均衡配置 |
| **静态 Pod 重启** | 修改 manifest 会触发重启 | 先备份原配置，谨慎修改 |
| **日志量** | API Server 日志量可能很大 | 使用 tail 或 grep 过滤 |
| **时钟同步** | 证书验证依赖时钟 | 确保节点时间同步 |
| **etcd 依赖** | API Server 强依赖 etcd | 先确认 etcd 正常再排查 API Server |

#### 2.4.3 排查顺序建议

1. **先外后内**：先从外部（kubectl）测试，再登录 Master 节点检查
2. **先简后繁**：先检查进程和网络，再检查日志和配置
3. **先主后从**：高可用场景先检查主 API Server
4. **保留现场**：修复前先保存日志和配置

### 🚀 2.5 深度解析（专家专区）

#### 2.5.1 API 聚合器（Aggregation Layer）问题
当使用了 Metrics Server 或 Prometheus Adapter 等扩展 API 时，如果这些 Aggregated API Server 响应极慢，会导致主 API Server 的某些请求（如 `kubectl get --all-namespaces`）整体超时。
- **排查方法**：`kubectl get apiservice` 检查状态不为 `Available` 的服务。
- **专家提示**：API Server 会串行处理某些聚合请求，一个坏掉的扩展可能会拖慢全局。

#### 2.5.2 僵尸 Pod 与 Watch 机制
现象：`kubectl delete pod` 后 Pod 消失，但 `crictl ps` 仍然能看到。
- **原因**：API Server 可能因为高负载丢失了 Watch 事件，或者 kubelet 与 API Server 的连接断开且未正确触发重同步。
- **解决**：强制删除 (`--force --grace-period=0`) 并重启该节点的 kubelet。

#### 2.5.3 Webhook 的“自杀效应”
如果一个 `ValidatingWebhookConfiguration` 配置为 `FailurePolicy: Fail` 且指向了集群内部的一个 Pod（如 Admission Controller），当该 Pod 异常或网络不通时，会导致所有（或符合规则的）API 请求被拒绝，甚至连修复该 Webhook 的 `kubectl delete` 请求也被拒绝。
- **紧急避险**：直接登录 master 节点，跳过 Webhook 修改 API Server 配置，或直接在 etcd 中删除该 Webhook 配置（高风险）。

---

## 3. 解决方案与风险控制

### 3.1 API Server 进程未运行

#### 3.1.1 解决步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：检查并启动服务（systemd 方式）
systemctl start kube-apiserver
systemctl enable kube-apiserver

# 步骤 2：检查启动失败原因
journalctl -u kube-apiserver -b --no-pager | tail -100

# 步骤 3：验证配置文件语法（静态 Pod 方式）
# 备份当前配置
cp /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/kube-apiserver.yaml.bak

# 检查 YAML 语法
python3 -c "import yaml; yaml.safe_load(open('/etc/kubernetes/manifests/kube-apiserver.yaml'))"

# 步骤 4：检查必需文件
ls -la /etc/kubernetes/pki/apiserver.crt
ls -la /etc/kubernetes/pki/apiserver.key
ls -la /etc/kubernetes/pki/ca.crt
```
#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 启动失败可能影响集群 | 在非生产时段操作，准备回滚方案 |
| **中** | 配置修改可能导致无法启动 | 修改前备份配置文件 |
| **低** | 日志查看一般无风险 | - |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 操作前确认当前是否有关键业务正在运行
2. 如果是高可用集群，确认其他 API Server 实例正常
3. 准备好回滚方案，保存原始配置
4. 操作后立即验证服务恢复
5. 建议在变更窗口期操作
```

### 3.2 证书过期 - 生产环境最佳实践

#### 3.2.1 预防性措施

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 🛡️ 证书监控告警配置
# PrometheusRule 示例
cat << EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: certificate-expiration-alerts
  namespace: monitoring
spec:
  groups:
  - name: certificate.rules
    rules:
    - alert: CertificateExpiresSoon
      expr: cert_expire_time_seconds - time() < 86400 * 30
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Certificate expires in 30 days"
        description: "{{ \$labels.name }} certificate will expire soon"
EOF

# 自动化证书轮转配置
# kubeadm 集群启用自动轮转
kubeadm alpha certs renew --certificate-dir=/etc/kubernetes/pki
```
#### 3.2.1 解决步骤

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 步骤 1：确认证书过期情况
kubeadm certs check-expiration

# 步骤 2：备份现有证书
cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d)

# 步骤 3：续签所有证书（kubeadm 管理的集群）
kubeadm certs renew all

# 步骤 4：重启控制平面组件
# 静态 Pod 方式：移动并恢复 manifest 文件
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
sleep 10
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

# 或者重启 kubelet
systemctl restart kubelet

# 步骤 5：更新 kubeconfig
cp /etc/kubernetes/admin.conf ~/.kube/config

# 步骤 6：验证证书更新
kubeadm certs check-expiration
kubectl get nodes
```
#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 证书续签期间 API Server 会重启 | 在维护窗口操作，通知相关方 |
| **高** | 证书链不一致可能导致组件无法通信 | 确保所有组件使用新证书 |
| **中** | kubeconfig 未更新导致 kubectl 失效 | 同步更新所有 kubeconfig |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 证书续签会导致短暂的服务中断
2. 高可用集群需要逐个节点操作
3. 操作后需要验证所有控制平面组件正常
4. 确保工作节点的 kubelet 能够使用新证书连接
5. 建议设置证书到期告警，避免紧急续签
6. 生产环境建议配置证书自动轮转
```

### 3.3 etcd 连接问题

#### 3.3.1 解决步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：确认 etcd 服务状态
systemctl status etcd
# 或者（容器化部署）
crictl ps -a | grep etcd

# 步骤 2：检查 etcd 端点连通性
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 步骤 3：检查 API Server 的 etcd 配置
grep -A5 "etcd" /etc/kubernetes/manifests/kube-apiserver.yaml

# 步骤 4：如果 etcd 证书不匹配，检查证书路径
ls -la /etc/kubernetes/pki/etcd/

# 步骤 5：如果 etcd 不可用，查看 etcd 日志
journalctl -u etcd -f --no-pager
# 或者
crictl logs $(crictl ps -q --name etcd)

# 步骤 6：验证修复
kubectl get nodes
```
#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **极高** | etcd 是数据存储核心 | 不要随意重启或修改 etcd |
| **高** | etcd 配置错误可能丢数据 | 有完整备份后再操作 |
| **中** | 网络问题可能影响集群分裂 | 检查网络分区情况 |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. etcd 是集群数据的核心存储，操作前必须有完整备份
2. etcd 问题可能影响整个集群，必须谨慎处理
3. 高可用 etcd 集群确保多数节点正常再操作
4. 不要在 etcd 数据不一致时强制恢复
5. 网络分区场景需要特别注意数据一致性
6. 联系云厂商支持（如使用托管 etcd）
```

### 3.4 资源不足（CPU/内存/文件描述符）

#### 3.4.1 解决步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：确认资源瓶颈
top -p $(pgrep kube-apiserver) -b -n 1
cat /proc/$(pgrep kube-apiserver)/limits

# 步骤 2：临时增加文件描述符限制
# 编辑 systemd service 文件或 Pod manifest
# systemd 方式：
mkdir -p /etc/systemd/system/kube-apiserver.service.d/
cat > /etc/systemd/system/kube-apiserver.service.d/limits.conf << EOF
[Service]
LimitNOFILE=65536
LimitNPROC=65536
EOF
systemctl daemon-reload
systemctl restart kube-apiserver

# 步骤 3：调整 API Server 资源限制（静态 Pod 方式）
# 编辑 /etc/kubernetes/manifests/kube-apiserver.yaml
# 在 resources 部分增加限制：
# resources:
#   requests:
#     cpu: "250m"
#     memory: "512Mi"
#   limits:
#     cpu: "2000m"
#     memory: "4Gi"

# 步骤 4：优化 API Server 参数减少资源使用
# 添加以下参数：
# --max-requests-inflight=400        # 限制并发请求
# --max-mutating-requests-inflight=200  # 限制变更请求
# --watch-cache-sizes=...           # 调整 watch 缓存

# 步骤 5：验证资源使用
curl -k https://127.0.0.1:6443/metrics | grep -E "process_resident_memory|process_cpu"
```
#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启 API Server 会短暂中断服务 | 在维护窗口操作 |
| **中** | 限制参数设置不当可能限流正常请求 | 根据实际负载调整 |
| **低** | 增加资源限制一般无风险 | 确保节点有足够资源 |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 资源调整需要重启 API Server，注意服务中断
2. 限流参数需要根据实际业务负载调整
3. 高可用集群逐个节点操作
4. 监控资源使用趋势，提前扩容
5. 考虑升级 Master 节点规格（长期方案）
```

### 3.5 请求限流（429 Too Many Requests）

#### 3.5.1 解决步骤

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1：确认限流情况
curl -k https://127.0.0.1:6443/metrics | grep apiserver_current_inflight_requests
curl -k https://127.0.0.1:6443/metrics | grep apiserver_dropped_requests_total

# 步骤 2：查看 APF（API Priority and Fairness）配置
kubectl get flowschemas
kubectl get prioritylevelconfigurations

# 步骤 3：识别高频请求来源
# 查看审计日志
cat /var/log/kubernetes/audit/audit.log | jq -r '.user.username' | sort | uniq -c | sort -rn | head

# 步骤 4：调整 APF 配置（增加特定用户的配额）
cat << EOF | kubectl apply -f -
apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
kind: FlowSchema
metadata:
  name: high-priority-system
spec:
  priorityLevelConfiguration:
    name: workload-high
  matchingPrecedence: 500
  distinguisherMethod:
    type: ByUser
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: important-controller
        namespace: kube-system
    resourceRules:
    - verbs: ["*"]
      apiGroups: ["*"]
      resources: ["*"]
EOF

# 步骤 5：增加 API Server 并发限制
# 修改启动参数：
# --max-requests-inflight=800
# --max-mutating-requests-inflight=400

# 步骤 6：验证调整效果
kubectl get --raw /metrics | grep apiserver_flowcontrol
```
#### 3.5.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | APF 配置错误可能影响正常请求 | 测试环境先验证 |
| **中** | 增加并发限制可能增加资源消耗 | 确保节点资源充足 |
| **低** | 配置查看无风险 | - |

#### 3.5.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. APF 配置变更立即生效，谨慎操作
2. 不要禁用默认的限流保护
3. 排查限流根因，优化客户端请求频率
4. 考虑水平扩展 API Server（添加更多实例）
5. 监控 API Server 指标，设置告警阈值
```

### 3.6 高可用场景故障切换

#### 3.6.1 解决步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1：检查所有 API Server 实例状态
# 假设有 3 个 Master 节点
for node in master1 master2 master3; do
  echo "=== $node ==="
  ssh $node "crictl ps | grep kube-apiserver"
  ssh $node "curl -k https://127.0.0.1:6443/healthz"
done

# 步骤 2：检查负载均衡器健康检查
# 根据具体 LB 类型检查
# haproxy 示例：
echo "show stat" | socat unix-connect:/var/lib/haproxy/stats stdio

# nginx 示例：
curl http://localhost:8080/nginx_status

# 步骤 3：检查 VIP 状态（如使用 keepalived）
ip addr show | grep <vip>
systemctl status keepalived

# 步骤 4：如果某个实例问题，手动从 LB 摘除
# haproxy 示例：
echo "disable server kubernetes/master1" | socat unix-connect:/var/lib/haproxy/stats stdio

# 步骤 5：修复问题实例后重新加入
echo "enable server kubernetes/master1" | socat unix-connect:/var/lib/haproxy/stats stdio

# 步骤 6：验证集群状态
kubectl get nodes
kubectl get cs  # 已废弃但部分版本可用
kubectl get --raw /healthz
```
#### 3.6.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 摘除实例减少可用容量 | 确保剩余实例能承载负载 |
| **中** | LB 配置错误可能导致服务不可用 | 谨慎修改 LB 配置 |
| **低** | 状态检查无风险 | - |

#### 3.6.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 高可用集群至少保持 2 个 API Server 实例在线
2. 故障切换期间避免执行大规模变更操作
3. 修复问题实例前先确认数据一致性
4. LB 健康检查间隔建议不超过 10 秒
5. 考虑配置 API Server 的优雅终止时间
6. 定期演练故障切换流程
```

### 3.7 紧急恢复流程

#### 3.7.1 完全不可用时的恢复步骤

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 紧急恢复检查清单
# ==================

# 1. 确认所有 Master 节点可 SSH 登录
ssh master1 hostname

# 2. 检查系统基础服务
systemctl status kubelet
systemctl status containerd  # 或 docker

# 3. 检查 etcd 状态（最重要）
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 4. 如果 etcd 正常，尝试重启 API Server
# 静态 Pod 方式：
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
sleep 5
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
sleep 30

# 5. 如果仍无法启动，检查日志
crictl logs $(crictl ps -a -q --name kube-apiserver | head -1) 2>&1 | tail -100

# 6. 如果证书问题，紧急续签
kubeadm certs renew all
systemctl restart kubelet

# 7. 验证恢复
kubectl get nodes
kubectl get pods -A
```
#### 3.7.2 安全生产风险提示

```
⚠️  紧急恢复安全生产风险提示：
1. 【通知】立即通知相关团队和管理层
2. 【评估】评估业务影响范围
3. 【备份】任何操作前确认有 etcd 备份
4. 【记录】记录所有操作步骤和时间
5. 【验证】恢复后全面验证集群功能
6. 【复盘】故障恢复后进行根因分析
7. 【演练】定期进行故障恢复演练
```

---

## 附录

### A. API Server 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `apiserver_request_duration_seconds` | 请求延迟 | P99 > 1s |
| `apiserver_current_inflight_requests` | 当前并发请求数 | > max * 0.8 |
| `apiserver_request_total` | 请求总数 | 错误率 > 1% |
| `etcd_request_duration_seconds` | etcd 请求延迟 | P99 > 500ms |
| `process_resident_memory_bytes` | 内存使用 | > 节点内存 80% |

### B. 常见启动参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--max-requests-inflight` | 400 | 最大并发非变更请求数 |
| `--max-mutating-requests-inflight` | 200 | 最大并发变更请求数 |
| `--request-timeout` | 1m0s | 请求超时时间 |
| `--etcd-servers` | - | etcd 服务器地址 |
| `--secure-port` | 6443 | HTTPS 端口 |
| `--enable-admission-plugins` | - | 启用的准入控制器 |

### C. 相关文档链接

- [Kubernetes API Server 文档](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/)
- [API Priority and Fairness](https://kubernetes.io/docs/concepts/cluster-administration/flow-control/)
- [PKI 证书和要求](https://kubernetes.io/docs/setup/best-practices/certificates/)

---

## 📚 D. 生产环境实战案例精选

### 案例 1：大促期间 API Server QPS 骤增导致集群瘫痪

#### 🎯 问题场景
某电商公司在双十一大促期间，集群规模 1000+ 节点，运行 10000+ Pod。凌晨 0 点流量峰值时，所有 `kubectl` 命令超时，监控告警风暴，业务 Pod 无法扩容，损失预估数百万。

#### 🔍 排查过程
1. **初步发现**：监控显示 API Server CPU 达到 100%，内存接近 limit
   ```bash
   kubectl top pod -n kube-system | grep kube-apiserver
   # kube-apiserver-master1   3800m   7.5Gi
   ```

2. **指标分析**：
   ```bash
   curl -k https://127.0.0.1:6443/metrics | grep apiserver_current_inflight_requests
   # apiserver_current_inflight_requests{requestKind="readOnly"} 2500  # 远超默认限制400
   ```

3. **请求来源分析**：通过审计日志发现
   ```bash
   cat /var/log/kubernetes/audit/audit.log | jq -r '.user.username' | sort | uniq -c | sort -rn | head -10
   # 6500 system:serviceaccount:monitoring:prometheus
   # 3200 system:serviceaccount:ci-cd:jenkins
   ```
   **根因**：Prometheus 大规模 LIST 请求 + Jenkins CI 并发构建触发大量 Pod 创建请求。

#### ⚡ 应急措施
1. **立即限流关键来源**：
   ```bash
   # 临时降低 Prometheus 抓取频率
   kubectl -n monitoring scale deploy prometheus --replicas=1
   
   # 暂停非紧急 Jenkins Job
   kubectl -n ci-cd scale deploy jenkins --replicas=0
   ```

2. **扩容 API Server**：
   ```bash
   # 临时提高资源限制（静态 Pod）
   vim /etc/kubernetes/manifests/kube-apiserver.yaml
   # resources.limits.cpu: 8000m
   # resources.limits.memory: 16Gi
   
   # 增加并发限制
   # --max-requests-inflight=800
   # --max-mutating-requests-inflight=400
   ```

3. **5 分钟后恢复正常**，流量峰值平稳度过。

#### 🛡️ 长期优化
1. **APF 精细化配置**：
   ```yaml
   apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
   kind: FlowSchema
   metadata:
     name: monitoring-low-priority
   spec:
     priorityLevelConfiguration:
       name: catch-all  # 降低监控优先级
     matchingPrecedence: 8000
     rules:
     - subjects:
       - kind: ServiceAccount
         serviceAccount:
           name: prometheus
           namespace: monitoring
       resourceRules:
       - verbs: ["list", "watch"]
         apiGroups: ["*"]
         resources: ["*"]
   ```

2. **Prometheus 优化**：
   - 启用 `honor_timestamps: false` 减少精度
   - 增加抓取间隔至 30s
   - 使用 PodMonitor 代替 ServiceMonitor 减少 API 调用

3. **水平扩展 API Server**：从 3 节点扩至 5 节点，并启用 LB 智能路由。

#### 💡 经验总结
- **监控盲区**：未监控 API Server 的 QPS 与来源分布，无法提前预警
- **容量规划**：大促前未做压测与容量评估
- **优先级缺失**：所有请求平等对待，关键业务无保障
- **改进方向**：建立 API QPS 基线、定期压测、分级流控、提前扩容

---

### 案例 2：证书批量过期导致集群完全不可用

#### 🎯 问题场景
某金融公司生产集群，周一早上 8 点突然所有 `kubectl` 命令报 `x509: certificate has expired`，所有自动化运维中断，业务 Pod 无法重启，持续 2 小时才恢复。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   kubectl get nodes
   # Unable to connect to the server: x509: certificate has expired or is not yet valid
   ```

2. **证书检查**：
   ```bash
   kubeadm certs check-expiration
   # CERTIFICATE                EXPIRES                  RESIDUAL TIME
   # apiserver                 Dec 25, 2023 08:00 UTC   0d   ❌
   # apiserver-kubelet-client  Dec 25, 2023 08:00 UTC   0d   ❌
   ```

3. **根因分析**：
   - kubeadm 默认证书有效期 1 年
   - 未配置自动续签
   - 监控未覆盖证书到期时间
   - 正好在周末过期，未及时发现

#### ⚡ 紧急恢复
1. **登录 master 节点续签证书**：
   ```bash
   # 备份旧证书
   cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%s)
   
   # 续签所有证书
   kubeadm certs renew all
   # [renew] Reading configuration from the cluster...
   # certificate embedded in the kubeconfig file for the admin to use and for kubeadm itself renewed
   # certificate for serving the Kubernetes API renewed
   # ✅ Done
   ```

2. **重启关键组件**：
   ```bash
   # 重启 kubelet 使新证书生效
   systemctl restart kubelet
   
   # 重启 API Server（自动重启）
   mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
   sleep 10
   mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
   
   # 重启 controller-manager 和 scheduler
   kubectl -n kube-system delete pod -l component=kube-controller-manager
   kubectl -n kube-system delete pod -l component=kube-scheduler
   ```

3. **更新 kubeconfig**：
   ```bash
   # 更新管理员 kubeconfig
   cp /etc/kubernetes/admin.conf ~/.kube/config
   
   # 验证恢复
   kubectl get nodes
   # NAME    STATUS   ROLES           AGE   VERSION
   # master  Ready    control-plane   365d  v1.28.0
   ```

#### 🛡️ 长期防护
1. **自动化证书轮转**：
   ```bash
   # 配置 kubelet 证书自动轮转
   cat >> /var/lib/kubelet/config.yaml << EOF
   rotateCertificates: true
   serverTLSBootstrap: true
   EOF
   
   # 配置 API Server 自动批准 CSR
   kubectl create clusterrolebinding kubelet-csr-auto-approve \
     --clusterrole=system:certificates.k8s.io:certificatesigningrequests:selfnodeclient \
     --group=system:nodes
   ```

2. **监控告警**：
   ```yaml
   # Prometheus 告警规则
   - alert: CertificateExpiresSoon
     expr: (certmanager_certificate_expiration_timestamp_seconds - time()) / 86400 < 30
     labels:
       severity: warning
     annotations:
       summary: "证书将在 30 天内过期"
       description: "证书 {{ $labels.name }} 将在 {{ $value | humanizeDuration }} 后过期"
   ```

3. **定期演练**：每季度模拟证书过期问题，验证恢复流程。

#### 💡 经验总结
- **自动化缺失**：依赖手动续签，人为疏忽不可避免
- **监控盲区**：未监控证书到期时间
- **应急准备不足**：周末值班人员未掌握证书续签流程
- **改进方向**：自动化证书管理（[[cert-manager|cert-manager]]）、提前 60 天告警、定期演练

---

### 案例 3：etcd 慢查询拖垮 API Server

#### 🎯 问题场景
某互联网公司，集群规模 500 节点、5000 Pod，用户反馈 `kubectl get pods` 经常超时 30s+，但偶尔又能秒返，影响运维效率和问题响应速度。

#### 🔍 排查过程
1. **初步定位**：
   ```bash
   # API Server 指标正常
   curl -k https://127.0.0.1:6443/metrics | grep apiserver_request_duration
   # apiserver_request_duration_seconds{verb="GET",resource="pods"}...0.8  # P99 < 1s
   
   # 但 etcd 延迟异常
   curl -k https://127.0.0.1:6443/metrics | grep etcd_request_duration
   # etcd_request_duration_seconds{operation="get",type="range"}...15.2  # P99 > 15s ❌
   ```

2. **etcd 诊断**：
   ```bash
   # 检查 etcd 数据库大小
   ETCDCTL_API=3 etcdctl endpoint status --write-out=table
   # +------------------+------------------+---------+---------+-----------+
   # |     ENDPOINT     |        ID        | VERSION | DB SIZE | IS LEADER |
   # +------------------+------------------+---------+---------+-----------+
   # | 127.0.0.1:2379   | 8e9e05c52164694d | 3.5.9   | 8.2 GB  | true      |  # ❌ 超大！
   # +------------------+------------------+---------+---------+-----------+
   
   # 检查磁盘性能
   fio --name=etcd-bench --rw=write --bs=4k --size=1G --direct=1
   # write: IOPS=2500, BW=10MB/s  # ❌ 远低于推荐 3000+ IOPS
   ```

3. **根因分析**：
   - etcd 数据库超过 8GB（推荐 < 2GB）
   - 运行在机械硬盘上，IOPS 不足
   - 未定期压缩（compaction）和碎片整理（defragment）
   - 大量 Event 对象未清理，占用空间

#### ⚡ 应急优化
1. **立即压缩和整理**：
   ```bash
   # 获取当前版本
   rev=$(ETCDCTL_API=3 etcdctl endpoint status --write-out=json | jq -r '.[] | .Status.header.revision')
   
   # 压缩历史版本
   ETCDCTL_API=3 etcdctl compact $rev
   # compacted revision 123456
   
   # 整理碎片（注意：会短暂阻塞）
   ETCDCTL_API=3 etcdctl defrag
   # Finished defragmenting etcd member[127.0.0.1:2379]
   
   # 验证
   ETCDCTL_API=3 etcdctl endpoint status --write-out=table
   # DB SIZE: 1.8 GB  ✅ 大幅减少
   ```

2. **清理 Event 对象**：
   ```bash
   # Event 对象默认保留 1 小时，但可能堆积
   kubectl get events -A --sort-by='.lastTimestamp' | tail -100
   
   # 调整 API Server 参数（降低 Event TTL）
   # --event-ttl=30m  # 默认 1h
   ```

3. **10 分钟后性能恢复**：
   ```bash
   # 再次测试
   time kubectl get pods -A | wc -l
   # 5234 pods
   # real    0m1.2s  ✅ 恢复正常
   ```

#### 🛡️ 长期优化
1. **定时压缩任务**：
   ```bash
   # CronJob 每天凌晨压缩和整理
   cat << EOF | kubectl apply -f -
   apiVersion: batch/v1
   kind: CronJob
   metadata:
     name: etcd-maintenance
     namespace: kube-system
   spec:
     schedule: "0 2 * * *"
     jobTemplate:
       spec:
         template:
           spec:
             containers:
             - name: etcd-compact
               image: quay.io/coreos/etcd:v3.5.9
               command:
               - /bin/sh
               - -c
               - |
                 rev=\$(etcdctl endpoint status --write-out=json | jq -r '.[].Status.header.revision')
                 etcdctl compact \$rev
                 etcdctl defrag
               env:
               - name: ETCDCTL_API
                 value: "3"
             restartPolicy: OnFailure
   EOF
   ```

2. **迁移至 SSD**：
   - 评估：机械硬盘 IOPS 2500，SSD IOPS 10000+
   - 迁移：使用 etcd 快照恢复至 SSD 节点
   - 效果：P99 延迟从 15s 降至 200ms

3. **监控告警**：
   ```promql
   # etcd 数据库大小告警
   etcd_mvcc_db_total_size_in_bytes > 2 * 1024 * 1024 * 1024  # > 2GB
   
   # etcd 慢请求告警
   histogram_quantile(0.99, etcd_disk_wal_fsync_duration_seconds_bucket) > 0.1  # > 100ms
   ```

#### 💡 经验总结
- **容量规划失误**：未考虑 etcd 存储增长与性能要求
- **维护缺失**：未定期压缩和整理，数据库膨胀
- **硬件选型错误**：etcd 对磁盘 IOPS 极度敏感，机械硬盘不适用
- **改进方向**：自动化维护、SSD 存储、容量监控、定期备份

---

### 案例 4：Webhook 自杀效应导致集群无法操作

#### 🎯 问题场景
某科技公司部署了一个自研的准入控制 Webhook，用于校验 Pod 镜像来源。某天 Webhook Pod 因 OOM 崩溃，之后所有 `kubectl apply` 都失败，甚至无法删除该 Webhook 配置本身，陷入"死锁"。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   kubectl apply -f deployment.yaml
   # Error from server (InternalError): Internal error occurred: failed calling webhook "validate.pod.com": Post "https://pod-validator.default.svc:443/validate": dial tcp 10.96.100.200:443: connect: connection refused
   
   # 尝试删除 Webhook 配置也失败！
   kubectl delete validatingwebhookconfiguration pod-validator
   # Error from server (InternalError): Internal error occurred: failed calling webhook "validate.pod.com": ...
   ```

2. **根因分析**：
   - ValidatingWebhookConfiguration 的 `failurePolicy: Fail`（失败即拒绝）
   - Webhook Pod OOM 后无法响应
   - Webhook 规则匹配 `*/*`（所有资源），包括自身的删除操作
   - 形成"自杀效应"：无法删除 Webhook 配置 → 无法恢复服务

#### ⚡ 紧急恢复
1. **跳过 Webhook 直接修改 API Server**（高风险操作）：
   ```bash
   # 方案 1：临时禁用 Webhook 准入控制（需重启 API Server）
   vim /etc/kubernetes/manifests/kube-apiserver.yaml
   # 移除 ValidatingAdmissionWebhook 插件
   # --enable-admission-plugins=...,ValidatingAdmissionWebhook,...
   #                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 删除
   
   # API Server 会自动重启
   sleep 30
   kubectl get nodes  # 验证恢复
   
   # 删除 Webhook 配置
   kubectl delete validatingwebhookconfiguration pod-validator
   # validatingwebhookconfiguration.admissionregistration.k8s.io "pod-validator" deleted ✅
   
   # 恢复 API Server 配置（重新启用 ValidatingAdmissionWebhook）
   vim /etc/kubernetes/manifests/kube-apiserver.yaml
   # --enable-admission-plugins=...,ValidatingAdmissionWebhook,...
   ```

2. **方案 2：直接操作 etcd（更高风险）**：
   ```bash
   # 列出所有 ValidatingWebhookConfiguration
   ETCDCTL_API=3 etcdctl get /registry/admissionregistration.k8s.io/validatingwebhookconfigurations/ --prefix --keys-only
   
   # 删除问题配置
   ETCDCTL_API=3 etcdctl del /registry/admissionregistration.k8s.io/validatingwebhookconfigurations/pod-validator
   
   # ⚠️ 风险：直接操作 etcd 跳过 API Server 校验，可能导致数据不一致
   ```

3. **修复 Webhook Pod**：
   ```bash
   # 提高资源限制，防止 OOM
   kubectl -n default set resources deployment pod-validator --limits=memory=512Mi
   kubectl -n default rollout status deployment pod-validator
   ```

#### 🛡️ 最佳实践
1. **防御性 Webhook 配置**：
   ```yaml
   apiVersion: admissionregistration.k8s.io/v1
   kind: ValidatingWebhookConfiguration
   metadata:
     name: pod-validator
   webhooks:
   - name: validate.pod.com
     failurePolicy: Ignore  # ✅ 失败时忽略，而非拒绝
     timeoutSeconds: 5      # ✅ 设置超时，避免长时间阻塞
     namespaceSelector:     # ✅ 排除关键命名空间
       matchExpressions:
       - key: kubernetes.io/metadata.name
         operator: NotIn
         values: ["kube-system", "default"]
     rules:
     - operations: ["CREATE"]
       apiGroups: [""]
       apiVersions: ["v1"]
       resources: ["pods"]
       scope: "Namespaced"
   ```

2. **健康检查与熔断**：
   - Webhook 服务配置 Liveness/Readiness 探针
   - 启用 HPA 自动扩容
   - 设置 PDB 防止意外全部下线

3. **应急预案**：
   - 文档化跳过 Webhook 的恢复流程
   - 定期演练 Webhook 问题场景
   - 准备备用管理员 kubeconfig（绕过 Webhook）

#### 💡 经验总结
- **配置不当**：`failurePolicy: Fail` + 规则范围过大 = 灾难
- **单点问题**：Webhook 服务无高可用保障
- **测试不足**：未模拟 Webhook 不可用场景
- **改进方向**：防御性配置、高可用部署、定期演练、监控告警

## Related

- [[生态参考/topic-index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
