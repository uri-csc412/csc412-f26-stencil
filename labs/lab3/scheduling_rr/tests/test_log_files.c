#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "minunit.h"

#define LOG_FOLDER "logFolder/"  // ensure trailing slash for proper path handling
#define MAX_THREADS 3           // max expected threads from our handout
#define MAX_PATH_LEN 40         // increased size for safety : max size of logFolder/thread10.txt?

#define LOG_FOLDER_PERMISSIONS 0755
#define LOG_FILE_PERMISSIONS 0755

// ANSI color codes for terminal output
#define COLOR_RESET "\033[0m"      // reset to default color when printing to terminal
#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED "\033[1;31m"
#define COLOR_BLUE "\033[1;34m"

void log_message(const char *message) {
    printf("%s✔ %s%s\n", COLOR_GREEN, message, COLOR_RESET);
}

void log_error(const char *message) {
    fprintf(stderr, "%s✘ Error: %s%s\n", COLOR_RED, message, COLOR_RESET);
}

int file_exists(int thread_number) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%sthread%d.txt", LOG_FOLDER, thread_number);
    return access(file_path, F_OK) == 0;
}

int directory_exists(const char *path) {
    if (!path) return 0;  // Prevent NULL pointer dereference
    struct stat info;
    return (stat(path, &info) == 0 && S_ISDIR(info.st_mode));
}

int check_permissions(const char *path, mode_t expected_perms) {
    struct stat st;
    if (stat(path, &st) != 0) {
        log_error("Could not access file permissions");
        perror("stat");
        return -1;
    }
    return (st.st_mode & 0777) == expected_perms;
}

MU_TEST(test_log_folder_exists) {
    if (directory_exists(LOG_FOLDER)) {
        log_message("logFolder exists!");
    } else {
        mu_fail("logFolder directory does not exist!");
    }
}

MU_TEST(test_thread_log_files_exists) {
    char error_msg[MAX_PATH_LEN + 32];
    for (int i = 1; i <= MAX_THREADS; i++) {
        if (!file_exists(i)) {
            snprintf(error_msg, sizeof(error_msg), "Missing log file: %sthread%d.txt", LOG_FOLDER, i);
            mu_fail(error_msg);
        }
    }
    log_message("All expected log files exist!");
}

MU_TEST(test_log_folder_permissions) {
    int result = check_permissions(LOG_FOLDER, LOG_FOLDER_PERMISSIONS);
    mu_assert(result != -1, "Could not check logFolder permissions!");
    if (result) {
        log_message("logFolder has correct permissions!");
    } else {
        mu_fail("Incorrect permissions for logFolder!");
    }
}

MU_TEST(test_log_file_permissions) {
    char file_path[MAX_PATH_LEN];
    char error_msg[MAX_PATH_LEN + 64]; // extra space for formatting
    int all_correct = 1; // flag to check if all files have correct permissions

    for (int i = 1; i <= MAX_THREADS; i++) {
        snprintf(file_path, sizeof(file_path), "%sthread%d.txt", LOG_FOLDER, i);
        int result = check_permissions(file_path, LOG_FILE_PERMISSIONS);

        if (result == -1) {
            snprintf(error_msg, sizeof(error_msg), "Could not check permissions for %s", file_path);
            mu_fail(error_msg);
        }
        if (!result) {
            snprintf(error_msg, sizeof(error_msg), "Incorrect permissions for %s", file_path);
            mu_fail(error_msg);
            all_correct = 0;
        }
    }

    if (all_correct) {
        log_message("All log files have correct permissions!");
    }
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_log_folder_permissions);
    MU_RUN_TEST(test_log_folder_exists);
    MU_RUN_TEST(test_log_file_permissions);
    MU_RUN_TEST(test_thread_log_files_exists);
}

int main() {
    printf("\n\033[1;34m::: Running Tests... :::\n\033[0m");
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    printf("\n\033[1;34m::: Tests Completed :::\n\033[0m");
    return 0;
}
