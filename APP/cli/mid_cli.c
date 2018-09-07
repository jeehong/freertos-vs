#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Utils includes. */
#include "hal_cli.h"
#include "mid_cli.h"

/* os environment */
#include "task.h"

#define CLI_VERSION_MAJOR		(0)		/* ���汾�� */
#define CLI_VERSION_MINOR		(2)		/* �ΰ汾�� */

/** @breif ����ÿ����������ַ�����Ϊ29��+'\0'*/
#define	cmdMAX_STRING_SIZE		30
/** @breif ����ÿ������������ַ�������������Ϊ7�������������*/
#define	cmdMAX_VARS_SIZE		7
/** @breif dump����һ������ӡ����������byte*/
#define COLLECT_MAX_SIZE		128
/** @breif dump���� ÿ�д�ӡ�ֽ���*/
#define ONE_LINE_MAX_BYTES		16

/** @breif ��ǰȨ��״̬*/
enum passwd_state 
{
	PASSWD_INCORRECT = 0,
	PASSWD_CORRECT = 1,
};

typedef struct _list_command_t
{
	const struct _command_t *module;	/**< ����������Ϣ*/
	struct _list_command_t *next;		/**< ������һ������ڵ�*/
} list_command_t;

typedef struct _mid_cli_t
{
	const char * prefix;							/**< �����з���ǰ׺��Ϣ*/
	struct _list_command_t list_head;				/**< ��������ͷ*/
	#ifdef CLI_SUPPORT_PASSWD
	enum passwd_state permission;					/**< Ȩ��*/
	#endif
	char output_string[cmdMAX_OUTPUT_SIZE];			/**< ���������*/
	char whole_command[cmdMAX_INPUT_SIZE];			/**< ���뻺����*/
	char vars[cmdMAX_VARS_SIZE][cmdMAX_STRING_SIZE];/**< ��������ַ���������*/
	char *argv[cmdMAX_VARS_SIZE];					/**< ��������ַ���ָ��*/
	TaskHandle_t task_handle;						/**< ������*/
} mid_cli_t;

/* DEL acts as a backspace. */
#define cmdASCII_DEL		( 0x7F )
#define cmdASCII_BS			'\b'
#define cmdASCII_NEWLINE	'\n'
#define cmdASCII_HEADLINE	'\r'
#define cmdASCII_STRINGEND	'\0'
#define cmdASCII_SPACE		' '
#define	cmdASCII_TILDE		'~'

#define cli_malloc(wanted_size)		pvPortMalloc(wanted_size)

static int8_t mid_cli_string_split(char **dest, const char *commandString);
static void mid_cli_console_task(void *pvParameters);
static BaseType_t mid_cli_parse_command(const char * const input, char *dest, size_t len);
static void assist_print_task(void);

#ifdef CLI_SUPPORT_PASSWD
static const char * const passwd = "jhg";
static const char * const incorrect_passwd_msg = "Password incorrect!\r\n";
static const char * const input_passwd_msg = "Input password: ";
#endif
static const char * const memory_allocate_error = ": Allocate memory faild!\r\n";
static const char * const parame_overflow_error = ": error, parame overflow!\r\n";
static const char * def_prefix = "Terminal ";
static const char * const pc_new_line = "\r\n";
static const char * const backspace = " \b";
static const char * error_remind = " not be recognised. Input 'help' to view available commands.\r\n";
const char allocate_cli_error[] = "struct cli";

build_var(help, "Lists all the registered commands.", 0);

static struct _mid_cli_t *cli = NULL;

/*
 * ����ע�ắ��
 */
BaseType_t mid_cli_register(const struct _command_t * const p)
{
    struct _list_command_t *last_cmd_in_list = &cli->list_head, *new_list_item;
    BaseType_t ret = pdFAIL;

	/* Check the parameter is not NULL. */
	configASSERT(p);

	if(p->expect_parame_num > cmdMAX_VARS_SIZE)
	{
		hal_cli_data_tx(( char *)p->command, strlen(p->command));
		hal_cli_data_tx(( char *)parame_overflow_error, strlen(parame_overflow_error));
		return ret;
	}
	while (last_cmd_in_list->next != NULL)
	{
		last_cmd_in_list = last_cmd_in_list->next;
	}
	/* ��������ڵ����Ӹ������� */
	new_list_item = (list_command_t *) cli_malloc(sizeof(list_command_t));
	configASSERT(new_list_item);

	if(new_list_item != NULL)
	{
		taskENTER_CRITICAL();
		/* ����µĳ�Ա������ */
		new_list_item->module = p;
		/* ����β�ڵ��־ */
		new_list_item->next = NULL;
		/* �����³�Ա */
		last_cmd_in_list->next = new_list_item;
		/* ��¼��ǰ�ڵ㣬�ȴ���һ���ڵ㱻���� */
		last_cmd_in_list = new_list_item;
		taskEXIT_CRITICAL();
		ret = pdPASS;
	}
	else
	{
		hal_cli_data_tx(( char *)p->command, strlen(p->command));
		hal_cli_data_tx(( char *)memory_allocate_error, strlen(memory_allocate_error));
	}
	
	return ret;
}

int mid_cli_init(unsigned short usStackSize, UBaseType_t uxPriority, char *t)
{
	unsigned char i;

	cli = (struct _mid_cli_t *)cli_malloc(sizeof(*cli));
	if(cli == NULL)
	{
		hal_cli_data_tx((char *)allocate_cli_error, strlen(allocate_cli_error));
		hal_cli_data_tx((char *)memory_allocate_error, strlen(memory_allocate_error));
		return -1;
	}
	cli->list_head.module = &help;
	cli->list_head.next = NULL;
	#ifdef CLI_SUPPORT_PASSWD
	cli->permission = PASSWD_INCORRECT;
	#endif
	for(i = 0; i < cmdMAX_VARS_SIZE; i ++)
	{
		cli->argv[i] = &cli->vars[i][0];
	}
	cli->prefix = (t == NULL ? def_prefix : t);
	xTaskCreate(mid_cli_console_task, "Command line", usStackSize, NULL, uxPriority, &cli->task_handle);
	
	return 0;
}

static portTASK_FUNCTION( mid_cli_console_task, pvParameters )
{
	unsigned char input_index = 0;
	unsigned char status = 0;
	char input_char;

	/* Stop warnings. */
	( void ) pvParameters;

	for(;;)
	{
		/* ������� */
		if(status == 0)
		{
			/* �ȴ��ն����� */
			while(hal_cli_data_rx(&input_char, 0) < 0)
			{
				vTaskDelay(5);
			}
			/* �������ݻ��� */
			if(input_char != cmdASCII_BS || input_index)
			{
				#ifdef CLI_SUPPORT_PASSWD
				if(cli->permission != PASSWD_INCORRECT)
				{
					hal_cli_data_tx(&input_char, sizeof(input_char));
				}
				#else
				hal_cli_data_tx(&input_char, sizeof(input_char));
				#endif
			}
			/* ����������� */
			if(input_char == cmdASCII_NEWLINE || input_char == cmdASCII_HEADLINE)
			{
			#ifdef CLI_SUPPORT_PASSWD
				/* Ȩ���ж� */
				if(cli->permission == PASSWD_INCORRECT)
					status = 1;
				else
					status = 2;	/* ����������ϣ�����Ȩ���ж� */
			#else
				status = 2;	/* ֱ�ӽ����������Ȩ���ж� */
			#endif
				hal_cli_data_tx(( char *)pc_new_line, strlen(pc_new_line));
			}
			else			/* ����׶Σ���֧��backspace��ɾ���� */
			{
				if(input_char == cmdASCII_BS && input_index > 0)
				{
					input_index --;
					cli->whole_command[input_index] = cmdASCII_STRINGEND;
					hal_cli_data_tx(( char *)backspace, strlen(backspace));
				}
				else if((input_char >= cmdASCII_SPACE) 
					&& (input_char <= cmdASCII_TILDE)
					&& input_index < cmdMAX_INPUT_SIZE)
				{
					cli->whole_command[input_index] = input_char;
					input_index ++;
				}
			}
		}
		/* ��ȫ��֤ */
		if(status == 1)
		{
			if(strcmp(passwd, cli->whole_command))
			{
				if(input_index)
				{
					hal_cli_data_tx(( char *)incorrect_passwd_msg, strlen(incorrect_passwd_msg));
				}
				hal_cli_data_tx(( char *)input_passwd_msg, strlen(input_passwd_msg));
				status = 0;
			}
			else
			{
				cli->permission = PASSWD_CORRECT;
				status = 2;
			}
			memset(cli->whole_command, 0, strlen(cli->whole_command));
			input_index = 0;
		}
		/* ������� */
		if(status == 2)
		{
			if(input_index != 0)
			{
				BaseType_t reted;
				
				do
				{
					cli->output_string[0] = '\0';
					reted = mid_cli_parse_command(cli->whole_command, cli->output_string, cmdMAX_OUTPUT_SIZE);
					hal_cli_data_tx(( char *)cli->output_string, strlen(cli->output_string));
				} while(reted != pdFALSE);
				memset(cli->whole_command, 0, strlen(cli->whole_command));
				input_index = 0;
			}
			hal_cli_data_tx(( char *)cli->prefix, strlen(cli->prefix));
			status = 0;
		}
	}
}

/* Ĭ�Ϸָ��Ϊ cmdASCII_SPACE ���β�� 0 */
static BaseType_t mid_cli_parse_command(const char * const input, char *dest, size_t len)
{
	const struct _list_command_t *cmd = NULL;
	BaseType_t ret = pdFALSE;
	const char *cmd_string;
	unsigned short cmd_string_len;

	for(cmd = &cli->list_head; cmd != NULL; cmd = cmd->next)
	{
		cmd_string = cmd->module->command;
		cmd_string_len = strlen(cmd_string);
		if(!strncmp(input, cmd_string, cmd_string_len))
		{
			/* ����������������������趨�ĸ���������Ϊ������Ч */
			if(mid_cli_string_split(cli->argv, input) == cmd->module->expect_parame_num)
			{
				ret = pdTRUE;
			}
			break;
		}
	}
	
    /* ���������������������Ҫ�� */
	if(cmd != NULL && ret == pdTRUE)
	{
		/* ִ��ע������Ļص����� */
		ret = cmd->module->handle(dest, cli->argv, cmd->module->help_info);
	}
	else
	{
		sprintf_s(dest, cmdMAX_OUTPUT_SIZE, "  '%s'%s", input, error_remind);
		ret = pdFALSE;
	}
	
	return ret;
}

/*
 * @dest: ���зֺõ����ݷ����Ӧ�ĵ�ַ
 * @cmd_string: ԭʼ�ַ�����Դ
 */
static int8_t mid_cli_string_split(char **dest, const char *cmd_string)
{
	int8_t segment = 0, index = 0;
	BaseType_t was_space = pdFALSE;

	while(*cmd_string != cmdASCII_STRINGEND)
	{
		/* ����ָ��ָ���cmdASCII_SPACE */
		if((*cmd_string) == cmdASCII_SPACE
			&& was_space == pdFALSE)
		{
			was_space = pdTRUE;
			index = 0;
			segment ++;
			/* ����������������֧�������������˳����� */
			if(segment >= cmdMAX_VARS_SIZE)
			{
				break;
			}
		}
		else if(index <= cmdMAX_STRING_SIZE - 1)
		{
			/* ����ȡ�����ַ�����������buffer���ռ�cmdMAX_STRING_SIZEʱ��ֹͣ��ȡ�����ȴ���һ���ε��� */
			if(index == cmdMAX_STRING_SIZE - 1)
			{
				dest[segment][index] = '\0';
			}
			else
			{
				dest[segment][index ++] = *cmd_string;
			}
			was_space = pdFALSE;
		}
		cmd_string ++;
	}
	return segment;
}

cmd_handle(help)
{
	static struct _list_command_t *cmd = NULL;
	
	( void ) help_info;

	configASSERT( dest );

	if(cmd == NULL)
	{
		cmd = &cli->list_head;
		sprintf_s(dest, cmdMAX_OUTPUT_SIZE, "        COMMAND HELP              [VER:%d.%d]\r\n", CLI_VERSION_MAJOR, CLI_VERSION_MINOR);
	}
	else
	{
	    strcat_s(dest, cmdMAX_OUTPUT_SIZE, "\t");
	    strcat_s(dest, cmdMAX_OUTPUT_SIZE, cmd->module->command);
	    strcat_s(dest, cmdMAX_OUTPUT_SIZE, "\t");
	    strcat_s(dest, cmdMAX_OUTPUT_SIZE, cmd->module->help_info);
		strcat_s(dest, cmdMAX_OUTPUT_SIZE, "\r\n");
		cmd = cmd->next;
	}
	
	vTaskDelay(10);			/* ��Ҫ�ȴ����ڷ��ͣ����򴮿ڻ�������²��ִ�ӡ��ʧ */
	if(cmd == NULL)
		return pdFALSE;		/* ������Ϣ������� */
	else
		return pdTRUE;
}
