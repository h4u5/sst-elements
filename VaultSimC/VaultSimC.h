// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _VAULTSIMC_H
#define _VAULTSIMC_H

#include <sst/core/event.h>
#include <sst/core/introspectedComponent.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/output.h>

#include "vaultGlobals.h"
#include "globals.h"
#include "Vault.h"
#include "transaction.h"


using namespace std;
using namespace SST;

class VaultSimC : public IntrospectedComponent {

public: // functions

    VaultSimC( ComponentId_t id, Params& params );
    void finish();
    void init(unsigned int phase);

    Output dbg;
    
private: // types
    
    typedef SST::Link memChan_t;
    typedef multimap<uint64_t, MemHierarchy::MemEvent*> t2MEMap_t;
    
private: // functions
    
    VaultSimC (const VaultSimC& c);
    
    bool clock_phx (Cycle_t currentCycle);
    
    void readData (unsigned id, uint64_t addr, uint64_t clockcycle);
    void writeData (unsigned id, uint64_t addr, uint64_t clockcycle);
    
    std::deque<transaction_c> m_transQ;
    t2MEMap_t transactionToMemEventMap; // maps original MemEvent to a Vault transaction ID
    Vault* m_memorySystem;
    

    uint8_t *memBuffer;
    memChan_t* m_memChan;
    size_t numVaults2;  // not clear if used
    int numOutstanding; //number of mem requests outstanding (non-phx)

    unsigned vaultID;
    size_t getInternalAddress(MemHierarchy::Addr in) {
        // calculate address
        size_t lower = in & VAULT_MASK;
        size_t upper = in >> (numVaults2 + VAULT_SHIFT);
        size_t out = (upper << VAULT_SHIFT) + lower;
        return out;
    }

    // statistics
    Statistic<uint64_t>*  memOutStat;
};


#endif
