# 快速安装 mdbook（Windows）

## 方案 1：使用 Cargo 安装（需要先安装 Rust）

```powershell
# 1. 安装 Rust（如果尚未安装）
# 访问 https://rustup.rs/ 或执行：
Invoke-WebRequest -Uri https://win.rustup.rs/x86_64 -OutFile rustup-init.exe
.\rustup-init.exe

# 2. 重启 PowerShell

# 3. 安装 mdbook
cargo install mdbook

# 4. 验证
mdbook --version
```

## 方案 2：直接下载预编译版本（推荐，更快）

```powershell
# 在 PowerShell 中执行以下命令

# 1. 创建临时目录
New-Item -ItemType Directory -Force -Path "$env:TEMP\mdbook-download"
cd "$env:TEMP\mdbook-download"

# 2. 下载最新版本（示例：v0.4.40，请访问 GitHub Releases 查看最新版本）
# https://github.com/rust-lang/mdBook/releases
$version = "v0.4.40"
$url = "https://github.com/rust-lang/mdBook/releases/download/$version/mdbook-$version-x86_64-pc-windows-msvc.zip"
Invoke-WebRequest -Uri $url -OutFile "mdbook.zip"

# 3. 解压
Expand-Archive -Path "mdbook.zip" -DestinationPath "." -Force

# 4. 方式 A：复制到系统 PATH（需要管理员权限）
# Copy-Item "mdbook.exe" -Destination "C:\Windows\System32\" -Force

# 4. 方式 B：复制到用户目录（推荐，无需管理员权限）
$userBin = "$env:USERPROFILE\.local\bin"
New-Item -ItemType Directory -Force -Path $userBin
Copy-Item "mdbook.exe" -Destination $userBin -Force

# 5. 添加到 PATH（仅当前会话）
$env:Path += ";$userBin"

# 6. 永久添加到 PATH（用户环境变量）
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";$userBin", "User")

# 7. 验证安装
mdbook --version

# 8. 清理临时文件
cd ..
Remove-Item -Recurse -Force "$env:TEMP\mdbook-download"
```

## 方案 3：手动下载

1. 访问 https://github.com/rust-lang/mdBook/releases
2. 下载最新的 `mdbook-vX.X.X-x86_64-pc-windows-msvc.zip`
3. 解压得到 `mdbook.exe`
4. 将 `mdbook.exe` 移动到以下位置之一：
   - `C:\Windows\System32\` (需要管理员权限)
   - `%USERPROFILE%\.local\bin\` (推荐)
   - 或项目的 `gitbook` 目录下（使用时需指定完整路径）
5. 如果放在自定义位置，需要将该路径添加到系统 PATH 环境变量

## 验证安装

```powershell
# 重启 PowerShell 后执行
mdbook --version

# 应该显示类似：
# mdbook v0.4.40
```

## 安装完成后

继续执行离线版本生成：

```powershell
cd C:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook
.\export-static.ps1
```

或打包为 ZIP：

```powershell
.\export-static.ps1 -Zip
```
