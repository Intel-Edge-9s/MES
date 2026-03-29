## 🏭 OPC UA 기반 Smart Factory MES 구축 프로젝트

### 🛠 개발 배경
수작업 기반의 생산 관리로 인한 데이터 부정확성을 해결하고, 설비마다 상이한 통신 및 제어 방식을 **OPC UA 표준 프로토콜**로 통합하여 실시간 모니터링 및 체계적인 공정 제어를 실현하고자 개발되었습니다.

### 📝 한 줄 요약
**OPC UA와 Modbus TCP를 활용하여 분산된 공정 데이터를 통합하고, 실시간 설비 제어 및 생산 이력을 관리하는 스마트 팩토리 제조 실행 시스템(MES)입니다.**

---

## 📅 프로젝트 개요
- **프로젝트 명:** OPC UA 기반 MES 구축 프로젝트
- **수행 기간:** 2026.02.25 ~ 2026.03.10
- **주요 기능**
  - **실시간 모니터링:** 공정별 온도, 습도 및 설비 상태(Run/Stop) 실시간 시각화
  - **통합 설비 제어:** OPC UA 및 Modbus TCP를 통한 이기종 설비 원격 제어
  - **데이터베이스 관리:** MariaDB를 활용한 생산 계획, 재고 현황, 환경 로그 저장 및 조회
  - **사용자 인터페이스:** Qt 기반의 직관적인 HMI(Human-Machine Interface) 제공

---

## 🛠 기술 스택
| 분류 | 기술 Stack |
| :--- | :--- |
| **Languages** | C, C++, Python |
| **Communication** | OPC UA (open62541), Modbus TCP |
| **Frameworks** | Qt 6, GStreamer |
| **Database** | MariaDB |
| **Hardware/OS** | Raspberry Pi, OpenPLC (Simulator) |

---

## 📂 디렉토리 구조
```text
.
├── client/MainUI          # Qt 기반 HMI 클라이언트 (MVP 패턴 적용)
│   ├── src/core           # DB Manager, User Session 관리
│   ├── src/models         # 데이터 구조 정의
│   ├── src/services       # 비즈니스 로직 및 OPC UA 서비스
│   └── src/views          # UI 화면 (Dashboard, Process 등)
├── database               # MariaDB 스키마 및 DB 설정 스크립트
├── servers                # OPC UA 제조(MFG) 및 물류(LOG) 서버 소스
├── servers_mod            # Modbus 통신 기능이 통합된 서버 소스
└── README.md              # 프로젝트 문서
