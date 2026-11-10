#ifndef __SHT30_H__
#define	__SHT30_H__


typedef struct __SHT30 c_sht30;

typedef struct __SHT30
{
    void* this;
    
    int(*get)(const c_sht30* this,float* temp,float* humi);
}c_sht30;

c_sht30 sht30_create(const c_my_iic* iic);

#endif
