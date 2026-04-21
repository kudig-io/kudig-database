# rook v1.17 Release Notes

Source: [v1.17.9](https://github.com/rook/rook/releases/tag/v1.17.9)

# Improvements
Rook v1.17.9 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- pool: Retry pool status updates in the radosnamespace controller (#16700, @parth-gr)
- object: Fix user quotas being overwritten when OBC bucketOwner is set (#16672, @jhoblitt)
- mon: Wait for the canary pods to be terminated (#16619, @sp98)
- mon: Respond quickly to the mon canary pod deletion (#16629, @travisn)
- namespace: Blocklist `ip:nonce` in cleanup job (#16532, @Madhu-1)
- core: Fix typos in ObjectZoneSpec.ZoneGroup and ObjectZoneGroupSpec.Realm field descriptions (#16496, @jhoblitt)
