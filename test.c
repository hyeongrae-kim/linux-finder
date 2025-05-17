#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ui.h"
#include "fs.h"
int main() {
    setlocale(LC_ALL, ""); // 로케일 설정

    // 파일 목록 저장을 위한 배열 및 현재 경로/디스크 정보를 위한 버퍼
    FileEntry files[MAX_FILES];
    int file_count = 0;
    char current_path[MAX_PATH_LEN];
    char disk_free[32];

    // 초기 상태로 현재 디렉토리의 파일 정보 로드
    get_current_path(current_path, sizeof(current_path));
    file_count = get_file_list(current_path, files, MAX_FILES);
    get_disk_free_space(current_path, disk_free, sizeof(disk_free));

    // 현재 경로와 총 파일 수 출력
    printf("현재 경로: %s\n", current_path);
    printf("총 파일 수: %d\n", file_count);
    printf("디스크 여유 공간: %s\n\n", disk_free);

    // 파일 목록 헤더 출력
    printf("%-30s %-16s %-20s %-10s\n", "이름", "유형", "수정 시간", "크기");
    printf("----------------------------------------------------------------------\n");

    // 각 파일 정보 출력
    for (int i = 0; i < file_count; i++) {
        printf("%-30s %-16s %-20s %-10s\n",
               files[i].name,
               files[i].type,
               files[i].mtime,
               files[i].size);
    }

    // 파일 모드도 출력 (8진수로)
    printf("\n파일 모드 정보 (8진수):\n");
    for (int i = 0; i < file_count; i++) {
        printf("%-30s: %o\n", files[i].name, files[i].mode);
    }

    return 0;
}


/*
int main() {
    setlocale(LC_ALL, ""); // 로케일 설정
    init_ui(); // ncurses 및 윈도우 초기화

    int current_selection = 0;
    int scroll_offset = 0;
    int ch;
    
    // 파일 목록 저장을 위한 배열 및 현재 경로/디스크 정보를 위한 버퍼
    FileEntry files[MAX_FILES];
    int file_count = 0;
    char current_path[MAX_PATH_LEN];
    char disk_free[32];
    
    // 초기 상태로 현재 디렉토리의 파일 정보 로드
    get_current_path(current_path, sizeof(current_path));
    file_count = get_file_list(current_path, files, MAX_FILES);
    get_disk_free_space(current_path, disk_free, sizeof(disk_free));

    while(1) {
        // 파일 목록 및 푸터 표시
        display_files(files, file_count, current_selection, scroll_offset);
        display_footer(current_path, file_count, disk_free);

        ch = getch(); // 사용자 입력 받기

        if (ch == 'q' || ch == 'Q') {
            break; // 'q' 입력 시 종료
        }

        // 터미널 크기 변경 이벤트 처리
        if (ch == KEY_RESIZE) {
            resize_ui();
            int screen_rows, screen_cols;
            getmaxyx(stdscr, screen_rows, screen_cols);
            int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;

            // 화면 크기 변경 후 선택 및 스크롤 위치 조정
            if (current_selection >= file_count) {
                current_selection = file_count > 0 ? file_count - 1 : 0;
            }
            
            // 스크롤 오프셋 조정: 선택된 항목이 화면에 보이도록
            if (current_selection < scroll_offset) {
                scroll_offset = current_selection;
            } else if (current_selection >= scroll_offset + main_content_display_height && main_content_display_height > 0) {
                scroll_offset = current_selection - main_content_display_height + 1;
            }
            
            // 스크롤 오프셋의 최대값 조정
            int max_scroll_offset = file_count - main_content_display_height;
            if (max_scroll_offset < 0) max_scroll_offset = 0;
            if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;
            if (scroll_offset < 0) scroll_offset = 0;

            continue;
        }

        switch(ch) {
            case KEY_UP:
                if (current_selection > 0) {
                    current_selection--;
                    if (current_selection < scroll_offset) {
                        scroll_offset = current_selection;
                    }
                }
                break;
                
            case KEY_DOWN:
                if (current_selection < file_count - 1) {
                    current_selection++;
                    int screen_rows, screen_cols;
                    getmaxyx(stdscr, screen_rows, screen_cols);
                    int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
                    if (main_content_display_height < 0) main_content_display_height = 0;

                    // 현재 선택이 화면 아래로 벗어나면 스크롤
                    if (main_content_display_height > 0 && current_selection >= scroll_offset + main_content_display_height) {
                        scroll_offset = current_selection - main_content_display_height + 1;
                    }
                }
                break;
                
            case KEY_PPAGE: // Page Up
                {
                    int screen_rows, screen_cols;
                    getmaxyx(stdscr, screen_rows, screen_cols);
                    int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
                    if (main_content_display_height < 0) main_content_display_height = 0;

                    current_selection -= main_content_display_height;
                    if (current_selection < 0) current_selection = 0;
                    scroll_offset = current_selection;
                    if (scroll_offset < 0) scroll_offset = 0;
                }
                break;
                
            case KEY_NPAGE: // Page Down
                {
                    int screen_rows, screen_cols;
                    getmaxyx(stdscr, screen_rows, screen_cols);
                    int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
                    if (main_content_display_height < 0) main_content_display_height = 0;

                    current_selection += main_content_display_height;
                    if (current_selection >= file_count) current_selection = file_count - 1;
                    scroll_offset = current_selection - main_content_display_height + 1;

                    // 스크롤 오프셋 조정
                    int max_scroll_offset = file_count - main_content_display_height;
                    if (max_scroll_offset < 0) max_scroll_offset = 0;
                    if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;
                    if (scroll_offset < 0) scroll_offset = 0;
                }
                break;
                
            case KEY_HOME:
                current_selection = 0;
                scroll_offset = 0;
                break;
                
            case KEY_END:
                {
                    int screen_rows, screen_cols;
                    getmaxyx(stdscr, screen_rows, screen_cols);
                    int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
                    if (main_content_display_height < 0) main_content_display_height = 0;

                    current_selection = file_count - 1;
                    if (file_count > main_content_display_height && main_content_display_height > 0) {
                        scroll_offset = file_count - main_content_display_height;
                    } else {
                        scroll_offset = 0;
                    }
                    if (scroll_offset < 0) scroll_offset = 0;
                }
                break;
                
            case '\n': // Enter 키
                if (current_selection >= 0 && current_selection < file_count) {
                    FileEntry *selected_file = &files[current_selection];
                    
                    // 선택한 파일 경로 생성
                    char selected_path[MAX_PATH_LEN];
                    snprintf(selected_path, sizeof(selected_path), "%s/%s", current_path, selected_file->name);
                    
                    if (is_directory(selected_file)) {
                        // 디렉토리인 경우 해당 디렉토리로 이동
                        if (change_directory(selected_path)) {
                            // 디렉토리 변경 성공 - 새 경로의 파일 목록 가져오기
                            get_current_path(current_path, sizeof(current_path));
                            file_count = get_file_list(current_path, files, MAX_FILES);
                            get_disk_free_space(current_path, disk_free, sizeof(disk_free));
                            
                            // 선택 및 스크롤 위치 초기화
                            current_selection = 0;
                            scroll_offset = 0;
                        } else {
                            // 디렉토리 변경 실패 - 에러 메시지 표시
                            mvprintw(0, 0, "디렉토리 변경 실패: %s", selected_file->name);
                            clrtoeol();
                            refresh();
                        }
                    } else if (is_executable(selected_file)) {
                        // 실행 가능한 파일인 경우 실행
                        close_ui(); // ncurses 종료
                        
                        if (execute_file(selected_path)) {
                            // 실행 후 ncurses 환경 복원
                            init_ui();
                            
                            // 파일 목록 갱신 (실행 후 변경 사항 반영)
                            get_current_path(current_path, sizeof(current_path));
                            file_count = get_file_list(current_path, files, MAX_FILES);
                            get_disk_free_space(current_path, disk_free, sizeof(disk_free));
                        } else {
                            // 실행 실패
                            init_ui();
                            mvprintw(0, 0, "파일 실행 실패: %s", selected_file->name);
                            clrtoeol();
                            refresh();
                        }
                    } else {
                        // 실행 불가능한 일반 파일
                        mvprintw(0, 0, "실행 불가능한 파일: %s", selected_file->name);
                        clrtoeol();
                        refresh();
                    }
                }
                break;
                
            case '.': // 상위 디렉토리로 이동 (옵션)
                if (change_directory("..")) {
                    get_current_path(current_path, sizeof(current_path));
                    file_count = get_file_list(current_path, files, MAX_FILES);
                    get_disk_free_space(current_path, disk_free, sizeof(disk_free));
                    current_selection = 0;
                    scroll_offset = 0;
                }
                break;
        }
        
        // 스크롤 오프셋과 선택 범위 유효성 검사
        if (scroll_offset < 0) scroll_offset = 0;
        if (file_count > 0) {
            int screen_rows, screen_cols;
            getmaxyx(stdscr, screen_rows, screen_cols);
            int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
            if (main_content_display_height < 0) main_content_display_height = 0;

            int max_scroll_offset = file_count - main_content_display_height;
            if (max_scroll_offset < 0) max_scroll_offset = 0;

            if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;

            if (current_selection >= file_count) current_selection = file_count - 1;
        } else { // 파일이 없을 경우
            scroll_offset = 0;
            current_selection = 0;
        }
        if (current_selection < 0) current_selection = 0;
    }

    close_ui(); // ncurses 종료 및 윈도우 정리

    printf("Finder 프로그램이 종료되었습니다.\n");
    if (file_count > 0 && current_selection >= 0 && current_selection < file_count) {
        printf("마지막 선택: %s\n", files[current_selection].name);
    }
    
    // 프로그램 종료 시 현재 디렉토리는 이미 변경되어 있음 (요구사항 6)

    return 0;
}
*/
