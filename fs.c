// fs.c
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>

// 파일 유형을 문자열로 반환
static void get_file_type(mode_t mode, char *type, size_t size) {
    if (S_ISDIR(mode)) {
        strncpy(type, "디렉토리", size);
    } else if (S_ISREG(mode)) {
        strncpy(type, "일반 파일", size);
    } else if (S_ISLNK(mode)) {
        strncpy(type, "심볼릭 링크", size);
    } else if (S_ISFIFO(mode)) {
        strncpy(type, "FIFO", size);
    } else if (S_ISSOCK(mode)) {
        strncpy(type, "소켓", size);
    } else if (S_ISBLK(mode)) {
        strncpy(type, "블록 장치", size);
    } else if (S_ISCHR(mode)) {
        strncpy(type, "문자 장치", size);
    } else {
        strncpy(type, "알 수 없음", size);
    }
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

// 파일 목록 가져오기
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
    
    while ((entry = readdir(dir)) != NULL && count < max_files) {
        // . 과 .. 디렉토리는 건너뛰기 (선택사항)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
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
        get_file_type(file_stat.st_mode, files[count].type, sizeof(files[count].type));
        
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
        
        count++;
    }
    
    closedir(dir);
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
    get_file_type(file_stat.st_mode, file->type, sizeof(file->type));
    
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
