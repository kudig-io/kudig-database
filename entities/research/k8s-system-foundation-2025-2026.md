---
title: K8S System Foundation 2025 2026
summary: 'Generated: 2025-05-24 Status: Research findings from web sources + K8s documentation'
category: entities
tags:
- k8s-system-foundation-2025-2026
tier: supporting
created: '2026-07-01'
---

# Kubernetes System Foundation Research 2025-2026
# Hardware Trends, Linux Kernel for K8S, K8S Events & Audit

Generated: 2025-05-24
Status: Research findings from web sources + K8s documentation

---

## 1. HARDWARE TRENDS (2025-2026)

### 1.1 DPU / SmartNIC

**NVIDIA BlueField-4 DPU**
- Latest generation DPU (Data Processing Unit) from NVIDIA networking (formerly Mellanox)
- Integrates ARM cores (up to 480 Armv9 Neoverse N2 cores), ConnectX-8 SmartNIC, accelerators
- Purpose: offload networking, storage, security from host CPU
- DOCA SDK 3.0 (2025) provides software framework for DPU programming
- K8S integration: BlueField can run containerized network functions, act as a dedicated node for control plane
- NVIDIA DOCA drivers integrate with Kubernetes via SR-IOV and hardware device plugins
- Use cases: OVS offload, RDMA for AI/ML networking, firewall/crypto offload, storage virt
- BlueField-3 was GA in 2024; BlueField-4 announced 2025 with ConnectX-8
- Source: https://docs.nvidia.com/networking/
- Source: https://developer.nvidia.com/networking/doca

**AMD Pensando DSC (Distributed Services Card)**
- Acquired by AMD in 2022; now integrated into AMD data center strategy
- Pensando Elba/Salina DPUs with P4-programmable pipeline
- Used in Azure (SmartNIC), VMware vSphere integration
- Capabilities: stateful firewall, NAT, telemetry, encryption offload
- K8S integration via SR-IOV device plugin and CNI plugins
- Source: https://www.amd.com/en/products/accelerators/pensando.html

**Kubernetes DPU Integration Patterns**
- DPU as a dedicated management node (runs kubelet, network services)
- Device Plugin API exposes DPU resources (crypto engines, DMA channels)
- SR-IOV CNI for high-performance VF passthrough to pods
- Metal3 + DPU for bare-metal lifecycle management
- TNSR/FRR running on DPU for advanced routing offload
- Source: https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/

### 1.2 GPU Evolution

**NVIDIA Data Center GPUs (2024-2026 roadmap)**
| GPU     | Architecture | HBM   | TDP   | Status (mid-2025) |
|---------|-------------|-------|-------|-------------------|
| H100    | Hopper      | 80GB HBM3 | 700W | Widely deployed |
| H200    | Hopper      | 141GB HBM3e | 700W | GA shipping |
| B100    | Blackwell   | 192GB HBM3e | 700W | GA / early deployment |
| B200    | Blackwell   | 192GB HBM3e | 1000W | GA shipping |
| GB200   | Blackwell (superchip) | 384GB HBM3e | 1200W | NVL72 racks |
| B300    | Blackwell Ultra | 288GB HBM3e | TBD   | Announced 2025 |

- Blackwell introduces FP4 tensor cores, 2nd gen Transformer Engine
- NVLink 5.0 with 1.8TB/s bidirectional bandwidth per GPU
- NVSwitch for 72-GPU NVL72 superpod configurations
- K8S integration: NVIDIA GPU Operator (v24.x), NVIDIA Device Plugin, MIG (Multi-Instance GPU)
- DRA (Dynamic Resource Allocation) for GPU scheduling in K8S 1.34+
- Source: https://developer.nvidia.com/data-center
- Source: https://docs.nvidia.com/datacenter/cloud-native/

**AMD Instinct MI300 Series**
| GPU        | HBM     | TDP  | Status |
|------------|---------|------|--------|
| MI300X     | 192GB HBM3 | 750W | GA |
| MI300A     | 128GB HBM3 | 760W | GA (APU: CPU+GPU) |
| MI350      | HBM3e   | TBD  | Expected 2025 |

- MI300X: 192GB HBM3 = largest single-GPU memory (as of early 2025)
- ROCm 6.x software stack for Kubernetes: AMD GPU Operator, K8S device plugin
- CDNA 3 architecture with enhanced matrix cores for AI
- Source: https://www.amd.com/en/products/accelerators/instinct.html

### 1.3 ARM64 Servers

**Ampere Altra / AmpereOne**
- Ampere Altra Max: 128 cores, single socket, 128 PCIe Gen4 lanes
- AmpereOne (2024): up to 192 cores, 2MB L2 per core, DDR5
- AmpereOne Aurora (2025): announced with AI acceleration
- Power efficiency advantage: 2-3x perf/watt vs x86 for cloud-native workloads
- Source: https://amperecomputing.com/

**AWS Graviton4**
- Announced re:Invent 2023, GA in 2024-2025
- Up to 96 Neoverse V2 cores per socket
- 50% more compute than Graviton3, DDR5-5600 memory
- R8g, M8g, C8g instances available
- EKS optimized AMI for Graviton with K8S multi-arch support
- Source: https://aws.amazon.com/ec2/graviton/

**Azure Cobalt 100**
- Microsoft's first custom ARM64 CPU (announced 2023, GA 2024)
- 128 Neoverse N2 cores, DDR5, PCIe Gen5
- Powers Azure Cobalt VMs (Dpsv6, Epsv6 series)
- AKS support for Cobalt-based node pools
- Source: https://learn.microsoft.com/en-us/azure/virtual-machines/cobalt-100

**K8S ARM64 Considerations**
- Multi-arch container images (linux/amd64,linux/arm64) now standard
- K8S upstream fully supports arm64 since v1.22
- EKS, AKS, GKE all support ARM64 node pools
- Buildx/QEMU for cross-compilation; many base images now multi-arch
- Key challenge: third-party operators/controllers may lag on arm64 builds

### 1.4 Confidential Computing Hardware

**Intel TDX (Trust Domain Extensions)**
- Hardware-isolated VMs (Trust Domains) with encrypted memory
- Available on 4th/5th Gen Intel Xeon (Sapphire Rapids / Emerald Rapids)
- TDX Module encrypts VM memory; attestation via Intel TDX module
- Azure Confidential VMs (DCasv5/ECasv5 series) use Intel TDX
- K8S integration: Kata Containers + TDX, CoCo (Confidential Containers) project
- Source: https://www.intel.com/content/www/us/en/developer/tools/trust-domain-extensions/overview.html

**AMD SEV-SNP (Secure Encrypted Virtualization - Secure Nested Paging)**
- Memory encryption per-VM with integrity protection
- Available on AMD EPYC 4th Gen (Genoa/Berga)
- SEV-SNP prevents hypervisor from reading/modifying guest memory
- Azure, GCP offer SEV-SNP confidential VMs
- K8S integration: Kata Containers, Confidential Containers (CoCo) project
- Source: https://www.amd.com/en/developer/sev.html

**K8S Confidential Containers (CoCo)**
- CNCF sandbox project: https://github.com/confidential-containers
- Integrates TDX, SEV-SNP, IBM Secure Execution
- RuntimeClass for confidential workloads
- Attestation service for verifying TEE integrity
- K8S 1.34+: CoCo becoming more mature for production use

### 1.5 CXL (Compute Express Link) Memory Pooling

**CXL 2.0/3.0 Overview**
- CXL 2.0: memory pooling via CXL switches, type 3 devices (memory expanders)
- CXL 3.0: fabric capabilities, multi-headed devices, enhanced pooling
- CXL Type 3 devices: Samsung CMM-H, SK Hynix CMS, Micron CZ120
- Enables memory disaggregation: attach remote memory as local NUMA node
- Linux kernel: CXL support in 6.x kernels (cxl_mem, cxl_port drivers)
- K8S impact: topology manager can be aware of CXL NUMA topology
- Source: https://www.computeexpresslink.org/
- Source: https://www.samsung.com/semiconductor/dram/cxl/

**CXL & K8S (2025-2026)**
- CXL memory appears as additional NUMA nodes to the OS
- Kubernetes Topology Manager and NUMA-aware scheduling can leverage CXL memory
- No direct K8S CXL API yet; managed via kernel/hardware topology
- Potential for memory tiering: fast local DRAM + CXL-pooled memory
- Early adopters: hyperscalers testing CXL memory expanders

---

## 2. LINUX KERNEL FOR KUBERNETES (2025-2026)

### 2.1 cgroup v2 GA Adoption

**Status in K8S (2025)**
- cgroup v2 has been the default cgroup driver since K8S 1.25 (when systemd is init)
- containerd defaults to cgroup v2 since 2.0
- Most modern Linux distros (Ubuntu 22.04+, RHEL 9, Fedora 36+) default to cgroup v2
- cgroup v1 is deprecated; still supported but cgroup v2 recommended
- K8S 1.36 (current): cgroup v2 is the expected default
- Source: https://kubernetes.io/docs/concepts/architecture/cgroups/

**Key cgroup v2 features for K8S**
- Unified hierarchy (single tree vs v1's multiple hierarchies)
- PSI (Pressure Stall Information) support (see below)
- Memory.high (soft limit), memory.low (protection), memory.max (hard limit)
- io controller for block I/O bandwidth control
- cpu.weight replaces cpu.shares
- rdma controller for RDMA resource accounting
- freezer controller for pausing cgroups

**Migration considerations**
- Some older monitoring tools (Prometheus node_exporter < 1.5) may have issues
- cadvisor updated for cgroup v2; kubelet metrics work correctly
- Memory accounting differences: cgroup v2 uses memory.current vs v1's memory.usage_in_bytes

### 2.2 eBPF Kernel Features (5.x - 6.x)

**Key eBPF capabilities evolution**
| Kernel | Feature |
|--------|---------|
| 5.3    | BPF ring buffer (bpf_ringbuf) |
| 5.5    | BPF LSM (Linux Security Module) hooks |
| 5.8    | BPF iterator, BPF timers, struct_ops |
| 5.10   | BPF arena, CO-RE improvements |
| 5.15   | BPF_MAP_TYPE_RINGBUF, BPF LSM |
| 6.0    | BPF memory allocator improvements |
| 6.1    | BPF arena (type-generic maps) |
| 6.3    | BPF cookie, kfunc improvements |
| 6.4    | BPF exceptions |
| 6.6    | BPF JIT improvements |
| 6.8+   | BPF token, better delegation |

**K8S eBPF ecosystem**
- Cilium: primary eBPF-based CNI for K8S; replaces iptables/kube-proxy
  - Cilium 1.16+ (2025): Gateway API, Service Mesh, Hubble observability
  - Source: https://docs.cilium.io/
- Tetragon: eBPF-based security observability from Cilium/Isovalent
  - Process execution, file access, network monitoring without sidecars
- Calico eBPF dataplane: alternative eBPF CNI
- Pixie: eBPF-based auto-instrumentation observability
- Falco: uses eBPF for syscall monitoring in K8S

**K8S networking with eBPF**
- Cilium can replace kube-proxy entirely (eBPF-based service routing)
- Bypasses iptables/nftables; O(1) service lookup vs O(n) iptables
- Enables WireGuard encryption, bandwidth manager (EDT/FQ)
- Host-level firewall, DDoS protection via BPF programs
- Kernel 6.1+ recommended for full Cilium features

### 2.3 io_uring for Storage

**io_uring overview**
- Async I/O interface introduced in Linux 5.1
- Submission Queue (SQ) + Completion Queue (CQ) shared between user/kernel
- Significantly lower syscall overhead vs epoll/read/write
- Kernel 5.1-6.x: continuous improvements (fixed files, multishot, poll, etc.)

**io_uring capabilities (6.x)**
- io_uring_cmd (kernel 5.19): passthrough commands to NVMe devices
- Registered buffers and files for zero-copy
- Multishot accept/recv for network servers
- io_uring fixed files and buffers
- Kernel 6.1+: io_uring_zcrx (zero-copy receive)

**K8S / Container relevance**
- containerd: not yet using io_uring by default (2025)
- Storage: io_uring beneficial for high-IOPS container storage
- Application-level: apps can use io_uring via liburing
- Database workloads (PostgreSQL, MySQL) seeing 2-5x improvement with io_uring
- Security concern: io_uring attack surface led to Google disabling it in GKE for a period
- seccomp: io_uring syscalls (io_uring_setup/enter/register) must be allowed in profiles
- K8S 1.35+: Pod Security Standards may restrict io_uring in restricted profile

### 2.4 PSI (Pressure Stall Information) GA in K8S 1.36

**CONFIRMED from kubernetes.io: PSI Metrics is a v1.36 feature**

**What PSI provides**
- /proc/pressure/cpu, /proc/pressure/memory, /proc/pressure/io
- Three metrics per resource: some (partial stall), full (complete stall), total (microseconds)
- Measures time tasks are stalled waiting for resources
- Much better signal than raw utilization metrics for detecting contention

**K8S 1.36 PSI integration**
- Kubelet reads PSI metrics from cgroup v2 pressure files
- Exposed as K8S metrics for scheduler decision-making
- Can be used for intelligent eviction (replaces OOM-kill-only approach)
- KEP-4205 (related) and dedicated PSI KEP for node-level pressure signals
- Enables proactive workload migration when nodes are under pressure
- Source: https://kubernetes.io/blog/ (1.36 release notes)

**PSI vs traditional metrics**
| Metric Type   | What it measures         | Problem            |
|--------------|--------------------------|---------------------|
| CPU %        | Utilization              | Ignores latency     |
| PSI CPU some | Tasks waiting for CPU    | Shows real impact   |
| Memory usage | Bytes allocated          | Ignores reclaim     |
| PSI mem full | All tasks blocked on mem | Shows OOM risk      |
| IOPS/through | Storage activity         | Ignores queuing     |
| PSI IO full  | All tasks blocked on I/O | Shows storage stall |

### 2.5 User Namespaces GA in K8S 1.36

**CONFIRMED from kubernetes.io: User Namespaces is STABLE in K8S v1.36**

**Source**: https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/
- Feature State: Kubernetes v1.36 [stable] (enabled by default)

**What User Namespaces provide**
- Isolate container user from host user
- Root (UID 0) in container maps to non-root user on host
- Full privileges inside namespace; unprivileged outside
- Mitigates container escape vulnerabilities
- Requires: Linux kernel >= 6.3 (recommended 6.6+), CRI runtime support

**K8S implementation**
- Pod.spec.hostUsers: false (opt-in per pod)
- Kubelet manages UID/GID mapping between container and host
- Works with cgroup v2 (required for modern container runtimes)
- Supports overlayfs and most volume types
- Source: https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/

**Security impact**
- Dramatically reduces container escape risk
- Root in container != root on host
- Combined with seccomp and AppArmor, provides defense-in-depth
- Critical for multi-tenant K8S clusters

### 2.6 Kernel Same-Page Merging (KSM) for AI Workloads

**KSM overview**
- Kernel feature: scans memory for identical pages, merges them
- Reduces memory usage when multiple processes share identical data
- /sys/kernel/mm/ksm/ directory controls behavior

**KSM for AI/ML workloads**
- Multiple model replicas can share identical model weights via KSM
- Reduces GPU memory pressure by deduplicating host-side model data
- containerd supports KSM via annotations
- K8S: can enable KSM via kubelet --kernel-memcg-notification or node labels
- AI frameworks (vLLM, TensorRT-LLM) benefit from KSM for serving multiple LLM copies
- Drawback: CPU overhead for scanning, potential side-channel attacks (security risk)

**K8S KSM integration**
- kubelet feature: MemoryQoS with kernel memory notifications
- KEP-2570: KSM awareness in K8S scheduling
- Not yet a first-class K8S API; controlled via node-level kernel settings

### 2.7 NUMA Topology Management

**K8S Topology Manager**
- GA since K8S 1.27; critical for HPC, AI/ML, NFV workloads
- Policies: none, best-effort, restricted, single-numa-node
- Coordinates CPU, device, memory alignment to same NUMA node
- Source: https://kubernetes.io/docs/tasks/administer-cluster/topology-manager/

**Evolution (2025-2026)**
- Memory Manager: guarantees memory allocated from same NUMA
- Device Manager: GPU/NVMe device NUMA affinity
- K8S 1.34+: improved NUMA-aware scheduling with DRA (Dynamic Resource Allocation)
- CXL impact: new NUMA nodes from CXL memory expanders
- Multi-socket and NUMA-aware pod placement for low-latency workloads

---

## 3. KUBERNETES EVENTS & AUDIT (2025-2026)

### 3.1 Structured Audit Logging

**Source**: https://kubernetes.io/docs/tasks/debug/debug-cluster/audit/

**Audit Policy Levels**
| Level         | What is logged |
|---------------|---------------|
| None          | Don't log |
| Metadata      | Request metadata (who, when, what resource) |
| Request       | Metadata + request body |
| RequestResponse | Metadata + request + response body |

**Audit Event structure (JSON)**
```json
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "level": "RequestResponse",
  "auditID": "...",
  "stage": "ResponseComplete",
  "requestURI": "...",
  "verb": "create",
  "user": { "username": "...", "groups": [...] },
  "sourceIPs": [...],
  "objectRef": { "resource": "pods", "namespace": "default", ... },
  "responseStatus": { "code": 201 },
  "requestObject": { ... },
  "responseObject": { ... },
  "requestReceivedTimestamp": "...",
  "stageTimestamp": "..."
}
```

**Backend options**
- Log backend: writes to file (structured JSON lines)
- Webhook backend: sends to external service (Falco, Splunk, ELK)
- Dynamic audit configuration (beta): AuditSink API for runtime policy changes

### 3.2 Event Compression & Management

**K8S Events API (core/v1 Event)**
- Events have TTL (default 1 hour, kube-apiserver --event-ttl)
- Events are stored in etcd; high-churn clusters can overwhelm etcd
- Source: https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/

**Event compression (K8S 1.30+)**
- KEP-3836: deduplication of similar events
- Events with same reason/object are compressed into count
- Reduces etcd storage and API server load
- Event series: incrementing count field instead of new events

**K8S 1.35-1.36 Event improvements**
- Server-Side Apply for events (better deduplication)
- Mutable events (KEP in progress): allow updating events instead of creating new ones
- Event backoff: rate limiting for high-frequency events
- Priority and fairness for event API requests

### 3.3 Kubernetes Events API Evolution

**Current state (K8S 1.36)**
- core/v1 Event: basic event object
- events.k8s.io/v1 Event: enhanced event API (GA since 1.19)
- Events.k8s.io/v1beta1: deprecated
- KEP-3836: event lifecycle improvements

**Key improvements**
- Event deduplication at source (controller-runtime, client-go)
- Event recorder improvements: batching, compression
- Better event attribution (reportingController, reportingInstance)
- Watch-based event consumption for real-time monitoring

### 3.4 Cloud Audit Log Integration

**GKE Audit Logs**
- Admin Activity logs (always on)
- Data Access logs (configurable per API)
- Export to Cloud Logging, Pub/Sub, BigQuery
- K8S audit events map to GCP audit log format
- Source: https://cloud.google.com/kubernetes-engine/docs/how-to/audit-logging

**EKS Audit Logs**
- Control plane logs: API server, audit, authenticator, controller manager, scheduler
- CloudWatch Logs integration
- FireLens/Fluent Bit for forwarding
- K8S audit policy file can be customized
- Source: https://docs.aws.amazon.com/eks/latest/userguide/control-plane-logs.html

**AKS Audit Logs**
- kube-audit and kube-audit-admin log categories
- Azure Monitor / Log Analytics integration
- Azure Policy for K8S audit compliance
- Source: https://learn.microsoft.com/en-us/azure/aks/view-control-plane-logs

### 3.5 SIEM Integration Patterns

**Common architecture**
```
kube-apiserver --> audit webhook/log --> Fluentd/Fluent Bit --> SIEM
                                              |
                                        Filter/Transform
                                              |
                              Splunk / Elastic / Sentinel / Chronicle
```

**Pattern 1: Audit webhook to Fluentd/Fluent Bit**
- Fluentd/Fluent Bit DaemonSet on K8S nodes
- audit-webhook-config-file points to Fluentd endpoint
- Parse JSON audit events, enrich with K8S metadata
- Forward to SIEM (Splunk, Elasticsearch, etc.)

**Pattern 2: Falco + Audit Events**
- Falco integrates audit events with syscall monitoring
- Falco Rules for K8S-specific threats
- Source: https://falco.org/docs/

**Pattern 3: Cloud-native SIEM**
- Azure Sentinel: native K8S audit log connector
- Google Chronicle: GKE audit log integration
- AWS Security Lake: EKS audit log ingestion
- Datadog Security: K8S audit event monitoring

**Pattern 4: OpenTelemetry Collector**
- K8S audit events -> OTel Collector -> any backend
- K8S cluster receiver for events
- Source: https://opentelemetry.io/docs/

**Best practices (2025)**
1. Use RequestResponse level only for sensitive resources (secrets, RBAC)
2. Use Metadata level for most resources to reduce volume
3. Stream audit logs to external system immediately (don't rely on etcd storage)
4. Include user agent and source IP in audit policy
5. Monitor audit log volume to detect attacks
6. Correlate K8S audit events with container runtime audit (sysdig, falco)
7. Implement alerting on suspicious patterns (privilege escalation, secret access)

---

## SOURCE URLS

### K8S Official Documentation
- https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/ (GA in 1.36)
- https://kubernetes.io/docs/tasks/debug/debug-cluster/audit/
- https://kubernetes.io/docs/concepts/architecture/cgroups/
- https://kubernetes.io/docs/concepts/scheduling-eviction/ (PSI reference)
- https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/

### K8S GitHub
- https://github.com/kubernetes/kubernetes/blob/master/CHANGELOG/CHANGELOG-1.36.md
- https://github.com/kubernetes/enhancements/blob/master/keps/sig-node/4205-user-namespaces/README.md

### Hardware / Vendor
- https://docs.nvidia.com/networking/ (BlueField DPU docs)
- https://developer.nvidia.com/data-center/ (GPU architecture)
- https://docs.nvidia.com/datacenter/cloud-native/ (K8S GPU operator)
- https://www.amd.com/en/products/accelerators/instinct.html (MI300)
- https://amperecomputing.com/ (ARM64 servers)
- https://aws.amazon.com/ec2/graviton/ (Graviton4)
- https://learn.microsoft.com/en-us/azure/virtual-machines/cobalt-100
- https://www.computeexpresslink.org/ (CXL)
- https://www.amd.com/en/developer/sev.html (SEV-SNP)
- https://www.intel.com/content/www/us/en/developer/tools/trust-domain-extensions/overview.html (TDX)

### eBPF / Networking
- https://docs.cilium.io/ (Cilium eBPF CNI)
- https://ebpf.io/ (eBPF overview)

### Cloud Provider K8S
- https://cloud.google.com/kubernetes-engine/docs/how-to/audit-logging
- https://docs.aws.amazon.com/eks/latest/userguide/control-plane-logs.html
- https://learn.microsoft.com/en-us/azure/aks/view-control-plane-logs

### Security / Audit
- https://falco.org/docs/ (Falco K8S audit)
- https://opentelemetry.io/docs/ (OTel for K8S events)

---

## KEY TAKEAWAYS FOR KUDIG DATABASE

1. **K8S 1.36 confirmed features**: User Namespaces (stable, enabled by default) and PSI Metrics are GA in v1.36.

2. **Hardware is diversifying**: DPU/SmartNIC adoption accelerating; ARM64 becoming mainstream; CXL memory emerging; confidential computing hardware (TDX, SEV-SNP) enabling secure multi-tenancy.

3. **Kernel foundation is critical**: cgroup v2 is now the de facto standard; eBPF (via Cilium) is replacing iptables; io_uring improving storage performance; PSI providing better resource pressure signals.

4. **Audit & Events maturing**: Structured audit logging is well-established; event compression reducing etcd load; SIEM integration via Fluent Bit/OTel is the standard pattern.

5. **AI/ML infrastructure driving changes**: KSM for memory efficiency; NUMA topology management for GPU affinity; DRA for dynamic GPU allocation; DPU for network-intensive AI workloads.
