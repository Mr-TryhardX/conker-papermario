/* Deterministic controller for local asset research, not the shipped game.
 * Input is normal controller state; no RAM edits or progress grants. */
#define M64P_PLUGIN_PROTOTYPES
#include "m64p_plugin.h"
#include "m64p_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
static void* (*guest_memory)(int);
static void capture_guest(void){
    const char* flag=getenv("MERCY_CAPTURE_RAM_FLAG");
    const char* path=getenv("MERCY_CAPTURE_RAM_PATH");
    if(!flag||!path||!guest_memory||access(flag,F_OK))return;
    void* ram=guest_memory(1);if(!ram)return;
    FILE* out=fopen(path,"wb");if(!out)return;
    size_t size=fwrite(ram,1,0x800000,out);int result=fclose(out);
    if(size==0x800000&&!result){unlink(flag);fprintf(stderr,"[asset-capture] read-only 8 MiB guest snapshot written\n");}
}
EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle core,void* context,void (*debug)(void*,int,const char*)){
    (void)context;(void)debug;guest_memory=(void* (*)(int))dlsym(core,"DebugMemGetPointer");return M64ERR_SUCCESS;
}
EXPORT m64p_error CALL PluginShutdown(void){return M64ERR_SUCCESS;}
EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type* type,int* version,int* api,const char** name,int* capabilities){
    if(type)*type=M64PLUGIN_INPUT;
    if(version)*version=0x010000;
    if(api)*api=0x020100;
    if(name)*name="Cloud asset research controller";
    if(capabilities)*capabilities=0;
    return M64ERR_SUCCESS;
}
EXPORT void CALL InitiateControllers(CONTROL_INFO info){
    for(int i=0;i<4;++i){info.Controls[i].Present=i==0;info.Controls[i].RawData=0;info.Controls[i].Plugin=PLUGIN_NONE;}
}
EXPORT void CALL GetKeys(int controller,BUTTONS* keys){
    keys->Value=0;if(controller!=0)return;capture_guest();const char* path=getenv("MERCY_RESEARCH_INPUT");if(!path)return;
    FILE* file=fopen(path,"r");if(!file)return;unsigned mask=0;int x=0,y=0;
    if(fscanf(file,"%x %d %d",&mask,&x,&y)==3&&x>=-80&&x<=80&&y>=-80&&y<=80){keys->Value=mask&0xffff;keys->X_AXIS=x;keys->Y_AXIS=y;}
    fclose(file);
}
EXPORT int CALL RomOpen(void){return 1;}
EXPORT void CALL RomClosed(void){}
EXPORT void CALL ControllerCommand(int control,unsigned char* command){(void)control;(void)command;}
EXPORT void CALL ReadController(int control,unsigned char* command){(void)control;(void)command;}
EXPORT void CALL SDL_KeyDown(int mod,int key){(void)mod;(void)key;}
EXPORT void CALL SDL_KeyUp(int mod,int key){(void)mod;(void)key;}
