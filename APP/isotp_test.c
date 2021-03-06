#include "isotp.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#include "comm_typedef.h"

#define CLIENT_ADDRESS	0x766
#define SERVER_ADDRESS	0x706

#define TEST_BS (1UL)
#define TEST_STMIN (10UL)

static ERROR_CODE sender_test_send(struct phy_msg_t *msg);
static ERROR_CODE sender_test_receive(struct phy_msg_t *msg);
static ERROR_CODE receiver_test_send(struct phy_msg_t *msg);
static ERROR_CODE receiver_test_receive(struct phy_msg_t *msg);
static ERROR_CODE receiver_set_FS(struct isotp_t* msg);
static void debug_out(const char *fmt, ...);


static struct isotp_t sender, receiver;

static void debug_out(const char *fmt, ...)
{
	va_list vp;

	//pthread_mutex_lock(&dbg_mutex);
	va_start(vp, fmt);
	vprintf(fmt, vp);
	va_end(vp);
	//pthread_mutex_unlock(&dbg_mutex);
}

void rx_thread(void *arg)
{
	for(;;)
	{
		isotp_receive(&receiver);
	}
}

void isotp_test_main(unsigned short datalen, unsigned char bs, unsigned char stmin)
{
	uint16_t index;
	TaskHandle_t rc_task;

	/* 
	 * initialize sender parameters
	 * sender: isotp_send object
	 * CLIENT_ADDRESS: local address
	 * SERVER_ADDRESS: remote address
	 * NULL: don't need to set this parameter,because it is suitable for receiver.
	 * sender_test_send: isotp_send function of the sender at physical layer
	 * sender_test_receive: isotp_receive function of the sender at physical layer
	 */
	isotp_init(&sender, CLIENT_ADDRESS, SERVER_ADDRESS, NULL, sender_test_send, sender_test_receive);
	/* 
	 * initialize receiver parameters
	 * receiver: receiver object
	 * CLIENT_ADDRESS: remote address
	 * SERVER_ADDRESS: local address
	 * receiver_set_FS: Consecutive frame status control
	 * receiver_test_send: isotp_send function of the receiver at physical layer
	 * receiver_test_receive: isotp_receive function of the receiver at physical layer
	 */
	isotp_init(&receiver, SERVER_ADDRESS, CLIENT_ADDRESS, receiver_set_FS, receiver_test_send, receiver_test_receive);
	/*
	 * set special parameters of flow control status
	 * receiver: operate object
	 * ISOTP_FS_CTS: Continue To Send
	 * TEST_BS: (BS)
	 * TEST_STMIN: STmin
	 */
	fc_set(&receiver, ISOTP_FS_CTS, bs, stmin);

	/* create isotp_receive service */
	xTaskCreate(rx_thread,			/* The task that implements the command console. */
			"isotp_receive_test",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
			200,					/* The size of the stack allocated to the task. */
			NULL,					/* The parameter is not used, so NULL is passed. */
			1,						/* The priority allocated to the task. */
			&rc_task);				/* A handle is not required, so just pass NULL. */
	/* Test 1,single frame */
	sender.DL = 5UL;
	debug_out("Single Frame test,DL:%d\r\n", sender.DL);
	for(index = 0; index < sender.DL; index ++)
	{
		sender.Buffer[index] = (uint8_t)6UL;
	}
	isotp_send(&sender);

	/* Test 2,consecutive frame */
	if(datalen > ISOTP_FF_DL)
	{
		sender.DL = ISOTP_FF_DL;
	}
	else
	{
		sender.DL = datalen;
	}
	
	//pthread_mutex_init(&dbg_mutex, NULL);
	
	debug_out("Consecutive Frame test,DL:%d\r\n", sender.DL);
	for(index = 0; index < sender.DL; index ++)
	{
		sender.Buffer[index] = (uint8_t)index;
	}
	isotp_send(&sender);
	
	vTaskDelete(rc_task);
}

static ERROR_CODE sender_test_send(struct phy_msg_t *msg)
{
	static uint32_t seq = 0UL;

	if(msg->new_data == TRUE)
	{
		msg->new_data = FALSE;
		seq ++;
		debug_out("Sder-Tx Seq:%04d Len:%02d Id:0x%04X Data:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					seq,
					msg->length,
					msg->id,
					msg->data[0], msg->data[1], msg->data[2],
					msg->data[3], msg->data[4], msg->data[5],
					msg->data[6], msg->data[7]);
		memcpy(&receiver.isotp.phy_rx, msg, sizeof(*msg));
		receiver.isotp.phy_rx.new_data = TRUE;
	}
	return STATUS_NORMAL;
}

static ERROR_CODE sender_test_receive(struct phy_msg_t *msg)
{
	static uint32_t seq = 0UL;
	ERROR_CODE err = ERR_EMPTY;

	if(msg->new_data == TRUE)
	{
		msg->new_data = FALSE;
		err = STATUS_NORMAL;
		seq ++;
		debug_out("Sder-Rx Seq:%04d Len:%02d Id:0x%04X Data:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					seq,
					msg->length,
					msg->id,
					msg->data[0], msg->data[1], msg->data[2],
					msg->data[3], msg->data[4], msg->data[5],
					msg->data[6], msg->data[7]);
	}
	
	return err;
}

static ERROR_CODE receiver_test_send(struct phy_msg_t *msg)
{
	static uint32_t seq = 0UL;

	if(msg->new_data == TRUE)
	{
		msg->new_data = FALSE;
		seq ++;
		
		debug_out("Rcer-Tx Seq:%04d Len:%02d Id:0x%04X Data:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					seq,
					msg->length,
					msg->id,
					msg->data[0], msg->data[1], msg->data[2],
					msg->data[3], msg->data[4], msg->data[5],
					msg->data[6], msg->data[7]);
		memcpy(&sender.isotp.phy_rx, msg, sizeof(*msg));
		sender.isotp.phy_rx.new_data = TRUE;
	}
	return STATUS_NORMAL;
}

static ERROR_CODE receiver_test_receive(struct phy_msg_t *msg)
{
	static uint32_t seq = 0UL;
	ERROR_CODE err = ERR_EMPTY;

	if(msg->new_data == TRUE)
	{
		msg->new_data = FALSE;
		err = STATUS_NORMAL;
		seq ++;
		debug_out("Rcer-Rx Seq:%04d Len:%02d Id:0x%04X Data:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
					seq,
					msg->length,
					msg->id,
					msg->data[0], msg->data[1], msg->data[2],
					msg->data[3], msg->data[4], msg->data[5],
					msg->data[6], msg->data[7]);
	}
	return err;
}

static ERROR_CODE receiver_set_FS(struct isotp_t* msg)
{
	/*
	 * set special parameters of flow control status
	 * receiver: operate object
	 * ISOTP_FS_CTS: Continue To Send
	 * TEST_BS: (BS)
	 * TEST_STMIN: STmin
	 */
	/* fc_set(&receiver, ISOTP_FS_CTS, TEST_BS, TEST_STMIN); */
	debug_out("Rcer-FC-reply FS:%d BS:%d STmin:%d\r\n", msg->FS, msg->BS, msg->STmin);
	return STATUS_NORMAL;
}

