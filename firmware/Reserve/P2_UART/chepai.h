#ifndef __CHAPAI_H__
#define __CHAPAI_H__

#define LAN_PAI  1
#define LV_PAI   2
#define HA_PAI   3
typedef struct __CHEPAI c_chepai;

/*≥µ≈∆∂‘œÛ*/
typedef struct __CHEPAI
{
    void* this;
    
    int (*set_callback)(c_chepai* this,void* param,int (*callback)(void* param,u8 type,const char* no));
}c_chepai;

c_chepai chepai_create(u8 uart_id);

#endif