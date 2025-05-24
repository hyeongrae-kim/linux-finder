// fs.h
#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_FILES 1024
#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 1024
#define LARGE_FILE_SIZE (100 * 1024 * 1024) // 100MB

// 복사 상태를 나타내는 열거형
typedef enum {
    COPY_STATUS_NONE = 0,        // 복사 중이 아님
    COPY_STATUS_IN_PROGRESS = 1, // 복사 진행 중
    COPY_STATUS_COMPLETED = 2    // 복사 완료
} CopyStatus;

// fs.h 구조체 필드 크기 수정
typedef struct {
    char name[MAX_NAME_LEN];  // 파일 이름
    char type[32];            // 파일 종류 (16에서 32로 증가)
    char mtime[24];           // 수정일 (20에서 24로 증가)
    char size[16];            // 파일 크기 (그대로 유지)
    mode_t mode;              // 파일 모드
    CopyStatus copy_status;   // 복사 상태 추가
} FileEntry;

// 클립보드 구조체
typedef struct {
    char source_path[MAX_PATH_LEN];  // 복사할 파일/디렉토리의 절대 경로
    bool is_valid;                   // 클립보드에 유효한 데이터가 있는지
    bool is_directory;               // 복사 대상이 디렉토리인지 여부
} Clipboard;

// 백그라운드 복사 작업 정보
typedef struct CopyTask {
    char source_path[MAX_PATH_LEN];
    char dest_path[MAX_PATH_LEN];
    char dest_dir[MAX_PATH_LEN];     // 대상 디렉토리 추가
    char dest_name[MAX_NAME_LEN];    // 대상 파일명 추가
    bool is_directory;
    pthread_t thread_id;
    bool is_running;
    struct CopyTask* next;  // 연결 리스트로 여러 작업 관리
} CopyTask;

// 전역 클립보드 및 작업 목록
extern Clipboard g_clipboard;
extern CopyTask* g_copy_tasks;
extern pthread_mutex_t g_clipboard_mutex;
extern pthread_mutex_t g_tasks_mutex;

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

// 파일을 편집기로 여는 함수
bool edit_file(const char *path);

// 복사-붙여넣기 관련 함수들
bool copy_to_clipboard(const char *file_path);
bool paste_from_clipboard(const char *dest_dir);
bool init_clipboard_system();
void cleanup_clipboard_system();
void cleanup_finished_tasks();
char* generate_unique_name(const char *dest_dir, const char *base_name);
off_t get_file_size(const char *path);
bool copy_file_sync(const char *src, const char *dest);
bool copy_directory_sync(const char *src, const char *dest);
void* copy_thread_func(void* arg);
bool should_use_background_copy(const char *path);

// 복사 상태 관리 함수들
void update_file_copy_status(FileEntry *files, int file_count, const char *current_path);
bool is_copying_file(const char *file_path);

#endif
