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
        std::cout<< std::endl << "Build window default overidden: " << TS_Diff << std::flush;
    }

    if(ReadBin){
        if(WriteTree) {
        }
        std::cout<<std::endl<<"Running bin sort with "<<gIO->Digitisers.size()<<" digitiser modules."<<std::flush;
        gIO->SortBins(WriteTree, DoHistSort, Overwrite, TS_Diff);
    }else if(ReadTree){
        if(!DoHistSort) {
            std::cout << std::endl << "Running tree sort with " << EventData->GetEntries() << " entries." << std::flush;
            gIO->SortTree(EventData, WriteTree, DoHistSort, Overwrite, HistWorkers);
        }else{      
        std::cout<<std::endl<<"Running tree sort with "<<EventData->GetEntries()<<" entries."<<std::flush;
        gIO->SortTree(EventData, WriteTree, DoHistSort, Overwrite, HistWorkers);
    }else{
        std::cout<<std::endl<<"No input data found. Exiting."<<std::flush;
    }   





    if(EventData!=nullptr)delete EventData;
    if(gIO!=nullptr)delete gIO;
    return 0;
}
