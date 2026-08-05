---
title: Java Kubernetes Client 与 Operator SDK 开发指南
description: 'title: Java Kubernetes Client 与 Operator SDK 开发指南'
summary: 'title: Java Kubernetes Client 与 Operator SDK 开发指南'
category: general
tags:
- k8s
- devops
- daily-ops
- guide
- docker
- statefulset
- ingress
- rbac
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Operator是什么？
- 如何使用Operator？
- Operator的最佳实践是什么？
trigger_keywords:
- Java
- Kubernetes
- Client
- Operator
- SDK
- 开发指南
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Java [[kubernetes|Kubernetes]] Client 与 Operator SDK 开发指南
description: '# Java Kubernetes Client 与 Operator SDK 开发指南'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- docker
- [[statefulset|statefulset]]
- [[ingress|ingress]]
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Java Kubernetes Client 与 Operator SDK 开发指南 是什么
- 如何 Java Kubernetes Client 与 Operator SDK 开发指南
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- Java
- Kubernetes
- Client
- Operator
- SDK
- 开发指南
- platform
- ops
cross_refs:
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Java Kubernetes Client 与 Operator SDK 开发指南

> **适用版本**: Kubernetes Java Client 20+ / Java Operator SDK 4.9+ / Quarkus Operator SDK 6.8+  
> **最后更新**: 2026-04-30  
> **难度**: 高级

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、Java K8s Client 生态全景](#一java-k8s-client-生态全景)
- [二、fabric8 Kubernetes Client](#二fabric8-kubernetes-client)
- [三、官方 Kubernetes Java Client](#三官方-kubernetes-java-client)
- [四、Java Operator SDK (JOSDK)](#四java-operator-sdk-josdk)
- [五、Quarkus Operator SDK](#五quarkus-operator-sdk)
- [六、Informer 与 List-Watch 模式](#六informer-与-list-watch-模式)
- [七、Leader Election](#七leader-election)
- [八、CRD 定义与 Status 管理](#八crd-定义与-status-管理)
- [九、测试策略](#九测试策略)
- [十、生产部署最佳实践](#十生产部署最佳实践)

---

<!-- chunk: 一、Java K8s Client 生态全景 -->
## 一、Java K8s Client 生态全景

```mermaid
graph TD
    A[Java K8s 开发] --> B{场景}
    B -->|应用集成| C[Kubernetes Client]
    B -->|Operator 开发| D[Operator SDK]

    C --> C1[fabric8/kubernetes-client<br/>DSL 风格, 易用]
    C --> C2[kubernetes-client/java<br/>官方维护, 完整 API]

    D --> D1[Java Operator SDK (JOSDK)<br/>轻量, Spring Boot 友好]
    D --> D2[Quarkus Operator SDK<br/>原生编译, 低内存]

    style A fill:#22c55e,stroke:#166534,color:#fff
    style D fill:#326ce5,stroke:#1a3a8f,color:#fff
```

### 1.1 Client 对比

| 特性 | fabric8 | kubernetes-client/java |
|------|---------|----------------------|
| **维护方** | Fabric8 社区 | Kubernetes 官方 |
| **API 风格** | DSL (流式) | Builder 模式 |
| **类型安全** | 强 | 强 |
| **学习曲线** | 低 | 中 |
| **扩展性** | 高 | 高 |
| **社区活跃度** | 高 | 极高 |
| **推荐场景** | 日常 K8s 交互 | 复杂 API 操作 |

---

<!-- chunk: 二、fabric8 Kubernetes Client -->
## 二、fabric8 Kubernetes Client

### 2.1 依赖

```xml
<dependency>
    <groupId>io.fabric8</groupId>
    <artifactId>kubernetes-client</artifactId>
    <version>6.13.4</version>
</dependency>
```

### 2.2 基本使用

```java
@Configuration
public class KubernetesConfig {

    @Bean
    public KubernetesClient kubernetesClient() {
        Config config = ConfigBuilder.empty()
            .withMasterUrl("https://kubernetes.default.svc")
            .withNamespace("default")
            .build();
        return new KubernetesClientBuilder().withConfig(config).build();
    }
}

@Service
public class PodService {
    private final KubernetesClient client;

    public List<String> listPods(String namespace) {
        return client.pods()
            .inNamespace(namespace)
            .list()
            .getItems()
            .stream()
            .map(pod -> pod.getMetadata().getName())
            .toList();
    }

    public Pod getPod(String namespace, String name) {
        return client.pods()
            .inNamespace(namespace)
            .withName(name)
            .get();
    }

    public Pod createPod(String namespace, Pod pod) {
        return client.pods()
            .inNamespace(namespace)
            .resource(pod)
            .create();
    }

    public Pod patchPod(String namespace, String name, Pod pod) {
        return client.pods()
            .inNamespace(namespace)
            .withName(name)
            .patch(pod);
    }

    public Boolean deletePod(String namespace, String name) {
        return client.pods()
            .inNamespace(namespace)
            .withName(name)
            .delete();
    }
}
```

### 2.3 List-Watch 示例

```java
@Service
public class PodWatcher {

    public void watchPods(String namespace) {
        client.pods()
            .inNamespace(namespace)
            .watch(new Watcher<Pod>() {
                @Override
                public void eventReceived(Action action, Pod pod) {
                    String name = pod.getMetadata().getName();
                    switch (action) {
                        case ADDED -> log.info("Pod added: {}", name);
                        case MODIFIED -> log.info("Pod modified: {}", name);
                        case DELETED -> log.info("Pod deleted: {}", name);
                        case ERROR -> log.error("Pod error: {}", name);
                    }
                }

                @Override
                public void onClose(WatcherException cause) {
                    log.warn("Watch closed", cause);
                }
            });
    }
}
```

### 2.4 事务式操作

```java
client.apps().deployments()
    .inNamespace("production")
    .withName("spring-app")
    .edit(deployment -> new DeploymentBuilder(deployment)
        .editSpec()
            .editTemplate()
                .editSpec()
                    .editContainer(0)
                        .withImage("registry.example.com/spring-app:v2.0.0")
                    .endContainer()
                .endSpec()
            .endTemplate()
        .endSpec()
        .build());
```

---

<!-- chunk: 三、官方 Kubernetes Java Client -->
## 三、官方 Kubernetes Java Client

### 3.1 依赖

```xml
<dependency>
    <groupId>io.kubernetes</groupId>
    <artifactId>client-java</artifactId>
    <version>20.0.1</version>
</dependency>
```

### 3.2 基本使用

```java
@Configuration
public class K8sConfig {

    @Bean
    public ApiClient apiClient() throws IOException {
        ApiClient client = ClientBuilder.cluster().build();
        client.setHttpClient(
            client.getHttpClient().newBuilder()
                .readTimeout(Duration.ofSeconds(30))
                .writeTimeout(Duration.ofSeconds(30))
                .build()
        );
        Configuration.setDefaultApiClient(client);
        return client;
    }
}

@Service
public class DeploymentService {
    private final AppsV1Api api;

    public V1DeploymentList listDeployments(String namespace) throws ApiException {
        return api.listNamespacedDeployment(namespace)
            .execute();
    }

    public V1Deployment patchDeployment(String namespace, String name, V1Deployment body) throws ApiException {
        return api.patchNamespacedDeployment(name, namespace, body)
            .fieldManager("java-operator")
            .force(true)
            .execute();
    }
}
```

---

<!-- chunk: 四、Java Operator SDK (JOSDK) -->
## 四、Java Operator SDK (JOSDK)

### 4.1 依赖

```xml
<dependency>
    <groupId>io.javaoperatorsdk</groupId>
    <artifactId>operator-framework</artifactId>
    <version>4.9.5</version>
</dependency>
```

### 4.2 CRD 定义

```java
@Group("apps.example.com")
@Version("v1alpha1")
public class WebApplication extends CustomResource<WebApplicationSpec, WebApplicationStatus>
    implements Namespaced {}

public class WebApplicationSpec {
    private String image;
    private int replicas;
    private String host;
    private Map<String, String> env;

    public String getImage() { return image; }
    public void setImage(String image) { this.image = image; }
    public int getReplicas() { return replicas; }
    public void setReplicas(int replicas) { this.replicas = replicas; }
    public String getHost() { return host; }
    public void setHost(String host) { this.host = host; }
    public Map<String, String> getEnv() { return env; }
    public void setEnv(Map<String, String> env) { this.env = env; }
}

public class WebApplicationStatus {
    private boolean ready;
    private String url;
    private List<String> conditions;

    public boolean isReady() { return ready; }
    public void setReady(boolean ready) { this.ready = ready; }
    public String getUrl() { return url; }
    public void setUrl(String url) { this.url = url; }
    public List<String> getConditions() { return conditions; }
    public void setConditions(List<String> conditions) { this.conditions = conditions; }
}
```

### 4.3 Reconciler 实现

```java
@ControllerConfiguration
public class WebApplicationReconciler
        implements Reconciler<WebApplication>,
                   ContextInitializer<WebApplication> {

    private final KubernetesClient client;

    @Override
    public UpdateControl<WebApplication> reconcile(WebApplication resource, Context<WebApplication> context) {
        String name = resource.getMetadata().getName();
        String namespace = resource.getMetadata().getNamespace();
        WebApplicationSpec spec = resource.getSpec();

        try {
            ensureDeployment(resource, spec);
            ensureService(resource, spec);
            ensureIngress(resource, spec);

            WebApplicationStatus status = new WebApplicationStatus();
            status.setReady(true);
            status.setUrl("https://" + spec.getHost());
            resource.setStatus(status);

            return UpdateControl.updateStatus(resource);

        } catch (Exception e) {
            WebApplicationStatus status = new WebApplicationStatus();
            status.setReady(false);
            status.setConditions(List.of("Error: " + e.getMessage()));
            resource.setStatus(status);
            return UpdateControl.updateStatus(resource)
                .rescheduleAfter(Duration.ofSeconds(30));
        }
    }

    private void ensureDeployment(WebApplication resource, WebApplicationSpec spec) {
        Deployment deployment = new DeploymentBuilder()
            .withNewMetadata()
                .withName(resource.getMetadata().getName())
                .withNamespace(resource.getMetadata().getNamespace())
                .addToLabels("app", resource.getMetadata().getName())
                .addToLabels("managed-by", "webapp-operator")
                .addNewOwnerReference()
                    .withApiVersion(resource.getApiVersion())
                    .withKind(resource.getKind())
                    .withName(resource.getMetadata().getName())
                    .withUid(resource.getMetadata().getUid())
                .endOwnerReference()
            .endMetadata()
            .withNewSpec()
                .withReplicas(spec.getReplicas())
                .withNewSelector()
                    .addToMatchLabels("app", resource.getMetadata().getName())
                .endSelector()
                .withNewTemplate()
                    .withNewMetadata()
                        .addToLabels("app", resource.getMetadata().getName())
                    .endMetadata()
                    .withNewSpec()
                        .addNewContainer()
                            .withName("app")
                            .withImage(spec.getImage())
                            .addNewPort()
                                .withContainerPort(8080)
                            .endPort()
                            .withNewResources()
                                .addToRequests("memory", new Quantity("256Mi"))
                                .addToRequests("cpu", new Quantity("100m"))
                                .addToLimits("memory", new Quantity("512Mi"))
                                .addToLimits("cpu", new Quantity("500m"))
                            .endResources()
                        .endContainer()
                    .endSpec()
                .endTemplate()
            .endSpec()
            .build();

        client.apps().deployments()
            .inNamespace(resource.getMetadata().getNamespace())
            .resource(deployment)
            .serverSideApply();
    }

    @Override
    public void initContext(WebApplication resource, Context<WebApplication> context) {
    }
}
```

### 4.4 Operator 启动

```java
@SpringBootApplication
public class OperatorApplication {
    public static void main(String[] args) {
        SpringApplication.run(OperatorApplication.class, args);
    }

    @Bean
    public Operator operator(KubernetesClient client) {
        Operator operator = new Operator(client);
        operator.register(new WebApplicationReconciler(client));
        return operator;
    }
}
```

---

<!-- chunk: 五、Quarkus Operator SDK -->
## 五、Quarkus Operator SDK

### 5.1 依赖

```xml
<dependency>
    <groupId>io.quarkiverse.operatorsdk</groupId>
    <artifactId>quarkus-operator-sdk</artifactId>
    <version>6.8.4</version>
</dependency>
```

### 5.2 Quarkus Reconciler

```java
@ControllerConfiguration(namespaces = Constants.WATCH_CURRENT_NAMESPACE)
@ApplicationScoped
public class DatabaseReconciler implements Reconciler<Database> {

    @Inject
    KubernetesClient client;

    @Override
    public UpdateControl<Database> reconcile(Database database, Context<Database> context) {
        ensureStatefulSet(database);
        ensureService(database);
        updateStatus(database);
        return UpdateControl.updateStatus(database);
    }

    @Override
    public DeleteControl cleanup(Database database, Context<Database> context) {
        return DeleteControl.defaultDelete();
    }
}
```

### 5.3 Native 编译

```bash
# 构建原生 Operator
./mvnw package -Dnative \
    -Dquarkus.native.container-build=true \
    -Dquarkus.container-image.build=true \
    -Dquarkus.container-image.push=true

# 内存占用: ~30MB (vs JVM 模式 ~200MB)
```

---

<!-- chunk: 六、Informer 与 List-Watch 模式 -->
## 六、Informer 与 List-Watch 模式

### 6.1 SharedInformerFactory

```java
@Configuration
public class InformerConfig {

    @Bean
    public SharedInformerFactory sharedInformerFactory(KubernetesClient client) {
        SharedInformerFactory factory = client.informers();

        factory.sharedIndexInformerFor(
            Pod.class,
            PodList.class,
            Duration.ofMinutes(5).toMillis()
        );

        return factory;
    }
}

@Component
public class PodInformer {

    @PostConstruct
    public void startWatching() {
        Indexer<Pod> indexer = informerFactory.sharedIndexInformerFor(
            Pod.class, PodList.class, 300000L
        ).getIndexer();

        informerFactory.addSharedInformerEventListener(event -> {
            log.info("Informer event: {}", event);
        });
    }
}
```

---

<!-- chunk: 七、Leader Election -->
## 七、Leader Election

### 7.1 JOSDK Leader Election

```java
@ControllerConfiguration
@LeaderElectionConfiguration(
    leaseDuration = "PT30S",
    renewalDeadline = "PT15S",
    retryPeriod = "PT5S"
)
public class MyReconciler implements Reconciler<MyResource> {
    @Override
    public UpdateControl<MyResource> reconcile(MyResource resource, Context<MyResource> context) {
        if (!context.eventSourceRetriever().isLeader()) {
            return UpdateControl.noUpdate();
        }
        return doReconcile(resource);
    }
}
```

### 7.2 K8s Deployment 配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-operator
spec:
  replicas: 3
  template:
    spec:
      serviceAccountName: my-operator-sa
      containers:
        - name: operator
          image: registry.example.com/my-operator:v1.0.0
          env:
            - name: JAVA_OPTS
              value: "-XX:+UseContainerSupport -XX:MaxRAMPercentage=75.0"
          resources:
            requests: { memory: "256Mi", cpu: "100m" }
            limits: { memory: "512Mi", cpu: "500m" }
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-operator-sa
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: my-operator-role
rules:
  - apiGroups: ["apps.example.com"]
    resources: ["webapplications", "webapplications/status", "webapplications/finalizers"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  - apiGroups: [""]
    resources: ["services", "configmaps", "secrets"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  - apiGroups: ["networking.k8s.io"]
    resources: ["ingresses"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  - apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: my-operator-binding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: my-operator-role
subjects:
  - kind: ServiceAccount
    name: my-operator-sa
```

---

<!-- chunk: 八、CRD 定义与 Status 管理 -->
## 八、CRD 定义与 Status 管理

### 8.1 CRD 自动生成

```bash
# JOSDK 自动生成 CRD YAML
./mvnw k8s:generate-crd

# Quarkus 自动生成
./mvnw quarkus:generate-crd
```

### 8.2 CRD 安装

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: webapplications.apps.example.com
spec:
  group: apps.example.com
  names:
    kind: WebApplication
    listKind: WebApplicationList
    plural: webapplications
    singular: webapplication
    shortNames:
      - webapp
  scope: Namespaced
  versions:
    - name: v1alpha1
      served: true
      storage: true
      subresources:
        status: {}
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                image:
                  type: string
                replicas:
                  type: integer
                  minimum: 1
                  maximum: 100
                host:
                  type: string
              required:
                - image
                - replicas
                - host
            status:
              type: object
              properties:
                ready:
                  type: boolean
                url:
                  type: string
                conditions:
                  type: array
                  items:
                    type: string
```

---

<!-- chunk: 九、测试策略 -->
## 九、测试策略

### 9.1 单元测试

```java
@ExtendWith(MockitoExtension.class)
class WebApplicationReconcilerTest {

    @Mock
    KubernetesClient client;

    @Mock
    MixedOperation<Deployment, DeploymentList, Resource<Deployment>> deploymentOp;

    WebApplicationReconciler reconciler;

    @BeforeEach
    void setup() {
        reconciler = new WebApplicationReconciler(client);
        when(client.apps().deployments()).thenReturn(deploymentOp);
        when(deploymentOp.inNamespace(anyString())).thenReturn(deploymentOp);
        when(deploymentOp.resource(any())).thenReturn(deploymentOp);
        when(deploymentOp.serverSideApply()).thenReturn(null);
    }

    @Test
    void shouldCreateDeployment() {
        WebApplication resource = createTestResource();
        UpdateControl<WebApplication> result = reconciler.reconcile(resource, mock(Context.class));
        assertTrue(result.getResource().getStatus().isReady());
    }
}
```

### 9.2 集成测试 (JOSDK)

```java
@Testcontainers
class OperatorIntegrationTest {

    @Container
    static K3sContainer k3s = new K3sContainer(
        DockerImageName.parse("rancher/k3s:v1.30.0-k3s1")
    );

    @Test
    void shouldReconcileWebApplication() {
        Config config = new ConfigBuilder()
            .withMasterUrl(k3s.getHttpsUrl())
            .withCaCertData(k3s.getCaCert())
            .build();

        try (KubernetesClient client = new KubernetesClientBuilder()
                .withConfig(config).build()) {
            Operator operator = new Operator(client);
            operator.register(new WebApplicationReconciler(client));
            operator.start();

            WebApplication webApp = createTestResource();
            client.resource(webApp).create();

            await().atMost(30, TimeUnit.SECONDS)
                .untilAsserted(() -> {
                    WebApplication updated = client.resource(webApp).get();
                    assertTrue(updated.getStatus().isReady());
                });
        }
    }
}
```

---

<!-- chunk: 十、生产部署最佳实践 -->
## 十、生产部署最佳实践

| 检查项 | 配置 | 说明 |
|--------|------|------|
| Leader Election | `@LeaderElectionConfiguration` | 多副本仅活跃一个 |
| RBAC 最小权限 | 限定 resources + verbs | 不使用 `*:*` |
| 优雅关闭 | `preStop: sleep 10` | 确保当前 Reconcile 完成 |
| 资源限制 | memory 256-512Mi | Operator 通常轻量 |
| 日志结构化 | JSON 格式 | 便于日志平台检索 |
| 健康检查 | `/health/live`, `/health/ready` | Operator 可用性 |
| CRD 校验 | OpenAPI v3 Schema | 防止非法 CRD 输入 |
| Event 记录 | `client.events().create()` | K8s Event 审计 |
| Finalizer | 实现 `Cleaner` 接口 | 资源清理 |

---

<!-- chunk: 🔗 相关文档 -->
## 🔗 相关文档

- [CRD 开发指南](../../16-%E4%B8%93%E9%A1%B9%E6%8A%80%E6%9C%AF/03-%E6%89%A9%E5%B1%95%E6%9C%BA%E5%88%B6/01-crd-development-guide.md) — CRD 基础概念
- [Operator 开发模式](10-crd-operator-development.md) — Operator 设计模式
- [Java 容器化](../../14-%E5%AE%B9%E5%99%A8%E8%BF%90%E8%A1%8C%E6%97%B6/01-Docker/12-java-containerization-guide.md) — Operator 容器化
- [Java 安全](../../08-安全/06-合规审计/14-java-security-kubernetes-guide.md) — RBAC 安全

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 MOC
- [[10-平台工程/README.md|Platform Ops Domain (平台运维领域)]]
- Domain-9 平台运维 — 开源项目索引
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## Related

- 12-demo-env-guide
- 21-platform-selection-guide

## See Also

- 25-virtual-clusters
- 26-kubectl-plugin-ecosystem
- 99-kubernetes-v1.33-platform-ops-guide
- 01-platform-ops-overview


<!-- risk-assessed -->
