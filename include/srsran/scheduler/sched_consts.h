/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/ran/resource_block.h"
#include <cstddef>

namespace srsran {

/// Maximum number of layers (implementation-defined)
const size_t MAX_NOF_LAYERS = 2;

/// SSB constants.
/// FR1 = [ 410 MHz – 7125 MHz] (TS 38.101, Section 5.1) and ARFCN corresponding to 7.125GHz is 875000.
const unsigned FR1_MAX_FREQUENCY_ARFCN = 875000;
/// The cutoff frequency for case A, B and C paired is 3GHz, corresponding to 600000 ARFCN (TS 38.213, Section 4.1).
const unsigned CUTOFF_FREQ_ARFCN_CASE_A_B_C = 600000;
/// The cutoff frequency for case C unpaired is 1.88GHz, corresponding to 376000 ARFCN (TS 38.213, Section 4.1).
const unsigned CUTOFF_FREQ_ARFCN_CASE_C_UNPAIRED = 376000;
const unsigned NOF_SSB_OFDM_SYMBOLS              = 4;

/// SIB1 constants.
/// SIB1 periodicity, see TS 38.331, Section 5.2.1.
const unsigned SIB1_PERIODICITY = 160;
/// [Implementation defined] Max numbers of beams, to be used for SIB1 scheduler.
/// NOTE: This is temporary, and valid only for FR1.
const unsigned MAX_NUM_BEAMS = 8;

/// [Implementation defined] Maximum allowed slot offset between DCI and its scheduled PDSCH. Values {0,..,32}.
const unsigned SCHEDULER_MAX_K0 = 15;

/// [Implementation defined] Maximum allowed slot offset between PDSCH to the DL ACK/NACK. Values {0,..,15}.
const unsigned SCHEDULER_MAX_K1 = 15;

/// [Implementation defined] Minimum allowed slot offset between PDSCH to the DL ACK/NACK. Values {0,..,15}.
/// \remark Tested UEs do not support k1 < 4.
const unsigned SCHEDULER_MIN_K1 = 4;

/// [Implementation defined] Maximum allowed slot offset between DCI and its scheduled first PUSCH. Values {0,..,32}.
const unsigned SCHEDULER_MAX_K2 = 15;

/// Maximum value of Msg delta. See table 6.1.2.1.1-5, in TS 38.214.
const unsigned MAX_MSG3_DELTA = 6;

/// Maximum number of PDSCH time domain resource allocations. See TS 38.331, \c maxNrofDL-Allocations.
const unsigned MAX_NOF_PDSCH_TD_RESOURCE_ALLOCATIONS = 16;

} // namespace srsran
