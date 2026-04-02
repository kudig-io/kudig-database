# Init Containers

## 概述
Init 容器是在 Pod 启动期间、于应用容器之前运行的特殊容器。它们通常用于执行应用镜像中不包含的初始化工具或设置脚本。

## 核心概念/原理
- **执行顺序**：Init 容器按定义顺序依次运行，每个必须成功完成后下一个才会启动；全部成功后，应用容器才会并行启动。
- **与普通容器的区别**：
  - 必须运行到完成（run to completion）。
  - 不支持 `lifecycle`、存活探针、就绪探针或启动探针（Sidecar 类型的 init 容器除外）。
  - 资源请求/限制的计算方式不同：取所有 init 容器中每项资源的最高值作为 effective init request/limit。
- **失败处理**：若 init 容器失败，kubelet 会根据 Pod 的 `restartPolicy` 重试（`Never` 时 Pod 整体失败）。

## 关键机制或特性
- **资源共享**：Init 容器与应用容器共享网络命名空间和存储卷（如 `emptyDir`），但不直接交互；可通过共享卷单向传递数据。
- **Pod 就绪条件**：在所有 init 容器完成前，Pod 不会进入 `Ready` 状态，`Initialized` 条件为 `False`。
- **镜像更新**：直接修改 init 容器的 `image` 字段不会触发 Pod 重启；若 Pod 尚未开始启动，则可能影响启动行为。
- **幂等性**：由于 init 容器可能被重启或重新执行，其代码应具备幂等性。
- **安全优势**：可将敏感工具或初始化逻辑与主应用镜像分离，减少应用镜像的攻击面。

## 使用场景
- 等待依赖服务就绪（如通过循环探测等待数据库或上游服务）。
- 从 Git 仓库克隆代码到共享卷。
- 生成配置文件、注册 Pod 到远程服务器。
- 执行一次性的数据迁移或权限设置。

## 最佳实践/注意事项
- 将不需要长期存在于应用镜像中的工具放入 init 容器。
- 使用 `activeDeadlineSeconds` 防止 init 容器无限失败（但注意该字段会作用于整个 Pod 生命周期）。
- Init 容器和应用容器的名称在 Pod 内必须唯一。
- 如果通过 Pod 模板修改 init 容器，影响取决于使用该模板的工作负载控制器。
- 多个 init 容器会延长 Pod 启动时间，应尽量减少不必要的 init 步骤。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/init-containers/
