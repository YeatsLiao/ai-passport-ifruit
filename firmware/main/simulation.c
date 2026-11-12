#include "simulation.h"
#include <string.h>
#include <stdio.h>
void sim_init(sim_t *s){memset(s,0,sizeof(*s));for(unsigned r=0;r<3;r++)s->balance[r]=10000;}
bool sim_valid(const sim_t *s){for(unsigned r=0;r<3;r++)if(s->balance[r]>1000000000 || s->count[r]>8 || (s->purchases[r]&~3u) || (s->rewarded[r]&~(1023u<<(r*10))))return false;return true;}
static void log_entry(sim_t *s,unsigned r,int32_t amount,unsigned kind){unsigned n=s->count[r];if(n==8){memmove(s->ledger[r],s->ledger[r]+1,7*sizeof(sim_entry_t));n=7;}else s->count[r]++;s->ledger[r][n]=(sim_entry_t){amount,kind};}
bool sim_reward(sim_t *s,unsigned r,unsigned task){if(r>=3 || task>=30 || task/10!=r || (s->rewarded[r]&(1u<<task)) || s->balance[r]>999999000)return false;s->rewarded[r]|=1u<<task;s->balance[r]+=1000;log_entry(s,r,1000,1);return true;}
bool sim_buy(sim_t *s,unsigned r,unsigned item){if(r>=3||item>=2|| (s->purchases[r]&(1u<<item)))return false;unsigned cost=item?5000:500;if(s->balance[r]<cost)return false;s->balance[r]-=cost;s->purchases[r]|=1u<<item;log_entry(s,r,-(int32_t)cost,2);return true;}
void sim_hangout(sim_t *s,unsigned r,unsigned contact){if(r<3)log_entry(s,r,0,100+contact);}
void sim_summary(const sim_t *s,unsigned r,char *out,unsigned cap){if(r>=3||!cap)return;int n=snprintf(out,cap,"OFFLINE SIMULATION\n\nBalance: $%lu\n\nDemo starting funds: $10000\nTask demo reward: $1000\nNot original mission payouts.\n\nRecent transactions:\n",(unsigned long)s->balance[r]);if(n<0)return;unsigned used=(unsigned)n;if(used>=cap)return;for(unsigned i=0;i<s->count[r];i++){sim_entry_t e=s->ledger[r][i];n=snprintf(out+used,cap-used,"%s %s$%ld\n",e.kind>=100?"Hang Out accepted":e.kind==1?"Task demo":"Donation",e.amount>=0?"+":"-",(long)(e.amount>=0?e.amount:-e.amount));if(n<0 || (unsigned)n>=cap-used)break;used+=(unsigned)n;}}
