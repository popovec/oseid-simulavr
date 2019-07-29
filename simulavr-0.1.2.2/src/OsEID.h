#ifndef _OsEID
#define _OsEID

extern VDevice *ee_create (int addr, char *name, int rel_addr, void *data);
extern VDevice *oseid_create (int addr, char *name, int rel_addr, void *data);

uint8_t gdb_ee_read(int adr);
void    gdb_ee_write(int adr,uint8_t data);
#endif
