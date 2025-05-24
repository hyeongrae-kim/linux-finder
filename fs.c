// fs.c
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <signal.h>

Clipboard g_clipboard = {0};
CopyTask* g_copy_tasks = NULL;
pthread_mutex_t g_clipboard_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_tasks_mutex = PTHREAD_MUTEX_INITIALIZER;

// SIGINT 핸들러 (Ctrl+C 무시)
void sigint_handler(int sig) {}

// 파일 확장자를 기반으로 프로그래밍 언어 타입 반환
static const char* get_programming_language(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return NULL; // 확장자가 없는 경우

    ext++; // '.' 다음 문자로 이동

    if (strcasecmp(ext, "c") == 0) return "C source";
    if (strcasecmp(ext, "h") == 0) return "C header";
    if (strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "cc") == 0) return "C++ source";
    if (strcasecmp(ext, "hpp") == 0) return "C++ header";
    if (strcasecmp(ext, "py") == 0) return "Python source";
    if (strcasecmp(ext, "java") == 0) return "Java source";
    if (strcasecmp(ext, "js") == 0) return "JS source";
    if (strcasecmp(ext, "html") == 0) return "HTML file";
    if (strcasecmp(ext, "css") == 0) return "CSS file";
    if (strcasecmp(ext, "php") == 0) return "PHP source";
    if (strcasecmp(ext, "rb") == 0) return "Ruby source";
    if (strcasecmp(ext, "go") == 0) return "Go source";
    if (strcasecmp(ext, "rs") == 0) return "Rust source";
    if (strcasecmp(ext, "sh") == 0) return "Shell script";
    if (strcasecmp(ext, "asm") == 0 || strcasecmp(ext, "s") == 0) return "Assembly source";
    if (strcasecmp(ext, "swift") == 0) return "Swift source";
    if (strcasecmp(ext, "kt") == 0) return "Kotlin source";

    return NULL; // 알려진 프로그래밍 언어가 아닌 경우
}

// 파일 유형을 문자열로 반환
// fs.c의 get_file_type 함수 수정
static void get_file_type(mode_t mode, char *type, size_t size, const char *filename) {
    // 버퍼 크기 확인
    if (size < 16) {
        strncpy(type, "?", size);
        type[size - 1] = '\0';
        return;
    }
    
	if (S_ISREG(mode)) {
        const char *prog_lang = get_programming_language(filename);
        if (prog_lang) {
            strncpy(type, prog_lang, size - 1);
            type[size - 1] = '\0';
            return;
        }
        strcpy(type, "일반 파일");
    } else if (S_ISDIR(mode)) {
        strcpy(type, "디렉토리");
    } else if (S_ISLNK(mode)) {
        strcpy(type, "심볼릭 링크");
    } else if (S_ISFIFO(mode)) {
        strcpy(type, "FIFO");
    } else if (S_ISSOCK(mode)) {
        strcpy(type, "소켓");
    } else if (S_ISBLK(mode)) {
        strcpy(type, "블록 장치");
    } else if (S_ISCHR(mode)) {
        strcpy(type, "문자 장치");
    } else {
        strcpy(type, "알 수 없음");
    }
    
    // null 종료 문자 보장
    type[size - 1] = '\0';
}

// 파일 크기를 읽기 쉬운 형태로 변환 (KB, MB, GB 등)
static void format_size(off_t size, char *buf, size_t buf_size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double size_d = size;
    
    while (size_d >= 1024 && i < 4) {
        size_d /= 1024;
        i++;
    }
    
    if (i == 0) {
        snprintf(buf, buf_size, "%ld%s", (long)size_d, units[i]);
    } else {
        snprintf(buf, buf_size, "%.1f%s", size_d, units[i]);
    }
}

// 파일 날짜를 형식화
static void format_time(time_t mtime, char *buf, size_t buf_size) {
    struct tm *tm_info = localtime(&mtime);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M", tm_info);
}

// file list 불러오기 
int get_file_list(const char *path, FileEntry *files, int max_files) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[MAX_PATH_LEN];
    int count = 0;
    
    if ((dir = opendir(path)) == NULL) {
        perror("opendir 실패");
        return -1;
    }
    
    // 모든 파일 및 디렉토리 가져오기
    while ((entry = readdir(dir)) != NULL && count < max_files) {
        // "."은 제외
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        
        // 전체 경로 생성
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // 파일 정보 가져오기
        if (lstat(full_path, &file_stat) == -1) {
            perror("lstat 실패");
            continue;
        }
        
        // 파일 정보 저장
        strncpy(files[count].name, entry->d_name, MAX_NAME_LEN - 1);
        files[count].name[MAX_NAME_LEN - 1] = '\0';
        
        // 파일 종류 저장
		get_file_type(file_stat.st_mode, files[count].type, sizeof(files[count].type), entry->d_name);
                
        // 파일 크기 저장 (디렉토리는 "-"로 표시)
        if (S_ISDIR(file_stat.st_mode)) {
            strncpy(files[count].size, "-", sizeof(files[count].size));
        } else {
            format_size(file_stat.st_size, files[count].size, sizeof(files[count].size));
        }
        
        // 수정 시간 저장
        format_time(file_stat.st_mtime, files[count].mtime, sizeof(files[count].mtime));
        
        // 파일 모드 저장 (실행 가능 여부 확인용)
        files[count].mode = file_stat.st_mode;
        
        // 복사 상태 초기화
        files[count].copy_status = COPY_STATUS_NONE;
        
        count++;
    }
    
    closedir(dir);
    
    // ".."만 맨 앞으로 이동
    for (int i = 0; i < count; i++) {
        if (strcmp(files[i].name, "..") == 0 && i > 0) {
            FileEntry temp = files[i];
            for (int j = i; j > 0; j--) {
                files[j] = files[j-1];
            }
            files[0] = temp;
            break; // ".."은 하나만 있으므로 찾으면 바로 종료
        }
    }
    
    return count;
}

// 경로 변경
bool change_directory(const char *path) {
    if (chdir(path) == -1) {
        perror("chdir 실패");
        return false;
    }
    return true;
}

// 파일 실행
bool execute_file(const char *path) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork 실패");
        return false;
    } else if (pid == 0) {
        // 자식 프로세스
        execl(path, path, NULL);
        perror("execl 실패");
        exit(EXIT_FAILURE);
    } else {
        // 부모 프로세스
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

// 현재 경로 가져오기
char* get_current_path(char *buf, size_t size) {
    if (getcwd(buf, size) == NULL) {
        perror("getcwd 실패");
        strncpy(buf, "알 수 없는 경로", size);
        buf[size - 1] = '\0';
    }
    return buf;
}

// 디스크 여유 공간 계산
char* get_disk_free_space(const char *path, char *buf, size_t size) {
    struct statvfs stat;
    
    if (statvfs(path, &stat) == -1) {
        perror("statvfs 실패");
        strncpy(buf, "알 수 없음", size);
    } else {
        double free_space = (double)stat.f_bsize * stat.f_bavail;
        char formatted_size[16];
        format_size(free_space, formatted_size, sizeof(formatted_size));
        snprintf(buf, size, "%s 사용가능", formatted_size);
    }
    
    return buf;
}

// 파일 정보 가져오기
bool get_file_info(const char *path, FileEntry *file) {
    struct stat file_stat;
    
    if (lstat(path, &file_stat) == -1) {
        perror("lstat 실패");
        return false;
    }
    
    // 파일 이름 저장
    char *base = basename((char *)path);
    strncpy(file->name, base, MAX_NAME_LEN - 1);
    file->name[MAX_NAME_LEN - 1] = '\0';
    
    // 파일 종류 저장
	get_file_type(file_stat.st_mode, file->type, sizeof(file->type), file->name);
    
    // 파일 크기 저장
    if (S_ISDIR(file_stat.st_mode)) {
        strncpy(file->size, "-", sizeof(file->size));
    } else {
        format_size(file_stat.st_size, file->size, sizeof(file->size));
    }
    
    // 수정 시간 저장
    format_time(file_stat.st_mtime, file->mtime, sizeof(file->mtime));
    
    // 파일 모드 저장
    file->mode = file_stat.st_mode;
    
    return true;
}

// 파일이 실행 가능한지 확인
bool is_executable(const FileEntry *file) {
    return S_ISREG(file->mode) && (file->mode & S_IXUSR);
}

// 파일이 디렉토리인지 확인
bool is_directory(const FileEntry *file) {
    return S_ISDIR(file->mode);
}

// 파일을 편집기로 열기
bool edit_file(const char *path) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork 실패");
        return false;
    } else if (pid == 0) {
        // 자식 프로세스: 먼저 vim을 시도하고, 실패하면 vi 시도
        execlp("vim", "vim", path, NULL);
        // vim이 실패하면 vi 시도
        execlp("vi", "vi", path, NULL);
        // 둘 다 실패한 경우
        perror("편집기 실행 실패");
        exit(EXIT_FAILURE);
    } else {
        // 부모 프로세스: 자식 프로세스 종료 대기
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status); // 정상 종료 여부 반환
    }
}

// ================ 복사-붙여넣기 관련 함수들 ================

// 클립보드 시스템 초기화
bool init_clipboard_system() {
    // SIGINT 핸들러 등록하여 Ctrl+C 무시
    signal(SIGINT, sigint_handler);

    memset(&g_clipboard, 0, sizeof(Clipboard));
    g_copy_tasks = NULL;
    return true;
}

// 클립보드 시스템 정리
void cleanup_clipboard_system() {
    pthread_mutex_lock(&g_tasks_mutex);

    // 모든 실행 중인 작업 정리
    CopyTask* current = g_copy_tasks;
    while (current) {
        if (current->is_running) {
            pthread_cancel(current->thread_id);
            pthread_join(current->thread_id, NULL);
        }
        CopyTask* next = current->next;
        free(current);
        current = next;
    }
    g_copy_tasks = NULL;

    pthread_mutex_unlock(&g_tasks_mutex);

    // SIGINT 핸들러 복원
    signal(SIGINT, SIG_DFL);
}

// 완료된 작업들 정리
void cleanup_finished_tasks() {
    pthread_mutex_lock(&g_tasks_mutex);

    CopyTask** current = &g_copy_tasks;
    while (*current) {
        if (!(*current)->is_running) {
            CopyTask* to_remove = *current;
            *current = (*current)->next;
            pthread_join(to_remove->thread_id, NULL);
            free(to_remove);
        } else {
            current = &((*current)->next);
        }
    }

    pthread_mutex_unlock(&g_tasks_mutex);
}

// 파일 크기 가져오기
off_t get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        return -1;
    }
    return st.st_size;
}

// 백그라운드 복사가 필요한지 판단
bool should_use_background_copy(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        return false;
    }

    // 디렉토리이거나 100MB 이상인 파일
    return S_ISDIR(st.st_mode) || st.st_size > LARGE_FILE_SIZE;
}

// 고유한 파일명 생성
char* generate_unique_name(const char *dest_dir, const char *base_name) {
    static char unique_name[MAX_PATH_LEN];
    char test_path[MAX_PATH_LEN];
    
    // 원본 이름 먼저 복사
    strcpy(unique_name, base_name);
    
    // 원본 이름 시도
    snprintf(test_path, sizeof(test_path), "%s/%s", dest_dir, unique_name);
    if (access(test_path, F_OK) != 0) {
        return unique_name;
    }
    
    // (1), (2), ... 형태로 시도 - 더 안전한 방식
    for (int i = 1; i < 1000; i++) {
        const char *ext = strrchr(base_name, '.');
        if (ext && ext != base_name) {
            // 확장자가 있는 경우: 파일명(숫자).확장자
            size_t name_len = ext - base_name;
            memcpy(unique_name, base_name, name_len);
            unique_name[name_len] = '\0';
            strcat(unique_name, "(");
            
            char num_str[16];
            sprintf(num_str, "%d", i);
            strcat(unique_name, num_str);
            strcat(unique_name, ")");
            strcat(unique_name, ext);
        } else {
            // 확장자가 없는 경우: 파일명(숫자)
            strcpy(unique_name, base_name);
            strcat(unique_name, "(");
            
            char num_str[16];
            sprintf(num_str, "%d", i);
            strcat(unique_name, num_str);
            strcat(unique_name, ")");
        }
        
        snprintf(test_path, sizeof(test_path), "%s/%s", dest_dir, unique_name);
        if (access(test_path, F_OK) != 0) {
            return unique_name;
        }
    }
    
    // 1000개까지 시도해도 안되면 원본 이름 반환
    strcpy(unique_name, base_name);
    return unique_name;
}


// 동기 파일 복사
bool copy_file_sync(const char *src, const char *dest) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        return false;
    }

    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        close(src_fd);
        return false;
    }

    char buffer[8192];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            close(src_fd);
            close(dest_fd);
            unlink(dest);
            return false;
        }
    }

    close(src_fd);
    close(dest_fd);

    // 원본 파일의 권한 복사
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dest, st.st_mode);
    }

    return bytes_read == 0;
}

// 동기 디렉토리 복사
bool copy_directory_sync(const char *src, const char *dest) {
    // 대상 디렉토리 생성
    if (mkdir(dest, 0755) == -1 && errno != EEXIST) {
        return false;
    }

    DIR *dir = opendir(src);
    if (!dir) {
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[MAX_PATH_LEN];
        char dest_path[MAX_PATH_LEN];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        struct stat st;
        if (lstat(src_path, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!copy_directory_sync(src_path, dest_path)) {
                closedir(dir);
                return false;
            }
        } else {
            if (!copy_file_sync(src_path, dest_path)) {
                closedir(dir);
                return false;
            }
        }
    }

    closedir(dir);
    return true;
}

// 백그라운드 복사 스레드 함수
void* copy_thread_func(void* arg) {
    CopyTask* task = (CopyTask*)arg;

    bool success;
    if (task->is_directory) {
        success = copy_directory_sync(task->source_path, task->dest_path);
    } else {
        success = copy_file_sync(task->source_path, task->dest_path);
    }

    // 복사 실패 시 임시 파일 삭제
    if (!success) {
        if (task->is_directory) {
            rmdir(task->dest_path);
        } else {
            unlink(task->dest_path);
        }
    }

    // 작업 완료 표시
    task->is_running = false;

    return NULL;
}

// 특정 파일이 복사 중인지 확인
bool is_copying_file(const char *file_path) {
    pthread_mutex_lock(&g_tasks_mutex);

    CopyTask* current = g_copy_tasks;
    while (current) {
        if (current->is_running) {
            if (strcmp(current->dest_path, file_path) == 0) {
                pthread_mutex_unlock(&g_tasks_mutex);
                return true;
            }
        }
        current = current->next;
    }

    pthread_mutex_unlock(&g_tasks_mutex);
    return false;
}


// 파일 목록의 복사 상태 업데이트
void update_file_copy_status(FileEntry *files, int file_count, const char *current_path) {
    pthread_mutex_lock(&g_tasks_mutex);

    // 모든 파일을 기본 상태로 초기화
    for (int i = 0; i < file_count; i++) {
        files[i].copy_status = COPY_STATUS_NONE;
    }

    // 실행 중인 작업들과 비교
    CopyTask* current = g_copy_tasks;
    while (current) {
        if (current->is_running) {
            // 현재 디렉토리에 있는 파일인지 확인
            char task_dir[MAX_PATH_LEN];
            strcpy(task_dir, current->dest_path);
            char *last_slash = strrchr(task_dir, '/');
            if (last_slash) {
                *last_slash = '\0';  // 디렉토리 부분만 남김

                // 현재 디렉토리와 일치하는지 확인
                if (strcmp(task_dir, current_path) == 0) {
                    char *task_filename = last_slash + 1;

                    // 파일 목록에서 해당 파일 찾기
                    for (int i = 0; i < file_count; i++) {
                        if (strcmp(files[i].name, task_filename) == 0) {
                            files[i].copy_status = COPY_STATUS_IN_PROGRESS;
                            break;
                        }
                    }
                }
            }
        }
        current = current->next;
    }

    pthread_mutex_unlock(&g_tasks_mutex);
}

// 클립보드에 복사
bool copy_to_clipboard(const char *file_path) {
    // ".." 복사 방지 - basename 문제 해결
    const char *last_slash = strrchr(file_path, '/');
    const char *filename;
    if (last_slash) {
        filename = last_slash + 1;
    } else {
        filename = file_path;
    }
    
    if (strcmp(filename, "..") == 0) {
        return false;
    }
    
    pthread_mutex_lock(&g_clipboard_mutex);

    // 절대 경로로 변환
    char abs_path[MAX_PATH_LEN];
    if (file_path[0] != '/') {
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd))) {
            snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, file_path);
        } else {
            pthread_mutex_unlock(&g_clipboard_mutex);
            return false;
        }
    } else {
        strncpy(abs_path, file_path, sizeof(abs_path) - 1);
        abs_path[sizeof(abs_path) - 1] = '\0';
    }

    // 파일/디렉토리 존재 확인
    struct stat st;
    if (stat(abs_path, &st) == -1) {
        pthread_mutex_unlock(&g_clipboard_mutex);
        return false;
    }

    // 클립보드에 저장
    strncpy(g_clipboard.source_path, abs_path, sizeof(g_clipboard.source_path) - 1);
    g_clipboard.source_path[sizeof(g_clipboard.source_path) - 1] = '\0';
    g_clipboard.is_valid = true;
    g_clipboard.is_directory = S_ISDIR(st.st_mode);

    pthread_mutex_unlock(&g_clipboard_mutex);
    return true;
}

// 클립보드에서 붙여넣기 - 즉시 업데이트 지원
bool paste_from_clipboard(const char *dest_dir) {
    pthread_mutex_lock(&g_clipboard_mutex);

    if (!g_clipboard.is_valid) {
        pthread_mutex_unlock(&g_clipboard_mutex);
        return false;
    }

    // 원본 파일 이름 추출 - 안전한 방식
    const char *last_slash = strrchr(g_clipboard.source_path, '/');
    const char *base_name;
    if (last_slash) {
        base_name = last_slash + 1;
    } else {
        base_name = g_clipboard.source_path;
    }
    
    char *unique_name = generate_unique_name(dest_dir, base_name);
    
    char dest_path[MAX_PATH_LEN];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, unique_name);

    bool use_background = should_use_background_copy(g_clipboard.source_path);

    if (use_background) {
        // 백그라운드 복사 작업 생성
        CopyTask* task = malloc(sizeof(CopyTask));
        if (!task) {
            pthread_mutex_unlock(&g_clipboard_mutex);
            return false;
        }

        strcpy(task->source_path, g_clipboard.source_path);
        strcpy(task->dest_path, dest_path);
        strcpy(task->dest_dir, dest_dir);
        strcpy(task->dest_name, unique_name);
        task->is_directory = g_clipboard.is_directory;
        task->is_running = true;

        // 즉시 임시 파일 생성 (빈 파일/디렉토리) - 더 확실한 생성
        if (task->is_directory) {
            if (mkdir(dest_path, 0755) != 0 && errno != EEXIST) {
                free(task);
                pthread_mutex_unlock(&g_clipboard_mutex);
                return false;
            }
        } else {
            int fd = open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                free(task);
                pthread_mutex_unlock(&g_clipboard_mutex);
                return false;
            }
            close(fd);
        }

        // 스레드 생성
        if (pthread_create(&task->thread_id, NULL, copy_thread_func, task) != 0) {
            // 임시 파일 삭제
            if (task->is_directory) {
                rmdir(dest_path);
            } else {
                unlink(dest_path);
            }
            free(task);
            pthread_mutex_unlock(&g_clipboard_mutex);
            return false;
        }

        // 작업 목록에 추가
        pthread_mutex_lock(&g_tasks_mutex);
        task->next = g_copy_tasks;
        g_copy_tasks = task;
        pthread_mutex_unlock(&g_tasks_mutex);

        pthread_mutex_unlock(&g_clipboard_mutex);
        return true;
    } else {
        // 동기 복사
        bool success;
        if (g_clipboard.is_directory) {
            success = copy_directory_sync(g_clipboard.source_path, dest_path);
        } else {
            success = copy_file_sync(g_clipboard.source_path, dest_path);
        }

        pthread_mutex_unlock(&g_clipboard_mutex);
        return success;
    }
}
