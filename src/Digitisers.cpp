#include <Digitisers.h>

Long64_t DigitiserBase::TS_TOLERANCE = 100000; // ns


std::vector<std::unique_ptr<DigitiserBase>>
BuildDigitiserList(const TString& runName)
{
    std::cout<<"Building digitiser list from files."<<std::endl;
    std::vector<std::unique_ptr<DigitiserBase>> digitisers;
    
    //APV8104
    // Check existence (kFALSE = exists and accessible)
    if (!gSystem->AccessPathName(APV8104::buildFileName(runName, 0))) {
        // Construct only if file exists
        std::cout<<"Added APV8104 as module "<<digitisers.size()<<std::endl;
        digitisers.push_back(std::make_unique<APV8104>(runName,digitisers.size()));
    }
    //APV8032
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8032::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            std::cout<<"Added APV8032_"<<fmodl<<" as module "<<digitisers.size()<<std::endl;
            digitisers.push_back(std::make_unique<APV8032>(runName,digitisers.size(), fmodl));
        }
    }
    //APV8016A
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8016A::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            std::cout<<"Added APV8016A_"<<fmodl<<" as module "<<digitisers.size()<<std::endl;
            digitisers.push_back(std::make_unique<APV8016A>(runName,digitisers.size(),fmodl));
        }
    }
    
    return digitisers;
}
