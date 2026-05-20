---
title: 开源项目索引模板
description: '- [五、选型指南](#五选型指南)'
category: general
tags:
- k8s
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 开源项目索引模板 是什么
- 如何 开源项目索引模板
trigger_keywords:
- 开源项目索引模板
---

# 开源项目索引模板

> 用于为各 Domain 快速创建 `00-open-source-projects-index.md` 标准化文件

---

## 复制以下模板并填充

```markdown
# Domain-XX 领域名称 — 开源项目索引

> **最后更新**: YYYY-MM-DD
> **适用版本**: 核心项目 vX.Y

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、子分类 A](#二子分类-a)
- [三、子分类 B](#三子分类-b)
- [四、版本兼容矩阵](#四版本兼容矩阵)
- [五、选型指南](#五选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Project-A** | 简短描述 | Graduated/Incubating/Sandbox/非 CNCF | vX.Y.Z | Xk+ | Apache-2.0 |
| **Project-B** | 简短描述 | ... | vX.Y.Z | Xk+ | ... |

---

## 二、子分类 A

### 2.1 Project-A 详解

```yaml
# 核心特性
- 特性 1
- 特性 2
```

**版本里程碑**
- **vX.Y**: 重大更新说明

**GitHub**: https://github.com/org/repo
**文档**: https://docs.example.com/

---

## 三、子分类 B

### 3.1 Project-B 详解

...

---

## 四、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Project-A vX.Y | ✅ | ✅ | ✅ | 说明 |
| Project-B vX.Y | ✅ | ✅ | ✅ | 说明 |

---

## 五、选型指南

```
决策树文本或架构图
```

---

## 参考链接

- [Project-A 文档](https://docs.example.com/)
- [相关白皮书](https://github.com/cncf/...)
```

---

## 填写规范

1. **项目表格**必须包含: 项目名称、作用、CNCF 状态、最新版本、Stars、License
2. **CNCF 状态**准确使用: Graduated / Incubating / Sandbox / 非 CNCF / K8s SIG
3. **版本信息**标注数据日期，建议每季度更新一次
4. **选型指南**提供决策树或分层架构图，帮助读者快速决策
5. **参考链接**指向官方文档和 CNCF 白皮书

---

## 已完成的 Domain 索引 (参考示例)

| Domain | 文件 |
|:---|:---|
| domain-1 架构基础 | `domain-1/00-open-source-projects-index.md` |
| domain-10 扩展 | `domain-10/00-open-source-projects-index.md` |
| domain-11 AI 基础设施 | `domain-11/00-open-source-projects-index.md` |
| domain-12 故障排查 | `domain-12/00-open-source-projects-index.md` |
| domain-13 Docker | `domain-13/00-open-source-projects-index.md` |
| domain-15 网络基础 | `domain-15/00-open-source-projects-index.md` |
| domain-16 存储基础 | `domain-16/00-open-source-projects-index.md` |
| domain-17 云厂商 | `domain-17/00-open-source-projects-index.md` |
| domain-18 生产运维 | `domain-18/00-open-source-projects-index.md` |
| domain-20 监控告警 | `domain-20/00-open-source-projects-index.md` |
| domain-21 日志管理 | `domain-21/00-open-source-projects-index.md` |
| domain-22 镜像管理 | `domain-22/00-open-source-projects-index.md` |
| domain-23 GitOps CI/CD | `domain-23/00-open-source-projects-index.md` |
| domain-24 IaC | `domain-24/00-open-source-projects-index.md` |
| domain-25 云原生安全 | `domain-25/00-open-source-projects-index.md` |
| domain-26 服务网格 | `domain-26/00-open-source-projects-index.md` |
| domain-27 多云混合 | `domain-27/00-open-source-projects-index.md` |
