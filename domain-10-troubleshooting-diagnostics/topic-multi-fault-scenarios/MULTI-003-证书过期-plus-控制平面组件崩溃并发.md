---
title: 证书过期 + 控制平面组件崩溃并发
summary: 证书过期 + 控制平面组件崩溃并发：集群证书过期导致apiserver拒绝所有连接，同时etcd因磁盘压力 unhealthy，控制平面完全不可用。
category: uncategorized
tags:
- uncategorized
- visibility/public
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
scenario_id: MULTI-003
type: multi-fault
skills:
- 06-certificate-expiry
- 11-control-plane-failure
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 证书过期 + 控制平面组件崩溃并发

## 关联Skill
- [[06-certificate-expiry]]
- [[11-control-plane-failure]]

## 场景描述
集群证书过期导致apiserver拒绝所有连接，同时etcd因磁盘压力 unhealthy，控制平面完全不可用。

## 根因分析
证书过期触发apiserver认证失败，同时etcd日志堆积导致磁盘满，形成级联问题。

## 诊断流程
1. 检查证书: openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates
2. 检查etcd: etcdctl endpoint health
3. 检查磁盘: df -h /var/lib/etcd
4. 检查apiserver日志: journalctl -u kube-apiserver -n 50
5. 检查etcd日志: journalctl -u etcd -n 50

## 修复方案
1. 清理etcd日志和快照释放磁盘
2. 执行etcd defrag: etcdctl defrag
3. 续期证书: kubeadm certs renew all
4. 重启apiserver和etcd（移动manifest文件触发）
5. 更新所有kubeconfig中的证书
6. 配置证书自动续期和etcd自动压缩

## 升级决策点
- **P0（立即升级）**：核心业务服务完全不可用，数据面临丢失风险
- **P1（建议升级）**：部分服务受影响，有临时workaround但修复复杂
- **P2（观察）**：非关键路径，当前影响可控

## 预防性措施
1. 建立多维度监控（节点 + 应用 + 网络）
2. 配置级联告警（当多个关联指标同时异常时触发）
3. 定期进行混沌工程演练模拟并发问题
4. 维护问题关联矩阵（哪些问题容易并发出现）

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
