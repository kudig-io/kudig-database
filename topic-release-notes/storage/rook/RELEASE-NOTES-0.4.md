# rook v0.4 Release Notes

Source: [v0.4.0](https://github.com/rook/rook/releases/tag/v0.4.0)

## Notes
* [Breaking changes list for v0.4.0](https://github.com/rook/rook/wiki/v0.4-TPR-Breaking-Changes)
* Kubernetes 1.6 is now the minimum supported version
* In place upgrades are not yet supported
* Rook releases now support "[release channels](https://github.com/rook/rook/blob/master/build/Releasing.md#promoting-from-master)" (master, alpha, beta, stable)
  * Users can now always try the latest from master in `quay.io/rook/rookd:master-latest`

## Features and Improvements
* Additional Kubernetes support and improvements to the Rook Operator and TPRs, including:
  * [Dynamic volume provisioner](https://github.com/rook/rook/blob/master/Documentation/k8s-block.md#provision-storage) that creates and deletes block storage on demand for use by pods
  * Flexible storage configuration settings in the [Cluster TPR](https://github.com/rook/rook/blob/master/Documentation/cluster-tpr.md)
  * [Role Based Access Control](https://kubernetes.io/docs/admin/authorization/rbac/) (RBAC) is now supported
  * [`kubeadm`](https://kubernetes.io/docs/admin/kubeadm/) support
  * [Minikube](https://github.com/rook/rook/blob/master/Documentation/k8s-pre-reqs.md#minikube) support
* [Automated integration testing](https://github.com/rook/rook/blob/master/e2e/README.md) pipeline now runs on every pull request and commit to master
* Single Rook container image `quay.io/rook/rookd`
* Practical walkthrough examples for [block](https://github.com/rook/rook/blob/master/Documentation/k8s-block.md), [file](https://github.com/rook/rook/blob/master/Documentation/k8s-filesystem.md) and [object](https://github.com/rook/rook/blob/master/Documentation/k8s-object.md) storage
* [Advanced cluster configuration and troubleshooting documentation](https://github.com/rook/rook/blob/master/Documentation/advanced-configuration.md)
* Numerous bug fixes and reliability improvements

The full set of completed issues can be found in the [v0.4.0 milestone](https://github.com/rook/rook/milestone/4?closed=1).