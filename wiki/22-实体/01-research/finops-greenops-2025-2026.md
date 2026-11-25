---
title: Finops Greenops 2025 2026
summary: The FinOps Framework has evolved significantly from the original "Crawl/Walk/Run"
  maturity model.
category: entities
tags:
- finops-greenops-2025-2026
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes FinOps, Cost Governance & GreenOps Research 2025-2026

## 1. FINOPS MATURITY MODEL EVOLUTION

### FinOps Foundation Framework (2025)
The FinOps Framework has evolved significantly from the original "Crawl/Walk/Run" maturity model.

**Current Maturity Phases (2025):**
- **Crawl**: Basic cost visibility, allocating shared costs, rightsizing recommendations
- **Walk**: Automated showback/chargeback, anomaly detection, commitment discount management
- **Run**: Real-time cost optimization, unit economics, FinOps as culture, forecasting accuracy

**Key Evolution (2024-2026):**
- Framework expanded from 3 Domains to include: Understand Cloud Usage & Cost, Quantify Business Value, Optimize Cloud Usage & Cost, Manage the FinOps Practice
- Added "Maturity Activities" per capability rather than blanket Crawl/Walk/Run
- Integration with FOCUS specification for standardized cost data
- FinOps Certified Practitioner (FOCP) and FOCUS Certified Practitioner certifications expanding

**Source**: https://finops.org/framework/
**Source**: https://finops.org/what-is-finops/

---

## 2. FOCUS SPECIFICATION 1.0 (FinOps Open Cost & Usage Specification)

### Overview
FOCUS is a community-driven specification for cloud cost and usage data, developed by the FinOps Foundation.

**FOCUS 1.0 Key Details:**
- **GA Release**: November 2023, with ongoing updates in 2024-2025
- **Purpose**: Standardize billing data across cloud providers (AWS, Azure, GCP, OCI)
- **Key Columns**: ChargePeriodStart, ChargePeriodEnd, BilledCost, EffectiveCost, ListCost, UsageQuantity
- **Provider Support**: AWS CUR 2.0, Azure Cost Management exports, GCP billing exports

**FOCUS 1.1+ Updates (2025):**
- Added support for SaaS cost data sources
- Improved SKU Price ID standardization
- Enhanced commitment discount handling
- Coverage columns for amortized vs. billed costs
- Support for on-premises and Kubernetes cost data

**Why It Matters for Kubernetes:**
- Enables unified cost reporting across K8s + cloud infra
- Standardizes how Kubernetes costs appear alongside IaaS costs
- Kubecost, OpenCost, and other K8s cost tools now support FOCUS exports

**Source**: https://focus.finops.org/
**Source**: https://finops.org/focus/

---

## 3. KUBERNETES COST ALLOCATION STRATEGIES

### Namespace-Based Allocation
The most common and recommended strategy for K8s cost allocation.

**Best Practices:**
- Map namespaces to teams, projects, or environments (prod, staging, dev)
- Use Kubernetes labels: `app.kubernetes.io/part-of`, `cost-center`, `team`
- Deploy Kubecost or OpenCost for real-time namespace-level cost breakdown
- Implement shared cost distribution models (proportional, fixed, equal split)

**Key Metrics to Track:**
- CPU/Memory request vs. actual usage (efficiency ratio)
- Idle cost (requested but unused resources)
- Shared namespace overhead allocation

### Label-Based Allocation
More granular than namespace-based.

**Recommended Labels:**
- `kubecost/team` - Team ownership
- `kubecost/product` - Product/service
- `kubecost/environment` - prod/staging/dev
- `cost-center` - Financial cost center mapping

### Pod-Level Allocation
Finest granularity using resource requests/limits:
- CPU cost = (pod CPU request / node CPU allocatable) * node cost
- Memory cost = (pod memory request / node memory allocatable) * node cost
- GPU cost = direct assignment or time-sharing model

**Source**: https://www.kubecost.com/kubernetes-cost-allocation/
**Source**: https://www.apptio.com/products/kubecost/

---

## 4. RESOURCE QUOTA BEST PRACTICES

### ResourceQuota Patterns (2025)

**Namespace-Level Quotas:**
```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: team-a
spec:
  hard:
    requests.cpu: "16"
    requests.memory: 32Gi
    limits.cpu: "32"
    limits.memory: 64Gi
    persistentvolumeclaims: "10"
    pods: "50"
    nvidia.com/gpu: "4"
```

**Best Practices:**
1. **Always set both requests and limits quotas** - prevents unbounded cost
2. **Use LimitRange for defaults** - ensures every pod has resource constraints
3. **Implement GPU quotas separately** - GPU costs dominate AI workloads
4. **Monitor quota utilization** - alert at 80% to prevent scheduling failures
5. **Use ResourceQuota with LimitRange together** - defense in depth

### LimitRange Patterns

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: team-a
spec:
  limits:
  - default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "4"
      memory: "8Gi"
    min:
      cpu: "50m"
      memory: "64Mi"
    type: Container
  - max:
      cpu: "16"
      memory: "32Gi"
    type: Pod
```

**LimitRange Best Practices:**
- Set sensible defaults per namespace (prevents "no request" pods)
- Set max limits to cap runaway resource consumption
- Use per-pod limits for batch/job namespaces
- Different LimitRange profiles for different workload types

**Source**: https://kubernetes.io/docs/concepts/policy/limit-range/
**Source**: https://kubernetes.io/docs/concepts/policy/resource-quotas/

---

## 5. NAMESPACE COST ALLOCATION

### Strategies for Shared Cost Distribution

**1. Proportional Allocation:**
- Distribute shared cluster costs (networking, monitoring, storage) proportional to namespace resource usage
- Formula: namespace_share = (namespace_cpu + namespace_mem) / (cluster_total_cpu + cluster_total_mem)

**2. Fixed Allocation:**
- Pre-agreed percentages per team/project
- Simple but doesn't reflect actual usage changes

**3. Hybrid Model:**
- Fixed base allocation + proportional variable allocation
- Most commonly adopted by mature FinOps teams

### OpenCost Integration
OpenCost (CNCF project) provides open-source K8s cost allocation:
- Real-time cost per namespace, controller, pod, label
- Prometheus metrics export
- FOCUS-compatible output

**Source**: https://www.opencost.io/

---

## 6. GREENOPS & SUSTAINABLE COMPUTING

### kube-green
**What it is**: A Kubernetes operator that automatically scales down resources during off-hours.

**Key Features:**
- CRD-based configuration (`SleepInfo`)
- Scales Deployments and StatefulSets to 0 during configured sleep periods
- Supports CronJob suspension
- IANA timezone support
- Exclude specific resources
- CNCF Landscape listed project

**Example SleepInfo CRD:**
```yaml
apiVersion: kube-green.com/v1alpha1
kind: SleepInfo
metadata:
  name: working-hours
spec:
  weekdays: "1-5"
  sleepAt: "20:00"
  wakeUpAt: "08:00"
  timeZone: "Europe/Rome"
  suspendCronJobs: true
  excludeRef:
  - apiVersion: "apps/v1"
    kind: Deployment
    name: api-gateway
```

**Impact:**
- Can reduce dev/staging cluster costs by 60-70%
- Direct CO2 reduction from reduced compute
- Typical implementation: sleep non-production namespaces at night/weekends

**Source**: https://github.com/kube-green/kube-green
**Source**: https://kube-green.dev

### Carbon-Aware Scheduling

**Concept**: Schedule workloads when/where carbon intensity is lowest.

**Key Approaches:**
1. **Carbon-Aware KEDA Scaler**: Scale workloads based on grid carbon intensity
2. **Emissions-aware node selection**: Prefer regions/zones with lower carbon intensity
3. **Time-shifting batch jobs**: Run ML training during low-carbon periods
4. **Boavizta / WattTime APIs**: Real-time carbon intensity data for scheduling decisions

**Carbon-Aware Kubernetes Tools (2025):**
- **KEDA + Carbon Aware Scaler**: Scale based on carbon intensity metrics
- **Green Software Foundation's Impact Framework**: Measure software carbon footprint
- **Scaphandre**: Power consumption monitoring agent for K8s nodes
- **Kepler (Kubernetes Efficient Power Level Exporter)**: CNCF sandbox project for K8s energy monitoring

**Source**: https://github.com/Green-Software-Foundation/if
**Source**: https://www.sustainable-computing.io/ (Kepler)

---

## 7. GPU COST OPTIMIZATION FOR AI WORKLOADS

### The GPU Cost Problem
GPU costs for AI/ML workloads can be 10-100x CPU costs. A single NVIDIA A100 instance can cost $3-4/hr.

### Optimization Strategies (2025)

**1. GPU Time-Slicing (MIG - Multi-Instance GPU):**
- NVIDIA MIG partitions a single GPU into up to 7 instances
- Better GPU utilization for smaller workloads
- Kubernetes device plugin support for MIG

**2. GPU Sharing with NVIDIA Time-Slicing:**
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-time-slicing-config
data:
  any: |-
    version: v1
    flags:
      migStrategy: none
    sharing:
      timeSlicing:
        renameByDefault: false
        resources:
        - name: nvidia.com/gpu
          replicas: 4
```

**3. Spot/Preemptible GPU Instances:**
- 60-90% discount on GPU spot instances
- Requires checkpointing and graceful termination handling
- Use with node affinity rules for fallback to on-demand

**4. Right-Sizing GPU Requests:**
- Monitor actual GPU utilization vs. requested
- Use `nvidia-smi` exporters and DCGM metrics
- Common waste: requesting A100 for inference that runs fine on T4

**5. GPU Quotas and Governance:**
- Separate ResourceQuota for `nvidia.com/gpu` per namespace
- Chargeback at premium rate for GPU usage
- Auto-terminate idle GPU pods (GPU utilization < 5% for 30 min)

**6. Inference Optimization:**
- Use vLLM, TensorRT, or Triton for efficient GPU inference
- Batch inference requests to maximize GPU throughput
- Consider CPU inference for smaller models (Phi, Gemma, etc.)

---

## 8. SPOT/PREEMPTIBLE INSTANCE STRATEGIES

### Kubernetes Spot Instance Best Practices

**1. Node Pool Design:**
- Dedicated spot node pool with taints/tolerations
- Mixed node pool (spot + on-demand) with priority-based scheduling
- Separate pools per instance type/family for diversity

**2. Pod Disruption Budget (PDB):**
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  maxUnavailable: 1
  selector:
    matchLabels:
      app: my-app
```

**3. Graceful Termination:**
- Set `terminationGracePeriodSeconds` appropriately (30-120s)
- Implement preemption handlers in application code
- Use K8s SIGTERM/SIGKILL signals for cleanup

**4. Karpenter (AWS) / Cluster Autoscaler Spot Support:**
- Karpenter: automatic spot instance diversification
- Falls back to on-demand when spot unavailable
- Consolidation: replace underutilized nodes with smaller/cheaper ones

**5. Workload Suitability:**
- **Good for spot**: stateless web servers, batch processing, CI/CD, dev/staging
- **Poor for spot**: stateful databases, critical production services, long-running stateful jobs
- **AI/ML training**: excellent spot candidate if checkpointing is implemented

**6. Cost Savings Typical:**
- 60-90% savings on compute costs
- 30-50% of total cluster cost reducible to spot
- Best practice: 70% spot / 30% on-demand for resilient workloads

---

## 9. KEY TOOLS & PLATFORMS (2025-2026)

| Tool | Type | Key Capability |
|------|------|----------------|
| Kubecost (IBM/Apptio) | Commercial/OSS | Real-time K8s cost allocation, FOCUS export |
| OpenCost | CNCF OSS | Open-source K8s cost monitoring |
| kube-green | CNCF OSS | Sleep/wake non-production workloads |
| Kepler | CNCF Sandbox | K8s energy/power monitoring |
| Scaphandre | OSS | Power consumption monitoring |
| Karpenter | CNCF OSS | Just-in-time node provisioning (spot-aware) |
| FOCUS Specification | Standard | Multi-cloud billing standardization |
| KEDA | CNCF | Event-driven autoscaling (carbon-aware) |
| DCGM | NVIDIA OSS | GPU utilization monitoring |
| Goldilocks (Fairwinds) | OSS | VPA-based resource request recommendations |

---

## 10. SOURCE URLS

1. FinOps Foundation Framework: https://finops.org/framework/
2. FOCUS Specification: https://focus.finops.org/
3. FOCUS on FinOps.org: https://finops.org/focus/
4. Kubecost (IBM/Apptio): https://www.apptio.com/products/kubecost/
5. OpenCost: https://www.opencost.io/
6. kube-green GitHub: https://github.com/kube-green/kube-green
7. kube-green Docs: https://kube-green.dev
8. Kepler (Sustainable K8s): https://www.sustainable-computing.io/
9. Green Software Foundation: https://greensoftware.foundation/
10. Scaphandre: https://github.com/hubblo-org/scaphandre
11. NVIDIA MIG: https://docs.nvidia.com/datacenter/tesla/mig-user-guide/
12. Karpenter: https://karpenter.sh/
13. Goldilocks: https://github.com/FairwindsOps/goldilocks
14. K8s Resource Quotas: https://kubernetes.io/docs/concepts/policy/resource-quotas/
15. K8s LimitRange: https://kubernetes.io/docs/concepts/policy/limit-range/


<!-- risk-assessed -->
