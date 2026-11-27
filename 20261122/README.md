# [20261122] Robotics AI 실습 보고서 - ROS 2 Launch, 커스텀 인터페이스, Service 통신

- **학과:** 컴퓨터공학부 (컴퓨터공학전공)
- **학번:** 202235357
- **이름:** 현경훈
- **제출일:** 2026-11-26

---

## 실습 목차 (Summary)

| 번호 | 실습 주제 | 사용 보드 / 환경 | 핵심 이론 및 기술 |
| :---: | :--- | :--- | :--- |
| **Ex 1** | WASD 위치 추적 토픽 파이프라인과 Launch 파일 | Ubuntu 24.04 + ROS 2 Jazzy / colcon, rclpy | Pub/Sub, `.launch.py`, `ExecuteProcess`, `RegisterEventHandler` |
| **Ex 2** | 커스텀 메시지·서비스 정의와 Service 통신 (AddTwoInts) | Ubuntu 24.04 + ROS 2 Jazzy / ament_cmake, rosidl | `.msg` / `.srv`, 중첩 메시지, Client - Server |
| **Ex 3** | 복합 커스텀 메시지와 Service 통신 (MoveRobot) | Ubuntu 24.04 + ROS 2 Jazzy / ament_cmake, rosidl | `RobotPose` 메시지 재사용, 좌표 변환 |
| **참고** | Action 통신 (이론 정리) | - | Goal / Feedback / Result, 취소 요청 |

---

## 1. [실습 1] WASD 위치 추적 토픽 파이프라인과 Launch 파일

### 1.1 실습 수행 과정
- **실습 목표:** 키보드(WASD) 입력을 받아 로봇의 2D 위치를 계산하는 노드 3개(`publisher` → `processor` → `subscriber`)를 만들고, 이를 launch 파일 하나로 순서대로 실행한다.
- **환경 / 회로 구성:**
  - Ubuntu 24.04 + ROS 2 Jazzy, workspace `~/robot_navigation/colcon_ws`
  - 패키지 `1_pubsub` (ament_python), 노드별 로그를 따로 보기 위해 터미널창을 노드마다 하나씩 사용
  - 데이터 흐름

    ```text
    publisher ──(/publisher, String)──▶ processor ──(/processor, String)──▶ subscriber
     키보드 입력                         좌표 계산·발행                       위치 출력
    ```

- **코드 구현 및 디버깅:**
  - **초기화 (Config):**
    - `setup.py`의 `data_files`에 launch 파일을 등록하고, `entry_points`에 실행 명령 5개(`talker`, `listener`, `processor`, `publisher`, `subscriber`)를 등록한다.
      ```python
      ('share/' + package_name + '/launch', ['launch/my_location.launch.py'])
      ```
    - 파일 위치: `src/1_pubsub/launch/my_location.launch.py`
  - **메인 로직:**

    | 노드 | 동작 |
    | :---: | :--- |
    | `publisher` | 타이머(0.1초) 콜백에서 `input()`으로 키를 받아 소문자로 바꿔 `/publisher` 토픽에 발행 |
    | `processor` | `/publisher`를 구독해 `w`: y+1, `a`: x−1, `s`: y−1, `d`: x+1로 좌표를 갱신하고, 0.5초마다 `"(x, y)"` 문자열을 `/processor` 토픽에 발행 (시작 위치 (0, 0)) |
    | `subscriber` | `/processor`를 구독해 현재 위치를 로그로 출력 |

  - **Launch 파일 구성 (`my_location.launch.py`):**
    - `generate_launch_description()`에서 노드마다 `ExecuteProcess`를 만든다. 명령은 `gnome-terminal -- bash -c 'source <workspace>/install/setup.bash; ros2 run 1_pubsub <노드>; exec bash'` 형태이다.
    - `gnome-terminal` : 터미널 창을 새로 엽니다.
    - `publisher`는 키보드 입력(`input()`)을 받아야 하므로 launch에 묶인 프로세스가 아니라 **별도 터미널 창**에서 실행해야 한다. 이 방법으로 노드마다 로그도 따로 볼 수 있고, `exec bash` 덕분에 노드가 끝나도 창이 닫히지 않는다.
    - 실행 순서는 `RegisterEventHandler(OnProcessStart(target_action=..., on_start=[...]))`로 연결한다. publisher 프로세스가 시작되면 processor를, processor가 시작되면 subscriber를 실행한다.
      ```python
      return LaunchDescription([
          publisher,
          RegisterEventHandler(OnProcessStart(target_action=publisher, on_start=[processor])),
          RegisterEventHandler(OnProcessStart(target_action=processor, on_start=[subscriber])),
      ])
      ```
  - **디버깅 / 점검 사항:**
    - launch 파일 등록(`data_files`) 후 `colcon build`와 `source install/setup.bash`를 하지 않으면 `ros2 launch`가 파일을 찾지 못한다.
    - `input()`은 입력을 기다리며 **멈추는(blocking) 함수**라서, 타이머 콜백 안에서 호출하면 Enter를 누를 때까지 `spin`이 멈춘다. 실습에서는 키 입력마다 한 번씩 발행하는 용도라 동작하지만, 타이머 주기(0.1초)는 의미가 없다. 별도 스레드에서 입력을 받거나 키를 즉시 읽는 방식으로 개선할 수 있다.

### 1.2 적용된 이론 및 기술
- **Launch 파일:** 여러 노드와 실행 작업을 하나의 파일에 정의해 한 번에 실행하는 도구이다. 파라미터 전달, 토픽 remap, 네임스페이스 지정, 다른 launch 파일 포함도 설정할 수 있다.
  - 파일 확장자는 `.launch.py`이고 패키지의 `launch/` 폴더에 둔다.
  - 반드시 `generate_launch_description()`을 정의해 `LaunchDescription`을 반환한다.
  - 실행: `ros2 launch 1_pubsub my_location.launch.py`
- **`ros2 run` vs `ros2 launch`:**

  | 구분 | `ros2 run` | `ros2 launch` |
  | :---: | :--- | :--- |
  | 주요 목적 | 실행 파일 하나를 실행 | 정의된 시스템 전체를 실행 |
  | 실행 대상 | 지정한 노드(실행 파일) 하나 | 여러 노드 및 실행 작업 |

- **`ExecuteProcess`와 이벤트 핸들러:** `ExecuteProcess`는 노드 외의 임의의 명령도 실행할 수 있는 launch 액션이다. `RegisterEventHandler`로 특정 이벤트(`OnProcessStart` 등)가 발생했을 때 실행할 액션을 연결하면 실행 순서를 제어할 수 있다.
- **처리 노드를 사이에 둔 파이프라인:** 입력(publisher) → 처리(processor) → 출력(subscriber)을 토픽으로 연결하면 각 노드의 역할이 분리되어, 한 노드를 다른 노드(예: 실제 로봇 제어 노드)로 바꿔도 나머지는 그대로 쓸 수 있다.
- **상태를 가진 노드:** `processor`는 현재 좌표를 멤버 변수에 저장하고 입력이 들어올 때마다 갱신한다(콜백에서 상태 갱신, 타이머에서 주기적 발행).

### 1.3 구현 결과
[실습 1 결과]
- **결과 분석:** launch를 실행하면 터미널 창 3개가 순서대로 열리고 노드가 실행된다.
  - `publisher` 창: `Input WASD:` 입력창이 뜨고, `w` + Enter를 누르면 `Publishing : w`가 출력된다.
  - `processor` 창: `Received WASD: "w"`와 함께 0.5초마다 `Publishing Position: "(0, 1)"`이 출력되고, 키를 입력할 때마다 좌표가 바뀐다.
  - `subscriber` 창: `Received Position: "(0, 1)"`이 출력된다.
  - `ros2 node list`와 `rqt_graph`로 `/publisher` → `/processor` → `/subscriber`의 연결을 확인한다.

---

## 2. [실습 2] 커스텀 메시지·서비스 정의와 Service 통신 (AddTwoInts)

### 2.1 실습 수행 과정
- **실습 목표:** 커스텀 메시지(`Num`)를 정의하고, 이를 요청·응답 필드로 쓰는 커스텀 서비스(`AddTwoInts`)를 만들어 두 정수의 합을 서버에 요청하고 응답받는다.
- **환경 / 회로 구성:**
  - Ubuntu 24.04 + ROS 2 Jazzy, 패키지 2개를 사용한다.
    - `custom_interfaces` (**ament_cmake**): 인터페이스 정의 전용 패키지
    - `2_srvmsg` (ament_python): Server / Client 노드
  - 인터페이스

    | 파일 | 내용 |
    | :--- | :--- |
    | `msg/Num.msg` | `int64 my_num` |
    | `srv/AddTwoInts.srv` | 요청: `Num a`, `Num b` / 응답: `Num sum` |

- **코드 구현 및 디버깅:**
  - **초기화 (Config):**
    - `CMakeLists.txt`에서 `find_package(rosidl_default_generators REQUIRED)`를 선언하고, `rosidl_generate_interfaces()`에 `msg/Num.msg`, `srv/AddTwoInts.srv`를 등록한다.
    - `package.xml`에는 `buildtool_depend: ament_cmake`, `build_depend: rosidl_default_generators`, `exec_depend: rosidl_default_runtime`, `member_of_group: rosidl_interface_packages`를 선언한다.
    - `2_srvmsg/setup.py`의 `entry_points`에 `add_ints_server`, `add_ints_client` 명령을 등록한다.
  - **메인 로직:**
    - **서버:** `create_service(AddTwoInts, '/add_two_ints', callback)`로 요청을 기다리고, 콜백에서 `response.sum.my_num = request.a.my_num + request.b.my_num`을 계산해 반환한다. 요청·응답의 각 값이 `Num` 메시지이므로 `.my_num`으로 접근한다.
    - **클라이언트:** 명령행 인자 2개를 받아 `req.a.my_num`, `req.b.my_num`에 넣고 요청한 뒤 `Sum`을 출력한다.
    - **클라이언트 공통 구조:** `create_client()` → `wait_for_service(timeout_sec=1.0)` 루프로 서버 대기 → `Request()` 객체에 값을 채움 → `call_async()` → `spin_until_future_complete()`로 응답 대기 → `future.result()` 사용
  - **디버깅 / 점검 사항:**
    - `ModuleNotFoundError: No module named 'custom_interfaces'`가 나오면 `custom_interfaces`를 먼저 빌드하고 `source install/setup.bash`를 했는지 확인한다.
    - 인자 없이 클라이언트를 실행하면 `sys.argv[1]`에서 `IndexError`가 난다. 인자 개수를 확인하는 처리를 추가하는 것이 좋다.

### 2.2 적용된 이론 및 기술
- **커스텀 인터페이스:** 표준 메시지(`std_msgs` 등)로 표현하기 어려운 데이터는 직접 정의한다.
  - `.msg`는 토픽에 흐르는 데이터 구조, `.srv`는 요청과 응답 구조를 `---`로 구분해 정의한다.
  - 다른 메시지를 필드 타입으로 쓸 수 있다(중첩). `AddTwoInts.srv`의 `Num a`처럼 쓰면 코드에서 `request.a.my_num`으로 접근한다.
  - 인터페이스 생성에는 CMake 기반 빌드가 필요해 `ament_cmake` 패키지로 만든다. 인터페이스를 별도 패키지로 분리하면 여러 패키지에서 같은 인터페이스를 가져다 쓸 수 있다.
- **Service:** 노드 간 **요청(Request) / 응답(Response)** 기반 통신이다.
  - **Client - Server** 구조이며 **일회성 요청 또는 짧은 작업**에 적합하다. 예: 센서 데이터 요청, 특정 값 초기화
  - 같은 이름의 Server는 하나이고 Client는 여러 개일 수 있다.
  - 클라이언트 코드에서는 `call_async()`와 future를 사용해 `spin`이 멈추지 않게 한다.

### 2.3 구현 결과
[실습 2 결과]
- **결과 분석:** 서버를 먼저 실행한 뒤(`Srv Server Ready.` 출력) 다른 터미널에서 클라이언트를 실행해 확인한다. (아래는 코드에서 계산한 기대 결과이며, 실제 출력 캡처로 교체)

  | 실행 | 인자 | 기대 결과 |
  | :--- | :--- | :--- |
  | `ros2 run 2_srvmsg add_ints_client 3 5` | a=3, b=5 | `Sum: 8` |

  - 서버 터미널에는 받은 요청(`a: 3, b: 5`)과 보낸 응답(`[8]`)이 로그로 출력된다.
  - 서버가 없으면 클라이언트는 `Waiting for service...`를 반복한다.

---

## 3. [실습 3] 복합 커스텀 메시지와 Service 통신 (MoveRobot)

### 3.1 실습 수행 과정
- **실습 목표:** 여러 값(x, y, theta)을 묶은 커스텀 메시지 `RobotPose`를 요청·응답에 사용하는 서비스 `MoveRobot`을 만들어, 현재 위치와 방향, 이동 거리를 받아 이동 후 위치를 계산한다.
- **환경 / 회로 구성:**
  - Ubuntu 24.04 + ROS 2 Jazzy, 실습 2와 같은 `custom_interfaces`(인터페이스), `2_srvmsg`(Server / Client 노드) 패키지를 사용한다.
  - 인터페이스

    | 파일 | 내용 |
    | :--- | :--- |
    | `msg/RobotPose.msg` | `float64 x`, `float64 y`, `float64 theta` |
    | `srv/MoveRobot.srv` | 요청: `RobotPose current_pose`, `float64 distance` / 응답: `RobotPose result_pose`, `bool success` |

- **코드 구현 및 디버깅:**
  - **초기화 (Config):**
    - `CMakeLists.txt`의 `rosidl_generate_interfaces()`에 `msg/RobotPose.msg`, `srv/MoveRobot.srv`를 추가로 등록하고, 인터페이스가 바뀌었으므로 `custom_interfaces`를 다시 빌드한다.
    - `2_srvmsg/setup.py`의 `entry_points`에 `move_robot_server`, `move_robot_client` 명령을 등록한다.
  - **메인 로직:**
    - **서버:** `create_service(MoveRobot, '/move_robot', callback)`로 요청을 기다린다. 콜백에서 현재 위치(`current_pose.x`, `.y`), 방향(`current_pose.theta`, **도 단위**), 이동 거리(`distance`)를 읽고, 각도를 `math.radians()`로 라디안으로 바꾼 뒤 이동 후 위치를 계산한다.

      $$x_{next} = x + d\cos\theta, \qquad y_{next} = y + d\sin\theta$$

      계산 결과를 `response.result_pose`에 담고, 방향(theta)은 그대로 돌려주며 `success = True`를 반환한다.
    - **클라이언트:** 명령행 인자 4개(x, y, theta, distance)를 `float`로 바꿔 요청에 담고, 응답의 `success`와 이동 후 위치·방향을 출력한다. (좌표는 소수점 둘째 자리까지 출력)
  - **디버깅 / 점검 사항:**
    - `RobotPose`, `MoveRobot`을 추가하거나 수정한 뒤에는 `custom_interfaces`를 다시 빌드하고 `source install/setup.bash`를 해야 반영된다.
    - theta는 라디안이 아니라 **도(degree)** 단위로 입력해야 한다.
    - math.sin()과 math.cos()은 **라디안(radian)** 단위로 입력해야 한다.
    - 서버는 입력 검증 없이 항상 `success = True`를 반환한다. 이동 불가능한 경우(예: 음수 거리, 범위 밖)를 판단해 `False`를 반환하도록 확장할 수 있다.
    - `cos(90°)`처럼 정확히 0이어야 하는 값이 `6.1e-17` 같은 부동소수점 오차로 계산된다. 서버 로그에는 `1.0000000000000002`처럼 보일 수 있고, 클라이언트는 `:.2f`로 출력해 가려진다.
    - 인자가 4개 미만이면 `sys.argv` 접근에서 `IndexError`가 난다.

### 3.2 적용된 이론 및 기술
- **복합(중첩) 메시지:** 여러 값을 묶은 메시지를 따로 정의해 두면(`RobotPose`) 요청과 응답에서 같은 타입을 재사용할 수 있다. 서비스 정의가 간결해지고, 코드에서는 `request.current_pose.x`, `response.result_pose.theta`처럼 점(`.`)으로 접근한다. 실습 2에서는 정수 하나를 감싼 `Num`을 썼고, 이번에는 위치와 방향(자세, pose)을 묶었다.
- **좌표 변환:** 현재 위치에서 방향 $\theta$로 거리 $d$만큼 이동하면 새 좌표는 $(x + d\cos\theta,\ y + d\sin\theta)$이다. 삼각함수는 라디안을 쓰므로 도(degree)를 $\theta_{rad} = \theta_{deg} \times \pi/180$(또는 `math.radians(theta)`)으로 바꾼다.
- **Service의 활용:** 로봇의 이동 계산처럼 한 번 요청하고 바로 결과를 받는 짧은 작업은 서비스에 적합하다. 긴 시간이 걸리는 이동 자체는 Action이 적합하다.

### 3.3 구현 결과
[실습 3 결과]
- **결과 분석:** 서버를 먼저 실행한 뒤(`Move Robot Server Ready.` 출력) 다른 터미널에서 클라이언트를 실행해 확인한다. (아래는 코드에서 계산한 기대 결과이며, 실제 출력 캡처로 교체)

  | 실행 | 인자 | 기대 결과 |
  | :--- | :--- | :--- |
  | `ros2 run 2_srvmsg move_robot_client 0 0 0 1` | (0, 0), θ=0°, d=1 | 위치 (1.00, 0.00), θ=0.0, `success: True` |
  | `ros2 run 2_srvmsg move_robot_client 1 2 90 3` | (1, 2), θ=90°, d=3 | 위치 (1.00, 5.00), θ=90.0, `success: True` |

  - 서버 터미널에는 받은 요청(현재 위치, 방향, 거리)과 보낸 응답(이동 후 위치, 방향)이 로그로 출력된다.

---

## 4. [참고 이론] Action 통신
- **Action:** **장시간 수행되는 작업**을 위한 비동기 통신 방식이며 Client - Server 구조이다. 예: 목표 지점까지 로봇 이동
  - **Goal:** 달성할 목표 (예: 목표 좌표)
  - **Feedback:** 작업이 진행되는 동안의 중간 상황 (예: 남은 거리)
  - **Result:** 작업 종료 후의 최종 결과
  - 작업 도중 Client가 **취소(cancel)를 요청**할 수 있다.
  ```

---

## 5. 필수 의존성 및 환경 설정 (Dependencies & Environment)

- **시뮬레이터 / 개발 환경:**
  - Ubuntu 24.04, ROS 2 Jazzy, Python 3, colcon
  - workspace: `~/robot_navigation/colcon_ws`
- **필수 라이브러리 및 패키지:**
  - Python 패키지(`1_pubsub`, `2_srvmsg`): `rclpy`, `std_msgs`, `launch`, `launch_ros`, `custom_interfaces`
  - 인터페이스 패키지(`custom_interfaces`): `ament_cmake`, `rosidl_default_generators`, `rosidl_default_runtime`
  - 표준 라이브러리: `math`, `sys`
---

## 6. 제출 파일 구성 및 실행 방법 (File Structure & How to Run)

### 6.1 실행 방법
1. 세 패키지를 workspace의 `src/`에 두고, 전체를 빌드한다.
   ```bash
   cd ~/robot_navigation/colcon_ws
   colcon build
   source install/setup.bash
   ```
2. **Ex 1 (Launch):**
   ```bash
   ros2 launch 1_pubsub my_location.launch.py
   ```
   publisher 창에서 `w`, `a`, `s`, `d`와 Enter를 입력하면 processor, subscriber 창에 좌표가 갱신되어 출력된다.
3. **Ex 2 (AddTwoInts):** 터미널 1에서 서버, 터미널 2에서 클라이언트를 실행한다.
   ```bash
   # 터미널 1
   ros2 run 2_srvmsg add_ints_server
   # 터미널 2
   ros2 run 2_srvmsg add_ints_client 3 5
   ```
4. **Ex 3 (MoveRobot):** 같은 방식으로 서버와 클라이언트를 실행한다.
   ```bash
   # 터미널 1
   ros2 run 2_srvmsg move_robot_server
   # 터미널 2
   ros2 run 2_srvmsg move_robot_client 1 2 90 3     # x y theta(도) distance
   ```

---

## 7. 기타 참고 사항 (Notes & References)

- **주의 사항:**
  - 새 터미널을 열 때마다 `source install/setup.bash`가 필요하며, 같은 `ROS_DOMAIN_ID`(기본 0)를 사용해야 노드끼리 통신한다.
  - 인터페이스 파일을 수정하면 `custom_interfaces`를 다시 빌드하고 `source`한 뒤 사용하는 패키지를 실행해야 한다.
  - MoveRobot의 theta는 라디안이 아니라 **도(degree)** 단위이다.
