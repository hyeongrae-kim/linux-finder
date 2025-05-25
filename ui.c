#include "ui.h"      // ui.h에 선언된 함수들을 구현하기 위해 포함
#include <string.h>  // strlen, snprintf 등 문자열 처리 함수 사용
#include <stdlib.h>  // abs, exit 등 표준 라이브러리 함수 사용
#include <ncurses.h> // ncurses 함수를 사용하기 위해 필요
#include <stdio.h>   // fprintf, stderr 사용

// 주 내용(파일 목록) 표시용 윈도우와 푸터용 윈도우들을 위한 포인터
WINDOW *main_win = NULL;
WINDOW *footer_win_path = NULL;
WINDOW *footer_win_stats = NULL;

void init_ui() {
	putenv("NCURSES_NO_UTF8_ACS=1");
    initscr();              // ncurses 모드 시작
    clear();                // 화면 지우기
    noecho();               // 입력한 문자가 화면에 바로 보이지 않도록 설정
    raw();                  // cbreak() 대신 raw() 사용 - Ctrl 키 조합을 위해 필요
    curs_set(0);            // 커서 숨기기
    keypad(stdscr, TRUE);   // 특수 키(화살표 키 등) 사용 가능하도록 설정
    intrflush(stdscr, FALSE); // 인터럽트 키 무시 - Ctrl+C가 프로그램을 종료시키지 않도록

    if (has_colors()) {     // 터미널이 색상 지원하는지 확인
        start_color();      // 색상 사용 시작
		if (COLORS >= 8) {
            // 색상 쌍 초기화: (ID, 전경색, 배경색)
            init_pair(COLOR_PAIR_REGULAR, COLOR_WHITE, COLOR_BLACK);     // 일반: 흰색 글씨, 검은색 배경
            init_pair(COLOR_PAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);    // 강조: 검은색 글씨, 시안색 배경
            init_pair(COLOR_PAIR_FOOTER, COLOR_YELLOW, COLOR_BLACK);     // 푸터: 노란색 글씨, 검은색 배경
            
            // 복사 중 상태를 위한 색상 정의
            init_pair(COLOR_PAIR_COPYING, COLOR_BLUE, COLOR_BLACK);      // 복사 중: 파란색 글씨, 검은색 배경
            init_pair(COLOR_PAIR_COPYING_SELECTED, COLOR_WHITE, COLOR_BLUE); // 복사 중 선택된 상태: 흰색 글씨, 파란색 배경
        }
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
	refresh();        // stdscr 갱신
    wrefresh(main_win);     // 메인 윈도우 갱신
    wrefresh(footer_win_path);  // 경로 푸터 갱신
    wrefresh(footer_win_stats); // 통계 푸터 갱신
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
    int type_col_width = max_x / 5;
    int mtime_col_width = max_x / 5;
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

	// 각 컬럼의 시작 위치 계산
	int col1 = 0;
	int col2 = name_col_width + 1;
	int col3 = col2 + type_col_width + 1;
	int col4 = col3 + mtime_col_width + 1;
	
	// 헤더 출력
	wattron(main_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_REGULAR));
	mvwprintw(main_win, 0, col1, "%-*s", name_col_width, "Name");
	mvwprintw(main_win, 0, col2, "%-*s", type_col_width, "Type");
	mvwprintw(main_win, 0, col3, "%-*s", mtime_col_width, "Modified");
	mvwprintw(main_win, 0, col4, "Size");
	wattroff(main_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_REGULAR));

	// 화면에 표시될 수 있는 파일들만 루프
	for (int i = 0; i < window_height && (i + scroll_offset) < num_files; ++i) {
		int file_index = i + scroll_offset;
		if (file_index >= num_files) break;

		int display_row = i + 1; // 헤더 다음 줄부터 파일 정보 표시
		if (display_row >= max_y) break;

		// 복사 상태에 따른 색상 결정
		int color_pair;
		bool is_copying = (files[file_index].copy_status == COPY_STATUS_IN_PROGRESS);
		bool is_selected = (file_index == current_selection);
		
		if (is_selected) {
			// 선택된 항목
			if (is_copying) {
				color_pair = COLOR_PAIR_COPYING_SELECTED; // 복사 중이면서 선택된 상태 (흰색 글씨, 파란색 배경)
			} else {
				color_pair = COLOR_PAIR_HIGHLIGHT; // 일반 선택 상태 (검은색 글씨, 시안색 배경)
			}
			
			// 먼저 전체 행에 색상 배경 적용
			wattron(main_win, COLOR_PAIR(color_pair));
			// 행 전체를 공백으로 채워 배경색 적용
			for (int x = 0; x < max_x; x++) {
				mvwaddch(main_win, display_row, x, ' ');
			}

			// 이제 텍스트 출력
			mvwprintw(main_win, display_row, col1, "%-*s", name_col_width, files[file_index].name);
			mvwprintw(main_win, display_row, col2, "%-*s", type_col_width, files[file_index].type);
			mvwprintw(main_win, display_row, col3, "%-*s", mtime_col_width, files[file_index].mtime);
			mvwprintw(main_win, display_row, col4, "%s", files[file_index].size);
			wattroff(main_win, COLOR_PAIR(color_pair));
		} else {
			// 선택되지 않은 항목
			if (is_copying) {
				color_pair = COLOR_PAIR_COPYING; // 복사 중 (파란색 글씨, 검은색 배경)
			} else {
				color_pair = COLOR_PAIR_REGULAR; // 일반 상태 (흰색 글씨, 검은색 배경)
			}
			
			wattron(main_win, COLOR_PAIR(color_pair));
			mvwprintw(main_win, display_row, col1, "%-*s", name_col_width, files[file_index].name);
			mvwprintw(main_win, display_row, col2, "%-*s", type_col_width, files[file_index].type);
			mvwprintw(main_win, display_row, col3, "%-*s", mtime_col_width, files[file_index].mtime);
			mvwprintw(main_win, display_row, col4, "%s", files[file_index].size);
			wattroff(main_win, COLOR_PAIR(color_pair));
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

// 확인 다이얼로그 표시 함수
bool ui_show_confirmation_dialog(const char* message) {
    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols);
    
    // 다이얼로그 크기 및 위치 계산
    int dialog_width = screen_cols / 2;
    if (dialog_width < 40) dialog_width = screen_cols - 4; // 최소 너비 보장
    if (dialog_width > screen_cols - 4) dialog_width = screen_cols - 4;
    
    int dialog_height = 5;
    int start_y = (screen_rows - dialog_height) / 2;
    int start_x = (screen_cols - dialog_width) / 2;
    
    // 다이얼로그 창 생성
    WINDOW *dialog_win = newwin(dialog_height, dialog_width, start_y, start_x);
    if (!dialog_win) return false;
    
    // 테두리 설정 및 배경색 설정
    box(dialog_win, 0, 0);
    wbkgd(dialog_win, COLOR_PAIR(COLOR_PAIR_FOOTER));
    
    // 메시지 및 안내 표시
    mvwprintw(dialog_win, 1, 2, "%s", message);
    mvwprintw(dialog_win, 3, 2, "확인: Enter, 취소: ESC");
    
    wrefresh(dialog_win);
    
    // 입력 처리
    keypad(dialog_win, TRUE);
    noecho();
    curs_set(0);
    
    int ch;
    bool result = false;
    
    // 다이얼로그 입력 대기 루프
    while (1) {
        ch = wgetch(dialog_win);
        
        if (ch == '\n' || ch == KEY_ENTER) { // 확인 (Enter)
            result = true;
            break;
        } else if (ch == 27 || ch == KEY_MESSAGE || ch == 'n' || ch == 'N') { // 취소 (ESC, n)
            result = false;
            break;
        }
    }
    
    // 윈도우 정리 및 화면 갱신
    delwin(dialog_win);
    touchwin(stdscr);
    refresh();
    
    return result;
}

// 임시 메시지 표시 함수
void ui_display_temporary_message(const char* message, bool is_error) {
    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols);
    
    // 메시지 창 크기 및 위치 계산
    int msg_width = strlen(message) + 4;
    if (msg_width > screen_cols - 4) msg_width = screen_cols - 4;
    if (msg_width < 20) msg_width = 20;
    
    int msg_height = 3;
    int start_y = (screen_rows - msg_height) / 2;
    int start_x = (screen_cols - msg_width) / 2;
    
    // 메시지 창 생성
    WINDOW *msg_win = newwin(msg_height, msg_width, start_y, start_x);
    if (!msg_win) return;
    
    // 테두리 설정 및 배경색 설정
    box(msg_win, 0, 0);
    
    if (is_error) {
        wattron(msg_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_COPYING)); // 에러는 파란색으로
    } else {
        wattron(msg_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_FOOTER)); // 일반 메시지는 노란색으로
    }
    
    // 메시지 중앙 정렬
    int msg_len = strlen(message);
    int msg_x = (msg_width - msg_len) / 2;
    if (msg_x < 1) msg_x = 1;
    
    mvwprintw(msg_win, 1, msg_x, "%s", message);
    
    if (is_error) {
        wattroff(msg_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_COPYING));
    } else {
        wattroff(msg_win, A_BOLD | COLOR_PAIR(COLOR_PAIR_FOOTER));
    }
    
    wrefresh(msg_win);
    
    // 일정 시간 후 메시지 창 제거
    napms(1500); // 1.5초 대기
    
    // 윈도우 정리 및 화면 갱신
    delwin(msg_win);
    touchwin(stdscr);
    refresh();
}

// 복사 진행률 표시 함수
void ui_display_copy_progress(CopyTask* task) {
    if (!task) return;
    
    int screen_rows, screen_cols;
    getmaxyx(stdscr, screen_rows, screen_cols);
    
    // 진행률 창 크기 및 위치 계산
    int progress_width = screen_cols / 2;
    if (progress_width < 40) progress_width = screen_cols - 4;
    if (progress_width > screen_cols - 4) progress_width = screen_cols - 4;
    
    int progress_height = 5;
    int start_y = screen_rows - progress_height - 2;
    int start_x = (screen_cols - progress_width) / 2;
    
    // 진행률 창 생성
    WINDOW *progress_win = newwin(progress_height, progress_width, start_y, start_x);
    if (!progress_win) return;
    
    // 테두리 설정 및 배경색 설정
    box(progress_win, 0, 0);
    wbkgd(progress_win, COLOR_PAIR(COLOR_PAIR_REGULAR));
    
    // 파일 이름 및 상태 표시
    mvwprintw(progress_win, 1, 2, "복사 중: %s", task->dest_name);
    
    // 진행률 계산
    pthread_mutex_lock(&task->progress_mutex);
    double progress_percent = 0.0;
    if (task->total_size > 0) {
        progress_percent = (double)task->copied_size / task->total_size * 100.0;
    }
    pthread_mutex_unlock(&task->progress_mutex);
    
    // 진행률 막대 표시
    int bar_width = progress_width - 10;
    int filled_width = (int)(bar_width * progress_percent / 100.0);
    
    mvwprintw(progress_win, 2, 2, "[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled_width) {
            waddch(progress_win, '=');
        } else {
            waddch(progress_win, ' ');
        }
    }
    wprintw(progress_win, "] %.1f%%", progress_percent);
    
    // 취소 안내 표시
    mvwprintw(progress_win, 3, 2, "취소: ESC");
    
    wrefresh(progress_win);
    delwin(progress_win);
}

// 복사 작업 취소 확인 함수
bool ui_confirm_cancel_copy(const char* filename) {
    char message[MAX_PATH_LEN + 30];
    snprintf(message, sizeof(message), "'%s' 복사를 취소하시겠습니까?", filename);
    return ui_show_confirmation_dialog(message);
}
