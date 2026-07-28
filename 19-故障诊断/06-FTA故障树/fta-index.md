---
title: FTA 故障树完整索引
description: '| TE-4 | 网络通信异常 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#五te-4-网络通信异常-p1)
  | #5 |'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- kubelet
- istio
- envoy
- coredns
- ingress
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- FTA 故障树完整索引 是什么
- 如何 FTA 故障树完整索引
- FTA 故障树完整索引 根因分析
- FTA 故障树完整索引 故障树
trigger_keywords:
- FTA
- 故障树完整索引
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- etcd-basics
- observability-basics
tier: supporting
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# FTA 故障树完整索引

> **文档版本**: v2.0 Enhanced
> **适用范围**: 所有 FTA 故障树的交叉引用和快速定位
> **更新日期**: 2026-05-18

---

## 一、顶事件索引

| 编号 | 名称 | 严重程度 | 文件位置 | 页码 |
|:---|:---|:---:|:---|:---:|
| TE-1 | 集群完全不可用 | 🔴 P0 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#二te-1-集群完全不可用-p0) | #2 |
| TE-2 | 应用服务不可用 | 🔴 P0 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#三te-2-应用服务不可用-p0) | #3 |
| TE-3 | Pod启动失败 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#四te-3-pod启动失败-p1) | #4 |
| TE-4 | 网络通信异常 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#五te-4-网络通信异常-p1) | #5 |
| TE-5 | 存储访问失败 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#六te-5-存储访问失败-p1) | #6 |
| TE-6 | 资源调度异常 | 🟡 P2 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#七te-6-资源调度异常-p2) | #7 |
| TE-7 | 安全认证失败 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#八te-7-安全认证失败-p1) | #8 |
| TE-8 | 监控告警异常 | 🟡 P2 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#九te-8-监控告警异常-p2) | #9 |
| TE-9 | Terway 网络问题 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十te-9-terway-网络问题-p1-新增) | #10 |
| TE-10 | ASM 服务网格问题 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十一te-10-asm-服务网格问题-p1-新增) | #11 |
| TE-11 | ACK-One 多集群异常 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十二te-11-ack-one-多集群异常-p1-新增) | #12 |
| TE-12 | 资源配额超限 | 🟡 P2 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十三te-12-资源配额超限-p2-新增) | #13 |
| TE-13 | 变更管理问题 | 🟠 P1 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十四te-13-变更管理问题-p1-新增) | #14 |
| TE-14 | 容量规划失效 | 🟡 P2 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十五te-14-容量规划失效-p2-新增) | #15 |
| TE-15 | 灾难恢复失败 | 🔴 P0 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十六te-15-灾难恢复失败-p0-新增) | #16 |
| TE-16 | 可观测性完整性缺失 | 🟡 P2 | [kubernetes-fta-full-analysis-v2.md](./kubernetes-fta-full-analysis-v2.md#十七te-16-可观测性完整性缺失-p2-新增) | #17 |

---

## 二、中间事件索引

### 2.1 TE-1 集群完全不可用

| IE 编号 | 名称 | 逻辑门 | BE 数量 |
|:---|:---|:---:|:---:|
| IE-1.1 | 控制平面问题 | OR | 4 |
| IE-1.2 | 工作节点批量问题 | AND | 3 |
| IE-1.3 | 网络基础设施问题 | OR | 2 |
| IE-1.4 | 阿里云 IaaS 层问题 | OR | 3 |
| IE-1.5 | 运维人为失误 | OR | 2 |

### 2.2 TE-2 应用服务不可用

| IE 编号 | 名称 | 逻辑门 | BE 数量 |
|:---|:---|:---:|:---:|
| IE-2.1 | Pod运行异常 | OR | 4 |
| IE-2.2 | Service/Endpoint 访问异常 | OR | 3 |
| IE-2.3 | Ingress/IngressController 访问异常 | OR | 3 |
| IE-2.4 | ASM 服务网格问题 | OR | 3 |
| IE-2.5 | ARMS 应用监控问题 | OR | 3 |

### 2.3 TE-3 ~ TE-8

| TE | IE 编号 | 名称 | 逻辑门 |
|:---|:---|:---|:---:|
| TE-3 | IE-3.1 | 调度失败 | OR |
| TE-3 | IE-3.2 | 镜像拉取失败 | OR |
| TE-3 | IE-3.3 | 容器创建失败 | OR |
| TE-4 | IE-4.1 | DNS 解析异常 | OR |
| TE-4 | IE-4.2 | Pod 间通信异常 | OR |
| TE-4 | IE-4.3 | 集群外部访问异常 | OR |
| TE-4 | IE-4.4 | SLB/Ingress 外部访问异常 | OR |
| TE-4 | IE-4.5 | 跨可用区网络延迟 | OR |
| TE-5 | IE-5.1 | PVC 绑定失败 | OR |
| TE-5 | IE-5.2 | 存储卷挂载失败 | OR |
| TE-5 | IE-5.3 | 存储性能/数据异常 | OR |
| TE-5 | IE-5.4 | ACK 特有存储问题 | OR |
| TE-6 | IE-6.1 | Pod 无法调度 | OR |
| TE-6 | IE-6.2 | 调度结果不符合预期 | OR |
| TE-6 | IE-6.3 | 自定义调度器问题 | OR |
| TE-6 | IE-6.4 | ACK 资源配额限制 | OR |
| TE-7 | IE-7.1 | 证书相关问题 | OR |
| TE-7 | IE-7.2 | RBAC 权限问题 | OR |
| TE-7 | IE-7.3 | 准入控制问题 | OR |
| TE-7 | IE-7.4 | 阿里云安全服务问题 | OR |
| TE-8 | IE-8.1 | 监控数据采集异常 | OR |
| TE-8 | IE-8.2 | 告警系统异常 | OR |
| TE-8 | IE-8.3 | 可视化系统异常 | OR |
| TE-8 | IE-8.4 | ARMS/MSP 特有问题 | OR |

### 2.4 TE-9 ~ TE-16 (新增)

| TE | IE 编号 | 名称 | ACK 特有 |
|:---|:---|:---|:---:|
| TE-9 | IE-9.1 | ENI 模式问题 | ✅ |
| TE-9 | IE-9.2 | IPVLAN 模式问题 | ✅ |
| TE-9 | IE-9.3 | BGP 模式问题 | ✅ |
| TE-9 | IE-9.4 | Service/Ingress 流量异常 | ✅ |
| TE-10 | IE-10.1 | 数据面问题 | ✅ |
| TE-10 | IE-10.2 | 控制面问题 | ✅ |
| TE-10 | IE-10.3 | 流量管理问题 | ✅ |
| TE-10 | IE-10.4 | 可观测性问题 | ✅ |
| TE-10 | IE-10.5 | ASM 特有配置问题 | ✅ |
| TE-11 | IE-11.1 | 集群注册问题 | ✅ |
| TE-11 | IE-11.2 | 跨集群服务发现问题 | ✅ |
| TE-11 | IE-11.3 | 配置同步问题 | ✅ |
| TE-11 | IE-11.4 | 统一监控/日志问题 | ✅ |
| TE-12 | IE-12.1 | API 对象数量限制 | ✅ |
| TE-12 | IE-12.2 | ACK 资源组配额 | ✅ |
| TE-12 | IE-12.3 | 存储配额 | ✅ |
| TE-12 | IE-12.4 | 网络配额 | ✅ |
| TE-13 | IE-13.1 | 升级失败 | - |
| TE-13 | IE-13.2 | 回滚失败 | - |
| TE-13 | IE-13.3 | 配置漂移 | - |
| TE-13 | IE-13.4 | 变更窗口问题 | - |
| TE-14 | IE-14.1 | 节点容量耗尽 | - |
| TE-14 | IE-14.2 | 存储容量耗尽 | - |
| TE-14 | IE-14.3 | 自动扩容问题 | ✅ |
| TE-14 | IE-14.4 | 容量规划不当 | - |
| TE-15 | IE-15.1 | 备份失败 | - |
| TE-15 | IE-15.2 | 恢复失败 | - |
| TE-15 | IE-15.3 | DR 演练失败 | - |
| TE-15 | IE-15.4 | 跨区域问题 | ✅ |
| TE-16 | IE-16.1 | 指标完整性缺失 | - |
| TE-16 | IE-16.2 | 日志完整性缺失 | - |
| TE-16 | IE-16.3 | 链路追踪完整性缺失 | ✅ |
| TE-16 | IE-16.4 | OpenTelemetry 集成问题 | ✅ |
| TE-16 | IE-16.5 | 可观测性盲区 | - |

---

## 三、底事件快速查找表

### 3.1 按问题类型

| 问题类型 | BE 编号 | 所属 TE |
|:---|:---|:---:|
| **OOM 相关** | | |
| OOMKilled (容器) | BE-2.3 | TE-2 |
| OOMKilled (API Server) | BE-1.1.1 | TE-1 |
| OOMKilled (Envoy) | BE-10.1.1 | TE-10 |
| **证书相关** | | |
| 证书过期 (通用) | BE-7.1 | TE-7 |
| API Server 证书过期 | BE-1.1.2 | TE-1 |
| Kubelet 证书过期 | BE-1.5.3 | TE-1 |
| mTLS 证书过期 | BE-2.11.4 | TE-2 |
| **网络相关** | | |
| DNS 解析异常 | BE-4.1, BE-4.2, BE-4.3 | TE-4 |
| CNI 插件问题 | BE-4.4 | TE-4 |
| SLB 健康检查失败 | BE-4.10 | TE-4 |
| **存储相关** | | |
| PVC 绑定失败 | BE-5.1, BE-5.2, BE-5.3 | TE-5 |
| 存储卷挂载失败 | BE-5.4, BE-5.5, BE-5.6 | TE-5 |
| **阿里云特有** | | |
| ENI 多队列压力 | BE-1.8.1.1 | TE-1 |
| ESSD 性能降级 | BE-1.2.1.1 | TE-1 |
| VPC CIDR 耗尽 | BE-9.2.1.1 | TE-9 |
| ASM Istiod 问题 | BE-10.3.1 | TE-10 |

### 3.2 按阿里云服务

| 阿里云服务 | 相关 BE |
|:---|:---|
| **ECS** | BE-1.10.1, BE-1.10.2, BE-1.10.3, BE-9.1.2.1, BE-3.8.1.1 |
| **ESSD** | BE-1.2.1.1, BE-5.2.1.1, BE-5.4.1.1, BE-5.7.1.1 |
| **SLB** | BE-1.11.1, BE-1.11.2, BE-4.10, BE-4.11, BE-12.7 |
| **VPC** | BE-3.7.1.1, BE-4.7.1, BE-4.9.1, BE-9.2.1.1 |
| **Terway** | BE-1.8.1, BE-1.8.2, BE-4.4.1, BE-4.4.2, BE-9.1~9.11 |
| **ASM** | BE-2.11~2.13, BE-10.1~10.11 |
| **ARMS** | BE-2.14~2.16, BE-8.9~8.11 |
| **ACK-One** | BE-11.1~11.8 |
| **OSS** | BE-3.9.3.1, BE-5.3.1.1, BE-15.1.1.1 |

---

## 四、问题传播路径索引

### 4.1 高频问题传播路径

```
1. etcd → API Server → 集群不可用
   BE-1.2.1 → BE-1.2 → IE-1.1 → BE-1.1 → TE-1

2. 内存泄漏 → OOMKilled → Service 不可用
   BE-2.3.1 → BE-2.3 → IE-2.1 → TE-2

3. ENI 带宽瓶颈 → Pod 网络中断 → Service 不可用
   BE-1.8.1.1 → BE-4.4.1.1 → IE-2.1 → TE-2

4. Istiod OOM → xDS 推送失败 → Envoy 无法连接
   BE-10.3.1.1 → BE-10.3 → BE-10.1 → TE-2

5. VPC CIDR 耗尽 → Pod IP 分配失败 → Pod 无法启动
   BE-9.2.1.1 → BE-9.2 → IE-9.1 → TE-9

6. SLB 健康检查失败 → Endpoint 为空 → Service 不可用
   BE-4.10.1 → BE-4.10 → IE-2.2 → TE-2

7. CoreDNS 问题 → DNS 解析失败 → Pod 间通信异常
   BE-4.1.1 → BE-4.1 → IE-4.1 → TE-4

8. CSI driver 异常 → PVC 挂载失败 → Pod 启动失败
   BE-5.3.1.1 → BE-5.3 → IE-5.1 → TE-5
```

### 4.2 ACK 特有问题传播路径

```
9. ESSD burst credits 用尽 → etcd 写入变慢 → API Server 超时
   BE-5.7.1.1 → BE-1.2.4.1 → BE-1.2 → TE-1

10. ENI 多队列资源耗尽 → Pod 网络延迟 → ASM Envoy 超时
    BE-1.8.1.1.1 → BE-9.1.1 → BE-10.2.1 → TE-2

11. ARMS Java Agent 注入失败 → JVM 启动失败 → CrashLoopBackOff
    BE-2.14.1 → BE-2.1 → IE-2.1 → TE-2

12. ACK-One 集群注册失败 → 多集群状态不一致 → 全局服务发现失败
    BE-11.1.1 → BE-11.1 → IE-11.1 → TE-11
```

---

## 五、交叉引用表

### 5.1 FEBM 案例关联

| FEBM 案例 | 关联 TE | 关联 BE | 融合说明 |
|:---|:---:|:---|:---|
| FEBM-case-INC-2026-0215 | TE-2 | BE-2.3 | Java heap space 泄漏 → OOMKilled → FEBM 证据链验证 |
| FEBM-case-INC-2026-0220 | TE-10 | BE-10.1.1 | Envoy OOM → xDS 配置推送失败 → ASM 特有证据链 |
| FEBM-case-INC-2026-0305 | TE-1 | BE-1.2.1 | etcd 磁盘满 → 数据损坏 → 取证时间线重建 |

### 5.2 Runbook 关联

| RB 编号 | 关联 BE | Runbook 名称 |
|:---|:---|:---|
| RB-BE-1.2 | BE-1.2 | etcd 集群故障诊断与修复 |
| RB-BE-2.3 | BE-2.3 | OOMKilled 自动修复 |
| RB-BE-10.3 | BE-10.3 | Istiod 配置推送失败修复 |
| RB-BE-9.2.1.1 | BE-9.2.1.1 | VPC CIDR 扩容方案 |

### 5.3 修复动作关联

| HA 编号 | 关联 BE | 修复动作 | 风险等级 |
|:---|:---|:---|:---:|
| HA-1.2.1 | BE-1.2 | etcd 数据碎片整理 | medium |
| HA-2.3.1 | BE-2.3 | 增加内存 limits | low |
| HA-9.2.1.1 | BE-9.2.1.1 | VPC CIDR 子网扩容 | high |
| HA-10.3.1.1 | BE-10.3.1.1 | Istiod 重启并调整内存 | medium |

---

## 六、概率数据矩阵

### 6.1 高频底事件概率

| BE 编号 | 名称 | 年问题率 | MTTR (min) | 自动修复率 |
|:---|:---|:---:|:---:|:---:|
| BE-2.3 | OOMKilled | 5% | 15 | 70% |
| BE-1.2.1 | etcd 磁盘空间耗尽 | 3% | 45 | 50% |
| BE-4.1.1 | CoreDNS Pod 问题 | 4% | 10 | 80% |
| BE-2.1.1 | CrashLoopBackOff (配置错误) | 8% | 20 | 60% |
| BE-10.1.1 | Envoy OOMKilled | 2% | 5 | 90% |
| BE-9.2.1.1 | VPC CIDR 耗尽 | 1% | 120 | 30% |

### 6.2 阿里云特有底事件概率

| BE 编号 | 名称 | 年问题率 | MTTR (min) | 阿里云服务 |
|:---|:---|:---:|:---:|:---|
| BE-1.8.1.1.1 | ENI 带宽瓶颈 | 2% | 30 | Terway |
| BE-1.2.1.1.1 | ESSD 性能降级 | 1% | 15 | ESSD |
| BE-9.2.1.1 | VPC CIDR 耗尽 | 0.5% | 120 | VPC |
| BE-10.3.1.1 | Istiod OOM | 1.5% | 10 | ASM |

---

## 七、快速查询算法

```
输入: 问题现象/告警/症状
输出: 候选 TE → IE → BE 路径

步骤:
1. 匹配问题现象 → 顶事件 (TE)
2. 根据故障域 → 中间事件 (IE)
3. 根据可观测性 → 底事件 (BE)
4. 根据逻辑门类型 → 确定诊断策略 (OR=并行, AND=顺序)
5. 验证根因 → 修复动作 (HA)
```

---

> **索引版本**: v2.0
> **维护团队**: SRE Team / Platform Team
> **下次更新**: 每季度或重大问题后

<!-- risk-assessed -->
