#ifndef __HAL_CLI__
#define __HAL_CLI__

#include "FreeRTOS.h"

portBASE_TYPE hal_cli_data_tx(char *data, unsigned short len);
portBASE_TYPE hal_cli_data_rx(char *data, unsigned short len);

#endif