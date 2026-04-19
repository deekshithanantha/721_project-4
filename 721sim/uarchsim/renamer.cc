#include "renamer.h"
#include <vector>
#include <cassert>
#include <iostream>

#define MOD_INC(val, size) ((val + 1) % size)

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active) {
    this->n_log_regs=n_log_regs;
    this->n_phys_regs =n_phys_regs;
    this->n_branches = n_branches;
    this->n_active =n_active;


    RMT.resize(n_log_regs);

    AMT.resize(n_log_regs);
    for (uint64_t i= 0;i< n_log_regs;i++) 
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
    al_head =0;
    al_tail =0;
    al_head_phase=0;
    al_tail_phase=0;


    prf_ready.resize(n_phys_regs,false);
    prf_value.resize(n_phys_regs,0);


    for (uint64_t i=0;i<n_log_regs;i++) {
        prf_ready[i]=true;
    }

    checkpoints.resize(n_branches);
    GBM = 0;
}

renamer::~renamer() {}

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
    uint64_t free_branches=0;
    for (uint64_t i = 0; i<n_branches;i++) {
        if (!((GBM>>i)&1)) {
            free_branches++;
        }
    }
    return (free_branches<bundle_branch);
}

bool renamer::stall_dispatch(uint64_t bundle_inst) {
    uint64_t used_al;
    if (al_head_phase == al_tail_phase) 
    {
        used_al =al_tail-al_head;
    } 
    
    else {
        used_al=n_active-al_head+al_tail;
    }
    uint64_t free_al=n_active-used_al;
    return (free_al < bundle_inst);
}

uint64_t renamer::get_branch_mask() { return GBM;}

uint64_t renamer::rename_rsrc(uint64_t log_reg) { return RMT[log_reg]; }

uint64_t renamer::rename_rdst(uint64_t log_reg) {

    assert(!(fl_head == fl_tail && fl_head_phase == fl_tail_phase));
    uint64_t phys_reg = FL[fl_head];
    prf_ready[phys_reg] = false;
    fl_head = MOD_INC(fl_head, n_phys_regs);
    
    if (fl_head == 0) fl_head_phase = !fl_head_phase;

    RMT[log_reg] = phys_reg;
    return phys_reg;
}

uint64_t renamer::checkpoint() {
    int branch_id = -1;

    for (uint64_t i=0;i < (int)n_branches;i++) {
        if (!((GBM >> i) & 1)) {
            branch_id = i;
            break;
        }
    }
    assert(branch_id != -1);


    checkpoints[branch_id].rmt_snapshot = RMT;

    checkpoints[branch_id].fl_head= fl_head;
    checkpoints[branch_id].fl_head_phase =fl_head_phase;
    

    GBM |= (1ULL << branch_id);

    checkpoints[branch_id].gbm = GBM;

    return (uint64_t)branch_id;
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg,
                       bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC) {


    assert(!(al_head ==al_tail && al_head_phase != al_tail_phase));

    uint64_t index = al_tail;
    AL[index].has_dest = dest_valid;
    AL[index].log_reg =log_reg;

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
    return prf_ready[phys_reg];
}

void renamer::clear_ready(uint64_t phys_reg) {
    prf_ready[phys_reg]=false;
}

void renamer::set_ready(uint64_t phys_reg) {
    prf_ready[phys_reg]=true;
}

uint64_t renamer::read(uint64_t phys_reg) {
    return prf_value[phys_reg];
}

void renamer::write(uint64_t phys_reg, uint64_t value) {
    prf_value[phys_reg]=value;
}

void renamer::set_complete(uint64_t AL_index) {
    AL[AL_index].completed=true;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {
    if (correct) {

        GBM &= ~(1ULL << branch_ID);

        for (uint64_t i =0;i<n_branches; i++) {
             checkpoints[i].gbm &= ~(1ULL << branch_ID);
        }
    } 
    
    else {

        RMT = checkpoints[branch_ID].rmt_snapshot;
        fl_head = checkpoints[branch_ID].fl_head;
        fl_head_phase = checkpoints[branch_ID].fl_head_phase;
        GBM = checkpoints[branch_ID].gbm;


        //if (AL[AL_index].has_dest) {
        //    RMT[AL[AL_index].log_reg]=AL[AL_index].phys_reg;
        //    fl_head = MOD_INC(fl_head,n_phys_regs);
        //    if (fl_head == 0) fl_head_phase= !fl_head_phase;
        //}

        GBM &= ~(1ULL << branch_ID);


        al_tail = MOD_INC(AL_index, n_active);


        if (al_tail > al_head)
        {
             al_tail_phase = al_head_phase;
        } 
        
        else if (al_tail < al_head) 
        {
            al_tail_phase= !al_head_phase;
        } 
        
        else {
             al_tail_phase = !al_head_phase;
        }
    }
}

bool renamer::precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, 
               bool &val_misp, bool &load, bool &store, bool &branch, bool &amo, 
               bool &csr, uint64_t &offending_PC) {

    if (al_head==al_tail && al_head_phase == al_tail_phase) {
        return false; 
    }

    AL_Entry &head_inst = AL[al_head];
    completed=head_inst.completed;
    exception=head_inst.exception;
    load_viol=head_inst.load_violation;
    br_misp = head_inst.branch_misprediction;
    val_misp = head_inst.value_misprediction;
    load = head_inst.is_load;
    store =head_inst.is_store;
    branch= head_inst.is_branch;
    amo=head_inst.is_amo;
    csr=head_inst.is_csr;
    offending_PC = head_inst.PC;

    return true; 
}

void renamer::commit() {

    assert(!(al_head==al_tail && al_head_phase==al_tail_phase));

    AL_Entry &head_inst = AL[al_head];

    assert(head_inst.completed && !head_inst.exception && !head_inst.load_violation);

    if (head_inst.has_dest) {
        uint64_t old_phys_reg=AMT[head_inst.log_reg];
        AMT[head_inst.log_reg]=head_inst.phys_reg;


        FL[fl_tail]=old_phys_reg;
        fl_tail=MOD_INC(fl_tail, n_phys_regs);
        if (fl_tail == 0)fl_tail_phase = !fl_tail_phase;
    }

    al_head = MOD_INC(al_head,n_active);
    if (al_head == 0)al_head_phase = !al_head_phase;
}

void renamer::squash() {
    RMT =AMT;
    fl_head=0;
    fl_tail=0;
    fl_head_phase = 0;
    fl_tail_phase = 0;

    std::vector<bool> is_in_amt(n_phys_regs, false);


    for (uint64_t i =0; i<n_log_regs;i++) {

        is_in_amt[AMT[i]] = true;
    }


    for (uint64_t i = 0;i<n_phys_regs;i++) {

        if (!is_in_amt[i]) {
            FL[fl_tail] = i;
            fl_tail = MOD_INC(fl_tail, n_phys_regs);
            if (fl_tail == 0) fl_tail_phase = !fl_tail_phase;
        }
    }
    
    al_head=0;
    al_tail=0;
    al_head_phase= 0;
    al_tail_phase =0;
    GBM = 0;


    std::fill(prf_ready.begin(), prf_ready.end(), false);


    for (uint64_t i = 0; i < n_log_regs; i++) 
    {
        prf_ready[AMT[i]]=true;
    }
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