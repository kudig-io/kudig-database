# Domain-34: CNCF Landscape 开源项目

> **文档数量**: 218 篇 | **最后更新**: 2026-03 | **数据来源**: [CNCF Landscape](https://landscape.cncf.io/)

---

## 概述

CNCF (Cloud Native Computing Foundation) Landscape 是云原生生态系统的全景图，涵盖了云原生计算基金会托管的所有开源项目。本领域收录了 CNCF 旗下全部开源项目的详细技术文档，按项目成熟度分为三个级别：Graduated（毕业）、Incubating（孵化中）、Sandbox（沙箱）。

## 核心价值

- **毕业项目 (Graduated)**: 经过生产验证的成熟项目，具有完善的治理和社区
- **孵化项目 (Incubating)**: 快速发展中的项目，已被多个组织采用
- **沙箱项目 (Sandbox)**: 早期创新项目，展示云原生技术的未来方向

---

## 项目统计

| 成熟度级别 | 项目数量 | 说明 |
|:---|:---:|:---|
| **Graduated** | 34 | 生产就绪，广泛采用 |
| **Incubating** | 37 | 快速成长，社区活跃 |
| **Sandbox** | 147 | 早期阶段，创新探索 |
| **总计** | **218** | - |

---

## Graduated 项目 (34个)

成熟稳定的云原生核心项目，已被大规模生产环境验证。

| 项目 | 分类 | 简介 |
|:---|:---|:---|
| [Argo](./graduated/argo/argo.md) | App Definition | Kubernetes 工作流引擎和 GitOps 持续交付 |
| [cert-manager](./graduated/cert-manager/cert-manager.md) | Security | Kubernetes 原生证书管理控制器 |
| [Cilium](./graduated/cilium/cilium.md) | Networking | 基于 eBPF 的网络、安全和可观测性 |
| [CloudEvents](./graduated/cloudevents/cloudevents.md) | Serverless | 事件数据规范 |
| [containerd](./graduated/containerd/containerd.md) | Container Runtime | 行业标准容器运行时 |
| [CoreDNS](./graduated/coredns/coredns.md) | Networking | 云原生 DNS 服务器 |
| [CRI-O](./graduated/cri-o/cri-o.md) | Container Runtime | Kubernetes 轻量级容器运行时 |
| [Crossplane](./graduated/crossplane/crossplane.md) | App Definition | 云原生控制平面框架 |
| [CubeFS](./graduated/cubefs/cubefs.md) | Storage | 云原生分布式存储 |
| [Dapr](./graduated/dapr/dapr.md) | App Definition | 分布式应用运行时 |
| [Dragonfly](./graduated/dragonfly/dragonfly.md) | App Definition | P2P 文件分发系统 |
| [Envoy](./graduated/envoy/envoy.md) | Service Mesh | 云原生高性能代理 |
| [etcd](./graduated/etcd/etcd.md) | Database | 分布式键值存储 |
| [Falco](./graduated/falco/falco.md) | Security | 云原生运行时安全 |
| [Fluentd](./graduated/fluentd/fluentd.md) | Observability | 统一日志层 |
| [Flux](./graduated/flux/flux.md) | App Definition | GitOps 持续交付 |
| [Harbor](./graduated/harbor/harbor.md) | Provisioning | 企业级容器镜像仓库 |
| [Helm](./graduated/helm/helm.md) | App Definition | Kubernetes 包管理器 |
| [in-toto](./graduated/in-toto/in-toto.md) | Security | 软件供应链安全框架 |
| [Istio](./graduated/istio/istio.md) | Service Mesh | 服务网格平台 |
| [Jaeger](./graduated/jaeger/jaeger.md) | Observability | 分布式追踪系统 |
| [KEDA](./graduated/keda/keda.md) | Orchestration | Kubernetes 事件驱动自动扩缩 |
| [Knative](./graduated/knative/knative.md) | Serverless | Kubernetes 无服务器平台 |
| [KubeEdge](./graduated/kubeedge/kubeedge.md) | Edge Computing | Kubernetes 边缘计算平台 |
| [Kubernetes](./graduated/kubernetes/kubernetes.md) | Orchestration | 容器编排平台 |
| [Linkerd](./graduated/linkerd/linkerd.md) | Service Mesh | 超轻量服务网格 |
| [OPA](./graduated/opa/opa.md) | Security | 开放策略代理 |
| [Prometheus](./graduated/prometheus/prometheus.md) | Observability | 监控和告警系统 |
| [Rook](./graduated/rook/rook.md) | Storage | 云原生存储编排 |
| [SPIFFE](./graduated/spiffe/spiffe.md) | Security | 安全生产身份框架 |
| [SPIRE](./graduated/spire/spire.md) | Security | SPIFFE 运行时环境 |
| [TUF](./graduated/tuf/tuf.md) | Security | 更新框架 |
| [TiKV](./graduated/tikv/tikv.md) | Database | 分布式事务键值数据库 |
| [Vitess](./graduated/vitess/vitess.md) | Database | MySQL 水平扩展集群 |

---

## Incubating 项目 (37个)

快速发展中的项目，已被多个组织在生产环境中采用。

| 项目 | 分类 | 简介 |
|:---|:---|:---|
| [Artifact Hub](./incubating/artifact-hub/artifact-hub.md) | App Definition | 云原生制品发现中心 |
| [Backstage](./incubating/backstage/backstage.md) | App Definition | 开发者门户平台 |
| [Buildpacks](./incubating/buildpacks/buildpacks.md) | App Definition | 云原生构建工具 |
| [Chaos Mesh](./incubating/chaos-mesh/chaos-mesh.md) | Observability | 混沌工程平台 |
| [Cloud Custodian](./incubating/cloud-custodian/cloud-custodian.md) | Provisioning | 云资源治理工具 |
| [CNI](./incubating/cni/cni.md) | Networking | 容器网络接口规范 |
| [Contour](./incubating/contour/contour.md) | Networking | Kubernetes Ingress 控制器 |
| [Cortex](./incubating/cortex/cortex.md) | Observability | 多租户 Prometheus 存储 |
| [Emissary-Ingress](./incubating/emissary-ingress/emissary-ingress.md) | Networking | Kubernetes 原生 API 网关 |
| [Flatcar](./incubating/flatcar/flatcar.md) | Provisioning | 容器优化 Linux 发行版 |
| [Fluid](./incubating/fluid/fluid.md) | Storage | Kubernetes 数据集编排 |
| [gRPC](./incubating/grpc/grpc.md) | RPC | 高性能 RPC 框架 |
| [Karmada](./incubating/karmada/karmada.md) | Orchestration | 多集群 Kubernetes 管理 |
| [Keycloak](./incubating/keycloak/keycloak.md) | Security | 身份和访问管理 |
| [KServe](./incubating/kserve/kserve.md) | AI/ML | Kubernetes ML 模型推理 |
| [Kubeflow](./incubating/kubeflow/kubeflow.md) | AI/ML | Kubernetes ML 平台 |
| [Kubescape](./incubating/kubescape/kubescape.md) | Security | Kubernetes 安全平台 |
| [KubeVela](./incubating/kubevela/kubevela.md) | App Definition | 应用交付平台 |
| [KubeVirt](./incubating/kubevirt/kubevirt.md) | Provisioning | Kubernetes 虚拟机管理 |
| [Kyverno](./incubating/kyverno/kyverno.md) | Security | Kubernetes 策略引擎 |
| [Lima](./incubating/lima/lima.md) | Provisioning | macOS Linux 虚拟机 |
| [Litmus](./incubating/litmus/litmus.md) | Observability | 混沌工程工具集 |
| [Longhorn](./incubating/longhorn/longhorn.md) | Storage | 云原生分布式块存储 |
| [metal3-io](./incubating/metal3-io/metal3-io.md) | Provisioning | 裸金属 Kubernetes |
| [NATS](./incubating/nats/nats.md) | Streaming | 云原生消息系统 |
| [Notary Project](./incubating/notary-project/notary-project.md) | Security | 容器签名和验证 |
| [OpenCost](./incubating/opencost/opencost.md) | Observability | Kubernetes 成本监控 |
| [OpenFeature](./incubating/openfeature/openfeature.md) | App Definition | 特性标志标准 |
| [OpenFGA](./incubating/openfga/openfga.md) | Security | 细粒度授权 |
| [OpenKruise](./incubating/openkruise/openkruise.md) | App Definition | Kubernetes 增强工作负载 |
| [OpenTelemetry](./incubating/opentelemetry/opentelemetry.md) | Observability | 可观测性框架 |
| [OpenYurt](./incubating/openyurt/openyurt.md) | Edge Computing | 边缘 Kubernetes |
| [Operator Framework](./incubating/operator-framework/operator-framework.md) | App Definition | Kubernetes Operator SDK |
| [Strimzi](./incubating/strimzi/strimzi.md) | Streaming | Kubernetes Kafka |
| [Thanos](./incubating/thanos/thanos.md) | Observability | Prometheus 高可用方案 |
| [Volcano](./incubating/volcano/volcano.md) | Orchestration | Kubernetes 批处理调度 |
| [wasmCloud](./incubating/wasmcloud/wasmcloud.md) | Serverless | WebAssembly 应用平台 |

---

## Sandbox 项目 (147个)

早期创新项目，代表云原生技术的探索方向。

### 服务网格与网络 (15个)

| 项目 | 简介 |
|:---|:---|
| [Aeraki Mesh](./sandbox/aeraki-mesh/aeraki-mesh.md) | 非 HTTP 协议服务网格 |
| [Antrea](./sandbox/antrea/antrea.md) | Kubernetes 网络方案 |
| [BFE](./sandbox/bfe/bfe.md) | 七层负载均衡器 |
| [Easegress](./sandbox/easegress/easegress.md) | 云原生流量编排 |
| [k8gb](./sandbox/k8gb/k8gb.md) | Kubernetes 全局负载均衡 |
| [Kmesh](./sandbox/kmesh/kmesh.md) | eBPF 服务网格 |
| [Kube-OVN](./sandbox/kube-ovn/kube-ovn.md) | 企业级 Kubernetes 网络 |
| [kube-vip](./sandbox/kube-vip/kube-vip.md) | Kubernetes 虚拟 IP |
| [Kuma](./sandbox/kuma/kuma.md) | 通用服务网格 |
| [LoxiLB](./sandbox/loxilb/loxilb.md) | eBPF 负载均衡 |
| [Meshery](./sandbox/meshery/meshery.md) | 服务网格管理 |
| [MetalLB](./sandbox/metallb/metallb.md) | 裸金属负载均衡 |
| [Network Service Mesh](./sandbox/network-service-mesh/network-service-mesh.md) | L2/L3 网络服务 |
| [OVN-Kubernetes](./sandbox/ovn-kubernetes/ovn-kubernetes.md) | OVN 网络方案 |
| [Submariner](./sandbox/submariner/submariner.md) | 多集群网络连接 |

### 存储 (10个)

| 项目 | 简介 |
|:---|:---|
| [Carina](./sandbox/carina/carina.md) | 本地存储方案 |
| [HwameiStor](./sandbox/hwameistor/hwameistor.md) | 高可用本地存储 |
| [OpenEBS](./sandbox/openebs/openebs.md) | 容器原生存储 |
| [Piraeus Datastore](./sandbox/piraeus-datastore/piraeus-datastore.md) | 高可用存储 |
| [Vineyard](./sandbox/vineyard/vineyard.md) | 内存数据管理 |
| [Longhorn](./sandbox/longhorn/longhorn.md) | 分布式块存储 |
| [Stacker](./sandbox/stacker/stacker.md) | OCI 镜像构建 |
| [ORAS](./sandbox/oras/oras.md) | OCI 制品注册 |
| [Distribution](./sandbox/distribution/distribution.md) | 容器镜像分发 |
| [zot](./sandbox/zot/zot.md) | OCI 镜像仓库 |

### 安全 (20个)

| 项目 | 简介 |
|:---|:---|
| [Athenz](./sandbox/athenz/athenz.md) | 角色访问控制 |
| [Cedar](./sandbox/cedar/cedar.md) | 策略语言 |
| [Confidential Containers](./sandbox/confidential-containers/confidential-containers.md) | 机密容器 |
| [Copa](./sandbox/copa/copa.md) | 容器镜像修补 |
| [Dex](./sandbox/dex/dex.md) | OIDC 身份服务 |
| [Eraser](./sandbox/eraser/eraser.md) | 镜像清理工具 |
| [Hexa](./sandbox/hexa/hexa.md) | 策略编排 |
| [Inclavare Containers](./sandbox/inclavare-containers/inclavare-containers.md) | 机密计算容器 |
| [Keylime](./sandbox/keylime/keylime.md) | 远程证明 |
| [KubeArmor](./sandbox/kubearmor/kubearmor.md) | 运行时安全 |
| [Kubewarden](./sandbox/kubewarden/kubewarden.md) | 策略引擎 |
| [OAuth2 Proxy](./sandbox/oauth2-proxy/oauth2-proxy.md) | OAuth2 代理 |
| [Open Policy Containers](./sandbox/open-policy-containers/open-policy-containers.md) | OPA 容器 |
| [Paralus](./sandbox/paralus/paralus.md) | Kubernetes 访问管理 |
| [Parsec](./sandbox/parsec/parsec.md) | 安全服务 API |
| [Ratify](./sandbox/ratify/ratify.md) | 制品验证 |
| [SOPS](./sandbox/sops/sops.md) | 密钥管理 |
| [Tokenetes](./sandbox/tokenetes/tokenetes.md) | 服务账户令牌 |
| [external-secrets](./sandbox/external-secrets/external-secrets.md) | 外部密钥同步 |
| [Bank-Vaults](./sandbox/bank-vaults/bank-vaults.md) | Vault 工具集 |

### Kubernetes 发行版与管理 (25个)

| 项目 | 简介 |
|:---|:---|
| [k0s](./sandbox/k0s/k0s.md) | 零摩擦 Kubernetes |
| [k3s](./sandbox/k3s/k3s.md) | 轻量级 Kubernetes |
| [Kairos](./sandbox/kairos/kairos.md) | 不可变 Kubernetes |
| [Kubean](./sandbox/kubean/kubean.md) | 集群生命周期管理 |
| [KubeClipper](./sandbox/kubeclipper/kubeclipper.md) | 集群管理平台 |
| [kcp](./sandbox/kcp/kcp.md) | Kubernetes 控制平面 |
| [Capsule](./sandbox/capsule/capsule.md) | 多租户管理 |
| [Clusternet](./sandbox/clusternet/clusternet.md) | 多集群管理 |
| [Clusterpedia](./sandbox/clusterpedia/clusterpedia.md) | 多集群资源查询 |
| [KubeFleet](./sandbox/kubefleet/kubefleet.md) | 多集群编排 |
| [KubeSlice](./sandbox/kubeslice/kubeslice.md) | 多集群连接 |
| [KubeStellar](./sandbox/kubestellar/kubestellar.md) | 多集群配置 |
| [Open Cluster Management](./sandbox/open-cluster-management/open-cluster-management.md) | 多集群管理 |
| [Kured](./sandbox/kured/kured.md) | 节点重启守护进程 |
| [Virtual Kubelet](./sandbox/virtual-kubelet/virtual-kubelet.md) | 虚拟 Kubelet |
| [Interlink](./sandbox/interlink/interlink.md) | 远程执行 |
| [Kuasar](./sandbox/kuasar/kuasar.md) | 容器运行时 |
| [youki](./sandbox/youki/youki.md) | 容器运行时 |
| [urunc](./sandbox/urunc/urunc.md) | Unikernel 运行时 |
| [Hyperlight](./sandbox/hyperlight/hyperlight.md) | 微虚拟机 |
| [bootc](./sandbox/bootc/bootc.md) | 可启动容器 |
| [composefs](./sandbox/composefs/composefs.md) | 组合文件系统 |
| [container2wasm](./sandbox/container2wasm/container2wasm.md) | 容器转 WASM |
| [ContainerSSH](./sandbox/containerssh/containerssh.md) | SSH 容器服务 |
| [Podman Container Tools](./sandbox/podman-container-tools/podman-container-tools.md) | Podman 工具集 |

### 应用定义与交付 (25个)

| 项目 | 简介 |
|:---|:---|
| [Carvel](./sandbox/carvel/carvel.md) | 应用构建工具集 |
| [CDK8s](./sandbox/cdk8s/cdk8s.md) | 代码定义 Kubernetes |
| [Devfile](./sandbox/devfile/devfile.md) | 开发环境规范 |
| [DevSpace](./sandbox/devspace/devspace.md) | 开发工作流 |
| [Headlamp](./sandbox/headlamp/headlamp.md) | Kubernetes Dashboard |
| [KCL](./sandbox/kcl/kcl.md) | 配置语言 |
| [ko](./sandbox/ko/ko.md) | Go 容器构建 |
| [Konveyor](./sandbox/konveyor/konveyor.md) | 应用现代化 |
| [kpt](./sandbox/kpt/kpt.md) | 配置包管理 |
| [KUDO](./sandbox/kudo/kudo.md) | Operator 工具包 |
| [KusionStack](./sandbox/kusionstack/kusionstack.md) | 平台工程套件 |
| [OpenGitOps](./sandbox/opengitops/opengitops.md) | GitOps 规范 |
| [OpenTofu](./sandbox/opentofu/opentofu.md) | IaC 工具 |
| [Porter](./sandbox/porter/porter.md) | CNAB 打包工具 |
| [Radius](./sandbox/radius/radius.md) | 应用平台 |
| [SchemaHero](./sandbox/schemahero/schemahero.md) | 数据库 Schema 管理 |
| [Score](./sandbox/score/score.md) | 工作负载规范 |
| [Shipwright](./sandbox/shipwright/shipwright.md) | 容器构建框架 |
| [Spin](./sandbox/spin/spin.md) | WASM 微服务框架 |
| [SpinKube](./sandbox/spinkube/spinkube.md) | Kubernetes WASM |
| [werf](./sandbox/werf/werf.md) | GitOps 交付工具 |
| [Atlantis](./sandbox/atlantis/atlantis.md) | Terraform PR 自动化 |
| [PipeCD](./sandbox/pipecd/pipecd.md) | GitOps CD 平台 |
| [Dalec](./sandbox/dalec/dalec.md) | 构建规范 |
| [KitOps](./sandbox/kitops/kitops.md) | ML 模型打包 |

### 可观测性 (15个)

| 项目 | 简介 |
|:---|:---|
| [Inspektor Gadget](./sandbox/inspektor-gadget/inspektor-gadget.md) | eBPF 调试工具 |
| [Kepler](./sandbox/kepler/kepler.md) | 能耗监控 |
| [Perses](./sandbox/perses/perses.md) | 仪表盘即代码 |
| [Pixie](./sandbox/pixie/pixie.md) | Kubernetes 可观测性 |
| [Trickster](./sandbox/trickster/trickster.md) | 时序数据库代理 |
| [Kube-burner](./sandbox/kube-burner/kube-burner.md) | 压力测试工具 |
| [Kuberhealthy](./sandbox/kuberhealthy/kuberhealthy.md) | 健康检查框架 |
| [Cartography](./sandbox/cartography/cartography.md) | 基础设施图谱 |
| [Drasi](./sandbox/drasi/drasi.md) | 变更检测 |
| [Tremor](./sandbox/tremor/tremor.md) | 事件处理 |
| [openGemini](./sandbox/opengemini/opengemini.md) | 时序数据库 |
| [Oxia](./sandbox/oxia/oxia.md) | 元数据存储 |
| [Logging Operator](./sandbox/logging-operator/logging-operator.md) | 日志管理 |
| [bpfman](./sandbox/bpfman/bpfman.md) | eBPF 管理 |
| [HolmesGPT](./sandbox/holmesgpt/holmesgpt.md) | AI 故障诊断 |

### AI/ML 与 GPU (10个)

| 项目 | 简介 |
|:---|:---|
| [KAITO](./sandbox/kaito/kaito.md) | AI 模型推理 |
| [K8sGPT](./sandbox/k8sgpt/k8sgpt.md) | AI Kubernetes 助手 |
| [kagent](./sandbox/kagent/kagent.md) | AI 代理 |
| [ModelPack](./sandbox/modelpack/modelpack.md) | ML 模型打包 |
| [hami](./sandbox/hami/hami.md) | GPU 虚拟化 |
| [Koordinator](./sandbox/koordinator/koordinator.md) | 混合编排 |
| [KubeElasti](./sandbox/kubeelasti/kubeelasti.md) | AI 弹性扩缩 |
| [Armada](./sandbox/armada/armada.md) | 多集群批处理 |
| [Cadence](./sandbox/cadence/cadence.md) | 工作流引擎 |
| [Serverless Workflow](./sandbox/serverless-workflow/serverless-workflow.md) | 工作流规范 |

### 边缘计算与 IoT (5个)

| 项目 | 简介 |
|:---|:---|
| [Akri](./sandbox/akri/akri.md) | 边缘设备发现 |
| [Kgateway](./sandbox/kgateway/kgateway.md) | Kubernetes API 网关 |
| [Tinkerbell](./sandbox/tinkerbell/tinkerbell.md) | 裸金属配置 |
| [WasmEdge](./sandbox/wasmedge/wasmedge.md) | WebAssembly 运行时 |
| [Serverless Devs](./sandbox/serverless-devs/serverless-devs.md) | 无服务器开发工具 |

### 混沌工程与测试 (5个)

| 项目 | 简介 |
|:---|:---|
| [Chaosblade](./sandbox/chaosblade/chaosblade.md) | 混沌实验工具 |
| [Krkn](./sandbox/krkn/krkn.md) | 混沌测试 |
| [Microcks](./sandbox/microcks/microcks.md) | API 模拟测试 |
| [Runme Notebooks](./sandbox/runme-notebooks/runme-notebooks.md) | 可执行文档 |
| [Telepresence](./sandbox/telepresence/telepresence.md) | 本地开发调试 |

### 数据库与中间件 (8个)

| 项目 | 简介 |
|:---|:---|
| [CloudNativePG](./sandbox/cloudnativepg/cloudnativepg.md) | PostgreSQL Operator |
| [Kanister](./sandbox/kanister/kanister.md) | 数据管理 |
| [Sermant](./sandbox/sermant/sermant.md) | 无代理服务网格 |
| [K8up](./sandbox/k8up/k8up.md) | 备份 Operator |
| [SlimFaaS](./sandbox/slimfaas/slimfaas.md) | 轻量 FaaS |
| [SlimToolkit](./sandbox/slimtoolkit/slimtoolkit.md) | 容器瘦身 |
| [Connect RPC](./sandbox/connect-rpc/connect-rpc.md) | gRPC 兼容 RPC |
| [xRegistry](./sandbox/xregistry/xregistry.md) | 注册中心规范 |

### 其他工具 (9个)

| 项目 | 简介 |
|:---|:---|
| [CoHDI](./sandbox/cohdi/cohdi.md) | 硬件设备接口 |
| [Cozystack](./sandbox/cozystack/cozystack.md) | PaaS 平台 |
| [kube-rs](./sandbox/kube-rs/kube-rs.md) | Rust Kubernetes 客户端 |
| [Kuadrant](./sandbox/kuadrant/kuadrant.md) | API 管理 |
| [OpenChoreo](./sandbox/openchoreo/openchoreo.md) | 集成平台 |
| [OpenFunction](./sandbox/openfunction/openfunction.md) | FaaS 平台 |
| [OSCAL-COMPASS](./sandbox/oscal-compass/oscal-compass.md) | 合规自动化 |
| [Podman Desktop](./sandbox/podman-desktop/podman-desktop.md) | 容器桌面应用 |
| [VS Code Kubernetes Tools](./sandbox/vscode-kubernetes-tools/vscode-kubernetes-tools.md) | VS Code 插件 |

---

## 学习路径建议

### 云原生入门路径
```
Kubernetes → containerd → Helm → Prometheus → CoreDNS
```

### 服务网格路径
```
Envoy → Istio → Linkerd → Cilium
```

### 安全合规路径
```
OPA → Falco → SPIFFE/SPIRE → in-toto → cert-manager
```

### 可观测性路径
```
Prometheus → Jaeger → Fluentd → OpenTelemetry → Thanos
```

### GitOps 路径
```
Flux → Argo → Helm → Crossplane
```

---

## 相关领域

- **[Domain-8: 可观测性](../domain-8-observability)** - Prometheus、日志、追踪深度实践
- **[Domain-10: 扩展生态](../domain-10-extensions)** - Helm、Operator、GitOps 详解
- **[Domain-25: 云原生安全](../domain-25-cloud-native-security)** - 安全策略与合规实践
- **[Domain-26: 服务网格](../domain-26-service-mesh-microservices)** - Istio、Linkerd 深度分析

---

## 参考资源

- [CNCF Landscape](https://landscape.cncf.io/)
- [CNCF Projects](https://www.cncf.io/projects/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)
- [CNCF Annual Report](https://www.cncf.io/reports/)

---

**维护者**: Kudig Team | **许可证**: MIT | **最后更新**: 2026-03
