# 10 - 生产迁移实战案例

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: 案例复盘, 50+ 微服务, 零停机, 灰度切流, 生产实战

---

## 目录

1. [案例背景](#1-案例背景)
2. [迁移规划](#2-迁移规划)
3. [Phase 0: 评估与准备](#3-phase-0-评估与准备)
4. [Phase 1: ACK 集群搭建](#4-phase-1-ack-集群搭建)
5. [Phase 2: 工作负载迁移](#5-phase-2-工作负载迁移)
6. [Phase 3: 灰度切流](#6-phase-3-灰度切流)
7. [Phase 4: 退役与收尾](#7-phase-4-退役与收尾)
8. [踩坑记录与经验总结](#8-踩坑记录与经验总结)

---

## 1. 案例背景

### 1.1 业务概况

| 维度 | 详情 |
|------|------|
| **业务类型** | 电商 SaaS 平台（B2B） |
| **日活用户** | ~50,000 |
| **峰值 QPS** | ~8,000 |
| **微服务数** | 56 个（含 BFF、API、Worker、CronJob） |
| **团队规模** | 后端 15 人、运维 3 人、DBA 1 人 |

### 1.2 源集群架构

```
IDC 机房（自建）
├── Kubernetes v1.26.8 (kubeadm)
├── 节点: 3 Master + 12 Worker (物理机 + VM 混合)
├── CNI: Calico (IPIP 模式)
├── 存储:
│   ├── Ceph RBD (有状态服务)
│   ├── NFS (共享文件/日志)
│   └── Local PV (ES 数据)
├── Ingress: nginx-ingress-controller v1.8
├── 有状态服务:
│   ├── MySQL 8.0 (主从, StatefulSet)
│   ├── Redis 6.2 (哨兵模式, StatefulSet)
│   ├── Elasticsearch 7.17 (3 节点, StatefulSet)
│   └── Kafka 3.4 (3 broker, StatefulSet)
├── 监控: Prometheus + Grafana + Alertmanager
├── 日志: Filebeat → Elasticsearch → Kibana
├── CI/CD: GitLab CI + ArgoCD
└── 镜像仓库: Harbor v2.8
```

### 1.3 迁移驱动力

| 痛点 | 说明 |
|------|------|
| **运维成本高** | 3 人团队维护物理机 + K8s + Ceph + 网络，精力不足 |
| **扩容慢** | 新节点上线需采购物理机，周期 2-4 周 |
| **SLA 不足** | 无多 AZ 容灾，机房断电曾导致 4h 服务中断 |
| **安全合规** | 客户要求等保三级，自建难以满足 |
| **IDC 合同到期** | 机房托管合同 6 个月后到期，需迁出 |

---

## 2. 迁移规划

### 2.1 时间线

```
Week 1-2:  Phase 0 — 评估与准备
Week 3-4:  Phase 1 — ACK 集群搭建与基线
Week 5-6:  Phase 2a — 无状态服务迁移
Week 7-8:  Phase 2b — 有状态服务迁移
Week 9-10: Phase 3 — 灰度切流（10% → 30% → 50% → 100%）
Week 11:   Phase 4 — 稳定观察 + 源集群退役
```

### 2.2 迁移策略

- **整体策略**: 双集群并行 + DNS 灰度切流（零停机）
- **有状态服务**: MySQL/Redis → 阿里云 RDS/Redis（托管服务），ES/Kafka → ACK StatefulSet
- **网络互通**: VPN Gateway（IDC ↔ 阿里云 VPC）
- **镜像**: Harbor → ACR 企业版同步

---

## 3. Phase 0: 评估与准备

### 3.1 评估发现

```bash
# 运行评估脚本后发现的关键问题:

# 1. 弃用 API
# pluto 扫描结果:
#   - 3 个 Ingress 使用 extensions/v1beta1（已移除）
#   - 2 个 CronJob 使用 batch/v1beta1（需升级到 batch/v1）
#   修复: 更新 apiVersion 后重新 apply

# 2. PodSecurityPolicy
#   - 集群使用了 PSP（ACK 1.25+ 已移除）
#   修复: 转换为 Pod Security Standards

# 3. Docker 运行时
#   - 2 个服务挂载了 docker.sock（日志采集 + 构建服务）
#   修复: 日志改用 Filebeat DaemonSet，构建改用 Kaniko

# 4. Ceph 特有配置
#   - 12 个 PVC 使用 ceph-rbd StorageClass
#   修复: 映射到 alicloud-disk-essd

# 5. 自定义 CRD
#   - cert-manager v1.10（需在 ACK 安装）
#   - prometheus-operator CRDs（需在 ACK 安装）
#   - ArgoCD CRDs（需在 ACK 安装）
```

### 3.2 网络打通

```bash
# IDC ↔ 阿里云 VPN 配置
# IDC 侧: Cisco ASA 防火墙
# 阿里云侧: VPN Gateway

# 验证互通
# 从 IDC 服务器 ping ACK VPC 网段
ping 10.0.0.1  # ACK vSwitch gateway
# 从 ACK Pod ping IDC MySQL
kubectl run ping-test --rm -it --image=busybox -- ping 192.168.1.100
```

### 3.3 镜像同步

```bash
# ACR 企业版配置同步规则
# 源: harbor.internal.com (通过 VPN 可达)
# 目标: registry.cn-hangzhou.aliyuncs.com/saas-prod/
# 模式: 自动同步 (tag trigger)

# 手动触发全量同步
image-syncer --auth sync-config.yaml --images sync-images.yaml --retries 3
# 结果: 187 个镜像 tag 全部同步成功
```

---

## 4. Phase 1: ACK 集群搭建

### 4.1 集群规格

| 配置项 | 值 |
|--------|-----|
| **集群类型** | ACK Pro 托管版 |
| **K8s 版本** | 1.28.9-aliyun.1 |
| **区域** | 华东 1（杭州） |
| **可用区** | cn-hangzhou-h, cn-hangzhou-i |
| **VPC CIDR** | 10.0.0.0/8 |
| **Service CIDR** | 172.21.0.0/16 |
| **CNI** | Terway (ENI 多 IP) |
| **节点池** | system (3x ecs.g7.xlarge) + app (6x ecs.g7.2xlarge) + stateful (3x ecs.r7.2xlarge) |

### 4.2 有状态服务准备

```bash
# 创建 RDS MySQL (主实例 + 只读实例)
# 规格: mysql.n4.large.2c (4C16G), 200GB ESSD PL1
# 结果: rm-bp1xxxxxxxxx

# 创建阿里云 Redis (主从版)
# 规格: redis.master.mid.default (2G)
# 结果: r-bp1xxxxxxxxx

# 在 ACK 部署 Elasticsearch StatefulSet
# 3 节点, 每节点 100GB ESSD PL1
# 使用 ECK Operator 管理

# 在 ACK 部署 Kafka (Strimzi Operator)
# 3 broker, 每 broker 200GB ESSD PL1
```

---

## 5. Phase 2: 工作负载迁移

### 5.1 迁移批次

```
批次 1 (Week 5): 内部工具服务 (8 个)
  ├── 管理后台 BFF
  ├── 文件上传服务
  ├── 邮件通知服务
  └── ...其他内部工具
  验证: 内部人员使用 1 周

批次 2 (Week 6): 核心 API 服务 (20 个)
  ├── 用户服务
  ├── 订单服务
  ├── 支付服务
  ├── 商品服务
  └── ...其他 API
  验证: 通过 ACK Ingress IP 直接测试

批次 3 (Week 7): 有状态服务
  ├── MySQL → RDS (DTS 增量同步)
  ├── Redis → 阿里云 Redis (redis-shake 同步)
  ├── ES → ACK ES (snapshot/restore)
  └── Kafka → ACK Kafka (MirrorMaker2)

批次 4 (Week 8): 剩余服务 + Worker + CronJob (28 个)
  ├── 异步 Worker
  ├── CronJob (暂停状态)
  └── 所有 DaemonSet
```

### 5.2 关键操作记录

```bash
# MySQL DTS 增量同步
# 创建 DTS 迁移任务: 全量 + 增量同步
# 全量同步耗时: 2h (120GB 数据)
# 增量延迟: < 1s

# Redis 同步
# redis-shake sync 模式
# 全量同步耗时: 15min (8GB 数据)
# 增量延迟: < 100ms

# ES 快照迁移
# 快照大小: 450GB
# 上传到 OSS 耗时: 1.5h
# 恢复耗时: 2h

# Kafka MirrorMaker2
# 配置源→目标单向同步
# 37 个 topic 全部同步成功
# 消费者 offset 同步误差 < 10 条
```

---

## 6. Phase 3: 灰度切流

### 6.1 切流记录

| 时间 | 权重 | 持续 | 发现的问题 | 处理方式 |
|------|------|------|-----------|---------|
| Week 9 Day 1 | 10% ACK | 48h | 支付回调 IP 白名单未加 ACK 出口 IP | 添加 NAT Gateway 出口 IP 到支付平台白名单 |
| Week 9 Day 3 | 30% ACK | 48h | 文件上传服务 NAS 延迟偶发高 | NAS 挂载参数优化 (nconnect=4) |
| Week 10 Day 1 | 50% ACK | 48h | 高峰期 HPA 扩容触发，正常 | 确认 HPA 策略正确 |
| Week 10 Day 3 | 100% ACK | 观察 7d | 无问题 | - |
| Week 11 Day 3 | 100% ACK | 稳定 | 退役源集群 | 最终备份后关停 |

### 6.2 关键指标对比

```
指标                  源集群      ACK 集群     差异
─────────────────────────────────────────────────
API P99 延迟          45ms        38ms        -15% (更好)
API 错误率            0.02%       0.01%       -50% (更好)
峰值 QPS              8,200       8,500       +3.6%
节点 CPU 利用率        65%         52%         -13% (更优)
Pod 启动时间          15s         8s          -46% (更快)
存储 IOPS (MySQL)     25,000      55,000      +120% (ESSD)
```

---

## 7. Phase 4: 退役与收尾

### 7.1 退役时间线

```
Day 1:  停止 DTS 增量同步（确认 ACK RDS 为主库）
Day 2:  停止 redis-shake 同步
Day 3:  停止 Kafka MirrorMaker2
Day 5:  源集群最终 Velero 全量备份
Day 7:  源集群所有 Deployment scale=0
Day 8:  确认 ACK 无异常后，关停源集群节点
Day 14: IDC 设备下架
```

### 7.2 成本对比

| 费用项 | 源集群 (月) | ACK (月) | 变化 |
|--------|-----------|---------|------|
| IDC 托管费 | ¥45,000 | ¥0 | -100% |
| 服务器折旧 | ¥30,000 | ¥0 | -100% |
| 运维人力 | ¥30,000 (1 人) | ¥10,000 (兼职) | -67% |
| ACK + ECS | ¥0 | ¥38,000 | 新增 |
| RDS + Redis | ¥0 | ¥8,500 | 新增 |
| 网络带宽 | ¥5,000 | ¥6,000 | +20% |
| 存储 | ¥8,000 | ¥5,000 | -37% |
| **合计** | **¥118,000** | **¥67,500** | **-43%** |

---

## 8. 踩坑记录与经验总结

### 8.1 踩坑记录

| 序号 | 问题 | 影响 | 根因 | 解决方案 | 教训 |
|:---:|------|------|------|---------|------|
| 1 | 切流后支付回调 502 | P0 支付中断 15min | 支付平台 IP 白名单未包含 ACK NAT 出口 IP | 添加 NAT IP 到白名单 | 迁移前梳理所有外部 IP 白名单 |
| 2 | NAS 挂载偶发超时 | 文件上传失败 | 默认 NFS 挂载参数未优化 | 添加 nconnect=4,hard,timeo=600 | NAS 迁移前做性能测试 |
| 3 | ES 恢复后索引 readonly | ES 查询 403 | ESSD 磁盘空间不足触发 readonly | 扩容磁盘 + 清理旧索引 | ES 磁盘预留 30% 空间 |
| 4 | CronJob 重复执行 | 数据重复处理 | 源集群 CronJob 未暂停 | 紧急暂停源集群 CronJob | 迁移 CronJob 前先暂停源端 |
| 5 | Calico NP 迁移遗漏 | 服务间无法通信 | GlobalNetworkPolicy 未转换 | 逐条转为 K8s NetworkPolicy | 提前完整导出 NetworkPolicy |
| 6 | HPA 未生效 | 高峰期 Pod 不扩容 | metrics-server 未安装 | 安装 metrics-server Addon | 迁移 HPA 时验证 metrics 可用 |

### 8.2 经验总结

**做得好的**:
1. **双集群灰度切流** — 零停机迁移，发现问题可立即回滚
2. **DTS 增量同步** — MySQL 数据零丢失，切换窗口 < 1min
3. **分批迁移** — 内部服务先行试错，核心服务后迁更安心
4. **完整的监控对比** — 双集群 Grafana 对比看板，实时发现异常
5. **提前降低 DNS TTL** — 切流/回滚生效时间 < 2min

**需要改进的**:
1. 外部系统 IP 白名单清单应在 Phase 0 就完整梳理
2. NAS 性能测试应提前进行，而非切流后才发现
3. CronJob 迁移应有独立的暂停/恢复流程
4. NetworkPolicy 迁移应有自动化转换工具
5. 回滚演练应在正式切流前至少执行一次

### 8.3 给后来者的建议

```
1. 永远高估迁移时间，预留 50% 缓冲
2. 有状态服务优先考虑云托管（RDS/Redis），运维成本大幅降低
3. 迁移不只是技术活，务必与业务方充分沟通停机窗口
4. 每个阶段都做备份，Velero 是救命稻草
5. DNS TTL 提前降低，这是零停机的关键
6. 监控先行，没有可观测性就是盲飞
7. 文档记录每一步操作，后续复盘和知识沉淀价值巨大
8. 自动化脚本可复用，值得投入时间开发
```

---

## 附录: 迁移 Checklist 汇总

```
Phase 0: 评估
  [ ] 集群现状采集报告
  [ ] 兼容性评估（API/存储/网络）
  [ ] 风险矩阵与应对方案
  [ ] 迁移计划与排期
  [ ] 成本估算与审批

Phase 1: 搭建
  [ ] ACK 集群创建完成
  [ ] VPC/网络/VPN 打通
  [ ] 节点池就绪
  [ ] 监控/日志基线建立
  [ ] 有状态服务实例创建

Phase 2: 迁移
  [ ] 镜像全部同步到 ACR
  [ ] Namespace/RBAC 迁移
  [ ] ConfigMap/Secret 迁移
  [ ] 无状态 Deployment 迁移
  [ ] Service/Ingress 迁移
  [ ] 存储/数据迁移
  [ ] 有状态服务数据同步
  [ ] 数据一致性校验
  [ ] 功能验证通过
  [ ] 性能测试通过

Phase 3: 切流
  [ ] DNS TTL 降至 60s
  [ ] 10% 切流 + 24h 观察
  [ ] 30% 切流 + 24h 观察
  [ ] 50% 切流 + 24h 观察
  [ ] 100% 切流
  [ ] 7 天稳定观察

Phase 4: 退役
  [ ] 源集群最终备份
  [ ] 停止所有同步任务
  [ ] 源集群关停
  [ ] 迁移复盘报告
  [ ] 知识沉淀与文档归档
```

---

**上一步**: ← [09-迁移工具链参考](./09-migration-toolchain.md)
**回到目录**: → [README](./README.md)
