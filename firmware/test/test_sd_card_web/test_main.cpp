// EnvNode: see repository LICENSE.
#include <unity.h>
#include <cstring>
#include "SdCardWebView.h"
#include "SdCardMountPath.h"
#include "WebNavigation.h"
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
using namespace EnvNode;
void setUp() {} void tearDown() {}
void test_mount_path_fits_legacy_fat_vfs_unregister_storage() {
    // ESP_VFS_PATH_MAX is 15 in the installed SDK; FAT uses strlcpy(size-1).
    TEST_ASSERT_LESS_OR_EQUAL_UINT(13, strlen(SdCardMountPath));
    TEST_ASSERT_EQUAL_CHAR('/', SdCardMountPath[0]);
}
void test_paths_reject_traversal_and_controls() {
    TEST_ASSERT_TRUE(validSdDirectoryPath("/"));
    TEST_ASSERT_TRUE(validSdDirectoryPath("/a b/ä&x"));
    const char* invalid[]={"", "relative", "/../a", "/a/..", "/a/.", "/a//b", "/a/", "/a\\b", "/a\nb"};
    for (const auto* path:invalid) TEST_ASSERT_FALSE(validSdDirectoryPath(path));
    String huge="/"; for (unsigned i=0;i<241;++i) huge+='a';
    TEST_ASSERT_FALSE(validSdDirectoryPath(huge));
}
void test_urls_encode_names_without_turning_them_into_query_parameters() {
    TEST_ASSERT_EQUAL_STRING("/sd-card?path=%2Fa%20%26%3F%25%23&offset=32", sdDirectoryUrl("/a &?%#",32).c_str());
}
void test_listing_escapes_names_and_preserves_large_sizes() {
    SdDirectorySnapshot s; s.status=SdDirectoryStatus::Ready;
    s.info.cardBytes=7948206080ULL; s.info.filesystemBytes=7931645952ULL;
    s.entries[0].name="<script>&\".bin"; s.entries[0].bytes=5000000000ULL;
    s.entries[1].name="dir & files"; s.entries[1].directory=true;
    s.count=2; s.more=true;
    const String html=renderSdDirectory(s,Locale::GermanGermany);
    TEST_ASSERT_NULL(strstr(html.c_str(),"<script>"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"&lt;script&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"5000000000"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"dir%20%26%20files"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"offset=2"));
}
void test_errors_do_not_expose_stale_entries_and_are_localized() {
    SdDirectorySnapshot s; s.status=SdDirectoryStatus::NoCard; s.count=1; s.entries[0].name="stale-secret.bin";
    const String de=renderSdDirectory(s,Locale::GermanGermany);
    const String en=renderSdDirectory(s,Locale::EnglishUnitedKingdom);
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"Keine Karte"));
    TEST_ASSERT_NOT_NULL(strstr(en.c_str(),"No card"));
    TEST_ASSERT_NULL(strstr(de.c_str(),"stale-secret"));
}
void test_long_escaped_names_use_a_bounded_page() {
    SdDirectorySnapshot s; s.status=SdDirectoryStatus::Ready;
    String name; for (unsigned i=0;i<255;++i) name+='&';
    s.count=SdDirectoryPageSize;
    for (size_t i=0;i<s.count;++i) s.entries[i].name=name;
    const String html=renderSdDirectory(s,Locale::EnglishUnitedKingdom);
    TEST_ASSERT_LESS_THAN_UINT(12288, html.length());
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),">Next<"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"</section>"));
}
void test_parent_limit_and_navigation() {
    SdDirectorySnapshot s; s.status=SdDirectoryStatus::Ready; s.path="/one/two"; s.limited=true; s.more=true;
    const String html=renderSdDirectory(s,Locale::EnglishUnitedStates);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"path=%2Fone"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(),"Listing limit reached"));
    TEST_ASSERT_NULL(strstr(html.c_str(),">Next<"));
    const String nav=buildWebNavigationHtml("/sd-card",Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(nav.c_str(),"href='/sd-card' class='active'"));
}
int main() { UNITY_BEGIN();
    RUN_TEST(test_mount_path_fits_legacy_fat_vfs_unregister_storage);
    RUN_TEST(test_paths_reject_traversal_and_controls);
    RUN_TEST(test_urls_encode_names_without_turning_them_into_query_parameters);
    RUN_TEST(test_listing_escapes_names_and_preserves_large_sizes);
    RUN_TEST(test_errors_do_not_expose_stale_entries_and_are_localized);
    RUN_TEST(test_long_escaped_names_use_a_bounded_page);
    RUN_TEST(test_parent_limit_and_navigation);
    return UNITY_END(); }
