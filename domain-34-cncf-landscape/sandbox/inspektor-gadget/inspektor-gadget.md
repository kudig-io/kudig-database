---
title: Inspektor Gadget
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- mysql
- daemonset
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Inspektor Gadget 是什么
- 如何 Inspektor Gadget
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Inspektor
- Gadget
- cncf
- landscape
---


# Inspektor Gadget

> **成熟度**: Sandbox | **加入时间**: 2022-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.inspektor-gadget.io |
| **GitHub** | https://github.com/inspektor-gadget/inspektor-gadget |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, C (eBPF) |
| **CNCF 分类** | Observability & Analysis |
| **适用场景** | Kubernetes 调试和检查 |

---

## 项目概述

Inspektor Gadget 是一组基于 eBPF 的工具集合 ("gadgets")，用于调试和检查 Kubernetes 集群中的应用程序。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 sidecar。

---

## 核心特性

- **eBPF 驱动**: 利用 eBPF 实现低开销的内核级可观测性
- **Kubernetes 感知**: 自动关联容器和 Pod 元数据
- **多种 Gadgets**: 网络、进程、文件系统、安全等多领域工具
- **本地和远程**: 支持 kubectl 插件和独立 CLI
- **可编程**: 支持自定义 eBPF 程序
- **跨平台**: 支持 Linux 内核 5.4+

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                 Inspektor Gadget Architecture                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      User Interface                       │   │
│  │  ┌─────────────────┐  ┌────────────────────────────────┐ │   │
│  │  │ kubectl gadget  │  │      ig (standalone CLI)       │ │   │
│  │  │ (K8s plugin)    │  │                                │ │   │
│  │  └────────┬────────┘  └───────────────┬────────────────┘ │   │
│  └───────────┼───────────────────────────┼─────────────────┘   │
│              │                           │                      │
│  ┌───────────▼───────────────────────────▼─────────────────┐   │
│  │                  Gadget Daemon (DaemonSet)               │   │
│  │  ┌─────────────────────────────────────────────────────┐│   │
│  │  │                Gadget Runtime                        ││   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  ││   │
│  │  │  │   Gadget    │  │   Tracer    │  │   Event    │  ││   │
│  │  │  │   Loader    │  │   Manager   │  │  Enricher  │  ││   │
│  │  │  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  ││   │
│  │  │         │                │               │          ││   │
│  │  │  ┌──────▼────────────────▼───────────────▼──────┐  ││   │
│  │  │  │              eBPF Programs                    │  ││   │
│  │  │  │  ┌─────────┐ ┌─────────┐ ┌─────────────────┐ │  ││   │
│  │  │  │  │ Kprobes │ │Tracepoints│ │    tc/XDP      │ │  ││   │
│  │  │  │  └─────────┘ └─────────┘ └─────────────────┘ │  ││   │
│  │  │  └──────────────────────────────────────────────┘  ││   │
│  │  └─────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                    Linux Kernel                          │   │
│  │  ┌─────────────────────────────────────────────────────┐│   │
│  │  │                 eBPF Virtual Machine                 ││   │
│  │  │  ┌──────────┐  ┌──────────┐  ┌───────────────────┐ ││   │
│  │  │  │ Network  │  │ Process  │  │   Filesystem      │ ││   │
│  │  │  │ Hooks    │  │ Hooks    │  │   Hooks           │ ││   │
│  │  │  └──────────┘  └──────────┘  └───────────────────┘ ││   │
│  │  └─────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 Kubernetes Metadata                       │   │
│  │  Pod Name │ Namespace │ Container │ Node │ Labels        │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **kubectl gadget** | Kubernetes 集成的 kubectl 插件 |
| **ig CLI** | 独立 CLI，可在任何 Linux 系统使用 |
| **Gadget Daemon** | 运行在每个节点的 DaemonSet |
| **eBPF Programs** | 内核级数据收集程序 |
| **Event Enricher** | 将事件与 K8s 元数据关联 |

---

## 快速开始

### 安装 kubectl gadget 插件

```bash
# 使用 krew 安装
kubectl krew install gadget

# 或直接下载
ARCH=$(uname -m | sed 's/x86_64/amd64/' | sed 's/aarch64/arm64/')
curl -LO https://github.com/inspektor-gadget/inspektor-gadget/releases/latest/download/kubectl-gadget-linux-${ARCH}.tar.gz
tar -xzf kubectl-gadget-linux-${ARCH}.tar.gz
sudo mv kubectl-gadget /usr/local/bin/
```

### 部署到集群

```bash
# 部署 Gadget DaemonSet
kubectl gadget deploy

# 验证部署
kubectl get pods -n gadget

# 卸载
kubectl gadget undeploy
```

### 独立 CLI (ig)

```bash
# 安装 ig
curl -LO https://github.com/inspektor-gadget/inspektor-gadget/releases/latest/download/ig-linux-amd64.tar.gz
tar -xzf ig-linux-amd64.tar.gz
sudo mv ig /usr/local/bin/

# 直接在主机上使用
sudo ig trace exec
```

---

## 常用 Gadgets

### 进程追踪

```bash
# 追踪进程执行
kubectl gadget trace exec -n default

# 输出示例:
# RUNTIME.CONTAINERNAME  PID    PPID   COMM        ARGS
# nginx                  12345  1      nginx       nginx: worker process
# myapp                  12346  1      python      python app.py

# 追踪特定 Pod
kubectl gadget trace exec -n production --podname myapp-xxx

# 追踪进程信号
kubectl gadget trace signal -n default
```

### 网络追踪

```bash
# 追踪 TCP 连接
kubectl gadget trace tcp -n default

# 追踪 DNS 请求
kubectl gadget trace dns -n default

# 输出示例:
# RUNTIME.CONTAINERNAME  PID    ID      QTYPE  QNAME             RCODE
# nginx                  12345  0x1234  A      api.example.com   NoError

# 追踪网络绑定
kubectl gadget trace bind -n default

# 追踪 TCP 连接建立
kubectl gadget trace tcpconnect -n production
```

### 文件系统追踪

```bash
# 追踪文件打开
kubectl gadget trace open -n default

# 追踪文件读写
kubectl gadget trace fsslower -n default --min-latency 10

# 输出示例:
# RUNTIME.CONTAINERNAME  PID    COMM     T  BYTES   OFF     LAT(ms)  FILE
# mysql                  12345  mysqld   R  4096    0       15.2     ibdata1
```

### 安全检测

```bash
# 追踪能力 (capabilities) 检查
kubectl gadget trace capabilities -n default

# 追踪 seccomp 违规
kubectl gadget trace seccomp -n default

# 追踪 OOM Kill 事件
kubectl gadget trace oomkill -n default

# 追踪 SUID 程序执行
kubectl gadget trace sni -n default
```

---

## 高级功能

### Top 命令 (实时统计)

```bash
# 文件使用排行
kubectl gadget top file -n default

# 网络 I/O 排行
kubectl gadget top tcp -n default

# 块 I/O 排行
kubectl gadget top block-io -n default

# 输出示例:
# RUNTIME.CONTAINERNAME  PID    COMM    READS  WRITES  RBYTES   WBYTES
# mysql                  12345  mysqld  150    85      614400   348160
```

### Snapshot 命令 (状态快照)

```bash
# 列出进程
kubectl gadget snapshot process -n default

# 列出套接字
kubectl gadget snapshot socket -n default

# 输出示例:
# RUNTIME.CONTAINERNAME  PROTOCOL  LOCAL            REMOTE          STATUS
# nginx                  TCP       0.0.0.0:80       0.0.0.0:0       LISTEN
# myapp                  TCP       10.0.0.5:35642   10.0.0.10:5432  ESTABLISHED
```

### Profile 命令 (性能分析)

```bash
# CPU 分析
kubectl gadget profile cpu -n default -K

# 块 I/O 分析
kubectl gadget profile block-io -n default
```

---

## 过滤选项

```bash
# 按命名空间过滤
kubectl gadget trace exec -n production

# 按 Pod 名称过滤
kubectl gadget trace exec --podname myapp-xxx

# 按标签过滤
kubectl gadget trace exec -l app=nginx

# 按容器名称过滤
kubectl gadget trace exec --containername nginx

# 按节点过滤
kubectl gadget trace exec --node worker-1

# 组合过滤
kubectl gadget trace exec -n production -l app=backend --node worker-1
```

---

## 输出格式

```bash
# JSON 格式输出
kubectl gadget trace exec -n default -o json

# YAML 格式输出
kubectl gadget trace exec -n default -o yaml

# 列格式输出 (默认)
kubectl gadget trace exec -n default -o columns

# 自定义列
kubectl gadget trace exec -n default -o columns=runtime.containername,pid,comm,args
```

---

## 自定义 Gadget

### 使用自定义 eBPF 程序

```bash
# 运行自定义 gadget
kubectl gadget run ghcr.io/my-org/my-gadget:latest

# 从本地运行
ig run ./my-gadget.bpf.o
```

### Gadget 配置

```yaml
# gadget.yaml
apiVersion: gadget.kinvolk.io/v1alpha1
kind: Trace
metadata:
  name: custom-trace
  namespace: gadget
spec:
  gadget: trace_exec
  filter:
    namespace: production
    podname: myapp-*
  output:
    - name: stdout
```

---

## 与其他工具集成

### Prometheus 集成

```bash
# 导出指标到 Prometheus
kubectl gadget prometheus -n monitoring \
  --metrics-bind-address :9090
```

### 输出到文件

```bash
# 保存追踪结果
kubectl gadget trace exec -n default -o json > trace.json

# 持续追踪并保存
kubectl gadget trace exec -n default -o json --timeout 60 > trace.json
```

---

## 最佳实践

1. **资源开销**: eBPF 程序运行在内核空间，开销极低
2. **权限控制**: Gadget DaemonSet 需要特权权限
3. **过滤优化**: 使用过滤器减少数据量
4. **安全审计**: 使用 capabilities 和 seccomp 追踪进行安全审计
5. **故障排查**: 结合多个 gadget 进行综合分析
6. **内核版本**: 确保内核版本 >= 5.4

---

## 参考资源

- [官方文档](https://www.inspektor-gadget.io/docs/)
- [GitHub Repo](https://github.com/inspektor-gadget/inspektor-gadget)
- [Gadget 列表](https://www.inspektor-gadget.io/docs/gadgets/)
- [eBPF 指南](https://ebpf.io)
- [博客文章](https://www.inspektor-gadget.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT
