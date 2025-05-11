// fs.h
#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>

#define MAX_FILES 1024
#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 1024

typedef struct {
    char name[MAX_NAME_LEN];  // 파일 이름
    char type[16];            // 파일 종류 (예: "디렉토리", "일반 파일")
    char mtime[20];           // 수정일 (예: "2025-05-02")
    char size[16];            // 파일 크기 (예: "1.2KB")
    mode_t mode;              // 파일 모드 (파일 유형 및 권한 확인용)
} FileEntry;

// 파일 목록 가져오기 함수
int get_file_list(const char *path, FileEntry *files, int max_files);

// 경로 이동 함수
bool change_directory(const char *path);

// 파일 실행 함수
bool execute_file(const char *path);

// 현재 디렉토리 경로 가져오기
char* get_current_path(char *buf, size_t size);

// 디스크 여유 공간 정보 가져오기
char* get_disk_free_space(const char *path, char *buf, size_t size);

// 파일 정보 가져오기
bool get_file_info(const char *path, FileEntry *file);

// 파일이 실행 가능한지 확인
bool is_executable(const FileEntry *file);

// 파일이 디렉토리인지 확인
bool is_directory(const FileEntry *file);

#endif
