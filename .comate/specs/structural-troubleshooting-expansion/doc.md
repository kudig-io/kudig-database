# Structural Troubleshooting Knowledge Base Expansion

## Feature Name
`structural-troubleshooting-expansion`

## Requirement Analysis

### Background
The `topic-structural-trouble-shooting` directory is a Kubernetes production operations knowledge base containing 46 Markdown documents across 12 categories. While categories 01-08 (control-plane, node-components, networking, storage, workloads, security-auth, resources-scheduling, cluster-operations) are well-covered, categories 09-12 (cloud-provider, ai-ml-workloads, gitops-devops, monitoring-observability) and 04-storage are significantly under-represented with only 1-2 documents each. The README.md does not reflect these newer categories, creating a documentation gap.

### Expansion Scope
This task focuses on **content expansion** for under-represented directories by creating high-quality troubleshooting documents following the project's established "Four-Element Methodology" (四要素法):
1. **Problem Phenomenon** - Clear description of symptoms, reproduction paths, and business impact
2. **Error Messages & Key Information** - Specific logs, stack traces, console output, or system metrics
3. **Troubleshooting Approach** - Logical, step-by-step diagnostic procedures
4. **Solution & Recommendations** - Actionable code fixes, configuration adjustments, or preventive best practices

### Target Directories & New Documents

#### 04-storage (Current: 2 docs → Target: 4 docs)
| New Document | Focus Area | Description |
|-------------|-----------|-------------|
| `03-snapshot-backup-troubleshooting.md` | CSI Snapshot & Volume Backup | CSI VolumeSnapshot, snapshot controller, backup/restore failures, data consistency |
| `04-storage-performance-troubleshooting.md` | Storage I/O Performance | High latency I/O, throughput bottlenecks, fio benchmarking, storage class tuning |

#### 09-cloud-provider (Current: 1 doc → Target: 3 docs)
| New Document | Focus Area | Description |
|-------------|-----------|-------------|
| `02-multi-cloud-networking-troubleshooting.md` | Multi-Cloud / Hybrid Network | Cross-cloud VPC peering, VPN gateways, inter-cluster service mesh, DNS federation |
| `03-cloud-resource-quota-troubleshooting.md` | Cloud API Quota & Throttling | Cloud API rate limiting, resource quota exhaustion, instance limits, cost controls |

#### 10-ai-ml-workloads (Current: 1 doc → Target: 3 docs)
| New Document | Focus Area | Description |
|-------------|-----------|-------------|
| `02-kubeflow-troubleshooting.md` | Kubeflow Platform | Kubeflow Pipelines, Katib, KServe, Notebook servers, ML workflow orchestration |
| `03-mpi-operator-troubleshooting.md` | MPI / Distributed Training Jobs | MPI Operator, Horovod, all-reduce communication, launcher-worker pattern |

#### 11-gitops-devops (Current: 1 doc → Target: 3 docs)
| New Document | Focus Area | Description |
|-------------|-----------|-------------|
| `02-tekton-troubleshooting.md` | Tekton CI/CD Pipelines | PipelineRun failures, TaskRun hangs, workspace issues, pipeline resource limits |
| `03-flux-image-automation-troubleshooting.md` | Flux Image Automation | ImageUpdateAutomation, ImagePolicy, image scanning, automated Git commits |

#### 12-monitoring-observability (Current: 1 doc → Target: 4 docs)
| New Document | Focus Area | Description |
|-------------|-----------|-------------|
| `02-opentelemetry-troubleshooting.md` | OpenTelemetry Collector | OTel collector pipelines, exporter failures, sampling, instrumentation gaps |
| `03-ebpf-observability-troubleshooting.md` | eBPF-based Observability | Cilium Hubble, Pixie, Tetragon, eBPF program loading, kernel compatibility |
| `04-finops-cost-optimization-troubleshooting.md` | FinOps & Cost Monitoring | Kubecost/OpenCost, resource right-sizing, idle resource detection, chargeback |

#### README.md Update
- Update document statistics from 40 → 51
- Add 09-12 categories to the directory structure table
- Add new documents to the quick lookup guides (by symptom and by component)
- Update the changelog section

## Architecture & Technical Approach

### Document Structure Template
Each new document must follow the established project template:

```
# {Title}
> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: {初级/中级/高级}

## 0. 10 分钟快速诊断
[Numbered quick diagnostic checklist]

## 1. 问题现象与影响分析
### 1.1 常见问题现象
[Tables: 现象 | 报错信息 | 报错来源 | 查看方式]
### 1.2 影响面分析
[Direct/Indirect impact assessment]

## 2. 排查方法与步骤
### 2.1 诊断原理说明
[Technical explanation of the subsystem]
### 2.2 排查逻辑决策树
[ASCII decision tree diagram]
### 2.3 详细诊断命令
[Shell scripts with commentary]

## 3. 解决方案与风险控制
### 3.1 解决方案
[YAML configurations, code fixes, step-by-step commands]
### 3.2 风险控制与回滚
[Risk assessment table: 操作 | 风险等级 | 影响评估 | 回滚方案]
### 3.3 验证与监控
[Verification scripts, monitoring rules]
### 3.4 最佳实践
[Preventive measures and production recommendations]
```

### Style Guidelines
- Use Simplified Chinese as the primary language (consistent with existing docs)
- Include shell scripts with `#!/bin/bash` shebang where applicable
- Include YAML manifests for Kubernetes resources
- Include Prometheus alerting rules where relevant
- Use severity ratings: ⭐⭐⭐ 高 / ⭐⭐ 中 / ⭐ 低
- Include emergency levels: P0 / P1 / P2
- All commands must be production-safe or clearly marked as requiring caution

## Affected Files

### New Files (11 documents)
1. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md`
2. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md`
3. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/09-cloud-provider/02-multi-cloud-networking-troubleshooting.md`
4. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/09-cloud-provider/03-cloud-resource-quota-troubleshooting.md`
5. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md`
6. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md`
7. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/11-gitops-devops/02-tekton-troubleshooting.md`
8. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting.md`
9. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting.md`
10. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/12-monitoring-observability/03-ebpf-observability-troubleshooting.md`
11. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md`

### Modified Files
12. `/Users/allengaller/Documents/GitHub/kudig-io/kudig-database/topic-structural-trouble-shooting/README.md` - Update statistics, add new categories to directory structure, add quick lookup entries, update changelog

## Implementation Details

### Storage Documents

#### 03-snapshot-backup-troubleshooting.md
- **Core topics**: VolumeSnapshotClass, CSI snapshot controller, snapshot creation hangs, restore from snapshot failures, snapshot consistency
- **Key commands**: `kubectl get volumesnapshots`, `kubectl get volumesnapshotcontents`, checking snapshot sidecar logs
- **Solutions**: Correct VolumeSnapshotClass configuration, snapshot controller deployment verification, pre/post snapshot hooks

#### 04-storage-performance-troubleshooting.md
- **Core topics**: I/O latency analysis, storage class performance tiers, fio benchmarking, filesystem fragmentation, cache tuning
- **Key commands**: `fio` benchmarks, `iostat`, `iotop`, PVC latency metrics
- **Solutions**: StorageClass parameter tuning, local SSD vs network storage selection, read-ahead/cache configuration

### Cloud Provider Documents

#### 02-multi-cloud-networking-troubleshooting.md
- **Core topics**: Cross-cluster networking, cluster mesh, service federation, VPN/Peering connectivity, multi-cloud DNS
- **Key commands**: Cross-cluster ping tests, Submariner/Linkerd multicluster diagnostics
- **Solutions**: VPC peering configuration, Submariner broker setup, global load balancer configuration

#### 03-cloud-resource-quota-troubleshooting.md
- **Core topics**: Cloud API rate limits, instance quota exhaustion, vCPU/network limits, cost management alerts
- **Key commands**: Cloud CLI quota queries, CCM rate limit metrics
- **Solutions**: Quota increase requests, resource pooling, multi-region distribution, spot instance usage

### AI/ML Documents

#### 02-kubeflow-troubleshooting.md
- **Core topics**: Kubeflow Pipelines execution, Katib experiment failures, KServe model serving, Notebook server startup
- **Key commands**: Kubeflow component logs, pipeline run diagnostics
- **Solutions**: Kubeflow manifest fixes, pipeline component resource limits, Istio integration fixes

#### 03-mpi-operator-troubleshooting.md
- **Core topics**: MPI launcher-worker pattern, Horovod ring initialization, hostfile generation, SSH-less communication
- **Key commands**: MPI job status, worker pod logs, `mpirun` diagnostics
- **Solutions**: MPI Operator version compatibility, hostfile ConfigMap, network policy for MPI communication

### GitOps/DevOps Documents

#### 02-tekton-troubleshooting.md
- **Core topics**: PipelineRun/TaskRun failures, workspace PVC issues, pipeline resource limits, task timeout, pipeline cache
- **Key commands**: `tkn` CLI diagnostics, TaskRun pod logs, pipeline resource inspection
- **Solutions**: Pipeline timeout tuning, workspace volume configuration, pipeline resource quotas

#### 03-flux-image-automation-troubleshooting.md
- **Core topics**: ImageUpdateAutomation, ImagePolicy semver parsing, automated Git commit/push, image repository scanning
- **Key commands**: `flux get images`, image policy status, automation run logs
- **Solutions**: Image policy regex fixes, Git write permissions, image repository authentication

### Monitoring/Observability Documents

#### 02-opentelemetry-troubleshooting.md
- **Core topics**: OTel collector pipeline configuration, exporter backpressure, tail-based sampling, missing spans, instrumentation library compatibility
- **Key commands**: OTel collector metrics, pprof profiling, OTLP endpoint testing
- **Solutions**: Collector pipeline batch tuning, exporter retry configuration, sampling policy adjustment

#### 03-ebpf-observability-troubleshooting.md
- **Core topics**: eBPF program loading failures, kernel version compatibility, Cilium Hubble visibility, Tetragon tracing, BTF requirements
- **Key commands**: `bpftool`, kernel config verification, eBPF map inspection
- **Solutions**: Kernel upgrade/downgrade guidance, BTF compilation, eBPF resource limit tuning

#### 04-finops-cost-optimization-troubleshooting.md
- **Core topics**: Kubecost/OpenCost deployment, resource right-sizing recommendations, idle resource detection, namespace chargeback, spot instance optimization
- **Key commands**: Cost allocation queries, resource utilization analysis, savings recommendations
- **Solutions**: Resource request/limit optimization, autoscaling policies, spot instance migration strategies

## Boundary Conditions
- All documents must target Kubernetes v1.25-v1.32 compatibility
- Commands must not assume specific cloud provider unless explicitly scoped
- Solutions must include rollback procedures for production safety
- Documents must not duplicate existing content in 01-08 categories
- Each document should be 400-1000 lines to match existing quality standards

## Expected Outcomes
- 11 new high-quality troubleshooting documents
- Updated README.md with accurate statistics (51 total documents) and complete category coverage
- All documents following the project's "Four-Element Methodology"
- Consistent formatting, severity ratings, and production safety warnings
- Enhanced knowledge base coverage for cloud-native, AI/ML, GitOps, and observability domains
