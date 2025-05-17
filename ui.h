#ifndef UI_H // 헤더 파일 중복 포함 방지
#define UI_H

#include <ncurses.h> // ncurses 라이브러리 사용을 위한 헤더
#include "fs.h"      // FileEntry 구조체와 MAX_FILES 등을 사용하기 위해 포함 (fs.h에 정의되어 있다고 가정)

// 색상 쌍(Color Pair) 정의 (사용자 정의 가능)
#define COLOR_PAIR_REGULAR 1   // 일반 텍스트용 색상 쌍 ID
#define COLOR_PAIR_HIGHLIGHT 2 // 선택된 항목 강조용 색상 쌍 ID
#define COLOR_PAIR_FOOTER 3    // 하단 정보 표시용 색상 쌍 ID

// 푸터 높이 정의
#define FOOTER_HEIGHT_PATH 1
#define FOOTER_HEIGHT_STATS 1
#define FOOTER_TOTAL_HEIGHT (FOOTER_HEIGHT_PATH + FOOTER_HEIGHT_STATS)

/**
 * @brief ncurses 화면 및 UI 설정을 초기화합니다.
 * 프로그램 시작 시 한 번 호출되어야 합니다.
 * ncurses 환경(색상, 입력 모드 등)을 설정합니다.
 */
void init_ui();

/**
 * @brief ncurses 화면을 닫고 UI 관련 리소스를 정리합니다.
 * 프로그램 종료 시 한 번 호출되어야 합니다.
 */
void close_ui();

/**
 * @brief 주 화면(window)에 파일 및 디렉토리 목록을 표시합니다.
 *
 * @param files 표시할 FileEntry 구조체 배열.
 * @param num_files 배열 내 총 파일/디렉토리 수.
 * @param current_selection 현재 선택된 항목의 인덱스.
 * @param scroll_offset 화면에 표시할 파일 목록의 시작 오프셋 (스크롤 처리용).
 * @param window_height 파일 목록을 표시할 영역의 높이.
 *
 * 파일 이름, 수정일, 크기, 종류를 나열하며, 현재 선택된 항목은 강조 표시됩니다.
 * (PDF 요구사항 1, 2, 4번 관련)
 */
void display_files(FileEntry files[], int num_files, int current_selection, int scroll_offset);

/**
 * @brief 화면 하단에 푸터(footer) 정보를 표시합니다.
 *
 * 푸터에는 다음 정보가 포함됩니다:
 * 1. 현재 절대 경로 (PDF 요구사항 3-1번)
 * 2. 현재 디렉토리의 항목 수 및 사용 가능한 디스크 공간 (PDF 요구사항 3-2번)
 *
 * @param current_path 현재 디렉토리의 절대 경로 문자열.
 * @param num_items_in_dir 현재 디렉토리 내 항목(파일/디렉토리)의 수.
 * @param disk_free_space 사용 가능한 디스크 공간을 나타내는 문자열 (예: "10GB 사용가능").
 */
void display_footer(const char* current_path, int num_items_in_dir, const char* disk_free_space);

/**
 * @brief 화면의 주 내용 영역을 지웁니다.
 * 파일 목록을 다시 그리기 전에 사용됩니다.
 */
void clear_main_content_area();

/**
 * @brief 물리적 화면을 새로고침하여 변경 사항을 반영합니다.
 */
void refresh_screen();

/**
 * @brief 터미널 크기 변경 시 UI 윈도우를 재구성합니다.
 */
void resize_ui();

#endif // UI_H
