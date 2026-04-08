# falco v0.35 Release Notes

Source: [0.35.1](https://github.com/falcosecurity/falco/releases/tag/0.35.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.35.1-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.35.1-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.35.1-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.35.1-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.35.1-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.35.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.35.1-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.35.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.35.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.35.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.35.1`                 |

### Major Changes


### Minor Changes

* update(userspace): change description of snaplen option stating only performance implications [[#2634](https://github.com/falcosecurity/falco/pull/2634)] - [@loresuso](https://github.com/loresuso)
* update(cmake): bump libs to 0.11.3 [[#2662](https://github.com/falcosecurity/falco/pull/2662)] - [@jasondellaluce](https://github.com/jasondellaluce)
* cleanup(config): minor config clarifications [[#2651](https://github.com/falcosecurity/falco/pull/2651)] - [@incertum](https://github.com/incertum)
* update(cmake): bump falco rules to v1.0.1 [[#2648](https://github.com/falcosecurity/falco/pull/2648)] - [@jasondellaluce](https://github.com/jasondellaluce)
* chore(userspace/falco): make source matching error more expressive [[#2623](https://github.com/falcosecurity/falco/pull/2623)] - [@jasondellaluce](https://github.com/jasondellaluce)
* update(.github): integrate Go regression tests [[#2437](https://github.com/falcosecurity/falco/pull/2437)] - [@jasondellaluce](https://github.com/jasondellaluce)


### Bug Fixes

* fix(scripts): fixed falco-driver-loader to manage debian kernel rt and cloud flavors. [[#2627](https://github.com/falcosecurity/falco/pull/2627)] - [@FedeDP](https://github.com/FedeDP)
* fix(userspace/falco): solve live multi-source issues when loading more than two sources [[#2653](https://github.com/falcosecurity/falco/pull/2653)] - [@jasondellaluce](https://github.com/jasondellaluce)
* fix(driver-loader): fix ubuntu kernel version parsing [[#2635](https://github.com/falcosecurity/falco/pull/2635)] - [@therealbobo](https://github.com/therealbobo)
* fix(userspace): switch to timer_settime API for stats writer. [[#2646](https://github.com/falcosecurity/falco/pull/2646)] - [@FedeDP](https://github.com/FedeDP)



### Non user-facing changes

* CI: bump ubuntu version for tests-driver-loader-integration job [[#2661](https://github.com/falcosecurity/falco/pull/2661)] - [@Andreagit97](https://github.com/Andreagit97)

#### Release Manager @jasondellaluce