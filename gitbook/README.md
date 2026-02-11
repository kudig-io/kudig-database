# Gitbook - 本地文档浏览系统

基于 [mdBook](https://rust-lang.github.io/mdBook/) 构建的本地知识库浏览系统，支持全文搜索、目录折叠导航。

## 前置条件

需要安装 Rust 工具链和 mdBook：

```bash
# 安装 Rust（如已安装可跳过）
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# 安装 mdBook
cargo install mdbook
```

## 快速启动

```bash
cd gitbook
bash refresh.sh
# 浏览器访问 http://localhost:3000
```

首次运行会自动完成：更新符号链接 → 生成目录索引 → 构建 HTML → 启动本地服务。

## 刷新 Gitbook

内容更新后，使用 `refresh.sh` 快速刷新：

| 命令 | 说明 | 适用场景 |
|:---|:---|:---|
| `bash refresh.sh` | 完整刷新（默认） | 新增/删除了文件或目录 |
| `bash refresh.sh build` | 仅重新构建 | 只修改了已有文件的内容 |
| `bash refresh.sh serve` | 等同默认模式 | - |
| `PORT=8080 bash refresh.sh` | 指定端口 | 端口 3000 被占用时 |

**完整刷新流程**：停止已有服务 → 更新符号链接 → 重新生成 SUMMARY.md → 构建 → 启动服务

**仅构建模式**：跳过符号链接和 SUMMARY.md 生成，直接构建。适用于只修改了已有 .md 文件内容的场景，速度更快。

## 静态导出

导出完整的静态 HTML 站点，可直接用浏览器打开，无需启动服务。

```bash
cd gitbook

# 导出到 gitbook/dist/ 目录
bash export-static.sh

# 导出并打包为 zip（文件名含时间戳）
bash export-static.sh --zip

# 直接打开查看
open dist/index.html
```

导出时会自动移除 `site-url` 配置，确保所有链接使用相对路径，兼容 `file://` 协议。构建完成后自动恢复原始配置。

## 目录结构

```
gitbook/
├── book.toml              # mdBook 主配置文件
├── generate-summary.sh    # SUMMARY.md 自动生成脚本
├── refresh.sh             # 快速刷新脚本
├── export-static.sh       # 静态导出脚本
├── theme/
│   ├── custom.css         # 自定义样式（目录行高、标题导航隐藏）
│   └── collapse-all.js    # 侧边栏默认折叠脚本
├── src/
│   ├── README.md          # → ../../README.md（符号链接）
│   ├── SUMMARY.md         # 目录索引（自动生成，勿手动编辑）
│   ├── domain-*           # → ../../domain-*（符号链接）
│   └── topic-*            # → ../../topic-*（符号链接）
├── book/                  # 构建输出（serve 模式，已 gitignore）
└── dist/                  # 静态导出输出（已 gitignore）
```

## 注意事项

### 符号链接

- `src/` 下的 domain-\* 和 topic-\* 目录是**符号链接**，指向项目根目录下的实际内容
- 修改 `src/` 下的文件等同于修改原始文件
- 新增 domain 或 topic 目录后需运行完整刷新（`bash refresh.sh`）以创建新的符号链接

### SUMMARY.md

- 由 `generate-summary.sh` 自动生成，**不要手动编辑**
- 自动从各目录的 README.md 提取标题
- 如文件没有 Markdown 标题行，将使用文件名作为标题
- 新增或删除 .md 文件后需运行完整刷新以更新目录

### 构建产物

- `book/` 和 `dist/` 目录已在 `.gitignore` 中排除，不会提交到仓库
- 导出的 zip 文件同样不会提交

### 搜索

- 搜索索引较大（约 85MB），首次加载可能需要几秒
- 支持布尔 AND 搜索，输入多个关键词会取交集
- 快捷键：按 `/` 或 `s` 打开搜索框

### 常见问题

**Q: 构建时提示 "unclosed HTML tag" 警告？**
A: Markdown 中的 `<tag>` 会被 mdBook 解析为 HTML 标签。用反引号包裹即可：`` `<tag>` ``。

**Q: 端口 3000 被占用？**
A: 使用 `PORT=8080 bash refresh.sh` 指定其他端口。

**Q: 侧边栏目录没有折叠？**
A: 尝试强制刷新浏览器（Cmd+Shift+R），确保加载最新的 CSS 和 JS 文件。

**Q: 静态导出后页面跳转 404？**
A: 确认使用 `export-static.sh` 导出（而非直接复制 `book/` 目录），该脚本会移除 site-url 以确保相对路径正确。
