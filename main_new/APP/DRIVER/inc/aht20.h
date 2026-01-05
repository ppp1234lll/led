#ifndef __AHT20_H_
#define __AHT20_H_

#include "./SYSTEM/sys/sys.h"

void aht20_init_function(void);
int8_t aht20_measure(double *humidity, double *temperature);
void aht20_test(void);
#endif
