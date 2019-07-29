#ifndef _OsEID
#define _OsEID

extern VDevice *ee_create (int addr, char *name, int rel_addr, void *data);
extern VDevice *oseid_create (int addr, char *name, int rel_addr, void *data);

#endif
