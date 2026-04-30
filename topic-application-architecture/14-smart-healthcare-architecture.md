# 智慧医疗 Kubernetes 生产架构设计

> **适用场景**: 互联网医院 / 智慧医院 / 区域医疗平台 / 医保结算 / 医学影像 AI / 慢病管理  
> **云厂商**: 阿里云 ACK + 产品体系 (等保三级/互联互通测评)  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: 医疗信息化架构师、HIS 技术负责人、阿里云解决方案架构师

---

## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、互联网医院架构](#二互联网医院架构)
- [三、HIS/EMR 核心系统架构](#三hisemr-核心系统架构)
- [四、医学影像云 (PACS) 架构](#四医学影像云-pacs-架构)
- [五、医保结算与电子票据架构](#五医保结算与电子票据架构)
- [六、医疗 AI 辅助诊断架构](#六医疗-ai-辅助诊断架构)
- [七、数据安全与隐私保护架构](#七数据安全与隐私保护架构)
- [八、ACK 阿里云部署架构](#八ack-阿里云部署架构)

---

## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Patients["患者/用户"]
        PATIENT_APP["患者 App<br">挂号/问诊/报告"]
        WECHAT_HOSPITAL["医院公众号<br">轻量服务"]
        KIOSK_HOSPITAL["院内自助机<br">取号/缴费/打印"]
    end

    subgraph Gateway["接入层"]
        DNS_HOSPITAL["云解析 DNS"]
        WAF_HOSPITAL["WAF + DDoS"]
        ALB_HOSPITAL["ALB 负载均衡"]
    end

    subgraph Platform["医疗中台 (ACK)"]
        REGISTRATION["挂号预约<br">号源管理"]
        CONSULTATION["在线问诊<br">图文/视频"]
        EMR["电子病历<br">HL7/FHIR"]
        PACS_SVC["影像服务<br">存储/调阅"]
        LIS_SVC["检验服务<br">报告/互认"]
        PRESCRIPTION["处方流转<br">审方/配送"]
        PAYMENT_MED["移动支付<br">医保/自费"]
    end

    subgraph Hospital["院内系统"]
        HIS["HIS<br">医院信息系统"]
        CIS["CIS<br">临床信息系统"]
        RIS["RIS<br">放射信息系统"]
        INTEGRATION["集成平台<br">ESB/消息总线"]
    end

    subgraph DataMed["医疗数据"]
        CDR["临床数据中心<br">CDR"]
        DATA_LAKE_MED["医疗数据湖<br">科研/质控"]
        AI_TRAIN["AI 训练数据<br">脱敏/标注"]
    end

    Patients --> Gateway --> Platform --> Hospital
    Hospital --> DataMed
    Platform --> DataMed

    style Platform fill:#e3f2fd
    style Hospital fill:#e8f5e9
    style DataMed fill:#fff8e1
```

### 阿里云产品映射

| 架构层 | 阿里云方案 | 医疗合规 |
|:---|:---|:---|
| 容器平台 | **ACK Pro** / **ACK 专有版** | 等保三级 |
| 数据库 | **PolarDB** + **Lindorm** | 数据持久化 |
| 对象存储 | **OSS** (归档/低频) | 影像长期存储 |
| 大数据 | **MaxCompute** + **PAI** | 科研分析/AI |
| 安全 | **云安全中心** + **操作审计** | 等保/密评 |
| 视频 | **阿里云视频直播** | 远程问诊/手术示教 |
| IoT | **阿里云 IoT** | 医疗设备接入 |
| 网络 | **云企业网 CEN** + **专线** | 院内-云端互联 |

---

## 二、互联网医院架构

```mermaid
flowchart TB
    subgraph OnlineServices["在线服务"]
        REGISTER["挂号预约<br">科室/医生/时间"]
        VIDEO_CLINIC["视频问诊<br">实时音视频"]
        IMG_CONSULT["图文咨询<br">异步交流"]
        RE_VISIT["复诊开方<br">慢性病管理"]
    end

    subgraph CoreProcess["核心流程"]
        TRIAGE["智能分诊<br">症状/科室匹配"]
        QUEUE["排队叫号<br">虚拟候诊"]
        CONSULT_DOC["医生接诊<br">问诊/查体"]
        PRESCRIBE["开具处方<br">药品/检查"]
        REVIEW_PHARM["药师审方<br">合理用药"]
    end

    subgraph Fulfillment["履约"]
        DRUG_DELIVERY["药品配送<br">快递/自提"]
        CHECK_APPLY["检查预约<br">到院/上门"]
        FOLLOW_UP["随访管理<br">用药/康复"]
    end

    OnlineServices --> CoreProcess --> Fulfillment

    style OnlineServices fill:#e3f2fd
    style CoreProcess fill:#fff8e1
    style Fulfillment fill:#e8f5e9
```

---

## 三、HIS/EMR 核心系统架构

```mermaid
flowchart TB
    subgraph EMRData["EMR 数据模型"]
        ADMISSION["入院记录"]
        PROGRESS["病程记录"]
        ORDER_MED["医嘱<br">长期/临时"]
        NURSING["护理记录"]
        DISCHARGE["出院记录"]
    end

    subgraph Integration["集成平台"]
        ESB_MED["ESB 总线<br">HL7 V2/V3"]
        FHIR_GATE["FHIR 网关<br">标准 API"]
        MESSAGE_Q["消息队列<br">异步集成"]
    end

    subgraph Systems["业务系统"]
        HIS_CORE["HIS 核心"]
        LIS_CORE["LIS 检验"]
        RIS_CORE["RIS 放射"]
        ORIS["手术麻醉"]
        PHARMACY["药房系统"]
    end

    EMRData --> Integration --> Systems

    style EMRData fill:#e3f2fd
    style Integration fill:#e8f5e9
```

---

## 四、医学影像云 (PACS) 架构

```mermaid
flowchart TB
    subgraph Modality["影像设备"]
        CT["CT"]
        MRI["MRI"]
        XRAY["DR/X-Ray"]
        ULTRASOUND["超声"]
        ENDOSCOPY["内镜"]
    end

    subgraph Upload["影像上传"]
        DICOM_RECV["DICOM 接收<br">SCP"]
        COMPRESS["压缩<br">JPEG2000/转码"]
        INDEX["索引<br">患者/检查/序列"]
    end

    subgraph StorageMed["影像存储"]
        HOT_STORE["热存储<br">OSS 标准"]
        WARM_STORE["温存储<br">OSS 低频"]
        COLD_STORE["冷存储<br">OSS 归档"]
    end

    subgraph View["影像调阅"]
        VIEWER["Web Viewer<br">MPR/MIP/3D"]
        AI_VIEW["AI 辅助<br">肺结节/骨折"]
        REPORT["诊断报告<br">结构化"]
    end

    Modality --> Upload --> StorageMed --> View

    style Upload fill:#e3f2fd
    style StorageMed fill:#e8f5e9
    style View fill:#fff8e1
```

### PACS 影像存储 K8s 配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dicom-gateway
  namespace: healthcare-pacs
spec:
  replicas: 3
  selector:
    matchLabels:
      app: dicom-gateway
  template:
    metadata:
      labels:
        app: dicom-gateway
    spec:
      containers:
        - name: dicom
          image: registry.cn-hangzhou.aliyuncs.com/healthcare/dicom-gateway:v1.0
          ports:
            - containerPort: 11112
              name: dicom-scp
          env:
            - name: DICOM_AET
              value: "PACS_GATEWAY"
            - name: DICOM_PORT
              value: "11112"
            - name: OSS_BUCKET
              value: "medical-images-hz"
            - name: OSS_ENDPOINT
              value: "oss-cn-hangzhou.aliyuncs.com"
            - name: METADATA_DB_URL
              valueFrom:
                secretKeyRef:
                  name: pacs-db-secret
                  key: url
          resources:
            requests:
              cpu: "1"
              memory: "2Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
          volumeMounts:
            - name: dicom-temp
              mountPath: /tmp/dicom
      volumes:
        - name: dicom-temp
          emptyDir:
            sizeLimit: 100Gi
```

---

## 五、医保结算与电子票据架构

```mermaid
flowchart TB
    subgraph PatientFlow["患者就医"]
        REGISTRATION_MED["挂号<br">医保身份校验"]
        CONSULT_MED["就诊<br">医保目录对照"]
        PRESCRIPTION_MED["处方<br">药品报销比例"]
        PAYMENT_MED["结算<br">医保/自费拆分"]
    end

    subgraph Settlement["医保结算"]
        INSURANCE_VERIFY["医保核验<br">参保/待遇"]
        FEE_SPLIT["费用分解<br">甲类/乙类/自费"]
        CLAIM["结算申报<br">实时/批量"]
        RECONCILE_MED["对账<br">医院/医保局"]
    end

    subgraph Invoice["电子票据"]
        GENERATE["票据生成<br">财政监制"]
        SIGN_DIGITAL["数字签名<br">防篡改"]
        DELIVER["票据交付<br">App/邮件"]
        VERIFY_INV["验真<br">财政平台"]
    end

    PatientFlow --> Settlement --> Invoice

    style Settlement fill:#e3f2fd
    style Invoice fill:#e8f5e9
```

---

## 六、医疗 AI 辅助诊断架构

```mermaid
flowchart TB
    subgraph InputData["输入数据"]
        IMG_DICOM["医学影像<br">DICOM"]
        PATHOLOGY["病理切片<br">WSI"]
        ECG_DATA["心电数据<br">波形"]
        TEXT_REPORT["病历文本<br">结构化"]
    end

    subgraph Preprocess["预处理"]
        DENOISE["去噪<br">增强"]
        SEGMENT["分割<br">ROI 提取"]
        NORMALIZE["标准化<br">归一化"]
    end

    subgraph AIEngine["AI 引擎"]
        DETECT["检测模型<br">病灶定位"]
        CLASSIFY["分类模型<br">良恶性"]
        SEGMENT_AI["分割模型<br">器官/病变"]
        NLP_MED["NLP 模型<br">病历理解"]
    end

    subgraph Output["输出"]
        MARKUP["标注结果<br">热力图"]
        SCORE_AI["置信度<br">概率"]
        REPORT_AI["结构化报告<br">推荐诊断"]
    end

    InputData --> Preprocess --> AIEngine --> Output

    style AIEngine fill:#e3f2fd
    style Output fill:#e8f5e9
```

---

## 七、数据安全与隐私保护架构

```mermaid
flowchart TB
    subgraph DataClassify["数据分级分类"]
        L1["L1 公开<br">科普/公告"]
        L2["L2 内部<br">运营数据"]
        L3["L3 敏感<br">患者信息"]
        L4["L4 核心<br">基因/精神"]
    end

    subgraph Protection["保护措施"]
        DE_IDENTIFY["去标识化<br">K-匿名"]
        ENCRYPT_FIELD["字段加密<br">SM4/AES"]
        ACCESS_CTRL["访问控制<br">RBAC/ABAC"]
        AUDIT_TRAIL["审计追踪<br">全链路"]
    end

    subgraph Compliance["合规"]
        CYBERSECURITY["网络安全法"]
        DATA_SECURITY["数据安全法"]
        PIPL["个人信息保护法"]
        GRADE_PROTECT["等保 2.0"]
    end

    DataClassify --> Protection --> Compliance

    style Protection fill:#e3f2fd
    style Compliance fill:#e8f5e9
```

---

## 八、ACK 阿里云部署架构

### 医疗等保三级部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: emr-core-service
  namespace: healthcare-critical
spec:
  replicas: 3
  selector:
    matchLabels:
      app: emr-core
  template:
    metadata:
      labels:
        app: emr-core
        data-classification: L3
    spec:
      serviceAccountName: emr-service-account
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                  - key: app
                    operator: In
                    values:
                      - emr-core
              topologyKey: kubernetes.io/hostname
      containers:
        - name: emr
          image: registry.cn-hangzhou.aliyuncs.com/healthcare/emr-core:v2.0
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            runAsUser: 10001
            capabilities:
              drop:
                - ALL
          ports:
            - containerPort: 8443
          env:
            - name: DB_HOST
              valueFrom:
                secretKeyRef:
                  name: emr-db-secret
                  key: host
            - name: FHIR_ENDPOINT
              value: "http://fhir-gateway:8080"
            - name: AUDIT_ENABLED
              value: "true"
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "8"
              memory: "16Gi"
```

---

## 参考链接

- [阿里云医疗行业解决方案](https://www.aliyun.com/solution/scenario/healthcare)
- [FHIR 标准](https://www.hl7.org/fhir/)
- [等保 2.0  healthcare](https://www.miit.gov.cn/)
