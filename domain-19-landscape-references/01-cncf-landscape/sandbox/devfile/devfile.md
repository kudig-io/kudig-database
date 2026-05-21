---
title: Devfile
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- vpa
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Devfile 是什么
- 如何 Devfile
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Devfile
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: Devfile
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- vpa
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Devfile 是什么
- 如何 Devfile
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Devfile
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Devfile

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://devfile.io/ |
| **GitHub** | https://github.com/devfile/api |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Devfile 是一个开放标准，用于定义云原生开发环境。它通过 YAML 格式的 devfile.yaml 描述开发工具容器、端口转发、命令和生命周期事件，使开发环境可复现、可共享，并被多种 IDE 和开发工具支持（如 Eclipse Che、odo、OpenShift Dev Spaces）。

### 核心特性

- **标准化规范**: 开放的开发环境定义标准 (v2.x)
- **容器化开发**: 定义开发所需的容器组件和工具链
- **多 IDE 支持**: Eclipse Che, VS Code, IntelliJ 等
- **命令定义**: 预定义 build, run, test, debug 等开发命令
- **Devfile Registry**: 可复用的 Devfile 模板仓库
- **Parent 继承**: Devfile 继承和组合机制

---

## 快速开始

### devfile.yaml 示例

```yaml
schemaVersion: 2.2.0
metadata:
  name: nodejs-app
  version: 1.0.0
  language: JavaScript
  projectType: Node.js

components:
  - name: tools
    container:
      image: registry.access.redhat.com/ubi8/nodejs-18:latest
      memoryLimit: 1Gi
      cpuLimit: "1"
      mountSources: true
      endpoints:
        - name: http
          targetPort: 3000
          exposure: public
      env:
        - name: NODE_ENV
          value: development

  - name: postgres
    container:
      image: postgres:16
      memoryLimit: 512Mi
      env:
        - name: POSTGRES_PASSWORD
          value: devpassword
        - name: POSTGRES_DB
          value: myapp
      endpoints:
        - name: db
          targetPort: 5432
          exposure: internal

commands:
  - id: install
    exec:
      component: tools
      commandLine: npm install
      workingDir: ${PROJECT_SOURCE}
      group:
        kind: build
  - id: run
    exec:
      component: tools
      commandLine: npm start
      workingDir: ${PROJECT_SOURCE}
      group:
        kind: run
        isDefault: true
  - id: test
    exec:
      component: tools
      commandLine: npm test
      workingDir: ${PROJECT_SOURCE}
      group:
        kind: test
  - id: debug
    exec:
      component: tools
      commandLine: npm run debug
      workingDir: ${PROJECT_SOURCE}
      group:
        kind: debug

events:
  postStart:
    - install
```

### 使用 odo

```bash
# 安装 odo CLI
curl -L https://developers.redhat.com/content-gateway/rest/mirror/pub/openshift-v4/clients/odo/latest/odo-linux-amd64 -o odo
chmod +x odo && sudo mv odo /usr/local/bin/

# 从 Devfile Registry 创建项目
odo init --devfile nodejs

# 启动开发环境
odo dev
```

### Parent 继承

```yaml
schemaVersion: 2.2.0
metadata:
  name: my-java-app
parent:
  id: java-springboot
  registryUrl: https://registry.devfile.io
  version: 2.0.0
components:
  - name: tools
    container:
      env:
        - name: JAVA_OPTS
          value: "-Xmx512m"
```

---

## 最佳实践

1. **仓库内置**: 将 devfile.yaml 放在项目根目录，确保所有开发者环境一致
2. **Registry 复用**: 使用 Devfile Registry 提供的模板作为 parent
3. **命令分组**: 将命令按 build/run/test/debug 分组，便于 IDE 集成
4. **资源限制**: 为容器设置合理的 CPU 和内存限制
5. **环境变量**: 使用 env 配置开发环境变量，避免硬编码

---

## 参考资源

- [Devfile 官方文档](https://devfile.io/docs/)
- [Devfile API 规范](https://github.com/devfile/api)
- [Devfile Registry](https://registry.devfile.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/devfile.md|Devfile]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
