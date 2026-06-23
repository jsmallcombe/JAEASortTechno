#ifndef JAEASortThreadedHistograms
#define JAEASortThreadedHistograms

#include <ThreadedHistogramList.h>

#include <TH1F.h>
#include <TH2F.h>
#include <TH3.h>
#include <TH3F.h>

#include <memory>

#define JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(X) \
    X(TH1F, s3_raw_ring_mult, "s3_raw", "S3 raw ring multiplicity", 11, -0.5, 10.5) \
    X(TH1F, s3_raw_sector_mult, "s3_raw", "S3 raw sector multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_raw_ring_energy, "s3_raw", "S3 raw ring energy;Ring;Ring energy", 24, -0.5, 23.5, 500,0,50) \
    X(TH2F, s3_raw_sector_energy, "s3_raw", "S3 raw sector energy;Sector;Sector energy", 32, -0.5, 31.5, 500,0,50) \
    X(TH2F, s3_raw_ring_sector, "s3_raw", "S3 raw ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH2F, s3_raw_ring_sector_energy, "s3_raw", "S3 raw ring energy vs sector energy;Ring energy;Sector energy", 500, 0, 50, 500, 0, 50) \
    X(TH3I, s3_raw_sector_ring_vs_sector_energy, "s3_raw", "S3 raw sector vs ring vs sector energy;Sector;Ring;Sector energy", 32, -0.5, 31.5,24, -0.5, 23.5, 100, 0, 50) \
    X(TH1F, s3_raw_ring_sector_dt, "s3_raw", "S3 raw ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_raw_ring_dt, "s3_raw", "S3 raw ring vs ring-sector time difference;Ring;Ring - sector time", 24, -0.5, 23.5, 201, -1005, 1005) \
    X(TH2F, s3_raw_sector_dt, "s3_raw", "S3 raw sector vs ring-sector time difference;Sector;Ring - sector time", 32, -0.5, 31.5, 201, -1005, 1005) \
    X(TH1F, s3_pixel_mult, "s3_pixels", "S3 built pixel multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_pixel_ring_sector, "s3_pixels", "S3 built pixel ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH1F, s3_pixel_energy, "s3_pixels", "S3 built pixel primary energy;Energy", 500, 0, 50) \
    X(TH2F, s3_pixel_ring_energy, "s3_pixels", "S3 built pixel ring vs primary energy;Ring;Energy", 24, -0.5, 23.5, 500,0,50) \
    X(TH2F, s3_pixel_sector_energy, "s3_pixels", "S3 built pixel sector vs primary energy;Sector;Energy", 32, -0.5, 31.5, 500,0,50) \
    X(TH2F, s3_pixel_ring_sector_energy, "s3_pixels", "S3 built pixel ring energy vs sector energy;Ring energy;Sector energy", 250,0,50, 250,0,50) \
    X(TH1F, s3_pixel_ring_sector_dt, "s3_pixels", "S3 built pixel ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_pixel_theta_energy, "s3_pixels", "S3 built pixel theta vs energy;Theta [rad];Energy", 180, 0, 3.14159265358979323846, 500,0,50) \
    X(TH2F, s3_pixel_position_xy, "s3_pixels", "S3 built pixel position;X;Y", 200, -50, 50, 200, -50, 50) \
    X(TH1F, s3_pixel_ring_sector_dt_cut, "s3_pixels", "S3 built pixel ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_pixel_ring_sector_energy_cut, "s3_pixels", "S3 built pixel ring energy vs sector energy;Ring energy;Sector energy", 250,0,50, 250,0,50) \
    X(TH2F, s3_pixel_theta_energy_cut, "s3_pixels", "S3 built pixel theta vs energy;Theta [rad];Energy", 180, 0, 3.14159265358979323846, 500,0,50) \
    X(TH2F, s3_pixel_ring_sector_cut, "s3_pixels", "S3 built pixel ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH2F, s3_pixel_position_xy_cut, "s3_pixels", "S3 built pixel position;X;Y", 200, -50, 50, 200, -50, 50) \
    X(TH3F, s3_pixel_position_xyz, "s3_pixels", "S3 built pixel position;Z;X;Y", 120, -60, 60, 120, -60, 60, 120, -60, 60) \
    X(TH1F, cdte_energy, "gammas", "CdTe summed energy;Energy [keV]", 2000, 0, 400) \
    X(TH1F, cdte_energy_S3, "gammas", "CdTe summed energy (S3 gated);Energy [keV]", 2000, 0, 400) \
    X(TH1F, cdte_energy_S3_bg, "gammas", "Background CdTe summed energy (S3 gated);Energy [keV]", 2000, 0, 400) \
    X(TH2F, cdte_chan, "gammas", "CdTe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 2000, 0, 400) \
    X(TH2F, cdte_chan_S3, "gammas", "CdTe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 2000, 0, 400) \
    X(TH2F, cdte_chan_doppler, "gammas", "CdTe Evergy Doppler-corrected vs Channel;Channel;Energy", 16, -0.5, 15.5, 2000, 0, 400) \
    X(TH1F, cdte_doppler, "gammas", "CdTe Doppler-corrected energy;Energy [keV]", 2000, 0, 400) \
    X(TH1F, cdte_doppler_bg, "gammas", "Background CdTe Doppler-corrected energy;Energy [keV]", 2000, 0, 400) \
    X(TH1F, cdte_corrected, "gammas", "Time Corrected CdTe Energy;Energy [keV]", 2000, 0, 400) \
    X(TH1F, cdte_corrected_doppler, "gammas", "Time Corrected CdTe Doppler Energy;Energy [keV]", 2000, 0, 400) \
    X(TH2F, cdte_ring_doppler, "gammas", "S3 ring vs CdTe Doppler-corrected energy;Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 400) \
    X(TH2F, cdte_ring_doppler_back, "gammas", "S3 ring vs CdTe Doppler-corrected energy Background;Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 400) \
    X(TH1F, hpge_energy, "gammas", "HPGe summed energy;Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_energy_S3, "gammas", "HPGe summed energy (S3 gated);Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_energy_S3_bg, "gammas", "Background HPGe summed energy (S3 gated);Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_chan, "gammas", "HPGe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 2000, 0, 2000) \
    X(TH2F, hpge_chan_S3, "gammas", "HPGe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 2000, 0, 2000) \
    X(TH2F, hpge_chan_doppler, "gammas", "HPGe Evergy Doppler-corrected vs Channel;Channel;Energy", 16, -0.5, 15.5, 2000, 0, 2000) \
    X(TH2F, hpge_chan_doppler_beam, "gammas", "HPGe Beam Doppler-corrected vs Channel;Channel;Energy", 16, -0.5, 15.5, 2000, 0, 2000) \
    X(TH1F, hpge_doppler, "gammas", "HPGe Doppler-corrected energy;Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_doppler_bg, "gammas", "Background HPGe Doppler-corrected energy;Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_doppler_Beam, "gammas", "HPGe Beam Doppler-corrected energy (beam);Energy [keV]", 4000, 0, 2000) \
    X(TH1F, hpge_doppler_Beam_bg, "gammas", "Background HPGe Beam Doppler-corrected energy (beam);Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_ring_doppler, "gammas", "S3 ring vs HPGe Doppler-corrected energy;Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 2000) \
    X(TH2F, hpge_ring_doppler_bg, "gammas", "BG S3 ring vs HPGe Doppler-corrected energy;Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 2000) \
    X(TH2F, hpge_ring_doppler_Beam, "gammas", "S3 ring vs HPGe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 2000) \
    X(TH2F, hpge_ring_doppler_Beam_bg, "gammas", "BG S3 ring vs HPGe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 2000) \
    X(TH3F, gamma_positions, "gammas", "Gamma detector hit positions;Z;X;Y", 400, -100, 100, 400, -100, 100, 400, -100, 100) \
    X(TH1F, cdte_S3time, "gammaT", "S3-CdTe time;Time", 400, -2000, 2000) \
    X(TH1F, cdte_S3time_g, "gammaT", "S3-CdTe time;Time", 400, -2000, 2000) \
    X(TH1F, cdte_S3time_w, "gammaT", "S3-CdTe time;Time", 2000, -20000, 20000) \
    X(TH2F, cdte_chan_S3time, "gammaT", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 400, -2000, 2000) \
    X(TH2F, cdte_chan_S3time_w, "gammaT", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 2000, -20000, 20000) \
    X(TH2F, cdte_chan_S3time_gate, "gammaT", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 400, -2000, 2000) \
    X(TH2F, CdTeES3dT, "gammaT", "CdTe energy vs S3 dT", 800,-4000,4000,500,0,250)\
    X(TH2F, CdTeES3dTDopp, "gammaT", "CdTe Doppler energy vs S3 dT", 800,-4000,4000,500,0,250)\
    X(TH2F, CdTeES3dTcorr, "gammaT", "CdTe energy vs S3 dT", 800,-4000,4000,500,0,250)\
    X(TH2F, CdTeES3dTDoppcorr, "gammaT", "CdTe Doppler energy vs S3 dT", 800,-4000,4000,500,0,250)\
    X(TH1F, hpge_S3time, "gammaT", "S3-CdTe time;Time", 400, -2000, 2000) \
    X(TH1F, hpge_S3time_g, "gammaT", "S3-CdTe time;Time", 400, -2000, 2000) \
    X(TH1F, hpge_S3time_w, "gammaT", "S3-CdTe time;Time", 2000, -20000, 20000) \
    X(TH2F, hpge_chan_S3time, "gammaT", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 400, -2000, 2000) \
    X(TH2F, hpge_chan_S3time_w, "gammaT", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 2000, -20000, 20000) \
    X(TH2F, hpge_chan_S3time_gate, "gammaT", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 400, -2000, 2000) \
    X(TH2F, HPGeES3dT, "gammaT", "HPGe energy vs S3 dT", 800,-4000,4000,2000,0,2000)\
    X(TH2F, HPGeES3dTDopp, "gammaT", "HPGe Doppler energy vs S3 dT", 800,-4000,4000,2000,0,2000)\
    X(TH2F, HPGeKin, "kinematics", "HPGe energy vs target/gamma angle;Angle Target-Gamma;Gamma Energy Lab [keV]", 180,0,180,2000,0,2000)\
    X(TH2F, HPGeKinBeam, "kinematics", "HPGe energy vs beam/gamma angle;Angle Beam-Gamma;Gamma Energy Lab Energy [keV]", 180,0,180,2000,0,2000)\
    X(TH2F, HPGeKinDopp, "kinematics", "HPGe doppler energy vs target/gamma angle;Angle Target-Gamma;Gamma Energy Target [keV]", 180,0,180,2000,0,2000)\
    X(TH2F, HPGeKinBeamDopp, "kinematics", "HPGe doppler energy vs beam/gamma angle;Angle Beam-Gamma;Gamma Energy Beam [keV]", 180,0,180,2000,0,2000)\
    X(TH2F, HPGeKinBeamDopp_bg, "kinematics", "BG HPGe doppler energy vs beam/gamma angle;Angle Beam-Gamma;Gamma Energy Beam [keV]", 180,0,180,2000,0,2000)\
    X(TH2F, CdTeKin, "kinematics", "CdTe energy vs target/gamma angle;Angle Target-Gamma;Gamma Energy Lab [keV]", 180,0,180,2000,0,500)\
    X(TH2F, CdTeKinBeam, "kinematics", "CdTe energy vs beam/gamma angle;Angle Beam-Gamma;Gamma Energy Lab Energy [keV]", 180,0,180,2000,0,500)\
    X(TH2F, CdTeKinDopp, "kinematics", "CdTe doppler energy vs target/gamma angle;Angle Target-Gamma;Gamma Energy Target [keV]", 180,0,180,2000,0,500)\
    X(TH2F, CdTeKinBeamDopp, "kinematics", "CdTe doppler energy vs beam/gamma angle;Angle Beam-Gamma;Gamma Energy Beam [keV]", 180,0,180,2000,0,500)\
    X(TH1F, cdte_doppler_beam, "kinematics_beam", "CdTe Doppler-corrected energy (beam);Energy [keV]", 2000, 0, 400) \
    X(TH2F, cdte_ring_doppler_beam, "kinematics_beam", "S3 ring vs CdTe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 400) \
    X(TH1F, hpge_doppler_beam, "kinematics_beam", "HPGe Doppler-corrected energy (beam);Energy [keV]", 4000, 0, 2000) \
    X(TH2F, hpge_ring_doppler_beam, "kinematics_beam", "S3 ring vs HPGe Doppler-corrected energy (beam);Ring;Energy [keV]", 24, -0.5, 23.5, 2000, 0, 2000) \
    X(TH2F, cdte_cdte, "gammagamma", "CdTe-CdTe energy;CdTe energy [keV];CdTe energy [keV]", 2000, 0, 400, 2000, 0, 400) \
    X(TH2F, cdte_cdte_S3, "gammagamma", "CdTe-CdTe energy (S3 gated);CdTe energy [keV];CdTe energy [keV]", 2000, 0, 400, 2000, 0, 400) \
    X(TH2F, cdte_addback_raw, "gammagamma", "CdTe Addback vs raw;CdTe Addback Energy [keV];CdTe raw energy [keV]", 2000, 0, 400, 2000, 0, 400) \
    X(TH2F, cdte_addback_time, "gammagamma", "CdTe Addback; CdTe-CdTe Time;CdTe Addback energy [keV]", 200, -1000, 1000, 2000, 0, 400) \
    X(TH2F, cdte_addback_doppler_raw, "gammagamma", "CdTe Addback Doppler vs raw;CdTe Addback Energy [keV];CdTe raw energy [keV]", 2000, 0, 400, 2000, 0, 400) \
    X(TH2F, cdte_addback_doppler_time, "gammagamma", "CdTe Addback Doppler; CdTe-CdTe Time;CdTe Addback energy [keV]", 200, -1000, 1000, 2000, 0, 400) \
    X(TH2F, hpge_hpge, "gammagamma", "HPGe-HPGe energy;HPGe energy [keV];HPGe energy [keV]", 1000, 0, 1000, 1000, 0, 1000) \
    X(TH2F, cdte_hpge, "gammagamma", "CdTe-HPGe energy;CdTe energy [keV];HPGe energy [keV]", 2000, 0, 400, 1000, 0, 1000) \
    X(TH2F, hpge_hpge_dopp, "gammagamma", "HPGe-HPGe Doppler energy;HPGe energy [keV];HPGe energy [keV]", 1000, 0, 1000, 1000, 0, 1000) \
    X(TH2F, cdte_hpge_dopp, "gammagamma", "CdTe-HPGe Doppler energy;CdTe energy [keV];HPGe energy [keV]", 2000, 0, 400, 1000, 0, 1000) \
    X(TH2F, cdte_hpge_S3, "gammagamma", "CdTe-HPGe energy (S3 gated);CdTe energy [keV];HPGe energy [keV]", 2000, 0, 400, 1000, 0, 1000) \
    X(TH1F, cdte_cdte_dt, "gammagamma", "CdTe-CdTe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_cdte_dt_gate, "gammagamma", "CdTe-CdTe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000) \
    X(TH1F, hpge_hpge_dt, "gammagamma", "HPGe-HPGe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, hpge_hpge_dt_gate, "gammagamma", "HPGe-HPGe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_hpge_dt, "gammagamma", "CdTe-HPGe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, cdte_hpge_dt_gate, "gammagamma", "CdTe-HPGe time difference |#Deltat| < 100;#Deltat", 400, -2000, 2000)\
    X(TH2F, gamgamTHPGe1, "gamTe", "HPGe-HPGe Time vs HPGe Energy 1;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600) \
    X(TH2F, gamgamTHPGe2, "gamTe", "HPGe-HPGe Time vs HPGe Energy 2;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600) \
    X(TH2F, gamgamTHPGe3, "gamTe", "HPGe-HPGe Time vs HPGe Energy 3;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600) \
    X(TH2F, gamgamTHPGe4, "gamTe", "HPGe-HPGe Time vs HPGe Energy 4;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600) \
    X(TH2F, gamgamTHPGe5, "gamTe", "HPGe-HPGe Time vs HPGe Energy 5;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600) \
    X(TH2F, gamgamTHPGe6, "gamTe", "HPGe-HPGe Time vs HPGe Energy 6;Time;Energy [keV]",  1600, -8000, 8000, 400, 0, 1600)

constexpr int kS3RingKinematicsCount = 24;
constexpr int kS3SectorCount = 32;

struct HistogramRefs {
    #define JAEA_DECLARE_DETECTOR_REF(Type, Name, Directory, ...) Type* Name = nullptr;
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_DECLARE_DETECTOR_REF)
    #undef JAEA_DECLARE_DETECTOR_REF

    TH2F* CdTeKinematics[kS3RingKinematicsCount];
    TH2F* HPGeKinematics[kS3RingKinematicsCount];
    TH2F* CdTeKinematicsBeam[kS3RingKinematicsCount];
    TH2F* HPGeKinematicsBeam[kS3RingKinematicsCount];
    TH2F* HPGeS3TimeEnergy[6];
    TH2F* CdTeS3TimeEnergy[16];
    TH2F* Ringi_Sector[kS3RingKinematicsCount];
    TH2F* Ringi_Sector_ch[kS3RingKinematicsCount];
    TH2F* Sectori_Ring[kS3SectorCount];
    TH2F* Sectori_Ring_ch[kS3SectorCount];
};

class ThreadedHistogramSet : public ThreadedHistogramList {
public:
    #define JAEA_DECLARE_DETECTOR_THREADED_HIST(Type, Name, Directory, ...) std::unique_ptr<TThreadedObject<Type>> Name;
    JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_DECLARE_DETECTOR_THREADED_HIST)
    #undef JAEA_DECLARE_DETECTOR_THREADED_HIST

    std::unique_ptr<TThreadedObject<TH2F>> CdTeKinematics[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> HPGeKinematics[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> CdTeKinematicsBeam[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> HPGeKinematicsBeam[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> HPGeS3TimeEnergy[6];
    std::unique_ptr<TThreadedObject<TH2F>> CdTeS3TimeEnergy[16];
    std::unique_ptr<TThreadedObject<TH2F>> Ringi_Sector[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> Ringi_Sector_ch[kS3RingKinematicsCount];
    std::unique_ptr<TThreadedObject<TH2F>> Sectori_Ring[kS3SectorCount];
    std::unique_ptr<TThreadedObject<TH2F>> Sectori_Ring_ch[kS3SectorCount];

    ThreadedHistogramSet()
    {
        #define JAEA_REGISTER_DETECTOR_HIST(Type, Name, Directory, ...) \
            Name.reset(new TThreadedObject<Type>(#Name, __VA_ARGS__)); \
            Register(*Name, Directory);
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_REGISTER_DETECTOR_HIST)
        #undef JAEA_REGISTER_DETECTOR_HIST

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            CdTeKinematics[i].reset(new TThreadedObject<TH2F>(
                Form("CdTe_kinematics_%d", i),
                Form("CdTe opening angle vs energy for S3 ring %d;Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 2000, 0, 400));
            Register(*CdTeKinematics[i], "kinematics/cdte");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            HPGeKinematics[i].reset(new TThreadedObject<TH2F>(
                Form("HPGe_kinematics_%d", i),
                Form("HPGe opening angle vs energy for S3 ring %d;Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 2000, 0, 2000));
            Register(*HPGeKinematics[i], "kinematics/hpge");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            CdTeKinematicsBeam[i].reset(new TThreadedObject<TH2F>(
                Form("CdTe_kinematics_beam_%d", i),
                Form("CdTe opening angle vs energy for S3 ring %d (beam);Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 2000, 0, 400));
            Register(*CdTeKinematicsBeam[i], "kinematics_beam");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            HPGeKinematicsBeam[i].reset(new TThreadedObject<TH2F>(
                Form("HPGe_kinematics_beam_%d", i),
                Form("HPGe opening angle vs energy for S3 ring %d (beam);Opening angle [deg];Energy [keV]", i),
                180, 0, 180, 2000, 0, 2000));
            Register(*HPGeKinematicsBeam[i], "kinematics_beam");
        }

        for (int i = 0; i < 6; ++i) {
            HPGeS3TimeEnergy[i].reset(new TThreadedObject<TH2F>(
                Form("HPGeS3TimeEnergy_%d", i),Form("HPGe-S3 Time vs HPGe Energy %d;Time;Energy [keV]", i),
                                   1600, -8000, 8000, 500, 0, 2000));
            Register(*HPGeS3TimeEnergy[i], "gamT");
        }
        for (int i = 0; i < 16; ++i) {
            CdTeS3TimeEnergy[i].reset(new TThreadedObject<TH2F>(
                Form("CdTeS3TimeEnergy_%d", i),Form("CdTe-S3 Time vs CdTe Energy %d;Time;Energy [keV]", i),
                                                                800, -8000, 8000, 100, 0, 200));
            Register(*CdTeS3TimeEnergy[i], "gamT");
        }

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            Ringi_Sector[i].reset(new TThreadedObject<TH2F>(
                Form("Ring%d_Sector", i),
                  Form("Ring %d vs Sector;Ring %d Charge;Sector Energy", i,i),
                                      200, 0, 50, 200, 0, 50));
            Register(*Ringi_Sector[i], "s3_raw/rings");

            Ringi_Sector_ch[i].reset(new TThreadedObject<TH2F>(
                Form("Ring%d_Sector_ch", i),
                Form("Ring %d vs Sector;Ring %d Energy;Sector Energy", i,i),
                                        600, 0, 6000, 200, 0, 50));
            Register(*Ringi_Sector_ch[i], "s3_raw/rings");


        }

        for (int i = 0; i < kS3SectorCount; ++i) {
            Sectori_Ring[i].reset(new TThreadedObject<TH2F>(
                Form("Sector%d_Ring", i),
                Form("Sector %d vs Ring;Sector %d Energy;Ring Energy", i, i),
                200, 0, 50, 200, 0, 50));
            Register(*Sectori_Ring[i], "s3_raw/sectors");

            Sectori_Ring_ch[i].reset(new TThreadedObject<TH2F>(
                Form("Sector%d_Ring_ch", i),
                Form("Sector %d vs Ring;Sector %d Charge;Ring Energy", i, i),
                600, 0, 6000, 200, 0, 50));
            Register(*Sectori_Ring_ch[i], "s3_raw/sectors");
        }
    }

    HistogramRefs ResolveHistogramRefs()
    {
        HistogramRefs refs;

        #define JAEA_RESOLVE_DETECTOR_REF(Type, Name, Directory, ...) refs.Name = Name->Get().get();
        JAEA_THREADED_DETECTOR_HISTOGRAM_LIST(JAEA_RESOLVE_DETECTOR_REF)
        #undef JAEA_RESOLVE_DETECTOR_REF

        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.CdTeKinematics[i] = CdTeKinematics[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.HPGeKinematics[i] = HPGeKinematics[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.CdTeKinematicsBeam[i] = CdTeKinematicsBeam[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.HPGeKinematicsBeam[i] = HPGeKinematicsBeam[i]->Get().get();
        }
        for (int i = 0; i < 6; ++i) {
            refs.HPGeS3TimeEnergy[i] = HPGeS3TimeEnergy[i]->Get().get();
        }
        for (int i = 0; i < 16; ++i) {
            refs.CdTeS3TimeEnergy[i] = CdTeS3TimeEnergy[i]->Get().get();
        }
        for (int i = 0; i < kS3RingKinematicsCount; ++i) {
            refs.Ringi_Sector[i] = Ringi_Sector[i]->Get().get();
            refs.Ringi_Sector_ch[i] = Ringi_Sector_ch[i]->Get().get();
        }
        for (int i = 0; i < kS3SectorCount; ++i) {
            refs.Sectori_Ring[i] = Sectori_Ring[i]->Get().get();
            refs.Sectori_Ring_ch[i] = Sectori_Ring_ch[i]->Get().get();
        }

        return refs;
    }
};

#endif
