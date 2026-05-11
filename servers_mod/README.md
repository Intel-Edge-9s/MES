## 📡 Modbus (PLC Layer)

### 배경 및 필요성

프로젝트에서 사용하는 **OpenPLC**는 OPC UA 프로토콜을 기본적으로 지원하지 않는다.
OPC UA Server와 제어 신호 및 데이터를 주고받기 위해 두 프로토콜 사이를 중계하는 **Modbus-to-OPC UA 브리지**를 별도로 구성했다.
OPC UA Server에서 발생한 컨베이어 제어 신호는 Modbus를 통해 OpenPLC로 전달되며, 이를 통해 컨베이어의 ON/OFF 제어 및 재고 수량, 생산량 등의 데이터를 갱신한다.

```
[OPC UA Client]
      │  제어 명령 (컨베이어 ON/OFF 등)
      ▼
[OPC UA Server (port 4840)]
      │  Modbus TCP (port 502)
      ▼
[Modbus-to-OPC UA Bridge]
      │  coil write / register read
      ▼
[OpenPLC Simulator]
      │  재고 수량, 생산량 갱신
      ▼
[Modbus-to-OPC UA Bridge]
      │  상태 데이터 반환
      ▼
[OPC UA Server (port 4840)]
```

> Modbus를 선택한 이유: 메인 통신 프로토콜이 아닌 PLC 내부 연동 구간에 해당하므로,
> 구조가 단순하고 구현 부담이 적은 프로토콜이 적합했다.

---

### Modbus의 특징

| 특징 | 설명 |
|------|------|
| 마스터-슬레이브 구조 | 마스터가 요청을 보내고 슬레이브가 응답하는 단방향 주도 방식 |
| 결정론적 타이밍 | 응답 시점이 예측 가능하여 실시간 제어 환경에 적합 |
| 단순·경량 | 저사양 장비(8비트 마이크로컨트롤러 등)에서도 동작 가능 |

---

<details>
<summary>📊 Modbus 자료형</summary>

| 자료형 | 비트 수 | 방향 | 용도 |
|--------|---------|------|------|
| `QX` | 1 bit | 출력 (쓰기 가능) | 디지털 출력 — 모터 ON/OFF, 밸브 개폐 등 |
| `IX` | 1 bit | 입력 (읽기 전용) | 디지털 입력 — 버튼, 센서 감지 신호 등 |
| `QW` | 16 bit | 출력 (쓰기 가능) | 아날로그 출력 — 모터 속도, 온도 설정값 등 |
| `IW` | 16 bit | 입력 (읽기 전용) | 아날로그 입력 — 센서 측정값, 전류값 등 |

**자료형 의미**

- **Q** : Output (출력) — PLC가 외부 장치를 **제어**하는 방향
- **I** : Input (입력) — 외부 장치의 상태를 PLC가 **읽는** 방향
- **X** : 1비트 단위 (디지털, On/Off)
- **W** : 16비트 단위 (워드, 숫자값)

</details>

---

<details>
<summary>🔧 Modbus 주요 내장 함수</summary>

| 함수 | 설명 |
|------|------|
| `modbus_new_tcp(ip, port)` | TCP 방식의 Modbus 연결 객체 생성 |
| `modbus_set_slave(ctx, id)` | 통신할 슬레이브 장치 ID 설정 |
| `modbus_set_response_timeout(ctx, sec, usec)` | 응답 대기 타임아웃 설정 |
| `modbus_connect(ctx)` | 설정된 연결 객체로 실제 TCP 연결 수행 |
| `modbus_close(ctx)` | 연결 종료 |
| `modbus_free(ctx)` | 연결 객체 메모리 해제 |
| `modbus_write_bit(ctx, addr, value)` | 단일 코일(1비트)에 값 쓰기 — `QX` 출력 |
| `modbus_write_register(ctx, addr, value)` | 단일 홀딩 레지스터(16비트)에 값 쓰기 — `QW` 출력 |
| `modbus_read_bits(ctx, addr, nb, dest)` | 코일(1비트) 값 읽기 — `QX` / `IX` 입력 |
| `modbus_read_registers(ctx, addr, nb, dest)` | 홀딩 레지스터(16비트) 값 읽기 — `QW` / `IW` 입력 |

</details>

---

<details>
<summary>⚙️ 커스텀 함수</summary>

libmodbus를 래핑하여 **자동 재연결**, **재시도 로직**, **편의 기능**을 추가한 함수들이다.

**초기화 / 연결 관리**

| 함수 | 설명 |
|------|------|
| `mb_init(ip, port, slave_id)` | Modbus TCP 연결 객체 생성 및 초기 연결. 타임아웃은 300ms로 고정 |
| `mb_cleanup()` | 연결 종료 및 객체 메모리 해제 |
| `mb_is_connected()` | 현재 연결 상태 반환 (`1`: 연결됨, `0`: 끊김) |
| `mb_reconnect()` | 연결 종료 후 200ms 대기, 재연결 시도 |
| `mb_get_ctx()` | 내부 `modbus_t` 객체 반환 (직접 접근이 필요한 경우) |

**읽기 / 쓰기 (재시도 포함)**

모든 읽기·쓰기 함수는 **실패 시 1회 재연결 후 재시도**하는 구조를 가진다.

| 함수 | 설명 |
|------|------|
| `mb_write_bit_retry(addr, value)` | 코일(1비트) 쓰기. 실패 시 재연결 후 1회 재시도 |
| `mb_write_reg_u16_retry(addr, value)` | 홀딩 레지스터(16비트) 쓰기. 실패 시 재연결 후 1회 재시도 |
| `mb_read_bit_retry(addr, out)` | 코일(1비트) 읽기. 실패 시 재연결 후 1회 재시도 |
| `mb_read_reg_u16_retry(addr, out)` | 홀딩 레지스터(16비트) 읽기. 실패 시 재연결 후 1회 재시도 |

**편의 함수**

| 함수 | 설명 |
|------|------|
| `mb_pulse_coil(addr, pulse_ms)` | 코일을 ON → `pulse_ms`ms 대기 → OFF 순서로 펄스 신호 전송 |
| `mb_set_coil(addr, value)` | 코일에 값을 쓰고, 즉시 readback하여 실제 반영 여부를 로그로 출력 |

</details>
