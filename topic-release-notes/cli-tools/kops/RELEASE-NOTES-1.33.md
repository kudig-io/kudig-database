# kops v1.33 Release Notes

Source: [v1.33.2](https://github.com/kubernetes/kops/releases/tag/v1.33.2)

## What's Changed
* Automated cherry pick of #17722: scaleway: Fix failing terraform test by @hakman in https://github.com/kubernetes/kops/pull/17724
* Automated cherry pick of #17559: Bump ko-build by @hakman in https://github.com/kubernetes/kops/pull/17729
* Update Go to 1.24.7 and some deps by @hakman in https://github.com/kubernetes/kops/pull/17730
* Automated cherry pick of #17640: Update cluster-autoscaler to v1.34.0 releases
#17725: Update cluster-autoscaler to v1.34.1 by @hakman in https://github.com/kubernetes/kops/pull/17727
* Automated cherry pick of #17776: aws: Enable CloudWatch metrics for the warm pool of an ASG by @recollir in https://github.com/kubernetes/kops/pull/17779
* Automated cherry pick of #17792: aws: Disable the kubelet systemd unit during warm pool warming by @dezmodue in https://github.com/kubernetes/kops/pull/17802
* Automated cherry pick of #17793: gcp: Update ccm to fix broken arm64 jobs by @upodroid in https://github.com/kubernetes/kops/pull/17814
* Automated cherry pick of #17712: gce: bump GCE PD CSI Driver by @upodroid in https://github.com/kubernetes/kops/pull/17836
* Automated cherry pick of #17144: Normalize the hardcoded images used for warmpool pre-pulling
#17861: Feature: pull user defined images for warm pool instances by @hakman in https://github.com/kubernetes/kops/pull/17977
* chore: Back-port pulling CNI plugins from GitHub by @hakman in https://github.com/kubernetes/kops/pull/17971
* Automated cherry pick of #17980: chore: Add asset hashes for February 2026 releases by @hakman in https://github.com/kubernetes/kops/pull/17983
* Automated cherry pick of #17976: drop cdn.dl.k8s.io as a mirror
#17987: drop storage.googleapis.com/k8s-artifacts-cni as a mirror by @hakman in https://github.com/kubernetes/kops/pull/17990
* Automated cherry pick of #17956: versionbump: go 1.25.7 by @hakman in https://github.com/kubernetes/kops/pull/17995
* Automated cherry pick of #18021: chore: Add hashes for additional February releases by @hakman in https://github.com/kubernetes/kops/pull/18024
* Automated cherry pick of #18043: Fix node bootstrap challenge response hashing by @rifelpet in https://github.com/kubernetes/kops/pull/18046
* Automated cherry pick of #18058: chore: Bump Go to v1.25.8 by @hakman in https://github.com/kubernetes/kops/pull/18061
* Release 1.33.2 by @hakman in https://github.com/kubernetes/kops/pull/18092


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.33.1...v1.33.2