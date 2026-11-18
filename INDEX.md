# INDEX · 故障案例总索引

> ⚠️ 本文件由 `tools/build-index.py` 自动生成，**禁止手工编辑**。
> 生成日期: 2026-10-25 · 案例总数: **112**（含 draft 3 篇）

## 统计概览

| 维度 | 分布 |
|---|---|
| 严重等级 | SEV-1 × 83 · SEV-2 × 24 · SEV-3 × 5 |
| 公司类型 | cloud-native × 73 · ai-native × 4 · internet × 29 · public-sector × 6 |
| 高频标签 | `cascading-failure` × 27 · `control-plane` × 10 · `bgp` × 8 · `canary-missing` × 7 · `us-east-1` × 7 · `blast-radius` × 7 · `dns` × 6 · `public-service` × 6 |

## 全部案例（按日期倒序）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-10-20 | [Google Cloud](incidents/cloud-infrastructure/2026-10-20-gcp-europewest2-power.md) | SEV-1 | 8h | `hardware-facility` | Google Cloud europe-west2（伦敦）供电事件致 b 可用区中断约 8 小时，Revolut/劳埃德/汇丰/英国法院系统连带停摆 |
| 2026-09-30 | [Microsoft Azure](incidents/cloud-infrastructure/2026-09-30-azure-gateway-multiregion.md) | SEV-1 | 5h45m | `change-management` | Azure 平台触发的网关控制面更新在 OS 维护窗口恢复，请求量压垮区域网关管理服务，30 区域网关/混合连接服务中断 5 小时 45 分 |
| 2026-09-29 | [Microsoft Azure](incidents/ai-ml-services/2026-09-29-azure-swedencentral-ai.md) | SEV-2 | 5h55m | `software-bug` | Azure Sweden Central 平台问题致 OpenAI/Foundry/AI 服务间歇失败 5 小时 55 分（PIR 7QL5-Z50） |
| 2026-09-03 | [Microsoft Azure](incidents/ai-ml-services/2026-09-03-azure-eastus-ai-outage.md) | SEV-2 | 1h30m | `dependency-failure` | Azure East US 基础设施故障 90 分钟，ChatGPT/Claude/Grok/Copilot 同窗口集体不可用 |
| 2026-09-01 | [Google Cloud](incidents/cloud-infrastructure/2026-09-01-gcp-uscentral1b-network.md) | SEV-2 | 4h08m | `change-management` | Google Cloud us-central1-b 例行网络 fabric 维护触发单集群异常，15 个产品降级 4 小时 8 分 |
| 2026-08-20 | [Google Cloud](incidents/cloud-infrastructure/2026-08-20-gcp-uswest1-fiber.md) | SEV-3 | 2h22m | `change-management` | Google Cloud us-west1 光纤维护后拥塞，计算/容器/存储/IAM 多服务降级 2 小时 22 分 |
| 2026-07-25 | [华为云](incidents/identity-access/2026-07-25-huaweicloud-intl-iam.md) | SEV-3 | 未披露 | `dependency-failure` | 华为云国际站部分账号异常：控制台登录/资源冻结报告持续约 5 小时，时间上与 IAM 计划升级窗口高度重合 |
| 2026-07-24 | [AWS](incidents/cloud-infrastructure/2026-07-24-aws-uswest2-network-outage.md) | SEV-1 | 1h17m | `network-routing` | AWS us-west-2 区域网络中断（区域至 Seattle Metro 网络硬件故障，主影响 20 分钟但下游恢复尾达 10 小时） |
| 2026-07-23 | [Microsoft Azure](incidents/cloud-infrastructure/2026-07-23-azure-westus-route-removal.md) | SEV-1 | 4h57m | `software-bug` | Azure West US 区域进出流量中断（维护请求转换软件 bug 移除过多 IP 路由，约 5 小时 19 个下游服务级联） |
| 2026-07-16 | [AWS](incidents/cdn-edge/2026-07-16-aws-cloudfront-vpc-origins-outage.md) | SEV-1 | 3h33m | `software-bug` | AWS CloudFront 全球中断（VPC Origins 连接管理 fleet 内部约束致配置分发失败，全球 5xx 约 3.5 小时） |
| 2026-07-14 | [Google Cloud](incidents/cloud-infrastructure/2026-07-14-gcp-vmware-engine-routes.md) | SEV-2 | 10h40m | `config-error` | Google Cloud VMware Engine 网络控制面配置更新丢弃站点间路由， stretched 集群互联中断 10 小时 40 分 |
| 2026-06-10 | [Google](incidents/ai-ml-services/2026-06-10-google-gemini-outage.md) | SEV-1 | 7h05m | `software-bug` | Google Gemini 全球服务中断（工具部署元数据数据库索引热点致极端读争用，约 7 小时错误率升高） |
| 2026-05-29 | [Microsoft Azure](incidents/ai-ml-services/2026-05-29-azure-openai-retry-storm.md) | SEV-2 | 7h26m | `capacity-overload` | Azure OpenAI 上游内部工作负载重试风暴压垮路由层，服务降级 7 小时 26 分 |
| 2026-05-29 | [Microsoft Azure](incidents/cloud-infrastructure/2026-05-29-azure-westus2-lightning-power.md) | SEV-1 | 22h06m | `hardware-facility` | 雷暴致 Azure West US 2 多数据中心市电全失，发电机部分失效 + 制冷中断，故障超出 N+1 冗余持续 22 小时 6 分 |
| 2026-05-19 | [Railway](incidents/business-operations/2026-05-19-railway-gcp-suspension.md) | SEV-2 | 7h54m | `commercial` | Google Cloud 以违反 ToS 为由无预警暂停 Railway 生产账号，平台全站中断约 8 小时 |
| 2026-05-07 | [AWS](incidents/cloud-infrastructure/2026-05-07-aws-thermal-outage.md) | SEV-1 | 未披露 | `hardware-facility` | AWS us-east-1 热失控故障（单可用区冷却失效致服务器过热断电，EC2/EBS 等 150+ 服务受损） |
| 2026-04-24 | [Microsoft Azure](incidents/cloud-infrastructure/2026-04-24-azure-eastus-pubsub-lock.md) | SEV-2 | 11h52m | `software-bug` | Azure East US PubSub 网络控制面锁竞争致资源配给失败 11 小时 52 分 |
| 2026-03-09 | [Microsoft Azure](incidents/ai-ml-services/2026-03-09-azure-openai-gpt52-config.md) | SEV-2 | 20h12m | `config-error` | Azure OpenAI GPT-5.2 配置与已部署引擎版本不兼容，模型调用跨区失败持续 20 小时 12 分 |
| 2026-02-27 | [Google Cloud](incidents/ai-ml-services/2026-02-27-gcp-vertex-safety-filter.md) | SEV-3 | 1h58m | `change-management` | Google Cloud Vertex AI 安全过滤服务配置变更致 Gemini 模型 429/503 错误 1 小时 58 分 |
| 2026-02-20 | [Cloudflare](incidents/networking-dns/2026-02-20-cloudflare-byoip-outage.md) | SEV-1 | 6h07m | `software-bug` | Cloudflare BYOIP 全球前缀撤回故障（清理子任务 API 参数 bug 致 25% BYOIP 前缀被 BGP 撤回约 6 小时） |
| 2026-02-07 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-07-azure-westus-transformer-power.md) | SEV-2 | 20h26m | `hardware-facility` | Azure West US 现场变压器电气故障致数据中心市电全失，间歇性服务不可用 20 小时 26 分 |
| 2026-02-02 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-02-azure-storage-policy-vm-outage.md) | SEV-2 | 10h19m | `change-management` | Azure 存储策略修复误删扩展包匿名读权限，全球 VM/VMSS/AKS 生命周期操作中断，恢复洪峰再击穿托管身份服务 |
| 2026-01-22 | [Cloudflare](incidents/networking-dns/2026-01-22-cloudflare-miami-route-leak.md) | SEV-3 | 未披露 | `config-error` | Cloudflare 迈阿密路由器自动路由策略配置错误致 BGP 前缀意外泄漏 |
| 2026-01-08 | [Cloudflare](incidents/networking-dns/2026-01-08-cloudflare-cname-ordering.md) | SEV-2 | 1h55m | `software-bug` | Cloudflare 1.1.1.1 内存优化变更改变 CNAME 记录顺序，部分 DNS 客户端解析失败约 2 小时 |
| 2025-12-05 | [Cloudflare](incidents/cdn-edge/2025-12-05-cloudflare-react2shell-outage.md) | SEV-1 | 25m | `change-management` | Cloudflare 安全补丁引发中断（React2Shell CVE 缓解措施致 28% 全球流量 500 错误约 25 分钟，FL1 代理规则引擎 nil 崩溃） |
| 2025-11-18 | [Cloudflare](incidents/cdn-edge/2025-11-18-cloudflare-bot-outage.md) | SEV-1 | 5h46m | `software-bug` | Cloudflare 全球 5xx 中断（Bot Management 配置生成 bug 触发性文件超限引发 Rust panic，约 28% 核心流量受损 5.8 小时） |
| 2025-10-29 | [Microsoft Azure](incidents/cdn-edge/2025-10-29-azure-frontdoor-outage.md) | SEV-1 | 8h20m | `config-error` | Azure Front Door 全球中断（租户配置变更绕过安全验证部署至全局控制面，AFD 节点失效约 8 小时） |
| 2025-10-20 | [AWS](incidents/cloud-infrastructure/2025-10-20-aws-dynamodb-dns-cascade.md) | SEV-1 | 14h30m | `software-bug` | AWS us-east-1 大规模级联故障（DynamoDB DNS Enactor 竞态删除端点记录） |
| 2025-09-18 | [Clerk](incidents/sa-architecture/2025-09-18-clerk-database-incident.md) | SEV-1 | 62h11m | `architecture-decision` | Clerk 数据库事件（供应商自动升级触发连接池同步回收，惊群效应致间歇性故障 4 天，诊断被监控采样率掩盖） |
| 2025-08-05 | [Anthropic](incidents/ai-ml-services/2025-08-05-anthropic-quality-degradation.md) | SEV-3 | 未披露 | `software-bug` | Claude 模型质量劣化事件（三个独立基础设施 bug 叠加，约五周输出质量下降） |
| 2025-07-14 | [Cloudflare](incidents/networking-dns/2025-07-14-cloudflare-1111-outage.md) | SEV-1 | 1h02m | `config-error` | Cloudflare 1.1.1.1 DNS 解析器全球中断（遗留配置误包含 1.1.1.1 前缀至预发布服务拓扑，BGP 撤消约 62 分钟） |
| 2025-06-26 | [Clerk](incidents/sa-architecture/2025-06-26-clerk-cloudrun-outage.md) | SEV-1 | 45m | `architecture-decision` | Clerk 全服务中断 45 分钟（单区域架构 + 过度信任 Cloud Run 冗余承诺，认证基础设施整体不可用） |
| 2025-06-12 | [Google Cloud](incidents/cloud-infrastructure/2025-06-12-gcp-service-control-outage.md) | SEV-1 | 10h | `software-bug` | Google Cloud 全球中断（Service Control 空指针崩溃循环） |
| 2025-04-16 | [Zoom](incidents/saas-platforms/2025-04-16-zoom-domain-outage.md) | SEV-1 | 1h47m | `dependency-failure` | Zoom 全球服务中断（GoDaddy Registry 误封 zoom.us 域名，DNS 解析失败致网站/会议/App 不可用约 2 小时） |
| 2025-03-21 | [Cloudflare](incidents/cdn-edge/2025-03-21-cloudflare-r2-outage.md) | SEV-1 | 1h07m | `operational-safeguard` | Cloudflare R2 全球中断（密钥轮换中 `--env production` 参数遗漏致 100% 写入失败 67 分钟，R2/Cache Reserve/Stream 等受损） |
| 2025-01-27 | [DeepSeek](incidents/ai-ml-services/2025-01-27-deepseek-malicious-attack.md) | SEV-2 | 未披露 | `security-attack` | DeepSeek 大规模恶意攻击事件（爆红高峰期遭攻击，限制非 +86 注册，服务间歇中断） |
| 2025-01-08 | [Microsoft Azure](incidents/cloud-infrastructure/2025-01-08-azure-eastus2-outage.md) | SEV-1 | 50h | `change-management` | Azure East US 2 长达 50 小时的网络中断（区域网络配置变更致存储分区不可用，虚拟机/App Service/数据库等大量服务连续 3 天受损） |
| 2024-12-11 | [OpenAI](incidents/ai-ml-services/2024-12-11-openai-k8s.md) | SEV-1 | 4h15m | `change-management` | OpenAI 全平台中断（新遥测服务压垮 Kubernetes 控制面，DNS 缓存延迟暴露） |
| 2024-09-10 | [阿里云](incidents/cloud-infrastructure/2024-09-10-aliyun-singapore-fire.md) | SEV-1 | 30h | `hardware-facility` | 阿里云新加坡数据中心火灾（锂电池爆炸导致可用区 C 长时间停电，部分服务中断超 30 小时，多日恢复） |
| 2024-07-30 | [Microsoft Azure](incidents/cloud-infrastructure/2024-07-30-azure-ddos-amplification.md) | SEV-1 | 7h58m | `security-attack` | Azure 全球服务中断（DDoS 攻击触发防护机制实施错误反向放大攻击效果，Front Door/CDN 间歇不可用约 8 小时） |
| 2024-07-19 | [CrowdStrike](incidents/security-services/2024-07-19-crowdstrike-falcon.md) | SEV-1 | 72h | `software-bug` | CrowdStrike Falcon 内容更新缺陷致全球约 850 万台 Windows 蓝屏 |
| 2024-05-02 | [Google Cloud](incidents/cloud-infrastructure/2024-05-02-gcp-unisuper-deletion.md) | SEV-1 | 336h | `operational-safeguard` | Google Cloud 误删 UniSuper 私有云事故（GCVE 部署工具参数缺省触发一年期自动删除，1350 亿澳元养老基金停摆一周） |
| 2024-04-08 | [腾讯云](incidents/cloud-infrastructure/2024-04-08-tencentcloud-api-outage.md) | SEV-1 | 1h27m | `change-management` | 腾讯云 4·8 云 API 服务异常（新版本前向兼容性问题叠加灰度不足，控制台登录与依赖服务中断 87 分钟） |
| 2024-03-05 | [Meta (Facebook)](incidents/saas-platforms/2024-03-05-meta-outage.md) | SEV-1 | 2h30m | `software-bug` | Meta 全球大规模中断（Facebook/Instagram/Messenger/Threads 登录与信息流故障约 2 小时，2024 年 Meta 首次全球级故障） |
| 2023-11-12 | [阿里云](incidents/identity-access/2023-11-12-aliyun-ak-outage.md) | SEV-1 | 2h30m | `software-bug` | 阿里云全球性服务异常（AK 服务白名单读取异常叠加异常处理缺陷，全线产品管控中断） |
| 2023-10-23 | [语雀（蚂蚁集团）](incidents/saas-platforms/2023-10-23-yuque-storage-outage.md) | SEV-1 | 7h50m | `change-management` | 语雀 10·23 长时间宕机（运维升级工具 bug 误下线存储服务器，从备份恢复历时 8 小时） |
| 2023-09-12 | [Unity](incidents/sa-architecture/2023-09-12-unity-runtime-fee.md) | SEV-2 | 未披露 | `commercial` | Unity Runtime Fee 收费政策引爆开发者生态危机（按安装收费决策 + 傲慢沟通，CEO 下台、政策最终取消） |
| 2023-06-13 | [AWS](incidents/cloud-infrastructure/2023-06-13-aws-lambda-outage.md) | SEV-1 | 3h48m | `software-bug` | AWS us-east-1 Lambda 大规模中断（单 cell 容量阈值触发潜在缺陷，STS/Console/EKS 等级联受损近 4 小时） |
| 2023-06-08 | [中国电信广东](incidents/public-services/2023-06-08-china-telecom-guangdong-outage.md) | SEV-1 | 5h | `network-routing` | 广东电信 6·8 全网故障：移动语音/短信中断约 5 小时（2023-06-08），高考期间被内部定级"特别重大网络故障"B 级 |
| 2023-04-18 | [Reddit](incidents/business-operations/2023-04-18-reddit-api-pricing.md) | SEV-2 | 未披露 | `commercial` | Reddit API 收费引爆第三方生态崩溃（定价决策未协商 + 沟通对抗，Apollo 等客户端集体关闭、社区大罢工） |
| 2023-03-29 | [腾讯](incidents/saas-platforms/2023-03-29-tencent-wechat-outage.md) | SEV-1 | 14h | `hardware-facility` | 微信/QQ 大面积功能异常（广州电信机房冷却系统故障致 14 小时多项服务受损，腾讯定性为一级事故并问责高管） |
| 2023-03-20 | [OpenAI](incidents/ai-ml-services/2023-03-20-openai-redis-leak.md) | SEV-2 | 9h | `software-bug` | ChatGPT 数据泄露事故（redis-py 竞态导致会话标题与部分支付信息串号，主动全站下线） |
| 2023-03-08 | [Datadog](incidents/observability/2023-03-08-datadog-global-outage.md) | SEV-1 | 26h55m | `change-management` | Datadog 全球性宕机（systemd 自动安全更新触发网络失联，五区域全产品受损 27 小时） |
| 2023-01-25 | [Microsoft](incidents/networking-dns/2023-01-25-microsoft-wan-outage.md) | SEV-1 | 5h38m | `change-management` | Microsoft 365 全球中断（WAN 路由器 IP 变更引发全网路由重算） |
| 2023-01-24 | [Cloudflare](incidents/cdn-edge/2023-01-24-cloudflare-zerotrust-outage.md) | SEV-1 | 2h01m | `software-bug` | Cloudflare Zero Trust 中断（Service Token 代码发布覆盖元数据致 121 分钟多服务不可用，WARP/Cache/R2 等受影响） |
| 2023-01-19 | [Twitter (X)](incidents/business-operations/2023-01-19-twitter-api-crackdown.md) | SEV-2 | 未披露 | `commercial` | Twitter 无预警封杀第三方客户端（断流一周后才改协议，Tweetbot/Twitterrific 等 16 年老应用集体死亡） |
| 2023-01-11 | [美国联邦航空管理局（FAA）](incidents/public-services/2023-01-11-faa-notam-outage.md) | SEV-1 | 11h50m | `operational-safeguard` | FAA NOTAM 1·11 故障：承包商误删文件致全美航班停飞（2023-01-11），9/11 以来首次全国地面停飞 |
| 2022-12-18 | [阿里云](incidents/cloud-infrastructure/2022-12-18-aliyun-hongkong-outage.md) | SEV-1 | 15h34m | `hardware-facility` | 阿里云香港可用区 C 大规模服务中断（冷机水路气阻+群控死锁，运营十余年最长大故障） |
| 2022-07-25 | [Google Cloud](incidents/business-operations/2022-07-25-google-cloud-billing-outage.md) | SEV-2 | 225h05m | `change-management` | Google Cloud 账单系统故障（GCE 变更引发用量数据错乱，9 天延迟报告 + 重复计费显示，所有客户账务可见性受损） |
| 2022-07-08 | [Rogers Communications](incidents/networking-dns/2022-07-08-rogers-national-outage.md) | SEV-1 | 19h | `change-management` | Rogers 加拿大全国网络瘫痪（变更删除路由过滤器导致核心网内存耗尽） |
| 2022-06-21 | [Cloudflare](incidents/cdn-edge/2022-06-21-cloudflare-outage.md) | SEV-1 | 1h15m | `config-error` | Cloudflare 全球中断（BGP 前缀通告策略变更中 term 重排序致 19 个数据中心离线约 75 分钟，50% 全球流量受损） |
| 2022-04-05 | [Atlassian](incidents/saas-platforms/2022-04-05-atlassian-site-deletion.md) | SEV-2 | 336h | `operational-safeguard` | Atlassian 站点误删事故（清理脚本用错 ID 永久删除 883 个客户站点，恢复历时 14 天） |
| 2022-01-10 | [粤康码（广东省政务服务数据管理局）](incidents/public-services/2022-01-10-yuekangma-outage.md) | SEV-2 | 2h | `capacity-overload` | 粤康码 1·10 故障：早高峰瞬时流量 140 万次/分钟触发系统保护，全省亮码异常约 2 小时（2022-01-10） |
| 2021-12-07 | [AWS](incidents/cloud-infrastructure/2021-12-07-aws-us-east-1.md) | SEV-1 | 7h05m | `capacity-overload` | AWS us-east-1 内部网络过载引发多服务中断 |
| 2021-10-28 | [Roblox](incidents/container-orchestration/2021-10-28-roblox-consul.md) | SEV-1 | 73h | `software-bug` | Roblox 73 小时全站中断（Consul 流式特性与 BoltDB 性能病理叠加） |
| 2021-10-04 | [Meta (Facebook)](incidents/networking-dns/2021-10-04-facebook-bgp.md) | SEV-1 | 6h05m | `network-routing` | Facebook 全球中断（骨干网维护命令撤销全部 BGP 路由） |
| 2021-07-22 | [Akamai](incidents/networking-dns/2021-07-22-akamai-dns-outage.md) | SEV-1 | 1h | `software-bug` | Akamai Edge DNS 全球中断（DNS 软件缺陷致 Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时） |
| 2021-07-13 | [哔哩哔哩](incidents/cdn-edge/2021-07-13-bilibili-slb-outage.md) | SEV-1 | 3h | `software-bug` | B 站 713 事故（SLB Lua 代码 weight="0" 触发死循环，CPU 100% 全站不可用） |
| 2021-06-08 | [Fastly](incidents/cdn-edge/2021-06-08-fastly-global.md) | SEV-1 | 49m | `software-bug` | Fastly 全球服务中断（客户配置触发潜伏软件缺陷） |
| 2021-03-15 | [Microsoft Azure](incidents/identity-access/2021-03-15-azure-ad-outage.md) | SEV-1 | 3h | `change-management` | Azure AD 全球认证中断（认证系统变更致 Microsoft 365 全家桶认证失败约 3 小时，Teams/Outlook/Xbox 等全部中招） |
| 2021-01-10 | [Parler](incidents/business-operations/2021-01-10-parler-aws-termination.md) | SEV-1 | 未披露 | `commercial` | Parler 被 AWS 终止服务全面下线（商务合同终止 = 数字生存权终结，全平台失联约 36 天） |
| 2021-01-04 | [Slack](incidents/saas-platforms/2021-01-04-slack-new-year-outage.md) | SEV-1 | 4h | `capacity-overload` | Slack 新年首个工作日全球宕机（流量回升叠加 AWS Transit Gateway 扩容滞后） |
| 2020-12-14 | [Google](incidents/identity-access/2020-12-14-google-auth-quota-outage.md) | SEV-1 | 47m | `change-management` | Google 全球认证服务中断（配额系统迁移遗留问题致 User ID 服务配额归零，OAuth/登录全球不可用 47 分钟） |
| 2020-11-25 | [AWS](incidents/messaging-streaming/2020-11-25-aws-kinesis-thread-limit.md) | SEV-1 | 17h | `capacity-overload` | AWS Kinesis 线程上限故障（前端 fleet 扩容触发 OS 线程数超限，us-east-1 级联 17 小时） |
| 2020-08-30 | [CenturyLink (Lumen)](incidents/networking-dns/2020-08-30-centurylink-flowspec-outage.md) | SEV-1 | 5h06m | `network-routing` | CenturyLink/Level 3 全球骨干故障（错误 Flowspec 规则阻断 BGP） |
| 2020-07-17 | [Cloudflare](incidents/networking-dns/2020-07-17-cloudflare-backbone-outage.md) | SEV-1 | 27m | `config-error` | Cloudflare 骨干网中断（骨干网络配置错误导致 27 分钟全球中断，Discord/Shopify 等连带受影响） |
| 2019-07-25 | [GitHub](incidents/business-operations/2019-07-25-github-sanctions-restriction.md) | SEV-2 | 未披露 | `commercial` | GitHub 按美国制裁要求封禁伊朗等国家开发者（合规误伤 + 公共仓库豁免引发争议，两年后 OFAC 发放许可证解除） |
| 2019-07-02 | [Cloudflare](incidents/cdn-edge/2019-07-02-cloudflare-waf-regex.md) | SEV-1 | 27m | `change-management` | Cloudflare 全球边缘 CPU 耗尽（WAF 正则灾难性回溯） |
| 2019-06-02 | [Google Cloud](incidents/cloud-infrastructure/2019-06-02-gcp-network-congestion.md) | SEV-1 | 4h26m | `change-management` | Google Cloud 多区域网络拥塞（自动化误降网络控制面调度） |
| 2019-05-17 | [Salesforce](incidents/saas-platforms/2019-05-17-salesforce-permission-incident.md) | SEV-1 | 15h | `change-management` | Salesforce 权限脚本事故（数据库脚本误授全量修改权限，紧急全局封锁） |
| 2019-03-13 | [Meta (Facebook)](incidents/saas-platforms/2019-03-13-facebook-global-outage.md) | SEV-1 | 14h | `change-management` | Facebook 全球大规模中断（服务器配置变更致 Facebook/Instagram/WhatsApp 全家族瘫痪约 14 小时，2019 年社交平台最大故障） |
| 2019-03-03 | [阿里云](incidents/cloud-infrastructure/2019-03-03-aliyun-io-hang.md) | SEV-1 | 3h | `software-bug` | 阿里云华北 2 可用区 C IO HANG 故障（ECS 服务器磁盘 IO 不响应约 3 小时，内核缺陷导致底层存储链路异常） |
| 2018-10-21 | [GitHub](incidents/database-storage/2018-10-21-github-mysql.md) | SEV-2 | 24h17m | `network-routing` | GitHub 43 秒网络分区引发 MySQL 跨洋脑裂，降级运行 24 小时 |
| 2018-07-20 | [腾讯云](incidents/database-storage/2018-07-20-tencentcloud-data-loss.md) | SEV-2 | 未披露 | `data-integrity` | 腾讯云"前沿数控"数据丢失事件（磁盘静默错误叠加违规迁移操作） |
| 2018-03-02 | [AWS](incidents/cloud-infrastructure/2018-03-02-aws-equinix-power-outage.md) | SEV-1 | 2h | `hardware-facility` | AWS us-east-1 第三方机房电源故障（Equinix 断电致 Alexa/Atlassian/Slack/Twilio 等大面积服务中断约 2 小时） |
| 2017-07-25 | [Marketo](incidents/business-operations/2017-07-25-marketo-domain-expiry.md) | SEV-1 | 未披露 | `operational-safeguard` | Marketo 全球服务中断（marketo.com 域名忘记续费而过期，网站/客户控制台/API 全线下线数小时） |
| 2017-05-12 | [英国国家医疗服务体系（NHS）](incidents/public-services/2017-05-12-nhs-wannacry.md) | SEV-1 | 未披露 | `security-attack` | NHS WannaCry 5·12 攻击：勒索蠕虫瘫痪英格兰 1/3 医疗机构（2017-05-12），未打补丁的"基础 IT 安全"缺失 |
| 2017-02-28 | [AWS](incidents/cloud-infrastructure/2017-02-28-aws-s3-us-east-1.md) | SEV-1 | 4h17m | `operational-safeguard` | AWS S3 us-east-1 大规模服务中断（运维命令输入错误） |
| 2017-01-31 | [GitLab](incidents/database-storage/2017-01-31-gitlab-data-loss.md) | SEV-1 | 18h | `data-integrity` | GitLab.com 生产数据库目录被误删，五重备份机制全部失效 |
| 2016-10-21 | [Dyn](incidents/networking-dns/2016-10-21-dyn-ddos.md) | SEV-1 | 11h | `security-attack` | Dyn 托管 DNS 遭 Mirai 僵尸网络 DDoS，美国东海岸大面积断网 |
| 2016-08-09 | [澳大利亚统计局（ABS）](incidents/public-services/2016-08-09-abs-census-outage.md) | SEV-1 | 43h | `capacity-overload` | 澳洲人口普查 8·9 故障：普查之夜网站崩溃离线 40+ 小时（2016-08-09），DDoS 防护误触发叠加测试盲区，额外成本 3000 万美元 |
| 2015-09-20 | [AWS](incidents/cloud-infrastructure/2015-09-20-aws-dynamodb-outage.md) | SEV-1 | 未披露 | `network-routing` | AWS DynamoDB us-east-1 中断（DNS 解析故障致 DynamoDB 不可用，级联影响多个 AWS 服务） |
| 2015-05-28 | [携程](incidents/saas-platforms/2015-05-28-ctrip-code-deletion.md) | SEV-1 | 12h30m | `operational-safeguard` | 携程 5·28 全站瘫痪（运维误删生产服务器执行代码，12 小时全业务中断） |
| 2015-05-27 | [支付宝（蚂蚁金服）](incidents/networking-dns/2015-05-27-alipay-fiber-cut.md) | SEV-2 | 2h30m | `hardware-facility` | 支付宝杭州光缆挖断故障（异地多活的成人礼） |
| 2015-03-28 | [GitHub](incidents/saas-platforms/2015-03-28-github-ddos-attack.md) | SEV-1 | 未披露 | `security-attack` | GitHub 历史上最大规模 DDoS 攻击（中国境内 Baidu 流量被劫持重定向至 GitHub，持续数日） |
| 2014-11-18 | [Microsoft Azure](incidents/cloud-infrastructure/2014-11-18-azure-storage-outage.md) | SEV-1 | 11h | `change-management` | Azure 存储服务全球中断（性能升级操作中的人为错误致存储集群大面积不可用约 11 小时） |
| 2014-01-24 | [Google](incidents/saas-platforms/2014-01-24-gmail-dual-network-outage.md) | SEV-1 | 1h11m | `network-routing` | Gmail 71 分钟中断（罕见的双重网络故障致 Gmail 双数据中心同时失效，冗余设计失效） |
| 2013-08-16 | [Google](incidents/cloud-infrastructure/2013-08-16-google-all-services-outage.md) | SEV-1 | 5m | `change-management` | Google 全服务 5 分钟中断（批量配置更新工具 bug 同时推送错误配置至全部服务，全球互联网短暂消失） |
| 2013-02-22 | [Microsoft Azure](incidents/cloud-infrastructure/2013-02-22-azure-ssl-expiry.md) | SEV-1 | 12h | `operational-safeguard` | Azure 全球存储中断（HTTPS 证书过期） |
| 2012-12-26 | [中国铁路 12306](incidents/public-services/2012-12-26-china-railway-12306-outage.md) | SEV-2 | 未披露 | `hardware-facility` | 12306 春运购票高峰三天两故障：机房空调故障暂停全网售票（2012-12-26），容量设计不足致全国网络购票瘫痪（后续数年春运反复崩溃） |
| 2012-12-24 | [AWS](incidents/cloud-infrastructure/2012-12-24-aws-elb-state-deletion.md) | SEV-1 | 23h | `operational-safeguard` | AWS ELB 平安夜故障（维护进程误删生产负载均衡器状态数据） |
| 2012-10-22 | [AWS](incidents/cloud-infrastructure/2012-10-22-aws-us-east-1-ebs-outage.md) | SEV-1 | 未披露 | `software-bug` | AWS us-east-1 EBS 大规模中断（运维数据采集代理 latent bug 导致 EBS 存储服务器大面积故障） |
| 2012-02-29 | [Microsoft Azure](incidents/cloud-infrastructure/2012-02-29-azure-leap-year.md) | SEV-1 | 12h | `software-bug` | Azure 闰年证书故障（SSL 证书生成逻辑未处理 2 月 29 日，云计算服务瘫痪超 12 小时） |
| 2011-04-21 | [AWS](incidents/cloud-infrastructure/2011-04-21-aws-ebs-remirroring-storm.md) | SEV-1 | 96h | `change-management` | AWS EBS re-mirroring 风暴（us-east-1 多日故障，云计算史上第一次大考） |
| 2010-02-24 | [Google](incidents/cloud-infrastructure/2010-02-24-google-appengine-outage.md) | SEV-1 | 2h | `operational-safeguard` | Google App Engine 全平台中断（数据中心维护操作失误致 App Engine 控制面不可用约 2 小时） |
| 2008-08-15 | [Google](incidents/saas-platforms/2008-08-15-gmail-outage.md) | SEV-1 | 24h | `software-bug` | Gmail 24 小时大中断（存储系统软件 bug 致用户邮箱锁死，Google 应用服务首次重大信任危机） |
| 2008-02-24 | [YouTube (Google)](incidents/networking-dns/2008-02-24-youtube-bgp-hijack.md) | SEV-1 | 2h | `network-routing` | YouTube 全球 BGP 劫持（巴基斯坦电信黑洞路由泄漏） |
| 2008-02-15 | [AWS](incidents/cloud-infrastructure/2008-02-15-aws-s3-first-outage.md) | SEV-1 | 2h | `capacity-overload` | AWS S3 首次重大中断（认证请求洪峰致单数据中心不可用约 2 小时，云计算"第一次大考"） |
| 2007-09-29 | [AWS](incidents/cloud-infrastructure/2007-09-29-aws-ec2-first-outage.md) | SEV-1 | 未披露 | `data-integrity` | AWS EC2 首次重大故障（beta 期实例数据丢失，云计算史上第一次信任危机） |
| 1997-04-25 | [MAI Network Services](incidents/networking-dns/1997-04-25-as7007-route-leak.md) | SEV-1 | 3h | `network-routing` | AS7007 路由泄漏事件（全表解聚合宣告导致互联网大面积瘫痪） |
| 1990-01-15 | [AT&T](incidents/networking-dns/1990-01-15-att-network-collapse.md) | SEV-1 | 9h | `software-bug` | AT&T 长途网络崩溃（交换机恢复逻辑缺陷引发级联重启） |
| 1980-10-27 | [ARPANET (DARPA/BBN)](incidents/networking-dns/1980-10-27-arpanet-collapse.md) | SEV-1 | 4h | `software-bug` | ARPANET 全网崩溃（状态消息污染与垃圾回收算法缺陷） |

## 按技术领域

### 云基础设施 `cloud-infrastructure`（37）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-10-20 | [Google Cloud](incidents/cloud-infrastructure/2026-10-20-gcp-europewest2-power.md) | SEV-1 | 8h | `hardware-facility` | Google Cloud europe-west2（伦敦）供电事件致 b 可用区中断约 8 小时，Revolut/劳埃德/汇丰/英国法院系统连带停摆 |
| 2026-09-30 | [Microsoft Azure](incidents/cloud-infrastructure/2026-09-30-azure-gateway-multiregion.md) | SEV-1 | 5h45m | `change-management` | Azure 平台触发的网关控制面更新在 OS 维护窗口恢复，请求量压垮区域网关管理服务，30 区域网关/混合连接服务中断 5 小时 45 分 |
| 2026-09-01 | [Google Cloud](incidents/cloud-infrastructure/2026-09-01-gcp-uscentral1b-network.md) | SEV-2 | 4h08m | `change-management` | Google Cloud us-central1-b 例行网络 fabric 维护触发单集群异常，15 个产品降级 4 小时 8 分 |
| 2026-08-20 | [Google Cloud](incidents/cloud-infrastructure/2026-08-20-gcp-uswest1-fiber.md) | SEV-3 | 2h22m | `change-management` | Google Cloud us-west1 光纤维护后拥塞，计算/容器/存储/IAM 多服务降级 2 小时 22 分 |
| 2026-07-24 | [AWS](incidents/cloud-infrastructure/2026-07-24-aws-uswest2-network-outage.md) | SEV-1 | 1h17m | `network-routing` | AWS us-west-2 区域网络中断（区域至 Seattle Metro 网络硬件故障，主影响 20 分钟但下游恢复尾达 10 小时） |
| 2026-07-23 | [Microsoft Azure](incidents/cloud-infrastructure/2026-07-23-azure-westus-route-removal.md) | SEV-1 | 4h57m | `software-bug` | Azure West US 区域进出流量中断（维护请求转换软件 bug 移除过多 IP 路由，约 5 小时 19 个下游服务级联） |
| 2026-07-14 | [Google Cloud](incidents/cloud-infrastructure/2026-07-14-gcp-vmware-engine-routes.md) | SEV-2 | 10h40m | `config-error` | Google Cloud VMware Engine 网络控制面配置更新丢弃站点间路由， stretched 集群互联中断 10 小时 40 分 |
| 2026-05-29 | [Microsoft Azure](incidents/cloud-infrastructure/2026-05-29-azure-westus2-lightning-power.md) | SEV-1 | 22h06m | `hardware-facility` | 雷暴致 Azure West US 2 多数据中心市电全失，发电机部分失效 + 制冷中断，故障超出 N+1 冗余持续 22 小时 6 分 |
| 2026-05-07 | [AWS](incidents/cloud-infrastructure/2026-05-07-aws-thermal-outage.md) | SEV-1 | 未披露 | `hardware-facility` | AWS us-east-1 热失控故障（单可用区冷却失效致服务器过热断电，EC2/EBS 等 150+ 服务受损） |
| 2026-04-24 | [Microsoft Azure](incidents/cloud-infrastructure/2026-04-24-azure-eastus-pubsub-lock.md) | SEV-2 | 11h52m | `software-bug` | Azure East US PubSub 网络控制面锁竞争致资源配给失败 11 小时 52 分 |
| 2026-02-07 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-07-azure-westus-transformer-power.md) | SEV-2 | 20h26m | `hardware-facility` | Azure West US 现场变压器电气故障致数据中心市电全失，间歇性服务不可用 20 小时 26 分 |
| 2026-02-02 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-02-azure-storage-policy-vm-outage.md) | SEV-2 | 10h19m | `change-management` | Azure 存储策略修复误删扩展包匿名读权限，全球 VM/VMSS/AKS 生命周期操作中断，恢复洪峰再击穿托管身份服务 |
| 2025-10-20 | [AWS](incidents/cloud-infrastructure/2025-10-20-aws-dynamodb-dns-cascade.md) | SEV-1 | 14h30m | `software-bug` | AWS us-east-1 大规模级联故障（DynamoDB DNS Enactor 竞态删除端点记录） |
| 2025-06-12 | [Google Cloud](incidents/cloud-infrastructure/2025-06-12-gcp-service-control-outage.md) | SEV-1 | 10h | `software-bug` | Google Cloud 全球中断（Service Control 空指针崩溃循环） |
| 2025-01-08 | [Microsoft Azure](incidents/cloud-infrastructure/2025-01-08-azure-eastus2-outage.md) | SEV-1 | 50h | `change-management` | Azure East US 2 长达 50 小时的网络中断（区域网络配置变更致存储分区不可用，虚拟机/App Service/数据库等大量服务连续 3 天受损） |
| 2024-09-10 | [阿里云](incidents/cloud-infrastructure/2024-09-10-aliyun-singapore-fire.md) | SEV-1 | 30h | `hardware-facility` | 阿里云新加坡数据中心火灾（锂电池爆炸导致可用区 C 长时间停电，部分服务中断超 30 小时，多日恢复） |
| 2024-07-30 | [Microsoft Azure](incidents/cloud-infrastructure/2024-07-30-azure-ddos-amplification.md) | SEV-1 | 7h58m | `security-attack` | Azure 全球服务中断（DDoS 攻击触发防护机制实施错误反向放大攻击效果，Front Door/CDN 间歇不可用约 8 小时） |
| 2024-05-02 | [Google Cloud](incidents/cloud-infrastructure/2024-05-02-gcp-unisuper-deletion.md) | SEV-1 | 336h | `operational-safeguard` | Google Cloud 误删 UniSuper 私有云事故（GCVE 部署工具参数缺省触发一年期自动删除，1350 亿澳元养老基金停摆一周） |
| 2024-04-08 | [腾讯云](incidents/cloud-infrastructure/2024-04-08-tencentcloud-api-outage.md) | SEV-1 | 1h27m | `change-management` | 腾讯云 4·8 云 API 服务异常（新版本前向兼容性问题叠加灰度不足，控制台登录与依赖服务中断 87 分钟） |
| 2023-06-13 | [AWS](incidents/cloud-infrastructure/2023-06-13-aws-lambda-outage.md) | SEV-1 | 3h48m | `software-bug` | AWS us-east-1 Lambda 大规模中断（单 cell 容量阈值触发潜在缺陷，STS/Console/EKS 等级联受损近 4 小时） |
| 2022-12-18 | [阿里云](incidents/cloud-infrastructure/2022-12-18-aliyun-hongkong-outage.md) | SEV-1 | 15h34m | `hardware-facility` | 阿里云香港可用区 C 大规模服务中断（冷机水路气阻+群控死锁，运营十余年最长大故障） |
| 2021-12-07 | [AWS](incidents/cloud-infrastructure/2021-12-07-aws-us-east-1.md) | SEV-1 | 7h05m | `capacity-overload` | AWS us-east-1 内部网络过载引发多服务中断 |
| 2019-06-02 | [Google Cloud](incidents/cloud-infrastructure/2019-06-02-gcp-network-congestion.md) | SEV-1 | 4h26m | `change-management` | Google Cloud 多区域网络拥塞（自动化误降网络控制面调度） |
| 2019-03-03 | [阿里云](incidents/cloud-infrastructure/2019-03-03-aliyun-io-hang.md) | SEV-1 | 3h | `software-bug` | 阿里云华北 2 可用区 C IO HANG 故障（ECS 服务器磁盘 IO 不响应约 3 小时，内核缺陷导致底层存储链路异常） |
| 2018-03-02 | [AWS](incidents/cloud-infrastructure/2018-03-02-aws-equinix-power-outage.md) | SEV-1 | 2h | `hardware-facility` | AWS us-east-1 第三方机房电源故障（Equinix 断电致 Alexa/Atlassian/Slack/Twilio 等大面积服务中断约 2 小时） |
| 2017-02-28 | [AWS](incidents/cloud-infrastructure/2017-02-28-aws-s3-us-east-1.md) | SEV-1 | 4h17m | `operational-safeguard` | AWS S3 us-east-1 大规模服务中断（运维命令输入错误） |
| 2015-09-20 | [AWS](incidents/cloud-infrastructure/2015-09-20-aws-dynamodb-outage.md) | SEV-1 | 未披露 | `network-routing` | AWS DynamoDB us-east-1 中断（DNS 解析故障致 DynamoDB 不可用，级联影响多个 AWS 服务） |
| 2014-11-18 | [Microsoft Azure](incidents/cloud-infrastructure/2014-11-18-azure-storage-outage.md) | SEV-1 | 11h | `change-management` | Azure 存储服务全球中断（性能升级操作中的人为错误致存储集群大面积不可用约 11 小时） |
| 2013-08-16 | [Google](incidents/cloud-infrastructure/2013-08-16-google-all-services-outage.md) | SEV-1 | 5m | `change-management` | Google 全服务 5 分钟中断（批量配置更新工具 bug 同时推送错误配置至全部服务，全球互联网短暂消失） |
| 2013-02-22 | [Microsoft Azure](incidents/cloud-infrastructure/2013-02-22-azure-ssl-expiry.md) | SEV-1 | 12h | `operational-safeguard` | Azure 全球存储中断（HTTPS 证书过期） |
| 2012-12-24 | [AWS](incidents/cloud-infrastructure/2012-12-24-aws-elb-state-deletion.md) | SEV-1 | 23h | `operational-safeguard` | AWS ELB 平安夜故障（维护进程误删生产负载均衡器状态数据） |
| 2012-10-22 | [AWS](incidents/cloud-infrastructure/2012-10-22-aws-us-east-1-ebs-outage.md) | SEV-1 | 未披露 | `software-bug` | AWS us-east-1 EBS 大规模中断（运维数据采集代理 latent bug 导致 EBS 存储服务器大面积故障） |
| 2012-02-29 | [Microsoft Azure](incidents/cloud-infrastructure/2012-02-29-azure-leap-year.md) | SEV-1 | 12h | `software-bug` | Azure 闰年证书故障（SSL 证书生成逻辑未处理 2 月 29 日，云计算服务瘫痪超 12 小时） |
| 2011-04-21 | [AWS](incidents/cloud-infrastructure/2011-04-21-aws-ebs-remirroring-storm.md) | SEV-1 | 96h | `change-management` | AWS EBS re-mirroring 风暴（us-east-1 多日故障，云计算史上第一次大考） |
| 2010-02-24 | [Google](incidents/cloud-infrastructure/2010-02-24-google-appengine-outage.md) | SEV-1 | 2h | `operational-safeguard` | Google App Engine 全平台中断（数据中心维护操作失误致 App Engine 控制面不可用约 2 小时） |
| 2008-02-15 | [AWS](incidents/cloud-infrastructure/2008-02-15-aws-s3-first-outage.md) | SEV-1 | 2h | `capacity-overload` | AWS S3 首次重大中断（认证请求洪峰致单数据中心不可用约 2 小时，云计算"第一次大考"） |
| 2007-09-29 | [AWS](incidents/cloud-infrastructure/2007-09-29-aws-ec2-first-outage.md) | SEV-1 | 未披露 | `data-integrity` | AWS EC2 首次重大故障（beta 期实例数据丢失，云计算史上第一次信任危机） |

### CDN 与边缘接入 `cdn-edge`（10）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-07-16 | [AWS](incidents/cdn-edge/2026-07-16-aws-cloudfront-vpc-origins-outage.md) | SEV-1 | 3h33m | `software-bug` | AWS CloudFront 全球中断（VPC Origins 连接管理 fleet 内部约束致配置分发失败，全球 5xx 约 3.5 小时） |
| 2025-12-05 | [Cloudflare](incidents/cdn-edge/2025-12-05-cloudflare-react2shell-outage.md) | SEV-1 | 25m | `change-management` | Cloudflare 安全补丁引发中断（React2Shell CVE 缓解措施致 28% 全球流量 500 错误约 25 分钟，FL1 代理规则引擎 nil 崩溃） |
| 2025-11-18 | [Cloudflare](incidents/cdn-edge/2025-11-18-cloudflare-bot-outage.md) | SEV-1 | 5h46m | `software-bug` | Cloudflare 全球 5xx 中断（Bot Management 配置生成 bug 触发性文件超限引发 Rust panic，约 28% 核心流量受损 5.8 小时） |
| 2025-10-29 | [Microsoft Azure](incidents/cdn-edge/2025-10-29-azure-frontdoor-outage.md) | SEV-1 | 8h20m | `config-error` | Azure Front Door 全球中断（租户配置变更绕过安全验证部署至全局控制面，AFD 节点失效约 8 小时） |
| 2025-03-21 | [Cloudflare](incidents/cdn-edge/2025-03-21-cloudflare-r2-outage.md) | SEV-1 | 1h07m | `operational-safeguard` | Cloudflare R2 全球中断（密钥轮换中 `--env production` 参数遗漏致 100% 写入失败 67 分钟，R2/Cache Reserve/Stream 等受损） |
| 2023-01-24 | [Cloudflare](incidents/cdn-edge/2023-01-24-cloudflare-zerotrust-outage.md) | SEV-1 | 2h01m | `software-bug` | Cloudflare Zero Trust 中断（Service Token 代码发布覆盖元数据致 121 分钟多服务不可用，WARP/Cache/R2 等受影响） |
| 2022-06-21 | [Cloudflare](incidents/cdn-edge/2022-06-21-cloudflare-outage.md) | SEV-1 | 1h15m | `config-error` | Cloudflare 全球中断（BGP 前缀通告策略变更中 term 重排序致 19 个数据中心离线约 75 分钟，50% 全球流量受损） |
| 2021-07-13 | [哔哩哔哩](incidents/cdn-edge/2021-07-13-bilibili-slb-outage.md) | SEV-1 | 3h | `software-bug` | B 站 713 事故（SLB Lua 代码 weight="0" 触发死循环，CPU 100% 全站不可用） |
| 2021-06-08 | [Fastly](incidents/cdn-edge/2021-06-08-fastly-global.md) | SEV-1 | 49m | `software-bug` | Fastly 全球服务中断（客户配置触发潜伏软件缺陷） |
| 2019-07-02 | [Cloudflare](incidents/cdn-edge/2019-07-02-cloudflare-waf-regex.md) | SEV-1 | 27m | `change-management` | Cloudflare 全球边缘 CPU 耗尽（WAF 正则灾难性回溯） |

### 网络 / DNS / BGP `networking-dns`（16）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-02-20 | [Cloudflare](incidents/networking-dns/2026-02-20-cloudflare-byoip-outage.md) | SEV-1 | 6h07m | `software-bug` | Cloudflare BYOIP 全球前缀撤回故障（清理子任务 API 参数 bug 致 25% BYOIP 前缀被 BGP 撤回约 6 小时） |
| 2026-01-22 | [Cloudflare](incidents/networking-dns/2026-01-22-cloudflare-miami-route-leak.md) | SEV-3 | 未披露 | `config-error` | Cloudflare 迈阿密路由器自动路由策略配置错误致 BGP 前缀意外泄漏 |
| 2026-01-08 | [Cloudflare](incidents/networking-dns/2026-01-08-cloudflare-cname-ordering.md) | SEV-2 | 1h55m | `software-bug` | Cloudflare 1.1.1.1 内存优化变更改变 CNAME 记录顺序，部分 DNS 客户端解析失败约 2 小时 |
| 2025-07-14 | [Cloudflare](incidents/networking-dns/2025-07-14-cloudflare-1111-outage.md) | SEV-1 | 1h02m | `config-error` | Cloudflare 1.1.1.1 DNS 解析器全球中断（遗留配置误包含 1.1.1.1 前缀至预发布服务拓扑，BGP 撤消约 62 分钟） |
| 2023-01-25 | [Microsoft](incidents/networking-dns/2023-01-25-microsoft-wan-outage.md) | SEV-1 | 5h38m | `change-management` | Microsoft 365 全球中断（WAN 路由器 IP 变更引发全网路由重算） |
| 2022-07-08 | [Rogers Communications](incidents/networking-dns/2022-07-08-rogers-national-outage.md) | SEV-1 | 19h | `change-management` | Rogers 加拿大全国网络瘫痪（变更删除路由过滤器导致核心网内存耗尽） |
| 2021-10-04 | [Meta (Facebook)](incidents/networking-dns/2021-10-04-facebook-bgp.md) | SEV-1 | 6h05m | `network-routing` | Facebook 全球中断（骨干网维护命令撤销全部 BGP 路由） |
| 2021-07-22 | [Akamai](incidents/networking-dns/2021-07-22-akamai-dns-outage.md) | SEV-1 | 1h | `software-bug` | Akamai Edge DNS 全球中断（DNS 软件缺陷致 Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时） |
| 2020-08-30 | [CenturyLink (Lumen)](incidents/networking-dns/2020-08-30-centurylink-flowspec-outage.md) | SEV-1 | 5h06m | `network-routing` | CenturyLink/Level 3 全球骨干故障（错误 Flowspec 规则阻断 BGP） |
| 2020-07-17 | [Cloudflare](incidents/networking-dns/2020-07-17-cloudflare-backbone-outage.md) | SEV-1 | 27m | `config-error` | Cloudflare 骨干网中断（骨干网络配置错误导致 27 分钟全球中断，Discord/Shopify 等连带受影响） |
| 2016-10-21 | [Dyn](incidents/networking-dns/2016-10-21-dyn-ddos.md) | SEV-1 | 11h | `security-attack` | Dyn 托管 DNS 遭 Mirai 僵尸网络 DDoS，美国东海岸大面积断网 |
| 2015-05-27 | [支付宝（蚂蚁金服）](incidents/networking-dns/2015-05-27-alipay-fiber-cut.md) | SEV-2 | 2h30m | `hardware-facility` | 支付宝杭州光缆挖断故障（异地多活的成人礼） |
| 2008-02-24 | [YouTube (Google)](incidents/networking-dns/2008-02-24-youtube-bgp-hijack.md) | SEV-1 | 2h | `network-routing` | YouTube 全球 BGP 劫持（巴基斯坦电信黑洞路由泄漏） |
| 1997-04-25 | [MAI Network Services](incidents/networking-dns/1997-04-25-as7007-route-leak.md) | SEV-1 | 3h | `network-routing` | AS7007 路由泄漏事件（全表解聚合宣告导致互联网大面积瘫痪） |
| 1990-01-15 | [AT&T](incidents/networking-dns/1990-01-15-att-network-collapse.md) | SEV-1 | 9h | `software-bug` | AT&T 长途网络崩溃（交换机恢复逻辑缺陷引发级联重启） |
| 1980-10-27 | [ARPANET (DARPA/BBN)](incidents/networking-dns/1980-10-27-arpanet-collapse.md) | SEV-1 | 4h | `software-bug` | ARPANET 全网崩溃（状态消息污染与垃圾回收算法缺陷） |

### 数据库与存储 `database-storage`（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2018-10-21 | [GitHub](incidents/database-storage/2018-10-21-github-mysql.md) | SEV-2 | 24h17m | `network-routing` | GitHub 43 秒网络分区引发 MySQL 跨洋脑裂，降级运行 24 小时 |
| 2018-07-20 | [腾讯云](incidents/database-storage/2018-07-20-tencentcloud-data-loss.md) | SEV-2 | 未披露 | `data-integrity` | 腾讯云"前沿数控"数据丢失事件（磁盘静默错误叠加违规迁移操作） |
| 2017-01-31 | [GitLab](incidents/database-storage/2017-01-31-gitlab-data-loss.md) | SEV-1 | 18h | `data-integrity` | GitLab.com 生产数据库目录被误删，五重备份机制全部失效 |

### 容器编排与服务发现 `container-orchestration`（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2021-10-28 | [Roblox](incidents/container-orchestration/2021-10-28-roblox-consul.md) | SEV-1 | 73h | `software-bug` | Roblox 73 小时全站中断（Consul 流式特性与 BoltDB 性能病理叠加） |

### AI / ML 服务 `ai-ml-services`（10）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-09-29 | [Microsoft Azure](incidents/ai-ml-services/2026-09-29-azure-swedencentral-ai.md) | SEV-2 | 5h55m | `software-bug` | Azure Sweden Central 平台问题致 OpenAI/Foundry/AI 服务间歇失败 5 小时 55 分（PIR 7QL5-Z50） |
| 2026-09-03 | [Microsoft Azure](incidents/ai-ml-services/2026-09-03-azure-eastus-ai-outage.md) | SEV-2 | 1h30m | `dependency-failure` | Azure East US 基础设施故障 90 分钟，ChatGPT/Claude/Grok/Copilot 同窗口集体不可用 |
| 2026-06-10 | [Google](incidents/ai-ml-services/2026-06-10-google-gemini-outage.md) | SEV-1 | 7h05m | `software-bug` | Google Gemini 全球服务中断（工具部署元数据数据库索引热点致极端读争用，约 7 小时错误率升高） |
| 2026-05-29 | [Microsoft Azure](incidents/ai-ml-services/2026-05-29-azure-openai-retry-storm.md) | SEV-2 | 7h26m | `capacity-overload` | Azure OpenAI 上游内部工作负载重试风暴压垮路由层，服务降级 7 小时 26 分 |
| 2026-03-09 | [Microsoft Azure](incidents/ai-ml-services/2026-03-09-azure-openai-gpt52-config.md) | SEV-2 | 20h12m | `config-error` | Azure OpenAI GPT-5.2 配置与已部署引擎版本不兼容，模型调用跨区失败持续 20 小时 12 分 |
| 2026-02-27 | [Google Cloud](incidents/ai-ml-services/2026-02-27-gcp-vertex-safety-filter.md) | SEV-3 | 1h58m | `change-management` | Google Cloud Vertex AI 安全过滤服务配置变更致 Gemini 模型 429/503 错误 1 小时 58 分 |
| 2025-08-05 | [Anthropic](incidents/ai-ml-services/2025-08-05-anthropic-quality-degradation.md) | SEV-3 | 未披露 | `software-bug` | Claude 模型质量劣化事件（三个独立基础设施 bug 叠加，约五周输出质量下降） |
| 2025-01-27 | [DeepSeek](incidents/ai-ml-services/2025-01-27-deepseek-malicious-attack.md) | SEV-2 | 未披露 | `security-attack` | DeepSeek 大规模恶意攻击事件（爆红高峰期遭攻击，限制非 +86 注册，服务间歇中断） |
| 2024-12-11 | [OpenAI](incidents/ai-ml-services/2024-12-11-openai-k8s.md) | SEV-1 | 4h15m | `change-management` | OpenAI 全平台中断（新遥测服务压垮 Kubernetes 控制面，DNS 缓存延迟暴露） |
| 2023-03-20 | [OpenAI](incidents/ai-ml-services/2023-03-20-openai-redis-leak.md) | SEV-2 | 9h | `software-bug` | ChatGPT 数据泄露事故（redis-py 竞态导致会话标题与部分支付信息串号，主动全站下线） |

### 安全产品可用性 `security-services`（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2024-07-19 | [CrowdStrike](incidents/security-services/2024-07-19-crowdstrike-falcon.md) | SEV-1 | 72h | `software-bug` | CrowdStrike Falcon 内容更新缺陷致全球约 850 万台 Windows 蓝屏 |

### SaaS 与互联网平台 `saas-platforms`（12）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-04-16 | [Zoom](incidents/saas-platforms/2025-04-16-zoom-domain-outage.md) | SEV-1 | 1h47m | `dependency-failure` | Zoom 全球服务中断（GoDaddy Registry 误封 zoom.us 域名，DNS 解析失败致网站/会议/App 不可用约 2 小时） |
| 2024-03-05 | [Meta (Facebook)](incidents/saas-platforms/2024-03-05-meta-outage.md) | SEV-1 | 2h30m | `software-bug` | Meta 全球大规模中断（Facebook/Instagram/Messenger/Threads 登录与信息流故障约 2 小时，2024 年 Meta 首次全球级故障） |
| 2023-10-23 | [语雀（蚂蚁集团）](incidents/saas-platforms/2023-10-23-yuque-storage-outage.md) | SEV-1 | 7h50m | `change-management` | 语雀 10·23 长时间宕机（运维升级工具 bug 误下线存储服务器，从备份恢复历时 8 小时） |
| 2023-03-29 | [腾讯](incidents/saas-platforms/2023-03-29-tencent-wechat-outage.md) | SEV-1 | 14h | `hardware-facility` | 微信/QQ 大面积功能异常（广州电信机房冷却系统故障致 14 小时多项服务受损，腾讯定性为一级事故并问责高管） |
| 2022-04-05 | [Atlassian](incidents/saas-platforms/2022-04-05-atlassian-site-deletion.md) | SEV-2 | 336h | `operational-safeguard` | Atlassian 站点误删事故（清理脚本用错 ID 永久删除 883 个客户站点，恢复历时 14 天） |
| 2021-01-04 | [Slack](incidents/saas-platforms/2021-01-04-slack-new-year-outage.md) | SEV-1 | 4h | `capacity-overload` | Slack 新年首个工作日全球宕机（流量回升叠加 AWS Transit Gateway 扩容滞后） |
| 2019-05-17 | [Salesforce](incidents/saas-platforms/2019-05-17-salesforce-permission-incident.md) | SEV-1 | 15h | `change-management` | Salesforce 权限脚本事故（数据库脚本误授全量修改权限，紧急全局封锁） |
| 2019-03-13 | [Meta (Facebook)](incidents/saas-platforms/2019-03-13-facebook-global-outage.md) | SEV-1 | 14h | `change-management` | Facebook 全球大规模中断（服务器配置变更致 Facebook/Instagram/WhatsApp 全家族瘫痪约 14 小时，2019 年社交平台最大故障） |
| 2015-05-28 | [携程](incidents/saas-platforms/2015-05-28-ctrip-code-deletion.md) | SEV-1 | 12h30m | `operational-safeguard` | 携程 5·28 全站瘫痪（运维误删生产服务器执行代码，12 小时全业务中断） |
| 2015-03-28 | [GitHub](incidents/saas-platforms/2015-03-28-github-ddos-attack.md) | SEV-1 | 未披露 | `security-attack` | GitHub 历史上最大规模 DDoS 攻击（中国境内 Baidu 流量被劫持重定向至 GitHub，持续数日） |
| 2014-01-24 | [Google](incidents/saas-platforms/2014-01-24-gmail-dual-network-outage.md) | SEV-1 | 1h11m | `network-routing` | Gmail 71 分钟中断（罕见的双重网络故障致 Gmail 双数据中心同时失效，冗余设计失效） |
| 2008-08-15 | [Google](incidents/saas-platforms/2008-08-15-gmail-outage.md) | SEV-1 | 24h | `software-bug` | Gmail 24 小时大中断（存储系统软件 bug 致用户邮箱锁死，Google 应用服务首次重大信任危机） |

### 消息与流处理 `messaging-streaming`（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2020-11-25 | [AWS](incidents/messaging-streaming/2020-11-25-aws-kinesis-thread-limit.md) | SEV-1 | 17h | `capacity-overload` | AWS Kinesis 线程上限故障（前端 fleet 扩容触发 OS 线程数超限，us-east-1 级联 17 小时） |

### 可观测性平台 `observability`（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2023-03-08 | [Datadog](incidents/observability/2023-03-08-datadog-global-outage.md) | SEV-1 | 26h55m | `change-management` | Datadog 全球性宕机（systemd 自动安全更新触发网络失联，五区域全产品受损 27 小时） |

### 身份与访问控制 `identity-access`（4）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-07-25 | [华为云](incidents/identity-access/2026-07-25-huaweicloud-intl-iam.md) | SEV-3 | 未披露 | `dependency-failure` | 华为云国际站部分账号异常：控制台登录/资源冻结报告持续约 5 小时，时间上与 IAM 计划升级窗口高度重合 |
| 2023-11-12 | [阿里云](incidents/identity-access/2023-11-12-aliyun-ak-outage.md) | SEV-1 | 2h30m | `software-bug` | 阿里云全球性服务异常（AK 服务白名单读取异常叠加异常处理缺陷，全线产品管控中断） |
| 2021-03-15 | [Microsoft Azure](incidents/identity-access/2021-03-15-azure-ad-outage.md) | SEV-1 | 3h | `change-management` | Azure AD 全球认证中断（认证系统变更致 Microsoft 365 全家桶认证失败约 3 小时，Teams/Outlook/Xbox 等全部中招） |
| 2020-12-14 | [Google](incidents/identity-access/2020-12-14-google-auth-quota-outage.md) | SEV-1 | 47m | `change-management` | Google 全球认证服务中断（配额系统迁移遗留问题致 User ID 服务配额归零，OAuth/登录全球不可用 47 分钟） |

### 商务运营 `business-operations`（7）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-05-19 | [Railway](incidents/business-operations/2026-05-19-railway-gcp-suspension.md) | SEV-2 | 7h54m | `commercial` | Google Cloud 以违反 ToS 为由无预警暂停 Railway 生产账号，平台全站中断约 8 小时 |
| 2023-04-18 | [Reddit](incidents/business-operations/2023-04-18-reddit-api-pricing.md) | SEV-2 | 未披露 | `commercial` | Reddit API 收费引爆第三方生态崩溃（定价决策未协商 + 沟通对抗，Apollo 等客户端集体关闭、社区大罢工） |
| 2023-01-19 | [Twitter (X)](incidents/business-operations/2023-01-19-twitter-api-crackdown.md) | SEV-2 | 未披露 | `commercial` | Twitter 无预警封杀第三方客户端（断流一周后才改协议，Tweetbot/Twitterrific 等 16 年老应用集体死亡） |
| 2022-07-25 | [Google Cloud](incidents/business-operations/2022-07-25-google-cloud-billing-outage.md) | SEV-2 | 225h05m | `change-management` | Google Cloud 账单系统故障（GCE 变更引发用量数据错乱，9 天延迟报告 + 重复计费显示，所有客户账务可见性受损） |
| 2021-01-10 | [Parler](incidents/business-operations/2021-01-10-parler-aws-termination.md) | SEV-1 | 未披露 | `commercial` | Parler 被 AWS 终止服务全面下线（商务合同终止 = 数字生存权终结，全平台失联约 36 天） |
| 2019-07-25 | [GitHub](incidents/business-operations/2019-07-25-github-sanctions-restriction.md) | SEV-2 | 未披露 | `commercial` | GitHub 按美国制裁要求封禁伊朗等国家开发者（合规误伤 + 公共仓库豁免引发争议，两年后 OFAC 发放许可证解除） |
| 2017-07-25 | [Marketo](incidents/business-operations/2017-07-25-marketo-domain-expiry.md) | SEV-1 | 未披露 | `operational-safeguard` | Marketo 全球服务中断（marketo.com 域名忘记续费而过期，网站/客户控制台/API 全线下线数小时） |

### SA 架构与客情 `sa-architecture`（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-09-18 | [Clerk](incidents/sa-architecture/2025-09-18-clerk-database-incident.md) | SEV-1 | 62h11m | `architecture-decision` | Clerk 数据库事件（供应商自动升级触发连接池同步回收，惊群效应致间歇性故障 4 天，诊断被监控采样率掩盖） |
| 2025-06-26 | [Clerk](incidents/sa-architecture/2025-06-26-clerk-cloudrun-outage.md) | SEV-1 | 45m | `architecture-decision` | Clerk 全服务中断 45 分钟（单区域架构 + 过度信任 Cloud Run 冗余承诺，认证基础设施整体不可用） |
| 2023-09-12 | [Unity](incidents/sa-architecture/2023-09-12-unity-runtime-fee.md) | SEV-2 | 未披露 | `commercial` | Unity Runtime Fee 收费政策引爆开发者生态危机（按安装收费决策 + 傲慢沟通，CEO 下台、政策最终取消） |

### 公共服务与政企 `public-services`（6）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2023-06-08 | [中国电信广东](incidents/public-services/2023-06-08-china-telecom-guangdong-outage.md) | SEV-1 | 5h | `network-routing` | 广东电信 6·8 全网故障：移动语音/短信中断约 5 小时（2023-06-08），高考期间被内部定级"特别重大网络故障"B 级 |
| 2023-01-11 | [美国联邦航空管理局（FAA）](incidents/public-services/2023-01-11-faa-notam-outage.md) | SEV-1 | 11h50m | `operational-safeguard` | FAA NOTAM 1·11 故障：承包商误删文件致全美航班停飞（2023-01-11），9/11 以来首次全国地面停飞 |
| 2022-01-10 | [粤康码（广东省政务服务数据管理局）](incidents/public-services/2022-01-10-yuekangma-outage.md) | SEV-2 | 2h | `capacity-overload` | 粤康码 1·10 故障：早高峰瞬时流量 140 万次/分钟触发系统保护，全省亮码异常约 2 小时（2022-01-10） |
| 2017-05-12 | [英国国家医疗服务体系（NHS）](incidents/public-services/2017-05-12-nhs-wannacry.md) | SEV-1 | 未披露 | `security-attack` | NHS WannaCry 5·12 攻击：勒索蠕虫瘫痪英格兰 1/3 医疗机构（2017-05-12），未打补丁的"基础 IT 安全"缺失 |
| 2016-08-09 | [澳大利亚统计局（ABS）](incidents/public-services/2016-08-09-abs-census-outage.md) | SEV-1 | 43h | `capacity-overload` | 澳洲人口普查 8·9 故障：普查之夜网站崩溃离线 40+ 小时（2016-08-09），DDoS 防护误触发叠加测试盲区，额外成本 3000 万美元 |
| 2012-12-26 | [中国铁路 12306](incidents/public-services/2012-12-26-china-railway-12306-outage.md) | SEV-2 | 未披露 | `hardware-facility` | 12306 春运购票高峰三天两故障：机房空调故障暂停全网售票（2012-12-26），容量设计不足致全国网络购票瘫痪（后续数年春运反复崩溃） |

## 按根因分类

### 变更管理 `change-management`（23）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-09-30 | [Microsoft Azure](incidents/cloud-infrastructure/2026-09-30-azure-gateway-multiregion.md) | SEV-1 | 5h45m | `change-management` | Azure 平台触发的网关控制面更新在 OS 维护窗口恢复，请求量压垮区域网关管理服务，30 区域网关/混合连接服务中断 5 小时 45 分 |
| 2026-09-01 | [Google Cloud](incidents/cloud-infrastructure/2026-09-01-gcp-uscentral1b-network.md) | SEV-2 | 4h08m | `change-management` | Google Cloud us-central1-b 例行网络 fabric 维护触发单集群异常，15 个产品降级 4 小时 8 分 |
| 2026-08-20 | [Google Cloud](incidents/cloud-infrastructure/2026-08-20-gcp-uswest1-fiber.md) | SEV-3 | 2h22m | `change-management` | Google Cloud us-west1 光纤维护后拥塞，计算/容器/存储/IAM 多服务降级 2 小时 22 分 |
| 2026-02-27 | [Google Cloud](incidents/ai-ml-services/2026-02-27-gcp-vertex-safety-filter.md) | SEV-3 | 1h58m | `change-management` | Google Cloud Vertex AI 安全过滤服务配置变更致 Gemini 模型 429/503 错误 1 小时 58 分 |
| 2026-02-02 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-02-azure-storage-policy-vm-outage.md) | SEV-2 | 10h19m | `change-management` | Azure 存储策略修复误删扩展包匿名读权限，全球 VM/VMSS/AKS 生命周期操作中断，恢复洪峰再击穿托管身份服务 |
| 2025-12-05 | [Cloudflare](incidents/cdn-edge/2025-12-05-cloudflare-react2shell-outage.md) | SEV-1 | 25m | `change-management` | Cloudflare 安全补丁引发中断（React2Shell CVE 缓解措施致 28% 全球流量 500 错误约 25 分钟，FL1 代理规则引擎 nil 崩溃） |
| 2025-01-08 | [Microsoft Azure](incidents/cloud-infrastructure/2025-01-08-azure-eastus2-outage.md) | SEV-1 | 50h | `change-management` | Azure East US 2 长达 50 小时的网络中断（区域网络配置变更致存储分区不可用，虚拟机/App Service/数据库等大量服务连续 3 天受损） |
| 2024-12-11 | [OpenAI](incidents/ai-ml-services/2024-12-11-openai-k8s.md) | SEV-1 | 4h15m | `change-management` | OpenAI 全平台中断（新遥测服务压垮 Kubernetes 控制面，DNS 缓存延迟暴露） |
| 2024-04-08 | [腾讯云](incidents/cloud-infrastructure/2024-04-08-tencentcloud-api-outage.md) | SEV-1 | 1h27m | `change-management` | 腾讯云 4·8 云 API 服务异常（新版本前向兼容性问题叠加灰度不足，控制台登录与依赖服务中断 87 分钟） |
| 2023-10-23 | [语雀（蚂蚁集团）](incidents/saas-platforms/2023-10-23-yuque-storage-outage.md) | SEV-1 | 7h50m | `change-management` | 语雀 10·23 长时间宕机（运维升级工具 bug 误下线存储服务器，从备份恢复历时 8 小时） |
| 2023-03-08 | [Datadog](incidents/observability/2023-03-08-datadog-global-outage.md) | SEV-1 | 26h55m | `change-management` | Datadog 全球性宕机（systemd 自动安全更新触发网络失联，五区域全产品受损 27 小时） |
| 2023-01-25 | [Microsoft](incidents/networking-dns/2023-01-25-microsoft-wan-outage.md) | SEV-1 | 5h38m | `change-management` | Microsoft 365 全球中断（WAN 路由器 IP 变更引发全网路由重算） |
| 2022-07-25 | [Google Cloud](incidents/business-operations/2022-07-25-google-cloud-billing-outage.md) | SEV-2 | 225h05m | `change-management` | Google Cloud 账单系统故障（GCE 变更引发用量数据错乱，9 天延迟报告 + 重复计费显示，所有客户账务可见性受损） |
| 2022-07-08 | [Rogers Communications](incidents/networking-dns/2022-07-08-rogers-national-outage.md) | SEV-1 | 19h | `change-management` | Rogers 加拿大全国网络瘫痪（变更删除路由过滤器导致核心网内存耗尽） |
| 2021-03-15 | [Microsoft Azure](incidents/identity-access/2021-03-15-azure-ad-outage.md) | SEV-1 | 3h | `change-management` | Azure AD 全球认证中断（认证系统变更致 Microsoft 365 全家桶认证失败约 3 小时，Teams/Outlook/Xbox 等全部中招） |
| 2020-12-14 | [Google](incidents/identity-access/2020-12-14-google-auth-quota-outage.md) | SEV-1 | 47m | `change-management` | Google 全球认证服务中断（配额系统迁移遗留问题致 User ID 服务配额归零，OAuth/登录全球不可用 47 分钟） |
| 2019-07-02 | [Cloudflare](incidents/cdn-edge/2019-07-02-cloudflare-waf-regex.md) | SEV-1 | 27m | `change-management` | Cloudflare 全球边缘 CPU 耗尽（WAF 正则灾难性回溯） |
| 2019-06-02 | [Google Cloud](incidents/cloud-infrastructure/2019-06-02-gcp-network-congestion.md) | SEV-1 | 4h26m | `change-management` | Google Cloud 多区域网络拥塞（自动化误降网络控制面调度） |
| 2019-05-17 | [Salesforce](incidents/saas-platforms/2019-05-17-salesforce-permission-incident.md) | SEV-1 | 15h | `change-management` | Salesforce 权限脚本事故（数据库脚本误授全量修改权限，紧急全局封锁） |
| 2019-03-13 | [Meta (Facebook)](incidents/saas-platforms/2019-03-13-facebook-global-outage.md) | SEV-1 | 14h | `change-management` | Facebook 全球大规模中断（服务器配置变更致 Facebook/Instagram/WhatsApp 全家族瘫痪约 14 小时，2019 年社交平台最大故障） |
| 2014-11-18 | [Microsoft Azure](incidents/cloud-infrastructure/2014-11-18-azure-storage-outage.md) | SEV-1 | 11h | `change-management` | Azure 存储服务全球中断（性能升级操作中的人为错误致存储集群大面积不可用约 11 小时） |
| 2013-08-16 | [Google](incidents/cloud-infrastructure/2013-08-16-google-all-services-outage.md) | SEV-1 | 5m | `change-management` | Google 全服务 5 分钟中断（批量配置更新工具 bug 同时推送错误配置至全部服务，全球互联网短暂消失） |
| 2011-04-21 | [AWS](incidents/cloud-infrastructure/2011-04-21-aws-ebs-remirroring-storm.md) | SEV-1 | 96h | `change-management` | AWS EBS re-mirroring 风暴（us-east-1 多日故障，云计算史上第一次大考） |

### 配置错误 `config-error`（7）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-07-14 | [Google Cloud](incidents/cloud-infrastructure/2026-07-14-gcp-vmware-engine-routes.md) | SEV-2 | 10h40m | `config-error` | Google Cloud VMware Engine 网络控制面配置更新丢弃站点间路由， stretched 集群互联中断 10 小时 40 分 |
| 2026-03-09 | [Microsoft Azure](incidents/ai-ml-services/2026-03-09-azure-openai-gpt52-config.md) | SEV-2 | 20h12m | `config-error` | Azure OpenAI GPT-5.2 配置与已部署引擎版本不兼容，模型调用跨区失败持续 20 小时 12 分 |
| 2026-01-22 | [Cloudflare](incidents/networking-dns/2026-01-22-cloudflare-miami-route-leak.md) | SEV-3 | 未披露 | `config-error` | Cloudflare 迈阿密路由器自动路由策略配置错误致 BGP 前缀意外泄漏 |
| 2025-10-29 | [Microsoft Azure](incidents/cdn-edge/2025-10-29-azure-frontdoor-outage.md) | SEV-1 | 8h20m | `config-error` | Azure Front Door 全球中断（租户配置变更绕过安全验证部署至全局控制面，AFD 节点失效约 8 小时） |
| 2025-07-14 | [Cloudflare](incidents/networking-dns/2025-07-14-cloudflare-1111-outage.md) | SEV-1 | 1h02m | `config-error` | Cloudflare 1.1.1.1 DNS 解析器全球中断（遗留配置误包含 1.1.1.1 前缀至预发布服务拓扑，BGP 撤消约 62 分钟） |
| 2022-06-21 | [Cloudflare](incidents/cdn-edge/2022-06-21-cloudflare-outage.md) | SEV-1 | 1h15m | `config-error` | Cloudflare 全球中断（BGP 前缀通告策略变更中 term 重排序致 19 个数据中心离线约 75 分钟，50% 全球流量受损） |
| 2020-07-17 | [Cloudflare](incidents/networking-dns/2020-07-17-cloudflare-backbone-outage.md) | SEV-1 | 27m | `config-error` | Cloudflare 骨干网中断（骨干网络配置错误导致 27 分钟全球中断，Discord/Shopify 等连带受影响） |

### 软件缺陷 `software-bug`（27）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-09-29 | [Microsoft Azure](incidents/ai-ml-services/2026-09-29-azure-swedencentral-ai.md) | SEV-2 | 5h55m | `software-bug` | Azure Sweden Central 平台问题致 OpenAI/Foundry/AI 服务间歇失败 5 小时 55 分（PIR 7QL5-Z50） |
| 2026-07-23 | [Microsoft Azure](incidents/cloud-infrastructure/2026-07-23-azure-westus-route-removal.md) | SEV-1 | 4h57m | `software-bug` | Azure West US 区域进出流量中断（维护请求转换软件 bug 移除过多 IP 路由，约 5 小时 19 个下游服务级联） |
| 2026-07-16 | [AWS](incidents/cdn-edge/2026-07-16-aws-cloudfront-vpc-origins-outage.md) | SEV-1 | 3h33m | `software-bug` | AWS CloudFront 全球中断（VPC Origins 连接管理 fleet 内部约束致配置分发失败，全球 5xx 约 3.5 小时） |
| 2026-06-10 | [Google](incidents/ai-ml-services/2026-06-10-google-gemini-outage.md) | SEV-1 | 7h05m | `software-bug` | Google Gemini 全球服务中断（工具部署元数据数据库索引热点致极端读争用，约 7 小时错误率升高） |
| 2026-04-24 | [Microsoft Azure](incidents/cloud-infrastructure/2026-04-24-azure-eastus-pubsub-lock.md) | SEV-2 | 11h52m | `software-bug` | Azure East US PubSub 网络控制面锁竞争致资源配给失败 11 小时 52 分 |
| 2026-02-20 | [Cloudflare](incidents/networking-dns/2026-02-20-cloudflare-byoip-outage.md) | SEV-1 | 6h07m | `software-bug` | Cloudflare BYOIP 全球前缀撤回故障（清理子任务 API 参数 bug 致 25% BYOIP 前缀被 BGP 撤回约 6 小时） |
| 2026-01-08 | [Cloudflare](incidents/networking-dns/2026-01-08-cloudflare-cname-ordering.md) | SEV-2 | 1h55m | `software-bug` | Cloudflare 1.1.1.1 内存优化变更改变 CNAME 记录顺序，部分 DNS 客户端解析失败约 2 小时 |
| 2025-11-18 | [Cloudflare](incidents/cdn-edge/2025-11-18-cloudflare-bot-outage.md) | SEV-1 | 5h46m | `software-bug` | Cloudflare 全球 5xx 中断（Bot Management 配置生成 bug 触发性文件超限引发 Rust panic，约 28% 核心流量受损 5.8 小时） |
| 2025-10-20 | [AWS](incidents/cloud-infrastructure/2025-10-20-aws-dynamodb-dns-cascade.md) | SEV-1 | 14h30m | `software-bug` | AWS us-east-1 大规模级联故障（DynamoDB DNS Enactor 竞态删除端点记录） |
| 2025-08-05 | [Anthropic](incidents/ai-ml-services/2025-08-05-anthropic-quality-degradation.md) | SEV-3 | 未披露 | `software-bug` | Claude 模型质量劣化事件（三个独立基础设施 bug 叠加，约五周输出质量下降） |
| 2025-06-12 | [Google Cloud](incidents/cloud-infrastructure/2025-06-12-gcp-service-control-outage.md) | SEV-1 | 10h | `software-bug` | Google Cloud 全球中断（Service Control 空指针崩溃循环） |
| 2024-07-19 | [CrowdStrike](incidents/security-services/2024-07-19-crowdstrike-falcon.md) | SEV-1 | 72h | `software-bug` | CrowdStrike Falcon 内容更新缺陷致全球约 850 万台 Windows 蓝屏 |
| 2024-03-05 | [Meta (Facebook)](incidents/saas-platforms/2024-03-05-meta-outage.md) | SEV-1 | 2h30m | `software-bug` | Meta 全球大规模中断（Facebook/Instagram/Messenger/Threads 登录与信息流故障约 2 小时，2024 年 Meta 首次全球级故障） |
| 2023-11-12 | [阿里云](incidents/identity-access/2023-11-12-aliyun-ak-outage.md) | SEV-1 | 2h30m | `software-bug` | 阿里云全球性服务异常（AK 服务白名单读取异常叠加异常处理缺陷，全线产品管控中断） |
| 2023-06-13 | [AWS](incidents/cloud-infrastructure/2023-06-13-aws-lambda-outage.md) | SEV-1 | 3h48m | `software-bug` | AWS us-east-1 Lambda 大规模中断（单 cell 容量阈值触发潜在缺陷，STS/Console/EKS 等级联受损近 4 小时） |
| 2023-03-20 | [OpenAI](incidents/ai-ml-services/2023-03-20-openai-redis-leak.md) | SEV-2 | 9h | `software-bug` | ChatGPT 数据泄露事故（redis-py 竞态导致会话标题与部分支付信息串号，主动全站下线） |
| 2023-01-24 | [Cloudflare](incidents/cdn-edge/2023-01-24-cloudflare-zerotrust-outage.md) | SEV-1 | 2h01m | `software-bug` | Cloudflare Zero Trust 中断（Service Token 代码发布覆盖元数据致 121 分钟多服务不可用，WARP/Cache/R2 等受影响） |
| 2021-10-28 | [Roblox](incidents/container-orchestration/2021-10-28-roblox-consul.md) | SEV-1 | 73h | `software-bug` | Roblox 73 小时全站中断（Consul 流式特性与 BoltDB 性能病理叠加） |
| 2021-07-22 | [Akamai](incidents/networking-dns/2021-07-22-akamai-dns-outage.md) | SEV-1 | 1h | `software-bug` | Akamai Edge DNS 全球中断（DNS 软件缺陷致 Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时） |
| 2021-07-13 | [哔哩哔哩](incidents/cdn-edge/2021-07-13-bilibili-slb-outage.md) | SEV-1 | 3h | `software-bug` | B 站 713 事故（SLB Lua 代码 weight="0" 触发死循环，CPU 100% 全站不可用） |
| 2021-06-08 | [Fastly](incidents/cdn-edge/2021-06-08-fastly-global.md) | SEV-1 | 49m | `software-bug` | Fastly 全球服务中断（客户配置触发潜伏软件缺陷） |
| 2019-03-03 | [阿里云](incidents/cloud-infrastructure/2019-03-03-aliyun-io-hang.md) | SEV-1 | 3h | `software-bug` | 阿里云华北 2 可用区 C IO HANG 故障（ECS 服务器磁盘 IO 不响应约 3 小时，内核缺陷导致底层存储链路异常） |
| 2012-10-22 | [AWS](incidents/cloud-infrastructure/2012-10-22-aws-us-east-1-ebs-outage.md) | SEV-1 | 未披露 | `software-bug` | AWS us-east-1 EBS 大规模中断（运维数据采集代理 latent bug 导致 EBS 存储服务器大面积故障） |
| 2012-02-29 | [Microsoft Azure](incidents/cloud-infrastructure/2012-02-29-azure-leap-year.md) | SEV-1 | 12h | `software-bug` | Azure 闰年证书故障（SSL 证书生成逻辑未处理 2 月 29 日，云计算服务瘫痪超 12 小时） |
| 2008-08-15 | [Google](incidents/saas-platforms/2008-08-15-gmail-outage.md) | SEV-1 | 24h | `software-bug` | Gmail 24 小时大中断（存储系统软件 bug 致用户邮箱锁死，Google 应用服务首次重大信任危机） |
| 1990-01-15 | [AT&T](incidents/networking-dns/1990-01-15-att-network-collapse.md) | SEV-1 | 9h | `software-bug` | AT&T 长途网络崩溃（交换机恢复逻辑缺陷引发级联重启） |
| 1980-10-27 | [ARPANET (DARPA/BBN)](incidents/networking-dns/1980-10-27-arpanet-collapse.md) | SEV-1 | 4h | `software-bug` | ARPANET 全网崩溃（状态消息污染与垃圾回收算法缺陷） |

### 容量与过载 `capacity-overload`（7）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-05-29 | [Microsoft Azure](incidents/ai-ml-services/2026-05-29-azure-openai-retry-storm.md) | SEV-2 | 7h26m | `capacity-overload` | Azure OpenAI 上游内部工作负载重试风暴压垮路由层，服务降级 7 小时 26 分 |
| 2022-01-10 | [粤康码（广东省政务服务数据管理局）](incidents/public-services/2022-01-10-yuekangma-outage.md) | SEV-2 | 2h | `capacity-overload` | 粤康码 1·10 故障：早高峰瞬时流量 140 万次/分钟触发系统保护，全省亮码异常约 2 小时（2022-01-10） |
| 2021-12-07 | [AWS](incidents/cloud-infrastructure/2021-12-07-aws-us-east-1.md) | SEV-1 | 7h05m | `capacity-overload` | AWS us-east-1 内部网络过载引发多服务中断 |
| 2021-01-04 | [Slack](incidents/saas-platforms/2021-01-04-slack-new-year-outage.md) | SEV-1 | 4h | `capacity-overload` | Slack 新年首个工作日全球宕机（流量回升叠加 AWS Transit Gateway 扩容滞后） |
| 2020-11-25 | [AWS](incidents/messaging-streaming/2020-11-25-aws-kinesis-thread-limit.md) | SEV-1 | 17h | `capacity-overload` | AWS Kinesis 线程上限故障（前端 fleet 扩容触发 OS 线程数超限，us-east-1 级联 17 小时） |
| 2016-08-09 | [澳大利亚统计局（ABS）](incidents/public-services/2016-08-09-abs-census-outage.md) | SEV-1 | 43h | `capacity-overload` | 澳洲人口普查 8·9 故障：普查之夜网站崩溃离线 40+ 小时（2016-08-09），DDoS 防护误触发叠加测试盲区，额外成本 3000 万美元 |
| 2008-02-15 | [AWS](incidents/cloud-infrastructure/2008-02-15-aws-s3-first-outage.md) | SEV-1 | 2h | `capacity-overload` | AWS S3 首次重大中断（认证请求洪峰致单数据中心不可用约 2 小时，云计算"第一次大考"） |

### 操作防护缺失 `operational-safeguard`（10）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-03-21 | [Cloudflare](incidents/cdn-edge/2025-03-21-cloudflare-r2-outage.md) | SEV-1 | 1h07m | `operational-safeguard` | Cloudflare R2 全球中断（密钥轮换中 `--env production` 参数遗漏致 100% 写入失败 67 分钟，R2/Cache Reserve/Stream 等受损） |
| 2024-05-02 | [Google Cloud](incidents/cloud-infrastructure/2024-05-02-gcp-unisuper-deletion.md) | SEV-1 | 336h | `operational-safeguard` | Google Cloud 误删 UniSuper 私有云事故（GCVE 部署工具参数缺省触发一年期自动删除，1350 亿澳元养老基金停摆一周） |
| 2023-01-11 | [美国联邦航空管理局（FAA）](incidents/public-services/2023-01-11-faa-notam-outage.md) | SEV-1 | 11h50m | `operational-safeguard` | FAA NOTAM 1·11 故障：承包商误删文件致全美航班停飞（2023-01-11），9/11 以来首次全国地面停飞 |
| 2022-04-05 | [Atlassian](incidents/saas-platforms/2022-04-05-atlassian-site-deletion.md) | SEV-2 | 336h | `operational-safeguard` | Atlassian 站点误删事故（清理脚本用错 ID 永久删除 883 个客户站点，恢复历时 14 天） |
| 2017-07-25 | [Marketo](incidents/business-operations/2017-07-25-marketo-domain-expiry.md) | SEV-1 | 未披露 | `operational-safeguard` | Marketo 全球服务中断（marketo.com 域名忘记续费而过期，网站/客户控制台/API 全线下线数小时） |
| 2017-02-28 | [AWS](incidents/cloud-infrastructure/2017-02-28-aws-s3-us-east-1.md) | SEV-1 | 4h17m | `operational-safeguard` | AWS S3 us-east-1 大规模服务中断（运维命令输入错误） |
| 2015-05-28 | [携程](incidents/saas-platforms/2015-05-28-ctrip-code-deletion.md) | SEV-1 | 12h30m | `operational-safeguard` | 携程 5·28 全站瘫痪（运维误删生产服务器执行代码，12 小时全业务中断） |
| 2013-02-22 | [Microsoft Azure](incidents/cloud-infrastructure/2013-02-22-azure-ssl-expiry.md) | SEV-1 | 12h | `operational-safeguard` | Azure 全球存储中断（HTTPS 证书过期） |
| 2012-12-24 | [AWS](incidents/cloud-infrastructure/2012-12-24-aws-elb-state-deletion.md) | SEV-1 | 23h | `operational-safeguard` | AWS ELB 平安夜故障（维护进程误删生产负载均衡器状态数据） |
| 2010-02-24 | [Google](incidents/cloud-infrastructure/2010-02-24-google-appengine-outage.md) | SEV-1 | 2h | `operational-safeguard` | Google App Engine 全平台中断（数据中心维护操作失误致 App Engine 控制面不可用约 2 小时） |

### 网络与路由 `network-routing`（9）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-07-24 | [AWS](incidents/cloud-infrastructure/2026-07-24-aws-uswest2-network-outage.md) | SEV-1 | 1h17m | `network-routing` | AWS us-west-2 区域网络中断（区域至 Seattle Metro 网络硬件故障，主影响 20 分钟但下游恢复尾达 10 小时） |
| 2023-06-08 | [中国电信广东](incidents/public-services/2023-06-08-china-telecom-guangdong-outage.md) | SEV-1 | 5h | `network-routing` | 广东电信 6·8 全网故障：移动语音/短信中断约 5 小时（2023-06-08），高考期间被内部定级"特别重大网络故障"B 级 |
| 2021-10-04 | [Meta (Facebook)](incidents/networking-dns/2021-10-04-facebook-bgp.md) | SEV-1 | 6h05m | `network-routing` | Facebook 全球中断（骨干网维护命令撤销全部 BGP 路由） |
| 2020-08-30 | [CenturyLink (Lumen)](incidents/networking-dns/2020-08-30-centurylink-flowspec-outage.md) | SEV-1 | 5h06m | `network-routing` | CenturyLink/Level 3 全球骨干故障（错误 Flowspec 规则阻断 BGP） |
| 2018-10-21 | [GitHub](incidents/database-storage/2018-10-21-github-mysql.md) | SEV-2 | 24h17m | `network-routing` | GitHub 43 秒网络分区引发 MySQL 跨洋脑裂，降级运行 24 小时 |
| 2015-09-20 | [AWS](incidents/cloud-infrastructure/2015-09-20-aws-dynamodb-outage.md) | SEV-1 | 未披露 | `network-routing` | AWS DynamoDB us-east-1 中断（DNS 解析故障致 DynamoDB 不可用，级联影响多个 AWS 服务） |
| 2014-01-24 | [Google](incidents/saas-platforms/2014-01-24-gmail-dual-network-outage.md) | SEV-1 | 1h11m | `network-routing` | Gmail 71 分钟中断（罕见的双重网络故障致 Gmail 双数据中心同时失效，冗余设计失效） |
| 2008-02-24 | [YouTube (Google)](incidents/networking-dns/2008-02-24-youtube-bgp-hijack.md) | SEV-1 | 2h | `network-routing` | YouTube 全球 BGP 劫持（巴基斯坦电信黑洞路由泄漏） |
| 1997-04-25 | [MAI Network Services](incidents/networking-dns/1997-04-25-as7007-route-leak.md) | SEV-1 | 3h | `network-routing` | AS7007 路由泄漏事件（全表解聚合宣告导致互联网大面积瘫痪） |

### 依赖故障 `dependency-failure`（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-09-03 | [Microsoft Azure](incidents/ai-ml-services/2026-09-03-azure-eastus-ai-outage.md) | SEV-2 | 1h30m | `dependency-failure` | Azure East US 基础设施故障 90 分钟，ChatGPT/Claude/Grok/Copilot 同窗口集体不可用 |
| 2026-07-25 | [华为云](incidents/identity-access/2026-07-25-huaweicloud-intl-iam.md) | SEV-3 | 未披露 | `dependency-failure` | 华为云国际站部分账号异常：控制台登录/资源冻结报告持续约 5 小时，时间上与 IAM 计划升级窗口高度重合 |
| 2025-04-16 | [Zoom](incidents/saas-platforms/2025-04-16-zoom-domain-outage.md) | SEV-1 | 1h47m | `dependency-failure` | Zoom 全球服务中断（GoDaddy Registry 误封 zoom.us 域名，DNS 解析失败致网站/会议/App 不可用约 2 小时） |

### 安全攻击 `security-attack`（5）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-01-27 | [DeepSeek](incidents/ai-ml-services/2025-01-27-deepseek-malicious-attack.md) | SEV-2 | 未披露 | `security-attack` | DeepSeek 大规模恶意攻击事件（爆红高峰期遭攻击，限制非 +86 注册，服务间歇中断） |
| 2024-07-30 | [Microsoft Azure](incidents/cloud-infrastructure/2024-07-30-azure-ddos-amplification.md) | SEV-1 | 7h58m | `security-attack` | Azure 全球服务中断（DDoS 攻击触发防护机制实施错误反向放大攻击效果，Front Door/CDN 间歇不可用约 8 小时） |
| 2017-05-12 | [英国国家医疗服务体系（NHS）](incidents/public-services/2017-05-12-nhs-wannacry.md) | SEV-1 | 未披露 | `security-attack` | NHS WannaCry 5·12 攻击：勒索蠕虫瘫痪英格兰 1/3 医疗机构（2017-05-12），未打补丁的"基础 IT 安全"缺失 |
| 2016-10-21 | [Dyn](incidents/networking-dns/2016-10-21-dyn-ddos.md) | SEV-1 | 11h | `security-attack` | Dyn 托管 DNS 遭 Mirai 僵尸网络 DDoS，美国东海岸大面积断网 |
| 2015-03-28 | [GitHub](incidents/saas-platforms/2015-03-28-github-ddos-attack.md) | SEV-1 | 未披露 | `security-attack` | GitHub 历史上最大规模 DDoS 攻击（中国境内 Baidu 流量被劫持重定向至 GitHub，持续数日） |

### 硬件与设施 `hardware-facility`（10）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-10-20 | [Google Cloud](incidents/cloud-infrastructure/2026-10-20-gcp-europewest2-power.md) | SEV-1 | 8h | `hardware-facility` | Google Cloud europe-west2（伦敦）供电事件致 b 可用区中断约 8 小时，Revolut/劳埃德/汇丰/英国法院系统连带停摆 |
| 2026-05-29 | [Microsoft Azure](incidents/cloud-infrastructure/2026-05-29-azure-westus2-lightning-power.md) | SEV-1 | 22h06m | `hardware-facility` | 雷暴致 Azure West US 2 多数据中心市电全失，发电机部分失效 + 制冷中断，故障超出 N+1 冗余持续 22 小时 6 分 |
| 2026-05-07 | [AWS](incidents/cloud-infrastructure/2026-05-07-aws-thermal-outage.md) | SEV-1 | 未披露 | `hardware-facility` | AWS us-east-1 热失控故障（单可用区冷却失效致服务器过热断电，EC2/EBS 等 150+ 服务受损） |
| 2026-02-07 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-07-azure-westus-transformer-power.md) | SEV-2 | 20h26m | `hardware-facility` | Azure West US 现场变压器电气故障致数据中心市电全失，间歇性服务不可用 20 小时 26 分 |
| 2024-09-10 | [阿里云](incidents/cloud-infrastructure/2024-09-10-aliyun-singapore-fire.md) | SEV-1 | 30h | `hardware-facility` | 阿里云新加坡数据中心火灾（锂电池爆炸导致可用区 C 长时间停电，部分服务中断超 30 小时，多日恢复） |
| 2023-03-29 | [腾讯](incidents/saas-platforms/2023-03-29-tencent-wechat-outage.md) | SEV-1 | 14h | `hardware-facility` | 微信/QQ 大面积功能异常（广州电信机房冷却系统故障致 14 小时多项服务受损，腾讯定性为一级事故并问责高管） |
| 2022-12-18 | [阿里云](incidents/cloud-infrastructure/2022-12-18-aliyun-hongkong-outage.md) | SEV-1 | 15h34m | `hardware-facility` | 阿里云香港可用区 C 大规模服务中断（冷机水路气阻+群控死锁，运营十余年最长大故障） |
| 2018-03-02 | [AWS](incidents/cloud-infrastructure/2018-03-02-aws-equinix-power-outage.md) | SEV-1 | 2h | `hardware-facility` | AWS us-east-1 第三方机房电源故障（Equinix 断电致 Alexa/Atlassian/Slack/Twilio 等大面积服务中断约 2 小时） |
| 2015-05-27 | [支付宝（蚂蚁金服）](incidents/networking-dns/2015-05-27-alipay-fiber-cut.md) | SEV-2 | 2h30m | `hardware-facility` | 支付宝杭州光缆挖断故障（异地多活的成人礼） |
| 2012-12-26 | [中国铁路 12306](incidents/public-services/2012-12-26-china-railway-12306-outage.md) | SEV-2 | 未披露 | `hardware-facility` | 12306 春运购票高峰三天两故障：机房空调故障暂停全网售票（2012-12-26），容量设计不足致全国网络购票瘫痪（后续数年春运反复崩溃） |

### 数据完整性 `data-integrity`（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2018-07-20 | [腾讯云](incidents/database-storage/2018-07-20-tencentcloud-data-loss.md) | SEV-2 | 未披露 | `data-integrity` | 腾讯云"前沿数控"数据丢失事件（磁盘静默错误叠加违规迁移操作） |
| 2017-01-31 | [GitLab](incidents/database-storage/2017-01-31-gitlab-data-loss.md) | SEV-1 | 18h | `data-integrity` | GitLab.com 生产数据库目录被误删，五重备份机制全部失效 |
| 2007-09-29 | [AWS](incidents/cloud-infrastructure/2007-09-29-aws-ec2-first-outage.md) | SEV-1 | 未披露 | `data-integrity` | AWS EC2 首次重大故障（beta 期实例数据丢失，云计算史上第一次信任危机） |

### 商务与合同 `commercial`（6）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-05-19 | [Railway](incidents/business-operations/2026-05-19-railway-gcp-suspension.md) | SEV-2 | 7h54m | `commercial` | Google Cloud 以违反 ToS 为由无预警暂停 Railway 生产账号，平台全站中断约 8 小时 |
| 2023-09-12 | [Unity](incidents/sa-architecture/2023-09-12-unity-runtime-fee.md) | SEV-2 | 未披露 | `commercial` | Unity Runtime Fee 收费政策引爆开发者生态危机（按安装收费决策 + 傲慢沟通，CEO 下台、政策最终取消） |
| 2023-04-18 | [Reddit](incidents/business-operations/2023-04-18-reddit-api-pricing.md) | SEV-2 | 未披露 | `commercial` | Reddit API 收费引爆第三方生态崩溃（定价决策未协商 + 沟通对抗，Apollo 等客户端集体关闭、社区大罢工） |
| 2023-01-19 | [Twitter (X)](incidents/business-operations/2023-01-19-twitter-api-crackdown.md) | SEV-2 | 未披露 | `commercial` | Twitter 无预警封杀第三方客户端（断流一周后才改协议，Tweetbot/Twitterrific 等 16 年老应用集体死亡） |
| 2021-01-10 | [Parler](incidents/business-operations/2021-01-10-parler-aws-termination.md) | SEV-1 | 未披露 | `commercial` | Parler 被 AWS 终止服务全面下线（商务合同终止 = 数字生存权终结，全平台失联约 36 天） |
| 2019-07-25 | [GitHub](incidents/business-operations/2019-07-25-github-sanctions-restriction.md) | SEV-2 | 未披露 | `commercial` | GitHub 按美国制裁要求封禁伊朗等国家开发者（合规误伤 + 公共仓库豁免引发争议，两年后 OFAC 发放许可证解除） |

### 架构决策 `architecture-decision`（2）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-09-18 | [Clerk](incidents/sa-architecture/2025-09-18-clerk-database-incident.md) | SEV-1 | 62h11m | `architecture-decision` | Clerk 数据库事件（供应商自动升级触发连接池同步回收，惊群效应致间歇性故障 4 天，诊断被监控采样率掩盖） |
| 2025-06-26 | [Clerk](incidents/sa-architecture/2025-06-26-clerk-cloudrun-outage.md) | SEV-1 | 45m | `architecture-decision` | Clerk 全服务中断 45 分钟（单区域架构 + 过度信任 Cloud Run 冗余承诺，认证基础设施整体不可用） |

## 按年份

### 2026（24）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2026-10-20 | [Google Cloud](incidents/cloud-infrastructure/2026-10-20-gcp-europewest2-power.md) | SEV-1 | 8h | `hardware-facility` | Google Cloud europe-west2（伦敦）供电事件致 b 可用区中断约 8 小时，Revolut/劳埃德/汇丰/英国法院系统连带停摆 |
| 2026-09-30 | [Microsoft Azure](incidents/cloud-infrastructure/2026-09-30-azure-gateway-multiregion.md) | SEV-1 | 5h45m | `change-management` | Azure 平台触发的网关控制面更新在 OS 维护窗口恢复，请求量压垮区域网关管理服务，30 区域网关/混合连接服务中断 5 小时 45 分 |
| 2026-09-29 | [Microsoft Azure](incidents/ai-ml-services/2026-09-29-azure-swedencentral-ai.md) | SEV-2 | 5h55m | `software-bug` | Azure Sweden Central 平台问题致 OpenAI/Foundry/AI 服务间歇失败 5 小时 55 分（PIR 7QL5-Z50） |
| 2026-09-03 | [Microsoft Azure](incidents/ai-ml-services/2026-09-03-azure-eastus-ai-outage.md) | SEV-2 | 1h30m | `dependency-failure` | Azure East US 基础设施故障 90 分钟，ChatGPT/Claude/Grok/Copilot 同窗口集体不可用 |
| 2026-09-01 | [Google Cloud](incidents/cloud-infrastructure/2026-09-01-gcp-uscentral1b-network.md) | SEV-2 | 4h08m | `change-management` | Google Cloud us-central1-b 例行网络 fabric 维护触发单集群异常，15 个产品降级 4 小时 8 分 |
| 2026-08-20 | [Google Cloud](incidents/cloud-infrastructure/2026-08-20-gcp-uswest1-fiber.md) | SEV-3 | 2h22m | `change-management` | Google Cloud us-west1 光纤维护后拥塞，计算/容器/存储/IAM 多服务降级 2 小时 22 分 |
| 2026-07-25 | [华为云](incidents/identity-access/2026-07-25-huaweicloud-intl-iam.md) | SEV-3 | 未披露 | `dependency-failure` | 华为云国际站部分账号异常：控制台登录/资源冻结报告持续约 5 小时，时间上与 IAM 计划升级窗口高度重合 |
| 2026-07-24 | [AWS](incidents/cloud-infrastructure/2026-07-24-aws-uswest2-network-outage.md) | SEV-1 | 1h17m | `network-routing` | AWS us-west-2 区域网络中断（区域至 Seattle Metro 网络硬件故障，主影响 20 分钟但下游恢复尾达 10 小时） |
| 2026-07-23 | [Microsoft Azure](incidents/cloud-infrastructure/2026-07-23-azure-westus-route-removal.md) | SEV-1 | 4h57m | `software-bug` | Azure West US 区域进出流量中断（维护请求转换软件 bug 移除过多 IP 路由，约 5 小时 19 个下游服务级联） |
| 2026-07-16 | [AWS](incidents/cdn-edge/2026-07-16-aws-cloudfront-vpc-origins-outage.md) | SEV-1 | 3h33m | `software-bug` | AWS CloudFront 全球中断（VPC Origins 连接管理 fleet 内部约束致配置分发失败，全球 5xx 约 3.5 小时） |
| 2026-07-14 | [Google Cloud](incidents/cloud-infrastructure/2026-07-14-gcp-vmware-engine-routes.md) | SEV-2 | 10h40m | `config-error` | Google Cloud VMware Engine 网络控制面配置更新丢弃站点间路由， stretched 集群互联中断 10 小时 40 分 |
| 2026-06-10 | [Google](incidents/ai-ml-services/2026-06-10-google-gemini-outage.md) | SEV-1 | 7h05m | `software-bug` | Google Gemini 全球服务中断（工具部署元数据数据库索引热点致极端读争用，约 7 小时错误率升高） |
| 2026-05-29 | [Microsoft Azure](incidents/ai-ml-services/2026-05-29-azure-openai-retry-storm.md) | SEV-2 | 7h26m | `capacity-overload` | Azure OpenAI 上游内部工作负载重试风暴压垮路由层，服务降级 7 小时 26 分 |
| 2026-05-29 | [Microsoft Azure](incidents/cloud-infrastructure/2026-05-29-azure-westus2-lightning-power.md) | SEV-1 | 22h06m | `hardware-facility` | 雷暴致 Azure West US 2 多数据中心市电全失，发电机部分失效 + 制冷中断，故障超出 N+1 冗余持续 22 小时 6 分 |
| 2026-05-19 | [Railway](incidents/business-operations/2026-05-19-railway-gcp-suspension.md) | SEV-2 | 7h54m | `commercial` | Google Cloud 以违反 ToS 为由无预警暂停 Railway 生产账号，平台全站中断约 8 小时 |
| 2026-05-07 | [AWS](incidents/cloud-infrastructure/2026-05-07-aws-thermal-outage.md) | SEV-1 | 未披露 | `hardware-facility` | AWS us-east-1 热失控故障（单可用区冷却失效致服务器过热断电，EC2/EBS 等 150+ 服务受损） |
| 2026-04-24 | [Microsoft Azure](incidents/cloud-infrastructure/2026-04-24-azure-eastus-pubsub-lock.md) | SEV-2 | 11h52m | `software-bug` | Azure East US PubSub 网络控制面锁竞争致资源配给失败 11 小时 52 分 |
| 2026-03-09 | [Microsoft Azure](incidents/ai-ml-services/2026-03-09-azure-openai-gpt52-config.md) | SEV-2 | 20h12m | `config-error` | Azure OpenAI GPT-5.2 配置与已部署引擎版本不兼容，模型调用跨区失败持续 20 小时 12 分 |
| 2026-02-27 | [Google Cloud](incidents/ai-ml-services/2026-02-27-gcp-vertex-safety-filter.md) | SEV-3 | 1h58m | `change-management` | Google Cloud Vertex AI 安全过滤服务配置变更致 Gemini 模型 429/503 错误 1 小时 58 分 |
| 2026-02-20 | [Cloudflare](incidents/networking-dns/2026-02-20-cloudflare-byoip-outage.md) | SEV-1 | 6h07m | `software-bug` | Cloudflare BYOIP 全球前缀撤回故障（清理子任务 API 参数 bug 致 25% BYOIP 前缀被 BGP 撤回约 6 小时） |
| 2026-02-07 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-07-azure-westus-transformer-power.md) | SEV-2 | 20h26m | `hardware-facility` | Azure West US 现场变压器电气故障致数据中心市电全失，间歇性服务不可用 20 小时 26 分 |
| 2026-02-02 | [Microsoft Azure](incidents/cloud-infrastructure/2026-02-02-azure-storage-policy-vm-outage.md) | SEV-2 | 10h19m | `change-management` | Azure 存储策略修复误删扩展包匿名读权限，全球 VM/VMSS/AKS 生命周期操作中断，恢复洪峰再击穿托管身份服务 |
| 2026-01-22 | [Cloudflare](incidents/networking-dns/2026-01-22-cloudflare-miami-route-leak.md) | SEV-3 | 未披露 | `config-error` | Cloudflare 迈阿密路由器自动路由策略配置错误致 BGP 前缀意外泄漏 |
| 2026-01-08 | [Cloudflare](incidents/networking-dns/2026-01-08-cloudflare-cname-ordering.md) | SEV-2 | 1h55m | `software-bug` | Cloudflare 1.1.1.1 内存优化变更改变 CNAME 记录顺序，部分 DNS 客户端解析失败约 2 小时 |

### 2025（13）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2025-12-05 | [Cloudflare](incidents/cdn-edge/2025-12-05-cloudflare-react2shell-outage.md) | SEV-1 | 25m | `change-management` | Cloudflare 安全补丁引发中断（React2Shell CVE 缓解措施致 28% 全球流量 500 错误约 25 分钟，FL1 代理规则引擎 nil 崩溃） |
| 2025-11-18 | [Cloudflare](incidents/cdn-edge/2025-11-18-cloudflare-bot-outage.md) | SEV-1 | 5h46m | `software-bug` | Cloudflare 全球 5xx 中断（Bot Management 配置生成 bug 触发性文件超限引发 Rust panic，约 28% 核心流量受损 5.8 小时） |
| 2025-10-29 | [Microsoft Azure](incidents/cdn-edge/2025-10-29-azure-frontdoor-outage.md) | SEV-1 | 8h20m | `config-error` | Azure Front Door 全球中断（租户配置变更绕过安全验证部署至全局控制面，AFD 节点失效约 8 小时） |
| 2025-10-20 | [AWS](incidents/cloud-infrastructure/2025-10-20-aws-dynamodb-dns-cascade.md) | SEV-1 | 14h30m | `software-bug` | AWS us-east-1 大规模级联故障（DynamoDB DNS Enactor 竞态删除端点记录） |
| 2025-09-18 | [Clerk](incidents/sa-architecture/2025-09-18-clerk-database-incident.md) | SEV-1 | 62h11m | `architecture-decision` | Clerk 数据库事件（供应商自动升级触发连接池同步回收，惊群效应致间歇性故障 4 天，诊断被监控采样率掩盖） |
| 2025-08-05 | [Anthropic](incidents/ai-ml-services/2025-08-05-anthropic-quality-degradation.md) | SEV-3 | 未披露 | `software-bug` | Claude 模型质量劣化事件（三个独立基础设施 bug 叠加，约五周输出质量下降） |
| 2025-07-14 | [Cloudflare](incidents/networking-dns/2025-07-14-cloudflare-1111-outage.md) | SEV-1 | 1h02m | `config-error` | Cloudflare 1.1.1.1 DNS 解析器全球中断（遗留配置误包含 1.1.1.1 前缀至预发布服务拓扑，BGP 撤消约 62 分钟） |
| 2025-06-26 | [Clerk](incidents/sa-architecture/2025-06-26-clerk-cloudrun-outage.md) | SEV-1 | 45m | `architecture-decision` | Clerk 全服务中断 45 分钟（单区域架构 + 过度信任 Cloud Run 冗余承诺，认证基础设施整体不可用） |
| 2025-06-12 | [Google Cloud](incidents/cloud-infrastructure/2025-06-12-gcp-service-control-outage.md) | SEV-1 | 10h | `software-bug` | Google Cloud 全球中断（Service Control 空指针崩溃循环） |
| 2025-04-16 | [Zoom](incidents/saas-platforms/2025-04-16-zoom-domain-outage.md) | SEV-1 | 1h47m | `dependency-failure` | Zoom 全球服务中断（GoDaddy Registry 误封 zoom.us 域名，DNS 解析失败致网站/会议/App 不可用约 2 小时） |
| 2025-03-21 | [Cloudflare](incidents/cdn-edge/2025-03-21-cloudflare-r2-outage.md) | SEV-1 | 1h07m | `operational-safeguard` | Cloudflare R2 全球中断（密钥轮换中 `--env production` 参数遗漏致 100% 写入失败 67 分钟，R2/Cache Reserve/Stream 等受损） |
| 2025-01-27 | [DeepSeek](incidents/ai-ml-services/2025-01-27-deepseek-malicious-attack.md) | SEV-2 | 未披露 | `security-attack` | DeepSeek 大规模恶意攻击事件（爆红高峰期遭攻击，限制非 +86 注册，服务间歇中断） |
| 2025-01-08 | [Microsoft Azure](incidents/cloud-infrastructure/2025-01-08-azure-eastus2-outage.md) | SEV-1 | 50h | `change-management` | Azure East US 2 长达 50 小时的网络中断（区域网络配置变更致存储分区不可用，虚拟机/App Service/数据库等大量服务连续 3 天受损） |

### 2024（7）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2024-12-11 | [OpenAI](incidents/ai-ml-services/2024-12-11-openai-k8s.md) | SEV-1 | 4h15m | `change-management` | OpenAI 全平台中断（新遥测服务压垮 Kubernetes 控制面，DNS 缓存延迟暴露） |
| 2024-09-10 | [阿里云](incidents/cloud-infrastructure/2024-09-10-aliyun-singapore-fire.md) | SEV-1 | 30h | `hardware-facility` | 阿里云新加坡数据中心火灾（锂电池爆炸导致可用区 C 长时间停电，部分服务中断超 30 小时，多日恢复） |
| 2024-07-30 | [Microsoft Azure](incidents/cloud-infrastructure/2024-07-30-azure-ddos-amplification.md) | SEV-1 | 7h58m | `security-attack` | Azure 全球服务中断（DDoS 攻击触发防护机制实施错误反向放大攻击效果，Front Door/CDN 间歇不可用约 8 小时） |
| 2024-07-19 | [CrowdStrike](incidents/security-services/2024-07-19-crowdstrike-falcon.md) | SEV-1 | 72h | `software-bug` | CrowdStrike Falcon 内容更新缺陷致全球约 850 万台 Windows 蓝屏 |
| 2024-05-02 | [Google Cloud](incidents/cloud-infrastructure/2024-05-02-gcp-unisuper-deletion.md) | SEV-1 | 336h | `operational-safeguard` | Google Cloud 误删 UniSuper 私有云事故（GCVE 部署工具参数缺省触发一年期自动删除，1350 亿澳元养老基金停摆一周） |
| 2024-04-08 | [腾讯云](incidents/cloud-infrastructure/2024-04-08-tencentcloud-api-outage.md) | SEV-1 | 1h27m | `change-management` | 腾讯云 4·8 云 API 服务异常（新版本前向兼容性问题叠加灰度不足，控制台登录与依赖服务中断 87 分钟） |
| 2024-03-05 | [Meta (Facebook)](incidents/saas-platforms/2024-03-05-meta-outage.md) | SEV-1 | 2h30m | `software-bug` | Meta 全球大规模中断（Facebook/Instagram/Messenger/Threads 登录与信息流故障约 2 小时，2024 年 Meta 首次全球级故障） |

### 2023（13）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2023-11-12 | [阿里云](incidents/identity-access/2023-11-12-aliyun-ak-outage.md) | SEV-1 | 2h30m | `software-bug` | 阿里云全球性服务异常（AK 服务白名单读取异常叠加异常处理缺陷，全线产品管控中断） |
| 2023-10-23 | [语雀（蚂蚁集团）](incidents/saas-platforms/2023-10-23-yuque-storage-outage.md) | SEV-1 | 7h50m | `change-management` | 语雀 10·23 长时间宕机（运维升级工具 bug 误下线存储服务器，从备份恢复历时 8 小时） |
| 2023-09-12 | [Unity](incidents/sa-architecture/2023-09-12-unity-runtime-fee.md) | SEV-2 | 未披露 | `commercial` | Unity Runtime Fee 收费政策引爆开发者生态危机（按安装收费决策 + 傲慢沟通，CEO 下台、政策最终取消） |
| 2023-06-13 | [AWS](incidents/cloud-infrastructure/2023-06-13-aws-lambda-outage.md) | SEV-1 | 3h48m | `software-bug` | AWS us-east-1 Lambda 大规模中断（单 cell 容量阈值触发潜在缺陷，STS/Console/EKS 等级联受损近 4 小时） |
| 2023-06-08 | [中国电信广东](incidents/public-services/2023-06-08-china-telecom-guangdong-outage.md) | SEV-1 | 5h | `network-routing` | 广东电信 6·8 全网故障：移动语音/短信中断约 5 小时（2023-06-08），高考期间被内部定级"特别重大网络故障"B 级 |
| 2023-04-18 | [Reddit](incidents/business-operations/2023-04-18-reddit-api-pricing.md) | SEV-2 | 未披露 | `commercial` | Reddit API 收费引爆第三方生态崩溃（定价决策未协商 + 沟通对抗，Apollo 等客户端集体关闭、社区大罢工） |
| 2023-03-29 | [腾讯](incidents/saas-platforms/2023-03-29-tencent-wechat-outage.md) | SEV-1 | 14h | `hardware-facility` | 微信/QQ 大面积功能异常（广州电信机房冷却系统故障致 14 小时多项服务受损，腾讯定性为一级事故并问责高管） |
| 2023-03-20 | [OpenAI](incidents/ai-ml-services/2023-03-20-openai-redis-leak.md) | SEV-2 | 9h | `software-bug` | ChatGPT 数据泄露事故（redis-py 竞态导致会话标题与部分支付信息串号，主动全站下线） |
| 2023-03-08 | [Datadog](incidents/observability/2023-03-08-datadog-global-outage.md) | SEV-1 | 26h55m | `change-management` | Datadog 全球性宕机（systemd 自动安全更新触发网络失联，五区域全产品受损 27 小时） |
| 2023-01-25 | [Microsoft](incidents/networking-dns/2023-01-25-microsoft-wan-outage.md) | SEV-1 | 5h38m | `change-management` | Microsoft 365 全球中断（WAN 路由器 IP 变更引发全网路由重算） |
| 2023-01-24 | [Cloudflare](incidents/cdn-edge/2023-01-24-cloudflare-zerotrust-outage.md) | SEV-1 | 2h01m | `software-bug` | Cloudflare Zero Trust 中断（Service Token 代码发布覆盖元数据致 121 分钟多服务不可用，WARP/Cache/R2 等受影响） |
| 2023-01-19 | [Twitter (X)](incidents/business-operations/2023-01-19-twitter-api-crackdown.md) | SEV-2 | 未披露 | `commercial` | Twitter 无预警封杀第三方客户端（断流一周后才改协议，Tweetbot/Twitterrific 等 16 年老应用集体死亡） |
| 2023-01-11 | [美国联邦航空管理局（FAA）](incidents/public-services/2023-01-11-faa-notam-outage.md) | SEV-1 | 11h50m | `operational-safeguard` | FAA NOTAM 1·11 故障：承包商误删文件致全美航班停飞（2023-01-11），9/11 以来首次全国地面停飞 |

### 2022（6）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2022-12-18 | [阿里云](incidents/cloud-infrastructure/2022-12-18-aliyun-hongkong-outage.md) | SEV-1 | 15h34m | `hardware-facility` | 阿里云香港可用区 C 大规模服务中断（冷机水路气阻+群控死锁，运营十余年最长大故障） |
| 2022-07-25 | [Google Cloud](incidents/business-operations/2022-07-25-google-cloud-billing-outage.md) | SEV-2 | 225h05m | `change-management` | Google Cloud 账单系统故障（GCE 变更引发用量数据错乱，9 天延迟报告 + 重复计费显示，所有客户账务可见性受损） |
| 2022-07-08 | [Rogers Communications](incidents/networking-dns/2022-07-08-rogers-national-outage.md) | SEV-1 | 19h | `change-management` | Rogers 加拿大全国网络瘫痪（变更删除路由过滤器导致核心网内存耗尽） |
| 2022-06-21 | [Cloudflare](incidents/cdn-edge/2022-06-21-cloudflare-outage.md) | SEV-1 | 1h15m | `config-error` | Cloudflare 全球中断（BGP 前缀通告策略变更中 term 重排序致 19 个数据中心离线约 75 分钟，50% 全球流量受损） |
| 2022-04-05 | [Atlassian](incidents/saas-platforms/2022-04-05-atlassian-site-deletion.md) | SEV-2 | 336h | `operational-safeguard` | Atlassian 站点误删事故（清理脚本用错 ID 永久删除 883 个客户站点，恢复历时 14 天） |
| 2022-01-10 | [粤康码（广东省政务服务数据管理局）](incidents/public-services/2022-01-10-yuekangma-outage.md) | SEV-2 | 2h | `capacity-overload` | 粤康码 1·10 故障：早高峰瞬时流量 140 万次/分钟触发系统保护，全省亮码异常约 2 小时（2022-01-10） |

### 2021（9）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2021-12-07 | [AWS](incidents/cloud-infrastructure/2021-12-07-aws-us-east-1.md) | SEV-1 | 7h05m | `capacity-overload` | AWS us-east-1 内部网络过载引发多服务中断 |
| 2021-10-28 | [Roblox](incidents/container-orchestration/2021-10-28-roblox-consul.md) | SEV-1 | 73h | `software-bug` | Roblox 73 小时全站中断（Consul 流式特性与 BoltDB 性能病理叠加） |
| 2021-10-04 | [Meta (Facebook)](incidents/networking-dns/2021-10-04-facebook-bgp.md) | SEV-1 | 6h05m | `network-routing` | Facebook 全球中断（骨干网维护命令撤销全部 BGP 路由） |
| 2021-07-22 | [Akamai](incidents/networking-dns/2021-07-22-akamai-dns-outage.md) | SEV-1 | 1h | `software-bug` | Akamai Edge DNS 全球中断（DNS 软件缺陷致 Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时） |
| 2021-07-13 | [哔哩哔哩](incidents/cdn-edge/2021-07-13-bilibili-slb-outage.md) | SEV-1 | 3h | `software-bug` | B 站 713 事故（SLB Lua 代码 weight="0" 触发死循环，CPU 100% 全站不可用） |
| 2021-06-08 | [Fastly](incidents/cdn-edge/2021-06-08-fastly-global.md) | SEV-1 | 49m | `software-bug` | Fastly 全球服务中断（客户配置触发潜伏软件缺陷） |
| 2021-03-15 | [Microsoft Azure](incidents/identity-access/2021-03-15-azure-ad-outage.md) | SEV-1 | 3h | `change-management` | Azure AD 全球认证中断（认证系统变更致 Microsoft 365 全家桶认证失败约 3 小时，Teams/Outlook/Xbox 等全部中招） |
| 2021-01-10 | [Parler](incidents/business-operations/2021-01-10-parler-aws-termination.md) | SEV-1 | 未披露 | `commercial` | Parler 被 AWS 终止服务全面下线（商务合同终止 = 数字生存权终结，全平台失联约 36 天） |
| 2021-01-04 | [Slack](incidents/saas-platforms/2021-01-04-slack-new-year-outage.md) | SEV-1 | 4h | `capacity-overload` | Slack 新年首个工作日全球宕机（流量回升叠加 AWS Transit Gateway 扩容滞后） |

### 2020（4）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2020-12-14 | [Google](incidents/identity-access/2020-12-14-google-auth-quota-outage.md) | SEV-1 | 47m | `change-management` | Google 全球认证服务中断（配额系统迁移遗留问题致 User ID 服务配额归零，OAuth/登录全球不可用 47 分钟） |
| 2020-11-25 | [AWS](incidents/messaging-streaming/2020-11-25-aws-kinesis-thread-limit.md) | SEV-1 | 17h | `capacity-overload` | AWS Kinesis 线程上限故障（前端 fleet 扩容触发 OS 线程数超限，us-east-1 级联 17 小时） |
| 2020-08-30 | [CenturyLink (Lumen)](incidents/networking-dns/2020-08-30-centurylink-flowspec-outage.md) | SEV-1 | 5h06m | `network-routing` | CenturyLink/Level 3 全球骨干故障（错误 Flowspec 规则阻断 BGP） |
| 2020-07-17 | [Cloudflare](incidents/networking-dns/2020-07-17-cloudflare-backbone-outage.md) | SEV-1 | 27m | `config-error` | Cloudflare 骨干网中断（骨干网络配置错误导致 27 分钟全球中断，Discord/Shopify 等连带受影响） |

### 2019（6）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2019-07-25 | [GitHub](incidents/business-operations/2019-07-25-github-sanctions-restriction.md) | SEV-2 | 未披露 | `commercial` | GitHub 按美国制裁要求封禁伊朗等国家开发者（合规误伤 + 公共仓库豁免引发争议，两年后 OFAC 发放许可证解除） |
| 2019-07-02 | [Cloudflare](incidents/cdn-edge/2019-07-02-cloudflare-waf-regex.md) | SEV-1 | 27m | `change-management` | Cloudflare 全球边缘 CPU 耗尽（WAF 正则灾难性回溯） |
| 2019-06-02 | [Google Cloud](incidents/cloud-infrastructure/2019-06-02-gcp-network-congestion.md) | SEV-1 | 4h26m | `change-management` | Google Cloud 多区域网络拥塞（自动化误降网络控制面调度） |
| 2019-05-17 | [Salesforce](incidents/saas-platforms/2019-05-17-salesforce-permission-incident.md) | SEV-1 | 15h | `change-management` | Salesforce 权限脚本事故（数据库脚本误授全量修改权限，紧急全局封锁） |
| 2019-03-13 | [Meta (Facebook)](incidents/saas-platforms/2019-03-13-facebook-global-outage.md) | SEV-1 | 14h | `change-management` | Facebook 全球大规模中断（服务器配置变更致 Facebook/Instagram/WhatsApp 全家族瘫痪约 14 小时，2019 年社交平台最大故障） |
| 2019-03-03 | [阿里云](incidents/cloud-infrastructure/2019-03-03-aliyun-io-hang.md) | SEV-1 | 3h | `software-bug` | 阿里云华北 2 可用区 C IO HANG 故障（ECS 服务器磁盘 IO 不响应约 3 小时，内核缺陷导致底层存储链路异常） |

### 2018（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2018-10-21 | [GitHub](incidents/database-storage/2018-10-21-github-mysql.md) | SEV-2 | 24h17m | `network-routing` | GitHub 43 秒网络分区引发 MySQL 跨洋脑裂，降级运行 24 小时 |
| 2018-07-20 | [腾讯云](incidents/database-storage/2018-07-20-tencentcloud-data-loss.md) | SEV-2 | 未披露 | `data-integrity` | 腾讯云"前沿数控"数据丢失事件（磁盘静默错误叠加违规迁移操作） |
| 2018-03-02 | [AWS](incidents/cloud-infrastructure/2018-03-02-aws-equinix-power-outage.md) | SEV-1 | 2h | `hardware-facility` | AWS us-east-1 第三方机房电源故障（Equinix 断电致 Alexa/Atlassian/Slack/Twilio 等大面积服务中断约 2 小时） |

### 2017（4）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2017-07-25 | [Marketo](incidents/business-operations/2017-07-25-marketo-domain-expiry.md) | SEV-1 | 未披露 | `operational-safeguard` | Marketo 全球服务中断（marketo.com 域名忘记续费而过期，网站/客户控制台/API 全线下线数小时） |
| 2017-05-12 | [英国国家医疗服务体系（NHS）](incidents/public-services/2017-05-12-nhs-wannacry.md) | SEV-1 | 未披露 | `security-attack` | NHS WannaCry 5·12 攻击：勒索蠕虫瘫痪英格兰 1/3 医疗机构（2017-05-12），未打补丁的"基础 IT 安全"缺失 |
| 2017-02-28 | [AWS](incidents/cloud-infrastructure/2017-02-28-aws-s3-us-east-1.md) | SEV-1 | 4h17m | `operational-safeguard` | AWS S3 us-east-1 大规模服务中断（运维命令输入错误） |
| 2017-01-31 | [GitLab](incidents/database-storage/2017-01-31-gitlab-data-loss.md) | SEV-1 | 18h | `data-integrity` | GitLab.com 生产数据库目录被误删，五重备份机制全部失效 |

### 2016（2）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2016-10-21 | [Dyn](incidents/networking-dns/2016-10-21-dyn-ddos.md) | SEV-1 | 11h | `security-attack` | Dyn 托管 DNS 遭 Mirai 僵尸网络 DDoS，美国东海岸大面积断网 |
| 2016-08-09 | [澳大利亚统计局（ABS）](incidents/public-services/2016-08-09-abs-census-outage.md) | SEV-1 | 43h | `capacity-overload` | 澳洲人口普查 8·9 故障：普查之夜网站崩溃离线 40+ 小时（2016-08-09），DDoS 防护误触发叠加测试盲区，额外成本 3000 万美元 |

### 2015（4）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2015-09-20 | [AWS](incidents/cloud-infrastructure/2015-09-20-aws-dynamodb-outage.md) | SEV-1 | 未披露 | `network-routing` | AWS DynamoDB us-east-1 中断（DNS 解析故障致 DynamoDB 不可用，级联影响多个 AWS 服务） |
| 2015-05-28 | [携程](incidents/saas-platforms/2015-05-28-ctrip-code-deletion.md) | SEV-1 | 12h30m | `operational-safeguard` | 携程 5·28 全站瘫痪（运维误删生产服务器执行代码，12 小时全业务中断） |
| 2015-05-27 | [支付宝（蚂蚁金服）](incidents/networking-dns/2015-05-27-alipay-fiber-cut.md) | SEV-2 | 2h30m | `hardware-facility` | 支付宝杭州光缆挖断故障（异地多活的成人礼） |
| 2015-03-28 | [GitHub](incidents/saas-platforms/2015-03-28-github-ddos-attack.md) | SEV-1 | 未披露 | `security-attack` | GitHub 历史上最大规模 DDoS 攻击（中国境内 Baidu 流量被劫持重定向至 GitHub，持续数日） |

### 2014（2）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2014-11-18 | [Microsoft Azure](incidents/cloud-infrastructure/2014-11-18-azure-storage-outage.md) | SEV-1 | 11h | `change-management` | Azure 存储服务全球中断（性能升级操作中的人为错误致存储集群大面积不可用约 11 小时） |
| 2014-01-24 | [Google](incidents/saas-platforms/2014-01-24-gmail-dual-network-outage.md) | SEV-1 | 1h11m | `network-routing` | Gmail 71 分钟中断（罕见的双重网络故障致 Gmail 双数据中心同时失效，冗余设计失效） |

### 2013（2）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2013-08-16 | [Google](incidents/cloud-infrastructure/2013-08-16-google-all-services-outage.md) | SEV-1 | 5m | `change-management` | Google 全服务 5 分钟中断（批量配置更新工具 bug 同时推送错误配置至全部服务，全球互联网短暂消失） |
| 2013-02-22 | [Microsoft Azure](incidents/cloud-infrastructure/2013-02-22-azure-ssl-expiry.md) | SEV-1 | 12h | `operational-safeguard` | Azure 全球存储中断（HTTPS 证书过期） |

### 2012（4）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2012-12-26 | [中国铁路 12306](incidents/public-services/2012-12-26-china-railway-12306-outage.md) | SEV-2 | 未披露 | `hardware-facility` | 12306 春运购票高峰三天两故障：机房空调故障暂停全网售票（2012-12-26），容量设计不足致全国网络购票瘫痪（后续数年春运反复崩溃） |
| 2012-12-24 | [AWS](incidents/cloud-infrastructure/2012-12-24-aws-elb-state-deletion.md) | SEV-1 | 23h | `operational-safeguard` | AWS ELB 平安夜故障（维护进程误删生产负载均衡器状态数据） |
| 2012-10-22 | [AWS](incidents/cloud-infrastructure/2012-10-22-aws-us-east-1-ebs-outage.md) | SEV-1 | 未披露 | `software-bug` | AWS us-east-1 EBS 大规模中断（运维数据采集代理 latent bug 导致 EBS 存储服务器大面积故障） |
| 2012-02-29 | [Microsoft Azure](incidents/cloud-infrastructure/2012-02-29-azure-leap-year.md) | SEV-1 | 12h | `software-bug` | Azure 闰年证书故障（SSL 证书生成逻辑未处理 2 月 29 日，云计算服务瘫痪超 12 小时） |

### 2011（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2011-04-21 | [AWS](incidents/cloud-infrastructure/2011-04-21-aws-ebs-remirroring-storm.md) | SEV-1 | 96h | `change-management` | AWS EBS re-mirroring 风暴（us-east-1 多日故障，云计算史上第一次大考） |

### 2010（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2010-02-24 | [Google](incidents/cloud-infrastructure/2010-02-24-google-appengine-outage.md) | SEV-1 | 2h | `operational-safeguard` | Google App Engine 全平台中断（数据中心维护操作失误致 App Engine 控制面不可用约 2 小时） |

### 2008（3）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2008-08-15 | [Google](incidents/saas-platforms/2008-08-15-gmail-outage.md) | SEV-1 | 24h | `software-bug` | Gmail 24 小时大中断（存储系统软件 bug 致用户邮箱锁死，Google 应用服务首次重大信任危机） |
| 2008-02-24 | [YouTube (Google)](incidents/networking-dns/2008-02-24-youtube-bgp-hijack.md) | SEV-1 | 2h | `network-routing` | YouTube 全球 BGP 劫持（巴基斯坦电信黑洞路由泄漏） |
| 2008-02-15 | [AWS](incidents/cloud-infrastructure/2008-02-15-aws-s3-first-outage.md) | SEV-1 | 2h | `capacity-overload` | AWS S3 首次重大中断（认证请求洪峰致单数据中心不可用约 2 小时，云计算"第一次大考"） |

### 2007（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 2007-09-29 | [AWS](incidents/cloud-infrastructure/2007-09-29-aws-ec2-first-outage.md) | SEV-1 | 未披露 | `data-integrity` | AWS EC2 首次重大故障（beta 期实例数据丢失，云计算史上第一次信任危机） |

### 1997（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 1997-04-25 | [MAI Network Services](incidents/networking-dns/1997-04-25-as7007-route-leak.md) | SEV-1 | 3h | `network-routing` | AS7007 路由泄漏事件（全表解聚合宣告导致互联网大面积瘫痪） |

### 1990（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 1990-01-15 | [AT&T](incidents/networking-dns/1990-01-15-att-network-collapse.md) | SEV-1 | 9h | `software-bug` | AT&T 长途网络崩溃（交换机恢复逻辑缺陷引发级联重启） |

### 1980（1）

| 日期 | 公司 | 等级 | 时长 | 根因分类 | 标题 |
|---|---|---|---|---|---|
| 1980-10-27 | [ARPANET (DARPA/BBN)](incidents/networking-dns/1980-10-27-arpanet-collapse.md) | SEV-1 | 4h | `software-bug` | ARPANET 全网崩溃（状态消息污染与垃圾回收算法缺陷） |
