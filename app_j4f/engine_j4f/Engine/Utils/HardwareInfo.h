#pragma once

// https://gist.github.com/9prady9/a5e1e8bdbc9dc58b3349

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>

#ifdef j4f_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(j4f_PLATFORM_LINUX)
#include <unistd.h>
#endif

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;
#endif

namespace engine {
    class CPUID {
        uint32_t regs[4];

    public:
        explicit CPUID(unsigned funcId, unsigned subFuncId) {
#ifdef _WIN32
            __cpuidex((int *)regs, (int)funcId, (int)subFuncId);

#else
            asm volatile
                    ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
                    : "a" (funcId), "c" (subFuncId));
            // ECX is set to zero for CPUID function 4
#endif
        }

        const uint32_t &EAX() const { return regs[0]; }

        const uint32_t &EBX() const { return regs[1]; }

        const uint32_t &ECX() const { return regs[2]; }

        const uint32_t &EDX() const { return regs[3]; }
    };

    class CPUInfo {
    public:
        CPUInfo() {
            // Get vendor name EAX=0
            CPUID cpuID0(0, 0);
            uint32_t HFS = cpuID0.EAX();
            mVendorId += std::string((const char *) &cpuID0.EBX(), 4);
            mVendorId += std::string((const char *) &cpuID0.EDX(), 4);
            mVendorId += std::string((const char *) &cpuID0.ECX(), 4);
            // Get SSE instructions availability
            CPUID cpuID1(1, 0);
            mIsHTT = cpuID1.EDX() & AVX_POS;
            mIsSSE = cpuID1.EDX() & SSE_POS;
            mIsSSE2 = cpuID1.EDX() & SSE2_POS;
            mIsSSE3 = cpuID1.ECX() & SSE3_POS;
            mIsSSE41 = cpuID1.ECX() & SSE41_POS;
            mIsSSE42 = cpuID1.ECX() & SSE41_POS;
            mIsAVX = cpuID1.ECX() & AVX_POS;
            // Get AVX2 instructions availability
            CPUID cpuID7(7, 0);
            mIsAVX2 = cpuID7.EBX() & AVX2_POS;

            std::string upVId = mVendorId;
            std::for_each(upVId.begin(), upVId.end(), [](char &in) { in = ::toupper(in); });
            // Get num of cores
            if (upVId.find("INTEL") != std::string::npos) {
                if (HFS >= 11) {
                    constexpr uint8_t max_intel_top_lvl = 4;

                    for (uint8_t lvl = 0; lvl < max_intel_top_lvl; ++lvl) {
                        CPUID cpuID4(0x0B, lvl);
                        uint32_t currLevel = (LVL_TYPE & cpuID4.ECX()) >> 8;
                        switch (currLevel) {
                            case 0x01:
                                mNumSMT = LVL_CORES & cpuID4.EBX();
                                break;
                            case 0x02:
                                mNumLogCpus = LVL_CORES & cpuID4.EBX();
                                break;
                            default:
                                break;
                        }
                    }
                    mNumCores = mNumLogCpus / mNumSMT;
                } else {
                    if (HFS >= 1) {
                        mNumLogCpus = (cpuID1.EBX() >> 16) & 0xFF;
                        if (HFS >= 4) {
                            mNumCores = 1 + (CPUID(4, 0).EAX() >> 26) & 0x3F;
                        }
                    }
                    if (mIsHTT) {
                        if (!(mNumCores > 1)) {
                            mNumCores = 1;
                            mNumLogCpus = (mNumLogCpus >= 2 ? mNumLogCpus : 2);
                        }
                    } else {
                        mNumCores = mNumLogCpus = 1;
                    }
                }
            } else if (upVId.find("AMD") != std::string::npos) {
                if (HFS >= 1) {
                    mNumLogCpus = (cpuID1.EBX() >> 16) & 0xFF;
                    if (CPUID(0x80000000, 0).EAX() >= 8) {
                        mNumCores = 1 + (CPUID(0x80000008, 0).ECX() & 0xFF);
                    }
                }
                if (mIsHTT) {
                    if (!(mNumCores > 1)) {
                        mNumCores = 1;
                        mNumLogCpus = (mNumLogCpus >= 2 ? mNumLogCpus : 2);
                    }
                } else {
                    mNumCores = mNumLogCpus = 1;
                }
            } else {
                // "Unexpected vendor id"
                assert(false);
            }
            // Get processor brand string
            // This seems to be working for both Intel & AMD vendors
            for (int i = 0x80000002; i < 0x80000005; ++i) {
                CPUID cpuID(i, 0);
                mModelName += std::string((const char *) &cpuID.EAX(), 4);
                mModelName += std::string((const char *) &cpuID.EBX(), 4);
                mModelName += std::string((const char *) &cpuID.ECX(), 4);
                mModelName += std::string((const char *) &cpuID.EDX(), 4);
            }

            int8_t i = mModelName.length() - 1;
            for (; i > 0 && (mModelName[i] == ' ' || mModelName[i] == '\0'); --i) { }
            mModelName = mModelName.substr(0, i);
        }

        std::string vendor() const noexcept { return mVendorId; }

        std::string model() const noexcept { return mModelName; }

        int cores() const noexcept { return mNumCores; }

        float cpuSpeedInMHz() const noexcept { return mCPUMHz; }

        bool isSSE() const noexcept { return mIsSSE; }

        bool isSSE2() const noexcept { return mIsSSE2; }

        bool isSSE3() const noexcept { return mIsSSE3; }

        bool isSSE41() const noexcept { return mIsSSE41; }

        bool isSSE42() const noexcept { return mIsSSE42; }

        bool isAVX() const noexcept { return mIsAVX; }

        bool isAVX2() const noexcept { return mIsAVX2; }

        bool isHyperThreaded() const noexcept { return mIsHTT; }

        int logicalCpus() const noexcept { return mNumLogCpus; }

    private:
        // Bit positions for data extractions
        inline static constexpr uint32_t SSE_POS = 0x02000000;
        inline static constexpr uint32_t SSE2_POS = 0x04000000;
        inline static constexpr uint32_t SSE3_POS = 0x00000001;
        inline static constexpr uint32_t SSE41_POS = 0x00080000;
        inline static constexpr uint32_t SSE42_POS = 0x00100000;
        inline static constexpr uint32_t AVX_POS = 0x10000000;
        inline static constexpr uint32_t AVX2_POS = 0x00000020;
        inline static constexpr uint32_t LVL_NUM = 0x000000FF;
        inline static constexpr uint32_t LVL_TYPE = 0x0000FF00;
        inline static constexpr uint32_t LVL_CORES = 0x0000FFFF;

        // Attributes
        std::string mVendorId;
        std::string mModelName;
        int mNumSMT;
        int mNumCores;
        int mNumLogCpus;
        float mCPUMHz;
        bool mIsHTT;
        bool mIsSSE;
        bool mIsSSE2;
        bool mIsSSE3;
        bool mIsSSE41;
        bool mIsSSE42;
        bool mIsAVX;
        bool mIsAVX2;
    };

#ifdef j4f_PLATFORM_WINDOWS
    unsigned long long getTotalSystemMemory() {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullTotalPhys;
    }
#elif defined(j4f_PLATFORM_LINUX)
    unsigned long long getTotalSystemMemory() {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        return pages * page_size;
    }
#endif
    // mem usage in linux
    // https://www.tutorialspoint.com/how-to-get-memory-usage-at-runtime-using-cplusplus
}