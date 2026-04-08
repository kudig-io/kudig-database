# kind v0.4 Release Notes

Source: [v0.4.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.4.0)

v0.4.0 brings improved networking features in particular, including initial IPv6 support and additional node port forwards. It also continues to improve speed and reliability (hopefully! :upside_down_face:)


<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes v1.15.0 image `kindest/node:v1.15.0@sha256:b4d092fd2b507843dd096fe6c85d06a27a0cbd740a0b32a880fe61aba24bb478`
- The deprecated `kind.sigs.k8s.io/v1alpha2` version of config was removed, please switch to `kind.sigs.k8s.io/v1alpha3`
- `kind build node-image --type=apt` was removed. Please use `kind build node-image` or `kind build node-image --type=bazel` instead, or one of the pre-built images. A future release will re-work these commands and add support for building from upstream release tarballs.


<h1 id="new-features">New Features</h1>

- Additional node port forwards may be configured with the `nodes[].extraPortMappings` of `Cluster` configuration in `kind.sigs.k8s.io/v1alpha3`. This can be used to access workloads more easily from the host. Expect guides using this soon!
- Limited IPv6 support. The `networking.ipFamily` may be set to `ipv6` in `kind.sigs.k8s.io/v1alpha3` `Cluster` configuration to create an ipv6 enabled cluster on Linux. We have set up [continuous Kubernetes conformance testing](https://testgrid.k8s.io/conformance-kind#kind%20(IPv6),%20master%20(dev)) with IPv6 enabled and are working to fix the tests
- A warning is emitted if the chosen cluster name is too long and likely to cause the generated node names to be too long.
- `make install` now supports overriding `INSTALL` to specify an alternate tool to `install` and ensures the output directory
- Support / documentation for [using kind on WSL2](https://kind.sigs.k8s.io/docs/user/using-wsl2/).
- The `kind load ...` sub-commands now avoid loading images that are already present
- kubeadm `v1beta2` config is now use opportunistically for recent enough Kubernetes versions
- Reduced startup time for single-node clusters in particular

New Node have been Images for kind `v0.4.0`, please use these **exact** images or build your own as we may need to change the image format again in the future :sweat_smile: 

- `kindest/node:v1.15.0@sha256:b4d092fd2b507843dd096fe6c85d06a27a0cbd740a0b32a880fe61aba24bb478` 
- `kindest/node:v1.14.3@sha256:583166c121482848cd6509fbac525dd62d503c52a84ff45c338ee7e8b5cfe114`
- `kindest/node:v1.13.7@sha256:f3f1cfc2318d1eb88d91253a9c5fa45f6e9121b6b1e65aea6c7ef59f1549aaaf`
- `kindest/node:v1.12.9@sha256:bcb79eb3cd6550c1ba9584ce57c832dcd6e442913678d2785307a7ad9addc029`
- `kindest/node:v1.11.10@sha256:176845d919899daef63d0dbd1cf62f79902c38b8d2a86e5fa041e491ab795d33`

<h1 id="fixes">Fixes</h1>

- The generated KUBECONFIG file references the API Server by listen address (IP) instead of `localhost` (domain), which is more correct and should work on systems without a valid `localhost` entry (!)
- The new kind images should work on ipv4 only hosts with ipv6 fully disabled
- The node subnet is properly added included in `NO_PROXY` on the nodes when proxy settings are detected 
- Clusters with multiple control plane nodes now have proper healthchecks of the API servers in the external loadbalancer
- Minor cleanup and typo fixes

<h1 id="contributors">Contributors</h1>

Thanks again to everyone who committed to this release! You all are the best! :heart: 

Alphabetically by user name:
- @alisondy
- @amwat
- @aojea
- @BenTheElder
- @fabriziopandini
- @fllaca
- @jieyu
- @k8s-ci-robot 
- @kaspernissen
- @mistydemeo
- @PatrickLang
- @praseodym
- @xhzhf
- @yeya24
- @ytinirt
- @zegl
