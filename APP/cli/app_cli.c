/* FreeRTOS includes. */
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+CLI includes. */
#include "FreeRTOSConfig.h"
#include "mid_cli.h" 
#include "hal_cli.h" 

/*
 * 添加一个新命令需要三步操作:
 * 1)创建一个命令结构体，设定相关信息
 *		build_var(cmd, help_info, parame_num);
 *			@cmd: 命令名称；
 *			@help_info: 帮助信息，输入各参数所代表的含义；
 *			@parame_num: 期望输入的参数个数，输入参数不匹配时，命令行无法识别；参数不包括命令本身；
 * 2)创建一个函数，这个函数用于命令参数解析和功能执行；
 *		static BaseType_t cmd##_handle( char *dest, argv_attribute argv, const char * const help_info);
 *			字符串cmd必须与build_var中的cmd保持一致；
 *			@dest: 需要返回信息时，向该地址写入字符串数据，如果需要多项数据返回，可参考info_handle函数结构；
 *			@argv: 提供给命令执行函数各个参数信息
 *				例如命令cp -r src dest，则argv[0] = "cp",argv[1] = "-r",argv[2] = "src",argv[3] = "dest"
 *			@help_info: 提供命令在构建时的帮助信息所在地址；
 * 3)将命令注册到命令行功能中:
 *		mid_cli_register(&cmd);
 *			@cmd: 构建命令时的命令名称；
 *		将上面的函数放到app_cli_register( void )函数中调用；
 */
build_var(info, "Device information.", 0);
build_var(date, "Display current time.", 0);
#if ( configUSE_TRACE_FACILITY == 1 )
build_var(top, "List all the tasks state.", 0);
#endif
build_var(isotp, "Test isotp function.Usage:isotp <datalen> <BS> <STmin>", 3);
build_var(clear, "Clear Terminal.", 0);

static void app_cli_register(void)
{
	mid_cli_register(&info);
	mid_cli_register(&date);
	mid_cli_register(&top);
	mid_cli_register(&isotp);
	mid_cli_register(&clear);
}

cmd_handle(info)
{
	SYSTEM_INFO  sysInfo;
	OSVERSIONINFOEX osvi;
	char buffer[50];

	(void) help_info;
	(void) argv;
	configASSERT(dest);
	GetSystemInfo(&sysInfo);
	sprintf_s(buffer, 50, "    OemId : %u\n", sysInfo.dwOemId);
	strcat_s(dest, 50, buffer);
	sprintf_s(buffer, 50, "    处理器架构 : %u\n", sysInfo.wProcessorArchitecture);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    页面大小 : %u\n", sysInfo.dwPageSize);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    应用程序最小地址 : 0x%X\n", sysInfo.lpMinimumApplicationAddress);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    应用程序最大地址 : 0x%X\n", sysInfo.lpMaximumApplicationAddress);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    处理器掩码 : 0x%X\n", sysInfo.dwActiveProcessorMask);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    处理器数量 : %u\n", sysInfo.dwNumberOfProcessors);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    处理器类型 : %u\n", sysInfo.dwProcessorType);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    虚拟内存分配粒度 : 0x%X\n", sysInfo.dwAllocationGranularity);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    处理器级别 : %u\n", sysInfo.wProcessorLevel);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	sprintf_s(buffer, 50, "    处理器版本 : %u\n", sysInfo.wProcessorRevision);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	osvi.dwOSVersionInfoSize=sizeof(osvi);
	if (GetVersionEx((LPOSVERSIONINFOW)&osvi))
	{
		sprintf_s(buffer, 50, "    Version     : %u.%u\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
		strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
		sprintf_s(buffer, 50, "    Build       : %u\n", osvi.dwBuildNumber);
		strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
		sprintf_s(buffer, 50, "    Service Pack: %u.%u\n", osvi.wServicePackMajor, osvi.wServicePackMinor);
		strcat_s(dest, cmdMAX_OUTPUT_SIZE - strlen(dest), buffer);
	}

	return pdFALSE;
}

cmd_handle(clear)
{
	( void ) help_info;
	configASSERT(dest);
	strcpy_s(dest, cmdMAX_OUTPUT_SIZE, "\033[H\033[J");

	return pdFALSE;
}

cmd_handle(date)
{
	time_t nowtime;
	struct tm timeinfo;

	(void) help_info;
	(void) argv;
	configASSERT(dest);

	time( &nowtime );
	localtime_s(&timeinfo, &nowtime );
	sprintf_s(dest, cmdMAX_OUTPUT_SIZE, "    Current time: %d-%d-%d %d %02d:%02d:%02d\n", 
		timeinfo.tm_year + 1900, 
		timeinfo.tm_mon + 1, 
		timeinfo.tm_mday, 
		timeinfo.tm_wday, 
		timeinfo.tm_hour, 
		timeinfo.tm_min, 
		timeinfo.tm_sec);

	return pdFALSE;
}

extern void isotp_test_main(unsigned short datalen, unsigned char bs, unsigned char stmin);
cmd_handle(isotp)
{
	(void) help_info;
	(void) argv;
	configASSERT(dest);

	isotp_test_main(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));

	return pdFALSE;
}

#if (configGENERATE_RUN_TIME_STATS == 1)
cmd_handle(top)
{
	/* 定义了最大可罗列的任务数，当实际任务数超过该值时最大显示为 MAX_TASKS，并给出警告 */
	#define MAX_TASKS 	(20)	
	
	static unsigned char curr_task = 0;
	static TaskStatus_t ptasks[MAX_TASKS];
	TaskStatus_t *p;
	unsigned int run_time;
	unsigned long num_of_tasks = uxTaskGetNumberOfTasks();
	char temp_str[30];
	
	(void) help_info;
	configASSERT(dest);

	if(ptasks == NULL)
		return pdFALSE;
	vTaskDelay(10);
	
	if(curr_task == 0)
	{
		if(num_of_tasks > MAX_TASKS)
		{
			num_of_tasks = MAX_TASKS;
			sprintf_s(dest, cmdMAX_OUTPUT_SIZE, "Warning: real num(%d) of tasks more than defines(%d)!\r\n", num_of_tasks, MAX_TASKS);
		}
		uxTaskGetSystemState(ptasks, num_of_tasks, NULL);
		sprintf_s(dest, cmdMAX_OUTPUT_SIZE, "        PRI     STATE   MEM(W)  %%TIME   NAME\r\n");
	}
	p = &ptasks[curr_task];
	sprintf_s(temp_str, 30, "\t%d\t", p->uxCurrentPriority);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE, temp_str);
	switch(p->eCurrentState)
	{
		case eRunning: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "run"); break;
		case eReady: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "ready"); break;
		case eBlocked: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "block"); break;
		case eSuspended: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "suspend"); break;
		case eDeleted: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "deleted"); break;
		case eInvalid: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "invalid"); break;
		default: strcat_s(dest, cmdMAX_OUTPUT_SIZE, "null"); break;
	}
	run_time = p->ulRunTimeCounter * 1000 / portGET_RUN_TIME_COUNTER_VALUE();
	/* sprintf_s(temp_str, 30, "\t%d\t%c%d\t", p->usStackHighWaterMark, run_time < 10 ? '<' : ' ', run_time < 10 ? 1 : run_time / 10); */
	sprintf_s(temp_str, 30, "\t%d\t=X\t", p->usStackHighWaterMark);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE, temp_str);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE, p->pcTaskName);
	strcat_s(dest, cmdMAX_OUTPUT_SIZE, "\r\n");
	curr_task ++;
	if(curr_task == num_of_tasks)
	{
		curr_task = 0;
		return pdFALSE;
	}
	else
		return pdTRUE;
}
#endif

void app_cli_init(unsigned char priority, char *t, TaskHandle_t *handle)
{
	mid_cli_init(400, priority, t);

	app_cli_register();
}

