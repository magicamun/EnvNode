// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#include <initializer_list>
#include "RmsWindow.h"
#include "RawDiagnostics.h"
#include "LowPass.h"
#include "CurrentGate.h"
#include <cassert>
int main(){
 CurrentGate gate;
 for(double value : {0.045,0.060,0.070,0.080}) assert(gate.apply(value)==0);
 for(double value : {0.161,0.080,0.070,0.060}) assert(gate.apply(value)==value);
 assert(gate.apply(0.059)==0 && gate.apply(0.075)==0);
 gate.offMa=100;gate.onMa=140;
 assert(gate.apply(0.130)==0 && gate.apply(0.141)==0.141 && gate.apply(0.099)==0);

 // Validate DC rejection and response at 50 Hz, cutoff and above cutoff.
 for (double cutoff : {100.,200.,300.,500.}) {
  for (double freq : {50.,cutoff,800.}) {
   LowPass f(cutoff,2000); RmsWindow result;
   for(int i=0;i<2000;++i){
    const double y=f.add(sqrt(2.)*sin(2*3.141592653589793*freq*i/2000));
    if(i>=200)result.add(y);
   }
   const double ratio=tan(3.141592653589793*freq/2000)/tan(3.141592653589793*cutoff/2000);
   const double expected=1/sqrt(1+pow(ratio,4));
   assert(fabs(result.acRmsMv()-expected)<1e-7);
  }
  LowPass f(cutoff,2000);
  for(int i=0;i<2000;++i)assert(f.add(0)==0);
 }

 uint16_t codes[100], mv[100];
 for(int i=0;i<100;++i){codes[i]=i;mv[i]=1650;}
 codes[99]=4095;mv[99]=1710;mv[98]=1680;
 RawDiagnostics<100> d;
 d.calculate(codes,mv,1650);
 assert(d.p01==0 && d.p50==49 && d.p99==98);
 assert(d.lowRail==1 && d.highRail==1 && d.beyond20==2 && d.beyond50==1);
 assert(codes[99]==4095 && codes[0]==0);
 for(int i=0;i<100;++i){codes[i]=2000;mv[i]=1650;}
 d.calculate(codes,mv,1650);
 assert(d.raw.acRmsMv()==0 && d.p01==2000 && d.p99==2000);
 assert(d.lowRail==0 && d.highRail==0 && d.beyond20==0 && d.beyond50==0);

 RmsWindow dc; for(int i=0;i<2000;++i)dc.add(1650);assert(dc.acRmsMv()==0);
 for(double rms: {8.7,26.1,500.0}) {
  RmsWindow w;for(int i=0;i<2000;++i)w.add(1650+sqrt(2)*rms*sin(2*3.141592653589793*50*i/2000));
  assert(fabs(w.mean-1650)<1e-8);assert(fabs(w.acRmsMv()-rms)<1e-8);
 }
}
