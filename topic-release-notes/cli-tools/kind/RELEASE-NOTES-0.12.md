# kind v0.12 Release Notes

Source: [v0.12.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.12.0)

`v0.12.0` has been focused on stability, with improvements and fixes for support rootless and cgroupsv2, and bringing support for s390x architectures.

<h1 id="breaking-changes">Breaking Changes</h1>

- The default node image is a Kubernetes `v1.23.4` image: `kindest/node:v1.23.4@sha256:0e34f0d0fd448aa2f2819cfd74e99fe5793a6e4938b328f657c8e3f81ee0dfb9`
- The haproxy image used for multiple control-plane cluster is distroless now

<h1 id="new-features">New Features</h1>

- Pods running at different nodes have different product_uuid
- Support for s390x architectures
- Adapt kind to handle the label migration from node-role.kubernetes.io/master to node-role.kubernetes.io/control-plane
- Base image updates
  - Containerd version 1.5.10
  - Crictl 1.23.0
  - CNI plugins 1.1.0
  - Containerd containerd/fuse-overlayfs-snapshotter 1.0.4

New Node images have been built for kind `v0.12.0`, please use these **exact** images (IE like `kindest/node:v1.23.4@sha256:0e34f0d0fd448aa2f2819cfd74e99fe5793a6e4938b328f657c8e3f81ee0dfb9` including the digest) or build your own as we may need to change the image format again in the future :sweat_smile:

Images built for this release:
  - 1.23: `kindest/node:v1.23.4@sha256:0e34f0d0fd448aa2f2819cfd74e99fe5793a6e4938b328f657c8e3f81ee0dfb9`
  - 1.22: `kindest/node:v1.22.7@sha256:1dfd72d193bf7da64765fd2f2898f78663b9ba366c2aa74be1fd7498a1873166`
  - 1.21: `kindest/node:v1.21.10@sha256:84709f09756ba4f863769bdcabe5edafc2ada72d3c8c44d6515fc581b66b029c`
  - 1.20: `kindest/node:v1.20.15@sha256:393bb9096c6c4d723bb17bceb0896407d7db581532d11ea2839c80b28e5d8deb`
  - 1.19: `kindest/node:v1.19.16@sha256:81f552397c1e6c1f293f967ecb1344d8857613fb978f963c30e907c32f598467`
  - 1.18: `kindest/node:v1.18.20@sha256:e3dca5e16116d11363e31639640042a9b1bd2c90f85717a7fc66be34089a8169`
  - 1.17: `kindest/node:v1.17.17@sha256:e477ee64df5731aa4ef4deabbafc34e8d9a686b49178f726563598344a3898d5`
  - 1.16: `kindest/node:v1.16.15@sha256:64bac16b83b6adfd04ea3fbcf6c9b5b893277120f2b2cbf9f5fa3e5d4c2260cc`
  - 1.15: `kindest/node:v1.15.12@sha256:9dfc13db6d3fd5e5b275f8c4657ee6a62ef9cb405546664f2de2eabcfd6db778`
  - 1.14: `kindest/node:v1.14.10@sha256:b693339da2a927949025869425e20daf80111ccabf020d4021a23c00bae29d82`

NOTE: these node images support amd64 and arm64 now. It remains possible to build custom images for other architectures (see the docs).

<h1 id="fixes">Fixes</h1>

- Enable sysctl route_localnet on node images to allow nodes dns to work
- Only mount /dev/fuse on rootless environments
- Fixed node labels assignments from configuration
- Fix docker+cgroup2+rootless initialization
- Fix sysbox runtime regression on docker in docker
- Fix support on distros without systemd and WSL2
- Fix kind load docker-image if all images are already present

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this release! ❤️**

Users whose commits are in this release (alphabetically by user name)

Contributors since v0.11.1:
- @AkihiroSuda
- @Alfa-Alex
- @amwat
- @aojea
- @atoato88
- @bagnaram
- @BenTheElder
- @chaodaiG
- @csams
- @dlipovetsky
- @enyineer
- @fedepaol
- @fenggw-fnst
- @flouthoc
- @fstrudel
- @gAmUssA
- @gotzmann
- @gprossliner
- @guirish
- @hs0210
- @jayonlau
- @jimangel
- @juniorjbn
- @k8s-ci-robot
- @kerthcet
- @knight42
- @KrisJohnstone
- @ktock
- @marcosnils
- @matzew
- @ml-
- @neolit123
- @networkop
- @oke-py
- @pchan
- @pmorch
- @PushkarJ
- @qinqon
- @rahulii
- @rikatz
- @sestegra
- @sethp
- @simon-geard
- @stevekuznetsov
- @stmcginnis
- @tao12345666333
- @tgeoghegan
- @valeneiko
- @vdjagilev
- @viveksyngh
- @wanlerong
- @xinydev
- @YuviGold