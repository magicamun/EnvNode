// EnvNode: see repository LICENSE.
#ifdef ARDUINO
#include "HttpsProfileTransport.h"
#include "DocsRootCertificate.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>
#include <new>
#include <algorithm>
#include <cstring>
namespace EnvNode { namespace Testing {
bool HttpsProfileTransport::fetch(const char* filename, size_t maximum, const char* digest,
    std::unique_ptr<uint8_t[]>& output, size_t& size) {
    output.reset();size=0;
    if(!safeProfileFilename(filename) || maximum>MaximumProfileBytes
        || ESP.getFreeHeap()<70000 || ESP.getMaxAllocHeap()<24000) return false;
    WiFiClientSecure client;client.setCACert(DocsRootCertificate);client.setHandshakeTimeout(2);
    HTTPClient http;http.setConnectTimeout(1500);http.setTimeout(1500);http.useHTTP10(true);
    // Redirects are deliberately disabled: every request stays on the fixed origin.
    if(!http.begin(client,String(ProfileBaseUrl)+filename)) return false;
    const int code=http.GET();const int length=http.getSize();
    if(code!=200 || length<=0 || static_cast<size_t>(length)>maximum) {
        logger_.warnf("Remote profile request failed HTTP=%d bytes=%d",code,length);http.end();return false;
    }
    std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[length]);
    if(!bytes) { http.end();return false; }
    auto* stream=http.getStreamPtr();size_t count=0;const uint32_t started=millis();
    while(count<static_cast<size_t>(length) && uint32_t(millis()-started)<3000) {
        const int available=stream->available();
        if(available>0) {
            const size_t chunk=std::min(static_cast<size_t>(available),static_cast<size_t>(length)-count);
            const int read=stream->read(bytes.get()+count,chunk);
            if(read<=0) break;
            count+=static_cast<size_t>(read);
        } else { if(!http.connected()) break;delay(1); }
    }
    http.end();
    if(count!=static_cast<size_t>(length)) return false;
    if(digest) {
        uint8_t hash[32];char hex[65];
        if(mbedtls_sha256_ret(bytes.get(),count,hash,0)) return false;
        const char* digits="0123456789abcdef";
        for(size_t i=0;i<32;++i) { hex[2*i]=digits[hash[i]>>4];hex[2*i+1]=digits[hash[i]&15]; }
        hex[64]=0;
        if(strcmp(hex,digest)) { logger_.warn("Remote profile SHA-256 mismatch");return false; }
    }
    output=std::move(bytes);size=count;return true;
}
} }
#endif
