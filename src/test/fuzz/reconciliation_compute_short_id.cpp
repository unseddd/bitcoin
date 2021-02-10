#include <chrono>
#include <map>
#include <set>
#include <vector>

#include <crypto/siphash.h>
#include <uint256.h>
#include <reconciliation.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

void initialize_reconciliation_compute_short_id()
{
    static const BasicTestingSetup basic_testing_setup;
}

FUZZ_TARGET_INIT(reconciliation_compute_short_id, initialize_reconciliation_compute_short_id)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    ReconState recon(false, false, false, 0, 0);
    while (fuzzed_data_provider.ConsumeBool()) {
        recon.ComputeShortID(ConsumeUInt256(fuzzed_data_provider));
    }
}
