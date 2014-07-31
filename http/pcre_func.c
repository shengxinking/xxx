#include <stdio.h>
#include "pcre_func.h"

pcre *
pcre_compile_exp(const char *pattern, int options)
{
	pcre *re;
	const char *err;
	int err_offset;

	if (pattern == NULL)
		return NULL;

	re = pcre_compile(pattern, options, &err, &err_offset, NULL);

	return re;
}


pcre_extra * 
pcre_study_exp(pcre *re, int options)
{
	pcre_extra *extra;
	const char *err;

	if (re == NULL) {
		return NULL;
	}

	extra = pcre_study(re, options, &err);

	return extra;
}

int pcre_study_exp2(pcre *re, pcre_extra **extra)
{
	const char *err = NULL;

	if (re == NULL || extra == NULL) {
		return -1;
	}

	//*extra = pcre_study(re, 0, &err);
	*extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &err);
	
	if (*extra == NULL) {
		printf("Can not study pattern.\n");
	}

	if (*extra != NULL && !((*extra)->flags & PCRE_EXTRA_EXECUTABLE_JIT)) {
		printf("JIT compiler does not support.\n");
	}

	if (*extra == NULL && err != NULL) {
		printf("%s: pcre_study() %s\n", __FUNCTION__, err);
		return -1;
	}

	return 0;
}

int pcre_match_exp2(pcre *exp, pcre_extra *extra, 
		    int index, char *buf, int len, 
		    int *begin, int *end)
{
#define SVR_PCRE_CAPTURE_NUM	10
	int ret = 0, idx;
	int ovector[3 * SVR_PCRE_CAPTURE_NUM] = {-1};

	if (exp == NULL || buf == NULL || len <= 0) {
		return -1;
	}
	
	if (index >= SVR_PCRE_CAPTURE_NUM) {
		return -1;
	}

	ret = pcre_exec(exp, extra, buf, len, 0, 0, ovector, 
			3 * SVR_PCRE_CAPTURE_NUM);
	if (ret < 0) {
		return -1;
	}

	idx = index * 2;

	if (begin != NULL && end != NULL) {
		if (ovector[idx] < 0 || ovector[idx + 1] < 0 || \
		    ovector[idx + 1] < ovector[idx]) {
			return -1;
		}

		*begin = ovector[idx];
		*end = ovector[idx + 1];
	}

	return 1;
}


int pcre_match_exp(pcre *exp, pcre_extra *extra, 
		   char *buf, int len, 
		   int *begin, int *end)
{
	int ret = 0;
	int ovector[3] = {-1, -1, -1};

	if (!exp || !buf || len < 0) {
		return 0;
	}

	ret = pcre_exec(exp, extra, buf, len, 0, 0, ovector, 3);
	if (ret < 0) {
		return 0;
	}

	if (ovector[0] >= 0 && ovector[1] > ovector[0] && begin && end) {
		*begin = ovector[0];
		*end = ovector[1];
	}

	return 1;
}


