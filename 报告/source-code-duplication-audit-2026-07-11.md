---
title: 源码/ vs code/ 目录重复评估报告
description: 评估 源码/ 和 code/ 两个源码参考目录的重复情况，给出合并建议
category: reports
tags:
- reports
- quality
- cleanup
- duplicate
tier: supporting
created: '2026-07-11'
last_updated: 2026-07-11
---

# 源码/ vs code/ 目录重复评估报告

## 结论

**code/ 目录是 源码/ 的近似完全副本**，两目录共享 21 个相同的顶级子目录（含 .zip 归档），仅 源码/ 独有 4 个额外项目。

## 重复统计

| 维度 | 源码/ | code/ |
|------|-------|-------|
| 总文件数 | ~277,765 | ~265,000 |
| 总大小 | 4.4 GB | 4.1 GB |
| Markdown 文件 | 4,903 | 4,384 |
| 顶级子目录数 | 25 | 21 |
| 共享子目录 | 21 | 21 |
| 独有子目录 | 4 (alertmanager, higress, kong, terway) | 0 |

## 重复明细

### 完全相同的子目录（21 个）

`alibaba-cloud-csi-driver-1.36.1`, `apiserver-master`, `apisix-3.17.0`, `cloud-kernel-6.6.102-6`, `cloud-provider-master`, `coredns-011`, `coredns-1.14.4`, `flannel-0.28.7`, `helm-4.2.2`, `kong-3.9.3`, `kruise-1.9.1`, `kube-controller-manager-master`, `kube-proxy-master`, `kube-scheduler-master`, `kubeadm-main`, `kubectl-master`, `kubernetes-release-1.18` ~ `1.34`, `opentelemetry-collector-0.156.0`, `prometheus-3.13.0` 等

### 仅在 源码/ 中（4 个）

| 目录 | 文件数 | 说明 |
|------|--------|------|
| `alertmanager-0.33.1` | ~500 | Alertmanager 源码 |
| `higress-2.2.3` | ~2,286 | Higress 网关源码 |
| `kong-3.9.3` | ~2,407 | Kong 网关源码（含 .zip） |
| `terway-1.17.2` | — | Terway CNI 源码 |

## 建议

| 优先级 | 操作 | 理由 | 节省空间 |
|--------|------|------|---------|
| 🔴 高 | **删除 code/ 目录** | 完全重复，浪费 4.1GB | ~4.1 GB |
| 🟡 中 | 将 4 个独有目录移入 源码/ | 保留完整参考 | — |
| 🟢 低 | 在 .gitignore 中排除 源码/*.zip | .zip 是源码的压缩副本 | ~2 GB |

**预期效果**：删除 code/ 后可节省 ~4.1GB 磁盘空间，减少 Git 仓库体积，且不影响任何知识库内容（Agent 语料配置默认排除 源码/ 和 code/）。

## 风险评估

- **Agent 语料**：无影响（两个目录都在 exclude 列表中）
- **Git 历史**：如 code/ 已提交到 Git，需要 `git filter-branch` 清理历史
- **构建依赖**：无（知识库站点构建不依赖源码目录）

---

*生成时间：2026-07-11*
