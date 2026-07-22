---
title: Eraser [entities]
description: '## 概述'
summary: 'Eraser 是一个 Kubernetes 原生的镜像清理工具，用于自动从集群节点中删除存在漏洞的和未使用的容器镜像。它通过与漏洞扫描器（如 [[Trivy|Trivy]]）集成，定期扫描节点上的镜像，自动移除包含高危漏洞的镜像，减小节点的攻击面并释放磁盘空间。'
category: entities
tags:
- k8s
- cncf
- image
- eraser
- coredns
- containerd
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Eraser 是什么
- 如何 Eraser
trigger_keywords:
- Eraser
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Eraser

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

Eraser 是一个 CNCF 沙箱项目，由 Microsoft 开源，是 Kubernetes 集群的自动化镜像清理工具。它定期扫描节点上的容器镜像，删除不安全或不再使用的镜像，减少节点存储消耗和攻击面。Eraser 特别关注安全管理——可以自动删除包含已知漏洞（CVE）的镜像，防止不安全镜像在节点上被使用。与 K8s 原生的镜像垃圾回收（基于磁盘阈值）不同，Eraser 提供基于策略的主动镜像清理。

## Key Features（核心能力）

- **漏洞扫描清理**：集成 Trivy 自动扫描并删除包含 CVE 的镜像
- **未使用镜像清理**：删除节点上没有运行容器的镜像
- **镜像排除列表**：通过配置保护关键镜像不被清理
- **定时清理**：通过 CronJob 或 EraserSchedule CRD 定期执行
- **节点资源释放**：可视化报告清理后的存储空间回收
- **ImageList 管理**：通过 CRD 声明式管理需要清理的镜像列表

## 架构与工作原理

Eraser 由 Manager、Collector 和 Remover 三个组件构成。Manager 作为 Controller 管理清理任务的生命周期；Collector 以 DaemonSet 方式运行在每个节点上，扫描本地镜像列表和漏洞信息；Remover 执行实际的镜像删除操作（通过 containerd/CRI API）。通过 ImageList CRD 声明需要删除的镜像，Manager 协调各节点上的 Collector 和 Remover 执行清理。

## K8s 集成

Eraser 通过 CRD 与 Kubernetes 集成。ImageList CRD 定义需要从节点清理的镜像列表（通过镜像名或正则匹配）。Eraser ConfigMap 配置全局策略（排除列表、扫描器配置）。Manager 通过 Deployment 部署，Collector/Remover 通过 DaemonSet 运行在每个节点。通过 containerd 的 Image Service API 或 nerdctl 执行镜像删除。

## 生产用例

- **节点存储管理**：定期清理未使用镜像释放节点磁盘
- **安全漏洞修复**：自动删除包含已知 CVE 的镜像
- **合规要求**：确保节点上不残留过期或不安全的镜像
- **大规模集群维护**：数千节点集群的镜像清理自动化

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add eraser https://eraser-dev.github.io/eraser/charts
helm repo update

# 🟢 安装 Eraser
helm install eraser eraser/eraser \
  -n eraser-system --create-namespace \
  --set vulnerabilityReport.enabled=true \
  --set scanner.type=trivy

# 🟢 验证安装
kubectl get pods -n eraser-system
kubectl get crd | grep eraser

# 🟢 查看节点镜像状态
kubectl get imagelist -A
kubectl get vulnerabilityreport -A
```

### ImageList CRD 示例

```yaml
# 声明式清理指定镜像
apiVersion: eraser.sh/v1
kind: ImageList
metadata:
  name: cleanup-vulnerable-images
spec:
  images:
    - name: "my-registry.com/app:v1.0-old"
    - name: "my-registry.com/app:v1.1-deprecated"
    - name: "docker.io/library/nginx:1.19"  # 包含已知 CVE
---
# 自动清理策略（基于漏洞扫描）
apiVersion: eraser.sh/v1
kind: ImageList
metadata:
  name: auto-vulnerability-cleanup
spec:
  # 自动清理包含严重/高危 CVE 的镜像
  vulnerabilityPolicy:
    severity:
      - CRITICAL
      - HIGH
    # 排除列表（不清理的镜像）
    excludedImages:
      - "registry.k8s.io/*"
      - "quay.io/prometheus/*"
      - "docker.io/library/busybox:*"
---
# Eraser 全局配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: eraser-config
  namespace: eraser-system
data:
  eraser-config.yaml: |
    scanner:
      type: trivy
      severity:
        - CRITICAL
        - HIGH
    cleanup:
      schedule: "0 2 * * *"  # 每天凌晨2点
      repeatPeriod: 24h
    filter:
      excludedImages:
        - "registry.k8s.io/pause:*"
        - "registry.k8s.io/coredns:*"
```

## 运维操作

```bash
# 🟢 查看清理任务状态
kubectl get imagelist -A
kubectl get vulnerabilityreport -A

# 🟢 查看节点镜像清理报告
kubectl get imagelist cleanup-vulnerable-images -o yaml

# 🟢 查看 Eraser 日志
kubectl logs -n eraser-system -l app=eraser-manager --tail=100

# 🟡 手动触发清理
kubectl annotate imagelist cleanup-vulnerable-images \
  eraser.sh/trigger-cleanup=$(date +%s) --overwrite

# 🟡 添加镜像到排除列表
kubectl edit configmap eraser-config -n eraser-system

# 🔴 删除 ImageList（停止清理策略）
kubectl delete imagelist cleanup-vulnerable-images
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 镜像未被清理 | 排除列表包含 | `kubectl get configmap eraser-config` | 检查排除规则 |
| 扫描失败 | Trivy 不可用 | `kubectl logs -n eraser-system` | 检查 Trivy 镜像拉取 |
| 节点磁盘未释放 | 镜像仍在使用 | `crictl images` | 确认无容器使用该镜像 |
| CRD 未生效 | Manager 异常 | `kubectl get pods -n eraser-system` | 重启 Manager |

```bash
# 排查流程
# 1. 检查 Eraser 组件状态
kubectl get pods -n eraser-system
kubectl logs -n eraser-system -l app=eraser-manager --tail=50

# 2. 检查节点镜像状态
kubectl get nodes -o name | while read node; do
  echo "=== $node ==="
  kubectl debug $node -it --image=alpine -- crictl images 2>/dev/null
done

# 3. 检查清理事件
kubectl get events -n eraser-system --sort-by='.lastTimestamp'

# 4. 检查漏洞报告
kubectl get vulnerabilityreport -A -o wide
```

## 生产案例

### 案例1：节点存储自动管理
- **场景**：500 节点集群，节点磁盘经常被旧镜像占满导致 Pod 调度失败
- **方案**：Eraser 每日清理未使用镜像；排除列表保护系统镜像；清理报告可视化
- **效果**：节点磁盘使用率从 85% 降到 50%，磁盘压力导致的调度失败降为 0

### 案例2：CVE 紧急响应
- **场景**：发现基础镜像包含严重 CVE，需要快速清理所有节点上的受影响镜像
- **方案**：创建 ImageList 指定受影响镜像；Eraser 自动在所有节点执行清理；验证无容器使用受影响镜像
- **效果**：500 节点清理完成时间 < 30min，替代原来 2天 的手工操作

## 对比替代方案

| 维度 | Eraser | K8s 原生 GC | 手动脚本 | Trivy+CI |
|------|--------|-----------|---------|----------|
| 主动清理 | 是 | 否(被动) | 是 | 否 |
| 漏洞扫描 | 内置 | 无 | 无 | 核心 |
| 声明式 | CRD | 无 | 无 | 无 |
| 多节点 | 自动 | 每节点 | 手动 | 无 |
| 学习曲线 | 低 | - | 低 | 中 |

## 检查清单

- [ ] Eraser 已部署且所有组件 Running
- [ ] 排除列表已配置（保护系统镜像）
- [ ] 清理策略已在测试节点验证
- [ ] 漏洞扫描器已配置（Trivy）
- [ ] 清理报告已配置可视化
- [ ] 清理时间窗口已配置（避免影响业务）
- [ ] 告警已配置（清理失败/磁盘压力）

## Related

- [[实体/external-secrets.md|secrets]]]] — External Secrets Operator
- [[kube-burner]] — Kube-burner
- [[实体/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[coredns]] — CoreDNS

- eraser
- [[实体/zot.md|zot]]
- [[实体/kitops.md|KitOps]]
- [[实体/copa.md|Copa (Copacetic)]]
- [[实体/stacker.md|Stacker]]
- [[实体/xregistry.md|xRegistry]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
