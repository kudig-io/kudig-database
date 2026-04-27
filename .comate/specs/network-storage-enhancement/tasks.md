# 网络与存储故障排查内容全面加强 - 任务计划

- [x] Task 1: Create Terway Troubleshooting Document
    - 1.1: Research Terway architecture (ENI/Veth/IPVlan modes, IPAM, Trunk ENI)
    - 1.2: Draft 03-networking/07-terway-troubleshooting.md following Four-Element Methodology
    - 1.3: Include terway-cli commands, node annotations, security group debugging
    - 1.4: Add prevention, monitoring alerts, and best practices sections

- [x] Task 2: Create Flannel Troubleshooting Document
    - 2.1: Research Flannel backends (VXLAN, host-gw, UDP, cloud VPC extensions)
    - 2.2: Draft 03-networking/08-flannel-troubleshooting.md following Four-Element Methodology
    - 2.3: Cover subnet allocation, etcd vs Kubernetes API backend, MTU, cross-node issues
    - 2.4: Add upgrade/migration guidance and Canal (Flannel+Calico) coexistence

- [x] Task 3: Create StorageClass Troubleshooting Document
    - 3.1: Research StorageClass parameters across major CSI drivers (AWS, AliCloud, GCP, local)
    - 3.2: Draft 04-storage/05-storageclass-troubleshooting.md following Four-Element Methodology
    - 3.3: Cover volumeBindingMode, allowVolumeExpansion, default class conflicts, topology
    - 3.4: Add cloud-provider-specific parameter tables and performance tier guidance

- [x] Task 4: Update Cross-References in Existing Documents
    - 4.1: Add link to new Flannel doc in 03-networking/01-cni-troubleshooting.md
    - 4.2: Add link to new StorageClass doc in 04-storage/01-pv-pvc-troubleshooting.md

- [x] Task 5: Update README.md
    - 5.1: Add 07-terway and 08-flannel to 03-networking directory table
    - 5.2: Add 05-storageclass to 04-storage directory table
    - 5.3: Update document count (60 -> 63)
    - 5.4: Add new entries to "by symptom" quick lookup
    - 5.5: Add new entries to "by component" quick lookup
    - 5.6: Update statistics table counts

