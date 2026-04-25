#ifndef VALPRED_H
#define VALPRED_H

#include <inttypes.h>
#include <vector>
#include <cassert>

class valpred {
private:
    /////////////////////////////////////////////////////////////////////
    // Value Prediction Data Structures
    /////////////////////////////////////////////////////////////////////
    struct SVP_Entry {
        bool valid;
        uint64_t tag;
        uint64_t conf;
        uint64_t retired_value;
        int64_t stride;
        uint64_t instance;
    };

    struct VPQ_Entry {
        bool valid;
        uint64_t pc;
        uint64_t value;
        bool value_valid;
    };

    /////////////////////////////////////////////////////////////////////
    // Stride Value Predictor (SVP)
    /////////////////////////////////////////////////////////////////////
    std::vector<SVP_Entry> SVP;
    uint64_t SVP_num_entries;

    /////////////////////////////////////////////////////////////////////
    // Value Prediction Queue (VPQ)
    /////////////////////////////////////////////////////////////////////
    std::vector<VPQ_Entry> VPQ;
    uint64_t VPQ_size;
    uint64_t VPQ_head;
    uint64_t VPQ_tail;
    bool VPQ_head_phase;
    bool VPQ_tail_phase;

    /////////////////////////////////////////////////////////////////////
    // Configuration variables
    /////////////////////////////////////////////////////////////////////
    uint64_t n_branches;

    /////////////////////////////////////////////////////////////////////
    // Private helper functions
    /////////////////////////////////////////////////////////////////////
    uint64_t svp_index(uint64_t pc) const;
    uint64_t svp_tag(uint64_t pc) const;
    bool svp_hit(uint64_t pc, uint64_t &idx) const;
    uint64_t vpq_count_pc(uint64_t pc) const;

public:
    /////////////////////////////////////////////////////////////////////
    // Constructor and Destructor
    /////////////////////////////////////////////////////////////////////
    valpred(uint64_t n_branches);
    ~valpred();

    /////////////////////////////////////////////////////////////////////
    // Public Value Prediction Functions
    /////////////////////////////////////////////////////////////////////

    /**
     * Check if VPQ has space for new value predictions
     * @param bundle_valpred Number of entries needed
     * @return true if stall is needed (not enough space)
     */
    bool stall_vp(uint64_t bundle_valpred);

    /**
     * Save VPQ tail state in checkpoint for branch recovery
     * @param vpq_tail Tail index to save
     * @param vpq_tail_phase Tail phase bit to save
     */
    void set_checkpoint_vpq_tail(uint64_t vpq_tail, bool vpq_tail_phase);

    /**
     * Retrieve VPQ tail state from checkpoint
     * @param vpq_tail Reference to save tail index
     * @param vpq_tail_phase Reference to save tail phase bit
     */
    void get_checkpoint_vpq_tail(uint64_t &vpq_tail, bool &vpq_tail_phase) const;

    /**
     * Checkpoint current VPQ tail for branch recovery
     */
    void vpq_checkpoint();

    /**
     * Get value prediction for an instruction
     * @param pc Program counter of instruction
     * @param pred_avail Output: whether prediction is available
     * @param confident Output: whether prediction is confident
     * @param prediction Output: predicted value
     */
    void vp_predict(uint64_t pc, bool &pred_avail, bool &confident, uint64_t &prediction);

    /**
     * Allocate entry in VPQ for a value prediction
     * @param pc Program counter of instruction
     * @return Index into VPQ
     */
    uint64_t vpq_alloc(uint64_t pc);

    /**
     * Set the actual value for a predicted instruction
     * @param idx Index into VPQ (from vpq_alloc)
     * @param value Actual computed value
     */
    void valpred_sitter(uint64_t idx, uint64_t value);

    /**
     * Process retirement of predicted instruction value
     * @param idx Index into VPQ
     */
    void vp_retire(uint64_t idx);

    /**
     * Restore VPQ state after branch misprediction
     * @param tail New tail index to restore
     * @param tail_phase New tail phase bit to restore
     */
    void vpq_restore_tail(uint64_t tail, bool tail_phase);

    /**
     * Reset VPQ and SVP state (used during squash)
     */
    void reset();

    /**
     * Store checkpoint data for branch recovery
     */
    struct VP_Checkpoint {
        uint64_t vpq_tail;
        bool vpq_tail_phase;
    };

    /**
     * Get current checkpoint data
     */
    VP_Checkpoint get_checkpoint() const;
};

#endif // VALPRED_H
