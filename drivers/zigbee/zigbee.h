
#ifndef __zigbee_h__
#define __zigbee_h__

#define zigbee_out(ptr) 	uart0_print(ptr)
#define zigbee_readline() 	uart0_readline()

extern int8_t send_to_end_device(char *, char *,char *);
extern int8_t ping_zigbee(void);

#endif