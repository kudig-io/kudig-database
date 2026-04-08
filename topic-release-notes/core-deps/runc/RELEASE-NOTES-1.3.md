# runc v1.3 Release Notes

Source: [v1.3.5](https://github.com/opencontainers/runc/releases/tag/v1.3.5)

This is the fifth patch release of the 1.3.z release series of runc,
and primarily contains a few fixes for issues found in 1.3.4.

### Fixed
 * Recursive atime-related mount flags (rrelatime et al.) are now applied
   properly. (#5115, #5098)
 * PR #4757 caused a regression that resulted in spurious
   `cannot start a container that has stopped` errors when
   running `runc create` and has thus been reverted. (#5158,
   #5153, #5151, #4645, #4757)

### Changed
 * Updated builds to Go 1.25, libseccomp v2.6.0. (#5111, #5053)
 * Minor signing keyring updates. (#5146, #5139, #5144, #5148)
 
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

 * Aleksa Sarai <cyphar@cyphar.com>
 * Kir Kolyshkin <kolyshkin@gmail.com>
 * Li Fu Bang <lifubang@acmcoder.com>
 * Ricardo Branco <rbranco@suse.de>
