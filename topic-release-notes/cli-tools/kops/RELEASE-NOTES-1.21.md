# kops v1.21 Release Notes

Source: [v1.21.5](https://github.com/kubernetes/kops/releases/tag/v1.21.5)

## Release notes for kOps 1.21 series

# Significant changes

## Service Account Issuer Discovery and AWS IAM Roles for Service Accounts (IRSA)

kOps now supports publishing an OIDC-compatible discovery document to an S3 bucket and configuring AWS to use it for IAM Roles for Service Accounts (IRSA).

See the [Service Account Issuer Discovery](https://kops.sigs.k8s.io/cluster_spec#service-account-issuer-discovery-and-aws-iam-roles-for-service-accounts-irsa) documentation for more information.

## Dedicated API Server nodes.

kOps now supports extending the control plane with [dedicated apiserver nodes](https://kops.sigs.k8s.io/operations/scaling). These nodes run in dedicated instance groups that can be scaled horizontally.

In 1.21, this feature is behind a feature flag as node role name, labels, taints, and domains can change based on feedback from the community.

## Warm Pool (AWS only)

A Warm Pool contains pre-initialized EC2 instances that can join the cluster significantly faster than regular instances. These instances run the kOps configuration process, pull known container images, and then shut down. When the ASG needs to scale out it will pull instances from the warm pool if any are available.

See the [warm pool](https://kops.sigs.k8s.io/instance_groups/#warmpool-aws-only) documentation for more information.

# Other significant changes

* Protokube now runs as a systemd process rather than a docker container.

* Support for AWS launch configurations has been removed in favour of launch templates.

* kOps can now use Node Termination Handler's Queue Processor mode, which offers more functionality than the IMDS Processor mode. See [the addons page](https://kops.sigs.k8s.io/addons/#queue-processor-mode) for more information.

* New addon for the [CSI snapshot-controller](https://kops.sigs.k8s.io/addons/#snapshot-controller).

# Breaking changes

* Support for Kubernetes versions 1.13 and 1.14 has been removed.

# Required Actions

* The ClusterRoleBinding for AWS EBS CSI DaemonSet has changed name. If you installed this addon before kOps 1.21, you need run `kubectl delete crb ebs-csi-node-binding`.

* To support [Node Termination Handler's Queue Process mode](https://kops.sigs.k8s.io/addons/#node-termination-handler), AWS cluster deletion now requires the kops CLI have `sqs:ListQueues` and `events:ListRules` permissions regardless of whether or not the addon is used.

# Deprecations

* Support for Kubernetes versions 1.15 and 1.16 is deprecated and will be removed in kOps 1.22.

* Support for Kubernetes version 1.17 is deprecated and will be removed in kOps 1.23.

* Support for CentOS 7 is deprecated and will be removed in future versions of kOps.

* Support for CentOS 8 is deprecated and will be removed in future versions of kOps.

* Support for Debian 9 (Stretch) is deprecated and will be removed in future versions of kOps.

* Support for RHEL 7 is deprecated and will be removed in future versions of kOps.

* Support for Ubuntu 18.04 (Bionic) is deprecated and will be removed in future versions of kOps.

* The legacy location for downloads `s3://https://kubeupv2.s3.amazonaws.com/kops/` has been deprecated and will not be used as of kOps 1.22. The new canonical downloads location is `https://artifacts.k8s.io/binaries/kops/`.

* The [manifest based metrics server addon](https://github.com/kubernetes/kops/tree/master/addons/metrics-server) has been deprecated in favour of a configurable addon.

* The [manifest based cluster autoscaler addon](https://github.com/kubernetes/kops/tree/master/addons/cluster-autoscaler) has been deprecated in favour of a configurable addon.

* The `node-role.kubernetes.io/master` and `kubernetes.io/role` labels are deprecated and might be removed from control plane nodes in kOps 1.23.

* Due to lack of maintainers, the Aliyun/Alibaba Cloud support has been deprecated. The current implementation will be left as-is until the implementation needs updates or otherwise becomes incompatible. At that point, it will be removed. We very much welcome anyone willing to contribute to this cloud provider.

# Full change list since 1.21.4 release

* Release 1.21.4 [@johngmyers](https://github.com/johngmyers) [#12800](https://github.com/kubernetes/kops/pull/12800)
* Add support for etcd v3.5.1 [@hakman](https://github.com/hakman) [#12826](https://github.com/kubernetes/kops/pull/12826)
* Add support for --dns flag in Docker config [@jwolski2](https://github.com/jwolski2) [#12789](https://github.com/kubernetes/kops/pull/12789)
* Update Go to v1.16.11 [@hakman](https://github.com/hakman) [#12897](https://github.com/kubernetes/kops/pull/12897)
* Update Go to v1.16.12 [@hakman](https://github.com/hakman) [#12956](https://github.com/kubernetes/kops/pull/12956)
* Prevent creation of unsupported etcd clusters [@olemarkus](https://github.com/olemarkus) [#13011](https://github.com/kubernetes/kops/pull/13011)
* Add action for automatically tagging releases [@johngmyers](https://github.com/johngmyers) [#12805](https://github.com/kubernetes/kops/pull/12805)
* Fix CSI migration feature gates [@olemarkus](https://github.com/olemarkus) [#13203](https://github.com/kubernetes/kops/pull/13203)
* upgrade cluster: support comma separated list for machineType [1.21] [@MeirP-3](https://github.com/MeirP-3) [#13210](https://github.com/kubernetes/kops/pull/13210)
* Simplify Flatcar containerd exec command [@pothos](https://github.com/pothos) [#12900](https://github.com/kubernetes/kops/pull/12900)
* Update to etcd-manager v3.0.20220203 [@justinsb](https://github.com/justinsb) [#13196](https://github.com/kubernetes/kops/pull/13196)
* Add support for ed25519 keys in AWS [@aclevername](https://github.com/aclevername) [#13304](https://github.com/kubernetes/kops/pull/13304)