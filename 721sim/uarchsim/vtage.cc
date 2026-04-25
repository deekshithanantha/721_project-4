#include "vtage.h"
#include <cstring>

vtage::vtage(uint64_t n_chkpts,
             uint64_t num_tables,
             uint64_t base_idx_bits,
             uint64_t tagged_idx_bits,
             uint64_t min_hist_len,
             uint64_t tag_bits,
             uint64_t conf_max)
    : num_tables(num_tables),
      base_idx_bits(base_idx_bits),
      tagged_idx_bits(tagged_idx_bits),
      tag_bits(tag_bits),
      conf_max(conf_max),
      spec_bhr(0),
      n_chkpts(n_chkpts)
{
    uint64_t base_size = 1ULL << base_idx_bits;
    base_table.resize(base_size);
    for (auto &e : base_table) { e.value = 0; e.conf = 0; }

    uint64_t tagged_size = 1ULL << tagged_idx_bits;
    tagged.resize(num_tables);
    hist_lengths.resize(num_tables);
    max_hist = 0;
    for (uint64_t t = 0; t < num_tables; t++) {
        hist_lengths[t] = min_hist_len << t;
        if (hist_lengths[t] > max_hist) max_hist = hist_lengths[t];
        tagged[t].resize(tagged_size);
        for (auto &e : tagged[t]) {
            e.valid  = false;
            e.tag    = 0;
            e.value  = 0;
            e.conf   = 0;
            e.useful = false;
        }
    }

    bhr_ckpt.resize(n_chkpts, 0);
}

uint64_t vtage::fold_history(uint64_t hist, uint64_t hist_len, uint64_t out_bits) const {
    if (out_bits == 0 || hist_len == 0) return 0;
    uint64_t result = 0;
    uint64_t out_mask = (out_bits >= 64) ? ~0ULL : ((1ULL << out_bits) - 1ULL);
    for (uint64_t i = 0; i < hist_len; i++) {
        uint64_t bit = (hist >> i) & 1ULL;
        result ^= (bit << (i % out_bits));
    }
    return result & out_mask;
}

uint64_t vtage::base_index(uint64_t pc) const {
    uint64_t mask = (1ULL << base_idx_bits) - 1ULL;
    return (pc >> 2) & mask;
}

uint64_t vtage::tagged_index_bhr(uint64_t pc, uint64_t t, uint64_t bhr) const {
    uint64_t mask   = (1ULL << tagged_idx_bits) - 1ULL;
    uint64_t pc_idx = (pc >> 2) & mask;
    uint64_t h      = bhr & hist_mask(hist_lengths[t]);
    uint64_t hfold  = fold_history(h, hist_lengths[t], tagged_idx_bits);
    return (pc_idx ^ hfold) & mask;
}

uint32_t vtage::compute_tag_bhr(uint64_t pc, uint64_t t, uint64_t bhr) const {
    if (tag_bits == 0) return 0;
    uint64_t tmask  = (tag_bits >= 32) ? 0xFFFFFFFFULL : ((1ULL << tag_bits) - 1ULL);
    uint64_t pc_tag = (pc >> (2 + tagged_idx_bits)) & tmask;
    uint64_t h      = bhr & hist_mask(hist_lengths[t]);
    uint64_t hfold  = fold_history(h, hist_lengths[t], tag_bits);
    return (uint32_t)((pc_tag ^ hfold) & tmask);
}

int vtage::predict(uint64_t pc, bool &pred_avail, bool &confident, uint64_t &prediction) {
    pred_avail = false;
    confident  = false;
    prediction = 0;
    int provider = -1;

    for (int t = (int)num_tables - 1; t >= 0; t--) {
        uint64_t idx = tagged_index_bhr(pc, (uint64_t)t, spec_bhr);
        uint32_t tag = compute_tag_bhr(pc, (uint64_t)t, spec_bhr);
        const auto &e = tagged[(uint64_t)t][idx];
        if (e.valid && e.tag == tag && e.conf >= 1) {
            provider   = t;
            prediction = e.value;
            pred_avail = true;
            confident  = (e.conf >= conf_max);
            break;
        }
    }


    if (!pred_avail) {
        uint64_t bidx = base_index(pc);
        const auto &be = base_table[bidx];
        if (be.conf >= 1) {
            prediction = be.value;
            pred_avail = true;
            confident  = (be.conf >= conf_max);

        }
    }

    return provider;
}

void vtage::branch_checkpoint(uint64_t branch_id, bool predicted_taken) {
    assert(branch_id < n_chkpts);
    bhr_ckpt[branch_id] = spec_bhr;
    uint64_t bit = predicted_taken ? 1ULL : 0ULL;
    spec_bhr = ((spec_bhr << 1) | bit) & hist_mask(max_hist);
}

void vtage::bhr_restore(uint64_t branch_id) {
    assert(branch_id < n_chkpts);
    spec_bhr = bhr_ckpt[branch_id];
}

void vtage::train(uint64_t pc, uint64_t actual_value, int provider, uint64_t bhr_snapshot) {

    uint64_t bidx = base_index(pc);
    auto &be = base_table[bidx];
    if (be.value == actual_value) {
        if (be.conf < conf_max) be.conf++;
    } else {
        be.value = actual_value;
        be.conf  = 0;
    }


    bool any_tagged_hit = false;
    for (uint64_t t = 0; t < num_tables; t++) {
        uint64_t idx = tagged_index_bhr(pc, t, bhr_snapshot);
        uint32_t tag = compute_tag_bhr(pc, t, bhr_snapshot);
        auto &e = tagged[t][idx];
        if (e.valid && e.tag == tag) {
            any_tagged_hit = true;
            if (e.value == actual_value) {
                if (e.conf < conf_max) e.conf++;
                if ((int)t == provider) e.useful = true;
            } else {
                e.value  = actual_value;
                e.conf   = 0;
                e.useful = false;
            }
        }
    }

    if (!any_tagged_hit) {
        for (uint64_t t = 0; t < num_tables; t++) {
            uint64_t idx = tagged_index_bhr(pc, t, bhr_snapshot);
            uint32_t tag = compute_tag_bhr(pc, t, bhr_snapshot);
            auto &e = tagged[t][idx];
            if (!e.valid || !e.useful) {
                e.valid  = true;
                e.tag    = tag;
                e.value  = actual_value;
                e.conf   = 0;
                e.useful = false;
                break;
            }
        }
    }
}

void vtage::reset() {
    spec_bhr = 0;
    for (auto &c : bhr_ckpt) c = 0;

}

uint64_t vtage::storage_bits() const {
r.
    uint64_t conf_bits = 0;
    uint64_t cm = conf_max + 1;
    while (cm > 1) { conf_bits++; cm >>= 1; }
    if (conf_bits == 0) conf_bits = 1;

    // Base table: (value=64 + conf=conf_bits) per entry.
    uint64_t base_size  = 1ULL << base_idx_bits;
    uint64_t base_bits  = base_size * (64 + conf_bits);

    // Tagged tables: (valid=1 + tag=tag_bits + value=64 + conf=conf_bits + useful=1) per entry.
    uint64_t tagged_size  = 1ULL << tagged_idx_bits;
    uint64_t entry_bits   = 1 + tag_bits + 64 + conf_bits + 1;
    uint64_t tagged_bits  = num_tables * tagged_size * entry_bits;

    return base_bits + tagged_bits;
}
