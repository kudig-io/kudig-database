---
title: Pod CrashLoopBackOff & OOMKilled 诊断与修复 — 数字人播报脚本 (video-scripts)
description: '**内容类型**: Skills 运维技能'
summary: '**内容类型**: Skills 运维技能'
category: general
tags:
- k8s
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod CrashLoopBackOff & OOMKilled 诊断与修复 — 数字人播报脚本 是什么
- 如何 Pod CrashLoopBackOff & OOMKilled 诊断与修复 — 数字人播报脚本
trigger_keywords:
- Pod
- CrashLoopBackOff
- OOMKilled
- 诊断与修复
- 数字人播报脚本
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod CrashLoopBackOff & OOMKilled 诊断与修复 — 数字人播报脚本

> **生成时间**: 2026-05-18 21:09
> **内容类型**: Skills 运维技能
> **目标受众**: SRE, Ops Engineer
> **预计时长**: 10 分钟

---

## 视频结构

| 段落 | 内容 | 时长 | 镜头 |
|:---|:---|:---:|:---|
| 开场 | 症状识别与影响评估 | 12s | 主播近景 |
| 诊断 | 分步诊断工作流 | 50s | 终端+图示 |
| 修复 | 修复操作执行 | 35s | 操作界面 |
| 验证 | 修复确认与监控 | 20s | 监控面板 |
| 结尾 | 升级路径与要点 | 15s | 主播近景 |

---

## 段落一：症状识别（12s）

**主播台词**：
> 大家好，今天我们来讲解 Pod CrashLoopBackOff & OOMKilled 诊断与修复 的完整处理流程。
作为 SRE 工程师，掌握这个技能可以快速定位和解决常见问题。

**症状列表**：
- 症状1
- 症状2

---

## 段落二：诊断工作流（50s）

**诊断步骤**：

1. Step D1.1**: 获取 Pod 全局状态

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o wide
  ```
2. Step D1.2**: 获取 Pod 详细描述（核心诊断信息）

- **命令**:
  ```bash
  kubectl describe pod <pod> -n <namespace>
  ```
3. Step D1.4**: 检查 Init Containers 状态

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.initContainerStatuses[*]}{"init:"}{.name}{" state:"}{.state}{" restarts:"}{.restartCount}{"\n"}{end}'
  ```
4. Step D2.1**: 检查容器日志（最重要的诊断信息源）

- **命令**:
  ```bash
  # 查看当前容器日志（如果容器正在运行）
  kubectl logs <pod> -n <namespace> -c <[[entities/docker.md|container]]>
  
  # 查看上一次崩溃的容器日志（CrashLoop 场景必用）
  kubectl logs <pod> -n <namespace> -c <container> --previous
  
  # 如果日志很长，只看最后 100 行
  kubectl logs <pod> -n <namespace> -c <container> --previous --tail=100
  ```
5. Step D2.2**: 检查容器启动命令和参数

- **命令**:
  ```bash
  # 查看 Pod spec 中的 command 和 args
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  command: "}{.command}{"\n  args: "}{.args}{"\n  image: "}{.image}{"\n"}{end}'
  ```
6. Step D2.4**: 检查环境变量、ConfigMap、Secret

- **命令**:
  ```bash
  # 查看容器环境变量（包括来自 ConfigMap/Secret 的引用）
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[0].env[*]}{.name}{"="}{.value}{.valueFrom}{"\n"}{end}'
  
  # 查看 envFrom 引用的 ConfigMap/Secret
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[0].envFrom[*]}{"configMapRef: "}{.configMapRef.name}{"  secretRef: "}{.secretRef.name}{"\n"}{end}'
  
  # 检查引用的 ConfigMap 是否存在
  kubectl get configmap <configmap-name> -n <namespace>
  
  # 检查引用的 Secret 是否存在
  kubectl get secret <secret-name> -n <namespace>
  
  # 检查 volume mounts 中引用的 ConfigMap/Secret
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.volumes[*]}{"volume: "}{.name}{" configMap: "}{.configMap.name}{" secret: "}{.secret.secretName}{"\n"}{end}'
  ```

**主播台词**：
> 接下来我们按照诊断工作流逐步排查。

---

## 段落三：修复操作（35s）

**🟡 中风险**：建议人工审批后执行

**修复命令**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查同一 Deployment/StatefulSet 下所有 Pod 的状态
kubectl get pods -n <namespace> -l <label-selector> -o wide
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Pod 所属的 Deployment 和 Namespace
kubectl get pod <pod> -n <namespace> -o jsonpath='{.metadata.ownerReferences[0].kind}/{.metadata.ownerReferences[0].name}'
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Deployment 的 ready 副本数
kubectl get deployment <deployment> -n <namespace> -o jsonpath='Ready: {.status.readyReplicas}/{.status.replicas}'
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Deployment 的最近 rollout 历史
kubectl rollout history deployment/<deployment> -n <namespace> --revision=0
```
**主播台词**：
> 修复操作需要谨慎，请确保已备份配置。

---

## 段落四：验证确认（20s）

**验证步骤**：

1. 验证步骤...
2. 验证步骤...
3. 验证步骤...

**主播台词**：
> 修复后需要验证确认，确保服务恢复正常。

---

## 段落五：结尾（15s）

**主播台词**：
> 以上就是完整的处理流程。遇到无法解决的问题，请及时升级。

---

## 数字人参数配置

| 参数 | 值 |
|:---|:---|
| 形象 | SRE 工程师（实战派） |
| 声音 | 中文女声（清晰专业） |
| 语速 | 1.3x（修复部分 1.0x） |
| 分辨率 | 1920x1080 |

---

## 关联知识库

- Skill 源文档：domain-10-troubleshooting-diagnostics/topic-skills/02-pod-crashloop-oomkilled.md
- 相关 FTA：参考 domain-10-troubleshooting-diagnostics/topic-fta/
- 深度排查：参考 domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/


<!-- risk-assessed -->
