---
title: 04 - 存储与数据迁移 [migration]
description: 'title: 04 - 存储与数据迁移'
summary: 'title: 04 - 存储与数据迁移'
category: general
tags:
- migration
- upgrade
- storage
- ceph
- mysql
- job
- ingress
- rag
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 存储与数据迁移 是什么
- 如何 存储与数据迁移
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 存储与数据迁移
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- mysql-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 04 - 存储与数据迁移
description: '# 04 - 存储与数据迁移'
category: migration
tags:
- k8s
- migration
- modernization
- ceph
- mysql
- job
- [[Ingress|ingress]]
- rag
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 存储与数据迁移 是什么
- 如何 存储与数据迁移
trigger_keywords:
- 存储与数据迁移
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

# 04 - 存储与数据迁移

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: PV, PVC, CSI, 云盘, NAS, OSS, Velero, 数据同步, StorageClass

---

<!-- chunk: 目录 -->## 目录

1. [存储架构差异分析](#1-存储架构差异分析)
2. [ACK 存储体系](#2-ack-存储体系)
3. [StorageClass 迁移](#3-storageclass-迁移)
4. [NFS → 阿里云 NAS 迁移](#4-nfs--阿里云-nas-迁移)
5. [Ceph → 阿里云云盘迁移](#5-ceph--阿里云云盘迁移)
6. [Local PV → 云盘迁移](#6-local-pv--云盘迁移)
7. [Velero 备份恢复方案](#7-velero-备份恢复方案)
8. [数据校验](#8-数据校验)

---

<!-- chunk: 1. 存储架构差异分析 -->## 1. 存储架构差异分析

## 1.1 存储方案映射

| 自建存储 | 访问模式 | ACK 推荐方案 | 迁移策略 |
|---------|---------|------------|---------|
| **NFS Server** | ReadWriteMany | 阿里云 NAS (CSI) | rsync 数据同步 |
| **Ceph RBD** | ReadWriteOnce | 阿里云 ESSD 云盘 (CSI) | 数据导出 + 云盘导入 |
| **CephFS** | ReadWriteMany | 阿里云 NAS (CSI) | rsync 数据同步 |
| **GlusterFS** | ReadWriteMany | 阿里云 NAS (CSI) | rsync 数据同步 |
| **Local PV** | ReadWriteOnce | 阿里云 ESSD 云盘 (CSI) | tar + 传输 + 解压 |
| **hostPath** | ReadWriteOnce | 阿里云 ESSD / NAS | 手动数据复制 |
| **[[OpenEBS|OpenEBS]] Jiva** | ReadWriteOnce | 阿里云 ESSD 云盘 | 快照 + 数据复制 |
| **[[Longhorn|Longhorn]]** | ReadWriteOnce | 阿里云 ESSD 云盘 | Longhorn 备份 + 恢复 |

## 1.2 存储容量规划

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 采集自建集群存储使用情况
echo "=== PV 总容量 ==="
kubectl get pv -o json | jq '[.items[].spec.capacity.storage | rtrimstr("Gi") | tonumber] | add'

echo "=== 各 StorageClass PVC 统计 ==="
kubectl get pvc -A -o json | jq -r '
  [.items[] | {sc: .spec.storageClassName, size: .status.capacity.storage}] |
  group_by(.sc) |
  .[] | {storageClass: .[0].sc, count: length, totalSize: [.[].size | rtrimstr("Gi") | tonumber] | add}
'

echo "=== 实际磁盘使用率 ==="
# 在挂载了 PV 的 Pod 中检查
kubectl get pods -A -o json | jq -r '
  .items[] | select(.spec.volumes[]?.persistentVolumeClaim != null) |
  .metadata.namespace + "/" + .metadata.name
' | head -10 | while read pod; do
  ns=$(echo $pod | cut -d/ -f1)
  name=$(echo $pod | cut -d/ -f2)
  echo "--- $pod ---"
  kubectl exec -n $ns $name -- df -h 2>/dev/null | grep -E "^/dev" || echo "无法执行 df"
done
```
---

<!-- chunk: 2. ACK 存储体系 -->## 2. ACK 存储体系

## 2.1 预置 StorageClass

| StorageClass | 存储类型 | 性能 | 访问模式 | 适用场景 |
|-------------|---------|------|---------|---------|
| `alicloud-disk-essd` | ESSD PL1 云盘 | IOPS: 50,000 | RWO | 通用业务数据库 |
| `alicloud-disk-essd-pl0` | ESSD PL0 云盘 | IOPS: 10,000 | RWO | 日志/非关键数据 |
| `alicloud-disk-essd-pl2` | ESSD PL2 云盘 | IOPS: 100,000 | RWO | 高性能数据库 |
| `alicloud-disk-essd-pl3` | ESSD PL3 云盘 | IOPS: 1,000,000 | RWO | 超高 IO 场景 |
| `alicloud-disk-ssd` | SSD 云盘 | IOPS: 25,000 | RWO | 成本敏感场景 |
| `alicloud-disk-efficiency` | 高效云盘 | IOPS: 5,000 | RWO | 开发测试 |
| 自定义 NAS SC | NAS 文件存储 | 吞吐型/极速型 | RWX | 共享文件 |
| 自定义 OSS SC | OSS 对象存储 | - | ROX/RWX | 静态资源/日志 |

## 2.2 创建 NAS StorageClass

```yaml
# 先创建 NAS 文件系统（通过控制台或 API）
# aliyun nas CreateFileSystem --ProtocolType NFS --StorageType Performance --VpcId <vpc-id> --VSwitchId <vsw-id>

# NAS StorageClass（动态供给）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-nas
provisioner: nasplugin.csi.alibabacloud.com
parameters:
  server: "<nas-mount-target>.cn-hangzhou.nas.aliyuncs.com"
  path: "/k8s"
  vers: "4.0"
reclaimPolicy: Retain
mountOptions:
  - nolock
  - proto=tcp
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=2
  - noresvport
```

---

<!-- chunk: 3. StorageClass 迁移 -->## 3. StorageClass 迁移

## 3.1 StorageClass 映射

```bash
#!/bin/bash
# adapt-storage-class.sh
# 将自建集群的 StorageClass 名称映射到 ACK

CLEAN_DIR="./migration-clean"

# 定义映射关系
declare -A SC_MAP
SC_MAP["ceph-rbd"]="alicloud-disk-essd"
SC_MAP["ceph-block"]="alicloud-disk-essd"
SC_MAP["nfs-client"]="alicloud-nas"
SC_MAP["nfs-storage"]="alicloud-nas"
SC_MAP["local-storage"]="alicloud-disk-essd"
SC_MAP["local-path"]="alicloud-disk-essd"
SC_MAP["openebs-jiva"]="alicloud-disk-essd"
SC_MAP["longhorn"]="alicloud-disk-essd"
SC_MAP["standard"]="alicloud-disk-essd"

# 批量替换 PVC 中的 StorageClass
find $CLEAN_DIR -name "*.yaml" -exec grep -l "storageClassName" {} + | while read f; do
  for old_sc in "${!SC_MAP[@]}"; do
    new_sc="${SC_MAP[$old_sc]}"
    sed -i '' "s/storageClassName: $old_sc/storageClassName: $new_sc/g" "$f"
  done
done

echo "StorageClass 映射完成"
```

## 3.2 PVC 迁移注意事项

```yaml
# 自建集群 PVC（Ceph RBD）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
  namespace: production
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: ceph-rbd           # 自建 Ceph
  resources:
    requests:
      storage: 100Gi

---
# ACK 适配后
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
  namespace: production
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: alicloud-disk-essd  # ACK ESSD 云盘
  resources:
    requests:
      storage: 100Gi                    # 云盘最小 20Gi，按需调整
```

---

<!-- chunk: 4. NFS → 阿里云 NAS 迁移 -->## 4. NFS → 阿里云 NAS 迁移

## 4.1 迁移方案

```
自建 NFS Server                    阿里云 NAS
┌────────────────┐                ┌────────────────┐
│  /data/k8s/    │   rsync 同步   │  /k8s/         │
│  ├── app-a/    │ ──────────────►│  ├── app-a/    │
│  ├── app-b/    │                │  ├── app-b/    │
│  └── shared/   │                │  └── shared/   │
└────────────────┘                └────────────────┘
```

## 4.2 rsync 同步操作

```bash
# 1. 创建 NAS 文件系统
aliyun nas CreateFileSystem \
  --ProtocolType NFS \
  --StorageType Performance \
  --Description "migration-nas"

# 2. 创建挂载点
NAS_FS_ID="<nas-filesystem-id>"
aliyun nas CreateMountTarget \
  --FileSystemId $NAS_FS_ID \
  --AccessGroupName DEFAULT_VPC_GROUP_NAME \
  --NetworkType Vpc \
  --VpcId $VPC_ID \
  --VSwitchId "<vsw-id>"

# 获取挂载地址
NAS_MOUNT=$(aliyun nas DescribeMountTargets --FileSystemId $NAS_FS_ID \
  --output cols=MountTargetDomain --rows MountTargets.MountTarget[] | tail -1)
echo "NAS 挂载地址: $NAS_MOUNT"

# 3. 在跳板机挂载 NAS
sudo mkdir -p /mnt/ack-nas
sudo mount -t nfs -o vers=4,nolock,proto=tcp,rsize=1048576,wsize=1048576 \
  $NAS_MOUNT:/ /mnt/ack-nas

# 4. rsync 同步数据（首次全量）
rsync -avz --progress \
  --exclude='lost+found' \
  /path/to/nfs/data/ \
  /mnt/ack-nas/k8s/

# 5. rsync 增量同步（切换前执行最后一次）
rsync -avz --progress --delete \
  /path/to/nfs/data/ \
  /mnt/ack-nas/k8s/

# 6. 验证数据完整性
diff <(find /path/to/nfs/data/ -type f -exec md5sum {} + | sort) \
     <(find /mnt/ack-nas/k8s/ -type f -exec md5sum {} + | sort)
```

## 4.3 通过 K8s Job 进行数据同步

```yaml
# 在 ACK 集群中运行 rsync Job
apiVersion: batch/v1
kind: Job
metadata:
  name: nfs-to-nas-sync
  namespace: migration
spec:
  template:
    spec:
      containers:
      - name: rsync
        image: instrumentisto/rsync-ssh:latest
        command:
        - rsync
        - -avz
        - --progress
        - rsync://<nfs-server-ip>/data/
        - /target/
        volumeMounts:
        - name: nas-vol
          mountPath: /target
      volumes:
      - name: nas-vol
        persistentVolumeClaim:
          claimName: nas-migration-pvc
      restartPolicy: Never
  backoffLimit: 3
```

---

<!-- chunk: 5. Ceph → 阿里云云盘迁移 -->## 5. Ceph → 阿里云云盘迁移

## 5.1 迁移方案

```
方案 A: rbd export + 传输 + 云盘导入
  ① rbd export → 本地 raw 文件
  ② 通过专线/OSS 传输到阿里云
  ③ 导入为云盘快照 → 创建云盘

方案 B: 应用层数据复制（推荐）
  ① 在 ACK 创建新 PVC（ESSD 云盘）
  ② 部署数据复制 Job（rsync/pg_dump/mysqldump 等）
  ③ 从源集群复制数据到 ACK PVC

方案 C: Velero 备份恢复
  ① 在源集群安装 Velero + Restic
  ② 备份 PV 数据到 OSS
  ③ 在 ACK 恢复（自动创建 PVC）
```

## 5.2 应用层数据复制（方案 B）

```yaml
# 在 ACK 集群部署数据复制 Pod
apiVersion: v1
kind: Pod
metadata:
  name: data-migrator
  namespace: migration
spec:
  containers:
  - name: migrator
    image: alpine:3.18
    command: ["sleep", "86400"]  # 保持运行，手动执行复制
    volumeMounts:
    - name: target-vol
      mountPath: /target
  volumes:
  - name: target-vol
    persistentVolumeClaim:
      claimName: mysql-data    # ACK 新建的 PVC
---
# 在源集群创建数据导出 Pod
apiVersion: v1
kind: Pod
metadata:
  name: data-exporter
  namespace: production
spec:
  containers:
  - name: exporter
    image: alpine:3.18
    command: ["sleep", "86400"]
    volumeMounts:
    - name: source-vol
      mountPath: /source
  volumes:
  - name: source-vol
    persistentVolumeClaim:
      claimName: mysql-data    # 源集群 PVC
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 kubectl cp 复制数据（适合中小数据量 < 10GB）
# 源集群导出
kubectl --context=source-cluster exec -n production data-exporter -- \
  tar czf /tmp/data-backup.tar.gz -C /source .
kubectl --context=source-cluster cp production/data-exporter:/tmp/data-backup.tar.gz ./data-backup.tar.gz

# ACK 集群导入
kubectl --context=ack-cluster cp ./data-backup.tar.gz migration/data-migrator:/tmp/data-backup.tar.gz
kubectl --context=ack-cluster exec -n migration data-migrator -- \
  tar xzf /tmp/data-backup.tar.gz -C /target

# 大数据量使用 rsync over SSH
kubectl --context=source-cluster exec -n production data-exporter -- \
  rsync -avz /source/ rsync://<ack-node-ip>:<port>/target/
```
---

<!-- chunk: 6. Local PV → 云盘迁移 -->## 6. Local PV → 云盘迁移

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 识别 Local PV 数据
kubectl --context=source-cluster get pv -o json | jq -r '
  .items[] | select(.spec.local != null) |
  .metadata.name + " → " + .spec.local.path + " on " + .spec.nodeAffinity.required.nodeSelectorTerms[0].matchExpressions[0].values[0]
'

# 2. 在对应节点上打包数据
ssh <node-ip> "tar czf /tmp/local-pv-backup.tar.gz -C /data/local-pv ."

# 3. 传输到跳板机
scp <node-ip>:/tmp/local-pv-backup.tar.gz ./

# 4. 在 ACK 创建 PVC 并恢复数据
kubectl --context=ack-cluster apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: migrated-local-data
  namespace: production
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: alicloud-disk-essd
  resources:
    requests:
      storage: 50Gi
EOF

# 等待 PVC 绑定
kubectl --context=ack-cluster wait --for=jsonpath='{.status.phase}'=Bound \
  pvc/migrated-local-data -n production --timeout=120s

# 5. 恢复数据
kubectl --context=ack-cluster run restore-job --rm -it \
  --image=alpine:3.18 \
  --overrides='{"spec":{"containers":[{"name":"restore","image":"alpine:3.18","command":["sh","-c","sleep 3600"],"volumeMounts":[{"name":"data","mountPath":"/data"}]}],"volumes":[{"name":"data","persistentVolumeClaim":{"claimName":"migrated-local-data"}}]}}' \
  -n production -- sh

# 在 Pod 内:
# kubectl cp production/restore-job:/tmp/ 将备份文件复制进去
# tar xzf /tmp/local-pv-backup.tar.gz -C /data/
```
---

<!-- chunk: 7. Velero 备份恢复方案 -->## 7. Velero 备份恢复方案

## 7.1 安装 Velero（双集群）

```bash
# 创建 OSS Bucket 用于存储备份
aliyun oss mb oss://velero-migration-backup --region cn-hangzhou

# 创建 RAM 用户并授权 OSS
aliyun ram CreateUser --UserName velero-backup
aliyun ram CreateAccessKey --UserName velero-backup
# 记录 AccessKeyId 和 AccessKeySecret

# 创建凭证文件
cat > credentials-velero <<EOF
[default]
aws_access_key_id=<AccessKeyId>
aws_secret_access_key=<AccessKeySecret>
EOF

# 在源集群安装 Velero
velero install \
  --provider alibabacloud \
  --bucket velero-migration-backup \
  --secret-file ./credentials-velero \
  --backup-location-config region=cn-hangzhou \
  --snapshot-location-config region=cn-hangzhou \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.2 \
  --use-node-agent \
  --default-volumes-to-fs-backup \
  --kubecontext source-cluster

# 在 ACK 集群安装 Velero
velero install \
  --provider alibabacloud \
  --bucket velero-migration-backup \
  --secret-file ./credentials-velero \
  --backup-location-config region=cn-hangzhou \
  --snapshot-location-config region=cn-hangzhou \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.2 \
  --use-node-agent \
  --kubecontext ack-cluster
```

## 7.2 执行备份与恢复

```bash
# 在源集群执行备份（按 Namespace 备份）
velero backup create migration-backup-prod \
  --include-namespaces production \
  --default-volumes-to-fs-backup \
  --kubecontext source-cluster

# 查看备份进度
velero backup describe migration-backup-prod --kubecontext source-cluster

# 等待备份完成
velero backup wait migration-backup-prod --kubecontext source-cluster

# 在 ACK 恢复
velero restore create migration-restore-prod \
  --from-backup migration-backup-prod \
  --namespace-mappings "production:production" \
  --kubecontext ack-cluster

# 查看恢复进度
velero restore describe migration-restore-prod --kubecontext ack-cluster

# 注意: 恢复后需手动处理
# 1. PVC 的 StorageClass 可能需要调整
# 2. Service 的 LoadBalancer IP 会变化
# 3. Ingress 的 external IP 会变化
```

## 7.3 Velero StorageClass 映射

```yaml
# 创建 StorageClass 映射 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: change-storage-class-config
  namespace: velero
  labels:
    velero.io/plugin-config: ""
    velero.io/change-storage-class: RestoreItemAction
data:
  ceph-rbd: alicloud-disk-essd
  nfs-client: alicloud-nas
  local-storage: alicloud-disk-essd
  standard: alicloud-disk-essd
```

---

<!-- chunk: 8. 数据校验 -->## 8. 数据校验

## 8.1 文件级校验

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# verify-data-integrity.sh
# 校验迁移前后数据一致性

echo "=== PVC 容量校验 ==="
SOURCE_CONTEXT="source-cluster"
ACK_CONTEXT="ack-cluster"
NS="production"

# 对比源和目标 PVC 容量
echo "PVC | 源集群容量 | ACK 容量 | 状态"
echo "----|----------|--------|----"
for pvc in $(kubectl --context=$ACK_CONTEXT get pvc -n $NS --no-headers -o custom-columns=:metadata.name); do
  src_size=$(kubectl --context=$SOURCE_CONTEXT get pvc $pvc -n $NS -o jsonpath='{.status.capacity.storage}' 2>/dev/null || echo "N/A")
  ack_size=$(kubectl --context=$ACK_CONTEXT get pvc $pvc -n $NS -o jsonpath='{.status.capacity.storage}')
  status="OK"
  if [ "$src_size" != "$ack_size" ] && [ "$src_size" != "N/A" ]; then
    status="WARN"
  fi
  echo "$pvc | $src_size | $ack_size | $status"
done

echo ""
echo "=== 文件数量校验 ==="
# 在 Pod 内执行 find 计数
for pvc in $(kubectl --context=$ACK_CONTEXT get pvc -n $NS --no-headers -o custom-columns=:metadata.name); do
  # 找到挂载该 PVC 的 Pod
  pod=$(kubectl --context=$ACK_CONTEXT get pods -n $NS -o json | \
    jq -r ".items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == \"$pvc\") | .metadata.name" | head -1)
  if [ -n "$pod" ]; then
    mount_path=$(kubectl --context=$ACK_CONTEXT get pod $pod -n $NS -o json | \
      jq -r ".spec.containers[].volumeMounts[] | select(.name == (.. | .persistentVolumeClaim? | select(.claimName == \"$pvc\") | .claimName // empty)) | .mountPath" 2>/dev/null | head -1)
    if [ -n "$mount_path" ]; then
      count=$(kubectl --context=$ACK_CONTEXT exec -n $NS $pod -- find $mount_path -type f 2>/dev/null | wc -l)
      echo "$pvc ($pod): $count 个文件"
    fi
  fi
done
```
## 8.2 检查清单

- [ ] 所有 PVC 在 ACK 已创建并绑定
- [ ] NFS → NAS 数据 rsync 完成，md5 校验通过
- [ ] Ceph → 云盘数据复制完成
- [ ] Local PV 数据已恢复到云盘
- [ ] StorageClass 映射正确
- [ ] 应用能正常读写存储
- [ ] 存储性能基线测试通过（IOPS/吞吐）
- [ ] Velero 备份可正常恢复（如使用 Velero 方案）

---

**上一步**: ← [03-应用工作负载迁移](./03-application-workload-migration.md)
**下一步**: → [05-网络迁移与流量切换](./05-network-migration-traffic-cutover.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration KUDIG Database — Global MOC
- [[domain-08-release-change-management/topic-migration/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[domain-08-release-change-management/topic-migration/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[domain-08-release-change-management/topic-migration/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[domain-08-release-change-management/topic-migration/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[domain-08-release-change-management/topic-migration/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]]
- [[domain-08-release-change-management/topic-migration/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- [[domain-08-release-change-management/topic-migration/07-observability-security-migration.md|07 - 可观测性与安全迁移]]
- [[domain-08-release-change-management/topic-migration/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]]
- [[domain-08-release-change-management/topic-migration/09-migration-toolchain.md|09 - 迁移工具链参考]]
- [[domain-08-release-change-management/topic-migration/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 02-ack-target-cluster-design
- 03-application-workload-migration
- 05-network-migration-traffic-cutover
- 06-stateful-services-migration

## Related

- [[domain-19-landscape-references/topic-index/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]

```

<!-- risk-assessed -->
