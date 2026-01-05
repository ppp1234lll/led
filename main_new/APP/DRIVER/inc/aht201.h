#ifndef AHT20_H_
#define AHT20_H_

#include "./SYSTEM/sys/sys.h"

void aht201_init_function(void);
int8_t aht201_measure(double *humidity, double *temperature);
void aht201_test(void);

#endif
