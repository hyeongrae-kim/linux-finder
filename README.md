# Finder 프로그램

이 프로그램은 C언어로 system call과 ncurses 라이브러리를 활용하여 간단한 finder를 구현한 것입니다.

## 컴파일 방법

다음 명령어를 사용하여 컴파일할 수 있습니다:

```
gcc -o finder main.c ui.c fs.c -lncursesw
```

## 실행 방법

컴파일 후 생성된 실행 파일을 다음과 같이 실행합니다:

```
./finder
```

