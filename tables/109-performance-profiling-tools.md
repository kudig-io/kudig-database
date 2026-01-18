# 109 - 性能分析与调优工具 (Performance Profiling)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 性能分析工具矩阵

| 工具 (Tool) | 分析对象 (Target) | 技术栈 (Tech) | 适用场景 |
|------------|-----------------|--------------|---------|
| **Inspektor Gadget** | 系统调用、网络 | eBPF | 内核级性能分析 |
| **Pyroscope** | 应用代码 | 持续 Profiling | CPU/内存热点 |
| **Pixie** | 全栈可观测 | eBPF | 零侵入性监控 |
| **Kperf** | API Server | 压测 | 控制平面性能 |
| **pprof** | Go 应用 | 原生 | Go 程序优化 |

## Inspektor Gadget eBPF 分析

### 1. 安装与使用
```bash
# 安装
kubectl gadget deploy

# 查看可用 Gadgets
kubectl gadget list

# 追踪系统调用
kubectl gadget trace exec -n production

# 网络延迟分析
kubectl gadget trace tcpconnect -n production

# 文件 I/O 分析
kubectl gadget trace open -n production
```

### 2. 常用场景
```bash
# 查找 CPU 占用高的进程
kubectl gadget top ebpf

# DNS 查询追踪
kubectl gadget trace dns -n production

# OOM 事件监控
kubectl gadget trace oomkill
```

## Pyroscope 持续性能分析

### 1. Agent 部署
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
      - name: myapp
        image: myapp:v1.0
        env:
        - name: PYROSCOPE_SERVER_ADDRESS
          value: "http://pyroscope:4040"
        - name: PYROSCOPE_APPLICATION_NAME
          value: "myapp"
```

### 2. 火焰图分析
- **CPU Profile**: 识别计算密集函数
- **Heap Profile**: 内存分配热点
- **Goroutine Profile**: 并发瓶颈

### 3. 对比分析
```bash
# 对比两个版本性能
pyroscope compare   --baseline myapp{version="v1.0"}   --comparison myapp{version="v1.1"}
```

## Kperf API Server 压测

### 压测命令
```bash
# 安装
go install sigs.k8s.io/kperf@latest

# 压测 Pod 创建
kperf pod --replicas 100 --duration 60s

# 压测 Service 创建
kperf service --replicas 50 --duration 30s

# 自定义 QPS
kperf pod --replicas 100 --qps 50
```

### 分析指标
- **P50/P95/P99 延迟**
- **吞吐量 (QPS)**
- **错误率**

## pprof Go 应用分析

### 1. 启用 pprof
```go
import _ "net/http/pprof"

func main() {
    go func() {
        log.Println(http.ListenAndServe("localhost:6060", nil))
    }()
    // 应用逻辑
}
```

### 2. 采集 Profile
```bash
# CPU Profile
go tool pprof http://localhost:6060/debug/pprof/profile?seconds=30

# Heap Profile
go tool pprof http://localhost:6060/debug/pprof/heap

# Goroutine Profile
go tool pprof http://localhost:6060/debug/pprof/goroutine
```

### 3. 分析命令
```bash
# 进入交互模式
(pprof) top10
(pprof) list <function-name>
(pprof) web  # 生成火焰图
```

## 性能调优最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **持续 Profiling** | 生产环境常态化采集 |
| **基线对比** | 版本间性能回归检测 |
| **资源限制** | 合理设置 CPU/Memory Limits |
| **并发优化** | 调整 Worker 数量 |
| **缓存策略** | 减少重复计算 |
| **异步处理** | 非关键路径异步化 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)