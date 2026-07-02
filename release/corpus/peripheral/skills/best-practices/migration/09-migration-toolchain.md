---
title: 09 - 迁移工具链参考 [migration]
description: 'description: 2. [Velero 完整指南](#2-velero-完整指南)'
summary: 'description: 2. [Velero 完整指南](#2-velero-完整指南)'
category: general
tags:
- migration
- upgrade
- helm
- docker
- harbor
- redis
- mysql
- statefulset
- daemonset
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 迁移工具链参考 是什么
- 如何 迁移工具链参考
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 迁移工具链参考
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- redis-basics
- mysql-basics
- backup-basics
---



title: 09 - 迁移工具链参考
description: 2. [Velero 完整指南](#2-velero-完整指南)
category: migration
tags:
- k8s
- migration
- modernization
- [[Helm|helm]]
- docker
- [[Harbor|harbor]]
- redis
- mysql
- [[StatefulSet|statefulset]]
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 迁移工具链参考 是什么
- 如何 迁移工具链参考
trigger_keywords:
- 迁移工具链参考
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

# 09 - 迁移工具链参考

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: Velero, skopeo, kubectl-neat, yq, jq, pluto, 自动化脚本

---

## 目录

1. [工具总览](#1-工具总览)
2. [Velero 完整指南](#2-velero-完整指南)
3. [镜像同步工具](#3-镜像同步工具)
4. [YAML 处理工具](#4-yaml-处理工具)
5. [兼容性检查工具](#5-兼容性检查工具)
6. [数据迁移工具](#6-数据迁移工具)
7. [迁移脚本集合](#7-迁移脚本集合)

---

## 1. 工具总览

| 工具 | 用途 | 安装方式 | 阶段 |
|------|------|---------|------|
| **velero** | K8s 资源 + 数据备份恢复 | `brew install velero` | 全程 |
| **skopeo** | 镜像仓库间复制 | `brew install skopeo` | 镜像迁移 |
| **kubectl-neat** | 清洗 YAML 元数据 | `kubectl krew install neat` | 资源导出 |
| **yq** | YAML 处理/转换 | `brew install yq` | 资源适配 |
| **jq** | JSON 处理 | `brew install jq` | 数据分析 |
| **pluto** | API 弃用检测 | `brew install FairwindsOps/tap/pluto` | 兼容性评估 |
| **kubent** | API 弃用检测 (替代) | `brew install kubent` | 兼容性评估 |
| **rsync** | 文件数据同步 | 系统自带 | 存储迁移 |
| **redis-shake** | Redis 数据同步 | Docker/二进制 | Redis 迁移 |
| **pt-table-checksum** | MySQL 数据校验 | `brew install percona-toolkit` | 数据校验 |
| **wrk** | HTTP 压测 | `brew install wrk` | 性能验证 |
| **aliyun CLI** | 阿里云 API | `brew install aliyun-cli` | 全程 |
| **helm** | Chart 部署 | `brew install helm` | 全程 |

### 一键安装

```bash
# macOS 一键安装所有迁移工具
brew install kubectl helm velero skopeo yq jq wrk aliyun-cli
brew install FairwindsOps/tap/pluto

# 安装 krew 插件管理器
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/arm64/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)

# 安装 kubectl 插件
kubectl krew install neat
kubectl krew install ctx
kubectl krew install ns
```

---

## 2. Velero 完整指南

### 2.1 Velero + 阿里云 OSS 配置

```bash
# 创建 OSS Bucket
aliyun oss mb oss://k8s-migration-velero --region cn-hangzhou --acl private

# 创建 RAM 策略
cat > velero-policy.json <<EOF
{
  "Version": "1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "oss:PutObject", "oss:GetObject", "oss:DeleteObject",
        "oss:GetBucket", "oss:ListObjects", "oss:ListBuckets"
      ],
      "Resource": [
        "acs:oss:*:*:k8s-migration-velero",
        "acs:oss:*:*:k8s-migration-velero/*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "ecs:DescribeSnapshots", "ecs:CreateSnapshot", "ecs:DeleteSnapshot",
        "ecs:DescribeDisks", "ecs:CreateDisk"
      ],
      "Resource": ["*"]
    }
  ]
}
EOF

aliyun ram CreatePolicy --PolicyName velero-migration --PolicyDocument "$(cat velero-policy.json)"

# 创建 RAM 用户并授权
aliyun ram CreateUser --UserName velero-migration
aliyun ram AttachPolicyToUser --PolicyType Custom --PolicyName velero-migration --UserName velero-migration
aliyun ram CreateAccessKey --UserName velero-migration
# 记录 AccessKeyId 和 AccessKeySecret

# 凭证文件
cat > credentials-velero <<EOF
[default]
aws_access_key_id=<AccessKeyId>
aws_secret_access_key=<AccessKeySecret>
EOF
```

### 2.2 Velero 常用操作

```bash
# 安装到源集群
velero install \
  --provider alibabacloud \
  --bucket k8s-migration-velero \
  --secret-file ./credentials-velero \
  --backup-location-config region=cn-hangzhou \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.2 \
  --use-node-agent \
  --default-volumes-to-fs-backup \
  --kubecontext source-cluster

# 按 Namespace 备份
velero backup create ns-production \
  --include-namespaces production \
  --default-volumes-to-fs-backup \
  --kubecontext source-cluster

# 排除特定资源
velero backup create selective-backup \
  --include-namespaces production,staging \
  --exclude-resources events,endpoints \
  --kubecontext source-cluster

# 查看备份
velero backup get --kubecontext source-cluster
velero backup describe ns-production --kubecontext source-cluster
velero backup logs ns-production --kubecontext source-cluster

# 在 ACK 恢复
velero restore create restore-production \
  --from-backup ns-production \
  --kubecontext ack-cluster

# 查看恢复状态
velero restore describe restore-production --kubecontext ack-cluster
velero restore logs restore-production --kubecontext ack-cluster

# 定时备份（迁移期间源集群每日备份）
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --include-namespaces production \
  --default-volumes-to-fs-backup \
  --ttl 168h \
  --kubecontext source-cluster
```

---

## 3. 镜像同步工具

### 3.1 skopeo 批量同步

```bash
# skopeo 不需要 Docker Daemon，效率更高

# 单镜像复制
skopeo copy \
  --src-tls-verify=false \
  docker://harbor.internal.com/app/web:v1.2 \
  docker://registry.cn-hangzhou.aliyuncs.com/myns/web:v1.2

# 批量复制（支持 manifest list/multi-arch）
skopeo copy --all \
  docker://harbor.internal.com/app/api:latest \
  docker://registry.cn-hangzhou.aliyuncs.com/myns/api:latest

# 同步整个仓库
skopeo sync --src docker --dest docker \
  harbor.internal.com/app \
  registry.cn-hangzhou.aliyuncs.com/myns

# 检查镜像信息（不拉取）
skopeo inspect docker://registry.cn-hangzhou.aliyuncs.com/myns/web:v1.2
```

### 3.2 ACR 镜像仓库同步

```bash
# 使用阿里云 ACR 企业版的镜像同步功能
# 控制台: ACR → 实例 → 仓库同步 → 创建同步规则

# 或使用 image-syncer (阿里云开源)
# https://github.com/AliyunContainerService/image-syncer

cat > sync-config.yaml <<EOF
harbor.internal.com:
  username: admin
  password: Harbor12345
  insecure: true
registry.cn-hangzhou.aliyuncs.com:
  username: <acr-user>
  password: <acr-password>
EOF

cat > sync-images.yaml <<EOF
harbor.internal.com/app/web: registry.cn-hangzhou.aliyuncs.com/myns/web
harbor.internal.com/app/api: registry.cn-hangzhou.aliyuncs.com/myns/api
harbor.internal.com/app/worker: registry.cn-hangzhou.aliyuncs.com/myns/worker
EOF

image-syncer --auth sync-config.yaml --images sync-images.yaml --retries 3
```

---

## 4. YAML 处理工具

### 4.1 kubectl-neat

```bash
# 清洗 YAML（去除 K8s 自动添加的元数据）
kubectl get deploy web -o yaml | kubectl neat

# 导出干净的 YAML
kubectl get deploy web -o yaml | kubectl neat > web-deploy.yaml

# 批量导出整个 Namespace
kubectl get all -n production -o yaml | kubectl neat > production-all.yaml
```

### 4.2 yq 常用操作

```bash
# 修改 StorageClass
yq eval '.spec.storageClassName = "alicloud-disk-essd"' pvc.yaml

# 批量修改镜像前缀
yq eval '
  (.spec.template.spec.containers[].image | select(test("harbor.internal.com"))) |=
  sub("harbor.internal.com/", "registry.cn-hangzhou.aliyuncs.com/myns/")
' deploy.yaml

# 添加注解
yq eval '.metadata.annotations["migrated-from"] = "source-cluster"' deploy.yaml

# 删除特定字段
yq eval 'del(.metadata.uid, .metadata.resourceVersion, .status)' resource.yaml

# 合并文件
yq eval-all '. as $item ireduce ({}; . * $item)' base.yaml overlay.yaml

# 提取特定信息
yq eval '.items[].metadata.name' deployments.yaml
```

---

## 5. 兼容性检查工具

### 5.1 pluto — API 弃用检测

```bash
# 扫描集群
pluto detect-all-in-cluster

# 扫描 Helm release
pluto detect-helm -A

# 指定目标版本检查
pluto detect-all-in-cluster --target-versions k8s=v1.28.0

# 扫描本地 YAML 文件
pluto detect-files -d ./migration-export/

# JSON 输出（便于自动化处理）
pluto detect-all-in-cluster -o json | jq '.[] | select(.removed == true)'
```

### 5.2 kubent — 弃用 API 检测

```bash
# 替代 pluto 的选择
kubent

# 指定 kubeconfig
kubent --kubeconfig ~/.kube/source-cluster.yaml

# 扫描 Helm
kubent --helm3
```

---

## 6. 数据迁移工具

### 6.1 rsync — 文件同步

```bash
# 基本同步
rsync -avz --progress /source/data/ /target/data/

# 增量同步（只传输变化的文件）
rsync -avz --progress --delete /source/data/ /target/data/

# 通过 SSH 远程同步
rsync -avz -e "ssh -p 22" /source/data/ user@remote:/target/data/

# 排除特定文件
rsync -avz --exclude='*.log' --exclude='tmp/' /source/ /target/

# 带宽限制（KB/s）
rsync -avz --bwlimit=10000 /source/ /target/

# 断点续传
rsync -avz --partial --progress /source/ /target/
```

### 6.2 redis-shake — Redis 数据同步

```bash
# Docker 方式运行
docker run --rm -v $(pwd)/shake.toml:/etc/redis-shake.toml \
  redisshake/redis-shake:latest /etc/redis-shake.toml

# 支持模式: sync (实时同步), restore (RDB恢复), scan (扫描同步)
```

---

## 7. 迁移脚本集合

### 7.1 完整迁移工具箱

```bash
#!/bin/bash
# migration-toolkit.sh
# 迁移工具箱 - 统一入口

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_CONTEXT="${SOURCE_CONTEXT:-source-cluster}"
ACK_CONTEXT="${ACK_CONTEXT:-ack-cluster}"

usage() {
  echo "Usage: $0 <command>"
  echo ""
  echo "Commands:"
  echo "  assess      运行迁移评估采集"
  echo "  export      导出源集群资源"
  echo "  clean       清洗导出的 YAML"
  echo "  adapt       适配 ACK (镜像/StorageClass/注解)"
  echo "  apply       应用到 ACK"
  echo "  verify      验证迁移结果"
  echo "  compare     双集群资源对比"
  echo "  rollback    紧急回滚"
}

case "${1:-}" in
  assess)
    echo "运行迁移评估..."
    bash $SCRIPT_DIR/01-assess.sh
    ;;
  export)
    echo "导出源集群资源..."
    bash $SCRIPT_DIR/02-export.sh
    ;;
  clean)
    echo "清洗 YAML..."
    bash $SCRIPT_DIR/03-clean.sh
    ;;
  adapt)
    echo "适配 ACK..."
    bash $SCRIPT_DIR/04-adapt.sh
    ;;
  apply)
    echo "应用到 ACK..."
    bash $SCRIPT_DIR/05-apply.sh
    ;;
  verify)
    echo "验证迁移..."
    bash $SCRIPT_DIR/06-verify.sh
    ;;
  compare)
    echo "双集群对比..."
    echo "资源类型           | 源集群 | ACK"
    echo "-------------------|--------|-----"
    for r in deployments statefulsets daemonsets services ingresses cronjobs configmaps secrets pvc; do
      src=$(kubectl --context=$SOURCE_CONTEXT get $r -A --no-headers 2>/dev/null | grep -cvE "^kube-" || echo 0)
      ack=$(kubectl --context=$ACK_CONTEXT get $r -A --no-headers 2>/dev/null | grep -cvE "^kube-" || echo 0)
      printf "%-19s| %-6s | %s\n" "$r" "$src" "$ack"
    done
    ;;
  rollback)
    echo "!!! 执行紧急回滚 !!!"
    bash $SCRIPT_DIR/emergency-rollback.sh
    ;;
  *)
    usage
    exit 1
    ;;
esac
```

### 7.2 kubeconfig 多集群管理

```bash
# 设置多集群 kubeconfig
export KUBECONFIG=~/.kube/source-cluster.yaml:~/.kube/ack-cluster.yaml

# 查看所有 context
kubectl config get-contexts

# 快速切换
kubectl config use-context source-cluster
kubectl config use-context ack-cluster

# 或使用 kubectx
kubectx source-cluster
kubectx ack-cluster

# 同时操作两个集群
alias ksrc="kubectl --context=source-cluster"
alias kack="kubectl --context=ack-cluster"

# 使用示例
ksrc get pods -A
kack get pods -A
```

---

**上一步**: ← [08-验收、切换与旧集群退役](./08-validation-cutover-decommission.md)
**下一步**: → [10-生产迁移实战案例](./10-real-world-case-study.md)

---

## Obsidian 相关文档

- topic-migration MOC
- [[domain-08-release-change-management/topic-migration/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[domain-08-release-change-management/topic-migration/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[domain-08-release-change-management/topic-migration/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[domain-08-release-change-management/topic-migration/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[domain-08-release-change-management/topic-migration/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[domain-08-release-change-management/topic-migration/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]]
- [[domain-08-release-change-management/topic-migration/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- [[domain-08-release-change-management/topic-migration/07-observability-security-migration.md|07 - 可观测性与安全迁移]]
- [[domain-08-release-change-management/topic-migration/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]]
- [[domain-08-release-change-management/topic-migration/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 07-observability-security-migration
- 08-validation-cutover-decommission
- 10-real-world-case-study
- logging

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
