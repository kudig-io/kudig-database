---
title: Domain 07 内容索引
summary: Domain 07 内容索引
category: 平台工程
tags:
- index
- 平台工程
- navigation
tier: supporting
sources:
- auto-generated
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain 07 内容索引

> 本索引汇总了 平台工程 下的所有文档，按主题分组。

## 概述
- [[README]] — Domain 总览

## 根目录文档
- [[08-automated-operations-toolchain]] — Automated operations toolchain
- [[17-karpenter-node-autoscaling-guide]] — Karpenter node autoscaling guide
- [[18-keda-event-driven-autoscaling-guide]] — Keda event driven autoscaling guide

## 按主题分组

### 98 Merged Indexes

- [[00-open-source-projects-index-from-domain-36]] — Open source projects index from domain 36
- [[01-open-source-projects-index-from-domain-9]] — Open source projects index from domain 9
- [[MOC-from-domain-36]] — MOC from domain 36
- [[MOC-from-domain-9]] — MOC from domain 9
- [[README-from-domain-36]] — README from domain 36
- [[README-from-domain-9]] — README from domain 9

### 平台构建

- [[01-platform-engineering-overview]] — Platform engineering overview
- [[02-idp-design-principles]] — Idp design principles
- [[03-backstage-deployment]] — Backstage deployment
- [[04-backstage-catalog-techdocs]] — Backstage catalog techdocs
- [[05-backstage-scaffolder-templates]] — Backstage scaffolder templates
- [[06-kratix-platform-as-code]] — Kratix platform as code
- [[07-crossplane-platform-composition]] — Crossplane platform composition
- [[08-golden-paths-design]] — Golden paths design
- [[09-vercel-frontend-deployment-platform]] — Vercel frontend deployment platform
- [[10-crd-operator-development]] — Crd operator development
- [[11-api-aggregation]] — Api aggregation
- [[12-client-libraries]] — Client libraries
- [[13-addons-extensions]] — Addons extensions
- [[15-backstage-idp-guide]] — Backstage idp guide
- [[16-java-k8s-client-operator-guide]] — Java k8s client operator guide

### 开发者体验

- [[03-developer-experience-metrics]] — Developer experience metrics
- [[04-platform-team-topology]] — Platform team topology
- [[05-cli-enhancement-tools]] — Cli enhancement tools
- [[06-kubectl-plugin-ecosystem]] — Kubectl plugin ecosystem

### 治理与管控

- [[10-平台工程/03-治理/01-capacity-planning-resource-assessment]] — Capacity planning resource assessment
- [[02-performance-benchmarking-tuning]] — Performance benchmarking tuning
- [[03-cost-optimization-finops]] — Cost optimization finops
- [[04-security-compliance]] — Security compliance
- [[05-large-scale-cluster-optimization]] — Large scale cluster optimization
- [[06-multi-tenant-management]] — Multi tenant management

### 平台运维

- [[01-platform-ops-overview]] — Platform ops overview
- [[02-cluster-lifecycle-management]] — Cluster lifecycle management
- [[03-operations-metrics-system]] — Operations metrics system
- [[04-monitoring-alerting-system]] — Monitoring alerting system
- [[05-gitops-configuration-management]] — Gitops configuration management
- [[06-automation-toolchain]] — Automation toolchain
- [[07-disaster-recovery-business-continuity]] — Disaster recovery business continuity
- [[09-backup-recovery-strategy]] — Backup recovery strategy
- [[13-multi-cluster-management]] — Multi cluster management
- [[12-production-troubleshooting]] — Production troubleshooting
- [[13-platform-upgrade-migration]] — Platform upgrade migration
- [[14-platform-observability-practice]] — Platform observability practice
- [[15-lease-leader-election]] — Lease leader election
- [[16-virtual-clusters]] — Virtual clusters
- [[19-kubernetes-v1.33-platform-ops-guide]] — Kubernetes v1.33 platform ops guide

### 代码分析专题

#### Topic Code Analysis

- [[MOC]] — MOC
- [[README]] — README

#### 集群证书

- [[10-平台工程/06-代码分析/cluster-cert/01-pki-architecture.md|01-pki-architecture]] — Pki architecture
- [[10-平台工程/06-代码分析/cluster-cert/02-ca-generation.md|02-ca-generation]] — Ca generation
- [[10-平台工程/06-代码分析/cluster-cert/03-apiserver-cert.md|03-apiserver-cert]] — Apiserver cert
- [[10-平台工程/06-代码分析/cluster-cert/04-etcd-cert.md|04-etcd-cert]] — Etcd cert
- [[10-平台工程/06-代码分析/cluster-cert/05-kubelet-cert.md|05-kubelet-cert]] — Kubelet cert
- [[10-平台工程/06-代码分析/cluster-cert/06-cert-rotation.md|06-cert-rotation]] — Cert rotation
- [[10-平台工程/06-代码分析/cluster-cert/07-service-account-keys.md|07-service-account-keys]] — Service account keys
- [[10-平台工程/06-代码分析/cluster-cert/08-rbac-mapping.md|08-rbac-mapping]] — Rbac mapping
- [[10-平台工程/06-代码分析/cluster-cert/09-join-cert-flow.md|09-join-cert-flow]] — Join cert flow
- [[10-平台工程/06-代码分析/cluster-cert/10-front-proxy-workflow.md|10-front-proxy-workflow]] — Front proxy workflow
- [[10-平台工程/06-代码分析/cluster-cert/11-apiserver-cert-flags.md|11-apiserver-cert-flags]] — Apiserver cert flags
- [[10-平台工程/06-代码分析/cluster-cert/12-kubeconfig-certs.md|12-kubeconfig-certs]] — Kubeconfig certs
- [[10-平台工程/06-代码分析/cluster-cert/13-cert-config.md|13-cert-config]] — Cert config
- [[10-平台工程/06-代码分析/cluster-cert/14-admission-webhook-certs.md|14-admission-webhook-certs]] — Admission webhook certs
- [[10-平台工程/06-代码分析/cluster-cert/15-cert-format-encoding.md|15-cert-format-encoding]] — Cert format encoding
- [[10-平台工程/06-代码分析/cluster-cert/16-openssl-cookbook.md|16-openssl-cookbook]] — Openssl cookbook
- [[10-平台工程/06-代码分析/cluster-cert/17-pki-security-best-practices.md|17-pki-security-best-practices]] — Pki security best practices
- [[10-平台工程/06-代码分析/cluster-cert/README.md|README]] — README

#### 集群创建

- [[10-平台工程/06-代码分析/cluster-create/01-overview.md|01-overview]] — Overview
- [[10-平台工程/06-代码分析/cluster-create/02-preflight.md|02-preflight]] — Preflight
- [[10-平台工程/06-代码分析/cluster-create/03-certs.md|03-certs]] — Certs
- [[10-平台工程/06-代码分析/cluster-create/04-kubeconfig.md|04-kubeconfig]] — Kubeconfig
- [[10-平台工程/06-代码分析/cluster-create/05-control-plane.md|05-control-plane]] — Control plane
- [[10-平台工程/06-代码分析/cluster-create/06-join.md|06-join]] — Join
- [[10-平台工程/06-代码分析/cluster-create/07-etcd.md|07-etcd]] — Etcd
- [[10-平台工程/06-代码分析/cluster-create/08-ha.md|08-ha]] — Ha
- [[10-平台工程/06-代码分析/cluster-create/09-upgrade.md|09-upgrade]] — Upgrade
- [[10-平台工程/06-代码分析/cluster-create/10-cloud-comparison.md|10-cloud-comparison]] — Cloud comparison
- [[10-平台工程/06-代码分析/cluster-create/11-advanced.md|11-advanced]] — Advanced
- [[10-平台工程/06-代码分析/cluster-create/12-join-advanced.md|12-join-advanced]] — Join advanced
- [[10-平台工程/06-代码分析/cluster-create/13-etcd-advanced.md|13-etcd-advanced]] — Etcd advanced
- [[10-平台工程/06-代码分析/cluster-create/14-ha-advanced.md|14-ha-advanced]] — Ha advanced
- [[10-平台工程/06-代码分析/cluster-create/15-upgrade-advanced.md|15-upgrade-advanced]] — Upgrade advanced
- [[10-平台工程/06-代码分析/cluster-create/16-security.md|16-security]] — Security
- [[10-平台工程/06-代码分析/cluster-create/17-init-phases.md|17-init-phases]] — Init phases
- [[10-平台工程/06-代码分析/cluster-create/18-cri-runtime.md|18-cri-runtime]] — Cri runtime
- [[10-平台工程/06-代码分析/cluster-create/19-cni-networking.md|19-cni-networking]] — Cni networking
- [[10-平台工程/06-代码分析/cluster-create/20-node-registration.md|20-node-registration]] — Node registration
- [[10-平台工程/06-代码分析/cluster-create/21-kube-proxy.md|21-kube-proxy]] — Kube proxy
- [[10-平台工程/06-代码分析/cluster-create/22-storage-volumes.md|22-storage-volumes]] — Storage volumes
- [[10-平台工程/06-代码分析/cluster-create/23-scheduler.md|23-scheduler]] — Scheduler
- [[10-平台工程/06-代码分析/cluster-create/24-what-kubeadm-does-not-install.md|24-what-kubeadm-does-not-install]] — What kubeadm does not install
- [[10-平台工程/06-代码分析/cluster-create/25-resource-management.md|25-resource-management]] — Resource management
- [[10-平台工程/06-代码分析/cluster-create/README.md|README]] — README

#### 集群删除

- [[10-平台工程/06-代码分析/cluster-delete/01-overview.md|01-overview]] — Overview
- [[10-平台工程/06-代码分析/cluster-delete/02-reset.md|02-reset]] — Reset
- [[10-平台工程/06-代码分析/cluster-delete/03-delete-node.md|03-delete-node]] — Delete node
- [[10-平台工程/06-代码分析/cluster-delete/04-cleanup.md|04-cleanup]] — Cleanup
- [[10-平台工程/06-代码分析/cluster-delete/05-etcd-cleanup.md|05-etcd-cleanup]] — Etcd cleanup
- [[10-平台工程/06-代码分析/cluster-delete/06-force-delete.md|06-force-delete]] — Force delete
- [[10-平台工程/06-代码分析/cluster-delete/07-ha-delete.md|07-ha-delete]] — Ha delete
- [[10-平台工程/06-代码分析/cluster-delete/08-cloud-delete.md|08-cloud-delete]] — Cloud delete
- [[10-平台工程/06-代码分析/cluster-delete/09-reset-phase-commands.md|09-reset-phase-commands]] — Reset phase commands
- [[10-平台工程/06-代码分析/cluster-delete/10-security-delete.md|10-security-delete]] — Security delete
- [[10-平台工程/06-代码分析/cluster-delete/11-network-cleanup.md|11-network-cleanup]] — Network cleanup
- [[10-平台工程/06-代码分析/cluster-delete/12-troubleshooting.md|12-troubleshooting]] — Troubleshooting
- [[10-平台工程/06-代码分析/cluster-delete/13-pre-delete-backup-checklist.md|13-pre-delete-backup-checklist]] — Pre delete backup checklist
- [[10-平台工程/06-代码分析/cluster-delete/README.md|README]] — README

#### Deployment 创建

- [[10-平台工程/06-代码分析/deployment-create/01-overview.md|01-overview]] — Overview
- [[10-平台工程/06-代码分析/deployment-create/02-deployment-controller.md|02-deployment-controller]] — Deployment controller
- [[10-平台工程/06-代码分析/deployment-create/03-replicaset-controller.md|03-replicaset-controller]] — Replicaset controller
- [[10-平台工程/06-代码分析/deployment-create/04-rolling-update.md|04-rolling-update]] — Rolling update
- [[10-平台工程/06-代码分析/deployment-create/05-deployment-status.md|05-deployment-status]] — Deployment status
- [[10-平台工程/06-代码分析/deployment-create/06-revision-history.md|06-revision-history]] — Revision history
- [[10-平台工程/06-代码分析/deployment-create/07-recreate-strategy.md|07-recreate-strategy]] — Recreate strategy
- [[10-平台工程/06-代码分析/deployment-create/08-hpa-integration.md|08-hpa-integration]] — Hpa integration
- [[10-平台工程/06-代码分析/deployment-create/09-canary-bluegreen.md|09-canary-bluegreen]] — Canary bluegreen
- [[10-平台工程/06-代码分析/deployment-create/10-workload-comparison.md|10-workload-comparison]] — Workload comparison
- [[10-平台工程/06-代码分析/deployment-create/README.md|README]] — README

#### 节点创建

- [[10-平台工程/06-代码分析/node-create/01-overview.md|01-overview]] — Overview
- [[10-平台工程/06-代码分析/node-create/02-registration.md|02-registration]] — Registration
- [[10-平台工程/06-代码分析/node-create/03-condition.md|03-condition]] — Condition
- [[10-平台工程/06-代码分析/node-create/04-drain.md|04-drain]] — Drain
- [[10-平台工程/06-代码分析/node-create/05-upgrade.md|05-upgrade]] — Upgrade
- [[10-平台工程/06-代码分析/node-create/06-certificate.md|06-certificate]] — Certificate
- [[10-平台工程/06-代码分析/node-create/07-autoscaling.md|07-autoscaling]] — Autoscaling
- [[10-平台工程/06-代码分析/node-create/08-troubleshooting.md|08-troubleshooting]] — Troubleshooting
- [[10-平台工程/06-代码分析/node-create/09-cni-node.md|09-cni-node]] — Cni node
- [[10-平台工程/06-代码分析/node-create/10-kubelet-config.md|10-kubelet-config]] — Kubelet config
- [[10-平台工程/06-代码分析/node-create/11-eviction.md|11-eviction]] — Eviction
- [[10-平台工程/06-代码分析/node-create/12-monitoring.md|12-monitoring]] — Monitoring
- [[10-平台工程/06-代码分析/node-create/13-security.md|13-security]] — Security
- [[10-平台工程/06-代码分析/node-create/14-storage-node.md|14-storage-node]] — Storage node
- [[10-平台工程/06-代码分析/node-create/15-cloud-node.md|15-cloud-node]] — Cloud node
- [[10-平台工程/06-代码分析/node-create/16-windows-node.md|16-windows-node]] — Windows node
- [[10-平台工程/06-代码分析/node-create/17-arm-multiarch.md|17-arm-multiarch]] — Arm multiarch
- [[10-平台工程/06-代码分析/node-create/README.md|README]] — README

## 相关 Domain
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|Domain 08 发布与变更管理 索引]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|Domain 11 生产运维 索引]]


<!-- risk-assessed -->
