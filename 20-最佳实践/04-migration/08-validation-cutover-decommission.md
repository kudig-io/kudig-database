---
title: 08 - 验收、切换与旧集群退役 [migration]
description: 'description: 5. [旧集群安全退役](#5-旧集群安全退役)'
summary: 'description: 5. [旧集群安全退役](#5-旧集群安全退役)'
category: general
tags:
- migration
- upgrade
- prometheus
- grafana
- coredns
- hpa
- statefulset
- job
- cronjob
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 验收、切换与旧集群退役 是什么
- 如何 验收、切换与旧集群退役
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 验收
- 切换与旧集群退役
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- tls-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 08 - 验收、切换与旧集群退役
description: 5. [旧集群安全退役](#5-旧集群安全退役)
category: migration
tags:
- k8s
- migration
- modernization
- [[Prometheus|prometheus]]
- grafana
- [[CoreDNS|coredns]]
- hpa
- [[StatefulSet|statefulset]]
- job
- [[CronJob|cronjob]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 验收、切换与旧集群退役 是什么
- 如何 验收、切换与旧集群退役
trigger_keywords:
- 验收
- 切换与旧集群退役
- migration
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 08 - 验收、切换与旧集群退役

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: 功能验证, 性能测试, 全量切换, SOP, 旧集群退役, 资源释放

---

<!-- chunk: 目录 -->## 目录

1. [功能验证清单](#1-功能验证清单)
2. [性能对比验证](#2-性能对比验证)
3. [全量切换 SOP](#3-全量切换-sop)
4. [稳定性观察期](#4-稳定性观察期)
5. [旧集群安全退役](#5-旧集群安全退役)
6. [迁移复盘](#6-迁移复盘)

---

<!-- chunk: 1. 功能验证清单 -->## 1. 功能验证清单

## 1.1 自动化验证脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# full-validation.sh
# ACK 迁移后全面功能验证

ACK_CONTEXT="ack-cluster"
REPORT_FILE="validation-report-$(date +%Y%m%d-%H%M%S).txt"

exec > >(tee -a $REPORT_FILE) 2>&1

echo "=============================================="
echo "  ACK 迁移功能验证报告"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

PASS=0
FAIL=0
WARN=0

check() {
  local name=$1
  local cmd=$2
  echo -n "  [$name] ... "
  if eval "$cmd" > /dev/null 2>&1; then
    echo "PASS"
    ((PASS++))
  else
    echo "FAIL"
    ((FAIL++))
  fi
}

echo ""
echo "=== 1. 基础设施 ==="
check "所有节点 Ready" "kubectl --context=$ACK_CONTEXT get nodes --no-headers | grep -v Ready | wc -l | grep -q '^0$'"
check "kube-system Pod 正常" "test \$(kubectl --context=$ACK_CONTEXT get pods -n kube-system --no-headers | grep -cvE 'Running|Completed') -eq 0"
check "CoreDNS 正常" "kubectl --context=$ACK_CONTEXT get pods -n kube-system -l k8s-app=kube-dns --no-headers | grep -q Running"
check "CSI 驱动正常" "kubectl --context=$ACK_CONTEXT get pods -n kube-system | grep -q csi-plugin"

echo ""
echo "=== 2. 工作负载 ==="
check "所有 Deployment 就绪" "test \$(kubectl --context=$ACK_CONTEXT get deploy -A --no-headers | awk '\$3!=\$4' | grep -cv kube-system) -eq 0"
check "所有 StatefulSet 就绪" "test \$(kubectl --context=$ACK_CONTEXT get sts -A --no-headers | awk '\$2!=\$3' | grep -cv kube-system) -eq 0"
check "无 CrashLoopBackOff Pod" "test \$(kubectl --context=$ACK_CONTEXT get pods -A --no-headers | grep CrashLoopBackOff | wc -l) -eq 0"
check "无 ImagePullBackOff Pod" "test \$(kubectl --context=$ACK_CONTEXT get pods -A --no-headers | grep ImagePullBackOff | wc -l) -eq 0"
check "无 Pending Pod" "test \$(kubectl --context=$ACK_CONTEXT get pods -A --no-headers | grep Pending | wc -l) -eq 0"

echo ""
echo "=== 3. 网络 ==="
check "Ingress Controller 运行" "kubectl --context=$ACK_CONTEXT get pods -n kube-system -l app.kubernetes.io/name=ingress-nginx --no-headers | grep -q Running"
check "Ingress 外部 IP 已分配" "kubectl --context=$ACK_CONTEXT get svc -n kube-system -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].status.loadBalancer.ingress[0].ip}' | grep -qE '[0-9]+\.[0-9]+'"
check "所有 LB Service 有 IP" "test \$(kubectl --context=$ACK_CONTEXT get svc -A --no-headers | grep LoadBalancer | grep -c '<pending>') -eq 0"

echo ""
echo "=== 4. 存储 ==="
check "所有 PVC 已绑定" "test \$(kubectl --context=$ACK_CONTEXT get pvc -A --no-headers | grep -cv Bound) -eq 0"

echo ""
echo "=== 5. 监控 ==="
check "Prometheus 运行" "kubectl --context=$ACK_CONTEXT get pods -n monitoring -l app.kubernetes.io/name=prometheus --no-headers 2>/dev/null | grep -q Running || echo skip"
check "Grafana 运行" "kubectl --context=$ACK_CONTEXT get pods -n monitoring -l app.kubernetes.io/name=grafana --no-headers 2>/dev/null | grep -q Running || echo skip"

echo ""
echo "=== 6. 安全 ==="
check "NetworkPolicy 已应用" "test \$(kubectl --context=$ACK_CONTEXT get networkpolicies -A --no-headers 2>/dev/null | wc -l) -gt 0 || echo skip"

echo ""
echo "=============================================="
echo "  总结: PASS=$PASS FAIL=$FAIL WARN=$WARN"
echo "  报告已保存: $REPORT_FILE"
echo "=============================================="

if [ $FAIL -gt 0 ]; then
  echo "  *** 存在失败项，请排查后再进行切流 ***"
  exit 1
fi
```
## 1.2 业务接口验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 ACK Ingress IP 直接测试业务接口（绕过 DNS）
ACK_INGRESS_IP=$(kubectl --context=ack-cluster get svc -n kube-system \
  -l app.kubernetes.io/name=ingress-nginx \
  -o jsonpath='{.items[0].status.loadBalancer.ingress[0].ip}')

# 测试核心接口
echo "=== 核心接口验证 ==="
endpoints=(
  "api.example.com /health 200"
  "api.example.com /api/v1/users 200"
  "api.example.com /api/v1/orders 200"
  "www.example.com / 200"
)

for ep in "${endpoints[@]}"; do
  host=$(echo $ep | awk '{print $1}')
  path=$(echo $ep | awk '{print $2}')
  expected=$(echo $ep | awk '{print $3}')
  actual=$(curl -s -o /dev/null -w "%{http_code}" -H "Host: $host" "http://$ACK_INGRESS_IP$path")
  if [ "$actual" = "$expected" ]; then
    echo "  PASS: $host$path → $actual"
  else
    echo "  FAIL: $host$path → $actual (expected $expected)"
  fi
done
```
---

<!-- chunk: 2. 性能对比验证 -->## 2. 性能对比验证

## 2.1 压测对比

```bash
# 使用 wrk 进行压测对比

# 源集群压测
echo "=== 源集群压测 ==="
wrk -t4 -c100 -d60s -H "Host: api.example.com" \
  http://<source-ingress-ip>/api/v1/health

# ACK 集群压测（相同参数）
echo "=== ACK 集群压测 ==="
wrk -t4 -c100 -d60s -H "Host: api.example.com" \
  http://$ACK_INGRESS_IP/api/v1/health

# 关注指标:
# - Requests/sec (QPS)
# - Latency Avg/P99
# - Transfer/sec
# - Non-2xx or 3xx responses

# ACK 性能应不低于源集群的 90%
# 如果差距较大，检查:
# 1. 节点规格是否匹配
# 2. Pod 资源 requests/limits 是否合理
# 3. HPA 是否正常工作
# 4. Terway 网络模式性能
```

## 2.2 资源水位对比

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 双集群资源对比
echo "=== 源集群资源 ==="
kubectl --context=source-cluster top nodes
echo ""
echo "=== ACK 集群资源 ==="
kubectl --context=ack-cluster top nodes

# Pod 资源对比
echo "=== 源集群 Top 10 CPU Pod ==="
kubectl --context=source-cluster top pods -A --sort-by=cpu | head -11

echo "=== ACK Top 10 CPU Pod ==="
kubectl --context=ack-cluster top pods -A --sort-by=cpu | head -11
```
---

<!-- chunk: 3. 全量切换 SOP -->## 3. 全量切换 SOP

## 3.1 切换前检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# pre-cutover-check.sh
# 全量切换前检查清单

echo "=============================================="
echo "  全量切换前检查 - $(date)"
echo "=============================================="

READY=true

# 1. ACK 集群健康
echo ">>> 1. ACK 集群健康检查"
UNHEALTHY=$(kubectl --context=ack-cluster get pods -A --no-headers | grep -cvE "Running|Completed")
echo "  非健康 Pod 数: $UNHEALTHY"
[ $UNHEALTHY -gt 0 ] && READY=false

# 2. 灰度期间无异常
echo ">>> 2. 灰度期间错误率"
echo "  请确认 Grafana 监控面板显示: 错误率 < 0.1%"

# 3. 数据一致性
echo ">>> 3. 数据一致性"
echo "  请确认最新一次数据校验通过"

# 4. 回滚方案就绪
echo ">>> 4. 回滚方案"
echo "  源集群仍在运行: $(kubectl --context=source-cluster get nodes --no-headers | wc -l | xargs) 个节点"
echo "  DNS TTL: 请确认为 60s"

# 5. 团队就位
echo ">>> 5. 团队确认"
echo "  运维工程师: [ ]"
echo "  DBA: [ ]"
echo "  开发负责人: [ ]"
echo "  网络工程师: [ ]"

if [ "$READY" = true ]; then
  echo ""
  echo "  *** 前置检查通过，可以执行全量切换 ***"
else
  echo ""
  echo "  *** 前置检查未通过，请排查后重试 ***"
fi
```
## 3.2 全量切换执行

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# execute-full-cutover.sh
# 全量切换执行脚本

set -euo pipefail

echo "=============================================="
echo "  全量流量切换至 ACK"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

# 确认执行
read -p "确认执行全量切换？(yes/no): " confirm
[ "$confirm" != "yes" ] && echo "取消执行" && exit 0

# Step 1: DNS 全量切换
echo ">>> Step 1: DNS 全量切换到 ACK"
# 方案 A: 直接修改 DNS 记录
aliyun alidns UpdateDomainRecord --RecordId <ack-record-id> --RR api --Type A --Value $ACK_INGRESS_IP --Weight 10
aliyun alidns UpdateDomainRecord --RecordId <source-record-id> --RR api --Type A --Value <source-ip> --Weight 0

# 对所有域名执行（可配置为列表）
DOMAINS=("api.example.com" "www.example.com" "admin.example.com")
for domain in "${DOMAINS[@]}"; do
  echo "  切换: $domain → ACK"
  # 实际 aliyun dns 命令...
done

echo ">>> Step 2: 等待 DNS 生效 (60s TTL)"
sleep 70

# Step 3: 验证流量已切换
echo ">>> Step 3: 验证"
echo "  源集群 Ingress 请求量（应趋近于 0）:"
kubectl --context=source-cluster logs -n ingress-nginx deploy/ingress-nginx-controller --tail=5 --since=1m 2>/dev/null | wc -l

echo "  ACK Ingress 请求量:"
kubectl --context=ack-cluster logs -n kube-system deploy/nginx-ingress-controller --tail=5 --since=1m 2>/dev/null | wc -l

echo ""
echo "=============================================="
echo "  全量切换完成"
echo "  源集群保留运行，7 天后执行退役"
echo "=============================================="
```
---

<!-- chunk: 4. 稳定性观察期 -->## 4. 稳定性观察期

## 4.1 观察清单（7 天）

| 天数 | 检查项 | 预期 |
|------|--------|------|
| Day 1 | 错误率、RT P99、Pod 重启次数 | 与灰度期一致 |
| Day 2 | 自动扩缩是否正常（高峰期） | HPA 正常触发 |
| Day 3 | 有状态服务数据增长正常 | 磁盘使用率无异常 |
| Day 4 | CronJob 执行记录 | 所有 CronJob 按时执行 |
| Day 5 | 证书续期（如有到期） | cert-manager 自动续期 |
| Day 6 | 日志采集完整性 | SLS/EFK 日志无缺失 |
| Day 7 | 整体回顾 | 可执行退役 |

## 4.2 每日巡检脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# daily-patrol.sh
echo "=== ACK 每日巡检 $(date +%Y-%m-%d) ==="

echo ">>> 节点状态"
kubectl --context=ack-cluster get nodes

echo ">>> 异常 Pod"
kubectl --context=ack-cluster get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded | grep -v NAMESPACE

echo ">>> Warning 事件 (过去 24h)"
kubectl --context=ack-cluster get events -A --field-selector type=Warning --sort-by=.lastTimestamp | tail -20

echo ">>> Pod 重启次数 > 0"
kubectl --context=ack-cluster get pods -A -o json | jq -r '
  .items[] | select(.status.containerStatuses[]?.restartCount > 0) |
  .metadata.namespace + "/" + .metadata.name + " restarts=" +
  (.status.containerStatuses[].restartCount | tostring)
' | head -20

echo ">>> PVC 使用率 (需 metrics-server)"
kubectl --context=ack-cluster top pods -A --sort-by=memory | head -11

echo ">>> 巡检完成"
```
---

<!-- chunk: 5. 旧集群安全退役 -->## 5. 旧集群安全退役

## 5.1 退役前确认

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# pre-decommission-check.sh

echo "=== 旧集群退役前检查 ==="

# 1. 确认无流量
echo ">>> 1. 源集群流量检查"
echo "  Ingress 日志 (最近 1h):"
kubectl --context=source-cluster logs -n ingress-nginx deploy/ingress-nginx-controller --since=1h 2>/dev/null | wc -l
echo "  如果 > 0，说明仍有流量残留，不要退役！"

# 2. 确认 DNS 已切换
echo ">>> 2. DNS 解析检查"
for domain in api.example.com www.example.com; do
  resolved=$(dig +short $domain)
  echo "  $domain → $resolved"
  echo "  (应为 ACK Ingress IP: $ACK_INGRESS_IP)"
done

# 3. 最后一次备份
echo ">>> 3. 创建最终备份"
echo "  建议使用 Velero 创建源集群最终快照"
echo "  velero backup create final-backup-$(date +%Y%m%d) --kubecontext source-cluster"
```
## 5.2 退役执行

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# decommission-source-cluster.sh

echo "=============================================="
echo "  源集群退役"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

read -p "确认退役源集群？此操作不可逆！(type DECOMMISSION): " confirm
[ "$confirm" != "DECOMMISSION" ] && echo "取消" && exit 0

# Step 1: 最终备份
echo ">>> Step 1: 最终备份"
velero backup create final-backup-$(date +%Y%m%d) \
  --include-namespaces '*' \
  --default-volumes-to-fs-backup \
  --kubecontext source-cluster
echo "  等待备份完成..."
velero backup wait final-backup-$(date +%Y%m%d) --kubecontext source-cluster

# Step 2: 停止工作负载
echo ">>> Step 2: 停止所有工作负载"
for ns in $(kubectl --context=source-cluster get ns --no-headers -o custom-columns=:metadata.name | grep -vE "^kube-"); do
  kubectl --context=source-cluster scale deploy --all --replicas=0 -n $ns 2>/dev/null
  kubectl --context=source-cluster scale sts --all --replicas=0 -n $ns 2>/dev/null
done

# Step 3: 等待确认无影响
echo ">>> Step 3: 等待 30 分钟确认 ACK 无异常"
echo "  请检查 ACK 集群监控面板..."
sleep 1800

# Step 4: 记录资源清单
echo ">>> Step 4: 记录源集群资源清单（存档）"
kubectl --context=source-cluster get all -A > source-cluster-final-inventory.txt
kubectl --context=source-cluster get nodes -o wide >> source-cluster-final-inventory.txt

# Step 5: 关机节点
echo ">>> Step 5: 关停源集群节点"
echo "  请手动执行节点关机或 VM 删除"
echo "  建议保留磁盘快照 30 天"

echo ""
echo "=============================================="
echo "  源集群退役完成"
echo "  备份位置: Velero + OSS"
echo "  资源清单: source-cluster-final-inventory.txt"
echo "=============================================="
```
---

<!-- chunk: 6. 迁移复盘 -->## 6. 迁移复盘

## 6.1 复盘模板

```
迁移复盘报告
═══════════════════════════════════════
项目: 自建 K8s → 阿里云 ACK 迁移
日期: YYYY-MM-DD
参与人: ...

一、迁移概况
  - 源集群: X 节点, Y 个服务, Z TB 数据
  - 目标集群: ACK Pro, X 节点池
  - 总耗时: X 周
  - 停机时间: 0 (零停机迁移)

二、达成目标
  [✓] 所有业务服务迁移至 ACK
  [✓] 数据完整性校验通过
  [✓] 性能不低于源集群
  [✓] 监控/日志/告警体系完整
  [✓] 零业务中断

三、遇到的问题与解决方案
  1. 问题: ...
     解决: ...
  2. 问题: ...
     解决: ...

四、经验总结
  做得好的:
  - ...
  需要改进的:
  - ...

五、后续优化建议
  - ...
```

---

<!-- chunk: 检查清单 -->## 检查清单

- [ ] 功能验证脚本全部通过
- [ ] 核心业务接口验证通过
- [ ] 性能压测对比合格
- [ ] 全量切换 SOP 已执行
- [ ] 7 天稳定性观察完成
- [ ] 源集群最终备份已完成
- [ ] 源集群已退役
- [ ] 迁移复盘已完成

---

**上一步**: ← [07-可观测性与安全迁移](./07-observability-security-migration.md)
**下一步**: → [09-迁移工具链参考](./09-migration-toolchain.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration MOC
- [[11-发布变更/07-迁移方案/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[11-发布变更/07-迁移方案/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[11-发布变更/07-迁移方案/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[11-发布变更/07-迁移方案/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[11-发布变更/07-迁移方案/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[11-发布变更/07-迁移方案/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]]
- [[11-发布变更/07-迁移方案/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- [[11-发布变更/07-迁移方案/07-observability-security-migration.md|07 - 可观测性与安全迁移]]
- [[11-发布变更/07-迁移方案/09-migration-toolchain.md|09 - 迁移工具链参考]]
- [[11-发布变更/07-迁移方案/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 06-stateful-services-migration
- 07-observability-security-migration
- 09-migration-toolchain
- 10-real-world-case-study


<!-- risk-assessed -->
