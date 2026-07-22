---
title: Ephemeral Containers
summary: Ephemeral Containers 是临时添加到运行中 Pod 的容器，用于调试和故障排查。
category: concepts
tags:
- pod
- debugging
- ephemeral
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Ephemeral Containers

## 概述

Ephemeral Containers（临时容器，1.25 GA）是 Kubernetes 提供的一种**临时注入到正在运行的 Pod 中**的特殊容器，专用于故障排查。生产镜像通常经过 distroless / scratch 精简，不含 shell、curl、ps、netstat 等调试工具，传统 `kubectl exec` 无法进入。临时容器允许在不修改原 Pod（不影响业务、不重启、不重建）的前提下，挂载一个带完整工具链的容器，进入目标 Pod 的网络/进程命名空间进行诊断——这是云原生"无侵入调试"的标准手段。

## 架构与工作原理

```
原 Pod（运行中，无法 exec）             注入 Ephemeral Container
┌──────────────────────────┐          ┌──────────────────────────────────┐
│ app: distroless 无 shell │   ───►   │ app: distroless（业务，不动）      │
│ 共享 netns / ipc / pid    │          │ debug: nicolaka/netshoot（临时）   │
└──────────────────────────┘          │   共享同一 Pod netns（探测本地端口）│
                                      └──────────────────────────────────┘
```

**关键特性**：
- **不可重启 Pod**：Ephemeral 容器添加到已存在的 Pod，业务容器不会重启。
- **无资源保证**：不参与调度资源计算（无 requests/limits 也可），不能声明 ports/volumes。
- **生命周期随 Pod**：Pod 删除则临时容器一同消失，不会持久化到 Pod 模板。
- **可共享 PID/Network 命名空间**：通过 `shareProcessNamespace: true` 或 targetContainer 深入诊断。
- **`kubectl debug` 是入口**：底层调用 EphemeralContainers 子资源 API。
- **不能进入容器文件系统**：默认只共享 netns，要访问目标容器进程需 `shareProcessNamespace` 或 `--target`。

**三种典型调试模式**：
1. **容器复制（--copy-to）**：复制 Pod 并修改（如改启动命令、加调试镜像），不影响原 Pod。
2. **节点 Shell（node/<name>）**：在节点上起一个 privileged Pod，用于节点级排查。
3. **Profile 抓取（--profile）**：抓取 CPU/堆 profile（需 profile 端口）。

## 关键组件与特性

| 元素 | 说明 |
|------|------|
| EphemeralContainers 子资源 | 专门的 API，绕过 Pod spec 不可变限制 |
| `kubectl debug` | 用户面 CLI |
| targetContainer | process namespace 共享目标容器 |
| copy-to | 复制 Pod 做更深入改动 |
| shareProcessNamespace | Pod 级共享 PID 命名空间 |
| 不可声明字段 | resources、ports、livenessProbe、volumeDevices 等 |

## 配置示例

```bash
# 1. 最常见：给运行中的 Pod 注入一个 busybox/netshoot 调试容器
kubectl debug -it webapp-xxx --image=nicolaka/netshoot --target=webapp
# 进入后：curl localhost:8080/health、ps aux、netstat、tcpdump

# 2. 复制 Pod 调试（修改副本，不动原 Pod）
kubectl debug webapp-xxx --copy-to=webapp-debug \
  --set-image=webapp=webapp:debug --share-processes

# 3. 节点级 shell（需权限）
kubectl debug node/worker-1 -it --image=ubuntu

# 4. 抓 profile
kubectl debug -it webapp-xxx --image=nicolaka/netshoot \
  --target=webapp -- /bin/sh -c 'curl localhost:6060/debug/pprof/profile'
```

对应的底层 EphemeralContainers 资源（一般不直接写，kubectl debug 帮你做）：

```yaml
# kubectl debug 实际生成的 ephemeralcontainers 子资源（示意）
apiVersion: v1
kind: Pod
metadata: {name: webapp-xxx}
spec:
  ephemeralContainers:
  - name: debugger
    image: nicolaka/netshoot:latest
    targetContainerName: webapp
    stdin: true
    tty: true
```

## 常用操作与命令

```bash
# 查看注入的临时容器
kubectl describe pod webapp-xxx | grep -A15 "Ephemeral Containers"
kubectl get pod webapp-xxx -o jsonpath='{.spec.ephemeralContainers[*].name}'

# 进入已注入的临时容器
kubectl exec -it webapp-xxx -c debugger -- /bin/bash

# 查看临时容器日志（通常调试容器没日志，主要用于排查业务）
kubectl logs webapp-xxx -c debugger

# 调试结束后清理：临时容器随 Pod 删除自动消失
# 删除复制的调试 Pod
kubectl delete pod webapp-debug

# RBAC：使用 kubectl debug 需要 pods/ephemeralconfigs 权限
kubectl auth can-i create pods/ephemeralcontainers -n production
```

## 最佳实践

1. **生产镜像 distroless + 用 ephemeral 调试**：减小攻击面，调试时再注入工具集。
2. **预置调试镜像**：维护一个含 curl/jq/tcpdump/strace/gdb/nc 的 netshoot 镜像，加速排障。
3. **限定 RBAC**：ephemeralcontainers 是强权限（可注入任意镜像到 Pod），按 Namespace 授给 SRE 角色。
4. **复制模式做破坏性调试**：要改启动命令/挂载点用 `--copy-to`，不污染生产 Pod。
5. **shareProcessNamespace 谨慎**：共享 PID 可看业务进程内存，开启需评估安全影响。
6. **结合 ephemeral + node debug**：业务问题用 Pod 注入，节点（kubelet/CNI/OOM）问题用 `kubectl debug node/`。
7. **审计日志记录**：开启 audit，记录谁在何时对哪个 Pod 注入了调试容器。

## 常见陷阱

- **镜像拉取失败**：临时容器用 `:latest` tag 触发拉取策略问题，建议用固定版本或 imagePullPolicy: Always。
- **不支持 resources/ports**：在 ephemeralcontainers spec 写 resources 会被 API 拒绝。
- **process namespace 未共享**：`ps` 看不到业务进程，需 Pod 模板开 `shareProcessNamespace: true`（需重建）或用 `--target`。
- **复制的调试 Pod 调度失败**：原 Pod 所在节点资源不足，复制 Pod 没有特殊调度约束可能落别处。
- **节点 debug Pod 残留**：忘记 `kubectl delete`，留下 privileged Pod 成安全隐患。
- **RBAC 不足**：默认开发者角色无 ephemeralcontainers 权限，需管理员授予。
- **Pod 已删除临时容器消失**：复现问题时 Pod 被控制器重建，调试上下文丢失，考虑用 `--copy-to`。

## 源码实现分析

### kubectl debug 实现机制

```go
// k8s.io/kubectl/pkg/cmd/debug/debug.go
func (o *DebugOptions) generatePodDebug() (*v1.Pod, error) {
    // 模式一：注入临时容器到现有 Pod
    if o.copyTo == "" {
        // 1. 获取目标 Pod
        pod := o.getPod(o.podName)
        // 2. 构建 EphemeralContainer 规格
        ec := v1.EphemeralContainer{
            EphemeralContainerCommon: v1.EphemeralContainerCommon{
                Name:  "debugger-" + rand.String(5),
                Image: o.image, // 如 nicolaka/netshoot
                Command: []string{"/bin/bash"},
                Stdin: true, TTY: true,
                // 注意：不支持 resources/ports/livenessProbe
            },
            TargetContainerName: o.targetContainer, // 共享目标容器 PID
        }
        // 3. 调用 API Server 的 ephemeralcontainers 子资源
        pod.Spec.EphemeralContainers = append(pod.Spec.EphemeralContainers, ec)
        o.client.CoreV1().Pods(ns).UpdateEphemeralContainers(ctx, pod.Name, pod)
    }
    // 模式二：复制 Pod（--copy-to）
    // 复制原 Pod spec，修改启动命令，添加调试工具
    return debugPod, nil
}
```

### 临时容器与 Pod 生命周期关系

```
┌──────────────────────────────────────────────────────────┐
│          临时容器在 Pod 生命周期中的位置              │
├──────────────────────────────────────────────────────────┤
│  Pod Created                                             │
│    │                                                    │
│    ├─ Init Containers (order 1..N)                      │
│    ├─ Sidecar Containers (restartPolicy=Always)         │
│    ├─ Main Containers                                   │
│    │     │                                              │
│    │     ├─ Running...                                  │
│    │     │     │                                        │
│    │     │     ├─ kubectl debug 注入 EphemeralContainer │
│    │     │     │   • 不影响现有容器                    │
│    │     │     │   • 不可重启/不可删除(单独)          │
│    │     │     │   • 无 resources/probes/ports          │
│    │     │     │                                        │
│    │     │     └─ Pod 删除时临时容器一起消失          │
│    │     │                                              │
│    └─ Pod Terminated                                    │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：调试 Distroless 镜像 Pod

```bash
# 🟡 中风险：向生产 Pod 注入调试容器
# 目标 Pod 使用 distroless 镜像（无 shell/工具）
kubectl debug -it webapp-abc123 --image=nicolaka/netshoot --target=app -- /bin/bash
# 注入后可以：
# - tcpdump 抓包分析网络问题
# - curl 测试服务连通性
# - nslookup/dig 排查 DNS
# - ip route 检查路由表
# - strace -p 1 跟踪业务进程系统调用（需 shareProcessNamespace）
```

### 场景二：复制 Pod 做破坏性调试

```bash
# 🟡 中风险：创建调试副本（不影响原 Pod）
kubectl debug webapp-abc123 --copy-to=webapp-debug --container=app \
  --image=nicolaka/netshoot -- /bin/bash
# 复制的 Pod 可以：
# - 修改启动命令（如用 sleep 替代主进程）
# - 添加挂载点查看数据
# - 修改环境变量测试配置
# 调试完成后删除
kubectl delete pod webapp-debug  # 🟡 清理调试 Pod
```

### 场景三：节点级调试

```bash
# 🟠 高危：创建特权 Pod 挂载宿主机文件系统
kubectl debug node/worker-01 -it --image=ubuntu:22.04 -- /bin/bash
# 进入后宿主机文件系统在 /host 下
chroot /host
# 可以检查：
# - kubelet 日志和配置
# - CNI 网络插件状态
# - 内核日志 (dmesg)
# - OOM killer 记录
# 完成后必须删除！
exit
kubectl delete pod node-debugger-worker-01  # 🔴 必须清理特权 Pod
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 临时容器可以单独删除 | 临时容器一旦注入无法单独移除，只能随 Pod 删除消失 |
| 2 | 临时容器支持 resources 限制 | API 会拒绝 ephemeralcontainers 中的 resources/ports/probes 字段 |
| 3 | ps 能看到业务进程 | 默认 PID namespace 隔离；需 shareProcessNamespace=true 或 --target |
| 4 | 复制的 Pod 会调度到同一节点 | --copy-to 不保证同节点；需手动添加 nodeSelector/nodeName |
| 5 | 节点 debug Pod 会自动清理 | 不会！忘记删除会留下 privileged Pod，成为安全隐患 |
| 6 | 任何角色都可以 kubectl debug | 需要 pods/ephemeralcontainers create 权限；默认开发者角色无此权限 |

## 面试要点

1. **Q: 临时容器（Ephemeral Container）与普通容器有何区别？**
   A: ① 生命周期：临时容器在 Pod 运行中动态注入，不能单独删除，随 Pod 消失；② 功能限制：不支持 resources、ports、liveness/readiness probe、startup probe；③ 用途：专为调试设计（distroless 镜像无 shell 时注入工具）；④ 权限：需要 pods/ephemeralcontainers RBAC 权限；⑤ 不影响现有容器的运行。

2. **Q: kubectl debug 的三种模式分别适用什么场景？**
   A: ① 注入模式（默认）：向现有 Pod 注入调试容器，适合快速网络/DNS 排查，不影响业务；② 复制模式（--copy-to）：创建 Pod 副本做破坏性调试（改启动命令、加挂载），不影响生产 Pod；③ 节点模式（node/）：创建特权 Pod 挂载宿主机文件系统，排查 kubelet/CNI/内核问题。注意节点模式必须及时清理。

3. **Q: 生产环境使用临时容器的安全注意事项？**
   A: ① RBAC 严格管控：ephemeralcontainers 权限只授给 SRE 角色，按 Namespace 限制；② 审计日志：记录谁在何时对哪个 Pod 注入了什么镜像；③ 镜像白名单：通过 Admission Webhook 限制只能用预审批的调试镜像；④ 节点 debug 必须清理：privileged Pod 残留 = 后门；⑤ 避免在临时容器中执行写操作（影响业务状态）。

4. **Q: 如何调试一个 CrashLoopBackOff 的 distroless 容器？**
   A: ① 先查日志：kubectl logs <pod> --previous（查看崩溃前日志）；② 查事件：kubectl describe pod（查看退出码、OOM 等）；③ 复制调试：kubectl debug <pod> --copy-to=debug-pod --container=app --image=nicolaka/netshoot -- sleep 3600（用 sleep 替代主进程保持运行）；④ 进入调试：kubectl exec -it debug-pod -- /bin/bash（检查文件系统、配置、依赖）；⑤ 手动执行原命令观察报错。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/pods.md|Pod]] — 调试对象
- [[概念/init-containers.md|Init Containers]]
- [[概念/sidecar-containers.md|Sidecar Containers]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
