# Linux Finder

Linux CLI 환경에서 동작하는 터미널 기반 파일 탐색기입니다. ncurses 라이브러리를 사용하여 직관적인 UI를 제공하며, 파일 및 디렉토리 탐색, 복사/붙여넣기, 삭제 등의 기능을 지원합니다.

## 🚀 빠른 시작

### 컴파일 방법

#### GCC를 사용한 직접 컴파일
```bash
gcc -o finder main.c ui.c fs.c -lncursesw -lpthread
```

#### Makefile을 사용한 컴파일
```bash
make
```

### 실행
```bash
./finder
```

## 📁 프로젝트 구조

```
linux-finder/
├── main.c          # 메인 프로그램 및 사용자 입력 처리
├── ui.c/.h          # 사용자 인터페이스 (ncurses 기반)
├── fs.c/.h          # 파일 시스템 관련 기능
├── Makefile         # 빌드 설정
└── README.md        # 프로젝트 문서
```

### 파일별 역할

- **main.c**: 프로그램의 진입점, 사용자 입력 처리, 메인 루프 관리
- **ui.c/.h**: ncurses를 이용한 화면 출력, 색상 관리, 윈도우 레이아웃
- **fs.c/.h**: 파일 시스템 작업 (디렉토리 탐색, 파일 복사/삭제, 클립보드 관리)
- **Makefile**: 프로젝트 빌드 및 정리를 위한 설정

## 📋 기능

- **파일/디렉토리 탐색**: 방향키를 이용한 직관적인 탐색
- **파일 타입 인식**: 프로그래밍 언어 파일 자동 인식 및 분류
- **복사/붙여넣기**: 백그라운드 복사 지원 및 진행률 표시
- **파일/디렉토리 삭제**: 확인 다이얼로그와 함께 안전한 삭제
- **파일 편집**: 프로그래밍 파일 자동 편집기 실행
- **실행 파일 실행**: 실행 가능한 파일 직접 실행
- **실시간 정보**: 현재 경로, 파일 수, 디스크 여유 공간 표시

## 🎮 사용 방법

### 기본 조작
- **↑/↓ 방향키**: 파일/디렉토리 선택
- **Page Up/Down**: 페이지 단위 이동
- **Home/End**: 목록의 처음/끝으로 이동
- **Enter**: 디렉토리 진입 또는 파일 실행/편집
- **q/Q**: 프로그램 종료

### 파일 작업
- **Ctrl+C**: 선택한 파일/디렉토리를 클립보드에 복사
- **Ctrl+V**: 클립보드 내용을 현재 디렉토리에 붙여넣기
- **d/D**: 선택한 파일/디렉토리 삭제 (확인 필요)

### 고급 기능
- **ESC**: 진행 중인 복사 작업 취소
- **백그라운드 복사**: 100MB 이상 파일 또는 디렉토리는 자동으로 백그라운드에서 복사
- **진행률 표시**: 복사 중인 파일의 실시간 진행률 확인
- **자동 파일명 변경**: 동일한 이름의 파일이 존재할 경우 자동으로 고유한 이름 생성

## 🔧 요구사항

- **운영체제**: Linux/Unix 계열
- **라이브러리**: ncursesw
- **컴파일러**: GCC 또는 호환 C 컴파일러

### 의존성 설치 (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install libncurses5-dev libncursesw5-dev build-essential
```

### 의존성 설치 (CentOS/RHEL/Fedora)
```bash
sudo yum install ncurses-devel gcc make
# 또는 (Fedora)
sudo dnf install ncurses-devel gcc make
```

## 🐛 알려진 제한사항

- 매우 큰 디렉토리(수천 개 파일)에서는 성능이 저하될 수 있습니다
- 터미널 크기가 너무 작으면 일부 UI 요소가 올바르게 표시되지 않을 수 있습니다
- 일부 특수 문자가 포함된 파일명에서 표시 문제가 발생할 수 있습니다

## 🛠️ 빌드 옵션

### 정리 명령어
```bash
make clean      # 빌드된 파일 정리
make rebuild    # 정리 후 다시 빌드
```

### 개발용 빌드
```bash
make simple     # 간단한 컴파일 및 실행
```

## 🤝 기여하기

1. 이 저장소를 Fork하세요
2. 새로운 기능 브랜치를 만드세요 (`git checkout -b feature/새기능`)
3. 변경사항을 커밋하세요 (`git commit -am '새 기능 추가'`)
4. 브랜치에 Push하세요 (`git push origin feature/새기능`)
5. Pull Request를 만드세요

## 🎓 프로젝트 배경

이 프로젝트는 경북대학교 시스템 프로그래밍 수업의 기말 프로젝트로 시작되었습니다. 

---

**개발자**: hyeongrae-kim ,JiDeori
**저장소**: https://github.com/hyeongrae-kim/linux-finder
