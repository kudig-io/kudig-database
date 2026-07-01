---
title: kops v1.25 Release Notes
description: kops v1.25 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- containerd
- nvidia
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.25 Release Notes 是什么
- 如何 kops v1.25 Release Notes
trigger_keywords:
- kops
- v1.25
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
- etcd-basics
---



# kops v1.25 Release Notes

Source: [v1.25.4](https://github.com/kubernetes/kops/releases/tag/v1.25.4)

## What's Changed
* Automated cherry pick of #14667: We no longer release an images.tar.gz by @hakman in https://github.com/kubernetes/kops/pull/14673
* Automated cherry pick of #14704: Update OWNERS files by @hakman in https://github.com/kubernetes/kops/pull/14757
* Automated cherry pick of #14734: Update [[etcd|etcd]] to v3.5.6
#14752: Update etcd-manager to v3.0.20221209 by @hakman in https://github.com/kubernetes/kops/pull/14755
* Automated cherry pick of #14779: Update Go to v1.19.4 by @hakman in https://github.com/kubernetes/kops/pull/14780
* Update dependencies by @hakman in https://github.com/kubernetes/kops/pull/14781
* Automated cherry pick of #14782: Update [[containerd|containerd]] to v1.6.12 by @hakman in https://github.com/kubernetes/kops/pull/14783
* Automated cherry pick of #14789: Update containerd to v1.6.13 by @hakman in https://github.com/kubernetes/kops/pull/14790
* Automated cherry pick of #14815: Update containerd to v1.6.14 by @hakman in https://github.com/kubernetes/kops/pull/14816
* Automated cherry pick of #14848: Validate control-plane IG size by @hakman in https://github.com/kubernetes/kops/pull/14849
* Automated cherry pick of #14880: Use short service name with discovery labels by @johngmyers in https://github.com/kubernetes/kops/pull/14895
* Automated cherry pick of #14902: etcd domains are now under .internal. by @johngmyers in https://github.com/kubernetes/kops/pull/14904
* Automated cherry pick of #14974: Update containerd to v1.6.15 by @hakman in https://github.com/kubernetes/kops/pull/14976
* Automated cherry pick of #14978: Update Go to v1.19.5 by @hakman in https://github.com/kubernetes/kops/pull/14980
* Automated cherry pick of #14993: Rename version.go to kops-version.go by @johngmyers in https://github.com/kubernetes/kops/pull/14994
* Automated cherry pick of #15002: Run kops-controller server on non-leaders as well by @johngmyers in https://github.com/kubernetes/kops/pull/15010
* Automated cherry pick of #15011: Upgrade AWS CCM to 1.25.2 by @johngmyers in https://github.com/kubernetes/kops/pull/15013
* Automated cherry pick of #15072: Update containerd to v1.6.16 by @hakman in https://github.com/kubernetes/kops/pull/15074
* Automated cherry pick of #15088: Update etcd to v3.5.7 by @hakman in https://github.com/kubernetes/kops/pull/15089
* Automated cherry pick of #15096: Use ubuntu18.04 repos for nvidia-container-toolkit by @zetaab in https://github.com/kubernetes/kops/pull/15101
* Automated cherry pick of #15105: aws: Remove S3 region validation by @hakman in https://github.com/kubernetes/kops/pull/15107
* Automated cherry pick of #15134: Use registry.k8s.io for legacy addons by @hakman in https://github.com/kubernetes/kops/pull/15137
* Automated cherry pick of #15131: Update containerd to v1.6.17 by @hakman in https://github.com/kubernetes/kops/pull/15133
* Automated cherry pick of #15153: Add terraform target support for configuring Warm Pool by @hakman in https://github.com/kubernetes/kops/pull/15155
* Automated cherry pick of #15160: Update Go to v1.19.6 by @hakman in https://github.com/kubernetes/kops/pull/15162
* Automated cherry pick of #15169: update openstack csi & ccm versions by @zetaab in https://github.com/kubernetes/kops/pull/15171
* Automated cherry pick of #15159: Update containerd to v1.6.18 by @hakman in https://github.com/kubernetes/kops/pull/15164
* Automated cherry pick of #15040: gce: When using network native pod IPs, open firewall to by @hakman in https://github.com/kubernetes/kops/pull/15189
* Automated cherry pick of #15198: Update Go to v1.19.7 by @hakman in https://github.com/kubernetes/kops/pull/15200
* Release 1.25.4 by @hakman in https://github.com/kubernetes/kops/pull/15202


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.25.3...v1.25.4