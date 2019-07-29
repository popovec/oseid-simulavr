#ifndef _SPM_HELPER
#define _SPM_HELPER
typedef struct _SPMhelper SPMhelper;

struct _SPMhelper
{
    AvrClass parent;
    uint8_t page_buffer[256];
    uint8_t SPMCSR;
    Flash *flash;
        
    VDevice *SPM;                /* Virtual Device for SPM subsystem */
};

typedef struct _SPMdata SPMdata;

struct _SPMdata
{  
  VDevice parent;
   
  uint16_t addr;
}; 



extern VDevice *spm_create (int addr, char *name, int rel_addr, void *data);
extern void spm_run (SPMhelper *spmhelper, int reg0, int reg1, int Z);
extern SPMhelper * spmhelper_new(Flash * flash);
#endif
