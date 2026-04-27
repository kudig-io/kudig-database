# topic-skills 生产级增强规格文档

## 1. 需求概述

从 Kubernetes 运维专家（SRE）视角，全面审查并增强 `topic-skills` 目录下的所有内容，确保其达到生产环境高质量标准。

**目标版本范围**: Kubernetes v1.28.x - v1.32.x（以 v1.28/v1.29 稳定版为基线，v1.30+ 新特性标注）

**核心改进维度**:
1. **版本明确化**: 所有命令/API 标注版本兼容性，移除废弃用法
2. **生产规范**: YAML 示例包含资源限制、健康检查、安全上下文
3. **内容深度**: 补充最佳实践、故障排查逻辑、性能优化、安全加固
4. **脚本质量**: 修复可移植性问题、消除安全风险、增强错误处理

---

## 2. 目录结构与文件清单

### 2.1 根目录主题技能（19 个 Markdown 文件）

| 文件 | Skill ID | 类别 | 主要增强方向 |
|------|----------|------|-------------|
| `01-node-notready.md` | SKILL-NODE-001 | Node | 版本差异标注、驱逐策略最佳实践、SSH 诊断安全加固 |
| `02-pod-crashloop-oomkilled.md` | SKILL-POD-001 | Pod | Ephemeral Containers 版本标注、内存诊断工具链、sidecar 容器 |
| `03-pod-pending.md` | SKILL-POD-002 | Pod | SchedulingGates (v1.27 GA)、PodDisruptionConditions、GPU 调度 |
| `04-dns-resolution-failure.md` | SKILL-NET-001 | Network | CoreDNS 版本差异、NodeLocal DNSCache 部署、ndots 优化 |
| `05-service-connectivity.md` | SKILL-NET-002 | Network | nftables kube-proxy (v1.32 GA)、EndpointSlice 诊断、NetworkPolicy 调试 |
| `06-certificate-expiry.md` | SKILL-SEC-001 | Security | cert-manager API 版本、证书自动轮转、 trusts 链验证 |
| `07-pvc-storage-failure.md` | SKILL-STORE-001 | Storage | VolumeAttributesClass (v1.31 beta)、PVC 快照恢复、多挂载冲突 |
| `08-deployment-rollout-failure.md` | SKILL-WORK-001 | Workload | maxUnavailable/maxSurge 计算、PodDisruptionBudget 互锁、Recreate 策略风险 |
| `09-rbac-quota-failure.md` | SKILL-SEC-002 | Security | RBAC 最小权限原则、LimitRange 默认注入、ValidatingAdmissionPolicy |
| `10-image-pull-failure.md` | SKILL-IMAGE-001 | Image | ImagePullSecrets 自动挂载、registry mirror、多架构镜像 |
| `11-control-plane-failure.md` | SKILL-CP-001 | ControlPlane | etcd 数据备份策略、APF 优先级配置、API Server 健康检查端点 |
| `12-autoscaling-failure.md` | SKILL-SCALE-001 | Scaling | HPA v2 行为差异、KEDA 版本兼容性、Cluster Autoscaler 调优 |
| `13-ingress-gateway-failure.md` | SKILL-NET-003 | Network | Gateway API v1beta1→v1、IngressClass 默认设置、gRPC 路由 |
| `14-configmap-secret-failure.md` | SKILL-CONFIG-001 | Configuration | immutable ConfigMap/Secret、External Secrets Operator、KMS 解密错误 |
| `15-monitoring-alerting-failure.md` | SKILL-MONITOR-001 | Observability | Prometheus v2.x→v3.x 差异、Thanos 架构、Alertmanager 路由树 |
| `16-logging-pipeline-failure.md` | SKILL-LOG-001 | Observability | Vector/Fluent Bit 性能调优、Loki 标签基数控制、审计日志 |
| `17-performance-bottleneck.md` | SKILL-PERF-001 | Performance | CPU throttling 量化、内存碎片分析、网络带宽排查、 eBPF 工具链 |
| `18-security-incident-response.md` | SKILL-SECURITY-001 | Security | 容器逃逸检测、供应链安全、Falco/Trivy 集成、取证流程 |
| `19-skill-local-demo-guide.md` | DEMO-GUIDE-001 | Demo | Kind 版本兼容性、m1/m2 芯片支持、资源要求说明 |

### 2.2 skill-set 模板目录

```
skill-set/k8s-node-notready/
├── SKILL.md                          → 版本矩阵增强、emoji→text 标准化
├── USAGE-GUIDE.md                    → 修复文件引用错误、添加异常处理伪代码
├── assets/
│   ├── skill-metadata.yaml           → k8s_versions 增加 patch 粒度、RBAC 最小权限细化
│   ├── root-cause-map.yaml           → 版本感知诊断规则、消除 remediation_map 重复
│   ├── symptom-patterns.yaml         → 版本排除规则、正则校验
│   └── escalation-template.md        → 告警模板标准化、版本信息注入
├── reference/
│   ├── diagnostic-workflow.md        → 每步添加版本兼容性说明
│   ├── remediation-playbook.md       → 风险等级量化、回滚步骤
│   ├── root-cause-catalog.md         → 版本行为差异对照
│   └── version-matrix.md             → v1.28-v1.32 完整特性矩阵
└── scripts/
    ├── diagnose-quick.sh             → 增加 kubectl 版本检查、错误处理
    ├── diagnose-deep.sh              → SSH 安全检查、sudo 权限验证
    ├── check-resources.sh            → 阈值可配置、输出结构化
    ├── cleanup-disk.sh               → 增加确认提示、白名单保护
    └── verify-node.sh                → 多条件验证、超时机制
```

### 2.3 skills-run 演示目录

```
skills-run/
├── README.md                         → 修复路径错误、增加故障排查章节
├── setup-kind-cluster.sh             → mktemp 可移植性修复、trap 清理、资源检查
├── run-skill-demo.sh                 → glob 处理加固、权限检查
├── teardown.sh                       → 删除确认提示、集群信息展示
└── scenarios/*.sh (10个)             → eval 安全替换、单节点集群兼容、版本检查
```

---

## 3. 通用增强规则

### 3.1 Front Matter 标准化（适用于所有 .md 文件）

**修改前**:
```yaml
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
```

**修改后**:
```yaml
k8s_versions:
  - "1.28.x"
  - "1.29.x"
  - "1.30.x"
  - "1.31.x"
  - "1.32.x"
tested_on:
  - "1.28.15"
  - "1.29.12"
  - "1.30.8"
  - "1.31.4"
  - "1.32.0"
k8s_version_notes:
  - "v1.28+: Ephemeral Containers GA, Native Sidecar Containers (beta)"
  - "v1.29+: PodDisruptionConditions GA, Node graceful shutdown GA"
  - "v1.30+: Node swap support (beta), Topology Manager (GA)"
  - "v1.31+: VolumeAttributesClass (beta), BoundServiceAccountTokenVolume (GA)"
  - "v1.32+: nftables kube-proxy mode (GA), Sidecar Containers (GA)"
```

### 3.2 YAML 示例生产规范化

**所有 YAML 示例必须包含**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: example
spec:
  replicas: 3
  selector:
    matchLabels:
      app: example
  template:
    metadata:
      labels:
        app: example
    spec:
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      containers:
        - name: app
          image: nginx:1.27-alpine
          imagePullPolicy: IfNotPresent
          ports:
            - containerPort: 80
              protocol: TCP
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            capabilities:
              drop:
                - ALL
          livenessProbe:
            httpGet:
              path: /healthz
              port: 80
            initialDelaySeconds: 10
            periodSeconds: 10
            timeoutSeconds: 5
            failureThreshold: 3
          readinessProbe:
            httpGet:
              path: /ready
              port: 80
            initialDelaySeconds: 5
            periodSeconds: 5
            timeoutSeconds: 3
            failureThreshold: 3
```

**标注规则**:
- `# Valid for v1.28+` — 命令/API 在此版本及之后可用
- `# Deprecated in v1.XX, use <replacement>` — 废弃用法标注替代方案
- `# Requires v1.XX+` — 特性版本要求
- `# Default behavior changed in v1.XX` — 默认行为变更提示

### 3.3 RBAC 最小权限原则

**修改前**:
```yaml
rbac: cluster-admin
```

**修改后**:
```yaml
rbac_minimum:
  core_api:
    resources: ["nodes", "pods", "events", "pods/log", "pods/status"]
    verbs: ["get", "list", "watch"]
  coordination:
    apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["get", "list"]
  certificates:
    apiGroups: ["certificates.k8s.io"]
    resources: ["certificatesigningrequests"]
    verbs: ["get", "list"]
# 如需执行修复操作，额外需要:
#   nodes: ["patch", "delete"]
#   pods: ["delete", "evict"]
```

### 3.4 废弃 API 替换清单

| 废弃用法 | 状态 | 替代方案 | 版本说明 |
|---------|------|---------|---------|
| `policy/v1beta1 PodDisruptionBudget` | v1.25 移除 | `policy/v1` | `# Valid for v1.21+` |
| `autoscaling/v2beta2 HorizontalPodAutoscaler` | v1.26 移除 | `autoscaling/v2` | `# Valid for v1.23+` |
| `batch/v1beta1 CronJob` | v1.25 移除 | `batch/v1` | `# Valid for v1.21+` |
| `discovery.k8s.io/v1beta1 EndpointSlice` | v1.25 移除 | `discovery.k8s.io/v1` | `# Valid for v1.21+` |
| `networking.k8s.io/v1beta1 Ingress` | v1.22 移除 | `networking.k8s.io/v1` | `# Valid for v1.19+` |
| `node-role.kubernetes.io/master` | v1.24 废弃 | `node-role.kubernetes.io/control-plane` | `# Valid for v1.20+` |

---

## 4. 分文件详细修改规格

### 4.1 根目录 Markdown 文件（01-19）

#### 通用修改（适用于所有文件）

1. **Front Matter 增强**:
   - `k8s_versions` 改为 `"1.XX.x"` 格式
   - 新增 `tested_on` 字段（具体 patch 版本）
   - 新增 `k8s_version_notes` 字段（版本特性摘要）
   - `last_updated` 更新为当前日期

2. **前置条件章节增强**:
   - RBAC 权限从 `cluster-admin` 改为最小权限列表
   - 工具要求增加最低版本号（kubectl v1.28+）
   - 增加 `kubectl version --client` 验证命令

3. **所有 YAML 代码块增强**:
   - 补充 `resources`（requests + limits）
   - 补充 `livenessProbe` / `readinessProbe`
   - 补充 `securityContext`（Pod 级 + 容器级）
   - 补充 `imagePullPolicy: IfNotPresent`
   - 在代码块首行或关键行添加版本注释

4. **kubectl 命令增强**:
   - 关键命令旁添加版本注释
   - 增加 `--dry-run=client` 验证建议
   - 增加 `-o yaml` / `-o json` 输出格式建议用于调试

#### 文件特定修改

**01-node-notready.md**:
- 第 57 行: `pod-eviction-timeout` 补充 PodDisruptionConditions (v1.29 GA) 的交互说明
- 第 69 行: RBAC 权限细化
- 增加版本差异章节: EventedPLEG (v1.31 GA) vs GenericPLEG 诊断命令差异
- 增加节点优雅关闭 (GracefulNodeShutdown v1.28 GA) 的排查步骤

**02-pod-crashloop-oomkilled.md**:
- 第 72 行: `kubectl debug` 标注版本要求 `# Requires v1.28+ (Ephemeral Containers GA)`
- 增加 Native Sidecar Containers (v1.28 beta, v1.32 GA) 对 CrashLoop 的影响
- 增加 cgroup v2 内存统计差异说明 (v1.25+ 默认)

**03-pod-pending.md**:
- SchedulingGates 标注 `# Requires v1.27+ (GA)`
- 增加 PodDisruptionConditions 对调度影响的说明
- Topology Spread Constraints 增加 `nodeAffinityPolicy` 和 `nodeTaintsPolicy` (v1.26+)

**04-dns-resolution-failure.md**:
- CoreDNS 版本兼容性: CoreDNS 1.11+ (K8s v1.30+) 的插件变更
- NodeLocal DNSCache 标注 `# Valid for v1.18+`
- 增加 DNS 内存限制调优建议（CoreDNS OOM 问题）

**05-service-connectivity.md**:
- kube-proxy 模式增加 nftables (v1.32 GA) 说明
- EndpointSlice 诊断增加 `discovery.k8s.io/v1` API 说明
- NetworkPolicy 增加 `AdminNetworkPolicy` / `BaselineAdminNetworkPolicy` (v1.30 alpha) 提示

**06-certificate-expiry.md**:
- cert-manager API: `cert-manager.io/v1` 稳定性说明
- 增加 `kubernetes.io/kube-apiserver-client` CSR 类型说明
- 手动证书轮转增加回滚步骤

**07-pvc-storage-failure.md**:
- 增加 VolumeAttributesClass (v1.31 beta) 说明
- CSI 快照恢复增加版本兼容性
- 多挂载错误 (`Multi-Attach error`) 增加诊断命令

**08-deployment-rollout-failure.md**:
- 增加 maxSurge/maxUnavailable 计算最佳实践
- 增加 `progressDeadlineSeconds` 调优建议
- PodDisruptionBudget 互锁增加警告

**09-rbac-quota-failure.md**:
- 增加 ValidatingAdmissionPolicy (v1.30 GA) 说明
- LimitRange 默认注入示例增加
- 最小权限原则增加具体 Role 示例

**10-image-pull-failure.md**:
- 增加镜像拉取策略矩阵
- registry mirror 配置增加 containerd 版本差异
- 多架构镜像增加 `nodeSelector` 兼容性说明

**11-control-plane-failure.md**:
- etcd 备份增加 `etcdctl snapshot save` 版本要求
- APF (API Priority and Fairness) 增加 v1.29 增强特性
- 增加 API Server 健康检查端点 `/livez` / `/readyz` 说明

**12-autoscaling-failure.md**:
- HPA `behavior.scaleDown.stabilizationWindowSeconds` 调优
- 增加 `containerResource` 指标类型 (HPA v2, v1.27 GA)
- Cluster Autoscaler 版本匹配要求

**13-ingress-gateway-failure.md**:
- Gateway API `v1beta1` → `v1` (v1.30+) 迁移说明
- IngressClass 增加 `ingressclass.kubernetes.io/is-default-class` 注解
- gRPC 路由增加 `GRPCRoute` (v1.1.0+) 说明

**14-configmap-secret-failure.md**:
- immutable ConfigMap/Secret (v1.21 GA) 增加使用建议
- External Secrets Operator 版本兼容性
- Vault Agent 注入增加安全上下文要求

**15-monitoring-alerting-failure.md**:
- Prometheus v2.x → v3.x 配置差异
- Alertmanager `route` 配置增加 `continue` 字段说明
- Thanos 架构增加 Store Gateway 诊断

**16-logging-pipeline-failure.md**:
- Vector/Fluent Bit 性能调优参数
- Loki 标签基数控制最佳实践
- 审计日志增加 `audit.k8s.io/v1` API 说明

**17-performance-bottleneck.md**:
- CPU throttling 量化: `cpu.cfs_quota_us` / `cpu.cfs_period_us`
- 增加 eBPF 工具链 (`kubectl trace`, `inspektor-gadget`)
- 内存碎片分析增加 `memory.available` 计算说明

**18-security-incident-response.md**:
- 容器逃逸检测增加 `securityContext` 加固清单
- 供应链安全增加 Sigstore/Cosign 验证
- 增加 Falco 规则版本兼容性

**19-skill-local-demo-guide.md**:
- Kind 镜像版本更新建议
- macOS Apple Silicon (M1/M2/M3) 兼容性说明
- 资源要求: 最少 4 CPU / 8GB RAM

### 4.2 skill-set/k8s-node-notready/ 文件

**SKILL.md**:
- 第 9 行: `k8s_versions` 增加 patch 粒度
- 第 12-15 行: `requires` 增加工具最低版本
- 第 121-123 行: emoji 风险指示器替换为文本（LOW/MEDIUM/HIGH/CRITICAL）
- 增加版本测试日期字段

**USAGE-GUIDE.md**:
- 第 73 行: 修复 `remediate.sh` 引用错误（该文件不存在）
- 第 361-371 行: 增加异常处理伪代码
- 第 409-413 行: 增加 ID 命名空间验证正则

**assets/skill-metadata.yaml**:
- 第 14 行: `k8s_versions` 增加 patch 粒度，新增 `tested_on`
- 第 22-26 行: RBAC 权限细化，移除 `cluster-admin`
- 第 162-172 行: remediation 列表增加 `risk` 字段

**assets/root-cause-map.yaml**:
- 第 167-171 行: RC-004 增加 swap 诊断规则（v1.30+）
- 第 293-301 行: RC-007 增加自动轮转失败分析
- 第 342-349 行: RC-008 增加 EventedPLEG 区分诊断
- 第 641-689 行: 消除 `remediation_map` 与 `root_causes[].remediation` 的重复

**scripts/diagnose-quick.sh**:
- 增加 `kubectl version --client` 版本检查
- 增加错误处理（`set -euo pipefail`）
- 增加节点存在性验证

**scripts/diagnose-deep.sh**:
- 增加 SSH 连接超时和重试
- 增加 sudo 权限验证
- 增加命令失败回退

**scripts/check-resources.sh**:
- 增加阈值可配置（环境变量或参数）
- 输出格式化为 JSON/表格

**scripts/cleanup-disk.sh**:
- 增加操作确认提示
- 增加镜像白名单保护
- 增加日志保留天数参数

**scripts/verify-node.sh**:
- 增加多条件验证（Ready + 无压力 + 可调度）
- 增加超时机制和重试

### 4.3 skills-run/ 文件

**README.md**:
- 修复 `cd topic-skills/demo` → `cd topic-skills/skills-run`
- 增加故障排查章节（Kind 创建失败、节点不 Ready）
- 增加硬件资源要求说明

**setup-kind-cluster.sh**:
- 第 78 行: `mktemp` 可移植性修复
  ```bash
  KIND_CONFIG=$(mktemp "${TMPDIR:-/tmp}/kind-config-XXXXXX.yaml") || exit 1
  trap 'rm -f "${KIND_CONFIG}"' EXIT
  ```
- 第 130 行: 增加 `--wait 300s` 或环境变量配置
- 增加 Docker 资源检查
- 部署 YAML 增加 securityContext 和 probes

**run-skill-demo.sh**:
- `ls ${script}` 替换为数组 glob 处理
- 增加脚本可执行性检查
- 增加 `--dry-run` 全局标志支持

**teardown.sh**:
- 增加删除确认提示
- 增加集群信息显示

**scenarios/*.sh（全部 10 个）**:
- `eval "$1"` 模式替换为 `eval` 安全包装或函数调用
- 增加单节点集群兼容处理
- `sleep` 替换为条件轮询
- 增加 `set -euo pipefail`

---

## 5. 边界条件与异常处理

### 5.1 版本兼容性边界

- **kubectl 客户端版本**: 要求 `kubectl` 版本不低于集群版本 +/- 1 个 minor 版本。所有脚本增加版本检查。
- **Kind 集群版本**: 默认 `kindest/node:v1.31.4`，支持通过 `KIND_IMAGE` 环境变量覆盖。
- **macOS 可移植性**: 所有 `mktemp` 调用必须兼容 BSD 和 GNU 版本。

### 5.2 安全边界

- **RBAC 最小化**: 所有 `cluster-admin` 引用替换为具体权限列表，保留注释说明如需修复操作需额外权限。
- **脚本安全**: 消除 `eval` 对不可信输入的使用，增加输入验证。
- **生产环境保护**: 所有清理/修改类脚本增加确认提示，支持 `--force` / `--yes` 参数用于 CI。

### 5.3 错误处理

- 所有 Bash 脚本统一使用 `set -euo pipefail`
- 增加 `trap` 清理临时文件
- 关键命令增加错误消息和退出码
- 网络操作增加超时和重试

---

## 6. 数据流路径

```
用户查询/告警
    ↓
topic-skills/XX-*.md (触发条件匹配)
    ↓
skill-set/.../assets/skill-metadata.yaml (Agent 路由)
    ↓
skill-set/.../reference/diagnostic-workflow.md (诊断流程)
    ↓
skill-set/.../scripts/diagnose-quick.sh / diagnose-deep.sh (执行诊断)
    ↓
skill-set/.../assets/root-cause-map.yaml (根因映射)
    ↓
skill-set/.../reference/remediation-playbook.md (修复方案)
    ↓
skill-set/.../scripts/*.sh (执行修复)
    ↓
skill-set/.../scripts/verify-node.sh (验证)
```

---

## 7. 预期输出

1. 所有文件内容更新，保持原有目录结构和文件名
2. 所有 YAML 示例符合生产规范（resources + probes + securityContext）
3. 所有 kubectl 命令/API 标注版本兼容性
4. 所有废弃 API 已替换为稳定版本语法
5. 所有脚本通过 `shellcheck` 基本检查（无未定义变量、无可移植性问题）
6. 所有 front matter 包含 patch 级别版本信息和测试版本

---

## 8. 验收标准

- [ ] 内容符合资深 SRE 交付标准，侧重实战指令
- [ ] 所有示例代码和命令标注了清晰的版本兼容性说明
- [ ] YAML 示例包含 resource limits、probes、securityContext
- [ ] 废弃 API 已全部替换（policy/v1beta1 → policy/v1 等）
- [ ] 脚本可移植性修复（mktemp、eval 等）
- [ ] RBAC 配置从 cluster-admin 最小化到具体权限
- [ ] 所有 Bash 脚本使用 `set -euo pipefail`
- [ ] 无纯理论描述，每个技术点对应具体命令或配置
