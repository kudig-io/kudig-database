---
title: Kubernetes v1.33 弃用功能与迁移指南
description: '# Kubernetes v1.33 弃用功能与迁移指南'
summary: 'kubectl get psp restricted -o yaml > psp-restricted.yaml'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- prometheus
- hpa
- vpa
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.33 弃用功能与迁移指南 是什么
- 如何 Kubernetes v1.33 弃用功能与迁移指南
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.33
- 弃用功能与迁移指南
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---



# [[Kubernetes|Kubernetes]] v1.33 弃用功能与迁移指南

> **适用版本**: Kubernetes v1.25 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 弃用功能识别、迁移操作、兼容性保障

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、弃用与移除总览](#一弃用与移除总览)
- [二、已移除功能 (v1.25-v1.33)](#二已移除功能-v1.25-v1.33)
- [三、已弃用功能 (即将移除)](#三已弃用功能-即将移除)
- [四、迁移操作指南](#四迁移操作指南)
- [五、自动化检测脚本](#五自动化检测脚本)
- [六、兼容性保障检查清单](#六兼容性保障检查清单)

---

<!-- chunk: 一、弃用与移除总览 -->
## 一、弃用与移除总览

```
Kubernetes 弃用政策
├── API 弃用: 1 年或 1 个发布周期 (以较长者为准)
├── 功能弃用: 根据影响范围评估
├── 标志弃用: 2 个发布周期后移除
└── 行为变更: 在发布说明中公告

v1.25 → v1.33 移除/弃用时间线
    │
    ├── v1.25: PodSecurityPolicy 移除, CronJob v1beta1 移除
    ├── v1.26: flowcontrol.apiserver.k8s.io/v1beta1 移除
    ├── v1.27: CSIStorageCapacity v1beta1 移除
    ├── v1.28: 多个 Beta API 升级
    ├── v1.29: flowcontrol.apiserver.k8s.io/v1beta2 移除, Node v1beta1 metrics 弃用
    ├── v1.30: in-tree storage drivers 弃用
    ├── v1.31: kubelet --cloud-provider flag 弃用
    ├── v1.32: 多个 Beta API 升级
    └── v1.33: 持续清理旧 API
```

---

<!-- chunk: 二、已移除功能 (v1.25-v1.33) -->
## 二、已移除功能 (v1.25-v1.33)

### 2.1 已移除 API

| API | 移除版本 | 替代方案 | 紧急程度 |
|:---|:---|:---|:---|
| PodSecurityPolicy (policy/v1beta1) | **v1.25** | Pod Securityod Security Admission]] | 🔴 已移除 |
| CronJob (batch/v1beta1) | **v1.25** | batch/v1 | 🔴 已移除 |
| EndpointSlice (discovery.k8s.io/v1beta1) | **v1.25** | discovery.k8s.io/v1 | 🔴 已移除 |
| Event (events.k8s.io/v1beta1) | **v1.25** | events.k8s.io/v1 | 🔴 已移除 |
| HorizontalPodAutoscaler (autoscaling/v2beta1) | **v1.25** | autoscaling/v2 | 🔴 已移除 |
| PodDisruptionBudget (policy/v1beta1) | **v1.25** | policy/v1 | 🔴 已移除 |
| RuntimeClass (node.k8s.io/v1beta1) | **v1.25** | node.k8s.io/v1 | 🔴 已移除 |
| FlowSchema (flowcontrol.apiserver.k8s.io/v1beta1) | **v1.26** | flowcontrol.apiserver.k8s.io/v1 | 🔴 已移除 |
| PriorityLevelConfiguration (flowcontrol.apiserver.k8s.io/v1beta1) | **v1.26** | flowcontrol.apiserver.k8s.io/v1 | 🔴 已移除 |
| CSIStorageCapacity (storage.k8s.io/v1beta1) | **v1.27** | storage.k8s.io/v1 | 🔴 已移除 |
| FlowSchema (flowcontrol.apiserver.k8s.io/v1beta2) | **v1.29** | flowcontrol.apiserver.k8s.io/v1 | 🔴 已移除 |

### 2.2 已弃用功能

| 功能 | 弃用版本 | 预计移除 | 替代方案 | 状态 |
|:---|:---|:---|:---|:---|
| Node v1beta1 metrics | v1.29 | v1.34+ | metrics/v1 | 🟡 需迁移 |
| in-tree storage drivers | v1.30 | v1.35+ | CSI 驱动 | 🟡 需迁移 |
| kubelet --cloud-provider flag | v1.31 | v1.35+ | 外部 CCM | 🟡 需迁移 |
| kube-proxy iptables 模式 | - | - | nftables / IPVS | 🟢 可选迁移 |
| kubeadm v1beta2 | - | - | v1beta3 / v1 | 🟡 建议迁移 |

### 2.3 已弃用标志

| 标志 | 组件 | 弃用版本 | 替代方案 |
|:---|:---|:---|:---|
| `--cloud-provider` | kubelet | v1.31 | 外部云控制器管理器 |
| `--master-service-namespace` | kube-apiserver | v1.32 | 无替代，行为变更 |
| `--service-account-extend-token-expiration` | kube-apiserver | v1.30 | 默认启用，无需配置 |

---

<!-- chunk: 三、已弃用功能 (即将移除) -->
## 三、已弃用功能 (即将移除)

### 3.1 v1.34+ 预计移除

| 功能 | 当前状态 | 预计移除 | 影响 | 迁移复杂度 |
|:---|:---|:---|:---|:---|
| Node v1beta1 metrics | 弃用 (v1.29) | v1.34 | 监控查询需更新 | 低 |
| PodSecurityPolicy (已移除) | 已移除 | - | 需迁移到 PSA | 中 |
| in-tree storage drivers | 弃用 (v1.30) | v1.35+ | 需安装 CSI 驱动 | 高 |

### 3.2 行为变更预警

| 变更 | 版本 | 影响 | 应对措施 |
|:---|:---|:---|:---|
| 默认启用 Sidecar 容器 | v1.33 | init 容器支持 restartPolicy | 检查现有 Pod 兼容性 |
| ServiceAccount Token 1h 过期 | v1.30 GA | 长期 Token 失效 | 使用 BoundSA Token |
| 匿名用户禁止绑定 cluster-admin | v1.30 | 安全加固 | 检查 RBAC 绑定 |
| Parallel Image Pulls 默认启用 | v1.31 | 镜像拉取行为变化 | 无需操作 (优化) |

---

<!-- chunk: 四、迁移操作指南 -->
## 四、迁移操作指南

### 4.1 PodSecurityPolicy → Pod Security Admission

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 1. 检查现有 PSP
kubectl get psp

# 2. 导出 PSP 策略
kubectl get psp restricted -o yaml > psp-restricted.yaml

# 3. 创建等效的 PSA 标签
kubectl label namespace production \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/enforce-version=v1.33 \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 4. 验证
kubectl auth can-i use podsecuritypolicies --as=system:serviceaccount:default:default
# 预期: no (PSP 已移除)
```

### 4.2 in-tree 存储驱动 → CSI

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 1. 检查当前存储类
kubectl get sc

# 2. 确认 CSI 驱动已安装
kubectl get csidrivers

# 3. 创建新的 CSI StorageClass
cat <<EOF | kubectl apply -f -
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: csi-gp3
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  encrypted: "true"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
EOF

# 4. 设置为默认
kubectl patch storageclass csi-gp3 -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 5. 迁移现有 PVC (需重建)
# 注意: 无法直接迁移，需创建新 PVC 并复制数据
```

### 4.3 kubelet --cloud-provider → 外部 CCM

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 检查当前 kubelet 配置
ps aux | grep kubelet | grep cloud-provider

# 2. 部署外部云控制器管理器
# AWS
kubectl apply -k github.com/kubernetes/cloud-provider-aws/manifests/base

# 3. 更新 kubelet 配置
# 移除 --cloud-provider=aws
# 重启 kubelet
systemctl restart kubelet

# 4. 验证 CCM 运行
kubectl get pods -n kube-system | grep cloud-controller
```

### 4.4 Node v1beta1 metrics → v1

```bash
# 1. 检查 Prometheus 规则
grep -r "v1beta1/metrics" /etc/prometheus/rules/

# 2. 更新查询
# 旧: /apis/metrics.k8s.io/v1beta1/nodes
# 新: /apis/metrics.k8s.io/v1/nodes (kubectl top 已自动使用)

# 3. 验证
kubectl top nodes
kubectl top pods
```

---

<!-- chunk: 五、自动化检测脚本 -->
## 五、自动化检测脚本

### 5.1 全面弃用检测

```bash
#!/bin/bash
# deprecation-check.sh

echo "=== Kubernetes 弃用功能检测 ==="

# 1. 检查已弃用 API 使用
echo -e "\n[1/6] 已弃用 API 使用:"
kubectl get --raw /metrics 2>/dev/null | grep apiserver_requested_deprecated_apis || echo "无已弃用 API"

# 2. 检查旧版 API 资源
echo -e "\n[2/6] 旧版 API 资源:"
for api in "extensions/v1beta1" "apps/v1beta1" "apps/v1beta2" "batch/v1beta1" "policy/v1beta1"; do
  count=$(kubectl api-resources --api-group=$(echo $api | cut -d/ -f1) --verbs=list -o name 2>/dev/null | wc -l)
  if [ $count -gt 0 ]; then
    echo "  ⚠️  $api 仍被使用 ($count 资源)"
  fi
done

# 3. 检查 PSP (已移除)
echo -e "\n[3/6] PodSecurityPolicy (已移除 v1.25):"
if kubectl get psp &>/dev/null; then
  echo "  🔴 仍有 PSP 存在，需迁移到 PSA"
  kubectl get psp
else
  echo "  ✅ 无 PSP (已迁移)"
fi

# 4. 检查 kubelet cloud-provider
echo -e "\n[4/6] kubelet cloud-provider flag (弃用 v1.31):"
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  provider=$(kubectl get --raw /api/v1/nodes/$node/proxy/configz 2>/dev/null | \
    jq -r '.kubeletconfig.cloudProvider // "external/null"')
  if [ "$provider" != "external" ] && [ "$provider" != "null" ] && [ "$provider" != "" ]; then
    echo "  ⚠️  $node: cloud-provider=$provider (需迁移到 CCM)"
  fi
done

# 5. 检查 CSI 驱动
echo -e "\n[5/6] CSI 驱动 (in-tree 已弃用 v1.30):"
kubectl get csidrivers 2>/dev/null || echo "  ⚠️ 无 CSI 驱动 (需安装)"

# 6. 检查 ValidatingWebhook (可迁移到 CEL)
echo -e "\n[6/6] ValidatingWebhook (可迁移到 ValidatingAdmissionPolicy v1.30 GA):"
webhooks=$(kubectl get validatingwebhookconfigurations -o jsonpath='{.items[*].metadata.name}' 2>/dev/null)
if [ -n "$webhooks" ]; then
  echo "  ℹ️  现有 Webhook: $webhooks"
  echo "  建议: 评估是否可迁移到 CEL Admission"
else
  echo "  ✅ 无 ValidatingWebhook"
fi

echo -e "\n=== 检测完成 ==="
```

### 5.2 自动修复脚本

```bash
#!/bin/bash
# auto-fix.sh

# 修复 1: 将旧版 API 转换为新版
convert_api() {
  local file=$1
  sed -i 's|apiVersion: extensions/v1beta1|apiVersion: apps/v1|g' $file
  sed -i 's|apiVersion: apps/v1beta1|apiVersion: apps/v1|g' $file
  sed -i 's|apiVersion: apps/v1beta2|apiVersion: apps/v1|g' $file
  sed -i 's|apiVersion: batch/v1beta1|apiVersion: batch/v1|g' $file
  echo "已转换: $file"
}

# 批量转换目录
for f in $(find . -name "*.yaml" -o -name "*.yml"); do
  if grep -q "extensions/v1beta1|apps/v1beta1|apps/v1beta2|batch/v1beta1" $f 2>/dev/null; then
    convert_api $f
  fi
done
```

---

<!-- chunk: 六、兼容性保障检查清单 -->
## 六、兼容性保障检查清单

### 6.1 升级前检查

- [ ] 所有 YAML 使用 v1 API (无 v1beta1/v1beta2)
- [ ] PSP 已迁移到 PSA
- [ ] CSI 驱动已安装 (替代 in-tree)
- [ ] CCM 已部署 (kubelet 无 --cloud-provider)
- [ ] 监控查询使用 metrics/v1
- [ ] 无已弃用 API 使用
- [ ] ValidatingWebhook 评估迁移到 CEL
- [ ] 所有节点容器运行时 ≥ 1.7.18
- [ ] [[etcd|etcd]] ≥ 3.5.15
- [ ] CNI 插件 ≥ 推荐版本

### 6.2 升级后验证

- [ ] 所有 Pod 正常运行
- [ ] 存储挂载正常
- [ ] 网络连通正常
- [ ] 监控数据采集正常
- [ ] HPA/VPA 工作正常
- [ ] 新特性可按需启用
- [ ] 回滚方案可用

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s API 弃用指南](https://kubernetes.io/docs/reference/using-api/deprecation-guide/)
- [K8s 移除公告](https://kubernetes.io/blog/)
- [PSP 迁移指南](https://kubernetes.io/docs/tasks/configure-pod-container/migrate-from-psp/)
- [CSI 迁移](https://kubernetes.io/docs/concepts/storage/volumes/#csi-migration)
- [Cloud Controller Manager](https://kubernetes.io/docs/tasks/administer-cluster/running-cloud-controller/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubernetes-v1.29-v1.33-complete-feature-gates-reference
- 99-kubernetes-v1.29-v1.33-features-guide
- 99-kubernetes-v1.33-ecosystem-compatibility-matrix
- 99-kubernetes-v1.33-practical-cookbook
