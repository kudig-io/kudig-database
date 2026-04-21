# trivy v0.2 Release Notes

Source: [v0.2.1](https://github.com/aquasecurity/trivy/releases/tag/v0.2.1)

## Changes
- Support GITHUB_TOKEN for rate limiting
- Ignore files under vendor dir to avoid false positives
- New logo

## Changelog

35429e3 chore(logo): replace with new logo (#269)
fb26541 chore(clear-cache): add an explanation (#276)
15af65b feat(github): add GITHUB_TOKEN for rate limiting (#281)
c2fdfab fix(lockfile): ignore files under vendor dir (#279)


## Docker images

- `docker pull docker.io/aquasec/trivy:0.2.1`
- `docker pull docker.io/aquasec/trivy:latest`
