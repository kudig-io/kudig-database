# 清理包含 " 2" 的重复文件 — 操作报告

## 操作摘要

| 项目 | 数量 |
|------|------|
| 删除的重复文件 | 853 |
| 删除的重复目录 | 106 |
| 总计删除项 | 959 |
| 误删文件 | 0 |
| 未找到原始对应项的文件 | 0 |

## 重复判断依据

- **模式识别**：文件名/目录名中包含 " 2"（空格+数字2）后缀
- **验证规则**：每个 " 2" 项必须存在去除 " 2" 后的对应原始项
- **排除规则**：文件名中 "2" 是序号一部分的文件（如 `02-xxx.md`、`20-xxx.md`）不属于重复

## 删除范围

### 涉及的域名目录
- domain-4-workloads
- domain-5-networking
- domain-6-storage
- domain-7-security
- domain-8-observability
- domain-9-platform-ops
- domain-10-extensions
- domain-12-troubleshooting
- domain-13-docker
- domain-14-linux
- domain-17-cloud-provider
- domain-18-production-operations
- domain-20 到 domain-40

### 涉及的主题目录
- topic-ai-agent, topic-ai-coding, topic-cheat-sheet
- topic-deployment, topic-dictionary, topic-febm, topic-fta
- topic-learn, topic-migration, topic-presentations, topic-publish
- topic-release-notes, topic-skills, topic-structural-trouble-shooting

### 涉及的其他目录
- gitbook/（含 book/ 子目录中的大量重复文件和目录）
- man/、reports/、visualizations/

## 文件类型分布

| 文件类型 | 数量（约） |
|----------|-----------|
| .md | ~700 |
| .html | ~130 |
| .cmd | ~3 |
| .toml | ~1 |
| .pdf | ~2 |
| .xmind | ~1 |
| .json | ~1 |
| .svg | ~1 |
| .pptx | ~1 |

## 验证结果

- 删除后再次扫描：含 " 2" 模式的文件数为 0
- 删除后再次扫描：含 " 2" 模式的目录数为 0
- 所有原始文件/目录完好保留
