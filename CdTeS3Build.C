R__LOAD_LIBRARY(bin/libJAEASort.so)

#pragma cling add_include_path("include")

#include <Detectors.h>
#include <IO.h>

void CdTeS3Build(const char* outputFile = "S3CalFile.cal")
{
    for (int i = 0; i < 32; i++) {
        DetHit::SetDetType(0, i, DetHit::S3Sector);
        DetHit::SetIndex(0, i, i);
        DetHit::SetTOff(0, i, -10);
    }
    DetHit::SetTOff(0, 25, -30);
    DetHit::SetTOff(0, 30, -20);
    DetHit::SetTOff(0, 31, -20);


    for (int i = 0; i < 12; i++) {
        DetHit::SetDetType(1, i, DetHit::S3Ring);
        DetHit::SetIndex(1, i, i+12);
        DetHit::SetCalibrationParam(1, i,0,1.325);
        DetHit::SetTOff(1, i, 0);
        
        DetHit::SetDetType(1, i+16, DetHit::S3Ring);
        DetHit::SetIndex(1, i+16, i);
        DetHit::SetTOff(1, i+16, -10);

    }
    
    for (int i = 0; i < 16; i++) {
        DetHit::SetDetType(2, i, DetHit::CdTe);
        DetHit::SetIndex(2, i, i);
        DetHit::SetTOff(2, i, 600);
        DetHit::SetTOff(3, i, 600);
    }
    
    DetHit::SetDetType(3, 0, DetHit::HPGe);
    DetHit::SetIndex(3, 0, 0);
    
    DetHit::SetDetType(3, 3, DetHit::HPGe);
    DetHit::SetIndex(3, 3, 1);

    WriteCal(outputFile);
}
