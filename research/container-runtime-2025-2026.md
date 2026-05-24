# Kubernetes Container Runtime Research 2025-2026
## Structured Findings with Source URLs

---

## 1. CONTAINERD 2.x

### Release Timeline
- **containerd 2.0**: Released November 5, 2024 (LTS until March 2027)
- **containerd 2.1**: Released May 7, 2025 (End of Life May 5, 2026)
- **containerd 2.2**: Released November 5, 2025 (Active until November 2026)
- **containerd 2.3**: Released April 30, 2026 (LTS until April 2028) — first release with 4-month cadence synchronized with Kubernetes
- **containerd 2.4**: Scheduled August 26, 2026 (tentative)

### Key Features (containerd 2.x)
- **CRI v1 GA**: Since containerd 2.0, CRI v1 is the default and only supported CRI API version. CRI v1alpha2 is removed. All Kubernetes versions 1.32+ use CRI v1.
- **Sandbox API**: containerd 2.0 introduced the native Sandbox API, decoupling sandbox lifecycle from container lifecycle. This enables cleaner integration with Kata Containers, WASM runtimes, and other non-traditional sandbox implementations.
- **nerdctl**: The containerd-native CLI tool (nerdctl) continues as the recommended CLI for containerd, providing Docker-compatible commands with enhanced features (image encryption, lazy pulling, etc.).
- **Major breaking changes in 2.0**: Removal of deprecated APIs, plugin interface changes, removal of CRI v1alpha2, configuration file format updates.
- **Release cadence**: Starting with v2.3 (April 2026), containerd switched to a 4-month release cadence (April, August, December) synchronized with Kubernetes releases.

### Kubernetes Compatibility Matrix
| Kubernetes | containerd versions | CRI Version |
|------------|-------------------|-------------|
| 1.32       | 2.1.0+, 2.0.1+, 1.7.24+ | v1 |
| 1.33       | 2.1.0+, 2.0.4+, 1.7.24+ | v1 |
| 1.34       | 2.1.3+, 2.0.6+, 1.7.28+ | v1 |
| 1.35       | 2.2.0+, 2.1.5+, 1.7.28+ | v1 |
| 1.36       | 2.3.0+, 2.2.0+          | v1 |

### Sources
- https://github.com/containerd/containerd/blob/main/RELEASES.md
- https://github.com/containerd/containerd/releases/tag/v2.0.0
- https://github.com/containerd/containerd/milestone/51

---

## 2. CRI-O EVOLUTION

### Overview
- CRI-O remains the alternative CRI-compliant container runtime, primarily maintained by Red Hat for OpenShift.
- CRI-O follows Kubernetes version numbering (e.g., CRI-O 1.33 for Kubernetes 1.33).
- CRI-O 1.33+ uses CRI v1 exclusively, aligned with containerd.

### Key Developments 2025-2026
- CRI-O adopted the same Sandbox API abstractions for improved VM-based runtime support.
- Continued focus on stability and OpenShift integration rather than broad ecosystem features.
- Support for User Namespaces, confidential containers, and WASM via runtime classes.
- CRI-O 1.35+ supports crun as the default OCI runtime on RHEL-based systems.

### Sources
- https://github.com/cri-o/cri-o
- https://kubernetes.io/docs/setup/production-environment/container-runtimes/

---

## 3. DOCKER RUNTIME (MOBY)

### Current Status
- Docker Engine (Moby) is no longer a direct Kubernetes container runtime since dockershim removal in Kubernetes 1.24.
- Docker remains relevant through containerd (which Docker uses internally).
- Moby project continues as the upstream for Docker Engine.
- Docker Desktop includes containerd as the container runtime.
- Docker's direct integration with Kubernetes is via the containerd CRI plugin.

### Sources
- https://github.com/moby/moby
- https://kubernetes.io/blog/2022/05/03/dockershim-historical-context/

---

## 4. CRUN vs RUNC PERFORMANCE

### Overview
- **runc**: The reference OCI runtime implementation (Go). Default for containerd and Docker.
- **crun**: Alternative OCI runtime written in C by Red Hat (Giuseppe Scrivano).

### Performance Comparison
- crun is **2-5x faster** than runc for container startup time due to lower overhead from C implementation vs Go runtime.
- crun has significantly **lower memory footprint** (~1MB vs ~10MB for runc).
- crun supports **WASM/WASI workloads** natively (as of crun 1.8+), making it a dual-purpose OCI + WASM runtime.
- crun is the **default OCI runtime in RHEL 9+** and OpenShift 4.14+.
- crun supports **cgroup v2 exclusively** as of recent versions, while runc still supports both v1 and v2.
- For high-density deployments (thousands of pods per node), crun's lower overhead becomes significant.

### Recommendation for 2025-2026
- Use crun for new deployments prioritizing performance and WASM support.
- Use runc for maximum compatibility and when cgroup v1 support is still needed.

### Sources
- https://github.com/containers/crun
- https://github.com/opencontainers/runc
- https://www.redhat.com/en/blog/crun-and-runc-what-difference

---

## 5. USER NAMESPACES GA (KEP-127)

### Milestone
- **Alpha**: Kubernetes 1.25
- **Beta**: Kubernetes 1.35
- **Stable (GA)**: Kubernetes 1.36
- **Status**: Implemented
- **Feature Gate**: `UserNamespacesSupport` (kubelet, kube-apiserver)

### What It Enables
- Pods can run with user namespaces, mapping root inside the container to an unprivileged user on the host.
- Significantly improves container security by reducing the impact of container escapes.
- Uses Linux kernel's user namespace support and idmap mounts for volumes.
- Requires container runtime support (containerd 2.x with CRI v1, CRI-O 1.35+).
- Integrates with Pod Security Standards (PSS).

### Key Details
- The Pod spec gains a `hostUsers` field (set to `false` to enable user namespaces).
- Volumes use idmap mounts to handle UID/GID translation transparently.
- Non-conformant volume types may have limitations.

### Sources
- https://github.com/kubernetes/enhancements/blob/master/keps/sig-node/127-user-namespaces/README.md
- https://github.com/kubernetes/enhancements/blob/master/keps/sig-node/127-user-namespaces/kep.yaml
- https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/ (documented as stable in 1.36)

---

## 6. OCI RUNTIME SPECIFICATION

### Current State
- The OCI Runtime Spec defines the configuration, execution environment, and lifecycle of containers.
- Supports platforms: Linux, Windows, FreeBSD, Solaris, z/OS, VM.
- Key documents: runtime.md, config.md, features.md, plus platform-specific docs.

### Recent Developments (2025-2026)
- The spec continues to evolve with improvements for:
  - **User namespace mapping** in config-linux
  - **cgroup v2** as the primary cgroup driver
  - **VM platform** for lightweight VM runtimes like Kata
  - **features.md** for runtime capability discovery (used by containerd to query runtime features)
- The OCI image spec and distribution spec are maintained separately but coordinate with the runtime spec.

### Sources
- https://github.com/opencontainers/runtime-spec/blob/main/spec.md
- https://github.com/opencontainers/runtime-spec
- https://github.com/opencontainers/image-spec
- https://github.com/opencontainers/distribution-spec

---

## 7. WASM CONTAINERS

### runwasi (containerd/runwasi)
- A containerd sub-project for running WASM/WASI workloads managed by containerd.
- Implemented as a Rust library for building containerd shims.
- Supports Wasmtime, WasmEdge, and other WASM engines.
- Containerd runtime class: `io.containerd.wasmtime.v1`
- Actively maintained with community calls every other Tuesday.

### SpinKube
- **containerd-shim-spin**: Provides the containerd shim for Fermyon Spin workloads.
- Enables running Spin applications on Kubernetes via runtime classes.
- Latest shim version v0.24.0 uses Spin v3.6.3.
- Architecture: Install shim on K8s nodes → create RuntimeClass → schedule Spin workloads.
- SpinKube = containerd-shim-spin + Spin Operator for Kubernetes.
- Website: https://www.spinkube.dev/

### Other WASM Shims
- **deislabs/containerd-wasm-shims**: Additional WASM runtime shims for containerd.
- **Spin Operator**: Kubernetes operator for managing Spin applications.

### Maturity
- WASM containers are still early-stage but production-viable for specific workloads.
- Key advantages: Sub-millisecond cold start, small memory footprint, sandboxed by design.
- Limitations: No networking stack matching containers, limited filesystem access, ecosystem maturity.

### Sources
- https://github.com/containerd/runwasi
- https://github.com/spinkube/containerd-shim-spin
- https://www.spinkube.dev/
- https://github.com/deislabs/containerd-wasm-shims
- https://runwasi.dev/

---

## 8. CONFIDENTIAL CONTAINERS (CoCo)

### Overview
- **Confidential Containers (CoCo)**: CNCF project for running containers in Trusted Execution Environments (TEEs).
- Uses hardware-based isolation (Intel TDX, AMD SEV-SNP, ARM CCA, IBM SE).
- Two main runtime backends: Kata Containers (VM-based) and simple-kbs.

### Kata Containers
- Lightweight VMs that feel and perform like containers.
- Supports x86_64 (Intel VT-x, AMD SVM), aarch64, ppc64le, s390x.
- Integrated with containerd via the Sandbox API (containerd 2.x).
- Kata 3.x series is the current major version line.
- Uses QEMU, Cloud Hypervisor, or Firecracker as VMM backends.

### CoCo Architecture
- **Trustee**: Attestation and secret management service.
- **Attested Containers**: Containers that can prove their integrity to remote services.
- **CDH (Confidential Data Hub)**: In-VM service for secret delivery.
- **KBS (Key Broker Service)**: Server-side attestation and key distribution.

### Roadmap Focus Areas
- End-to-end confidential container deployment stability.
- GPU passthrough for confidential AI/ML workloads.
- Multi-arch support improvements.
- Integration with cloud provider TEE offerings (Azure, GCP, AWS).

### Sources
- https://github.com/confidential-containers/confidential-containers
- https://github.com/confidential-containers/confidential-containers/blob/main/roadmap.md
- https://github.com/kata-containers/kata-containers
- https://confidentialcontainers.org/
- https://katacontainers.io/

---

## 9. IMAGE LAZY PULLING

### Nydus (containerd/nydus-snapshotter)
- **Most mature** lazy pulling solution, a containerd non-core sub-project.
- Implements RAFS (Registry Acceleration File System) format.
- Chunk-based content-addressable filesystem.
- Runtime backends: FUSE, virtiofs, in-kernel EROFS.
- Supports lazy pulling: containers can start before full image is downloaded.
- Also supports (e)Stargz and OCI lazy pulling via zran **without explicit conversion**.
- Nydus snapshotter is a containerd proxy plugin.
- Requires containerd 1.4.0+.

### Stargz / eStargz (containerd/stargz-snapshotter)
- Part of the "eStargz" standard for lazy-pullable OCI images.
- Enables random access to layer contents without full download.
- Integrated into containerd as an alternative snapshotter.
- Compatible with standard OCI registries.
- eStargz is the enhanced version with content verification.

### OverlayBD
- Developed by Alibaba (containerd/overlaybd).
- Block-device-based layered filesystem for containers.
- Enables on-demand reading of image layers.
- Particularly effective for large images (AI/ML workloads).
- Integrates with containerd via the transfer service.

### Performance Benefits
- Container cold start time reduction: typically 50-80% faster for large images.
- Network bandwidth savings: only fetch required chunks.
- Particularly beneficial for: AI/ML images (multi-GB), CI/CD pipelines, serverless/FaaS.

### Sources
- https://github.com/containerd/nydus-snapshotter
- https://nydus.dev/
- https://github.com/containerd/stargz-snapshotter
- https://github.com/containerd/overlaybd
- https://github.com/dragonflyoss/nydus

---

## SUMMARY TABLE

| Technology | Maturity (2025-2026) | Key Version | Status |
|-----------|---------------------|-------------|--------|
| containerd 2.x | Production | 2.3 LTS (Apr 2026) | Active, 4-month cadence |
| CRI v1 | GA | Default since containerd 2.0 | Stable |
| Sandbox API | GA | containerd 2.0+ | Stable |
| CRI-O | Production | 1.35+ | Active, K8s-aligned |
| Docker/Moby | Indirect (via containerd) | N/A | Not a K8s runtime directly |
| crun | Production | Latest | Default in RHEL 9+ |
| runc | Production | Latest | Default in containerd/Docker |
| User Namespaces | GA (K8s 1.36) | KEP-127 | Stable |
| OCI Runtime Spec | Stable | Latest | Evolving |
| runwasi | Early production | Latest | Active development |
| SpinKube | Early production | v0.24.0 | Growing ecosystem |
| Kata Containers | Production | 3.x | Active |
| CoCo | Beta | Latest | Maturing |
| Nydus | Production | Latest | Containerd sub-project |
| Stargz/eStargz | Production | Latest | OCI-compatible |
| OverlayBD | Beta/Production | Latest | Alibaba-backed |

---

*Research compiled: May 24, 2026*
*Sources: Official project repositories, Kubernetes Enhancement Proposals, containerd RELEASES.md*
