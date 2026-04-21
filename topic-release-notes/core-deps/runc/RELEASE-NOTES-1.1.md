# runc v1.1 Release Notes

Source: [v1.1.15](https://github.com/opencontainers/runc/releases/tag/v1.1.15)

This is the fifteenth patch release in the 1.1.z release branch of runc.
It fixes a few issues with seccomp, leaked mounts, and system performance.

 * The `-ENOSYS` seccomp stub is now always generated for the native
   architecture that `runc` is running on. This is needed to work around some
   arguably specification-incompliant behaviour from Docker on architectures
   such as ppc64le, where the allowed architecture list is set to `null`. This
   ensures that we always generate at least one `-ENOSYS` stub for the native
   architecture even with these weird configs. (#4391)
 * On a system with older kernel, reading `/proc/self/mountinfo` may skip some
   entries, as a consequence runc may not properly set mount propagation,
   causing container mounts leak onto the host mount namespace. (#2404, #4425)
 * In order to fix performance issues in the "lightweight" bindfd protection
   against [CVE-2019-5736], the temporary `ro` bind-mount of `/proc/self/exe`
   has been removed. runc now creates a binary copy in all cases. (#4392, #2532)
   
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

<hr/>

Thanks to all of the contributors who made this release possible:

 * Akihiro Suda <akihiro.suda.cz@hco.ntt.co.jp>
 * Aleksa Sarai <cyphar@cyphar.com>
 * Kir Kolyshkin <kolyshkin@gmail.com>
 * lifubang <lifubang@acmcoder.com>
 * Rodrigo Campos <rodrigoca@microsoft.com>
