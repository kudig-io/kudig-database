---
title: RuntimeClass 配置指南
description: 使用 RuntimeClass 将 gVisor / Kata / runsc 等 handler 按工作负载分配，含节点 RuntimeHandler 与 Pod 绑定
summary: 使用 RuntimeClass 将 gVisor / Kata / runsc 等 handler 按工作负载分配，含节点 RuntimeHandler 与 Pod 绑定
category: container-runtime
tags:
- containerd
- cri
- runtime
- runtimeclass
- gvisor
- kata
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 平台工程师
---

> **生产环境安全提示**
>
> 风险标注：🔴 高风险 / 🟡 中风险 / 🟢 只读。

# RuntimeClass 配置指南

## 概述

集群中常混用多种容器运行时：默认 runc（高性能、共享内核）、gVisor（用户态内核、强隔离）、Kata（轻量 VM、最强隔离）。**RuntimeClass** 让你在同一集群里按工作负载指定运行时：不受信任的代码用 gVisor，合规要求高的用 Kata，普通业务用 runc——无需为每个 Pod 单独配节点。

## 工作机制

```
Pod.spec.runtimeClassName → RuntimeClass(node.k8s.io) → handler
   ↓ 调度
节点 containerd [plugins."...".containerd.runtimes.<handler>]
   ↓
对应 OCI runtime（runc / runsc / kata）
```

- RuntimeClass 是集群级 API 对象，`handler` 字段对应节点 containerd 配置里的 `runtimes.<name>`。
- 调度可通过 `RuntimeClass.scheduling` 选择装了对应 runtime 的节点（`nodeSelector`/`tolerations`）。

## 节点侧：注册 RuntimeHandler

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
  runtime_type = "io.containerd.runsc.v1"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
  runtime_type = "io.containerd.kata.v2"
```

``` bash
# 🟢 只读：确认节点已识别 handler
crictl info | jq '.config.containerd.runtimes | keys'
```

## 创建 RuntimeClass

``` yaml
# gvisor.yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    sandbox-runtime: gvisor   # 仅调度到装了 runsc 的节点
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
scheduling:
  nodeSelector:
    sandbox-runtime: kata
```

> ⚠️ **🟡 中危变更**

``` bash
# 🟡 中风险：创建集群级对象
kubectl apply -f gvisor.yaml
kubectl get runtimeclass
```

## Pod 绑定 RuntimeClass

``` yaml
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-job
spec:
  runtimeClassName: gvisor        # ← 一行即可
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/demo/runner:v1
```

``` bash
# 🟢 只读：确认实际使用的 runtime
kubectl get pod untrusted-job -o jsonpath='{.status.runtimeHandler}{"\n"}'
crictl inspectp <sandbox-id> | jq '.info.runtimeType'
```

## 节点标签与调度隔离

``` bash
# 🟡 中风险：给节点打标签（控制调度）
kubectl label node node-arm-sandbox sandbox-runtime=gvisor
# 容忍：让 RuntimeClass Pod 只跑在专用节点
```

若节点未装 handler 但 Pod 指定了 RuntimeClass，会报 `RuntimeClass "gvisor" not found` 或 `handler not registered`。

## handler 选型

| 场景 | 推荐 handler | 隔离强度 | 性能损耗 |
|---|---|---|---|
| 普通业务 | runc | 内核共享 | 无 |
| 不受信任代码 / SaaS 多租 | runsc (gVisor) | 用户态内核 | 中（10-30%） |
| 合规 / 金融强隔离 | kata | 轻 VM | 高（启动慢） |
| Serverless 敏感 | firecracker-containerd | microVM | 中 |

## 常见故障

| 现象 | 根因 | 处理 |
|---|---|---|
| `RuntimeClass not found` | 集群未创建该 RC | `kubectl get runtimeclass` |
| `handler not registered` | 节点 containerd 未配 runtimes.<handler> | 补 config.toml 并重启 containerd |
| Pod Pending | 调度 `nodeSelector` 无匹配节点 | 给节点打标签或扩容专用池 |
| `runsc: operation not permitted` | runsc 需特定内核能力 | 升级 runsc，检查 kernel ≥ 5.4 |

## 生产检查清单

- [ ] 节点 containerd 注册了所有业务所需 handler
- [ ] RuntimeClass 配 `scheduling.nodeSelector` 隔离专用节点池
- [ ] 关键 Pod 已设 `runtimeClassName`，并验证 `.status.runtimeHandler`
- [ ] runsc / kata 二进制版本与内核匹配，已通过冒烟测试

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| Pod 调度失败 | RuntimeClass 不存在 | `kubectl get runtimeclass` | 创建对应的 RuntimeClass 资源 |
| 容器启动失败 | 运行时二进制缺失 | `ls /usr/local/bin/runsc` | 安装对应运行时二进制 |
| 性能下降明显 | 沙箱运行时开销 | `kubectl top pod` | 评估是否真正需要强隔离 |
| 节点不匹配 | nodeSelector 配置错误 | `kubectl get nodes --show-labels` | 确认节点标签与 RuntimeClass 匹配 |
| 内核模块缺失 | gVisor 需要特定内核 | `uname -r` | 升级内核到 4.15+ |
| Kata 启动失败 | 虚拟化未启用 | `grep -E 'vmx|svm' /proc/cpuinfo` | BIOS 启用 VT-x/AMD-V |
| 运行时切换无效 | Pod 未指定 runtimeClassName | `kubectl get pod -o yaml` | 在 spec 中添加 runtimeClassName |
| 多运行时冲突 | containerd 配置错误 | `containerd config dump` | 检查 runtime_type 配置 |

## RuntimeClass 配置示例

```yaml
# gVisor RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    runtime: gvisor
overhead:
  podFixed:
    memory: "100Mi"
    cpu: "100m"
---
# Kata Containers RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata
scheduling:
  nodeSelector:
    runtime: kata
overhead:
  podFixed:
    memory: "160Mi"
    cpu: "250m"
```

## 运行时对比矩阵

| 运行时 | 隔离级别 | 性能开销 | 启动时间 | 适用场景 |
|--------|----------|----------|----------|----------|
| runc | 容器级 | 无 | ~100ms | 通用工作负载 |
| gVisor (runsc) | 内核级 | 10-30% | ~200ms | 多租户/不可信代码 |
| Kata | VM 级 | 5-15% | ~500ms | 强隔离/合规要求 |
| Firecracker | microVM | 5-10% | ~125ms | Serverless/FaaS |
| Wasm (spin) | 沙箱级 | 极低 | ~10ms | 边缘计算/插件 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 节点池 | 专用节点池运行沙箱容器 | 通过 nodeSelector 隔离 |
| overhead | 配置 Pod overhead | 确保资源调度准确 |
| 测试 | 新运行时先冒烟测试 | 验证基本功能正常 |
| 回滚 | 保留 runc 作为默认 | 问题时可快速切回 |
| 监控 | 监控沙箱运行时资源开销 | 与 runc 对比基线 |
| 版本 | 运行时二进制与内核版本匹配 | 避免兼容性问题 |
| 文档 | 记录每个 RuntimeClass 的用途 | 便于团队理解 |
| 升级 | 运行时升级需滚动重启 Pod | 不支持热升级 |

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| runsc | gVisor 运行时 | 随 gVisor 安装 |
| kata-runtime | Kata 运行时 | 随 kata-containers 安装 |
| firecracker | microVM 运行时 | 随 firecracker 安装 |
| crictl | 验证运行时配置 | `crictl info` |
| kubectl | RuntimeClass 管理 | `kubectl get/apply runtimeclass` |
| containerd | 运行时注册 | 编辑 config.toml |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| RuntimeClass 和 RuntimeHandler 的关系？ | RuntimeClass 是 K8s 资源，handler 对应 containerd 中的 runtime_type |
| 如何查看节点支持的运行时？ | `crictl info` 查看 runtimeHandlers 列表 |
| 能否动态添加 RuntimeClass？ | 可以，kubectl apply 即可，但节点需已安装对应运行时 |
| overhead 的作用？ | 让调度器考虑沙箱额外资源消耗 |
| 如何回滚到 runc？ | 删除 Pod 的 runtimeClassName 字段即可 |
| gVisor 和 Kata 如何选择？ | 性能敏感选 gVisor，强隔离选 Kata |
| 所有 Pod 都需要沙箱吗？ | 不需要，仅不可信工作负载使用 |
| 如何测试 RuntimeClass 是否生效？ | `kubectl get pod -o jsonpath='{.status.runtimeHandler}'` |

## 性能调优

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 沙箱启动慢 | 预热运行时 | 节点初始化时预加载二进制 |
| 性能开销大 | 评估必要性 | 仅不可信负载用沙箱 |
| 资源调度不准 | 配置 overhead | 让调度器考虑额外开销 |
| 节点不匹配 | nodeSelector | 专用节点池隔离 |
| 升级影响 | 滚动重启 | 不支持热升级 |
| 回滚慢 | 保留 runc 默认 | 快速切回 |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| runtime_class_usage | 各运行时使用量 | 异常分布 |
| sandbox_start_duration | 沙箱启动耗时 | P99 > 2s |
| runtime_overhead_cpu | 运行时 CPU 开销 | > 基线 30% |
| runtime_overhead_memory | 运行时内存开销 | > 基线 50% |
| runtime_errors | 运行时错误 | > 0 |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 默认运行时 | 保持 runc | 沙箱仅用于不可信负载 |
| 节点隔离 | 专用节点池 | 避免混合部署 |
| 二进制权限 | 755，仅 root 可写 | 避免篡改 |
| 升级 | 先测试后生产 | 滚动升级 |
| 审计 | 记录运行时切换 | 便于安全审计 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| runc | gVisor | 安装 runsc→创建 RuntimeClass→修改 Pod |
| runc | Kata | 安装 kata→创建 RuntimeClass→修改 Pod |
| 单运行时 | 多运行时 | 配置多个 runtime→创建多个 RuntimeClass |
| 无 overhead | 有 overhead | 更新 RuntimeClass 添加 overhead |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| RuntimeClass 存在 | `kubectl get runtimeclass` | 包含预期名称 |
| 节点标签 | `kubectl get nodes --show-labels` | 包含 runtime 标签 |
| 运行时二进制 | `which runsc/kata-runtime` | 存在 |
| Pod 生效 | `kubectl get pod -o jsonpath='{.status.runtimeHandler}'` | 预期值 |
| 性能 | `kubectl top pod` | 在预期范围 |
| 回滚 | 删除 runtimeClassName | Pod 用 runc 运行 |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| RuntimeClass v1beta1 | K8s 1.12 | 初始引入 |
| RuntimeClass v1 | K8s 1.20 | GA 稳定 |
| overhead 支持 | K8s 1.18+ | 资源调度考虑开销 |
| scheduling 支持 | K8s 1.20+ | nodeSelector 隔离 |

## 架构对比

```text
RuntimeClass 工作流程：

Pod (runtimeClassName: gvisor)
  └── kubelet
       └── CRI: RunPodSandbox(runtime_handler="runsc")
            └── containerd
                 └── 查找 runtime_type = "io.containerd.runsc.v1"
                      └── containerd-shim-runsc-v1
                           └── runsc (gVisor)
                                └── 容器进程

多运行时共存：
  containerd
    ├── runc (default) → 普通容器
    ├── runsc (gvisor) → 沙箱容器
    └── kata (kata) → VM 容器
```

## 容量规划

| 场景 | 建议配置 | 说明 |
|------|----------|------|
| 通用负载 | runc (default) | 无额外开销 |
| 多租户 | gVisor | 10-30% 开销 |
| 强隔离 | Kata | 5-15% 开销 |
| Serverless | Firecracker | 5-10% 开销 |

## 检查清单（补充）

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 节点标签 | `kubectl get nodes -l runtime=gvisor` | 有节点 |
| 运行时二进制 | `ssh node which runsc` | 存在 |
| containerd 配置 | `ssh node containerd config dump` | 包含 runtime |
| Pod 调度 | `kubectl describe pod` | 调度到正确节点 |
| 运行时生效 | `kubectl get pod -o jsonpath='{.status.runtimeHandler}'` | 预期值 |
| RuntimeClass 存在 | `kubectl get runtimeclass` | 包含目标运行时 |
| 节点 taint | `kubectl describe node` | 无意外 taint |
| Pod 事件 | `kubectl get events --field-selector reason=FailedCreatePodSandBox` | 无错误 |
| 运行时二进制版本 | `ssh node runsc --version` | 符合预期 |

## 相关文档

- [[14-容器运行时/06-沙箱运行时/05-gvisor-sandbox-production.md|gVisor 生产指南]]
- [[14-容器运行时/06-沙箱运行时/06-firecracker-microvm-guide.md|Firecracker microVM]]
- [[14-容器运行时/03-containerd-CRI-O/04-kata-containers-secure-container.md|Kata Containers]]
- [[14-容器运行时/03-containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]

<!-- risk-assessed -->
