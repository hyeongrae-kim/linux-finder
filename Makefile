# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LIBS = -lncursesw -lpthread

# 실행 파일명
TARGET = finder

# 소스 파일들 (기존에 사용하던 순서대로)
SOURCES = main.c ui.c fs.c

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성 (기존 gcc 명령어와 동일한 방식)
$(TARGET): $(SOURCES)
	$(CC) -o $(TARGET) $(SOURCES) $(LIBS)

# 간단한 정리
clean:
	rm -f $(TARGET)

# 재빌드
rebuild: clean all

# 기존 방식과 동일한 단일 명령어 (백업용)
simple:
	gcc -o finder main.c ui.c fs.c -lncursesw -lpthread

.PHONY: all clean rebuild simple
