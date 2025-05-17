#include <ncurses.h> // getch, KEY_UP 등 사용
#include <stdio.h>   // printf (ncurses 종료 후)
#include <stdlib.h>  // exit (필요시)
#include <string.h>  // strcpy, snprintf 등
#include <locale.h> // 로케일 설정을 위해 추가

#include "ui.h"      // ui.h에 선언된 함수 및 구조체 사용
#include "ui.c" // ui.c는 컴파일 시 링크되므로 여기서 include하지 않습니다.
#include "fs.h"
#include "fs.c" // 실제 파일 시스템 접근 로직으로 대체 필요


int main() {
    setlocale(LC_ALL, ""); // 모든 로케일 설정을 시스템 기본값으로 설정
    init_ui(); // ncurses 및 윈도우 초기화

    int current_selection = 0;
    int scroll_offset = 0;
    int ch;

    // ui.c에서 윈도우 크기 및 배치를 관리하므로 test.c에서 직접 윈도우를 다루거나 크기를 계산할 필요가 줄어듭니다.
    // display_files 함수 내부에서 메인 윈도우의 크기를 가져와 표시 가능한 아이템 수를 계산합니다.
    // 따라서 main_content_display_height 변수는 더 이상 test.c에서 유지할 필요가 없습니다.

    while(1) {
        // display_files 함수가 내부적으로 메인 윈도우의 크기를 사용합니다.
        // display_files 함수 호출 시 window_height를 넘겨줄 필요가 없습니다.
        display_files(mock_files, mock_file_count, current_selection, scroll_offset);
        display_footer(mock_current_path, mock_file_count, mock_disk_free);
        // refresh_screen(); // ui.c의 display_ 함수들 내부에서 wrefresh를 호출하므로 불필요

        ch = getch(); // 사용자 입력 받기

        if (ch == 'q' || ch == 'Q') {
            break; // 'q' 입력 시 종료
        }

        // 터미널 크기 변경 이벤트를 감지하고 resize_ui 함수 호출
        if (ch == KEY_RESIZE) {
            resize_ui(); // UI 윈도우 재구성
            // resize_ui 호출 후에는 화면 내용을 다시 그려야 합니다.
            // 루프의 시작 부분에서 display_files와 display_footer가 호출되므로 추가적인 화면 갱신 코드는 필요 없습니다.
            // 단, 현재 선택 항목이나 스크롤 오프셋의 유효성은 다시 확인해야 할 수 있습니다.
             int screen_rows, screen_cols;
             getmaxyx(stdscr, screen_rows, screen_cols);
             int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1; // 재계산 필요

             // 화면 크기 변경 후 선택 및 스크롤 위치 조정 로직 추가
             if (current_selection >= mock_file_count) {
                 current_selection = mock_file_count > 0 ? mock_file_count - 1 : 0;
             }
             // 스크롤 오프셋 조정: 선택된 항목이 화면에 보이도록 조정
             if (current_selection < scroll_offset) {
                 scroll_offset = current_selection;
             } else if (current_selection >= scroll_offset + main_content_display_height && main_content_display_height > 0) {
                 scroll_offset = current_selection - main_content_display_height + 1;
             }
             // 스크롤 오프셋의 최대값 조정
             int max_scroll_offset = mock_file_count - main_content_display_height;
             if (max_scroll_offset < 0) max_scroll_offset = 0;
             if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;
             if (scroll_offset < 0) scroll_offset = 0; // 스크롤 오프셋이 음수가 되지 않도록 보정

            continue; // 크기 변경 처리 후 나머지 입력 처리는 건너뛰고 다음 루프에서 화면을 다시 그립니다.
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
                if (current_selection < mock_file_count - 1) {
                    current_selection++;
                    // 메인 윈도우에 표시 가능한 아이템 높이를 다시 가져와 사용
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
                    if (current_selection >= mock_file_count) current_selection = mock_file_count - 1;
                    scroll_offset = current_selection - main_content_display_height + 1;

                    // 스크롤 오프셋이 너무 커지지 않도록 조정
                    int max_scroll_offset = mock_file_count - main_content_display_height;
                    if (max_scroll_offset < 0) max_scroll_offset = 0;
                    if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;
                    if (scroll_offset < 0) scroll_offset = 0; // 스크롤 오프셋이 음수가 되지 않도록 보정
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

                    current_selection = mock_file_count - 1;
                    if (mock_file_count > main_content_display_height && main_content_display_height > 0) {
                         scroll_offset = mock_file_count - main_content_display_height;
                    } else {
                        scroll_offset = 0;
                    }
                    // 스크롤 오프셋이 음수가 되지 않도록 보정
                    if (scroll_offset < 0) scroll_offset = 0;
                }
                break;
            case '\n': // Enter 키 (여기서는 간단한 메시지만)
                if (current_selection >= 0 && current_selection < mock_file_count) {
                    // ui.c에서 푸터 윈도우 포인터를 전역으로 관리하므로, 직접 접근하는 대신
                    // 메시지 표시를 위한 별도의 UI 함수를 ui.c에 추가하거나 다른 방식으로 처리하는 것이 좋습니다.
                    // 여기서는 간단히 표준 출력으로 메시지를 대체합니다.
                    // ncurses 모드에서는 printf 출력이 화면에 바로 나타나지 않을 수 있습니다.
                    // 실제 앱에서는 상태 바 또는 메시지 팝업 기능을 ui.c에 구현하여 사용하는 것이 일반적입니다.
                     char msg[256];
                     snprintf(msg, sizeof(msg), "Selected: %s", mock_files[current_selection].name);
                     // 임시 메시지 표시 (ncurses 호환 방식)
                     mvprintw(0, 0, "%s", msg); // 화면 상단에 메시지 출력
                     clrtoeol(); // 줄의 나머지 부분을 지웁니다.
                     refresh(); // stdscr 변경 사항 반영
                     // napms(1000); // 1초 대기 (사용자 입력이 블록됨) - 필요에 따라 사용
                     // 메인 루프에서 메시지를 지우는 로직 필요 (예: 일정 시간 후 또는 다음 입력 시)
                }
                break;
            // KEY_RESIZE 처리는 위로 이동했습니다.
        }
        // 스크롤 오프셋과 선택 범위 유효성 검사
        if (scroll_offset < 0) scroll_offset = 0;
        if (mock_file_count > 0) {
             int screen_rows, screen_cols;
             getmaxyx(stdscr, screen_rows, screen_cols);
             int main_content_display_height = (screen_rows - FOOTER_TOTAL_HEIGHT) - 1;
             if (main_content_display_height < 0) main_content_display_height = 0;

             int max_scroll_offset = mock_file_count - main_content_display_height;
             if (max_scroll_offset < 0) max_scroll_offset = 0;

             if (scroll_offset > max_scroll_offset) scroll_offset = max_scroll_offset;

            if (current_selection >= mock_file_count) current_selection = mock_file_count -1;
        } else { // 파일이 없을 경우
             scroll_offset = 0;
             current_selection = 0;
        }
         if (current_selection < 0) current_selection = 0;

    }

    close_ui(); // ncurses 종료 및 윈도우 정리

    printf("Finder test program finished.\n");
    if (mock_file_count > 0 && current_selection >= 0 && current_selection < mock_file_count) {
        printf("Last selection: %s\n", mock_files[current_selection].name);
    }

    return 0;
}