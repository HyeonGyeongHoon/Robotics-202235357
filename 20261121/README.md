# [20261121] Robotics AI 실습 보고서 - IMU·LiDAR·모터 제어와 ROS 2 Pub/Sub 기초

- **학과:** 컴퓨터공학부 (컴퓨터공학전공)
- **학번:** 202235357
- **이름:** 현경훈
- **제출일:** 2026-11-26

---

## 실습 목차 (Summary)

| 번호 | 실습 주제 | 사용 보드 / 환경 | 핵심 이론 및 기술 |
| :---: | :--- | :--- | :--- |
| **Ex 1** | MPU6050 IMU 데이터 읽기 | ESP32 + MPU6050 / Arduino IDE | I2C 통신, 레지스터 맵, 가속도·각속도 단위 변환 |
| **Ex 2** | LiDAR 거리 데이터 수신 및 파싱 | ESP32 + 2D LiDAR (LDROBOT LD06/LD19 계열 패킷) / Arduino IDE | UART(8N1), 패킷 구조 파싱, 각도 보간 |
| **Ex 3** | 4륜 DC 모터 구동 테스트 | ESP32 / Arduino IDE | GPIO 디지털 출력, H-브리지 진리표 |
| **Ex 4** | ROS 2 Publisher / Subscriber 통신 | Ubuntu + ROS 2 / colcon, rclpy | 노드·토픽·메시지, DDS, QoS, colcon workspace |

---

## 1. [실습 1] MPU6050 IMU 데이터 읽기

### 1.1 실습 수행 과정
- **실습 목표:** ESP32에서 I2C 통신으로 MPU6050의 가속도·자이로 raw 값을 읽고, 물리 단위(m/s², °/s)로 변환해 시리얼 모니터에 출력한다.
- **환경 / 회로 구성:**
  - 보드: ESP32, 센서: MPU6050 모듈
  - 배선 (센서 → ESP32): `VCC → 3.3V`, `GND → GND`, `SDA → GPIO21`, `SCL → GPIO22`
  - I2C 주소 `0x68`
  - 시리얼 모니터 속도: 115200 baud
- **코드 구현 및 디버깅:**
  - **초기화 (`setup()`):** `Serial.begin(115200)`, `Wire.begin(SDA_PIN, SCL_PIN)`로 I2C를 시작한 뒤, `PWR_MGMT_1(0x6B)` 레지스터에 `0x00`을 써서 sleep 비트를 해제한다. MPU6050은 전원 인가 직후 sleep 상태이므로 이 과정이 없으면 값이 갱신되지 않는다.
  - **메인 로직 (`loop()`):**
    1. `0x3B`(ACCEL_XOUT_H) 레지스터를 지정하고 `endTransmission(false)`로 **repeated start**를 보내 버스를 놓지 않은 채 읽기로 전환한다.
    2. `requestFrom(0x68, 14)`로 14바이트를 연속으로 읽는다.
    3. 상위·하위 바이트를 합쳐 `int16_t` raw 값을 만든 뒤 단위를 변환해 출력하고, `delay(100)`으로 약 10 Hz 주기로 반복한다.
  - **디버깅 / 점검 사항:**
    - `Wire.available() < 14`이면 `MPU6050 read failed`를 출력하고 해당 주기를 건너뛰도록 해, 배선 불량·주소 오류를 바로 확인할 수 있게 했다.
    - 값이 계속 0이면 sleep 해제(0x6B) 누락, 값이 전혀 안 나오면 SDA/SCL 배선·주소·전원(3.3V)을 순서대로 점검한다.
    - **개선점:** `(Wire.read() << 8) | Wire.read()` 형태는 C++에서 두 `Wire.read()`의 평가 순서가 보장되지 않아 상·하위 바이트가 뒤바뀔 가능성이 이론상 존재한다. 아래처럼 문장을 분리하면 안전하다.
      ```cpp
      int16_t readInt16() {
          uint8_t high = Wire.read();
          uint8_t low  = Wire.read();
          return (int16_t)((high << 8) | low);
      }
      ```

**MPU6050 데이터 레지스터 구성 (0x3B부터 14바이트)**

| 레지스터 | 내용 | 크기 |
| :---: | :--- | :---: |
| `0x3B ~ 0x40` | 가속도 X, Y, Z (각 상위/하위) | 6 B |
| `0x41 ~ 0x42` | 온도 | 2 B |
| `0x43 ~ 0x48` | 자이로 X, Y, Z (각 상위/하위) | 6 B |

### 1.2 적용된 이론 및 기술
- **IMU (Inertial Measurement Unit, 관성 측정 장치):** 관성(Inertial)을 이용해 물체의 움직임을 측정하는 센서 묶음이다.
  - **자이로스코프(Gyroscope):** 각속도(°/s, rad/s)를 측정한다. 적분하면 각도를 얻지만 오차가 누적(drift)된다.
  - **가속도계(Accelerometer):** 선가속도(m/s², g)를 측정한다. 정지 상태에서도 **중력 가속도(1 g ≈ 9.81 m/s²)**가 측정되므로, 기울기 계산에 활용할 수 있다.
  - **지자기 센서(Magnetometer):** 지구 자기장을 측정해 방위(heading)를 얻는다. 단, 이번에 사용한 MPU6050은 **가속도 3축 + 자이로 3축의 6축 센서**로 지자기 센서가 없다(9축 IMU는 MPU9250 등).
  - 실제 로봇에서는 자이로(단기 정확, 장기 drift)와 가속도계(장기 안정, 단기 노이즈)를 상보 필터·칼만 필터로 융합해 자세를 추정한다.
- **임베디드 통신 프로토콜 비교:**

  | 프로토콜 | 동기/비동기 | 신호선 | 연결 구조 | 특징 및 사용 예 |
  | :---: | :---: | :--- | :--- | :--- |
  | **UART** | 비동기 | TX, RX | 1:1 | 별도 clock 없이 사전에 약속한 전송 속도(baud)와 프레임 형식 사용. LiDAR, GPS |
  | **I2C** | 동기 | SDA, SCL | 1:N (주소 지정) | 2선으로 여러 장치 연결, 풀업 저항 필요. IMU, 온습도 센서 |
  | **SPI** | 동기 | SCK, MOSI, MISO, 장치별 CS | 1:N (CS로 선택) | 고속·전이중 통신. 장치가 늘면 CS선도 늘어남. SD카드, 디스플레이 |
  | **CAN** | 비동기(별도 clock선 없음) | CAN_H, CAN_L (차동) | 멀티 마스터 | 노이즈에 강하고 우선순위 중재 지원. 차량, 산업용 로봇 |

  > 동기식은 clock 신호를 기준으로 데이터를 송수신하고, 비동기식은 clock 없이 약속된 전송 속도와 프레임 형식(start/stop bit)으로 송수신한다.
- **I2C (Inter-Integrated Circuit):** 2선(SDA: 데이터, SCL: 클럭)을 쓰는 동기식 직렬 통신이다.
  - **Start 조건과 Stop 조건**
    - **Start 조건:** SCL이 HIGH인 동안 SDA가 **falling**.
    - **Stop 조건:** SCL이 HIGH인 동안 SDA가 **rising**.
    - 데이터 전송 중에는 SCL이 HIGH일 때 SDA가 변하지 않아야 한다.
  - **주소 바이트:** 
    - 7bit 슬레이브 주소 + 1bit R/W (0 = Write, 1 = Read). 
    - MPU6050(0x68)이면 쓰기 `0xD0`, 읽기 `0xD1`이 된다.(0x68 << 1 == 0xD0, Write : 0, Read : 1) 
    - Arduino `Wire` 라이브러리는 7bit 주소(`0x68`)를 그대로 사용한다.
  - **ACK / NACK:** 바이트(8bit)를 받은 수신 측이 9번째 클럭에서 SDA를 LOW로 당기면 ACK(수신 성공), 당기지 않아 HIGH로 남으면 NACK(수신 실패 또는 읽기 종료 신호)이다.
  - **Repeated Start:** Stop 없이 Start를 다시 보내 버스를 점유한 채 쓰기(레지스터 지정) → 읽기로 전환한다. 코드의 `endTransmission(false)`가 이에 해당한다.
  - SDA/SCL은 **open-drain 구조**라 풀업 저항이 필수이며, 일반적인 MPU6050 모듈에는 풀업이 내장되어 있다.
  - **open-drain 구조** : 내부 스위치 ON → LOW(0V) / 스위치 OFF → 외부 저항을 통한 HIGH
- **raw 값 → 물리량 변환:** 각 축 값은 뱡향에 따라 음수로 존재하여 상위 8bit + 하위 8bit의 16bit **부호 있는 정수(2의 보수, big-endian)** 이므로 반드시 `int16_t`(−32768 ~ +32767)로 합쳐야 한다. 
- 기본 측정 범위(가속도 **±2 g**, 자이로 **±250 °/s**)에서의 감도(LSB)는 다음과 같다.(g = 9.80665 m/s<sup>2</sup>)
  - 32768 / 2g = 16384 LSB/g
  - 32768 / 250°/s ≈ 131 LSB/(°/s)

  $$a\,[\mathrm{m/s^2}] = \frac{raw}{16384} \times 9.80665, \qquad \omega\,[^\circ/\mathrm{s}] = \frac{raw}{131}$$

  (참고: 온도는 $T[^\circ C] = raw/340 + 36.53$. 이번 코드에서는 사용하지 않지만, 14바이트를 연속으로 읽기 때문에 반드시 읽어서 넘겨야 이후 자이로 값의 위치가 맞는다.)

### 1.3 구현 결과
[실습 1 결과]
- **결과 분석:** 시리얼 모니터에 `Accel [m/s^2]: ax, ay, az | Gyro [deg/s]: gx, gy, gz` 형식으로 약 0.1초마다 출력된다.
  - 센서를 수평으로 정지시키면 `az ≈ +9.8`, `ax, ay ≈ 0`이고 자이로는 0 근처의 작은 값(bias)을 보여야 한다. (실제 측정값으로 교체)
  - 센서를 기울이면 중력이 축별로 분배되어 가속도 값이 변하고, 회전시키는 동안에만 해당 축의 자이로 값이 크게 변한다.
  - 정지 상태에서도 자이로 값이 0이 아닌 것은 bias 때문이며, 필요하면 정지 상태 평균을 구해 보정(calibration)한다.

---

## 2. [실습 2] LiDAR 거리 데이터 수신 및 파싱

### 2.1 실습 수행 과정
- **실습 목표:** UART로 들어오는 LiDAR 패킷(47 byte)을 파싱해 각도(0~359°)별 거리(mm)를 배열에 저장하고, 한 바퀴 스캔이 끝날 때마다 출력한다.
- **환경 / 회로 구성:**
  - 보드: ESP32, LiDAR의 TX → ESP32 `GPIO16` (UART2 RX), GND 공통 연결
  - LiDAR는 데이터를 송신만 하므로 ESP32 TX는 사용하지 않는다 (`Serial2.begin(230400, SERIAL_8N1, 16, -1)` : 라이다 센서에 UART 2번 객체 사용).
  - 통신 설정: **230400 baud, 8N1**, 시리얼 모니터는 115200 baud (두 속도가 서로 다름에 주의)
  - 각도 기준 (장착 방향 기준): Back 0°, Left 90°, Front 180°, Right 270°
- **코드 구현 및 디버깅:**
  - **초기화 (`setup()`):** `Serial`(PC 모니터, 115200), `Serial2`(LiDAR 수신, 230400, 8N1)를 각각 초기화한다.
  - **메인 로직 (`loop()`):**
    1. **헤더 동기화:** 수신 바이트가 `0x54`이고, 그다음 바이트가 `0x2C`일 때만 패킷의 시작으로 인정한다. 바이트가 어긋나도 다음 헤더에서 자동으로 복구된다.
    2. 나머지 45바이트를 읽어 47바이트 패킷을 완성한다.
    3. 시작 각도·끝 각도를 구하고(0.01° 단위이므로 `/100.0`), 끝 각도가 시작 각도보다 작으면(360° → 0° 경계) `+360`을 해준다.
    4. 12개 측정점 사이는 균등 간격이므로 `angleStep = (end - start) / 11`로 각 점의 각도를 보간한다. (12개 점 → 11개 구간)
    5. 점마다 거리(2 byte)를 읽어 `distances[degree]`에 저장한다. 거리가 0이면 유효하지 않은 값으로 보고 저장하지 않는다.
    6. 이전 각도가 300° 초과이고 현재 각도가 60° 미만이면(360° → 0° 넘어감) 한 바퀴 완료로 판단해 `printAllDistances()`로 전체 값을 출력한다.
  - **디버깅 / 점검 사항:**
    - 아무 값도 안 들어오면 LiDAR 전원, TX→RX 연결, baud rate(230400)를 확인한다. baud rate가 다르면 깨진 값이 수신된다.

**LiDAR 패킷 구조 (47 byte, 12개 측정점)**

| Byte | 내용 | 비고 |
| :---: | :--- | :--- |
| 0 | Header (`0x54`) | 고정 |
| 1 | VerLen (`0x2C`) | 고정 (측정점 12개 의미) |
| 2~3 | 회전 속도 | little-endian |
| 4~5 | 시작 각도 | 0.01° 단위 |
| 6~41 | 측정점 12개 | 점당 3 byte = 거리 2 byte(mm) + 신뢰도(confidence) 1 byte |
| 42~43 | 끝 각도 | 0.01° 단위 |
| 44~45 | Timestamp | |
| 46 | CRC | 무결성 검사 |

### 2.2 적용된 이론 및 기술
- **UART (Universal Asynchronous Receiver/Transmitter):** clock선 없이 **TX/RX 2선**으로 통신하는 비동기 직렬 통신이다.
  - 수신 측이 데이터의 경계를 알 수 있도록 프레임에 **start bit**(LOW, 1bit)와 **stop bit**(HIGH, 1bit 이상)를 붙인다. 평상시(idle)에는 선이 HIGH 상태다.
  - `SERIAL_8N1`은 데이터 8bit, 패리티 없음(None), stop bit 1을 의미한다. 1 byte 전송에 start + 8 data + stop = 10bit가 필요하므로 230400 baud는 최대 약 23 KB/s이다.
  - 양쪽의 baud rate가 같아야 하며, 한쪽의 TX는 상대편의 RX에 교차 연결한다.
- **Little-endian 바이트 결합:** LiDAR는 2 byte 값을 하위 바이트(low) → 상위 바이트(high) 순서로 보내므로 `low | (high << 8)`로 합친다. (Ex 1의 MPU6050은 반대로 상위 바이트가 먼저인 big-endian이므로 장치마다 데이터시트를 확인해야 한다.)

### 2.3 구현 결과
[실습 2 결과]
- **결과 분석:** 한 바퀴 스캔마다 `0 deg : xxx mm` ~ `359 deg : xxx mm`가 출력되고 구분선(`----------------`)이 표시된다. 장애물(벽, 손 등)을 특정 방향에 가져다 두면 해당 각도 부근의 거리값이 줄어드는 것을 확인한다.
  - 거리가 0인 각도는 측정 실패(범위 밖, 반사 없음)로 값이 갱신되지 않는다.

---

## 3. [실습 3] 4륜 DC 모터 구동 테스트

### 3.1 실습 수행 과정
- **실습 목표:** 4개의 DC 모터(FR, FL, RR, RL)를 드라이버의 IN1/IN2 신호로 하나씩 정회전·정지·역회전시켜 배선과 회전 방향을 확인한다.
- **환경 / 회로 구성:**
  - 보드: ESP32 연결

  | 모터 | IN1 | IN2 |
  | :---: | :---: | :---: |
  | Front Right (FR) | GPIO26 | GPIO25 |
  | Front Left (FL) | GPIO19 | GPIO27 |
  | Rear Right (RR) | GPIO23 | GPIO12 |
  | Rear Left (RL) | GPIO0 | GPIO15 |

  > 모터 전원은 ESP32와 별도 전원(배터리)을 쓰고, 드라이버·ESP32의 GND를 공통으로 연결한다.
- **코드 구현 및 디버깅:**
  - **초기화 (`setup()`):** 8개 핀을 `OUTPUT`으로 설정하고 모두 `LOW`로 초기화해 모터가 의도치 않게 돌지 않도록 한다. 핀 번호는 배열(`motorPins[8]`)에 `{IN1, IN2}` 쌍으로 담아 반복문으로 처리한다.
  - **메인 로직 (`loop()`):** 모터마다 `정회전 1초 → 정지 0.2초 → 역회전 1초 → 정지 0.2초`를 수행하고 다음 모터로 넘어간다.
  - **디버깅 / 점검 사항:**
    - 정회전 방향이 반대로 나오면 해당 모터의 IN1/IN2 배선(또는 코드)을 서로 바꾼다.
    - 특정 모터만 안 돌면 해당 핀 배선, 드라이버 채널, 모터 전원을 확인한다.
    - **주의 (Strapping 핀):** ESP32의 GPIO0, GPIO12, GPIO15(및 GPIO2, GPIO5)는 부팅 시 상태로 부팅 모드·설정이 결정되는 strapping 핀이다. 이 핀들에 드라이버 입력이 걸려 풀업/풀다운 상태가 바뀌면 업로드 실패나 부팅 실패가 발생할 수 있다. 문제가 생기면 32, 33 같은 일반 GPIO로 옮기는 것이 안전하다.

### 3.2 적용된 이론 및 기술
- **H-브리지 모터 드라이버:** 4개의 스위치로 모터 양단의 전압 극성을 바꿔 회전 방향을 제어한다. ESP32 GPIO는 전류가 매우 작아(수십 mA 이하) 모터를 직접 구동할 수 없으므로 드라이버를 통해 별도 전원으로 구동한다. 입력 조합은 다음과 같다.

  | IN1 | IN2 | 동작 |
  | :---: | :---: | :--- |
  | HIGH | LOW | 정회전 |
  | LOW | HIGH | 역회전 |
  | LOW | LOW | 정지 (공회전) |
  | HIGH | HIGH | 정지 (브레이크) |

- **정지 구간(0.2초)의 역할:** 정회전에서 곧바로 역회전으로 바꾸면 모터의 역기전력과 큰 전류가 드라이버·전원에 부담을 주므로, 사이에 정지 시간을 둬 안전하게 방향을 전환한다.

### 3.3 구현 결과
[실습 3 결과]
- **결과 분석:** FR → FL → RR → RL 순서로 각 모터가 1초 정회전, 정지, 1초 역회전, 정지를 반복하는 것을 확인한다.

---

## 4. [실습 4] ROS 2 Publisher / Subscriber 통신

### 4.1 실습 수행 과정
- **실습 목표:** `rclpy`로 Publisher 노드(`talker`)와 Subscriber 노드(`listener`)를 작성하고, colcon으로 빌드해 `/demo_topic` 토픽으로 문자열 메시지를 주고받는다.
- **환경 / 회로 구성:**
  - Ubuntu + ROS 2 Jazzy, colcon workspace의 `src/` 아래에 ament_python 패키지 `1_pubsub` 생성
  - 패키지 구조
    ```text
    ros2_ws/
    ├── src/
    │   └── 1_pubsub/              # ROS가 관리하는 패키지 폴더
    │       ├── package.xml        # 패키지 정보·의존성
    │       ├── setup.py           # entry_points(실행 명령) 등록
    │       ├── setup.cfg
    │       ├── resource/1_pubsub  # ament 인덱스용 마커 파일
    │       ├── test/              # 코드 스타일 테스트
    │       └── 1_pubsub/          # 파이썬 소스코드 폴더
    │           ├── __init__.py
    │           ├── talker.py
    │           └── listener.py
    ├── build/    # 컴파일·빌드 중간 파일
    ├── install/  # 실행 파일, 라이브러리, 설정 파일
    └── log/      # 빌드 로그
    ```
- **코드 구현 및 디버깅:**
  - **초기화 (Config):**
    - `package.xml`에 `rclpy`, `std_msgs`, `geometry_msgs`, `sensor_msgs`, `nav_msgs` 의존성을 선언한다.
    - `setup.py`의 `entry_points`에 실행 명령을 등록한다. `'명령이름 = 패키지.모듈:함수'` 형식이며, 왼쪽의 명령 이름이 `ros2 run <패키지> <명령이름>`에서 쓰인다. (파일 이름과 같을 필요는 없지만 보통 맞춰 짓는다.)
      ```python
      entry_points={
          'console_scripts': [
              'talker = 1_pubsub.talker:main',
              'listener = 1_pubsub.listener:main',
          ],
      },
      ```
  - **메인 로직:**
    - **`talker.py`:** `Node`를 상속한 `Talker` 클래스에서 `create_publisher(String, '/demo_topic', 10)`로 Publisher를 만들고, `create_timer(1.0, ...)`로 1초마다 콜백을 호출한다. 콜백에서 프로그램 시작 후 경과 시간(초)을 담은 `Hello ROS2: {elapsed}` 문자열을 publish하고 로그를 남긴다.
    - **`listener.py`:** `create_subscription(String, '/demo_topic', self.callback, 10)`로 같은 토픽을 구독하고, 메시지가 도착할 때마다 `callback`이 호출되어 `Received: "..."`를 출력한다.
    - **`main()`:** `rclpy.init()` → 노드 생성 → `rclpy.spin(node)`(이벤트 루프; 타이머·구독 콜백을 처리하며 종료될 때까지 대기) → `destroy_node()` → `rclpy.shutdown()` 순서로 동작한다.
  - **디버깅 / 점검 사항:**
    - `ros2 run`에서 실행 파일을 찾지 못하면 `setup.py`의 entry_points 수정 여부, 재빌드(`colcon build`), 새 터미널에서의 `source install/setup.bash` 실행 여부를 확인한다.
    - 코드를 수정한 뒤에는 다시 `colcon build`를 해야 `install/`에 반영된다.
    - 두 노드가 통신하지 않으면 토픽 이름(`/demo_topic`), 메시지 타입, `ROS_DOMAIN_ID`를 확인한다.
    - **참고 (패키지 이름):** 이번 실습에서는 `1_pubsub`을 그대로 사용했지만, ROS 2 패키지 이름은 소문자로 시작하는 것이 권장 규칙이므로 숫자로 시작하면 경고가 나올 수 있다. 새로 만들 때는 `pubsub_1`처럼 짓는 것을 권장한다.

### 4.2 적용된 이론 및 기술
- **ROS (Robot Operating System):** 이름과 달리 운영체제(OS)가 아니라 로봇 소프트웨어 개발을 위한 **프레임워크(메타 OS)** 이다. 하드웨어 추상화, 프로세스(노드) 간 통신, 패키지 관리, 시각화·개발 도구(RViz, Gazebo, rqt)를 하나의 생태계로 제공하며, 표준 통신 규격을 통일해 서로 다른 개발자의 모듈을 쉽게 결합할 수 있다.
- **ROS 1 vs ROS 2:**

  | 구분 | ROS 1 | ROS 2 |
  | :---: | :--- | :--- |
  | 통신 구조 | 중앙 집중형 (마스터 `roscore`가 노드 연결 관리) | P2P 분산 통신 (DDS 기반, 마스터 불필요) |
  | 보안 | 기본 보안 기능 없음 | 보안 지원 (SROS2) |
  | 지원 OS | 주로 Linux (Ubuntu) | Linux, Windows, macOS |

- **DDS (Data Distribution Service):** 고신뢰성 데이터 통신을 위한 국제 표준 미들웨어이다. 노드들이 서로를 자동으로 발견(discovery)하고 P2P로 통신하며, QoS 설정이 가능하다. 구현체로 Fast DDS, Cyclone DDS 등이 있다.
  - **Domain ID (`ROS_DOMAIN_ID`):** 같은 네트워크 안에서 ROS 노드 그룹을 **논리적으로 분리**하는 번호이다. 같은 Domain ID를 가진 노드끼리만 통신하며, 설정하지 않으면 기본값 0이다. (한 네트워크에서 여러 팀이 실습할 때 서로 간섭을 막는 용도)
- **노드·토픽·메시지:**
  - **노드(Node):** 하나의 기능을 수행하는 실행 단위(프로세스)이다. 보통 터미널 하나에서 노드 하나를 실행한다.
  - **토픽(Topic):** 노드 간 비동기 데이터 통로이다. Publisher가 토픽에 메시지를 publish하고, Subscriber가 구독(subscribe)해 받는다 (N:M 가능).
  - **메시지 타입:** 토픽에 흐르는 데이터 형식이다. 예: `std_msgs`(String, Int32 등), `geometry_msgs`(Pose, Twist 등), `sensor_msgs`(Imu, LaserScan 등), `nav_msgs`(Odometry 등). 필드는 `pose.position.x`처럼 접근한다.
  - 앞의 실습과 연결하면 IMU 데이터는 `sensor_msgs/Imu`, LiDAR 데이터는 `sensor_msgs/LaserScan`, 모터 속도 명령은 `geometry_msgs/Twist` 토픽으로 주고받는 것이 일반적이다.
- **서비스와 액션:**
  - **서비스(Service):** 요청(request)-응답(response) 방식의 **짧고 빠른 명령**에 적합하다.
  - **액션(Action):** **오래 걸리는 작업**에 사용하며, 진행 상황 피드백과 취소 기능이 있다 (예: 목표 지점까지 이동).
- **QoS (Quality of Service):** 토픽 통신의 품질 정책이다. 코드의 `10`은 History 정책의 큐 깊이(depth)이다.
  - **Reliability:** `Reliable`(유실 시 재전송, 신뢰성 유지) / `Best Effort`(일부 유실을 허용, 센서 데이터처럼 최신값이 중요할 때 유리)
  - **Durability:** `Volatile`(구독 이후의 메시지만 전달) / `Transient Local`(Publisher가 마지막 메시지를 보관해 나중에 접속한 구독자에게도 전달)
  - Publisher와 Subscriber의 QoS가 **호환되어야** 통신된다. 예를 들어 Best Effort Publisher에 Reliable Subscriber를 붙이면 연결되지 않는다.
- **colcon workspace와 빌드 도구:**
  - workspace는 `src/`(작성한 소스코드), `build/`(컴파일 중간 파일), `install/`(실행 파일·라이브러리·설정 파일), `log/`(빌드 로그)로 소스와 빌드 결과를 분리한다.
  - `colcon`은 패키지 간 의존성을 분석해 빌드 순서를 결정하고, 여러 패키지를 일괄 또는 선택적으로 빌드하는 CLI 도구이다. (`colcon build`, `colcon build --packages-select <패키지>`)
  - 빌드 후에는 `source install/setup.bash`로 환경을 불러와야 `ros2 run`에서 패키지를 찾을 수 있다.
- **텔레오퍼레이션 (Teleoperation, teleop):** 사람이 키보드·조이스틱으로 로봇을 원격 조종하는 것으로, 입력을 `geometry_msgs/Twist` 메시지로 변환해 속도 명령 토픽(`/cmd_vel`)으로 publish하는 방식이 일반적이다.
- **자주 쓰는 명령어:**

  | 명령어 | 설명 |
  | :--- | :--- |
  | `ros2 run <패키지> <실행파일>` | 노드 실행 |
  | `ros2 node list` | 실행 중인 노드 목록 |
  | `ros2 topic list` | 토픽 목록 (기본: `/parameter_events`, `/rosout`) |
  | `ros2 topic echo /토픽이름` | 토픽 메시지 출력 |
  | `rqt_graph` | 노드-토픽 연결 그래프 시각화 |

### 4.3 구현 결과
[실습 4 결과]
- **결과 분석:** 두 터미널에서 각각 `talker`와 `listener`를 실행하면 다음과 같이 동작한다.
  - `talker` 터미널: `Publishing: "Hello ROS2: 0"`, `"Hello ROS2: 1"`, ... 이 1초 간격으로 출력된다.
  - `listener` 터미널: `Received: "Hello ROS2: 0"`, ... 이 같은 간격으로 출력된다.
  - `ros2 node list`에서 `/talker`, `/listener`가, `ros2 topic list`에서 `/demo_topic`이 확인되고, `ros2 topic echo /demo_topic`으로 `data: Hello ROS2: N` 형태의 메시지를 직접 볼 수 있다.
  - `rqt_graph`에서 `/talker` → `/demo_topic` → `/listener` 연결을 확인한다.

---

## 5. 필수 의존성 및 환경 설정 (Dependencies & Environment)

- **시뮬레이터 / 개발 환경:**
  - 펌웨어: Arduino IDE (WeMos D1 R32 보드)
  - ROS: Ubuntu 24.04, ROS 2 Jazzy, Python 3, colcon
- **필수 라이브러리 및 패키지:**
  - Arduino: `Wire.h`(I2C), `Serial` / `Serial2`(HardwareSerial) — ESP32 코어에 내장되어 별도 설치가 필요 없다.
  - ROS 2: `rclpy`, `std_msgs`, `geometry_msgs`, `sensor_msgs`, `nav_msgs`, `setuptools`
  - 테스트용: `ament_copyright`, `ament_flake8`, `ament_pep257`, `python3-pytest`
  - 설치 예시: `sudo apt install ros-<배포판>-desktop python3-colcon-common-extensions`

---

## 6. 제출 파일 구성 및 실행 방법 (File Structure & How to Run)

### 6.1 디렉토리 구조
```text
├── README.md                     # 통합 실습 결과 보고서
├── imu_test/
│   └── imu_test.ino              # [실습 1] MPU6050 I2C 읽기 및 단위 변환
├── lidar_test/
│   └── lidar_test.ino            # [실습 2] LiDAR UART 패킷 파싱
├── motor_test/
│   └── motor_test.ino            # [실습 3] 4륜 모터 순차 구동
└── 1_pubsub/                     # [실습 4] ROS 2 패키지 (ament_python)
    ├── package.xml               # 패키지 정보 및 의존성
    ├── setup.py                  # entry_points (talker, listener)
    ├── setup.cfg
    ├── resource/
    │   └── 1_pubsub              # ament 인덱스 마커 파일 (빈 파일)
    ├── 1_pubsub/
    │   ├── __init__.py
    │   ├── talker.py             # Publisher 노드
    │   └── listener.py           # Subscriber 노드
    └── test/
        ├── test_copyright.py     # 자동 생성된 스타일 테스트
        ├── test_flake8.py
        └── test_pep257.py
```

### 6.2 실행 방법
1. **Ex 1~3 (ESP32):**
   1. Arduino IDE에서 해당 `.ino` 파일을 열고 보드를 `WeMos D1 R32`, 포트를 ESP32가 연결된 포트로 선택한다. (Arduino IDE는 스케치 폴더 이름과 `.ino` 파일 이름이 같아야 한다.)
   2. 회로를 구성한 뒤 업로드한다.
   3. Ex 1, Ex 2는 시리얼 모니터를 **115200 baud**로 열어 출력을 확인한다. Ex 3은 모터가 순서대로 회전하는지 확인한다.
2. **Ex 4 (ROS 2):**
   ```bash
   # 1) 패키지를 workspace의 src/에 배치
   cd ~/colcon_ws
   cp -r 20261121/1_pubsub src/

   # 2) 빌드 및 환경 불러오기
   colcon build
   또는
   colcon build --packages-select 1_pubsub

   source install/setup.bash

   # 3) 터미널 1: Publisher / 터미널 2: Subscriber (새 터미널마다 source 필요)
   ros2 run 1_pubsub talker
   ros2 run 1_pubsub listener

   # 4) 확인
   ros2 node list
   ros2 topic list
   ros2 topic echo /demo_topic
   rqt_graph
   ```

---

## 7. 기타 참고 사항 (Notes & References)

- **주의 사항:**
  - ESP32 GPIO는 **3.3V 로직**이다. MPU6050, LiDAR 신호선에 5V를 직접 연결하지 않는다.
  - 센서·드라이버·ESP32는 **GND를 반드시 공통**으로 연결한다. 모터 전원은 ESP32와 분리한다.
  - Ex 3에서 GPIO0, 12, 15는 strapping 핀이라 부팅·업로드 문제를 일으킬 수 있다.
  ```text
  업로드가 안 될 때 (Connecting...... 에서 멈춤)
  Arduino IDE에서 업로드를 시작합니다.
  Connecting...이 뜨면 보드의 BOOT 버튼(GPIO0에 연결)을 누른 채로 유지합니다.
  BOOT를 누른 상태에서 EN(리셋) 버튼을 한 번 눌렀다 뗍니다.
  업로드가 시작되어 Writing at ...이 나오면 BOOT에서 손을 뗍니다.
  ```
  - LiDAR(`Serial2`, 230400 baud)와 시리얼 모니터(`Serial`, 115200 baud)의 baud rate가 서로 다르다.
  - ROS 2는 새 터미널을 열 때마다 `source install/setup.bash`를 해야 하며, 통신할 노드들은 같은 `ROS_DOMAIN_ID`(기본 0)를 사용해야 한다.
