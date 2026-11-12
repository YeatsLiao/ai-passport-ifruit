#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct {int32_t amount;uint32_t kind;} sim_entry_t;
typedef struct {uint32_t balance[3],rewarded[3],purchases[3],count[3];sim_entry_t ledger[3][8];} sim_t;
void sim_init(sim_t *);
bool sim_valid(const sim_t *);
bool sim_reward(sim_t *,unsigned,unsigned);
bool sim_buy(sim_t *,unsigned,unsigned);
void sim_hangout(sim_t *,unsigned,unsigned);
void sim_summary(const sim_t *,unsigned,char *,unsigned);
