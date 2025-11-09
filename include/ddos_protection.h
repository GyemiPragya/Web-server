#ifndef DDOS_PROTECTION_H
#define DDOS_PROTECTION_H

int ddos_init(int capacity, int refill_per_sec, int ban_duration_sec, int max_conn_per_ip);
int ddos_check_and_consume(const char *ipstr); /* returns 0 ok, -1 over limit, -2 banned, -3 conn cap */
void ddos_release_connection(const char *ipstr);
void ddos_shutdown(void);

#endif
