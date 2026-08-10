#pragma once

typedef struct {
    char copy_uid[32];
    char enrollment_no[32];
    char book_name[64];
    char book_code[32];
    char user_name[64];
    char email[64];          
    char due_date[32];
    char otp[8]; 
} issue_session_t;


extern issue_session_t g_issue_session;
