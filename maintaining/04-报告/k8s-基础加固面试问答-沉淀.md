# K8s 基础加固面试 · 一问一答沉淀

> 来源：K8s 基础加固面试准备会话（概念 + 常见问题）。每问答完即沉淀一条，格式同答案卡。
> 模块路线：集群 → 节点 → 负载 → 网络 → 存储 → 安全。参照 CIS Benchmark 体系。
> 开始日期：2026-10-15

---

## 模块 1/6：集群

### Q1. 如何加固 Kubernetes 的 API Server？

**一句话**｜API Server 是集群唯一入口，加固就是守住"认证 → 授权 → 准入控制 → 传输与暴露面"四道闸门。

**30 秒答**｜①认证：`--anonymous-auth=false` 禁匿名，不用 basic-auth / token 静态文件，用户走 OIDC、组件走 X.509 或 SA Token。②授权：`--authorization-mode=Node,RBAC`，RBAC 最小权限——日常不用 cluster-admin、不给 default SA 绑权限、禁止通配符授权。③准入控制：必开 `NodeRestriction`（kubelet 只能改自己节点的对象）；用 ValidatingAdmissionWebhook 或 1.26+ 内置的 ValidatingAdmissionPolicy（CEL）拦特权容器、强制镜像白名单；开 `AlwaysPullImages` 防本地缓存镜像冒用。④暴露面：只留 6443 HTTPS、TLS 1.2+ 强加密套件；etcd 用 `--encryption-provider-config` 开 Secret 静态加密（优先 KMS v2），否则 Secret 只是 base64；API Server 不放公网，前面 LB + 白名单或云私网 endpoint。判断线：能把四道闸门按顺序讲清、每层给得出具体 flag 的，才算真加固过。

**弹药**
- `--service-account-lookup=true`、`--insecure-port=0`（1.20 后已移除）
- Secret 加密：1.27+ 推荐 KMS v2；更早的 `aescbc` 可用，KMS v1 有性能问题
- 审计：`--audit-policy-file`，对 secrets 读写、exec、RBAC 变更记 RequestResponse 级，日志送出集群独立存储防抹除
- 验证：`kube-bench` 跑 CIS Benchmark；`kubectl get --raw /api` 探测匿名访问应返回 403

**红线**｜❌ 认为 etcd 里的 Secret "默认就是加密的"（只是 base64）；❌ 只讲 RBAC 讲不出准入控制和审计；❌ 把 API Server 直接挂公网靠"没人知道地址"当防护。

**常见追问**
- *Secret 在 etcd 里安全吗？* → 默认仅 base64，必须开 encryption-provider-config。
- *怎么验证配置生效？* → kube-bench + 匿名探测 + 审计日志抽查。
- *审计日志放哪？* → 送出集群到独立存储，防止攻击者删痕迹。

**下一问**｜etcd 本身怎么加固？（→ 模块 1 Q2：etcd 加固）

---
