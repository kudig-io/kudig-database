---
title: 远程顾问模式改进执行计划
category: execution-plan
tags: [execution, remote-advisor, dialogue, skill]
created: "2026-05-23"
updated: "2026-05-23"
---

# 远程顾问模式改进执行计划

## 阶段 3.5（立即执行）— 对话式顾问转型

### 任务 1：5 个核心 Skill 对话脚本构建
- **目标**：将手册格式转为「对话脚本 + 信息收集清单 + 替代方案」
- **Skill 列表**：
  1. k8s-node-notready（最高频）
  2. k8s-pod-crashloop（最高频）
  3. k8s-dns-failure（高影响）
  4. k8s-deployment-rollout（高频）
  5. k8s-certificate-expiry（高影响）
- **输出**：每个 Skill 新增 `DIALOGUE.md` 文件

### 任务 2：17 个 Skill 信息收集清单
- **目标**：每个 Skill 添加「远程顾问需要收集的信息」清单
- **输出**：修改现有 SKILL.md，添加 `## 远程顾问信息收集` 章节

### 任务 3：核心命令替代方案
- **目标**：为每个核心命令添加「如果无法执行」的 2-3 个替代方案
- **输出**：修改 SKILL.md 中的命令代码块

### 任务 4：QA Action 补充
- **目标**：将 QA Action 覆盖率提升至 80%+
- **输出**：修改 `command-output-diagnosis-p0.yaml`

## 验收标准
- 每个核心 Skill 有完整的对话脚本（≥10 轮对话）
- 每个 Skill 有信息收集清单
- 每个核心命令有替代方案
- QA Action 覆盖率 ≥80%


## 参见

- [[DIALOGUE]] — dialogue 领域核心页面
