#ifndef ARCH_DETAIL_SAPPHIRERAPIDS_H
#define ARCH_DETAIL_SAPPHIRERAPIDS_H

#include <stdint.h>

#define ARCH 4

#define MAX_SOCKETS 4
// Determine the number of CHA:
// lspci | grep :1e.3
// setpci -s XX:1e.3 0x9c.l
// XX in based on the lspci output
#define NUM_CHA 28
#define NUM_CTR_PER_CHA 4


#define JSON_FILE_PATH "events/cha_events_spr_parsed.json" // Path to events json file
#define OFFSET_FILE "cha_map_spr.log" 

// Global Performance Monitoring Control MSRs
#define U_MSR_PMON_GLOBAL_CTL          0x2FF0L       // contains bits that can stop (.frz_all) / restart (.unfrz_all) all the uncore counters
#define U_MSR_PMON_GLOBAL_STATUS       0x2FF2L       // 0x2FF2,0x2FF3 Global Status
#define U_MSR_PMON_GLOBAL_CTL_frz_all  (1UL << 0)  // Freeze all uncore performance monitors (bit 0 to 1)
#define U_MSR_PMON_GLOBAL_CTL_unfrz_all (0UL << 0)          // unfreeze all counters (bit 0 to 0)

// Unit Level PMON State
#define CHA_MSR_PMON_BASE(cha)   (0x2000 + (cha) * 0x10)     // Unit Ctrl = 0x2000 + (CHA * 0x10)
#define CHA_MSR_PMON_STATUS(cha) (0x2001 + (cha) * 0x10)     // Unit Status = 0x2001 + (CHA * 0x10)

// Unit Level PMON State RESET
#define U_MSR_PMON_UNIT_CTL_rst_ctrl (1UL << 8)
#define U_MSR_PMON_UNIT_CTL_rst_ctrs (1UL << 9)
#define U_MSR_PMON_UNIT_CTL_rst_both (U_MSR_PMON_UNIT_CTL_rst_ctrl | U_MSR_PMON_UNIT_CTL_rst_ctrs)

// Unit PMON state - Counter/Control Pairs
// Ctrl0 is at 0x2, ctrl1 is at 0x3, ctrl2 is at 0x4, ctrl3 is at 0x5 from the specific CHA_MSR_PMON_BASE
#define MSR_UNIT_CTRL0(cha)   (CHA_MSR_PMON_BASE(cha) + 0x2)
#define MSR_UNIT_CTRL1(cha)   (CHA_MSR_PMON_BASE(cha) + 0x3)
#define MSR_UNIT_CTRL2(cha)   (CHA_MSR_PMON_BASE(cha) + 0x4)
#define MSR_UNIT_CTRL3(cha)   (CHA_MSR_PMON_BASE(cha) + 0x5)
// Ctr0 is at 0x8, ctr1 is at 0x9, ctr2 is at 0xA, ctr3 is at 0xB from the specific CHA_MSR_PMON_BASE
#define MSR_UNIT_CTR0(cha)    (CHA_MSR_PMON_BASE(cha) + 0x8)
#define MSR_UNIT_CTR1(cha)    (CHA_MSR_PMON_BASE(cha) + 0x9)
#define MSR_UNIT_CTR2(cha)    (CHA_MSR_PMON_BASE(cha) + 0xA)
#define MSR_UNIT_CTR3(cha)    (CHA_MSR_PMON_BASE(cha) + 0xB)

// CHA Filters
#define MSR_UNIT_FILTER0(cha)   (CHA_MSR_PMON_BASE(cha) + 0xE)
#define MSR_UNIT_FILTER1(cha)   (CHA_MSR_PMON_BASE(cha) + 0x6) // Not available
// Filter0 FMESI filter
#define MSR_UNIT_FILTER0_FMESI 0x01E20000
#define MSR_UNIT_FILTER0_CLR 0x200

// Unit PMON state - Control reset
#define MSR_UNIT_CTL_RST    (1UL << 17)
// Unit PMON state - Control enable
// Sapphire Rapids does not have a unit counter ctrl enable bit
#define MSR_UNIT_CTL_EN     (0UL << 0)
// Unit PMON state - Control umask bits (8 bits [15:8])
#define MSR_UNIT_CTL_UMASK(umask)   ((umask) << 8)
// Unit PMON state - Control event bits (8 bits [7:0])
#define MSR_UNIT_CTL_EVENT(event)   (event)
// Unit PMON state - Control umask extra bits (16 bits [57:32])
#define MSR_UNIT_CTL_EXTRA(extra)   ((extra) << 32)

#endif // ARCH_DETAIL_SAPPHIRERAPIDS_H
