R__LOAD_LIBRARY(bin/libJAEASort.so)

#pragma cling add_include_path("include")

#include <Detectors.h>
#include <IO.h>

void CalBuildExample(const char* outputFile = "InitialCalFile.cal")
{
    for (int i = 0; i < 4; i++) {
        DetHit::SetDetType(0, i, DetHit::LaBr);
        DetHit::SetIndex(0, i, i);
    }

    for (int i = 0; i < 30; i++) {
        DetHit::SetDetType(1, i, DetHit::Solar);
        DetHit::SetIndex(1, i, i);
    }

    for (int i = 0; i < 16; i++) {
        DetHit::SetDetType(2, i, DetHit::SiDeltaE);
        DetHit::SetIndex(2, i, i);

        DetHit::SetDetType(3, i, DetHit::SiDeltaE_B);
        DetHit::SetIndex(3, i, 15 - i);

        DetHit::SetDetType(2, i + 16, DetHit::Si);
        DetHit::SetIndex(2, i + 16, i);

        DetHit::SetDetType(3, i + 16, DetHit::Si_B);
        DetHit::SetIndex(3, i + 16, i);
    }

    for (int i = 10; i < 14; i++) {
        DetHit::SetDetType(4, i, DetHit::HPGe);
        DetHit::SetIndex(4, i, i - 10);
    }

    WriteCal(outputFile);
}
