#pragma once
#include <stdint.h>
#define COMM_LIMIT 12
enum { COMM_RECEIVED,COMM_DIALLED,COMM_MISSED,COMM_REJECTED };
enum { FOLLOW_CALL=1,FOLLOW_INVITE,FOLLOW_REFUSAL,FOLLOW_TASK,FOLLOW_MISSED,FOLLOW_TAXI };
typedef struct {uint16_t contact,minute,seconds;uint8_t role,status;} comm_log_t;
typedef struct {uint16_t contact;int16_t task;uint8_t role,kind,read,reserved;} comm_message_t;
typedef struct {
 uint32_t version,revision;uint8_t log_count,message_count,life_cursor[3],reserved[3];
 comm_log_t logs[COMM_LIMIT];comm_message_t messages[COMM_LIMIT];
 uint32_t dial_serial[3];uint8_t task_contact[30];
} comm_state_t;
