/* Created by w.h. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "DataInterchange.h"

static int write_stream(const void *buffer, size_t size, void *app_key)
{
	FILE *fp = app_key;
	size_t wrote;
	wrote = fwrite(buffer, 1, size, fp);
	return (wrote == size) ? 0 : -1;
}

int main(int argc, char **argv)
{
	int rc;
	FILE *fp = 0;
	char path[65];
	long file_size;
	void *buffer = NULL;
	DataInterChange_t *di = NULL;
	asn_dec_rval_t rval;
	asn_enc_rval_t er;

	if (argc < 2) {
		fprintf(stderr,"Usage:asn1c_test asn1_file\n");
		goto exit;
	}

	memset((void *)path, 0, 65);
	strncpy(path, argv[1], 60);
	fprintf(stdout,"file name: %s\n",path);

	if ((fp = fopen(path, "rb+")) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	fprintf(stdout,"file size: %d\n",file_size);

	if ((buffer = malloc(file_size)) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto exit;
	}
	memset(buffer, 0, file_size);

	if ((rc = fread(buffer, 1, file_size, fp)) != file_size) {
		fprintf(stderr, "fread fail, only read %d bytes\n", rc);
		goto exit;
	}

	rval = asn_DEF_DataInterChange.ber_decoder(0, &asn_DEF_DataInterChange, (void **)&di, buffer, file_size, 0);
	if (rval.code != RC_OK) {
		fprintf(stderr, "ber_decoder fail\n");
		goto exit;
	}

	asn_fprint(stdout, &asn_DEF_DataInterChange, di);

	fclose(fp);

	strcat(path, ".2");
	fprintf(stdout, "creating encoded file %s\n", path);

	if ((fp = fopen(path, "wb+")) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto exit;
	}

	er = der_encode(&asn_DEF_DataInterChange, di, write_stream, fp);
	if (er.encoded == -1) {
		fprintf(stderr, "der_encode fail %s : %s\n", er.failed_type->name, strerror(errno));
		goto exit;
	}

	fprintf(stdout, "der_encode %d bytes\n", er.encoded);

exit:

	if (buffer != NULL)
		free(buffer);

	if (di != NULL)
		asn_DEF_DataInterChange.free_struct(&asn_DEF_DataInterChange, di, 0);

	if (fp != NULL)
		fclose(fp);
}

