#include "ui.h"      // ui.h에 선언된 함수들을 구현하기 위해 포함
#include <string.h>  // strlen, snprintf 등 문자열 처리 함수 사용
#include <stdlib.h>  // abs, exit 등 표준 라이브러리 함수 사용
#include <ncurses.h> // ncurses 함수를 사용하기 위해 필요
#include <stdio.h>   // fprintf, stderr 사용

// 주 내용(파일 목록) 표시용 윈도우와 푸터용 윈도우들을 위한 포인터
WINDOW *main_win = NULL;
WINDOW *footer_win_path = NULL;
WINDOW *footer_win_stats = NULL;

// 푸터의 높이 정의
#define FOOTER_HEIGHT_PATH 1  // 경로 표시 푸터 라인 수
#define FOOTER_HEIGHT_STATS 1 // 통계 표시 푸터 라인 수
#define FOOTER_TOTAL_HEIGHT (FOOTER_HEIGHT_PATH + FOOTER_HEIGHT_STATS) // 총 푸터 높이

void init_ui() {
    initscr();              // ncurses 모드 시작
    clear();                // 화면 지우기
    noecho();               // 입력한 문자가 화면에 바로 보이지 않도록 설정
    cbreak();               // 버퍼링 없이 바로 입력 받도록 설정 (Enter 키 없이도 입력 처리)
    curs_set(0);            // 커서 숨기기
    keypad(stdscr, TRUE);   // 특수 키(화살표 키 등) 사용 가능하도록 설정

    if (has_colors()) {     // 터미널이 색상 지원하는지 확인
        start_color();      // 색상 사용 시작
        // 색상 쌍 초기화: (ID, 전경색, 배경색)
        init_pair(COLOR_PAIR_REGULAR, COLOR_WHITE, COLOR_BLACK);     // 일반: 흰색 글씨, 검은색 배경
        init_pair(COLOR_PAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);    // 강조: 검은색 글씨, 시안색 배경 (예시)
        init_pair(COLOR_PAIR_FOOTER, COLOR_YELLOW, COLOR_BLACK);     // 푸터: 노란색 글씨, 검은색 배경
    }

    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols); // 현재 터미널 화면의 크기(행, 열) 가져오기

    // 파일 목록을 표시할 메인 윈도우 생성 (푸터 공간 제외)
    main_win = newwin(screen_rows - FOOTER_TOTAL_HEIGHT, screen_cols, 0, 0);
    if (!main_win) { // 윈도우 생성 실패 시
        endwin();
        fprintf(stderr, "Error creating main window.\n");
        exit(1);
    }
    // 푸터용 윈도우들 생성
    // 경로 푸터는 화면 하단에서 총 푸터 높이만큼 위로 올라간 위치에 시작
    footer_win_path = newwin(FOOTER_HEIGHT_PATH, screen_cols, screen_rows - FOOTER_TOTAL_HEIGHT, 0);
    // 통계 푸터는 경로 푸터 바로 아래에 시작
    footer_win_stats = newwin(FOOTER_HEIGHT_STATS, screen_cols, screen_rows - FOOTER_TOTAL_HEIGHT + FOOTER_HEIGHT_PATH, 0);

    if (!footer_win_path || !footer_win_stats) { // 윈도우 생성 실패 시
        endwin();
        fprintf(stderr, "Error creating footer windows.\n");
        exit(1);
    }

    // 각 윈도우의 기본 배경색 설정
    wbkgd(stdscr, COLOR_PAIR(COLOR_PAIR_REGULAR)); // stdscr 배경도 설정
    wbkgd(main_win, COLOR_PAIR(COLOR_PAIR_REGULAR));
    wbkgd(footer_win_path, COLOR_PAIR(COLOR_PAIR_FOOTER));
    wbkgd(footer_win_stats, COLOR_PAIR(COLOR_PAIR_FOOTER));

    // 메인 윈도우에서 키 입력 받을 경우 필요
    keypad(main_win, TRUE);

    refresh_screen(); // 초기 화면 반영
}

void close_ui() {
    if (main_win) delwin(main_win);             // 메인 윈도우 삭제
    if (footer_win_path) delwin(footer_win_path); // 경로 푸터 윈도우 삭제
    if (footer_win_stats) delwin(footer_win_stats); // 통계 푸터 윈도우 삭제
    endwin();                                   // ncurses 모드 종료
}

void clear_main_content_area() {
    if (main_win) {
        werase(main_win); // 메인 윈도우 내용 지우기
        // box(main_win, 0, 0); // 선택 사항: 메인 윈도우에 테두리 그리기
    }
}

void display_files(FileEntry files[], int num_files, int current_selection, int scroll_offset) {
    if (!main_win) return; // 메인 윈도우가 없으면 함수 종료

    clear_main_content_area(); // 이전 내용 지우기

    int max_y, max_x;
    getmaxyx(main_win, max_y, max_x); // 메인 윈도우의 크기 가져오기

    // 메인 윈도우의 첫 번째 줄은 헤더로 사용될 수 있습니다.
    // 실제 파일 목록을 그릴 영역의 높이는 max_y - 1 입니다.
    int window_height = max_y - 1; // 헤더 라인 제외

    // 각 컬럼의 너비 정의 (대략적인 값, 동적으로 조절 가능)
    // 컬럼 간 최소 1칸의 공백 또는 구분자가 필요
    int name_col_width = max_x / 2;
    int type_col_width = max_x / 6;
    int mtime_col_width = max_x / 6;
    // 나머지 공간을 크기 컬럼에 할당 (헤더, 타입, 수정일 컬럼 너비 및 구분자/패딩 공간 고려)
    int size_col_width = max_x - name_col_width - type_col_width - mtime_col_width;

    // 각 컬럼 너비가 최소값 이상인지 확인 (화면이 매우 작을 때 크래시 방지)
    if (name_col_width < 1 || type_col_width < 1 || mtime_col_width < 1 || size_col_width < 1) {
         // 화면이 너무 작아 컬럼을 표시할 수 없음, 오류 처리 또는 최소 레이아웃 표시 고려
         mvwprintw(main_win, 0, 0, "Terminal too small");
         wrefresh(main_win);
         return;
    }

    // 컬럼 너비 조정 - 구분자 공간 확보 (예: 각 컬럼 뒤에 1칸 공백)
    name_col_width -= 1;
    type_col_width -= 1;
    mtime_col_width -= 1;
    // size_col_width는 남은 공간을 모두 사용

    // 헤더(컬럼명) 출력
    wattron(main_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_REGULAR)); // 굵게, 일반 색상 적용
    mvwprintw(main_win, 0, 0, "%-*s %-*s %-*s %s",
              name_col_width, "Name",
              type_col_width, "Type",
              mtime_col_width, "Modified",
              "Size"); // Size 컬럼은 남은 공간을 사용하므로 %-*s 대신 %s
    wattroff(main_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_REGULAR)); // 속성 해제

    // 화면에 표시될 수 있는 파일들만 루프 (스크롤 오프셋과 윈도우 높이 고려)
    for (int i = 0; i < window_height && (i + scroll_offset) < num_files; ++i) {
        int file_index = i + scroll_offset; // 실제 파일 배열에서의 인덱스
        if (file_index >= num_files) break; // 파일 개수 초과 시 중단

        int display_row = i + 1; // 헤더 다음 줄부터 파일 정보 표시
        if (display_row >= max_y) break; // 윈도우 높이 초과 시 중단 (실제 사용 가능 높이는 window_height지만 안전하게 체크)

        // 현재 선택된 항목이면 강조 색상 적용
        if (file_index == current_selection) {
            wattron(main_win, COLOR_PAIR(COLOR_PAIR_HIGHLIGHT)); // 강조 색상 적용
        } else {
            wattron(main_win, COLOR_PAIR(COLOR_PAIR_REGULAR));   // 일반 색상 적용
        }

        // 필드 내용이 너무 길 경우 자르기 위한 임시 버퍼 (컬럼 너비 + null 종료 문자)
        char name_display[name_col_width + 1];
        char type_display[type_col_width + 1];
        char mtime_display[mtime_col_width + 1];
        char size_display[size_col_width + 1]; // Size는 남은 공간 전부 사용

        // 각 필드 내용을 컬럼 너비에 맞게 자르거나 형식화
        snprintf(name_display, sizeof(name_display), "%.*s", name_col_width, files[file_index].name);
        snprintf(type_display, sizeof(type_display), "%.*s", type_col_width, files[file_index].type);
        snprintf(mtime_display, sizeof(mtime_display), "%.*s", mtime_col_width, files[file_index].mtime);
        snprintf(size_display, sizeof(size_display), "%.*s", size_col_width, files[file_index].size);

        // 파일 정보 출력 (각 컬럼 너비에 맞춰 왼쪽 정렬하여 출력)
        mvwprintw(main_win, display_row, 0, "%-*s %-*s %-*s %s", // %-*s 사용
                  name_col_width, name_display,
                  type_col_width, type_display,
                  mtime_col_width, mtime_display,
                  size_display); // size_display는 남은 공간이므로 너비 지정 안 함

        // 적용했던 색상 속성 해제
        if (file_index == current_selection) {
            wattroff(main_win, COLOR_PAIR(COLOR_PAIR_HIGHLIGHT));
        } else {
            wattroff(main_win, COLOR_PAIR(COLOR_PAIR_REGULAR));
        }
    }
    wrefresh(main_win); // 메인 윈도우 변경 사항 화면에 반영
}


void display_footer(const char* current_path, int num_items_in_dir, const char* disk_free_space) {
    if (!footer_win_path || !footer_win_stats) return; // 푸터 윈도우가 없으면 함수 종료

    int max_y_path, max_x_path;
    int max_y_stats, max_x_stats;

    getmaxyx(footer_win_path, max_y_path, max_x_path);   // 경로 푸터 윈도우 크기
    getmaxyx(footer_win_stats, max_y_stats, max_x_stats); // 통계 푸터 윈도우 크기

    // 푸터 첫 번째 줄: 현재 경로
    werase(footer_win_path); // 이전 내용 지우기
    wattron(footer_win_path, COLOR_PAIR(COLOR_PAIR_FOOTER) | A_BOLD); // 푸터 색상 및 굵게 적용
    // 경로가 너무 길 경우 자르기 위한 버퍼 (윈도우 너비 - "Path: " 길이 - 패딩 공간)
    char path_display[max_x_path - 7 + 1];
    snprintf(path_display, sizeof(path_display), "Path: %.*s", max_x_path - 7, current_path); // "Path: " 접두사 고려
    mvwprintw(footer_win_path, 0, 0, "%s", path_display); // 경로 출력 (왼쪽 정렬, 시작 열 0)
    wattroff(footer_win_path, COLOR_PAIR(COLOR_PAIR_FOOTER) | A_BOLD); // 속성 해제
    wrefresh(footer_win_path); // 경로 푸터 윈도우 변경 사항 화면에 반영

    // 푸터 두 번째 줄: 항목 수 및 디스크 공간
    werase(footer_win_stats); // 이전 내용 지우기
    wattron(footer_win_stats, COLOR_PAIR(COLOR_PAIR_FOOTER) | A_BOLD); // 푸터 색상 및 굵게 적용
    char stats_str[max_x_stats + 1]; // 통계 문자열 버퍼
    // 형식: "64개 항목 | 10GB 사용가능"
    snprintf(stats_str, sizeof(stats_str), "%d item(s) | %s", num_items_in_dir, disk_free_space);

    // 통계 문자열 가운데 정렬
    int stats_len = strlen(stats_str);
    int start_col_stats = (max_x_stats - stats_len) / 2;
    if (start_col_stats < 0) start_col_stats = 0; // 음수 방지 (윈도우가 텍스트보다 작을 경우)

    mvwprintw(footer_win_stats, 0, start_col_stats, "%s", stats_str); // 통계 정보 출력
    wattroff(footer_win_stats, COLOR_PAIR(COLOR_PAIR_FOOTER) | A_BOLD); // 속성 해제
    wrefresh(footer_win_stats); // 통계 푸터 윈도우 변경 사항 화면에 반영
}

// 터미널 크기 변경 시 UI 윈도우를 재구성하는 함수
void resize_ui() {
    int screen_rows, screen_cols;

    // 변경된 터미널 화면의 새로운 크기 가져오기
    getmaxyx(stdscr, screen_rows, screen_cols);

    // ncurses 내부 상태 갱신 (필요한 경우)
    // resizeterm(screen_rows, screen_cols);

    // 기존 윈도우 삭제
    if (main_win) delwin(main_win);
    if (footer_win_path) delwin(footer_win_path);
    if (footer_win_stats) delwin(footer_win_stats);

    // 화면 전체를 지우고 배경색으로 채워 깨끗하게 만듭니다.
    clear();
    refresh(); // stdscr을 지운 것을 화면에 반영

    // 새로운 크기에 맞춰 윈도우 다시 생성 및 배치

    // 메인 윈도우 생성: 새로운 화면 높이에서 푸터 높이를 뺀 영역을 차지합니다.
    main_win = newwin(screen_rows - FOOTER_TOTAL_HEIGHT, screen_cols, 0, 0);
    if (!main_win) {
        endwin();
        fprintf(stderr, "Error creating main window after resize.\n");
        exit(1);
    }

    // 푸터 윈도우 생성: 새로운 화면 높이를 기준으로 하단에 위치하도록 y 좌표를 계산합니다.
    int footer_path_y = screen_rows - FOOTER_TOTAL_HEIGHT;
    int footer_stats_y = screen_rows - FOOTER_TOTAL_HEIGHT + FOOTER_HEIGHT_PATH;

    footer_win_path = newwin(FOOTER_HEIGHT_PATH, screen_cols, footer_path_y, 0);
    footer_win_stats = newwin(FOOTER_HEIGHT_STATS, screen_cols, footer_stats_y, 0);

     if (!footer_win_path || !footer_win_stats) {
        endwin();
        fprintf(stderr, "Error creating footer windows after resize.\n");
        exit(1);
    }

    // 각 새로운 윈도우에 배경색 및 속성 다시 설정
    wbkgd(stdscr, COLOR_PAIR(COLOR_PAIR_REGULAR)); // stdscr 배경도 다시 설정
    wbkgd(main_win, COLOR_PAIR(COLOR_PAIR_REGULAR));
    wbkgd(footer_win_path, COLOR_PAIR(COLOR_PAIR_FOOTER));
    wbkgd(footer_win_stats, COLOR_PAIR(COLOR_PAIR_FOOTER));

    // 필요한 경우 새로운 윈도우에 대해 keypad 등 설정 재적용
    keypad(main_win, TRUE); // 메인 윈도우에서 키 입력 받을 경우 필요

    // 윈도우 변경 사항을 화면에 반영합니다.
    doupdate();

    // === 중요: 화면 내용 다시 그리기 필요 ===
    // 이 함수는 윈도우 구조만 재설정합니다. 실제 파일 목록이나 푸터 내용은
    // 이 함수를 호출한 후 메인 프로그램에서 최신 데이터를 가지고
    // display_files()와 display_footer() 함수를 다시 호출하여 그려야 합니다.
}


void refresh_screen() {
    // 모든 윈도우의 변경 사항을 효율적으로 화면에 반영합니다.
    doupdate();
    // 개별 wrefresh 호출 방식도 가능하지만, doupdate가 일반적으로 더 효율적입니다.
}