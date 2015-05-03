#ifndef __gsm_h__
#define __gsm_h__

/* define uart functions */
#define modem_out(ptr) 				uart3_print(ptr)
#define modem_readline()			uart3_readline()
#define modem_getc()				uart3_getc()
#define modem_putc(c)				uart3_putc(c)
#define modem_flush_rx()			uart3_flushrx()

/* modem response line types */
#define __LINE_BLANK 				0
#define __LINE_DATA					1
#define __LINE_ERROR 				2
#define __LINE_OTHER				3
#define __LINE_START_RDY			4
#define __LINE_START_CRDY			5
#define __LINE_START_OTHER			6

#define __LINE_PARSE_SUCCESS		111

/* MODEM ERRORS */
#define __MODEM_LINE_STR_ERROR 		-1		
#define __MODEM_LINE_NOT_BLANK		-2
#define __MODEM_LINE_NOT_DATA		-3
#define __MODEM_LINE_NOT_OTHER		-4
#define __MODEM_LINE_NOT_OK			-5
#define __MODEM_LINE_TIMEOUT		-6
#define __MODEM_LINE_CS_ERROR		-7
#define __MODEM_LINE_PARSE_ERROR	-8
#define __PARAM_PASS_VALUE_ERROR	-9
#define __MODEM_UNKNOWN_ERROR		-10

typedef struct 
{
	uint8_t 	rssi;
	uint8_t		lastsmsloc;
	uint8_t		nw_reg;
	uint16_t	http_status;

	char 	* 	tcpstatus;
	char 	* 	setapn;
	char 	* 	getapn;
	char 	* 	ip_addr;

	char	*	operator_name;

	char 	* 	httpdata;
	char	*	httpheader;

}Modem_Type_t;

/* modem data struct */
extern Modem_Type_t modem;

extern void gsm_buff_init(void);

/* process */
//extern int8_t 	gsm_process_response(char *, uint16_t);

/* query */
extern int8_t 	gsm_ping_modem(void);
extern int8_t 	gsm_get_tcpstatus(void);
extern int8_t 	gsm_get_rssi(void);
extern int8_t 	gsm_get_accesspoint(void);
extern int8_t 	gsm_set_accesspoint(void);
extern int8_t 	gsm_get_operator_name(void);
extern int8_t 	gsm_start_gprs(void);

/* TCP IP */
extern int8_t 	gsm_get_ip_address(void);
extern int8_t 	gsm_tcp_connect(char *, char *);
extern int8_t 	gsm_tcp_disconnect(void);
extern int8_t 	gsm_send(char);
extern int8_t 	gsm_tcp_shutdown(void);

/* HTTP */
extern int8_t	gsm_http_head(char *, char *);
extern int8_t	gsm_http_get(char *, char *);
extern int8_t	gsm_http_delete(char *, char *);
extern int8_t	gsm_http_post(char *, char *, char *);
extern int8_t	gsm_http_put(char *, char *, char *);
extern void 	http_read_data(void);

/* SMS */
extern int8_t	gsm_set_text_mode(uint8_t);
extern int8_t	gsm_send_sms(char *, char *);

int8_t gsm_get_network_reg_state(void);

#endif