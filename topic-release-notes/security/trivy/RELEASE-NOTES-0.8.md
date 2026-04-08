# trivy v0.8 Release Notes

Source: [v0.8.0](https://github.com/aquasecurity/trivy/releases/tag/v0.8.0)

## New Feature
### Add image subcommand (#493)
We deprecated `$ trivy IMAGE_NAME` and introduced `image` subcommand.

```
$ trivy image alpine:3.11
```

### Add CVSS Vectors to JSON output. (#484)
You can see CVSS vectors in a result JSON.

```
$ trivy image --format=json alpine=3.10.4
[...output snipped...]
        "VendorVectors": {
          "nvd": {
            "v2": "AV:N/AC:L/Au:N/C:N/I:N/A:P",
            "v3": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H"
          },
          "redhat": {
            "v3": "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H"
          }
        },
[...output snipped...]
```

### Support registry token (#482)
To scan a private image, you can pass a registry token instead of ID/PW. This is useful when you develop a registry integration such as Harbor and Quay.

```
$ export TRIVY_REGISTRY_TOKEN=$(curl -u "username:password" "https://auth.docker.io/token?service=registry.docker.io&scope=repository:org/private_image:pull")
$ trivy org/private_image:latest
```


## Changelog

78b7529 Add image subcommand (#493)
e2bcb44 fix: remove help template (#500)
a57c27e vulnerability: Add CVSS Vectors to JSON output. (#484)
926f323 feat: support registry token (#482)
aa20adb chore: bump up urfave/cli to v2 (#499)
3e0779a chore(doc): update README (#490)



## Docker images

- `docker pull docker.io/aquasec/trivy:0.8.0`
- `docker pull docker.io/aquasec/trivy:latest`
