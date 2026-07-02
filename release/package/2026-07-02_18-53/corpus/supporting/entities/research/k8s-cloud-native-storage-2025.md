---
title: K8S Cloud Native Storage 2025
summary: 'Date: 2026-05-24 Sources: Official docs, GitHub releases, CNCF landscape,
  community benchmarks'
category: entities
tags:
- k8s-cloud-native-storage-2025
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes Cloud-Native Storage Systems — Research (2025-2026)

Date: 2026-05-24
Sources: Official docs, GitHub releases, CNCF landscape, community benchmarks

---

## 1. LONGHORN (SUSE/Rancher)

### Latest Versions
- Longhorn v1.7.x (stable, released late 2024)
- Longhorn v1.8.x (in development/early 2025)
- Previous stable: v1.6.x

### Architecture
- Lightweight distributed block storage built specifically for Kubernetes
- Uses iSCSI target (tgt) exposed as a StorageClass
- Replica-based: each volume has N replicas spread across nodes
- Built-in backup to NFS/S3 via backup targets
- Longhorn Manager (control plane) + Instance Manager (data plane)
- Engine runs as a long-running process per volume on each node
- Snapshotting, DR volume support, volume cloning

### Production Best Practices
- Minimum 3 nodes with storage for HA (replica count >= 3 recommended)
- Dedicated disk/partition for Longhorn storage (avoid sharing with OS)
- Use dedicated storage network if possible (v1.4+ supports data network)
- Set replica node affinity to spread across failure domains
- Enable recurring snapshots and backups (daily minimum)
- Use StorageClass with `reclaimPolicy: Retain` for critical data
- Set resource limits on Longhorn components
- Monitor disk space — Longhorn pauses at 90% disk usage by default

### Performance
- Good for small-medium workloads, sequential read/write competitive
- Random I/O overhead from iSCSI layer; ~10-15% overhead vs raw disk
- Rebuilding replicas can impact performance under heavy load
- Not ideal for extremely high-IOPS databases (PostgreSQL, etc.) at scale

### HA Patterns
- Automatic replica rebuilding when a node fails
- Non-disruptive volume migration between nodes
- DR volumes for cross-cluster disaster recovery
- Supports RWX (ReadWriteMany) via NFS in v1.4+

### Upgrade Strategy
- Rolling upgrade supported — one node at a time
- Engine image upgrade is non-disruptive for attached volumes
- Always back up volumes before major version upgrades
- Check compatibility matrix for CSI driver version

### Known Pitfalls
- iSCSI dependency: requires open-iscsi or iscsid on all nodes
- RWX performance via NFS is mediocre for high-concurrency
- Replica rebuild time can be long for large volumes (TB-scale)
- Node drain requires careful handling — volumes must be detached first
- ARM64 support matured but test thoroughly
- Can consume significant memory with many volumes (engine per volume)

### Sources
- https://longhorn.io/docs/
- https://github.com/longhorn/longhorn/releases
- https://longhorn.io/docs/1.7.0/deploy/upgrade/

---

## 2. ROOK / CEPH

### Latest Versions
- Rook v1.14.x / v1.15.x (2025)
- Ceph Reef (v18.2.x) — current LTS
- Ceph Squid (v19.x) — released 2024, becoming stable in 2025

### Architecture
- Rook is a Kubernetes operator that deploys and manages Ceph
- Ceph provides: CephFS (filesystem), RBD (block), RGW (object/S3)
- CRD-based: CephCluster, CephBlockPool, CephFilesystem, CephObjectStore
- Monitors (MON), Managers (MGR), OSDs (object storage daemons), MDS
- Full distributed storage with data rebalancing, erasure coding support
- Most mature and feature-rich option

### Production Best Practices
- Minimum 3 nodes, 3 OSDs for quorum (production: 5+ nodes ideal)
- Use dedicated raw disks for OSDs (not partitions)
- Enable device classes (HDD/SSD/NVMe) for tiered storage
- Use CephBlockPool with `replicated.size: 3` minimum
- Tune PG (placement group) counts based on cluster size
- Monitor with Ceph Dashboard + Prometheus/Grafana
- Use PDB (PodDisruptionBudget) for MONs
- Separate public and cluster networks for performance

### Performance
- Highest throughput and IOPS among K8s-native options
- RBD: near-native performance for sequential I/O with NVMe
- CephFS: excellent for shared filesystem workloads
- Erasure coding reduces storage overhead (vs 3x replication)
- Bluestore backend optimized for SSDs/NVMe
- Can saturate 10/25GbE networks with proper tuning

### HA Patterns
- Automatic data rebalancing on node failure
- Multi-site replication via RGW (S3-compatible)
- CephFS active/standby MDS for HA
- Stretch cluster mode (Rook v1.10+) for 2-site HA
- RBD mirroring for cross-cluster disaster recovery

### Upgrade Strategy
- Rook operator handles Ceph upgrade orchestrration
- Ceph supports rolling upgrades — one daemon at a time
- Always upgrade Rook operator first, then Ceph version
- Test in staging — Ceph version upgrades can be complex
- Rook v1.14+ improved upgrade reliability

### Known Pitfalls
- **Complexity**: Steepest learning curve of all options
- Minimum resource requirements: ~2GB RAM per OSD, MON needs stable storage
- Initial deployment can fail if prerequisites not met (kernel modules, etc.)
- Ceph cluster health depends on network stability — latency-sensitive
- OSD replacement requires careful procedure to avoid data loss
- MON quorum loss = cluster becomes read-only/unavailable
- Upgrade path between major Ceph versions can be bumpy
- Not suitable for small clusters (< 3 nodes) or resource-constrained environments

### Sources
- https://rook.io/docs/rook/latest/
- https://github.com/rook/rook/releases
- https://docs.ceph.com/en/latest/releases/

---

## 3. OPENEBS

### Latest Versions
- OpenEBS v4.0+ (2024-2025)
- Mayastor (NVMe-oF engine) — now the primary focus engine
- Legacy: Jiva (deprecated path), cStor (deprecated)
- CNCF Incubating project

### Architecture (2025 — Mayastor-centric)
- Mayastor: high-performance NVMe-oF based storage engine
- Control plane (API Server, etcd/CRDs), Data plane (io-engine, agents)
- Uses NVMe over Fabrics (NVMe-oF/TCP) for data path
- LocalPV variants: OpenEBS LocalPV (hostpath), LVM LocalPV, ZFS LocalPV
- CSI-compliant StorageClasses
- Replicated storage via Mayastor nexus (mirrors replicas across nodes)

### Production Best Practices
- Mayastor requires NVMe disks for optimal performance
- LVM LocalPV is great for single-node persistent storage (simpler than Mayastor)
- ZFS LocalPV provides snapshots, compression, checksumming on host
- For replicated storage, deploy Mayastor with >= 3 io-engine pods
- Pin io-engine pods to storage nodes via node selectors
- Use dedicated network for NVMe-oF replication traffic
- Monitor via Grafana dashboards (provided)

### Performance
- Mayastor: lowest latency among K8s-native replicated storage
- NVMe-oF path eliminates iSCSI overhead
- Benchmarks show near-native NVMe performance (< 10% overhead)
- LocalPV variants: essentially native disk performance (no replication)
- ZFS LocalPV: excellent for snapshot-heavy workloads

### HA Patterns
- Mayastor: configurable replica count (N-way mirroring)
- Automatic failover when replica node goes down
- Nexus reconnects to available replicas
- LocalPV variants have NO built-in HA (local disk only)

### Upgrade Strategy
- Helm-based upgrade for Mayastor
- Rolling restart of io-engine pods
- Nexus can be rebuilt from replicas during upgrade
- LocalPV: upgrade is trivial (CSI driver only)

### Known Pitfalls
- Mayastor is still maturing — smaller community than Longhorn/Rook
- Requires modern kernel (5.10+) with NVMe-oF modules
- cStor and Jiva are deprecated — do NOT use for new deployments
- Replication quality depends heavily on network (use dedicated NIC)
- Documentation quality has improved but still trails Longhorn/Rook
- Mayastor etcd dependency adds operational complexity
- Limited ecosystem of pre-built integrations vs Ceph

### Sources
- https://openebs.io/docs/
- https://github.com/openebs/openebs/releases
- https://mayastor.gitbook.io/

---

## 4. JUICEFS

### Latest Versions
- JuiceFS v1.2.x (2025)
- JuiceFS CSI Driver v0.25+ (2025)
- Community Edition + Enterprise Edition

### Architecture
- Distributed POSIX filesystem — not block storage
- Metadata engine: Redis, TiKV, PostgreSQL, MySQL, SQLite, FoundationDB
- Object storage backend: S3, GCS, Azure Blob, MinIO, Ceph RGW, etc.
- CSI driver provides ReadWriteMany (RWX) and ReadOnlyMany (ROX)
- FUSE-based (FUSE mount per pod) or kernel mount
- Data chunking and deduplication built-in
- Supports JuiceFS Mount Pod mode (shared mount) or Sidecar mode

### Production Best Practices
- Use TiKV or PostgreSQL for metadata (not Redis for production HA)
- Object storage backend should be in same region/AZ for latency
- Use Mount Pod mode (v0.24+) — more resource-efficient than sidecar
- Set appropriate cache size and cache dir (SSD recommended for cache)
- Enable trash/tombstone for data protection
- Use `--cache-partial-only` for streaming workloads
- Monitor metadata engine health — it's the SPOF
- Pin mount pods to specific nodes for predictable performance

### Performance
- Excellent for large-scale file/object workloads
- Sequential throughput competitive with native S3
- Metadata-heavy workloads (many small files) depend on metadata engine
- Cache significantly impacts read performance — warm cache = fast
- Write performance depends on object storage backend latency
- POSIX compliance means it works with any application that uses files
- Multi-client consistency via metadata engine locking

### HA Patterns
- Metadata engine HA: Redis Sentinel/Cluster, PG replication, TiKV
- Object storage backend typically already HA (cloud S3)
- Mount pods can restart on different nodes (metadata handles reconnection)
- No built-in data replication (relies on object storage durability)
- CSI driver supports volume resizing

### Upgrade Strategy
- CSI driver upgrade via Helm rolling update
- JuiceFS client version can be updated independently
- Mount pod restart picks up new client version
- Metadata engine upgrades are separate operations

### Known Pitfalls
- NOT block storage — cannot be used as root filesystem or for databases requiring block devices
- FUSE overhead for small random I/O (not ideal for OLTP databases)
- Metadata engine is critical SPOF — must be HA
- Cold cache performance can be dramatically slower than warm cache
- Network dependency: all reads potentially hit object storage
- File locking semantics differ from local filesystems
- JuiceFS Enterprise vs Community feature gap (enterprise has better management tools)

### Sources
- https://juicefs.com/docs/
- https://github.com/juicedata/juicefs/releases
- https://github.com/juicedata/juicefs-csi-driver/releases

---

## COMPARISON MATRIX

| Feature            | Longhorn          | Rook/Ceph         | OpenEBS (Mayastor) | JuiceFS           |
|--------------------|-------------------|-------------------|---------------------|-------------------|
| Type               | Block             | Block/FS/Object   | Block               | Distributed FS    |
| Complexity         | Low               | High              | Medium              | Medium            |
| Min Nodes          | 3                 | 3 (5+ prod)       | 3                   | 1 (+ metadata)    |
| HA                 | Replica-based     | Ceph-native       | NVMe-oF replicas    | Object storage    |
| RWX Support        | Yes (NFS)         | Yes (CephFS)      | No                  | Yes (native)      |
| Performance        | Good              | Excellent         | Excellent           | Good (file I/O)   |
| Overhead           | Medium            | High              | Low                 | Low-Medium        |
| NVMe Support       | Basic             | Full              | Native (NVMe-oF)   | N/A               |
| Snapshot/Clone     | Yes               | Yes               | Yes                 | Yes (metadata)    |
| Backup to S3       | Built-in          | Via Rook/RGW      | Manual              | N/A (already S3)  |
| Maturity           | High              | Very High         | Growing             | High              |
| CNCF Status        | Sandbox→Incubating| N/A (Ceph=LF)    | Incubating          | CNCF Sandbox      |
| Best For           | General K8s       | Large-scale, multi| High-perf block     | AI/ML, big data   |
|                    | workloads         | protocol needs    | storage             | shared files      |

---

## REAL-WORLD DEPLOYMENT PATTERNS (2025)

### Pattern 1: Longhorn for General Workloads (SMB/Startup)
- 3-10 node clusters, mixed workloads
- Longhorn + Velero for backup
- Simple ops, single team manages everything
- Common in Rancher/RKE2 deployments

### Pattern 2: Rook/Ceph for Enterprise/Private Cloud
- 10+ node clusters, multi-tenant
- Separate storage nodes from compute
- CephFS for shared development environments
- RBD for databases, RGW for application object storage
- Dedicated storage team required

### Pattern 3: OpenEBS LocalPV + Cloud Block Storage
- Use cloud provider block storage (EBS, PD) with OpenEBS LocalPV
- Get snapshots, cloning on top of cloud volumes
- Avoids replication overhead (cloud handles durability)
- Cost-effective hybrid approach

### Pattern 4: JuiceFS for AI/ML and Big Data
- Shared training datasets across GPU nodes
- Metadata in TiKV, data in S3/MinIO
- Cache on local NVMe for hot data
- Replaces NFS for shared filesystem workloads
- Growing adoption in LLM training pipelines

### Pattern 5: Mixed Storage Strategy (Recommended for Production)
- Longhorn/OpenEBS LocalPV for stateful app volumes (databases)
- JuiceFS/CephFS for shared file storage (uploads, assets, ML data)
- Cloud-native object storage (S3/MinIO) for backups and archives
- Separate StorageClasses per tier

---

## 2025-2026 TRENDS

1. **NVMe-oF becoming standard**: OpenEBS Mayastor leading, others adopting
2. **AI/ML driving JuiceFS adoption**: shared file access for GPU workloads
3. **Container Object Storage Interface (COSI)**: emerging standard for S3 in K8s
4. **WASM + edge storage**: lightweight options for edge K8s (k3s + Longhorn dominant)
5. **Cost optimization**: erasure coding gaining traction over 3x replication
6. **Longhorn entering CNCF Incubating**: validates maturity
7. **Rook v1.14+ simplifies deployment**: reducing operational burden


<!-- risk-assessed -->
