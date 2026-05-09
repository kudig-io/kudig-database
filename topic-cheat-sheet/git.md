---
title: "Git 速查卡"
description: "版本控制日常操作快速参考，覆盖 Git 2.30+ 常用命令"
category: cheatsheet
tags: [git, version-control, cheatsheet, quick-reference, devops]
k8s_versions: []
last_updated: "2026-05"
authors:
  - name: "KUDIG Team"
    role: "contributor"
difficulty: "beginner"
related_docs:
  - path: "../domain-9-platform-ops/"
    desc: "GitOps 平台运维文档"
  - path: "../topic-cheat-sheet/linux.md"
    desc: "Linux 速查卡"
---

# Git 速查表

> 版本控制日常操作快速参考 | Git 2.30+ | **最后更新**: 2026-05

---

## 目录

- [配置](#配置)
- [基础操作](#基础操作)
- [分支管理](#分支管理)
- [查看与对比](#查看与对比)
- [撤销操作](#撤销操作)
- [远程仓库](#远程仓库)
- [Stash](#stash)
- [标签管理](#标签管理)
- [高级操作](#高级操作)
- [故障排查](#故障排查)

---

## 配置

### 全局配置

```bash
# 用户信息（必须）
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 编辑器设置
git config --global core.editor vim
git config --global core.editor "code --wait"    # VS Code

# 默认分支名
git config --global init.defaultBranch main

# 颜色输出
git config --global color.ui auto

# 查看配置
git config --list
git config user.name
```

### 别名配置

```bash
# 常用别名
git config --global alias.st status
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.lg "log --oneline --graph --all"

# 使用别名
git st          # git status
git co main     # git checkout main
git lg          # 图形化日志
```

---

## 基础操作

### 仓库初始化

```bash
# 新建仓库
git init
git init myproject

# 克隆仓库
git clone https://github.com/user/repo.git
git clone https://github.com/user/repo.git mydir    # 指定目录名
git clone --depth 1 https://github.com/user/repo.git # 浅克隆（仅最新提交）
```

### 日常提交流程

```bash
# 查看状态
git status
git status -s                     # 简短格式

# 添加文件到暂存区
git add filename                  # 添加单个文件
git add .                         # 添加所有变更
git add -A                        # 添加所有变更（包括删除）
git add -p                        # 交互式添加（按块）

# 提交更改
git commit -m "commit message"
git commit -a -m "message"        # 跳过 add，直接提交已跟踪文件
git commit --amend                # 修改最后一次提交

# 推送到远程
git push origin main
git push -u origin main           # 首次推送并设置上游
```

### 忽略文件 (.gitignore)

```bash
# 常用 .gitignore 模板
cat > .gitignore << 'EOF'
# 依赖目录
node_modules/
vendor/
__pycache__/
*.pyc

# 构建输出
dist/
build/
*.exe
*.dll
*.so

# IDE
.idea/
.vscode/
*.swp
*.swo
*~

# 操作系统
.DS_Store
Thumbs.db

# 日志和临时文件
*.log
*.tmp
*.temp
.env
.env.local

# 敏感信息
secrets.yaml
config/credentials.json
EOF
```

---

## 分支管理

### 基本操作

```bash
# 查看分支
git branch                        # 本地分支
git branch -r                     # 远程分支
git branch -a                     # 所有分支
git branch -v                     # 显示最后提交

# 创建分支
git branch feature-branch         # 创建分支
git checkout -b feature-branch    # 创建并切换
git switch -c feature-branch      # 新方式（Git 2.23+）

# 切换分支
git checkout main
git switch main                   # 新方式（Git 2.23+）

# 重命名分支
git branch -m old-name new-name   # 重命名当前分支
git branch -m new-name            # 重命名

# 删除分支
git branch -d feature-branch      # 删除已合并分支
git branch -D feature-branch      # 强制删除
```

### 合并与变基

```bash
# 合并分支
git checkout main
git merge feature-branch
git merge --no-ff feature-branch  # 禁用快进，保留分支历史

# 变基
git checkout feature-branch
git rebase main                   # 将 feature 变基到 main

# 交互式变基
git rebase -i HEAD~3              # 整理最近3个提交

# 继续/跳过/取消变基
git rebase --continue
git rebase --skip
git rebase --abort
```

### 分支策略

```bash
# 查看分支图
git log --oneline --graph --all
git log --oneline --graph --all --decorate

# 查看已合并/未合并分支
git branch --merged main          # 已合并到 main
git branch --no-merged main       # 未合并到 main
```

---

## 查看与对比

### 日志查看

```bash
# 基础日志
git log
git log --oneline                 # 简洁格式
git log --oneline -10             # 最近10条
git log --graph                   # 图形化显示
git log --all --graph --oneline   # 全部分支图形化

# 格式化输出
git log --pretty=format:"%h - %an, %ar : %s"
git log --pretty=oneline

# 查看文件修改历史
git log -p filename               # 显示详细差异
git log --stat filename           # 统计信息
git log --follow filename         # 跟踪重命名

# 查看某行代码历史
git blame filename
git blame -L 10,20 filename       # 查看 10-20 行
```

### 差异对比

```bash
# 工作区 vs 暂存区
git diff
git diff filename

# 暂存区 vs 最新提交
git diff --staged
git diff --cached

# 工作区 vs 指定提交
git diff HEAD
git diff HEAD~1

# 指定提交之间
git diff commit1 commit2
git diff main..feature            # main 到 feature 的差异

# 查看某次提交的修改
git show commit-hash
git show HEAD                     # 最后一次提交
git show HEAD~1                   # 倒数第二次
```

### 查看文件内容

```bash
# 查看指定版本文件
git show HEAD:filename
git show branch:path/to/file

# 查看暂存区文件
git show :filename
```

---

## 撤销操作

### 撤销修改

```bash
# 撤销工作区修改（未 add）
git checkout -- filename          # 旧方式
git restore filename              # 新方式（Git 2.23+）
git restore .                     # 撤销所有修改

# 撤销暂存区修改（已 add 未 commit）
git reset HEAD filename           # 旧方式
git restore --staged filename     # 新方式
git restore --staged .            # 撤销所有暂存

# 撤销提交（保留修改）
git reset --soft HEAD~1           # 撤销到最后一次提交，保留修改到暂存区

# 撤销提交（不保留修改）
git reset --hard HEAD~1           # 彻底丢弃最后一次提交

# 撤销提交（保留修改到工作区）
git reset --mixed HEAD~1          # 默认行为
```

### 修改历史

```bash
# 修改最后一次提交
git commit --amend
git commit --amend -m "新提交信息"
git commit --amend --no-edit      # 不修改提交信息

# 修改历史提交（交互式变基）
git rebase -i HEAD~3

# 合并多个提交
git rebase -i HEAD~3
# 在编辑器中将 pick 改为 squash 或 s
```

### 回滚到指定版本

```bash
# 查看历史版本
git log --oneline

# 回滚到指定版本（保留历史）
git revert commit-hash            # 生成新的提交，撤销指定提交的修改

# 回滚到指定版本（不保留历史，危险！）
git reset --hard commit-hash

# 查看所有操作记录（用于恢复）
git reflog
git reflog --all

# 从 reflog 恢复
git reset --hard HEAD@{2}
git checkout commit-hash
```

---

## 远程仓库

### 远程管理

```bash
# 查看远程仓库
git remote -v
git remote show origin

# 添加远程仓库
git remote add origin https://github.com/user/repo.git
git remote add upstream https://github.com/original/repo.git

# 修改远程 URL
git remote set-url origin https://new-url.git

# 删除远程仓库
git remote remove origin
```

### 同步操作

```bash
# 拉取更新
git fetch                         # 下载远程分支，不合并
git pull                          # fetch + merge
git pull --rebase                 # fetch + rebase（推荐）

# 拉取特定分支
git fetch origin main
git pull origin main

# 推送
git push origin main
git push -u origin main           # 首次推送并设置上游

# 强制推送（危险！）
git push -f origin main
git push --force-with-lease       # 较安全的强制推送

# 删除远程分支
git push origin --delete feature-branch
git push origin :feature-branch   # 旧语法

# 推送所有分支
git push --all origin

# 推送标签
git push origin tag-name
git push origin --tags            # 推送所有标签
```

### Fork 工作流

```bash
# 配置上游仓库
git remote add upstream https://github.com/original/repo.git

# 同步上游更新
git fetch upstream
git checkout main
git merge upstream/main
# 或
git rebase upstream/main

# 推送到自己的远程
git push origin main
```

---

## Stash

### 暂存操作

```bash
# 暂存当前修改
git stash
git stash push -m "描述信息"

# 暂存包括未跟踪文件
git stash -u
git stash --include-untracked

# 暂存并保留暂存区
git stash --keep-index

# 查看暂存列表
git stash list

# 应用暂存
git stash pop                     # 应用并删除
git stash apply                   # 应用但不删除
git stash apply stash@{2}         # 应用指定暂存

# 查看暂存内容
git stash show
git stash show -p                 # 显示完整差异

# 删除暂存
git stash drop stash@{0}          # 删除指定暂存
git stash clear                   # 删除所有暂存
```

---

## 标签管理

### 标签操作

```bash
# 查看标签
git tag
git tag -l "v1.*"                 # 过滤标签

# 创建轻量标签
git tag v1.0.0

# 创建附注标签（推荐）
git tag -a v1.0.0 -m "Version 1.0.0"

# 在指定提交创建标签
git tag -a v1.0.0 commit-hash -m "message"

# 推送标签到远程
git push origin v1.0.0
git push origin --tags            # 推送所有标签

# 删除标签
git tag -d v1.0.0                 # 删除本地标签
git push origin --delete v1.0.0   # 删除远程标签

# 检出标签
git checkout v1.0.0               # 分离头指针
git checkout -b version1 v1.0.0   # 从标签创建分支
```

---

## 高级操作

### 子模块

```bash
# 添加子模块
git submodule add https://github.com/user/repo.git path/to/submodule

# 克隆包含子模块的仓库
git clone --recurse-submodules https://github.com/user/repo.git

# 初始化子模块
git submodule update --init

# 更新子模块
git submodule update --remote

# 删除子模块
# 1. 删除 .gitmodules 中相关条目
# 2. 删除 .git/config 中相关条目
# 3. git rm --cached path/to/submodule
# 4. rm -rf path/to/submodule
```

### Cherry-pick

```bash
# 挑选特定提交到当前分支
git cherry-pick commit-hash

# 挑选多个提交
git cherry-pick hash1 hash2

# 挑选并继续
git cherry-pick --continue
git cherry-pick --abort
```

### 二分查找

```bash
# 开始二分查找
git bisect start

# 标记当前为 bad
git bisect bad

# 标记已知 good 的提交
git bisect good v1.0.0

# Git 会自动 checkout 中间提交，测试后标记
# 直到找到第一个 bad 提交

# 结束二分查找
git bisect reset
```

### 工作树 (Worktree)

```bash
# 创建新工作树
git worktree add ../repo-feature feature-branch

# 列出工作树
git worktree list

# 删除工作树
git worktree remove ../repo-feature
```

---

## 故障排查

### 常见问题

```bash
# 文件权限变更
git config core.filemode false      # 忽略权限变更

# 行尾符问题
git config core.autocrlf input      # Linux/Mac
git config core.autocrlf true       # Windows

# 大文件问题
git lfs track "*.psd"               # 使用 Git LFS

# 删除已跟踪的大文件
git filter-branch --force --index-filter \\
  'git rm --cached --ignore-unmatch 大文件' \\
  HEAD
```

### 恢复丢失的提交

```bash
# 使用 reflog 查找
git reflog

# 恢复分支
git checkout -b recovered-branch HEAD@{3}

# 恢复删除的文件
git checkout HEAD@{1} -- path/to/file
```

### 清理仓库

```bash
# 清理未跟踪文件（危险！）
git clean -n                        # 预览
git clean -f                        # 强制删除
git clean -fd                       # 包括目录
git clean -fdx                      # 包括忽略文件

# 垃圾回收
git gc                              # 基础清理
git gc --aggressive                 # 深度清理

# 查看仓库大小
git count-objects -vH
```

---

## 相关文档

- [domain-23-gitops-ci-cd/](../domain-23-gitops-ci-cd/) - GitOps 与 CI/CD
- [Git 官方文档](https://git-scm.com/doc)
