#include <MakeEventTreeSingle.h>


// =========================
// Core TChain processing
// =========================

void ProcessChainBufferedDefault(TChain* chain, TTree* outtree,Long64_t tdiff,size_t BUFFER)
{
    const Long64_t COINC_WINDOW = tdiff;
    
    const size_t BUFFER_TARGET    = BUFFER;
    const size_t REFILL_THRESHOLD = BUFFER*0.1;
    
    
    // Branches
    Long64_t tTs;
    UShort_t tMod, tCh, tAdc;
    
    chain->SetBranchAddress("Ts",  &tTs);
    chain->SetBranchAddress("Mod", &tMod);
    chain->SetBranchAddress("Ch",  &tCh);
    chain->SetBranchAddress("Adc", &tAdc);
    
    std::vector<UShort_t> tTs_vec, tMod_vec, tCh_vec, tAdc_vec;
    
    outtree->Branch("Ts",  &tTs_vec);
    outtree->Branch("Mod", &tMod_vec);
    outtree->Branch("Ch",  &tCh_vec);
    outtree->Branch("Adc", &tAdc_vec);
    
    // ROOT I/O optimisation
    chain->SetCacheSize(500 * 1024 * 1024);
    chain->AddBranchToCache("*");
    
    // Buffer
    std::vector<Event> buffer;
    buffer.reserve(BUFFER);  // avoid reallocations
    // Event logic
    Long64_t firstTs = -1;
    Long64_t lastTs  = -1;
    
    // Read control
    Long64_t entry = 0;
    Long64_t n = chain->GetEntries();
    
    // =========================
    // Fill function
    // =========================
    auto FillBuffer = [&](size_t nFill) {
        size_t added = 0;
        
        while (added < nFill && entry < n) {
            chain->GetEntry(entry++);
            buffer.push_back({tTs, tMod, tCh, tAdc});
            added++;
        }
        return added;
    };
    
    
    // =========================
    // Initial fill + sort
    // =========================    
    FillBuffer(BUFFER_TARGET);
    
    std::sort(buffer.begin(), buffer.end(),
              [](const Event& a, const Event& b){
                  return a.ts < b.ts;
              });
    
    size_t start = 0;   // logical start of valid data
    size_t idx   = 0;   // absolute index into buffer
    size_t popCount = 0;    
    
    // =========================
    // Main loop
    // =========================
    while (idx < buffer.size()) {
        
        Event& ev = buffer[idx++];
        popCount++;
        
        Long64_t tTs_local = ev.ts;
        
        // --- event building ---
        if (tTs_vec.empty()) {
            
            // Only actually called for very first event
            
            firstTs = tTs_local;
            lastTs  = tTs_local;
            
            tTs_vec.push_back(0);
            tMod_vec.push_back(ev.mod);
            tCh_vec.push_back(ev.ch);
            tAdc_vec.push_back(ev.adc);
        }
        else {
            
            if (tTs_local < lastTs) {
                std::cout << "[TIME RESET]\n";
                
                tTs_vec.clear();
                tMod_vec.clear();
                tCh_vec.clear();
                tAdc_vec.clear();
                
                firstTs = tTs_local;
                lastTs  = tTs_local;
                
                tTs_vec.push_back(0);
                tMod_vec.push_back(ev.mod);
                tCh_vec.push_back(ev.ch);
                tAdc_vec.push_back(ev.adc);
            }
            else if (tTs_local - lastTs < COINC_WINDOW) {
                
                tTs_vec.push_back(SafeTsDiff(tTs_local, firstTs));
                tMod_vec.push_back(ev.mod);
                tCh_vec.push_back(ev.ch);
                tAdc_vec.push_back(ev.adc);
                
                lastTs = tTs_local;
            }
            else {
                
                // EVENT COMPLETE
                outtree->Fill();
                
                tTs_vec.clear();
                tMod_vec.clear();
                tCh_vec.clear();
                tAdc_vec.clear();
                
                firstTs = tTs_local;
                lastTs  = tTs_local;
                
                tTs_vec.push_back(0);
                tMod_vec.push_back(ev.mod);
                tCh_vec.push_back(ev.ch);
                tAdc_vec.push_back(ev.adc);
            }
        }
        
        // =========================
        // Refill condition
        // =========================
        
        if (popCount >= REFILL_THRESHOLD) {
            
            ///// remove consumed elements on every read
            //             buffer.erase(buffer.begin(), buffer.begin() + idx);
            //             idx = 0;
            
            size_t old_size = buffer.size() - idx;
            
            // Fill and get actual number added
            size_t added = FillBuffer(BUFFER_TARGET - old_size);
            
            // Nothing added → end of data
            if (added == 0) {
                // no more refill possible
            } else {
                
                size_t new_size = buffer.size() - idx;
                
                auto base = buffer.begin() + idx;
                
                // Sort ONLY new elements
                std::sort(base + old_size,
                          base + new_size,
                          [](const Event& a, const Event& b){
                              return a.ts < b.ts;
                          });
                
                // Merge only as far as current index
                std::inplace_merge(base,
                                   base + old_size,
                                   base + new_size);
            }
            
            popCount = 0;
            
            // =========================
            // Occasional compaction. Not notably faster than shrinking every fill
            // =========================
            if (buffer.size() > 2*BUFFER) {
                buffer.erase(buffer.begin(), buffer.begin() + idx);
                idx = 0;
            }
        }
        
        if (idx % 100000 == 0)
            std::cout<< entry << "/" << n  <<" : "<< idx << "/" << buffer.size() <<"                    \r"<< std::flush;
    }
    
    if (!tTs_vec.empty()) {
        outtree->Fill();
    }
}


void ProcessChainBuffered(TChain* chain, TTree* outtree,Long64_t tdiff,int BUFFER){
    if(BUFFER<0)BUFFER=gBuildBuffDefaultSize;
    ProcessChainBufferedDefault(chain,outtree,tdiff,BUFFER);
}

// =========================
// Core TTree processing
// =========================
void ProcessTree(TTree* tree, TTree* outtree,Long64_t tdiff)
{
    const Long64_t COINC_WINDOW = tdiff;
    
    Long64_t tTs;
    UShort_t tMod, tCh, tAdc;
    
    tree->SetBranchAddress("Ts",  &tTs);
    tree->SetBranchAddress("Mod", &tMod);
    tree->SetBranchAddress("Ch",  &tCh);
    tree->SetBranchAddress("Adc", &tAdc);
    
    std::vector<UShort_t> tTs_vec, tMod_vec, tCh_vec, tAdc_vec;
    
    outtree->Branch("Ts",  &tTs_vec);
    outtree->Branch("Mod", &tMod_vec);
    outtree->Branch("Ch",  &tCh_vec);
    outtree->Branch("Adc", &tAdc_vec);
    
    
    Long64_t firstTs = -1;
    Long64_t lastTs  = -1;
    Long64_t last_timestamp = -1;
    
    
    tree->BuildIndex("Ts");
    TTreeIndex *index = (TTreeIndex*)tree->GetTreeIndex();
    //index->Print();
    
    tree->LoadBaskets(2e10);
    
    
    Long64_t n = index->GetN();
    
    for(int i=0;i<n;i++){
        Long64_t local = tree->LoadTree(index->GetIndex()[i]);
        tree->GetEntry(local);
        
        if (tTs_vec.empty()) {
            
            firstTs = tTs;
            lastTs  = tTs;
            
            tTs_vec.push_back(0);
            tMod_vec.push_back(tMod);
            tCh_vec.push_back(tCh);
            tAdc_vec.push_back(tAdc);
        }
        else {
            
            if (tTs - lastTs < COINC_WINDOW) {
                
                tTs_vec.push_back(SafeTsDiff(tTs, firstTs));
                tMod_vec.push_back(tMod);
                tCh_vec.push_back(tCh);
                tAdc_vec.push_back(tAdc);
                
                lastTs = tTs;  // critical
            }
            else {
                
                outtree->Fill();
                
                tTs_vec.clear();
                tMod_vec.clear();
                tCh_vec.clear();
                tAdc_vec.clear();
                
                firstTs = tTs;
                lastTs  = tTs;
                
                tTs_vec.push_back(0);
                tMod_vec.push_back(tMod);
                tCh_vec.push_back(tCh);
                tAdc_vec.push_back(tAdc);
            }
        }
        
        if (last_timestamp > tTs) {
            std::cout << "TIME ERROR "
            << last_timestamp << " -> " << tTs << std::endl;
        }
        
        last_timestamp = tTs;
        
        if (i % 100000 == 0)
            std::cout << i << "/" << n << "\r" << std::flush;
    }
    
    if (!tTs_vec.empty())
        outtree->Fill();
}

// =========================
// Wrapper
// =========================
void MakeEventTreeNew(TString infilename,
                      TString outfilename,
                      bool chainmode,
                      Long64_t tdiff,
                      int BufferSize)
{
    
    // Start timer
    TStopwatch timer;
    timer.Start();
    
    std::vector<TString> files = GetTreeSplitFileList(infilename);
    
    if(!outfilename.Length())outfilename=infilename + "_events.root";
    TFile *outfile = new TFile(outfilename,"RECREATE");
    TTree *outtree = new TTree("EventTree","EventTree");
    
    // ROOT auto file splitting (~1.9 GB safe)
    outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
    // Removes autosave in the case of crash, but removes duplicate TTree keys
    outtree->SetAutoSave(0);
    
    if (chainmode) {
        
        TChain chain("OutTTree01");
        
        for (auto &f : files)
            chain.Add(f);
        
        // Retrieve the parameter
        chain.LoadTree(0);
        auto p = dynamic_cast<TParameter<int>*>(chain.GetTree()->GetUserInfo()->FindObject("ChunkSize"));
        
        int chunkSize=-1;
        if (p) { chunkSize = p->GetVal();}
        else { std::cout << "ChunkSize flag not found!" << std::endl;}
        
        if(chunkSize<0){
            std::cout << "Data Is NOT Chunked! Buffered Sort Will Fail!"<< std::endl;
            if(files.size()>1){
                std::cout << "TTree spread over multiple files. File-wise LoadBaskets sort will also Fail!"<< std::endl;
            }else{
                std::cout << "Defaulting to file-wise LoadBaskets sort."<< std::endl;
                chainmode=false;
            }
        }else{
            std::cout << "Chunk Size = " << chunkSize << std::endl;
            if(std::abs(chunkSize)*100>gBuildBuffDefaultSize){
                std::cout << "Default Buffer Size will be too small! = " << gBuildBuffDefaultSize << std::endl;
                if(BufferSize>0&&std::abs(chunkSize)*100>BufferSize){
                    std::cout << "User set Buffer Size will also be too small! = " << BufferSize << std::endl;
                }
            }else{
                ProcessChainBuffered(&chain, outtree,tdiff,BufferSize);
            }
        }
    }
    
    if(!chainmode) {
        
        for (auto &f : files) {
            TFile infile(f,"READ");
            TTree *tree = (TTree*)infile.Get("OutTTree01");
            ProcessTree(tree, outtree,tdiff);
        }
    }
    
    
    TFile* currentFile = outtree->GetCurrentFile();
    if (currentFile) {
        currentFile->Write("", TObject::kOverwrite);
        currentFile->Close();
    }
    
    std::cout << "\nDone\n";
    
    timer.Stop();
    Double_t rtime = timer.RealTime();
    Double_t ctime = timer.CpuTime();
    std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );
}
