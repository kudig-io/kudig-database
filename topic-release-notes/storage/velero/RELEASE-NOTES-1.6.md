# velero v1.6 Release Notes

Source: [v1.6.3](https://github.com/vmware-tanzu/velero/releases/tag/v1.6.3)

## v1.6.3
### 2021-08-12

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.6.3

### Container Image
`velero/velero:v1.6.3`

### Documentation
https://velero.io/docs/v1.6/

### Upgrading
https://velero.io/docs/v1.6/upgrade-to-1.6/

### Highlights

This release introduces changes to provide compatibility with Kubernetes v1.22.

The `apiextensions.k8s.io/v1beta1` API version of `CustomResourceDefinition` will no longer be served in Kubernetes v1.22.
Velero will now use the cluster preferred API version for the `CustomResourceDefinitions` that it creates.

If you are using Kubernetes v1.15 or earlier, the `apiextensions.k8s.io/v1beta1` API version will be used.
If you are using Kubernetes v1.22 or later, the `apiextensions.k8s.io/v1` API version will be used.
For clusters between these versions, the cluster preferred API version will be used.

The `rbac.authorization.k8s.io/v1beta1` API version of `ClusterRoleBinding` will no longer be served in Kubernetes v1.22.
Velero will now use the `rbac.authorization.k8s.io/v1` API version for the `ClusterRoleBinding`s that it creates.
This API version was introduced in Kubernetes v1.8.

### All Changes

  * enable e2e tests to choose crd apiVersion (#3941, @sseago)
  * Upgrade Velero ClusterRoleBinding to use v1 API (#3995, @jenting)
  * Install Kubernetes preferred CRDs API version (v1beta1/v1). (#3999, @jenting)
  * Use the cluster preferred CRD API version when polling for Velero CRD readiness. (#4015, @zubron)
  * Add a RestoreItemAction plugin (`velero.io/apiservice`) which skips the restore of any `APIService` which is managed by Kubernetes. These are identified using the `kube-aggregator.kubernetes.io/automanaged` label. (#4028, @zubron)