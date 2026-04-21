# kind v0.7 Release Notes

Source: [v0.7.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.7.0)

`v0.7.0` notably brings greatly improved support for dynamic PersistentVolumes by way of [rancher.io/localhost-path] integration for clusters with Kubernetes v1.12.X or greater.

It is also worth noting that kind is now committed to IPv6 PR testing and running 80% of the GCE presubmit test cases on Kubernetes Pull Requests.

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.17.0` image `kindest/node:v1.17.0@sha256:9512edae126da271b66b990b6fff768fbb7cd786c7d39e86bdf55906352fdf62`
- **Node images built with kind `v0.7.0` have many improvements and require kind `v0.5.0+`**, images built with older releases should continue to work.
  - To get the new dynamic PV support you must upgrade both kind and your node image(s) to this release or newer. The images are compatible with older releases but only both a newer kind CLI and newer images together will enable the updated PV support.
  - When the new PV support enabled by these versions the default storage class is changed from the previous `k8s.io/host-path` based implementation.

<h1 id="new-features">New Features</h1>

- Dynamic PersistentVolume Support is greatly enhanced with [rancher.io/localhost-path] integration out of the box
- Upgraded dependencies around the board, notably including fixes to the CNI portmap plugin to reduce flakiness
- Documentation Enhancements
  - We have started an effort to comprehensively cover configuration in the docs
  - We have [a dedicated guide for using Ingress](https://kind.sigs.k8s.io/docs/user/ingress/)
  - Added more resources
  - Improved site code, in particular added copyable snippets
  - Other assorted smaller improvements
- Again show one box emoji per node being provisioned

New Node have been Images for kind `v0.7.0`, please use these **exact** images (IE like `kindest/node:v1.16.4@sha256:b91a2c2317a000f3a783489dfb755064177dbc3a0b2f4147d50f04825d016f55`) or build your own as we may need to change the image format again in the future :sweat_smile: 

 Kubernetes Version @ `digest`
  - v1.18.0 @ `sha256:0e20578828edd939d25eb98496a685c76c98d54084932f76069f886ec315d694`
  - v1.17.0 @ `sha256:9512edae126da271b66b990b6fff768fbb7cd786c7d39e86bdf55906352fdf62`
  - v1.16.4 @ `sha256:b91a2c2317a000f3a783489dfb755064177dbc3a0b2f4147d50f04825d016f55`
  - v1.15.7 @ `sha256:e2df133f80ef633c53c0200114fce2ed5e1f6947477dbc83261a6a921169488d`
  - v1.14.10 @ `sha256:81ae5a3237c779efc4dda43cc81c696f88a194abcc4f8fa34f86cf674aa14977`
  - v1.13.12 @`sha256:5e8ae1a4e39f3d151d420ef912e18368745a2ede6d20ea87506920cd947a7e3a`
  - v1.12.10 @ `sha256:68a6581f64b54994b824708286fafc37f1227b7b54cbb8865182ce1e036ed1cc`
  - v1.11.10 @ `sha256:e6f3dade95b7cb74081c5b9f3291aaaa6026a90a977e0b990778b6adc9ea6248`


<h1 id="fixes">Fixes</h1>

- Improved messaging in a number of user facing errors
- Disabled GINKGO_FLAKE_ATTEMPTS when testing
- Fixed local image check
- When the default kubeconfig is empty / non-existent, `kind create cluster` and `kind export kubeconfig` will preserve extra fields from kubeadm, notably `apiVersion` and `kind` (though they are not strictly necessary...)
- Reduced cruft in base image

<h1 id="contributors">Contributors</h1>

**Thanks again to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @amwat
- @aojea
- @BenTheElder
- @danielhelfand
- @darkowlzz
- @daxmc99
- @dustinspecker
- @HeavyWombat
- @jpreese
- @k8s-ci-robot
- @kandros
- @kezmorris
- @longkb
- @marcindulak
- @MasayaAoyama
- @matti
- @truongnh1992
- @ytsarev

[rancher.io/localhost-path]: https://github.com/rancher/local-path-provisioner
