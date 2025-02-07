#pragma once

#ifndef ART_EPOCHE_H
#define ART_EPOCHE_H

#include <atomic>
#include <array>
#include <vector>
#include "tbb/enumerable_thread_specific.h"
#include "tbb/combinable.h"

namespace ART{

    constexpr uint32_t NUMBER_EPOCHS = 3;
    constexpr uint32_t NOT_IN_EPOCH = NUMBER_EPOCHS;

    class ThreadSpecificEpochBasedReclamationInformation {
        std::array <std::vector<void *>, NUMBER_EPOCHS> mFreeLists;
        std::atomic <uint32_t> mLocalEpoch;
        uint32_t mPreviouslyAccessedEpoch;
        bool mThreadWantsToAdvance;

    public:
        static std::atomic <size_t> mNumberFrees;

        ThreadSpecificEpochBasedReclamationInformation()
                : mFreeLists(), mLocalEpoch(NOT_IN_EPOCH), mPreviouslyAccessedEpoch(NOT_IN_EPOCH),
                  mThreadWantsToAdvance(false) {
        }

        ThreadSpecificEpochBasedReclamationInformation(
                ThreadSpecificEpochBasedReclamationInformation const &other) = delete;

        ThreadSpecificEpochBasedReclamationInformation(ThreadSpecificEpochBasedReclamationInformation &&other) = delete;

        ~ThreadSpecificEpochBasedReclamationInformation() {
            for (uint32_t i = 0; i < 3; ++i) {
                freeForEpoch(i);
            }
        }

        void scheduleForDeletion(void *childPointer) {
            assert(mLocalEpoch != NOT_IN_EPOCH);
            std::vector<void *> &currentFreeList = mFreeLists[mLocalEpoch];
            currentFreeList.emplace_back(childPointer);
            mThreadWantsToAdvance = (currentFreeList.size() % 64u) == 0;
        }

        uint32_t getLocalEpoch() const {
            return mLocalEpoch.load(std::memory_order_acquire);
        }

        void enter(uint32_t newEpoch) {
            assert(mLocalEpoch == NOT_IN_EPOCH);
            if (mPreviouslyAccessedEpoch != newEpoch) {
                freeForEpoch(newEpoch);
                mThreadWantsToAdvance = false;
                mPreviouslyAccessedEpoch = newEpoch;
            }
            mLocalEpoch.store(newEpoch, std::memory_order_release);
        }

        void leave() {
            mLocalEpoch.store(NOT_IN_EPOCH, std::memory_order_release);
        }

        bool doesThreadWantToAdvanceEpoch() {
            return (mThreadWantsToAdvance);
        }

    private:
        void freeForEpoch(uint32_t epoch) {
            std::vector<void *> &previousFreeList = mFreeLists[epoch];
            for (void *pointer : previousFreeList) {
                delete pointer;
            }
            previousFreeList.resize(0u);
        }
    };

    std::atomic <size_t> ThreadSpecificEpochBasedReclamationInformation::mNumberFrees{0};

    class EpochBasedMemoryReclamationStrategy {
    public:
        static uint32_t NEXT_EPOCH[3];
        static uint32_t PREVIOUS_EPOCH[3];

        std::atomic <uint32_t> mCurrentEpoch;
        tbb::enumerable_thread_specific <ThreadSpecificEpochBasedReclamationInformation, tbb::cache_aligned_allocator<ThreadSpecificEpochBasedReclamationInformation>, tbb::ets_key_per_instance> mThreadSpecificInformations;

    private:
        EpochBasedMemoryReclamationStrategy() : mCurrentEpoch(0), mThreadSpecificInformations() {
        }

    public:

        static EpochBasedMemoryReclamationStrategy *getInstance() {
            static EpochBasedMemoryReclamationStrategy instance;
            return &instance;
        }

        void enterCriticalSection() {
            ThreadSpecificEpochBasedReclamationInformation &currentMemoryInformation = mThreadSpecificInformations.local();
            uint32_t currentEpoch = mCurrentEpoch.load(std::memory_order_acquire);
            currentMemoryInformation.enter(currentEpoch);
            if (currentMemoryInformation.doesThreadWantToAdvanceEpoch() && canAdvance(currentEpoch)) {
                mCurrentEpoch.compare_exchange_strong(currentEpoch, NEXT_EPOCH[currentEpoch]);
            }
        }

        bool canAdvance(uint32_t currentEpoch) {
            uint32_t previousEpoch = PREVIOUS_EPOCH[currentEpoch];
            return !std::any_of(mThreadSpecificInformations.begin(), mThreadSpecificInformations.end(), [previousEpoch](
                    ThreadSpecificEpochBasedReclamationInformation const &threadInformation) {
                return (threadInformation.getLocalEpoch() == previousEpoch);
            });
        }

        void leaveCriticialSection() {
            ThreadSpecificEpochBasedReclamationInformation &currentMemoryInformation = mThreadSpecificInformations.local();
            currentMemoryInformation.leave();
        }

        void scheduleForDeletion(void *childPointer) {
            mThreadSpecificInformations.local().scheduleForDeletion(childPointer);
        }
    };

    uint32_t EpochBasedMemoryReclamationStrategy::NEXT_EPOCH[3] = {1, 2, 0};
    uint32_t EpochBasedMemoryReclamationStrategy::PREVIOUS_EPOCH[3] = {2, 0, 1};

    class Epoche;

    class EpocheGuard;

    class ThreadInfo {
        friend class Epoche;

        friend class EpocheGuard;

        Epoche &epoche;

    public:

        ThreadInfo(Epoche &epoche) : epoche(epoche) {}

        ThreadInfo(const ThreadInfo &ti) : epoche(ti.epoche) {}

        Epoche &getEpoche() const {
            return epoche;
        }
    };

    class Epoche {
        friend class ThreadInfo;

        EpochBasedMemoryReclamationStrategy *mMemoryReclamation;


    public:
        Epoche(size_t startGCThreshhold) {
            mMemoryReclamation = EpochBasedMemoryReclamationStrategy::getInstance();
            (mMemoryReclamation->mThreadSpecificInformations).clear();
        }

        ~Epoche() {
        }

        void enterEpoche(ThreadInfo &epocheInfo) {
            mMemoryReclamation->enterCriticalSection();
        }

        void markNodeForDeletion(void *n, ThreadInfo &epocheInfo) {
            mMemoryReclamation->scheduleForDeletion(n);
        }

        void exitEpocheAndCleanup(ThreadInfo &info) {
            mMemoryReclamation->leaveCriticialSection();
        }

        void showDeleteRatio() {}

    };

    class EpocheGuard {
        ThreadInfo &threadEpocheInfo;
    public:

        EpocheGuard(ThreadInfo &threadEpocheInfo) : threadEpocheInfo(threadEpocheInfo) {
            threadEpocheInfo.getEpoche().enterEpoche(threadEpocheInfo);
        }


        ~EpocheGuard() {
            threadEpocheInfo.getEpoche().exitEpocheAndCleanup(threadEpocheInfo);
        }

    };

    class EpocheGuardReadonly {
    public:
        ThreadInfo &threadEpocheInfo;

        EpocheGuardReadonly(ThreadInfo &threadEpocheInfo) : threadEpocheInfo(threadEpocheInfo) {
            threadEpocheInfo.getEpoche().enterEpoche(threadEpocheInfo);
        }

        ~EpocheGuardReadonly() {
            threadEpocheInfo.getEpoche().exitEpocheAndCleanup(threadEpocheInfo);
        }
    };

}

#endif //ART_EPOCHE_H
