# nRF52840 DK - Integrated Firmware Application Roadmaps

본 프로젝트는 **nRF52840 DK** 보드와 Zephyr RTOS 환경을 기반으로 하드웨어 제어 인프라를 단계별로 검증하고, 최종적으로 고신뢰성 커스텀 프로토콜을 통합해 나가는 펌웨어 애플리케이션 개발 로드맵입니다.

각 단계는 소스 코드 프로젝트의 빌드업 순서에 따라 `01`부터 차례대로 번호를 증가시키며 관리하며, `100`번대 이후 디렉터리는 문서 및 기술 규격 자료용으로 격리하여 운영합니다.

---

## 📂 1. 프로젝트 디렉터리 구조 (Directory Structure)

### 💻 펌웨어 소스 및 개발 프로젝트 (Code)
* **`01_hello_zephyr/`**: Step 1. Zephyr RTOS 환경 구축 및 기본 커널 동작 검증 [cite: 1]
* **`02_blinky_led/`**: Step 2. 기본 GPIO 제어를 통한 LED 점멸 테스트 [cite: 1]
* **`03_sysinit_driver/`**: Step 3. 시스템 초기화 및 드라이버 레이어 구조 셋업 [cite: 1]
* **`04_button_interrupt/`**: Step 4. Zephyr GPIO 콜백 인터페이스 기반 하드웨어 인터럽트(ISR) 구동 검증 [cite: 1]
* **`05_pwm_led_dimming/`**: Step 5. 내부 타이머 하드웨어 연동을 통한 PWM LED 디밍 로직 구현 [cite: 1]
* **`06_uart_protocol_fsm/`**: Step 6. **[Current]** UART 드라이버, 커스텀 링 버퍼 및 수신 패킷 파서(FSM) 통합 구현

### 📄 기술 문서 및 규격 자료 (Documents)
* **`100_protocol/`**: 시스템 통합 가변 길이 패킷 통신 규격서 (`R00` 아카이브 완료) [cite: 1]

---

## 🧠 2. 프로토콜 핵심 설계 사상 (Key Architecture)

`100_protocol`에 반영된 통신 패킷 구조는 실무 최적화 사상을 담고 있습니다[cite: 1].

1. **효율적인 서브넷 브로드캐스트 (`0xXF` 와일드카드 마스킹):** 전체 시스템 기기를 다 깨우지 않고, `Device ID`로 특정 기기를 지정한 상태에서 `Sub ID` 내부의 하위 니블 마스킹 규칙을 통해 **단일 장치 내의 특정 조명 채널 그룹만 일괄 제어**할 수 있는 정교한 주소 체계를 구축했습니다.
2. **데이터 싱크 일원화 (Closed-Loop Sync):** 제어 명령(`0x02`, `0x03`) 성공 시, 드라이버 단의 하드웨어 반영 상태까지 검증하여 **최신 장치 상태 조회 응답(`0x81`) 포맷으로 일괄 동기화 회신**합니다. PC 측 UI 데이터 갱신 오버헤드를 줄이고 통신 신뢰성을 보증합니다.
3. **가변 페이로드 최적화 (LENGTH 슬라이싱):** 헤더 내 `LENGTH` 필드를 순수 페이로드 바이트 수로 정의하고 리틀 엔디안(Little-Endian) 구조를 엄격히 준수하여, MCU 내부 파서의 포인터 연산 효율을 극대화하고 체크섬 위치 산출 공식을 단순화했습니다.

---

## 📅 3. 통합 개발 마일스톤 & 로드맵 (Milestones)

### ✅ Done (지금까지 완료한 것)
- [x] **01 ~ 04 단계:** Zephyr RTOS 커널 부팅, 시스템 초기화 및 인터럽트 기반 하드웨어 입출력 인프라 검증 완료
- [x] **05 단계:** 내부 하드웨어 타이머 제어를 통한 PWM 기반 LED 디밍 내장 제어 로직 구현 완료
- [x] **100_protocol:** `Device ID`/`Sub ID` 마스킹 사상을 반영한 가변 패킷 기술 규격서 아카이브 완료

### 🚀 To-Do (06번 단계: UART 통신 가드 루틴 및 파서 통합 - Current Stage)
* 관련 폴더: `/06_uart_protocol_fsm`
- [ ] 고속 데이터 연속 수신 시 유실(Overrun)을 방지하기 위한 구조적 **링 버퍼(Ring Buffer)** 커스텀 설계
- [ ] UART 인터럽트 런타임 내에서 오버헤드 없이 동작할 **수신 FSM 파서** 코딩 (SOF ➡️ ID 스크리닝 ➡️ LENGTH 슬라이싱 ➡️ CHECKSUM 무결성 검증)
- [ ] 최상위 비트(MSB Set) 판별 조건문 구현 및 파싱 완료된 페이로드 디밍 값을 **기존(05번 단계) PWM 제어 드라이버와 인터페이스 매핑**

### 🔋 Next Roadmap
- [ ] **07_firmware_ota:** 가변 패킷 기반 FOTA 다운로드 세션 오픈, 섹터 단위 플래시 메모리 제어 및 이미지 CRC 검증 루틴 구축
- [ ] **08_battery_adc:** 내장 SAADC Peripheral 기반 배터리 잔량 정밀 측정 및 소프트웨어 필터 알고리즘 적용
- [ ] **09_ble_nus_migration:** 무선 확장성을 위한 BLE NUS(Nordic UART Service) 스택 마이그레이션 및 상위 FSM 결합도 분리 검증
