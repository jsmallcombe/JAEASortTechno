#ifndef JAEASortThreadedHistogramList
#define JAEASortThreadedHistogramList

#include <ROOT/TThreadedObject.hxx>
#include <TDirectory.h>
#include <TH1.h>
#include <TString.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

using ROOT::TThreadedObject;

class ThreadedHistogramList {
public:
    template <typename HistT>
    void Register(TThreadedObject<HistT>& histogram, const TString& directory = "")
    {
        static_assert(std::is_base_of<TH1, HistT>::value, "HistT must derive from TH1");
        writeList.push_back(std::bind(&ThreadedHistogramList::MergeAndWrite<HistT>, &histogram));
        dirlist.push_back(directory);
    }

    void WriteAll(TDirectory* outputDirectory = nullptr)
    {
        TDirectory* previousDirectory = gDirectory;
        if (outputDirectory) {
            outputDirectory->cd();
        }

        TDirectory* baseDirectory = outputDirectory ? outputDirectory : gDirectory;

        for (size_t i = 0; i < writeList.size(); ++i) {
            if (baseDirectory && dirlist[i].Length() > 0) {
                TDirectory* subdir = baseDirectory->GetDirectory(dirlist[i]);
                if (!subdir) {
                    baseDirectory->mkdir(dirlist[i]);
                    subdir = baseDirectory->GetDirectory(dirlist[i]);
                }
                subdir->cd();
            } else if (baseDirectory) {
                baseDirectory->cd();
            }

            writeList[i]();
        }

        if (previousDirectory) {
            previousDirectory->cd();
        }
    }

private:
    std::vector<std::function<void()>> writeList;
    std::vector<TString> dirlist;

    template <typename HistT>
    static void MergeAndWrite(TThreadedObject<HistT>* histogram)
    {
        std::unique_ptr<HistT> merged = histogram->SnapshotMerge();
        if (merged) {
            merged->Write();
        }
    }
};

#endif
