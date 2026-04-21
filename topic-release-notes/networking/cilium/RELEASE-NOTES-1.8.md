# cilium v1.8 Release Notes

Source: [v1.8.13](https://github.com/cilium/cilium/releases/tag/v1.8.13)

We are pleased to release Cilium v1.8.13. This release bumps Istio integration to v1.10.4, fixes some reported bugs and updates the underlying base images for the latest library dependencies. For more details, see the full release notes below.

Summary of Changes
------------------

**Minor Changes:**
* Istio integration is updated to Istio release 1.10.4. (Backport PR #17388, Upstream PRs #14621, #14271, #14704, #17417, @jrajahalme)

**Bugfixes:**
* Set right User Agent in Kubernetes client for all Cilium components. (Backport PR #17533, Upstream PR #17417, @aanm)

**CI Changes:**
* [v1.8] fix MLH config trigger (#17421, @nbusseneau)
* test: bump coredns version to 1.7.0 (Backport PR #17533, Upstream PR #17489, @aanm)
* test: Skip Istio test on k8s <1.17 (Backport PR #17388, Upstream PR #17445, @jrajahalme)

**Misc Changes:**
* build(deps): bump actions/checkout from 2.3.4 to 2.3.5 (#17633, @dependabot[bot])
* build(deps): bump actions/checkout from 2.3.5 to 2.4.0 (#17768, @dependabot[bot])
* build(deps): bump docker/setup-buildx-action from 1.5.1 to 1.6.0 (#17323, @dependabot[bot])
* contrib/backporting: add environment variables to set ORG and REPO (Backport PR #17533, Upstream PR #17424, @aanm)
* docs: clarify language on libceph and kernel 5.8 in kubeproxy-free GSG (Backport PR #17533, Upstream PR #16969, @bluikko)
* docs: Fix command for overwriting iptables on kube-proxy replacement install (Backport PR #17533, Upstream PR #16264, @Stijn98s)
* jenkinsfiles: Don't display nulls in current build display name (Backport PR #17388, Upstream PR #17258, @twpayne)
* v1.8: .github: Remove conformance test from lint workflow (#17334, @joestringer)
* v1.8: Update Cilium base images (#17800, @joestringer)
* vendor: update mongo-driver to 1.5.1 to fix CVE-2021-20329 (Backport PR #17388, Upstream PR #17234, @aanm)

**Other Changes:**
* install: Update image digests for v1.8.12 (#17296, @joestringer)

## Docker Manifests                                                             
                                                                                
### cilium                                                                      
                                                                                
`docker.io/cilium/cilium:v1.8.13@sha256:070a57faa72ca55b045861453a2f1697e4d582a75cf2b24937e0397684abcb3f`
`quay.io/cilium/cilium:v1.8.13@sha256:070a57faa72ca55b045861453a2f1697e4d582a75cf2b24937e0397684abcb3f`
                                                                                
### docker-plugin                                                               
                                                                                
`docker.io/cilium/docker-plugin:v1.8.13@sha256:6ee38e8d87a3e41f175163cbc093ff10061db3864d9a483cb75a9937c3ca506d`
`quay.io/cilium/docker-plugin:v1.8.13@sha256:6ee38e8d87a3e41f175163cbc093ff10061db3864d9a483cb75a9937c3ca506d`
                                                                                
### hubble-relay                                                                
                                                                                
`docker.io/cilium/hubble-relay:v1.8.13@sha256:ddb57b1f0cb5953bb090853f72334a11a59d1732f685baac191dea0ff2acefd0`
`quay.io/cilium/hubble-relay:v1.8.13@sha256:ddb57b1f0cb5953bb090853f72334a11a59d1732f685baac191dea0ff2acefd0`
                                                                                
### operator-aws                                                                
                                                                                
`docker.io/cilium/operator-aws:v1.8.13@sha256:1829d3cbcbf7541a6960f6cea7991fd4a55e921936a9d67636928f6481070162`
`quay.io/cilium/operator-aws:v1.8.13@sha256:1829d3cbcbf7541a6960f6cea7991fd4a55e921936a9d67636928f6481070162`
                                                                                
### operator-azure                                                              
                                                                                
`docker.io/cilium/operator-azure:v1.8.13@sha256:3e8c511cf17791b37f90afe502d39e62d4cfa7c891fa76c7347317a58d5e0652`
`quay.io/cilium/operator-azure:v1.8.13@sha256:3e8c511cf17791b37f90afe502d39e62d4cfa7c891fa76c7347317a58d5e0652`
                                                                                
### operator-generic                                                            
                                                                                
`docker.io/cilium/operator-generic:v1.8.13@sha256:9e6677599565637d479886d038c366b40ce4acded54ab7bca1c7ad660b0c0a83`
`quay.io/cilium/operator-generic:v1.8.13@sha256:9e6677599565637d479886d038c366b40ce4acded54ab7bca1c7ad660b0c0a83`
                                                                                
### operator                                                                    
                                                                                
`docker.io/cilium/operator:v1.8.13@sha256:4e865ab71494c27df6c6f4f1ba113bdcdfa1313e2d69c4cfe54eed7fa13bde14`
`quay.io/cilium/operator:v1.8.13@sha256:4e865ab71494c27df6c6f4f1ba113bdcdfa1313e2d69c4cfe54eed7fa13bde14`