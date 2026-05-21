---
title: 01 - 迁移评估与规划
description: echo "=== 3. 存储资源 ==="
category: migration
tags:
- k8s
- migration
- modernization
- etcd
- kubelet
- prometheus
- grafana
- cilium
- flannel
- calico
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 迁移评估与规划 是什么
- 如何 迁移评估与规划
trigger_keywords:
- 迁移评估与规划
- migration
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- etcd-basics
- tls-basics
- logging-basics
---

# 01 - 迁移评估与规划

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: 迁移评估, 兼容性分析, 风险矩阵, 项目计划

---

## 目录

1. [集群现状采集](#1-集群现状采集)
2. [兼容性评估](#2-兼容性评估)
3. [风险分析与应对](#3-风险分析与应对)
4. [迁移策略选择](#4-迁移策略选择)
5. [迁移项目计划模板](#5-迁移项目计划模板)
6. [成本估算](#6-成本估算)

---

## 1. 集群现状采集

### 1.1 自动化采集脚本

> 在自建集群上运行以下脚本，一次性采集所有迁移决策所需信息。

```bash
#!/bin/bash
# migration-assessment.sh - 迁移评估信息采集脚本
# 用法: bash migration-assessment.sh > assessment-report-$(date +%Y%m%d).txt

set -euo pipefail

echo "=========================================="
echo "  Kubernetes 迁移评估报告"
echo "  采集时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="

echo ""
echo "=== 1. 集群版本与节点信息 ==="
echo "--- Kubernetes 版本 ---"
kubectl version --short 2>/dev/null || kubectl version
echo ""
echo "--- 节点信息 ---"
kubectl get nodes -o wide
echo ""
echo "--- 节点资源概况 ---"
kubectl top nodes 2>/dev/null || echo "metrics-server 未安装，跳过"
echo ""
echo "--- 节点详情 (OS/容器运行时/内核) ---"
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
STATUS:.status.conditions[-1].type,\
OS:.status.nodeInfo.osImage,\
KERNEL:.status.nodeInfo.kernelVersion,\
CONTAINER_RUNTIME:.status.nodeInfo.containerRuntimeVersion,\
KUBELET:.status.nodeInfo.kubeletVersion
echo ""

echo "=== 2. 命名空间与工作负载统计 ==="
echo "--- 命名空间列表 ---"
kubectl get namespaces --no-headers | awk '{print $1}'
echo ""
echo "--- 各命名空间工作负载数量 ---"
for ns in $(kubectl get namespaces --no-headers -o custom-columns=:metadata.name | grep -v "^kube-"); do
  deploys=$(kubectl get deployments -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  sts=$(kubectl get statefulsets -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  ds=$(kubectl get daemonsets -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  jobs=$(kubectl get jobs -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  cj=$(kubectl get cronjobs -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  pods=$(kubectl get pods -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  svcs=$(kubectl get services -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  echo "  $ns: Deploy=$deploys STS=$sts DS=$ds Job=$jobs CronJob=$cj Pods=$pods Svc=$svcs"
done
echo ""

echo "=== 3. 存储资源 ==="
echo "--- StorageClass ---"
kubectl get storageclass
echo ""
echo "--- PV 列表 ---"
kubectl get pv -o custom-columns=\
NAME:.metadata.name,\
CAPACITY:.spec.capacity.storage,\
ACCESS_MODE:.spec.accessModes,\
RECLAIM:.spec.persistentVolumeReclaimPolicy,\
STATUS:.status.phase,\
STORAGECLASS:.spec.storageClassName
echo ""
echo "--- PVC 列表 ---"
kubectl get pvc -A -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
STATUS:.status.phase,\
VOLUME:.spec.volumeName,\
CAPACITY:.status.capacity.storage,\
STORAGECLASS:.spec.storageClassName
echo ""

echo "=== 4. 网络配置 ==="
echo "--- CNI 插件 ---"
ls /etc/cni/net.d/ 2>/dev/null || echo "无法读取 CNI 配置（在 Master 节点执行）"
kubectl get pods -n kube-system -o wide | grep -E "(calico|flannel|cilium|weave|canal|terway)" || echo "无法确定 CNI"
echo ""
echo "--- Service 类型分布 ---"
kubectl get svc -A --no-headers | awk '{print $3}' | sort | uniq -c | sort -rn
echo ""
echo "--- Ingress 资源 ---"
kubectl get ingress -A 2>/dev/null
echo ""
echo "--- Ingress Controller ---"
kubectl get pods -A | grep -iE "(ingress|nginx|traefik|higress|apisix)" || echo "未检测到 Ingress Controller"
echo ""

echo "=== 5. CRD 与自定义资源 ==="
echo "--- 自定义 CRD 列表 ---"
kubectl get crd --no-headers | grep -v "^$" | awk '{print $1}'
echo ""
echo "--- CRD 数量统计 ---"
kubectl get crd --no-headers | wc -l | xargs
echo ""

echo "=== 6. RBAC 与安全 ==="
echo "--- ClusterRole 数量 ---"
kubectl get clusterroles --no-headers | wc -l | xargs
echo ""
echo "--- 自定义 ClusterRoleBinding ---"
kubectl get clusterrolebindings --no-headers | grep -v "^system:" | head -20
echo ""
echo "--- ServiceAccount 列表 (非 default) ---"
kubectl get sa -A --no-headers | grep -v "^default " | grep -v " default " | head -20
echo ""
echo "--- NetworkPolicy ---"
kubectl get networkpolicies -A 2>/dev/null
echo ""

echo "=== 7. ConfigMap 与 Secret 统计 ==="
echo "--- 各命名空间 ConfigMap/Secret 数量 ---"
for ns in $(kubectl get namespaces --no-headers -o custom-columns=:metadata.name | grep -v "^kube-"); do
  cm=$(kubectl get configmaps -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  sec=$(kubectl get secrets -n "$ns" --no-headers 2>/dev/null | wc -l | xargs)
  echo "  $ns: ConfigMap=$cm Secret=$sec"
done
echo ""

echo "=== 8. 资源配额与限制 ==="
echo "--- ResourceQuota ---"
kubectl get resourcequota -A 2>/dev/null
echo ""
echo "--- LimitRange ---"
kubectl get limitrange -A 2>/dev/null
echo ""

echo "=== 9. Helm Release ==="
helm list -A 2>/dev/null || echo "helm 未安装或无 release"
echo ""

echo "=== 10. 镜像清单 ==="
echo "--- 使用的镜像列表 ---"
kubectl get pods -A -o jsonpath='{range .items[*]}{range .spec.containers[*]}{.image}{"\n"}{end}{end}' | sort -u
echo ""

echo "=========================================="
echo "  采集完成"
echo "=========================================="
```

**使用方式：**

```bash
# 在自建集群 Master 节点上运行
chmod +x migration-assessment.sh
bash migration-assessment.sh > assessment-report-$(date +%Y%m%d).txt

# 查看报告
less assessment-report-*.txt
```

### 1.2 关键信息汇总表

根据采集结果填写以下表格：

| 维度 | 自建集群现状 | ACK 对应方案 | 迁移难度 |
|------|------------|-------------|---------|
| **K8s 版本** | v1.2x.x (kubeadm) | ACK 支持 v1.24-v1.32 | 低（版本匹配即可） |
| **节点数** | ___ Master + ___ Worker | ACK 托管版 + ___ 节点池 | 低 |
| **CNI** | Calico / Flannel / Cilium | Terway (推荐) / Flannel | 中（需验证 NetworkPolicy） |
| **容器运行时** | Docker / containerd / CRI-O | containerd (ACK 默认) | 低 |
| **存储** | NFS / Ceph / Local PV | 云盘 CSI / NAS CSI / OSS CSI | 高（数据迁移） |
| **Ingress** | nginx-ingress / Traefik | nginx-ingress / Higress / ALB Ingress | 中（注解适配） |
| **监控** | Prometheus + Grafana | ARMS / 自建 Prometheus | 中（指标/告警迁移） |
| **日志** | EFK / Loki | SLS / 自建 EFK | 中（日志管道重配） |
| **CI/CD** | Jenkins / ArgoCD / GitLab CI | 保持不变 / ACR + ArgoCD | 低-中 |
| **镜像仓库** | Harbor / 自建 Registry | ACR (推荐) / 保留 Harbor | 低（镜像同步） |

---

## 2. 兼容性评估

### 2.1 API 版本兼容性检查

```bash
# 检查已弃用的 API 版本
# 安装 pluto (Fairwinds 出品的 API 弃用检测工具)
brew install FairwindsOps/tap/pluto

# 扫描集群中的弃用 API
pluto detect-all-in-cluster

# 预期输出示例:
# NAME                        KIND         VERSION              REPLACEMENT       REMOVED   DEPRECATED
# my-ingress                  Ingress      extensions/v1beta1   networking.k8s.io/v1   true      true
# my-psp                      PodSecurityPolicy  policy/v1beta1  N/A (removed)    true      true

# 针对 Helm release 检查
pluto detect-helm -A

# 导出需要修改的资源清单
pluto detect-all-in-cluster -o json > deprecated-apis.json
```

### 2.2 兼容性对照矩阵

| 组件/特性 | 自建集群 | ACK 兼容性 | 迁移动作 |
|----------|---------|-----------|---------|
| **PodSecurityPolicy (PSP)** | 可能使用 | ACK ≥1.25 已移除 PSP | 需迁移至 Pod Security Standards (PSS) |
| **Docker 运行时** | 可能使用 dockershim | ACK 使用 containerd | 确认镜像兼容性，去除 docker.sock 挂载 |
| **自定义 CRD** | [[domain-19-landscape-references/01-cncf-landscape/graduated/cert-manager/cert-manager|cert-manager]], prometheus-operator 等 | 需在 ACK 重新安装 CRD Controller | 确认 CRD 版本兼容 |
| **HostPath Volume** | 开发环境常用 | ACK 支持但不推荐 | 改为 云盘 CSI / NAS CSI |
| **NodePort Service** | 常用 | ACK 支持，但推荐 LoadBalancer | 迁移为 SLB/NLB 类型 |
| **特权容器** | 部分组件需要 | ACK 支持，需安全审计 | 逐个确认必要性 |
| **自定义调度器** | 可能部署 | ACK 支持 | 需重新部署 |
| **etcd 直接访问** | 部分工具直连 etcd | ACK 托管版不暴露 etcd | 需改为 API Server 接口 |

### 2.3 镜像仓库兼容性

```bash
# 导出所有镜像列表
kubectl get pods -A -o jsonpath='{range .items[*]}{range .spec.containers[*]}{.image}{"\n"}{end}{end}' | sort -u > images-list.txt

# 检查镜像是否可从 ACK 节点拉取
# 常见问题：
# 1. 使用 docker.io 国内可能超时 → 改用阿里云镜像加速器或推送到 ACR
# 2. 使用私有 Harbor → 配置 ACK ImagePullSecret 或迁移到 ACR
# 3. 使用 gcr.io/quay.io → 需要预拉取或同步到 ACR

# 批量同步镜像到 ACR 的脚本
cat images-list.txt | while read img; do
  # 生成 ACR 镜像名
  acr_img="registry.cn-hangzhou.aliyuncs.com/your-namespace/$(basename $img)"
  echo "docker pull $img && docker tag $img $acr_img && docker push $acr_img"
done > sync-images.sh
```

### 2.4 存储兼容性评估

| 自建存储方案 | ACK 替代方案 | 数据迁移方式 | 复杂度 |
|------------|------------|------------|--------|
| **NFS Server** | 阿里云 NAS (CSI) | rsync / NAS 数据迁移服务 | 中 |
| **Ceph RBD** | 阿里云 ESSD 云盘 (CSI) | rbd export → 云盘快照导入 | 高 |
| **CephFS** | 阿里云 NAS (CSI) | rsync / Ceph 导出 + NAS 导入 | 高 |
| **GlusterFS** | 阿里云 NAS (CSI) | rsync 同步 | 中 |
| **Local PV** | 阿里云 ESSD 云盘 (CSI) | tar + rsync → 云盘写入 | 中 |
| **hostPath** | 阿里云 ESSD / NAS | 手动数据复制 | 低 |
| **OpenEBS** | 阿里云 ESSD (CSI) | 快照 + 数据复制 | 中 |
| **Longhorn** | 阿里云 ESSD (CSI) | Longhorn 备份 → 恢复到云盘 | 中 |

---

## 3. 风险分析与应对

### 3.1 风险矩阵

| 风险项 | 概率 | 影响 | 等级 | 应对策略 |
|--------|------|------|------|---------|
| 数据丢失 | 低 | 极高 | **P0** | 迁移前全量备份，双写校验，保留源集群 7 天 |
| 业务中断 | 中 | 高 | **P0** | 灰度切流，10%→30%→50%→100% 逐步放量 |
| 网络不通 | 中 | 高 | **P1** | 提前打通 VPC 互联/专线，预留回退 DNS |
| 性能下降 | 中 | 中 | **P1** | 迁移前后性能基线对比，预留扩容余量 20% |
| 镜像拉取失败 | 中 | 中 | **P1** | 提前同步镜像到 ACR，配置镜像加速器 |
| 存储驱动不兼容 | 低 | 高 | **P1** | 提前测试 CSI 驱动，准备数据迁移方案 |
| Ingress 注解不兼容 | 高 | 中 | **P2** | 逐条映射注解，灰度验证 |
| RBAC 权限不匹配 | 中 | 中 | **P2** | 导出所有 RBAC 资源，在 ACK 重建并测试 |
| CRD 版本不兼容 | 低 | 中 | **P2** | 提前在 ACK 安装并验证 CRD Controller |
| 证书过期 | 低 | 中 | **P2** | 迁移前更新证书，ACK 使用 cert-manager |
| 迁移耗时超预期 | 中 | 低 | **P3** | 预留 50% 缓冲时间，分批迁移 |

### 3.2 回滚策略

```
迁移回滚决策树：

  问题发生
      │
      ▼
  影响范围评估
      │
  ┌───┴───────────┐
  │ 单个服务异常   │ 多个服务异常 / 数据不一致
  │               │
  ▼               ▼
  单服务回滚     全量回滚
  │               │
  ▼               ▼
  ① DNS 切回源集群  ① 停止 ACK 集群流量引入
  ② 排查并修复     ② DNS 全量切回源集群
  ③ 重新迁移该服务  ③ 验证源集群服务正常
                   ④ 根因分析
                   ⑤ 修复后重新迁移
```

**回滚执行 SOP：**

```bash
# 1. 立即将 DNS 切回源集群
# 修改 DNS 解析记录（以阿里云 DNS 为例）
aliyun alidns UpdateDomainRecord \
  --RecordId <record-id> \
  --RR www \
  --Type A \
  --Value <source-cluster-ingress-ip>

# 2. 验证源集群仍在正常工作
kubectl --context=source-cluster get pods -A | grep -v Running

# 3. 确认流量已回切
# 观察源集群 Ingress 访问日志恢复
kubectl --context=source-cluster logs -n ingress-nginx deploy/ingress-nginx-controller -f

# 4. 在 ACK 侧停止接收流量
kubectl --context=ack-cluster scale deploy --all --replicas=0 -n <business-ns>
```

---

## 4. 迁移策略选择

### 4.1 三种迁移策略对比

| 策略 | 双集群灰度（推荐） | 蓝绿切换 | 直接迁移 |
|------|------------------|---------|---------|
| **原理** | 新旧集群并行运行，DNS 权重逐步切流 | 新集群完全就绪后一次切换 | 停机后资源导出导入 |
| **停机时间** | 零停机 | < 30min（DNS 生效时间） | 1-4 小时 |
| **资源成本** | 高（双集群并行期间） | 高（新集群完全就绪） | 低 |
| **风险** | 最低（随时回滚） | 低（可回切 DNS） | 高（停机期间无法回滚） |
| **适用场景** | 生产环境、金融/电商 | 测试/预发环境 | 开发环境 |
| **实施复杂度** | 高 | 中 | 低 |
| **数据一致性** | 需处理双写 | 切换前保证源数据最新 | 停机保证一致 |

### 4.2 推荐策略：双集群灰度迁移

```
时间轴                                                          
──────────────────────────────────────────────────────────►
  │          │          │          │          │          │
  T0         T1         T2         T3         T4         T5
  准备完成    10% 切流   30% 切流   50% 切流   100% 切流  源集群退役
  │          │          │          │          │          │
  ▼          ▼          ▼          ▼          ▼          ▼
  双集群     观察       观察       观察       全量       下线
  就绪       24h        24h        24h        稳定 7d    源集群

各阶段检查项：
  T1 (10%): 核心接口 RT/错误率正常，日志无异常
  T2 (30%): 扩展接口验证，边缘 case 覆盖
  T3 (50%): 高峰期流量验证，资源水位正常
  T4 (100%): 全量切换，源集群保留但不接流量
  T5: 源集群停机退役
```

---

## 5. 迁移项目计划模板

### 5.1 里程碑计划

| 阶段 | 里程碑 | 持续时间 | 产出物 | 完成标准 |
|------|--------|---------|--------|---------|
| **Phase 0** | 迁移评估完成 | 1-2 周 | 评估报告、风险矩阵、迁移计划 | 团队评审通过 |
| **Phase 1** | ACK 集群就绪 | 1-2 周 | ACK 集群、网络打通、监控基线 | 集群健康检查全绿 |
| **Phase 2a** | 无状态服务迁移 | 1-2 周 | Deployment/Service 在 ACK 运行 | 所有无状态服务在 ACK 健康 |
| **Phase 2b** | 有状态服务迁移 | 1-2 周 | DB/缓存/MQ 在 ACK 就绪 | 数据校验通过 |
| **Phase 3** | 灰度切流完成 | 1-2 周 | 100% 流量在 ACK | 7 天稳定运行 |
| **Phase 4** | 源集群退役 | 1 周 | 退役报告、资源释放 | 确认无残留流量 |

### 5.2 RACI 矩阵

| 任务 | 迁移负责人 | 运维工程师 | DBA | 开发 | 网络工程师 |
|------|-----------|-----------|-----|------|-----------|
| 迁移评估 | **R** | C | C | I | C |
| ACK 集群搭建 | A | **R** | I | I | C |
| 无状态应用迁移 | A | **R** | I | C | I |
| 存储/数据迁移 | A | C | **R** | I | I |
| 网络/DNS 切换 | A | C | I | I | **R** |
| 功能验证 | A | C | C | **R** | I |
| 性能验证 | A | **R** | C | C | C |
| 灰度切流决策 | **R** | C | C | C | C |
| 旧集群退役 | **R** | **R** | C | I | C |

> R=Responsible, A=Accountable, C=Consulted, I=Informed

---

## 6. 成本估算

### 6.1 ACK 集群成本构成

| 费用项 | 计算方式 | 月估算（中型集群） | 备注 |
|--------|---------|-------------------|------|
| **ACK 集群管理费** | 托管版免费 / Pro 版按集群收费 | ¥0 (标准版) / ¥1500 (Pro) | Pro 版含高级调度、安全扫描 |
| **ECS 节点** | 按实例规格×数量×小时 | ¥8,000-30,000 | 取决于节点数和规格 |
| **云盘 (ESSD)** | 按容量 GB × 月 | ¥500-3,000 | PL0/PL1/PL2/PL3 不同价位 |
| **NAS 存储** | 按存储量 + 吞吐 | ¥500-2,000 | 通用型/极速型 |
| **SLB/NLB** | 按实例 + 带宽/流量 | ¥200-1,000 | 公网带宽另计 |
| **公网带宽** | 按固定带宽或按流量 | ¥500-5,000 | 取决于业务流量 |
| **ACR 镜像仓库** | 个人版免费 / 企业版按规格 | ¥0-680 | 企业版支持镜像安全扫描 |
| **SLS 日志** | 按写入量 + 存储量 | ¥200-1,000 | 30 天保留 |
| **ARMS 监控** | 按 Agent 数量 | ¥0-2,000 | 基础版免费 |

### 6.2 迁移期间额外成本

```
迁移期间成本 = 正常 ACK 运行成本 + 源集群运行成本（并行期间）

预估并行期间: 2-4 周
额外成本 ≈ 源集群月成本 × (并行周数 / 4)

建议:
- 迁移期间源集群不缩容，保留完整回滚能力
- ACK 集群初始可略小于源集群，灰度放量后逐步扩容
- 利用阿里云按量付费或抢占式实例降低迁移期间成本
```

---

## 检查清单

### Phase 0 完成标准

- [ ] 集群现状采集脚本已运行，报告已生成
- [ ] 兼容性评估表已填写，所有不兼容项已标注
- [ ] 弃用 API 已扫描，修复计划已制定
- [ ] 镜像列表已导出，ACR 同步方案已确认
- [ ] 存储方案映射表已确认
- [ ] 风险矩阵已评审，所有 P0/P1 风险有应对方案
- [ ] 迁移策略已选择（推荐双集群灰度）
- [ ] 项目计划已制定，RACI 已明确
- [ ] 成本估算已完成，预算已审批
- [ ] 回滚方案已文档化，全团队知悉

---

**下一步**: → [02-ACK 目标集群设计](./02-ack-target-cluster-design.md)

## Related

- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
