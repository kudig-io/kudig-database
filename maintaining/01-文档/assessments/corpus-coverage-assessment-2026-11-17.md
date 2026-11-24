# 语料覆盖评估：vela / terway / ingress-nginx-controller / dubbo / higress / spring-cloud-alibaba / nacos / arthas

- 评估日期：2026-11-17
- 评估范围：`wiki/`（主语料）、`wiki/19-故障诊断/10-QA语料/`（QA 语料）、`skills/`（技能语料）、`code/repo/`（内置源码仓）、`wiki/28-源码分析/`（源码分析）、`console/content/`（发布镜像）
- 评估方法：对每个关键词做全库字面检索（不区分大小写），逐一核对：① 是否有专属文档（文件名/独立成篇）② 正文提及量 ③ QA 语料条数 ④ FTA/工单/技能/培训配套 ⑤ 是否内置源码仓

## 总览

> 状态已按 2026-11-17 的修复与补建更新，详见文末「修复与补建记录」。

| 项目 | 结论 | 专属文档 | 提及量（wiki，约） | QA 语料（约） | 源码仓 | 综合评级 |
|---|---|---|---|---|---|---|
| terway | ✅ 有，且为全库最深 | 28 篇 | 8,655 次 / 308 文件 | 7,805 处 | `code/repo/terway`、`terway-qos` | **完备** |
| higress | ✅ 有 | 5 篇 | 738 次 / 104 文件 | 128 处 | 无 | **完备** |
| ingress-nginx-controller | ✅ 有 | 6 篇 | 1,123 次 / 191 文件（精确词 214 次 / 49 文件） | 203 处 | `code/repo/ingress-nginx` | **完备** |
| vela（KubeVela） | ✅ 6 条 QA 悬空引用已修复 | 3 篇 | 218 次 / 35 文件 | ~32 处（悬空已清零） | 无 | **中等** |
| nacos | ✅ 已补建专属文档 + 8 条 QA | 1 篇（新增 `23-实体/04-网络/nacos.md`） | 130+ 次 / 27+ 文件 | 8 条（新增） | 无 | **可用** |
| dubbo | ✅ 已补建专属文档 + 8 条 QA | 1 篇（新增 `23-实体/04-网络/dubbo.md`） | 63+ 次 / 12+ 文件 | 8 条（新增） | 无 | **可用** |
| spring-cloud-alibaba | ✅ 已补建专属文档 + 7 条 QA | 1 篇（新增 `23-实体/10-平台与开发工具/spring-cloud-alibaba.md`） | 原 2 处 | 7 条（新增） | 无 | **可用** |
| arthas | ✅ 已补建专属文档 + 8 条 QA | 1 篇（新增 `23-实体/10-平台与开发工具/arthas.md`） | 原 2 处 | 8 条（新增） | 无 | **可用** |

## 逐一评估

### 1. terway —— 完备 ✅

全库覆盖最深的项目之一，文档、QA、FTA、技能、培训、工单、源码、源码分析全链路齐全。

- **专属文档（28 篇）**，主要分布：
  - `wiki/05-网络/06-Terway/`：product / architecture / usage / CRD-operations / operations / testing / performance / troubleshooting-fta 等 10 篇 + index
  - `wiki/05-网络/01-K8s网络核心/` 42–49 系列：产品概览、架构深潜、使用指南、CRD 操作、运维手册、性能调优、测试验证、GC 机制、排障 FTA 等 10 篇
  - `wiki/23-实体/04-网络/`：03–10 terway 实体文档 8 篇
  - `wiki/19-故障诊断/06-FTA故障树/list/terway-fta.md`、`wiki/19-故障诊断/04-高级排障/structural-03-networking/07-terway-troubleshooting.md`、`wiki/19-故障诊断/02-资源排障` 相关条目
- **QA 语料**：`10-QA语料/` 内 7,805 处命中（`网络-qa.yaml` 349、`故障诊断-qa.yaml` 76、`generated/command-output-diagnosis-*` 系列 450+，另有归档 `domain-5-networking-qa.yaml`）
- **技能与培训**：`skills/05-网络/cni/`（terway-fta、day-24-terway 实操、kubernetes-terway-presentation）
- **工单**：`ticket-case-001-terway-eni-exhaustion.md`
- **源码**：内置 `code/repo/terway`、`code/repo/terway-qos`；源码分析 `wiki/28-源码分析/analysis/terway/` 19 篇

### 2. higress —— 完备 ✅

- **专属文档（5 篇）**：
  - `wiki/05-网络/04-API网关/04-higress-enterprise-gateway.md`（1,297 行，主力篇）
  - `wiki/19-故障诊断/04-高级排障/structural-03-networking/09-higress-troubleshooting.md`（420 行）
  - `wiki/19-故障诊断/06-FTA故障树/list/higress-fta.md`（300 行）
  - `wiki/21-生态参考/03-领域索引/higress-index.md`（118 行）
  - `skills/05-网络/ingress/higress-fta.md`
- **散见**：API 网关选型/迁移/安全/观测/基准测试等专题均有实质章节（`09-nginx-ingress-migration-guide.md` 46 处、`11-api-gateway-security-practices.md` 32 处等）
- **QA 语料**：128 处（`故障诊断-qa.yaml` 82、`网络-qa.yaml` 21、`生态参考-qa.yaml` 12 等）
- **缺口**：无内置源码仓、无专门工单案例（工单中仅作为排障对象零星出现）

### 3. ingress-nginx-controller —— 完备 ✅

注：检索词为 `ingress-nginx-controller`，库中亦大量使用 `nginx-ingress` 命名，两者均已核对。

- **专属文档（6 篇）**：
  - `wiki/05-网络/01-K8s网络核心/22-nginx-ingress-complete-guide.md`（1,087 行）
  - `wiki/05-网络/04-API网关/09-nginx-ingress-migration-guide.md`（1,190 行）
  - `wiki/19-故障诊断/04-高级排障/structural-03-networking/10-nginx-ingress-troubleshooting.md`（464 行）
  - `wiki/19-故障诊断/06-FTA故障树/list/nginx-ingress-fta.md`（337 行）
  - `wiki/21-生态参考/03-领域索引/nginx-ingress-index.md`（119 行）
  - `wiki/05-网络/01-K8s网络核心/21-ingress-controller-deep-dive.md` 等
- **提及量**：`ingress-nginx` 1,123 次 / 191 文件；精确 `ingress-nginx-controller` 214 次 / 49 文件
- **QA 语料**：`10-QA语料/` 内 203 处（精确 `ingress-nginx-controller` 4 处）
- **工单**：非常密集——`ticket-case-011/016/021/026/031/036/041/046` 共 8 个 ingress-controller 404/502/OOM 案例
- **技能/培训**：`skills/05-网络/ingress/`（nginx-ingress-fta、day-23、演示文稿）
- **源码**：内置 `code/repo/ingress-nginx`
- **小瑕疵**：QA 中精确写 `ingress-nginx-controller` 的仅 4 处，其余以 `ingress-nginx`/泛称出现

### 4. vela（KubeVela）—— 中等 ⚠️

- **专属文档（3 篇）**：
  - `wiki/04-应用模式/01-子模式/06-kubevela-oam-application-delivery.md`（490 行，主力篇）
  - `wiki/23-实体/10-平台与开发工具/kubevela.md`（248 行）
  - `wiki/17-系统基础/06-知识字典/platform-engineering/kubevela.md`（81 行）
- **提及量**：218 次 / 35 文件（含 ACKOne/多集群、CNCF 全景索引等散见）
- **QA 语料**：约 32 处命中（≈11 条 QA），**但全部 source 指向 `wiki/21-生态参考/01-cncf-landscape/incubating/kubevela/kubevela.md`——该路径已不存在**（目录已改名为 `01-CNCF全景`，且其下无 kubevela 文件），即这 11 条 QA 均为悬空引用，检索命中时无法回溯正文。
- **缺口**：无 FTA、无工单、无技能、无培训、无源码仓、无源码分析

### 5. nacos —— 薄弱 ⚠️

- **专属文档：0**。无任何以 nacos 命名的文件
- **提及量**：130 次 / 27 文件，集中于：
  - `wiki/04-应用模式/02-行业架构/20-microservice-governance-architecture.md`（30）
  - `wiki/18-云厂商/01-阿里云/公共云/15-中间件产品选型.md`（17，MSE 语境）
  - `wiki/19-故障诊断/04-高级排障/structural-03-networking/09-higress-troubleshooting.md`（16，Higress 注册集成语境）
  - `wiki/18-云厂商/01-阿里云/公共云/02-微服务架构方案.md`（11）、`wiki/05-网络/03-服务网格/13-spring-cloud-kubernetes-service-mesh-guide.md`（8）
- **QA 语料：0**；无 FTA、无工单、无技能、无源码仓
- **结论**：只在"微服务治理 / 阿里云 MSE / Higress 注册中心"语境下被顺带覆盖，没有可独立检索利用的 nacos 专属语料

### 6. dubbo —— 薄弱 ⚠️

- **专属文档：0**
- **提及量**：63 次 / 12 文件，集中在：
  - `wiki/23-实体/04-网络/aeraki-mesh.md`（22）与 `wiki/17-系统基础/06-知识字典/networking/aeraki-mesh.md`（17）——Dubbo 作为 Aeraki 的目标协议出现
  - 阿里云中间件/微服务选型文档（`15-中间件产品选型` 4、`02-微服务架构方案` 3 等，MSE/SAE 语境）
  - `chaosblade.md`（3，混沌注入对象）、`service-mesh-evolution.md`（2）
- **QA 语料：0**；无 FTA、无工单、无技能、无源码仓（`code/repo/cilium` vendor 里的 envoy dubbo_proxy 不计）
- **结论**：全部为"别人文档里的一个词"，无以 Dubbo 为主体的语料

### 7. spring-cloud-alibaba —— 缺失 ❌

- **提及仅 2 处**，且都是行业架构文档中的选型表格行：
  - `wiki/04-应用模式/02-行业架构/42-secondhand-circular.md:358`
  - `wiki/04-应用模式/02-行业架构/54-social-gaming-metaverse.md:336`
- **专属文档、QA 语料、FTA、工单、技能、源码仓：均为 0**
- **注意区分**：`wiki/05-网络/03-服务网格/13-spring-cloud-kubernetes-service-mesh-guide.md` 是 Spring Cloud **Kubernetes** 主题，不是 Spring Cloud Alibaba，不能计入
- **结论**：8 个项目中覆盖最弱

### 8. arthas —— 缺失 ❌

- **提及仅 2 处**：
  - `wiki/19-故障诊断/05-JVM调优/01-jdk-flight-recorder-k8s.md`（作为 JVM 诊断工具被提及）
  - `wiki/13-生产运维/05-工单案例/ticket-case-002-java-oom-essd-iohang.md`（提到 arthas agent 占用堆外内存）
- **专属文档、QA 语料、FTA、技能、源码仓：均为 0**
- **结论**：无实质语料；Java 诊断场景（JVM 调优、Java-on-K8s）本是现成语料的邻接区，补建成本低

## 镜像与发布链路

- `wiki/` 内容已同步镜像至 `console/content/wiki/`（JSON），本评估涉及的 kubevela、terway、higress、nginx-ingress 等专属文档均有对应 JSON 镜像条目；`skills/` 同步至 `console/content/skills/`。补建语料后需走既有重生成/镜像流程。
- `code/repo/` 为 gitignore 的内置源码仓目录，当前 50 个仓库中与本清单相关的仅 `terway`、`terway-qos`、`ingress-nginx`。

## 结论与建议

1. **直接可用（3/8）**：terway、higress、ingress-nginx-controller——文档 + QA + FTA + 技能齐备，可直接投入检索/训练。
2. **可用但需修（1/8）**：KubeVela——3 篇专属文档质量尚可，但需修复约 11 条 QA 的悬空 source 路径（`01-cncf-landscape` → 现行路径，或重建 CNCF 全景 kubevela 源文件）。
3. **待补建（4/8）**，按建议优先级：
   - **nacos**：散见最多、邻接文档最密（微服务治理、MSE、Higress 注册），最易成篇
   - **dubbo**：Aeraki/MSE/ChaosBlade 语境已有素材，可先建实体文档 + 领域索引
   - **arthas**：与既有 JVM 调优、Java-on-K8s、工单语料直接邻接，补建成本最低
   - **spring-cloud-alibaba**：目前仅 2 处表格提及，需从零建（可与 nacos/dubbo 合并为"阿里云微服务栈"专题一次补齐）

## 修复与补建记录（2026-11-17）

### KubeVela QA 悬空引用修复

- `wiki/19-故障诊断/10-QA语料/生态参考-qa.yaml` 中 6 条 KubeVela QA（4 concept + 2 operation）原指向已改名删除的 `wiki/21-生态参考/01-cncf-landscape/incubating/kubevela/kubevela.md`，已全部重指向现存文档并按真实章节映射：
  - 基本信息 → `wiki/23-实体/10-平台与开发工具/kubevela.md`「概述」
  - 项目概述 → `wiki/04-应用模式/01-子模式/06-kubevela-oam-application-delivery.md`「概述与核心概念」
  - 核心特性 → 实体文档「核心能力」；架构设计 → 实体文档「架构」
  - 两条 operation（安装、`vela up`）→ 实体文档「安装与配置」/「运维操作」，命令同步对齐文档实际写法（`kubevela.net` 域名、`vela up -f app.yaml`）
- YAML 解析校验通过（3,426 条），KubeVela 相关 source 悬空数为 0

### 新增专属语料（4 个项目）

| 项目 | 新增文档（约 150-170 行/篇） | 新增 QA | QA 落点 |
|---|---|---|---|
| nacos | `wiki/23-实体/04-网络/nacos.md` | 8 条 | `应用模式-qa.yaml`（519 → 542，含 dubbo/SCA 合计 +23） |
| dubbo | `wiki/23-实体/04-网络/dubbo.md` | 8 条 | 同上 |
| spring-cloud-alibaba | `wiki/23-实体/10-平台与开发工具/spring-cloud-alibaba.md` | 7 条 | 同上 |
| arthas | `wiki/23-实体/10-平台与开发工具/arthas.md` | 8 条 | `故障诊断-qa.yaml`（2580 → 2588） |

- 每篇文档含完整 frontmatter、生产安全提示、概述/架构/关键机制/K8s 部署/运维操作/故障排查/最佳实践/对比/参考链接/Related 章节，格式与既有实体文档（如 kubevela.md、grpc.md）一致
- 事实核验：Dubbo 确认为 Apache 顶级项目（2019-05 毕业）；Nacos 为阿里巴巴开源项目（非 ASF 孵化项目，已纠正初稿）
- 索引更新：`wiki/23-实体/04-网络/index.md`（+dubbo、nacos）、`wiki/23-实体/10-平台与开发工具/index.md`（+arthas、spring-cloud-alibaba）
- 校验：两个 QA 文件 YAML 可解析、`total_questions` 与实际条数一致、新增 source 路径全部存在、4 篇文档 frontmatter 可解析
- 发布镜像：已运行 `make console-sync` 重建 `console/data/` 与 `console/content/`，`console-corpus.py --check` 通过

### 已知遗留（本次范围外）

1. ~~系统性悬空引用~~ **已清理（2026-11-18 执行原地合并重建）**：`generate-qa-corpus.py` 对 `wiki/19-故障诊断/10-QA语料/` 全部 23 个 QA 文件完成合并式重建——全库悬空 source 清零（此前 `生态参考-qa` 2,642 行 `01-cncf-landscape` 引用、`应用模式-qa` 194 个、`故障诊断-qa` 335 个等历史悬空全部删除），总计 14,319 条 QA；手工补充的 nacos/dubbo/SCA/arthas 条目与 6 条修复后的 KubeVela 条目全部保留，README frontmatter 保留。典型变化：`生态参考-qa` 3426→286、`故障诊断-qa` 2588→1886（删悬空）、`系统基础-qa` 1220→3013、`网络-qa` 567→726（补新文档生成的 QA）。
2. ~~QA 重生成脚本过期~~ **已修复（2026-11-17）**：`code/scripts/generate-qa-corpus.py` 已适配现行目录结构（ROOT 相对定位、`wiki/NN-域名` 解析、FTA/技能/行业架构新路径、`--output-dir` 试运行、README frontmatter 保留）。
3. **四主题深配套未启动**（nacos / dubbo / spring-cloud-alibaba / arthas）：已有专属实体文档 + 31 条 QA（见上表），但对比 terway/higress 的深度（FTA 故障树、工单案例、技能卡、领域索引、内置源码仓）仍为单卡状态。**2026-11-24 复核：仍未启动**，列入重排待办 T7（详见 `structure-reorg-plan-2026-11-18.md` 顺序：T5 迁移之后按需执行）；完成后需跑 `generate-qa-corpus.py` 补 QA 并 `make console-sync`。

### 2026-11-18 → 2026-11-24 状态保鲜补记

- 本评估所依据的 4 篇实体文档（nacos/dubbo/SCA/arthas）与 2 个 QA 文件在 11-18 的合并式悬空清零（遗留#1）中全部保留，结论不受影响。
- 期间结构事件：32-故障案例库并入为 `wiki/32-故障案例库`（4730e7400d）；terway S-1 收尾两项完成（2026-11-24：23-实体侧 8 卡压成 1 张速览卡 763f10084e、terway-index 21 副本落桩 50e452598e）——terway 覆盖统计中 `23-实体/04-网络` 侧由 8 篇收敛为 1 篇速览卡，正文全部仍在 `05-网络/06-Terway/` 主场，「全库最深」结论不变。
- 基线推进：本文档 11-17 写就时的 HEAD 已先后推进至 f6068ba8ed（11-19 B3②）→ 50e452598e（11-24 F3 收尾）。
3. **FTA/工单/技能/源码仓缺口**：4 个补建项目仍无 FTA 故障树、工单案例、技能卡与内置源码仓（terway/higress 级别的深配套未做）。
4. **工作区原有冲突**：`code/GTM/index.html` 处于未解决合并状态（UU），与本次改动无关，未处理。
