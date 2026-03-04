# Copa (Copacetic)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://project-copacetic.github.io/copacetic/ |
| **GitHub** | https://github.com/project-copacetic/copacetic |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Copa (Copacetic) 是一个容器镜像漏洞修补工具，能够直接在现有容器镜像中修补 OS 级别的漏洞，而无需从源代码重新构建整个镜像。它通过解析漏洞扫描报告（如 Trivy），自动为镜像中受影响的包应用安全补丁，极大缩短了从漏洞发现到修复的响应时间。

### 核心特性

- **直接修补**: 无需重建即可修补容器镜像中的 OS 漏洞
- **扫描报告驱动**: 支持 Trivy、Grype 等主流扫描器的报告格式
- **多发行版支持**: 支持 Debian、Ubuntu、Alpine、Red Hat 等基础镜像
- **Buildkit 集成**: 利用 BuildKit 高效创建修补后的镜像层
- **最小变更**: 只修改需要更新的包，保持镜像其余部分不变
- **CI/CD 友好**: 可嵌入 GitHub Actions、Azure DevOps 等流水线

---

## 架构设计

```
┌──────────────┐     ┌──────────────┐
│ 容器镜像      │     │ 漏洞扫描报告  │
│ (带漏洞)      │     │ (Trivy JSON)  │
└──────┬───────┘     └──────┬───────┘
       │                     │
       └──────────┬──────────┘
                  │
          ┌───────▼───────┐
          │    Copa CLI    │
          │                │
          │ ┌────────────┐ │
          │ │ 报告解析器  │ │
          │ └──────┬─────┘ │
          │ ┌──────▼─────┐ │
          │ │ 包管理器    │ │
          │ │ apt/apk/dnf│ │
          │ └──────┬─────┘ │
          │ ┌──────▼─────┐ │
          │ │ BuildKit   │ │
          │ │ 镜像构建    │ │
          │ └────────────┘ │
          └───────┬───────┘
                  │
          ┌───────▼───────┐
          │ 修补后的镜像   │
          │ (漏洞已修复)   │
          └───────────────┘
```

---

## 快速开始

### 安装

```bash
# 通过 Go 安装
go install github.com/project-copacetic/copacetic/cmd/copa@latest

# 或下载二进制文件
curl -LO "https://github.com/project-copacetic/copacetic/releases/latest/download/copa_$(uname -s)_$(uname -m).tar.gz"
tar xzf copa_*.tar.gz
sudo mv copa /usr/local/bin/

# 确保 BuildKit 可用
docker buildx create --name copa-buildkit --use
```

### 基本工作流

```bash
# 1. 扫描镜像获取漏洞报告
trivy image --format json --output report.json nginx:1.25

# 2. 使用 Copa 修补镜像
copa patch \
  -i docker.io/library/nginx:1.25 \
  -r report.json \
  -t 1.25-patched

# 3. 验证修补结果
trivy image nginx:1.25-patched
```

### 指定修补地址

```bash
# 使用自定义 BuildKit 地址
copa patch \
  -i myapp:latest \
  -r scan-report.json \
  -t latest-patched \
  --addr buildkit-host:1234

# 使用 BuildKit 容器
docker run -d --name buildkitd --privileged moby/buildkit:latest
copa patch \
  -i myapp:latest \
  -r scan-report.json \
  -t latest-patched \
  --addr docker-container://buildkitd
```

---

## CI/CD 集成

### GitHub Actions

```yaml
name: Patch Container Images
on:
  schedule:
    - cron: '0 6 * * *'  # 每天扫描并修补
  workflow_dispatch:

jobs:
  patch:
    runs-on: ubuntu-latest
    steps:
      - name: Set up Copa
        uses: project-copacetic/copa-action@v1

      - name: Scan image
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: 'myorg/myapp:latest'
          format: 'json'
          output: 'report.json'

      - name: Patch image
        run: |
          copa patch \
            -i myorg/myapp:latest \
            -r report.json \
            -t latest-patched

      - name: Push patched image
        run: |
          docker tag myorg/myapp:latest-patched myorg/myapp:latest
          docker push myorg/myapp:latest
```

### Azure DevOps Pipeline

```yaml
trigger: none
schedules:
  - cron: "0 6 * * *"
    displayName: Daily patch scan

steps:
  - task: trivy@1
    inputs:
      image: 'myorg/myapp:latest'
      format: 'json'
      output: '$(Build.ArtifactStagingDirectory)/report.json'

  - script: |
      copa patch \
        -i myorg/myapp:latest \
        -r $(Build.ArtifactStagingDirectory)/report.json \
        -t latest-patched
    displayName: 'Patch vulnerabilities'
```

---

## 与传统修复方式对比

| 特性 | Copa 直接修补 | 重新构建镜像 | 更换基础镜像 |
|:---|:---|:---|:---|
| 修复速度 | 秒级 | 分钟~小时 | 分钟~小时 |
| 需要源码 | 否 | 是 | 是 |
| 变更范围 | 仅受影响的包 | 整个镜像 | 基础层+应用层 |
| 测试影响 | 最小 | 需要完整测试 | 需要完整测试 |
| 适用场景 | 紧急漏洞修复 | 常规发布 | 定期维护 |
| 第三方镜像 | 支持 | 不可行 | 有限 |

---

## 最佳实践

1. **自动化流水线**: 将 Copa 集成到 CI/CD，实现漏洞的自动扫描和修补
2. **镜像签名**: 修补后对镜像重新签名，保持供应链安全
3. **分级修补**: 对 Critical/High 漏洞优先修补，Low/Medium 可在下次构建时处理
4. **保留原始镜像**: 保留原始镜像标签作为回滚备份
5. **定期重建**: Copa 修补适合紧急修复，定期仍应从源码完整重建镜像

---

## 参考资源

- [Copa 官方文档](https://project-copacetic.github.io/copacetic/)
- [Copa GitHub](https://github.com/project-copacetic/copacetic)
- [Copa GitHub Action](https://github.com/project-copacetic/copa-action)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
