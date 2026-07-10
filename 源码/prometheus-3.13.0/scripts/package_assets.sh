#!/usr/bin/env bash
#
# compress static assets

set -euo pipefail

version="$(< VERSION)"
mkdir -p .tarballs
cd 站点/ui
find static -type f -not -name '*.gz' -print0 | xargs -0 tar czf ../../.tarballs/prometheus-web-ui-${version}.tar.gz
