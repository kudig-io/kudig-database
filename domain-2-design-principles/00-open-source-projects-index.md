# Domain-2 设计原则 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes** | 声明式 API 与控制器的典范实现 | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **etcd** | 分布式一致性存储 (Raft) | Graduated | v3.5.21 | 48k+ | Apache-2.0 |
| **Controller-runtime** | K8s 控制器框架 | K8s SIG | v0.20.0 | 2.5k+ | Apache-2.0 |
| **client-go** | K8s 官方 Go 客户端 | K8s SIG | v0.33.0 | - | Apache-2.0 |
| **Informer** | K8s 缓存与事件机制 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Workqueue** | K8s 速率限制队列 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Cel-go** | CEL 表达式引擎 (K8s 验证) | Google | v0.24.0 | 1k+ | Apache-2.0 |
| **controller-gen** | CRD / Webhook 代码生成 | K8s SIG | v0.17.0 | - | Apache-2.0 |

---

## 参考链接

- [K8s 架构设计文档](https://kubernetes.io/docs/concepts/architecture/)
- [controller-runtime](https://github.com/kubernetes-sigs/controller-runtime)
- [client-go 示例](https://github.com/kubernetes/client-go/tree/master/examples)
