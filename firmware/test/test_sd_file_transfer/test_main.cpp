// EnvNode: see repository LICENSE.
#include <unity.h>
#include <string>
#include <cstring>
#include <algorithm>
#include "SdFileTransfer.h"
#include "SdCardWebView.h"
void pinMode(unsigned char,int) {} void digitalWrite(unsigned char,int) {}
using namespace EnvNode;
namespace {
struct File : ISdStoredFile {
    std::string data="abcdef0123456789";size_t offset=0;unsigned reads=0;uint64_t reported=16;
    SdFileStamp stamp() const override {return SdFileStamp(reported,42,77);}
    int read(uint8_t* out,size_t size) override {++reads;size=std::min(size,data.size()-offset);memcpy(out,data.data()+offset,size);offset+=size;return size;}
    bool remove() override {return false;}
};
struct Sink : ISdDownloadSink {
    std::string result; bool online=true,busy=false,fail=false;size_t limit=3;
    bool connected() override {return online;}
    int send(const uint8_t* in,size_t size) override {if(fail)return -1;if(busy)return 0;size=std::min(size,limit);result.append(reinterpret_cast<const char*>(in),size);return size;}
};
void test_partial_stream_preserves_bytes_and_does_one_chunk_per_step() {
    File f;Sink s;SdDownloadPump pump(f,s,0);SdTransferState state=SdTransferState::More;
    for(unsigned i=0;i<20 && state==SdTransferState::More;++i) {state=pump.step(i);TEST_ASSERT_LESS_OR_EQUAL_UINT((i+1)*3,s.result.size());}
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdTransferState::Complete),static_cast<int>(state));
    TEST_ASSERT_EQUAL_STRING(f.data.c_str(),s.result.c_str());TEST_ASSERT_EQUAL_UINT(1,f.reads);
}
void test_multiple_blocks_and_binary_data_are_preserved() {
    File f;Sink s;f.data.resize(4097);f.reported=f.data.size();s.limit=333;
    for(size_t i=0;i<f.data.size();++i) f.data[i]=static_cast<char>(i%256);
    SdDownloadPump pump(f,s,0);SdTransferState state=SdTransferState::More;
    for(unsigned i=0;i<100 && state==SdTransferState::More;++i) state=pump.step(i);
    TEST_ASSERT_EQUAL_INT(1,static_cast<int>(state));TEST_ASSERT_EQUAL_UINT(5,f.reads);
    TEST_ASSERT_EQUAL_UINT(f.data.size(),s.result.size());
    TEST_ASSERT_EQUAL_MEMORY(f.data.data(),s.result.data(),f.data.size());
}
void test_busy_socket_retains_buffer_and_times_out() {
    File f;Sink s;s.busy=true;SdDownloadPump pump(f,s,0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdTransferState::More),static_cast<int>(pump.step(1)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdTransferState::More),static_cast<int>(pump.step(9999)));
    TEST_ASSERT_EQUAL_UINT(1,f.reads);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdTransferState::TimedOut),static_cast<int>(pump.step(10000)));
}
void test_disconnect_truncation_and_send_failure_abort() {
    {File f;Sink s;s.online=false;SdDownloadPump p(f,s,0);TEST_ASSERT_EQUAL_INT(2,static_cast<int>(p.step(1)));TEST_ASSERT_EQUAL_UINT(0,f.reads);}
    {File f;Sink s;s.limit=100;f.reported=17;SdDownloadPump p(f,s,0);TEST_ASSERT_EQUAL_INT(0,static_cast<int>(p.step(1)));TEST_ASSERT_EQUAL_INT(2,static_cast<int>(p.step(2)));}
    {File f;Sink s;s.fail=true;SdDownloadPump p(f,s,0);TEST_ASSERT_EQUAL_INT(2,static_cast<int>(p.step(1)));}
}
void test_zero_length_and_clock_wrap() {
    File f;Sink s;f.reported=0;SdDownloadPump empty(f,s,0);TEST_ASSERT_EQUAL_INT(1,static_cast<int>(empty.step(0)));
    f.reported=16;s.busy=true;SdDownloadPump p(f,s,0xFFFFFF00U);
    TEST_ASSERT_EQUAL_INT(0,static_cast<int>(p.step(100)));
    TEST_ASSERT_EQUAL_INT(3,static_cast<int>(p.step(10000)));
}
const char* token="0123456789abcdef0123456789abcdef";
void test_delete_ticket_is_bound_one_use_and_expires() {
    SdDeleteTicket ticket;SdFileStamp expected,stamp(12,4,99);
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,stamp,100));
    TEST_ASSERT_TRUE(ticket.consume("/test.txt",token,101,expected));
    TEST_ASSERT_TRUE(sameSdFileStamp(stamp,expected));TEST_ASSERT_FALSE(ticket.consume("/test.txt",token,102,expected));
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,stamp,100));
    TEST_ASSERT_FALSE(ticket.consume("/other.txt",token,101,expected));
    TEST_ASSERT_FALSE(ticket.consume("/test.txt",token,102,expected));
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,stamp,100));
    TEST_ASSERT_FALSE(ticket.consume("/test.txt","wrong",101,expected));
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,stamp,100));
    TEST_ASSERT_FALSE(ticket.consume("/test.txt",token,120100,expected));
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,stamp,100));ticket.clear();
    TEST_ASSERT_FALSE(ticket.consume("/test.txt",token,101,expected));
}
void test_fat_descriptor_without_timestamp_uses_path_timestamp() {
    const SdFileMetadata descriptor(262144,0,true);
    const SdFileMetadata path(262144,1793048400,true);
    SdFileStamp preview,reopened;
    TEST_ASSERT_TRUE(captureSdFileStamp(descriptor,path,99,preview));
    TEST_ASSERT_EQUAL_INT64(path.modified,preview.modified);
    TEST_ASSERT_TRUE(captureSdFileStamp(descriptor,path,99,reopened));
    TEST_ASSERT_TRUE(sameSdFileStamp(preview,reopened));
    const SdFileStamp finalPathStat(path.bytes,path.modified,99);
    TEST_ASSERT_TRUE(sameSdFileStamp(preview,finalPathStat));
    SdDeleteTicket ticket;SdFileStamp expected;
    TEST_ASSERT_TRUE(ticket.issue("/test.txt",token,preview,100));
    TEST_ASSERT_TRUE(ticket.consume("/test.txt",token,101,expected));
    TEST_ASSERT_TRUE(sameSdFileStamp(expected,finalPathStat));
}
void test_inconsistent_or_nonregular_metadata_is_rejected() {
    SdFileStamp output;
    TEST_ASSERT_FALSE(captureSdFileStamp(SdFileMetadata(1,0,true),SdFileMetadata(2,5,true),99,output));
    TEST_ASSERT_FALSE(captureSdFileStamp(SdFileMetadata(1,0,true),SdFileMetadata(1,5,false),99,output));
    TEST_ASSERT_FALSE(captureSdFileStamp(SdFileMetadata(1,0,false),SdFileMetadata(1,5,true),99,output));
    TEST_ASSERT_FALSE(captureSdFileStamp(SdFileMetadata(1,0,true),SdFileMetadata(1,5,true),0,output));
}
void test_changed_file_or_media_is_rejected() {
    TEST_ASSERT_TRUE(sameSdFileStamp(SdFileStamp(12,4,99),SdFileStamp(12,4,99)));
    TEST_ASSERT_FALSE(sameSdFileStamp(SdFileStamp(12,4,99),SdFileStamp(13,4,99)));
    TEST_ASSERT_FALSE(sameSdFileStamp(SdFileStamp(12,4,99),SdFileStamp(12,5,99)));
    TEST_ASSERT_FALSE(sameSdFileStamp(SdFileStamp(12,4,99),SdFileStamp(12,4,98)));
    TEST_ASSERT_FALSE(sameSdFileStamp(SdFileStamp(12,4,0),SdFileStamp(12,4,0)));
}
void test_delete_form_escapes_path_and_uses_post() {
    const String html=renderSdDeleteConfirmation("/a'&<x>.txt",SdFileStamp(123,4,99),token,Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"method='post'"));TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"123 Bytes"));
    TEST_ASSERT_NULL(strstr(html.c_str(),"<x>"));TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"&lt;x&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),token));
    TEST_ASSERT_EQUAL_STRING("a%22%0D%0A.txt",sdEncodeComponent("a\"\r\n.txt").c_str());
    const String disposition=sdDownloadDisposition("/folder/a\"\r\n.txt");
    TEST_ASSERT_NULL(strchr(disposition.c_str(),'\r'));TEST_ASSERT_NULL(strchr(disposition.c_str(),'\n'));
    TEST_ASSERT_NOT_NULL(strstr(disposition.c_str(),"filename=\"a___.txt\""));
    TEST_ASSERT_NOT_NULL(strstr(disposition.c_str(),"filename*=UTF-8''a%22%0D%0A.txt"));
}
}
void setUp(){} void tearDown(){}
int main(){UNITY_BEGIN();
RUN_TEST(test_partial_stream_preserves_bytes_and_does_one_chunk_per_step);
RUN_TEST(test_multiple_blocks_and_binary_data_are_preserved);
RUN_TEST(test_busy_socket_retains_buffer_and_times_out);
RUN_TEST(test_disconnect_truncation_and_send_failure_abort);
RUN_TEST(test_zero_length_and_clock_wrap);
RUN_TEST(test_delete_ticket_is_bound_one_use_and_expires);
RUN_TEST(test_fat_descriptor_without_timestamp_uses_path_timestamp);
RUN_TEST(test_inconsistent_or_nonregular_metadata_is_rejected);
RUN_TEST(test_changed_file_or_media_is_rejected);
RUN_TEST(test_delete_form_escapes_path_and_uses_post);
return UNITY_END();}
