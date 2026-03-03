# ChaosBlade

> **成熟度**: Sandbox | **加入时间**: 2021-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://chaosblade.io |
| **GitHub** | https://github.com/chaosblade-io/chaosblade |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, Java |
| **CNCF 分类** | Chaos Engineering |
| **维护组织** | Alibaba |

---

## 项目概述

ChaosBlade 是阿里巴巴开源的混沌工程实验工具，用于模拟各种故障场景以测试系统的韧性。它支持对主机、容器、Kubernetes 和各种中间件 (Dubbo、RocketMQ、MySQL) 进行故障注入。ChaosBlade 提供统一的 CLI 和 Kubernetes Operator 两种使用方式。

---

## 核心特性

- **多平台支持**: 主机、Docker、Kubernetes 环境
- **丰富场景**: CPU、内存、网络、磁盘、进程故障
- **中间件故障**: Java 应用、Dubbo、RocketMQ、MySQL 等
- **Kubernetes 原生**: Operator 模式，CRD 声明式实验
- **安全机制**: 实验自动恢复和销毁
- **统一 CLI**: 一致的命令行接口

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    ChaosBlade Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      User Interface                       │   │
│  │  ┌─────────────────┐  ┌────────────────────────────────┐ │   │
│  │  │  ChaosBlade CLI │  │   Kubernetes CRD / Operator    │ │   │
│  │  │  blade create   │  │   ChaosBlade CR                │ │   │
│  │  │  blade destroy  │  │                                │ │   │
│  │  └────────┬────────┘  └──────────────┬─────────────────┘ │   │
│  └───────────┼──────────────────────────┼──────────────────┘   │
│              │                          │                       │
│  ┌───────────▼──────────────────────────▼──────────────────┐   │
│  │                  ChaosBlade Core Engine                   │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │               Experiment Executors                   │ │   │
│  │  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────────┐ │ │   │
│  │  │  │  OS    │ │ Docker │ │  K8s   │ │  JVM Agent   │ │ │   │
│  │  │  │Executor│ │Executor│ │Executor│ │  (Java)      │ │ │   │
│  │  │  └────┬───┘ └────┬───┘ └────┬───┘ └──────┬───────┘ │ │   │
│  │  │       │          │          │             │         │ │   │
│  │  │  ┌────▼──────────▼──────────▼─────────────▼───────┐ │ │   │
│  │  │  │              Fault Models                       │ │ │   │
│  │  │  │ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌───────┐ │ │ │   │
│  │  │  │ │ CPU  │ │Memory│ │Network│ │ Disk │ │Process│ │ │ │   │
│  │  │  │ │      │ │      │ │      │ │      │ │       │ │ │ │   │
│  │  │  │ │fullload│ │oom │ │delay │ │fill  │ │kill   │ │ │ │   │
│  │  │  │ │      │ │load  │ │loss  │ │burn  │ │stop   │ │ │ │   │
│  │  │  │ │      │ │      │ │corrupt│ │      │ │       │ │ │ │   │
│  │  │  │ └──────┘ └──────┘ └──────┘ └──────┘ └───────┘ │ │ │   │
│  │  │  └────────────────────────────────────────────────┘ │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │               Kubernetes Operator Mode                    │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │ chaosblade-operator  ──►  chaosblade-tool (DaemonSet)│ │   │
│  │  │     (Deployment)          (Executes experiments)     │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **blade CLI** | 命令行工具，创建/销毁实验 |
| **chaosblade-operator** | K8s Operator，管理 CRD 实验 |
| **chaosblade-exec-os** | 操作系统级故障执行器 |
| **chaosblade-exec-docker** | Docker 容器故障执行器 |
| **chaosblade-exec-jvm** | Java 应用故障注入代理 |

---

## 快速开始

### CLI 安装

```bash
# 下载二进制
wget https://github.com/chaosblade-io/chaosblade/releases/latest/download/chaosblade-linux-amd64.tar.gz
tar -xzf chaosblade-linux-amd64.tar.gz
cd chaosblade-linux-amd64
export PATH=$PWD:$PATH

# 验证
blade version
```

### Kubernetes Operator 安装

```bash
# Helm 安装
helm repo add chaosblade https://chaosblade-io.github.io/chaosblade-operator
helm install chaosblade-operator chaosblade/chaosblade-operator \
  --namespace chaosblade \
  --create-namespace

# 验证
kubectl get pods -n chaosblade
```

---

## CLI 实验

### CPU 故障

```bash
# CPU 满载 (全部核心)
blade create cpu fullload

# 指定核心数和时长
blade create cpu fullload --cpu-count 2 --timeout 60

# 指定 CPU 使用率
blade create cpu fullload --cpu-percent 80 --timeout 300

# 销毁实验
blade destroy <experiment-uid>
```

### 内存故障

```bash
# 内存占用 (指定百分比)
blade create mem load --mode ram --mem-percent 80 --timeout 120

# 指定内存大小
blade create mem load --mode ram --reserve 256 --rate 100

# OOM 模拟
blade create mem load --mode ram --mem-percent 99
```

### 网络故障

```bash
# 网络延迟
blade create network delay --time 3000 --interface eth0

# 指定目标 IP
blade create network delay --time 500 --interface eth0 \
  --destination-ip 10.0.0.100

# 丢包
blade create network loss --percent 50 --interface eth0

# 网络损坏
blade create network corrupt --percent 30 --interface eth0

# DNS 故障
blade create network dns --domain example.com --ip 127.0.0.1
```

### 磁盘故障

```bash
# 磁盘填充
blade create disk fill --size 10240 --path /data --timeout 120

# 磁盘 I/O 高负载
blade create disk burn --read --write --path /data --size 1024

# 磁盘 I/O 延迟 (需要指定挂载点)
blade create disk burn --read --path /data --size 1024
```

### 进程故障

```bash
# 杀死进程
blade create process kill --process nginx

# 停止进程
blade create process stop --process java --timeout 60
```

---

## Kubernetes 实验

### CRD 方式 - Pod 故障

```yaml
apiVersion: chaosblade.io/v1alpha1
kind: ChaosBlade
metadata:
  name: pod-network-delay
spec:
  experiments:
    - scope: kubernetes
      target: pod
      action: network-delay
      desc: "Pod network delay experiment"
      matchers:
        - name: namespace
          value: ["production"]
        - name: labels
          value: ["app=backend"]
        - name: names
          value: ["backend-xxx"]  # 可选：指定 Pod 名
        - name: network-interface
          value: ["eth0"]
        - name: time
          value: ["3000"]  # 3000ms 延迟
        - name: offset
          value: ["500"]
```

### 容器 CPU 故障

```yaml
apiVersion: chaosblade.io/v1alpha1
kind: ChaosBlade
metadata:
  name: container-cpu-fullload
spec:
  experiments:
    - scope: kubernetes
      target: container
      action: cpu-fullload
      desc: "Container CPU stress"
      matchers:
        - name: namespace
          value: ["default"]
        - name: labels
          value: ["app=web"]
        - name: container-names
          value: ["nginx"]
        - name: cpu-percent
          value: ["80"]
        - name: timeout
          value: ["120"]
```

### Node 故障

```yaml
apiVersion: chaosblade.io/v1alpha1
kind: ChaosBlade
metadata:
  name: node-network-loss
spec:
  experiments:
    - scope: kubernetes
      target: node
      action: network-loss
      desc: "Node network packet loss"
      matchers:
        - name: names
          value: ["worker-1"]
        - name: percent
          value: ["30"]
        - name: interface
          value: ["eth0"]
        - name: destination-ip
          value: ["10.96.0.1"]
```

---

## Java 应用故障注入

### JVM 故障

```bash
# 方法延迟注入
blade create jvm delay --time 5000 \
  --classname com.example.service.UserService \
  --methodname getUser \
  --process java

# 方法异常注入
blade create jvm throwCustomException \
  --exception java.lang.RuntimeException \
  --classname com.example.service.OrderService \
  --methodname createOrder \
  --process java

# 线程池满载
blade create jvm cpufullload --process java

# GC 暂停
blade create jvm oom --process java
```

### Dubbo 故障

```bash
# Dubbo 服务延迟
blade create dubbo delay --time 3000 \
  --service com.example.UserService \
  --methodname getUser \
  --consumer \
  --process java

# Dubbo 服务异常
blade create dubbo throwCustomException \
  --exception java.lang.RuntimeException \
  --service com.example.OrderService \
  --provider \
  --process java
```

---

## 实验管理

```bash
# 查看所有实验
blade status --type create

# 查看实验详情
blade status <experiment-uid>

# 销毁指定实验
blade destroy <experiment-uid>

# 销毁所有实验
blade destroy --all

# Kubernetes 中查看
kubectl get chaosblade
kubectl describe chaosblade pod-network-delay

# 删除 K8s 实验
kubectl delete chaosblade pod-network-delay
```

---

## 最佳实践

1. **渐进式注入**: 从小规模开始，逐步扩大实验范围
2. **超时设置**: 始终设置 timeout 参数防止永久故障
3. **非生产先行**: 先在测试环境验证实验效果
4. **监控配合**: 结合监控系统观察系统响应
5. **团队协作**: 通知相关人员后再执行实验
6. **回滚计划**: 确保可以快速销毁实验

---

## 参考资源

- [官方文档](https://chaosblade.io/docs)
- [GitHub Repo](https://github.com/chaosblade-io/chaosblade)
- [Operator 文档](https://github.com/chaosblade-io/chaosblade-operator)
- [实验场景参考](https://chaosblade.io/docs/experiment-types/)
- [中文文档](https://chaosblade-io.gitbook.io/chaosblade-help-zh-cn/)

---

**维护者**: Kudig Team | **许可证**: MIT
