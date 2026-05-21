---
title: Backup and Restore etcd
description: '- [[references/k8s-storage-ecosystem.md|k8s-storage-ecosystem]] — 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复'
category: skills
tags:
- k8s
- etcd
- backup
- restore
- snapshot
- disaster-recovery
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Backup and Restore etcd 是什么
- 如何 Backup and Restore etcd
trigger_keywords:
- Backup
- and
- Restore
- etcd
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

# Backup and Restore etcd

## Why Backup etcd

etcd contains all Kubernetes cluster state. Without a backup, cluster failure means total data loss. Backups enable disaster recovery, migration, and point-in-time restores.

## Backup (Snapshot)

```bash
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

**Verify snapshot**:
```bash
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-20260120.db --write-out=table
```

## Restore

```bash
# Stop API Server first
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260120.db \
  --data-dir=/var/lib/etcd-restore \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://192.168.1.101:2380,etcd-2=https://192.168.1.102:2380,etcd-3=https://192.168.1.103:2380 \
  --initial-advertise-peer-urls=https://192.168.1.101:2380
```

After restore:
1. Update etcd manifest to point to new data-dir
2. Restart etcd (static Pod auto-restarts)
3. Restart API Server

## Backup Schedule

| Item | Frequency | Storage |
|------|-----------|---------|
| etcd snapshot | Hourly | Object storage (S3/OSS) |
| Certificate backup | After every change | Encrypted storage |
| Cluster manifests | After every change | Git repository |
| Application PV data | Daily (Velero) | Object storage |

## Testing

Run full cluster restore drills quarterly. A backup that hasn't been tested for restore is not a backup.

## Related

- [[references/k8s-storage-ecosystem.md|k8s-storage-ecosystem]] — 存储体系：PV、PVC、StorageClass、CSI 驱动与灾备恢复
- [[skills/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — Monitor Kubernetes Metrics
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[etcd|etcd]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]

- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]