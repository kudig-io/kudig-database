# Konveyor

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.konveyor.io/ |
| **GitHub** | https://github.com/konveyor/tackle2-hub |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, TypeScript |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Konveyor 是一个应用现代化平台，帮助组织将传统应用（如 Java EE、Spring）迁移和重构到 Kubernetes 平台。它提供应用清单管理、依赖分析、迁移评估、自动化代码重构等能力。Konveyor 通过 AI 辅助分析识别迁移障碍，生成迁移路径建议，并提供 IDE 插件帮助开发者自动化完成代码变更。

### 核心特性

- **应用清单**: 集中管理待迁移应用的评估和跟踪
- **分析引擎**: 扫描应用代码，识别与 K8s 不兼容的模式
- **迁移规则**: 内置数百条 Java EE → Quarkus/Spring Boot 迁移规则
- **AI 辅助**: 利用 LLM 生成代码修改建议和迁移方案
- **IDE 集成**: VS Code 和 IntelliJ 插件提供实时迁移建议
- **问卷评估**: 通过问卷评估应用的迁移复杂度和优先级

---

## 快速开始

### 安装

```bash
# 在 Kubernetes 上安装 Konveyor
kubectl apply -f https://raw.githubusercontent.com/konveyor/operator/main/konveyor-operator.yaml

# 或使用 Operator
kubectl create namespace konveyor
kubectl apply -f - <<EOF
apiVersion: tackle.konveyor.io/v1alpha1
kind: Tackle
metadata:
  name: tackle
  namespace: konveyor
spec:
  feature_auth_required: false
EOF
```

### 应用分析

```bash
# 使用 kantra CLI 分析应用
kantra analyze \
  --input /path/to/my-java-app \
  --output /path/to/report \
  --target cloud-readiness \
  --target quarkus

# 查看分析报告
open /path/to/report/index.html
```

### 自定义迁移规则

```yaml
# 自定义规则示例
- ruleID: custom-session-rule
  when:
    java.referenced:
      pattern: javax.servlet.http.HttpSession
  message: "HTTP Session 不适合 Kubernetes 无状态部署，建议使用 Redis 等外部会话存储"
  effort: 3
  category: mandatory
  labels:
    - konveyor.io/target=cloud-readiness
```

---

## 与其他方案对比

| 特性 | Konveyor | IBM Transformation Advisor | vFunction | 手动评估 |
|:---|:---|:---|:---|:---|
| 开源 | 是 | 否 | 否 | N/A |
| AI 辅助 | 支持 | 有限 | 支持 | 无 |
| 规则引擎 | 可扩展 | 内置 | 内置 | 无 |
| IDE 集成 | VS Code/IntelliJ | 无 | 无 | N/A |
| 适用语言 | Java 为主 | Java | Java/.NET | 任意 |

---

## 最佳实践

1. **评估优先**: 先用问卷评估确定迁移优先级，再做深度分析
2. **分批迁移**: 从低复杂度应用开始迁移，积累经验后处理复杂应用
3. **自定义规则**: 根据组织技术栈添加自定义分析规则
4. **AI 审查**: AI 生成的代码修改建议需要人工审查后再应用
5. **持续跟踪**: 利用应用清单功能跟踪整个迁移组合的进度

---

## 参考资源

- [Konveyor 官方文档](https://www.konveyor.io/docs/)
- [Konveyor GitHub](https://github.com/konveyor)
- [kantra CLI](https://github.com/konveyor/kantra)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
