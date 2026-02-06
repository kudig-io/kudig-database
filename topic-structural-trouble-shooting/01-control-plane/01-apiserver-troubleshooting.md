# API Server 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

本文档基于生产环境真实故障案例编写，提供：
- **系统性排查方法**：从现象到根因的完整排查路径
- **实战经验总结**：来自大型互联网公司的运维实践
- **风险控制指导**：安全生产的操作规范和应急预案
- **性能优化建议**：高负载场景下的调优方案

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

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
| 负载均衡器故障 | `502 Bad Gateway` | Load Balancer | 客户端响应 |
| API Server 内存溢出 | `OOMKilled` | Kubernetes | Pod 状态或日志 |
| 请求队列积压 | `context deadline exceeded` | 客户端 | kubectl 命令输出 |

#### 1.1.2 API Server 响应缓慢

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 请求超时 | `context deadline exceeded` | kubectl/客户端 | 命令行输出 |
| 请求延迟高 | `request latency exceeded threshold` | API Server 日志 | `journalctl -u kube-apiserver` |
| 限流触发 | `429 Too Many Requests` | API Server | 客户端响应码 |
| 优先级调度延迟 | `request is being throttled by APF` | API Server 日志 | API Server 日志 |

#### 1.1.3 API Server 间歇性故障

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

```bash
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
| **故障自愈** | 失效 | 节点故障后 Pod 无法重新调度 |
| **证书轮转** | 中断 | 证书到期后无法自动更新 |
| **审计日志** | 丢失 | 无法记录 API 操作审计日志 |

#### 1.3.3 影响严重程度评估

##### 业务连续性影响矩阵

| 故障类型 | RTO(恢复时间目标) | RPO(数据丢失目标) | 业务影响等级 | 处理优先级 |
|----------|-------------------|-------------------|--------------|------------|
| **完全不可用** | &lt; 5分钟 | 0 | P0-紧急 | 立即处理 |
| **部分功能受限** | &lt; 30分钟 | 0 | P1-高 | 快速响应 |
| **性能下降** | &lt; 2小时 | 0 | P2-中 | 计划处理 |
| **偶发性问题** | &lt; 24小时 | 0 | P3-低 | 持续观察 |

##### 核心业务依赖关系图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    API Server 故障影响传播链                                  │
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

1. **进程层面**：API Server 进程是否正常运行
2. **网络层面**：网络连通性、证书、端口绑定
3. **存储层面**：etcd 连接和数据存储
4. **资源层面**：CPU、内存、文件描述符等资源
5. **配置层面**：启动参数、特性门控、准入控制器

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

```bash
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

```bash
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

```bash
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

---

## 3. 解决方案与风险控制

### 3.1 API Server 进程未运行

#### 3.1.1 解决步骤

```bash
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

```bash
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

```bash
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

### 3.3 etcd 连接故障

#### 3.3.1 解决步骤

```bash
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

```bash
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

```bash
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

```bash
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

# 步骤 4：如果某个实例故障，手动从 LB 摘除
# haproxy 示例：
echo "disable server kubernetes/master1" | socat unix-connect:/var/lib/haproxy/stats stdio

# 步骤 5：修复故障实例后重新加入
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
3. 修复故障实例前先确认数据一致性
4. LB 健康检查间隔建议不超过 10 秒
5. 考虑配置 API Server 的优雅终止时间
6. 定期演练故障切换流程
```

### 3.7 紧急恢复流程

#### 3.7.1 完全不可用时的恢复步骤

```bash
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
