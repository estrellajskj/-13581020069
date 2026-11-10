#ifndef __HC_SR04_IIC_H__
#define __HC_SR04_IIC_H__

typedef struct __HC_SR04 c_hc_sr04;

typedef struct __HC_SR04
{
    void* this;

    int (*get)(const c_hc_sr04* this,float* dis_mm);
}c_hc_sr04;

c_hc_sr04 hc_sr04_creat(const c_my_iic* iic);

#endif

