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

void initialize_reconciliation_compute_sketch()
{
    static const BasicTestingSetup basic_testing_setup;
}

FUZZ_TARGET_INIT(reconciliation_compute_sketch, initialize_reconciliation_compute_sketch)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    ReconState recon(true, false, false, 0, 0);
    uint16_t capacity{16};
    while (fuzzed_data_provider.ConsumeBool()) {
        std::set<uint256> sketches;
        for (int i = 0; i < 8; ++i) {
            sketches.emplace(ConsumeUInt256(fuzzed_data_provider));
        }
        recon.ComputeSketch(sketches, capacity);
    }
}
