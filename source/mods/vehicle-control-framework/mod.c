/* Sole loader input subscriber. Providers arbitrate without owning physics. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/vehicle_adapter.h"
#include "../../runtime/native/services.h"
#include "sdk/control.h"
#include "../vehicle-framework/sdk/autonomous.h"
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(PTR,char *,DWORD);
int _fltused=0;
void *memcpy(void *to,const void *from,U64 n){unsigned char *d=to;const unsigned char *s=from;while(n--)*d++=*s++;return to;}
typedef U64 MCRoot;
typedef struct {MoonControlProvider def;char id[64];} MCProvider;
typedef struct {MCRoot car;int lease,injected,unattended,work,work_live;float original[4],last[4];unsigned char original_b[2],last_b[2];} MCCar;
static MCProvider providers[MC_LIMIT];static unsigned provider_count;
static const MoonAutonomous *autonomous;
static MCCar cars[MC_CARS];static PTR fields[6],alive_method;
static MCRoot (*hold)(PTR,BOOL);static PTR (*target)(MCRoot);static void (*drop)(MCRoot);
static void (*set_field)(PTR,PTR,void *);static int ready,starting;
static int mc_equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int mc_live(PTR car){int ok=1;void *a[]={car};PTR v=car?call(alive_method,0,a,&ok):0;return ok&&v&&*(unsigned char *)unbox(v);}
static int mc_id(const char *id){if(!id)return -1;for(unsigned i=0;i<provider_count;i++)if(mc_equal(id,providers[i].id))return (int)i;return -1;}
static MCCar *mc_car(PTR car,int create){
    if(!car)return 0;
    /* Existing physics callbacks do one liveness query, not one for every
     * earlier slot. Cleanup is needed only when adding a new vehicle. */
    for(unsigned i=0;i<MC_CARS;i++)if(cars[i].car&&target(cars[i].car)==car){
        if(mc_live(car))return cars+i;
        drop(cars[i].car);cars[i]=(MCCar){0};return 0;
    }
    if(!create||!mc_live(car))return 0;
    MCCar *free_slot=0;
    for(unsigned i=0;i<MC_CARS;i++){
        PTR p=cars[i].car?target(cars[i].car):0;
        if(cars[i].car&&!mc_live(p)){drop(cars[i].car);cars[i]=(MCCar){0};}
        if(!cars[i].car&&!free_slot)free_slot=cars+i;
    }
    if(!free_slot)return 0;free_slot->car=hold(car,0);free_slot->lease=-1;return free_slot->car?free_slot:0;
}
static int mc_register(const MoonControlProvider *p){
    if(!p||p->abi!=MC_ABI||p->size!=sizeof(*p)||!p->id||!p->callback||(p->kind!=MC_HARDWARE&&p->kind!=MC_ASSIST))return 0;
    unsigned n=0;while(p->id[n]){if(n>=63||!((p->id[n]>='a'&&p->id[n]<='z')||(p->id[n]>='0'&&p->id[n]<='9')||p->id[n]=='-'))return 0;n++;}if(!n)return 0;
    int found=mc_id(p->id);if(found>=0){MoonControlProvider *old=&providers[found].def;return old->callback==p->callback&&old->kind==p->kind&&old->priority==p->priority;}
    if(provider_count==MC_LIMIT)return 0;MCProvider *v=providers+provider_count++;*v=(MCProvider){0};v->def=*p;for(unsigned i=0;i<=n;i++)v->id[i]=p->id[i];v->def.id=v->id;return 1;
}
static int mc_acquire(const char *id,void *car){int i=mc_id(id);if(i<0||providers[i].def.kind!=MC_ASSIST)return 0;MCCar *s=mc_car(car,1);if(!s||(s->lease>=0&&s->lease!=i))return 0;s->lease=i;return 1;}
static void mc_work_off(MCCar *s,PTR car){if(s->work_live&&autonomous)autonomous->work(car,0);s->work_live=0;}
static int mc_release(const char *id,void *car){int i=mc_id(id);MCCar *s=mc_car(car,0);if(!s||i<0||s->lease!=i)return 0;s->lease=-1;s->unattended=0;s->work=0;mc_work_off(s,car);return 1;}
static int mc_owns(const char *id,void *car){int i=mc_id(id);MCCar *s=mc_car(car,0);return s&&i>=0&&s->lease==i;}
static int mc_can_unattended(void *car){return autonomous&&autonomous->supports(car);}
static int mc_unattended(const char *id,void *car,int enabled){int i=mc_id(id);MCCar *s=mc_car(car,0);if(!s||i<0||s->lease!=i||(enabled!=0&&enabled!=1))return 0;if(enabled&&!mc_can_unattended(car))return 0;s->unattended=enabled;if(!enabled){s->work=0;mc_work_off(s,car);}return 1;}
static int mc_can_work(void *car){return autonomous&&autonomous->can_work(car);}
static int mc_set_work(const char *id,void *car,int enabled){
    int i=mc_id(id);MCCar *s=mc_car(car,0);if(!s||i<0||s->lease!=i||(enabled!=0&&enabled!=1))return 0;
    if(enabled&&(!s->unattended||!mc_can_work(car)))return 0;s->work=enabled;if(!enabled)mc_work_off(s,car);return 1;
}
static const MoonControls api={MC_ABI,sizeof(MoonControls),mc_register,mc_acquire,mc_release,mc_owns,mc_unattended,mc_can_unattended,mc_set_work,mc_can_work};
__declspec(dllexport) const MoonControls *MoonLab_Controls_v1(unsigned abi,unsigned size){return abi==MC_ABI&&size==sizeof(api)?&api:0;}
static int mc_valid(const MoonVehicleInput *v){return v->valid==1&&v->steer==v->steer&&v->throttle==v->throttle&&v->steer>=-1&&v->steer<=1&&v->throttle>=-1&&v->throttle<=1&&v->brake<=1&&v->boost<=1;}
static void mc_read(PTR car,float *f,unsigned char *b){field_value(car,fields[0],f);field_value(car,fields[1],f+1);field_value(car,fields[2],b);field_value(car,fields[3],b+1);field_value(car,fields[4],f+2);field_value(car,fields[5],f+3);}
static void mc_write(PTR car,const float *f,const unsigned char *b){set_field(car,fields[0],(void *)f);set_field(car,fields[1],(void *)(f+1));set_field(car,fields[2],(void *)b);set_field(car,fields[3],(void *)(b+1));set_field(car,fields[4],(void *)(f+2));set_field(car,fields[5],(void *)(f+3));}
static int control_before(int id,Context *c,MoonVehicleInput *out){
    (void)id;PTR car=(PTR)c->rcx;if(autonomous&&autonomous->stepping(car))return 0;
    if(autonomous)autonomous->request(car,0);if(out)*out=(MoonVehicleInput){0};MCCar *s=mc_car(car,1);if(!s)return 0;
    float current[4];unsigned char buttons[2];mc_read(car,current,buttons);
    for(int i=0;i<4;i++)if(!s->injected||current[i]!=s->last[i])s->original[i]=current[i];
    for(int i=0;i<2;i++)if(!s->injected||buttons[i]!=s->last_b[i])s->original_b[i]=buttons[i];
    mc_write(car,s->original,s->original_b);s->injected=0;
    MoonControlFrame frame={MC_ABI,sizeof(frame),{1,s->original[1],s->original[0],s->original_b[0],s->original_b[1]},0};MoonVehicleInput best={0};int best_priority=-2147483647,have_hardware=0,assist_accepted=0;
    /* Provider order is stable; priority breaks ties only within hardware.
     * Every assist observes takeover and can stop cruise even without a lease. */
    for(unsigned kind=MC_HARDWARE;kind<=MC_ASSIST;kind++)for(unsigned i=0;i<provider_count;i++){
        MCProvider *p=providers+i;if(p->def.kind!=kind)continue;MoonVehicleInput proposal={0};
        (void)p->def.callback(car,&frame,&proposal);mc_write(car,s->original,s->original_b);
        if(!mc_valid(&proposal))continue;
        if(kind==MC_HARDWARE){
            if(!have_hardware||p->def.priority>best_priority){best=proposal;best_priority=p->def.priority;}have_hardware=1;
            if(proposal.steer<-.08f||proposal.steer>.08f||proposal.throttle<-.12f||proposal.throttle>.12f||proposal.brake||proposal.boost)frame.manual_active=1;
        }else if(!frame.manual_active&&s->lease==(int)i){best=proposal;assist_accepted=1;}
    }
    if(frame.manual_active){s->lease=-1;s->unattended=0;s->work=0;}
    if(mc_valid(&best)){
        s->last[0]=best.throttle;s->last[1]=best.steer;s->last[2]=best.steer;s->last[3]=best.steer;s->last_b[0]=(unsigned char)best.brake;s->last_b[1]=(unsigned char)best.boost;s->injected=1;mc_write(car,s->last,s->last_b);if(out)*out=best;
        if(s->unattended&&s->lease>=0&&assist_accepted&&autonomous){
            if(autonomous->request(car,&best)&&s->work)s->work_live=autonomous->work(car,1);
        }
    }if(!s->unattended||!s->work||!assist_accepted)mc_work_off(s,car);return 0;
}
static int before(int id,Context *c){return control_before(id,c,0);}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(ready)return 1;if(starting)return 0;starting=1;if(!bind_runtime(domain,game))return 0;
#define LOAD(N,S,T) N=(T)GetProcAddress(game,S);if(!N)return 0
    LOAD(hold,"il2cpp_gchandle_new",MCRoot(*)(PTR,BOOL));LOAD(target,"il2cpp_gchandle_get_target",PTR(*)(MCRoot));LOAD(drop,"il2cpp_gchandle_free",void(*)(MCRoot));LOAD(set_field,"il2cpp_field_set_value",void(*)(PTR,PTR,void *));
#undef LOAD
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        autonomous=runtime?(const MoonAutonomous *)runtime->find("vehicle-framework","autonomous",MVA_ABI,sizeof(MoonAutonomous)):0;
        if(!autonomous||autonomous->abi!=MVA_ABI||autonomous->size<sizeof(MoonAutonomous))return 0;
    }else{
    char path[1200];DWORD n=GetModuleFileNameA(0,path,1000);if(!n||n>=1000)return 0;while(n&&path[n-1]!='\\'&&path[n-1]!='/')n--;if(!n)return 0;
    cat(path+n,"MoonLab\\mods\\vehicle-framework\\mod.dll");PTR module=LoadLibraryA(path);if(!module)return 0;
    int (*start)(PTR,PTR)=(int(*)(PTR,PTR))GetProcAddress(module,"MoonLab_ModStart");MoonAutonomousGet get=(MoonAutonomousGet)GetProcAddress(module,"MoonLab_Autonomous_v1");
    if(!start||!get||!start(domain,game)||(autonomous=get(MVA_ABI,sizeof(MoonAutonomous)))==0)return 0;
    }
    PTR type=find_class("MoonCleaners","MoonPrototypeVehicle");int (*offset)(PTR)=(int(*)(PTR))GetProcAddress(game,"il2cpp_field_get_offset");if(!type||!offset)return 0;
    const char *names[]={"throttle","steer","brake","boost","<Steering>k__BackingField","airTurn"};const int offsets[]={280,284,301,303,116,472};
    for(int i=0;i<6;i++){fields[i]=get_field(type,names[i]);if(!fields[i]||offset(fields[i])!=offsets[i])return 0;}
    alive_method=method("UnityEngine","Object","op_Implicit",1);if(!alive_method)return 0;
    PTR fixed=method("MoonCleaners","MoonPrototypeVehicle","FixedUpdate",0);if(!fixed||*(PTR *)fixed!=(unsigned char *)game+0x4d0c80)return 0;
    if(!prepare_hook(fixed,100669323,0,game)||!install_vehicle_adapter(1,0,control_before))return 0;before_hook=before;
    (void)integer;(void)num;(void)boolean;if(!install_hooks())return 0;
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();const MoonServiceLimitV1 limits[]={{"max_providers",MC_LIMIT},{"max_cars",MC_CARS}};
        MoonServicePublicationLimitsV1 publication={.base={.size=sizeof(publication),.service_id="controls",.abi=MC_ABI,.api_size=sizeof(api),.api=&api},.limit_count=2,.reserved=0,.limits=limits};
        if(!runtime||!runtime->publish(&publication.base))return 0;
    }
    ready=1;starting=0;return 1;
}
