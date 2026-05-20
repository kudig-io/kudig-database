---
title: Domain-12 故障排查 — 开源项目索引
description: '# Domain-12 故障排查 — 开源项目索引'
category: troubleshooting
tags:
- k8s
- troubleshooting
- debugging
- fault-analysis
- prometheus
- helm
- falco
- kafka
- elasticsearch
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Domain-12 故障排查 — 开源项目索引 是什么
- 如何 Domain-12 故障排查 — 开源项目索引
- Kubernetes 12 troubleshooting 最佳实践
- Domain-12 故障排查 — 开源项目索引 故障排查
- Domain-12 故障排查 — 开源项目索引 排障步骤
trigger_keywords:
- Domain-12
- 故障排查
- 开源项目索引
- troubleshooting
cross_refs:
- type: domain
  path: ../domain-3-control-plane/
  label: '相关知识域: domain-3-control-plane'
- type: domain
  path: ../domain-5-networking/
  label: '相关知识域: domain-5-networking'
- type: domain
  path: ../domain-8-observability/
  label: '相关知识域: domain-8-observability'
---

# Domain-12 故障排查 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: kubectl v1.33 / K9s v0.40 / Stern v1.32

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、K8s 官方工具](#二k8s-官方工具)
- [三、终端 UI 工具](#三终端-ui-工具)
- [四、日志聚合工具](#四日志聚合工具)
- [五、网络诊断](#五网络诊断)
- [六、eBPF 与内核诊断](#六ebpf-与内核诊断)
- [七、资源与容量分析](#七资源与容量分析)
- [八、版本信息](#八版本信息)
- [九、排查工具链推荐](#九排查工具链推荐)

---

## 一、核心项目总览

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **kubectl** | K8s 官方 CLI | K8s | v1.33.0 | - | Apache-2.0 |
| **kubectx / kubens** | 上下文/命名空间切换 | 社区 | - | 17k+ | Apache-2.0 |
| **K9s** | 终端 K8s UI | 社区 | v0.40.0 | 27k+ | Apache-2.0 |
| **Lens (OpenLens)** | K8s IDE / GUI | Mirantis | v6.5.0 | 25k+ | MIT |
| **Stern** | 多 Pod 日志聚合 | 社区 | v1.32.0 | 12k+ | Apache-2.0 |
| **kube-state-metrics** | K8s 资源状态指标 | K8s SIG | v2.15.0 | 5.5k+ | Apache-2.0 |
| **node_exporter** | 主机指标导出 | Prometheus | v1.9.0 | 11k+ | Apache-2.0 |
| **cAdvisor** | 容器资源分析 | K8s SIG | v0.51.0 | 16k+ | Apache-2.0 |
| **Kubeshark** | K8s 流量抓包分析 | Kubeshark | v52.0.0 | 13k+ | Apache-2.0 |
| **Inspektor Gadget** | eBPF 排查工具集 | Inspektor | v0.38.0 | 7k+ | Apache-2.0 |
| **kubectl-debug** | 调试容器 | 社区 | v0.2.0 | 3k+ | Apache-2.0 |
| **kruise-debug** | OpenKruise 调试 | 阿里云 | - | - | Apache-2.0 |
| **ktop** | top 风格 K8s 监控 | 社区 | v0.3.0 | 1.5k+ | Apache-2.0 |
| **kube-capacity** | 资源容量概览 | 社区 | v0.8.0 | 2k+ | Apache-2.0 |
| **kubectl-ai** | AI 辅助 kubectl | 社区 | v1.3.0 | 2k+ | Apache-2.0 |

---

## 二、K8s 官方工具

### 2.1 kubectl

```bash
# 核心排查命令速查
kubectl get events --sort-by='.lastTimestamp' -A                    # 全局事件
kubectl describe pod <pod> -n <ns>                                   # Pod 详情
kubectl logs <pod> -n <ns> --previous                                # 上次崩溃日志
kubectl logs <pod> -n <ns> -f --all-containers                       # 实时全容器日志
kubectl exec -it <pod> -n <ns> -- /bin/sh                            # 进入容器
kubectl port-forward svc/<svc> 8080:80 -n <ns>                       # 本地转发
kubectl top pod -n <ns>                                              # Pod 资源使用
kubectl get pod -o yaml --export <pod>                               # 完整 YAML
kubectl debug <pod> -it --image=busybox --target=<container>         # 临时调试容器 (ephemeral)
```

### 2.2 kubectx / kubens

```bash
kubectx                    # 交互式切换集群
kubectx <context>          # 直接切换
kubens                     # 交互式切换命名空间
kubens <namespace>         # 直接切换
```

**GitHub**: https://github.com/ahmetb/kubectx

---

## 三、终端 UI 工具

### 3.1 K9s

```yaml
# 核心特性
- Vim 风格快捷键
- 实时资源监控
- 日志流式查看 (:logs)
- Shell 进入容器 (:shell)
- 端口转发 (:pf)
- 资源删除与编辑
- 插件扩展 (自定义命令)
- 皮肤与配置定制
```

```bash
# 安装
brew install k9s

# 启动
k9s -n <namespace>

# 常用快捷键
?        # 帮助
/        # 搜索
:pod     # 切换到 Pod 视图
:ns      # 切换到 Namespace 视图
:svc     # 切换到 Service 视图
d        # 删除资源
l        # 查看日志
s        # 进入 Shell
ctrl-l   # 查看容器日志 (多容器)
```

**GitHub**: https://github.com/derailed/k9s

### 3.2 Lens (OpenLens)

- 功能最丰富的 K8s GUI
- 内置终端、日志、Shell
- Helm Chart 管理
- 资源编辑器与差异对比
- 网络、存储可视化
- **注意**: 2024 年后 Lens 主要功能转向 Mirantis 商业版，OpenLens 社区版维护缓慢
- **替代推荐**: K9s (终端) 或 Headlamp (开源 GUI)

### 3.3 Headlamp

- Kinvolk/微软开源的 K8s Web UI
- 插件系统
- 完全开源 (Apache-2.0)
- 桌面应用或集群内部署

**GitHub**: https://github.com/headlamp-k8s/headlamp

---

## 四、日志聚合工具

### 4.1 Stern

```bash
# 多 Pod 实时日志 (按颜色区分)
stern -l app=myapp -n production

# 包含已停止 Pod 的日志
stern -l app=myapp --tail 100 --since 10m

# 正则匹配 Pod 名
stern "api-.*" -n default
```

**GitHub**: https://github.com/stern/stern

### 4.2 对比: Stern vs kubectl logs

| 场景 | 推荐工具 |
|:---|:---|
| 单 Pod 快速查看 | `kubectl logs` |
| 多 Pod/Deployment 实时跟踪 | `stern` |
| 历史日志搜索 | `kubectl logs` + grep / Loki / Elasticsearch |
| 交互式过滤与着色 | `stern` |

---

## 五、网络诊断

### 5.1 Kubeshark

```yaml
# 核心能力
- 类似 Wireshark 的 K8s 流量分析
- 自动解析 HTTP/1.1, HTTP/2, gRPC, AMQP, Kafka, DNS
- 实时流量捕获与回放
- 无需修改应用代码
- 基于 eBPF 和 libpcap
```

```bash
# 安装与运行
kubeshark tap -n <namespace>
# 自动打开浏览器 UI
```

**GitHub**: https://github.com/kubeshark/kubeshark

### 5.2 内置网络诊断

```bash
# 从集群内测试连通性
kubectl run tmp --rm -i --tty --image=nicolaka/netshoot -- /bin/bash
# 然后使用:
dig <svc>.<ns>.svc.cluster.local   # DNS 解析
nc -zv <svc> <port>                # 端口连通性
curl -v http://<svc>:<port>        # HTTP 测试
iptables -t nat -L -n              # NAT 规则
ss -tlnp                           # 监听端口
```

---

## 六、eBPF 与内核诊断

### 6.1 Inspektor Gadget

```yaml
# 核心 Gadget
- trace exec: 追踪进程执行
- trace open: 追踪文件打开
- trace tcp: 追踪 TCP 连接
- trace dns: 追踪 DNS 请求
- snapshot process: 进程快照
- top tcp: TCP 流量 Top
- profile cpu: CPU 性能分析
```

```bash
# 安装
kubectl gadget deploy

# 追踪某 Pod 的所有系统调用
kubectl gadget trace exec -n default -l app=myapp

# 追踪 DNS 请求
kubectl gadget trace dns -n default
```

**GitHub**: https://github.com/inspektor-gadget/inspektor-gadget

### 6.2 kubectl-debug (临时容器)

K8s v1.18+ 原生支持 Ephemeral Containers:

```bash
kubectl debug <pod> -it --image=nicolaka/netshoot --target=<container>
```

### 6.3 kubectl-node-shell

```bash
# 进入节点 Shell (特权 DaemonSet)
kubectl node-shell <node-name>
```

---

## 七、资源与容量分析

### 7.1 kube-capacity

```bash
# 集群资源概览 (requests vs limits vs 实际使用)
kubectl resource-capacity --util --sort cpu.util

# 输出示例
NODE        CPU REQUESTS   CPU LIMITS   CPU UTIL   MEMORY REQUESTS   MEMORY LIMITS   MEMORY UTIL
node-1      45%            80%          32%        60%               90%             45%
```

**GitHub**: https://github.com/robscott/kube-capacity

### 7.2 ktop

- top 风格的 K8s 资源监控终端工具
- Pod/Node/Namespace 维度
- 实时更新

**GitHub**: https://github.com/yskopets/ktop

---

## 八、版本信息

| 工具 | 最新版本 | 安装方式 | 备注 |
|:---|:---|:---|:---|
| kubectl | v1.33.0 | 官方二进制 / 包管理器 | 与集群版本差 <= 1 |
| K9s | v0.40.0 | brew / scoop / 二进制 | 独立运行 |
| Stern | v1.32.0 | brew / go install | 独立运行 |
| kubectx | - | brew / 脚本 | 纯 shell |
| Kubeshark | v52.0.0 | 官方脚本 | 需集群内权限 |
| Inspektor Gadget | v0.38.0 | kubectl krew / 二进制 | 需特权 |
| kube-capacity | v0.8.0 | kubectl krew / 二进制 | 客户端工具 |

---

## 九、排查工具链推荐

```
┌─────────────────────────────────────────────────────────────┐
│                 分层排查工具链推荐                             │
└─────────────────────────────────────────────────────────────┘

日常运维 (Daily Ops)
  ├── K9s ──► 终端快速浏览与操作
  ├── kubectx/kubens ──► 多集群多命名空间切换
  └── Stern ──► 实时多 Pod 日志跟踪

问题定位 (Troubleshooting)
  ├── kubectl describe / logs / events ──► 基础诊断
  ├── kubectl debug / ephemeral containers ──► 容器内排查
  ├── Kubeshark ──► 网络流量分析
  └── Inspektor Gadget ──► eBPF 内核级追踪

性能分析 (Performance)
  ├── kubectl top / ktop ──► 资源使用快照
  ├── kube-capacity ──► 容量规划与资源碎片
  ├── node_exporter + Prometheus ──► 历史趋势
  └── cAdvisor ──► 容器级资源细分

深度诊断 (Deep Dive)
  ├── kubectl-node-shell ──► 节点级排查
  ├── netshoot / debug 容器 ──► 网络连通性
  ├── eBPF (bpftrace/bcc) ──► 自定义内核追踪
  └── Falco ──► 运行时行为审计
```

---

## 参考链接

- [K9s GitHub](https://github.com/derailed/k9s)
- [Stern GitHub](https://github.com/stern/stern)
- [Kubeshark 文档](https://docs.kubeshark.co/)
- [Inspektor Gadget 文档](https://www.inspektor-gadget.io/docs/)
- [kubectl 官方参考](https://kubernetes.io/docs/reference/kubectl/)
- [K8s 故障排查指南](https://kubernetes.io/docs/tasks/debug/)
