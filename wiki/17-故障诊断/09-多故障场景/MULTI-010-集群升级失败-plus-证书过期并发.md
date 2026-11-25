---
title: 集群升级失败 + 证书过期并发
summary: 集群升级失败 + 证书过期并发：执行集群版本升级时，升级过程因kubelet证书过期中断，部分节点版本不一致导致API兼容性错误。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-010
type: multi-fault
skills:
- 25-cluster-upgrade-migration
- 06-certificate-expiry
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群升级失败 + 证书过期并发

## 关联Skill
- [[27-cluster-upgrade-migration]]
- [[06-certificate-expiry]]

## 场景描述
执行集群版本升级时，升级过程因kubelet证书过期中断，部分节点版本不一致导致API兼容性错误。

## 根因分析
升级前未检查证书有效期，kubelet证书在升级过程中过期导致节点无法加入新版本集群。

## 诊断流程
1. 检查节点版本: kubectl get nodes -o wide
2. 检查证书: openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
3. 检查升级状态: kubeadm upgrade plan
4. 检查apiserver日志: kubectl logs -n kube-system -l component=kube-apiserver --tail=50
5. 检查kubelet日志: journalctl -u kubelet -n 50

## 修复方案
1. 回滚未成功升级的节点到原版本
2. 续期kubelet证书: kubeadm certs renew all
3. 重启kubelet和apiserver
4. 重新执行升级: kubeadm upgrade apply <version>
5. 建立升级前检查清单（含证书有效期检查）

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）

## 时间线还原

| 时间 | 事件 | 操作 |
|------|------|------|
| 22:00 | 执行集群升级 1.26→1.27 | 🟢 `kubeadm upgrade plan` |
| 22:05 | 控制平面升级失败: 证书过期 | 🟢 `kubeadm certs check-expiration` |
| 22:08 | apiserver 不可用，kubectl 超时 | 🟢 `kubectl get nodes` (timeout) |
| 22:10 | 确认根因: 证书即将过期 + 升级过程触发证书验证失败 | 🟢 `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates` |
| 22:15 | 先续期证书再重试升级 | 🟡 `kubeadm certs renew all` |
| 22:20 | 重启控制平面组件 | 🟡 `crictl pods --name kube-apiserver -q \| xargs crictl stopp` |
| 22:25 | 重新执行升级成功 | 🟡 `kubeadm upgrade apply v1.27.x` |
| 22:40 | 集群升级完成，所有节点 Ready | 🟢 `kubectl get nodes -o wide` |

## 故障关联图

```
证书即将过期(根因) + 升级操作(触发因素)
    ├── 升级过程触发证书验证 → 证书过期
    │       └── apiserver TLS失败 → API不可用
    │               └── 升级中断 → 集群处于中间状态
    └── 影响: 集群不可用 + 升级未完成
```

## 关键教训

1. **升级前检查不充分**: 未先执行 `kubeadm certs check-expiration`
2. **升级窗口选择**: 应在证书充足有效期内升级
3. **回滚预案**: 升级失败后无明确回滚步骤

## 面试要点

1. **Q: 集群升级前必须检查哪些项？**
   A: 证书有效期 → etcd 健康 → CNI/CSI 兼容性 → API 废弃字段 → 备份 etcd → 确认 PDB 状态

2. **Q: 升级失败后集群处于中间状态如何恢复？**
   A: 检查哪个组件失败 → 修复根因(证书/配置) → 重启失败组件 → 重新执行升级 → 验证集群状态

3. **Q: 如何避免升级和证书问题并发？**
   A: 升级前先续期证书 → 启用自动轮转 → 升级前全面预检 → 选择证书充足有效期窗口

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
