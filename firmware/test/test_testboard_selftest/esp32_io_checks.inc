// EnvNode: see repository LICENSE.
// Compile production GPIO sequencing against trace primitives; no ESP32 is driven.
#include <algorithm>
struct GpioEvent { int kind,pin,value; };
static std::vector<GpioEvent> gpioTrace;
static int gpioLevels[64],gpioModes[64];
static void traceWrite(unsigned char p,int v){gpioLevels[p]=v;gpioTrace.push_back({0,p,v});}
static void traceMode(unsigned char p,int v){gpioModes[p]=v;gpioTrace.push_back({1,p,v});}
static int traceRead(unsigned char){return 2273;}
static void traceResolution(int){}
static void traceAttenuation(unsigned char,int){}
#define digitalWrite traceWrite
#define pinMode traceMode
#define analogRead traceRead
#define analogReadResolution traceResolution
#define analogSetPinAttenuation traceAttenuation
#define ADC_11db 3
#include "../../src/testboard/diagnostics/Esp32FixtureIo.cpp"
#undef digitalWrite
#undef pinMode
#undef analogRead
#undef analogReadResolution
#undef analogSetPinAttenuation
#undef ADC_11db
void test_shared_gpio_io_disables_before_address_changes_and_has_no_boot_side_effects() {
    gpioTrace.clear();std::fill_n(gpioLevels,64,-1);std::fill_n(gpioModes,64,-1);
    EnvNode::Esp32FixtureIo io;io.releaseDiagnostic();TEST_ASSERT_TRUE(gpioTrace.empty());
    io.selectDiagnostic({{3,0},-1,0,true});TEST_ASSERT_TRUE(gpioTrace.empty());
    io.selectDiagnostic({{1,6},2,14,true});
    TEST_ASSERT_EQUAL(LOW,gpioLevels[4]);TEST_ASSERT_EQUAL(HIGH,gpioLevels[23]);
    TEST_ASSERT_EQUAL(LOW,gpioLevels[33]);TEST_ASSERT_EQUAL(HIGH,gpioLevels[14]);TEST_ASSERT_EQUAL(HIGH,gpioLevels[26]);
    TEST_ASSERT_EQUAL(HIGH,gpioLevels[17]);TEST_ASSERT_EQUAL(OUTPUT,gpioModes[17]);
    TEST_ASSERT_EQUAL(0,gpioLevels[18]);TEST_ASSERT_EQUAL(1,gpioLevels[19]);TEST_ASSERT_EQUAL(1,gpioLevels[21]);TEST_ASSERT_EQUAL(0,gpioLevels[22]);
    TEST_ASSERT_EQUAL(0,gpioLevels[32]);TEST_ASSERT_EQUAL(1,gpioLevels[25]);TEST_ASSERT_EQUAL(1,gpioLevels[27]);TEST_ASSERT_EQUAL(1,gpioLevels[13]);
    gpioTrace.clear();io.selectDiagnostic({{0,8},0,7,false});
    size_t lastAddress=0,firstEnable=gpioTrace.size();
    for(size_t i=0;i<gpioTrace.size();++i){const auto& e=gpioTrace[i];
        if(e.kind==0&&(e.pin==18||e.pin==19||e.pin==21||e.pin==22||e.pin==32||e.pin==25||e.pin==27||e.pin==13))lastAddress=i;
        if(e.kind==0&&e.value==LOW&&(e.pin==4||e.pin==23||e.pin==14||e.pin==26||e.pin==33))firstEnable=std::min(firstEnable,i);
    }
    TEST_ASSERT_TRUE(lastAddress<firstEnable);TEST_ASSERT_EQUAL(LOW,gpioLevels[17]);
    io.selectDiagnostic({{255,8},-1,0,true});TEST_ASSERT_EQUAL(HIGH,gpioLevels[17]);
    for(int p:{4,23,14,26,33})TEST_ASSERT_EQUAL(HIGH,gpioLevels[p]);
    io.releaseDiagnostic();TEST_ASSERT_EQUAL(INPUT,gpioModes[17]);
    const auto count=gpioTrace.size();io.releaseDiagnostic();TEST_ASSERT_EQUAL(count,gpioTrace.size());
}
