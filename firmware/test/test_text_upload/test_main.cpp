// EnvNode: see repository LICENSE.
#include <unity.h>
#include "TextUpload.h"
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp(){String::reservationLimit()=static_cast<size_t>(-1);}
void tearDown(){String::reservationLimit()=static_cast<size_t>(-1);}
void test_preallocation_handles_file_chunks_when_later_growth_is_unavailable(){
    String text;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(prepareTextUpload(text,"31722",96*1024)));
    String::reservationLimit()=0;
    std::string data(31722,'x');
    for(size_t offset=0;offset<data.size();){size_t count=data.size()-offset;if(count>2048)count=2048;
        TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(appendTextUpload(text,reinterpret_cast<const uint8_t*>(data.data()+offset),count,96*1024)));offset+=count;}
    TEST_ASSERT_EQUAL(data.size(),text.length());TEST_ASSERT_EQUAL_STRING(data.c_str(),text.c_str());
    String fresh;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::OutOfMemory),static_cast<int>(appendTextUpload(fresh,reinterpret_cast<const uint8_t*>(data.data()),2048,96*1024)));
}
void test_reservation_failure_is_reported_before_receiving_data(){
    String text;String::reservationLimit()=0;
    TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::OutOfMemory),static_cast<int>(prepareTextUpload(text,"31722",96*1024)));
    TEST_ASSERT_TRUE(text.isEmpty());
}
void test_invalid_overflowing_and_oversized_hints_are_rejected(){
    for(const char* hint:{"0","-1","12x"," 12"}){size_t size=0;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::InvalidSize),static_cast<int>(parseTextUploadSize(hint,100,size)));}
    for(const char* hint:{"101","9999999999999999999999999"}){size_t size=0;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::TooLarge),static_cast<int>(parseTextUploadSize(hint,100,size)));}
    size_t size=0;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(parseTextUploadSize("100",100,size)));TEST_ASSERT_EQUAL(100,size);
}
void test_nonterminated_chunk_does_not_require_a_read_past_its_end(){
    const uint8_t raw[3]={'a','b','c'};String text;
    TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(appendTextUpload(text,raw,sizeof(raw),100)));
    TEST_ASSERT_EQUAL_STRING("abc",text.c_str());
}
void test_size_limit_and_null_input_do_not_append_invalid_data(){
    String text="abc";const uint8_t raw[2]={'d','e'};
    TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::TooLarge),static_cast<int>(appendTextUpload(text,raw,2,4)));
    TEST_ASSERT_EQUAL_STRING("abc",text.c_str());
    TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::InvalidSize),static_cast<int>(appendTextUpload(text,nullptr,1,4)));
    TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(appendTextUpload(text,nullptr,0,4)));
}
void test_missing_hint_remains_compatible_for_legacy_clients(){
    String text;TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(prepareTextUpload(text,"",100)));
    const uint8_t raw[3]={'o','l','d'};TEST_ASSERT_EQUAL(static_cast<int>(TextUploadResult::Success),static_cast<int>(appendTextUpload(text,raw,3,100)));
    TEST_ASSERT_EQUAL_STRING("old",text.c_str());
}
int main(){UNITY_BEGIN();RUN_TEST(test_preallocation_handles_file_chunks_when_later_growth_is_unavailable);RUN_TEST(test_reservation_failure_is_reported_before_receiving_data);RUN_TEST(test_invalid_overflowing_and_oversized_hints_are_rejected);RUN_TEST(test_nonterminated_chunk_does_not_require_a_read_past_its_end);RUN_TEST(test_size_limit_and_null_input_do_not_append_invalid_data);RUN_TEST(test_missing_hint_remains_compatible_for_legacy_clients);return UNITY_END();}
