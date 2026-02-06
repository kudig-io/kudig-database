# 34 - 联邦学习与分布式协同训练

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 专家级 | **参考**: [FATE](https://fate.readthedocs.io/) | [FedML](https://fedml.ai/) | [TensorFlow Federated](https://www.tensorflow.org/federated)

## 一、联邦学习架构

### 1.1 联邦学习全景架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         Federated Learning Architecture                             │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            Coordination Layer                                 │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Parameter       │  │ Aggregation     │  │ Security        │               │  │
│  │  │ Server          │  │ Controller      │  │ Orchestrator    │               │  │
│  │  │ (Parameter      │  │ (Federated      │  │ (Secure         │               │  │
│  │  │  Broadcasting)  │  │  Averaging)     │  │  Multi-party)   │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         Participant Network                                   │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Edge Device 1   │  │ Edge Device 2   │  │ Edge Device N   │               │  │
│  │  │ (Local Training)│  │ (Local Training)│  │ (Local Training)│               │  │
│  │  │ Secure Storage  │  │ Secure Storage  │  │ Secure Storage  │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         Security & Privacy Layer                              │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Homomorphic     │  │ Differential    │  │ Secure          │               │  │
│  │  │ Encryption      │  │ Privacy         │  │ Multi-party     │               │  │
│  │  │ (Computations)  │  │ (Noise Adding)  │  │ Computation     │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 联邦学习模式分类

| 模式类型 | 技术特点 | 适用场景 | 优势 | 挑战 |
|----------|----------|----------|------|------|
| **横向联邦** | 样本维度聚合 | 用户重叠、特征不同 | 实现简单、效率高 | 隐私保护有限 |
| **纵向联邦** | 特征维度聚合 | 特征重叠、用户不同 | 保护用户隐私 | 计算复杂度高 |
| **联邦迁移** | 知识迁移学习 | 域间知识共享 | 跨域学习能力强 | 模型适配困难 |
| **联邦强化** | 策略协同优化 | 多智能体协作 | 决策能力提升 | 收敛性保证难 |

---

## 二、FATE联邦学习平台部署

### 2.1 FATE集群架构

```yaml
# fate-cluster-deployment.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: fate-cluster

---
# MySQL数据库（元数据存储）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: fate-mysql
  namespace: fate-cluster
spec:
  serviceName: fate-mysql
  replicas: 1
  selector:
    matchLabels:
      app: fate-mysql
  template:
    metadata:
      labels:
        app: fate-mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: fate-secrets
              key: mysql-root-password
        - name: MYSQL_DATABASE
          value: \"fate_flow\"
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: mysql-storage
          mountPath: /var/lib/mysql
        resources:
          requests:
            cpu: \"1\"
            memory: \"2Gi\"
          limits:
            cpu: \"2\"
            memory: \"4Gi\"
  volumeClaimTemplates:
  - metadata:
      name: mysql-storage
    spec:
      accessModes: [\"ReadWriteOnce\"]
      resources:
        requests:
          storage: 100Gi

---
# Redis缓存（会话存储）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fate-redis
  namespace: fate-cluster
spec:
  replicas: 1
  selector:
    matchLabels:
      app: fate-redis
  template:
    metadata:
      labels:
        app: fate-redis
    spec:
      containers:
      - name: redis
        image: redis:6.2-alpine
        ports:
        - containerPort: 6379
        resources:
          requests:
            cpu: \"500m\"
            memory: \"1Gi\"
          limits:
            cpu: \"1\"
            memory: \"2Gi\"

---
# FATE Flow控制器
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fate-flow
  namespace: fate-cluster
spec:
  replicas: 2
  selector:
    matchLabels:
      app: fate-flow
  template:
    metadata:
      labels:
        app: fate-flow
    spec:
      containers:
      - name: fate-flow
        image: federatedai/fate-flow:1.11.1
        ports:
        - containerPort: 9380
        env:
        - name: FATE_FLOW_SERVER_PORT
          value: \"9380\"
        - name: FATE_FLOW_MODEL_TRANSFER_ENABLE
          value: \"true\"
        - name: REDIS_HOST
          value: \"fate-redis.fate-cluster.svc.cluster.local\"
        - name: REDIS_PORT
          value: \"6379\"
        - name: MYSQL_HOST
          value: \"fate-mysql.fate-cluster.svc.cluster.local\"
        - name: MYSQL_PORT
          value: \"3306\"
        - name: MYSQL_DATABASE
          value: \"fate_flow\"
        - name: MYSQL_USER
          value: \"root\"
        - name: MYSQL_PASSWORD
          valueFrom:
            secretKeyRef:
              name: fate-secrets
              key: mysql-root-password
        volumeMounts:
        - name: fate-data
          mountPath: /data/projects/fate
        resources:
          requests:
            cpu: \"2\"
            memory: \"4Gi\"
          limits:
            cpu: \"4\"
            memory: \"8Gi\"
      volumes:
      - name: fate-data
        persistentVolumeClaim:
          claimName: fate-data-pvc

---
# FATE Board可视化界面
apiVersion: apps/v1
kind: Deployment
metadata:
  name: fate-board
  namespace: fate-cluster
spec:
  replicas: 1
  selector:
    matchLabels:
      app: fate-board
  template:
    metadata:
      labels:
        app: fate-board
    spec:
      containers:
      - name: fate-board
        image: federatedai/fateboard:1.11.1
        ports:
        - containerPort: 8080
        env:
        - name: FATEBOARD_PORT
          value: \"8080\"
        - name: FATE_FLOW_HOST
          value: \"fate-flow.fate-cluster.svc.cluster.local\"
        - name: FATE_FLOW_PORT
          value: \"9380\"
        resources:
          requests:
            cpu: \"500m\"
            memory: \"1Gi\"
          limits:
            cpu: \"1\"
            memory: \"2Gi\"

---
# 服务暴露
apiVersion: v1
kind: Service
metadata:
  name: fate-flow-service
  namespace: fate-cluster
spec:
  selector:
    app: fate-flow
  ports:
  - port: 9380
    targetPort: 9380
  type: ClusterIP

---
apiVersion: v1
kind: Service
metadata:
  name: fate-board-service
  namespace: fate-cluster
spec:
  selector:
    app: fate-board
  ports:
  - port: 8080
    targetPort: 8080
  type: LoadBalancer
```

### 2.2 联邦学习参与者配置

```yaml
# participant-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fate-participant-config
  namespace: fate-cluster
data:
  party.conf: |
    {
      \"local\": {
        \"party_id\": 10000,
        \"role\": \"guest\"
      },
      \"partners\": [
        {
          \"party_id\": 9999,
          \"role\": \"host\",
          \"ip\": \"partner-host.company.com\",
          \"port\": 9380,
          \"secure_protocol\": \"https\"
        },
        {
          \"party_id\": 9998,
          \"role\": \"arbiter\",
          \"ip\": \"arbiter.company.com\",
          \"port\": 9380,
          \"secure_protocol\": \"https\"
        }
      ],
      \"communication\": {
        \"proxy\": \"nginx\",
        \"ssl_enabled\": true,
        \"cert_path\": \"/etc/fate/certs\",
        \"timeout\": 300
      },
      \"security\": {
        \"encrypt_type\": \"paillier\",
        \"key_length\": 1024,
        \"diffie_hellman\": {
          \"enabled\": true,
          \"group\": \"MODP_2048\"
        }
      }
    }

  algorithm.conf: |
    {
      \"algorithm\": \"hetero_lr\",
      \"initiator\": {
        \"role\": \"guest\",
        \"party_id\": 10000
      },
      \"job_parameters\": {
        \"work_mode\": 1,
        \"model_id\": \"hetero_logistic_regression_001\",
        \"model_version\": \"20260205\"
      },
      \"role_parameters\": {
        \"guest\": {
          \"args\": {
            \"data\": {
              \"train_data\": [{\"name\": \"guest_train_data\", \"namespace\": \"experiment\"}]
            }
          },
          \"dataio_0\": {
            \"with_label\": true,
            \"label_name\": \"y\",
            \"label_type\": \"int\"
          }
        },
        \"host\": {
          \"args\": {
            \"data\": {
              \"train_data\": [{\"name\": \"host_train_data\", \"namespace\": \"experiment\"}]
            }
          },
          \"dataio_0\": {
            \"with_label\": false
          }
        }
      },
      \"component_parameters\": {
        \"common\": {
          \"hetero_lr_0\": {
            \"penalty\": \"L2\",
            \"optimizer\": \"sgd\",
            \"alpha\": 0.01,
            \"max_iter\": 30,
            \"batch_size\": 500,
            \"learning_rate\": 0.15,
            \"decay\": 1,
            \"decay_sqrt\": true,
            \"init_param\": {
              \"init_method\": \"random_uniform\"
            }
          }
        }
      }
    }
```

---

## 三、联邦学习训练流程

### 3.1 横向联邦学习实现

```python
# horizontal_federated_learning.py
import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
import pickle
import requests
import json
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
import hashlib

class HorizontalFederatedClient:
    def __init__(self, client_id, coordinator_url, local_data_path):
        self.client_id = client_id
        self.coordinator_url = coordinator_url
        self.local_data = pd.read_csv(local_data_path)
        self.model = None
        self.private_key = self._generate_keys()
        
    def _generate_keys(self):
        \"\"\"生成RSA密钥对\"\"\"
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048
        )
        return private_key
    
    def preprocess_data(self, feature_columns, target_column):
        \"\"\"数据预处理\"\"\"
        X = self.local_data[feature_columns]
        y = self.local_data[target_column]
        
        # 标准化
        scaler = StandardScaler()
        X_scaled = scaler.fit_transform(X)
        
        return X_scaled, y.values
    
    def local_training(self, X, y, global_weights=None):
        \"\"\"本地模型训练\"\"\"
        # 初始化模型
        if self.model is None:
            self.model = LogisticRegression(
                penalty='l2',
                solver='lbfgs',
                max_iter=100,
                random_state=42
            )
        
        # 如果有全局权重，进行初始化
        if global_weights is not None:
            self.model.coef_ = global_weights['coef']
            self.model.intercept_ = global_weights['intercept']
        
        # 本地训练
        self.model.fit(X, y)
        
        return {
            'coef': self.model.coef_,
            'intercept': self.model.intercept_,
            'n_samples': len(X)
        }
    
    def encrypt_gradients(self, gradients):
        \"\"\"加密梯度\"\"\"
        # 简化实现：实际应使用同态加密
        serialized = pickle.dumps(gradients)
        encrypted = self.private_key.public_key().encrypt(
            serialized,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        return encrypted
    
    def participate_in_round(self, round_info):
        \"\"\"参与联邦学习轮次\"\"\"
        try:
            # 获取本轮参数
            feature_cols = round_info['feature_columns']
            target_col = round_info['target_column']
            global_weights = round_info.get('global_weights')
            
            # 预处理数据
            X, y = self.preprocess_data(feature_cols, target_col)
            
            # 本地训练
            local_model = self.local_training(X, y, global_weights)
            
            # 计算贡献度
            contribution = {
                'client_id': self.client_id,
                'weights': local_model,
                'samples_count': local_model['n_samples'],
                'round_id': round_info['round_id']
            }
            
            # 发送贡献到协调器
            response = requests.post(
                f\"{self.coordinator_url}/submit_contribution\",
                json=contribution,
                headers={'Content-Type': 'application/json'}
            )
            
            return response.json()
            
        except Exception as e:
            print(f\"Error in round participation: {e}\")
            return {'status': 'error', 'message': str(e)}

class HorizontalFederatedCoordinator:
    def __init__(self, participants_urls):
        self.participants = participants_urls
        self.global_model = None
        self.round_history = []
        
    def federated_averaging(self, contributions):
        \"\"\"联邦平均算法\"\"\"
        total_samples = sum(contrib['samples_count'] for contrib in contributions)
        
        # 加权平均
        weighted_coef = np.zeros_like(contributions[0]['weights']['coef'])
        weighted_intercept = 0.0
        
        for contrib in contributions:
            weight = contrib['samples_count'] / total_samples
            weighted_coef += weight * contrib['weights']['coef']
            weighted_intercept += weight * contrib['weights']['intercept']
        
        return {
            'coef': weighted_coef,
            'intercept': weighted_intercept
        }
    
    def run_federated_round(self, round_id, feature_columns, target_column):
        \"\"\"运行联邦学习轮次\"\"\"
        round_info = {
            'round_id': round_id,
            'feature_columns': feature_columns,
            'target_column': target_column,
            'global_weights': self.global_model
        }
        
        # 收集各参与方贡献
        contributions = []
        for participant_url in self.participants:
            try:
                response = requests.post(
                    f\"{participant_url}/participate\",
                    json=round_info
                )
                if response.status_code == 200:
                    contributions.append(response.json())
            except Exception as e:
                print(f\"Participant {participant_url} failed: {e}\")
        
        # 聚合模型
        if contributions:
            self.global_model = self.federated_averaging(contributions)
            self.round_history.append({
                'round_id': round_id,
                'participants_count': len(contributions),
                'global_model': self.global_model
            })
            
            return {
                'status': 'success',
                'round_id': round_id,
                'participants_count': len(contributions),
                'global_model_updated': True
            }
        else:
            return {
                'status': 'failed',
                'message': 'No contributions received'
            }

# 使用示例
# 客户端启动
client = HorizontalFederatedClient(
    client_id=\"hospital_a\",
    coordinator_url=\"http://coordinator.company.com:8000\",
    local_data_path=\"/data/patient_data.csv\"
)

# 协调器启动
coordinator = HorizontalFederatedCoordinator([
    \"http://hospital-a.company.com:8000\",
    \"http://hospital-b.company.com:8000\",
    \"http://hospital-c.company.com:8000\"
])

# 运行联邦学习
for round_num in range(10):
    result = coordinator.run_federated_round(
        round_id=round_num,
        feature_columns=['age', 'blood_pressure', 'cholesterol'],
        target_column='disease_indicator'
    )
    print(f\"Round {round_num}: {result}\")
```

### 3.2 纵向联邦学习实现

```python
# vertical_federated_learning.py
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
import pickle
import requests
from phe import paillier  # 同态加密库
import hashlib

class VerticalFederatedGuest:
    def __init__(self, guest_id, host_url, arbiter_url, local_data_path):
        self.guest_id = guest_id
        self.host_url = host_url
        self.arbiter_url = arbiter_url
        self.local_data = pd.read_csv(local_data_path)
        self.public_key, self.private_key = paillier.generate_paillier_keypair()
        self.encrypted_gradients = None
        
    def prepare_guest_data(self, feature_columns, label_column):
        \"\"\"准备访客方数据\"\"\"
        X_guest = self.local_data[feature_columns]
        y = self.local_data[label_column]
        
        # 数据标准化
        scaler_X = StandardScaler()
        X_guest_scaled = scaler_X.fit_transform(X_guest)
        
        scaler_y = StandardScaler()
        y_scaled = scaler_y.fit_transform(y.values.reshape(-1, 1)).flatten()
        
        return X_guest_scaled, y_scaled, scaler_X, scaler_y
    
    def compute_guest_gradients(self, X_guest, y, host_encrypted_u):
        \"\"\"计算访客方梯度（使用加密数据）\"\"\"
        n_samples = len(X_guest)
        
        # 解密主机方的u值
        u_decrypted = [self.private_key.decrypt(enc_u) for enc_u in host_encrypted_u]
        u_array = np.array(u_decrypted)
        
        # 计算梯度
        guest_gradient = (1/n_samples) * X_guest.T.dot(u_array - y)
        
        # 加密梯度发送给仲裁方
        encrypted_gradient = [self.public_key.encrypt(grad) for grad in guest_gradient]
        
        return encrypted_gradient
    
    def participate_in_vertical_training(self, training_round):
        \"\"\"参与纵向联邦训练\"\"\"
        try:
            # 准备数据
            X_guest, y, scaler_X, scaler_y = self.prepare_guest_data(
                training_round['guest_features'],
                training_round['label_column']
            )
            
            # 从主机方获取加密中间结果
            host_response = requests.post(
                f\"{self.host_url}/compute_host_forward\",
                json={
                    'round_id': training_round['round_id'],
                    'host_features': training_round['host_features']
                }
            )
            
            if host_response.status_code == 200:
                host_result = host_response.json()
                host_encrypted_u = host_result['encrypted_u']
                
                # 计算梯度
                guest_gradients = self.compute_guest_gradients(X_guest, y, host_encrypted_u)
                
                # 将梯度发送给仲裁方
                arbiter_response = requests.post(
                    f\"{self.arbiter_url}/aggregate_gradients\",
                    json={
                        'party_id': self.guest_id,
                        'gradients': [str(grad.ciphertext()) for grad in guest_gradients],
                        'public_key_n': str(self.public_key.n)
                    }
                )
                
                return arbiter_response.json()
            else:
                return {'status': 'error', 'message': 'Host computation failed'}
                
        except Exception as e:
            return {'status': 'error', 'message': str(e)}

class VerticalFederatedHost:
    def __init__(self, host_id, guest_url, arbiter_url, local_data_path):
        self.host_id = host_id
        self.guest_url = guest_url
        self.arbiter_url = arbiter_url
        self.local_data = pd.read_csv(local_data_path)
        self.weights = None
        
    def prepare_host_data(self, feature_columns):
        \"\"\"准备主机方数据\"\"\"
        X_host = self.local_data[feature_columns]
        scaler = StandardScaler()
        X_host_scaled = scaler.fit_transform(X_host)
        return X_host_scaled, scaler
    
    def compute_host_forward(self, X_host, guest_weights):
        \"\"\"计算主机方前向传播\"\"\"
        # 计算线性组合
        linear_combination = X_host.dot(guest_weights)
        
        # 应用激活函数（这里简化为线性）
        u_host = linear_combination
        
        # 加密u值
        # 注意：实际实现中需要从访客方获取公钥
        # 这里简化处理
        encrypted_u = [u for u in u_host]
        
        return encrypted_u

class VerticalFederatedArbiter:
    def __init__(self, guest_url, host_url):
        self.guest_url = guest_url
        self.host_url = host_url
        self.global_weights = None
        self.learning_rate = 0.01
        
    def aggregate_encrypted_gradients(self, guest_gradients, host_gradients):
        \"\"\"聚合加密梯度\"\"\"
        # 解密并聚合梯度
        # 简化实现：实际需要更复杂的同态加密操作
        total_gradients = []
        for g_grad, h_grad in zip(guest_gradients, host_gradients):
            # 这里应该解密并聚合
            aggregated = float(g_grad) + float(h_grad)
            total_gradients.append(aggregated)
        
        return np.array(total_gradients)
    
    def update_global_model(self, aggregated_gradients):
        \"\"\"更新全局模型\"\"\"
        if self.global_weights is None:
            self.global_weights = np.zeros(len(aggregated_gradients))
        
        # 梯度下降更新
        self.global_weights -= self.learning_rate * aggregated_gradients
        
        return self.global_weights

# 使用示例
# 访客方（拥有标签）
guest = VerticalFederatedGuest(
    guest_id=\"bank\",
    host_url=\"http://telecom.company.com:8000\",
    arbiter_url=\"http://arbiter.company.com:8000\",
    local_data_path=\"/data/bank_customer_data.csv\"
)

# 主机方（拥有特征）
host = VerticalFederatedHost(
    host_id=\"telecom\",
    guest_url=\"http://bank.company.com:8000\",
    arbiter_url=\"http://arbiter.company.com:8000\",
    local_data_path=\"/data/telecom_customer_data.csv\"
)

# 仲裁方
arbiter = VerticalFederatedArbiter(
    guest_url=\"http://bank.company.com:8000\",
    host_url=\"http://telecom.company.com:8000\"
)

# 联邦训练循环
for round_num in range(50):
    training_config = {
        'round_id': round_num,
        'guest_features': ['income', 'age', 'credit_score'],
        'host_features': ['call_duration', 'data_usage', 'network_type'],
        'label_column': 'churn'
    }
    
    # 访客方参与训练
    guest_result = guest.participate_in_vertical_training(training_config)
    print(f\"Round {round_num} - Guest result: {guest_result}\")
```

---

## 四、联邦学习安全机制

### 4.1 差分隐私保护

```python
# differential_privacy.py
import numpy as np
from scipy.stats import laplace
import pandas as pd

class DifferentialPrivacy:
    def __init__(self, epsilon=1.0, delta=1e-5):
        self.epsilon = epsilon
        self.delta = delta
        
    def add_laplace_noise(self, data, sensitivity):
        \"\"\"添加拉普拉斯噪声\"\"\"
        noise_scale = sensitivity / self.epsilon
        noise = laplace.rvs(size=len(data), scale=noise_scale)
        return data + noise
    
    def add_gaussian_noise(self, data, sensitivity):
        \"\"\"添加高斯噪声\"\"\"
        sigma = sensitivity * np.sqrt(2 * np.log(1.25 / self.delta)) / self.epsilon
        noise = np.random.normal(0, sigma, size=len(data))
        return data + noise
    
    def privatize_gradients(self, gradients, clipping_bound=1.0):
        \"\"\"梯度差分隐私化\"\"\"
        # 梯度裁剪
        clipped_gradients = np.clip(gradients, -clipping_bound, clipping_bound)
        
        # 计算敏感度
        sensitivity = 2 * clipping_bound
        
        # 添加噪声
        private_gradients = self.add_gaussian_noise(clipped_gradients, sensitivity)
        
        return private_gradients

class PrivateFederatedLearning:
    def __init__(self, privacy_budget=1.0):
        self.privacy = DifferentialPrivacy(epsilon=privacy_budget)
        self.noise_multiplier = 0.1
        
    def secure_aggregation(self, client_updates, num_clients):
        \"\"\"安全聚合带隐私保护\"\"\"
        # 聚合客户端更新
        aggregated_update = np.mean(client_updates, axis=0)
        
        # 添加聚合噪声
        sensitivity = 2.0 / num_clients  # 聚合敏感度
        noisy_aggregate = self.privacy.add_gaussian_noise(
            aggregated_update, 
            sensitivity
        )
        
        return noisy_aggregate
    
    def adaptive_privacy_budget(self, round_num, total_rounds):
        \"\"\"自适应隐私预算分配\"\"\"
        # 前期使用较少隐私预算，后期增加精度
        progress = round_num / total_rounds
        if progress < 0.3:
            return 0.1  # 初始探索阶段
        elif progress < 0.7:
            return 0.5  # 中期平衡阶段
        else:
            return 1.0  # 后期精确阶段

# 使用示例
dp_fl = PrivateFederatedLearning(privacy_budget=0.5)

# 模拟客户端梯度
client_gradients = [
    np.random.randn(10) for _ in range(5)  # 5个客户端，每个10维梯度
]

# 私有聚合
private_aggregate = dp_fl.secure_aggregation(client_gradients, len(client_gradients))
print(\"Private aggregate shape:\", private_aggregate.shape)
```

### 4.2 安全多方计算

```python
# secure_multiparty_computation.py
import numpy as np
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
import hashlib
import secrets

class SecureMultipartyComputation:
    def __init__(self, num_parties):
        self.num_parties = num_parties
        self.keys = self._generate_keys()
        
    def _generate_keys(self):
        \"\"\"为每个参与方生成密钥对\"\"\"
        keys = {}
        for i in range(self.num_parties):
            key = RSA.generate(2048)
            keys[f'party_{i}'] = {
                'private': key,
                'public': key.publickey()
            }
        return keys
    
    def secret_share(self, secret, num_shares=None):
        \"\"\"秘密分享\"\"\"
        if num_shares is None:
            num_shares = self.num_parties
            
        # 生成随机份额
        shares = [secrets.randbelow(2**32) for _ in range(num_shares - 1)]
        
        # 最后一个份额使得所有份额之和等于秘密
        last_share = (secret - sum(shares)) % (2**32)
        shares.append(last_share)
        
        return shares
    
    def reconstruct_secret(self, shares):
        \"\"\"重构秘密\"\"\"
        return sum(shares) % (2**32)
    
    def secure_sum(self, private_values):
        \"\"\"安全求和协议\"\"\"
        if len(private_values) != self.num_parties:
            raise ValueError(\"Number of values must match number of parties\")
        
        # 每个参与方将自己的值秘密分享给其他方
        shares_matrix = []
        for i, value in enumerate(private_values):
            shares = self.secret_share(value, self.num_parties)
            shares_matrix.append(shares)
        
        # 每个参与方收集自己收到的所有份额并求和
        partial_sums = []
        for j in range(self.num_parties):
            partial_sum = sum(shares_matrix[i][j] for i in range(self.num_parties))
            partial_sums.append(partial_sum)
        
        # 最终重构总和
        total_sum = self.reconstruct_secret(partial_sums)
        return total_sum
    
    def oblivious_transfer(self, sender_choices, receiver_choice):
        \"\"\"不经意传输\"\"\"
        # 简化的OT实现
        # 发送方有两个消息 m0, m1
        # 接收方选择其中一个而不让发送方知道选择
        m0, m1 = sender_choices
        choice_bit = receiver_choice  # 0 or 1
        
        # 使用RSA盲签名实现
        receiver_key = RSA.generate(2048)
        cipher = PKCS1_OAEP.new(receiver_key)
        
        # 接收方生成盲化消息
        blind_factor = secrets.randbelow(receiver_key.n)
        blinded_choice = (choice_bit * blind_factor) % receiver_key.n
        
        # 发送方加密两个消息
        encrypted_m0 = pow(m0, receiver_key.e, receiver_key.n)
        encrypted_m1 = pow(m1, receiver_key.e, receiver_key.n)
        
        # 根据盲化选择返回对应加密消息
        if choice_bit == 0:
            return cipher.decrypt(encrypted_m0)
        else:
            return cipher.decrypt(encrypted_m1)

# 使用示例
smc = SecureMultipartyComputation(num_parties=3)

# 安全求和示例
private_values = [100, 200, 300]  # 每个参与方的秘密值
secure_sum_result = smc.secure_sum(private_values)
print(f\"Secure sum result: {secure_sum_result}\")  # 应该是600

# 不经意传输示例
sender_messages = [\"Secret_A\", \"Secret_B\"]
receiver_choice = 1  # 想要接收Secret_B
received_message = smc.oblivious_transfer(sender_messages, receiver_choice)
print(f\"Received message: {received_message}\")
```

---

## 五、联邦学习监控与治理

### 5.1 联邦学习监控系统

```yaml
# federated-learning-monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: federated-learning-alerts
  namespace: monitoring
spec:
  groups:
  - name: federated.learning.rules
    rules:
    # 参与方健康检查
    - alert: ParticipantOffline
      expr: |
        federated_participant_status == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: \"Federated learning participant offline\"
        description: \"Participant {{ $labels.participant_id }} is offline\"
    
    # 联邦训练进度监控
    - alert: TrainingConvergenceSlow
      expr: |
        rate(federated_training_loss[30m]) > -0.001
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: \"Federated training convergence slow\"
        description: \"Training loss reduction rate is below threshold\"
    
    # 隐私预算消耗监控
    - alert: PrivacyBudgetExhausted
      expr: |
        federated_privacy_budget_consumed / federated_privacy_budget_total > 0.9
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: \"Privacy budget nearly exhausted\"
        description: \"Privacy budget consumption reached 90% of limit\"
    
    # 模型性能差异告警
    - alert: ModelPerformanceDivergence
      expr: |
        stddev(federated_model_accuracy) > 0.1
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: \"Model performance divergence detected\"
        description: \"Significant performance differences among participant models\"

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: federated-learning-dashboard
  namespace: monitoring
data:
  dashboard.json: |
    {
      \"dashboard\": {
        \"title\": \"Federated Learning Monitoring\",
        \"panels\": [
          {
            \"title\": \"Participant Status\",
            \"type\": \"stat\",
            \"targets\": [
              {
                \"expr\": \"count(federated_participant_status == 1)\",
                \"legendFormat\": \"Online Participants\"
              },
              {
                \"expr\": \"count(federated_participant_status == 0)\",
                \"legendFormat\": \"Offline Participants\"
              }
            ]
          },
          {
            \"title\": \"Global Model Performance\",
            \"type\": \"graph\",
            \"targets\": [
              {
                \"expr\": \"federated_model_accuracy\",
                \"legendFormat\": \"{{participant_id}}\"
              },
              {
                \"expr\": \"avg(federated_model_accuracy)\",
                \"legendFormat\": \"Average Accuracy\"
              }
            ]
          },
          {
            \"title\": \"Privacy Budget Consumption\",
            \"type\": \"gauge\",
            \"targets\": [
              {
                \"expr\": \"federated_privacy_budget_consumed / federated_privacy_budget_total * 100\",
                \"legendFormat\": \"Consumption %\"
              }
            ]
          },
          {
            \"title\": \"Communication Overhead\",
            \"type\": \"graph\",
            \"targets\": [
              {
                \"expr\": \"rate(federated_communication_bytes_total[5m])\",
                \"legendFormat\": \"Bytes/sec\"
              }
            ]
          }
        ]
      }
    }
```

### 5.2 联邦学习治理策略

```python
# federated_governance.py
import yaml
import json
from datetime import datetime, timedelta
from dataclasses import dataclass
from typing import List, Dict, Any

@dataclass
class Participant:
    participant_id: str
    role: str  # guest, host, arbiter
    organization: str
    data_size: int
    reputation_score: float
    last_active: datetime

@dataclass
class FederationPolicy:
    min_participants: int
    max_rounds: int
    privacy_budget: float
    quality_threshold: float
    reputation_weight: float

class FederatedGovernance:
    def __init__(self, config_file: str):
        with open(config_file, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.participants: List[Participant] = []
        self.policy = FederationPolicy(**self.config['policy'])
        self.audit_log: List[Dict[str, Any]] = []
        
    def register_participant(self, participant_info: Dict[str, Any]) -> bool:
        \"\"\"注册参与方\"\"\"
        # 验证参与方资质
        if not self._validate_participant(participant_info):
            return False
        
        # 检查数据质量
        if not self._assess_data_quality(participant_info):
            return False
        
        # 创建参与方对象
        participant = Participant(
            participant_id=participant_info['id'],
            role=participant_info['role'],
            organization=participant_info['organization'],
            data_size=participant_info['data_size'],
            reputation_score=self._calculate_initial_reputation(participant_info),
            last_active=datetime.now()
        )
        
        self.participants.append(participant)
        self._log_audit_event('participant_registered', participant_info)
        
        return True
    
    def _validate_participant(self, info: Dict[str, Any]) -> bool:
        \"\"\"验证参与方资质\"\"\"
        required_fields = ['id', 'role', 'organization', 'data_size', 'security_cert']
        if not all(field in info for field in required_fields):
            return False
        
        # 检查角色合法性
        valid_roles = ['guest', 'host', 'arbiter']
        if info['role'] not in valid_roles:
            return False
        
        # 检查数据规模
        if info['data_size'] < self.config['minimum_data_size']:
            return False
        
        return True
    
    def _assess_data_quality(self, info: Dict[str, Any]) -> bool:
        \"\"\"评估数据质量\"\"\"
        # 检查数据多样性
        if 'data_diversity_score' in info:
            if info['data_diversity_score'] < self.config['min_diversity_score']:
                return False
        
        # 检查数据新鲜度
        if 'data_last_updated' in info:
            last_update = datetime.fromisoformat(info['data_last_updated'])
            if datetime.now() - last_update > timedelta(days=365):
                return False
        
        return True
    
    def _calculate_initial_reputation(self, info: Dict[str, Any]) -> float:
        \"\"\"计算初始声誉分数\"\"\"
        base_score = 0.5
        
        # 组织信誉加成
        org_reputation = self.config['organization_reputation'].get(
            info['organization'], 0.1
        )
        
        # 数据质量加成
        quality_bonus = min(info.get('data_quality_score', 0) / 100, 0.3)
        
        # 安全认证加成
        security_bonus = 0.1 if info.get('security_cert') else 0
        
        return min(base_score + org_reputation + quality_bonus + security_bonus, 1.0)
    
    def evaluate_contribution(self, participant_id: str, contribution: Dict[str, Any]) -> float:
        \"\"\"评估参与方贡献\"\"\"
        participant = next((p for p in self.participants if p.participant_id == participant_id), None)
        if not participant:
            return 0.0
        
        # 计算贡献分数
        quality_score = contribution.get('model_quality_improvement', 0)
        timeliness_score = self._calculate_timeliness_score(contribution)
        resource_score = self._calculate_resource_efficiency(contribution)
        
        # 加权计算最终分数
        weights = self.config['contribution_weights']
        final_score = (
            weights['quality'] * quality_score +
            weights['timeliness'] * timeliness_score +
            weights['efficiency'] * resource_score
        )
        
        # 更新参与方声誉
        participant.reputation_score = min(
            0.9 * participant.reputation_score + 0.1 * final_score,
            1.0
        )
        participant.last_active = datetime.now()
        
        self._log_audit_event('contribution_evaluated', {
            'participant_id': participant_id,
            'score': final_score,
            'timestamp': datetime.now().isoformat()
        })
        
        return final_score
    
    def _calculate_timeliness_score(self, contribution: Dict[str, Any]) -> float:
        \"\"\"计算及时性分数\"\"\"
        expected_time = contribution.get('expected_completion_time')
        actual_time = contribution.get('actual_completion_time')
        
        if not expected_time or not actual_time:
            return 0.5
        
        time_ratio = actual_time / expected_time
        if time_ratio <= 1:
            return 1.0
        elif time_ratio <= 1.5:
            return 0.7
        else:
            return 0.3
    
    def _calculate_resource_efficiency(self, contribution: Dict[str, Any]) -> float:
        \"\"\"计算资源效率分数\"\"\"
        resource_usage = contribution.get('resource_usage', {})
        baseline_usage = self.config['baseline_resource_usage']
        
        efficiency_scores = []
        for resource, usage in resource_usage.items():
            if resource in baseline_usage:
                efficiency = baseline_usage[resource] / usage
                efficiency_scores.append(min(efficiency, 1.0))
        
        return sum(efficiency_scores) / len(efficiency_scores) if efficiency_scores else 0.5
    
    def enforce_policy(self, current_state: Dict[str, Any]) -> Dict[str, Any]:
        \"\"\"执行治理策略\"\"\"
        violations = []
        
        # 检查最小参与方数量
        active_participants = len([p for p in self.participants if p.reputation_score > 0.3])
        if active_participants < self.policy.min_participants:
            violations.append({
                'type': 'insufficient_participants',
                'current': active_participants,
                'required': self.policy.min_participants
            })
        
        # 检查隐私预算
        if current_state.get('privacy_budget_consumed', 0) > self.policy.privacy_budget:
            violations.append({
                'type': 'privacy_budget_exceeded',
                'consumed': current_state['privacy_budget_consumed'],
                'limit': self.policy.privacy_budget
            })
        
        # 检查模型质量
        avg_quality = current_state.get('average_model_quality', 0)
        if avg_quality < self.policy.quality_threshold:
            violations.append({
                'type': 'quality_threshold_not_met',
                'current': avg_quality,
                'threshold': self.policy.quality_threshold
            })
        
        # 生成治理决策
        decision = {
            'violations': violations,
            'actions': self._determine_actions(violations),
            'timestamp': datetime.now().isoformat()
        }
        
        self._log_audit_event('policy_enforcement', decision)
        return decision
    
    def _determine_actions(self, violations: List[Dict[str, Any]]) -> List[str]:
        \"\"\"根据违规情况确定行动\"\"\"
        actions = []
        
        for violation in violations:
            if violation['type'] == 'insufficient_participants':
                actions.append('suspend_training_until_more_participants_join')
            elif violation['type'] == 'privacy_budget_exceeded':
                actions.append('reduce_privacy_budget_allocation')
                actions.append('increase_noise_level')
            elif violation['type'] == 'quality_threshold_not_met':
                actions.append('adjust_hyperparameters')
                actions.append('request_additional_training_rounds')
        
        return actions
    
    def _log_audit_event(self, event_type: str, details: Dict[str, Any]):
        \"\"\"记录审计事件\"\"\"
        audit_entry = {
            'timestamp': datetime.now().isoformat(),
            'event_type': event_type,
            'details': details
        }
        self.audit_log.append(audit_entry)
        
        # 保存到持久化存储
        with open('/audit/federated_governance_audit.json', 'a') as f:
            f.write(json.dumps(audit_entry) + '\\n')

# 使用示例
governance = FederatedGovernance('federation_config.yaml')

# 注册参与方
participant_info = {
    'id': 'hospital_a',
    'role': 'guest',
    'organization': 'City General Hospital',
    'data_size': 50000,
    'security_cert': True,
    'data_quality_score': 85,
    'data_diversity_score': 0.75
}

registration_success = governance.register_participant(participant_info)
print(f\"Registration successful: {registration_success}\")

# 评估贡献
contribution = {
    'model_quality_improvement': 0.02,
    'expected_completion_time': 3600,
    'actual_completion_time': 3400,
    'resource_usage': {'cpu_hours': 10, 'memory_gb': 50}
}

score = governance.evaluate_contribution('hospital_a', contribution)
print(f\"Contribution score: {score}\")
```

---

**维护者**: Federated Learning Team | **最后更新**: 2026-02 | **版本**: v1.0