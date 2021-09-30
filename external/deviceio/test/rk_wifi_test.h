#ifndef __WIFI_TEST_H__
#define __WIFI_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

void rk_wifi_airkiss_start(void *data);
void rk_wifi_airkiss_stop(void *data);
void rk_wifi_softap_start(void *data);
void rk_wifi_softap_stop(void *data);
void rk_wifi_open(void *data);
void rk_wifi_close(void *data);
void rk_wifi_connect(void *data);
void rk_wifi_ping(void *data);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_TEST_H__ */
