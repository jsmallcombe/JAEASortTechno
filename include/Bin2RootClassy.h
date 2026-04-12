#ifndef Bin2RootClassySingle
#define Bin2RootClassySingle

#include <Digitisers.h>
#include <Globals.h>

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TStopwatch.h>
#include <TParameter.h>


// =========================
// Global chunk reader
// =========================
bool readChunk(DigitiserBase& digi, int chunkSize,TTree* tree, Event& ev);

// =========================
// Main
// =========================
void Bin2RootClassy(TString run,TString out="",int CHUNK_SIZE_IN = 100,Long64_t TS_TOLERANCE = gTS_TOLERANCE);

#endif
