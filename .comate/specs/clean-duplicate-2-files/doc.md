# 清理包含 " 2" 的重复文件

## 需求场景

项目中存在大量重复文件和目录，其命名模式为在原始文件名后追加 " 2"（空格+数字2）。例如：
- `01-docker-architecture-overview 2.md` 是 `01-docker-architecture-overview.md` 的重复
- `tools 2/` 是 `tools/` 目录的重复
- `README 2.md` 是 `README.md` 的重复

这些重复文件/目录的存在可能是由于某种文件复制或导出操作导致的，需要清理以保持项目整洁。

## 重复判断标准

**核心模式**：文件名或目录名中包含 " 2"（空格+2）后缀，且存在去除 " 2" 后的对应原始文件/目录。

**区分规则**：
- `02-deployment-production-patterns.md` — **不是重复**，"2" 是序号的一部分
- `12-cluster-deployment-patterns.md` — **不是重复**，"2" 是序号的一部分
- `20-kubelet-configuration.md` — **不是重复**，"2" 是序号的一部分
- `01-docker-architecture-overview 2.md` — **是重复**，" 2" 是追加的后缀
- `tools 2/` — **是重复**，" 2" 是追加的目录后缀

## 涉及的文件和目录

### 文件（" 2.md" / " 2.xmind" / " 2.pdf" / " 2.html" / " 2.json" / " 2.svg" / " 2.cmd" / " 2.toml" 模式）

涉及的域名目录（每个都有多个重复文件）：
- domain-4-workloads（约 25 个重复文件）
- domain-5-networking（约 55 个重复文件，含 .xmind）
- domain-6-storage（约 17 个重复文件）
- domain-7-security（约 22 个重复文件）
- domain-8-observability（约 30 个重复文件）
- domain-9-platform-ops（约 26 个重复文件）
- domain-10-extensions（约 17 个重复文件）
- domain-12-troubleshooting（约 43 个重复文件）
- domain-13-docker（约 12 个重复文件）
- domain-14-linux（约 10 个重复文件）
- domain-18-production-operations（约 4 个重复文件）
- domain-20 到 domain-40（每个目录约 10-40 个重复文件）
- topic-ai-agent（1 个重复文件）
- topic-ai-coding（约 25 个重复文件）
- topic-cheat-sheet（约 10 个重复文件）
- topic-deployment（约 5 个重复文件）
- topic-dictionary（约 2 个重复 .md 文件）
- topic-febm（约 10 个重复文件，含 .pdf）
- topic-fta（约 30 个重复文件）
- topic-migration（约 10 个重复文件）
- topic-presentations（约 12 个重复文件）
- topic-publish（约 5 个重复文件）
- topic-release-notes（1 个重复 .md 文件）
- topic-skills（约 20 个重复文件）
- topic-structural-trouble-shooting（2 个重复 .md 文件）
- gitbook（约 8 个重复文件，含 .cmd / .toml）
- visualizations（6 个重复文件，含 .html / .json / .svg）

### 目录（" 2/" 模式）

- domain-12-troubleshooting/tools 2/
- domain-17-cloud-provider/ 下 13 个 "2/" 子目录
- domain-34-cncf-landscape/ 下 3 个 "2/" 子目录（graduated 2/, incubating 2/, sandbox 2/）
- gitbook/ 下 6 个 "2/" 子目录
- man/ 下 2 个 "2/" 子目录
- reports/quality 2/
- topic-ai-agent/openclaw-workspace 2/
- topic-dictionary/ 下 12 个 "2/" 子目录
- topic-learn/ 下 2 个 "2/" 子目录
- topic-release-notes/ 下 8 个 "2/" 子目录
- topic-skills/ 下 2 个 "2/" 子目录
- topic-structural-trouble-shooting/ 下 12 个 "2/" 子目录

## 技术方案

使用 shell 脚本（find + rm）批量处理：

1. **第一步：生成完整清单** — 使用 `find` 命令定位所有文件名包含 " 2" 的文件和目录
2. **第二步：验证重复关系** — 对每个 " 2" 文件/目录，检查是否存在去除 " 2" 后的对应原始项
3. **第三步：内容对比（可选）** — 对 .md 文件使用 `diff` 确认内容是否相同
4. **第四步：批量删除** — 删除确认的重复文件和目录
5. **第五步：生成报告** — 记录所有被删除的项

### 关键命令

```bash
# 查找所有文件名含 " 2" 的文件（排除 .git 目录）
find . -not -path './.git/*' -not -path './.git.corrupted/*' -name '* 2.*' -type f

# 查找所有目录名含 " 2" 的目录
find . -not -path './.git/*' -not -path './.git.corrupted/*' -name '* 2' -type d

# 删除重复文件
find . -not -path './.git/*' -name '* 2.*' -type f -delete

# 删除重复目录
find . -not -path './.git/*' -name '* 2' -type d -exec rm -rf {} +
```

## 边界条件和异常处理

1. **只删除确认有对应原始项的重复文件** — 如果 " 2" 文件没有对应的原始文件，不删除
2. **不删除文件名中 "2" 是序号一部分的文件** — 如 `02-xxx.md`、`12-xxx.md`、`20-xxx.md`
3. **目录删除需递归** — 使用 `rm -rf` 删除目录及其内容
4. **排除 .git 和其他系统目录** — 不处理 .git、.git.corrupted、.zread、.ruff_cache 等目录
5. **处理特殊字符** — 文件名中可能包含空格，需正确引用

## 预期结果

- 预计删除约 400+ 个重复文件
- 预计删除约 60+ 个重复目录
- 项目文件结构更加整洁
- 所有删除操作有完整日志记录
