#ifndef __gsm_h__
#define __gsm_h__

/* modem response line types */
#define __LINE_BLANK 	0
#define __LINE_DATA		1
#define __LINE_ERROR 	2
#define __LINE_OTHER	3

typedef struct 
{
	char * ipstate;
	char * apn;
	uint8_t rssi;
}Modem_Type_t;

extern Modem_Type_t modem;

extern uint8_t 	process_response(char *, uint16_t);
extern uint8_t 	gsm_ping_modem(void);
extern uint8_t 	gsm_update_ipstatus(void);
extern void		gsm_allocate_mem(void);
extern uint8_t 	gsm_update_rssi(void);
extern uint8_t	gsm_set_apn(char *);
extern uint8_t	gsm_get_apn(void);

extern uint8_t	gsm_get_opr_name(void);

extern void 	gsm_tcp_start(void);
extern void     gsm_bring_wireless_up(void);


/* get apn

+CSTT: "TATA.DOCOMO.INTERNET","",""

OK
*/


#endif