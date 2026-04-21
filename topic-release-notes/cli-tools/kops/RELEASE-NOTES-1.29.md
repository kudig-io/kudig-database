# kops v1.29 Release Notes

Source: [v1.29.2](https://github.com/kubernetes/kops/releases/tag/v1.29.2)

(Note that v1.29.1 was not released, due to an problem in the release process)

## What's Changed (since v1.29.0)

* Upgrade node-termination-handler to 1.22.0 by @jim-barber-he in https://github.com/kubernetes/kops/pull/16595
* Make ASG Warmpool depend on ASG Lifecycle hook by @jim-barber-he in https://github.com/kubernetes/kops/pull/16603
* Support kube-controller-manager component by @chubchubsancho in https://github.com/kubernetes/kops/pull/16608
* Update aws-iam-authenticator image by @rifelpet in https://github.com/kubernetes/kops/pull/16616
* Update Go to v1.21.4 by @hakman in https://github.com/kubernetes/kops/pull/16619
* Upgrade cilium to v1.15.6 by @rifelpet in https://github.com/kubernetes/kops/pull/16628
* Update golang to 1.22.5 by @justinsb in https://github.com/kubernetes/kops/pull/16653
* Fix cluster-autoscaler priority expander config by @rifelpet in https://github.com/kubernetes/kops/pull/16672
* Bump cloudbuild to go 1.22.5 by @justinsb in https://github.com/kubernetes/kops/pull/16684
* Add the hubble-metrics service for cilium by @rifelpet in https://github.com/kubernetes/kops/pull/16687
* Add new API field for VPC CNI's network policy agent by @rifelpet in https://github.com/kubernetes/kops/pull/16689


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.29.0...v1.29.2