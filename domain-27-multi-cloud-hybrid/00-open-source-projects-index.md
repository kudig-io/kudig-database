# Domain-27 多云与混合云 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Karmada** | 多云多集群调度 | Incubating | v1.13.0 | 4.5k+ | Apache-2.0 |
| **Cluster API** | 声明式集群生命周期 | K8s SIG | v1.9.0 | 3.5k+ | Apache-2.0 |
| **Rancher** | 多集群管理平台 | SUSE | v2.10.0 | 23k+ | Apache-2.0 |
| **Fleet** | Rancher GitOps 多集群 | Rancher | v0.12.0 | 1.5k+ | Apache-2.0 |
| **vCluster** | 虚拟集群 | Loft | v0.24.0 | 7k+ | Apache-2.0 |
| **Kamaji** | 托管 K8s 控制平面 | Clastix | v1.0.0 | 1k+ | Apache-2.0 |
| **Admiralty** | 多集群调度联邦 | 非 CNCF | v0.15.0 | 500+ | Apache-2.0 |
| **Submariner** | 多集群网络互联 | 非 CNCF | v0.19.0 | 3k+ | Apache-2.0 |
| **Skupper** | 应用级安全网络 | Red Hat | v2.0.0 | 1k+ | Apache-2.0 |
| **KubeFed (已归档)** | K8s 集群联邦 | K8s SIG | 已归档 | 3k+ | Apache-2.0 |
| **OCM** | Open Cluster Management | 非 CNCF | v0.16.0 | 1k+ | Apache-2.0 |
| **Clusternet** | 大规模集群管理 | 非 CNCF | v0.20.0 | 1k+ | Apache-2.0 |

---

## 多集群管理选型

| 需求 | 推荐方案 | 说明 |
|:---|:---|:---|
| 应用级多集群分发 | Karmada | PropagationPolicy + OverridePolicy |
| 集群生命周期自动化 | Cluster API | 声明式创建/升级/销毁 |
| 统一运维平面 | Rancher | UI + 监控 + GitOps |
| 多租户虚拟集群 | vCluster | 命名空间内虚拟控制平面 |
| 托管 K8s 服务化 | Kamaji | 多租户控制平面隔离 |
| 跨集群网络直通 | Submariner | Pod IP 跨集群路由 |
| 应用级安全连接 | Skupper | 无需 CNI 改动 |
| 大规模 (>1000) 集群 | OCM / Clusternet | 专为超大规模设计 |

---

## 参考链接

- [Karmada 文档](https://karmada.io/docs/)
- [Cluster API 文档](https://cluster-api.sigs.k8s.io/)
- [Rancher 文档](https://ranchermanager.docs.rancher.com/)
- [vCluster 文档](https://www.vcluster.com/docs/)
- [Submariner 文档](https://submariner.io/)
