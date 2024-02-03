#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <endian.h>
#include <ctype.h>
#include <linux/swab.h>
#include "Start.h"
#define MAX_BUF_SIZE 500
#define TPM_CC_VENDOR_BIT_MASK 0x20000000
#define VENDOR_RC_ERR 0x00000500
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
static char test(){
	printf("Hello!");
}
/* Helpers to convert between binary and hex ascii and back. */
static char to_hexascii(uint8_t c)
{
	if (c <= 9)
		return '0' + c;
	return 'a' + c - 10;
}
static int from_hexascii(char c)
{
	/* convert to lower case. */
	c = tolower(c);
	if (c < '0' || c > 'f' || ((c > '9') && (c < 'a')))
		return -1; /* Not an ascii character. */
	if (c <= '9')
		return c - '0';
	return c - 'a' + 10;
}
static FILE *tpm_output;
static int ts_write(const void *out, size_t len)
{
	const char *cmd_head = "PATH=\"${PATH}:/usr/sbin\" trunks_send --raw ";
	size_t head_size = strlen(cmd_head);
	char full_command[head_size + 2 * len + 1];
	size_t i;
	strcpy(full_command, cmd_head);
	/*
	 * Need to convert binary input into hex ascii output to pass to the
	 * trunks_send command.
	 */
	for (i = 0; i < len; i++) {
		uint8_t c = ((const uint8_t *)out)[i];
		full_command[head_size + 2 * i] = to_hexascii(c >> 4);
		full_command[head_size + 2 * i + 1] = to_hexascii(c & 0xf);
	}
	/* Make it a proper zero terminated string. */
	full_command[sizeof(full_command) - 1] = 0;
	tpm_output = popen(full_command, "r");
	if (tpm_output)
		return len;
	fprintf(stderr, "Error: failed to launch trunks_send --raw\n");
	return -1;
}
static int ts_read(void *buf, size_t max_rx_size)
{
	int i;
	int pclose_rv;
	int rv;
	char response[max_rx_size * 2];
	if (!tpm_output) {
		fprintf(stderr, "Error: attempt to read empty output\n");
		return -1;
	}
	rv = fread(response, 1, sizeof(response), tpm_output);
	if (rv > 0)
		rv -= 1; /* Discard the \n character added by trunks_send. */
	if (tpm_output){
		pclose_rv = pclose(tpm_output);
	}
	if (pclose_rv < 0) {
		fprintf(stderr,
	 		"Error: pclose failed: error %d (%s)\n",
	 		errno, strerror(errno));
		return -1;
	}
	tpm_output = NULL;
	if (rv & 1) {
		fprintf(stderr,
			"Error: trunks_send returned odd number of bytes: %s\n",
		response);
		return -1;
	}
	for (i = 0; i < rv/2; i++) {
		uint8_t byte;
		char c;
		int nibble;
		c = response[2 * i];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr,	"Error: "
				"trunks_send returned non hex character %c\n",
				c);
			return -1;
		}
		byte = nibble << 4;
		c = response[2 * i + 1];
		nibble = from_hexascii(c);
		if (nibble < 0) {
			fprintf(stderr,	"Error: "
				"trunks_send returned non hex character %c\n",
				c);
			return -1;
		}
		byte |= nibble;
		((uint8_t *)buf)[i] = byte;
	}
	return rv/2;
}
static int tpm_send_pkt(struct transfer_descriptor *td, unsigned int digest,
			unsigned int addr, const void *data, int size,
			void *response, size_t *response_size,
			uint16_t subcmd)
{
	/* Used by transfer to /dev/tpm0 */
	static uint8_t outbuf[MAX_BUF_SIZE];
	struct upgrade_pkt *out = (struct upgrade_pkt *)outbuf;
	int len, done;
	int response_offset = offsetof(struct upgrade_pkt, command.data);
	void *payload;
	size_t header_size;
	uint32_t rv;
	const size_t rx_size = sizeof(outbuf);
	out->tag = htobe16(0x8001);
	out->subcmd = htobe16(subcmd);
    out->ordinal = htobe32(TPM_CC_VENDOR_BIT_MASK);
    header_size = offsetof(struct upgrade_pkt, command.data);
	payload = outbuf + header_size;
	len = size + header_size;
	out->length = htobe32(len);
	memcpy(payload, data, size);

	done = ts_write(out, len);

	if (done < 0) {
		perror("Could not write to TPM");
		return -1;
	} else if (done != len) {
		fprintf(stderr, "Error: Wrote %d bytes, expected to write %d\n",
			done, len);
		return -1;
	}

	len = ts_read(outbuf, rx_size);

	len = len - response_offset;
	if (len < 0) {
		fprintf(stderr, "Problems reading from TPM, got %d bytes\n",
			len + response_offset);
		return -1;
	}
	if (response && response_size) {
		len = MIN(len, *response_size);
		memcpy(response, outbuf + response_offset, len);
		*response_size = len;
	}
	/* Return the actual return code from the TPM response header. */
	memcpy(&rv, &((struct upgrade_pkt *)outbuf)->ordinal, sizeof(rv));
	rv = be32toh(rv);
	/* Clear out vendor command return value offset.*/
	if ((rv & VENDOR_RC_ERR) == VENDOR_RC_ERR)
		rv &= ~VENDOR_RC_ERR;
	return rv;
}
