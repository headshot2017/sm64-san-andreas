/*
    From gta-reversed-modern
*/

#pragma once

#include "CTaskComplex.h"

class CTaskComplexSequence2 : public CTaskComplex {
protected:
    CTaskComplexSequence2(plugin::dummy_func_t a) : CTaskComplex(a) {}

public:
    int32_t  m_nCurrentTaskIndex;      // Used in m_aTasks
    CTask*   m_aTasks[8];
    bool     m_bRepeatSequence;        // Sequence will loop if set to 1
    int32_t  m_nSequenceRepeatedCount; // m_nSequenceRepeatedCount simply tells us how many times the sequence has been>
                                       // If m_bRepeatSequence is true, this can be greater than 1,
                                       // otherwise it's set to 1 when the sequence is done executing tasks.
    bool     m_bFlushTasks;
    uint32_t m_nReferenceCount; // count of how many CTaskComplexUseSequence instances are using this sequence

public:
    //static constexpr auto Type = TASK_COMPLEX_SEQUENCE;

    CTaskComplexSequence2();
};
VALIDATE_SIZE(CTaskComplexSequence2, 0x40);
