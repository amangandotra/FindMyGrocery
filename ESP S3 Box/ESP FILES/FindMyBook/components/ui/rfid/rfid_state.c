#include "rfid_state.h"
#include <string.h>

char g_issue_rfid_uid[32] = {0};

void rfid_clear_issue_uid(void)
{
    memset(g_issue_rfid_uid, 0, sizeof(g_issue_rfid_uid));
}
