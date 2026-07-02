---
title: kubectl debug 深度指南
description: 'kubectl debug 命令详解：ephemeral 容器注入、节点调试、Pod 复制调试、Profile 调试及生产故障排查工作流'
summary: 'kubectl debug 命令详解：ephemeral 容器注入、节点调试、Pod 复制调试、Profile 调试及生产故障排查工作流'
category: cluster-fundamentals
tags:
- kubectl-debug
- ephemeral-container
- node-debug
- troubleshooting
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- kubectl debug 是什么
- 如何使用 kubectl debug 调试 Pod
- 如何调试 distroless 镜像
trigger_keywords:
- kubectl debug
- ephemeral container
- node debug
- distroless 调试
prerequisites:
- kubectl-basics
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


# kubectl debug 深度指南

## 概述

`kubectl debug`（Kubernetes 1.25+ GA）是官方提供的运行时调试工具，解决了生产环境中无法 `exec` 进入容器的核心痛点。它支持四种调试模式：ephemeral 容器注入、节点调试、Pod 复制调试、Profile 调试。

```
调试模式选择决策树:

目标是什么？
  ├─ 运行中的 Pod 内部排查 → ephemeral 容器注入
  ├─ distroless / scratch 镜像无 shell → ephemeral 容器注入
  ├─ 节点级问题（kubelet/内核/磁盘）→ 节点调试
  ├─ 需要修改 Pod 配置再调试 → Pod 复制调试
  └─ 性能分析（CPU/Memory profiling）→ Profile 调试
```

## 1. Ephemeral 容器注入

### 1.1 基本用法

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 向运行中的 Pod 注入调试容器（最常用）
kubectl debug -it pod/my-app \
  --image=busybox:1.36 \
  --target=my-app \
  -n production

# 注入并指定容器名
kubectl debug -it pod/my-app \
  --image=nicolaka/netshoot:latest \
  --container=debugger \
  --target=my-app \
  -n production

# 向 Deployment 注入（不影响原 Pod）
kubectl debug deployment/my-app \
  --image=busybox:1.36 \
  --target=my-app \
  -n production
```
### 1.2 调试 distroless 镜像

distroless 镜像没有 shell、没有包管理器，`kubectl exec` 完全无法使用。ephemeral 容器共享进程命名空间，可以直接查看目标容器的文件系统。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 典型场景：Go 微服务使用 distroless 镜像
# 无法 exec，无法安装工具，只能注入 ephemeral 容器

# 第一步：注入带完整工具链的调试容器
kubectl debug -it pod/go-service-7b8f9-x2k4l \
  --image=alpine:3.19 \
  --target=go-service \
  --profile=general \
  -n backend

# 第二步：在调试容器内访问目标容器的文件系统
# /proc/1/root/ 指向目标容器的根文件系统
ls /proc/1/root/
cat /proc/1/root/etc/os-release

# 第三步：检查目标进程
ps aux                # 可以看到目标容器的进程
ls -la /proc/1/fd/   # 查看文件描述符（排查文件泄漏）
cat /proc/1/status   # 查看进程状态和内存限制

# 第四步：网络排查（共享网络命名空间）
wget -qO- http://localhost:8080/healthz
netstat -tlnp
ss -tlnp

# 第五步：DNS 排查
nslookup kubernetes.default.svc.cluster.local
nslookup my-service.backend.svc.cluster.local
```
### 1.3 调试镜像推荐

| 场景 | 推荐镜像 | 包含工具 |
|------|----------|---------|
| 通用调试 | `alpine:3.19` | sh, wget, curl, nslookup |
| 网络排查 | `nicolaka/netshoot:latest` | tcpdump, iperf3, dig, mtr, strace |
| Go 服务 | `golang:1.22-alpine` | go tool pprof, dlv (delve) |
| Java 服务 | `eclipse-temurin:21-jdk` | jcmd, jstack, jmap, jfr |
| 系统排查 | `ubuntu:22.04` | apt, strace, lsof, pmap |
| 极简调试 | `busybox:1.36` | sh, wget, nc, vi |

### 1.4 安全限制

```yaml
# ephemeral 容器受限于 Pod 的 SecurityContext
# 如果 Pod 设置了 runAsNonRoot，ephemeral 容器也必须遵守
# 解决方案：创建专用的 debug Pod（见第3节）

# 查看当前 Pod 的安全上下文
kubectl get pod my-app -o jsonpath='{.spec.securityContext}'
kubectl get pod my-app -o jsonpath='{.spec.containers[0].securityContext}'
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果注入失败，检查以下常见原因:

# 1. Pod 已终止（ephemeral 容器只能注入到运行中的 Pod）
kubectl get pod my-app -o wide

# 2. EphemeralContainers 特性未启用（1.22 以下）
kubectl get --raw /metrics | grep ephemeral

# 3. Pod Security Standards 限制（PSS/PSA）
kubectl get ns backend -o jsonpath='{.metadata.labels}'

# 4. 容器运行时不支持（containerd >= 1.5 均支持）
kubectl get nodes -o wide
```
## 2. 节点调试（Node Shell）

### 2.1 基本用法

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在节点上启动一个带 chroot 的调试 Pod
# 这会在节点上创建一个 Pod，chroot 到节点的根文件系统
kubectl debug node/worker-1 -it --image=ubuntu:22.04

# 使用更轻量的镜像
kubectl debug node/worker-1 -it --image=alpine:3.19

# 指定命名空间（调试 Pod 默认创建在 default）
kubectl debug node/worker-1 -it --image=ubuntu:22.04 -n kube-system
```
### 2.2 节点调试实战

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 进入节点 shell 后的常用排查命令

# 检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago" --no-pager

# 检查容器运行时
crictl ps
crictl logs <container-id>
crictl inspect <container-id>

# 检查磁盘使用
df -h
du -sh /var/lib/containerd/*
du -sh /var/log/pods/*

# 检查网络
ip addr show
ip route show
iptables -t nat -L -n | head -20

# 检查内核日志
dmesg | tail -50
dmesg | grep -i "oom\|error\|panic"

# 检查系统资源
cat /proc/meminfo
cat /proc/cpuinfo | grep "model name" | head -1
top -bn1 | head -20
```
### 2.3 清理节点调试 Pod

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
# 节点调试完成后务必清理
kubectl get pods -l app.kubernetes.io/created-by=kubectl-debug
kubectl delete pod node-debugger-worker-1-xxxxx

# 如果忘记清理，可以通过标签批量清理
kubectl delete pods -l app.kubernetes.io/created-by=kubectl-debug --all-namespaces
```
## 3. Pod 复制调试

### 3.1 复制并修改配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 复制一个 Pod 并添加调试工具
kubectl debug my-app-7b8f9-x2k4l -it \
  --copy-to=my-app-debug \
  --container=my-app \
  --image=ubuntu:22.04 \
  -n production

# 复制 Pod 并修改启动命令
kubectl debug my-app-7b8f9-x2k4l -it \
  --copy-to=my-app-debug \
  --container=my-app \
  --image=ubuntu:22.04 \
  -- sh -c "sleep infinity"

# 复制 Pod 并设置环境变量（覆盖原始配置）
kubectl debug my-app-7b8f9-x2k4l -it \
  --copy-to=my-app-debug \
  --container=my-app \
  --image=ubuntu:22.04 \
  --env="LOG_LEVEL=debug" \
  -n production
```
### 3.2 保留 Pod 在节点上

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 默认情况下，复制的 Pod 会调度到同一个节点
# 使用 --same-node=false 允许调度到其他节点
kubectl debug my-app-7b8f9-x2k4l -it \
  --copy-to=my-app-debug \
  --image=ubuntu:22.04 \
  --same-node=false \
  -n production

# 查看复制 Pod 的调度结果
kubectl get pod my-app-debug -o wide -n production
```
## 4. Profile 调试

### 4.1 内置 Profile 说明

| Profile | 说明 | 适用场景 |
|---------|------|---------|
| `legacy` | 完全兼容 kubectl exec | 默认行为 |
| `general` | 添加基础调试工具 | 通用排查 |
| `baseline` | 符合 Pod Security Standards | 安全敏感环境 |
| `restricted` | 最严格安全限制 | 高安全要求环境 |
| `sysadmin` | 添加系统管理工具 | 节点级排查 |
| `netadmin` | 添加网络管理工具 | 网络排查 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 sysadmin profile（需要特权）
kubectl debug -it pod/my-app \
  --image=ubuntu:22.04 \
  --target=my-app \
  --profile=sysadmin \
  -n production

# 使用 netadmin profile（网络调试）
kubectl debug -it pod/my-app \
  --image=nicolaka/netshoot:latest \
  --target=my-app \
  --profile=netadmin \
  -n production
```
### 4.2 自定义 Profile

```yaml
# 在 kube-apiserver 的配置中自定义 Profile
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - command:
    - kube-apiserver
    - --feature-gates=EphemeralContainers=true
    # 自定义 debug profile 需要 KEP-4292 支持（1.31+）
```

## 5. 故障排查工作流

### 5.1 通用排查流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
故障排查工作流（kubectl debug 驱动）:

Step 1: 确定故障 Pod
  kubectl get pods -n <ns> | grep -E "Error|CrashLoop|Pending"
  kubectl describe pod <pod> -n <ns>
  kubectl logs <pod> -n <ns> --tail=50

Step 2: 判断调试模式
  ├─ Pod 正在运行但行为异常 → ephemeral 容器注入
  ├─ Pod CrashLoopBackOff → 复制 Pod 调试
  ├─ Pod Pending → 检查调度事件 + 节点调试
  └─ distroless 镜像 → ephemeral 容器注入（唯一选择）

Step 3: 注入调试容器
  kubectl debug -it pod/<pod> --image=netshoot --target=<container> -n <ns>

Step 4: 在调试容器内排查
  ├─ 进程排查: ps aux, top, /proc/<pid>/status
  ├─ 网络排查: ss, curl, nslookup, tcpdump
  ├─ 文件系统: ls, du, lsof, /proc/<pid>/fd/
  ├─ 内存排查: pmap, /proc/<pid>/smaps, free
  └─ DNS 排查: nslookup, dig, cat /etc/resolv.conf

Step 5: 清理
  kubectl delete pod <debug-pod> -n <ns>
```
### 5.2 排查命令速查表

```bash
# ===== 进程排查 =====
# 查看目标容器进程
ps aux

# 查看进程打开的文件描述符（排查 FD 泄漏）
ls -la /proc/1/fd/ | wc -l
ls -la /proc/1/fd/ | grep -v "^total"

# 查看进程的环境变量
cat /proc/1/environ | tr '\0' '\n'

# 查看进程的 cgroup 内存限制
cat /proc/1/cgroup
cat /sys/fs/cgroup/memory/memory.limit_in_bytes 2>/dev/null

# ===== 网络排查 =====
# 检查监听端口
ss -tlnp

# 测试内部服务连通性
curl -v http://service-name.namespace.svc.cluster.local:8080/healthz

# 测试外部连通性
curl -v https://api.external-service.com

# DNS 解析测试
nslookup kubernetes.default.svc.cluster.local
nslookup my-service.backend.svc.cluster.local

# 抓包（需要 netshoot 镜像）
tcpdump -i eth0 -nn -c 100 port 8080

# ===== 文件系统排查 =====
# 检查磁盘使用
df -h /tmp
du -sh /tmp/* | sort -rh | head -10

# 检查挂载点
mount | grep -E "overlay|tmpfs|nfs"

# 查找大文件
find / -type f -size +100M 2>/dev/null

# ===== 内存排查 =====
# 查看进程内存映射
pmap -x 1

# 查看详细内存使用
cat /proc/1/smaps_rollup

# 查看 cgroup 内存使用
cat /sys/fs/cgroup/memory/memory.usage_in_bytes 2>/dev/null
cat /sys/fs/cgroup/memory/memory.stat 2>/dev/null
```

### 5.3 生产环境注意事项

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ⚠️ 生产环境调试原则

# 1. 优先使用只命操作
kubectl debug -it pod/my-app --image=busybox:1.36 --target=my-app
# 避免在调试容器内执行写操作

# 2. 避免长时间占用 ephemeral 容器
# 调试完成后立即退出（exit 或 Ctrl+D）
# ephemeral 容器不会自动清理，需手动删除 Pod 重建

# 3. 记录调试过程
# 使用 script 命令记录终端输出
script /tmp/debug-session.log
# 或使用 tee 管道
kubectl debug ... 2>&1 | tee /tmp/debug.log

# 4. 权限最小化
# 优先使用 restricted 或 baseline profile
# 仅在必要时使用 sysadmin/netadmin

# 5. 避免影响在线流量
# 使用复制模式调试，避免直接操作生产 Pod
kubectl debug my-app-xxx --copy-to=debug-app --image=ubuntu:22.04
```
## 6. 常见问题与解决方案

### 6.1 调试容器注入失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 错误: error: ephemeral containers are disabled
# 原因: K8s < 1.23 未启用 EphemeralContainers 特性门控
# 解决: 升级 K8s 或手动启用 --feature-gates=EphemeralContainers=true

# 错误: the server rejected the request
# 原因: Pod Security Admission 限制
# 解决: 使用 baseline/restricted profile 或调整 namespace PSA 标签

# 错误: container <name> is not valid for pod
# 原因: --target 指定了错误的容器名
# 解决: kubectl get pod <pod> -o jsonpath='{.spec.containers[*].name}'

# 错误: couldn't find current container
# 原因: Pod 已终止或容器已重启
# 解决: 确认 Pod 状态为 Running，使用正在运行的容器名
```
### 6.2 调试容器内工具缺失

```bash
# busybox 镜像功能有限，推荐使用更完整的镜像:
# 网络: nicolaka/netshoot:latest
# 通用: ubuntu:22.04
# Go:   golang:1.22-alpine
# Java: eclipse-temurin:21-jdk

# 在 ephemeral 容器内安装临时工具（非 distroless 场景）
# Alpine:
apk add --no-cache tcpdump strace curl bind-tools

# Ubuntu:
apt-get update && apt-get install -y tcpdump strace curl dnsutils
```

### 6.3 与 exec 的对比

| 特性 | kubectl exec | kubectl debug |
|------|-------------|---------------|
| 需要 shell | 是 | 否（注入新容器） |
| distroless 支持 | 否 | 是 |
| 共享进程空间 | N/A | 是（--target） |
| 共享网络空间 | 是 | 是 |
| 安全限制 | 受容器 SecurityContext 限制 | 受 Pod SecurityContext 限制 |
| 影响原容器 | 否 | 否（ephemeral） |
| K8s 版本要求 | 所有版本 | 1.25+ GA |

---

## Related

- [[domain-01-cluster-fundamentals/05-kubectl/05-kubectl-commands-reference|kubectl 命令参考]]
- [[domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting|高级故障排查]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|K8s 速查卡]]

## See Also

- [Kubernetes 官方文档: Debug Running Pods](https://kubernetes.io/docs/tasks/debug/debug-application/debug-running-pod/)
- [KEP-277: Ephemeral Containers](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/277-ephemeral-containers)


<!-- risk-assessed -->
