// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
namespace EnvNode {
constexpr size_t SdRotationTestBatchBytes=2048;
constexpr unsigned SdRotationTestBatches=18;
constexpr size_t SdRotationTestPartBytes=8192;
constexpr uint64_t SdRotationTestBudgetBytes=24576;
bool fillSdRotationTestBatch(unsigned index,char* data,size_t capacity);
struct SdRotationTestResult {
    bool rotation=false,pruning=false,foreignFile=false,content=false,cleanup=false;
    const char* stage="not started";
    int error=0,cleanupError=0;
    uint32_t elapsedMs=0,mountMs=0,writeMs=0,verifyMs=0,cleanupMs=0;
    uint32_t startHeap=0,startLargest=0,finishHeap=0,finishLargest=0;
    char directory[64]={};
    bool passed() const {return rotation && pruning && foreignFile && content && cleanup;}
};
class ISdRotationTestTarget {
public:
    virtual ~ISdRotationTestTarget()=default;
    virtual bool mount(SdRotationTestResult& result)=0;
    virtual bool createWorkspace(SdRotationTestResult& result)=0;
    virtual bool exercise(SdRotationTestResult& result)=0;
    virtual void inspect(SdRotationTestResult& result)=0;
    virtual int cleanup()=0;
    virtual void release()=0;
};
void runSdRotationTest(ISdRotationTestTarget& target,SdRotationTestResult& result);
class ISdRotationTestJob {
public:
    virtual ~ISdRotationTestJob()=default;
    virtual bool done() const=0;
    virtual const SdRotationTestResult& result() const=0;
};
}
