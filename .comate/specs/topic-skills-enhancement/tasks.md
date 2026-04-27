# topic-skills 生产级增强任务计划

## 任务分组原则
- 按文件类别和依赖关系分组，同组文件共享通用增强规则
- 优先完成根目录主题技能文件（01-19），再处理 skill-set 和 skills-run
- 每完成一个顶级任务立即更新状态

---

- [x] Task 1: 根目录主题技能 — 通用 Front Matter 与前置条件增强（所有 19 个文件）
    - 1.1: 统一 `k8s_versions` 为 `"1.XX.x"` 格式，新增 `tested_on` 和 `k8s_version_notes` 字段
    - 1.2: 更新 `last_updated` 为当前日期
    - 1.3: 前置条件章节：RBAC 从 `cluster-admin` 改为最小权限列表，工具增加最低版本号
    - 1.4: 增加 `kubectl version --client` 验证命令示例

- [x] Task 2: 根目录主题技能 — YAML 示例生产规范化（所有 19 个文件）
    - 2.1: 所有 Deployment/StatefulSet/DaemonSet YAML 补充 `resources`（requests + limits）
    - 2.2: 补充 `livenessProbe` / `readinessProbe`（根据应用类型选择 httpGet/tcpSocket/exec）
    - 2.3: 补充 Pod 级和容器级 `securityContext`（runAsNonRoot, readOnlyRootFilesystem, capabilities drop ALL, allowPrivilegeEscalation: false, seccompProfile）
    - 2.4: 补充 `imagePullPolicy: IfNotPresent`
    - 2.5: 在 YAML 代码块首行或关键行添加版本注释（`# Valid for v1.28+` 等）

- [x] Task 3: 根目录主题技能 — kubectl 命令与 API 版本标注（所有 19 个文件）
    - 3.1: 关键 kubectl 命令旁添加版本注释
    - 3.2: 废弃 API 替换（`policy/v1beta1`→`policy/v1`, `autoscaling/v2beta2`→`autoscaling/v2`, `batch/v1beta1`→`batch/v1`, `discovery.k8s.io/v1beta1`→`v1`, `networking.k8s.io/v1beta1`→`v1`）
    - 3.3: 移除 `node-role.kubernetes.io/master` 引用，替换为 `control-plane`
    - 3.4: 增加 `--dry-run=client` 验证建议和 `-o yaml`/`-o json` 调试输出建议

- [x] Task 4: 根目录主题技能 — Node/Pod 类深度内容增强（01-03）
    - 4.1: `01-node-notready.md`: 补充 PodDisruptionConditions (v1.29 GA) 交互说明、EventedPLEG (v1.31 GA) 诊断差异、GracefulNodeShutdown (v1.28 GA) 排查步骤
    - 4.2: `02-pod-crashloop-oomkilled.md`: 补充 Native Sidecar Containers 影响、cgroup v2 内存统计差异、`kubectl debug` 版本标注
    - 4.3: `03-pod-pending.md`: 补充 SchedulingGates 版本标注、Topology Spread Constraints v1.26+ 新字段

- [x] Task 5: 根目录主题技能 — Network/Security 类深度内容增强（04-06, 09）
    - 5.1: `04-dns-resolution-failure.md`: CoreDNS 1.11+ 插件变更、NodeLocal DNSCache 内存限制调优
    - 5.2: `05-service-connectivity.md`: nftables kube-proxy (v1.32 GA)、EndpointSlice v1 API、NetworkPolicy 调试
    - 5.3: `06-certificate-expiry.md`: cert-manager API 稳定性、CSR 类型说明、手动轮转回滚步骤
    - 5.4: `09-rbac-quota-failure.md`: ValidatingAdmissionPolicy (v1.30 GA)、最小权限 Role 示例、LimitRange 默认注入

- [x] Task 6: 根目录主题技能 — Storage/Workload/Image 类深度内容增强（07-08, 10）
    - 6.1: `07-pvc-storage-failure.md`: VolumeAttributesClass (v1.31 beta)、CSI 快照恢复、Multi-Attach 诊断
    - 6.2: `08-deployment-rollout-failure.md`: maxSurge/maxUnavailable 计算最佳实践、progressDeadlineSeconds 调优、PDB 互锁警告
    - 6.3: `10-image-pull-failure.md`: 镜像拉取策略矩阵、registry mirror 版本差异、多架构镜像 nodeSelector

- [x] Task 7: 根目录主题技能 — ControlPlane/Scaling/Gateway 类深度内容增强（11-13）
    - 7.1: `11-control-plane-failure.md`: etcdctl snapshot 版本要求、APF v1.29 增强、`/livez` `/readyz` 端点
    - 7.2: `12-autoscaling-failure.md`: HPA behavior 调优、`containerResource` 指标类型 (v1.27 GA)、CA 版本匹配
    - 7.3: `13-ingress-gateway-failure.md`: Gateway API v1beta1→v1 迁移、IngressClass 默认注解、GRPCRoute

- [x] Task 8: 根目录主题技能 — Config/Observability/Performance/Security 类深度内容增强（14-18）
    - 8.1: `14-configmap-secret-failure.md`: immutable ConfigMap/Secret 建议、External Secrets Operator 兼容性、Vault Agent 安全上下文
    - 8.2: `15-monitoring-alerting-failure.md`: Prometheus v2→v3 差异、Alertmanager `continue` 字段、Thanos Store Gateway
    - 8.3: `16-logging-pipeline-failure.md`: Vector/Fluent Bit 性能调优、Loki 标签基数控制、审计日志 API
    - 8.4: `17-performance-bottleneck.md`: CPU throttling 量化、eBPF 工具链、内存碎片分析
    - 8.5: `18-security-incident-response.md`: securityContext 加固清单、Sigstore/Cosign、Falco 版本兼容性

- [x] Task 9: `19-skill-local-demo-guide.md` 增强
    - 9.1: Kind 镜像版本更新建议、macOS Apple Silicon 兼容性
    - 9.2: 资源要求说明（最少 4 CPU / 8GB RAM）
    - 9.3: 通用 Front Matter 和 YAML 规范化（同 Task 1-3 规则）

- [x] Task 10: skill-set 元数据与资产文件增强
    - 10.1: `assets/skill-metadata.yaml`: k8s_versions 增加 patch 粒度和 tested_on、RBAC 权限细化、remediation 增加 risk 字段
    - 10.2: `assets/root-cause-map.yaml`: RC-004 swap 诊断规则 (v1.30+)、RC-007 自动轮转失败分析、RC-008 EventedPLEG 区分、消除 remediation_map 重复
    - 10.3: `assets/symptom-patterns.yaml`: 版本排除规则、正则校验
    - 10.4: `assets/escalation-template.md`: 告警模板标准化、版本信息注入

- [x] Task 11: skill-set 参考文档增强
    - 11.1: `SKILL.md`: 版本矩阵增强、requires 增加工具最低版本、emoji→text 标准化（LOW/MEDIUM/HIGH/CRITICAL）
    - 11.2: `USAGE-GUIDE.md`: 修复 `remediate.sh` 引用错误、增加异常处理伪代码、ID 命名空间验证正则
    - 11.3: `reference/diagnostic-workflow.md`: 每步添加版本兼容性说明
    - 11.4: `reference/remediation-playbook.md`: 风险等级量化、回滚步骤
    - 11.5: `reference/root-cause-catalog.md`: 版本行为差异对照
    - 11.6: `reference/version-matrix.md`: v1.28-v1.32 完整特性矩阵更新

- [x] Task 12: skill-set 脚本增强
    - 12.1: `scripts/diagnose-quick.sh`: 增加 kubectl 版本检查、`set -euo pipefail`、节点存在性验证
    - 12.2: `scripts/diagnose-deep.sh`: SSH 连接超时和重试、sudo 权限验证、命令失败回退
    - 12.3: `scripts/check-resources.sh`: 阈值可配置（环境变量/参数）、输出结构化
    - 12.4: `scripts/cleanup-disk.sh`: 操作确认提示、镜像白名单保护、日志保留天数参数
    - 12.5: `scripts/verify-node.sh`: 多条件验证（Ready+无压力+可调度）、超时机制和重试

- [x] Task 13: skills-run 基础设施脚本增强
    - 13.1: `README.md`: 修复 `cd` 路径错误、增加故障排查章节、硬件资源要求
    - 13.2: `setup-kind-cluster.sh`: mktemp 可移植性修复（BSD/GNU 兼容）、trap 清理临时文件、增加 Docker 资源检查、部署 YAML 增加 securityContext 和 probes、等待超时改为 300s
    - 13.3: `run-skill-demo.sh`: `ls` glob 替换为数组处理、增加脚本可执行性检查、增加 `--dry-run` 全局标志
    - 13.4: `teardown.sh`: 增加删除确认提示、集群信息显示

- [x] Task 14: skills-run 场景脚本增强（全部 10 个）
    - 14.1: 所有场景脚本增加 `set -euo pipefail`
    - 14.2: `eval "$1"` 模式替换为安全函数调用包装
    - 14.3: 增加单节点集群兼容处理（检测节点数量，少于 2 个 worker 时跳过 cordon/drain 类场景或输出警告）
    - 14.4: `sleep` 替换为条件轮询（如 `kubectl wait` 或循环检测）
    - 14.5: 增加 kubectl 版本和集群连通性预检
    - 14.6: 增加 `trap` 清理创建的资源（on EXIT/ERR）

- [x] Task 15: 最终验证与收尾
    - 15.1: 检查所有文件 front matter 一致性（k8s_versions 格式、tested_on 存在）
    - 15.2: 全局搜索确认废弃 API 已替换（`policy/v1beta1`, `autoscaling/v2beta2`, `batch/v1beta1`, `discovery.k8s.io/v1beta1`, `networking.k8s.io/v1beta1`, `node-role.kubernetes.io/master`）
    - 15.3: 全局搜索确认 `cluster-admin` RBAC 已细化
    - 15.4: 检查所有新增/修改的 Bash 脚本是否使用 `set -euo pipefail`
    - 15.5: 运行 `git diff --stat` 确认修改范围符合预期
