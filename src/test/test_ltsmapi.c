/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (c) 2017-2023, GSI Helmholtz Centre for Heavy Ion Research
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "CuTest.h"
#include "ltsmapi.c"
#include "test_utils.h"

#define SERVERNAME	"tsmserver-8"
#define NODE		"polaris"
#define PASSWORD	"polaris1234"
#define OWNER           ""

#define NUM_FILES       10
#define LEN_RND_STR     6

void test_tsm_fcalls(CuTest *tc)
{
	int rc;
	struct login_t login;
	char fpath[NUM_FILES][PATH_MAX];

	memset(fpath, 0, sizeof(char) * NUM_FILES * PATH_MAX);

	for (uint8_t r = 0; r < sizeof(fpath)/sizeof(fpath[0]); r++) {
		char rnd_s[LEN_RND_STR + 1] = {0};
		rnd_str(rnd_s, LEN_RND_STR);
		snprintf(fpath[r], PATH_MAX, "/tmp/%s", rnd_s);
	}

	login_init(&login, SERVERNAME, NODE, PASSWORD,
		   OWNER, LINUX_PLATFORM, DEFAULT_FSNAME,
		   DEFAULT_FSTYPE);

	struct session_t session;
	memset(&session, 0, sizeof(struct session_t));

	rc = tsm_init(DSM_SINGLETHREAD);
	CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);

	rc = tsm_fconnect(&login, &session);
	CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);

	FILE *file = NULL;

        for (uint8_t r = 0; r < sizeof(fpath)/sizeof(fpath[0]); r++) {

		unlink(fpath[r]);

                file = fopen(fpath[r], "w+");
		CuAssertPtrNotNull(tc, file);

		rc = tsm_fopen(DEFAULT_FSNAME, fpath[r], "written by cutest",
			       &session);
		CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);

                unsigned char *buf = NULL;
                for (uint8_t b = 0; b < 16; b++) {
			size_t buf_size = 1 + (rand() % 1048576); /* 2^20 = 1 MiB */
			buf = calloc(buf_size, sizeof(unsigned char));
			CuAssertPtrNotNull(tc, buf);

			for (size_t i = 0; i < buf_size; i++)
				buf[i] = rand() % 256;

			size_t written = 0;
			size_t total_written = 0;
			size_t to_write = buf_size;
			size_t total_written_tsm = 0;

			do {
				size_t rand_nmemb;
				size_t written_tsm;

				rand_nmemb = 1 + (rand() % buf_size);
				written = fwrite(buf, 1, rand_nmemb, file);
				total_written += written;
				written_tsm = tsm_fwrite(buf, 1, rand_nmemb, &session);
				total_written_tsm += written_tsm;

				buf_size -= rand_nmemb;
			} while (buf_size != 0 || written == 0);

			CuAssertIntEquals(tc, to_write, total_written);
			CuAssertIntEquals(tc, to_write, total_written_tsm);
			free(buf);
		}

		fclose(file);
		file = NULL;

		uint32_t crc32sum = 0;
		rc = crc32file(fpath[r], &crc32sum);
		CuAssertIntEquals(tc, 0, rc);

		CuAssertIntEquals(tc, crc32sum ,
				  session.tsm_file->archive_info.obj_info.crc32);

		rc = tsm_fclose(&session);
		CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);

		rc = tsm_delete_fpath(DEFAULT_FSNAME, fpath[r], &session);
		CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);
	}

	tsm_fdisconnect(&session);
	tsm_cleanup(DSM_SINGLETHREAD);
}

void test_extract_hl_ll(CuTest *tc)
{
	const char *fpath = "/fs/hl/ll";
	const char *fs = "/fs";
	char hl[DSM_MAX_HL_LENGTH + 1] = {0};
	char ll[DSM_MAX_LL_LENGTH + 1] = {0};

	dsInt16_t rc;
	rc = extract_hl_ll(fpath, fs, hl, ll);
	CuAssertIntEquals(tc, DSM_RC_SUCCESSFUL, rc);
	CuAssertStrEquals(tc, "/hl", hl);
	CuAssertStrEquals(tc, "/ll", ll);
}

void test_login_init(CuTest *tc)
{
	struct login_t login;
	login_init(&login, "servername", "node", "password",
		   "owner", "platform", "fsname", "fstype");

	CuAssertStrEquals(tc, "-se=servername", login.options);
	CuAssertStrEquals(tc, "node", login.node);
	CuAssertStrEquals(tc, "password", login.password);
	CuAssertStrEquals(tc, "owner", login.owner);
	CuAssertStrEquals(tc, "platform", login.platform);
	CuAssertStrEquals(tc, "fsname", login.fsname);
	CuAssertStrEquals(tc, "fstype", login.fstype);

	login_init(&login, NULL, "node", "",
		   "owner", "platform", "", NULL);
	CuAssertStrEquals(tc, login.options, login.options);
	CuAssertStrEquals(tc, login.node, login.node);
	CuAssertStrEquals(tc, login.password, login.password);
	CuAssertStrEquals(tc, login.owner, login.owner);
	CuAssertStrEquals(tc, login.platform, login.platform);
	CuAssertStrEquals(tc, login.fsname, login.fsname);
	CuAssertStrEquals(tc, login.fstype, login.fstype);

	login_init(&login, "servername", "node", "",
		   NULL, "platform", "", NULL);
	CuAssertStrEquals(tc, "-se=servername", login.options);
	CuAssertStrEquals(tc, "node", login.node);
	CuAssertStrEquals(tc, "", login.password);
	CuAssertStrEquals(tc, "", login.owner);
	CuAssertStrEquals(tc, "platform", login.platform);
	CuAssertStrEquals(tc, "", login.fsname);
	CuAssertStrEquals(tc, "", login.fstype);

	login_init(&login, NULL, NULL, NULL,
		   NULL, NULL, NULL, NULL);
	CuAssertStrEquals(tc, login.options, login.options);
	CuAssertStrEquals(tc, login.node, login.node);
	CuAssertStrEquals(tc, login.password, login.password);
	CuAssertStrEquals(tc, login.owner, login.owner);
	CuAssertStrEquals(tc, login.platform, login.platform);
	CuAssertStrEquals(tc, login.fsname, login.fsname);
	CuAssertStrEquals(tc, login.fstype, login.fstype);
}

void test_set_prefix(CuTest *tc)
{
	char *_prefix = NULL;
	set_prefix(_prefix);
	CuAssertPtrEquals(tc, NULL, _prefix);

	_prefix = calloc(128, sizeof(char));
	CuAssertPtrNotNull(tc, _prefix);
	snprintf(_prefix, 11, "%s", "0123456789");
	set_prefix(_prefix);
	CuAssertStrEquals(tc, "/0123456789", prefix);

	snprintf(_prefix, 8, "%s", "/12/45/\0");
	set_prefix(_prefix);
	CuAssertStrEquals(tc, "/12/45/", prefix);

	snprintf(_prefix, 21, "%s", "/0/1/2/3/4/5/6/7/8/9\0");
	set_prefix(_prefix);
	CuAssertStrEquals(tc, "/0/1/2/3/4/5/6/7/8/9", prefix);

	snprintf(_prefix, 2, "%s", "/\0");
	set_prefix(_prefix);
	CuAssertStrEquals(tc, "/", prefix);

	free(_prefix);
}

CuSuite* ltsmapi_get_suite()
{
    CuSuite* suite = CuSuiteNew();
#ifdef TEST_TSM_CALLS
    SUITE_ADD_TEST(suite, test_tsm_fcalls);
#endif
    SUITE_ADD_TEST(suite, test_extract_hl_ll);
    SUITE_ADD_TEST(suite, test_login_init);
    SUITE_ADD_TEST(suite, test_set_prefix);

    return suite;
}

void run_all_tests(void)
{
	api_msg_set_level(API_MSG_DEBUG);

	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();
	CuSuite *ltsmapi_suite = ltsmapi_get_suite();

	CuSuiteAddSuite(suite, ltsmapi_suite);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuSuiteDelete(ltsmapi_suite);

	free(suite);
	CuStringDelete(output);
}

int main(void)
{
	struct timespec tspec = {0};

	clock_gettime(CLOCK_MONOTONIC, &tspec);
	srand(time(NULL) + tspec.tv_nsec);
	run_all_tests();

	return 0;
}
