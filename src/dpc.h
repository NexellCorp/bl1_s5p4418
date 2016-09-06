#ifndef __DPC_H__
#define __DPC_H__

void dpc_set_enable_all(unsigned int module, unsigned int enb);
 int dpc_get_pending_all(unsigned int module);
void dpc_clear_pending_all(unsigned int module);
int  dpc_enabled(unsigned int module);

#endif
