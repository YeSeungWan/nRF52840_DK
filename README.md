# nRF52840 DK - Integrated Firmware Application Roadmaps

본 프로젝트는 **nRF52840 DK** 보드와 Zephyr RTOS 환경을 기반으로 하드웨어 제어 인프라를 단계별로 검증하고, 최종적으로 고신뢰성 커스텀 프로토콜을 통합해 나가는 펌웨어 애플리케이션 개발 로드맵입니다.

각 단계는 소스 코드 프로젝트의 빌드업 순서에 따라 `01`부터 차례대로 번호를 증가시키며 관리하며, `100`번대 이후 디렉터리는 문서 및 기술 규격 자료용으로 격리하여 운영합니다.

---

## 📂 1. 프로젝트 디렉터리 구조 (Directory Structure)

### 💻 펌웨어 소스 및 개발 프로젝트 (Code)
* **`01_hello_zephyr/`**: Step 1. Zephyr RTOS 환경 구축 및 기본 커널 동작 검증
* **`02_blinky_led/`**: Step 2. 기본 GPIO 제어를 통한 LED 점멸 테스트
* **`03_sysinit_driver/`**: Step 3. 시스템 초기화 및 드라이버 레이어 구조 셋업
* **`04_button_interrupt/`**: Step 4. Zephyr GPIO 콜백 인터페이스 기반 하드웨어 인터럽트(ISR) 구동 검증
* **`05_pwm_led_dimming/`**: Step 5. 내부 타이머 하드웨어 연동을 통한 PWM LED 디밍 로직 구현
* **`06_uart_msgq/`**: Step 6. UART 비동기 드라이버, Zephyr 메시지 큐(k_msgq), 가변 패킷 파서(FSM) 통합 구현
* **`07_watchdog_wdg/`**: Step 7. 하드웨어 워치독(WDG) 드라이버 구현 및 시스템 데드락(Lock-up) 방지 인프라 구축
* **`08_battery_adc/`**: Step 8. 내장 SAADC 하드웨어 기반 배터리 전압 정밀 측정 및 소프트웨어 필터 알고리즘 적용
* **`09_low_power_pm/`**: Step 9. Zephyr PM 인프라 기반 CR2032 코인 배터리 최적화 저전력 파워 관리 스택 구현
* **`10_firmware_ota/`**: Step 10. 가변 패킷 기반 유선 FOTA 다운로드 세션 및 플래시 메모리(MCUBoot) 제어 루틴 구축
* **`11_ble_connectivity/`**: Step 11. BLE NUS(Nordic UART Service) 무선 스택 확장 및 상위 FSM 패킷 파서 결합
* **`12_nfc_pairing/`**: Step 12. Wake-on-NFC 기반 초저전력 BLE 페어링 및 상용화 사용자 경험(UX) 최적화

### 📄 기술 문서 및 규격 자료 (Documents)
* **`100_protocol/`**: 시스템 통합 가변 길이 패킷 통신 규격서 (`R00` 아카이브 완료)

---

## 🧠 2. 프로토콜 핵심 설계 사상 (Protocol Design Philosophy)

`100_protocol`에 반영된 통신 패킷 구조는 실무 최적화 사상을 담고 있습니다.

1. **효율적인 서브넷 브로드캐스트 (`0xXF` 와일드카드 마스킹):** 전체 시스템 기기를 다 깨우지 않고, `Device ID`로 특정 기기를 지정한 상태에서 `Sub ID` 내부의 하위 니블 마스킹 규칙을 통해 **단일 장치 내의 특정 조명 채널 그룹만 일괄 제어**할 수 있는 정교한 주소 체계를 구축했습니다.
2. **데이터 싱크 일원화 (Closed-Loop Sync):** 제어 명령(`0x02`, `0x03`) 성공 시, 드라이버 단의 하드웨어 반영 상태까지 검증하여 **최신 장치 상태 조회 응답(`0x81`) 포맷으로 일괄 동기화 회신**합니다. PC 측 UI 데이터 갱신 오버헤드를 줄이고 통신 신뢰성을 보증합니다.
3. **가변 페이로드 최적화 (LENGTH 슬라이싱):** 헤더 내 `LENGTH` 필드를 순수 페이로드 바이트 수로 정의하고 리틀 엔디안(Little-Endian) 구조를 엄격히 준수하여, MCU 내부 파서의 포인터 연산 효율을 극대화하고 체크섬 위치 산출 공식을 단순화했습니다.

---

## 🏗️ 3. 하드웨어 연동 아키텍처 (Hardware Telemetry Architecture)
1. **하드웨어 직접 참조형 배터리 텔레메트리 (Hardware-Direct Telemetry):** CR2032 배터리의 전원 공급 라인(`Internal VDD Line`)을 전압 분배 회로 없이 `NRF_SAADC_VDD` 아날로그 입력 멀티플렉서(MUX)에 직접 바인딩하여 하드웨어 비용을 최소화했습니다. 소프트웨어 레이어에서는 `CONFIG_NRFX_SAADC=y` 드라이버 인터페이스와 이동 평균 필터(Moving Average Filter)를 결합하여 노이즈를 제거하고 정밀한 SoC(State of Charge) 잔량 매핑 로그를 산출합니다.

<img width="821" height="416" alt="battery_telemetry" src="https://github.com/user-attachments/assets/0258f958-a5aa-45a2-9ba1-c689bf39fc9b" />

---

## 📅 4. 통합 개발 마일스톤 & 로드맵 (Milestones)

### ✅ Done (지금까지 완료한 것)
- [x] **01 ~ 04 단계:** Zephyr RTOS 커널 부팅, 시스템 초기화 및 인터럽트 기반 하드웨어 입출력 인프라 검증 완료
- [x] **05 단계:** 내부 하드웨어 타이머 제어를 통한 PWM 기반 LED 디밍 내장 제어 로직 구현 완료
- [x] **06 단계:** UART 비동기 드라이버, Zephyr 메시지 큐(k_msgq), 가변 패킷 파서(FSM) 통합 구현 완료
- [x] **07 단계:** 하드웨어 워치독(WDG) 드라이버 구현 및 시스템 데드락(Lock-up) 방지 인프라 구축
- [x] **08 단계:** 내장 SAADC 하드웨어 기반 배터리 전압 정밀 측정 및 소프트웨어 필터 알고리즘 적용
- [x] **100_protocol:** `Device ID`/`Sub ID` 마스킹 사상을 반영한 가변 패킷 기술 규격서 아카이브 완료

### 🚀 In Progress (09번 단계: 저전력 파워 관리 스택 구현 - Current Stage)
* 관련 폴더: `/09_low_power_pm`
- [ ] Zephyr PM(Power Management) 서브시스템 활성화 및 System Idle 상태 진입 검증
      Implement Zephyr PM subsystem infrastructure and verify System Idle/System Standby state transitions during inactivity.
- [ ] [ ] UART 및 SAADC 등 주변장치(Peripheral) 동적 전원 관리 및 서스펜드/이력 제어
      Configure dynamic device power management (Device PM) to suspend and resume high-current blocks like UART and SAADC.
- [ ] 배터리 수명 극대화를 위한 저전력 타이머 주기 튜닝 및 메인 FSM 연동 전력 프로파일링
      Optimize low-power system clock/timer intervals and profile total current consumption across main FSM states to maximize CR2032 lifespan.

### 🔋 Next Roadmap
- [ ] **10_firmware_ota:** 가변 패킷 기반 유선 FOTA 다운로드 세션 및 플래시 메모리(MCUBoot) 제어 루틴 구축
- [ ] **11_ble_connectivity:** BLE NUS(Nordic UART Service) 무선 스택 확장 및 상위 FSM 패킷 파서 결합
- [ ] **12_nfc_pairing:** Wake-on-NFC 기반 초저전력 BLE 페어링 및 상용화 사용자 경험(UX) 최적화
