---
title: Manage Persistent Storage
description: '- [[references/release-notes-storage.md|release-notes-storage]] — 发布说明索引 — 存储'
category: skills
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- csi
- volume
- statefulset
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Manage Persistent Storage 是什么
- 如何 Manage Persistent Storage
trigger_keywords:
- Manage
- Persistent
- Storage
prerequisites:
- kubectl-basics
---

# Manage Persistent Storage

## Storage Lifecycle

### Provision

**Dynamic** (recommended): Create a StorageClass with a CSI driver. PVCs automatically provision PVs.

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ssd-storage
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer  # Schedule Pod first
reclaimPolicy: Delete
allowVolumeExpansion: true
```

**Static**: Manually create PV, then create PVC that matches PV specs.

### Bind

The PVC Controller automatically binds PVCs to matching PVs based on:
- Storage class match
- Access mode compatibility
- Sufficient capacity
- Label selectors (if specified)

### Use

Reference PVC in Pod/Deployment spec:
```yaml
volumes:
- name: data
  persistentVolumeClaim:
    claimName: my-pvc
```

### Expand

If `allowVolumeExpansion: true` is set on the StorageClass, increase PVC size:
```bash
kubectl patch pvc my-pvc -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'
```

### Troubleshoot

```bash
kubectl get pvc                          # Check binding status
kubectl describe pvc <name>              # Check events for binding failures
kubectl get pv                           # Check PV status and reclaim policy
```

## Common Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| PVC stuck Pending | No matching PV or provisioner failure | Check StorageClass provisioner, describe PVC |
| Volume stuck in Terminating | Pod still referencing PVC | Check owner references, remove finalizers if orphaned |
| Expansion failed | CSI driver doesn't support expansion, filesystem needs resize | Use compatible CSI driver, resize filesystem manually |

## Related

- [[references/release-notes-storage.md|release-notes-storage]] — 发布说明索引 — 存储
- [[skills/ts-storage.md|ts-storage]] — 存储故障排查
- [[skills/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[deployment]] — Deployment
- [[concepts/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[concepts/storage-model.md|Persistent Storage Model]]
- [[entities/statefulset.md|StatefulSet]]
- [[entities/csi-drivers.md|CSI Drivers]]
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]

- [[synthesis/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]