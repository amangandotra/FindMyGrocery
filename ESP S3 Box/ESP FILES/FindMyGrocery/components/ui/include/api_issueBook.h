#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start background task to fetch issue confirmation data from server
 * Uses:
 *   g_issue_session.copy_uid
 *   g_issue_session.enrollment_no
 *
 * On success:
 *   - fills book_name
 *   - fills book_code
 *   - fills user_name
 *   - fills due_date
 *   - navigates to Issue Final Confirm screen
 */
void start_issue_fetch_task(void);

#ifdef __cplusplus
}
#endif
