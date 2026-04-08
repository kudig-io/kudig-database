# flux v2.6 Release Notes

Source: [v2.6.4](https://github.com/fluxcd/flux2/releases/tag/v2.6.4)

## Highlights

Flux v2.6.4 is a patch release that comes with various fixes. Users are encouraged to upgrade for the best experience.

Fixes:

- Fix for SOPS decryption with US Government KMS keys failing with the error:

```
STS: AssumeRoleWithWebIdentity, https response error\n   StatusCode: 0, RequestID: ,
request send failed, Post\n \"https://sts.arn.amazonaws.com/\": dial tcp:
lookupts.arn.amazonaws.com on 10.100.0.10:53: no such host
```

## Components changelog

- kustomize-controller [v1.6.1](https://github.com/fluxcd/kustomize-controller/blob/v1.6.1/CHANGELOG.md)

## CLI changed
* [release/v2.6.x] Update toolkit components by @fluxcdbot in https://github.com/fluxcd/flux2/pull/5444


**Full Changelog**: https://github.com/fluxcd/flux2/compare/v2.6.3...v2.6.4

