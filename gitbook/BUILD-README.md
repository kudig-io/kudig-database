# GitBook 三级目录构建脚本使用说明

## 概述

本脚本集成了以下功能：
1. **自动生成三级目录结构的 SUMMARY.md**
2. **修复 UTF-8 编码问题**（解决中文乱码）
3. **构建静态网站文件**
4. **自动导出并压缩**
5. **保持侧边栏展开状态**（已优化 localStorage 持久化）

## 文件说明

### 主要脚本

- **`BUILD-SIMPLIFIED.cmd`** - **推荐使用**：简化版一键构建脚本
  - 自动生成三级目录SUMMARY.md
  - 构建静态文件
  - 导出到`export/`目录
  - 自动打包ZIP
  - 自动在浏览器中打开

### 辅助脚本

- **`build-scripts/generate-summary-three-level.ps1`** - PowerShell脚本，生成三级目录SUMMARY.md
  - 支持 Domain 1-33 的完整结构
  - 支持子目录嵌套（如 `domain-17-cloud-provider/01-aws-eks/`）
  - 使用 Unicode 字符避免编码问题
  - 自动统计文件链接数量

## 快速开始

### 方法一：双击运行（推荐）

1. 双击 `BUILD-SIMPLIFIED.cmd`
2. 等待构建完成
3. 浏览器会自动打开静态网站

### 方法二：命令行运行

```cmd
cd c:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook
BUILD-SIMPLIFIED.cmd
```

## 目录结构支持

脚本支持以下三级目录结构：

```
Domain-17: 云厂商Kubernetes服务
├── README.md
├── 01-aws-eks/
│   └── aws-eks-overview.md
├── 02-google-cloud-gke/
│   └── google-cloud-gke-overview.md
├── 04-alicloud-ack/
│   ├── 240-ack-ecs-compute.md
│   ├── 241-ack-slb-nlb-alb.md
│   ├── alicloud-ack-overview.md
│   └── service-ack-practical-guide.md
└── 13-alicloud-apsara-ack/
    ├── 250-apsara-stack-ess-scaling.md
    ├── 251-apsara-stack-sls-logging.md
    └── alicloud-apsara-ack-overview.md
```

## 输出说明

### 构建输出

构建完成后，文件会被导出到：
```
gitbook/export/kudig-gitbook-20260213-1130/
```

时间戳格式：`YYYYMMDD-HHMM`

### ZIP 压缩包

同时生成ZIP文件：
```
gitbook/export/kudig-gitbook-20260213-1130.zip
```

## UTF-8 编码修复

脚本使用以下方法确保 UTF-8 编码正确：

1. **Unicode 字符编码**：
   ```powershell
   [char]0x9996 + [char]0x9875  # 首页
   [char]0x6838 + [char]0x5FC3 + [char]0x77E5 + [char]0x8BC6 + [char]0x57DF  # 核心知识域
   [char]0x6269 + [char]0x5C55 + [char]0x9886 + [char]0x57DF  # 扩展领域
   [char]0x4E13 + [char]0x9898 + [char]0x5185 + [char]0x5BB9  # 专题内容
   ```

2. **UTF-8 无BOM写入**：
   ```powershell
   $Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
   [System.IO.File]::WriteAllText($OutputFile, $Content, $Utf8NoBom)
   ```

## 验证

构建完成后，打开生成的网站，检查以下内容：

### 侧边栏结构

- [ ] 首页
- [ ] 核心知识域 (Domain 1-12)
  - [ ] Domain-1: Kubernetes架构基础
  - [ ] Domain-2: Kubernetes设计原则
  - [ ] ...
- [ ] 扩展领域 (Domain 13-33)
  - [ ] Domain-17: 云厂商Kubernetes服务
    - [ ] AWS EKS
    - [ ] Google Cloud GKE
    - [ ] 阿里云 ACK
      - [ ] ACK 关联产品 - ECS 计算资源
      - [ ] ACK 关联产品 - 负载均衡
      - [ ] ...
- [ ] 专题内容 (Topics)

### 中文显示

确保以下中文正确显示（无乱码）：
- ✓ 首页
- ✓ 核心知识域
- ✓ 扩展领域
- ✓ 专题内容

### 侧边栏功能

- [ ] 点击文档后侧边栏保持展开状态
- [ ] 刷新页面后侧边栏状态保持
- [ ] 24小时后自动清除状态缓存

## 故障排查

### 问题1: 中文乱码

**症状**: 显示 "涓撻鍐呭" 而不是 "专题内容"

**解决方案**:
```cmd
cd c:\Users\Allen\Documents\GitHub\kudig-io\kudig-database\gitbook
powershell -NoProfile -ExecutionPolicy Bypass -File "build-scripts\generate-summary-three-level.ps1"
```

### 问题2: 侧边栏折叠

**症状**: 每次点击文档后侧边栏都折叠

**解决方案**: 已修复，使用 localStorage 持久化，位于 `theme/collapse-all.js`

### 问题3: 三级目录未显示

**症状**: 子目录下的文件未显示

**原因**: 需要确保子目录中的文件都是 `.md` 格式

**检查**:
```cmd
dir domain-17-cloud-provider /s /b | findstr \.md$
```

## 技术细节

### 三级目录实现

递归函数 `Process-DirectoryRecursive`:
```powershell
function Process-DirectoryRecursive {
    param(
        [string]$Dir, 
        [string]$Indent = "",
        [int]$MaxDepth = 3,
        [int]$CurrentDepth = 0
    )
    
    if ($CurrentDepth -ge $MaxDepth) {
        return $Items
    }
    
    # 处理 README.md
    # 处理同级 .md 文件
    # 递归处理子目录
}
```

### 文件统计

当前项目统计（2026-02-13）：
- **总文件链接**: 668 个
- **Domain 1-12**: 约 300+ 文件
- **Domain 13-33**: 约 300+ 文件
- **专题内容**: 约 50+ 文件

## 更新历史

### v3.0 (2026-02-13)
- ✅ 实现三级目录支持
- ✅ 修复 UTF-8 编码问题
- ✅ 简化构建脚本
- ✅ 添加 localStorage 侧边栏持久化

### v2.0 (2026-02-12)
- ✅ 修复 PowerShell 双重编码问题
- ✅ 优化侧边栏折叠逻辑

### v1.0 (初始版本)
- ✅ 基本构建功能
- ✅ 二级目录支持

## 联系方式

如有问题，请提交 Issue 到项目仓库。
