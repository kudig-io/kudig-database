---
title: GitBook 构建脚本
description: '- 构建后恢复原始配置'
category: general
tags:
- k8s
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GitBook 构建脚本 是什么
- 如何 GitBook 构建脚本
trigger_keywords:
- GitBook
- 构建脚本
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# GitBook 构建脚本

本目录包含用于构建、导出和维护 GitBook 的所有脚本。

## 📂 脚本分类

### 🐧 Linux/macOS 脚本 (Bash)

| 脚本 | 功能 | 使用场景 |
|:---|:---|:---|
| `start.sh` | 启动本地开发服务 | 本地预览和开发 |
| `refresh.sh` | 刷新构建 | 内容更新后重新构建 |
| `export-static.sh` | 导出静态离线版本 | 生成可分享的离线站点 |
| `generate-summary.sh` | 生成 SUMMARY.md | 自动创建目录索引 |

### 🪟 Windows 脚本 (PowerShell)

| 脚本 | 功能 | 推荐度 | 说明 |
|:---|:---|:---:|:---|
| `FINAL-BUILD.ps1` | 一键构建离线版本 | ⭐⭐⭐ | **最推荐**，整合所有功能 |
| `install-mdbook.ps1` | 安装 mdbook | ⭐⭐⭐ | 首次使用必备 |
| `generate-summary-utf8.ps1` | 生成目录索引（UTF-8） | ⭐⭐⭐ | 确保中文正确编码 |

## 🚀 快速使用

### Linux/macOS 用户

```bash
cd build-scripts

# 首次启动
bash start.sh

# 内容更新后
bash refresh.sh

# 导出离线版本
bash export-static.sh --zip
```

### Windows 用户

**最简单方式（推荐）：**
```cmd
cd gitbook
QUICK-BUILD.cmd
```

**或使用PowerShell：**
```powershell
cd gitbook/build-scripts

# 首次使用：安装 mdbook
.\install-mdbook.ps1

# 构建离线版本
.\FINAL-BUILD.ps1 -Zip
```

## 📖 详细说明

### start.sh

**功能**: 初始化并启动本地开发服务器

**流程**:
1. 更新符号链接
2. 生成 SUMMARY.md
3. 构建 HTML
4. 启动 mdbook serve

**用法**:
```bash
bash start.sh          # 默认端口 3000
PORT=8080 bash start.sh  # 自定义端口
```

### refresh.sh

**功能**: 内容更新后刷新构建

**用法**:
```bash
bash refresh.sh        # 完整刷新
bash refresh.sh build  # 仅重新构建
```

**场景**:
- 新增/删除文件 → 完整刷新
- 仅修改内容 → 仅构建模式

### export-static.sh

**功能**: 导出可离线使用的静态 HTML 站点

**特性**:
- 自动移除 site-url（兼容 file:// 协议）
- 生成相对路径
- 构建后恢复原始配置

**用法**:
```bash
bash export-static.sh         # 导出到 dist/
bash export-static.sh --zip   # 导出并打包
```

### generate-summary.sh

**功能**: 自动生成 SUMMARY.md 目录索引

**特性**:
- 扫描所有 domain 和 topic 目录
- 从 README.md 提取标题
- 生成分层目录结构

**通常无需单独运行**，由其他脚本自动调用。

### FINAL-BUILD.ps1 ⭐

**功能**: Windows 下的一键构建解决方案

**流程**:
1. 清理旧文件
2. 创建目录 Junction
3. 复制 README
4. 生成 SUMMARY.md
5. 修改配置并构建
6. 恢复原始配置
7. 生成统计信息
8. 可选：打包 ZIP

**用法**:
```powershell
.\FINAL-BUILD.ps1          # 构建
.\FINAL-BUILD.ps1 -Zip     # 构建并打包
```

**输出**: `dist/` 目录包含完整的静态网站

### install-mdbook.ps1

**功能**: 在 Windows 上自动安装 mdbook

**流程**:
1. 下载最新版 mdbook
2. 解压到 `~/.local/bin/`
3. 添加到用户 PATH
4. 验证安装

**用法**:
```powershell
.\install-mdbook.ps1
```

安装后重启 PowerShell 生效。

### generate-summary-utf8.ps1

**功能**: Windows 版本的 SUMMARY.md 生成器（UTF-8编码）

**特性**:
- 与 bash 版本功能相同
- 支持方括号转义（mdBook 兼容性）
- 处理 Windows 路径
- **确保中文UTF-8编码正确**

**通常由 QUICK-BUILD.cmd 和 FINAL-BUILD.ps1 自动调用**。

## 🔧 脚本执行要求

### Linux/macOS

- Bash shell
- 已安装 mdbook
- 有权限创建符号链接

### Windows

- PowerShell 5.1 或更高版本
- 执行策略：`-ExecutionPolicy Bypass`
- 推荐以管理员身份运行（创建 Junction）

## 📝 脚本选择指南

### 我应该用哪个脚本？

**Linux/macOS 用户**:
- 开发调试 → `start.sh`
- 内容更新 → `refresh.sh`
- 导出分享 → `export-static.sh --zip`

**Windows 用户**:
- 首次使用 → `install-mdbook.ps1`
- **日常构建（推荐）** → 直接双击 `QUICK-BUILD.cmd`
- 打包分享 → `FINAL-BUILD.ps1 -Zip`

### 脚本依赖关系

```
Linux/macOS:
start.sh → generate-summary.sh
refresh.sh → generate-summary.sh
export-static.sh → generate-summary.sh

Windows:
QUICK-BUILD.cmd → generate-summary-utf8.ps1
FINAL-BUILD.ps1 → generate-summary-utf8.ps1
```

## ⚠️ 注意事项

1. **符号链接**
   - Linux/macOS: 使用 `ln -s`
   - Windows: 使用 `mklink /J` (Junction)

2. **路径处理**
   - Bash 脚本使用 Unix 风格路径
   - PowerShell 脚本处理 Windows 路径

3. **编码问题**
   - PowerShell 脚本使用 UTF-8 编码
   - 避免中文路径和特殊字符

4. **权限要求**
   - 符号链接可能需要管理员权限
   - Junction 创建需要特殊权限

## 🐛 问题排除

**脚本执行失败**:
```powershell
# Windows: 设置执行策略
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
```

**符号链接创建失败**:
- Windows: 以管理员身份运行
- Linux: 检查文件权限

**mdbook 未找到**:
```bash
# 检查安装
mdbook --version

# Windows: 检查 PATH
echo $env:Path
```

**构建错误**:
1. 检查 SUMMARY.md 格式
2. 查看错误日志
3. 删除 book/ 和 dist/ 重新构建

## 📚 更多信息

- 详细使用文档：`../documentation/`
- 项目说明：`../README.md`
- mdBook 官方文档：https://rust-lang.github.io/mdBook/

---

**提示**: Windows 用户优先使用 `FINAL-BUILD.ps1`，这是经过测试和优化的推荐方案。

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
