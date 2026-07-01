---
title: 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation — 数字人播报脚本 (video-scripts)
description: '**内容类型**: Skills 运维技能'
category: general
tags:
- k8s
- kubelet
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation — 数字人播报脚本 是什么
- 如何 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation — 数字人播报脚本
trigger_keywords:
- 节点
- NotReady
- 诊断与修复
- Node
- NotReady
- Diagnosis
- Remediation
- 数字人播报脚本
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation — 数字人播报脚本

> **生成时间**: 2026-05-18 20:57
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
> 大家好，今天我们来讲解 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation 的完整处理流程。
作为 SRE 工程师，掌握这个技能可以快速定位和解决常见问题。

**症状列表**：
- 症状1
- 症状2

---

## 段落二：诊断工作流（50s）

**诊断步骤**：

1. Step D1.1**: 获取节点全局状态概览
- **命令**:
  ```bash
  kubectl get nodes -o wide
  ```
2. Step D1.2**: 获取节点详细状态和 Conditions
- **命令**:
  ```bash
  kubectl describe node <node-name>
  ```
3. Step D1.3**: 检查节点事件
- **命令**:
  ```bash
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp --no-headers | tail -30
  ```
4. Step D1.4**: 检查节点 Taints
- **命令**:
  ```bash
  kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
  ```
5. Step D1.5**: 检查节点 Lease 对象
- **命令**:
  ```bash
  kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
  ```
6. Step D2.1**: 检查 kubelet 服务状态
- **命令**:
  ```bash
  ssh <node-ip> "systemctl status kubelet"
  ```

**主播台词**：
> 接下来我们按照诊断工作流逐步排查。

---

## 段落三：修复操作（35s）

**🟡 中风险**：建议人工审批后执行

**修复命令**：

```bash
# 获取所有节点状态，统计 NotReady 数量
kubectl get nodes --no-headers | awk '{print $2}' | sort | uniq -c
# 或更精确的统计
echo "NotReady nodes:" && kubectl get nodes --no-headers | grep -c "NotReady" && \
echo "Total nodes:" && kubectl get nodes --no-headers | wc -l
```
```bash
# 检查 NotReady 节点是否包含 control-plane/master 角色
kubectl get nodes --no-headers | grep "NotReady" | grep -E "control-plane|master"
```
```bash
# 查看 NotReady 节点上运行的 [[concepts/pod-lifecycle.md|pod]] 数量和关键 namespace
NODE_NAME="<notready-node>"
kubectl get pods --all-namespaces --field-selector spec.nodeName=${NODE_NAME} --no-headers | \
  awk '{print $1}' | sort | uniq -c | sort -rn
```
```bash
# 检查节点 Ready condition 的 lastTransitionTime
kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,LAST_TRANSITION:.status.conditions[-1].lastTransitionTime | grep -v "NAME"
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

- Skill 源文档：domain-10-troubleshooting-diagnostics/topic-skills/01-node-notready.md
- 相关 FTA：参考 domain-10-troubleshooting-diagnostics/topic-fta/
- 深度排查：参考 domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/
