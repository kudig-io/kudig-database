# Kubernetes Specialized Technologies 2025-2026

## Research Date: 2026-05-24

---

## 1. eBPF ECOSYSTEM

### 1.1 Cilium
- **Status**: Graduated CNCF project (Oct 2023), acquired by Cisco/Isovalent
- **Version**: Cilium 1.17+ (2025), ongoing development through 2026
- **Key Features**:
  - eBPF-based networking, security, and observability for Kubernetes
  - Replaces kube-proxy with eBPF dataplane
  - Service mesh capabilities (sidecar-free)
  - Multi-cluster networking and encryption (WireGuard, IPsec)
  - Hubble for network observability
  - Gateway API support as default ingress
  - CiliumClusterwideNetworkPolicy for cluster-wide security
- **2025-2026 Trends**:
  - Isovalent Enterprise Platform consolidating Cilium + Tetragon + Hubble
  - AWS Marketplace and Azure Marketplace availability
  - Mesh networking across Kubernetes, cloud, data centers, and legacy on-prem
  - Load balancer modernization for AI workloads
- **Sources**:
  - https://isovalent.com/
  - https://cilium.io/
  - https://github.com/cilium/cilium

### 1.2 Tetragon
- **Status**: CNCF project (sub-project under Cilium)
- **Version**: v1.7.0+ (2025-2026)
- **Key Features**:
  - eBPF-based security observability and runtime enforcement
  - Kubernetes-aware security policies
  - Process execution monitoring
  - File integrity monitoring (FIM)
  - Network observability
  - Capabilities monitoring
  - Privileges monitoring
  - Execution monitoring
  - OS integrity enforcement
  - Kernel-level policy enforcement (no TOCTOU vulnerabilities)
  - Minimal overhead via eBPF kernel-space filtering
- **Adopters**: Palantir, GitHub, G-Research, Ripple, Nationwide, Bell
- **Sources**:
  - https://tetragon.io/
  - https://github.com/cilium/tetragon

### 1.3 Pixie (by New Relic)
- **Status**: CNCF Sandbox project
- **Key Features**:
  - Auto-telemetry platform for K8s
  - eBPF-based data collection (no code instrumentation needed)
  - Captures full-body requests/responses
  - CPU, memory, network profiling
  - Script-based querying (PxL language)
  - Edge compute: data processed on-cluster, not sent to cloud
- **2025 Status**: Active development, integrated with New Relic platform
- **Sources**:
  - https://px.dev/
  - https://github.com/pixie-io/pixie

### 1.4 Falco (with eBPF driver)
- **Status**: CNCF Graduated project
- **Key Features**:
  - Runtime security and threat detection
  - Supports eBPF probe as kernel instrumentation (alternative to kernel module)
  - Syscall monitoring and anomaly detection
  - Kubernetes audit log integration
  - Plugin system for extending data sources
  - Falco Rules for defining security policies
- **2025-2026 Updates**:
  - Falco 0.38+ with improved eBPF driver performance
  - Enhanced Kubernetes metadata enrichment
  - CloudEvents output format
  - Integration with Falco Talon for automated response
- **Sources**:
  - https://falco.org/
  - https://github.com/falcosecurity/falco

### 1.5 Calico (eBPF dataplane)
- **Status**: CNCF project by Tigera
- **Key Features**:
  - eBPF dataplane option (alternative to iptables/kube-proxy)
  - High-performance networking and network policy
  - VXLAN and native BGP networking
  - Windows support
  - WireGuard encryption
  - eBPF mode: significant performance improvements, DSR (Direct Server Return)
- **2025-2026 Trends**:
  - Calico 3.29+ with enhanced eBPF features
  - eBPF dataplane becoming production-ready default
  - Service mesh integration (Istio ambient mesh compatibility)
  - Multi-cluster federation improvements
- **Sources**:
  - https://www.tigera.io/project-calico/
  - https://github.com/projectcalico/calico

---

## 2. WebAssembly on Kubernetes

### 2.1 SpinKube
- **Status**: CNCF Sandbox project (accepted 2024)
- **Key Features**:
  - Runs WebAssembly (Wasm) workloads on Kubernetes
  - Integrates with containerd via runwasi shim
  - Uses Spin operator for K8s
  - Defines SpinApp CRD for Wasm applications
  - Sub-millisecond cold starts
  - Tiny footprint (MBs vs GBs for containers)
  - Supports Rust, Go, JavaScript/TypeScript, Python
- **Architecture**: Spin runtime -> runwasi shim -> containerd -> kubelet
- **Sources**:
  - https://www.spinkube.dev/
  - https://github.com/spinkube

### 2.2 wasmCloud
- **Status**: CNCF Sandbox project
- **Key Features**:
  - Distributed application platform built on WebAssembly
  - Component model-based architecture
  - Capability providers for messaging, storage, HTTP, etc.
  - Runs on Kubernetes via wasmCloud operator
  - Portable across edge, cloud, and embedded
  - WASI (WebAssembly System Interface) compliance
  - Actor model for composable applications
- **2025-2026 Trends**:
  - wasmCloud 1.0+ stability
  - Wasm Component Model adoption
  - Multi-runtime support
  - Enterprise adoption for edge computing
- **Sources**:
  - https://wasmcloud.com/
  - https://github.com/wasmCloud/wasmCloud

### 2.3 runwasi
- **Status**: CNCF Sandbox / containerd sub-project
- **Key Features**:
  - Containerd shim for running Wasm workloads
  - Implements containerd's task API for Wasm runtimes
  - Supports multiple Wasm runtimes (Spin, Wasmtime, WasmEdge, WAMR)
  - Allows mixing containers and Wasm pods in same cluster
  - OCI image distribution for Wasm modules
- **Sources**:
  - https://github.com/containerd/runwasi

### 2.4 Additional Wasm Runtimes for K8s
- **WasmEdge**: CNCF project, optimized for edge/AI workloads
- **Wasmtime**: Bytecode Alliance project, reference WASI implementation
- **WAMR**: Lightweight runtime for embedded/IoT

---

## 3. EDGE COMPUTING

### 3.1 KubeEdge
- **Status**: CNCF Graduated project
- **Version**: v1.22 (November 2025), v1.21 (June 2025)
- **Key Features**:
  - Kubernetes-native edge computing framework
  - Cloud-edge coordination with bidirectional communication
  - Edge autonomy (offline operation capability)
  - Low resource footprint (~70MB memory)
  - Device management via DeviceModel/DeviceInstance CRDs
  - Native ARM support (ARMv7, ARMv8)
  - Autonomic Kube-API endpoint at edge
  - Support for AI/ML workloads at edge (Ianvs benchmarking)
- **Adopters**: ARM, Huawei, China Mobile, China Telecom, China Unicom, Orange, Inspur, KubeSphere
- **Sources**:
  - https://kubeedge.io/
  - https://github.com/kubeedge/kubeedge

### 3.2 K3s
- **Status**: CNCF Sandbox project by SUSE/Rancher
- **Key Features**:
  - Lightweight certified Kubernetes distribution
  - Single binary (<100MB)
  - SQLite as default datastore (etcd optional)
  - ARM64 and ARMv7 native support
  - Built-in Helm controller
  - Auto-deployment of manifests
  - Optimized for IoT, edge, CI/ARM
  - FIPS 140-2 compliant encryption
- **2025-2026 Trends**:
  - K3s 1.31+ aligned with Kubernetes releases
  - K3s for AI edge inference workloads
  - Windows worker node support improvements
  - RKE2 convergence for production use cases
- **Sources**:
  - https://k3s.io/
  - https://github.com/k3s-io/k3s

### 3.3 MicroK8s
- **Status**: Maintained by Canonical
- **Key Features**:
  - Minimal, conformant Kubernetes distribution
  - Snap-based packaging
  - Addon system (dns, ingress, helm3, istio, etc.)
  - Multi-node clustering
  - ARM64 native support
  - Strict confinement security
  - GPU addon for AI/ML edge workloads
- **Sources**:
  - https://microk8s.io/
  - https://github.com/canonical/microk8s

### 3.4 Akri
- **Status**: CNCF Sandbox project
- **Key Features**:
  - Kubernetes resource interface for leaf devices
  - Automatic discovery of IoT devices (ONVIF cameras, OPC UA, udev)
  - Broker pattern: each discovered device gets a workload pod
  - Custom discovery handler protocol
  - Works with K3s, MicroK8s, KubeEdge
  - Edge-native device management
- **Sources**:
  - https://akri.sh/
  - https://github.com/project-akri/akri

---

## 4. SERVERLESS / FaaS ON KUBERNETES

### 4.1 Knative
- **Status**: CNCF Graduated project
- **Key Features**:
  - Serving: request-driven autoscaling (scale-to-zero)
  - Eventing: CloudEvents-based event mesh
  - Build/Pipeline (deprecated, use Tekton)
  - Traffic splitting for canary/blue-green deployments
  - Revisions for version management
  - Integration with Istio, Contour, Kourier networking layers
  - Knative Functions for developer experience
- **2025-2026 Trends**:
  - Knative 1.17+ (2025)
  - Gateway API integration
  - Improved cold-start performance
  - Multi-tenant isolation
  - CloudEvents v1.0 compliance
  - Shift toward Kubernetes-native APIs
- **Sources**:
  - https://knative.dev/
  - https://github.com/knative

### 4.2 OpenFunction
- **Status**: CNCF Sandbox project
- **Key Features**:
  - Cloud-native FaaS platform
  - Sync and async functions
  - Multiple runtime support (Node.js, Python, Go, .NET, Java, Rust, Wasm)
  - Built-in event-driven architecture (Dapr integration)
  - Shipwright-based build system
  - KEDA for autoscaling
  - HTTP and event triggers
- **2025-2026 Trends**:
  - WebAssembly function runtime
  - GPU function support for AI inference
  - Enhanced Dapr bindings
- **Sources**:
  - https://openfunction.dev/
  - https://github.com/OpenFunction/OpenFunction

### 4.3 Dapr
- **Status**: CNCF Graduated project
- **Key Features**:
  - Distributed Application Runtime
  - Building block APIs:
    - Service-to-service invocation
    - State management
    - Pub/sub messaging
    - Bindings (input/output)
    - Actors (virtual actor pattern)
    - Observability
    - Secrets management
    - Configuration
    - Distributed lock
    - Workflows (durable orchestration)
  - Sidecar architecture
  - Works on Kubernetes, VMs, and standalone
  - Multi-language SDKs (Go, .NET, Java, Python, JavaScript, C++, Rust, PHP)
- **2025-2026 Trends**:
  - Dapr 1.15+ with workflow improvements
  - Agentic AI workload support
  - Enhanced durable workflow orchestration
  - gRPC-based component protocol
  - Improved placement service for actor scaling
  - Dapr on non-Kubernetes platforms
- **Sources**:
  - https://dapr.io/
  - https://github.com/dapr/dapr

---

## 5. IoT WORKLOADS ON KUBERNETES

### Key Technologies
- **KubeEdge**: Device management via CRDs, MQTT broker integration
- **Akri**: Automatic IoT device discovery and workload scheduling
- **K3s**: Lightweight enough for IoT gateways (ARM, resource-constrained)
- **EdgeX Foundry + K8s**: Industrial IoT platform integration
- **MQTT Brokers**: Eclipse Mosquitto, EMQX on Kubernetes

### IoT-Specific Considerations
- Device twin models (KubeEdge)
- Protocol translation (HTTP/MQTT/OPC-UA/Modbus)
- Offline operation and data sync
- Resource constraints (< 1GB RAM)
- ARM architecture requirements
- OTA updates for edge nodes

---

## 6. ARM64 SUPPORT

### Status Across Projects (2025-2026)
| Project | ARM64 | ARMv7 | Notes |
|---------|-------|-------|-------|
| Kubernetes | GA | N/A | Full ARM64 support since 1.17 |
| Cilium | GA | N/A | Full ARM64 images |
| K3s | GA | GA | First-class ARM support |
| MicroK8s | GA | N/A | Snap-based ARM64 support |
| KubeEdge | GA | GA | Native x86, ARMv7, ARMv8 |
| Tetragon | GA | N/A | ARM64 images available |
| Knative | GA | N/A | Multi-arch images default |
| Dapr | GA | N/A | ARM64 sidecar images |
| Falco | GA | N/A | eBPF driver for ARM64 |

### ARM64 Trends
- All major K8s distributions ship multi-arch (amd64+arm64) images
- Graviton (AWS), Ampere (Azure), Axion (GCP) driving cloud ARM adoption
- Apple Silicon (M-series) development environments
- NVIDIA Jetson for edge AI workloads
- RISC-V emerging for ultra-low-power edge

---

## 7. WINDOWS CONTAINERS

### Status (2025-2026)
- **Kubernetes**: Full Windows Server container support
- **Calico**: Windows network policy and CNI support
- **Cilium**: Windows support via Hubble observability (limited CNI)
- **K3s**: Windows worker node support
- **Containerd**: Windows container runtime support
- **HCS/HCS v2**: Host Compute Service for Windows containers

### Windows-Specific Considerations
- Windows Server 2022 and 2025 container support
- Process isolation vs Hyper-V isolation
- Windows-specific network policies
- Mixed Linux/Windows cluster management
- Azure/AWS managed K8s have best Windows container support
- GMSA (Group Managed Service Accounts) for Active Directory

---

## SUMMARY OF KEY 2025-2026 TRENDS

1. **eBPF Maturity**: eBPF is the de facto standard for K8s networking and security. Cilium dominates networking, Tetragon leads runtime security. Falco eBPF and Calico eBPF provide alternatives.

2. **WebAssembly on K8s**: Still emerging but accelerating. SpinKube and wasmCloud gained CNCF recognition. runwasi enables mixing Wasm and container workloads. Sub-millisecond cold starts and tiny footprints ideal for edge/serverless.

3. **Edge Computing**: KubeEdge leads for complex edge scenarios. K3s dominates lightweight/edge K8s. MicroK8s strong for Canonical ecosystem. Akri fills IoT device discovery niche.

4. **Serverless/FaaS**: Knative remains dominant but complex. Dapr's building-block API approach gaining traction. OpenFunction offers cloud-native FaaS with Wasm support.

5. **IoT**: KubeEdge + Akri provide comprehensive IoT K8s platform. MQTT integration is standard. Offline capability is critical differentiator.

6. **ARM64**: Full parity with x86 across ecosystem. Cloud ARM adoption driving investment. Edge AI (NVIDIA Jetson) creates new ARM64 use cases.

7. **Windows Containers**: Stable but niche. Best in cloud-managed K8s. Mixed Linux/Windows clusters require careful networking and scheduling.
