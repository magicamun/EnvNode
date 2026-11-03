// EnvNode: see repository LICENSE.
#include "ProfileCodec.h"
#include "JsonWriter.h"
#include <cstring>
#include <cmath>
namespace EnvNode { namespace Testing {
namespace {
bool array(CompactCborReader& r, size_t expected) { size_t n; return r.enterArray(n) && n == expected; }
bool utf8(const char* text, size_t size) {
    for(size_t i=0;i<size;) {
        const uint8_t first=static_cast<uint8_t>(text[i++]);
        if(first<128) { if(first<32 || first==127) return false; continue; }
        unsigned follow=0; uint32_t code=0,minimum=0;
        if(first>=0xc2 && first<=0xdf) { follow=1;code=first&31;minimum=0x80; }
        else if(first>=0xe0 && first<=0xef) { follow=2;code=first&15;minimum=0x800; }
        else if(first>=0xf0 && first<=0xf4) { follow=3;code=first&7;minimum=0x10000; }
        else return false;
        if(i+follow>size) return false;
        while(follow--) { const uint8_t next=static_cast<uint8_t>(text[i++]); if((next&0xc0)!=0x80) return false;code=(code<<6)|(next&63); }
        if(code<minimum || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) return false;
    }
    return true;
}
bool text(CompactCborReader& r, char* out, size_t capacity, bool empty = false) {
    const char* p; size_t n;
    if (!r.readText(p,n) || n >= capacity || (!empty && !n)) return false;
    if (!utf8(p,n)) return false;
    memcpy(out,p,n); out[n]=0; return true;
}
bool hex(const char* p) {
    if (strlen(p)!=64) return false;
    for (; *p; ++p) if (!(*p>='0' && *p<='9') && !(*p>='a' && *p<='f')) return false;
    return true;
}
bool revision(const char* p, size_t dots) {
    unsigned value=0; size_t digits=0, seen=0;
    for (;;++p) {
        if (*p>='0' && *p<='9') { value=value*10+(*p-'0'); if (value>255 || ++digits>3) return false; }
        else if (*p=='.' || !*p) {
            if (!digits) return false;
            if (!*p) return seen==dots;
            ++seen; digits=value=0;
        } else return false;
    }
}
bool number(CompactCborReader& r, size_t maximum, size_t& value) {
    uint64_t n; if (!r.readUnsigned(n) || n>maximum) return false; value=static_cast<size_t>(n); return true;
}
const char* stored(CompactCborReader& r, OwnedProfile& p, size_t cap, bool empty = false) {
    const size_t remaining=sizeof(p.text)-p.textUsed;
    if (cap>remaining) cap=remaining;
    char* out=p.text+p.textUsed;
    if (!text(r,out,cap,empty)) return nullptr;
    p.textUsed+=strlen(out)+1; return out;
}
bool position(CompactCborReader& r, double& value) { return r.readDouble(value) && std::isfinite(value) && fabs(value)<=5000; }
bool point(CompactCborReader& r, const OwnedProfile& p, float& x, float& y) {
    double a,b;
    if (!position(r,a) || !position(r,b) || a<p.bounds[0] || b<p.bounds[1]
        || a>p.bounds[0]+p.bounds[2] || b>p.bounds[1]+p.bounds[3]) return false;
    x=static_cast<float>(a); y=static_cast<float>(b); return true;
}
void member(String& json, const char* key, const char* value) {
    appendJsonString(json,key); json+=':'; appendJsonString(json,value);
}
void coordinates(String& json, float x, float y) { json+=",\"x\":"+String(x,5)+",\"y\":"+String(y,5); }
}
bool safeProfileFilename(const char* filename) {
    if (!filename) return false;
    const size_t n=strlen(filename);
    if (!n || n>175 || filename[0]=='.' || strstr(filename,"..")) return false;
    for (const char* p=filename; *p; ++p)
        if (!(*p>='a' && *p<='z') && !(*p>='0' && *p<='9') && *p!='.' && *p!='-') return false;
    return true;
}
bool decodeCatalog(const uint8_t* data, size_t size, RemoteCatalog& out) {
    if (!data || !size || size>MaximumCatalogBytes) return false;
    CompactCborReader r(data,size); size_t version,n;
    if (!array(r,2) || !number(r,1,version) || version!=1 || !r.enterArray(n) || !n || n>MaximumRemoteProfiles) return false;
    out.count=0;
    for (size_t i=0; i<n; ++i) {
        auto& e=out.entries[i];
        if (!array(r,7) || !text(r,e.id,sizeof(e.id)) || !safeProfileFilename(e.id)
            || !text(r,e.name,sizeof(e.name)) || !text(r,e.identity,sizeof(e.identity)) || !revision(e.identity,1)
            || !text(r,e.pcb,sizeof(e.pcb)) || !revision(e.pcb,2)
            || !text(r,e.filename,sizeof(e.filename)) || !safeProfileFilename(e.filename)
            || !number(r,MaximumProfileBytes,e.bytes) || !e.bytes || !text(r,e.sha256,sizeof(e.sha256)) || !hex(e.sha256)) return false;
        const String expected=String(e.id)+"."+e.sha256+".cbor";
        if (expected!=e.filename) return false;
        for (size_t j=0; j<i; ++j) if (!strcmp(e.id,out.entries[j].id)) return false;
    }
    if (r.remaining()) return false;
    out.count=n; return true;
}
bool decodeProfile(const uint8_t* data, size_t size, const RemoteProfileEntry& entry, OwnedProfile& p) {
    if (!data || size!=entry.bytes || size>MaximumProfileBytes) return false;
    CompactCborReader r(data,size); size_t version,n;
    if (!array(r,13) || !number(r,1,version) || version!=1) return false;
    p.textUsed=0;
    memcpy(p.contentSha256,entry.sha256,sizeof(p.contentSha256));
    p.board.id=stored(r,p,96); p.board.name=stored(r,p,96);
    p.board.identityRevision=stored(r,p,24); p.board.pcbRevision=stored(r,p,24);
    p.sha256=stored(r,p,65); p.source=stored(r,p,192);
    if (!p.board.id || !p.board.name || !p.board.identityRevision || !p.board.pcbRevision || !p.sha256 || !p.source
        || strcmp(entry.id,p.board.id) || strcmp(entry.name,p.board.name)
        || strcmp(entry.identity,p.board.identityRevision) || strcmp(entry.pcb,p.board.pcbRevision) || !hex(p.sha256)) return false;
    if (!array(r,4)) return false;
    for (auto& v:p.bounds) if (!position(r,v)) return false;
    if (p.bounds[2]<=0 || p.bounds[3]<=0 || p.bounds[2]>1000 || p.bounds[3]>1000) return false;
    if (!r.enterArray(n) || (n!=SocketPins && n!=MaxInputs)) return false;
    p.board.inputCount=n; p.board.socket=p.inputs;
    for (size_t i=0; i<n; ++i) {
        size_t numberValue;
        if (!array(r,4) || !number(r,MaxInputs,numberValue) || numberValue!=i+1) return false;
        p.inputs[i].number=numberValue; p.inputs[i].net=stored(r,p,128);
        if (!p.inputs[i].net || !point(r,p,p.inputs[i].x,p.inputs[i].y)) return false;
    }
    if (!r.enterArray(n) || !n || n>MaxConnectors) return false;
    p.board.count=n; p.board.connectors=p.connectors; size_t total=0;
    for (size_t i=0; i<n; ++i) {
        auto& c=p.connectors[i]; size_t kind,count;
        if (!array(r,4)) return false;
        c.id=stored(r,p,64); p.labels[i]=stored(r,p,96);
        if (!c.id || !p.labels[i] || !number(r,2,kind) || !r.enterArray(count) || count!=(kind==2?14:4)
            || total+count>MaximumProfileContacts) return false;
        for (size_t j=0; j<i; ++j) if (!strcmp(c.id,p.connectors[j].id)) return false;
        c.pigtail=static_cast<Pigtail>(kind); c.count=count; c.contacts=p.contacts+total;
        for (size_t j=0; j<count; ++j) {
            auto& pin=p.contacts[total++]; size_t pn;
            if (!array(r,5) || !number(r,14,pn) || pn!=j+1) return false;
            pin.number=pn; pin.net=stored(r,p,128);
            if (!pin.net || !point(r,p,pin.x,pin.y) || !r.readUnsigned(pin.expected) || (pin.expected>>p.board.inputCount)) return false;
        }
    }
    p.image=stored(r,p,69);
    if (!p.image || strlen(p.image)!=68 || strcmp(p.image+64,".svg")) return false;
    char digest[65]; memcpy(digest,p.image,64); digest[64]=0; if (!hex(digest)) return false;
    size_t paths;
    if (!r.enterArray(paths) || paths!=(p.board.inputCount==40?2u:0u)) return false;
    for (size_t i=0; i<paths; ++i) {
        size_t input,pin;
        if (!array(r,6) || !number(r,40,input) || input!=39+i) return false;
        p.referencePaths[i][0]=stored(r,p,32);
        if (!number(r,2,pin) || pin!=(i==0?2u:1u)) return false;
        p.referencePins[i]=pin;
        for (size_t j=1; j<4; ++j) p.referencePaths[i][j]=stored(r,p,128);
        for (const auto* t:p.referencePaths[i]) if (!t) return false;
        if (strcmp(p.referencePaths[i][3],p.inputs[38+i].net)
            || strcmp(p.referencePaths[i][2],i==0?"+3V3_SYS":"GND")) return false;
    }
    p.sourceCommit=stored(r,p,65,true);
    return p.sourceCommit && !r.remaining();
}
String profileJson(const OwnedProfile& p) {
    size_t contacts=0;
    for(size_t i=0;i<p.board.count;++i) contacts+=p.connectors[i].count;
    // Reserve a conservative bound up front; never return partially appended JSON.
    // Control characters are rejected, leaving at most two bytes per escaped byte.
    const size_t budget=2*p.textUsed+240*contacts+96*p.board.inputCount+128*p.board.count+1024;
    String json; if (budget>65536 || !json.reserve(budget)) return {};
    json="{\"schema\":1,"; member(json,"id",p.board.id); json+=','; member(json,"name",p.board.name);
    json+=','; member(json,"identityRevision",p.board.identityRevision); json+=','; member(json,"pcbRevision",p.board.pcbRevision);
    json+=','; member(json,"sha256",p.sha256); json+=','; member(json,"source",p.source);
    json+=','; member(json,"sourceCommit",p.sourceCommit); json+=',';
    member(json,"imageUrl",(String(ProfileBaseUrl)+p.image).c_str());
    json+=",\"profileOrigin\":\"remote\",\"bounds\":[";
    for (size_t i=0;i<4;++i) { if(i)json+=',';json+=String(p.bounds[i],5); }
    json+="],\"socket\":[";
    for (size_t i=0;i<p.board.inputCount;++i) {
        if(i)json+=',';const auto& pin=p.inputs[i];json+="{\"number\":"+String(pin.number)+",";member(json,"net",pin.net);
        if(i>=38) { json+=',';member(json,"label",i==38?"MB_3V3_SYS":"MB_GND"); }
        coordinates(json,pin.x,pin.y);json+='}';
    }
    json+="],\"connectors\":[";
    const char* kinds[]={"gpio","i2c","module"};
    for(size_t i=0;i<p.board.count;++i) {
        if(i)json+=',';const auto& c=p.connectors[i];json+='{';member(json,"id",c.id);json+=',';member(json,"label",p.labels[i]);
        json+=',';member(json,"pigtail",kinds[static_cast<unsigned>(c.pigtail)]);json+=",\"pins\":[";
        for(size_t j=0;j<c.count;++j) {
            if(j)json+=',';const auto& pin=c.contacts[j];json+="{\"number\":"+String(pin.number)+",";member(json,"net",pin.net);
            coordinates(json,pin.x,pin.y);json+=pin.expected?",\"covered\":true,\"expected\":[":",\"covered\":false,\"expected\":[";
            bool comma=false;
            for(size_t k=0;k<p.board.inputCount;++k) if((pin.expected>>k)&1) { if(comma)json+=',';json+=String(k+1);comma=true; }
            json+="]}";
        }
        json+="]}";
    }
    json+="],\"referencePaths\":[";
    if(p.board.inputCount==40) for(size_t i=0;i<2;++i) {
        if(i)json+=',';json+="{\"input\":"+String(39+i)+",\"pin\":"+String(p.referencePins[i])+",";
        const char* keys[]={"reference","diode","anode","cathode"};
        for(size_t k=0;k<4;++k) { if(k)json+=',';member(json,keys[k],p.referencePaths[i][k]); }
        json+=",\"direction\":\"output-to-input\"}";
    }
    json+="]}";return json;
}
} }
