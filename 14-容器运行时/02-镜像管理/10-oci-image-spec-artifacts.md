---
title: OCI Image Spec 1.1 与 Artifact 生态
description: OCI Image Spec 1.1 深度解读 — artifact 指针与 referrers API 取代 artifact.manifest 实践、Helm/WASM/SBOM/签名/OPA 策略等非镜像 artifact、oras CLI 推拉实操、registry 兼容性矩阵与版本核对
summary: OCI Image Spec 1.1（v1.1.1）与 OCI Artifact 生态全景 — referrers API 工作原理、五类常见 artifact 类型与媒体类型速查、oras CLI（v1.3.4）完整推拉/发现实操、Harbor/Docker Hub/云厂商 registry 兼容矩阵与升级注意事项
category: container-image-management
tags:
- k8s
- specification
- security
- cncf
- containerd
- oci
- oras
- sbom
- helm
- supply-chain
tier: supporting
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- DevOps 工程师
estimated_read_time: 15min
intent_queries:
- OCI Image Spec 1.1 的 referrers API 如何工作，artifact.manifest 为何被取代
- 如何用 oras CLI 推送拉取 Helm chart、SBOM、签名等非镜像 artifact
trigger_keywords:
- OCI
- image spec
- artifact
- referrers
- oras
- SBOM
- SPDX
- CycloneDX
- cosign
- supply chain
- 供应链安全
- media type
prerequisites:
- registry-basics
- containerd-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# OCI Image Spec 1.1 与 Artifact 生态

> **版本基线**: OCI Image Spec v1.1.1（撰写时最新 release，2026-10-14 经 GitHub Releases API 核对）；oras CLI v1.3.4（同日核对）
> **核心命题**: 让 registry 不只存镜像 —— 把 SBOM、签名、Helm chart、WASM、策略文件作为"一等公民"挂在镜像旁边

---

## 📋 目录

- [一、概述与核心概念](#一概述与核心概念)
- [二、原理与架构](#二原理与架构)
- [三、生产配置实操：oras CLI](#三生产配置实操oras-cli)
- [四、registry 兼容性矩阵](#四registry-兼容性矩阵)
- [五、故障排查与诊断流程](#五故障排查与诊断流程)
- [六、最佳实践与反模式](#六最佳实践与反模式)
- [七、性能与监控](#七性能与监控)
- [八、版本兼容与升级注意](#八版本兼容与升级注意)
- [九、参考链接与延伸阅读](#九参考链接与延伸阅读)

---

## 一、概述与核心概念

### 1.1 OCI 规范族与 1.1 的位置

OCI（Open Container Initiative）规范族包含 runtime-spec、distribution-spec 与 image-spec 三大件。image-spec 定义了镜像如何被描述为一份**内容寻址的 JSON 清单（manifest）+ blob 集合**：

- **Image Manifest**：指向各层（layer）与配置（config）blob 的摘要清单；
- **Image Index**：多架构（multi-arch）场景下指向多个 manifest 的"清单的清单"；
- **Image Layout**：文件系统布局（`.oci/`、`index.json`），用于离线分发。

OCI 1.1（2024-02 发布 1.1.0，其后修复性 release 至 v1.1.1，撰写时最新）没有推翻上述结构，而是回答了一个生态长期痛点：**"除了镜像，registry 还能标准地存什么、怎么关联？"**

### 1.2 1.1 之前的乱象：artifact.manifest 实践

在 1.1 之前，社区通行做法（即所谓 *artifact.manifest* 实践 / OCI Artifacts v1 提案）是：自定义 `artifactType` 字段塞进 image manifest、用 `config.mediaType` 携带自定义类型、甚至发明一个专属的 `application/vnd.oci.artifact.manifest.v1+json` 媒体类型。后果：

- registry 必须维护"已知媒体类型白名单"来猜测内容语义，出现 Harbor/云厂商各自为政的兼容矩阵；
- 签名/SBOM 与镜像的关联靠 **tag 约定**（如 `sha256-<digest>.sig`），tag 可被覆盖、可被删，供应链关系脆弱；
- 客户端要猜"这个 manifest 是不是镜像"，解析路径分叉。

### 1.3 1.1 的关键变化

| 变化 | 内容 | 影响 |
|------|------|------|
| **`artifactType` 标准化** | image manifest 顶层新增正式 `artifactType` 字段（如 `application/vnd.cncf.helm.config.v1+json`），config.mediaType 可统一用通用值 | 类型识别不再依赖 config 猜谜 |
| **subject + referrers API** | manifest 新增 `subject` 字段指向被关联对象（通常是镜像 digest）；distribution-spec 配套提供 `GET /v2/<name>/referrers/<digest>` 列出全部关联物 | 签名/SBOM 与镜像的关系从"tag 约定"升级为"API 查询"，删 tag 不再断链 |
| **artifact.manifest 媒体类型撤销** | 1.1.0-rc 阶段曾引入的 `vnd.oci.artifact.manifest.v1+json` 在正式版中被移除，回归"image manifest + artifactType"单一路径 | 早期按 rc 实现的系统（部分 CNCF 工具旧版本）需要迁移 |
| **空层（empty descriptor）合法化** | 非镜像 artifact（如纯 JSON 策略文件）可用单一 blob + 空 config 表达 | Helm chart、OPA bundle 可无损入库 |
| **`compatibility.md` 行为澄清** | 明确 manifest/index 中保留字段、必填字段的最小实现要求 | 客户端与 registry 实现分歧收窄 |

一句话总结：**1.1 用"artifact 指针（subject）+ 反向查询（referrers）"取代了"artifact.manifest 专属格式 + tag 约定"**。

### 1.4 为什么运维要关心

供应链安全合规（SBOM 留档、签名验证）、GitOps 产物管理（chart 与镜像同库）、平台工程（WASM 模块分发，见 [[14-容器运行时/03-containerd-CRI-O/16-wasm-runtime-integration.md|WASM 运行时集成]]）都正在迁到这套模型。理解 referrers，才能理解 cosign/kyverno/Harbor 的新式校验链路。

---

## 二、原理与架构

### 2.1 关联模型：subject 与 referrers

```text
                    ┌────────────────────────────┐
                    │  Image Index (multi-arch)  │
                    │  digest: sha256:aa11…      │◄──────────┐
                    └──────────┬─────────────────┘           │ subject
                               │ manifest 摘要引用           │
                    ┌──────────▼─────────────────┐    ┌──────┴────────────┐
                    │  Image Manifest (amd64)    │    │ Artifact Manifest │
                    │  digest: sha256:bb22…      │◄───┤ subject: bb22…    │
                    └────────────────────────────┘    │ artifactType:     │
                                                      │  application/     │
                                                      │  vnd.dev.cosign…  │
                                                      │ layers: [签名blob]│
                                                      └───────────────────┘
   查询方向：GET /v2/<repo>/referrers/sha256:bb22… → 返回所有 subject=bb22… 的 artifact
```

要点：

1. **关联是 digest 级的**：不可变、不可被 tag 覆盖，天然适合签名与审计；
2. **referrers 响应是 OCI 1.1 index**（`application/vnd.oci.image.index.v1+json`），带 `annotations` 以便过滤；
3. **fallback 链路**：registry 若未实现 referrers API，客户端回退为"向 `<digest>-artifact-referrers` tag 写一个 index"的兼容模式（oras/cosign 均内置该逻辑），Harbor 同时实现两条路径。

### 2.2 常见 artifact 类型与媒体类型速查

| Artifact | `artifactType` / config 媒体类型 | 用途 |
|----------|----------------------------------|------|
| **Helm chart** | `application/vnd.cncf.helm.config.v1+json`，chart tarball 层 `application/vnd.cncf.helm.chart.content.v1.tar+gzip` | `helm push` 到 OCI registry 即此模型 |
| **WASM 模块** | `application/vnd.wasm.content.layer.v1+wasm`，config `application/vnd.wasm.config.v1+json` | 容器化/非容器化 WASM 分发 |
| **SPDX SBOM** | `application/spdx+json`（或 `application/vnd.spdx+json` 实现差异见第八节） | SBOM 留档与合规扫描 |
| **CycloneDX SBOM** | `application/vnd.cyclonedx+json` / `+xml` | 同上，另一主流格式 |
| **Sigstore 签名（cosign）** | `application/vnd.dev.cosign.simplesigning.v1+json` | 镜像签名与 provenance 附加物 |
| **OPA/策略 bundle** | 自定义，如 `application/vnd.opa.policy.v1+json`（社区约定） | 策略与负载同库存放、按 digest 引用 |

其中 Helm 与 SBOM 属于"镜像的 referrer"（挂 `subject`），WASM/OPA 则常作为独立推送的主对象（不带 subject）。

### 2.3 与镜像分发链路的组装关系

```text
CI 构建镜像 (buildkit)
  ├─ oras attach SBOM   ──┐
  ├─ cosign sign         ──┼─► 全部通过 subject 关联到镜像 digest
  ├─ oras attach provenance┘
  └─ push index/multi-arch
部署侧 (kyverno / admission controller)
  └─ GET referrers(digest) → 过滤 artifactType → 校验签名/SBOM 存在性
```

这套"构建时挂附、准入时查询"的闭环正是 1.1 生态的设计中心。镜像构建侧的背景见 [[14-容器运行时/04-镜像构建/01-buildkit-production-guide.md|BuildKit 生产指南]]，扫描与策略侧见 [[14-容器运行时/02-镜像管理/04-harbor-enterprise-security-scanning.md|Harbor 安全扫描]]。

---

## 三、生产配置实操：oras CLI

oras 是 OCI artifact 推拉的事实标准 CLI（Go 库 oras-go 同源）。以下示例基于 oras v1.3.4。

### 3.1 安装与登录

```bash
# 🟢 低风险：安装 CLI 到本地
brew install oras   # macOS；Linux 用 release tar 或容器方式运行

# 🟢 低风险：登录（凭据进入本地 docker-credential 存储）
oras login registry.example.com -u robot$ci --password "$REGISTRY_TOKEN"
```

### 3.2 推送：SBOM 附加到镜像

```bash
# 🟡 中风险：向 registry 写入新 artifact（可回滚，oras delete 可移除）
IMAGE=registry.example.com/web/app@sha256:bb22…
oras attach --artifact-type application/spdx+json \
  $IMAGE sbom.spdx.json:application/spdx+json
```

多文件、带注解（注解会出现在 referrers 响应里，供下游过滤）：

```bash
# 🟡 中风险：同上，新增带 annotations 的多 blob artifact
oras attach --artifact-type application/vnd.cncf.helm.chart.notes.v1 \
  --annotation "org.opencontainers.image.created=2026-10-14T08:00:00Z" \
  --annotation "vendor.example.com/team=platform" \
  $IMAGE NOTES.md:text/markdown CHANGELOG.md:text/markdown
```

### 3.3 发现与拉取

```bash
# 🟢 低风险：列出镜像的全部 referrers（1.1 核心 API 的 CLI 封装）
oras discover $IMAGE -o tree
# 过滤特定类型
oras discover --artifact-type application/spdx+json -o json $IMAGE

# 🟢 低风险：把 artifact 拉到本地目录
mkdir -p /tmp/sbom && cd /tmp/sbom
oras pull --include-subject "registry.example.com/web/app@sha256:<artifact-digest>"

# 🟢 低风险：仅复制 artifact/镜像到另一 registry（跨域迁移、灾备）
oras cp -r registry.example.com/web/app:1.4.2 harbor-dr.example.com/web/app:1.4.2
```

### 3.4 独立 artifact：Helm chart 与自定义对象

```bash
# 🟡 中风险：推送 Helm chart 到 OCI registry（等价 helm push，二者产物兼容）
helm package ./charts/web
oras push registry.example.com/charts/web:1.4.2 \
  --artifact-type application/vnd.cncf.helm.config.v1+json \
  web-1.4.2.tgz:application/vnd.cncf.helm.chart.content.v1.tar+gzip

# 🟡 中风险：推送 OPA 策略 bundle
oras push registry.example.com/policies/gateway:2026-10-14 \
  --artifact-type application/vnd.opa.policy.v1+json \
  bundle.tar.gz:application/vnd.oci.image.layer.v1.tar+gzip

# 🟢 低风险：清单与 tag 管理
oras manifest fetch --pretty registry.example.com/web/app:1.4.2
oras repo tags registry.example.com/web/app
```

### 3.5 CI/CD 集成片段（GitLab CI 示意）

```yaml
# 🟡 中风险：流水线内推送 artifact（凭据来自 CI secret）
attach-sbom:
  stage: attest
  image: ghcr.io/oras-project/oras:v1.3.4
  script:
    - oras login -u "$HARBOR_ROBOT" -p "$HARBOR_TOKEN" registry.example.com
    - oras attach --artifact-type application/spdx+json
        "$CI_REGISTRY_IMAGE/web/app@${IMAGE_DIGEST}"
        sbom.spdx.json:application/spdx+json
```

---

## 四、registry 兼容性矩阵

| Registry | referrers API | 说明 |
|----------|---------------|------|
| **Harbor ≥ 2.8** | ✅ 原生支持（旧版本走 fallback tag） | 企业首选；与 cosign/oras/Trivy 深度联动，参考 [[14-容器运行时/02-镜像管理/08-harbor-enterprise-guide.md|Harbor 企业指南]] |
| **Docker Hub** | ⚠️ 部分 | 官方 registry 对非镜像 blob 长期保守；artifact 场景不建议作为主存储 |
| **AWS ECR** | ✅（2024 起逐步支持 referrers；个别区域/账号仍走 fallback） | 细节见 [[14-容器运行时/02-镜像管理/07-amazon-ecr-enterprise.md|Amazon ECR 企业实践]] |
| **Azure ACR** | ✅ 支持 OCI 1.1（含 oras/cosign 场景） | |
| **GHCR** | ✅ 支持 artifact 推送（oras 官方演示场） | |
| **Quay** | ⚠️ 历史上对自定义媒体类型较严 | 升级版本后改善，上线前以 `oras discover` 实测为准 |
| **自建 distribution ≥ 3.0** | ✅（3.0 起内置 referrers 支持） | 旧 2.x 需配置 fallback 或升级，见 [[14-容器运行时/02-镜像管理/02-docker-registry-enterprise-distribution.md|Docker Registry 企业分发]] |

**验收命令**（任何 registry 上线 artifact 能力前先跑通这三条）：

```bash
# 🟢 低风险：推 → 查 → 拉 三步验收
oras push  registry.example.com/verify/artifact-referrers:ok README.md:text/plain
oras discover registry.example.com/verify/artifact-referrers:ok
oras manifest fetch registry.example.com/verify/artifact-referrers:ok
```

---

## 五、故障排查与诊断流程

### 5.1 常见报错与定位

| 症状 | 根因方向 | 处理 |
|------|---------|------|
| `oras attach` 报 manifest unknown | subject digest 写错 / 镜像在另一 repo | 先 `oras manifest fetch` 确认 digest |
| `oras discover` 为空 | registry 未支持 referrers 且 fallback tag 未生成 | 升级 registry 或确认客户端版本 ≥ 1.0（自动 fallback） |
| 415 Unsupported Media Type | registry 拒绝自定义媒体类型（老 Quay/代理层拦截） | 升级 registry；检查前置 LB/WAF 是否过滤 |
| cosign 验证失败找不到签名 | 签名推送到了不同 repo（cosign 默认独立签名 repo） | 统一 `--registry-referrers-mode` 或改用 1.1 模式 |
| 拉取 artifact 只得到 config | `oras pull` 默认不拉 subject/注解 | 加 `--include-subject` 等参数 |

### 5.2 诊断流程

```bash
# 🟢 低风险：诊断三步
oras manifest fetch --pretty <ref>          # 1) 看 artifactType/subject 是否正确
oras discover <subject-ref> -o json         # 2) 看 registry 侧关联是否可见
oras repo ls 2>/dev/null || true            # 3) 确认 repo 与权限（401/403 区分认证与授权）
```

网络层排查走 registry 访问日志（Harbor：core 日志 + nginx access log），确认 `/v2/*/referrers/*` 请求的返回码分布——大量 404 说明客户端在探测 fallback，属兼容模式正常现象但意味着 registry 未升级。

---

## 六、最佳实践与反模式

### 6.1 最佳实践

1. **一律用 digest 关联**：CI 中先 `push` 镜像、解析 digest、再 attach，避免 tag 竞态。
2. **artifactType 用 IANA/官方注册值**：SBOM、Helm、cosign 均有约定值，不要自造同义类型造成下游过滤失效。
3. **annotations 携带过滤键**：团队、流水线 ID、过期时间，供准入控制器和清理策略使用。
4. **清理策略纳入 artifact**：Harbor 保留策略要覆盖 referrers，否则签名/SBOM 无限堆积。
5. **多架构镜像 attach 到 index digest**：把签名挂在 index（manifest list）而非单架构 manifest，下游按平台拉取仍可命中关联。
6. **验收前置**：新 registry 上线前跑通第 4 节三条验收命令。

### 6.2 反模式

- ❌ **继续用 tag 约定关联签名**（`sha256-xxx.sig`）：tag 可删可覆盖，供应链证明链断裂风险，应迁移到 subject/referrers。
- ❌ **把 artifact.manifest（rc 时期专属媒体类型）用于新系统**：正式 1.1 已移除该类型，新老工具互不识别。
- ❌ **把 OCI registry 当对象存储滥用**：大文件/高频更新数据（模型权重、日志）应走对象存储；registry 的强项是内容寻址与关联。
- ❌ **忽略 fallback tag 的存储成本**：兼容模式会在 repo 里生成 `<digest>-artifact-referrers` tag，规模大时需要批量清理。
- ❌ **无版本 pin 的 CI 镜像**：oras/构建工具用浮动 tag，某次 release 行为变化即流水线漂移（oras 各版本间 flags 有过调整）。

---

## 七、性能与监控

- **指标**：artifact 数量 / repo（容量治理）、referrers API P99 与错误率（registry 侧）、CI attach 阶段耗时。
- **Harbor**：利用项目配额与垃圾回收（GC 需停写窗口，属 🟡 变更），referrers fallback tag 计数可用 `oras repo tags | grep -c artifact-referrers` 粗估。
- **拉取加速联动**：artifact blob 同样走 blob 分发路径，可被第 9 篇所述的 P2P/缓存方案一并加速，见 [[14-容器运行时/02-镜像管理/09-image-distribution-acceleration.md|镜像分发加速全解]]。
- **规模经验**：单 repo referrers 超过数千条（多流水线 × 多 attest）时，`oras discover` 全量列表变大，下游应按 `--artifact-type` 与 annotations 过滤而非全量解析。

```bash
# 🟢 低风险：粗看一个 repo 的 artifact 规模
oras repo tags registry.example.com/web/app | wc -l
```

---

## 八、版本兼容与升级注意

**版本核对记录（2026-10-14，GitHub Releases API）**：

```bash
# 🟢 低风险：核对最新版本
gh api repos/opencontainers/image-spec/releases/latest --jq .tag_name
# → v1.1.1
gh api repos/oras-project/oras/releases/latest --jq .tag_name
# → v1.3.4
```

| 组件 | 注意事项 |
|------|---------|
| **image-spec v1.1.1** | 1.1.x 系列 修复字段/描述问题，不含破坏性变更；从 1.0 升级主要影响自定义 artifact 客户端 |
| **oras v1.3.x** | flags 与输出格式较 1.x 早期版本有微调；`oras cp` 默认递归行为、`--include-subject` 等以 `oras version` 对应文档为准；CI 镜像建议 pin 到 `v1.3.4` |
| **distribution-spec 1.1** | referrers API 正式版；registry 声明支持需通过 conformance 测试 |
| **Harbor** | ≥ 2.8 原生 referrers；从 fallback 迁移时注意旧 `-artifact-referrers` tag 的清理与保留策略 |
| **cosign ≥ 2.x** | 默认仍用独立签名 repo，可切换 referrers 模式；集群准入策略要同步升级 |
| **Helm ≥ 3.8** | OCI registry 为 stable 特性；`helm pull oci://…` 与 oras 推送的 chart 产物兼容 |
| **containerd/CRI** | 镜像拉取路径对 artifact 无感；WASM artifact 需运行时侧支持（见 [[14-容器运行时/03-containerd-CRI-O/16-wasm-runtime-integration.md|WASM 运行时集成]]） |

升级原则：**先升 registry（补 referrers），再升客户端（切原生模式），最后清理 fallback tag**；每步用 `oras discover` 回归验证。

---

## 九、参考链接与延伸阅读

- OCI Image Spec（v1.1.1）：https://github.com/opencontainers/image-spec
- OCI Distribution Spec（含 referrers API）：https://github.com/opencontainers/distribution-spec
- oras CLI（v1.3.4）：https://github.com/oras-project/oras
- Sigstore / cosign 与 OCI 关联：https://github.com/sigstore/cosign
- SPDX / CycloneDX 媒体类型约定：https://spdx.dev / https://cyclonedx.org
- CNCF Artifact 生态讨论（1.1 演进史）：https://github.com/opencontainers/wg-artifacts
- 库内延伸：[[14-容器运行时/03-containerd-CRI-O/04-oci-runtimes-comparison.md|OCI 运行时对比]]（runtime-spec 侧视角）、[[14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry.md|Harbor 企业级镜像仓库]]

<!-- risk-assessed -->
