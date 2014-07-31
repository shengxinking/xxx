#ifndef _LIB_MULTI_PAT
#define _LIB_MULTI_PAT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "acsmx.h"

#define	MAX_KEY_WORD_ID	4096
#define MAX_BIT_MAP_UNIT	(MAX_KEY_WORD_ID / 8)

#define MAX_KEY_WORD_LEN 128

enum{
	CHECK_NONE = 0,
	CHECK_LEFT ,
	CHECK_RIGHT,
	CHECK_BOTH,
};

typedef struct _match_mulpat_target_t {
	char is_key_word_optimize;
	long long target_type;
} match_mulpat_target_t;

typedef struct _match_pattern_rst_t {
	char have_general_keyword;
	int key_word_optimize_len;
	int *key_word_optimize;

	unsigned int matched_count;

	unsigned int search_target;
	match_mulpat_target_t meet_target[MAX_KEY_WORD_ID];
	unsigned char bit_map[MAX_BIT_MAP_UNIT];

	char *data;
	int data_len;
} match_pattern_rst_t;

typedef struct _multi_pattern_node_t {
	char keyword[MAX_KEY_WORD_LEN];
	unsigned int keyword_id;
	unsigned int border_flag;

	struct _multi_pattern_node_t *next;
} multi_pattern_node_t;

int check_hex_encode(char *src, char *dst);
int search_multi_pattern(ACSM_STRUCT *acbm_root, match_pattern_rst_t *mpat_rst, char *text, int len);

int multi_pattern_set_bit(int keyword_id, unsigned char *bit_map, int len);
int malti_pattern_is_set(int keyword_id, long long, match_pattern_rst_t *, int len);

#endif

