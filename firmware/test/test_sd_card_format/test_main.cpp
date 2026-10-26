// EnvNode: see repository LICENSE.
#include <unity.h>
#include "SdCardFormat.h"
#include "SdCardWebView.h"
#include <cstring>
void pinMode(unsigned char,int) {}
void digitalWrite(unsigned char,int) {}
using namespace EnvNode;
namespace {
const char* token="0123456789abcdef0123456789abcdef";
const SdFileStamp card(7948206080ULL,0,1234);
class Device : public ISdFormatDevice {
public:
    SdFileStamp current=card;
    bool readable=true,formatted=true,verified=true;
    unsigned inspections=0,formats=0,verifications=0;
    bool inspect(SdFileStamp& output) override {++inspections;output=current;return readable;}
    bool format() override {++formats;return formatted;}
    bool verify() override {++verifications;return verified;}
};
void test_capacity_boundaries() {
    TEST_ASSERT_FALSE(supportedSdFormatCapacity(0));
    TEST_ASSERT_FALSE(supportedSdFormatCapacity(2000000000ULL));
    TEST_ASSERT_TRUE(supportedSdFormatCapacity(3948206080ULL));
    TEST_ASSERT_TRUE(supportedSdFormatCapacity(32010928128ULL));
    TEST_ASSERT_FALSE(supportedSdFormatCapacity(64000000000ULL));
}
void test_confirmation_is_explicit_one_use_and_time_limited() {
    SdFormatTicket ticket;SdFileStamp expected;
    TEST_ASSERT_TRUE(ticket.issue(token,card,10));
    TEST_ASSERT_FALSE(ticket.consume(token,"formatieren",11,expected));
    TEST_ASSERT_FALSE(ticket.consume(token,"FORMATIEREN",12,expected));
    TEST_ASSERT_TRUE(ticket.issue(token,card,10));
    TEST_ASSERT_FALSE(ticket.consume("wrong","FORMATIEREN",11,expected));
    TEST_ASSERT_TRUE(ticket.issue(token,card,10));
    TEST_ASSERT_FALSE(ticket.consume(token,"FORMATIEREN",120010,expected));
    TEST_ASSERT_TRUE(ticket.issue(token,card,10));ticket.clear();
    TEST_ASSERT_FALSE(ticket.consume(token,"FORMATIEREN",11,expected));
    TEST_ASSERT_TRUE(ticket.issue(token,card,10));
    TEST_ASSERT_TRUE(ticket.consume(token,"FORMATIEREN",11,expected));
    TEST_ASSERT_TRUE(sameSdFileStamp(card,expected));
    TEST_ASSERT_FALSE(ticket.consume(token,"FORMATIEREN",12,expected));
}
void test_ticket_wraparound_and_unsupported_media() {
    SdFormatTicket ticket;SdFileStamp expected;
    TEST_ASSERT_TRUE(ticket.issue(token,card,0xfffffff0U));
    TEST_ASSERT_TRUE(ticket.consume(token,"FORMATIEREN",5,expected));
    TEST_ASSERT_FALSE(ticket.issue(token,SdFileStamp(2000000000,0,1234),5));
    TEST_ASSERT_FALSE(ticket.issue(token,SdFileStamp(card.bytes,0,0),5));
}
void test_changed_or_missing_media_never_formats() {
    Device changed;changed.current.media++;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::MediaChanged),static_cast<int>(runSdCardFormat(changed,card)));
    TEST_ASSERT_EQUAL_UINT(0,changed.formats);
    Device missing;missing.readable=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::Failed),static_cast<int>(runSdCardFormat(missing,card)));
    TEST_ASSERT_EQUAL_UINT(0,missing.formats);
    Device unsupported;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::Failed),static_cast<int>(runSdCardFormat(unsupported,SdFileStamp(64000000000ULL,0,1234))));
    TEST_ASSERT_EQUAL_UINT(0,unsupported.inspections);
}
void test_format_error_stops_before_verification() {
    Device device;device.formatted=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::Failed),static_cast<int>(runSdCardFormat(device,card)));
    TEST_ASSERT_EQUAL_UINT(1,device.formats);TEST_ASSERT_EQUAL_UINT(0,device.verifications);
}
void test_success_requires_post_format_verification() {
    Device device;device.verified=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::Failed),static_cast<int>(runSdCardFormat(device,card)));
    Device success;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdFormatState::Complete),static_cast<int>(runSdCardFormat(success,card)));
    TEST_ASSERT_EQUAL_UINT(1,success.inspections);TEST_ASSERT_EQUAL_UINT(1,success.formats);TEST_ASSERT_EQUAL_UINT(1,success.verifications);
}
void test_format_form_is_localized_post_and_escaped() {
    const String de=renderSdFormatConfirmation(card,"'<unsafe>",Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"7948206080"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"method='post' action='/sd-card/format'"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"Alle Dateien und Partitionen"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"pattern='FORMATIEREN'"));
    TEST_ASSERT_NULL(strstr(de.c_str(),"<unsafe>"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"&lt;unsafe&gt;"));
    const String en=renderSdFormatConfirmation(card,token,Locale::EnglishUnitedStates);
    TEST_ASSERT_NOT_NULL(strstr(en.c_str(),"All files and partitions"));
    TEST_ASSERT_NOT_NULL(strstr(en.c_str(),"Cancel"));
}
}
void setUp(){} void tearDown(){}
int main(){UNITY_BEGIN();
RUN_TEST(test_capacity_boundaries);
RUN_TEST(test_confirmation_is_explicit_one_use_and_time_limited);
RUN_TEST(test_ticket_wraparound_and_unsupported_media);
RUN_TEST(test_changed_or_missing_media_never_formats);
RUN_TEST(test_format_error_stops_before_verification);
RUN_TEST(test_success_requires_post_format_verification);
RUN_TEST(test_format_form_is_localized_post_and_escaped);
return UNITY_END();}
