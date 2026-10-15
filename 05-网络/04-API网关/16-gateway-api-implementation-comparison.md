---
title: Gateway API 实现生态对比与选型指南
description: Gateway API GA 核心价值（角色划分/可移植性/GAMMA）、主流实现对比矩阵（Envoy Gateway/kgateway/APISIX/Higress/Istio/云厂商）、Ingress 迁移实操、金丝雀流量切分全例、选型决策表
summary: 以 Gateway API v1.6 为基准对比六大实现的数据面性能、CRD 成熟度、扩展机制、多集群与治理活跃度 — 覆盖 Ingress 双栈迁移实操（ingress2gateway v1.2.0）、金丝雀发布流量切分完整 YAML、故障排查与选型决策表
category: cloud-native-api-gateway
tags:
- k8s
- ingress
- gateway-api
- api-gateway
- envoy
- envoy-gateway
- apisix
- higress
- istio
- kgateway
- service-mesh
- canary
- production
tier: supporting
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: advanced
audience:
- SRE
- 架构师
- 平台工程师
estimated_read_time: 18min
intent_queries:
- Gateway API 实现生态对比与选型指南 是什么
- 如何从 Ingress 迁移到 Gateway API
- Envoy Gateway 与 APISIX Higress kgateway 怎么选
- Gateway API 金丝雀流量切分怎么配置
trigger_keywords:
- Gateway API
- Envoy Gateway
- kgateway
- APISIX
- Higress
- Istio
- GAMMA
- Ingress 迁移
- 金丝雀
- 流量切分
- 双栈
prerequisites:
- kubectl-basics
- networking-basics
- ingress-basics
- helm-basics
- service-mesh-basics
k8s_versions:
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# Gateway API 实现生态对比与选型指南

> **适用版本**: Gateway API 标准 v1.6.3 / Envoy Gateway v1.9.2 / APISIX Ingress Controller 2.2.0 / Higress v2.2.5（2026-10-14 经 GitHub Releases 核对；kgateway 无 latest release 暴露，见 2.2 节说明）
> **前置阅读**: [[05-网络/04-API网关/02-kubernetes-gateway-api-deep-dive.md|Gateway API 标准深度解析]]

---

## 目录

- [一、概述：Gateway API 的核心价值](#一概述gateway-api-的核心价值)
- [二、主流实现对比矩阵](#二主流实现对比矩阵)
- [三、对比维度深度分析](#三对比维度深度分析)
- [四、从 Ingress 迁移到 Gateway API 实操](#四从-ingress-迁移到-gateway-api-实操)
- [五、生产案例：金丝雀发布流量切分全例](#五生产案例金丝雀发布流量切分全例)
- [六、故障排查与诊断流程](#六故障排查与诊断流程)
- [七、最佳实践与反模式](#七最佳实践与反模式)
- [八、性能与监控](#八性能与监控)
- [九、版本兼容性与升级注意](#九版本兼容性与升级注意)
- [十、参考链接与延伸阅读](#十参考链接与延伸阅读)

---

## 一、概述：Gateway API 的核心价值

Gateway API 是 SIG-Network 主导的下一代南北向流量 API（`gateway.networking.k8s.io`），定位是取代资源表达力见底的 Ingress（API 概览见 [[05-网络/04-API网关/02-kubernetes-gateway-api-deep-dive.md|Gateway API 深度解析]]）。相对 Ingress 的三大结构性价值：

### 1.1 角色划分（人机权限对齐）

Gateway API 的三类资源天然对应企业里三类角色，让 RBAC 边界与流量配置边界重合：

| 角色 | 拥有的资源 | 典型职责 |
|------|-----------|----------|
| 基础设施提供方 | `GatewayClass` | 选定实现（如 envoy-gateway），定义共享网关池的基线 |
| 集群/网关运营方 | `Gateway`（listener/TLS/域名） | 证书、入口 IP、端口、跨团队路由准入 |
| 应用开发者 | `HTTPRoute`/`GRPCRoute`/`TCPRoute`… | 路由规则、后端选择、（受限的）流量策略 |

Ingress 时代「一个注解定金丝雀」本质是把三类角色的权限揉进一个资源，注解方言泛滥正是这个结构缺陷的后果。

### 1.2 可移植性

路由语义（匹配优先级、header/path/query、`backendRefs[].weight`、TLS 终止/passthrough、跨 namespace `ReferenceGrant`）全部进入标准 CRD，控制面实现可替换——同一套 HTTPRoute 在 Envoy Gateway、Istio、APISIX 等实现间语义一致（以各自通过的一致性测试配置 conformance profile 为准）。厂商锁定从「重写所有注解」降为「替换 GatewayClass + 少量策略扩展」。

### 1.3 GAMMA（Gateway API for Mesh Management and Administration）

GAMMA 是 Gateway API 工作流中把同一套 Route API 用于**服务间（东西向）流量**的倡议：Route 资源不再只挂到 Gateway listener，也可绑定 Service（`parentRef` 指向 Service，即 "parentRef on Service"），由 mesh 数据面执行。Istio 是 GAMMA 的主要推动与实现者（见 2.6 节）。这意味着南北向与东西向可以共用一套心智模型与工具链——这是选型时「网关是否与 mesh 同源」的重要考量。

### 1.4 版本现状（2026-10）

- HTTPRoute 自 v1.0（2023-10）GA；GRPCRoute 于 v1.1 进入 Standard 通道；BackendTLSPolicy（网关→后端 mTLS，GEP-1897）于 v1.2 进入 Standard；
- 当前标准版 **v1.6.3**（`gh api repos/kubernetes-sigs/gateway-api/releases/latest` 核对）；Experimental 通道持续孵化 SessionPersistence（GEP-1619）、BackendTrafficPolicy（GEP-3547，限流/重试/超时等策略资源）、DirectResponse/HTTPRedirect 等能力；
- 选型时务必区分 Standard 与 Experimental 通道的能力， Experimental 字段随时可能变更（见[九、版本兼容性](#九版本兼容性与升级注意)）。

---

## 二、主流实现对比矩阵

### 2.1 对比总表

| 维度 | Envoy Gateway | kgateway（原 Gloo Gateway） | Apache APISIX IC | Higress | Istio | 云厂商（托管） |
|------|---------------|------------------------------|------------------|---------|-------|----------------|
| 数据面 | Envoy | Envoy | APISIX（Nginx+etcd+Lua/WASM 插件） | Envoy | Envoy（ztunnel/waypoint，ambient 或 sidecar） | 厂商 LB |
| Gateway API 支持广度 | 高（标准带头羊） | 高（双 API：Gateway API + 自有 CRD） | 中高（2.x 以 Gateway API 为一等公民） | 中（持续完善） | 高（官方推荐的入口方式 + GAMMA 主导） | 中（各云覆盖子集） |
| 流量切分/金丝雀 | HTTPRoute 权重原生；BackendTrafficPolicy（实验）加熔断/重试 | 原生 + 上游 CRD 增强（Gloo 血统的 Upstream 抽象） | HTTPRoute 权重原生 + ApisixPluginConfig 扩展 | HTTPRoute 权重原生 + 控制台灰度能力 | 原生权重 + VirtualService/DestinationRule 增强语义 | 厂商专有（ALB 加权目标组等） |
| TLS 终止/mTLS | listener TLS 原生；BackendTLSPolicy 到后端 mTLS | 同左 + 企业级证书管理 | listener TLS 原生；后端 mTLS 依赖插件 | listener TLS 原生 | listener TLS + PeerAuthentication/ DestinationRule 细粒度 mTLS | 托管证书体系 |
| 限流/认证扩展 | SecurityPolicy（JWT/OIDC/ext-auth）、BackendTrafficPolicy 内建限流、EnvoyExtensionPolicy（WASM） | Envoy 原生过滤器 + 企业扩展（GraphQL/转码等） | 插件生态最丰富（limit-req/jwt-auth/openid-connect/消费者体系） | WASM 插件热更新 + Nacos/注册中心集成 | EnvoyFilter/ WasmPlugin/ AuthorizationPolicy 体系 | 厂商 WAF/限流产品联动 |
| 多集群 | 无一等公民支持（可用 BackendTLSPolicy + 外部端点拼装） | 较强（Gloo 系多集群/多环境血统） | 依赖 etcd/控制面多实例方案 | 注册中心（Nacos 等）多集群服务发现场景强 | 最强（east-west gateway/ 多 primary 模型） | 厂商多区域 LB |
| 治理/社区 | CNCF（Envoy 家族），活跃度高 | 开源重铸（原 Solo.io Gloo Gateway），企业版由 Solo 支持；GitHub 未暴露 latest release（见 2.2） | Apache 软件基金会 | 阿里开源、CNCF 沙箱，国内生态活跃 | CNCF 毕业项目 | 厂商 SLA |
| 迁移成本（自 Ingress/注解生态） | 中：ingress2gateway 覆盖 nginx 方言 | 低-中：Gloo 存量直接沿用，注解迁移工具成熟 | 中：ApisixRoute CRD 存量多，注解→Route 需改造 | 中：控制台/CRD 双轨 | 高：若引入 mesh 心智，收益在东西向 | 低（托管）但反向锁定强 |

### 2.2 版本口径说明（2026-10-14 核对）

- Envoy Gateway：**v1.9.2**（`gh api repos/envoyproxy/gateway/releases/latest`）
- Apache APISIX Ingress Controller：**2.2.0**（`gh api repos/apache/apisix-ingress-controller/releases/latest`）
- Higress：**v2.2.5**（`gh api repos/alibaba/higress/releases/latest`）
- kgateway：`gh api repos/kgateway/kgateway/releases/latest` 返回 404——该仓库未以 GitHub Releases `latest` 通道暴露版本，选型评估请以官方文档/变更日志为准，本文不落死版本号；
- Gateway API 标准包：**v1.6.3**；迁移工具 ingress2gateway：**v1.2.0**。

### 2.3 各实现一句话画像

- **Envoy Gateway（EG）**：把「Envoy 官方 K8s 网关」做对做薄——标准 API 覆盖最快、策略 CRD（SecurityPolicy 等）覆盖 80% 企业需求；适合「只要一个高质量南北向网关」的团队。深度实践见 [[05-网络/04-API网关/07-envoy-gateway-enterprise.md|Envoy Gateway 企业实践]]。
- **kgateway**：Envoy 数据面 + Gloo 系多集群/多环境与企业级扩展（认证、转码）的沉淀；存量 Gloo 用户与需要商业支持团队的自然选择。
- **Apache APISIX Ingress Controller**：数据面插件生态最厚（认证/限流/可观测性/服务治理插件上百），etcd 集中配置实时下发；2.x 起以 Gateway API 为一等公民。实践见 [[05-网络/04-API网关/05-apisix-enterprise-gateway.md|APISIX 企业网关]]。
- **Higress**：阿里开源的「云原生网关」，Envoy 数据面 + WASM 插件热更新 + 注册中心/Nacos 深度集成，控制台体验好；国内微服务/多集群服务发现场景占优。见 [[05-网络/04-API网关/04-higress-enterprise-gateway.md|Higress 企业网关]]。
- **Istio**：把 Gateway API 作为官方推荐入口 API，且是 GAMMA 主导者——若已有或将建 mesh，「同源」红利最大（一套 xDS、一套策略、一套可观测性）。参见 [[05-网络/03-服务网格/index.md|服务网格]]。
- **云厂商**：AWS（VPC Lattice 的 Gateway API Controller / ALB Ingress 注解）、GCP（GKE Gateway，对接 Cloud Load Balancing）、Azure（Application Gateway for Containers 的 ALB Controller）；阿里云 ALB 主要经 Ingress 注解对接，Gateway API 生态多走 MSE 云原生网关或自管开源实现。托管省运维但反向锁定最强，且各云只实现 API 子集。

---

## 三、对比维度深度分析

### 3.1 数据面性能特征

- Envoy 系（EG/kgateway/Higress/Istio）：多线程 worker + 内置连接池，长连接/高并发南北向吞吐稳定；差异主要在**控制面翻译效率与配置下发路径**（EG 直接 xDS；APISIX 走 etcd watch）；
- APISIX：Nginx 事件模型 + Lua 插件链，插件链顺序影响尾延迟；重插件场景需压测验证（跨实现基准对比见 [[05-网络/04-API网关/13-api-gateway-performance-benchmarks.md|API 网关性能基准]]）；
- 结论：纯数据面差异通常小于 20%，且不是第一决策因子；**控制面一致性、策略表达力、运维熟悉度**才是长期成本大头。

### 3.2 CRD 成熟度

- 所有对比实现均已支持 v1 HTTPRoute/Gateway/GatewayClass 核心链路；差异在 Standard 通道新资源（BackendTLSPolicy/TLSRoute/GRPCRoute）与 Experimental 通道（BackendTrafficPolicy、SessionPersistence）的跟进速度——EG/Istio 最快，云厂商最慢；
- 验证某实现的真实覆盖度，用官方一致性报告：`kubectl apply` 标准一致性测试套件（见 [[05-网络/04-API网关/02-kubernetes-gateway-api-deep-dive.md|Gateway API 深度解析]] 的一致性测试章节）。

### 3.3 扩展机制（金丝雀/加权/认证/限流）

| 能力 | 标准内做法 | 标准外增强（按实现） |
|------|------------|----------------------|
| 加权/金丝雀 | `backendRefs[].weight`（无平滑概念，权重即时生效） | EG BackendTrafficPolicy（重试/断路）；Istio DestinationRule（渐进式、 locality LB）；APISIX 插件（按消费者/header 细分） |
| 会话保持 | SessionPersistence（GEP-1619，实验） | Istio cookie 一致性哈希（成熟）；APISIX 插件 |
| 认证 | 无（标准不管内容级安全） | EG SecurityPolicy（JWT/OIDC/ext-auth）；APISIX jwt-auth/openid-connect 插件；Istio RequestAuthentication/AuthorizationPolicy |
| 限流 | BackendTrafficPolicy（实验） | APISIX limit-req/limit-count（多维）；EG 内建 + WASM；云厂商产品 |
| 内容改写/重定向 | HTTPRoute filter（RequestHeaderModifier/ ResponseHeaderModifier/ URLRewrite/ RequestRedirect）标准内 | 各家 WASM/Lua/EnvoyFilter 深度改写 |

安全配套（WAF/加密/mTLS 全景）见 [[05-网络/04-API网关/11-api-gateway-security-practices.md|API 网关安全实践]]。

### 3.4 多集群与治理

- 需要跨集群统一入口/故障转移：Istio（east-west）> kgateway（多环境血统）> APISIX/Higress（注册中心式服务发现）> EG（DIY 拼装）> 云厂商（区域 LB）；
- 社区治理：EG/Istio（CNCF）、APISIX（Apache）、Higress（CNCF 沙箱）均开源可控；kgateway 开源 + 企业双轨；云厂商闭源托管。

---

## 四、从 Ingress 迁移到 Gateway API 实操

参考背景：[[05-网络/04-API网关/09-nginx-ingress-migration-guide.md|Nginx Ingress 迁移指南]] 处理的是「换控制器」，本节处理的是「换 API 形态」，两者可叠加。

### 4.1 迁移路径选择

1. **同实现平滑迁移**（推荐首选）：若保留现有数据面（如 NGINX→NGINX、Gloo→kgateway），先做 Ingress→Gateway API 的 API 翻译，流量面零变化；
2. **换实现迁移**：一次变更两件事（API + 控制器），风险翻倍——除非有硬诉求，建议拆成两步走；
3. **绿地**：新应用直接 Gateway API，存量走双栈共存期消化。

### 4.2 自动翻译：ingress2gateway

```bash
# 🟢 安装 ingress2gateway v1.2.0（go install 不触碰集群）
go install sigs.k8s.io/ingress2gateway@v1.2.0

# 🟢 预览翻译结果（不落集群，先 diff 人工审）
ingress2gateway print --providers ingress-nginx --namespace shop

# 🟢 全量生成到文件备审
ingress2gateway print --providers ingress-nginx --all-namespaces > gateway-api-translated.yaml
```

注意：注解方言（金丝雀/重写/白名单等）只有部分被翻译，`print` 结果**必须人工核对**——尤其 `nginx.ingress.kubernetes.io/canary-*` 系列要手工转成 `backendRefs` 权重（见第五节）。

### 4.3 双栈共存期策略（关键工程决策）

新旧两套入口在迁移期并行，控制爆炸半径：

```text
阶段 0（准备）：安装 Gateway API 实现到独立 GatewayClass，与现有 Ingress 控制器互不感知
阶段 1（影子验证）：同一应用同时挂 Ingress 与 HTTPRoute，Gateway 监听独立测试域名/内部入口验证
阶段 2（按域名灰度切流）：DNS/权重按 hostname 逐个从 Ingress 入口切到 Gateway 入口，观察 SLO
阶段 3（回收）：全部切完后下线 Ingress 控制器与旧注解生态
```

双栈期硬规则：

- **一个 hostname 只有一个入口来源**：避免两套控制器同时宣告同一域名造成证书/路由漂移；
- **策略资源先迁非破坏项**（header 改写、超时），认证/限流在切流窗口随入口同步切换；
- **回滚预案 = DNS 切回**：双栈期保留旧入口热备直至观察期（≥1 个发布周期）结束。

```bash
# 🟢 对照两套入口的路由清单（迁移核对表）
kubectl get ingress -A -o custom-columns='NS:.metadata.namespace,NAME:.metadata.name,CLASS:.spec.ingressClassName'
kubectl get httproute -A -o custom-columns='NS:.metadata.namespace,NAME:.metadata.name,GATEWAYS:.spec.parentRefs[*].name'
```

---

## 五、生产案例：金丝雀发布流量切分全例

场景：`shop.example.com` 后端从 `shop-v1`（stable）渐进放量到 `shop-v2`（canary）；要求：内部人员 header 直达金丝雀；默认流量 90/10 起步；网关侧 TLS 终止。

### 5.1 完整 YAML

```yaml
# 1) GatewayClass —— 由所选实现（示例为 Envoy Gateway）的控制器认领
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy-gateway
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
---
# 2) Gateway —— 运营方资源：监听器/证书/准入范围
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: prod-gateway
  namespace: gateway-system
  labels:
    gateway-access: prod            # 供下方 allowedRoutes selector 匹配
spec:
  gatewayClassName: envoy-gateway
  listeners:
  - name: https-shop
    hostname: shop.example.com
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate               # 网关终止 TLS；Passthrough 场景改用 TLSRoute
      certificateRefs:
      - group: ""
        kind: Secret
        name: shop-tls-2026         # 证书 Secret 在 gateway-system 中
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: prod    # 只有打该 label 的 namespace 的路由可挂载
---
# 3) 业务侧两个 Deployment 的 Service（同 namespace，跨 namespace 需 ReferenceGrant）
apiVersion: v1
kind: Service
metadata:
  name: shop-stable
  namespace: shop
spec:
  selector:
    app: shop
    version: v1
  ports:
  - name: http
    port: 80
    targetPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: shop-canary
  namespace: shop
spec:
  selector:
    app: shop
    version: v2
  ports:
  - name: http
    port: 80
    targetPort: 8080
---
# 4) HTTPRoute —— 开发者资源：header 直达 + 90/10 加权
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: shop-route
  namespace: shop                    # 已打 label: gateway-access=prod
spec:
  parentRefs:
  - name: prod-gateway
    namespace: gateway-system
    sectionName: https-shop          # 精确绑定到 listener，防误挂
  hostnames:
  - shop.example.com
  rules:
  # 规则 A：内部验证流量 100% 打金丝雀（特异性更高，优先于规则 B 生效）
  - matches:
    - headers:
      - type: Exact
        name: x-canary
        value: "true"
    backendRefs:
    - name: shop-canary
      port: 80
      weight: 100
  # 规则 B：默认流量加权
  - backendRefs:
    - name: shop-stable
      port: 80
      weight: 90
    - name: shop-canary
      port: 80
      weight: 10
```

语义要点（选型/评审常考点）：

- 规则匹配按**特异性**排序（匹配条件多者优先），与 YAML 书写顺序无关——header 规则 A 天然压过无条件的规则 B；
- `weight` 均为 0 的后端不接流量；单个 backendRef 缺省 weight=1；权重是**配置即时生效**，标准不含渐进算法，需要分步放量就在 CI 中逐步改权重（90/10 → 50/50 → 0/100）；
- 若金丝雀需要「同用户始终同版本」，启用 SessionPersistence（实验通道）或选具备一致性哈希增强的实现（Istio/APISIX 插件）；
- canary Service 变更（镜像升级）不经过网关——金丝雀回滚 = 改回权重，秒级生效（🟡 中风险：改路由即改线上流量分布，需走变更单）。

### 5.2 部署与验证

```bash
# 🟡 应用全部资源
kubectl apply -f shop-canary.yaml

# 🟢 网关就绪与地址
kubectl -n gateway-system get gateway prod-gateway -o jsonpath='{.status.addresses[0].value}'

# 🟢 路由接受状态（ResolvedRefs/Accepted 必须为 True）
kubectl -n shop get httproute shop-route -o jsonpath='{.status.parents[0].conditions}'

# 🟢 灰度比例实测（20 次请求统计版本分布，应约 18:2）
GW_IP=$(kubectl -n gateway-system get gateway prod-gateway -o jsonpath='{.status.addresses[0].value}')
for i in $(seq 1 20); do
  curl -sk --resolve shop.example.com:443:${GW_IP} https://shop.example.com/version
done | sort | uniq -c

# 🟢 header 直达验证（20 次应全部为 canary）
for i in $(seq 1 20); do
  curl -sk -H 'x-canary: true' --resolve shop.example.com:443:${GW_IP} https://shop.example.com/version
done | sort | uniq -c
```

放量节奏建议：10% → 观察 SLO（错误率/P95）→ 50% → 100% → 收编 stable；每步保留一键回滚权重的能力（把上一版 HTTPRoute 存入 Git，`kubectl apply` 回退即回滚）。

---

## 六、故障排查与诊断流程

Gateway API 把状态高度结构化，排查第一站永远是资源 `status`：

| 现象 | 高频根因 | 诊断命令（均 🟢 只读） |
|------|----------|------------------------|
| `Gateway/NotAssigned` 或无地址 | GatewayClass 未被控制器认领（controllerName 不匹配）；LB 未就绪 | `kubectl get gatewayclass <gc> -o jsonpath='{.status.conditions}'` |
| `HTTPRoute/Accepted=False` | listener 不匹配（hostname/sectionName/端口）；`allowedRoutes` 拒绝 | `kubectl get httproute <r> -o jsonpath='{.status.parents[0].conditions}'` |
| `ResolvedRefs=False` | backendRef Service/端口不存在；跨 namespace 缺 ReferenceGrant | `kubectl get referencegrant -A`；核对 parentRef/后端命名空间 |
| 404/路由不生效 | 匹配特异性被别的规则压过；hostnames 拼写 | 用 `kubectl describe httproute` 逐条对照；curl `-H Host:` 复现 |
| 证书不生效/告警 | Secret 类型不是 `kubernetes.io/tls`；Secret 与 listener 不同 namespace 且未引用 | `kubectl get secret shop-tls-2026 -o jsonpath='{.type}'` |
| 加权不生效 | 某后端无可用 endpoint（权重仍占配额但无流量） | `kubectl get endpoints -n shop` 核对两版本端点 |
| 后端 mTLS 失败 | BackendTLSPolicy 的 CA/SNI 与后端不符 | `kubectl get backendtlspolicy -o yaml`；实现侧（EG/Istio）连接日志 |
| 迁移双栈期证书告警/域名漂移 | 两套控制器宣告同一 hostname | 核对 4.3 节「一个 hostname 只有一个入口来源」规则 |

通用顺序：资源 `status.conditions` → 控制器日志（`kubectl -n <实现 ns> logs deploy/<controller>`）→ 数据面配置面（EG `kubectl get envoyproxypatchpolicy` / Istio `istioctl proxy-config`）→ 端到端 curl 复现。可观测性体系见 [[05-网络/04-API网关/12-api-gateway-observability.md|API 网关可观测性]]。

---

## 七、最佳实践与反模式

### 7.1 最佳实践

1. **资源即权限**：按 1.1 节三角色划分 namespace 与 RBAC，`allowedRoutes` 用 selector 收口谁能挂网关；
2. **parentRef 绑 sectionName**：路由显式指定 listener，防止网关新增监听器后路由被意外挂载；
3. **跨 namespace 只走 ReferenceGrant**：显式声明最小授权关系，审计友好；
4. **权重即发布工具**：金丝雀权重变更纳入 GitOps 流水线（权重历史 = 发布历史）；每步变更自动核对 `status.conditions`；
5. **能力分级使用**：生产只用 Standard 通道字段；Experimental 能力（SessionPersistence/BackendTrafficPolicy）单独评审并锁定实现版本；
6. **迁移期双入口资产台账**：hostname→入口来源的映射表纳入 CMDB，杜绝漂移；
7. **一致性测试入 CI**：对所选实现跑标准 conformance 套件，升级实现前先跑一致性防语义回退。

### 7.2 反模式

- ❌ **把 Gateway API 当注解容器**：继续用实现私有注解堆在 Route 上，可移植性收益归零——私有能力优先用该实现的策略 CRD 表达；
- ❌ **一个 Gateway 无限膨胀**：所有团队共享单 listener 单证书桶，变更互相踩踏；按域名/团队拆 listener 或 Gateway；
- ❌ **无 ReferenceGrant 的跨 namespace 引用习惯**：被 allowedRoutes/引用校验拒绝后排障困难；
- ❌ **灰度只看流量比例不看 SLO**：权重切完不盯错误率/P95，金丝雀失去意义；
- ❌ **双栈期两个入口同时宣告同一域名**：证书、路由、WAF 策略三者漂移的经典事故源；
- ❌ **Experimental 字段直接上生产**：跨版本升级时字段语义变化导致行为漂移。

---

## 八、性能与监控

### 8.1 性能关注点

- **控制面翻译延迟**：Route 变更到数据面生效的耗时（各实现 1–10s 量级不等），金丝雀"秒级回滚"依赖该项——上线前实测；
- **数据面**：跨实现对比与压测方法学见 [[05-网络/04-API网关/13-api-gateway-performance-benchmarks.md|API 网关性能基准]]；Envoy 系重点关注 worker 数与连接池配置，APISIX 关注插件链长度与 etcd watch 稳定性；
- **证书规模**：listener/证书数量大的多租户网关注意配置下发体量（xDS/etcd 快照大小）。

### 8.2 监控指标

- **标准状态信号**：`Gateway` Programmed/AddressAssigned、`HTTPRoute` Accepted/ResolvedRefs 的布尔率——接 Prometheus（各实现 controller 均暴露指标）并对 False 告警；
- **金丝雀专项**：按 version 标签拆分的后端 5xx 率、P95 延迟、QPS 对比（验证权重是否符合预期分布，可自动化比对 `weight` 配置与实测比例）；
- **网关通用**：连接数、活跃 listener、证书到期天数（证书管理全景见 [[05-网络/04-API网关/12-api-gateway-observability.md|可观测性]] 与 [[05-网络/04-API网关/14-api-gateway-production-operations.md|生产运维]]）。

---

## 九、版本兼容性与升级注意

- **版本核对口径（2026-10-14）**：Gateway API v1.6.3 / Envoy Gateway v1.9.2 / APISIX IC 2.2.0 / Higress v2.2.5 / ingress2gateway v1.2.0；kgateway 无 GitHub latest release，以官方 changelog 为准。任何固化版本请在使用当日重跑 `gh api repos/<org>/<repo>/releases/latest --jq .tag_name` 复核。
- **CRD 先行**：升级任何实现都应先升 Gateway API CRD（标准通道），再升控制器；反向顺序会出现控制器 Watch 不到新字段；
- **通道纪律**：生产固定 Standard 通道版本；引入 Experimental 字段时在 README/告警中登记 GEP 编号与实现版本，升级前逐项核对 GEP 状态变更；
- **实现升级注意**：EG 小版本迭代较快，升级阅读 release notes 中的策略 CRD 变更；APISIX IC 大版本（1.x→2.x）存在 CRD/注解语义调整；Istio 升级按官方修订通道（1.x→1.x+1 逐版）；云厂商由厂商托管但仍需关注其 Gateway API 支持子集变化公告；
- **迁移期冻结**：双栈共存期内避免升级入口控制器（两个变量叠加排障成本极高），升级安排在切流完成后的稳定窗口。

---

## 十、参考链接与延伸阅读

- Gateway API 官网/规范：<https://gateway-api.sigs.k8s.io/>（v1.6.3：<https://github.com/kubernetes-sigs/gateway-api/releases>）
- GAMMA 倡议：<https://gateway-api.sigs.k8s.io/concepts/gamma/>
- Envoy Gateway：<https://gateway.envoyproxy.io/> / <https://github.com/envoyproxy/gateway>（v1.9.2）
- kgateway：<https://kgateway.dev/> / <https://github.com/kgateway/kgateway>
- Apache APISIX Ingress Controller：<https://apisix.apache.org/docs/ingress-controller/latest/> / <https://github.com/apache/apisix-ingress-controller>（2.2.0）
- Higress：<https://higress.cn/> / <https://github.com/alibaba/higress>（v2.2.5）
- Istio Gateway API 文档：<https://istio.io/latest/docs/tasks/traffic-management/ingress/gateway-api/>
- ingress2gateway：<https://github.com/kubernetes-sigs/ingress2gateway>（v1.2.0）
- AWS Gateway API Controller：<https://www.gateway-api-controller.eks.aws.dev/> ；GKE Gateway：<https://cloud.google.com/kubernetes-engine/docs/concepts/gateway-api>；Azure ALB Controller：<https://learn.microsoft.com/azure/application-gateway/for-containers/>

## Related

- [[05-网络/04-API网关/02-kubernetes-gateway-api-deep-dive.md|Gateway API 标准深度解析]] — API 资源与字段详解
- [[05-网络/04-API网关/03-api-gateway-selection-guide.md|API 网关选型指南]] — 通用选型维度
- [[05-网络/04-API网关/07-envoy-gateway-enterprise.md|Envoy Gateway 企业实践]]
- [[05-网络/04-API网关/04-higress-enterprise-gateway.md|Higress 企业网关]]
- [[05-网络/04-API网关/05-apisix-enterprise-gateway.md|APISIX 企业网关]]
- [[05-网络/04-API网关/09-nginx-ingress-migration-guide.md|Nginx Ingress 迁移指南]]
- [[05-网络/04-API网关/13-api-gateway-performance-benchmarks.md|性能基准]]
- [[05-网络/03-服务网格/index.md|服务网格]] — GAMMA 与东西向流量
- [[05-网络/04-API网关/index.md|API 网关索引]]

<!-- risk-assessed -->
