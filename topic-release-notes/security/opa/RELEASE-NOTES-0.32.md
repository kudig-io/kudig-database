# opa v0.32 Release Notes

Source: [v0.32.1](https://github.com/open-policy-agent/opa/releases/tag/v0.32.1)

This is a bugfix release to address a problem related to mismatching checksums in the official go mod proxy.
As a consequence, users with code depending on the OPA Go module that bypassed the proxy would see an error like

    go get github.com/google/flatbuffers/go: github.com/google/flatbuffers@v1.12.0: verifying module: checksum mismatch
        downloaded: h1:N8EguYFm2wwdpoNcpchQY0tPs85vOJkboFb2dPxmixo=
        sum.golang.org: h1:/PtAHvnBY4Kqnx/xCQ3OIV9uYcSFGScBsWI3Oogeh6w=

**Be aware** that Github's Dependabot feature makes use of that check, and will start to _fail_ for projects using the OPA Go module version 0.32.0.

There workaround applied to OPA is to replace to flatbuffers dependency's version manually.

For more information, see
- https://github.com/google/flatbuffers/issues/6466: The issue has been discussed upstream, and a 1.12.1 release has been published to address it.
- https://github.com/dgraph-io/badger/pull/1746: OPA transitively depends on the flatbuffer package because of badger.

There are *no functional changes* in this bugfix release.
If you use the container images, or the published binaries, of OPA 0.32.0, you are **not affected** by this.

Many thanks to [James Alseth](https://github.com/jalseth) for triaging this, and engaging with upstream to fix this.