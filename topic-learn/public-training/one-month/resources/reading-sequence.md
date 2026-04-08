# 文档阅读顺序索引

本文档按学习计划顺序整理了 kudig-database 知识库中的关键文档。

---

## Week 1: 地基建设期

### Day 1-2: Docker 基础
1. `domain-13-docker/01-docker-architecture-overview.md`
2. `domain-13-docker/03-docker-container-lifecycle.md`
3. `domain-13-docker/04-docker-networking-deep-dive.md`
4. `domain-13-docker/05-docker-storage-volumes.md`
5. `domain-13-docker/07-docker-security-best-practices.md`
6. `domain-13-docker/99-docker-commands-reference.md`

### Day 3-4: Linux 基础
1. `domain-14-linux/01-linux-system-architecture.md`
2. `domain-14-linux/02-linux-process-management.md`
3. `domain-14-linux/04-linux-networking-configuration.md`
4. `domain-14-linux/06-linux-performance-tuning.md`
5. `domain-14-linux/08-linux-container-fundamentals.md`
6. `domain-14-linux/99-linux-commands-reference.md`

### Day 5-6: K8s 架构
1. `domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md` ⭐
2. `domain-1-architecture-fundamentals/02-core-components-deep-dive.md` ⭐
3. `domain-1-architecture-fundamentals/03-api-versions-features.md`
4. `domain-1-architecture-fundamentals/05-kubectl-commands-reference.md`
5. `domain-1-architecture-fundamentals/06-cluster-configuration-parameters.md`
6. `topic-cheat-sheet/k8s.md`

---

## Week 2: 核心技术构建期

### Day 8-9: 控制平面
1. `domain-3-control-plane/11-etcd-deep-dive.md`
2. `domain-3-control-plane/12-apiserver-deep-dive.md`
3. `domain-3-control-plane/13-kube-controller-manager-deep-dive.md`
4. `domain-3-control-plane/20-kube-scheduler-deep-dive.md`
5. `domain-2-design-principles/03-controller-pattern.md`
6. `domain-2-design-principles/07-distributed-consensus-etcd.md`

### Day 10-11: 工作负载
1. `domain-4-workloads/02-deployment-production-patterns.md`
2. `domain-4-workloads/03-statefulset-advanced-operations.md`
3. `domain-4-workloads/04-daemonset-management.md`
4. `domain-4-workloads/11-pod-lifecycle-events.md`
5. `domain-4-workloads/21-hpa-vpa-autoscaling.md`
6. `domain-4-workloads/23-resource-management.md`

### Day 12-13: 网络
1. `domain-5-networking/01-network-architecture-overview.md`
2. `domain-5-networking/02-cni-architecture-fundamentals.md`
3. `domain-5-networking/06-service-concepts-types.md`
4. `domain-5-networking/11-dns-service-discovery-coredns.md`
5. `domain-5-networking/16-networkpolicy-deep-practice.md`
6. `domain-5-networking/19-ingress-fundamentals.md`
7. `domain-5-networking/21-nginx-ingress-complete-guide.md`
8. `domain-5-networking/22-ingress-tls-certificate.md`

### Day 14: 存储
1. `domain-6-storage/01-storage-architecture-overview.md`
2. `domain-6-storage/02-pv-architecture-fundamentals.md`
3. `domain-6-storage/04-storageclass-dynamic-provisioning.md`

---

## Week 3: 运维作战能力期

### Day 15-16: 安全
1. `domain-7-security/01-authentication-authorization-system.md`
2. `domain-7-security/06-pod-security-standards.md`
3. `domain-7-security/07-rbac-matrix-configuration.md`
4. `domain-7-security/10-certificate-management.md`
5. `domain-7-security/11-secret-management-tools.md`
6. `domain-7-security/14-policy-engines-opa-kyverno.md`

### Day 17-18: 可观测性
1. `domain-8-observability/01-observability-architecture-overview.md`
2. `domain-8-observability/02-monitoring-metrics-system.md`
3. `domain-8-observability/03-logging-architecture.md`
4. `domain-8-observability/04-distributed-tracing.md`
5. `domain-8-observability/05-alerting-management.md`
6. `domain-8-observability/10-monitoring-metrics-prometheus.md`
7. `domain-8-observability/21-monitoring-playbooks.md`

### Day 19-21: 故障排查
1. `topic-structural-trouble-shooting/README.md` ⭐
2. `topic-fta/04-fta-core-principles.md` ⭐
3. `topic-febm/01-febm-theory-foundations.md` ⭐
4. `domain-12-troubleshooting/05-pod-pending-diagnosis.md`
5. `domain-12-troubleshooting/06-node-notready-diagnosis.md`
6. `domain-12-troubleshooting/07-oom-memory-diagnosis.md`
7. `domain-12-troubleshooting/08-pod-comprehensive-troubleshooting.md`
8. `domain-12-troubleshooting/10-service-comprehensive-troubleshooting.md`
9. `domain-9-platform-ops/02-cluster-lifecycle-management.md`
10. `domain-9-platform-ops/12-backup-recovery-strategy.md`

---

## Week 4: 企业级进阶期

### Day 22-23: 企业级工具
1. `domain-20-enterprise-monitoring-alerting/01-prometheus-enterprise-monitoring.md`
2. `domain-20-enterprise-monitoring-alerting/02-grafana-enterprise-observability.md`
3. `domain-21-logging-management-analytics/01-elk-stack-enterprise-logging.md`
4. `domain-23-gitops-ci-cd/01-argo-cd-enterprise-gitops.md`
5. `domain-8-observability/18-slo-sli-system.md`

### Day 24-25: 安全与最佳实践
1. `domain-25-cloud-native-security/04-kyverno-enterprise-policy-management.md`
2. `domain-25-cloud-native-security/05-vault-enterprise-secrets-management.md`
3. `domain-18-production-operations/01-production-architecture-design-principles.md`
4. `domain-18-production-operations/07-zero-trust-security-architecture.md`
5. `domain-18-production-operations/22-change-management-process.md`
6. `domain-18-production-operations/23-incident-response-handling.md`
7. `domain-18-production-operations/24-capacity-planning-forecasting.md`

### Day 26-27: 专题深化
1. `topic-fta/23-fta-production-quick-start.md` ⭐
2. `topic-fta/kubernetes-fta-full-analysis.md`
3. `topic-febm/08-febm-production-quick-start.md` ⭐
4. `topic-fta/10-agent-orchestration-patterns.md`
5. `domain-10-extensions/01-crd-development-guide.md`
6. `domain-10-extensions/06-helm-charts-management.md`
7. `domain-9-platform-ops/20-crd-operator-development.md`

---

## 标记说明

- ⭐ 核心文档，必须精读
- 其他文档按需阅读，可根据时间调整深度
