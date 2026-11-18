# kuba-database · 全网故障数据库

> 收录云原生、AI 原生及互联网公司公开的生产环境重大故障事件，基于 **SRE / CRE / FDE / SA 四视角**进行结构化复盘，构建可检索、可统计、可演进的故障知识库。

## 项目定位

- **数据范围**：全网公开的重大故障事件（Outage / Incident / Postmortem），涵盖系统中断、性能劣化、数据丢失、安全事件引发的可用性问题等；时间跨度从 **1980 年 ARPANET 全网崩溃**到 **2026 年 7 月 AWS/Azure 区域网络故障系列**（46 年），涵盖全球主要云服务商（AWS、Azure、GCP、阿里云、Cloudflare 等）、互联网平台（Meta、腾讯、Akamai 等）与政企公共服务机构（12306、中国电信、FAA、NHS、ABS、粤康码）的重大故障。
- **数据来源**：官方复盘报告（Postmortem / RCA）、状态页归档、工程博客、监管报告等**可验证的公开信息**；第三方分析仅作佐证并显式标注。
- **分析框架**：SRE 无指责复盘 + CRE 客户可靠性 + FDE 现场数据工程 + **SA 解决方案架构师**四重视角互补分析。
- **存储形式**：纯 Markdown + YAML Frontmatter 元数据，Git 版本控制，零数据库依赖，可被任何工具索引。

## 核心价值

1. **四视角结构化复盘**：每个案例不只是"发生了什么"，而是从系统（SRE）、客户信任（CRE）、数据证据（FDE）、关系与架构（SA）四个维度给出可操作的结论，覆盖"根因 → 影响 → 证据 → 对策"完整链条。
2. **可检索、可统计**：Frontmatter 结构化元数据支持按公司 / 领域 / 根因 / 等级 / 关键词多维检索；`build-index.py` 自动生成统计索引，支撑跨案例的定量分析（见 [docs/sre-cross-case-analysis.md](docs/sre-cross-case-analysis.md)）。
3. **可演进的活库**：Git 即版本机制，官方披露新细节时案例持续更新（`last_updated` + `status: updated`），历史全程可追溯。
4. **工程学习素材**：112 个真实案例覆盖软件缺陷、变更管理、容量过载、安全攻击、硬件设施、商务合同、架构决策、操作防护、网络路由等 12 类根因，是 SRE 面试、故障演练设计、架构评审的最佳语料。

## 方法论：四视角速览

| 视角 | 关注点 | 核心问题 | 在案例中的落点 |
|---|---|---|---|
| **SRE** | 系统与流程 | 系统为什么会失效？如何让它不再失效？ | 根因分析（因素三分法 / 5 Whys）、时间线、预防措施 |
| **CRE** | 客户与信任 | 客户感知到了什么？沟通是否及时透明？ | 影响评估、对外沟通表现、SLA/SLO 视角 |
| **FDE** | 数据与证据 | 现场数据如何还原真相？ | 时间线佐证、检测缺口、可观测性改进 |
| **SA** | 关系与架构 | 故障如何冲击客户关系？客户架构该如何改进？ | 客情危机评估（SA）、客户架构启示（SA） |

四视角协同顺序：**FDE 先行**（数据支撑一切）→ **SRE 定因**（区分 Trigger / Root Cause / Aggravating Factors）→ **CRE 定损**（客户侧度量影响）→ **SA 定策**（信任修复 + 架构建议）。详见 [docs/methodology/README.md](docs/methodology/README.md)。

## 数据现状（2026-10）

| 维度 | 统计 |
|---|---|
| 案例总数 | **112**（SEV-1 × 83 · SEV-2 × 24 · SEV-3 × 5） |
| 时间跨度 | 1980-2026（46 年） |
| 覆盖公司 | 49 家（微软系 17 · AWS 15 · Cloudflare 11 · Google Cloud 9 · Google 6 · 阿里云 4 · Meta 3 · GitHub 3 · 腾讯系 3 · OpenAI 2 · Railway 1 · 华为云 1 · 中国铁路 1 · 中国电信 1 · FAA 1 · NHS 1 · ABS 1 · 粤康码 1 等） |
| 公司类型 | cloud-native × 73 · internet × 29 · ai-native × 4 · public-sector × 6 |
| 技术领域 | 14 个：cloud-infrastructure 37 · networking-dns 16 · saas-platforms 12 · cdn-edge 10 · ai-ml-services 10 · business-operations 7 · public-services 6 · identity-access 4 · sa-architecture 3 · database-storage 3 · 其余 4 个领域各 1 |
| 高频标签 | `cascading-failure` × 27 · `control-plane` × 10 · `bgp` × 8 · `canary-missing` × 7 · `us-east-1` × 7 · `blast-radius` × 7 · `dns` × 6 · `public-service` × 6 |

最新统计以 [INDEX.md](INDEX.md) 为准（自动生成）。

## 目录结构

```
kuba/
├── README.md                  # 本文档
├── CONTRIBUTING.md            # 贡献指南：命名规范、录入流程、版本控制、内容红线
├── INDEX.md                   # 总索引 + 统计概览（由 tools/build-index.py 自动生成，勿手改）
├── docs/
│   ├── methodology/           # 复盘方法论
│   │   ├── README.md          # 四视角总览与协同方式
│   │   ├── sre-postmortem.md  # SRE 无指责复盘方法
│   │   ├── cre-practices.md   # CRE 客户可靠性工程实践
│   │   ├── fde-analysis.md    # FDE 现场数据工程分析方法
│   │   └── sa-practices.md    # SA 解决方案架构师实践（客情危机 + 技术危机）
│   ├── sre-cross-case-analysis.md  # SRE 跨案例定量分析（根因模式 / 恢复时间 / 级联特征）
│   └── severity-and-taxonomy.md    # 故障分级标准与分类体系（领域/根因分类，枚举权威来源）
├── templates/
│   ├── incident-report.md     # 标准故障复盘报告模板（完整版）
│   └── quick-entry.md         # 快速记录模板（信息不全时先占位）
├── incidents/                 # 故障案例库（按领域分目录，当前 112 个案例）
│   ├── README.md              # 领域分类说明与代表案例
│   ├── cloud-infrastructure/  # 云基础设施（AWS / Azure / GCP / 阿里云 …）
│   ├── cdn-edge/              # CDN 与边缘接入
│   ├── networking-dns/        # 网络 / DNS / BGP
│   ├── database-storage/      # 数据库与存储
│   ├── container-orchestration/  # 容器编排与服务发现
│   ├── messaging-streaming/   # 消息与流处理
│   ├── identity-access/       # 身份认证与访问控制
│   ├── saas-platforms/        # SaaS 与互联网平台
│   ├── observability/         # 可观测性平台
│   ├── ai-ml-services/        # AI / ML 服务
│   ├── security-services/     # 安全产品引发的可用性事件
│   ├── business-operations/   # 商务运营（合同终止/定价政策/资产过期等非技术原因）
│   ├── sa-architecture/       # SA 架构与客情（架构决策失误 / 客情危机）
│   └── public-services/       # 公共服务与政企（12306 / 广东电信 / 粤康码 / FAA / ABS / NHS）
└── tools/
    ├── search.sh              # 多维度检索（关键词/公司/领域/根因/等级）
    ├── new-incident.sh        # 从模板创建新案例（自动填充元数据骨架）
    └── build-index.py         # 扫描全库、校验元数据、重建 INDEX.md
```

## 快速开始

### 1. 检索故障案例

```bash
# 全文关键词检索
./tools/search.sh "BGP"

# 按公司检索（不区分大小写）
./tools/search.sh --company cloudflare

# 按技术领域 / 根因分类 / 严重等级检索（枚举值见 docs/severity-and-taxonomy.md）
./tools/search.sh --domain database-storage
./tools/search.sh --cause change-management
./tools/search.sh --severity SEV-1

# 过滤器与关键词可任意组合
./tools/search.sh --severity SEV-1 "DNS"
```

### 2. 阅读故障案例

每个案例文件是自包含的完整复盘报告，建议按以下顺序阅读：

```text
摘要（30 秒速览）→ 影响评估（客户视角损失）→ 时间线（FDE 证据链）
→ 技术细节与根因分析（因素三分法 + 5 Whys）→ 解决过程 → 经验教训
→ 预防与改进措施（预防/缩小爆炸半径/检测/恢复四层防御）
→ SRE/CRE/FDE/SA 视角速览（四视角一句话结论）→ 参考资料
```

### 3. 新增故障案例

```bash
# 用法: ./tools/new-incident.sh <领域目录> <YYYY-MM-DD> <公司> <标题slug>
./tools/new-incident.sh cloud-infrastructure 2026-07-01 example-cloud api-outage
# 生成: incidents/cloud-infrastructure/2026-07-01-example-cloud-api-outage.md
```

录入流程、命名规范与内容红线详见 [CONTRIBUTING.md](CONTRIBUTING.md)。

### 4. 重建索引

```bash
python3 tools/build-index.py          # 校验 + 重建 INDEX.md
python3 tools/build-index.py --check  # 只校验不写文件（提交前检查）
```

## 案例数据规范（摘要）

每个案例文件 = **YAML Frontmatter（结构化元数据，供机器检索）** + **Markdown 正文（复盘内容，供人阅读）**。

正文必备章节：`摘要` → `影响评估` → `时间线` → `技术细节与根因分析` → `解决过程` → `经验教训` → `预防与改进措施` → `SRE/CRE/FDE/SA 视角速览` → `参考资料`。

完整字段定义见 [templates/incident-report.md](templates/incident-report.md)，分级与分类枚举见 [docs/severity-and-taxonomy.md](docs/severity-and-taxonomy.md)。

## 数据质量原则

1. **只收录可验证的公开信息**，每个案例必须附带至少一个权威来源链接（官方复盘优先）。
2. **无指责（Blameless）**：聚焦系统性因素，不针对个人；转述官方报告时保持中立。
3. **区分事实与推断**：官方未披露的细节若来自第三方分析，须明确标注"第三方推断"。
4. **持续更新**：官方后续披露新细节时更新案例并递增 `last_updated`，历史通过 Git 追溯。

## 贡献与维护

- 新增 / 修改案例、修订方法论：阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 后按规范操作。
- 索引一致性：任何案例变更后必须重跑 `tools/build-index.py`，索引文件与案例在同一 commit 提交。
- 发现问题（事实错误、失效链接、缺失案例）：提 Issue 或直接提交修正。

## License 与免责声明

本库内容为对公开信息的整理与再分析，仅用于工程学习与可靠性研究。各案例版权与事实解释权归原披露方所有；如有事实性错误请提 Issue 修正。
