---
title: "Python 应用 Kubernetes 生产实践"
description: "Python 应用在 K8s 上的生产实践：镜像优化、GIL 与多进程、uvicorn/gunicorn 配置、资源限制与 AI/ML 工作负载特殊考虑"
summary: "面向 SRE 与 AI 平台工程师的 Python on Kubernetes 完整指南，覆盖 Web 服务与 ML 推理工作负载的镜像构建、并发模型、资源调优与故障排查。"
category: 工作负载
tags:
- python
- kubernetes
- gunicorn
- uvicorn
- gil
- ai
- ml
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- Python/AI 工程师
estimated_read_time: 20min
intent_queries:
- "Python 应用如何在 Kubernetes 上生产部署"
- "gunicorn uvicorn worker 数量如何配置"
- "Python AI/ML 推理服务如何配置资源"
trigger_keywords:
- python
- gunicorn
- uvicorn
- gil
- fastapi
- ml inference
prerequisites:
- kubectl-basics
- pod-lifecycle
- container-image-basics
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

# Python 应用 Kubernetes 生产实践

> **适用版本**: Python 3.11+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

Python 在人工智能、机器学习、数据工程以及快速业务原型开发领域占据着无可争议的主导地位。从 FastAPI 构建的高性能 API 服务，到 PyTorch、vLLM 驱动的大模型推理服务，Python 几乎覆盖了现代 AI 基础设施的每一层。然而，正是这种广泛的应用场景，使得 Python 在 Kubernetes 上的生产落地远比表面上看起来复杂。

Python 的几个语言特性给容器化部署带来了独特挑战。首先是 GIL（全局解释器锁）的存在，使得多线程无法真正并行执行 CPU 密集型任务，必须依靠多进程模型；其次是动态类型和庞大的依赖生态，导致镜像体积动辄数百兆甚至上 G；再者是解释型语言的启动特性和内存管理方式，使得资源规划需要格外谨慎。一个常见的反模式是：开发人员在本地 `pip install` 一切正常，到了容器里却因为 glibc 版本不匹配、worker 模型误配、内存超限被 OOMKilled 而故障频发。

本文覆盖两类典型 Python 工作负载：面向业务的 Web/API 服务（FastAPI、Flask、Django）和面向 AI 的推理服务（PyTorch、TensorFlow、vLLM）。我们将从镜像分层优化、并发模型选型，讲到资源限制与 GPU 协同，给出可以直接落地的生产配置。Python 服务的探针配置与生命周期管理遵循通用模式，可以参考 [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go 应用 Kubernetes 生产实践]] 中的相关讨论。

---

## 核心概念

### 1. GIL 与并发模型

GIL（Global Interpreter Lock）是 CPython 实现中的一把全局互斥锁，它保证同一时刻只有一个线程在执行 Python 字节码。这一设计简化了 CPython 的内存管理，但也带来了一个深远的影响：CPU 密集型任务无法通过多线程实现真正的并行。这意味着对于计算密集的工作负载，必须采用多进程模型，让每个进程拥有独立的 Python 解释器和独立的 GIL。

但对于 IO 密集型任务，情况则完全不同。网络请求、数据库查询、文件读写等操作大部分时间在等待 IO 完成，此时 GIL 会被释放，因此使用 asyncio 协程模型可以在单线程内高效处理成千上万的并发连接。理解这一区别是正确选择并发模型的前提。

| 模型 | 代表 | 适用场景 | 并行能力 | 内存开销 |
|------|------|---------|---------|---------|
| 多进程同步 | gunicorn (sync worker) | CPU 密集、传统 WSGI | 进程级并行 | 高（每进程独立内存） |
| 异步协程 | uvicorn / hypercorn | IO 密集、高并发 API | 单进程高并发 | 低 |
| 进程 + 协程 | gunicorn + uvicorn worker | 混合负载 | 进程并行 + 协程 | 中 |
| 多进程移除 GIL | Python 3.13 free-threaded | 实验性真并行 | 线程级 | 低 |

值得特别关注的是 Python 3.13 引入的实验性 free-threaded 模式（PEP 703），它允许在编译时禁用 GIL，从而实现真正的多线程并行。虽然目前仍处于实验阶段，生态兼容性有待验证，但它代表了 Python 并发模型的未来方向，值得在生产规划中持续跟踪。

### 2. Worker 数量经验公式

worker 数量的配置是 Python 服务调优的核心，配置不当要么浪费资源，要么导致性能瓶颈甚至 OOM。

```
gunicorn workers = (2 × CPU 核数) + 1     # 同步 worker
uvicorn workers  = CPU 核数               # 异步 worker，每进程已高并发
```

这里有一个极易被忽视的关键陷阱：worker 数量乘以单个 worker 的内存占用，必须小于容器的 memory limit，否则一旦总内存超限，整个 Pod 会被 OOMKilled。对于普通的 Web 服务，单个 worker 可能只占用几十兆内存，但对于 ML 推理服务，每个 worker 都会加载一份完整的模型到内存或显存中，一个 7B 参数的模型在 fp16 下就需要约 14GB，这种情况下 worker 数量往往只能设置为 1，必须通过 GPU 显存而非进程数来规划容量。

### 3. 镜像分层与依赖锁定

Python 镜像臃肿的根源主要有三个：编译工具链（gcc、build-essential 等）残留在最终镜像中、未清理的 pip 缓存、以及冗余的系统包。生产镜像必须通过多阶段构建将编译产物与运行时彻底分离，使用 `pip install --no-cache-dir` 避免缓存膨胀，并采用锁定文件（requirements.txt、uv.lock 或 poetry.lock）确保依赖版本的确定性和可重现性。在基底选择上，应优先使用 python:3.12-slim 而非完整版镜像，前者通常只有后者三分之一的大小。

---

## 生产部署/实现

### 1. 多阶段镜像构建 🟢

多阶段构建是 Python 镜像优化的基石。第一阶段安装编译工具并构建所有依赖，第二阶段只复制构建好的依赖到精简运行时镜像。

```dockerfile
# 🟢 低风险：仅影响构建产物
# ===== Builder =====
FROM python:3.12-slim AS builder
WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gcc \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir --prefix=/install -r requirements.txt

# ===== Runtime =====
FROM python:3.12-slim
WORKDIR /app

# 创建非 root 用户
RUN useradd --create-home --uid 1000 appuser
COPY --from=builder /install /usr/local
COPY --chown=appuser:appuser . .

USER appuser
ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONFAULTHANDLER=1
EXPOSE 8000
CMD ["gunicorn", "main:app", "-k", "uvicorn.workers.UvicornWorker", \
     "--workers", "4", "--bind", "0.0.0.0:8000", \
     "--graceful-timeout", "30", "--timeout", "60"]
```

这里使用了 `--prefix=/install` 将依赖安装到独立目录，再整体复制到运行时镜像，避免了编译工具链的污染。环境变量方面，PYTHONUNBUFFERED 确保日志实时输出而不被缓冲，PYTHONDONTWRITEBYTECODE 避免生成 .pyc 文件污染只读文件系统，PYTHONFAULTHANDLER 则在进程崩溃时输出 Python 调用栈，极大方便故障定位。CMD 中采用 gunicorn 管理 uvicorn worker 的组合，兼顾了进程管理的健壮性和异步处理的高并发能力。

### 2. Web API 服务 Deployment 🟡

```yaml
# 🟡 中风险：资源与探针配置影响调度与可用性
apiVersion: apps/v1
kind: Deployment
metadata:
  name: py-api
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: py-api
  template:
    metadata:
      labels:
        app: py-api
    spec:
      terminationGracePeriodSeconds: 60
      containers:
      - name: api
        image: registry.example.com/py-api:v2.1.0
        ports:
        - containerPort: 8000
        env:
        - name: WEB_CONCURRENCY
          value: "4"          # uvicorn worker 数
        - name: PYTHONUNBUFFERED
          value: "1"
        resources:
          requests:
            cpu: "1"
            memory: "1Gi"
          limits:
            memory: "2Gi"     # CPU 建议不设硬 limit，避免 throttling
        startupProbe:
          httpGet:
            path: /healthz
            port: 8000
          failureThreshold: 30
          periodSeconds: 2
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8000
          periodSeconds: 15
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8000
          periodSeconds: 5
          failureThreshold: 2
        lifecycle:
          preStop:
            exec:
              command: ["sh", "-c", "sleep 10"]
```

资源配置上有几个要点。memory limit 是必须设置的，因为 Python 存在内存泄漏的可能，没有 limit 的约束，一个泄漏的 Pod 可能拖垮整个节点。而 CPU 我们建议不设硬 limit，只设置 request，因为 Python 服务在突发流量时需要额外的 CPU  burst 能力，硬限制会导致请求处理变慢。startupProbe 的 failureThreshold 设置得较大（30 次），是为了容纳 Python 应用相对较慢的启动过程，尤其是导入了大量依赖的服务。

### 3. AI/ML 推理服务（含 GPU） 🔴

AI 推理服务的部署复杂度远高于普通 Web 服务，涉及 GPU 资源、模型加载、显存管理等多个维度。

```yaml
# 🔴 高风险：GPU 资源昂贵，配置错误导致显存 OOM 或资源浪费
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference
  namespace: ai-serving
spec:
  replicas: 2
  selector:
    matchLabels:
      app: llm-inference
  template:
    metadata:
      labels:
        app: llm-inference
    spec:
      nodeSelector:
        accelerator: nvidia-a100
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
      containers:
      - name: inference
        image: registry.example.com/vllm-server:v0.4.2
        ports:
        - containerPort: 8000
        env:
        - name: NVIDIA_VISIBLE_DEVICES
          value: "all"
        - name: HF_HOME
          value: /models/cache
        resources:
          requests:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: "1"
          limits:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: "1"
        volumeMounts:
        - name: model-store
          mountPath: /models
        readinessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 60    # 模型加载慢，必须留足时间
          periodSeconds: 10
          failureThreshold: 30
        startupProbe:
          httpGet:
            path: /health
            port: 8000
          failureThreshold: 60
          periodSeconds: 5
      volumes:
      - name: model-store
        persistentVolumeClaim:
          claimName: model-pvc
```

这个配置体现了 ML 推理服务的几个关键设计。首先，通过 nodeSelector 和 tolerations 将 Pod 精确调度到 GPU 节点。其次，模型文件通过 PVC 挂载而非打包进镜像，这样模型更新无需重新构建镜像，也避免了镜像体积爆炸。最重要的是探针配置：大模型加载可能需要数分钟，因此 startupProbe 的 failureThreshold 设置为 60 次、每次间隔 5 秒，总共预留了 5 分钟的加载窗口，readinessProbe 也设置了 60 秒的 initialDelaySeconds，确保模型完全加载后才接收流量。GPU 资源的请求和限制必须相等，因为 GPU 是不可超卖的整数资源。

---

## 运维操作

### 1. 依赖管理与锁定 🟢

依赖管理是 Python 项目可重现性的基础。我们强烈推荐使用 uv 这个新一代工具，它的解析和安装速度比 pip 快一个数量级。

```bash
# 🟢 低风险：本地依赖锁定
# 使用 uv（推荐，极快）
uv pip compile pyproject.toml -o requirements.txt --generate-hashes

# 容器内验证依赖一致性
kubectl -n production exec deploy/py-api -- pip check
kubectl -n production exec deploy/py-api -- pip freeze | head -30
```

`--generate-hashes` 参数会为每个依赖生成哈希值，确保安装的包与锁定时完全一致，防止依赖被篡改，这是供应链安全的重要一环。在容器内执行 pip check 可以验证依赖之间没有版本冲突，pip freeze 则用于核对实际安装的版本是否与锁定文件一致。

### 2. 运行时诊断 🟢

```bash
# 🟢 低风险：只读
kubectl -n production top pod -l app=py-api --containers
kubectl -n production logs deploy/py-api --tail=200 -f

# 进入容器排查（distroless 无 shell，slim 有）
kubectl -n production exec -it deploy/py-api -- python -c "import sys; print(sys.version)"

# py-spy 在线火焰图（需 SYS_PTRACE，仅调试 Pod）
kubectl -n production exec deploy/py-api -- py-spy dump --pid 1
```

py-spy 是 Python 服务诊断的利器，它能在不修改代码、不重启进程的情况下采集性能火焰图，定位 CPU 热点。但需要注意的是，py-spy 需要 SYS_PTRACE 权限才能 attach 到进程，这在生产环境通常被安全策略禁止，因此我们建议仅在专门的调试 Pod 中使用，或者在镜像中预装 py-spy 并通过启动参数控制。

### 3. 优雅停机验证 🟢

```bash
# 🟢 低风险：观察滚动更新期间的请求成功率
kubectl -n production rollout status deploy/py-api
kubectl -n production get pods -l app=py-api -w
```

验证优雅停机是否生效，最直接的方法是在滚动更新期间持续向服务发送请求，观察是否出现连接中断或 5xx 错误。一个配置正确的服务，在整个滚动更新过程中应该保持零错误。

---

## 故障排查

### 症状 1：Pod 频繁 OOMKilled

```bash
# 🟢 低风险
kubectl -n production describe pod <pod> | grep -A3 "Last State"
# Reason: OOMKilled, Exit Code: 137
```

OOMKilled 是 Python 服务最常见的故障之一，退出码 137 是其典型特征。根因通常是 worker 数量乘以单 worker 内存超过了 limit，或者 ML 模型的显存和内存估算不足。处置方法包括减少 worker 数量、提高 memory limit，对于 ML 服务还可以启用模型量化（int8 或 fp16）来降低显存占用。需要警惕的是，如果内存是缓慢增长直至 OOM，那很可能是内存泄漏，需要用 tracemalloc 或 objgraph 进一步分析。

### 症状 2：高并发下延迟飙升但 CPU 不高

这是一个非常具有迷惑性的现象：监控显示 CPU 使用率并不高，但请求延迟却急剧上升。根因通常是误用了同步 worker 来处理 IO 密集型负载，worker 在等待 IO 时被阻塞，无法处理新请求。处置方法是切换到 uvicorn.workers.UvicornWorker 异步模型，同时检查代码中是否存在同步阻塞调用，比如应该用 httpx 的异步客户端替换同步的 requests 库。

### 症状 3：ML 推理服务 startup 探针超时

大模型加载耗时往往超过默认的探针窗口，导致 Pod 在模型还没加载完时就被 kubelet 判定为启动失败而重启，陷入循环。根因是探针配置没有为模型加载预留足够时间。处置方法是增大 startupProbe 的 failureThreshold，将模型预加载到 PVC 以加快读取速度，并合理设置 initialDelaySeconds。

### 症状 4：滚动更新连接中断

根因是 gunicorn 的 graceful-timeout 短于在途请求的处理时间，或者缺少 preStop hook 导致 endpoints 摘除竞态。处置方法是设置 `--graceful-timeout 30`，添加 preStop sleep，并确保 readiness 探针在收到 SIGTERM 后立即返回失败。

### 排查决策树

```
Pod 异常
├── OOMKilled (137)?  → 减 worker / 提 limit / 量化模型
├── Error (1)?        → 查 logs → 依赖缺失 / 代码异常
├── 探针失败?         → 检查路径/端口/超时
└── 延迟高 CPU 低?    → 同步阻塞 → 改异步
```

---

## 最佳实践

第一，镜像构建采用多阶段加 slim 基底，使用 --no-cache-dir 并锁定依赖版本与哈希。第二，并发模型上，IO 密集型用 uvicorn 异步，CPU 密集型用 gunicorn 多进程，worker 数量要与内存联动评估。第三，资源管理方面，memory limit 必须设置以防泄漏拖垮节点，CPU 则谨慎设置硬 limit。第四，环境变量要配置 PYTHONUNBUFFERED 保证日志实时、PYTHONFAULTHANDLER 输出崩溃栈。第五，ML 专项实践中，模型与镜像分离存储到 PVC 或对象存储，startup 探针留足加载时间，GPU 配置详见 [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md|GPU 工作负载管理]]。第六，安全方面坚持非 root 运行和 readOnlyRootFilesystem，并接入 [[08-安全/05-供应链/10-image-security-scanning.md|镜像安全扫描]]。第七，可观测性方面导出 python_gc 和 process 系列指标，并接入分布式追踪。

```yaml
# 🟢 低风险：Pod 安全加固
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
  capabilities:
    drop: ["ALL"]
```

---

## Related

- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management.md|GPU 工作负载管理]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production.md|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/02-Java-on-K8s/02-spring-boot-kubernetes-production.md|Spring Boot on Kubernetes 生产实践指南]]
- [[08-安全/05-供应链/10-image-security-scanning.md|镜像安全扫描]]
- [[可观测性]]
