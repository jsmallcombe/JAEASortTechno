#include <Digitisers.h>

#include <algorithm>

Long64_t DigitiserBase::TS_TOLERANCE = 100000; // ns

int APV8104::ModuleZeroIndex=-1;
int APV8032::ModuleZeroIndex=-1;
int APV8016A::ModuleZeroIndex=-1;

DigitiserAdcHistograms::DigitiserAdcHistograms(DigitiserAdcHistograms&& other) noexcept
    : histograms(std::move(other.histograms)),
      moduleOffsets(std::move(other.moduleOffsets)),
      moduleChannels(std::move(other.moduleChannels)),
      ownedByThis(std::move(other.ownedByThis))
{
    other.histograms.clear();
    other.moduleOffsets.clear();
    other.moduleChannels.clear();
    other.ownedByThis.clear();
}

DigitiserAdcHistograms& DigitiserAdcHistograms::operator=(DigitiserAdcHistograms&& other) noexcept
{
    if (this != &other) {
        Clear();
        histograms = std::move(other.histograms);
        moduleOffsets = std::move(other.moduleOffsets);
        moduleChannels = std::move(other.moduleChannels);
        ownedByThis = std::move(other.ownedByThis);
        other.histograms.clear();
        other.moduleOffsets.clear();
        other.moduleChannels.clear();
        other.ownedByThis.clear();
    }
    return *this;
}

DigitiserAdcHistograms::~DigitiserAdcHistograms()
{
    Clear();
}

void DigitiserAdcHistograms::Fill(int module, int channel, double value)
{
    if (module < 0 || static_cast<size_t>(module) >= moduleOffsets.size()) {
        return;
    }

    const int offset = moduleOffsets[module];
    const int channels = moduleChannels[module];
    if (offset < 0 || channel < 0 || channel >= channels) {
        return;
    }

    histograms[offset + channel]->Fill(value);
}

Int_t DigitiserAdcHistograms::Write(const char* name, Int_t option, Int_t bufsize)
{
    Int_t bytesWritten = 0;
    for (TH1F* histogram : histograms) {
        if (histogram) {
            bytesWritten += histogram->Write(name, option, bufsize);
        }
    }
    return bytesWritten;
}

void DigitiserAdcHistograms::Clear()
{
    for (size_t i = 0; i < histograms.size(); ++i) {
        if (ownedByThis[i]) {
            delete histograms[i];
        }
    }
    histograms.clear();
    moduleOffsets.clear();
    moduleChannels.clear();
    ownedByThis.clear();
}

std::vector<std::unique_ptr<DigitiserBase>>
BuildDigitiserList(const TString& runName)
{
    std::cout<<"Building digitiser list from files."<<std::endl;
    std::vector<std::unique_ptr<DigitiserBase>> digitisers;
    
    //APV8104
    // Check existence (kFALSE = exists and accessible)
    if (!gSystem->AccessPathName(APV8104::buildFileName(runName, 0))) {
        // Construct only if file exists
        int moduleIndex = digitisers.size();
        if(APV8104::ModuleZeroIndex>=0)moduleIndex=APV8104::ModuleZeroIndex;
        std::cout<<"Added APV8104 as module "<<moduleIndex<<std::endl; 
        digitisers.push_back(std::make_unique<APV8104>(runName,moduleIndex));
    }
    //APV8032
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8032::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            int moduleIndex = digitisers.size();
            if(APV8032::ModuleZeroIndex>=0) moduleIndex = APV8032::ModuleZeroIndex + fmodl - 1; 
            std::cout<<"Added APV8032_"<<fmodl<<" as module "<<moduleIndex<<std::endl;
            digitisers.push_back(std::make_unique<APV8032>(runName,moduleIndex, fmodl));
        }
    }
    //APV8016A
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8016A::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            int moduleIndex = digitisers.size();
            if(APV8016A::ModuleZeroIndex>=0) moduleIndex = APV8016A::ModuleZeroIndex + fmodl - 1;
            std::cout<<"Added APV8016A_"<<fmodl<<" as module "<<moduleIndex<<std::endl;
            digitisers.push_back(std::make_unique<APV8016A>(runName,moduleIndex,fmodl));
        }
    }
    
    return digitisers;
}

DigitiserAdcHistograms BuildDigitiserAdcHistograms(const std::vector<std::unique_ptr<DigitiserBase>>& digitisers)
{
    DigitiserAdcHistograms histSet;
    int maxModule = -1;

    for (const auto& digitiser : digitisers) {
        if (digitiser) {
            maxModule = std::max(maxModule, digitiser->getModule());
        }
    }

    if (maxModule < 0) {
        return histSet;
    }

    histSet.moduleOffsets.assign(maxModule + 1, -1);
    histSet.moduleChannels.assign(maxModule + 1, 0);

    TString hName;
    TString hTitl;

    for (const auto& digitiser : digitisers) {
        if (!digitiser) {
            continue;
        }

        const int module = digitiser->getModule();
        const int channels = digitiser->channels();
        if (module < 0 || channels <= 0) {
            continue;
        }

        histSet.moduleOffsets[module] = histSet.histograms.size();
        histSet.moduleChannels[module] = channels;

        for (int channel = 0; channel < channels; ++channel) {
            hName.Form("hAdc_%d_%02d", module, channel + 1);
            hTitl.Form("histgram of ADC for %s-%d, Ch=%02d",
                           digitiser->name().c_str(), module, channel + 1);

            TH1F* histogram = new TH1F(hName.Data(), hTitl.Data(), 8192, 0.5, 8192.5);
            histSet.histograms.push_back(histogram);
            histSet.ownedByThis.push_back(histogram->GetDirectory() == nullptr);
        }
    }

    return histSet;
}
