# Sidecar Containers

## 概述
Sidecar 容器是与主应用容器运行在同一 Pod 内的辅助容器，用于增强或扩展主应用功能，如日志收集、监控、安全代理或数据同步。

## 核心概念/原理
- **Kubernetes 实现方式**：自 v1.29（默认启用 `SidecarContainers` 特性门控）起，Sidecar 被实现为一种特殊的 init 容器——即在 `initContainers` 列表中声明，并设置 `restartPolicy: Always`。
- **生命周期**：Sidecar 容器在 Pod 启动时先于主容器启动，并在整个 Pod 生命周期内持续运行；Pod 终止时，Sidecar 在主容器完全停止后才接收终止信号。
- **独立性**：Sidecar 容器拥有独立的重启策略，可独立于主容器启动、停止或重启。

## 关键机制或特性
- **启动顺序**：Sidecar 作为 init 容器享有顺序保证，可与普通 init 容器混合编排复杂的初始化流程。某个 Sidecar 启动并处于 `started=true` 状态后，才会启动下一个 init 容器。
- **终止顺序**：Sidecar 按定义顺序的反向依次终止，确保辅助服务在主应用需要时始终可用。
- **探针支持**：与普通 init 容器不同，Sidecar 支持 `livenessProbe`、`readinessProbe` 和 `startupProbe`。
- **Job 兼容性**：在 Job 中使用 Kubernetes 原生 Sidecar 容器时，Sidecar 不会阻止 Job 在主容器完成后标记为完成。
- **资源共享**：Sidecar 与主容器共享网络和存储命名空间；Pod 的有效资源请求/限制为：Pod Overhead + max(非 init 容器之和, 有效 init 容器值)。

## 使用场景
- 日志/指标收集代理（如 Fluent Bit、Prometheus exporter）。
- 服务网格代理（如 Istio Envoy）。
- 配置重载或文件同步工具。
- 安全审计或身份验证代理。

## 最佳实践/注意事项
- 优先使用原生 Sidecar（`initContainers` + `restartPolicy: Always`）而不是普通多容器 Pod，以获得更好的生命周期控制。
- Sidecar 的优雅终止优先级较低；若主容器占用了全部优雅终止时间，Sidecar 可能收到 SIGTERM 后很快收到 SIGKILL，因此其非零退出码在 Pod 终止时是正常现象。
- 更改 Sidecar 镜像不会导致整个 Pod 重启，仅会触发该容器重启。
- 如无需控制启动/停止顺序，也可直接在 `containers` 字段中运行多容器。

## 实战 YAML 示例

以下为带 Fluent Bit 日志采集 Sidecar 的生产级 Pod 配置：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-log-sidecar
  namespace: prod
  labels:
    app: myapp
spec:
  terminationGracePeriodSeconds: 60
  initContainers:
  # 原生 Sidecar: Fluent Bit 日志采集器
  - name: fluent-bit
    image: fluent/fluent-bit:3.1
    restartPolicy: Always                    # 标记为 Sidecar（持续运行）
    resources:
      requests:
        cpu: "50m"
        memory: "64Mi"
      limits:
        cpu: "200m"
        memory: "128Mi"
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/app
      readOnly: true                         # Sidecar 只读日志目录
    - name: fluent-bit-config
      mountPath: /fluent-bit/etc
    startupProbe:
      httpGet:
        path: /api/v1/health
        port: 2020
      periodSeconds: 5
      failureThreshold: 10
  # 普通 init 容器: 等待数据库就绪
  - name: wait-for-db
    image: busybox:1.36
    command: ['sh', '-c', 'until nc -z db-svc 5432; do echo waiting for db; sleep 2; done']
  containers:
  - name: app
    image: myregistry.com/myapp:v1.0.0
    ports:
    - containerPort: 8080
    resources:
      requests:
        cpu: "250m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "512Mi"
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/app                # 应用写日志到共享卷
  volumes:
  - name: shared-logs
    emptyDir: {}
  - name: fluent-bit-config
    configMap:
      name: fluent-bit-config
```

**启动顺序**: fluent-bit (Sidecar, 持续运行) → wait-for-db (init, 运行到完成) → app (主容器)

**终止顺序**: app (先终止) → wait-for-db (已完成, 无操作) → fluent-bit (最后终止)

## 故障排查

### Sidecar 导致 Pod 无法就绪
- **症状**: Pod 状态 `Init:0/2`，卡在 Sidecar init 容器启动阶段。
- **常见原因**: Sidecar 的 `startupProbe` 持续失败；镜像拉取失败；配置挂载错误。
- **诊断命令**:
  ```bash
  # 查看 init 容器状态
  kubectl get pod <pod-name> -n prod -o jsonpath='{.status.initContainerStatuses}'
  # 查看 Sidecar 容器日志
  kubectl logs <pod-name> -c fluent-bit -n prod
  # 查看 Pod 事件
  kubectl describe pod <pod-name> -n prod | grep -A 10 "Events"
  ```

### Job 中 Sidecar 不退出导致 Job 无法完成（旧模式）
- **症状**: 使用普通 `containers` 定义的 Sidecar，在主容器完成后 Pod 仍为 Running。
- **根因**: 未使用原生 Sidecar 模式（`initContainers` + `restartPolicy: Always`），普通容器不会随主容器完成而终止。
- **解决方案**: 升级到 K8s >= 1.29 并使用原生 Sidecar 模式，或在主容器完成后通过 `preStop` 钩子通知 Sidecar 退出。

### Sidecar 资源消耗过高
- **症状**: 节点资源告警，Sidecar 容器 CPU/内存使用远超预期。
- **诊断命令**:
  ```bash
  kubectl top pod <pod-name> -n prod --containers
  ```
- **解决方案**: 为 Sidecar 设置合理的 `resources.limits`，调整 Sidecar 配置（如 Fluent Bit 的 buffer 大小）。

## 生产就绪检查清单

- [ ] 使用原生 Sidecar 模式（`initContainers` + `restartPolicy: Always`），而非普通多容器
- [ ] Sidecar 容器已配置 `resources.requests/limits`
- [ ] Sidecar 配置了 `startupProbe`，确保启动检测
- [ ] 共享卷的读写权限已正确设置（Sidecar 通常 readOnly）
- [ ] 在 Job 场景中验证 Sidecar 不会阻止 Job 完成
- [ ] Sidecar 的 `terminationGracePeriodSeconds` 足够完成缓冲区刷新
- [ ] 监控 Sidecar 容器的资源消耗，防止影响主应用

## 命令快速参考

```bash
# 查看 Pod 中所有容器状态（含 init/sidecar）
kubectl get pod <pod-name> -n prod -o jsonpath='{range .status.initContainerStatuses[*]}{.name}: {.state}{"\n"}{end}'

# 查看 Sidecar 容器日志
kubectl logs <pod-name> -c <sidecar-name> -n prod

# 查看 Pod 容器资源消耗
kubectl top pod <pod-name> -n prod --containers

# 重启 Sidecar 而不影响 Pod（需 kubectl 1.29+）
kubectl debug -it <pod-name> --image=busybox --target=<sidecar-name> -n prod
```

## 交叉引用

- [Sidecar 容器高级模式](../../domain-4-workloads/14-sidecar-containers-patterns.md)
- [容器生命周期钩子](../../domain-4-workloads/13-container-lifecycle-hooks.md)
- [Init 容器](./init-containers.md)
- [Pod 生命周期事件](../../domain-4-workloads/11-pod-lifecycle-events.md)
- [工作负载故障排查手册](../../domain-4-workloads/07-workload-troubleshooting-handbook.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/sidecar-containers/
