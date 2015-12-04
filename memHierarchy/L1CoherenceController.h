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

#ifndef L1COHERENCECONTROLLER_H
#define L1COHERENCECONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class L1CoherenceController : public CoherencyController {
public:
    /** Constructor for L1CoherenceController */
    L1CoherenceController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, vector<Link*>* childLinks, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool groupStats, vector<int> statGroupIds, bool debugAll, Addr debugAddr, bool snoopL1Invs) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLinks, bottomNetworkLink, topNetworkLink, listener, mshr, 
                         wbClean, groupStats, statGroupIds, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
        snoopL1Invs_        = snoopL1Invs;
        protocol_           = protocol;
    }

    ~L1CoherenceController() {}
    
    /** Init funciton */
    void init(const char* name){}
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, uint32_t groupId, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherencyController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * origRequest, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);


    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    void sendResponseUp(MemEvent * event, State grantedState, vector<uint8_t>* data, bool replay, bool atomic = false);

    void printData(vector<uint8_t> * data, bool set);

    /** Print statistics at the end of simulation */
    void printStats(int statsFile, vector<int> statGroupIds, map<int, CtrlStats> _ctrlStats, uint64_t upgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    
private:
    bool                protocol_;  // True for MESI, false for MSI
    bool                snoopL1Invs_;

    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle Inv request */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay);

    /** Handle data response - GetSResp or GetXResp */
    void handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);

    
    /* Methods for sending events */
    /** Send response memEvent to lower level caches */
    void sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool replay);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);

    /** Send AckInv response to lower level caches */
    void sendAckInv(Addr baseAddr, string origRqstr);

    /* Helper methods */

    /* Stat counting and callbacks to prefetcher */
    void inc_GETXMissSM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissSM->addData(1);
            else statGetSExMissSM->addData(1);
            stats_[0].GETXMissSM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissSM_++;
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
			event->getVirtualAddress(), event->getInstructionPointer(),
			event->getSize(), WRITE, MISS);

        	listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GETXMissIM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissIM->addData(1);
            else statGetSExMissIM->addData(1);
            stats_[0].GETXMissIM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissIM_++;
        }

        if(!(event->isPrefetch())) {
		// probably should be 'if (prefetching is on and not a prefetch miss)'
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GETSHit(MemEvent* event){
        if(!event->statsUpdated()){
            if(!event->inMSHR()){
                stats_[0].GETSHit_++;
                if(groupStats_) stats_[getGroupId()].GETSHit_++;
            }
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GETXHit(MemEvent* event){
        if(!event->statsUpdated()){
            if(!event->inMSHR()){
                stats_[0].GETXHit_++;
                if(groupStats_) stats_[getGroupId()].GETXHit_++;
            }
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GETSMissIS(MemEvent* event){
        if(!event->statsUpdated()){
            statGetSMissIS->addData(1);
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GetSExReqsReceived(bool replay){
        if(!replay) stats_[0].GetSExReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].GetSExReqsReceived_++;
    }

    void inc_InvalidatePUTSReqSent(){
        stats_[0].InvalidatePUTSReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTSReqSent_++;
    }

    void inc_EvictionPUTSReqSent(){
        stats_[0].EvictionPUTSReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTSReqSent_++;
    }

    void inc_EvictionPUTMReqSent(){
        stats_[0].EvictionPUTMReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTMReqSent_++;
    }


    void inc_EvictionPUTEReqSent(){
        stats_[0].EvictionPUTEReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTEReqSent_++;
    }

    void inc_FetchInvReqSent(){
        stats_[0].FetchInvReqSent_++;
        if(groupStats_) stats_[getGroupId()].FetchInvReqSent_++;
    }

    void inc_FetchInvXReqSent(){
        stats_[0].FetchInvXReqSent_++;
        if(groupStats_) stats_[getGroupId()].FetchInvXReqSent_++;
    }

};


}}


#endif

