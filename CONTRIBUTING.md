---
title: 贡献指南
summary: 贡献指南：感谢你对 KUDIG 云原生运维知识库的兴趣！本指南说明如何为语料库贡献内容与工程改进。
category: meta
tags:
- meta
- visibility/public
- contributing
tier: core
sources:
- manual
created: 2026-01-16
updated: 2026-07-01
last_updated: 2026-07-01
---



# 贡献指南

感谢你对 KUDIG 云原生运维知识库的兴趣！本指南说明如何为语料库贡献内容与工程改进。

## 1. 项目性质

KUDIG 是一个**云原生（Kubernetes / AI Infra）运维诊断知识语料库**，双重用途：

- **人读**：作为 SRE / 平台工程师的运维手册与学习路径
- **机读**：作为 RAG 检索增强的知识源，供 AI 诊断 Agent 消费

内容组织为 **20 个知识域**（`domain-01` ~ `domain-20`）+ 跨层目录（`concepts/`、`entities/`、`skills/`）。详见 [STRUCTURE.md](STRUCTURE.md)。

## 2. 贡献类型

| 类型 | 说明 | 示例 |
|------|------|------|
| 内容新增 | 新建知识页 / 补充深度 | 新增 eBPF 安全实践页 |
| 内容修正 | 修复技术错误 / 过期信息 | 修正废弃 API、刷新镜像版本 |
| 工程改进 | 脚本、CI、构建链路 | 新增 lint、修复 broken link |
| 结构治理 | 去重、交叉链接、孤儿页救援 | wiki-lint 修复 |

## 3. 贡献流程

```bash
# 1. Fork 并拉取
git clone <your-fork>
cd kudig-database

# 2. 创建分支（按变更类型命名）
git checkout -b fix/alertmanager-deprecated-fields

# 3. 本地校验（提交前必跑，见第 5 节）
ruff check scripts/
python3 scripts/frontmatter-quality-check.py

# 4. 提交（遵循 Conventional Commits，见第 4 节）
git add -A && git commit -m "fix: 替换 Alertmanager 废弃字段"

# 5. 推送并发起 PR
git push origin fix/alertmanager-deprecated-fields
```

## 4. 提交规范（Conventional Commits）

| 前缀 | 用途 |
|------|------|
| `feat:` | 新增内容 / 功能 |
| `fix:` | 修复错误（技术错误、broken link、frontmatter）|
| `docs:` | 文档变更（README、CONTRIBUTING、报告）|
| `chore:` | 杂项（依赖、清理）|
| `ci:` | CI / 构建链路 |
| `dedup:` | 去重 / 合并 |

示例：`fix: 统一 ValidatingAdmissionPolicy GA 版本为 1.30`

## 5. 本地校验清单

提交前确保以下检查通过（CI 会强制执行部分项）：

- [ ] **ruff**：`ruff check scripts/`（Python 脚本 lint，line-length 120）
- [ ] **frontmatter**：每个 `.md` 必须有可解析的 YAML frontmatter，含 `title`、`category`、`tags`
- [ ] **wikilinks**：`bash scripts/check-broken-links.sh`（无新增 broken `[[...]]` 链接）
- [ ] **代码块**：`bash scripts/code-example-validation.sh`（YAML / bash 代码块语法有效）
- [ ] **构建**：`cd web && npm ci && npm run build`（Astro 站点可构建）

## 6. 内容质量标准

### 6.1 Frontmatter 规范

```yaml
---
title: "页面标题"              # 必需
description: "一句话摘要"       # 推荐
category: domain-XX-name       # 必需，对应所属域
tags: ["k8s", "observability"] # 必需
last_updated: 2026-07-01       # 必需，ISO 日期
difficulty: intermediate       # 推荐：beginner/intermediate/advanced
sources: ["kubernetes.io/..."] # 推荐，权威来源链接
---
```

### 6.2 技术准确性

- **K8s 版本特性**：声明 GA / Beta / Alpha 时必须标注准确版本（如 ValidatingAdmissionPolicy GA 于 1.30）
- **废弃项**：涉及已移除特性（如 PodSecurityPolicy、已废弃 API）必须附弃用横幅
- **配置示例**：YAML 字段名必须真实存在，禁止捏造（如 Alertmanager 用 `matchers` 而非已废弃的 `match_re`）
- **镜像版本**：示例镜像应使用较新稳定版（如 Prometheus 3.x，非 2.x 旧版）

### 6.3 链接

- 外部链接指向权威源（kubernetes.io、github.com 官方仓库、CNCF 项目站）
- 内部 wikilink `页面名` 指向必须存在的页面
- 每个 `_archives/` 与 `_archived-release-notes/` 下的内容不参与链接校验

## 7. PR 审查要点

- 是否符合 [STRUCTURE.md](STRUCTURE.md) 的目录约定
- 是否引入新的 broken link / orphan 页
- frontmatter 是否齐全且日期更新
- 技术声明是否有数据/来源支撑（避免猜测与幻觉）

## 8. 报告问题

通过 GitHub Issues 反馈：技术错误、过期内容、链接失效、内容缺口。

---

感谢你帮助提升云原生运维知识的质量与覆盖度！

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
