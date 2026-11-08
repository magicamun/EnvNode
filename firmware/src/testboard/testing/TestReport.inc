// EnvNode: see repository LICENSE.
// Bounded, read-only pages. Export never selects/retests a connector or drives GPIO.
void TestWebRoutes::reportContact() {
    auto& server=host_.server();uint32_t generation,c,p=0,offset=0;
    const bool whole=server.arg("contact").isEmpty();
    const auto* board=runner_.board();
    if(!board||runner_.phase()==Phase::Running||!unsignedValue(server.arg("generation"),generation)||generation!=runner_.generation()) {
        server.send(409,"application/json","{\"error\":\"No stable session; finish measurement and retry export\"}");return;
    }
    if(!unsignedValue(server.arg("connector"),c)||c>=board->count||
       (!whole&&!unsignedValue(server.arg("contact"),p))||p>=board->connectors[c].count||
       (!server.arg("offset").isEmpty()&&!unsignedValue(server.arg("offset"),offset))||offset>runner_.diagnosticCount()) {
        server.send(400,"application/json","{\"error\":\"Invalid report page\"}");return;
    }
    String json;if(!json.reserve(6500)){server.send(503,"application/json","{\"error\":\"Report buffer unavailable\"}");return;}
    json="{\"firmware\":";appendJsonString(json,EnvNode::FirmwareVersion);
    json+=",\"gitCommit\":";appendJsonString(json,gitCommit_);
    json+=",\"buildNumber\":";appendJsonString(json,buildNumber_);
    json+=",\"boardId\":";appendJsonString(json,board->id);
    json+=",\"generation\":"+String(generation)+",\"connectorIndex\":"+String(c)+",\"contact\":"+String(whole?0:p+1);
    json+=",\"verdict\":"+String(unsigned(whole?runner_.verdict(c):runner_.contactVerdict(c,p)));
    const size_t begin=whole?0:p*board->inputCount;
    const size_t cells=board->inputCount*(whole?board->connectors[c].count:1);
    const size_t completed=runner_.completedAt(c);
    json+=",\"completed\":"+String(unsigned(completed>begin?std::min(cells,completed-begin):0));
    json+=",\"results\":\"";
    for(size_t i=0;i<cells;++i)json+=char('0'+unsigned(runner_.resultAt(c,begin+i)));
    json+="\",\"rawResults\":\"";
    for(size_t i=0;i<cells;++i)json+=char('0'+unsigned(runner_.rawResultAt(c,begin+i)));
    json+="\",\"contactVerdicts\":\"";
    for(size_t n=0;n<board->connectors[c].count;++n)json+=char('0'+unsigned(runner_.contactVerdict(c,n)));
    json+="\",\"diagnosticCapacity\":"+String(unsigned(TestRunner::DiagnosticCapacity));
    json+=",\"diagnosticsDropped\":"+String(runner_.diagnosticsDropped())+",\"diagnostics\":[";
    size_t count=0,i=offset;
    for(;i<runner_.diagnosticCount()&&count<16;++i) {
        const auto& d=runner_.diagnosticAt(i);
        if(d.connector!=c||(!whole&&d.cell/board->inputCount!=p))continue;
        if(count++)json+=',';
        json+="{\"contact\":"+String(unsigned(d.cell/board->inputCount+1))+",\"input\":"+String(unsigned(d.cell%board->inputCount+1));
        json+=",\"attempt\":"+String(d.attempt)+",\"atUptimeMs\":"+String(d.atMs);
        json+=",\"observedResult\":"+String(unsigned(d.result))+",\"reason\":"+String(unsigned(d.trace.reason));
        json+=",\"low\":[";
        for(size_t n=0;n<d.trace.lowCount;++n){if(n)json+=',';json+=String(d.trace.low[n]);}
        json+="],\"high\":[";
        for(size_t n=0;n<d.trace.highCount;++n){if(n)json+=',';json+=String(d.trace.high[n]);}
        json+="]}";
    }
    json+="],\"nextOffset\":";
    if(i<runner_.diagnosticCount())json+=String(unsigned(i));else json+="null";
    json+='}';
    server.sendHeader("Cache-Control","no-store");server.send(200,"application/json; charset=utf-8",json);
}
