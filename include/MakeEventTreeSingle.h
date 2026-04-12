#ifndef MakeEventTreeSingle
#define MakeEventTreeSingle

#include <Globals.h>
#include <IO.h>


#include <TFile.h>
#include <TTree.h>
#include <TTreeIndex.h>
#include <TChain.h>
#include <TStopwatch.h>
#include <TParameter.h>

#include <iostream>
#include <vector>
#include <algorithm>


void ProcessChainBufferedDefault(TChain* chain, TTree* outtree,Long64_t tdiff=2000,size_t BUFFER=gBuildBuffDefaultSize);


void ProcessChainBuffered(TChain* chain, TTree* outtree,Long64_t tdiff=2000,int BUFFER=-1);


void ProcessTree(TTree* tree, TTree* outtree,Long64_t tdiff=2000);


void MakeEventTreeNew(TString infilename,
                     TString outfilename="",
                     bool chainmode = false,
					 Long64_t tdiff=2000,
                     int BufferSize=-1);


#endif
