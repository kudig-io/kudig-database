# cilium v1.16 Release Notes

Source: [v1.16.19](https://github.com/cilium/cilium/releases/tag/v1.16.19)

Summary of Changes
------------------

**Major Changes:**
* Publish Helm charts to OCI registries (Backport PR cilium/cilium#43688, Upstream PR cilium/cilium#43624, @aanm)

**Bugfixes:**
* ipcache: Fix leak in CIDR metadata consolidation logic (Backport PR cilium/cilium#43427, Upstream PR cilium/cilium#43074, @christarazi)
* xds: fix nil-pointer in `processRequestStream` (Backport PR cilium/cilium#43614, Upstream PR cilium/cilium#43609, @mhofstetter)

**CI Changes:**
* chore: comment job to use generated token instead of PAT (Backport PR cilium/cilium#43614, Upstream PR cilium/cilium#43148, @sekhar-isovalent)
* ci: Use newer lvh image for privileged tests (Backport PR cilium/cilium#43488, Upstream PR cilium/cilium#41082, @rastislavs)

**Misc Changes:**
* Add documentation and examples for using the egressDeny field in CiliumNetworkPolicy (Backport PR cilium/cilium#43427, Upstream PR cilium/cilium#40272, @syedazeez337)
* chore(deps): update all github action dependencies (v1.16) (cilium/cilium#43471, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.16) (cilium/cilium#43673, @cilium-renovate[bot])
* chore(deps): update anchore/sbom-action action to v0.21.0 (v1.16) (cilium/cilium#43514, @cilium-renovate[bot])
* chore(deps): update dependency protocolbuffers/protobuf to v33.3 (v1.16) (cilium/cilium#43672, @cilium-renovate[bot])
* chore(deps): update docker.io/library/busybox:1.36.1 docker digest to 4059bab (v1.16) (cilium/cilium#43670, @cilium-renovate[bot])
* chore(deps): update docker.io/library/golang:1.24.11 docker digest to 54528d1 (v1.16) (cilium/cilium#43418, @cilium-renovate[bot])
* chore(deps): update docker.io/library/golang:1.24.11 docker digest to a61b432 (v1.16) (cilium/cilium#43548, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.34.12-1767177245-7935d4d711cb6f8020385a50c996b90896e16a71 (v1.16) (cilium/cilium#43549, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.16) (patch) (cilium/cilium#43470, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.16) (patch) (cilium/cilium#43551, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.16) (patch) (cilium/cilium#43573, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.16) (patch) (cilium/cilium#43671, @cilium-renovate[bot])
* release: change OCI registry (Backport PR cilium/cilium#43688, Upstream PR cilium/cilium#43646, @aanm)
* route: install ingress proxy routes with WireGuard and L7Proxy (Backport PR cilium/cilium#43436, Upstream PR cilium/cilium#42835, @smagnani96)

**Other Changes:**
* [v1.16] deps: bump CNI plugins version to v1.9.0 (cilium/cilium#43591, @diyi0926)
* install: Update image digests for v1.16.18 (cilium/cilium#43402, @cilium-release-bot[bot])


## Docker Manifests

### cilium

`quay.io/cilium/cilium:v1.16.19@sha256:f0c260e30ef97ce3e45e833e702ab47efbbb1dadd0a394969c0a65553e98fefb`

### clustermesh-apiserver

`quay.io/cilium/clustermesh-apiserver:v1.16.19@sha256:115fbc5c73c3e087a653e8a6b690a935660cc70b8be4cd088c1439272c95722a`

### docker-plugin

`quay.io/cilium/docker-plugin:v1.16.19@sha256:2cd2ac3a80c1d1114a15ff81dafe1b463ec33fa302efed918d474afedf85aeef`

### hubble-relay

`quay.io/cilium/hubble-relay:v1.16.19@sha256:b9f23a99df5ddb3837211b94270d8cf70a942f9555b4c9afc3b37920323551bb`

### operator-alibabacloud

`quay.io/cilium/operator-alibabacloud:v1.16.19@sha256:7a13ffdfad021d53aa01c188e061025290a9edfaf62d0c2a493fcb3f55f441bb`

### operator-aws

`quay.io/cilium/operator-aws:v1.16.19@sha256:98f10effb7f222d9a1d99303734e851fce7a10ba61ff9b5b2f8d40fcc9162621`

### operator-azure

`quay.io/cilium/operator-azure:v1.16.19@sha256:4db7eaeeb2857e8ad778791b3cf9c1b2ab7509c2e12793ab0efd80ccece14ad3`

### operator-generic

`quay.io/cilium/operator-generic:v1.16.19@sha256:8879e792c5566f6349b5f2865e07c0dd690eb32638afc4417b51b0ec574fa5f0`

### operator

`quay.io/cilium/operator:v1.16.19@sha256:c12d281e3d4d80d7e551cda612c68ea5648932088a67d8629a80e47e73e8dcce`

