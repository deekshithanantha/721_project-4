#include "valpred.h"
#include "vtage.h"
#include "parameters.h"
#include <cassert>
#include <cstring>

#define MOD_INC(val, size) ((val + 1) % size)

valpred::valpred(uint64_t n_branches) : vt(nullptr),
    last_bhr_snapshot(0), last_vtage_provider(-1), n_branches(n_branches) {

    // Initialize Stride Value Predictor (SVP)
    if (valpred_Stride_selector) {
        SVP_num_entries = (1ULL << stride_valpred_index);
        if (SVP_num_entries == 0) SVP_num_entries = 1;
    } else {
        SVP_num_entries = 1;
    }

    SVP.resize(SVP_num_entries);
    for (uint64_t i = 0; i < SVP_num_entries; i++) {
        SVP[i].valid = false;
        SVP[i].tag = 0;
        SVP[i].conf = 0;
        SVP[i].retired_value = 0;
        SVP[i].stride = 0;
        SVP[i].instance = 0;
    }

    // Initialize Value Prediction Queue (VPQ)
    VPQ_size = (valpred_SIZE ? valpred_SIZE : 1);
    VPQ.resize(VPQ_size);
    for (uint64_t i = 0; i < VPQ_size; i++) {
        VPQ[i].valid        = false;
        VPQ[i].pc           = 0;
        VPQ[i].value        = 0;
        VPQ[i].value_valid  = false;
        VPQ[i].bhr_snapshot = 0;
        VPQ[i].vtage_provider = -1;
    }

    VPQ_head = 0;
    VPQ_tail = 0;
    VPQ_head_phase = 0;
    VPQ_tail_phase = 0;

    // Initialize VTAGE if selected.
    if (valpred_VTAGE_selector) {
        vt = new vtage(n_branches,
                       vtage_num_tables,
                       vtage_base_idx_bits,
                       vtage_tagged_idx_bits,
                       vtage_min_hist_len,
                       vtage_tag_bits,
                       vtage_conf_max);
    }
}

valpred::~valpred() {
    if (vt) delete vt;
}

bool valpred::stall_vp(uint64_t bundle_valpred) {
    if (!valpred_Stride_selector && !valpred_VTAGE_selector) return false;

    uint64_t used_vpq;
    if (VPQ_head_phase == VPQ_tail_phase) {
        used_vpq = VPQ_tail - VPQ_head;
    } else {
        used_vpq = VPQ_size - VPQ_head + VPQ_tail;
    }

    uint64_t free_vpq = VPQ_size - used_vpq;
    return (free_vpq < bundle_valpred);
}

void valpred::set_checkpoint_vpq_tail(uint64_t vpq_tail, bool vpq_tail_phase) {
    // Storage is handled by renamer's checkpoint structure.
}

void valpred::get_checkpoint_vpq_tail(uint64_t &vpq_tail, bool &vpq_tail_phase) const {
    vpq_tail = VPQ_tail;
    vpq_tail_phase = VPQ_tail_phase;
}

uint64_t valpred::svp_index(uint64_t pc) const {
    return ((pc >> 2) & (SVP_num_entries - 1));
}

uint64_t valpred::svp_tag(uint64_t pc) const {
    if (stride_valpred_tag == 0) return 0;

    uint64_t raw = (pc >> (2 + stride_valpred_index));
    uint64_t full_bits = 62 - stride_valpred_index;

    if (stride_valpred_tag >= full_bits) {
        return raw;
    }

    uint64_t mask = (1ULL << stride_valpred_tag) - 1ULL;
    return (raw & mask);
}

bool valpred::svp_hit(uint64_t pc, uint64_t &idx) const {
    idx = svp_index(pc);
    if (stride_valpred_tag == 0) return true;
    if (!SVP[idx].valid) return false;
    return (SVP[idx].tag == svp_tag(pc));
}

void valpred::vpq_checkpoint() {
    // VPQ tail is returned via get_checkpoint() — no action needed here.
}

void valpred::vp_predict(uint64_t pc, bool &pred_avail, bool &confident, uint64_t &prediction) {
    pred_avail = false;
    confident  = false;
    prediction = 0;
    last_bhr_snapshot  = 0;
    last_vtage_provider = -1;

    if (valpred_VTAGE_selector) {
        last_bhr_snapshot = vt->get_spec_bhr();
        last_vtage_provider = vt->predict(pc, pred_avail, confident, prediction);
        return;
    }

    if (!valpred_Stride_selector) return;

    uint64_t idx;
    if (!svp_hit(pc, idx)) return;

    SVP[idx].instance++;

    int64_t pred = (int64_t)SVP[idx].retired_value + ((int64_t)SVP[idx].instance * SVP[idx].stride);

    prediction = (uint64_t) pred;
    pred_avail = true;
    confident = (SVP[idx].conf == stride_MAX_confidencce);
}

uint64_t valpred::vpq_alloc(uint64_t pc) {
    assert(valpred_Stride_selector || valpred_VTAGE_selector);
    assert(!stall_vp(1));

    uint64_t idx = VPQ_tail;

    VPQ[idx].valid          = true;
    VPQ[idx].pc             = pc;
    VPQ[idx].value          = 0;
    VPQ[idx].value_valid    = false;
    VPQ[idx].bhr_snapshot   = last_bhr_snapshot;
    VPQ[idx].vtage_provider = last_vtage_provider;

    VPQ_tail = MOD_INC(VPQ_tail, VPQ_size);
    if (VPQ_tail == 0) VPQ_tail_phase = !VPQ_tail_phase;

    return idx;
}

void valpred::valpred_sitter(uint64_t idx, uint64_t value) {
    assert(idx < VPQ_size);
    assert(VPQ[idx].valid);

    VPQ[idx].value       = value;
    VPQ[idx].value_valid = true;
}

uint64_t valpred::vpq_count_pc(uint64_t pc) const {
    uint64_t count = 0;
    uint64_t idx = VPQ_head;
    bool phase = VPQ_head_phase;
    uint64_t target_index = svp_index(pc);
    uint64_t target_tag = svp_tag(pc);

    while (!((idx == VPQ_tail) && (phase == VPQ_tail_phase))) {
        if (VPQ[idx].valid) {
            if (svp_index(VPQ[idx].pc) == target_index) {
                if (stride_valpred_tag == 0 || svp_tag(VPQ[idx].pc) == target_tag) {
                    count++;
                }
            }
        }
        idx++;
        if (idx >= VPQ_size) {
            idx = 0;
            phase ^= 1;
        }
    }
    return count;
}

void valpred::vp_retire(uint64_t idx) {
    assert(valpred_Stride_selector || valpred_VTAGE_selector);
    assert(!(VPQ_head == VPQ_tail && VPQ_head_phase == VPQ_tail_phase));
    assert(VPQ_head == idx);
    assert(VPQ[VPQ_head].valid);
    assert(VPQ[VPQ_head].value_valid);

    uint64_t pc          = VPQ[VPQ_head].pc;
    uint64_t value       = VPQ[VPQ_head].value;
    uint64_t bhr_snap    = VPQ[VPQ_head].bhr_snapshot;
    int      provider    = VPQ[VPQ_head].vtage_provider;

    VPQ[VPQ_head].valid       = false;
    VPQ[VPQ_head].value_valid = false;

    VPQ_head = MOD_INC(VPQ_head, VPQ_size);
    if (VPQ_head == 0) VPQ_head_phase = !VPQ_head_phase;

    if (valpred_VTAGE_selector) {
        vt->train(pc, value, provider, bhr_snap);
        return;
    }

    // SVP training path.
    uint64_t svp_idx = svp_index(pc);
    bool hit = svp_hit(pc, svp_idx);

    if (hit) {
        SVP[svp_idx].valid = true;
        SVP[svp_idx].tag = svp_tag(pc);

        int64_t new_stride = (int64_t) value - (int64_t) SVP[svp_idx].retired_value;

        if (new_stride == SVP[svp_idx].stride) {
            if (SVP[svp_idx].conf < stride_MAX_confidencce)
                SVP[svp_idx].conf++;
        } else {
            SVP[svp_idx].stride = new_stride;
            SVP[svp_idx].conf = 0;
        }

        SVP[svp_idx].retired_value = value;
        assert(SVP[svp_idx].instance > 0);
        SVP[svp_idx].instance--;
    } else {
        SVP[svp_idx].valid = true;
        SVP[svp_idx].tag = svp_tag(pc);
        SVP[svp_idx].conf = 0;
        SVP[svp_idx].retired_value = value;
        SVP[svp_idx].stride = (int64_t) value;
        SVP[svp_idx].instance = vpq_count_pc(pc);
    }
}

void valpred::vpq_restore_tail(uint64_t tail, bool tail_phase) {
    if (!valpred_Stride_selector && !valpred_VTAGE_selector) return;

    if (valpred_Stride_selector) {
        // SVP: walk discarded entries and repair instance counters.
        uint64_t idx = tail;
        bool phase = tail_phase;

        while (!((idx == VPQ_tail) && (phase == VPQ_tail_phase))) {
            if (VPQ[idx].valid) {
                uint64_t svp_idx = 0;
                if (svp_hit(VPQ[idx].pc, svp_idx) && SVP[svp_idx].instance > 0) {
                    SVP[svp_idx].instance--;
                }
                VPQ[idx].valid = false;
                VPQ[idx].value_valid = false;
            }
            idx++;
            if (idx >= VPQ_size) {
                idx = 0;
                phase ^= 1;
            }
        }
    } else {
        // VTAGE: just invalidate discarded VPQ entries.
        uint64_t idx = tail;
        bool phase = tail_phase;

        while (!((idx == VPQ_tail) && (phase == VPQ_tail_phase))) {
            VPQ[idx].valid      = false;
            VPQ[idx].value_valid = false;
            idx++;
            if (idx >= VPQ_size) {
                idx = 0;
                phase ^= 1;
            }
        }
    }

    VPQ_tail = tail;
    VPQ_tail_phase = tail_phase;
}

void valpred::reset() {
    if (valpred_Stride_selector || valpred_VTAGE_selector) {
        VPQ_head = 0;
        VPQ_tail = 0;
        VPQ_head_phase = 0;
        VPQ_tail_phase = 0;

        for (uint64_t i = 0; i < VPQ_size; i++) {
            VPQ[i].valid       = false;
            VPQ[i].value_valid = false;
        }
    }

    if (valpred_Stride_selector) {
        for (uint64_t i = 0; i < SVP_num_entries; i++) {
            SVP[i].instance = 0;
        }
    }

    if (valpred_VTAGE_selector && vt) {
        vt->reset();
    }
}

void valpred::vtage_branch_checkpoint(uint64_t branch_id, bool predicted_taken) {
    if (vt) vt->branch_checkpoint(branch_id, predicted_taken);
}

void valpred::vtage_bhr_restore(uint64_t branch_id) {
    if (vt) vt->bhr_restore(branch_id);
}

valpred::VP_Checkpoint valpred::get_checkpoint() const {
    VP_Checkpoint checkpoint;
    checkpoint.vpq_tail = VPQ_tail;
    checkpoint.vpq_tail_phase = VPQ_tail_phase;
    return checkpoint;
}
