# runc v1.4 Release Notes

Source: [v1.4.2](https://github.com/opencontainers/runc/releases/tag/v1.4.2)

This is the second patch release of the 1.4.z release series of runc.

### Fixed ###

- A regression in runc v1.3.0 which can result in a stuck `runc exec` or
  `runc run` when the container process runs for a short time. (#5208,
  #5210, #5216)

- Mount sources that need to be open on the host are now closed earlier during
  container start, reducing the total amount of used file descriptors and
  helping to avoid hitting the open files limit when handling many such mounts.
  (#5177, #5201)

### Static Linking Notices ###

The `runc` binary distributed with this release are *statically linked* with
the following [GNU LGPL-2.1][lgpl-2.1] licensed libraries, with `runc` acting
as a "work that uses the Library":

[lgpl-2.1]: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html

  - [libseccomp](https://github.com/seccomp/libseccomp)

The versions of these libraries were not modified from their upstream versions,
but in order to comply with the LGPL-2.1 (&sect;6(a)), we have attached the
complete source code for those libraries which (when combined with the attached
runc source code) may be used to exercise your rights under the LGPL-2.1.

However we strongly suggest that you make use of your distribution's packages
or download them from the authoritative upstream sources, especially since
these libraries are related to the security of your containers.

----
Thanks to the following contributors for making this release possible:

 * Ayato Tokubi <atokubi@redhat.com>
 * Akihiro Suda <akihiro.suda.cz@hco.ntt.co.jp>
 * Aleksa Sarai <cyphar@cyphar.com>
 * Kir Kolyshkin <kolyshkin@gmail.com>
 * Li Fubang <lifubang@acmcoder.com>
 * Rodrigo Campos Catelin <rodrigo@amutable.com>

Signed-off-by: Kir Kolyshkin <kolyshkin@gmail.com> 