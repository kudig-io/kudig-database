# cri-o v1.14 Release Notes

Source: [v1.14.12](https://github.com/cri-o/cri-o/releases/tag/v1.14.12)

CRI-O 1.14.12

Welcome to the v1.14.12 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Nalin Dahyabhai
* Peter Hunt
* Giuseppe Scrivano
* Mrunal Patel
* Urvashi Mohnani
* Valentin Rothberg
* Qi Wang
* Sascha Grunert

### Changes

* 7f19b5f58 Bump up version to 1.14.12
* 74985cbcb Fix integration tests by adjusting image digest
* e6b9d39d1 [1.14] update github.com/containers/image
* a0bd2ca3c tests: adjust test to not depend on runc behavior
* 8e2076b14 contrib: add script for building crun
* 98d0d9a77 Destroy the pod's network when it can't be restored
* 12470bcd7 skip test failing on mount eperm
* 179ea6b55 fix ami rhel7
* 4a3c6d1f5 sunset our debugging plugin
* e46b42b33 Update package code for fedora/rhel
* ff6b507f6 disable nodev
* be95020d7 Check hooks directories ourselves
* 1b686092f Makefile: go build/test with -mod=vendor
* 108ead1d0 hack/build-rpms.sh: use cri-o.spec
* b27839f04 .travis.yml: "make vendor" uses "go mod", so skip it sometimes
* 39e84c5dc "make vendor"
* b51557e5c Update mockgen output to match newer storage
* c521e0f54 Update for API change in runtime-spec
* a55e9986b Update for API change in buildah
* 8cd1763a5 Bump github.com/containernetworking/plugins from 0.7.5 to 0.8.0
* 3cce9c22c Update for containerd API change
* 50d2cc684 makeRepoDigests(): add all manifest digests to RepoDigests
* e72f8fbeb image.bats: check start with canonical list reference
* 542ef7d9c image.bats: add tests to exercise manifest list support
* 15fe82058 imageConfigDigest: choose the local image if necessary
* 0e3a5f1eb bin2img: updates for API changes
* 0b97a1159 docker/docker/pkg/stringid -> containers/storage/pkg/stringid
* 3fc3edebe Move to containers/image v5 and buildah v1.11.4.
* 9b3070133 "make vendor"
* 614a70d0f Replace vendor.conf with go.mod
* daa093f09 Add disk usage for ListContainerStats
* 382f5d211 test: test failures and successes correctly

### Dependency Changes

Previous release can be found at [v1.14.11](https://github.com/cri-o/cri-o/releases/tag/v1.14.11)

* **github.com/Azure/go-ansiterm**                        19f72df4d05d -> d6e3b3328b78
* **github.com/BurntSushi/toml**                          v0.3.0 -> v0.3.1
* **github.com/Microsoft/go-winio**                       78439966b38d -> v0.4.14
* **github.com/Microsoft/hcsshim**                        43f972530799 -> v0.8.6
* **github.com/OpenPeeDeeP/depguard**                     v1.0.1 **_new_**
* **github.com/beorn7/perks**                             3ac7bf7a47d1 -> v1.0.1
* **github.com/blang/semver**                             v3.5.0 -> v3.5.1
* **github.com/bombsimon/wsl**                            v1.2.5 **_new_**
* **github.com/checkpoint-restore/go-criu**               v3.11 -> bdb7599cd87b
* **github.com/containerd/containerd**                    v1.2.2 -> v1.3.0
* **github.com/containerd/continuity**                    d8fb8589b0e8 -> aaeac12a7ffc
* **github.com/containerd/ttrpc**                         2a805f718635 -> 92c8520ef9f8
* **github.com/containernetworking/plugins**              v0.7.5 -> v0.8.2
* **github.com/containers/buildah**                       v1.8.4 -> 20e92ffe0982
* **github.com/containers/image/v5**                      82291c45f2b0 **_new_**
* **github.com/containers/libpod**                        b0b16bbea62f -> de32b89eff09
* **github.com/containers/libtrust**                      14b96171aa3b **_new_**
* **github.com/containers/psgo**                          v1.3.0 -> v1.3.2
* **github.com/containers/storage**                       cri-o-release-1.14 -> v1.13.5
* **github.com/coreos/go-systemd**                        v14 -> fd7a80b32e1f
* **github.com/coreos/pkg**                               v3 -> 399ea9e2e55f
* **github.com/cpuguy83/go-md2man**                       v1.0.10 **_new_**
* **github.com/creack/pty**                               v1.1.7 **_new_**
* **github.com/cri-o/ocicni**                             7bd73e9a7f59 -> deac903fd99b
* **github.com/cyphar/filepath-securejoin**               v0.2.1 -> v0.2.2
* **github.com/davecgh/go-spew**                          v1.1.0 -> v1.1.1
* **github.com/docker/distribution**                      5f6282db7d65 -> v2.7.1
* **github.com/docker/docker**                            54dddadc7d5d -> ada3c14355ce
* **github.com/docker/docker-credential-helpers**         v0.6.1 -> v0.6.3
* **github.com/docker/go-metrics**                        v0.0.1 **_new_**
* **github.com/docker/go-units**                          v0.3.1 -> v0.4.0
* **github.com/docker/libnetwork**                        5f7a3f68c3d9 -> 5a177b73e316
* **github.com/fatih/color**                              v1.7.0 **_new_**
* **github.com/fsnotify/fsnotify**                        7d7316ed6e1e -> v1.4.7
* **github.com/fsouza/go-dockerclient**                   v1.3.0 -> v1.5.0
* **github.com/go-critic/go-critic**                      d79a9f0c64db **_new_**
* **github.com/go-lintpack/lintpack**                     v0.5.2 **_new_**
* **github.com/go-toolsmith/astcast**                     v1.0.0 **_new_**
* **github.com/go-toolsmith/astcopy**                     v1.0.0 **_new_**
* **github.com/go-toolsmith/astequal**                    v1.0.0 **_new_**
* **github.com/go-toolsmith/astfmt**                      v1.0.0 **_new_**
* **github.com/go-toolsmith/astp**                        v1.0.0 **_new_**
* **github.com/go-toolsmith/strparse**                    v1.0.0 **_new_**
* **github.com/go-toolsmith/typep**                       v1.0.0 **_new_**
* **github.com/gobwas/glob**                              v0.2.3 **_new_**
* **github.com/godbus/dbus**                              a389bdde4dd6 -> 2ff6f7ffd60f
* **github.com/gofrs/flock**                              5135e617513b **_new_**
* **github.com/gogo/protobuf**                            v1.0.0 -> v1.2.1
* **github.com/golang/groupcache**                        b710c8433bd1 -> 5b532d6fd5ef
* **github.com/golang/protobuf**                          v1.2.0 -> v1.3.2
* **github.com/golangci/check**                           cfe4005ccda2 **_new_**
* **github.com/golangci/dupl**                            3e9179ac440a **_new_**
* **github.com/golangci/errcheck**                        ef45e06d44b6 **_new_**
* **github.com/golangci/go-misc**                         927a3d87b613 **_new_**
* **github.com/golangci/goconst**                         041c5f2b40f3 **_new_**
* **github.com/golangci/gocyclo**                         2becd97e67ee **_new_**
* **github.com/golangci/gofmt**                           244bba706f1a **_new_**
* **github.com/golangci/golangci-lint**                   v1.21.0 **_new_**
* **github.com/golangci/ineffassign**                     42439a7714cc **_new_**
* **github.com/golangci/lint-1**                          297bf364a8e0 **_new_**
* **github.com/golangci/maligned**                        b1d89398deca **_new_**
* **github.com/golangci/misspell**                        950f5d19e770 **_new_**
* **github.com/golangci/prealloc**                        215b22d4de21 **_new_**
* **github.com/golangci/revgrep**                         d9c87f5ffaf0 **_new_**
* **github.com/golangci/unconvert**                       28b1c447d1f4 **_new_**
* **github.com/google/gofuzz**                            44d81051d367 -> v1.0.0
* **github.com/gorilla/mux**                              v1.3.0 -> v1.7.3
* **github.com/gostaticanalysis/analysisutil**            4088753ea4d3 **_new_**
* **github.com/hashicorp/errwrap**                        7554cd9344ce -> v1.0.0
* **github.com/hashicorp/go-multierror**                  83588e72410a -> v1.0.0
* **github.com/hashicorp/go-version**                     v1.2.0 **_new_**
* **github.com/hashicorp/hcl**                            v1.0.0 **_new_**
* **github.com/imdario/mergo**                            0.2.2 -> v0.3.7
* **github.com/inconshreveable/mousetrap**                v1.0.0 **_new_**
* **github.com/ishidawataru/sctp**                        07191f837fed -> 6e2cb1366111
* **github.com/json-iterator/go**                         f2b4162afba3 -> v1.1.8
* **github.com/kisielk/gotool**                           v1.0.0 **_new_**
* **github.com/klauspost/compress**                       v1.4.1 -> v1.8.1
* **github.com/klauspost/cpuid**                          v1.2.0 -> v1.2.1
* **github.com/konsorten/go-windows-terminal-sequences**  v1.0.2 **_new_**
* **github.com/kr/pty**                                   v1.0.0 -> v1.1.8
* **github.com/magiconair/properties**                    v1.8.0 **_new_**
* **github.com/matoous/godox**                            5d6d842e92eb **_new_**
* **github.com/mattn/go-colorable**                       v0.1.4 **_new_**
* **github.com/mattn/go-isatty**                          v0.0.4 -> v0.0.8
* **github.com/mattn/go-shellwords**                      v1.0.6 **_new_**
* **github.com/matttproud/golang_protobuf_extensions**    fc2b8d3a73c4 -> v1.0.1
* **github.com/mitchellh/go-homedir**                     v1.1.0 **_new_**
* **github.com/mitchellh/mapstructure**                   v1.1.2 **_new_**
* **github.com/modern-go/reflect2**                       05fbef0ca5da -> v1.0.1
* **github.com/morikuni/aec**                             v1.0.0 **_new_**
* **github.com/nbutton23/zxcvbn-go**                      ae427f1e4c1d **_new_**
* **github.com/onsi/ginkgo**                              v1.7.0 -> v1.10.3
* **github.com/onsi/gomega**                              v1.4.3 -> v1.7.1
* **github.com/opencontainers/image-spec**                v1.0.0 -> 775207bd45b6
* **github.com/opencontainers/runc**                      11fc498ffa5c -> dd075602f158
* **github.com/opencontainers/runtime-spec**              eba862dc2470 -> a950415649c7
* **github.com/opencontainers/runtime-tools**             1c243a8a8eb4 -> v0.9.0
* **github.com/opencontainers/selinux**                   v1.2 -> v1.3.0
* **github.com/openshift/api**                            27fb16909b15 **_new_**
* **github.com/openshift/imagebuilder**                   705fe9255c57 -> v1.1.1
* **github.com/opentracing/opentracing-go**               25a84ff92183 -> v1.1.0
* **github.com/ostreedev/ostree-go**                      d0388bd827cf -> 759a8c1ac913
* **github.com/pelletier/go-toml**                        v1.2.0 **_new_**
* **github.com/pkg/errors**                               v0.8.0 -> v0.8.1
* **github.com/pquerna/ffjson**                           d49c2bc1aa13 -> dac163c6c0a9
* **github.com/prometheus/client_golang**                 e7e903064f5e -> v1.1.0
* **github.com/prometheus/client_model**                  fa8ad6fec335 -> fd36f4220a90
* **github.com/prometheus/common**                        13ba4ddd0caa -> v0.6.0
* **github.com/prometheus/procfs**                        65c1f6f8f0fc -> v0.0.3
* **github.com/russross/blackfriday**                     v1.5.2 **_new_**
* **github.com/seccomp/containers-golang**                v0.1 -> 8ca8945ccf5f
* **github.com/seccomp/libseccomp-golang**                v0.9.0 -> v0.9.1
* **github.com/securego/gosec**                           e680875ea14d **_new_**
* **github.com/sirupsen/logrus**                          v1.0.0 -> v1.4.2
* **github.com/sourcegraph/go-diff**                      v0.5.1 **_new_**
* **github.com/spf13/afero**                              v1.2.2 **_new_**
* **github.com/spf13/cast**                               v1.3.0 **_new_**
* **github.com/spf13/cobra**                              v0.0.3 -> v0.0.5
* **github.com/spf13/jwalterweatherman**                  v1.0.0 **_new_**
* **github.com/spf13/pflag**                              v1.0.1 -> v1.0.5
* **github.com/spf13/viper**                              v1.4.0 **_new_**
* **github.com/stretchr/objx**                            v0.2.0 **_new_**
* **github.com/stretchr/testify**                         v1.4.0 **_new_**
* **github.com/syndtr/gocapability**                      e7cb7fa329f4 -> d98352740cb2
* **github.com/tchap/go-patricia**                        v2.2.6 -> v2.3.0
* **github.com/timakin/bodyclose**                        f7f2e9bca95e **_new_**
* **github.com/ulikunitz/xz**                             v0.5.4 -> v0.5.6
* **github.com/ultraware/funlen**                         v0.0.2 **_new_**
* **github.com/ultraware/whitespace**                     v0.0.4 **_new_**
* **github.com/uudashr/gocognit**                         1655d0de0517 **_new_**
* **github.com/vbatts/git-validation**                    v1.0.0 **_new_**
* **github.com/vbatts/tar-split**                         v0.10.2 -> v0.11.1
* **github.com/vishvananda/netns**                        13995c7128cc -> 7109fa855b0f
* **github.com/xeipuuv/gojsonpointer**                    4e3ac2762d5f -> df4f5c81cb3b
* **golang.org/x/crypto**                                 49796115aa4b -> a832865fa7ad
* **golang.org/x/net**                                    e147a9138326 -> aa69164e4478
* **golang.org/x/oauth2**                                 a6bd8cefa181 -> 0f29369cfe45
* **golang.org/x/sync**                                   42b317875d0f -> 112230192c58
* **golang.org/x/sys**                                    c8c8c57fd1e1 -> 0a153f010e69
* **golang.org/x/text**                                   17bcc049122f -> v0.3.2
* **golang.org/x/time**                                   f51c12702a4d -> c4c64cad1fd0
* **golang.org/x/tools**                                  0337d82405ff **_new_**
* **google.golang.org/appengine**                         v1.6.1 **_new_**
* **google.golang.org/genproto**                          09f6ed296fc6 -> 6af8c5fc6601
* **gopkg.in/fsnotify.v1**                                v1.4.2 -> v1.4.7
* **gopkg.in/inf.v0**                                     v0.9.0 -> v0.9.1
* **gopkg.in/yaml.v2**                                    v2.2.1 -> v2.2.5
* **honnef.co/go/tools**                                  v0.0.1-2019.2.3 **_new_**
* **k8s.io/api**                                          kubernetes-1.14.0 -> 40a48860b5ab
* **k8s.io/apiextensions-apiserver**                      kubernetes-1.14.0 -> 53c4693659ed
* **k8s.io/apimachinery**                                 kubernetes-1.14.0 -> d7deff9243b1
* **k8s.io/apiserver**                                    kubernetes-1.14.0 -> 8b27c41bdbb1
* **k8s.io/client-go**                                    kubernetes-1.14.0 -> v11.0.0
* **k8s.io/cloud-provider**                               kubernetes-1.14.0 -> c892ea32361a
* **k8s.io/utils**                                        c2654d5206da -> c55fbcfc754a
* **mvdan.cc/interfacer**                                 c20040233aed **_new_**
* **mvdan.cc/lint**                                       adc824a0674b **_new_**
* **mvdan.cc/unparam**                                    d51796306d8f **_new_**
* **sourcegraph.com/sqs/pbtypes**                         d3ebe8f20ae4 **_new_**
