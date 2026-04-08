# cilium v1.11 Release Notes

Source: [v1.11.20](https://github.com/cilium/cilium/releases/tag/v1.11.20)

We are pleased to release Cilium v1.11.20. This release comes with an important fix for IPsec.

Remaining issues on the IPSec stack may cause interrupted connections during key rotations. Users may upgrade to this release only if this is considered acceptable.

Summary of Changes
------------------

**Bugfixes:**
* Fix a bug that could cause packet drops of type XfrmOutPolBlock when IPsec is enabled and node are recycled.
* Fix a bug that could cause IPsec-encrypted packets to be sent to the wrong destination node when node churn is high. (Backport PR #27148, Upstream PR #27029, @pchaigno)

**Misc Changes:**
* chore(deps): update docker.io/library/golang docker tag to v1.19.11 (#27252, @ferozsalam)

**Other Changes:**
* install: Update image digests for v1.11.19 (#27125, @nathanjsweet)


## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.11.20@sha256:60df3cb7155886e0b62060c7a4a31e457933c6e35af79febad5fd6e43bab2a99`
`quay.io/cilium/cilium:v1.11.20@sha256:60df3cb7155886e0b62060c7a4a31e457933c6e35af79febad5fd6e43bab2a99`

### clustermesh-apiserver

`docker.io/cilium/clustermesh-apiserver:v1.11.20@sha256:46760182f8c98227cfac27627275616987b71509227775350573d834133a6d49`
`quay.io/cilium/clustermesh-apiserver:v1.11.20@sha256:46760182f8c98227cfac27627275616987b71509227775350573d834133a6d49`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.11.20@sha256:9e036af06498d1a90d8eee3ce3c3dbeb10a6bbe2b2e6a55d04941c82624a2e3a`
`quay.io/cilium/docker-plugin:v1.11.20@sha256:9e036af06498d1a90d8eee3ce3c3dbeb10a6bbe2b2e6a55d04941c82624a2e3a`

### hubble-relay

`docker.io/cilium/hubble-relay:v1.11.20@sha256:e2f38b901fd8bd5adc9a765a5e68836364ebd1e7dfb85c2bcd8a5488b23c3470`
`quay.io/cilium/hubble-relay:v1.11.20@sha256:e2f38b901fd8bd5adc9a765a5e68836364ebd1e7dfb85c2bcd8a5488b23c3470`

### operator-alibabacloud

`docker.io/cilium/operator-alibabacloud:v1.11.20@sha256:5d5b44f0a08802972323adb7ca2d5df7e0983736ab3b195090906d2fa97f9594`
`quay.io/cilium/operator-alibabacloud:v1.11.20@sha256:5d5b44f0a08802972323adb7ca2d5df7e0983736ab3b195090906d2fa97f9594`

### operator-aws

`docker.io/cilium/operator-aws:v1.11.20@sha256:48b755858729f783a682d80693ef3a208ddb70fa912b119f82f99bb988b23586`
`quay.io/cilium/operator-aws:v1.11.20@sha256:48b755858729f783a682d80693ef3a208ddb70fa912b119f82f99bb988b23586`

### operator-azure

`docker.io/cilium/operator-azure:v1.11.20@sha256:65b2d2b143830e5a5764416d000244ac447b3e1fca07fe9c138c84094fa42085`
`quay.io/cilium/operator-azure:v1.11.20@sha256:65b2d2b143830e5a5764416d000244ac447b3e1fca07fe9c138c84094fa42085`

### operator-generic

`docker.io/cilium/operator-generic:v1.11.20@sha256:1439954acf620f048ef663524ae70b4a25693c58527a2f2cee51124496e29f90`
`quay.io/cilium/operator-generic:v1.11.20@sha256:1439954acf620f048ef663524ae70b4a25693c58527a2f2cee51124496e29f90`

### operator

`docker.io/cilium/operator:v1.11.20@sha256:998f7df39d12324a7d968a8c8725533b10b54c01f4aeab33d12b395af1f2edf8`
`quay.io/cilium/operator:v1.11.20@sha256:998f7df39d12324a7d968a8c8725533b10b54c01f4aeab33d12b395af1f2edf8`