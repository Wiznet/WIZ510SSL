#ifndef __NETBIOS_H__
#define __NETBIOS_H__

extern char netname[16];

void do_netbios(void);
void set_netbios_devname(uint8_t * devname);

#endif /* __NETBIOS_H__ */
