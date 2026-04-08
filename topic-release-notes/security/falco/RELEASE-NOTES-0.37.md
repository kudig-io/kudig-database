# falco v0.37 Release Notes

Source: [0.37.1](https://github.com/falcosecurity/falco/releases/tag/0.37.1)

[![LIBS](https://img.shields.io/badge/LIBS-0.14.3-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.14.3)
[![DRIVER](https://img.shields.io/badge/DRIVER-7.0.0-yellow)](https://github.com/falcosecurity/libs/releases/tag/7.0.0+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.37.1-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.37.1-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.37.1-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.37.1-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.37.1-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.37.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.37.1-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.37.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.37.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.37.1`             |
| `docker pull docker.io/falcosecurity/falco-driver-loader-legacy:0.37.1`      |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.37.1`                 |
| `docker pull docker.io/falcosecurity/falco-distroless:0.37.1`                |

## v0.37.1

Released on 2024-02-13

### Major Changes


* new(docker): added option for insecure http driver download to falco and driver-loader images [[#3058](https://github.com/falcosecurity/falco/pull/3058)] - [@toamto94](https://github.com/toamto94)


### Minor Changes

* update(cmake): bumped falcoctl to v0.7.2 [[#3076](https://github.com/falcosecurity/falco/pull/3076)] - [@FedeDP](https://github.com/FedeDP)
* update(build): link libelf dynamically [[#3048](https://github.com/falcosecurity/falco/pull/3048)] - [@LucaGuerra](https://github.com/LucaGuerra)


### Bug Fixes

* fix(userspace/engine): always consider all rules (even the ones below min_prio) in m_rule_stats_manager [[#3060](https://github.com/falcosecurity/falco/pull/3060)] - [@FedeDP](https://github.com/FedeDP)



### Non user-facing changes

* sync(docs): cherrypick CHANGELOG entry for 0.37.1 [[#3080](https://github.com/falcosecurity/falco/pull/3080)] - [@FedeDP](https://github.com/FedeDP)
* Added http headers option for driver download in docker images [[#3075](https://github.com/falcosecurity/falco/pull/3075)] - [@toamto94](https://github.com/toamto94)
* fix(build): install libstdc++ in the Wolfi image [[#3053](https://github.com/falcosecurity/falco/pull/3053)] - [@LucaGuerra](https://github.com/LucaGuerra)

### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |      3 |
| Release note    |      4 |
| Total           |      7 |

#### Release Manager @FedeDP
