#include "world.h"
#include <string.h>
const char *const world_roles[3]={"Michael","Franklin","Trevor"};
void world_init(world_t *w){memset(w,0,sizeof(*w));w->rng=1;w->wait_ms=45000;}
static uint32_t mask(unsigned role){return 3u<<(role*2);}
bool world_valid(const world_t *w){
 if(w->role>=3 || !w->rng || w->wait_ms>180000)return false;
 for(unsigned i=0;i<3;i++)if(((w->active[i]|w->done[i])&~mask(i)) || (w->active[i]&w->done[i]))return false;
 return true;
}
bool world_step(world_t *w,uint32_t elapsed,bool eligible){
 if(!eligible)return false;
 if(elapsed<w->wait_ms){w->wait_ms-=elapsed;return false;}
 w->rng^=w->rng<<13;w->rng^=w->rng>>17;w->rng^=w->rng<<5;
 w->wait_ms=90000+w->rng%90001;
 uint32_t available=mask(w->role)&~(w->active[w->role]|w->done[w->role]);
 if(!available || w->active[w->role])return false;
 unsigned first=w->role*2+(w->rng&1);
 if(!(available&(1u<<first)))first=w->role*2+((first+1)&1);
 w->active[w->role]|=1u<<first;return true;
}
bool world_complete(world_t *w,unsigned task){
 if(task>=6 || !(mask(w->role)&(1u<<task)) || !(w->active[w->role]&(1u<<task)))return false;
 w->active[w->role]&=~(1u<<task);w->done[w->role]|=1u<<task;return true;
}
int world_task(const world_t *w,unsigned row){for(unsigned i=0;i<6;i++)if(w->active[w->role]&(1u<<i)){if(!row--)return (int)i;}return -1;}
unsigned world_count(const world_t *w,bool done){uint32_t bits=done?w->done[w->role]:w->active[w->role];unsigned n=0;while(bits){n+=bits&1;bits>>=1;}return n;}
