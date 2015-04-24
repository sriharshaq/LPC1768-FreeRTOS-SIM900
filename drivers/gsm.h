#ifndef __gsm_h__
#define __gsm_h__

#include "jsmn.h"

/* modem response line types */
#define __LINE_BLANK 	0
#define __LINE_DATA		1
#define __LINE_ERROR 	2
#define __LINE_OTHER	3

typedef struct 
{
	char * ipstate;
	char * apn;
	char * opr;
	char * ip;
	char * httpdata;
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
extern uint8_t	gsm_get_ipaddr(void);

extern uint8_t 	gsm_tcp_start(char *, char *);
extern uint8_t 	gsm_tcp_close(void);
extern uint8_t  gsm_bring_wireless_up(void);
extern uint8_t	gsm_tcp_send(void);

extern uint8_t	gsm_http_head(char *, char *);
extern uint8_t	gsm_http_get(char *, char *);
extern uint8_t	gsm_http_delete(char *, char *);
extern uint8_t	gsm_http_post(char *, char *, char *);
extern uint8_t	gsm_http_put(char *, char *, char *);

extern uint8_t	gsm_send_sms(char *, char *);

extern int jsoneq(const char *json, jsmntok_t *tok, const char *s);

/* AT+CSTT

+CSTT: "TATA.DOCOMO.INTERNET","",""

OK
*/

/* AT+COPS?

+COPS: 0,0,"T24"

OK
*/

/* AT+CIFSR

100.106.186.61
*/

/* TCP Start
OK

CONNECT OK
*/

/* after reset 
RDY

+CFUN: 1

+CPIN: READY

Call Ready
*/

/* AT+CIPSHUT
SHUT OK
*/


#endif