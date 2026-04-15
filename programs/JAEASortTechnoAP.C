#include <IO.h>
#include <Globals.h>

#include <iostream>

int main(int argc, char** argv)
{
    gIO=new JAEASortIO(argc, argv);
    if(gIO==nullptr)return 1;
    if(!gIO->Validated)return 2;
    
    bool ReadBin=(gIO->Digitisers.size()>0);
    TChain* EventData=gIO->DataTree("EventTree");
    bool ReadTree=(EventData!=nullptr);
    if(ReadTree)ReadBin=false;
    bool WriteTree = gIO->WriteEventTree;
    bool DoHistSort = gIO->DoHistSort;
    bool Overwrite = gIO->Overwrite;

    int HistWorkers=gIO->GetInput("Workers",4);
    Long64_t TS_Diff = gIO->GetInput("Window",gTS_Diff);

    if(gIO->TestInput("Window")){
        std::cout << "Build window default overidden: " << TS_Diff << std::endl;
    }

    if(ReadBin){
        // Sorting from .bin files, writing either tree or histograms or both
        FunctionToChooseVersionOfThreadedBinSort(TS_Diff,Overwrite,WriteTree,DoHistSort,HistWorkers);
    }else if(ReadTree){
        // Threaded sort to histograms from event tree
        ThreadedTreeSort(EventData,HistWorkers,Overwrite);
    }

    if(EventData!=nullptr)delete EventData;
    if(gIO!=nullptr)delete gIO;
    return 0;
}
