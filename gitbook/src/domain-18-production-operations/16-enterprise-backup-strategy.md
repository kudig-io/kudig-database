# 16-企业级备份策略

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

企业级备份策略是保障业务连续性的核心要素。本文档详细介绍Kubernetes环境下的完整备份解决方案，包括etcd备份、应用数据备份和恢复演练。

## 🗃️ 备份架构设计

### 分层备份策略

#### 1. etcd备份配置
```yaml
# etcd备份控制器
apiVersion: apps/v1
kind: Deployment
metadata:
  name: etcd-backup-controller
  namespace: backup-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: etcd-backup-controller
  template:
    metadata:
      labels:
        app: etcd-backup-controller
    spec:
      containers:
      - name: backup-controller
        image: custom/etcd-backup-controller:latest
        env:
        - name: ETCD_ENDPOINTS
          value: "https://etcd-client:2379"
        - name: BACKUP_STORAGE_CLASS
          value: "backup-storage"
        - name: RETENTION_DAYS
          value: "30"
        - name: BACKUP_SCHEDULE
          value: "0 2 * * *"
        volumeMounts:
        - name: etcd-certs
          mountPath: /etc/etcd/ssl
          readOnly: true
        - name: backup-storage
          mountPath: /backups
      volumes:
      - name: etcd-certs
        secret:
          secretName: etcd-certs
      - name: backup-storage
        persistentVolumeClaim:
          claimName: etcd-backup-pvc
```

#### 2. 应用数据备份
```yaml
# Velero备份配置
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: daily-application-backup
  namespace: velero
spec:
  includedNamespaces:
  - production
  - staging
  excludedNamespaces:
  - kube-system
  includedResources:
  - deployments
  - statefulsets
  - configmaps
  - secrets
  - persistentvolumeclaims
  excludedResources:
  - events
  - nodes
  snapshotVolumes: true
  storageLocation: default
  ttl: 168h
  hooks:
    resources:
    - name: mysql-pre-backup
      includedNamespaces:
      - production
      includedResources:
      - pods
      labelSelector:
        matchLabels:
          app: mysql
      hooks:
      - exec:
          container: mysql
          command:
          - /bin/sh
          - -c
          - mysqldump -u root --all-databases > /tmp/backup.sql
          onError: Fail
          timeout: 300s
```

### 备份策略管理

#### 1. 备份策略定义
```yaml
# 备份策略CRD
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: backuppolicies.backup.example.com
spec:
  group: backup.example.com
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              backupTargets:
                type: array
                items:
                  type: object
                  properties:
                    name:
                      type: string
                    type:
                      type: string
                      enum: [etcd, applications, volumes, databases]
                    schedule:
                      type: string
                    retention:
                      type: object
                      properties:
                        daily:
                          type: integer
                        weekly:
                          type: integer
                        monthly:
                          type: integer
                    encryption:
                      type: boolean
                    compression:
                      type: boolean
  scope: Namespaced
  names:
    plural: backuppolicies
    singular: backuppolicy
    kind: BackupPolicy
```

#### 2. 备份控制器实现
```python
#!/usr/bin/env python3
# 备份策略控制器

import asyncio
from kubernetes import client, config
import json
from datetime import datetime
import subprocess
import boto3

class BackupController:
    def __init__(self):
        config.load_kube_config()
        self.core_v1 = client.CoreV1Api()
        self.custom_objects = client.CustomObjectsApi()
        
        self.backup_config = {
            'storage_class': 'backup-storage',
            'retention_days': 30,
            's3_bucket': 'company-backups',
            's3_region': 'us-west-2'
        }
        
        self.s3_client = boto3.client('s3', region_name=self.backup_config['s3_region'])
    
    async def run_backup_cycle(self):
        """运行备份周期"""
        while True:
            try:
                policies = self.custom_objects.list_namespaced_custom_object(
                    group='backup.example.com',
                    version='v1',
                    namespace='backup-system',
                    plural='backuppolicies'
                )
                
                for policy in policies['items']:
                    await self.execute_backup_policy(policy)
                
                await asyncio.sleep(3600)
                
            except Exception as e:
                print(f"Error in backup cycle: {e}")
                await asyncio.sleep(300)
    
    async def execute_backup_policy(self, policy):
        """执行备份策略"""
        policy_name = policy['metadata']['name']
        targets = policy['spec']['backupTargets']
        
        print(f"Executing backup policy: {policy_name}")
        
        for target in targets:
            try:
                backup_result = await self.execute_target_backup(target)
                await self.update_policy_status(policy_name, target['name'], backup_result)
                
            except Exception as e:
                print(f"Error executing backup for {target['name']}: {e}")
    
    async def execute_target_backup(self, target):
        """执行目标备份"""
        backup_functions = {
            'etcd': self.backup_etcd,
            'applications': self.backup_applications,
            'volumes': self.backup_volumes,
            'databases': self.backup_databases
        }
        
        backup_func = backup_functions.get(target['type'])
        if not backup_func:
            return {'success': False, 'error': f"Unknown backup type: {target['type']}"}
        
        return await backup_func(target)
    
    async def backup_etcd(self, target):
        """备份etcd"""
        try:
            timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
            backup_name = f"etcd-backup-{timestamp}"
            backup_path = f"/backups/{backup_name}.db"
            
            cmd = [
                'etcdctl',
                '--endpoints=https://etcd-client:2379',
                '--cert=/etc/etcd/ssl/etcd.crt',
                '--key=/etc/etcd/ssl/etcd.key',
                '--cacert=/etc/etcd/ssl/ca.crt',
                'snapshot', 'save', backup_path
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                s3_key = f"etcd/{backup_name}.db"
                self.s3_client.upload_file(backup_path, self.backup_config['s3_bucket'], s3_key)
                subprocess.run(['rm', '-f', backup_path])
                
                return {
                    'success': True,
                    'backup_name': backup_name,
                    's3_location': f"s3://{self.backup_config['s3_bucket']}/{s3_key}"
                }
            else:
                return {'success': False, 'error': result.stderr}
                
        except Exception as e:
            return {'success': False, 'error': str(e)}

# 使用示例
async def main():
    controller = BackupController()
    await controller.run_backup_cycle()

if __name__ == "__main__":
    asyncio.run(main())
```

## 🔧 备份验证和测试

### 自动化验证机制

#### 1. 备份完整性检查
```yaml
# 备份验证Job
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-verification
  namespace: backup-system
spec:
  schedule: "0 6 * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: verifier
            image: custom/backup-verifier:latest
            command:
            - /bin/sh
            - -c
            - |
              /scripts/verify-etcd-backup.sh
              /scripts/verify-app-backup.sh
            volumeMounts:
            - name: scripts
              mountPath: /scripts
            - name: backup-storage
              mountPath: /backups
          volumes:
          - name: scripts
            configMap:
              name: backup-verification-scripts
          - name: backup-storage
            persistentVolumeClaim:
              claimName: etcd-backup-pvc
          restartPolicy: OnFailure
```

#### 2. 恢复测试框架
```python
#!/usr/bin/env python3
# 恢复测试框架

import asyncio
import subprocess
import json
from datetime import datetime
from kubernetes import client, config

class RestoreTester:
    def __init__(self):
        config.load_kube_config()
        self.core_v1 = client.CoreV1Api()
        
        self.test_config = {
            'test_namespace': 'restore-test',
            'test_timeout': 1800
        }
    
    async def run_restore_test(self, backup_name=None):
        """运行恢复测试"""
        test_id = f"restore-test-{datetime.now().strftime('%Y%m%d-%H%M%S')}"
        
        if not backup_name:
            backup_name = await self.get_latest_backup()
        
        test_result = {
            'test_id': test_id,
            'backup_used': backup_name,
            'start_time': datetime.now().isoformat(),
            'final_status': 'pending'
        }
        
        try:
            test_result['setup'] = await self.setup_test_environment(test_id)
            test_result['restore'] = await self.execute_restore(backup_name, test_id)
            test_result['validation'] = await self.validate_restored_environment(test_id)
            test_result['final_status'] = 'passed'
            
        except Exception as e:
            test_result['final_status'] = 'failed'
            test_result['error'] = str(e)
        
        finally:
            test_result['cleanup'] = await self.cleanup_test_environment(test_id)
            test_result['end_time'] = datetime.now().isoformat()
        
        await self.generate_test_report(test_result)
        return test_result
    
    async def get_latest_backup(self):
        """获取最新备份"""
        etcd_backups = subprocess.run(
            ['ls', '-t', '/backups/etcd-backup-*.db'],
            capture_output=True, text=True
        )
        
        if etcd_backups.returncode == 0:
            latest_etcd = etcd_backups.stdout.split('\n')[0]
            return {'type': 'etcd', 'path': latest_etcd}
        
        app_backups = subprocess.run(
            ['velero', 'get', 'backups', '--sort', 'creation-time'],
            capture_output=True, text=True
        )
        
        if app_backups.returncode == 0:
            latest_app = app_backups.stdout.split('\n')[1].split()[0]
            return {'type': 'application', 'name': latest_app}
        
        raise Exception("No backups found")
    
    async def setup_test_environment(self, test_id):
        """准备测试环境"""
        try:
            namespace_body = {
                'apiVersion': 'v1',
                'kind': 'Namespace',
                'metadata': {
                    'name': f"{self.test_config['test_namespace']}-{test_id}"
                }
            }
            
            self.core_v1.create_namespace(body=namespace_body)
            
            return {'status': 'completed', 'namespace': f"{self.test_config['test_namespace']}-{test_id}"}
            
        except Exception as e:
            return {'status': 'failed', 'error': str(e)}

# 使用示例
async def main():
    tester = RestoreTester()
    result = await tester.run_restore_test()
    print(f"Test result: {json.dumps(result, indent=2)}")

if __name__ == "__main__":
    asyncio.run(main())
```

## 📊 备份监控和告警

### 备份状态监控

#### 1. 备份指标收集
```yaml
# 备份监控指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: backup-monitoring
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: backup-controller
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
```

#### 2. 备份告警规则
```yaml
# 备份告警配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: backup-alerts
  namespace: monitoring
spec:
  groups:
  - name: backup.rules
    rules:
    - alert: BackupFailed
      expr: |
        backup_last_status{status="failed"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Backup failed"
        
    - alert: BackupDelayed
      expr: |
        time() - backup_last_success_time > 86400
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Backup delayed"
```

## 🔧 实施检查清单

### 备份策略配置
- [ ] 设计分层备份架构(etcd、应用、数据)
- [ ] 配置自动化备份调度和执行
- [ ] 实施备份加密和压缩
- [ ] 设置合理的备份保留策略
- [ ] 配置多地备份存储
- [ ] 建立备份验证机制

### 恢复能力保障
- [ ] 制定详细的恢复流程文档
- [ ] 定期进行恢复演练
- [ ] 建立恢复时间目标(RTO)和恢复点目标(RPO)
- [ ] 配置恢复测试环境
- [ ] 实施恢复自动化工具
- [ ] 建立恢复验证机制

### 监控和运维
- [ ] 部署备份状态监控系统
- [ ] 配置备份告警和通知
- [ ] 建立备份质量评估体系
- [ ] 实施备份容量规划
- [ ] 定期审查和优化备份策略
- [ ] 维护备份操作手册

---

*本文档为企业级Kubernetes备份策略提供完整的架构设计和实施指导*