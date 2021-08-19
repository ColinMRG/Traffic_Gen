/*
 * Copyright (C) 2009 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include "addressparams.h"
#include "xscugic.h"

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

u32 b[NUM_OF_WORDS];

int transfer_data() {
	return 0;
}

void print_app_header()
{
#if (LWIP_IPV6==0)
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
#else
	xil_printf("\n\r\n\r-----lwIPv6 TCP echo server ------\n\r");
#endif
	xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}

u32 *pdata = NULL;
int data_length = 0;
const char *ok_message = "OK\n\r";
const char *err_message = "ERR\n\r";
const char *tbd_message = "...\n\r";
const char *unknown_message = "???\n\r";
const char *cr_message = "\n\r";

#define FPGA_RO_SOFT_SCAN_TRIGGER                     0x19
#define FPGA_RB_SOFT_SCAN_TRIGGER_RF_OVERRIDE_ENABLE  5
#define FPGA_RB_SOFT_SCAN_TRIGGER_ARM_TO_SCAN         3
#define FPGA_RB_SOFT_SCAN_TRIGGER_SOFT_SCAN_TRIGGER   1
#define FPGA_RB_SOFT_SCAN_TRIGGER_TEST_MODE           0

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	const char *reply = NULL;
	int n;
	char msgbuf[10];

	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	int command;
	command = (int)*((u8*)(p->payload));

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

	/* free the received pbuf */
	pbuf_free(p);

	switch (command) {
	case 'r':
		// Set up the DMA
		pdata = dma();
		data_length = dma_recvd_length();
		xil_printf("DMA length: %d\n\r", data_length);
		reply = (pdata == NULL) ? err_message : ok_message;
		err = tcp_write(tpcb, reply, strlen(reply), 1);
		xil_printf("Restarted DMA at %08X\n\r", (u32)pdata);
		break;
	case 's':
		// Run SOS and return the data length
		xil_printf("command = %d\n\r", command);
		wait_for_dma_done();
		data_length = dma_recvd_length();
		err = tcp_write(tpcb, &data_length, sizeof(int), 1);
		xil_printf("DMA length: %d\n\r", data_length);
		break;
	case 'd':
		n = (data_length <= TCP_MSS) ? data_length : TCP_MSS;
		err = tcp_write(tpcb, pdata,n, 1);
		xil_printf("Sent %d bytes\n\r", n);
		data_length -= n;
		pdata += n/sizeof(u32);
		break;
	case '\n':
	case '\r':
		err = tcp_write(tpcb, cr_message, strlen(cr_message), 1);
		break;
	default:
		err = tcp_write(tpcb, unknown_message, strlen(unknown_message), 1);
		xil_printf("Unknown command: %c\n\r", command);
	}

	tcp_output(tpcb);
	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	/* increment for subsequent accepted connections */
	connection++;

	return ERR_OK;
}

int start_application()
{
	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 7;
	/* create new TCP PCB structure */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}
	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ANY_TYPE, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}
	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}
	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);
	xil_printf("TCP echo server started @ port %d\n\r", port);
	return 0;
}
