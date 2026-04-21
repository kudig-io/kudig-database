# kind v0.3 Release Notes

Source: [v0.3.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.3.0)

This release focused on improving the speed and reliability of cluster creation,
networking, and bug fixes. A number of breaking changes were required, but Nodes
should be simpler, smaller, faster, and more reliable.


<h1 id="breaking-changes">Breaking Changes</h1>

**IMPORTANT**: Breaking Image Changes

kind `v0.3`+ requires new node images versus `v0.2.1` and earlier. A number of
important internal improvements were made that required changing how cluster 
bootup is performed. The following images we have previously published are 
incompatible with kind `v0.3`+:
- `kindest/node:v1.11.3`
- `kindest/node:v1.12.2`
- `kindest/node:v1.12.3`
- `kindest/node:v1.13.2`
- `kindest/node:v1.13.4`
- `kindest/node:v1.14.0`

The following images require unreleased versions of kind between `v0.3.0` and `v0.2.1`,
please do not use them:
- `kindest/node:v1.14.1`

For 0.3+ we are pushing the following images:
- `kindest/nodev1.14.2@sha256:33539d830a6cf20e3e0a75d0c46a4e94730d78c7375435e6b49833d81448c319`
- `kindest/node:v1.13.6@sha256:9e07014fb48c746deb98ec8aafd58c3918622eca6063e643c6e6d86c86e170b4`
- `kindest/node:v1.12.8@sha256:cc6e1a928a85c14b52e32ea97a198393fb68097f14c4d4c454a8a3bc1d8d486c`
- `kindest/node:v1.11.10@sha256:abd0275ead5ddfd477b7bc491f71957d7dd75408a346834ffe8a9bee5fbdc15b`

We hope not to make further breaking changes to the images, but strongly
recommend pinning images by sha256, the default image is pinned to a particular sha.
This can be done with `--image=kindest/node:v1.X.Y@sha256:hash`. For example:
`kind create cluster --image=kindest/node:v1.12.8@sha256:cc6e1a928a85c14b52e32ea97a198393fb68097f14c4d4c454a8a3bc1d8d486c`

- Default clusters will now be Kubernetes `v1.14.2`, the default node image is 
  now `kindest/nodev1.14.2@sha256:33539d830a6cf20e3e0a75d0c46a4e94730d78c7375435e6b49833d81448c319`

- Kubernetes on the nodes now uses CRI (containerd) instead of dockershim
  (dockerd) inside the "node" docker containers.
  **Please note** that details underlying Kubernetes are not guaranteed and may
  change again in the future, however we are still warning about this particular
  change as some advanced users depended on these details anyhow.

- Similarly, the default CNI / networking setup changed, kind now sets a 
  podSubnet and masquerades outbound pod traffic with a new CNI
  setup. This subnet is defaulted but is now configurable and the entire CNI setup
  can be disabled (see features below).

- Building kind now requires go modules (using any upstream supported go version)
 or using the new Makefile. Pre-built binaries for all platforms are included with
 this release. Please consider using one of the binaries in your CI setups, or
 at least pinning to a tagged release.


<h1 id="new-features">New Features</h1>

- The default CNI can be disabled with the `Cluster.networking.disableDefaultCNI`
  config field, making it easier to experiment with your own CNI.
- The Pod Subnet is now configurable with `Cluster.networking.podSubnet`
- `kind completion bash` is now supported to source shell completions.
  - **WARNING**: we are considering renaming this to align better with the `[verb] [noun]` Kubernetes style CLI structure.
- Limited PPC64LE support (requires building your own images)
- The kind command line binary is now built reproducibly
  - building now only requires `make` / `docker` on the host as an alternative to go 1.11+ with modules
- kind now runs a minimal and lightweight CNI setup
- internal progress on IPv6 support
- internal progress on cluster reboot support
- kind is now versioned with go modules

<h1 id="fixes">Fixes</h1>

- We've improved auto-detecting the kubernetes source location for `kind build node-image`
- Cluster bootup should be even faster and more reliable in constrained environments.
  - simplified startup logic with less waiting
  - Kubernetes images are fully preloaded at build time
  - our networking setup is now somewhat custom, and is simpler and lighter
- `kind export logs` should be less likley to flake with large numbers of nodes or
  log files, internally we've combined most of the streams.
- More improvements to proxy handling, though kind may still not work correctly
  in proxied environments.
- `/sys/class/dmi/id/product_name` is faked to `kind` where applicable, this avoids
  Kubernetes detecting if kind is running on GCE.

<h1 id="contributors">Contributors</h1>

Thanks again to everyone who committed to this release! :heart: 

Alphabetically by user name:
- @alejandrox1
- @ameukam
- @aojea
- @BenTheElder
- @dims
- @fiunchinho
- @fstab
- @k8s-ci-robot
- @mkumatag
- @neolit123
- @paranoidaditya
- @sohankunkerkar
- @steffengy
- @tao12345666333
