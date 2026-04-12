//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Tue Mar 31 23:12:39 2026 by ROOT version 6.36.10
// from TTree OutTTree01/CdTeTest_003
// found on file: CdTeTest_003.root
//////////////////////////////////////////////////////////

#ifndef Sort_h
#define Sort_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include "vector"
#include "vector"

class Sort {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
//   UShort_t        Num;
   vector<double>  *Ts;
   vector<unsigned short> *Mod;
   vector<unsigned short> *Ch;
   vector<unsigned short> *Adc;

   // List of branches
 //  TBranch        *b_Num;   //!
   TBranch        *b_Ts;   //!
   TBranch        *b_Mod;   //!
   TBranch        *b_Ch;   //!
   TBranch        *b_Adc;   //!

   Sort(const char* infilename = "Events_006.root");
   virtual ~Sort();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop(const char*);
   virtual bool     Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef Sort_cxx
Sort::Sort(const char* infilename) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.

   TFile *infile = new TFile(infilename,"READ"); 

   TTree *tree = (TTree*)infile->Get("EventTree");
	
   Init(tree);
}

Sort::~Sort()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t Sort::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t Sort::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void Sort::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   Ts = 0;
   Mod = 0;
   Ch = 0;
   Adc = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

//   fChain->SetBranchAddress("Num", &Num, &b_Num);
   fChain->SetBranchAddress("Ts", &Ts, &b_Ts);
   fChain->SetBranchAddress("Mod", &Mod, &b_Mod);
   fChain->SetBranchAddress("Ch", &Ch, &b_Ch);
   fChain->SetBranchAddress("Adc", &Adc, &b_Adc);
   Notify();
}

bool Sort::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return true;
}

void Sort::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t Sort::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef Sort_cxx
