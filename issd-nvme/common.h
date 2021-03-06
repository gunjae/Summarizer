
#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>

#include "platform.h"

#include "pcie.h"
#include "nvme_regs.h"
#include "dma_mgr.h"
#include "dma_df_nand.h"

#include <syslog.h>
#define COND_SUCCESS   0
#define RETURN_ON_ERROR(cond, ret, fmt, args...) \
	do {                                         \
		if (COND_SUCCESS != (ret = cond)) {      \
			if (*fmt) printf (fmt, ##args);      \
			return ret;                          \
		}                                        \
	} while (0)
#define JUMP_ON_ERROR(cond, ret, fmt, args...)   \
	do {                                         \
		if (COND_SUCCESS != (ret = cond)) {      \
			if (*fmt) printf (fmt, ##args);      \
			goto CLEAN_EXIT;                     \
		}                                        \
	} while (0)
#define PERROR_ON_ERROR(cond, ret, fmt, args...) \
	do {                                         \
		if (COND_SUCCESS != cond) {              \
			if (*fmt) printf (fmt, ##args);      \
			ret = errno;                         \
			goto CLEAN_EXIT;                     \
		}                                        \
	} while (0)

#define PF(n, str1, fmt, args...) \
	do { \
		if (n) { \
			if (*str1) { \
				printf (str1 fmt, ##args); \
			} \
		} \
	} while (0)

#define CIRCULAR_INCR(idx, limit) (idx = (idx + 1) % limit)
#ifndef likely
#define likely(x)       __builtin_expect((x),1)
#endif
#ifndef unlikely
#define unlikely(x)     __builtin_expect((x),0)
#endif

/// gunjae's log functions
//#define GK_PRINT(args...) fprintf(stdout, args)
//#define GK_PRINT(args...) ((void)0)

/*Structures*/
typedef struct NvmeTimer {
	timer_t timerID;
	struct sigevent sev;
	struct itimerspec its;
	struct sigaction sa;
} NvmeTimer;

/*Fix the offsets properly -TODO*/
#define FPGA_OFFS_NVME_REGS     0x0000
#define FPGA_OFFS_FIFO_ENTRY    0x0001
#define FPGA_OFFS_FIFO_COUNT    0x0002
#define FPGA_OFFS_FIFO_RESET    0x0003
#define FPGA_OFFS_FIFO_IRQ_CSR  0x0004 /*Unused in Target*/
#define FPGA_OFFS_IOSQDBST      0x0005 /*5 to 8: 4 32 bit regs*/
#define FPGA_OFFS_GPIO_CSR      0x0009

#if 0
#define FPGA_OFFS_DMA_CSR       0x0000
#define FPGA_OFFS_ICR           0x0001 /*Present from PCIe2 only*/
#define FPGA_OFFS_DMA_TABLE_SZ  0x0002
#define FPGA_OFFS_DMA_TABLE     0x1000
#else
#define FPGA_OFFS_DMA_CSR       0x2000
#define FPGA_OFFS_ICR           0x2001 /*Present from PCIe2 only*/
#define FPGA_OFFS_DMA_TABLE_SZ  0x2002
#define FPGA_OFFS_DMA_TABLE     0x40000
#endif

/**
 * @Structure  : FpgaCtrl
 * @Brief      : Contains pointers to various control registers and
 * 				 pools in FPGA
 * @Members
 *  @ep          : FPGA PCIe (2 and 4) Endpoint Info
 *  @nvme_regs   : NVMe controller register table
 *  @fifo_reg    : total number of BAR regions exposed by PCI device
 *  @fifo_count  : list of all BARs
 *  @fifo_reset    : Control and Status register for fpga
 *  @iosqdb_bits : 128 bit register for 128 IO SQ DB status - bit per SQ
 *  @gpio_csr    : Control and Status register for gpio interrupts
 *  @irq_csr     : Control and Status register for different types of irqs
 *  @dma_csr     : Control and Status register for dma events
 *  @dma_table   : dma tables from PCIe2 and PCIe4
 */

// gunjae: controller->FPGA ?
typedef struct FpgaCtrl {
	PCIDevice    ep[2];
	NvmeRegs     *nvme_regs;
	uint32_t     *fifo_reg;
	uint32_t     *fifo_count;
	uint32_t     *fifo_msi;
	uint32_t     *fifo_reset;
	uint32_t     *iosqdb_bits;
	uint32_t     *gpio_csr;
	uint32_t     *icr[2];
	DmaRegs      dma_regs;
	Nand_DmaRegs nand_dma_regs;
} FpgaCtrl;

typedef struct MsixTable {
	void         *addr_ptr;
	void         *data_ptr;
	void         *msix_en_ptr;
} MsixTable;

typedef struct MsiTable {
	void         *addr_ptr;
	void         *data_ptr;
	void         *msi_en_ptr;
} MsiTable;

// gunjae: host->PICe->controller ?
typedef struct HostCtrl {
	PCIDevice    rc;
	MemoryRegion io_mem;
	MemoryRegion msix_mem;
	MsixTable    msix;
	MsiTable     msi;
	uint8_t      irq_mode;
	int          fd_mem;
} HostCtrl;

// gunjae: ISSD local memory as a DMA buffer (host reflected)
typedef struct DmaBufCtrl {
	MemoryRegion io_mem;
	int          fd_mem;
} DmaBufCtrl;
/////////////////////////////////////////////

typedef struct PageInfo {
	uint64_t npages;
	uint64_t page_size;
} PageInfo;

typedef struct RamdiskCtrl {
	MemoryRegion mem;
	int          fd_mem;
	uint8_t      is_valid;
} RamdiskCtrl;

typedef struct PageInfo NandCtrl;
typedef struct PageInfo DDRCtrl;

typedef struct NvmeBioTarget {
	RamdiskCtrl  ramdisk;
	NandCtrl     nand;
	DDRCtrl      ddr;
	uint8_t      mem_type[3];
} NvmeBioTarget;

typedef struct NvmeIrq {
	uint32_t     irq_num;
	uint16_t     count;
} NvmeIrq;

typedef struct FpgaIrqs {
	uint32_t     irq_bits;
#define NIRQ_FIFO  0x00000001
#define NIRQ_IODB  0x00000002
#define NIRQ_DMA   0x00000004
#define NIRQ_GPIO1 0x00000010
#define NIRQ_GPIO2 0x00000020
#define NIRQ_GPIO3 0x00000040
#define NIRQ_GPIO4 0x00000080
	NvmeIrq      fifo;
	NvmeIrq      iodb;
	NvmeIrq      dma;
	NvmeIrq      gpio;
} FpgaIrqs;

typedef struct icr_reg {
	uint8_t     g_irq_support:1;
	uint8_t     irq_dma1_en:1;
	uint8_t     irq_dma2_en:1;
	uint8_t     irq_fifo_en:1;
	uint8_t     irq_db_en:1;
	uint8_t     rsv6:3;
	uint8_t     irq_stat:2;
	/*Verify the values -TODO*/
#define IRQ_DMA1 1
#define IRQ_DMA2 2
#define IRQ_FIFO 3
#define IRQ_DB   4
	uint32_t     rsv7:22;
} icr_reg;

/*Function pointers*/
typedef void (*sig_cb)(int, siginfo_t *, void *);

#ifndef MIN
#define MIN(a,b) (a<b)?a:b
#endif

/*if defined as 1 this assert()'s given condition, else just passes on*/
#define ASSERT_ENABLED 1

#if ASSERT_ENABLED
/*assert failed mempory allocation*/
#define MEM_ASSERT(mem) assert (mem)
#else
#define MEM_ASSERT(mem)
#endif

static inline void hexDump_32Bit (uint32_t *buf, uint16_t size)
{
	uint16_t n32 = 0;
	for (; n32 < size; n32++) {
		printf ("%08x%s", *buf++, (n32+1)%1?" ":"\r\n");
	}
}

#define hexDump_SQEntry(buf) hexDump_32Bit (buf, 16)

#define FREE_VALID(buf) do { if (buf) {free (buf); buf = NULL;} } while (0)

#define SAFE_CLOSE(fd) do { if (fd > 0) {close (fd); fd = 0;} } while (0)

inline void set_thread_affinity (int cpuid, pthread_t thread);
int setup_ctrl_c_handler (void);

int mmap_fpga_bars (FpgaCtrl *fpga, int *fd_uio);
void munmap_fpga_bars (FpgaCtrl *fpga);
int setupFpgaCtrlRegs (FpgaCtrl *fpga);
int mmap_host_mem (HostCtrl *host);
void munmap_host_mem (HostCtrl *host);
uint64_t prp_read_write (uint64_t host_base, uint64_t prp_ptr, uint64_t *list, uint64_t list_len);
inline void munmap_prp_addr (void *addr, uint8_t nPages, int *fd);

/*Timer functions*/
NvmeTimer *create_nvme_timer (void *timerCB, void *arg, uint16_t expiry_ns, int signo);
inline int nvme_timer_expired (NvmeTimer *timer);
inline int start_nvme_timer (NvmeTimer *timer);
inline void stop_nvme_timer (NvmeTimer *timer);
inline void delete_nvme_timer_fn (NvmeTimer **timer);
inline void ramdisk_prp_rw (void *prp, uint64_t ramdisk_addr, uint32_t req_type, uint64_t len);
inline void close_ramdisk (RamdiskCtrl *ramdisk);

/*Gc and Spare area handlers*/
uint8_t read_mem_addr();
uint8_t* mmap_oob_addr(uint32_t oob_mem_size,int idx);

#define nvme_timer_pending(timer) !nvme_timer_expired (timer)
#define delete_nvme_timer(timer) delete_nvme_timer_fn (&timer)

#if (GK_TEST_RD_NAND==2)	// functions
#endif	// GK_TEST_RD_NAND


#endif /*#ifndef __COMMON_H*/

