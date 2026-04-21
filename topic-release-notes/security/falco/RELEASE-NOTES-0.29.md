# falco v0.29 Release Notes

Source: [0.29.1](https://github.com/falcosecurity/falco/releases/tag/0.29.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.29.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.29.1-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.29.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.29.1-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.29.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.29.1-x86_64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.29.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.29.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.29.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.29.1`                 |

### Minor Changes

* update: bump the Falco engine version to version 9 [[#1675](https://github.com/falcosecurity/falco/pull/1675)] - [@leodido](https://github.com/leodido)

### Rule Changes

* rule(list user_known_userfaultfd_processes): list to exclude processes known to use userfaultfd syscall [[#1675](https://github.com/falcosecurity/falco/pull/1675)] - [@leodido](https://github.com/leodido)
* rule(macro consider_userfaultfd_activities): macro to gate the "Unprivileged Delegation of Page Faults Handling to a Userspace Process" rule [[#1675](https://github.com/falcosecurity/falco/pull/1675)] - [@leodido](https://github.com/leodido)
* rule(Unprivileged Delegation of Page Faults Handling to a Userspace Process): new rule to detect successful unprivileged userfaultfd syscalls [[#1675](https://github.com/falcosecurity/falco/pull/1675)] - [@leodido](https://github.com/leodido)
* rule(Linux Kernel Module Injection Detected): adding container info to the output of the rule [[#1675](https://github.com/falcosecurity/falco/pull/1675)] - [@leodido](https://github.com/leodido)

### Non user-facing changes

* docs(release.md): update steps [[#1684](https://github.com/falcosecurity/falco/pull/1684)] - [@maxgio92](https://github.com/maxgio92)

### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 2      |
| Release note    | 1      |
| Total           | 3      |


#### Release Manager [@leodido](https://github.com/leodido)