#ifndef _SEEPROM_H
#define _SEEPROM_H

#include "gl_types.h"

#define INCLUDE_LUA_CONVERSION

enum {
	UTYPE_STRING = 0,
	UTYPE_FLOAT  = 1,
	UTYPE_INT16  = 2,
	UTYPE_UINT32 = 3,
};

#ifdef __cplusplus
extern "C" {
#endif

extern UINT16 ipf_lookup_unpack(char *ipf, UINT16 tree, char *key, UINT8 utype, void *buff, UINT16 buff_size);
extern UINT32 make_ipf_by_files(char *fname0, char *fname1, char *ipf, UINT32 max_size);
extern UINT32 make_ipf_by_buff(char *in, int inlen, char *ipf, UINT32 max_size);

#ifdef __cplusplus
}
#endif

#endif /* _SEEPROM_H */
