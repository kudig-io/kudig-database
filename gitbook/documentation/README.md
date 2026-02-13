# GitBook 使用文档索引

本目录包含 GitBook 的详细使用文档和操作指南。

## 📄 文档列表

### 使用指南

1. **[全量导出指南.md](./全量导出指南.md)**
   - GitBook 全量导出的完整操作指南
   - 一键导出静态离线版本
   - UTF-8编码确保中文无乱码

2. **[Windows-离线版本生成指南.md](./Windows-离线版本生成指南.md)**
   - Windows 环境下生成 GitBook 离线版本的完整指南
   - 包含安装、配置、构建的详细步骤
   - 常见问题解答

3. **[快速安装mdbook.md](./快速安装mdbook.md)**
   - Windows 下快速安装 mdbook 的多种方法
   - 使用 Cargo 或直接下载预编译版本
   - 安装验证和故障排除

4. **[生成离线版本指南.md](./生成离线版本指南.md)**
   - 离线静态版本生成的详细流程
   - 手动步骤和自动脚本两种方式
   - 符号链接处理和配置修改说明

## 🎯 快速查找

### 我想...

- **快速导出** → 直接双击 `QUICK-BUILD.cmd`（gitbook根目录）
- **安装 mdbook** → 查看 `快速安装mdbook.md`
- **生成离线版本** → 查看 `全量导出指南.md` 或 `Windows-离线版本生成指南.md`
- **了解详细步骤** → 查看 `生成离线版本指南.md`

## 💻 推荐工作流

### 首次使用

1. 安装 mdbook：
   ```powershell
   cd gitbook/build-scripts
   .\install-mdbook.ps1
   ```

2. 构建离线版本：
   ```cmd
   cd gitbook
   QUICK-BUILD.cmd
   ```

3. 打开查看：
   导出完成后会自动在浏览器中打开

### 日常更新

内容更新后重新构建：

```cmd
cd gitbook
QUICK-BUILD.cmd
```

## 📋 文档概览

| 文档 | 适用场景 | 难度 |
|:---|:---|:---:|
| 全量导出指南 | 快速一键导出（推荐） | ⭐ |
| Windows-离线版本生成指南 | Windows 用户首次使用 | ⭐⭐ |
| 快速安装mdbook | 需要安装 mdbook | ⭐ |
| 生成离线版本指南 | 需要了解详细原理和步骤 | ⭐⭐⭐ |

## 🔗 相关链接

- [mdBook 官方文档](https://rust-lang.github.io/mdBook/)
- [mdBook GitHub](https://github.com/rust-lang/mdBook)
- [Rust 官网](https://www.rust-lang.org/)

## ❓ 常见问题快速解答

**Q: 我应该看哪个文档？**

- 快速上手 → 直接双击 `QUICK-BUILD.cmd`，有问题再看文档
- 详细了解 → 从 `全量导出指南.md` 或 `Windows-离线版本生成指南.md` 开始
- 安装问题 → 查看 `快速安装mdbook.md`

**Q: 文档太长，有快速方案吗？**

是的！只需执行：
```cmd
# 一键安装 + 构建
cd gitbook/build-scripts
.\install-mdbook.ps1

cd ..
QUICK-BUILD.cmd
```

**Q: 构建失败怎么办？**

1. 检查 mdbook 是否正确安装
2. 查看 `生成离线版本指南.md` 的故障排除部分
3. 尝试使用管理员权限运行 PowerShell

**Q: 中文显示乱码怎么办？**

使用 `QUICK-BUILD.cmd`，已内置UTF-8编码修复，确保中文正确显示。

## 📝 文档维护

这些文档记录了：
- mdbook 的安装和配置过程
- Windows 环境下的特殊处理（符号链接等）
- 完整的构建流程和脚本说明
- 常见问题和解决方案

如有疑问或发现问题，请参考相应文档或查看 `../README.md`。
