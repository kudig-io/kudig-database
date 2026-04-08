# kind v0.20 Release Notes

Source: [v0.20.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.20.0)

KIND v0.20.0 fixes runc 1.1.6+ / misc controller support as well as cgroupns support on cgroup v1 and begins the migration to private cgroupns for all kind nodes.

In a future release kind node images will drop support for kind binaries without cgroupns=private (which is already the default on all cgroup v2 hosts, and cgroup v1 in kind v0.20.0). This will allow us to ship a more consistent and reliable environment as the ecosystem heads towards dropping cgroup v1 more generally.

<h1 id="breaking-changes">Breaking Changes</h1>

- **Docker 20.10.0+ is now required, with no change for Podman.**
- **Node images built with kind v0.20.0+ will be required on cgroups v1 hosts for kind v0.20.0+**
- The default node image is a Kubernetes `v1.27.3` image: `kindest/node:v1.27.3@sha256:3966ac761ae0136263ffdb6cfd4db23ef8a83cba8a463690e98317add2c9ba72`

### Containerd CRI mirror config deprecation PSA

Additionally, we're asking that everyone using the local registry script update
to the latest version using `config_path`. https://kind.sigs.k8s.io/docs/user/local-registry/

Containerd deprecated the old CRI mirrors config approach and will remove support in v2.0. 
Eventually KIND will enable this by default or have upgraded to containerd 2.0.

Containerd does not support CRI mirror config if the new hosts `config_path` is enabled.

<h1 id="new-features">New Features</h1>

- Improved Kubernetes source code path detection in `kind build node-image`.
  - Now searches in this order: `$(pwd)`, `${GOPATH}/src/k8s.io/kubernetes`, `${GOPATH}/src/github.com/kubernetes/kubernetes` (default checkout location in Prow CI without `path_alias`).

New node images have been built for kind `v0.20.0`, please use these **exact** images (IE like `kindest/node:v1.27.3@sha256:3966ac761ae0136263ffdb6cfd4db23ef8a83cba8a463690e98317add2c9ba72` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images pre-built for this release:
 - 1.27: `kindest/node:v1.27.3@sha256:3966ac761ae0136263ffdb6cfd4db23ef8a83cba8a463690e98317add2c9ba72`
 - 1.26: `kindest/node:v1.26.6@sha256:6e2d8b28a5b601defe327b98bd1c2d1930b49e5d8c512e1895099e4504007adb`
 - 1.25: `kindest/node:v1.25.11@sha256:227fa11ce74ea76a0474eeefb84cb75d8dad1b08638371ecf0e86259b35be0c8`
 - 1.24: `kindest/node:v1.24.15@sha256:7db4f8bea3e14b82d12e044e25e34bd53754b7f2b0e9d56df21774e6f66a70ab`
 - 1.23: `kindest/node:v1.23.17@sha256:59c989ff8a517a93127d4a536e7014d28e235fb3529d9fba91b3951d461edfdb`
 - 1.22: `kindest/node:v1.22.17@sha256:f5b2e5698c6c9d6d0adc419c0deae21a425c07d81bbf3b6a6834042f25d4fba2`
 - 1.21: `kindest/node:v1.21.14@sha256:8a4e9bb3f415d2bb81629ce33ef9c76ba514c14d707f9797a01e3216376ba093`


Additional images built for this release:
- 1.28: `kindest/node:v1.28.0@sha256:b7a4cad12c197af3ba43202d3efe03246b3f0793f162afb40a33c923952d5b31`
- 1.29: `kindest/node:v1.29.0@sha256:eaa1450915475849a73a9227b8f201df25e55e268e5d619312131292e324d570`

**NOTE**: You *must* use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images


<!-- Additional images known compatible with this release: -->

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Upgraded runc to 1.1.7, containerd to 1.7.1
- Disabled unused misc controller on cgroups v1 hosts for KIND nodes
- Fixed cgroups on cgroup v1 hosts with cgroupns enabled
- Removed unnecessary flags from haproxy image 
- Set kubelet systemd KillMode=process

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

- @BenTheElder
- @dependabot[bot]
- @k8s-ci-robot
- @killianmuldoon
- @lixin963
- @maxerenberg

And thank you **very much** to everyone else not listed here who contributed in other ways like filing issues, giving feedback, testing fixes, helping users in slack, etc. 🙏