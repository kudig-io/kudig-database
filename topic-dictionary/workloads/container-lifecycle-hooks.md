# 容器生命周期钩子（Container Lifecycle Hooks）

## 概述

类似于 Angular 等编程框架中的组件生命周期钩子，Kubernetes 为容器提供了生命周期钩子（Lifecycle Hooks）机制。该机制使容器能够感知自身管理生命周期中的事件，并在相应钩子触发时执行处理程序（handler）中的代码。

## 核心概念/原理

### 可用的容器钩子

Kubernetes 目前为容器暴露以下两种生命周期钩子：

#### PostStart

- 在容器创建后立即执行
- 与容器的 `ENTRYPOINT`（主进程）**并发运行**
- 可能在主进程启动之前、期间或之后运行
- 不接收任何参数
- 若执行时间过长或挂起，可能延迟容器进入 `Running` 状态

#### PreStop

- 在容器因 API 请求或管理事件（如存活探针/启动探针失败、抢占、资源竞争等）被终止之前立即调用
- 必须在发送 TERM 信号之前完成
- Pod 的终止宽限期（`terminationGracePeriodSeconds`）在 PreStop 执行前就开始倒计时
- 无论 handler 执行结果如何，容器最终都会在宽限期内被终止
- 若容器已处于终止或完成状态，则 PreStop 调用会失败

#### StopSignal

- 用于定义容器停止时发送的信号
- 若设置了该值，将覆盖容器镜像中定义的 `STOPSIGNAL` 指令

### 钩子处理程序实现类型

容器可以通过实现并注册 handler 来访问钩子，支持三种类型：

1. **Exec**：在容器的 cgroups 和命名空间内执行特定命令（如 `pre-stop.sh`），消耗的资源计入该容器
2. **HTTP**：向容器内的特定端点发送 HTTP 请求
3. **Sleep**：让容器暂停指定的时长

### 钩子执行机制

- `httpGet`、`tcpSocket`（已弃用）和 `sleep` 由 kubelet 进程直接执行
- `exec` 在容器内部执行
- `PostStart` 与容器 ENTRYPOINT 同时触发，因此通常不适合使用 HTTP 钩子，因为无法保证容器进程已完全启动
- `PreStop` 钩子的执行与停止容器的信号**非异步**：钩子必须完成后才能发送 TERM 信号

## 关键机制或特性

### 钩子失败处理

- 若 `PostStart` 或 `PreStop` 钩子失败，**会杀死该容器**
- `PreStop` 钩子若挂起，Pod 将保持在 `Terminating` 状态，直到 `terminationGracePeriodSeconds` 到期后被强制终止
- 宽限期适用于 PreStop 执行时间 + 容器正常停止时间的总和。例如，宽限期 60 秒，PreStop 用了 55 秒，容器正常停止需要 10 秒，则容器会在发送信号后被提前杀死（因为 55+10 > 60）

### 投递保证

- 钩子的投递语义为 **at least once**（至少一次），即同一个事件可能触发多次钩子调用
- 通常只投递一次，但在 kubelet 重启等极少数情况下可能出现重复投递
- 钩子实现应具备幂等性，能够正确处理多次调用

### 调试钩子

- 钩子 handler 的日志不会直接显示在 Pod 事件中
- 若 handler 失败，Kubernetes 会广播事件：
  - `FailedPostStartHook`
  - `FailedPreStopHook`
- 可通过 `kubectl describe pod <pod-name>` 查看相关事件进行排查

## 使用场景

- **PostStart**：
  - 容器启动后注册服务到配置中心或服务发现系统
  - 执行初始化脚本，如创建临时目录、加载缓存数据
  
- **PreStop**：
  - 优雅地断开与下游服务的连接
  - 在容器停止前保存状态、刷新日志或上报指标
  - 从负载均衡池中移除实例

- **StopSignal**：
  - 应用程序使用非默认信号进行优雅关闭时，覆盖镜像中的 `STOPSIGNAL`

## 最佳实践/注意事项

- **钩子 handler 应尽量轻量**：避免长时间运行的操作，以免影响容器状态转换或超出终止宽限期
- **PostStart 慎用 HTTP 钩子**：由于与 ENTRYPOINT 并发执行，HTTP 服务端可能尚未准备好接收请求
- **合理设置 `terminationGracePeriodSeconds`**：为 PreStop 执行和容器正常关闭预留充足时间
- **确保钩子幂等性**：考虑到 at least once 的投递语义，handler 应能安全地处理重复调用
- **通过事件排查问题**：使用 `kubectl describe pod` 查看 `FailedPostStartHook` 或 `FailedPreStopHook` 事件定位失败原因

## 参考链接

- [Kubernetes 官方文档：容器生命周期钩子](https://kubernetes.io/docs/concepts/containers/container-lifecycle-hooks/)
- [Pod 终止行为](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#pod-termination)
- [为容器生命周期事件附加 handler（实践任务）](https://kubernetes.io/docs/tasks/configure-pod-container/attach-handler-lifecycle-event/)
