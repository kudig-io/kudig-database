---
title: Operator 测试策略
description: envtest 单元测试、集成测试与 e2e 测试框架
summary: 使用 envtest 进行控制器集成测试、ginkgo/gomega BDD 测试及生产级 e2e 测试策略
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- testing
- envtest
- e2e
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 开发工程师
- 平台工程师
estimated_read_time: 12min
intent_queries:
- Operator 如何测试
- envtest 集成测试
- controller-runtime testing
trigger_keywords:
- envtest
- testing
- ginkgo
- e2e
- integration-test
prerequisites:
- operator-basics
- golang-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator 测试策略

## 1. 测试金字塔

```
         /\
        /e2e\          ← 慢、真实集群、少量（端到端验证）
       /------\
      /integration\    ← 中速、envtest 假 API Server、中等
     /--------------\
    /  unit tests   \  ← 快、纯逻辑、大量
   /------------------\
```

## 2. envtest 集成测试

envtest 启动真实的 `etcd` + `kube-apiserver`（无 controller-manager），允许测试控制器的真实行为：

```go
package controller_test

import (
    "context"
    "path/filepath"
    "testing"
    "time"

    . "github.com/onsi/ginkgo/v2"
    . "github.com/onsi/gomega"
    "k8s.io/client-go/kubernetes/scheme"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/envtest"
    logf "sigs.k8s.io/controller-runtime/pkg/log"
    "sigs.k8s.io/controller-runtime/pkg/log/zap"
)

var (
    testEnv   *envtest.Environment
    k8sClient client.Client
    ctx       context.Context
    cancel    context.CancelFunc
)

var _ = BeforeSuite(func() {
    logf.SetLogger(zap.New(zap.UseDevMode(true)))
    ctx, cancel = context.WithCancel(context.Background())

    testEnv = &envtest.Environment{
        CRDDirectoryPaths:     []string{filepath.Join("..", "config", "crd")},
        ErrorIfCRDPathMissing: true,
    }

    cfg, err := testEnv.Start()
    Expect(err).NotTo(HaveOccurred())

    err = platformv1.AddToScheme(scheme.Scheme)
    Expect(err).NotTo(HaveOccurred())

    k8sClient, err = client.New(cfg, client.Options{Scheme: scheme.Scheme})
    Expect(err).NotTo(HaveOccurred())

    k8sManager, err := ctrl.NewManager(cfg, ctrl.Options{Scheme: scheme.Scheme})
    Expect(err).NotTo(HaveOccurred())

    err = (&WebAppReconciler{
        Client: k8sManager.GetClient(),
        Scheme: k8sManager.GetScheme(),
    }).SetupWithManager(k8sManager)
    Expect(err).NotTo(HaveOccurred())

    go func() {
        defer GinkgoRecover()
        err = k8sManager.Start(ctx)
        Expect(err).NotTo(HaveOccurred())
    }()
})

var _ = AfterSuite(func() {
    cancel()
    Expect(testEnv.Stop()).To(Succeed())
})
```

## 3. 控制器测试用例

```go
var _ = Describe("WebApp Controller", func() {
    Context("当创建 WebApp CR", func() {
        It("应该创建对应的 Deployment", func() {
            By("创建 WebApp 资源")
            webapp := &platformv1.WebApp{
                ObjectMeta: metav1.ObjectMeta{
                    Name:      "test-app",
                    Namespace: "default",
                },
                Spec: platformv1.WebAppSpec{
                    Image:    "nginx:1.25",
                    Replicas: 3,
                },
            }
            Expect(k8sClient.Create(ctx, webapp)).To(Succeed())

            By("等待 Deployment 被创建")
            Eventually(func(g Gomega) {
                deploy := &appsv1.Deployment{}
                g.Expect(k8sClient.Get(ctx, client.ObjectKey{
                    Name: "test-app", Namespace: "default",
                }, deploy)).To(Succeed())
                g.Expect(*deploy.Spec.Replicas).To(Equal(int32(3)))
                g.Expect(deploy.Spec.Template.Spec.Containers[0].Image).To(Equal("nginx:1.25"))
            }, 10*time.Second, 1*time.Second).Should(Succeed())
        })

        It("更新 replicas 后应同步到 Deployment", func() {
            By("更新 WebApp spec")
            webapp := &platformv1.WebApp{}
            Expect(k8sClient.Get(ctx, client.ObjectKey{
                Name: "test-app", Namespace: "default",
            }, webapp)).To(Succeed())

            webapp.Spec.Replicas = 5
            Expect(k8sClient.Update(ctx, webapp)).To(Succeed())

            By("验证 Deployment 副本数更新")
            Eventually(func(g Gomega) int32 {
                deploy := &appsv1.Deployment{}
                g.Expect(k8sClient.Get(ctx, client.ObjectKey{
                    Name: "test-app", Namespace: "default",
                }, deploy)).To(Succeed())
                return *deploy.Spec.Replicas
            }, 10*time.Second, 1*time.Second).Should(Equal(int32(5)))
        })
    })
})
```

## 4. Fake Client 单元测试

```go
func TestReconcileLogic(t *testing.T) {
    // 使用 fake client 进行纯逻辑测试（无真实 API Server）
    scheme := runtime.NewScheme()
    _ = platformv1.AddToScheme(scheme)
    _ = appsv1.AddToScheme(scheme)

    cl := fake.NewClientBuilder().
        WithScheme(scheme).
        WithObjects(&platformv1.WebApp{
            ObjectMeta: metav1.ObjectMeta{Name: "my-app", Namespace: "default"},
            Spec:       platformv1.WebAppSpec{Image: "nginx:1.25", Replicas: 2},
        }).
        Build()

    r := &WebAppReconciler{Client: cl, Scheme: scheme}
    _, err := r.Reconcile(context.Background(), ctrl.Request{
        NamespacedName: types.NamespacedName{Name: "my-app", Namespace: "default"},
    })
    assert.NoError(t, err)

    deploy := &appsv1.Deployment{}
    err = cl.Get(context.Background(), client.ObjectKey{
        Name: "my-app", Namespace: "default",
    }, deploy)
    assert.NoError(t, err)
    assert.Equal(t, int32(2), *deploy.Spec.Replicas)
}
```

## 5. E2E 测试（kind/Minikube）

```bash
# 🟢 低风险：本地集群测试
# 1. 创建 kind 集群
kind create cluster --name operator-test

# 2. 加载镜像
kind load docker-image webapp-operator:test --name operator-test

# 3. 部署 Operator
make deploy IMG=webapp-operator:test

# 4. 运行 e2e 测试
go test ./test/e2e/ -v -ginkgo.v
```

## 6. Makefile 测试目标

```makefile
# 单元测试
test: manifests generate fmt vet envtest
	KUBEBUILDER_ASSETS="$(shell $(ENVTEST) use $(ENVTEST_K8S_VERSION) --bin-dir $(LOCALBIN) -p path)" \
	go test ./... -coverprofile cover.out

# 集成测试
test-integration: envtest
	KUBEBUILDER_ASSETS="$(shell $(ENVTEST) use $(ENVTEST_K8S_VERSION) --bin-dir $(LOCALBIN) -p path)" \
	go test ./internal/controller/... -v

# E2E 测试
test-e2e:
	go test ./test/e2e/ -v -ginkgo.v -timeout 30m
```

## 7. 测试覆盖清单

| 层级 | 覆盖范围 | 工具 |
|------|----------|------|
| 单元测试 | 纯函数、转换逻辑、验证逻辑 | `testing`, `testify` |
| Fake Client | 控制器逻辑（无真实 API） | `fake.NewClientBuilder` |
| envtest | 控制器行为（真实 API Server） | `envtest`, `ginkgo` |
| E2E | 端到端流程（真实集群） | `kind`, `e2e test` |

## Related

- [[03-清单模式/04-Operator模式/03-operator-reconciliation-patterns|调谐循环模式]]
- [[03-清单模式/04-Operator模式/04-operator-finalizer-cleanup|Finalizer 清理模式]]

## See Also

- [controller-runtime envtest 文档](https://pkg.go.dev/sigs.k8s.io/controller-runtime/pkg/envtest)
- [Kubebuilder 测试指南](https://book.kubebuilder.io/cronjob-tutorial/writing-tests)

<!-- risk-assessed -->
