#ifndef ARCH_DETAIL_ICELAKEX_H
#define ARCH_DETAIL_ICELAKEX_H

#include <stdint.h>

#define ARCH 3

#define MAX_SOCKETS 2
#define NUM_CHA 40
#define NUM_CTR_PER_CHA 4

#define JSON_FILE_PATH "events/cha_events_icx_parsed.json" // Path to events json file
#define OFFSET_FILE "cha_map_icx_mammoth.log" 

// Global Performance Monitoring Control MSRs
#define U_MSR_PMON_GLOBAL_CTL          0x0700L       // contains bits that can stop (.frz_all) / restart (.unfrz_all) all the uncore counters
#define U_MSR_PMON_GLOBAL_STATUS       0x0701L       // shows overflows
#define U_MSR_PMON_GLOBAL_CTL_frz_all  (1UL << 63)           // freeze all counters (bit 63 to 1)
#define U_MSR_PMON_GLOBAL_CTL_unfrz_all (1UL << 61)          // unfreeze all counters (bit 61 to 1)

// Unit Level PMON State
#define CHA_MSR_PMON_BASE(cha) \
    ((cha) < 18 ? (0x0E00 + (cha) * 0x0e) : \
    ((cha) < 34 ? (0x0F0A + ((cha) - 18) * 0x0e) : \
    (0x0B60 + ((cha) - 34) * 0x0e)))
#define CHA_MSR_PMON_STATUS(cha) \
    ((cha) < 18 ? (0x0E07 + (cha) * 0x0e) : \
    ((cha) < 34 ? (0x0F11 + ((cha) - 18) * 0x0e) : \
    (0x0B67 + ((cha) - 34) * 0x0e)))

// Unit Level PMON State RESET
#define U_MSR_PMON_UNIT_CTL_rst_ctrl (1UL << 0)
#define U_MSR_PMON_UNIT_CTL_rst_ctrs (1UL << 1)
#define U_MSR_PMON_UNIT_CTL_rst_both (U_MSR_PMON_UNIT_CTL_rst_ctrl | U_MSR_PMON_UNIT_CTL_rst_ctrs)

// Unit PMON state - Counter/Control Pairs
// Ctrl0 is at 0x1, ctrl1 is at 0x2, ctrl2 is at 0x3, ctrl3 is at 0x4 from the specific CHA_MSR_PMON_BASE
#define MSR_UNIT_CTRL0(cha)   (CHA_MSR_PMON_BASE(cha) + 0x1)
#define MSR_UNIT_CTRL1(cha)   (CHA_MSR_PMON_BASE(cha) + 0x2)
#define MSR_UNIT_CTRL2(cha)   (CHA_MSR_PMON_BASE(cha) + 0x3)
#define MSR_UNIT_CTRL3(cha)   (CHA_MSR_PMON_BASE(cha) + 0x4)
// Ctr0 is at 0x8, ctr1 is at 0x9, ctr2 is at 0xA, ctr3 is at 0xB from the specific CHA_MSR_PMON_BASE
#define MSR_UNIT_CTR0(cha)    (CHA_MSR_PMON_BASE(cha) + 0x8)
#define MSR_UNIT_CTR1(cha)    (CHA_MSR_PMON_BASE(cha) + 0x9)
#define MSR_UNIT_CTR2(cha)    (CHA_MSR_PMON_BASE(cha) + 0xA)
#define MSR_UNIT_CTR3(cha)    (CHA_MSR_PMON_BASE(cha) + 0xB)

// CHA Filters
#define MSR_UNIT_FILTER0(cha)   (CHA_MSR_PMON_BASE(cha) + 0x5)
#define MSR_UNIT_FILTER1(cha)   (CHA_MSR_PMON_BASE(cha) + 0x6)
// Filter0 FMESI filter
#define MSR_UNIT_FILTER0_FMESI 0x0
#define MSR_UNIT_FILTER0_CLR 0x0

// Unit PMON state - Control reset
#define MSR_UNIT_CTL_RST    (1UL << 17)
// Unit PMON state - Control enable
#define MSR_UNIT_CTL_EN     (1UL << 22)
// Unit PMON state - Control umask bits (8 bits [15:7])
#define MSR_UNIT_CTL_UMASK(umask)   ((umask) << 8)
// Unit PMON state - Control event bits (8 bits [7:0])
#define MSR_UNIT_CTL_EVENT(event)   (event)

#endif // ARCH_DETAIL_ICELAKEX_H
