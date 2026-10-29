---
title: Cluster Extension and Governance Deep Dive
description: 集群扩展与治理层深度解析——CCM、API 扩展（CRD/聚合 apiserver）、准入 webhook、认证授权链、指标链路与 TLS Bootstrap
summary: 核心组件（etcd/apiserver/scheduler/KCM/kubelet/kube-proxy/runtime/DNS/PKI）之外的"下半场"：围绕 apiserver 的扩展点与治理链路，含面试串联图
category: architecture
tags:
- control-plane
- apiserver
- ccm
- crd
- admission
- rbac
- metrics-server
- tls-bootstrap
- interview
tier: core
created: '2026-10-16'
last_updated: 2026-10-16
---

# 集群扩展与治理层深度解析（CCM · API 扩展 · 准入 · 认证授权 · 指标 · TLS Bootstrap）

> 定位：`wiki/01-集群基础/03-控制平面/` 的组件深挖系列（01–41）覆盖了 etcd/apiserver/scheduler/KCM/kubelet/kube-proxy/runtime/存储/网络/PKI——那是"集群组件的下半场"之前的全部主角。
> 本文补齐**围绕 apiserver 的扩展与治理层**：谁的活该外置给云（CCM）、API 怎么扩展（CRD vs 聚合 apiserver）、请求怎么被治理（认证→授权→准入）、指标从哪来（metrics-server 链路）、新节点怎么拿到第一张证书（TLS Bootstrap）。
> 面试速记版见 `maintaining/04-报告/k8s-高级面试快问快答-沉淀.md` Q88–Q93。

---

## 1. 总纲：一个请求进 apiserver 要过几道门

所有扩展与治理机制，都挂在 apiserver 的处理链上。顺序背下来，后面每个组件都往这条链上挂：

```
客户端（kubectl / controller / kubelet）
   │
   ① 认证 Authentication —— "你是谁"
   │    x509 客户端证书 / bearer token / ServiceAccount token / OIDC
   │    失败 → 401
   │
   ② 授权 Authorization —— "你能干什么"
   │    RBAC（默认）/ Node / Webhook / ABAC
   │    拒绝 → 403
   │
   ③ 变更准入 Mutating Admission —— "改写你的请求"
   │    并发调用 MutatingWebhook + 内置插件（默认 ServiceAccount 注入等）
   │
   ④ 对象校验 Schema Validation —— "结构对不对"
   │    OpenAPI 校验、字段类型/必填；失败 → 422
   │
   ⑤ 验证准入 Validating Admission —— "语义行不行"
   │    并发调用 ValidatingWebhook + 内置插件（ResourceQuota/LimitRanger/EventRateLimit…）
   │
   ⑥ 落库 etcd —— 成为一串事件的起点，watch 广播给全部控制器
```

判断线：**401 是身份问题，403 是权限问题，422 是对象问题**——三者的排查方向完全不同。
这条链也是性能链：②的慢 Webhook（authz webhook 模式）、③⑤的准入 webhook，是 Q9「apiserver 变慢」里除了 etcd 之外的两大嫌疑人。

关联：[[18-api-priority-fairness.md|API Priority Fairness]]（链路入口处的排队与限流）、[[12-apiserver-deep-dive.md|Apiserver Deep Dive]]。

---

## 2. cloud-controller-manager：谁的活必须外置给云

### 2.1 为什么存在

早期云相关逻辑内嵌在 KCM/kubelet 里，导致：换云厂商要重编控制面、云 API 故障会拖死控制器。解法是把"跟云 API 说话"的代码拆成独立二进制 **cloud-controller-manager（CCM）**，以 Deployment/静态 Pod 形态跑，与 KCM 同样的 list-watch 模式，只是面向云厂商 OpenAPI。

### 2.2 四类内置控制器

| 控制器 | 职责 | 失效症状 |
|--------|------|---------|
| **cloud-node** | 节点初始化：填 `spec.providerID`、打 region/zone 标签、写地址 | 节点没有 providerID → 后续云盘挂载全失败 |
| **cloud-node-lifecycle** | 云侧已释放的机器 → 同步删除集群 Node 对象 | 退了机器 Node 还挂着，调度往死节点派 Pod |
| **route** | 把 Pod CIDR 到节点的路由下发给 VPC | 跨节点 Pod 网络不通（VPC 路由模式） |
| **service** | `type: LoadBalancer` → 创建/绑定 SLB、健康检查 | LB Service 一直 pending，不分配外部 IP |

### 2.3 面试三连

- **没有 CCM 会怎样？** LB 不分配、VPC 路由不下发、节点缺 providerID 导致 PV 挂不上——数据面在跑，"云集成"全瘫。
- **和 KCM 什么关系？** 同构不同责：都是调和控制器，KCM 管 Kubernetes 对象之间的调和，CCM 管 Kubernetes 对象与云资源之间的调和。
- **专有云/混合场景？** 可能出现两个 CCM 分域共治（公有云 CCM + 专有云侧自研 CCM），边界看"谁拥有那份云资源"。

关联：[[14-cloud-controller-manager-deep-dive.md|CCM Deep Dive]]。

---

## 3. API 扩展两路线：CRD vs 聚合 apiserver

| 维度 | CRD | 聚合 apiserver（Aggregated APIServer） |
|------|-----|--------------------------------------|
| 本质 | 声明一个自定义资源类型，apiserver 替你存 etcd、替你出 REST API | 挂一个**完整的 apiserver 二进制**在主 apiserver 后面（APIService 路由） |
| 开发成本 | 零代码（写 YAML），逻辑放 controller（operator） | 高——要自己实现整套 API 语义 |
| 存储 | 主 etcd（受 `--max-request-bytes`/db size 约束） | 自己定（可以是内存、数据库，甚至实时计算） |
| 性能 | 大对象/高 QPS 会压主 etcd（见 Q11 的教训） | 独立扩缩，与主 etcd 故障隔离 |
| 典型用户 | 几乎所有 operator：Prometheus Operator、Argo CRD… | **metrics-server 就是聚合 apiserver**（`metrics.k8s.io`）、Istiod 早期、云厂商私有 API |

**选型线**：数据模型是"存对象"→ CRD；数据模型是"实时计算/聚合/外置存储"→ 聚合 apiserver。CRD 写大对象高频写，等于把 Q11 的大 LIST 问题自己种进主 etcd。

CRD 的两个隐藏成本：**conversion webhook**（多版本 API 转换）和 **structural schema** 约束——面试常问"CRD 多版本怎么兼容"。

关联：[[29-api-extension-deep-dive.md|API Extension Deep Dive]]。

---

## 4. 准入 webhook：治理的最后一道闸，也是故障高发区

两种类型：**Mutating**（先跑，可改对象，如注入 sidecar）→ **Validating**（后跑，只说 yes/no，如策略校验 OPA/Gatekeeper、Kyverno）。

生产四要素：

1. **failurePolicy**：`Fail`（默认）还是 `Ignore`。webhook 挂了：Fail = **相关资源全部阻塞**（新 Pod 起不来），Ignore = 策略失效静默放行。安全策略必须 Fail，可用性敏感的注入类可斟酌 Ignore——这个选择本身就是面试题。
2. **timeoutSeconds**（默认 10s）：webhook 卡 10 秒 = 每个相关写请求卡 10 秒，这是 Q9「apiserver 变慢」清单里的"慢 webhook"真身。指标：`apiserver_admission_webhook_admission_duration_seconds`。
3. **范围收敛**：`namespaceSelector`/`objectSelector` 必配，别把 kube-system 和控制面组件圈进来——一个拦截 kubelet 请求的 webhook 能把整个集群打死。
4. **sideEffects: None**：声明无副作用，apiserver 的 dry-run（`kubectl apply --dry-run=server`）才能安全调用。

**默认不开的重要插件**：`EventRateLimit`、`PodNodeSelector` 等要显式加进 `--enable-admission-plugins`——"准入插件哪些默认开"是高频追问。

关联：[[12-apiserver-deep-dive.md|Apiserver Deep Dive]]、[[19-etcd-operations.md|Etcd Operations]]（event 洪泛治理）。

---

## 5. 认证授权：身份与权限的落地

### 5.1 认证（你是谁）

| 方式 | 场景 | 备注 |
|------|------|------|
| x509 客户端证书 | 人 + 控制面组件互信（CN=用户名，O=组） | kubeadm 证书体系的入口（见 Q20） |
| ServiceAccount token | Pod 内程序 | **1.24 起不再自动生成长期 Secret token**，改 TokenRequest 短期令牌 + 卷投影自动轮换 |
| OIDC | 企业 SSO 接入 kubectl | 人用的标准路径 |
| Bootstrap token | 新节点引导 | 见 §7 TLS Bootstrap |
| Webhook token / authn proxy | 定制身份源 | 灵活但引入可用性依赖 |

### 5.2 授权（你能干什么）

默认 **RBAC**：主体（User/Group/ServiceAccount）× Role/ClusterRole × RoleBinding/ClusterRoleBinding。要点：

- **权限是加法，没有 deny**——设计最小权限时只能"少给"，不能"给全了再扣"。
- `system:` 前缀的 ClusterRole 是内置的（view/edit/admin/cluster-admin），Binding 挂给团队最常见。
- **Node 授权**专门给 kubelet：只让它读写自己节点相关的对象——"为什么 kubelet 不是普通 RBAC 用户"是好追问。
- 排查口诀：403 先看 `kubectl auth can-i --list`（以对方身份），再看 audit log 里被拒的 verb/resource。

### 5.3 常见事故

- 给 CI/巡检发 `cluster-admin` 的 SA token → 泄漏即全集群沦陷（呼应 Q11 红线：放开 `get/list -A` 的 RBAC）。
- 升级到 1.24+ 后脚本还去读 `<sa>-token` Secret → 拿不到了，要改用 `kubectl create token` 或投影卷。

关联：[[28-authz-authn-deep-dive.md|Authz Authn Deep Dive]]、[[04-plane-security-hardening.md|Plane Security Hardening]]。

---

## 6. 指标链路：kubectl top 的数字从哪来

```
容器 cgroup ──(kubelet 内嵌 cAdvisor 汇总)──► kubelet Summary API
                                                   │ 每 60s 拉一次（--metric-resolution）
                                                   ▼
              metrics-server（内存态，聚合为 Metrics API：metrics.k8s.io）
                     │                                    │
                     ▼                                    ▼
               kubectl top nodes/pods                HPA（资源指标型）
```

四个面试要点：

1. **metrics-server 是聚合 apiserver**，不是 Prometheus 的对手——它只存**最近**的聚合值（内存态，重启即丢），专职给 HPA 和 `kubectl top` 供数。
2. **与 Prometheus 分工**：Prometheus 拉式采集、存时序、做告警与历史分析；metrics-server 推式汇总、只留当下、为调度决策供数。一个管"看清历史"，一个管"喂饱 HPA"。
3. HPA 要按业务指标（QPS）扩缩时，metrics-server 不够，要走 **custom.metrics.k8s.io**（prometheus-adapter 等）或 external.metrics。
4. `kubectl top` 没数据的第一刀：`kubectl get apiservice | grep metrics`——APIService 没 Ready，链路就断在聚合层。

关联：[[24-metrics-server-deep-dive.md|Metrics Server Deep Dive]]。

---

## 7. TLS Bootstrap：新节点怎么拿到第一张证书

承接 Q20 的证书体系——那张"第一张证书"是怎么来的：

```
新节点 kubelet 首次启动（只有 --bootstrap-kubeconfig）
   │  凭 bootstrap token（kubeadm token create，默认 24h 有效）
   ▼
以"申请者"身份提交 CSR（kubernetes.io/kubelet-serving 等类型）
   │  自动批准：csr-approver（RBAC 里 system:bootstrappers 组可批）
   │  自动签发：csr-signer（集群 CA）
   ▼
拿到 kubelet 客户端证书 → 用正式身份再来一轮申请 server 证书
   │
   ▼
此后 --rotate-certificates 周期自轮换（不再需要 bootstrap）
```

面试三连：

- **为什么不直接预置证书？** 千节点规模没法人工发证书；bootstrap token 只有一件事的权限（"申请成为节点"），最小权限自举。
- **token 泄漏了怕不怕？** 怕但可控：短 TTL + 用后即废（`kubeadm token delete`）+ RBAC 只放 `certificatesigningrequests` 创建权。
- **和 `certs renew` 什么关系？** 完全两套——`certs renew` 管控制面组件证书（人工/半自动），kubelet 自己的证书走 bootstrap + 自动轮换（Q15 已背 `--rotate-certificates`）。

关联：[[35-kubeadm-cluster-lifecycle.md|Kubeadm Cluster Lifecycle]]、[[38-certificate-pki-lifecycle-runbook.md|Certificate PKI Lifecycle Runbook]]。

---

## 8. 全景串联：把上下篇组件拼成一张图

```
                     ┌─────────────────── 控制面 ───────────────────┐
 人/CI ──kubectl──►  │ apiserver：authn→authz→准入→etcd             │ ◄──┐
                     │    ▲ webhook 扩展    ▲ CRD/聚合层            │    │
                     └────┼──────────────────────┼────────────────────┘    │
                          │                      │                         │
     ┌────────────────────┼──────────┐     metrics-server（聚合 apiserver）│
     │ etcd（唯一真相）    │ watch    │                                    │
     └────────────────────┘          │                                    │
          ▲        ▲                 ▼                                    │
        KCM      scheduler       各 operator（CRD 控制器）                 │
          │        │                                              list/watch
   （CCM 与云资源调和，四控制器：cloud-node / node-lifecycle / route / service）
          │
   kubelet（TLS bootstrap 进场，Node 授权）──CRI──► containerd → 容器
          │
   kube-proxy / CNI（数据面：Service 规则、Pod 网络）    CoreDNS（名字）
```

一句话收束：**核心组件回答"集群怎么跑"，本文的扩展与治理层回答"怎么安全地改它、怎么接住云、怎么看清它"。**

---

## 9. 对应面试沉淀卡

| 卡片 | 主题 |
|------|------|
| Q88 | 请求进 apiserver 的六道门（链路总纲 + 401/403/422 分界） |
| Q89 | CRD vs 聚合 apiserver 选型 |
| Q90 | 准入 webhook 的 fail-open/fail-closed 与慢 webhook |
| Q91 | CCM 四控制器与"没有 CCM 会怎样" |
| Q92 | metrics-server 链路、与 Prometheus 分工 |
| Q93 | kubelet TLS Bootstrap 全流程 |

位置：`maintaining/04-报告/k8s-高级面试快问快答-沉淀.md`。
