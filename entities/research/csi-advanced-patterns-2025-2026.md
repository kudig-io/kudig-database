---
title: Csi Advanced Patterns 2025 2026
summary: 'Research compiled: 2026-05-24 Sources: Kubernetes official blog, kubernetes-csi.github.io,
  kubernetes.io docs'
category: entities
tags:
- csi-advanced-patterns-2025-2026
tier: supporting
created: '2026-07-01'
---

# Kubernetes CSI Advanced Patterns & Latest Developments (2025-2026)

Research compiled: 2026-05-24
Sources: Kubernetes official blog, kubernetes-csi.github.io, kubernetes.io docs

---

## 1. CSI Migration from In-Tree Drivers

### Status (2025-2026)
- CSI migration for all major in-tree volume plugins is **complete and GA** as of Kubernetes 1.31+
- In-tree drivers (awsElasticBlockStore, gcePersistentDisk, azureDisk, azureFile, cinder, etc.) are fully migrated to CSI
- The `CSIMigration*` feature gates are removed in recent releases — CSI is the ONLY path
- FlexVolume is **deprecated** since v1.23 and removed from support
- Volume plugin types `portworxVolume` and `flexVolume` are deprecated for expansion

### Key Implications
- All new storage features (snapshots, cloning, expansion, topology) only work through CSI
- Existing PVs backed by in-tree plugins are automatically translated to CSI equivalents
- Operators must ensure CSI drivers for their cloud are installed

Source: https://kubernetes.io/docs/concepts/storage/persistent-volumes/
Source: https://kubernetes-csi.github.io/docs/

---

## 2. Volume Snapshots & Cloning

### Volume Snapshots (GA since v1.20)
- Three API resources: `VolumeSnapshot`, `VolumeSnapshotContent`, `VolumeSnapshotClass`
- Analogous to PVC/PV model — user-facing vs cluster-level resources
- Only supported via out-of-tree CSI volume plugins (never in-tree)
- Requires installation of snapshot controller + CRDs + validation webhook

### Volume Group Snapshots (NEW - progressing rapidly)
- **v1.32**: Moved to Beta
- **v1.34**: Moved to v1beta2
- **v1.36**: Moving to GA
- Enables taking consistent snapshots of a group of volumes simultaneously
- Uses `VolumeGroupSnapshot`, `VolumeGroupSnapshotContent`, `VolumeGroupSnapshotClass` resources
- Critical for multi-volume applications (e.g., distributed databases)

### Volume Cloning (GA since v1.17)
- Creates a new PVC from an existing PVC in the same namespace
- Uses `dataSource` field referencing an existing PVC
- CSI driver must implement `CLONE_VOLUME` capability
- Cross-namespace data sources now supported (beta/GA progression)

### Volume Populators (GA in v1.33)
- Allow custom data sources for PVCs beyond just snapshots and cloning
- Enables populating volumes from any external data source (backups, images, etc.)
- Uses `AnyVolumeDataSource` feature gate (now GA)

### Restore from Snapshot
```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restore-pvc
spec:
  storageClassName: csi-driver-class
  dataSource:
    name: new-snapshot-test
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 10Gi
```

Source: https://kubernetes.io/docs/concepts/storage/volume-snapshots/
Source: https://kubernetes.io/docs/concepts/storage/persistent-volumes/#volume-snapshot-and-restore
Source: https://kubernetes.io/blog/ (blog index: "Moving Volume Group Snapshots to GA" v1.36)

---

## 3. CSI Topology-Aware Provisioning

### Overview
- Allows CSI drivers to express topology constraints (zone, region, rack, node)
- CSIDriver object specifies `volumeLifecycleModes` and topology info
- StorageClass uses `volumeBindingMode: WaitForFirstConsumer` to delay provisioning
- Scheduler uses topology info to place pods on nodes with access to storage

### Storage Capacity Tracking (GA since v1.24)
- `CSIStorageCapacity` objects track available capacity per topology segment
- Scheduler uses this to avoid nodes without sufficient capacity
- Reduces failed provisioning attempts

### Storage Capacity Scoring (Alpha in v1.33)
- NEW: Nodes are scored based on available storage capacity during scheduling
- Enables preferential placement toward nodes with more storage available
- Goes beyond just "can it fit" to "where is it best"

### Mutable PV Node Affinity (Alpha in v1.35)
- Allows modifying PersistentVolume node affinity after creation
- Enables rebalancing of storage across topology domains
- Critical for day-2 operations in multi-zone clusters

Source: https://kubernetes-csi.github.io/docs/topology.html
Source: https://kubernetes.io/docs/concepts/storage/storage-capacity/
Source: https://kubernetes.io/blog/ (blog index entries for v1.33, v1.35)

---

## 4. CSI Inline Ephemeral Volumes

### CSI Ephemeral Volumes (GA since v1.25)
- Specified inline in the Pod spec under `volumes` with `csi:` type
- Only supported by CSI drivers that explicitly declare ephemeral support
- Lifecycle tied to the Pod — created on Pod start, deleted on Pod termination
- Use cases: encryption keys, temporary data, secret injection

### Generic Ephemeral Volumes (GA since v1.23)
- Any CSI driver supporting dynamic provisioning can be used
- Creates a PVC automatically (not restricted to ephemeral-only drivers)
- More flexible than CSI ephemeral — full PVC lifecycle management

### Image Volumes (Beta in v1.33)
- NEW: Read-only volumes based on OCI artifacts
- Mount OCI images directly as volumes into pods
- Useful for ML models, config bundles, static content

### Ephemeral Driver Restrictions
- CSI drivers must set `volumeLifecycleModes: ["Ephemeral"]` in CSIDriver spec
- Not all CSI drivers support ephemeral mode
- Security: ephemeral volumes don't go through normal PVC admission

Source: https://kubernetes.io/docs/concepts/storage/ephemeral-volumes/
Source: https://kubernetes-csi.github.io/docs/ephemeral-local-volumes.html

---

## 5. CSI Volume Expansion

### Status
- **CSI volume expansion is GA** (controller expansion + node expansion)
- **Online expansion** (while Pod is running) supported by many drivers
- Requires `allowVolumeExpansion: true` in StorageClass

### Recovery from Expansion Failure (GA in v1.34)
- NEW: When expansion fails (e.g., requested size too large), recovery mechanism
- Users can retry with a smaller size without manual intervention
- Previously required admin to manually fix stuck PVCs

### VolumeAttributesClass for Volume Modification (GA in v1.34)
- NEW: Enables modifying volume attributes (not just size) post-creation
- Examples: IOPS, throughput, encryption settings
- Uses `VolumeAttributesClass` resource referenced from PVC

### Mutable CSI Node Allocatable (Beta in v1.34)
- Allows CSI drivers to report and update allocatable volume counts
- Node-level volume limits can be dynamically adjusted
- Alpha in v1.33, Beta in v1.34

Source: https://kubernetes.io/docs/concepts/storage/persistent-volumes/#expanding-persistent-volumes-claims
Source: https://kubernetes.io/blog/ (v1.34 blog entries)

---

## 6. New CSI Driver Features & Developments

### Changed Block Tracking (CSI feature)
- CSI drivers can report which blocks changed since a previous snapshot
- Enables efficient incremental backups
- Critical for enterprise backup solutions

### Volume Health Monitoring
- CSI drivers can report volume health status
- External health monitor controller + agent sidecars
- Enables proactive alerting on storage issues

### SELinux Volume Label Changes (GA, with v1.37 implications)
- Moving from recursive SELinux relabeling to faster approaches
- Significant performance improvement for large volumes with many files

### CSI Windows Support
- CSI drivers can now work on Windows nodes
- Enables hybrid Linux/Windows cluster storage

### FSGroup Support
- CSI drivers can handle FSGroup natively
- Reduces kubelet overhead for permission management

### Token Requests
- CSI drivers can request service account tokens
- Enables secure communication with storage backends

### external-snapshot-metadata (NEW sidecar)
- New sidecar for exposing snapshot metadata
- Enables backup tools to query snapshot details

Source: https://kubernetes-csi.github.io/docs/
Source: https://kubernetes.io/blog/ (blog index)

---

## 7. CSI Security Hardening

### Secrets & Credentials Management
- StorageClass secrets: credentials for provision/delete operations
- VolumeSnapshotClass secrets: credentials for snapshot operations
- VolumeGroupSnapshotClass secrets: credentials for group snapshot operations
- Secrets are referenced by namespace/name and passed to CSI driver

### Preventing Unauthorized Volume Mode Conversion (GA in v1.30)
- Prevents converting PVC from Filesystem to Block mode without authorization
- Security measure against unauthorized access to raw block devices

### Pod Security Standards Integration
- CSI ephemeral volumes subject to Pod Security Standards admission
- Inline CSI volumes restricted by `restricted` pod security profile

### CSIDriver Object Security Features
- `attachRequired`: controls whether volume needs attach/detach
- `podInfoOnMount`: passes Pod metadata to CSI driver on mount
- `fsGroupPolicy`: controls FSGroup behavior (None, File, ReadWriteOnceWithFSType)
- `tokenRequests`: configure audience/token expiration for driver auth

### Read-Only Volume Mounts (GA in v1.30)
- `readOnly: true` is now enforced — truly read-only at mount level
- Previously could still be writable through certain paths

### PersistentVolume Deletion Protection Finalizer
- Prevents accidental PV deletion
- `kubernetes.io/pv-protection` finalizer added automatically

### Prevent PV Leaks on Out-of-Order Deletion (GA in v1.33)
- Fixes race condition where PVC deletion before PV could leak underlying storage
- Ensures proper cleanup sequencing

Source: https://kubernetes-csi.github.io/docs/
Source: https://kubernetes.io/docs/concepts/storage/persistent-volumes/

---

## 8. Complete CSI Feature Timeline (2023-2026)

| Kubernetes | Feature | Stage |
|------------|---------|-------|
| v1.30 | Unauthorized volume mode conversion prevention | GA |
| v1.30 | Read-only volume mounts enforcement | GA |
| v1.31 | PV last phase transition time | GA |
| v1.31 | Prevent PV leaks on out-of-order deletion | Beta |
| v1.31 | VolumeAttributesClass for modification | Beta |
| v1.31 | Read-only OCI volumes | Alpha |
| v1.32 | Volume Group Snapshots | Beta |
| v1.33 | Volume Populators | GA |
| v1.33 | Prevent PV leaks | GA |
| v1.33 | Mutable CSI Node Allocatable | Alpha |
| v1.33 | Storage Capacity Scoring | Alpha |
| v1.33 | Image Volumes | Beta |
| v1.34 | Recovery from Volume Expansion Failure | GA |
| v1.34 | Volume Group Snapshots | v1beta2 |
| v1.34 | Mutable CSI Node Allocatable | Beta |
| v1.34 | VolumeAttributesClass for modification | GA |
| v1.35 | Mutable PV Node Affinity | Alpha |
| v1.35 | Better SA Token passing to CSI Drivers | GA |
| v1.36 | Volume Group Snapshots | GA |
| v1.36 | SELinux volume label changes | GA |

---

## 9. Major CSI Drivers (Production-Ready)

### Cloud Provider Drivers
- AWS EBS CSI Driver (kubernetes-sigs/aws-ebs-csi-driver)
- AWS EFS CSI Driver (kubernetes-sigs/aws-efs-csi-driver)
- GCE PD CSI Driver (kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)
- Azure Disk CSI Driver (kubernetes-sigs/azuredisk-csi-driver)
- Azure File CSI Driver (kubernetes-sigs/azurefile-csi-driver)
- OpenStack Cinder CSI (kubernetes/cloud-provider-openstack)

### Storage Vendor Drivers
- Ceph CSI (ceph/ceph-csi) — RBD + CephFS
- Longhorn (longhorn/longhorn)
- NetApp Trident (NetApp/trident)
- Dell CSI (dell/csi-drivers)
- vSphere CSI (kubernetes-sigs/vsphere-csi-driver)
- Portworx CSI
- Pure Storage CSI
- NFS CSI Driver (kubernetes-csi/csi-driver-nfs)
- SMB CSI Driver (kubernetes-csi/csi-driver-smb)

### Special Purpose Drivers
- Hostpath CSI (kubernetes-csi/csi-driver-host-path) — testing/dev
- Local PV CSI (kubernetes-sigs/sig-storage-local-static-provisioner)
- LVM CSI (openebs/lvm-localpv)
- ZFS CSI (openebs/zfs-localpv)
- OpenEBS MayaStor

Source: https://kubernetes-csi.github.io/docs/drivers.html

---

## 10. Best Practices Summary

1. **Always use CSI** — in-tree drivers are fully migrated; CSI is the only path forward
2. **Install snapshot controller + CRDs** — required for any snapshot functionality
3. **Use WaitForFirstConsumer** — ensures topology-aware provisioning and avoids scheduling failures
4. **Enable allowVolumeExpansion** — on StorageClasses where volumes may need resizing
5. **Implement VolumeGroupSnapshots** — for consistent multi-volume backups
6. **Use VolumeAttributesClass** — for post-creation volume attribute modification
7. **Set fsGroupPolicy on CSIDriver** — explicitly control FSGroup behavior for security
8. **Enable storage capacity tracking** — lets scheduler make informed decisions
9. **Use Pod Security Standards** — restrict inline CSI ephemeral volumes in production
10. **Monitor volume health** — deploy health monitor sidecars for proactive alerting

---

Research Sources:
- https://kubernetes.io/docs/concepts/storage/
- https://kubernetes.io/docs/concepts/storage/persistent-volumes/
- https://kubernetes.io/docs/concepts/storage/volume-snapshots/
- https://kubernetes.io/docs/concepts/storage/ephemeral-volumes/
- https://kubernetes.io/docs/concepts/storage/storage-capacity/
- https://kubernetes-csi.github.io/docs/
- https://kubernetes-csi.github.io/docs/drivers.html
- https://kubernetes.io/blog/ (blog index for v1.30 through v1.36)
