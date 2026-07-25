---
title: kubelet证书过期导致全节点NotReady
summary: kubelet证书过期导致全节点NotReady：凌晨2:30，监控告警所有工作节点同时进入NotReady状态。kubectl get nodes显示所有worker节点NotReady，但master节点正常。
category: synthesis
tags:
- synthesis
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-01-08'
skill: 01-node-notready
severity: P0
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubelet证书过期导致全节点NotReady

**日期**: 2026-01-08  
**关联Skill**: [[01-node-notready]]  
**严重级别**: P0

## 场景描述
凌晨2:30，监控告警所有工作节点同时进入NotReady状态。kubectl get nodes显示所有worker节点NotReady，但master节点正常。

## 时间线
02:30 监控告警：所有worker节点NotReady
02:35 工程师登录集群，kubectl get nodes确认问题
02:40 检查master节点kube-apiserver日志，发现大量x509证书验证失败
02:45 检查节点证书：openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates，发现已于昨天过期
02:50 确认根因：kubelet证书过期导致无法与apiserver通信
03:00 执行kubeadm certs renew all续期所有证书
03:10 重启所有节点kubelet：systemctl restart kubelet
03:25 所有节点恢复Ready状态
03:30 更新~/.kube/config中的证书数据

## 根因分析
kubeadm默认证书有效期1年，但运维团队未配置自动续期机制。证书过期后kubelet无法向apiserver上报心跳，导致所有节点被标记为NotReady。

## 影响评估
所有worker节点上的Pod在5分钟后被驱逐，业务服务全部中断，影响约200个微服务。

## 教训与预防
1. 必须使用cert-manager或kubeadm自动续期机制
2. 证书过期前30天/7天/1天需分级告警
3. 建立证书清单，定期巡检所有证书有效期
## Related

- [[22-概念/14-case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
- [[22-概念/14-case-studies/2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断.md|2026-05-10-networkpolicy默认拒绝导致ci-cd流水线全中断]]


<!-- risk-assessed -->
