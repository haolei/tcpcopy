#ifndef  _TCPCOPY_RECEIVER_STATUS_H_INC
#define  _TCPCOPY_RECEIVER_STATUS_H_INC

#ifdef __cplusplus
extern "C"
{
#endif

	void status_init();
	void status_update(struct iphdr *ip_header);
	void status_add(uint32_t ,uint16_t,int);
	void status_del(uint32_t ,uint16_t);

#ifdef __cplusplus
}
#endif
#endif   /* ----- #ifndef _TCPCOPY_RECEIVER_STATUS_H_INC  ----- */

