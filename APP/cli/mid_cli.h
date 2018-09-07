/**  @file Mid_cli.h
 * @brief ���������й���
 * @details �ù������õ��Դ��ڣ����ն���ʵ���˻�������
 			ͨ����������õ����ؽ������ʵ�ֶ�MCU���ƻ�״̬��⡣
 *
 * Example usage:\n
	1)����һ������ṹ�壬�趨�����Ϣ
		build_var(cmd, help_info, parame_num);
			cmd: �������ƣ�
			help_info: ������Ϣ�����������������ĺ��壻
			parame_num: ��������Ĳ������������������ƥ��ʱ���������޷�ʶ�𣻲��������������
	2)����һ�������������������������������͹���ִ�У�
		static BaseType_t cmd##_main( char *dest, argv_attribute argv, const char * const help_info);
			�ַ���cmd������build_var�е�cmd����һ�£�
			dest: ��Ҫ������Ϣʱ����õ�ַд���ַ������ݣ������Ҫ�������ݷ��أ��ɲο�info_main�����ṹ��
			argv: �ṩ������ִ�к�������������Ϣ
				��������cp -r src dest����argv[0] = "cp",argv[1] = "-r",argv[2] = "src",argv[3] = "dest"
			help_info: �ṩ�����ڹ���ʱ�İ�����Ϣ���ڵ�ַ��
	3)������ע�ᵽ�����й�����:
		mid_cli_register(&cmd);
			cmd: ��������ʱ���������ƣ�
			������ĺ����ŵ�app_cli_register( cmd )�����е��ã�
	\n
*/

/** @defgroup Mid_cli ���������й���
*/
#ifndef __MID_CLI_H__
#define __MID_CLI_H__

/** @breif �����ʱ������������*/
typedef char** const argv_attribute;

/** @breif ������ص������ṹ����*/
typedef BaseType_t (*module_func_handle)( char * /*dest*/, argv_attribute /*argv*/, const char * const /*help_info*/);

/** @breif ��������ṹ��*/
typedef struct _command_t
{
	const char * const command;			/**< ����*/
	const char * const help_info;		/**< ������Ϣ*/
	const module_func_handle handle;	/**< ִ�к���*/
	unsigned char expect_parame_num;	/**< ��������Ĳ��������������������*/
} command_t;

/**
* �����Ҫ����������֤���붨��ú�
*/
#ifndef CLI_SUPPORT_PASSWD
	#define CLI_SUPPORT_PASSWD
#endif

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE		256
#define	cmdMAX_OUTPUT_SIZE		4096

/** @ingroup Mid_cli
*
* Ϊ�µ������һ�����ݽṹ
* 
* @param var ����ǰ׺������������Ĳ���
* 
* @param help ������Ϣ���ַ�������
*
* @param want �����Ĳ�������������������ǰ׺����
*
* @return �޷���ֵ
*/
#define	build_var(var, help, want) 			\
static BaseType_t var##_main(char* /*dest*/, argv_attribute /*argv*/, const char* const/*help_info*/);		\
static const struct _command_t var =	{#var, help, var##_main, want}

/** @ingroup Mid_cli
*
* Ϊ�µ������һ���ص�
* 
* @param dest Ŀ���ڴ��ַ���������Ϣ����õ�ַ
* 
* @param argv �ն������������Ϣ��ÿ���ո��жϵ��ַ�������ÿһ���ṹ������
*
* @param help_info ����������Ӧ�İ�����Ϣ�׵�ַ
*
* @return 
*	pdTRUE: �˳��ú����󣬻�����ڴ˽��룬�ûص�����
*	pdFALSE: �˳�������ûص�����
*/
#define cmd_handle(cmd) static BaseType_t cmd##_main(char *dest, argv_attribute argv, const char * const help_info)

/** @ingroup Mid_cli
*
* ע��һ���µ�����
* 
* @param p �����������ָ��
*
* @return 
����ֵ|����
------- | --------- 
pdFAIL | ע��ʧ��
pdPASS  | ע��ɹ�
*/
BaseType_t mid_cli_register(const struct _command_t *const p);

/** @ingroup Mid_cli
*
* ��ʼ�������й���
* Ĭ��ϵͳ���ṩһ���������� help
* help ����Ὣ��ǰ���õ������Լ���Ӧ�İ�����Ϣ��ӡ����
* 
* @param usStackSize �����й��ܷ����ջ�ߴ磬����:>=300
*
* @param uxPriority �����й����������ȼ�������:������������ȼ���ͬ
*
* @param prjPrefix �����ǰ׺��һ������Ϊ��Ŀ����+�汾��: "cmd-1.0.0 "���������ΪNULL����Ĭ��ǰ׺ΪTerminal
*
* @return 
* ����ֵ|����
* ------- | --------- 
* -1 | ��ʼ��ʧ��
* 0 | ��ʼ���ɹ�
*/
int mid_cli_init(unsigned short usStackSize, UBaseType_t uxPriority, char *t);


#endif

