# Gitbook - 本地文档浏览系统

基于 [mdBook](https://rust-lang.github.io/mdBook/) 构建的本地知识库浏览系统，支持全文搜索、目录折叠导航。

## 🚀 快速导出离线版本（推荐）

**一键生成完整的离线静态网站：**

**方式 1：双击运行（最简单）**
```
双击文件：QUICK-BUILD.cmd
```

**方式 2：命令行**
```cmd
cd gitbook
QUICK-BUILD.cmd
```

- ⏱️ 构建时间：1-3 分钟
- 📦 输出位置：`export/kudig-gitbook-YYYYMMDD-HHMMSS/`
- 🎯 自动生成ZIP压缩包
- ✅ UTF-8编码，无乱码
- 📖 详细说明：[全量导出指南](documentation/全量导出指南.md)

## 📁 目录结构

```
gitbook/
├── README.md              # 本文件
├── book.toml              # mdBook 主配置文件
│
├── src/                   # 源文件目录（符号链接到项目内容）
│   ├── README.md          # → ../../README.md
│   ├── SUMMARY.md         # 目录索引（自动生成）
│   ├── domain-*           # → ../../domain-* (符号链接)
│   └── topic-*            # → ../../topic-* (符号链接)
│
├── theme/                 # 主题自定义
│   ├── custom.css         # 自定义样式
│   └── collapse-all.js    # 侧边栏折叠脚本
│
├── build-scripts/         # 构建和导出脚本
│   ├── Linux/macOS 脚本:
│   │   ├── start.sh              # 启动本地服务
│   │   ├── refresh.sh            # 刷新构建
│   │   ├── export-static.sh      # 导出静态版本
│   │   └── generate-summary.sh   # 生成 SUMMARY.md
│   │
│   └── Windows 脚本:
│       ├── install-mdbook.ps1           # 安装 mdbook
│       ├── FINAL-BUILD.ps1              # 主构建脚本
│       └── generate-summary-utf8.ps1    # 生成 SUMMARY.md (UTF-8)
│
├── documentation/         # 使用文档
│   ├── Windows-离线版本生成指南.md
│   ├── 快速安装mdbook.md
│   └── 生成离线版本指南.md
│
├── book/                  # 构建输出（gitignore）
└── dist/                  # 静态导出输出（gitignore）
```

## 🚀 快速开始

### Linux/macOS

```bash
cd gitbook

# 首次使用：启动本地服务
bash build-scripts/start.sh

# 浏览器访问 http://localhost:3000
```

### Windows

```powershell
cd gitbook

# 方式 1：使用CMD脚本（推荐，最简单）
QUICK-BUILD.cmd

# 方式 2：使用PowerShell
powershell -ExecutionPolicy Bypass -File build-scripts\FINAL-BUILD.ps1 -Zip
```

## 📦 前置条件

需要安装 mdBook：

### 方式 1：自动安装（Windows）

```powershell
cd gitbook
.\build-scripts\install-mdbook.ps1
```

### 方式 2：使用 Cargo 安装

```bash
# 安装 Rust（如已安装可跳过）
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# 安装 mdBook
cargo install mdbook
```

## 📋 常用操作

### 本地开发服务（Linux/macOS）

```bash
# 启动服务
bash build-scripts/start.sh

# 自定义端口
PORT=8080 bash build-scripts/start.sh
```

### 内容更新后刷新

```bash
# 完整刷新（新增/删除了文件）
bash build-scripts/refresh.sh

# 仅重新构建（只修改了内容）
bash build-scripts/refresh.sh build
```

### 导出静态离线版本

**Linux/macOS:**
```bash
# 导出到 dist/ 目录
bash build-scripts/export-static.sh

# 导出并打包 ZIP
bash build-scripts/export-static.sh --zip
```

**Windows:**
```powershell
# 推荐使用
QUICK-BUILD.cmd

# 或使用PowerShell
.\build-scripts\FINAL-BUILD.ps1 -Zip
```

## 📚 使用文档

详细文档请查看 `documentation/` 目录：

- `Windows-离线版本生成指南.md` - Windows 环境完整指南
- `快速安装mdbook.md` - mdbook 快速安装说明
- `生成离线版本指南.md` - 离线版本生成详细步骤

## 🔧 脚本说明

### Linux/macOS 脚本

| 脚本 | 功能 | 用法 |
|:---|:---|:---|
| `start.sh` | 初始化并启动本地服务 | `bash build-scripts/start.sh` |
| `refresh.sh` | 内容变更后刷新构建 | `bash build-scripts/refresh.sh` |
| `export-static.sh` | 导出离线静态站点 | `bash build-scripts/export-static.sh` |
| `generate-summary.sh` | 生成 SUMMARY.md | 自动调用，无需单独执行 |

### Windows PowerShell 脚本

| 脚本 | 功能 | 推荐度 |
|:---|:---|:---:|
| `FINAL-BUILD.ps1` | 一键构建离线版本 | ⭐⭐⭐ |
| `install-mdbook.ps1` | 安装 mdbook 工具 | ⭐⭐⭐ |
| `generate-summary-utf8.ps1` | 生成目录索引（UTF-8） | ⭐⭐⭐ |

**推荐使用 `QUICK-BUILD.cmd`（根目录）**，它整合了所有功能并确保UTF-8编码正确。

## ⚙️ 配置说明

### book.toml

主配置文件，包含：
- 书籍元数据（标题、作者等）
- 构建选项
- HTML 输出配置
- 搜索功能配置
- 目录折叠功能

### 符号链接

- `src/` 下的 `domain-*` 和 `topic-*` 是符号链接
- 指向项目根目录的实际内容
- 修改 `src/` 下的文件等同于修改原始文件
- 新增目录后需运行刷新脚本

### SUMMARY.md

- 由 `generate-summary.sh/ps1` 自动生成
- **不要手动编辑**
- 自动从各目录 README.md 提取标题
- 新增/删除文件后需重新生成

## 💡 注意事项

1. **构建产物**
   - `book/` 和 `dist/` 已在 `.gitignore` 中
   - 不会提交到 Git 仓库

2. **搜索功能**
   - 搜索索引约 85MB
   - 首次加载需几秒
   - 支持布尔 AND 搜索
   - 快捷键：`/` 或 `s`

3. **Windows 环境**
   - 符号链接需要特殊处理
   - 推荐使用 PowerShell 脚本
   - 必要时以管理员身份运行

## ❓ 常见问题

**Q: 端口 3000 被占用？**

```bash
# 停掉残留进程
pkill -f "mdbook serve"

# 或使用其他端口
PORT=4000 bash build-scripts/start.sh
```

**Q: 构建时提示 "unclosed HTML tag"？**

Markdown 中的 `<tag>` 用反引号包裹：`` `<tag>` ``

**Q: 静态导出后页面 404？**

确保使用 `QUICK-BUILD.cmd`、`export-static.sh` 或 `FINAL-BUILD.ps1` 导出，不要直接复制 `book/` 目录。

**Q: Windows 下符号链接创建失败？**

以管理员身份运行 PowerShell 或查看 `documentation/` 下的详细指南。

## 📞 获取帮助

1. 查看 `documentation/` 目录下的详细文档
2. 运行脚本时查看输出信息
3. 检查 mdbook 版本：`mdbook --version`

---

**提示**: Windows 用户最简单的方式是直接双击 `QUICK-BUILD.cmd`，这是经过优化和测试的最可靠方案，确保UTF-8编码无乱码。
