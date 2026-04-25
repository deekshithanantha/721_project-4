#ifndef VTAGE_H
#define VTAGE_H

#include <inttypes.h>
#include <vector>
#include <cassert>
#include <algorithm>


class vtage {
private:
    struct base_entry_t {
        uint64_t value;
        uint64_t conf;
    };

    struct tagged_entry_t {
        bool     valid;
        uint32_t tag;
        uint64_t value;
        uint64_t conf;
        bool     useful;
    };

    std::vector<base_entry_t>                base_table;
    std::vector<std::vector<tagged_entry_t>> tagged;

    uint64_t              num_tables;
    uint64_t              base_idx_bits;
    uint64_t              tagged_idx_bits;
    uint64_t              tag_bits;
    uint64_t              conf_max;
    std::vector<uint64_t> hist_lengths;   
    uint64_t              max_hist;

    // Speculative BHR maintained at rename time.
    uint64_t              spec_bhr;
    uint64_t              n_chkpts;
    std::vector<uint64_t> bhr_ckpt;     

    static inline uint64_t hist_mask(uint64_t len) {
        if (len == 0) return 0ULL;
        return (len >= 64) ? ~0ULL : ((1ULL << len) - 1ULL);
    }

    uint64_t fold_history(uint64_t hist, uint64_t hist_len, uint64_t out_bits) const;
    uint64_t base_index(uint64_t pc) const;

    uint64_t tagged_index_bhr(uint64_t pc, uint64_t t, uint64_t bhr) const;
    uint32_t compute_tag_bhr(uint64_t pc, uint64_t t, uint64_t bhr) const;

public:

    vtage(uint64_t n_chkpts,
          uint64_t num_tables,
          uint64_t base_idx_bits,
          uint64_t tagged_idx_bits,
          uint64_t min_hist_len,
          uint64_t tag_bits,
          uint64_t conf_max);


    int predict(uint64_t pc, bool &pred_avail, bool &confident, uint64_t &prediction);


    uint64_t get_spec_bhr() const { return spec_bhr; }


    void branch_checkpoint(uint64_t branch_id, bool predicted_taken);


    void bhr_restore(uint64_t branch_id);


    void train(uint64_t pc, uint64_t actual_value, int provider, uint64_t bhr_snapshot);


    void reset();

    uint64_t storage_bits() const;
};

#endif // VTAGE_H
