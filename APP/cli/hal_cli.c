#include "hal_cli.h"
#include <stdio.h>
#include <conio.h>

portBASE_TYPE hal_cli_data_tx(char *data, unsigned short len)
{
	while(len --)
	{
		printf("%c", *(data++));
	}
	return pdTRUE;
}

portBASE_TYPE hal_cli_data_rx(char *data, unsigned short len)
{
	*data = _getch();
	fflush(stdin);
	return 1;
}