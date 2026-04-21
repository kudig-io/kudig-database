# falco v0.30 Release Notes

Source: [0.30.0](https://github.com/falcosecurity/falco/releases/tag/0.30.0)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.30.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.30.0-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.30.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.30.0-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.30.0-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.30.0-x86_64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.30.0`                          |
| `docker pull public.ecr.aws/falcosecurity/falco:0.30.0`                     |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.30.0`            |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.30.0`                |

### Major Changes

* new: add `--k8s-node` command-line options, which allows filtering by a node when requesting metadata of pods to the K8s API server [[#1671](https://github.com/falcosecurity/falco/pull/1671)] - [@leogr](https://github.com/leogr)
* new(outputs): expose rule tags and event source in gRPC and json outputs [[#1714](https://github.com/falcosecurity/falco/pull/1714)] - [@jasondellaluce](https://github.com/jasondellaluce)
* new(userspace/falco): add customizable metadata fetching params [[#1667](https://github.com/falcosecurity/falco/pull/1667)] - [@zuc](https://github.com/zuc)


### Minor Changes

* update: bump driver version to 3aa7a83bf7b9e6229a3824e3fd1f4452d1e95cb4 [[#1744](https://github.com/falcosecurity/falco/pull/1744)] - [@zuc](https://github.com/zuc)
* docs: clarify that previous Falco drivers will remain available at https://download.falco.org and no automated cleanup is run anymore [[#1738](https://github.com/falcosecurity/falco/pull/1738)] - [@leodido](https://github.com/leodido)
* update(outputs): add configuration option for tags in json outputs [[#1733](https://github.com/falcosecurity/falco/pull/1733)] - [@jasondellaluce](https://github.com/jasondellaluce)


### Bug Fixes

* fix(scripts): correct standard output redirection in systemd config (DEB and RPM packages) [[#1697](https://github.com/falcosecurity/falco/pull/1697)] - [@chirabino](https://github.com/chirabino)
* fix(scripts): correct lookup order when trying multiple `gcc` versions in the `falco-driver-loader` script [[#1716](https://github.com/falcosecurity/falco/pull/1716)] - [@Spartan-65](https://github.com/Spartan-65)


### Rule Changes

* rule(list miner_domains): add new miner domains [[#1729](https://github.com/falcosecurity/falco/pull/1729)] - [@AlbertoPellitteri](https://github.com/AlbertoPellitteri)
* rule(list https_miner_domains): add new miner domains [[#1729](https://github.com/falcosecurity/falco/pull/1729)] - [@AlbertoPellitteri](https://github.com/AlbertoPellitteri)


### Non user-facing changes

* add Qonto as adopter [[#1717](https://github.com/falcosecurity/falco/pull/1717)] - [@Issif](https://github.com/Issif)
* docs(proposals): proposal for a libs plugin system [[#1637](https://github.com/falcosecurity/falco/pull/1637)] - [@ldegio](https://github.com/ldegio)
* build: remove unused `ncurses` dependency [[#1658](https://github.com/falcosecurity/falco/pull/1658)] - [@leogr](https://github.com/leogr)
* build(.circleci): use new Debian 11 package names for python-pip [[#1712](https://github.com/falcosecurity/falco/pull/1712)] - [@zuc](https://github.com/zuc)
* build(docker): adding libssl-dev, upstream image reference pinned to `debian:buster` [[#1719](https://github.com/falcosecurity/falco/pull/1719)] - [@michalschott](https://github.com/michalschott)
* fix(test): avoid output_strictly_contains failures [[#1724](https://github.com/falcosecurity/falco/pull/1724)] - [@jasondellaluce](https://github.com/jasondellaluce)
* Remove duplicate allowed ecr registry rule [[#1725](https://github.com/falcosecurity/falco/pull/1725)] - [@TomKeyte](https://github.com/TomKeyte)
* docs(RELEASE.md): switch to 3 releases per year [[#1711](https://github.com/falcosecurity/falco/pull/1711)] - [@leogr](https://github.com/leogr)

### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 10     |
| Release note    | 9      |
| Total           | 19     |


#### Release Manager @araujof