# Windows 环境下生成 GitBook 离线静态版本指南

## 第一步：安装必要工具

### 方法 1：使用 Rust 工具链安装 mdbook（推荐）

1. **安装 Rust**（如果尚未安装）
   
   访问 [https://www.rust-lang.org/tools/install](https://www.rust-lang.org/tools/install) 下载 `rustup-init.exe`，运行安装。

   或在 PowerShell 中执行：
   ```powershell
   Invoke-WebRequest -Uri https://win.rustup.rs/x86_64 -OutFile rustup-init.exe
   .\rustup-init.exe
   ```

2. **重启终端**，让 PATH 环境变量生效

3. **安装 mdbook**
   ```powershell
   cargo install mdbook
   ```

4. **验证安装**
   ```powershell
   mdbook --version
   ```

### 方法 2：下载预编译的 mdbook 二进制文件

1. 访问 [mdBook Releases](https://github.com/rust-lang/mdBook/releases)
2. 下载最新的 `mdbook-vX.X.X-x86_64-pc-windows-msvc.zip`
3. 解压后将 `mdbook.exe` 放到系统 PATH 路径下（如 `C:\Windows\System32\`）
4. 或者直接解压到 `gitbook` 文件夹，使用时指定完整路径

## 第二步：生成离线静态版本

安装好 mdbook 后，在 PowerShell 中执行：

```powershell
cd C:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook
.\export-static.ps1
```

### 可选：生成并打包 ZIP 文件

```powershell
.\export-static.ps1 -Zip
```

## 第三步：使用离线版本

生成完成后，您会得到一个 `dist` 文件夹，里面包含完整的静态网站。

### 方式 1：直接打开
```powershell
start dist\index.html
```

### 方式 2：拷贝到其他位置
直接复制整个 `dist` 文件夹到任何地方（包括 U 盘、移动硬盘），双击 `index.html` 即可打开。

### 方式 3：使用生成的 ZIP 包
如果使用了 `-Zip` 参数，会在 gitbook 目录下生成一个带时间戳的 zip 文件，可以直接分享给他人。

## 脚本说明

### export-static.ps1 功能

1. ✅ 自动创建/更新符号链接到各个 domain 和 topic 目录
2. ✅ 自动生成 SUMMARY.md 目录索引
3. ✅ 修改配置使其支持离线访问（去除 site-url）
4. ✅ 构建完全静态的 HTML 文件
5. ✅ 自动恢复原始配置文件
6. ✅ 可选择打包为 ZIP 文件

### generate-summary.ps1 功能

自动扫描所有 domain 和 topic 目录，生成完整的导航目录结构。

## 常见问题

### Q1: PowerShell 提示"无法加载，因为在此系统上禁止运行脚本"

**解决方案：**
```powershell
# 临时允许执行（仅当前会话）
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process

# 然后再运行脚本
.\export-static.ps1
```

### Q2: 符号链接创建失败

**解决方案：**
以管理员身份运行 PowerShell，或使用 Junction（脚本已自动使用）。

### Q3: 中文显示乱码

**解决方案：**
确保 PowerShell 使用 UTF-8 编码：
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
```

### Q4: 生成的页面中链接无法跳转

确保使用 `export-static.ps1` 脚本生成，该脚本会自动处理路径问题。不要直接复制 `book` 目录。

## 输出结果

生成成功后，您会看到类似以下的输出：

```
[INFO] 更新符号链接...
[INFO] 生成 SUMMARY.md...
[INFO] 准备静态导出配置...
[INFO] 构建静态版本...
[INFO] 已恢复原始 book.toml
[INFO] 静态导出完成:
[INFO]   页面数量: 500+
[INFO]   总大小: XX MB
[INFO]   输出目录: C:\...\gitbook\dist
[INFO] 总耗时: XXs

使用方法:
  直接打开: start C:\...\gitbook\dist\index.html

离线版本已生成在 dist 目录，可以直接拷贝整个 dist 文件夹到任何地方使用！
```

## 目录结构

```
gitbook/
├── dist/                          # 生成的离线静态版本（可拷贝）
│   ├── index.html                 # 入口文件
│   ├── *.html                     # 所有页面
│   ├── css/                       # 样式文件
│   ├── FontAwesome/               # 图标字体
│   ├── fonts/                     # 字体文件
│   ├── searchindex.js             # 搜索索引
│   ├── searchindex.json           # 搜索数据
│   └── ...
├── export-static.ps1              # 静态导出脚本（PowerShell）
├── generate-summary.ps1           # 目录生成脚本（PowerShell）
└── kudig-database-gitbook-*.zip   # 打包的 ZIP 文件（使用 -Zip 参数时生成）
```

## 技术细节

- 使用 mdBook 构建静态网站
- 自动移除 `site-url` 配置，使用相对路径
- 所有资源（CSS、JS、字体、图片）都内嵌或使用相对路径
- 支持全文搜索（搜索索引已预生成）
- 支持导航目录折叠
- 完全离线可用，无需网络连接
