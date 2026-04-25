#include "renamer.h"
#include "parameters.h" // Added by teammate for valpred_Stride_selector and other macros
#include <vector>
#include <cassert>
#include <iostream>

#define MOD_INC(val, size) ((val + 1) % size)

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active) {
    this->n_log_regs=n_log_regs;
    this->n_phys_regs =n_phys_regs;
    this->n_branches = n_branches;
    this->n_active =n_active;

    assert(n_phys_regs > n_log_regs);
    assert(n_branches >= 1 && n_branches <= 64);
    assert(n_active > 0);

    RMT.resize(n_log_regs);
    AMT.resize(n_log_regs);
    for (uint64_t i= 0; i< n_log_regs;i++) 
	{
        RMT[i]=i;
        AMT[i]=i;
    }


    FL.resize(n_phys_regs); 
    fl_head =0;
    fl_tail =0;
    fl_head_phase=0;
    fl_tail_phase=0;


	for (uint64_t i = n_log_regs; i<n_phys_regs;i++) {
        FL[fl_tail]=i;
        fl_tail=MOD_INC(fl_tail,n_phys_regs);
        if (fl_tail== 0)fl_tail_phase = !fl_tail_phase;
    }


    AL.resize(n_active);
    for (uint64_t i = 0; i < n_active; i++) {
        AL[i].has_dest = false;
        AL[i].log_reg = 0;
        AL[i].phys_reg = 0;
        AL[i].completed = false;
        AL[i].exception = false;
        AL[i].load_violation = false;
        AL[i].branch_misprediction = false;
        AL[i].value_misprediction = false;
        AL[i].is_load = false;
        AL[i].is_store = false;
        AL[i].is_branch = false;
        AL[i].is_amo = false;
        AL[i].is_csr = false;
        AL[i].PC = 0;
    }

    al_head =0;
    al_tail =0;
    al_head_phase=0;
    al_tail_phase=0;

    prf_ready.resize(n_phys_regs,false);
    prf_value.resize(n_phys_regs,0);

    for (uint64_t i=0;i<n_log_regs;i++) {
        prf_ready[i]=true;
    }


    if (n_branches == 64) 
        valid_branch_mask = ~0ULL;
    else 
        valid_branch_mask = (1ULL << n_branches) - 1ULL;


    checkpoints.resize(n_branches);
    GBM = 0;

    for (uint64_t i = 0; i < n_branches; i++) {
        checkpoints[i].rmt_snapshot.resize(n_log_regs, 0);
        checkpoints[i].fl_head = 0;
        checkpoints[i].fl_head_phase = 0;
        checkpoints[i].gbm = 0;
        checkpoints[i].vpq_tail = 0;
        checkpoints[i].vpq_tail_phase = 0;
    }

    // Initialize Value Prediction Engine
    vp = new valpred(n_branches);
}

renamer::~renamer() {
    if (vp) delete vp;
}

bool renamer::stall_reg(uint64_t bundle_dst) {
    uint64_t free_regs;
    if (fl_head_phase==fl_tail_phase) {
        free_regs=fl_tail-fl_head;
    } 
	else {
        free_regs =n_phys_regs -fl_head+fl_tail;
    }
    return (free_regs<bundle_dst);
}

bool renamer::stall_branch(uint64_t bundle_branch) {
    uint64_t inuse = GBM & valid_branch_mask;
    uint64_t used = __builtin_popcountll(inuse); // Teammate's popcount efficiency
    uint64_t free_branches = n_branches - used;
    return (free_branches < bundle_branch);
}

uint64_t renamer::get_branch_mask() { 
    return (GBM & valid_branch_mask); 
}

uint64_t renamer::rename_rsrc(uint64_t log_reg) { 
    assert(log_reg < n_log_regs);
    return RMT[log_reg]; 
}

uint64_t renamer::rename_rdst(uint64_t log_reg) {
    assert(log_reg < n_log_regs);
    assert(!(fl_head == fl_tail && fl_head_phase == fl_tail_phase)); // Reverted from teammate's helper function
    
    uint64_t phys_reg = FL[fl_head];
    prf_ready[phys_reg] = false;
    
    fl_head = MOD_INC(fl_head, n_phys_regs);
    if (fl_head == 0) fl_head_phase = !fl_head_phase;

    RMT[log_reg] = phys_reg;
    return phys_reg;
}

uint64_t renamer::checkpoint() {
    uint64_t inuse = GBM & valid_branch_mask;
    uint64_t branch_id = 0;
    bool found = false;

    for (uint64_t i =0; i < n_branches; i++) {
        if (((inuse >> i) & 1ULL) == 0ULL) {
            branch_id = i;
            found = true;
            break;
        }
    }
    assert(found);

    checkpoints[branch_id].gbm = GBM;
    checkpoints[branch_id].rmt_snapshot = RMT;
    checkpoints[branch_id].fl_head = fl_head;
    checkpoints[branch_id].fl_head_phase = fl_head_phase;
    
    GBM |= (1ULL << branch_id);
    GBM &= valid_branch_mask;

    return branch_id;
}

void renamer::set_checkpoint_vpq_tail(uint64_t branch_ID, uint64_t tail, bool tail_phase) {
    assert(branch_ID < n_branches);
    checkpoints[branch_ID].vpq_tail = tail;
    checkpoints[branch_ID].vpq_tail_phase = tail_phase;
}

void renamer::get_checkpoint_vpq_tail(uint64_t branch_ID, uint64_t &tail, bool &tail_phase) const {
    assert(branch_ID < n_branches);
    tail = checkpoints[branch_ID].vpq_tail;
    tail_phase = checkpoints[branch_ID].vpq_tail_phase;
}

bool renamer::stall_vp(uint64_t bundle_valpred) {
    return vp->stall_vp(bundle_valpred);
}

void renamer::vpq_checkpoint(uint64_t branch_ID) {
    assert(branch_ID < n_branches);
    valpred::VP_Checkpoint checkpoint = vp->get_checkpoint();
    checkpoints[branch_ID].vpq_tail = checkpoint.vpq_tail;
    checkpoints[branch_ID].vpq_tail_phase = checkpoint.vpq_tail_phase;
}

void renamer::vp_predict(uint64_t pc, bool &pred_avail, bool &confident, uint64_t &prediction) {
    vp->vp_predict(pc, pred_avail, confident, prediction);
}

uint64_t renamer::vpq_alloc(uint64_t pc) {
    return vp->vpq_alloc(pc);
}

void renamer::valpred_sitter(uint64_t idx, uint64_t value) {
    vp->valpred_sitter(idx, value);
}

void renamer::vp_retire(uint64_t idx) {
    vp->vp_retire(idx);
}

void renamer::vpq_restore_tail(uint64_t tail, bool tail_phase) {
    vp->vpq_restore_tail(tail, tail_phase);
}

bool renamer::stall_dispatch(uint64_t bundle_inst) {
    uint64_t used_al;
    if (al_head_phase == al_tail_phase) {
        used_al = al_tail - al_head;
    } else {
        used_al = n_active - al_head + al_tail;
    }
    uint64_t free_al = n_active - used_al;
    return (free_al < bundle_inst);
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg,
                                bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC) {

    assert(!(al_head == al_tail && al_head_phase != al_tail_phase));

    uint64_t index = al_tail;

    AL[index].has_dest = dest_valid;
    AL[index].log_reg = log_reg;

    AL[index].phys_reg = phys_reg;
    AL[index].completed= false;
    AL[index].exception= false;
    AL[index].load_violation= false;
    AL[index].branch_misprediction=false;
    AL[index].value_misprediction=false;
    AL[index].is_load =load;
    AL[index].is_store=store;
    AL[index].is_branch=branch;
    AL[index].is_amo =amo;
    AL[index].is_csr =csr;
    AL[index].PC =PC;

    al_tail = MOD_INC(al_tail, n_active);
    if (al_tail == 0) al_tail_phase = !al_tail_phase;

    return index;
}

bool renamer::is_ready(uint64_t phys_reg) {
    assert(phys_reg < n_phys_regs);
    return prf_ready[phys_reg];
}

void renamer::clear_ready(uint64_t phys_reg) {
    assert(phys_reg < n_phys_regs);
    prf_ready[phys_reg] = false;
}

void renamer::set_ready(uint64_t phys_reg) {
    assert(phys_reg < n_phys_regs);
    prf_ready[phys_reg] = true;
}

uint64_t renamer::read(uint64_t phys_reg) {
    assert(phys_reg < n_phys_regs);
    return prf_value[phys_reg];
}

void renamer::write(uint64_t phys_reg, uint64_t value) {
    assert(phys_reg < n_phys_regs);
    prf_value[phys_reg] = value;
}

void renamer::set_complete(uint64_t AL_index) {
    assert(AL_index < n_active);
    AL[AL_index].completed = true;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {
    assert(branch_ID < n_branches);
    assert(AL_index < n_active);

    uint64_t bit = (1ULL << branch_ID);

    if (correct) {
        GBM &= ~bit;
        for (uint64_t i = 0; i < n_branches; i++) {
            checkpoints[i].gbm &= ~bit;
        }
        GBM &= valid_branch_mask;
        return;
    }

    // Misprediction Recovery
    GBM = checkpoints[branch_ID].gbm; 
    GBM &= ~bit;
    GBM &= valid_branch_mask;

    RMT = checkpoints[branch_ID].rmt_snapshot;
    fl_head = checkpoints[branch_ID].fl_head;
    fl_head_phase = checkpoints[branch_ID].fl_head_phase;

    // Teammate's reliable AL phase recovery logic adapted to your variables
    uint64_t new_tail_idx = MOD_INC(AL_index, n_active);
    bool branch_phase = (AL_index >= al_head) ? al_head_phase : !al_head_phase;
    bool new_tail_phase = (AL_index == (n_active - 1)) ? !branch_phase : branch_phase;

    al_tail = new_tail_idx;
    al_tail_phase = new_tail_phase;

    if (valpred_Stride_selector) {
        vpq_restore_tail(checkpoints[branch_ID].vpq_tail, checkpoints[branch_ID].vpq_tail_phase);
    }

    assert((GBM & bit) == 0ULL);
    assert(!(al_head == al_tail && al_head_phase == al_tail_phase));
}

bool renamer::precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, 
                        bool &val_misp, bool &load, bool &store, bool &branch, bool &amo, 
                        bool &csr, uint64_t &offending_PC) {

    if (al_head == al_tail && al_head_phase == al_tail_phase) {
        return false; 
    }

    AL_Entry &head_inst = AL[al_head];

    completed = head_inst.completed;
    exception = head_inst.exception;
    load_viol = head_inst.load_violation;
    br_misp = head_inst.branch_misprediction;
    val_misp = head_inst.value_misprediction;
    load = head_inst.is_load;
    store = head_inst.is_store;
    branch = head_inst.is_branch;
    amo = head_inst.is_amo;
    csr = head_inst.is_csr;
    offending_PC = head_inst.PC;

    return true; 
}

void renamer::commit() {
    assert(!(al_head == al_tail && al_head_phase == al_tail_phase));

    AL_Entry &head_inst = AL[al_head];

    assert(head_inst.completed && !head_inst.exception && !head_inst.load_violation);

    if (head_inst.has_dest) {
        uint64_t old_phys_reg = AMT[head_inst.log_reg];
        AMT[head_inst.log_reg] = head_inst.phys_reg;

        // Expanded teammate's free_list_push using your original structure
        FL[fl_tail] = old_phys_reg;
        fl_tail = MOD_INC(fl_tail, n_phys_regs);
        if (fl_tail == 0) fl_tail_phase = !fl_tail_phase;
    }

    al_head = MOD_INC(al_head, n_active);
    if (al_head == 0) al_head_phase = !al_head_phase;
}

void renamer::squash() {
    GBM = 0;
    RMT = AMT;
    
    al_head = 0;
    al_tail = 0;
    al_head_phase = 0;
    al_tail_phase = 0;
    
    fl_head = 0;
    fl_tail = 0;
    fl_head_phase = 0;
    fl_tail_phase = 0;

    // Using your original Free List reconstruction block
    std::vector<bool> is_in_amt(n_phys_regs, false);
    for (uint64_t i = 0; i < n_log_regs; i++) {
        is_in_amt[AMT[i]] = true;
    }

    for (uint64_t i = 0; i < n_phys_regs; i++) {
        if (!is_in_amt[i]) {
            FL[fl_tail] = i;
            fl_tail = MOD_INC(fl_tail, n_phys_regs);
            if (fl_tail == 0) fl_tail_phase = !fl_tail_phase;
        }
    }

    std::fill(prf_ready.begin(), prf_ready.end(), false);
    for (uint64_t i = 0; i < n_log_regs; i++) 
	{
        prf_ready[AMT[i]]=true;
    }

    // Reset Value Prediction Engine
    vp->reset();
}

void renamer::set_exception(uint64_t AL_index) 
{ 
    AL[AL_index].exception = true; 
}


void renamer::set_load_violation(uint64_t AL_index) 
{ 
    AL[AL_index].load_violation = true; 
}


void renamer::set_branch_misprediction(uint64_t AL_index) 
{ 
    AL[AL_index].branch_misprediction = true; 
}

void renamer::set_value_misprediction(uint64_t AL_index) 
{ 
    AL[AL_index].value_misprediction = true; 
}

bool renamer::get_exception(uint64_t AL_index) 
{ 
    return AL[AL_index].exception;
}