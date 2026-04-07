#include <3ds.h>
#include <CTRPluginFramework.hpp>

#include "external/rt.h"

#include "Megamix.hpp"
#include "Config.hpp"

using CTRPluginFramework::OSD;

using Megamix::TempoTable;

// c++ is stupid
namespace GHooks = Megamix::Game::Hooks;

namespace Megamix::Hooks {
    RT_HOOK tickflowHook;
    RT_HOOK gateHook;
    RT_HOOK gatePracHook;

    RT_HOOK tempoStrmHook;
    RT_HOOK tempoSeqHook;
    RT_HOOK tempoAllHook;

    RT_HOOK regionFSHook;
    RT_HOOK regionOtherHook;

    RT_HOOK tickflowCommandsHook;

    RT_HOOK scoringFunctionHook;


    void* getTickflowOffset(int index) {
        if (config->tickflows.contains(index)) {
            int result = Megamix::btks.LoadFile(config->tickflows[index]);
            if (!result) {
                return (void*)(Megamix::btks.start);
            } else {
                OSD::Notify(CTRPluginFramework::Utils::Format("Error: %s", Megamix::ErrorMessage(result).c_str()));
            }
        }
        return Game::gGameTable()[index].tfStart;  // og code
    }

    void* getGateTickflowOffset(int index) {
        if (config->tickflows.contains(index + 0x100)) {
            int result = Megamix::btks.LoadFile(config->tickflows[index + 0x100]);
            if (!result) {
                return (void*)(Megamix::btks.start);
            } else {
                OSD::Notify(CTRPluginFramework::Utils::Format("Error: %s", Megamix::ErrorMessage(result).c_str()));    
            }
        }
        return Game::gGateTable()[index].tfStart; // og code
    }

    void* getGatePracticeTickflowOffset(int index) {
        if (config->tickflows.contains((index >> 2) + 0x110)) {
            int result = Megamix::btks.LoadFile(config->tickflows[(index >> 2) + 0x110]);
            if (!result) {
                return (void*)(Megamix::btks.start);
            } else {
                OSD::Notify(CTRPluginFramework::Utils::Format("Error: %s", Megamix::ErrorMessage(result).c_str()));    
            }
        }
        return Game::gGateTable()[index].tfGatePractice; // og code
    }

    TempoTable* getTempoStrm(Megamix::CSoundManager* this_, u32 id) {
        if (Megamix::btks.tempos.contains(id)) {
            return Megamix::btks.tempos[id];
        } else { // Original code
            for (s32 low = 0, high = this_->numberTempos; low <= high;) {
                s32 current_num = (low + high) / 2;
                Megamix::SM_TempoTable* current = &this_->tableStrm[current_num];
                if (current->id > id) high = current_num - 1;
                if (current->id < id) low = current_num + 1;
                if (current->id == id) return current->tempo;
            }
            return 0;
        }
    }

    TempoTable* getTempoSeq(Megamix::CSoundManager* this_, u32 id) {
        if (Megamix::btks.tempos.contains(id)) {
            return Megamix::btks.tempos[id];
        } else { // Original code
            for (s32 low = 0, high = this_->numberTempos; low <= high;) {
                s32 current_num = (low + high) / 2;
                Megamix::SM_TempoTable* current = &this_->tableSeq[current_num];
                if (current->id > id) high = current_num - 1;
                if (current->id < id) low = current_num + 1;
                if (current->id == id) return current->tempo;
            }
            return 0;
        }
    }

    TempoTable* getTempoAll(Megamix::CSoundManager* this_, u32 id) {
        if (Megamix::btks.tempos.contains(id)) {
            return Megamix::btks.tempos[id];
        } else { // Original code
            for (int i = 0; i < this_->numberTempos; i++) {
                Megamix::TempoTable* current = &this_->tempoTable[i];
                if (current->id1 == id || current->id2 == id)
                    return current;
            }
            return 0;
        }
    }

    Game::RegionSDK getRegionCTR() {
        using namespace Game;
        //TODO: handle JP region / JP langpack (?)

        if (isJP())
            return RegionSDK::JP;
        else if (isUS())
            return RegionSDK::US;
        else if (isEU())
            return RegionSDK::EU;
        else if (isKR())
            return RegionSDK::KR;
        else
            return RegionSDK::UNK;
    }

    Game::RegionMegamix getRegionMegamix() {
        using namespace Game;
        //TODO: handle JP region / JP langpack (?)

        if (isJP())
            return RegionMegamix::JP;
        else if (isUS())
            return RegionMegamix::US;
        else if (isEU())
            return RegionMegamix::EU;
        else if (isKR())
            return RegionMegamix::KR;
        else
            return RegionMegamix::UNK;
    }

    int decideFinalScore(Megamix::CResultManager *arg1){
        //pointers
        u8 amtCategories = arg1->mAmtCategories;
        //OSD::Notify("amtCategories = " + std::to_string(amtCategories));
        u32* field6 = arg1->field6_0xc;
        u32* field7 = arg1->field7_0x10;
        u32* field8 = arg1->field8_0x14;
        long long catScore10000 = 0LL;
        float catScore10000float = 0.0f;
        float totalWeight = 0.0f;
        float scoreDivWeight = 0.0f;
        float amtInputs = 0.0f;
        float result = 0.0f;
        int finalResult = 0;
        float curScoreWeight = 0.0f;
        int i = 0;
        int g = 0;
        int validCategories = 0;
        float distributedWeight = 0.0f;
        float distributedAmtInputs = 0.0f;
        float distributedScore = 0.0f;

        if (0 != amtCategories){
            //OSD::Notify("amt categories loop entered!");
            do{
                amtInputs = (int)(field6[g] + field7[g] + field8[g]);
                if(amtInputs != 0){
                    validCategories += 1;
                }
                g += 1;
            }while (g < 7);

            distributedWeight = (float)(arg1->mScoreWeight[7]) / (float)(validCategories);
            distributedAmtInputs = (float)(field6[7] + field7[7] + field8[7]) / (float)(validCategories);
            if(arg1->points[7] < 1){
                distributedScore = (float)(arg1->points[7] + 1) / (float)(validCategories);
            } else {
                distributedScore = (float)(arg1->points[7]) / (float)(validCategories);
            }

            //OSD::Notify("distributedWeight = " + std::to_string(distributedWeight));
            //OSD::Notify("distributedAmtInputs = " + std::to_string(distributedAmtInputs));
            //OSD::Notify("distributedScore = " + std::to_string(distributedScore));

            do{
                if(i != 7){
                    amtInputs = (float)(field6[i] + field7[i] + field8[i]);

                    if (0 < amtInputs) {
                        if((float)(field6[i] + field7[i] + field8[i]) < 1){
                            amtInputs = 0;
                        }
                        else{
                            amtInputs += distributedAmtInputs;
                            //OSD::Notify("cat score = " + std::to_string(amtInputs));
                            if(arg1->points[i] < 1){
                                catScore10000 = ((arg1->points[i] + distributedScore + 1) * 10000) / (amtInputs * arg1->mMaxWeight[i]);
                                //OSD::Notify("points = " + std::to_string(arg1->points[i]));
                            } else {
                                catScore10000 = ((arg1->points[i] + distributedScore) * 10000) / (amtInputs * arg1->mMaxWeight[i]);
                                //OSD::Notify("catScore10000 = " + std::to_string(catScore10000));
                            }
                            //OSD::Notify("points = " + std::to_string(arg1->points[i]));

                            //OSD::Notify("mMaxWeight = " + std::to_string(arg1->mMaxWeight[i]));
                            catScore10000float = (float)catScore10000;
                            //OSD::Notify("catScore10000float = " + std::to_string(catScore10000float));
                        }
                        curScoreWeight = (float)(arg1->mScoreWeight[i]) + distributedWeight;
                        //OSD::Notify("curScoreWeight = " + std::to_string(curScoreWeight));
                        totalWeight += curScoreWeight;
                        //OSD::Notify("totalWeight = " + std::to_string(totalWeight));
                        scoreDivWeight += curScoreWeight/catScore10000float;
                        //OSD::Notify("scoreDivWeight= " + std::to_string(scoreDivWeight));
                    }
                }
                i += 1;
            } while (i < amtCategories);
            if (0.0f < totalWeight){
                result = totalWeight/scoreDivWeight;
                //OSD::Notify("result = " + std::to_string(result));
                finalResult = (int)(result - (arg1->field9_0x18 * arg1->field14_0x2c));
                if(10000 < finalResult){
                    finalResult = 10000;
                }
                //OSD::Notify("finalResult = " + std::to_string(finalResult));
                if (0 < finalResult){
                    return finalResult;
                }
            }
        }
        //OSD::Notify("finalResult = " + std::to_string(finalResult));
        return 0;
    }

    void TickflowHooks() {
        rtInitHook(&tickflowHook, GHooks::tickflow(), (u32)getTickflowOffset);
        rtEnableHook(&tickflowHook);
        rtInitHook(&gateHook, GHooks::gate(), (u32)getGateTickflowOffset);
        rtEnableHook(&gateHook);
        rtInitHook(&gatePracHook, GHooks::gatePractice(), (u32)getGatePracticeTickflowOffset);
        rtEnableHook(&gatePracHook);
    }

    void TempoHooks() {
        rtInitHook(&tempoStrmHook, GHooks::strmTempo(), (u32)getTempoStrm);
        rtEnableHook(&tempoStrmHook);
        rtInitHook(&tempoSeqHook, GHooks::seqTempo(), (u32)getTempoSeq);
        rtEnableHook(&tempoSeqHook);
        rtInitHook(&tempoAllHook, GHooks::allTempo(), (u32)getTempoAll);
        rtEnableHook(&tempoAllHook);
    }

    void RegionHooks() {
        if (!Megamix::isJP()){
            rtInitHook(&regionFSHook, GHooks::megamixRegionCode(), (u32)getRegionMegamix);
            rtEnableHook(&regionFSHook);
        }
        rtInitHook(&regionOtherHook, GHooks::sdkRegionCode(), (u32)getRegionCTR);
        rtEnableHook(&regionOtherHook);
    }

    void CommandHook() {
        rtInitHook(&tickflowCommandsHook, Game::hTickflowCmds::hook(), (u32)tickflowCommandsHookWrapper);
        rtEnableHook(&tickflowCommandsHook);
    }

    void ScoringHook(){
        rtInitHook(&scoringFunctionHook, GHooks::scoring(), (int)decideFinalScore);
        rtEnableHook(&scoringFunctionHook);
    }

    void DisableAllHooks() {
        rtDisableHook(&tickflowHook);
        rtDisableHook(&gateHook);
        rtDisableHook(&tempoStrmHook);
        rtDisableHook(&tempoSeqHook);
        rtDisableHook(&tempoAllHook);
        rtDisableHook(&regionFSHook);
        rtDisableHook(&regionOtherHook);
        rtDisableHook(&scoringFunctionHook);
        rtDisableHook(&tickflowCommandsHook);
    }



    template<typename T>
    T StubbedFunction() {
        return {};
    }

    template<>
    void StubbedFunction<void>() {
    }

    // redirects function at address to a stubbed function
    template<typename T>
    void StubFunction(u32 address) {
        rtGenerateJumpCode(
            (u32)   StubbedFunction<T>,
            (u32 *) address
        );
    }

    // template instantiations
    // if either StubbedFunction or StubFunction is used with a type, a template
    // instantiation must be added with that type
    template void StubFunction<void>(u32);

}
