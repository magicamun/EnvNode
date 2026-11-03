// EnvNode: see repository LICENSE.
#include "SdRotationTest.h"
#include <cstdio>
#include <cstring>
namespace EnvNode {
void runSdRotationTest(ISdRotationTestTarget& target,SdRotationTestResult& result) {
    result.stage="mount";
    if(target.mount(result)) {
        if(target.createWorkspace(result) && target.exercise(result)) target.inspect(result);
        result.cleanupError=target.cleanup();
        result.cleanup=result.cleanupError==0;
        if(!result.cleanup && !result.error) {result.stage="cleanup";result.error=result.cleanupError;}
    } else result.cleanup=true; // No workspace was created.
    target.release(); // Exactly once, after cleanup, including mount failure.
    if(result.passed()) result.stage="complete";
}
bool fillSdRotationTestBatch(unsigned index,char* data,size_t capacity) {
    if(!data || capacity<SdRotationTestBatchBytes || index>=SdRotationTestBatches) return false;
    memset(data,' ',SdRotationTestBatchBytes);
    for(unsigned row=0;row<32;++row) {
        char text[64];const int size=snprintf(text,sizeof(text),"{\"rotation_test_batch\":%u,\"row\":%u}",index,row);
        if(size<=0 || size>=63) return false;
        memcpy(data+row*64,text,size);data[row*64+63]='\n';
    }
    return true;
}
}
