#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <endian.h>
#include <linux/swab.h>
struct transfer_descriptor {
    /*
	 * Set to true for use in an upstart script. Do not reboot after
	 * transfer, and do not transfer RW if versions are the same.
	 *
	 * When using in development environment it is beneficial to transfer
	 * RW images with the same version, as they get started based on the
	 * header timestamp.
	 */
	uint32_t upstart_mode;
	/*
	 * Override in case updater is used w/ boards that do not follow
	 * the cr50 versioning scheme.
	 */
	uint32_t background_update_supported;
	/*
	 * offsets of RO and WR sections available for update (not currently
	 * active).
	 */
	uint32_t ro_offset;
	uint32_t rw_offset;
	uint32_t post_reset;
	/* Type of channel used to communicate with Cr50. */
	enum transfer_type {
		usb_xfer = 0,  /* usb interface. */
		dev_xfer = 1,  /* /dev/tpm0 */
		ts_xfer = 2    /* trunks_send */
	} ep_type;
	union {
		int tpm_fd;
	};
	
};
struct upgrade_pkt {
	__be16	tag;
	__be32	length;
	__be32	ordinal;
	__be16	subcmd;
	union {
		/*
		 * Upgrade PDUs as opposed to all other vendor and extension
		 * commands include two additional fields in the header.
		 */
		struct {
			__be32	digest;
			__be32	address;
			char data[0];
		} upgrade;
		struct {
			char data[0];
		} command;
	};
} __packed;