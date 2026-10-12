#ifndef _FFT_CALCULATE_H
#define _FFT_CALCULATE_H

#include "stm32_dsp.h"
#include "stdint.h"

#define NPT 256//��������256 1024
#define ARR 100-1
#define PSC 72-1
#define ADC_LEN    256
#define ������ 72000000/((ARR+1)*(PSC+1))

extern double THD;
extern long InBufArray[NPT];//ADC�ɼ�����ֵ
extern long OutBufArray[NPT/2];
extern long MagBufArray[NPT/2];//�洢����г���ķ�ֵ

//(HZ)
void FFt_Init(void);//FFT��ʼ��
void GetPowerMag(void);//�������г����ֵ
uint32_t FFT_Get_Fre(void);//TFT�õ�Ƶ��
double THD_Calculate(float U1,float U2,float U3,float U4,float U5);//���ڼ���г��

#endif
