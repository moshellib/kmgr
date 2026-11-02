#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gl_types.h"
#include "seeprom.h"

#include "kdefs.h"

//////////////////////////////////////////////////////////////////////////////////////////
// lookups and unpacking
//////////////////////////////////////////////////////////////////////////////////////////

static const char packed_chars[] = "0123456789+-. _"; // 15 characters as the 16'th is reserved for NULL

enum {
	NTYPE_UNKNW_E = 0, // node type is unknown yet...
	NTYPE_SPLTR_E = 1, // splitter node
	NTYPE_RSTRV_E = 2, // regular string value
	NTYPE_PSTRV_E = 3, // packed string value
};

typedef struct {
	UINT16 p_offset__le16;     // highest bit says if packet, lowest 15 bits are offset of name from strs or packed_strs
} str_info_t;

#define P_MASK 0x8000
#define O_MASK 0x7FFF

typedef struct {
	UINT16 ntype__le16;
	union {
		struct {
			UINT16 n_ptrs__le16;
		} splt;
		str_info_t xstr;
	} u;
} node_header_t;

typedef struct {
	str_info_t name;
	UINT16 offset__le16;               // offset of child from beginning of nodes segment
} node_ptr_t;

typedef struct {
	UINT16 header_size__le16;
	UINT16 nodes_segment_size__le16;
	UINT16 rstrs_segment_size__le16;
	UINT16 pstrs_segment_size__le16;
	UINT16 root_offset__le16[1]; // 1 or more, up to MAX_ROOTS
} ipf_header_t;

char str_info_char(char *rstrs, char *pstrs, str_info_t *str_info, UINT16 offset) {
	char c;
	UINT8 nibble;
	offset += (FE16(str_info->p_offset__le16) & O_MASK);
	if(FE16(str_info->p_offset__le16) & P_MASK) {
		nibble = (offset & 1) == 0 ? (pstrs[offset/2] >> 4) & 0xF : pstrs[offset/2] & 0xF;
		c = (nibble == 0xF) ? 0 : packed_chars[nibble];
	} else {
		c = rstrs[offset];
	}
	return c;
}

UINT16 str_info_prefix(char *rstrs, char *pstrs, str_info_t *str_info, char *key) {
	// if str_info is a prefix of key, we return the length of str_info, else we return 0
	int pos = 0;
	char c;
	do {
		c = str_info_char(rstrs, pstrs, str_info, pos);
		//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "pos=%d c=%d key[pos]=%d", pos, c, key[pos]);
		if(c == 0) return pos;
	} while(key[pos++] == c);
	return 0;
}

char *str_info_unpack(char *rstrs, char *pstrs, str_info_t *str_info, char *buff, int buff_size) {
	int pos = 0;
	char c;
	do {
		if(pos >= buff_size) {
			if(buff_size) buff[buff_size - 1] = 0;
			break;
		}
		c = str_info_char(rstrs, pstrs, str_info, pos);
		buff[pos++] = c;
	} while(c);
	return buff;
}

int str_info_unpack_string(char *rstrs, char *pstrs, str_info_t *str_info, char *buff, int buff_size) {
	return str_info_unpack(rstrs, pstrs, str_info, buff, buff_size) ? 1 : 0;
}

int str_info_unpack_float(char *rstrs, char *pstrs, str_info_t *str_info, float *buff, int buff_size) {
	int pos, idx = 0, item_len = 0;
	char c, item[32];
	for(pos = 0; ; pos++) {
		if(((char *)&buff[idx]) >= ((char *)buff) + buff_size) break; // out of space
		c = str_info_char(rstrs, pstrs, str_info, pos);
		item[item_len] = c != ' ' ? c : 0;
		if(item_len && (item[item_len] == 0)) {
			buff[idx++] = atof(item);
			item_len = 0;
		} else {
			item_len++;
			if(item_len >= (int) sizeof(item)) break; // item is too big for unpacking...
		}
		if(c == 0) break; // reached end of conversion
	};
	return idx;
}

int str_info_unpack_int16(char *rstrs, char *pstrs, str_info_t *str_info, INT16 *buff, int buff_size) {
	int pos, idx = 0, item_len = 0;
	char c, item[32];
	for(pos = 0; ; pos++) {
		if(((char *)&buff[idx]) >= ((char *)buff) + buff_size) break; // out of space
		c = str_info_char(rstrs, pstrs, str_info, pos);
		item[item_len] = c != ' ' ? c : 0;
		if(item_len && (item[item_len] == 0)) {
			buff[idx++] = atoi(item);
			item_len = 0;
		} else {
			item_len++;
			if(item_len >= (int) sizeof(item)) break; // item is too big for unpacking...
		}
		if(c == 0) break; // reached end of conversion
	};
	return idx;
}

int str_info_unpack_uint32(char *rstrs, char *pstrs, str_info_t *str_info, UINT32 *buff, int buff_size) {
	int pos, idx = 0, item_len = 0;
	char c, item[32];
	for(pos = 0; ; pos++) {
		if(((char *)&buff[idx]) >= ((char *)buff) + buff_size) break; // out of space
		c = str_info_char(rstrs, pstrs, str_info, pos);
		item[item_len] = c != ' ' ? c : 0;
		if(item_len && (item[item_len] == 0)) {
			buff[idx++] = atoi(item);
			item_len = 0;
		} else {
			item_len++;
			if(item_len >= (int) sizeof(item)) break; // item is too big for unpacking...
		}
		if(c == 0) break; // reached end of conversion
	};
	return idx;
}

UINT16 ipf_unpack_header(char *ipf, char **nodes_segment, char **rstrs_segment, char **pstrs_segment, UINT16 **roots__le16) {
	UINT16 n_roots, ofs;
	ipf_header_t *hdr = (ipf_header_t*) ipf;
	ofs = FE16(hdr->header_size__le16);
	if(nodes_segment) *nodes_segment = ipf + ofs;
	ofs += FE16(hdr->nodes_segment_size__le16);
	if(rstrs_segment) *rstrs_segment = ipf + ofs;
	ofs += FE16(hdr->rstrs_segment_size__le16);
	if(pstrs_segment) *pstrs_segment = ipf + ofs;
	n_roots = ((ipf + FE16(hdr->header_size__le16)) - ((char*)&hdr->root_offset__le16[0])) / sizeof(hdr->root_offset__le16[0]);
	if(roots__le16) *roots__le16 = &hdr->root_offset__le16[0];
	return n_roots;
}

node_header_t *ipf_lookup(char *ipf, UINT16 tree, char *key) {
	UINT16 roots, *root_offset__le16, node_offset, child_offset, key_pos, key_step, i;
	node_header_t *node;
	char *nodes_segment, *rstrs_segment, *pstrs_segment;
	roots = ipf_unpack_header(ipf, &nodes_segment, &rstrs_segment, &pstrs_segment, &root_offset__le16);
	//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%d roots");
	if(tree >= roots) return NULL;
	for(key_pos = 0, node_offset = FE16(root_offset__le16[tree]); node_offset != (UINT16) -1; node_offset = child_offset) {
		child_offset = (UINT16) -1;
		node = (node_header_t *) &nodes_segment[node_offset];
		//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "node offset %d, type %d", node_offset, node->ntype);
		node_ptr_t *p = (node_ptr_t *) &node[1];
		if(key[key_pos] == 0) {
			//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "match");
			return node;
		}
		switch(FE16(node->ntype__le16)) {
		case NTYPE_RSTRV_E:
		case NTYPE_PSTRV_E:
			return NULL; // we can not go on any further
		case NTYPE_SPLTR_E:
			for(i=0; i<FE16(node->u.splt.n_ptrs__le16); i++) {
				key_step = str_info_prefix(rstrs_segment, pstrs_segment, &p[i].name, &key[key_pos]);
				if((key_step != 0) && ((key[key_pos + key_step] == '.') || (key[key_pos + key_step] == 0))) {
					// string held by child is a prefix of key and form a complete step, we can go on to next node
					child_offset = FE16(p[i].offset__le16);
					key_pos += key_step;
					//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "advancing %d chars", key_step);
					if(key[key_pos] == '.') {
						//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "advancing . char");
						++key_pos;
					}
					break;
				}
			}
			break;
		default:
			return NULL;
		}
	}
	return NULL;
}

UINT16 ipf_lookup_unpack(char *ipf, UINT16 tree, char *key, UINT8 utype, void *buff, UINT16 buff_size) {
	char *rstrs_segment, *pstrs_segment;
	ipf_unpack_header(ipf, NULL, &rstrs_segment, &pstrs_segment, NULL);
	node_header_t *node = ipf_lookup(ipf, tree, key);
	if(node == NULL) return 0;
	switch(FE16(node->ntype__le16)) {
	case NTYPE_RSTRV_E:
	case NTYPE_PSTRV_E:
		switch(utype) {
		case UTYPE_STRING:
			return str_info_unpack_string(rstrs_segment, pstrs_segment, &node->u.xstr, buff, buff_size);
		case UTYPE_FLOAT:
			return str_info_unpack_float(rstrs_segment, pstrs_segment, &node->u.xstr, buff, buff_size);
		case UTYPE_INT16:
			return str_info_unpack_int16(rstrs_segment, pstrs_segment, &node->u.xstr, buff, buff_size);
		case UTYPE_UINT32:
			return str_info_unpack_uint32(rstrs_segment, pstrs_segment, &node->u.xstr, buff, buff_size);
		default:
			return 0; // 0 items unpacked
		}
	default:
		return 0; // 0 items unpacked
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// parsing of lua file into packed data structure
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef INCLUDE_LUA_CONVERSION

#define TRACE_LUA 0 // for debugging

#define MAX_TOKEN      1024
#define MAX_LEVEL      16
#define MAX_NODE_SPLIT 128  // maximal number of brother nodes
#define MAX_RSTRS      8196 // maximal size of regular string segment
#define MAX_PSTRS      8196 // maximal size of packed string segment
#define MAX_NODES      8196 // maximal size of packed string segment
#define MAX_ROOTS      4

typedef struct {
	int token_count;
	char accumulated_token[MAX_TOKEN];
	int accumulated_token_pos;
	char rstrs[MAX_RSTRS];
	int rstrs_pos;
	char pstrs[MAX_PSTRS];
	int pstrs_pos;
	char nodes[MAX_NODES];
	int nodes_pos;
	UINT16 roots_offset[MAX_ROOTS];
	int roots_pos;
	struct {
		char token[MAX_TOKEN];
		node_header_t node_header;       // node header for current node
		int n_children;                  // number of children
		node_ptr_t ptrs[MAX_NODE_SPLIT]; // pointers to children
	} token_stack[MAX_LEVEL];
	int token_stack_pos;
} lua2ipf_parser_t;

enum {
	TTYPE_OTR_E = 0,
	TTYPE_STR_E = 1,
	TTYPE_SGL_E = 2,
};

void parser_init(lua2ipf_parser_t *psr) {
	psr->token_count = 0;
	psr->accumulated_token_pos = 0;
	psr->token_stack_pos = 0;
	psr->rstrs_pos = 0;
	psr->pstrs_pos = 0;
	psr->nodes_pos = 0;
	psr->roots_pos = 0;
}

char *get_string(lua2ipf_parser_t *psr, str_info_t *str_info, char *buff, int buff_size) {
	return str_info_unpack(&psr->rstrs[0], &psr->pstrs[0], str_info, buff, buff_size);
}

int string_exists(lua2ipf_parser_t *psr, str_info_t *str_info, char *str) {
	char buff[MAX_TOKEN];
	UINT32 limit = FE16(str_info->p_offset__le16) & P_MASK ? psr->pstrs_pos : psr->rstrs_pos;
	UINT32 ofs = 0;
	while(ofs < limit) {
		str_info->p_offset__le16 = FE16((FE16(str_info->p_offset__le16) & P_MASK) | (ofs & O_MASK));
		get_string(psr, str_info, buff, sizeof(buff));
		if(strcmp(buff, str) == 0) {
			//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> using existing %s", str);
			return 1;
		}
		ofs += (strlen(buff) + 1);
	}
	return 0;
}

void make_string(lua2ipf_parser_t *psr, str_info_t *str_info, char *str) {
	// first we need to check if we can pack this string
	UINT32 length, packable = 1;
	for(length = 0; str[length]; length++) {
		if(strchr(packed_chars, str[length]) == NULL) {
			packable = 0;
		}
	}
	//packable = 0; // un-comment to avoid packing
	if(packable) {
		str_info->p_offset__le16 |= FE16(P_MASK);
	} else {
		str_info->p_offset__le16 &= FE16(~P_MASK);
	}
	if(string_exists(psr, str_info, str)) {
		return;
	} else if(packable) {
		if(psr->pstrs_pos + length + 1 >= 2 * sizeof(psr->rstrs)) {
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "out of packed string segment");
		} else {
			UINT32 i;
			UINT8 idx;
			str_info->p_offset__le16 = FE16((FE16(str_info->p_offset__le16) & P_MASK) | (psr->pstrs_pos & O_MASK));
			for(i = 0; i <= length; i++) {
				idx = str[i] ? strchr(packed_chars, str[i]) - packed_chars : 0xF;
				if((psr->pstrs_pos & 1) == 0) {
					psr->pstrs[psr->pstrs_pos >> 1] = (psr->pstrs[psr->pstrs_pos >> 1] & 0x0F) | ((idx << 4) & 0xF0);
				} else {
					psr->pstrs[psr->pstrs_pos >> 1] = (psr->pstrs[psr->pstrs_pos >> 1] & 0xF0) | (idx & 0x0F);
				}
				//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> %c @%d: 0x%X", str[i], psr->pstrs_pos, psr->pstrs[psr->pstrs_pos >> 1]);
				++psr->pstrs_pos;
			}
		}
	} else {
		if(psr->rstrs_pos + length + 1 >= sizeof(psr->rstrs)) {
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "out of regular string segment");
		} else {
			//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> rstr %s at %d", str, psr->rstrs_pos);
			str_info->p_offset__le16 = FE16((FE16(str_info->p_offset__le16) & P_MASK) | (psr->rstrs_pos & O_MASK));
			strcpy(&psr->rstrs[psr->rstrs_pos], str);
			psr->rstrs_pos += (length + 1);
		}
	}

}

void accumulate_token(lua2ipf_parser_t *psr, char *token) {
	if(token == NULL) {
		psr->accumulated_token_pos = 0;
		psr->accumulated_token[0] = 0;
	} else {
		if(psr->accumulated_token_pos == 0) {
			psr->accumulated_token_pos += snprintf(&psr->accumulated_token[psr->accumulated_token_pos], sizeof(psr->accumulated_token) - psr->accumulated_token_pos, "%s", token);
		} else {
			psr->accumulated_token_pos += snprintf(&psr->accumulated_token[psr->accumulated_token_pos], sizeof(psr->accumulated_token) - psr->accumulated_token_pos, " %s", token);
		}
	}
}

char *spaces(int n) {
	static char *s = "                                                                                       ";
	return &s[strlen(s) - 4 * n];
}

void token_stack_push(lua2ipf_parser_t *psr, char *token) {
	if(psr->token_stack_pos < MAX_LEVEL) {
#if TRACE_LUA
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%s%s {", spaces(psr->token_stack_pos), token);
#endif
		strncpy(&psr->token_stack[psr->token_stack_pos].token[0], token, sizeof(psr->token_stack[psr->token_stack_pos].token));
		psr->token_stack[psr->token_stack_pos].n_children = 0;
		psr->token_stack[psr->token_stack_pos].node_header.ntype__le16 = FE16(NTYPE_UNKNW_E);
		if(psr->token_stack_pos > 0) {
			// update parent with my details
			if(FE16(psr->token_stack[psr->token_stack_pos - 1].node_header.ntype__le16) == NTYPE_UNKNW_E) {
				psr->token_stack[psr->token_stack_pos - 1].node_header.ntype__le16 = FE16(NTYPE_SPLTR_E);
			}
			if(FE16(psr->token_stack[psr->token_stack_pos - 1].node_header.ntype__le16) == NTYPE_SPLTR_E) {
				make_string(psr, &psr->token_stack[psr->token_stack_pos - 1].ptrs[psr->token_stack[psr->token_stack_pos - 1].n_children].name, token);
			} else {
				klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error, parent is not a splitter");
			}
		}
		psr->token_stack_pos++;
		psr->token_stack[psr->token_stack_pos].node_header.ntype__le16 = FE16(NTYPE_UNKNW_E);
	} else {
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error, max level too small");
	}
}

void token_stack_pop(lua2ipf_parser_t *psr) {
	int i;
	char *p;
	if(psr->token_stack_pos > 0) {
		psr->token_stack_pos--;
		if(FE16(psr->token_stack[psr->token_stack_pos].node_header.ntype__le16) == NTYPE_SPLTR_E) {
			psr->token_stack[psr->token_stack_pos].node_header.u.splt.n_ptrs__le16 = FE16(psr->token_stack[psr->token_stack_pos].n_children);
		}
		UINT32 node_size = sizeof(node_header_t) + psr->token_stack[psr->token_stack_pos].n_children * sizeof(node_ptr_t);
#if TRACE_LUA
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%s} // %s: level %d, nodes_pos = %d-%d, type = %d, children = %d", spaces(psr->token_stack_pos),
				psr->token_stack[psr->token_stack_pos].token, psr->token_stack_pos, psr->nodes_pos, psr->nodes_pos + node_size - 1,
				FE16(psr->token_stack[psr->token_stack_pos].node_header.ntype__le16), psr->token_stack[psr->token_stack_pos].n_children);
#endif
		psr->token_stack[psr->token_stack_pos - 1].ptrs[psr->token_stack[psr->token_stack_pos - 1].n_children].offset__le16 = FE16(psr->nodes_pos);
		++psr->token_stack[psr->token_stack_pos - 1].n_children;
#if 0 // dump node content
		{
			char buff[MAX_TOKEN];
			switch(psr->token_stack[psr->token_stack_pos].node_header.ntype) {
			case NTYPE_RSTRV_E:
			case NTYPE_PSTRV_E:
				klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> value is %s", get_string(psr, &psr->token_stack[psr->token_stack_pos].node_header.u.xstr, buff, sizeof(buff)));
				break;
			case NTYPE_SPLTR_E:
				for(i=0; i<psr->token_stack[psr->token_stack_pos].node_header.u.splt.n_ptrs; i++) {
					klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> child %d (%s) at offset %d", i, get_string(psr, &psr->token_stack[psr->token_stack_pos].ptrs[i].name, buff, sizeof(buff)), psr->token_stack[psr->token_stack_pos].ptrs[i].offset);
				}
				break;
			default:
				klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error, unknown node type during pop (%d)", psr->token_stack[psr->token_stack_pos].node_header.ntype);
			}
		}
#endif
		if(psr->nodes_pos + node_size > sizeof(psr->nodes)) {
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "error, out of nodes segment");
		} else {
			if(psr->token_stack_pos == 0) {
				if(psr->roots_pos >= MAX_ROOTS) {
					klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "error too many roots");
				} else {
					// klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> root node at offset %d", psr->nodes_pos);
					psr->roots_offset[psr->roots_pos++] = psr->nodes_pos;
				}
			}
			p = &psr->nodes[psr->nodes_pos];
			//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> p at %p", p);
			memcpy(p, &psr->token_stack[psr->token_stack_pos].node_header, sizeof(psr->token_stack[psr->token_stack_pos].node_header));
			p += sizeof(psr->token_stack[psr->token_stack_pos].node_header);
			//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> p at %p", p);
			if(FE16(psr->token_stack[psr->token_stack_pos].node_header.ntype__le16) == NTYPE_SPLTR_E) {
				for(i=0; i<FE16(psr->token_stack[psr->token_stack_pos].node_header.u.splt.n_ptrs__le16); i++) {
					memcpy(p, &psr->token_stack[psr->token_stack_pos].ptrs[i], sizeof(psr->token_stack[psr->token_stack_pos].ptrs[i]));
					p += sizeof(psr->token_stack[psr->token_stack_pos].ptrs[i]);
					//klog(KLOG_SUBJECT_KMGR, LL_DEBUG, ">>> p at %p (added %d)", p, (int)sizeof(psr->token_stack[psr->token_stack_pos].ptrs[i]));
				}
			}
			psr->nodes_pos += node_size;
			if(p != &psr->nodes[psr->nodes_pos]) {
				klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error while dumping node");
			}
		}
	}
}

void token_value(lua2ipf_parser_t *psr, char *value) {
	if(psr->token_stack_pos > 0) {
#if TRACE_LUA
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%s%s", spaces(psr->token_stack_pos), value);
#endif
		if(FE16(psr->token_stack[psr->token_stack_pos - 1].node_header.ntype__le16) == NTYPE_UNKNW_E) {
			make_string(psr, &psr->token_stack[psr->token_stack_pos - 1].node_header.u.xstr, value);
			psr->token_stack[psr->token_stack_pos - 1].node_header.ntype__le16 =
					FE16(FE16(psr->token_stack[psr->token_stack_pos - 1].node_header.u.xstr.p_offset__le16) & P_MASK ? NTYPE_PSTRV_E : NTYPE_RSTRV_E);
		} else {
			klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error: can not set node as value");
		}
	}
}

void token_indication(lua2ipf_parser_t *psr, char *token, int type) {
	psr->token_count++;
	if((psr->token_count == 1) && (strcmp(token, "local") == 0)) return; // ignore first local token
	switch(type) {
	case TTYPE_OTR_E:
		accumulate_token(psr, token);
		break;
	case TTYPE_STR_E:
		accumulate_token(psr, token);
		break;
	case TTYPE_SGL_E:
		if(strcmp(token, "=") == 0) {
			token_stack_push(psr, psr->accumulated_token);
			accumulate_token(psr, NULL);
		} else if(strcmp(token, ";") == 0) {
			if(psr->accumulated_token_pos) {
				token_value(psr, psr->accumulated_token);
			}
			token_stack_pop(psr);
			accumulate_token(psr, NULL);
		} else {
			// ignore the token
		}
		break;
	default:
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "internal error at token: %s", token);
		break;
	}
}

int lua2ipf(lua2ipf_parser_t *psr, char *inbuf, int inlen) {
	int rv = 0;
	int nc = '\n', c;
	int cmnt = 0;
	int token_str = 0;
	unsigned int token_len = 0;
	char token[1024];
	int pos = 0;
	do {
		c = nc;
		nc = pos >= inlen ? EOF : inbuf[pos++];
		if((c == '-') && (nc == '-')) {
			cmnt = 1;
		} else if ((cmnt) && ((c == '\r') || (c == '\n'))) {
			cmnt = 0;
		}
		if(!cmnt) {
			if(token_str) {
				switch(c) {
				case '\r':
				case '\n':
				case EOF:
					return -3; // unexpected termination of line of file (in middle of string)
				case '\"':
					token_str = 0;
					token[token_len] = 0;
					token_indication(psr, token, TTYPE_STR_E); // we pass empty string as well
					token_len = 0;
					break;
				default:
					if(token_len < sizeof(token) - 2) {
						token[token_len++] = c;
					} else {
						return -2; // token too long...
					}
				}
			} else {
				switch(c) {
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				case EOF:
					// white space characters
					if(token_len) {
						token[token_len] = 0;
						token_indication(psr, token, TTYPE_OTR_E);
					}
					token_len = 0;
					break;
				case ';':
				case '}':
				case '{':
				case '[':
				case ']':
				case '=':
				case ',':
					// characters that form a token
					if(token_len) {
						token[token_len] = 0;
						token_indication(psr, token, TTYPE_OTR_E);
					}
					sprintf(token, "%c", c);
					token_indication(psr, token, TTYPE_SGL_E);
					token_len = 0;
					break;
				case '\"':
					token_str = 1;
					break;
				default:
					if(token_len < sizeof(token) - 2) {
						token[token_len++] = c;
					} else {
						return -2; // token too long...
					}
				}
			}
		}
	} while (c != EOF);
	return rv;
}

int luafile2ipf(lua2ipf_parser_t *psr, char *fname) {
	int rv, len;
	char filebuf[16384 + 1];
	FILE *fd;
	if((fd = fopen(fname, "rb")) == NULL) {
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "failed to open %s", fname);
		rv = -1;
	} else {
		len = fread(filebuf, 1, sizeof(filebuf), fd);
		fclose(fd);
		if(len == sizeof(filebuf)) {
			rv = -1;
		} else {
			rv = lua2ipf(psr, filebuf, len);
		}
	}
	return rv;
}

void ipf_tree_dump(char *prefix, int level, char *nodes_segment, char *rstrs_segment, char *pstrs_segment, int offset) {
	char *node = &nodes_segment[offset];
	node_header_t *h = (node_header_t*) node;
	node_ptr_t *p = (node_ptr_t *) &h[1];
	UINT32 i, co;
	char buff[MAX_TOKEN];
	switch(FE16(h->ntype__le16)) {
	case NTYPE_RSTRV_E:
	case NTYPE_PSTRV_E:
		printf("%s %s%s\n", prefix, spaces(level), str_info_unpack(rstrs_segment, pstrs_segment, &h->u.xstr, buff, sizeof(buff)));
		break;
	case NTYPE_SPLTR_E:
		for(i=0; i<FE16(h->u.splt.n_ptrs__le16); i++) {
			co = FE16(p[i].offset__le16);
			printf("%s %s%s\n", prefix, spaces(level), str_info_unpack(rstrs_segment, pstrs_segment, &p[i].name, buff, sizeof(buff)));
			ipf_tree_dump(prefix, level + 1, nodes_segment, rstrs_segment, pstrs_segment, co);
		}
		break;
	default:
		printf("internal error during dump at node offset %d\n", offset);
	}
}

void ipf_dump(char *ipf) {
	int i, roots;
	ipf_header_t *hdr = (ipf_header_t*) ipf;
	int ofs = FE16(hdr->header_size__le16);
	char *nodes_segment = ipf + ofs;
	ofs += FE16(hdr->nodes_segment_size__le16);
	char *rstrs_segment = ipf + ofs;
	ofs += FE16(hdr->rstrs_segment_size__le16);
	char *pstrs_segment = ipf + ofs;
	roots = (nodes_segment - ((char*)&hdr->root_offset__le16[0])) / sizeof(hdr->root_offset__le16[0]);
	printf("hdr   %d bytes\n", FE16(hdr->header_size__le16));
	printf("rstrs %d bytes\n", FE16(hdr->rstrs_segment_size__le16));
	printf("pstrs %d bytes\n", FE16(hdr->pstrs_segment_size__le16));
	printf("nodes %d bytes\n", FE16(hdr->nodes_segment_size__le16));
	printf("total %d bytes\n", FE16(hdr->header_size__le16) + FE16(hdr->rstrs_segment_size__le16) + FE16(hdr->pstrs_segment_size__le16) +
			FE16(hdr->nodes_segment_size__le16));
	printf("trees %d\n", roots);
	for(i = 0; i < roots; i++) {
		char prefix[16];
		sprintf(prefix, "%d", i);
		ipf_tree_dump(prefix, 0, nodes_segment, rstrs_segment, pstrs_segment, FE16(hdr->root_offset__le16[i]));
	}

}

void ipf_dump_element(char *ipf, UINT16 tree, char *key, char utype) {
	char buff[1024];
	int n, i;
	if((n =ipf_lookup_unpack(ipf, tree, key, utype, buff, sizeof(buff))) != 0) {
		switch(utype) {
		case UTYPE_STRING:
			printf("%s=%s\n", key, buff);
			break;
		case UTYPE_FLOAT:
			printf("%s[%d]=", key, n);
			for(i = 0; i < n; i++) {
				printf("%f%c", ((float*)&buff[0])[i], i < n - 1 ? ',' : '\n');
			}
			break;
		case UTYPE_INT16:
			printf("%s[%d]=", key, n);
			for(i = 0; i < n; i++) {
				printf("%d%c", ((INT16*)&buff[0])[i], i < n - 1 ? ',' : '\n');
			}
			break;
		default:
			printf("%d elements unpacked - no elements shown\n", n);
		}
	} else {
		printf("%s not found\n", key);
	}
}

static UINT32 make_ipf(lua2ipf_parser_t *parser, char *ipf, UINT32 max_size)
{
	char *pos = &ipf[0];
	ipf_header_t *hdr = (ipf_header_t*) pos;
	int i, total;
	hdr->header_size__le16 = FE16((UINT16)(UINT64)&((ipf_header_t*)NULL)->root_offset__le16[parser->roots_pos]);
	hdr->nodes_segment_size__le16 = FE16(parser->nodes_pos);
	hdr->rstrs_segment_size__le16 = FE16(parser->rstrs_pos);
	hdr->pstrs_segment_size__le16 = FE16((parser->pstrs_pos + 1) / 2);
	total = FE16(hdr->header_size__le16) + FE16(hdr->nodes_segment_size__le16) + FE16(hdr->rstrs_segment_size__le16) +
			FE16(hdr->pstrs_segment_size__le16);
	if((UINT32)total > max_size) {
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "error - total IPF is too big");
	} else {
		for(i = 0; i < parser->roots_pos; i++) {
			hdr->root_offset__le16[i] = FE16(parser->roots_offset[i]);
		}
		pos += FE16(hdr->header_size__le16);
		memcpy(pos, &parser->nodes[0], FE16(hdr->nodes_segment_size__le16));
		pos += FE16(hdr->nodes_segment_size__le16);
		memcpy(pos, &parser->rstrs[0], FE16(hdr->rstrs_segment_size__le16));
		pos += FE16(hdr->rstrs_segment_size__le16);
		memcpy(pos, &parser->pstrs[0], FE16(hdr->pstrs_segment_size__le16));
		pos += FE16(hdr->pstrs_segment_size__le16);
	}
	return total;
}

UINT32 make_ipf_by_files(char *fname0, char *fname1, char *ipf, UINT32 max_size)
{
	lua2ipf_parser_t parser;
	UINT32 bytes;
	parser_init(&parser);
	if(fname0) {
		luafile2ipf(&parser, fname0);
	}
	if(fname1) {
		luafile2ipf(&parser, fname1);
	}
	bytes = make_ipf(&parser, ipf, max_size);
	// now we have an ipf ready, we can dump and query it and so on
	//ipf_dump(ipf);
	//ipf_dump_element(ipf, 0, "inventory.serial_number", UTYPE_STRING);
	//ipf_dump_element(ipf, 1, "inventory.serial_number", UTYPE_STRING);
	return bytes;
}

UINT32 make_ipf_by_buff(char *in, int inlen, char *ipf, UINT32 max_size)
{
	lua2ipf_parser_t parser;
	UINT32 bytes;
	parser_init(&parser);
	lua2ipf(&parser, in, inlen);
	bytes = make_ipf(&parser, ipf, max_size);
	ipf_dump(ipf);
	return bytes;
}

#endif // INCLUDE_LUA_CONVERSION
