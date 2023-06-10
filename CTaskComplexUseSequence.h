/*
    From gta-reversed-modern
*/

#pragma once

#include "CTaskComplex.h"

class CTaskComplexUseSequence : public CTaskComplex {
protected:
    CTaskComplexUseSequence(plugin::dummy_func_t a) : CTaskComplex(a) {}

public:
    int32_t m_nSequenceIndex;         // Used in CTaskSequences::ms_taskSequence global array
    int32_t m_nCurrentTaskIndex;      // Used in CTaskComplexSequence::m_aTasks array
    int32_t m_nEndTaskIndex;          // Sequence will stop performing tasks when current index is equal to endTaskIndex
    int32_t m_nSequenceRepeatedCount; // m_nSequenceRepeatedCount simply tells us how many times the sequence has been repeated.
                                    // If CTaskComplexSequence::m_bRepeatSequence is true, this can be greater than 1,
                                    // otherwise it's set to 1 when the sequence is done executing tasks.

public:
    //static constexpr auto Type = TASK_COMPLEX_USE_SEQUENCE;

    explicit CTaskComplexUseSequence(int32_t sequenceIndex);
};
VALIDATE_SIZE(CTaskComplexUseSequence, 0x1C);
